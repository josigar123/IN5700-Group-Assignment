/*
 * TurtleMobility.h
 *
 *  Created on: Oct 20, 2025
 *      Author: joseph
 */


#include "inet/mobility/single/TurtleMobility.h"

namespace Extended{

class TurtleMobility : public inet::TurtleMobility
{

public:
    void setLeg(inet::cXMLElement * leg);
};


}
