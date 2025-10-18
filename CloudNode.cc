/*
 * CloudNode.cc
 *
 *  Created on: Oct 16, 2025
 *      Author: joseph
 */

#include "Node.h"

class CloudNode : public Node {

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(CloudNode);

void CloudNode::initialize(){
    Node::initialize();
}

void CloudNode::handleMessage(cMessage *msg){
    if(strcmp(msg->getName(), "7-Collect garbage") == 0){
        cMessage *resp = new cMessage("8-Ok");

        if(strcmp(configName, "GarbageInTheCansAndFast") == 0){
            emit(garbageCollectedSignalFromCan, true);
            send(resp, "gate$o", 1);
        }
        else
            send(resp, "gate$o", 0);
    }

    if(strcmp(msg->getName(), "9-Collect garbage") == 0){
        cMessage *resp = new cMessage("10-Ok");

        if(strcmp(configName, "GarbageInTheCansAndFast") == 0){
            emit(garbageCollectedSignalFromAnotherCan, true);
            send(resp, "gate$o", 2);
        }
        else
            send(resp, "gate$o", 0);
    }

    delete msg;
}



