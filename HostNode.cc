/*
 * HostNode.cc
 *
 *  Created on: Oct 16, 2025
 *      Author: joseph
 */
#include "TurtleMobility.h"
#include "Node.h"

class HostNode : public Node, public cListener{

protected:
    // Pointers to the nodes system
    Node* canNode;
    Node* anotherCanNode;
    Node* cloudNode;

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
    Mine::TurtleMobility *mobility;
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
    void handleFastFsmTransitions(cMessage *msg);
    void handleEmptyFsmTransitions(cMessage *msg);

    void handleSlowMessageTransmissions(cMessage *msg);
    void handleFastMessageTransmissions(cMessage *msg);
    void handleEmptyMessageTransmissions(cMessage *msg);

    void handleSendTimer(cMessage *msg, bool &inRage, bool &acked, bool &atWp, const char *targetName, int gateIndex, int sendState, int altSendState);
    void updateRangeState(bool nowInRange, bool &prevInRange, cMessage *timer, const char *name);

    void updateStatusText();
};

Define_Module(HostNode);

void HostNode::initialize(){

    Node::initialize();

    mobility = check_and_cast<Mine::TurtleMobility*>(getSubmodule("mobility"));
    mobility->subscribe(inet::MobilityBase::mobilityStateChangedSignal, this);

    canNode = check_and_cast<Node*>(getParentModule()->getSubmodule("can"));
    anotherCanNode = check_and_cast<Node*>(getParentModule()->getSubmodule("anotherCan"));
    cloudNode = check_and_cast<Node*>(network->getSubmodule("cloud"));

    // Subscribe to collection signals
    cloudNode->subscribe(Node::garbageCollectedSignalFromCan, this);
    cloudNode->subscribe(Node::garbageCollectedSignalFromAnotherCan, this);

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
        case FAST: handleFastMessageTransmissions(msg); handleFastFsmTransitions(msg); break;
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
            FSM_Goto(*currentFsm, FAST_SEND_TO_ANOTHER_CAN);
        }

        if(signalID == Node::garbageCollectedSignalFromAnotherCan){
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

            cXMLElement *movementLeg = root->getElementById("2");
            mobility->setLeg(movementLeg);
            break;
        }
        case FSM_Enter(SLOW_SEND_TO_ANOTHER_CAN_CLOUD):
        {
            cMessage *req = new cMessage("9-Collect garbage");
            send(req, "gate$o", 2);
            sendHostSlow++;
            updateStatusText();

            cXMLElement *movementLeg = root->getElementById("3");
            mobility->setLeg(movementLeg);
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
    }
}

void HostNode::handleFastFsmTransitions(cMessage *msg){
    FSM_Switch(*currentFsm){
        case FSM_Enter(FAST_SEND_TO_ANOTHER_CAN): {
            cXMLElement *movementLeg = root->getElementById("2");
            mobility->setLeg(movementLeg);
            break;

        }
        case FSM_Enter(FAST_EXIT):
        {
            cXMLElement *movementLeg = root->getElementById("3");
            mobility->setLeg(movementLeg);
            break;
        }
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

