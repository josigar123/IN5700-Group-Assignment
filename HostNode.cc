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

    cChannel *slowLink;
    cChannel *fastLink;

    // Variables for start-stop logic
    Coord waypointCan = Coord(290, 300);
    Coord waypointAnotherCan = Coord(290, 990);
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

    enum FsmType { FAST, SLOW, EMPTY };
    FsmType fsmType;

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

    virtual void finish() override;
    double getDelayFromChannel(const char* gateName, int index);
    double delayOn(const char* gateName, int ix);

};

Define_Module(HostNode);

void HostNode::initialize(){

    Node::initialize();

    mobility = check_and_cast<Extended::TurtleMobility*>(getSubmodule("mobility"));
    mobility->subscribe(inet::MobilityBase::mobilityStateChangedSignal, this);

    canNode = check_and_cast<Node*>(getParentModule()->getSubmodule("can"));
    anotherCanNode = check_and_cast<Node*>(getParentModule()->getSubmodule("anotherCan"));
    cloudNode = check_and_cast<Node*>(network->getSubmodule("cloud"));

    slowLink = gate("gate$o", 2)->getChannel();

    if (slowLink != nullptr) {
        // Cast to correct type for convenience

        RealisticDelayChannel *dataCh = check_and_cast<RealisticDelayChannel*>(slowLink);

        EV << "Current delay: " << dataCh->getCurrentDelay() << endl;
        EV << "Current datarate: " << dataCh->getDatarate() << endl;

        // Modify both dynamically
        //dataCh->setDelay(0.001);       // 200 ms
        dataCh->setDatarate(2e6);    // 2 Mbps

        EV << "NEW delay: " << dataCh->getCurrentDelay() << endl;
        EV << "NEW datarate: " << dataCh->getDatarate() << endl;
    }

    // Subscribe to collection signals
    canNode->subscribe(Node::garbageCollectedSignalFromCan, this);
    anotherCanNode->subscribe(Node::garbageCollectedSignalFromAnotherCan, this);

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

    if (mobility) {
        auto pos = mobility->getCurrentPosition();
        statusText->setPosition(cFigure::Point(pos.x - 500, pos.y - 100)); // Above the node
    }

    canvas->addFigure(statusText);
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
    Enter_Method_Silent();

    if (signalID == MobilityBase::mobilityStateChangedSignal) {
            // cast to non-const MobilityBase
            MobilityBase *m = check_and_cast<MobilityBase *>(source);

            if (mobility != nullptr) {
                auto pos = mobility->getCurrentPosition();
                oval->setBounds(cFigure::Rectangle(pos.x - range, pos.y - range, range*2, range*2));

                if (statusText) {
                    statusText->setPosition(cFigure::Point(pos.x - 500, pos.y - 100));
                }
            }

            bool nowInRangeCan = isInRangeOf(canNode);
            bool nowInRangeAnotherCan = isInRangeOf(anotherCanNode);

            atWaypointCan = mobility->getCurrentPosition().distance(waypointCan) <= 1 ? true : false;
            atWaypointAnotherCan = mobility->getCurrentPosition().distance(waypointAnotherCan) <= 1 ? true : false;

            updateRangeState(nowInRangeCan, inRangeOfCan, sendCanTimer, "Can");
            updateRangeState(nowInRangeAnotherCan, inRangeOfAnotherCan, sendAnotherCanTimer, "AnotherCan");
        }
}

void HostNode::receiveSignal(cComponent *source, simsignal_t signalID, bool value, cObject *details){

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
            cMessage *req = new cMessage("7-Collect garbage");
            send(req, "gate$o", 2);
            sendHostSlow++;
            updateStatusText();


            break;
        }
        case FSM_Enter(SLOW_SEND_TO_ANOTHER_CAN_CLOUD):
        {
            cMessage *req = new cMessage("9-Collect garbage");
            send(req, "gate$o", 2);
            sendHostSlow++;
            updateStatusText();


            break;
        }
    }
}

