/*
 * CanNode.cc
 *
 *  Created on: Oct 16, 2025
 *      Author: joseph
 */

#include "Node.h"

class CanNode : public Node {

protected:
    // Drop vars
    int dropCount = 0;
    int dropLimit = 3;

    // Statistic messages
    int sendCanFast = 0;
    int rcvdCanFast = 0;
    int numberOfLostCanMsgs = 0;

    // Figure to render stats text
    cTextFigure *statusText = nullptr;

protected:
    // Builting omnet overrides
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;


    void updateStatusText();
};

Define_Module(CanNode);

void CanNode::initialize(){
    Node::initialize(); // Init baseline from Super

    // ### SETUP STATUS TEXT ###
    statusText = new cTextFigure("canStatus");
    statusText->setColor(cFigure::BLUE);
    statusText->setFont(cFigure::Font("Arial", 36));
    updateStatusText();

    statusText->setPosition(cFigure::Point(x - 500, y - 100)); // Above the node

    system->canvas->addFigure(statusText);
}

void CanNode::handleMessage(cMessage *msg){

    int msgId = system->getMsgId(msg);

    switch(msgId){
        // Triggered for fast config, emit signal that comm is done
        case MSG_8_OK:
        {
            rcvdCanFast++;
            updateStatusText();
            emit(Node::garbageCollectedSignalFromCan, true);
            break;
        }
        // Mesasge from host
        case MSG_1_IS_CAN_FULL:
        {
            // Check if we should drop or process message
            if(dropCount < dropLimit){
                bubble("Lost message");
                dropCount++;
                numberOfLostCanMsgs++;
                updateStatusText();
                break;
            }

            // Create message based on config
            cMessage *resp = nullptr;
            switch(system->fsmType){
                case GarbageCollectionSystem::EMPTY: resp = system->createMessage(MSG_2_NO); break;
                default: resp = system->createMessage(MSG_3_YES); break;
            }


            // Calculate sending delay
            simtime_t delay = system->fastCellularLink->computeDynamicDelay(this, system->hostNode);
            GlobalDelays.fast_others_to_smartphone += delay.dbl();
            GlobalDelays.connection_from_can_to_others += delay.dbl();

            // Send and update status texts
            send(resp, "gate$o", 0);
            sendCanFast++;
            rcvdCanFast++;
            updateStatusText();

            // Send message simultaneously to cloud if we have the fast config
            cMessage *cloudMsg = nullptr;
            switch(system->fsmType){
                case GarbageCollectionSystem::FAST:
                    {
                        // Send message and update local and global stats
                        cloudMsg = system->createMessage(MSG_7_COLLECT_GARBAGE);
                        send(cloudMsg, "gate$o", 1);
                        sendCanFast++;
                        updateStatusText();

                        simtime_t delay = system->fastWiFiLink->computeDynamicDelay(this, system->cloudNode);
                        GlobalDelays.fast_others_to_cloud += delay.dbl();
                        GlobalDelays.connection_from_can_to_others += delay.dbl();
                        break;
                    }
                default: break;
            }
            break;
        }
    }

    delete msg;
}

// Util for text render
void CanNode::updateStatusText() {
    char buf[200];
    sprintf(buf, "sentCanFast: %d rcvdCanFast: %d numberOfLostCanMsgs: %d",
            sendCanFast, rcvdCanFast, numberOfLostCanMsgs);
    if (statusText) {
        statusText->setText(buf);
    }
}
