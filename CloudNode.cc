/*
 * CloudNode.cc
 *
 *  Created on: Oct 16, 2025
 *      Author: joseph
 */

#include "Node.h"

class CloudNode : public Node {

protected:
    // MSg stats variables
    int sentCloudFast = 0;
    int rcvdCloudFast = 0;
    int sentCloudSlow = 0;
    int rcvdCloudSlow = 0;

    // TExt for displaying stats
    cTextFigure *statusText = nullptr;

protected:
    // Base omnet overrides
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;

    // method for updating status text
    void updateStatusText();
};

Define_Module(CloudNode);

void CloudNode::initialize(){
    Node::initialize(); // Init baseline from super

    // ### SETUP STATUS TEXT ONLY IF THERE IS GARBAGE IN THE CANS ###
    if(strcmp(system->configName, "NoGarbageInTheCans") != 0){
        // set the text parameters
        statusText = new cTextFigure("cloudStatus");
        statusText->setColor(cFigure::BLUE);
        statusText->setFont(cFigure::Font("Arial", 36));
        updateStatusText();

        statusText->setPosition(cFigure::Point(x - 500, y - 130)); // Above the node

        system->canvas->addFigure(statusText);
    }
}

void CloudNode::handleMessage(cMessage *msg){

    int msgId = system->getMsgId(msg);

    switch(msgId){

        case MSG_7_COLLECT_GARBAGE:
        {
            cMessage *resp = system->createMessage(MSG_8_OK);

            // On a request if in the fast config, send the message to the Can
            if(strcmp(system->configName, "GarbageInTheCansAndFast") == 0){
                // Send message update local and global stats
                send(resp, "gate$o", 1);
                sentCloudFast++;
                rcvdCloudFast++;
                updateStatusText();

                simtime_t delay = system->fastWiFiLink->computeDynamicDelay(this, system->canNode);
                GlobalDelays.connection_from_others_to_can += delay.dbl();
                GlobalDelays.fast_cloud_to_others += delay.dbl();
            }
            else{

                // Need to check if we need to update the text or not for Slow Config, or not at all for empty
                if(statusText != nullptr){
                    sentCloudSlow++;
                    rcvdCloudSlow++;
                    updateStatusText();

                    // Calculate the delay when sending to the host and update global structure
                    simtime_t delay = system->slowCellularLink->computeDynamicDelay(this, system->hostNode);
                    GlobalDelays.slow_others_to_smartphone += delay.dbl();
                    GlobalDelays.slow_cloud_to_others += delay.dbl();
                }

                send(resp, "gate$o", 0);
            }

            break;
        }

        // Exact same logic as the above case
        case MSG_9_COLLECT_GARBAGE:
        {
            cMessage *resp = system->createMessage(MSG_10_OK);

            if(strcmp(system->configName, "GarbageInTheCansAndFast") == 0){
                send(resp, "gate$o", 2);
                sentCloudFast++;
                rcvdCloudFast++;
                updateStatusText();

                simtime_t delay = system->fastWiFiLink->computeDynamicDelay(this, system->anotherCanNode);
                GlobalDelays.connection_from_others_to_another_can += delay.dbl();
                GlobalDelays.fast_cloud_to_others += delay.dbl();
            }
            else{
                if(statusText != nullptr){
                    sentCloudSlow++;
                    rcvdCloudSlow++;
                    updateStatusText();

                    simtime_t delay = system->slowCellularLink->computeDynamicDelay(this, system->hostNode);
                    GlobalDelays.slow_others_to_smartphone += delay.dbl();
                    GlobalDelays.slow_cloud_to_others += delay.dbl();
                }

                send(resp, "gate$o", 0);
            }

            break;
        }
    }

    delete msg;
}

// Util for rendering text
void CloudNode::updateStatusText() {
    char buf[200];
    sprintf(buf, "sentCloudFast: %d rcvdCloudFast: %d sentCloudSlow: %d rcvdCloudSlow: %d",
            sentCloudFast, rcvdCloudFast, sentCloudSlow, rcvdCloudSlow);
    if (statusText) {
        statusText->setText(buf);
    }
}

