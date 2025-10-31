#include "RealisticDelayChannel.h"
#include "Node.h"

Define_Channel(RealisticDelayChannel);

void RealisticDelayChannel::initialize()
{
    cDatarateChannel::initialize();
    baseLatency = par("baseLatency");
    jitterPercentage = par("jitterPercentage");
    propSpeed = par("propSpeed");
}

simtime_t RealisticDelayChannel::computeDynamicDelay(cModule *src, cModule *dst)
{
    // Cast modules to Node pointers
    Node *srcNode = check_and_cast<Node *>(src);
    Node *dstNode = check_and_cast<Node *>(dst);

    // Calculate the differences in coords
    double dx = srcNode->x - dstNode->x;
    double dy = srcNode->y - dstNode->y;
    // Calculate the distance in meters
    double distanceM = sqrt(dx*dx + dy*dy);

    // baseLatency in seconds
    double baseSec = SIMTIME_DBL(baseLatency);

    // Calculate jitter in seconds
    double jitterSec = uniform(-jitterPercentage, jitterPercentage) * baseSec;
    double propagationSec = distanceM / propSpeed;   // meters / (m/s) = seconds

    // calculate the total delay, if its less than zero, set delay to zer
    double totalSec = baseSec + jitterSec + propagationSec;
    if (totalSec < 0) totalSec = 0;

    // Return in simtime seconds
    return SimTime(totalSec, SIMTIME_S);
}
