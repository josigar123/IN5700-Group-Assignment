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

    cTextFigure *statusText = nullptr;

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;

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
            if(dropCount < dropLimit){
                bubble("Lost message");
                dropCount++;
                numberOfLostAnotherCanMsgs++;
                updateStatusText();
                break; // Break early to reduce nesting by one level
            }


            // Create message based on config
            cMessage *resp;
            switch(system->fsmType){
                case GarbageCollectionSystem::EMPTY: resp = system->createMessage(MSG_5_NO); break;
                default: resp = system->createMessage(MSG_6_YES); break;
            }


            // Calculate delay and add to global structure, send and update local stats
            simtime_t delay = system->fastCellularLink->computeDynamicDelay(this, system->hostNode);
            GlobalDelays.fast_others_to_smartphone += delay.dbl();
            GlobalDelays.connection_from_another_can_to_others += delay.dbl();

            send(resp, "gate$o", 0);
            sendAnotherCanFast++;
            rcvdAnotherCanFast++;
            updateStatusText();

            switch(system->fsmType)
            {
                case GarbageCollectionSystem::FAST:
                {
                    cMessage *cloudMsg = system->createMessage(MSG_9_COLLECT_GARBAGE);
                    send(cloudMsg, "gate$o", 1);
                    sendAnotherCanFast++;
                    updateStatusText();

                    simtime_t delay = system->fastWiFiLink->computeDynamicDelay(this, system->cloudNode);
                    GlobalDelays.connection_from_another_can_to_others += delay.dbl();
                    GlobalDelays.fast_others_to_cloud += delay.dbl();
                    break;
                }
            }
            break;
        }
    }
    delete msg;
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
