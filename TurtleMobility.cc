/*
 * TurtleMobility.c
 *
 *  Created on: Oct 20, 2025
 *      Author: joseph
 */


#include "TurtleMobility.h"

Define_Module(Mine::TurtleMobility);

void Mine::TurtleMobility::setLeg(inet::cXMLElement *leg){
    turtleScript = leg;

    nextStatement = turtleScript->getFirstChild();
    EV << nextStatement->str();
}

