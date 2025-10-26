#include "RealisticDelayChannel.h"

Define_Channel(RealisticDelayChannel);

void RealisticDelayChannel::initialize()
{
    cDatarateChannel::initialize();
    baseLatency = par("baseLatency");
}
