/*
 * CloudNode.cc
 *
 *  Created on: Oct 16, 2025
 *      Author: joseph
 */

#include "Node.h"

class CloudNode : public Node {

protected:
    int sentCloudFast = 0;
    int rcvdCloudFast = 0;
    int sentCloudSlow = 0;
    int rcvdCloudSlow = 0;

    cTextFigure *statusText = nullptr;

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;

    void updateStatusText();
};

Define_Module(CloudNode);

void CloudNode::initialize(){
    Node::initialize();

    // ### SETUP STATUS TEXT ONLY IF THERE IS GARBAGE IN THE CANS ###
    if(strcmp(configName, "NoGarbageInTheCans") != 0){
    statusText = new cTextFigure("cloudStatus");
    statusText->setColor(cFigure::BLUE);
    statusText->setFont(cFigure::Font("Arial", 36));
    updateStatusText();

    statusText->setPosition(cFigure::Point(x - 500, y - 100)); // Above the node

    canvas->addFigure(statusText);
    }
}

void CloudNode::handleMessage(cMessage *msg){
    if(strcmp(msg->getName(), "7-Collect garbage") == 0){
        cMessage *resp = new cMessage("8-Ok");

        if(strcmp(configName, "GarbageInTheCansAndFast") == 0){
            send(resp, "gate$o", 1);
            sentCloudFast++;
            rcvdCloudFast++;
            updateStatusText();
        }
        else{
            if(statusText != nullptr){
                sentCloudSlow++;
                rcvdCloudSlow++;
                updateStatusText();
            }
            send(resp, "gate$o", 0);
        }
    }

    if(strcmp(msg->getName(), "9-Collect garbage") == 0){
        cMessage *resp = new cMessage("10-Ok");

        if(strcmp(configName, "GarbageInTheCansAndFast") == 0){
            send(resp, "gate$o", 2);
            sentCloudFast++;
            rcvdCloudFast++;
            updateStatusText();
        }
        else{
            if(statusText != nullptr){
                sentCloudSlow++;
                rcvdCloudSlow++;
                updateStatusText();
            }

            send(resp, "gate$o", 0);
        }

    }

    delete msg;
}

void CloudNode::updateStatusText() {
    char buf[200];
    sprintf(buf, "sentCloudFast: %d rcvdCloudFast: %d sentCloudSlow: %d rcvdCloudSlow: %d",
            sentCloudFast, rcvdCloudFast, sentCloudSlow, rcvdCloudSlow);
    if (statusText) {
        statusText->setText(buf);
    }
}

