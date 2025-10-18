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

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(AnotherCanNode);

void AnotherCanNode::initialize(){
    Node::initialize();
}

void AnotherCanNode::handleMessage(cMessage *msg){
    if(strcmp(msg->getName(), "4-Is the can full?") == 0 && dropCount == dropLimit){
        if(strcmp(configName, "GarbageInTheCansAndFast") == 0){
            cMessage *cloudMsg = new cMessage("9-Collect garbage");
            send(cloudMsg, "gate$o", 1);
        }

        cMessage *resp;
        if(strcmp(configName, "NoGarbageInTheCans") == 0){
            resp = new cMessage("5-No");
        }else{
            resp =new cMessage("6-Yes");
        }

        send(resp, "gate$o", 0);
    }
    else{
        bubble("Package lost");
        dropCount++;
    }
    delete msg;
}
