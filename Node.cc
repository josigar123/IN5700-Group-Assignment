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

// GLobal structure for delay accumulations
LinkDelays GlobalDelays;

// Register signals for fast config completed message tx
simsignal_t Node::garbageCollectedSignalFromCan = cComponent::registerSignal("garbageCollectedFromCan");
simsignal_t Node::garbageCollectedSignalFromAnotherCan = cComponent::registerSignal("garbageCollectedFromAnotherCan");

Define_Module(Node);

void Node::initialize()
{
    EV << "INITIALIZING NODE: " << getFullPath() << "\n";

    system = check_and_cast<GarbageCollectionSystem *>(getParentModule());

    // Extract module params
    x = par("x");
    y = par("y");
    range = par("range");

    std::string figureName = std::string("coverage_") + getName();
    oval = new cOvalFigure(figureName.c_str());

    // Create coverage circle
    renderCoverageCircle(x, y);

    EV << "Node " << getName() << " has channels: "
       << (system->slowCellularLink ? "slowCell " : "")
       << (system->fastCellularLink ? "fastCell " : "")
       << (system->fastWiFiLink ? "wifi " : "") << "\n";
}

void Node::renderCoverageCircle(double x, double y){
    oval->setBounds(cFigure::Rectangle(x - range, y - range, range * 2, range * 2));
    oval->setLineColor(cFigure::BLACK);
    oval->setLineWidth(2);
    system->canvas->addFigure(oval);
}
