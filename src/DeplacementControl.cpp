
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
    mIsStopped.store(false);
}

DeplacementControl::~DeplacementControl()
{
}

void DeplacementControl::startRunning()
{
    // printf("DeplacementControl::goForward(%d)\n", mIsRunning.load());
    if (mIsRunning.load() != true)
    {
        setReady();
        std::thread(&DeplacementControl::loop_motor_drive, this).detach();
        printf("DeplacementControl::goForward(%d)\n", mIsRunning.load());
    }
    else
    {
        // printf("DeplacementControl::goForward(Already started)\n");
    }
}

/// @brief sto but does not end !
/// used for example when you are detecting an obstacle
void DeplacementControl::stop()
{
    // printf("DeplacementControl::STOPPING()\n");
    mDrive->motors_off();
    mIsStopped.store(true);
}

void DeplacementControl::resume()
{
    // printf("DeplacementControl::resume()\n");
    std::lock_guard<std::mutex> lock(mMutexObjectiveAction);
    if (mObjectivesAction.size() > 0)
    {
        mDrive->motors_on();
    }
    mIsStopped.store(false);
}

void DeplacementControl::end()
{
    mDrive->motors_off();
    mIsRunning.store(false);
    mIsStopped.store(true);
    mEnd.store(true);
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
    mIsStopped.store(false);
}

