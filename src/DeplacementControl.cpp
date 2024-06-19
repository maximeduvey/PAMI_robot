
#include "DeplacementControl.h"

#include <thread>
#include <stdexcept>

DeplacementControl::DeplacementControl()
    : mId(0), mMotorSpeed(0), mLength(0.0), mTargetLength(0.0)
{
    mState.store(DeplacementControlState::DCS_IDLE);
}

DeplacementControl::~DeplacementControl()
{
}

void DeplacementControl::stop()
{
    exit(1);
}

/// @brief Is called before starting to move, it set motor to a ready state and set spee
void DeplacementControl::setReady()
{
    mDrive.motors_on();
    getDefaultMotorSpeed(mDrive.left.speed, mDrive.right.speed);
}

void DeplacementControl::goForward(int lenght)
{
    this->mTargetLength = mTargetLength;
    mLength = 0.0;
    if (mDrive.getDriveState() != DRIVE::DRIVE_STATE::DRIVE_RUNNING)
    {
        mDrive.motors_on();
        std::thread(&DeplacementControl::LoopManageMotor, this).detach();
    }
}

/// @brief  At it's date of creation this function is just:
/// forward, turn to the left, forward (touch flower)
void DeplacementControl::startStrat_simpleLinesToFlower()
{

}

void DeplacementControl::LoopManageMotor()
{
    auto startTime = std::chrono::high_resolution_clock::now();
    while (mDrive.getDriveState() == DRIVE::DRIVE_STATE::DRIVE_RUNNING &&
           mLength < mTargetLength)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = currentTime - startTime;

        mLength = mMotorSpeed * elapsed.count();
        if (mLength >= mTargetLength || elapsed.count() >= 10.0)
        {
            stop();
        }
    }
}

void DeplacementControl::testRun(int durationSeconds)
{
    double testmTargetLength = mMotorSpeed * durationSeconds;
    // std::cout << "Starting test run for " << durationSeconds << " seconds..." << std::endl;

    goForward(testmTargetLength);
    std::this_thread::sleep_for(std::chrono::seconds(durationSeconds));
    stop();
}

////////////////////
/// Odometry FCT ///
////////////////////

/// @brief fill delta right and left with the difference in position of each wheel by it's last position
///        typically you want to compensate if you do a straight line
/// @param delta_left
/// @param delta_right
/// @return return the absolute difference
int DeplacementControl::getDelta(signed long long &delta_left, signed long long &delta_right)
{
    delta_left = mDrive.left.position - mDrive.left.previous;
    delta_right = mDrive.right.position - mDrive.right.previous;
    int delta = abs(delta_left - delta_right);
    return delta;
}

/// @brief The goal of this funtion is to set the delta depending the direction and position we want to achieve
/// @return
void DeplacementControl::getDirectionFactor(signed long long &delta_left, signed long long &delta_right)
{
    /// to implement
    /// right now for the test we do straight line
}

/// @brief This function is meant to be called in loop and 
void DeplacementControl::loop_motor_drive()
{
    signed long long delta_left = 0;
    signed long long delta_right = 0;

    getDelta(delta_left, delta_right);
    getDirectionFactor(delta_left, delta_right);

    int correction = 5; // 5 is a known-good value
    if (delta_left > delta_right)
    {
        mDrive.left.speed -= correction;
        mDrive.right.speed += correction;
    }
    else if (delta_left < delta_right)
    {
        mDrive.left.speed += correction;
        mDrive.right.speed -= correction;
    }
}

/// @brief This function will set the arbitrary default value for the motor speed depending which PAMI you are
/// During test it was seen that similar motor were not reacting the same to the same value
/// after many test arbitrary value was decided to be a good reference point to have the same movement
/// @param left_motor
/// @param right_motor
void DeplacementControl::getDefaultMotorSpeed(signed long &left_motor, signed long &right_motor)
{
    switch (mId)
    {
    case PAMI_ID_ONE:
        left_motor = DEFAULT_MOTOR_SPEED_PAMI_ONE_WHEEL_LEFT;
        right_motor = DEFAULT_MOTOR_SPEED_PAMI_ONE_WHEEL_RIGHT;
        break;
    case PAMI_ID_TWO:
        left_motor = DEFAULT_MOTOR_SPEED_PAMI_TWO_WHEEL_LEFT;
        right_motor = DEFAULT_MOTOR_SPEED_PAMI_TWO_WHEEL_RIGHT;
        break;
    case PAMI_ID_THREE:
        left_motor = DEFAULT_MOTOR_SPEED_PAMI_THREE_WHEEL_LEFT;
        right_motor = DEFAULT_MOTOR_SPEED_PAMI_THREE_WHEEL_RIGHT;
        break;
    default:
        throw std::runtime_error("ID of the PAmi does not correspond to any referenced!");
    }
}

void DeplacementControl::setId(int id)
{
    mId = id;
}

