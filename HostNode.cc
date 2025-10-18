/*
 * HostNode.cc
 *
 *  Created on: Oct 16, 2025
 *      Author: joseph
 */

#define FSM_DEBUG
#include "inet/mobility/base/MobilityBase.h"
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
    MobilityBase *mobility;
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
        EMPTY_SEND_TO_ANOTHERCAN = FSM_Steady(2),
        EMPTY_EXIT = FSM_Steady(3),
    };
    int stateIterator;

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, bool value, cObject *details) override;
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) override;

    virtual void subscribeToMobilityStateChangedSignal();
    virtual bool isInRangeOf(Node *target);
    void handleSlowFsmTransitions(cMessage *msg);
    void handleFastFsmTransitions(cMessage *msg);
    //void handleEmptyFsmTransitions(cMessage *msg);

    void handleSlowMessageTransmissions(cMessage *msg);
    void handleFastMessageTransmissions(cMessage *msg);
    //void handleEmptyMessageTransmissions(cMessage *msg);
};

Define_Module(HostNode);

void HostNode::initialize(){

    Node::initialize();
    subscribeToMobilityStateChangedSignal();

    canNode = check_and_cast<Node*>(getParentModule()->getSubmodule("can"));
    anotherCanNode = check_and_cast<Node*>(getParentModule()->getSubmodule("anotherCan"));
    cloudNode = check_and_cast<Node*>(network->getSubmodule("cloud"));

    // Subscribe to collection signals
    cloudNode->subscribe(Node::garbageCollectedSignalFromCan, this);
    cloudNode->subscribe(Node::garbageCollectedSignalFromAnotherCan, this);

    sendCanTimer = new cMessage("sendCanTimer");
    sendAnotherCanTimer = new cMessage("sendAnotherCanTimer");

    stateIterator = 0;
    WATCH(stateIterator);

    // Point to the appropriate fsm based upon input
    if(strcmp(configName, "GarbageInTheCansAndSlow") == 0){
        currentFsm = &slowFsm;
        currentFsm->setName("slow");
        FSM_Goto(*currentFsm, SLOW_SEND_TO_CAN);
    }

    if(strcmp(configName, "GarbageInTheCansAndFast") == 0){
        currentFsm = &fastFsm;
        currentFsm->setName("fast");
        FSM_Goto(*currentFsm, FAST_SEND_TO_CAN);
    }

    if(strcmp(configName, "NoGarbageInTheCans") == 0){
        currentFsm = &emptyFsm;
        currentFsm->setName("empty");
        FSM_Goto(*currentFsm, EMPTY_SEND_TO_CAN);
    }
}

void HostNode::handleMessage(cMessage *msg){

    if (msg == sendCanTimer) {
            if (inRangeOfCan && !canAcked) {
                EV << "Sending periodic message to Can.\n";
                cMessage *req = new cMessage("1-Is the can full?");
                send(req, "gate$o", 0);

                scheduleAt(simTime() + 1, sendCanTimer);
            }
            return;
        }

    if (msg == sendAnotherCanTimer) {
            if (inRangeOfAnotherCan && !anotherCanAcked) {
                EV << "Sending periodic message to AnotherCan.\n";
                cMessage *req = new cMessage("4-Is the can full?");
                send(req, "gate$o", 1);

                scheduleAt(simTime() + 1, sendAnotherCanTimer);
            }
            return;
        }

        if(strcmp(configName, "GarbageInTheCansAndSlow") == 0) {handleSlowMessageTransmissions(msg); handleSlowFsmTransitions(msg);}

        if(strcmp(configName, "GarbageInTheCansAndFast") == 0) {handleFastMessageTransmissions(msg); handleFastFsmTransitions(msg);}

        //if(strcmp(configName, "NoGarbageInTheCans") == 0) {handleEmptyMessageTransmissions(msg); handleEmptyFsmTransitions(msg);}
}

