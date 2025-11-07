#include "RealisticDelayChannel.h"
#include "Node.h"

Define_Channel(RealisticDelayChannel);

void RealisticDelayChannel::initialize()
{
    cDatarateChannel::initialize(); // Init from super

    // Fetch all relevant parameters to calculate delay from
    baseLatency = par("baseLatency");
    jitterPercentage = par("jitterPercentage");
    propSpeed = par("propSpeed");
}

// calculate delay from src to dst
simtime_t RealisticDelayChannel::computeDynamicDelay(cModule *src, cModule *dst)
{
    // Cast modules to Node pointers
    Node *srcNode = check_and_cast<Node *>(src);
    Node *dstNode = check_and_cast<Node *>(dst);

    // Calculate the differences in coords
    double dx = srcNode->x - dstNode->x;
    double dy = srcNode->y - dstNode->y;
    // Calculate the distance in meters, pythagoras
    double distanceM = sqrt(dx*dx + dy*dy);

    // baseLatency in seconds
    double baseSec = SIMTIME_DBL(baseLatency);

    // Calculate jitter in seconds
    double jitterSec = uniform(-jitterPercentage, jitterPercentage) * baseSec;
    double propagationSec = distanceM / propSpeed;   // meters / (m/s) = seconds

    // calculate the total delay, if its less than zero, set delay to zero
    double totalMs = (baseSec + jitterSec + propagationSec) * 1000;
    if (totalMs < 0) totalMs = 0;

    // Return in simtime ms
    return SimTime(totalMs, SIMTIME_MS);
}
