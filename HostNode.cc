/*
 * HostNode.cc
 *
 *  Created on: Oct 16, 2025
 *      Author: joseph
 */
#include "TurtleMobility.h"
#include "Node.h"
#include "inet/mobility/base/MobilityBase.h"
#include "RealisticDelayChannel.h"
#include <sstream>




class HostNode : public Node, public cListener{

protected:
    // Pointers to the nodes system
    Node* canNode;
    Node* anotherCanNode;
    Node* cloudNode;

    // Member for modifying comm links
    cChannel *slowLink;
    cChannel *fastLink;

    // Text figures for holding the delay statistics to be displayed at the end
    cTextFigure *delayStatsHeader = nullptr;
    cTextFigure *hostDelayStats = nullptr;
    cTextFigure *canDelayStats = nullptr;
    cTextFigure *anotherCanDelayStats = nullptr;
    cTextFigure *cloudDelayStats = nullptr;

    // Variables for start-stop logic
    Coord waypointCan = Coord(290, 300); // Waypoint found by visual analysis
    Coord waypointAnotherCan = Coord(290, 990); // Waypoint found by visual analysis
    bool atWaypointCan = false;
    bool atWaypointAnotherCan = false;

    // Range checks and acks for recvd messages
    bool inRangeOfCan = false;
    bool canAcked = false;
    bool inRangeOfAnotherCan = false;
    bool anotherCanAcked = false;
    // Resend timers
    cMessage *sendCanTimer = nullptr;
    cMessage *sendAnotherCanTimer = nullptr;

    // Turtle wrapper for mobility control
    Extended::TurtleMobility *mobility;
    cXMLElement *root = getEnvir()->getXMLDocument("turtle.xml"); // Contains the legs for the turtle to complete

    // Stat counters
    int sendHostFast = 0;
    int rcvdHostFast = 0;
    int sendHostSlow = 0;
    int rcvdHostSlow = 0;
    // Text containing stats
    cTextFigure *statusText = nullptr;

    // Easy lookup of config, no need to str compare
    enum FsmType { FAST, SLOW, EMPTY };
    FsmType fsmType;

    // Defines the three state machines, one for each config at compile tieme, and a pointer which is given a value at runtime
    // pointing to the appropriate state machine
    cFSM *currentFsm; // For storing the appropriate FSM
    cFSM fastFsm;
    enum{
        FAST_SEND_TO_CAN = FSM_Steady(1),
        FAST_SEND_TO_ANOTHER_CAN = FSM_Steady(2),
        FAST_EXIT = FSM_Steady(3),
    };
    cFSM slowFsm;
    enum{
       SLOW_SEND_TO_CAN = FSM_Steady(1),
       SLOW_SEND_TO_CAN_CLOUD = FSM_Steady(2),
       SLOW_SEND_TO_ANOTHER_CAN = FSM_Steady(3),
       SLOW_SEND_TO_ANOTHER_CAN_CLOUD = FSM_Steady(4),
       SLOW_EXIT = FSM_Steady(5),
    };
    cFSM emptyFsm;
    enum{
        EMPTY_SEND_TO_CAN = FSM_Steady(1),
        EMPTY_SEND_TO_ANOTHER_CAN = FSM_Steady(2),
        EMPTY_EXIT = FSM_Steady(3),
    };
    

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, bool value, cObject *details) override;
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) override;

    virtual bool isInRangeOf(Node *target);
    void handleSlowFsmTransitions(cMessage *msg);
    void handleEmptyFsmTransitions(cMessage *msg);

    void handleSlowMessageTransmissions(cMessage *msg);
    void handleFastMessageTransmissions(cMessage *msg);
    void handleEmptyMessageTransmissions(cMessage *msg);

    void handleSendTimer(cMessage *msg, bool &inRage, bool &acked, bool &atWp, const char *targetName, int gateIndex, int sendState, int altSendState);
    void updateRangeState(bool nowInRange, bool &prevInRange, cMessage *timer, const char *name);

    void updateStatusText();
    void renderInitialDelayStats();

    virtual void finish() override;
    double getDelayFromChannel(const char* gateName, int index);
    double delayOn(const char* gateName, int ix);

};

Define_Module(HostNode);

