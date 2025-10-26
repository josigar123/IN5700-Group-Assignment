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

  protected:
    virtual void initialize() override;

  public:
    // Make a public safe getter for the full current delay
    simtime_t getCurrentDelay() const { return getDelay() + baseLatency; }
};

#endif
