/*
 * CanNode.cc
 *
 *  Created on: Oct 16, 2025
 *      Author: joseph
 */

#include "Node.h"

class CanNode : public Node {

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(CanNode);

void CanNode::initialize(){
    Node::initialize();
}

void CanNode::handleMessage(cMessage *msg){
    delete msg;
}
