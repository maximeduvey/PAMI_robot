
#include "DeplacementControl.h"

#include <thread>

DeplacementControl::DeplacementControl()
 : mMotorSpeed(0), mLength(0.0), mTargetLength(0.0)
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
        if (mLength >= mTargetLength || elapsed.count() >= 10.0) {
            stop();
        }
    }
}

void DeplacementControl::testRun(int durationSeconds)
{
    double testmTargetLength = mMotorSpeed * durationSeconds;
    //std::cout << "Starting test run for " << durationSeconds << " seconds..." << std::endl;
    
    goForward(testmTargetLength);
    std::this_thread::sleep_for(std::chrono::seconds(durationSeconds));
    stop();
}

////////////////////
/// Odometry FCT ///
////////////////////
int DeplacementControl::getDelta()
{
    signed long long delta_left = mDrive.left.position - mDrive.left.previous;
    signed long long delta_right = mDrive.right.position - mDrive.right.previous;
    int delta = abs (delta_left - delta_right);
}
