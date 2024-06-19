#pragma once

#include <atomic>
#include "drive.hpp"

/// @brief This class is meant for deplacement control, it's an abstraction to provide a more action like movement
/// like move forward of x meter, do a rotation x degrees, etc..
/// It's also meant for (in the future) a more spatial movement, PAMI think it's a specific position and will try to reach pos Y
/// The core motor is done by the driver drive.h 
class DeplacementControl
{
    public:
    /// this enum represent the current state of the objectif of the deplacement
    // are we running, are we block, etc...
    enum DeplacementControlState : uint8_t
    {
        DCS_IDLE = 0,
        DCS_RUNNING,
        DCS_BLOCKED,

        DCS_UNKNOW // should not happen
    };
private:
    DRIVE mDrive;
    std::atomic<DeplacementControlState> mState;
    /* data */

    double mLength;       // Length covered by the motor
    double mMotorSpeed;                // Speed of the motor
    double mTargetLength;              // Target length to reach

public:
    DeplacementControl(/* args */);
    ~DeplacementControl();

    /// basic Action ///
    // simple oneshot action that the pami will do
    void goForward(int lenght);
    void stop();

    /// objectif action ///
    // objectif action a followup of step that the pami want to achieve
    void startStrat_simpleLinesToFlower();

    /// infos function ///
    // provide informations (some mutexed) about ongoing actions
    inline DeplacementControlState getDeplacementControlState() {return mState.load();}

    void LoopManageMotor();
    void testRun(int durationSeconds);

private:
    int getDelta();
};
