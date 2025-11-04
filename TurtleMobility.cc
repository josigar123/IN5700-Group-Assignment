/*
 * TurtleMobility.c
 *
 *  Created on: Oct 20, 2025
 *      Author: joseph
 */


#include "TurtleMobility.h"

// Create the module in the appropriate namespace
Define_Module(Extended::TurtleMobility);

// Assumes a single stretch to traverse, but will handle an arbitrary movement,  but we are interested in essentially traversing a single street at a time
void Extended::TurtleMobility::setLeg(inet::cXMLElement *leg){
    // Set the turtleScript to the new leg
    turtleScript = leg;

    // Get the firstChild of the turtle, first child of <movement> tag
    nextStatement = turtleScript->getFirstChild();
    // Schedule nextChange to be now
    nextChange = inet::simTime();
    // Set it so that its no longe stationary
    stationary = false;

    // schedule the update so mobility state changes
    scheduleUpdate();
    EV << nextStatement->str();
}
