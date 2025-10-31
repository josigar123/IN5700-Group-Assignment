/*
 * GarbageCollectionSystem.h
 *
 *  Created on: Oct 31, 2025
 *      Author: joseph
 */

#ifndef GARBAGECOLLECTIONSYSTEM_H_
#define GARBAGECOLLECTIONSYSTEM_H_

#include <string.h>
#include <omnetpp.h>
#include "inet/common/INETDefs.h"
#include "inet/common/geometry/common/Coord.h"

using namespace omnetpp;
using namespace inet;

// Forward declarations
class Node;
class RealisticDelayChannel;

// Define a global struct instance for delay accumulation
struct LinkDelays {
    double slow_smartphone_to_others = 0, slow_others_to_smartphone = 0, fast_smartphone_to_others = 0, fast_others_to_smartphone = 0;

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

class GarbageCollectionSystem : public cSimpleModule{

protected:
    cTextFigure *delayStatsHeader = nullptr;
    cTextFigure *hostDelayStats = nullptr;
    cTextFigure *canDelayStats = nullptr;
    cTextFigure *anotherCanDelayStats = nullptr;
    cTextFigure *cloudDelayStats = nullptr;

public:
    cCanvas *canvas                         = nullptr;
    const char *configName                  = nullptr;

    Node* canNode                           = nullptr;
    Node* anotherCanNode                    = nullptr;
    Node* cloudNode                         = nullptr;
    Node* hostNode                          = nullptr;

    RealisticDelayChannel *slowCellularLink = nullptr;
    RealisticDelayChannel *fastCellularLink = nullptr;
    RealisticDelayChannel *fastWiFiLink     = nullptr;

    double range = 0;

    // Easy lookup of config, no need to str compare
    enum FsmType { FAST, SLOW, EMPTY };
    FsmType fsmType;

    // Defines the three state machines, one for each config at compile tieme, and a pointer which is given a value at runtime
    // pointing to the appropriate state machine
    cFSM *currentFsm; // For storing the appropriate FSM
    cFSM fastFsm;
    enum{
        FAST_SEND_TO_CAN = FSM_Steady(1),
        FAST_SEND_TO_ANOTHER_CAN = FSM_Steady(2),
        FAST_EXIT = FSM_Steady(3),
    };
    cFSM slowFsm;
    enum{
       SLOW_SEND_TO_CAN = FSM_Steady(1),
       SLOW_SEND_TO_CAN_CLOUD = FSM_Steady(2),
       SLOW_SEND_TO_ANOTHER_CAN = FSM_Steady(3),
       SLOW_SEND_TO_ANOTHER_CAN_CLOUD = FSM_Steady(4),
       SLOW_EXIT = FSM_Steady(5),
    };
    cFSM emptyFsm;
    enum{
        EMPTY_SEND_TO_CAN = FSM_Steady(1),
        EMPTY_SEND_TO_ANOTHER_CAN = FSM_Steady(2),
        EMPTY_EXIT = FSM_Steady(3),
    };

protected:
    virtual void initialize() override;
    virtual void finish() override;
    void renderInitialDelayStats();

public:
    cMessage *createMessage(MsgID id);
    int getMsgId(cMessage *msg);
};

#endif /* GARBAGECOLLECTIONSYSTEM_H_ */