/// @brief  At it's date of creation this function is just:
/// forward, turn to the left, forward (touch flower)
void DeplacementControl::startStrat_simpleLinesToFlower()
{
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
    delta_left = mDrive->left.position - mDrive->left.previous;
    delta_right = mDrive->right.position - mDrive->right.previous;
    int delta = delta_left - delta_right;
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
    mIsRunning.store(true);
    int nbr = 20;
    int current = 0;
    mEnd.store(false);

    // (action = getNextAction()).get()->getTypeAction() != ACTION::TYPE_OF_ACTION::NONE )
    while (mEnd.load() == false &&
           mObjectivesAction.size() > 0)
    {
        std::shared_ptr<Action> action = getNextAction();
        ((MovementAction *)action.get())->printValue();
        signed long long delta_left = 0;
        signed long long delta_right = 0;

        while (mEnd.load() == false &&
               action.get()->getTypeAction() != Action::TYPE_OF_ACTION::NONE &&
               action.get()->getToSkip() != true &&
               action.get()->isActionValid(mDrive) == false)
        {

            printf("DeplacementControl::loop_motor_drive(%d) \n", mIsStopped.load());
            if (mIsStopped.load() != true)
            {

                MovementAction *mvAction = (MovementAction *)action.get();
                int delta = getDelta(delta_left, delta_right);
                getDirectionFactor(delta_left, delta_right);

                int corection_speed_right = 0, corection_speed_left = 0;
                if (abs(delta) > MOTOR_POSITION_TOLERANCE)
                {
                    if (delta > 0) // left move faster
                    {
                        mvAction->mSpeedRight += SPEED_STRENGHT_CORRECTION;
                        mvAction->mSpeedLeft += -SPEED_STRENGHT_CORRECTION;
                    }
                    else // right move faster
                    {
                        mvAction->mSpeedRight += -SPEED_STRENGHT_CORRECTION;
                        mvAction->mSpeedLeft += SPEED_STRENGHT_CORRECTION;
                    }
                    printf("DeplacementControl::loop_motor_drive() speed corection was applied, left:%d, right:%d\n",
                           corection_speed_left, corection_speed_right);
                }

                mDrive->speed(mvAction->mSpeedLeft + corection_speed_left, mvAction->mSpeedRight + corection_speed_right);
                mDrive->move(mvAction->mLegacyMotorPosLeft, mvAction->mLegacyMotorPosRight);
                mDrive->task();
                printf("DeplacementControl::loop_motor_drive(A)\n");
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
    printf("DeplacementControl::validateAction() !!!\n");
    std::lock_guard<std::mutex> lock(mMutexObjectiveAction);
    if (mObjectivesAction.size() < 0)
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
    if (action.get()->getTypeAction() != Action::TYPE_OF_ACTION::NONE)
    {
        action.get()->setToSkip(true);
    }
}

/// @brief start a calibration to find the best value for motor speed
void DeplacementControl::doCalibrationOnMotorSpeed()
{
    // printf("DeplacementControl::goForward(%d)\n", mIsRunning.load());
    if (mIsRunning.load() != true)
    {
        setReady();
        std::thread(&DeplacementControl::motorSpeedCalibration, this).detach();
        printf("DeplacementControl::goForward(%d)\n", mIsRunning.load());
    }
}

/// @brief  will run forward and regulary check it's motor position to see if theire is a delta (difference) between them
/// it will then modify regulary it's speed to try to make this difference diseapear;
void DeplacementControl::motorSpeedCalibration()
{
    printf("DeplacementControl::motorSpeedCalibration(%d) will try to do %d actions\n", this->mId, mObjectivesAction.size());
    mIsRunning.store(true);
    int nbr = 20;
    int current = 0;
    mEnd.store(false);

    bool calibrationDone = false;
    std::shared_ptr<Action> tmpAction = MovementAction::createActionGoForward(100);
    MovementAction *forwardAction = (MovementAction *)tmpAction.get();

    long long currentDelta = 0;
    long long currentCorectionLeft = 0;
    const long long turnR = forwardAction->mLegacyMotorPosRight;
    const long long turnL = forwardAction->mLegacyMotorPosLeft;

    int turnCount = 0;
    int isIncreasing = 0;

    while (mEnd.load() == false && calibrationDone == false)
    {
        /*         mDrive->motors_off();
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                mDrive->motors_on();
                std::this_thread::sleep_for(std::chrono::milliseconds(20)); */

        forwardAction->mLegacyMotorPosRight = turnR + (turnR * turnCount);
        forwardAction->mLegacyMotorPosLeft = turnL + (turnL * turnCount);
        mDrive->printfDriveInfos();
        printf("DeplacementControl::motorSpeedCalibration() Turn , %d,  left:%lld, right:%lld\n\n",
               turnCount, forwardAction->mLegacyMotorPosLeft, forwardAction->mLegacyMotorPosRight);
        ++turnCount;

        while (mEnd.load() == false && calibrationDone == false &&
               forwardAction->getTypeAction() != Action::TYPE_OF_ACTION::NONE &&
               forwardAction->getToSkip() != true &&
               forwardAction->isActionValid(mDrive) == false)
        {
            long long cR = 0, cL = 0;
            currentDelta = getDelta(cL, cR);
            if (abs(currentDelta) > MOTOR_POSITION_TOLERANCE)
            {
                if (currentDelta > 0) // left move faster
                {
                    if (isIncreasing == 0) // not set
                    {
                        isIncreasing = 1;
                    }
                    else if (isIncreasing == 2)
                    {
                        calibrationDone = true;
                    }
                    forwardAction->mSpeedLeft += -SPEED_STRENGHT_CORRECTION;
                    forwardAction->mSpeedRight += SPEED_STRENGHT_CORRECTION;
                }
                else // right move faster
                {
                    if (isIncreasing == 0) // not set
                    {
                        isIncreasing = 2;
                    }
                    else if (isIncreasing == 1)
                    {
                        calibrationDone = true;
                    }
                    forwardAction->mSpeedLeft += SPEED_STRENGHT_CORRECTION;
                    forwardAction->mSpeedRight += -SPEED_STRENGHT_CORRECTION;
                }
                printf("DeplacementControl::motorSpeedCalibration() speed corection was applied, %lld,  left:%ld, right:%ld\n",
                       currentDelta, forwardAction->mSpeedLeft, forwardAction->mSpeedRight);
            }

            mDrive->speed(forwardAction->mSpeedLeft, forwardAction->mSpeedRight);
            mDrive->move(forwardAction->mLegacyMotorPosLeft, forwardAction->mLegacyMotorPosRight);
            mDrive->task();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (currentDelta < MOTOR_POSITION_TOLERANCE * 2 || calibrationDone == true)
        {
            printf("DeplacementControl::motorSpeedCalibration(quitting) CALIBRATION  DONE, current good value is: Left:%ld, right:%ld for a value of \n",
                   forwardAction->mSpeedLeft, forwardAction->mSpeedRight, currentDelta);
            calibrationDone = true;
        }
    }

    exit(1);
}