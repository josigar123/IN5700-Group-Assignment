/*
 * Node.cc
 *
 *  Created on: Oct 4, 2025
 *      Author: joseph
 */


#include <string.h>
#include <omnetpp.h>
#include "inet/common/INETDefs.h"
#include "inet/common/geometry/common/Coord.h"
#include "Node.h"

using namespace omnetpp;
using namespace inet;

// Register signals for fast config completed message tx
simsignal_t Node::garbageCollectedSignalFromCan = cComponent::registerSignal("garbageCollectedFromCan");
simsignal_t Node::garbageCollectedSignalFromAnotherCan = cComponent::registerSignal("garbageCollectedFromAnotherCan");

Define_Module(Node);

void Node::initialize()
{
    EV << "INITIALIZING NODE: " << getFullPath() << "\n";

    // Extract module params
    x = par("x");
    y = par("y");

    // Set class fields
    range = par("range");

    network = getParentModule();
    while (network->getParentModule() != nullptr) // Dig out the parentModule
        network = network->getParentModule();
    canvas = network->getCanvas();

    std::string figureName = std::string("coverage_") + getName();
    oval = new cOvalFigure(figureName.c_str());
    configName = getEnvir()->getConfigEx()->getActiveConfigName();

    // Create coverage circle
    renderCoverageCircle(x, y);

    EV << "Node " << getName() << " at (" << x << "," << y << ") with range " << range << "\n";
}


void Node::handleMessage(cMessage *msg)
{
    delete msg;
}

void Node::renderCoverageCircle(double x, double y){
    oval->setBounds(cFigure::Rectangle(x - range, y - range, range * 2, range * 2));
    oval->setLineColor(cFigure::BLACK);
    oval->setLineWidth(2);
    canvas->addFigure(oval);
}


