/*
 * GarbageCollectionSystem.cc
 *
 *  Created on: Oct 31, 2025
 *      Author: joseph
 */

#include "GarbageCollectionSystem.h"
#include "Node.h"
#include "RealisticDelayChannel.h"

Define_Module(GarbageCollectionSystem);

void GarbageCollectionSystem::initialize(){

    // Retrieve the system nodes
    canNode = check_and_cast<Node*>(getSubmodule("can"));
    anotherCanNode = check_and_cast<Node*>(getSubmodule("anotherCan"));
    cloudNode = check_and_cast<Node*>(getSubmodule("cloud"));
    hostNode = check_and_cast<Node*>(getSubmodule("host", 0));

    // get the network canvas
    canvas = getCanvas();

    // retrieve the config name
    configName = getEnvir()->getConfigEx()->getActiveConfigName();

    // ### SETUP APPROPRIATE FSM CONFIG ###
    // Assign fsdType, and initial state to go to
    if(strcmp(configName, "GarbageInTheCansAndSlow") == 0){
        fsmType = SLOW;
        currentFsm = &slowFsm;
        currentFsm->setName("slow");
        FSM_Goto(*currentFsm, SLOW_SEND_TO_CAN);
    }

    if(strcmp(configName, "GarbageInTheCansAndFast") == 0){
        fsmType = FAST;
        currentFsm = &fastFsm;
        currentFsm->setName("fast");
        FSM_Goto(*currentFsm, FAST_SEND_TO_CAN);
    }

    if(strcmp(configName, "NoGarbageInTheCans") == 0){
        fsmType = EMPTY;
        currentFsm = &emptyFsm;
        currentFsm->setName("empty");
        FSM_Goto(*currentFsm, EMPTY_SEND_TO_CAN);
    }

    // Populate gate pointers for runtime calculations
    if (hostNode->hasGate("gate$o", 0))
        fastCellularLink = check_and_cast<RealisticDelayChannel *>(
            hostNode->gate("gate$o", 0)->getChannel());

    if (canNode->hasGate("gate$o", 1))
        fastWiFiLink = check_and_cast<RealisticDelayChannel *>(
            canNode->gate("gate$o", 1)->getChannel());

    if (hostNode->hasGate("gate$o", 2))
        slowCellularLink = check_and_cast<RealisticDelayChannel *>(
            hostNode->gate("gate$o", 2)->getChannel());

    renderInitialDelayStats(); // Empty stats
}

// Enum as an easy index into a predefines array, also created an id field which can be accessed
cMessage *GarbageCollectionSystem::createMessage(MsgID id){
    static const char *names[] = {
        "",                       // 0 unused
        "1-Is the can full?",
        "2-NO",
        "3-YES",
        "4-Is the can full?",
        "5-NO",
        "6-YES",
        "7-Collect garbage",
        "8-OK",
        "9-Collect garbage",
        "10-OK"
    };

    cMessage *msg = new cMessage(names[id]);
    msg->addPar("msgId") = static_cast<int>(id);
    return msg;
}

// Get the id for a message
int GarbageCollectionSystem::getMsgId(cMessage *msg){
    return (int)(msg->hasPar("msgId") ? msg->par("msgId") : 0); // Fallback to 0 for invalid message type
}

// Render empty statistics initially
void GarbageCollectionSystem::renderInitialDelayStats(){
    delayStatsHeader = new cTextFigure("headerDelayStats");
    delayStatsHeader->setFont(cFigure::Font("Arial", FONT_SIZE, omnetpp::cAbstractImageFigure::FONT_BOLD));
    delayStatsHeader->setPosition(cFigure::Point(HORIZONTAL_PLACEMENT, VERTICAL_HEADER_PLACEMENT));

    switch(fsmType){
    case SLOW: delayStatsHeader->setText("Cloud-based solution with slow messages"); break;
    case FAST: delayStatsHeader->setText("Fog-based solution with fast messages"); break;
    case EMPTY: delayStatsHeader->setText("No garbage solution"); break;
    }


    hostDelayStats = makeStatFigure("hostDelayStats", 0);
    canDelayStats = makeStatFigure("canDelayStats", 2);
    anotherCanDelayStats = makeStatFigure("anotherCanDelayStats", 3);
    cloudDelayStats = makeStatFigure("cloudDelayStats", 4);

    hostDelayStats->setText("Slow connection from the smartphone to others (time it takes) = \n"
                            "Slow connection from others to the smartphone (time it takes) = \n"
                            "Fast connection from the smartphone to others (time it takes) = \n"
                            "Fast connection from others to the smartphone (time it takes) = \n");

    canDelayStats->setText("Connection from the can to others (time it takes) = \n"
                           "Connection from others to the can (time it takes) = \n");

    anotherCanDelayStats->setText("Connection from the anotherCan to others (time it takes) = \n"
                                  "Connection from others to the anotherCan (time it takes) = \n");

    cloudDelayStats->setText("Slow connection from the Cloud to others (time it takes) = \n"
                             "Slow connection from others to the Cloud (time it takes) = \n"
                             "Fast connection from the Cloud to others (time it takes) = \n"
                             "Fast connection from others to the Cloud (time it takes) = \n");

    canvas->addFigure(delayStatsHeader);
    canvas->addFigure(hostDelayStats);
    canvas->addFigure(canDelayStats);
    canvas->addFigure(anotherCanDelayStats);
    canvas->addFigure(cloudDelayStats);
}

