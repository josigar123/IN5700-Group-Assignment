/*
 * HostNode.cc
 *
 *  Created on: Oct 16, 2025
 *      Author: joseph
 */

#include "inet/mobility/base/MobilityBase.h"
#include "Node.h"

class HostNode : public Node, public cListener{

protected:
    MobilityBase *mobility;

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) override;

    virtual void subscribeToMobilityStateChangedSignal();
    virtual bool isInRangeOf(Node *target);
};

Define_Module(HostNode);

void HostNode::initialize(){

    Node::initialize();
    subscribeToMobilityStateChangedSignal();
}

void HostNode::handleMessage(cMessage *msg){
    delete msg;
}

void HostNode::subscribeToMobilityStateChangedSignal(){
    cModule *mobMod = getSubmodule("mobility");
    if (mobMod != nullptr) {
        mobility = check_and_cast<inet::MobilityBase *>(mobMod);
        mobility->subscribe(inet::MobilityBase::mobilityStateChangedSignal, this);
    } else {
        mobility = nullptr; // node is static
    }
}

bool HostNode::isInRangeOf(Node* target) {
    // Host must have mobility, otherwise no need to check
    if (!mobility) return false;

    auto posHost = mobility->getCurrentPosition();

    // Get target position
    double xOther, yOther, rangeOther;

    xOther = target->par("x");
    yOther = target->par("y");

    rangeOther = target->par("range");

    double dx = posHost.x - xOther;
    double dy = posHost.y - yOther;
    double distance = std::sqrt(dx*dx + dy*dy);

    // circles overlap if distance <= sum of ranges
    return distance <= (this->range + rangeOther);
}

void HostNode::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details){
    if (signalID == MobilityBase::mobilityStateChangedSignal) {
            // cast to non-const MobilityBase
            MobilityBase *m = check_and_cast<MobilityBase *>(source);

            if (mobility != nullptr) {
                auto pos = mobility->getCurrentPosition();
                oval->setBounds(cFigure::Rectangle(pos.x - range, pos.y - range, range*2, range*2));
            }

            Node* canNode = check_and_cast<Node*>(network->getSubmodule("can"));
            Node* anotherCanNode = check_and_cast<Node*>(network->getSubmodule("anotherCan"));
            Node* cloudNode = check_and_cast<Node*>(network->getSubmodule("cloud"));

            if(isInRangeOf(canNode) || isInRangeOf(anotherCanNode)){
                oval->setLineColor(cFigure::GREEN);
            }else
                oval->setLineColor(cFigure::BLACK);
        }
}