void HostNode::initialize(){

    // Init general fields in Node.h
    Node::initialize();

    // Subscribe to the signal for mobilitystatechanged
    mobility = check_and_cast<Extended::TurtleMobility*>(getSubmodule("mobility"));
    mobility->subscribe(inet::MobilityBase::mobilityStateChangedSignal, this);

    // Fetch the system nodes for access
    canNode = check_and_cast<Node*>(getParentModule()->getSubmodule("can"));
    anotherCanNode = check_and_cast<Node*>(getParentModule()->getSubmodule("anotherCan"));
    cloudNode = check_and_cast<Node*>(network->getSubmodule("cloud"));

    // Subscribe to collection signals
    canNode->subscribe(Node::garbageCollectedSignalFromCan, this);
    anotherCanNode->subscribe(Node::garbageCollectedSignalFromAnotherCan, this);

    // Create self messages for retries while waiting for ACK from cans
    sendCanTimer = new cMessage("sendCanTimer");
    sendAnotherCanTimer = new cMessage("sendAnotherCanTimer");

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

    // ### SETUP STATUS TEXT ###
    statusText = new cTextFigure("hostStatus");
    statusText->setColor(cFigure::BLUE);
    statusText->setFont(cFigure::Font("Arial", 36));
    updateStatusText();

    // Place initial status text
    if (mobility) {
        auto pos = mobility->getCurrentPosition();
        statusText->setPosition(cFigure::Point(pos.x - 500, pos.y - 100)); // Above the node
    }

    canvas->addFigure(statusText);

    // Renders status info based on config
    renderInitialDelayStats();
}

void HostNode::handleMessage(cMessage *msg){

    if (msg == sendCanTimer) {
        handleSendTimer(msg, inRangeOfCan, canAcked, atWaypointCan, "Can", 0, FAST_SEND_TO_CAN, SLOW_SEND_TO_CAN);
        return;
    }

    if (msg == sendAnotherCanTimer) {
        handleSendTimer(msg, inRangeOfAnotherCan, anotherCanAcked, atWaypointAnotherCan, "AnotherCan", 1, FAST_SEND_TO_ANOTHER_CAN, SLOW_SEND_TO_ANOTHER_CAN);
        return;
    }

    switch(fsmType){
        case FAST: handleFastMessageTransmissions(msg); break;
        case SLOW: handleSlowMessageTransmissions(msg); handleSlowFsmTransitions(msg); break;
        case EMPTY: handleEmptyMessageTransmissions(msg); handleEmptyFsmTransitions(msg); break;
    }

    delete msg;
}

bool HostNode::isInRangeOf(Node* target) {
    // Simple method for checking if coverage circles are overlapping a given target, only used on Can and AnotherCan since Cloud is all encompassing

    // Host must have mobility, otherwise no need to check
    if (!mobility) return false;

    auto posHost = mobility->getCurrentPosition();

    // Get target position
    double xOther, yOther, rangeOther;

    xOther = target->par("x");
    yOther = target->par("y");

    rangeOther = target->par("range");

    double dx = posHost.x - xOther;
    double dy = posHost.y - yOther;
    double distance = std::sqrt(dx*dx + dy*dy);

    // circles overlap if distance <= sum of ranges
    return distance <= (this->range + rangeOther);
}

void HostNode::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details){
    Enter_Method_Silent(); // Needed to work correctly, compiler suggestion

    // If we have changed position (mobility signals a state)
    //  - Update coverage circles
    //  - Check if we are in range of a can
    //  - Check if we are at a waypoint
    //  - Manage send timers by checking if we have been in range and are now no longer in range (we have driven past)
    if (signalID == MobilityBase::mobilityStateChangedSignal) {
            // cast to non-const MobilityBase
            MobilityBase *m = check_and_cast<MobilityBase *>(source);

            // Update coverage circle placement
            if (mobility != nullptr) {
                auto pos = mobility->getCurrentPosition();
                oval->setBounds(cFigure::Rectangle(pos.x - range, pos.y - range, range*2, range*2));

                if (statusText) {
                    statusText->setPosition(cFigure::Point(pos.x - 500, pos.y - 100));
                }
            }

            bool nowInRangeCan = isInRangeOf(canNode);
            bool nowInRangeAnotherCan = isInRangeOf(anotherCanNode);

            // Check that host is at waypoint with some margin
            atWaypointCan = mobility->getCurrentPosition().distance(waypointCan) <= 1 ? true : false;
            atWaypointAnotherCan = mobility->getCurrentPosition().distance(waypointAnotherCan) <= 1 ? true : false;

            updateRangeState(nowInRangeCan, inRangeOfCan, sendCanTimer, "Can");
            updateRangeState(nowInRangeAnotherCan, inRangeOfAnotherCan, sendAnotherCanTimer, "AnotherCan");
        }
}

