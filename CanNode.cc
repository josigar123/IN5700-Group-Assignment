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

    if(strcmp(msg->getName(), "8-Ok") == 0){
        rcvdCanFast++;
        updateStatusText();
    }

    if(strcmp(msg->getName(), "1-Is the can full?") == 0){

        // Check if we should drop or process message
        if(dropCount < dropLimit){
            bubble("Lost message");
            dropCount++;
            numberOfLostCanMsgs++;
            updateStatusText();
        }
        else{
            if(strcmp(configName, "GarbageInTheCansAndFast") == 0){
                cMessage *cloudMsg = new cMessage("7-Collect garbage");
                send(cloudMsg, "gate$o", 1);
                sendCanFast++;
                updateStatusText();
            }

            cMessage *resp;
            if(strcmp(configName, "NoGarbageInTheCans") == 0){
                resp = new cMessage("2-No");
            }else{
                resp = new cMessage("3-Yes");
            }
            send(resp, "gate$o", 0);
            sendCanFast++;
            rcvdCanFast++;
            updateStatusText();

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
