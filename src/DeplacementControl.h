#pragma once

#include <atomic>
#include "drive.hpp"

// 4 RPS / 240 RPM max speed
// Set the initial speed of the motors
// Initial values are unequal to compensate for issues with the motors.
#define DEFAULT_MOTOR_SPEED_PAMI_ONE_WHEEL_LEFT 2000
#define DEFAULT_MOTOR_SPEED_PAMI_ONE_WHEEL_RIGHT 2880
#define DEFAULT_MOTOR_SPEED_PAMI_TWO_WHEEL_LEFT 2880
#define DEFAULT_MOTOR_SPEED_PAMI_TWO_WHEEL_RIGHT 2050
#define DEFAULT_MOTOR_SPEED_PAMI_THREE_WHEEL_LEFT 1700
#define DEFAULT_MOTOR_SPEED_PAMI_THREE_WHEEL_RIGHT 2880

/// this is the default value to do a full turn turn for the wheel with the default above value
#define DEFAULT_ONE_FULL_TURN_RPM 36000

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
    int mId; // represent the id of the PAMI
    DRIVE mDrive;
    std::atomic<DeplacementControlState> mState;
    /* data */

    double mLength;       // Length covered by the motor
    double mMotorSpeed;                // Speed of the motor
    double mTargetLength;              // Target length to reach

public:
    DeplacementControl();
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
    void setId(int id);

    void LoopManageMotor();
    void testRun(int durationSeconds);

    void loop_motor_drive();
    void setReady();

private:
    int getDelta(signed long long &delta_left, signed long long &delta_right);
    void getDirectionFactor(signed long long &delta_left, signed long long &delta_right);

    void getDefaultMotorSpeed(signed long &left_motor, signed long &right_motor);
};