void HostNode::receiveSignal(cComponent *source, simsignal_t signalID, bool value, cObject *details){

    // Load appropriate leg for next movement based on the signal received, this method is only in use in the FAST config, that is why you see leg loading
    // in association with slow and empty concerning methods.
    if(signalID == Node::garbageCollectedSignalFromCan){
            cXMLElement *movementLeg = root->getElementById("2");
            mobility->setLeg(movementLeg);
            FSM_Goto(*currentFsm, FAST_SEND_TO_ANOTHER_CAN);
        }

    if(signalID == Node::garbageCollectedSignalFromAnotherCan){
        cXMLElement *movementLeg = root->getElementById("3");
        mobility->setLeg(movementLeg);
        FSM_Goto(*currentFsm, FAST_EXIT);
    }
}

void HostNode::handleSlowFsmTransitions(cMessage *msg){
    FSM_Switch(*currentFsm){
        case FSM_Enter(SLOW_SEND_TO_CAN_CLOUD):
        {
            cMessage *req = Node::createMessage(MSG_7_COLLECT_GARBAGE);
            send(req, "gate$o", 2);
            sendHostSlow++;
            updateStatusText();
            break;
        }
        case FSM_Enter(SLOW_SEND_TO_ANOTHER_CAN_CLOUD):
        {
            cMessage *req = Node::createMessage(MSG_9_COLLECT_GARBAGE);
            send(req, "gate$o", 2);
            sendHostSlow++;
            updateStatusText();
            break;
        }
    }
}

void HostNode::handleSlowMessageTransmissions(cMessage *msg){

    int msgId = Node::getMsgId(msg);

    switch(msgId){
        case MSG_3_YES:
        {
            // Received confirmation from Can
            rcvdHostFast++;
            updateStatusText();
            canAcked = true;
            cancelEvent(sendCanTimer);
            FSM_Goto(*currentFsm, SLOW_SEND_TO_CAN_CLOUD);
            break;
        }

        case MSG_8_OK:
        {
            // Received confirmation from cloud
            rcvdHostSlow++;
            updateStatusText();
            cXMLElement *movementLeg = root->getElementById("2");
            mobility->setLeg(movementLeg);
            FSM_Goto(*currentFsm, SLOW_SEND_TO_ANOTHER_CAN);
            break;
        }

        case MSG_6_YES:
        {
            // Received confirmation from AnotherCan
            rcvdHostFast++;
            updateStatusText();
            anotherCanAcked = true;
            cancelEvent(sendAnotherCanTimer);
            FSM_Goto(*currentFsm, SLOW_SEND_TO_ANOTHER_CAN_CLOUD);
            break;
        }

        case MSG_10_OK:
        {
            // Received confirmation from cloud
            rcvdHostSlow++;
            updateStatusText();
            FSM_Goto(*currentFsm, SLOW_EXIT);
            cXMLElement *movementLeg = root->getElementById("3");
            mobility->setLeg(movementLeg);
            break;

        }
    }

}

void HostNode::handleFastMessageTransmissions(cMessage *msg){

    int msgId = Node::getMsgId(msg);

    switch(msgId){
    case MSG_3_YES:
        {
            // Received confirmation from Can
            rcvdHostFast++;
            updateStatusText();
            canAcked = true;
            cancelEvent(sendCanTimer);
            break;
        }
    case MSG_6_YES:
        {
            // Received confirmation from AnotherCan
            rcvdHostFast++;
            updateStatusText();
            anotherCanAcked = true;
            cancelEvent(sendAnotherCanTimer);
            break;
        }
    }
}

void HostNode::handleEmptyFsmTransitions(cMessage *msg){
    FSM_Switch(*currentFsm){
        case FSM_Enter(EMPTY_SEND_TO_ANOTHER_CAN):
        {
           cXMLElement *movementLeg = root->getElementById("2");
           mobility->setLeg(movementLeg);
           break;
        }
        case FSM_Enter(EMPTY_EXIT):
        {
            cXMLElement *movementLeg = root->getElementById("3");
            mobility->setLeg(movementLeg);
            break;
        }
    }
}

