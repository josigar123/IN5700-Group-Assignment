/*
 * HostNode.cc
 *
 *  Created on: Oct 16, 2025
 *      Author: joseph
 */
#include "TurtleMobility.h"
#include "Node.h"
#include "DelayUtils.h"
#include "inet/mobility/base/MobilityBase.h"
#include "DelayUtils.h"
#include <sstream>

class HostNode : public Node, public cListener{

protected:

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
    void ackReceived(bool &ackedFlag, cMessage *timer, int nextState, int &rcvdCounter);
};

Define_Module(HostNode);

void HostNode::initialize(){

    // Init general fields in Node.h
    Node::initialize();

    // Subscribe to the signal for mobilitystatechanged
    mobility = check_and_cast<Extended::TurtleMobility*>(getSubmodule("mobility"));
    mobility->subscribe(inet::MobilityBase::mobilityStateChangedSignal, this);

    // Subscribe to collection signals
    system->canNode->subscribe(Node::garbageCollectedSignalFromCan, this);
    system->anotherCanNode->subscribe(Node::garbageCollectedSignalFromAnotherCan, this);

    // Create self messages for retries while waiting for ACK from cans
    sendCanTimer = new cMessage("sendCanTimer");
    sendAnotherCanTimer = new cMessage("sendAnotherCanTimer");


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

    system->canvas->addFigure(statusText);
}

void HostNode::handleMessage(cMessage *msg){

    if (msg == sendCanTimer) {
        handleSendTimer(msg, inRangeOfCan, canAcked, atWaypointCan, "Can", 0, GarbageCollectionSystem::FAST_SEND_TO_CAN, GarbageCollectionSystem::SLOW_SEND_TO_CAN);
        return;
    }

    if (msg == sendAnotherCanTimer) {
        handleSendTimer(msg, inRangeOfAnotherCan, anotherCanAcked, atWaypointAnotherCan, "AnotherCan", 1, GarbageCollectionSystem::FAST_SEND_TO_ANOTHER_CAN, GarbageCollectionSystem::SLOW_SEND_TO_ANOTHER_CAN);
        return;
    }

    switch(system->fsmType){
        case GarbageCollectionSystem::FAST: handleFastMessageTransmissions(msg); break;
        case GarbageCollectionSystem::SLOW: handleSlowMessageTransmissions(msg); handleSlowFsmTransitions(msg); break;
        case GarbageCollectionSystem::EMPTY: handleEmptyMessageTransmissions(msg); handleEmptyFsmTransitions(msg); break;
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

                // Update coords for module
                x = pos.x;
                y = pos.y;
            }

            bool nowInRangeCan = isInRangeOf(system->canNode);
            bool nowInRangeAnotherCan = isInRangeOf(system->anotherCanNode);

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
            FSM_Goto(*system->currentFsm, system->FAST_SEND_TO_ANOTHER_CAN);
        }

    if(signalID == Node::garbageCollectedSignalFromAnotherCan){
        cXMLElement *movementLeg = root->getElementById("3");
        mobility->setLeg(movementLeg);
        FSM_Goto(*system->currentFsm, system->FAST_EXIT);
    }
}

void HostNode::handleSlowFsmTransitions(cMessage *msg){
    FSM_Switch(*system->currentFsm){
        case FSM_Enter(GarbageCollectionSystem::SLOW_SEND_TO_CAN_CLOUD):
        {
            cMessage *req = system->createMessage(MSG_7_COLLECT_GARBAGE);

            simtime_t delay = system->slowCellularLink->computeDynamicDelay(this, system->cloudNode);
            GlobalDelays.slow_smartphone_to_others += delay.dbl();
            GlobalDelays.slow_others_to_cloud += delay.dbl();

            send(req, "gate$o", 2);
            sendHostSlow++;
            updateStatusText();
            break;
        }
        case FSM_Enter(GarbageCollectionSystem::SLOW_SEND_TO_ANOTHER_CAN_CLOUD):
        {
            cMessage *req = system->createMessage(MSG_9_COLLECT_GARBAGE);

            simtime_t delay = system->slowCellularLink->computeDynamicDelay(this, system->cloudNode);
            GlobalDelays.slow_smartphone_to_others += delay.dbl();
            GlobalDelays.slow_others_to_cloud += delay.dbl();

            send(req, "gate$o", 2);
            sendHostSlow++;
            updateStatusText();
            break;
        }
    }
}

