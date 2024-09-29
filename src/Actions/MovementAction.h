
#pragma once

#include "Actions.h"

class MovementAction : public Action
{
    public:
    
    signed long mSpeedRight;
    signed long mSpeedLeft;

    // represent the postition to reach, is relative to the PAMI,
    // the PAMI should be calibrated to be "Absolute"
    signed long mPosX = 0; 
    signed long mPosY = 0;

    /// @brief these variable are used for the old way of just moving "blindly"
    // we track the motor position to know what we do without knowing where we are
    signed long mLegacyMotorPosRight;
    signed long mLegacyMotorPosLeft;

    public :
    MovementAction();
    MovementAction(const MovementAction& ma);
    MovementAction(signed long speedR,
                               signed long speedL,
                               signed long posX,
                               signed long posY,
                               signed long motorPosR = 0,
                               signed long motorPosL = 0);
    virtual ~MovementAction();
    virtual TYPE_OF_ACTION getTypeAction() const override;
    virtual bool isActionValid(void *arg) const override;

    void printValue() const;

    /// @brief Position that will be reached
    /// @param posX 
    /// @param posY 
    void setMoveTo(signed long posX, signed long posY);
    void setMotorPosTo(signed long posRight, signed long posLeft);

    /// @brief speed in percentage of the motor capacities (between 0% and 100%)
    //void setSpeed(char percentage) { /* should do a calcul, will come later */}

    static signed long getWheelSpeed_Left_ForPami();
    static signed long getWheelSpeed_Right_ForPami();
    
    ///////////////////////////
    /// Simple basic Action ///
    //////////////////////////
    static std::shared_ptr<Action> createActionGoForward(float centimeterToRun);
    static std::shared_ptr<Action> createActionGoBackward();
    static std::shared_ptr<Action> createActionTurn90Right();
    static std::shared_ptr<Action> createActionTurn90Left();
    static std::shared_ptr<Action> createActionTurn180();


};