void HostNode::subscribeToMobilityStateChangedSignal(){
    cModule *mobMod = getSubmodule("mobility");
    if (mobMod != nullptr) {
        mobility = check_and_cast<inet::MobilityBase *>(mobMod);
        mobility->subscribe(inet::MobilityBase::mobilityStateChangedSignal, this);
    } else {
        mobility = nullptr; // node is static
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
            }

            bool nowInRangeCan = isInRangeOf(canNode);
            bool nowInRangeAnotherCan = isInRangeOf(anotherCanNode);

            if (nowInRangeCan && !inRangeOfCan) {
                // Just entered range
                inRangeOfCan = true;
                oval->setLineColor(cFigure::GREEN);
                EV << "Entered range of Can. Scheduling first message.\n";

                if (!sendCanTimer->isScheduled())
                    scheduleAt(simTime() + 1, sendCanTimer); // first send in 1s
            }
            else if (!nowInRangeCan && inRangeOfCan) {
                // Just left range
                inRangeOfCan = false;
                EV << "Left range of Can. Canceling timer.\n";
                cancelEvent(sendCanTimer);
            }

            if (nowInRangeAnotherCan && !inRangeOfAnotherCan) {
                // Just entered range
                inRangeOfAnotherCan = true;
                oval->setLineColor(cFigure::GREEN);
                EV << "Entered range of anotherCan. Scheduling first message.\n";

                if (!sendCanTimer->isScheduled())
                    scheduleAt(simTime() + 1, sendAnotherCanTimer); // first send in 1s
            }
            else if (!nowInRangeAnotherCan && inRangeOfAnotherCan) {
                // Just left range
                inRangeOfAnotherCan = false;
                EV << "Left range of AnotherCan. Canceling timer.\n";
                cancelEvent(sendAnotherCanTimer);
            }

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
                // Send msg again if no ack
                if(strcmp(msg->getName(), "3-Yes") != 0 && inRangeOfCan){
                    cMessage *req = new cMessage("1-Is the can full?");
                    send(req, "gate$o", 0);
                }
                break;
            }

            case FSM_Enter(SLOW_SEND_TO_CAN_CLOUD):
            {
                    cMessage *req = new cMessage("7-Collect garbage");
                    send(req, "gate$o", 2);
                    break;
            }

            case FSM_Enter(SLOW_SEND_TO_ANOTHER_CAN):
            {
                if(strcmp(msg->getName(), "6-Yes") != 0 && inRangeOfAnotherCan){
                    cMessage *req = new cMessage("4-Is the can full?");
                    send(req, "gate$o", 1);
                }
                break;
            }
            case FSM_Enter(SLOW_SEND_TO_ANOTHER_CAN_CLOUD):
            {
                cMessage *req = new cMessage("9-Collect garbage");
                send(req, "gate$o", 2);
                break;
            }

            case FSM_Enter(SLOW_EXIT):
            {
                endSimulation();
            }
        }

    delete msg;
}

void HostNode::handleSlowMessageTransmissions(cMessage *msg){
    if(strcmp(msg->getName(), "3-Yes") == 0){
                // Received confirmation from Can
                canAcked = true;
                cancelEvent(sendCanTimer);
                FSM_Goto(*currentFsm, SLOW_SEND_TO_CAN_CLOUD);
            }

    if(strcmp(msg->getName(), "8-Ok") == 0){
        FSM_Goto(*currentFsm, SLOW_SEND_TO_ANOTHER_CAN);
    }

    if(strcmp(msg->getName(), "6-Yes") == 0){
        // Received confirmation from AnotherCan
        anotherCanAcked = true;
        cancelEvent(sendAnotherCanTimer);
        FSM_Goto(*currentFsm, SLOW_SEND_TO_ANOTHER_CAN_CLOUD);
    }

    if(strcmp(msg->getName(), "10-Ok") == 0){
        FSM_Goto(*currentFsm, SLOW_EXIT);
    }
}

void HostNode::handleFastFsmTransitions(cMessage *msg){
    FSM_Switch(*currentFsm){
        case FSM_Enter(FAST_SEND_TO_CAN):
        {
            // Send msg again if no ack
            if(strcmp(msg->getName(), "3-Yes") != 0 && inRangeOfCan){
                cMessage *req = new cMessage("1-Is the can full?");
                send(req, "gate$o", 0);
            }
            break;
        }
        case FSM_Enter(FAST_SEND_TO_ANOTHER_CAN):
        {
            // Send msg again if no ack
           if(strcmp(msg->getName(), "6-Yes") != 0 && inRangeOfAnotherCan){
               cMessage *req = new cMessage("4-Is the can full?");
               send(req, "gate$o", 1);
           }
           break;
        }

        case FSM_Enter(FAST_EXIT):
        {
            endSimulation();
        }

    }

    delete msg;
}


void HostNode::handleFastMessageTransmissions(cMessage *msg){
    if(strcmp(msg->getName(), "3-Yes") == 0){
        // Received confirmation from Can
        canAcked = true;
        cancelEvent(sendCanTimer);
    }

    if(strcmp(msg->getName(), "6-Yes") == 0){
        // Received confirmation from AnotherCan
        anotherCanAcked = true;
        cancelEvent(sendAnotherCanTimer);
    }
}