void HostNode::handleEmptyMessageTransmissions(cMessage *msg){

    int msgId = Node::getMsgId(msg);
    switch(msgId){
        case MSG_2_NO:
            {
                rcvdHostFast++;
                updateStatusText();
                canAcked = true;
                cancelEvent(sendCanTimer);
                FSM_Goto(*currentFsm, EMPTY_SEND_TO_ANOTHER_CAN);
                break;
        }
        case MSG_5_NO:
            {
                // Received confirmation from AnotherCan
                rcvdHostFast++;
                updateStatusText();
                anotherCanAcked = true;
                cancelEvent(sendAnotherCanTimer);
                FSM_Goto(*currentFsm, EMPTY_EXIT);
                break;
         }
    }
}

void HostNode::handleSendTimer(cMessage *msg,
                               bool &inRange, bool &acked, bool &atWp,
                               const char *targetName, int gateIndex,
                               int sendState, int altSendState)
{
    // If we’re done or out of range, just stop; do NOT reschedule.
    if (acked || !inRange)
        return;

    // Are we in a state where a send should be done?
    bool stateOk = currentFsm &&
        (currentFsm->getState() == sendState || currentFsm->getState() == altSendState);

    // Are we in a sendable state, and is the waypoint reached?
    if (stateOk && atWp) {
        // gateIndex == 0 means we are sending to can, else (1) we send to  anotherCan
        cMessage *req = (gateIndex == 0) ? Node::createMessage(MSG_1_IS_CAN_FULL) : Node::createMessage(MSG_4_IS_CAN_FULL);
        send(req, "gate$o", gateIndex);
        sendHostFast++;
        updateStatusText();
    }

    // Re-arm regardless of atWp so we don't drop the timer while approaching the waypoint
    scheduleAt(simTime() + 1, msg);
}

void HostNode::updateStatusText() {
    char buf[200];
    sprintf(buf, "sentHostFast: %d rcvdHostFast: %d sentHostSlow: %d rcvdHostSlow: %d",
            sendHostFast, rcvdHostFast, sendHostSlow, rcvdHostSlow);
    if (statusText) {
        statusText->setText(buf);
    }
}

void HostNode::updateRangeState(bool nowInRange, bool &prevInRange, cMessage *timer, const char *name){
    if (nowInRange && !prevInRange) {
        prevInRange = true;
        oval->setLineColor(cFigure::GREEN);
        if (!timer->isScheduled())
            scheduleAt(simTime() + 1, timer);
    }
    else if (!nowInRange && prevInRange) {
        prevInRange = false;
        cancelEvent(timer);
    }
}

static double channelDelay(cGate *g) {
    if (!g) return 0;
    cChannel *ch = g->getChannel();
    if (!ch) return 0;
    if (auto realistic = dynamic_cast<RealisticDelayChannel*>(ch))
        return SIMTIME_DBL(realistic->getCurrentDelay());  // uses your dynamic delay
    return 0;
}

double HostNode::delayOn(const char* gateName, int ix) {
    // host-only safe accessor
    if (ix < 0 || ix >= gateSize(gateName)) return 0;
    return channelDelay(gate(gateName, ix));
}

static double delayFrom(cModule *m, const char* gateName, int ix) {
    if (!m) return 0;
    cGate *g = m->gate(gateName, ix);
    return channelDelay(g);
}

