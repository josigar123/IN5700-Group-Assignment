/*
 * Test.cc
 *
 *  Created on: Sep 30, 2025
 *      Author: joseph
 */

#include <string.h>
#include <omnetpp.h>

using namespace omnetpp;

class Node : public cSimpleModule
{
protected:
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(Node);

void Node::handleMessage(cMessage *msg)
{
    int n = gateSize("out");
    if (n > 0) {
        int k = intuniform(0, n-1);  // Pick a random output gate
        send(msg, "out", k);
    }
}
