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
    cModule *network;
    cOvalFigure *oval;
    MobilityBase *mobility;
    cCanvas *canvas;
    const char *configName;
    double range;

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) override;
    virtual void renderText();
    void sendMsgTo(const char* dest, const cMessage* msg);
    void renderCoverageCircle(double x, double y);
    void subscribeToMobilityStateChangedSignalIfMobile();
    bool isInRangeOf(Node* target);

};

Define_Module(Node);

void Node::initialize()
{
    EV << "INITIALIZING NODE: " << getFullPath() << "\n";

    // Extract module params
    double x = par("x");
    double y = par("y");

    // Set class fields
    range = par("range");

    network = getParentModule();
    while (network->getParentModule() != nullptr) // Dig out the parentModule
        network = network->getParentModule();
    canvas = network->getCanvas();

    std::string figureName = std::string("coverage_") + getName();
    oval = new cOvalFigure(figureName.c_str());
    configName = getEnvir()->getConfigEx()->getActiveConfigName();

    // Renders status info based on config
    renderText();

    // Create coverage circle
    renderCoverageCircle(x, y);

    subscribeToMobilityStateChangedSignalIfMobile();

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
            oval->setBounds(cFigure::Rectangle(pos.x - range, pos.y - range, range*2, range*2));
        }

        Node* canNode = check_and_cast<Node*>(network->getSubmodule("can"));
        Node* anotherCanNode = check_and_cast<Node*>(network->getSubmodule("anotherCan"));
        Node* cloudNode = check_and_cast<Node*>(network->getSubmodule("cloud"));

        if(isInRangeOf(canNode) || isInRangeOf(anotherCanNode)){
            oval->setLineColor(cFigure::GREEN);
        }else
            oval->setLineColor(cFigure::BLACK);
    }
}

void Node::renderText(){

    cTextFigure *header = new cTextFigure("configTextHeader");
    header->setFont(cFigure::Font("Arial", 30));
    header->setPosition(cFigure::Point(2100, 25));

    cTextFigure *body = new cTextFigure("configTextBody");
    body->setFont(cFigure::Font("Arial", 30));
    body->setPosition(cFigure::Point(2100, 150));

    if(strcmp(configName, "GarbageInTheCansAndSlow") == 0){
        header->setText("Cloud-based solution with slow messages");
    }else if(strcmp(configName, "GarbageInTheCansAndFast") == 0){
        header->setText("Fog-based solution with fast messages");

    }else if(strcmp(configName, "NoGarbageInTheCans") == 0){
        header->setText("Fog-based solution with no messages");
    }else{
        header->setText("Unknow config");
    }

    body->setText("Slow connection from the smartphone to others (time it takes) = \nSlow connection from others to the smartphone (time it takes) =\nFast connection from others to the smartphone (time it takes) =\n\nConnection from the can to others (time it takes) =\nConnection from others to the can (time it takes) = \n\nConnection from the anotherCan to others (time it takes) =\nConnection from others to the anotherCan (time it takes) =\n\nSlow connection from the Cloud to others (time it takes) =\nSlow connection from others to the Cloud (time it takes) =\nFast connection from the Cloud to others (time it takes) =\nFast connection from others to the Cloud (time it takes) =\n");

    canvas->addFigure(header);
    canvas->addFigure(body);
}

bool Node::isInRangeOf(Node* target) {
    // Host must have mobility, otherwise no need to check
    if (!mobility) return false;

    auto posHost = mobility->getCurrentPosition();

    // Get target position
    double xOther, yOther, rangeOther;

    xOther = target->par("x");
    yOther = target->par("y");

    rangeOther = target->par("range");

    double dx = posHost.x - xOther;
    double dy = posHost.y - yOther;
    double distance = std::sqrt(dx*dx + dy*dy);

    // circles overlap if distance <= sum of ranges
    return distance <= (this->range + rangeOther);
}

void Node::renderCoverageCircle(double x, double y){
    oval->setBounds(cFigure::Rectangle(x - range, y - range, range * 2, range * 2));
    oval->setLineColor(cFigure::BLACK);
    oval->setLineWidth(2);
    canvas->addFigure(oval);
}

void Node::subscribeToMobilityStateChangedSignalIfMobile(){
    cModule *mobMod = getParentModule()->getSubmodule("mobility");
    if (mobMod != nullptr) {
        mobility = check_and_cast<inet::MobilityBase *>(mobMod);
        mobility->subscribe(inet::MobilityBase::mobilityStateChangedSignal, this);
    } else {
        mobility = nullptr; // node is static
    }
}