void HostNode::handleSlowMessageTransmissions(cMessage *msg){
    if(strcmp(msg->getName(), "3-Yes") == 0){
        // Received confirmation from Can
        rcvdHostFast++;
        updateStatusText();
        canAcked = true;
        cancelEvent(sendCanTimer);
        FSM_Goto(*currentFsm, SLOW_SEND_TO_CAN_CLOUD);
    }

    if(strcmp(msg->getName(), "8-Ok") == 0){
        rcvdHostSlow++;
        updateStatusText();
        cXMLElement *movementLeg = root->getElementById("2");
        mobility->setLeg(movementLeg);
        FSM_Goto(*currentFsm, SLOW_SEND_TO_ANOTHER_CAN);
    }

    if(strcmp(msg->getName(), "6-Yes") == 0){
        // Received confirmation from AnotherCan
        rcvdHostFast++;
        updateStatusText();
        anotherCanAcked = true;
        cancelEvent(sendAnotherCanTimer);
        FSM_Goto(*currentFsm, SLOW_SEND_TO_ANOTHER_CAN_CLOUD);
    }

    if(strcmp(msg->getName(), "10-Ok") == 0){
        rcvdHostSlow++;
        updateStatusText();
        FSM_Goto(*currentFsm, SLOW_EXIT);
        cXMLElement *movementLeg = root->getElementById("3");
        mobility->setLeg(movementLeg);
    }
}

