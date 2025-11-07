/*
 * CloudNode.cc
 *
 *  Created on: Oct 16, 2025
 *      Author: joseph
 */

#include "Node.h"

class CloudNode : public Node {

protected:
    // MSg stats variables
    int sentCloudFast = 0;
    int rcvdCloudFast = 0;
    int sentCloudSlow = 0;
    int rcvdCloudSlow = 0;

    // TExt for displaying stats
    cTextFigure *statusText = nullptr;

    // Named gates
    enum GateIndex { GATE_HOST = 0, GATE_CAN = 1, GATE_ANOTHER_CAN = 2 };

protected:
    // Base omnet overrides
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;

    // method for updating status text
    void updateStatusText();

    void processCollectRequest(MsgID reqId, MsgID respId, Node* targetNode, int gateIndexFast);
};

Define_Module(CloudNode);

void CloudNode::initialize(){
    Node::initialize(); // Init baseline from super

    // ### SETUP STATUS TEXT ONLY IF THERE IS GARBAGE IN THE CANS ###
    if(system->fsmType != GarbageCollectionSystem::EMPTY){
        // set the text parameters
        statusText = new cTextFigure("cloudStatus");
        statusText->setColor(cFigure::BLUE);
        statusText->setFont(cFigure::Font("Arial", 36));
        updateStatusText();

        statusText->setPosition(cFigure::Point(x - 500, y - 130)); // Above the node

        system->canvas->addFigure(statusText);
    }
}

void CloudNode::handleMessage(cMessage *msg){

    int msgId = system->getMsgId(msg);

    switch (msgId) {
        case MSG_7_COLLECT_GARBAGE:
            processCollectRequest(MSG_7_COLLECT_GARBAGE, MSG_8_OK, system->canNode, GATE_CAN);
            break;
        case MSG_9_COLLECT_GARBAGE:
            processCollectRequest(MSG_9_COLLECT_GARBAGE, MSG_10_OK, system->anotherCanNode, GATE_ANOTHER_CAN);
            break;
    }

    delete msg;
}

void CloudNode::processCollectRequest(MsgID reqId, MsgID respId, Node* targetNode, int gateIndexFast){
    cMessage *resp = system->createMessage(respId);

    switch(system->fsmType) {
        case GarbageCollectionSystem::FAST: {
            send(resp, "gate$o", gateIndexFast);
            sentCloudFast++;
            rcvdCloudFast++;
            updateStatusText();

            simtime_t delay = system->fastWiFiLink->computeDynamicDelay(this, targetNode);
            GlobalDelays.fast_cloud_to_others += delay.dbl();
            if (targetNode == system->canNode)
                GlobalDelays.connection_from_others_to_can += delay.dbl();
            else
                GlobalDelays.connection_from_others_to_another_can += delay.dbl();
            break;
        }
        case GarbageCollectionSystem::SLOW:
        case GarbageCollectionSystem::EMPTY: {
            if (statusText != nullptr) {
                sentCloudSlow++;
                rcvdCloudSlow++;
                updateStatusText();

                simtime_t hostDelay = system->slowCellularLink->computeDynamicDelay(this, system->hostNode);
                GlobalDelays.slow_others_to_smartphone += hostDelay.dbl();
                GlobalDelays.slow_cloud_to_others += hostDelay.dbl();
            }

            send(resp, "gate$o", GATE_HOST);
            break;
        }
    }
}

// Util for rendering text
void CloudNode::updateStatusText() {
    char buf[200];
    sprintf(buf, "sentCloudFast: %d rcvdCloudFast: %d sentCloudSlow: %d rcvdCloudSlow: %d",
            sentCloudFast, rcvdCloudFast, sentCloudSlow, rcvdCloudSlow);
    if (statusText) {
        statusText->setText(buf);
    }
}

