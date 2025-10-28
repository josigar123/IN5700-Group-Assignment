/*
 * DelayUtils.cc
 *
 *  Created on: Oct 28, 2025
 *      Author: joseph
 */

#include "DelayUtils.h"
#include <omnetpp.h>
#include "RealisticDelayChannel.h"

using namespace omnetpp;

namespace DelayUtils {

double channelDelay(cGate *g) {
    if (!g) return 0;
    cChannel *ch = g->getChannel();
    if (!ch) return 0;

    // Try to access a custom delay channel that might vary dynamically
    if (auto realistic = dynamic_cast<RealisticDelayChannel*>(ch))
        return SIMTIME_DBL(realistic->getCurrentDelay());

    // Otherwise return a static delay if it's just a normal cDelayChannel
    if (auto base = dynamic_cast<cDelayChannel*>(ch))
        return SIMTIME_DBL(base->getDelay());

    return 0;
}

double delayFrom(cModule *m, const char* gateName, int ix) {
    if (!m) return 0;
    cGate *g = m->gate(gateName, ix);
    return channelDelay(g);
}

} // namespace DelayUtils

