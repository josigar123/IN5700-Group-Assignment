#ifndef __SMARTGARBAGECOLLECTION_REALISTICDELAYCHANNEL_H_
#define __SMARTGARBAGECOLLECTION_REALISTICDELAYCHANNEL_H_

#include <omnetpp.h>
using namespace omnetpp;

/**
 * Custom channel that adds a configurable base latency
 * on top of the regular datarate-based delay.
 */
class RealisticDelayChannel : public cDatarateChannel
{
  protected:
    // Values to calculate delay upon
    simtime_t baseLatency;
    double jitterPercentage;
    double propSpeed;

  protected:
    virtual void initialize() override;

  public:
    // Used for calculating the dynamic delay for a link from src to dst, returns the delay as simtime_t
    simtime_t computeDynamicDelay(cModule *src, cModule *dst);
};

#endif
