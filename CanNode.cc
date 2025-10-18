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

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(CanNode);

void CanNode::initialize(){
    Node::initialize();
}

void CanNode::handleMessage(cMessage *msg){
    if(strcmp(msg->getName(), "1-Is the can full?") == 0 && dropCount == dropLimit){
        if(strcmp(configName, "GarbageInTheCansAndFast") == 0){
            cMessage *cloudMsg = new cMessage("7-Collect garbage");
            send(cloudMsg, "gate$o", 1);
        }
        cMessage *resp = new cMessage("3-Yes");
        send(resp, "gate$o", 0);
    }
    else{
        bubble("Package lost");
        dropCount++;
    }
    delete msg;
}
