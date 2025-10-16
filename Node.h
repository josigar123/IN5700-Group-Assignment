/*
 * Node.h
 *
 *  Created on: Oct 16, 2025
 *      Author: joseph
 */

#ifndef NODE_H_
#define NODE_H_


#include <string.h>
#include <omnetpp.h>
#include "inet/common/INETDefs.h"
#include "inet/common/geometry/common/Coord.h"

using namespace omnetpp;
using namespace inet;

class Node : public cSimpleModule
{
protected:
    cModule *network = nullptr;
    cOvalFigure *oval = nullptr;
    cCanvas *canvas = nullptr;
    const char *configName = nullptr;
    double range = 0;

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;

    // Helper utilities
    virtual void renderText();
    void renderCoverageCircle(double x, double y);
};

Define_Module(Node);

#endif /* NODE_H_ */
