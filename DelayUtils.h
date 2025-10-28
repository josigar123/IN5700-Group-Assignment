/*
 * DelayUtils.h
 *  Created on: Oct 28, 2025
 *      Author: joseph
 */

#ifndef DELAYUTILS_H_
#define DELAYUTILS_H_

#include <omnetpp.h>

using namespace omnetpp;

namespace DelayUtils {
    double channelDelay(cGate *g);
    double delayFrom(cModule *m, const char* gateName, int ix);
}

#endif /* DELAYUTILS_H_ */
