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
    delete msg;
}