void HostNode::handleFastMessageTransmissions(cMessage *msg){
    if(strcmp(msg->getName(), "3-Yes") == 0){
        // Received confirmation from Can
        rcvdHostFast++;
        updateStatusText();
        canAcked = true;
        cancelEvent(sendCanTimer);
    }

    if(strcmp(msg->getName(), "6-Yes") == 0){
        // Received confirmation from AnotherCan
        rcvdHostFast++;
        updateStatusText();
        anotherCanAcked = true;
        cancelEvent(sendAnotherCanTimer);
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
    if(strcmp(msg->getName(), "2-No") == 0){
        // Received confirmation from Can
        rcvdHostFast++;
        updateStatusText();
        canAcked = true;
        cancelEvent(sendCanTimer);
        FSM_Goto(*currentFsm, EMPTY_SEND_TO_ANOTHER_CAN);
    }

    if(strcmp(msg->getName(), "5-No") == 0){
        // Received confirmation from AnotherCan
        rcvdHostFast++;
        updateStatusText();
        anotherCanAcked = true;
        cancelEvent(sendAnotherCanTimer);
        FSM_Goto(*currentFsm, EMPTY_EXIT);
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

    bool stateOk = currentFsm &&
        (currentFsm->getState() == sendState || currentFsm->getState() == altSendState);

    if (stateOk && atWp) {
        cMessage *req = new cMessage((gateIndex == 0) ? "1-Is the can full?" : "4-Is the can full?");
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

/*
double HostNode::getDelayFromChannel(const char* gateName, int index) {
    cGate* g = gate(gateName, index);
    if (!g) return 0;
    cChannel* ch = g->getChannel();
    if (!ch) return 0;

    if (auto realisticCh = dynamic_cast<RealisticDelayChannel*>(ch)) {
        return SIMTIME_DBL(realisticCh->getCurrentDelay());  // safe public access
    }
    return 0;
}
*/
// Safe helpers 

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




void HostNode::finish() {
    std::ostringstream out;

    // Which config are we running?
    const char* cfg = getEnvir()->getConfigEx()->getActiveConfigName();

    // Host gates (exactly 3 as per your NED)
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

    // Build per-config output
    if (strcmp(cfg, "GarbageInTheCansAndSlow") == 0) {
        // Cloud-based: host uses slow link to cloud; cans talk fast to host/cloud as needed
        out << "Slow connection from the smartphone to others (time it takes) = " << host_to_cloud << "s\n";
        out << "Slow connection from others to the smartphone (time it takes) = " << host_to_cloud << "s\n";
        out << "Fast connection from the smartphone to others (time it takes) = " << host_fast_to_others << "s\n";
        out << "Fast connection from others to the smartphone (time it takes) = " << others_fast_to_host << "s\n\n";

        out << "Connection from the can to others (time it takes) = " << can_to_cloud << "s\n";
        out << "Connection from others to the can (time it takes) = " << cloud_to_can << "s\n\n";

        out << "Connection from the anotherCan to others (time it takes) = " << another_to_cloud << "s\n";
        out << "Connection from others to the anotherCan (time it takes) = " << cloud_to_another << "s\n\n";

        out << "Slow connection from the Cloud to others (time it takes) = " << cloud_to_host_slow << "s\n";
        out << "Slow connection from others to the Cloud (time it takes) = " << host_to_cloud << "s\n";
        out << "Fast connection from the Cloud to others (time it takes) = " << cloud_fast_to_others << "s\n";
        out << "Fast connection from others to the Cloud (time it takes) = " << others_fast_to_cloud << "s\n";
    }
    else if (strcmp(cfg, "GarbageInTheCansAndFast") == 0) {
        // Fog-based: cans talk directly to cloud (fast); host slow link unused for results
        out << "Slow connection from the smartphone to others (time it takes) = 0s\n";
        out << "Slow connection from others to the smartphone (time it takes) = 0s\n";
        out << "Fast connection from the smartphone to others (time it takes) = " << host_fast_to_others << "s\n";
        out << "Fast connection from others to the smartphone (time it takes) = " << others_fast_to_host << "s\n\n";

        out << "Connection from the can to others (time it takes) = " << can_to_cloud << "s\n";
        out << "Connection from others to the can (time it takes) = " << cloud_to_can << "s\n\n";

        out << "Connection from the anotherCan to others (time it takes) = " << another_to_cloud << "s\n";
        out << "Connection from others to the anotherCan (time it takes) = " << cloud_to_another << "s\n\n";

        out << "Slow connection from the Cloud to others (time it takes) = 0s\n";
        out << "Slow connection from others to the Cloud (time it takes) = 0s\n";
        out << "Fast connection from the Cloud to others (time it takes) = " << cloud_fast_to_others << "s\n";
        out << "Fast connection from others to the Cloud (time it takes) = " << others_fast_to_cloud << "s\n";
    }
    else if (strcmp(cfg, "NoGarbageInTheCans") == 0) {
        // No-garbage: only query/reply between host and cans matters; cloud paths effectively 0
        out << "Slow connection from the smartphone to others (time it takes) = 0s\n";
        out << "Slow connection from others to the smartphone (time it takes) = 0s\n";
        out << "Fast connection from the smartphone to others (time it takes) = " << host_fast_to_others << "s\n";
        out << "Fast connection from others to the smartphone (time it takes) = " << others_fast_to_host << "s\n\n";

        out << "Connection from the can to others (time it takes) = " << can_to_cloud << "s\n";
        out << "Connection from others to the can (time it takes) = " << cloud_to_can << "s\n\n";

        out << "Connection from the anotherCan to others (time it takes) = " << another_to_cloud << "s\n";
        out << "Connection from others to the anotherCan (time it takes) = " << cloud_to_another << "s\n\n";

        out << "Slow connection from the Cloud to others (time it takes) = 0s\n";
        out << "Slow connection from others to the Cloud (time it takes) = 0s\n";
        out << "Fast connection from the Cloud to others (time it takes) = 0s\n";
        out << "Fast connection from others to the Cloud (time it takes) = 0s\n";
    }
    else {
        out << "(Unknown config)\n";
    }

    // Write to log
    EV_INFO << "\n--- Simulation Delay Summary (" << cfg << ") ---\n" << out.str() << "\n";

    if (auto text = dynamic_cast<cTextFigure*>(canvas->getRootFigure()->getFigureByPath("configTextBody"))) {
        text->setText(out.str().c_str());
    }

}



