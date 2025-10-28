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

    canvas->addFigure(statusText);
}

void CanNode::handleMessage(cMessage *msg){

    int msgId = Node::getMsgId(msg);

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
            if(strcmp(configName, "NoGarbageInTheCans") == 0){
                resp = Node::createMessage(MSG_2_NO);
            }else{
                resp = Node::createMessage(MSG_3_YES);
            }

            send(resp, "gate$o", 0);
            sendCanFast++;
            rcvdCanFast++;
            updateStatusText();

            cMessage *cloudMsg = nullptr;
            if(strcmp(configName, "GarbageInTheCansAndFast") == 0){
                cloudMsg = Node::createMessage(MSG_7_COLLECT_GARBAGE);
                send(cloudMsg, "gate$o", 1);
                sendCanFast++;
                updateStatusText();
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