// A helper for placing text with even spacing and parameters
cTextFigure *GarbageCollectionSystem::makeStatFigure(const char* name, int stepMultiplier){
    auto fig = new cTextFigure(name);
    fig->setFont(cFigure::Font("Arial", FONT_SIZE));
    fig->setPosition(cFigure::Point(HORIZONTAL_PLACEMENT,
        VERTICAL_BODY_PLACEMENT + stepMultiplier * VERTICAL_BODY_STEP_SIZE));
    return fig;
}

// Simply writes to a stream based on which config is active, the final delay values and sets the figure text with final info
void GarbageCollectionSystem::finish(){
    std::ostringstream hostOut, canOut, anotherCanOut, cloudOut;

    switch (fsmType) {
        case SLOW: {
            // Cloud-based: host uses slow link to cloud; cans talk fast to host/cloud as needed
            hostOut << "Slow connection from the smartphone to others (time it takes) = " << GlobalDelays.slow_smartphone_to_others << "\n";
            hostOut << "Slow connection from others to the smartphone (time it takes) = " << GlobalDelays.slow_others_to_smartphone << "\n";
            hostOut << "Fast connection from the smartphone to others (time it takes) = " << GlobalDelays.fast_smartphone_to_others << "\n";

            hostOut << "Fast connection from others to the smartphone (time it takes) = " << GlobalDelays.fast_others_to_smartphone << "\n";
            canOut << "Connection from the can to others (time it takes) = " << GlobalDelays.connection_from_can_to_others << "\n";
            canOut << "Connection from others to the can (time it takes) = " << GlobalDelays.connection_from_others_to_can << "\n\n";

            anotherCanOut << "Connection from the anotherCan to others (time it takes) = " << GlobalDelays.connection_from_another_can_to_others << "\n";
            anotherCanOut << "Connection from others to the anotherCan (time it takes) = " << GlobalDelays.connection_from_others_to_another_can << "\n";

            cloudOut << "Slow connection from the Cloud to others (time it takes) = " << GlobalDelays.slow_cloud_to_others << "\n";
            cloudOut << "Slow connection from others to the Cloud (time it takes) = " << GlobalDelays.slow_others_to_cloud << "\n";
            cloudOut << "Fast connection from the Cloud to others (time it takes) = 0\n";
            cloudOut << "Fast connection from others to the Cloud (time it takes) = 0\n";
            break;
        }

        case FAST: {
            // Fog-based: cans talk directly to cloud (fast); host slow link unused for results
            hostOut << "Slow connection from the smartphone to others (time it takes) = 0\n";
            hostOut << "Slow connection from others to the smartphone (time it takes) = 0\n";
            hostOut << "Fast connection from the smartphone to others (time it takes) = " << GlobalDelays.fast_smartphone_to_others << "\n";
            hostOut << "Fast connection from others to the smartphone (time it takes) = " << GlobalDelays.fast_others_to_smartphone << "\n";

            canOut << "Connection from the can to others (time it takes) = " << GlobalDelays.connection_from_can_to_others << "\n";
            canOut << "Connection from others to the can (time it takes) = " << GlobalDelays.connection_from_others_to_can << "\n";

            anotherCanOut << "Connection from the anotherCan to others (time it takes) = " << GlobalDelays.connection_from_another_can_to_others << "\n";
            anotherCanOut << "Connection from others to the anotherCan (time it takes) = " << GlobalDelays.connection_from_others_to_another_can << "\n\n";

            cloudOut << "Slow connection from the Cloud to others (time it takes) = 0\n";
            cloudOut << "Slow connection from others to the Cloud (time it takes) = 0\n";
            cloudOut << "Fast connection from the Cloud to others (time it takes) = " << GlobalDelays.fast_cloud_to_others << "\n";
            cloudOut << "Fast connection from others to the Cloud (time it takes) = " << GlobalDelays.fast_others_to_cloud << "\n";
            break;
        }

        case EMPTY: {
            // No-garbage: only query/reply between host and cans matters; cloud paths effectively 0
            hostOut << "Slow connection from the smartphone to others (time it takes) = 0\n";
            hostOut << "Slow connection from others to the smartphone (time it takes) = 0\n";
            hostOut << "Fast connection from the smartphone to others (time it takes) = " << GlobalDelays.fast_smartphone_to_others << "\n";
            hostOut << "Fast connection from others to the smartphone (time it takes) = " << GlobalDelays.fast_others_to_smartphone << "\n";

            canOut << "Connection from the can to others (time it takes) = " << GlobalDelays.connection_from_can_to_others << "\n";
            canOut << "Connection from others to the can (time it takes) = " << GlobalDelays.connection_from_others_to_can << "\n";

            anotherCanOut << "Connection from the anotherCan to others (time it takes) = " << GlobalDelays.connection_from_another_can_to_others << "\n";
            anotherCanOut << "Connection from others to the anotherCan (time it takes) = " << GlobalDelays.connection_from_others_to_another_can << "\n";

            cloudOut << "Slow connection from the Cloud to others (time it takes) = 0\n";
            cloudOut << "Slow connection from others to the Cloud (time it takes) = 0\n";
            cloudOut << "Fast connection from the Cloud to others (time it takes) = 0\n";
            cloudOut << "Fast connection from others to the Cloud (time it takes) = 0\n";
            break;
        }
    }

    hostDelayStats->setText(hostOut.str().c_str());
    canDelayStats->setText(canOut.str().c_str());
    anotherCanDelayStats->setText(anotherCanOut.str().c_str());
    cloudDelayStats->setText(cloudOut.str().c_str());
}
