/*
 * HostNode.cc
 *
 *  Created on: Oct 16, 2025
 *      Author: joseph
 */

#define FSM_DEBUG
#include "TurtleMobility.h"
#include "Node.h"

class HostNode : public Node, public cListener{

protected:
    Node* canNode;
    Node* anotherCanNode;
    Node* cloudNode;

    bool inRangeOfCan = false;
    bool canAcked = false;
    bool inRangeOfAnotherCan = false;
    bool anotherCanAcked = false;
    cMessage *sendCanTimer = nullptr;
    cMessage *sendAnotherCanTimer = nullptr;

    cXMLElement *root = getEnvir()->getXMLDocument("turtle.xml");

    Mine::TurtleMobility *mobility;

    int sendHostFast = 0;
    int rcvdHostFast = 0;
    int sendHostSlow = 0;
    int rcvdHostSlow = 0;

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

    void handleSendTimer(cMessage *msg, bool &inRage, bool &acked, const char *targetName, int gateIndex, int sendState, int altSendState);
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
        handleSendTimer(msg, inRangeOfCan, canAcked, "Can", 0, FAST_SEND_TO_CAN, SLOW_SEND_TO_CAN);
        return;
    }

    if (msg == sendAnotherCanTimer) {
        handleSendTimer(msg, inRangeOfAnotherCan, anotherCanAcked, "AnotherCan", 1, FAST_SEND_TO_ANOTHER_CAN, SLOW_SEND_TO_ANOTHER_CAN);
        return;
    }

    switch(fsmType){
        case FAST: handleFastMessageTransmissions(msg); handleFastFsmTransitions(msg); break;
        case SLOW: handleSlowMessageTransmissions(msg); handleSlowFsmTransitions(msg); break;
        case EMPTY: handleEmptyMessageTransmissions(msg); handleEmptyFsmTransitions(msg); break;
    }
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

            updateRangeState(nowInRangeCan, inRangeOfCan, sendCanTimer, "Can");
            updateRangeState(nowInRangeAnotherCan, inRangeOfAnotherCan, sendAnotherCanTimer, "AnotherCan");
        }
}

void HostNode::receiveSignal(cComponent *source, simsignal_t signalID, bool value, cObject *details){
    if(signalID == Node::garbageCollectedSignalFromCan){
            EV << "RECEIVED SIGNAL FROM CLOUD; GARBAGE IS COLLECTED; CONTINUYE TO NEXT STATE";
            FSM_Goto(*currentFsm, FAST_SEND_TO_ANOTHER_CAN);
        }

        if(signalID == Node::garbageCollectedSignalFromAnotherCan){
            FSM_Goto(*currentFsm, FAST_EXIT);
        }
}

void HostNode::handleSlowFsmTransitions(cMessage *msg){
    FSM_Switch(*currentFsm){
            case FSM_Enter(SLOW_SEND_TO_CAN):
            {
                break;
            }

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

            case FSM_Enter(SLOW_SEND_TO_ANOTHER_CAN):
            {
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

            case FSM_Enter(SLOW_EXIT):
            {
                EV << "Final slow state reached";
            }
        }

    delete msg;
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
        case FSM_Enter(FAST_EXIT):
        {
            EV << "Final Fast state reached";
        }
    }

    delete msg;
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
        case FSM_Enter(EMPTY_SEND_TO_CAN):
        {
            break;
        }
        case FSM_Enter(EMPTY_SEND_TO_ANOTHER_CAN):
           break;
        case FSM_Enter(EMPTY_EXIT):
        {
            EV << "Final Empty state reached";
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

void HostNode::handleSendTimer(cMessage *msg, bool &inRange, bool &acked, const char *targetName, int gateIndex, int sendState, int altSendState){
    if (inRange && !acked) {
        bool shouldSend = (currentFsm->getState() == sendState || currentFsm->getState() == altSendState);

        if (shouldSend) {
            EV << "Sending periodic message to " << targetName << ".\n";
            cMessage *req = new cMessage((gateIndex == 0) ? "1-Is the can full?" : "4-Is the can full?");
            send(req, "gate$o", gateIndex);
            sendHostFast++;
        }
        updateStatusText();
        scheduleAt(simTime() + 1, msg);
    }
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
        EV << "Entered range of " << name << ". Scheduling first message.\n";
        if (!timer->isScheduled())
            scheduleAt(simTime() + 1, timer);
    }
    else if (!nowInRange && prevInRange) {
        prevInRange = false;
        EV << "Left range of " << name << ". Canceling timer.\n";
        cancelEvent(timer);
    }
}

