#pragma once

#include "Drivers/drive.hpp"

/// @brief This class is meant for deplacement control, it's an abstraction to provide a more action like movement
/// like move forward of x meter, do a rotation x degrees, etc..
/// It's also meant for (in the future) a more spatial movement, PAMI think it's a specific position and will try to reach pos Y
/// The core motor is done by the driver drive.h 
class DeplacementControl
{
private:
    /* data */
public:
    DeplacementControl(/* args */);
    ~DeplacementControl();
};
