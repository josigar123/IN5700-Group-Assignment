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

    canNode = check_and_cast<Node*>(getSubmodule("can"));
    anotherCanNode = check_and_cast<Node*>(getSubmodule("anotherCan"));
    cloudNode = check_and_cast<Node*>(getSubmodule("cloud"));

    canvas = getCanvas();

    configName = getEnvir()->getConfigEx()->getActiveConfigName();

    // ### SETUP APPROPRIATE FSM CONFIG ###
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
    if (hasGate("gate$o", 0))
            fastCellularLink = check_and_cast<RealisticDelayChannel *>(gate("gate$o", 0)->getChannel());

        if (hasGate("gate$o", 1))
            fastWiFiLink = check_and_cast<RealisticDelayChannel *>(gate("gate$o", 1)->getChannel());
        // HostNode has SlowCellularLink at index 2
        if (hasGate("gate$o", 2))
            slowCellularLink = check_and_cast<RealisticDelayChannel *>(gate("gate$o", 2)->getChannel());

    renderInitialDelayStats();
}

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

int GarbageCollectionSystem::getMsgId(cMessage *msg){
    return (int)(msg->hasPar("msgId") ? msg->par("msgId") : 0); // Fallback to 0 for invalid message type
}

void GarbageCollectionSystem::renderInitialDelayStats(){
    delayStatsHeader = new cTextFigure("headerDelayStats");
    delayStatsHeader->setFont(cFigure::Font("Arial", 30, omnetpp::cAbstractImageFigure::FONT_BOLD));
    delayStatsHeader->setPosition(cFigure::Point(2100, 25));

    switch(fsmType){
    case SLOW: delayStatsHeader->setText("Cloud-based solution with slow messages"); break;
    case FAST: delayStatsHeader->setText("Fog-based solution with fast messages"); break;
    case EMPTY: delayStatsHeader->setText("Fog-based solution with no messages"); break;
    }


    hostDelayStats = new cTextFigure("headerDelayStats");
    hostDelayStats->setFont(cFigure::Font("Arial", 30));
    hostDelayStats->setPosition(cFigure::Point(2100, 150));

    canDelayStats = new cTextFigure("canDelayStats");
    canDelayStats->setFont(cFigure::Font("Arial", 30));
    canDelayStats->setPosition(cFigure::Point(2100, 350));

    anotherCanDelayStats = new cTextFigure("anotherCanDelayStats");
    anotherCanDelayStats->setFont(cFigure::Font("Arial", 30));
    anotherCanDelayStats->setPosition(cFigure::Point(2100, 450));

    cloudDelayStats = new cTextFigure("cloudDelayStats");
    cloudDelayStats->setFont(cFigure::Font("Arial", 30));
    cloudDelayStats->setPosition(cFigure::Point(2100, 550));

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

void GarbageCollectionSystem::finish(){
    std::ostringstream hostOut, canOut, anotherCanOut, cloudOut;

    switch (fsmType) {
        case SLOW: {
            // Cloud-based: host uses slow link to cloud; cans talk fast to host/cloud as needed
            hostOut << "Slow connection from the smartphone to others (time it takes) = " << GlobalDelays.slow_smartphone_to_others << "\n"; // Sum both sends to cloud from host
            hostOut << "Slow connection from others to the smartphone (time it takes) = " << GlobalDelays.slow_others_to_smartphone << "\n"; // Sum both sends from cloud to host
            hostOut << "Fast connection from the smartphone to others (time it takes) = " << GlobalDelays.fast_smartphone_to_others << "\n"; // Sum both sends from host to cans (dropped messages as well)
            hostOut << "Fast connection from others to the smartphone (time it takes) = " << GlobalDelays.fast_others_to_smartphone << "\n"; // Sum messages from cans to host Yes/NO

            canOut << "Connection from the can to others (time it takes) = " << GlobalDelays.connection_from_can_to_others << "\n"; // Sum messages from cans to host Yes/NO
            canOut << "Connection from others to the can (time it takes) = " << GlobalDelays.connection_from_others_to_can << "\n\n"; // Sum messages from host to can

            anotherCanOut << "Connection from the anotherCan to others (time it takes) = " << GlobalDelays.connection_from_another_can_to_others << "\n"; // Sum messages from anotherCan to host
            anotherCanOut << "Connection from others to the anotherCan (time it takes) = " << GlobalDelays.connection_from_others_to_another_can << "\n"; // Sum messages from host to anotherCan

            cloudOut << "Slow connection from the Cloud to others (time it takes) = " << GlobalDelays.slow_cloud_to_others << "\n"; // Sum sends from cloud to host
            cloudOut << "Slow connection from others to the Cloud (time it takes) = " << GlobalDelays.slow_others_to_cloud << "\n"; // Sum sends from host to cloud
            cloudOut << "Fast connection from the Cloud to others (time it takes) = 0\n";
            cloudOut << "Fast connection from others to the Cloud (time it takes) = 0\n";
            break;
        }

        case FAST: {
            // Fog-based: cans talk directly to cloud (fast); host slow link unused for results
            hostOut << "Slow connection from the smartphone to others (time it takes) = 0\n";
            hostOut << "Slow connection from others to the smartphone (time it takes) = 0\n";
            hostOut << "Fast connection from the smartphone to others (time it takes) = " << GlobalDelays.fast_smartphone_to_others << "\n"; // Sum both sends from host to cans (dropped messages as well)
            hostOut << "Fast connection from others to the smartphone (time it takes) = " << GlobalDelays.fast_others_to_smartphone << "\n"; // Sum messages from cans to host Yes/NO

            canOut << "Connection from the can to others (time it takes) = " << GlobalDelays.connection_from_can_to_others << "\n"; // Sum sent from can to host and cloud
            canOut << "Connection from others to the can (time it takes) = " << GlobalDelays.connection_from_others_to_can << "\n"; // Sum rcv from host and cloud to can

            anotherCanOut << "Connection from the anotherCan to others (time it takes) = " << GlobalDelays.connection_from_another_can_to_others << "\n"; // Sum sent from can to host and cloud
            anotherCanOut << "Connection from others to the anotherCan (time it takes) = " << GlobalDelays.connection_from_others_to_another_can << "\n"; // Sum rcv from host and cloud to can

            cloudOut << "Slow connection from the Cloud to others (time it takes) = 0\n";
            cloudOut << "Slow connection from others to the Cloud (time it takes) = 0\n";
            cloudOut << "Fast connection from the Cloud to others (time it takes) = " << GlobalDelays.fast_cloud_to_others << "\n"; // Sum sends from Cloud to both cans
            cloudOut << "Fast connection from others to the Cloud (time it takes) = " << GlobalDelays.fast_others_to_cloud << "\n"; // Sum sends from cans to cloud
            break;
        }

        case EMPTY: {
            // No-garbage: only query/reply between host and cans matters; cloud paths effectively 0
            hostOut << "Slow connection from the smartphone to others (time it takes) = 0\n";
            hostOut << "Slow connection from others to the smartphone (time it takes) = 0\n";
            hostOut << "Fast connection from the smartphone to others (time it takes) = " << GlobalDelays.fast_smartphone_to_others << "\n"; // Sum both sends from host to cans (dropped messages as well)
            hostOut << "Fast connection from others to the smartphone (time it takes) = " << GlobalDelays.fast_others_to_smartphone << "\n"; // Sum messages from cans to host Yes/NO

            canOut << "Connection from the can to others (time it takes) = " << GlobalDelays.connection_from_can_to_others << "\n"; // Sum messages from cans to host Yes/NO
            canOut << "Connection from others to the can (time it takes) = " << GlobalDelays.connection_from_others_to_can << "\n"; // Sum messages from host to can

            anotherCanOut << "Connection from the anotherCan to others (time it takes) = " << GlobalDelays.connection_from_another_can_to_others << "\n"; // Sum messages from anotherCan to host
            anotherCanOut << "Connection from others to the anotherCan (time it takes) = " << GlobalDelays.connection_from_others_to_another_can << "\n"; // Sum messages from host to anotherCan

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
