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

using namespace omnetpp;
using namespace inet;

// Define a global struct instance for delay accumulation
struct LinkDelays {
    double slow_smartphone_to_others = 0, slow_other_to_smartphone = 0, fast_smartphone_to_others = 0, fast_others_to_smartphone = 0;

    double connection_from_can_to_others = 0, connection_from_others_to_can = 0;

    double connection_from_another_can_to_others = 0, connection_from_others_to_another_can = 0;

    double slow_cloud_to_others = 0, slow_others_to_cloud = 0, fast_cloud_to_others = 0, fast_others_to_cloud = 0;
};

extern LinkDelays GlobalDelays;

enum MsgID {
    MSG_1_IS_CAN_FULL = 1,
    MSG_2_NO,
    MSG_3_YES,
    MSG_4_IS_CAN_FULL,
    MSG_5_NO,
    MSG_6_YES,
    MSG_7_COLLECT_GARBAGE,
    MSG_8_OK,
    MSG_9_COLLECT_GARBAGE,
    MSG_10_OK
};

class Node : public cSimpleModule
{

public:
    double x;
    double y;

protected:
    cModule *network           				= nullptr;
    cOvalFigure *oval          				= nullptr;
    cCanvas *canvas                         = nullptr;
    const char *configName     				= nullptr;

    Node* canNode              				= nullptr;
    Node* anotherCanNode       				= nullptr;
    Node* cloudNode            				= nullptr;

    RealisticDelayChannel *slowCellularLink = nullptr;
    RealisticDelayChannel *fastCellularLink = nullptr;
    RealisticDelayChannel *fastWiFiLink     = nullptr;

    double range = 0;

protected:
    virtual void initialize() override;

    // For rendering nodes initial coverage circles
    void renderCoverageCircle(double x, double y);

public:
    // For creating one of the systems pre-defined messages based on the enum value
    cMessage *createMessage(MsgID id);
    int getMsgId(cMessage *msg);

public:
    // Signals used for fast config when message exchange between can-cloud is complete
    static simsignal_t garbageCollectedSignalFromCan;
    static simsignal_t garbageCollectedSignalFromAnotherCan;
};

#endif /* NODE_H_ */