void HostNode::renderInitialDelayStats(){

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

void HostNode::finish() {
    std::ostringstream hostOut;
    std::ostringstream canOut;
    std::ostringstream anotherCanOut;
    std::ostringstream cloudOut;

    // Host gates (exactly 3 as per NED)
    // 0: host <-> can     (FastLink)
    // 1: host <-> anotherCan (FastLink)
    // 2: host <-> cloud   (SlowLink)

    // Read delays safely
    double host_to_can      = delayOn("gate$o", 0);
    double host_to_another  = delayOn("gate$o", 1);
    double host_to_cloud    = delayOn("gate$o", 2);

    // Can/anotherCan to Cloud (from those modules)
    // can.numGates = 2; can.gate[1] <-> FastLink <-> cloud.gate[1]
    // anotherCan.numGates = 2; anotherCan.gate[1] <-> FastLink <-> cloud.gate[2]
    double can_to_cloud         = delayFrom(canNode, "gate$o", 1);
    double another_to_cloud     = delayFrom(anotherCanNode, "gate$o", 1);
    double cloud_to_can         = delayFrom(cloudNode, "gate$o", 1);
    double cloud_to_another     = delayFrom(cloudNode, "gate$o", 2);
    double cloud_to_host_slow   = delayFrom(cloudNode, "gate$o", 0); // slow link back to host

    // Convenience aggregates (when a single line should represent two fast links)
    auto max2 = [](double a, double b){ return (a>b)?a:b; };
    double host_fast_to_others   = max2(host_to_can, host_to_another);
    double others_fast_to_host   = host_fast_to_others; // symmetric channel delay
    double cloud_fast_to_others  = max2(cloud_to_can, cloud_to_another);
    double others_fast_to_cloud  = max2(can_to_cloud, another_to_cloud);

    switch(fsmType){
        case SLOW:
        {

            // Cloud-based: host uses slow link to cloud; cans talk fast to host/cloud as needed
            hostOut << "Slow connection from the smartphone to others (time it takes) = " << host_to_cloud << "\n";
            hostOut << "Slow connection from others to the smartphone (time it takes) = " << host_to_cloud << "\n";
            hostOut << "Fast connection from the smartphone to others (time it takes) = " << host_fast_to_others << "\n";
            hostOut << "Fast connection from others to the smartphone (time it takes) = " << others_fast_to_host << "\n";

            canOut << "Connection from the can to others (time it takes) = " << can_to_cloud << "\n";
            canOut << "Connection from others to the can (time it takes) = " << cloud_to_can << "\n\n";

            anotherCanOut << "Connection from the anotherCan to others (time it takes) = " << another_to_cloud << "\n";
            anotherCanOut << "Connection from others to the anotherCan (time it takes) = " << cloud_to_another << "\n";

            cloudOut << "Slow connection from the Cloud to others (time it takes) = " << cloud_to_host_slow << "\n";
            cloudOut << "Slow connection from others to the Cloud (time it takes) = " << host_to_cloud << "\n";
            cloudOut << "Fast connection from the Cloud to others (time it takes) = 0\n";
            cloudOut << "Fast connection from others to the Cloud (time it takes) = 0\n";
            break;
        }
        case FAST:
        {
            // Fog-based: cans talk directly to cloud (fast); host slow link unused for results
            hostOut << "Slow connection from the smartphone to others (time it takes) = 0\n";
            hostOut << "Slow connection from others to the smartphone (time it takes) = 0\n";
            hostOut << "Fast connection from the smartphone to others (time it takes) = " << host_fast_to_others << "\n";
            hostOut << "Fast connection from others to the smartphone (time it takes) = " << others_fast_to_host << "\n";

            canOut << "Connection from the can to others (time it takes) = " << can_to_cloud << "\n";
            canOut << "Connection from others to the can (time it takes) = " << cloud_to_can << "\n";

            anotherCanOut << "Connection from the anotherCan to others (time it takes) = " << another_to_cloud << "\n";
            anotherCanOut << "Connection from others to the anotherCan (time it takes) = " << cloud_to_another << "\n";

            cloudOut << "Slow connection from the Cloud to others (time it takes) = 0\n";
            cloudOut << "Slow connection from others to the Cloud (time it takes) = 0\n";
            cloudOut << "Fast connection from the Cloud to others (time it takes) = " << cloud_fast_to_others << "\n";
            cloudOut << "Fast connection from others to the Cloud (time it takes) = " << others_fast_to_cloud << "\n";
            break;
        }
        case EMPTY:
        {
            // No-garbage: only query/reply between host and cans matters; cloud paths effectively 0
            hostOut << "Slow connection from the smartphone to others (time it takes) = 0\n";
            hostOut << "Slow connection from others to the smartphone (time it takes) = 0\n";
            hostOut << "Fast connection from the smartphone to others (time it takes) = " << host_fast_to_others << "\n";
            hostOut << "Fast connection from others to the smartphone (time it takes) = " << others_fast_to_host << "\n";

            canOut << "Connection from the can to others (time it takes) = " << can_to_cloud << "\n";
            canOut << "Connection from others to the can (time it takes) = " << cloud_to_can << "\n";

            anotherCanOut << "Connection from the anotherCan to others (time it takes) = " << another_to_cloud << "\n";
            anotherCanOut << "Connection from others to the anotherCan (time it takes) = " << cloud_to_another << "\n";

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



