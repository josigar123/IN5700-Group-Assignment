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

class Node : public cSimpleModule
{

public:
    double x;
    double y;

protected:
    GarbageCollectionSystem *system;
    cOvalFigure *oval          				= nullptr;
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
