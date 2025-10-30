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

simtime_t RealisticDelayChannel::computeDynamicDelay(cModule *src, cModule *dst){

    Node *srcNode = check_and_cast<Node *>(src);
    Node *dstNode = check_and_cast<Node *>(dst);
    double distanceM = sqrt(pow(srcNode->x - dstNode->x, 2) + pow(srcNode->y - dstNode->y, 2));


    double jitterMs = uniform(-jitterPercentage, jitterPercentage) * baseLatency.dbl();
    double propagationDelayMs = (distanceM / propSpeed) * 1000;

    return (simtime_t) (baseLatency + jitterMs + propagationDelayMs);
}
