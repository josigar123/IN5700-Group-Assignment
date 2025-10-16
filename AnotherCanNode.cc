/*
 * AnotherCanNode.cc
 *
 *  Created on: Oct 16, 2025
 *      Author: joseph
 */

#include "Node.h"

class AnotherCanNode : public Node {

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(AnotherCanNode);

void AnotherCanNode::initialize(){
    Node::initialize();
}

void AnotherCanNode::handleMessage(cMessage *msg){
    delete msg;
}



