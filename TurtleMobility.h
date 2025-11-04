/*
 * TurtleMobility.h
 *
 *  Created on: Oct 20, 2025
 *      Author: joseph
 */


#include "inet/mobility/single/TurtleMobility.h"

// The extended TurtleMobility inheritor for more flexible start/stopping
namespace Extended{

class TurtleMobility : public inet::TurtleMobility // inherit from base Turtle
{

public:
    // set a leg to traverse
    void setLeg(inet::cXMLElement * leg);
};


}
