/*
 * AnotherCanNode.cc
 *
 *  Created on: Oct 16, 2025
 *      Author: joseph
 */

#include "Node.h"

class AnotherCanNode : public Node {

protected:
    int dropCount = 0;
    int dropLimit = 3;

    int sendAnotherCanFast = 0;
    int rcvdAnotherCanFast = 0;
    int numberOfLostAnotherCanMsgs = 0;

    enum GateIndex {GATE_HOST = 0, GATE_CLOUD = 1};

    cTextFigure *statusText = nullptr;

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;

    bool shouldDropMessage();

    void updateStatusText();
};

Define_Module(AnotherCanNode);

void AnotherCanNode::initialize(){
    Node::initialize();

    // ### SETUP STATUS TEXT ###
    statusText = new cTextFigure("anotherCanStatus");
    statusText->setColor(cFigure::BLUE);
    statusText->setFont(cFigure::Font("Arial", 36));
    updateStatusText();

    statusText->setPosition(cFigure::Point(x - 500, y - 100)); // Above the node

    system->canvas->addFigure(statusText);
}

void AnotherCanNode::handleMessage(cMessage *msg){

    int msgId = system->getMsgId(msg);

    switch(msgId){
        // Triggered in fast config, emit signal that comm with cloud is complete so host knows to move
        case MSG_10_OK:
        {
            rcvdAnotherCanFast++;
            updateStatusText();
            emit(Node::garbageCollectedSignalFromAnotherCan, true);
            break;
        }
        case MSG_4_IS_CAN_FULL:
        {
            // Drop or process message
            if(shouldDropMessage()) break;

            // Create message based on config
            cMessage *resp = system->fsmType == GarbageCollectionSystem::EMPTY ? system->createMessage(MSG_5_NO) : system->createMessage(MSG_6_YES);

            // Calculate delay and add to global structure, send and update local stats
            simtime_t hostDelay = system->fastCellularLink->computeDynamicDelay(this, system->hostNode);
            GlobalDelays.fast_others_to_smartphone += hostDelay.dbl();
            GlobalDelays.connection_from_another_can_to_others += hostDelay.dbl();

            send(resp, "gate$o", GATE_HOST);
            sendAnotherCanFast++;
            rcvdAnotherCanFast++;
            updateStatusText();

            if(system->fsmType == GarbageCollectionSystem::FAST)
            {
                cMessage *cloudMsg = system->createMessage(MSG_9_COLLECT_GARBAGE);
                send(cloudMsg, "gate$o", GATE_CLOUD);
                sendAnotherCanFast++;
                updateStatusText();

                simtime_t cloudDelay = system->fastWiFiLink->computeDynamicDelay(this, system->cloudNode);
                GlobalDelays.connection_from_another_can_to_others += cloudDelay.dbl();
                GlobalDelays.fast_others_to_cloud += cloudDelay.dbl();
            }

            break;
        }
    }
    delete msg;
}

bool AnotherCanNode::shouldDropMessage(){
    if (dropCount < dropLimit) {
        bubble("Lost message");
        dropCount++;
        numberOfLostAnotherCanMsgs++;
        updateStatusText();
        return true;
    }
    return false;
}

// Util for text rendering
void AnotherCanNode::updateStatusText() {
    char buf[200];
    sprintf(buf, "sentAnotherCanFast: %d rcvdAnotherCanFast: %d numberOfLostAnotherCanMsgs: %d",
            sendAnotherCanFast, rcvdAnotherCanFast, numberOfLostAnotherCanMsgs);
    if (statusText) {
        statusText->setText(buf);
    }
}
