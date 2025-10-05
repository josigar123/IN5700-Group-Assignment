/*
 * Node.cc
 *
 *  Created on: Oct 4, 2025
 *      Author: joseph
 */


#include <string.h>
#include <omnetpp.h>
#include "inet/common/INETDefs.h"
#include "inet/mobility/base/MobilityBase.h"
#include "inet/common/geometry/common/Coord.h"

using namespace omnetpp;
using namespace inet;


class Node : public cSimpleModule, public cListener
{
private:
    cOvalFigure *oval;
    MobilityBase *mobility;

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) override;

};

Define_Module(Node);

void Node::initialize()
{
    EV << "INITIALIZING NODE: " << getFullPath() << "\n";

    double x = par("x");
    double y = par("y");
    double range = par("range");

    // Access the network canvas
    cModule *network = getParentModule();
    while (network->getParentModule() != nullptr)
        network = network->getParentModule();
    cCanvas *canvas = network->getCanvas();

    // Create coverage circle
    std::string figureName = std::string("coverage_") + getName();
    oval = new cOvalFigure(figureName.c_str());
    oval->setBounds(cFigure::Rectangle(x - range, y - range, range * 2, range * 2));
    oval->setLineColor(cFigure::BLACK);
    oval->setLineWidth(2);
    canvas->addFigure(oval);

    cModule *mobMod = getParentModule()->getSubmodule("mobility");
    if (mobMod != nullptr) {
        mobility = check_and_cast<inet::MobilityBase *>(mobMod);
        mobility->subscribe(inet::MobilityBase::mobilityStateChangedSignal, this);
    } else {
        mobility = nullptr; // node is static
    }

    EV << "Node " << getName() << " at (" << x << "," << y << ") with range " << range << "\n";
}


void Node::handleMessage(cMessage *msg)
{
    delete msg;
}

void Node::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details)
{
    if (signalID == MobilityBase::mobilityStateChangedSignal) {
        // cast to non-const MobilityBase
        MobilityBase *m = check_and_cast<MobilityBase *>(source);

        if (mobility != nullptr) {
            auto pos = mobility->getCurrentPosition();
            double range = par("range");
            oval->setBounds(cFigure::Rectangle(pos.x - range, pos.y - range, range*2, range*2));
        }
    }
}


