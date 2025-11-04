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
#include "RealisticDelayChannel.h"
#include "GarbageCollectionSystem.h"

using namespace omnetpp;
using namespace inet;

// Forward declaration
class GarbageCollectionSystem;

// The base class for all system nodes
class Node : public cSimpleModule
{

public:
    // Public access of coords
    double x;
    double y;

protected:
    // All subclasses can interact with the system
    GarbageCollectionSystem *system;
    // Oval figure for coverage circle
    cOvalFigure *oval = nullptr;

    // range of coverage
    double range = 0;

protected:
    virtual void initialize() override;

    // For rendering nodes initial coverage circles
    void renderCoverageCircle(double x, double y);

public:
    // Signals used for fast config when message exchange between can-cloud is complete
    static simsignal_t garbageCollectedSignalFromCan;
    static simsignal_t garbageCollectedSignalFromAnotherCan;
};

#endif /* NODE_H_ */
