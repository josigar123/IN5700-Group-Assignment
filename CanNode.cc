/*
 * CanNode.cc
 *
 *  Created on: Oct 16, 2025
 *      Author: joseph
 */

#include "Node.h"

class CanNode : public Node {

protected:
    int dropCount = 0;
    int dropLimit = 3;

    int sendCanFast = 0;
    int rcvdCanFast = 0;
    int numberOfLostCanMsgs = 0;

    cTextFigure *statusText = nullptr;

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;

    void updateStatusText();
};

Define_Module(CanNode);

void CanNode::initialize(){
    Node::initialize();

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
        case MSG_8_OK:
        {
            rcvdCanFast++;
            updateStatusText();
            emit(Node::garbageCollectedSignalFromCan, true);
            break;
        }
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

            cMessage *resp = nullptr;
            if(strcmp(system->configName, "NoGarbageInTheCans") == 0){
                resp = system->createMessage(MSG_2_NO);
            }else{
                resp = system->createMessage(MSG_3_YES);
            }

            simtime_t delay = system->fastCellularLink->computeDynamicDelay(this, system->hostNode);
            GlobalDelays.fast_others_to_smartphone += delay.dbl();
            GlobalDelays.connection_from_can_to_others += delay.dbl();

            send(resp, "gate$o", 0);
            sendCanFast++;
            rcvdCanFast++;
            updateStatusText();

            cMessage *cloudMsg = nullptr;
            if(strcmp(system->configName, "GarbageInTheCansAndFast") == 0){
                cloudMsg = system->createMessage(MSG_7_COLLECT_GARBAGE);
                send(cloudMsg, "gate$o", 1);
                sendCanFast++;
                updateStatusText();

                simtime_t delay = system->fastWiFiLink->computeDynamicDelay(this, system->cloudNode);
                GlobalDelays.fast_others_to_cloud += delay.dbl();
                GlobalDelays.connection_from_can_to_others += delay.dbl();

            }

            break;
        }
    }

    delete msg;
}

void CanNode::updateStatusText() {
    char buf[200];
    sprintf(buf, "sentCanFast: %d rcvdCanFast: %d numberOfLostCanMsgs: %d",
            sendCanFast, rcvdCanFast, numberOfLostCanMsgs);
    if (statusText) {
        statusText->setText(buf);
    }
}
