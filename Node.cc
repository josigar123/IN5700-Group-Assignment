/*
 * Node.cc
 *
 *  Created on: Oct 4, 2025
 *      Author: joseph
 */


#include <string.h>
#include <omnetpp.h>

using namespace omnetpp;

class Node : public cSimpleModule
{
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(Node);

void Node::initialize()
{
    // Get parameters
    double x = par("x");
    double y = par("y");
    double range = par("range");

    // Get the network's canvas
    cCanvas *canvas = getParentModule()->getCanvas();

    // Create unique name for this node's coverage circle
    std::string figureName = std::string("coverage_") + getName();
    cOvalFigure *oval = new cOvalFigure(figureName.c_str());

    // Set bounds: top-left corner and size
    // Rectangle(x, y, width, height)
    oval->setBounds(cFigure::Rectangle(x - range, y - range,
                                       range * 2, range * 2));

    // Set appearance
    oval->setLineColor(cFigure::BLACK);
    oval->setLineWidth(2);

    // Add to canvas
    canvas->addFigure(oval);

    EV << "Node " << getName() << " at (" << x << "," << y
       << ") with range " << range << "\n";
}

void Node::handleMessage(cMessage *msg)
{
    int n = gateSize("out");
    if (n > 0) {
        int k = intuniform(0, n-1);  // Pick a random output gate
        send(msg, "out", k);
    }
}

