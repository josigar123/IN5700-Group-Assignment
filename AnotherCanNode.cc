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

    canvas->addFigure(statusText);
}

void AnotherCanNode::handleMessage(cMessage *msg){

    if(strcmp(msg->getName(), "10-Ok") == 0){
        rcvdAnotherCanFast++;
        updateStatusText();
        emit(Node::garbageCollectedSignalFromAnotherCan, true);
    }

    if(strcmp(msg->getName(), "4-Is the can full?") == 0){

        if(dropCount < dropLimit){
            bubble("Lost message");
            dropCount++;
            numberOfLostAnotherCanMsgs++;
            updateStatusText();
        }else{

            if(strcmp(configName, "GarbageInTheCansAndFast") == 0){
                cMessage *cloudMsg = new cMessage("9-Collect garbage");
                send(cloudMsg, "gate$o", 1);
                sendAnotherCanFast++;
                updateStatusText();
            }

            cMessage *resp;
            if(strcmp(configName, "NoGarbageInTheCans") == 0){
                resp = new cMessage("5-No");
            }else{
                resp =new cMessage("6-Yes");
            }

            send(resp, "gate$o", 0);
            sendAnotherCanFast++;
            rcvdAnotherCanFast++;
            updateStatusText();
        }
    }
    delete msg;
}

void AnotherCanNode::updateStatusText() {
    char buf[200];
    sprintf(buf, "sentAnotherCanFast: %d rcvdAnotherCanFast: %d numberOfLostAnotherCanMsgs: %d",
            sendAnotherCanFast, rcvdAnotherCanFast, numberOfLostAnotherCanMsgs);
    if (statusText) {
        statusText->setText(buf);
    }
}