void HostNode::handleSlowMessageTransmissions(cMessage *msg){

    int msgId = system->getMsgId(msg);

    switch(msgId){
        case MSG_3_YES: ackReceived(canAcked, sendCanTimer, system->SLOW_SEND_TO_CAN_CLOUD, rcvdHostFast); break;
        case MSG_6_YES: ackReceived(anotherCanAcked, sendAnotherCanTimer, system->SLOW_SEND_TO_ANOTHER_CAN_CLOUD, rcvdHostFast); break;

        case MSG_8_OK:
        {
            // Received confirmation from cloud
            rcvdHostSlow++;
            updateStatusText();
            cXMLElement *movementLeg = root->getElementById("2");
            mobility->setLeg(movementLeg);
            FSM_Goto(*system->currentFsm, GarbageCollectionSystem::SLOW_SEND_TO_ANOTHER_CAN);
            break;
        }

        case MSG_10_OK:
        {
            // Received confirmation from cloud
            rcvdHostSlow++;
            updateStatusText();
            FSM_Goto(*system->currentFsm, GarbageCollectionSystem::SLOW_EXIT);
            cXMLElement *movementLeg = root->getElementById("3");
            mobility->setLeg(movementLeg);
            break;

        }
    }

}

void HostNode::handleFastMessageTransmissions(cMessage *msg){

    int msgId = system->getMsgId(msg);

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
    FSM_Switch(*system->currentFsm){
        case FSM_Enter(GarbageCollectionSystem::EMPTY_SEND_TO_ANOTHER_CAN):
        {
           cXMLElement *movementLeg = root->getElementById("2");
           mobility->setLeg(movementLeg);
           break;
        }
        case FSM_Enter(GarbageCollectionSystem::EMPTY_EXIT):
        {
            cXMLElement *movementLeg = root->getElementById("3");
            mobility->setLeg(movementLeg);
            break;
        }
    }
}

void HostNode::handleEmptyMessageTransmissions(cMessage *msg){

    int msgId = system->getMsgId(msg);
    switch(msgId){
        case MSG_2_NO: ackReceived(canAcked, sendCanTimer, GarbageCollectionSystem::EMPTY_SEND_TO_ANOTHER_CAN, rcvdHostFast); break;
        case MSG_5_NO: ackReceived(anotherCanAcked, sendAnotherCanTimer, GarbageCollectionSystem::EMPTY_EXIT, rcvdHostFast); break;
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
    bool stateOk = system->currentFsm &&
        (system->currentFsm->getState() == sendState || system->currentFsm->getState() == altSendState);

    // Are we in a sendable state, and is the waypoint reached?
    if (stateOk && atWp) {
        // gateIndex == 0 means we are sending to can, else (1) we send to  anotherCan
        cMessage *req = (gateIndex == 0) ? system->createMessage(MSG_1_IS_CAN_FULL) : system->createMessage(MSG_4_IS_CAN_FULL);

        Node *nodeToCalculateDelayFor = gateIndex == 0 ? system->canNode : system->anotherCanNode;
        simtime_t delay = system->fastCellularLink->computeDynamicDelay(this, nodeToCalculateDelayFor);
        GlobalDelays.fast_smartphone_to_others += delay.dbl();
        GlobalDelays.connection_from_others_to_can += delay.dbl();
        GlobalDelays.connection_from_others_to_another_can += delay.dbl();

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

void HostNode::ackReceived(bool &ackedFlag, cMessage *timer, int nextState, int &rcvdCounter){
    rcvdCounter++;
    updateStatusText();
    ackedFlag = true;
    cancelEvent(timer);
    FSM_Goto(*system->currentFsm, nextState);
}
