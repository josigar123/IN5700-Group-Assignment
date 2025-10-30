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
    simtime_t baseLatency;
    double jitterPercentage;
    double propSpeed;

  protected:
    virtual void initialize() override;

  public:
    // Make a public safe getter for the full current delay
    simtime_t getCurrentDelay() const { return getDelay() + baseLatency; }

    // Just get the base latency for the link
    simtime_t getBaseLatency() const { return baseLatency; }

    // Used for calculating the dynamic delay for a link from src to dst, returns the delay as simtime_t
    simtime_t computeDynamicDelay(cModule *src, cModule *dst);
};

#endif
