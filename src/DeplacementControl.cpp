
#include "DeplacementControl.h"

#include <thread>
#include <stdexcept>
#include <stdexcept>
#include "MovementAction.h"

DeplacementControl::DeplacementControl()
    : mId(0), mMotorSpeed(0), mLength(0.0), mTargetLength(0.0)
{
    printf("DeplacementControl::DeplacementControl()\n");
    mDrive = new DRIVE();
    mState.store(DeplacementControlState::DCS_IDLE);
    mEnd.store(true);
    mIsRunning.store(false);
}

DeplacementControl::~DeplacementControl()
{
}

void DeplacementControl::stop()
{
    printf("DeplacementControl::STOPPING()\n");
    mEnd.store(true);
    mIsRunning.store(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    exit(1);
}

/// @brief Is called before starting to move, it set motor to a ready state and set spee
void DeplacementControl::setReady()
{
    printf("DeplacementControl::setReady()\n");
    // mDrive->motors_off();
    // std::this_thread::sleep_for(std::chrono::milliseconds(100));
    printf("DeplacementControl::setReady(A)\n");

    if (mDrive->getDriveState() != DeplacementControl::DCS_RUNNING)
    {
        mDrive->motors_on();
        printf("DeplacementControl::setReady(B)\n");
    }
    getDefaultMotorSpeed(mDrive->left.speed, mDrive->right.speed);
    printf("DeplacementControl::setReady() Ready\n");
}

void DeplacementControl::goForward(int lenght)
{
    // printf("DeplacementControl::goForward(%d)\n", mIsRunning.load());
    this->mTargetLength = mTargetLength;
    mLength = 0.0;
    if (mIsRunning.load() != true)
    {
        mIsRunning.store(true);
        setReady();
        std::thread(&DeplacementControl::loop_motor_drive, this).detach();
        printf("DeplacementControl::goForward(%d)\n", mIsRunning.load());
    }
    else
    {
        // printf("DeplacementControl::goForward(Already started)\n");
    }
}

/// @brief  At it's date of creation this function is just:
/// forward, turn to the left, forward (touch flower)
void DeplacementControl::startStrat_simpleLinesToFlower()
{
}

void DeplacementControl::LoopManageMotor()
{
    printf("DeplacementControl::LoopManageMotor()\n");
    auto startTime = std::chrono::high_resolution_clock::now();
    while (mDrive->getDriveState() == DRIVE::DRIVE_STATE::DRIVE_RUNNING &&
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

/* void DeplacementControl::testRun(int durationSeconds)
{
    double testmTargetLength = mMotorSpeed * durationSeconds;
    // std::cout << "Starting test run for " << durationSeconds << " seconds..." << std::endl;

    goForward(testmTargetLength);
    std::this_thread::sleep_for(std::chrono::seconds(durationSeconds));
    stop();
} */

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
    delta_left = mDrive->left.position - mDrive->left.previous;
    delta_right = mDrive->right.position - mDrive->right.previous;
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
    printf("DeplacementControl::loop_motor_drive(%d) will try to do %d actions\n", this->mId, mObjectivesAction.size());
    int nbr = 20;
    int current = 0;
    mEnd.store(false);

    // (action = getNextAction()).get()->getTypeAction() != ACTION::TYPE_OF_ACTION::NONE )
    while (mEnd.load() == false &&
           mObjectivesAction.size() > 0)
    {
        std::shared_ptr<Action> action = getNextAction();
        ( (MovementAction*) action.get() )->printValue();
        signed long long delta_left = 0;
        signed long long delta_right = 0;

        while (mEnd.load() == false &&
               action.get()->getTypeAction() != Action::TYPE_OF_ACTION::NONE &&
               action.get()->getToSkip() != true &&
               action.get()->isActionValid(mDrive) == false)
        {
            MovementAction* mvAction = (MovementAction*) action.get();
            mDrive->speed(mvAction->mSpeedLeft, mvAction->mSpeedRight);
            mDrive->move(mvAction->mLegacyMotorPosLeft, mvAction->mLegacyMotorPosRight);
            /*         getDelta(delta_left, delta_right);
                    getDirectionFactor(delta_left, delta_right);

                    int correction = 5; // 5 is a known-good value
                    if (delta_left > delta_right)
                    {
                        mDrive->left.speed -= correction;
                        mDrive->right.speed += correction;
                    }
                    else if (delta_left < delta_right)
                    {
                        mDrive->left.speed += correction;
                        mDrive->right.speed -= correction;
                    } */
            //mDrive->move(DEFAULT_ONE_FULL_TURN_RPM, DEFAULT_ONE_FULL_TURN_RPM);
            mDrive->task();
            printf("DeplacementControl::loop_motor_drive(A)\n");

            std::this_thread::sleep_for(std::chrono::milliseconds(TIME_ONE_TURN_ACTION_MILISEC));
            if (++current > nbr)
            {
                mEnd.store(true);
                printf("DeplacementControl::loop_motor_drive(end reached)\n");
            }
        }
        validateAction();
    }
    printf("DeplacementControl::loop_motor_drive(quitting)\n");
    mIsRunning.store(false);
}

/// @brief This function will set the arbitrary default value for the motor speed depending which PAMI you are
/// During test it was seen that similar motor were not reacting the same to the same value
/// after many test arbitrary value was decided to be a good reference point to have the same movement
/// @param left_motor
/// @param right_motor
void DeplacementControl::getDefaultMotorSpeed(signed long &left_motor, signed long &right_motor)
{
    printf("DeplacementControl::getDefaultMotorSpeed(%d)\n", mId);
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

/// @brief  This set drive is to have the same Object between different part
/// it's mainly for the transition perido where we have both the old Pami logic to directly pilot the drive
/// and the DeplacementControl taking over
/// @param drive
void DeplacementControl::setDrive(DRIVE *drive)
{
    mDrive = drive;
}

void DeplacementControl::setId(int id)
{
    mId = id;
    printf("DeplacementControl::setId(%d-%d)\n", id, mId);
}

//////////////////////
/// ACTION RELATED ///
//////////////////////
std::shared_ptr<Action> DeplacementControl::getNextAction()
{
    std::lock_guard<std::mutex> lock(mMutexObjectiveAction);
    return (mObjectivesAction.size() > 0 ? mObjectivesAction[0] : std::make_shared<ActionNone>());
}

void DeplacementControl::addAction(std::shared_ptr<Action> action)
{
    std::lock_guard<std::mutex> lock(mMutexObjectiveAction);
    mObjectivesAction.push_back(action);
}

void DeplacementControl::addPriorityAction(std::shared_ptr<Action> action)
{
    std::lock_guard<std::mutex> lock(mMutexObjectiveAction);
    throw std::runtime_error("Not Implemented yet!");
}

void DeplacementControl::validateAction()
{
    std::lock_guard<std::mutex> lock(mMutexObjectiveAction);
    if (mObjectivesAction.size() < 0 )
        return;
    // shoud log / count point action as successed
    mObjectivesAction.erase(mObjectivesAction.begin());
}

void DeplacementControl::skipAction()
{
    std::lock_guard<std::mutex> lock(mMutexObjectiveAction);
    // shoud do something
    mObjectivesAction.erase(mObjectivesAction.begin());
}

void DeplacementControl::compomiseCurrentImpossibleAction()
{
    std::lock_guard<std::mutex> lock(mMutexObjectiveAction);
    throw std::runtime_error("Not Implemented yet!");
}

void DeplacementControl::clearAction()
{
    printf("DeplacementControl::clearAction()\n");
    auto action = getNextAction();
    std::lock_guard<std::mutex> lock(mMutexObjectiveAction);
    mObjectivesAction.clear();
    if (action.get()->getTypeAction() != Action::TYPE_OF_ACTION::NONE){
        action.get()->setToSkip(true);
    }
}

