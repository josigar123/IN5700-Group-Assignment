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

simsignal_t Node::garbageCollectedSignalFromCan;
simsignal_t Node::garbageCollectedSignalFromAnotherCan;

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

    // Renders status info based on config
    renderText();

    // Create coverage circle
    renderCoverageCircle(x, y);

    garbageCollectedSignalFromCan = cComponent::registerSignal("garbageCollectedFromCan");
    garbageCollectedSignalFromAnotherCan = cComponent::registerSignal("garbageCollectedFromAnotherCan");

    EV << "Node " << getName() << " at (" << x << "," << y << ") with range " << range << "\n";
}


void Node::handleMessage(cMessage *msg)
{
    delete msg;
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


void Node::renderCoverageCircle(double x, double y){
    oval->setBounds(cFigure::Rectangle(x - range, y - range, range * 2, range * 2));
    oval->setLineColor(cFigure::BLACK);
    oval->setLineWidth(2);
    canvas->addFigure(oval);
}





