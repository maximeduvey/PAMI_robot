#include "MovementAction.h"
#include "drive.hpp"

#include "sharedInfos.h"

MovementAction::MovementAction() : mSpeedLeft(DEFAULT_MOTOR_SPEED_PAMI_ONE_WHEEL_RIGHT),
                                   mSpeedRight(DEFAULT_MOTOR_SPEED_PAMI_ONE_WHEEL_LEFT),
                                   mPosX(0),
                                   mPosY(0),
                                   mLegacyMotorPosRight(0),
                                   mLegacyMotorPosLeft(0)
{
}

MovementAction::MovementAction(signed long speedR,
                               signed long speedL,
                               signed long posX,
                               signed long posY,
                               signed long motorPosR /*  = 0 */,
                               signed long motorPosL /*  = 0 */) : mSpeedLeft(speedR),
                                                                   mSpeedRight(speedL),
                                                                   mPosX(posX),
                                                                   mPosY(posY),
                                                                   mLegacyMotorPosLeft(motorPosL),
                                                                   mLegacyMotorPosRight(motorPosR)
{
    printf("MovementAction::MovementAction() left:%ld / %ld, right:%ld / %ld \n",
           mPosX, mPosY,
           mLegacyMotorPosRight, mLegacyMotorPosLeft);
}

MovementAction::MovementAction(const MovementAction &ma) : mSpeedRight(ma.mSpeedRight),
                                                           mSpeedLeft(ma.mSpeedLeft),
                                                           mPosX(ma.mPosX),
                                                           mPosY(ma.mPosY),
                                                           mLegacyMotorPosLeft(ma.mLegacyMotorPosLeft),
                                                           mLegacyMotorPosRight(ma.mLegacyMotorPosRight)
{
    printf("MovementAction::MovementAction(copy) left:%ld / %ld, right:%ld / %ld \n",
           mPosX, mPosY,
           mLegacyMotorPosRight, mLegacyMotorPosLeft);
}

void MovementAction::printValue() const
{
    printf("MovementAction::printValue() posy:%ld / posY:%ld, right:%ld / left:%ld \n",
           mPosX, mPosY,
           mLegacyMotorPosRight, mLegacyMotorPosLeft);
}

MovementAction::~MovementAction()
{
}
Action::TYPE_OF_ACTION MovementAction::getTypeAction() const
{
    return TYPE_OF_ACTION::MOVEMENT;
};

bool MovementAction::isActionValid(void *arg) const
{
    if (arg == nullptr)
    {
        printf("MovementAction::isActionValid(arg == nullptr)\n");
        return true;
    }
    DRIVE *dr = dynamic_cast<DRIVE *>(static_cast<DRIVE *>(arg)); //(DRIVE *)arg;//
    if (dr == nullptr)
    {
        throw std::runtime_error("MovementAction isActionValid Object is not of the desired type!");
    }

    // todo: legacy code !, should be changed for position
    if ((dr->left.position * mLegacyMotorPosLeft) > 0 &&
        (dr->right.position * mLegacyMotorPosRight) > 0 &&
        abs(dr->left.position - mLegacyMotorPosLeft) <= MOTOR_POSITION_TOLERANCE &&
        abs(dr->right.position - mLegacyMotorPosRight) <= MOTOR_POSITION_TOLERANCE)
    {
        printf("MovementAction::isActionValid(true)\n");
        return true;
    }

    printf("MovementAction::isActionValid(false) left:%lld / right:%lld, poleft:%ld / poright:%ld \n",
           dr->left.position,
           dr->right.position,
           mLegacyMotorPosLeft,
           mLegacyMotorPosRight);
    return false;
}

void MovementAction::setMoveTo(signed long posX, signed long posY)
{
    mPosX = posX;
    mPosY = mPosY;
}

void MovementAction::setMotorPosTo(signed long posRight, signed long posLeft)
{
    mLegacyMotorPosRight = posRight;
    mLegacyMotorPosLeft = posLeft;
    printf("MovementAction::setMotorPosTo(left:%ld - right:%d)\n", mLegacyMotorPosLeft, mLegacyMotorPosRight);
}

//////////////
/// Static ///
//////////////
std::shared_ptr<Action> MovementAction::createActionGoForward(float centimeterToRun)
{
    long long distance = (centimeterToRun * (MINIMETER_PER_TURN / 100) * DEFAULT_ONE_FULL_TURN_RPM);
    printf("MovementAction::createActionGoForward(%lld) %f, %d\n", distance, centimeterToRun, DEFAULT_ONE_FULL_TURN_RPM);
    return std::make_shared<MovementAction>(getWheelSpeed_Left_ForPami(),
                                            getWheelSpeed_Right_ForPami(),
                                            0, 0, distance, distance);
}

std::shared_ptr<Action> MovementAction::createActionGoBackward()
{
    printf("MovementAction::createActionGoForward()\n");
    return std::make_shared<MovementAction>(getWheelSpeed_Left_ForPami(),
                                            getWheelSpeed_Right_ForPami(),
                                            0, 0, -DEFAULT_ONE_FULL_TURN_RPM, -DEFAULT_ONE_FULL_TURN_RPM);
}

std::shared_ptr<Action> MovementAction::createActionTurn90Right()
{
    printf("MovementAction::createActionTurn90Right()\n");
    return std::make_shared<MovementAction>(getWheelSpeed_Left_ForPami(),
                                            getWheelSpeed_Right_ForPami(),
                                            0, 0, -(DEFAULT_ONE_FULL_TURN_RPM / 4), DEFAULT_ONE_FULL_TURN_RPM);
}

std::shared_ptr<Action> MovementAction::createActionTurn90Left()
{
    printf("MovementAction::createActionTurn90Left()\n");
    return std::make_shared<MovementAction>(getWheelSpeed_Left_ForPami(),
                                            getWheelSpeed_Right_ForPami(),
                                            0, 0, DEFAULT_ONE_FULL_TURN_RPM, -(DEFAULT_ONE_FULL_TURN_RPM / 4));
}

std::shared_ptr<Action> MovementAction::createActionTurn180()
{
}

/// @brief return the basic speed for the wheel left for a PAMI by id
/// @return
signed long MovementAction::getWheelSpeed_Left_ForPami()
{
    switch (PAMI_ID)
    {
    case 1:
        return DEFAULT_MOTOR_SPEED_PAMI_ONE_WHEEL_LEFT;
        break;
    case 2:
        return DEFAULT_MOTOR_SPEED_PAMI_TWO_WHEEL_LEFT;
        break;
    case 3:
        return DEFAULT_MOTOR_SPEED_PAMI_THREE_WHEEL_LEFT;
        break;
    default:
        return 0;
        break;
    }
}

signed long MovementAction::getWheelSpeed_Right_ForPami()
{
    switch (PAMI_ID)
    {
    case 1:
        return DEFAULT_MOTOR_SPEED_PAMI_ONE_WHEEL_RIGHT;
        break;
    case 2:
        return DEFAULT_MOTOR_SPEED_PAMI_TWO_WHEEL_RIGHT;
        break;
    case 3:
        return DEFAULT_MOTOR_SPEED_PAMI_THREE_WHEEL_RIGHT;
        break;
    default:
        return 0;
        break;
    }
}
