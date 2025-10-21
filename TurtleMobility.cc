/*
 * TurtleMobility.c
 *
 *  Created on: Oct 20, 2025
 *      Author: joseph
 */


#include "TurtleMobility.h"

Define_Module(Extended::TurtleMobility);

void Extended::TurtleMobility::setLeg(inet::cXMLElement *leg){
    turtleScript = leg;

    nextStatement = turtleScript->getFirstChild();
    nextChange = inet::simTime();
    stationary = false;

    scheduleUpdate();
    EV << nextStatement->str();
}
