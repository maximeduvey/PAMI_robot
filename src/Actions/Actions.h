#pragma once

#include "motor_BLMS4005v3.h"
#include <mutex>
#include <atomic>
#include <memory>

/// @brief this class represent the action / objectif the PAMI is trying to achieve
/// we just want to stack different objectif and let's the class manage it's reached state
/// not depending of the traduction we have to make to correctly understand it.
/// (for example a full turn is 36000 on the wheel position, and it "should" move up to 12-13cm)
/// in my strategic i just want to give a list of objective not some drive specific things
class Action {
    public :
    enum TYPE_OF_ACTION {
        NONE = 0, // happen when a list of action have depleted all action it has to do
        MOVEMENT,
        ARM_ACTION,
        HIBRYD
    };

    Action() {mToBeSkipped.store(false);isValid.store(false);}
    Action(const Action &action) {mToBeSkipped.store(action.mToBeSkipped.load());isValid.store(action.mToBeSkipped.load());}
    virtual ~Action() {}

    /// @brief represent if we should skipp this task
    /// can happen when the brain see us stuck too long on a task and want to change
    std::atomic<bool> mToBeSkipped;
    
    /// @brief is the task has been achieved
    std::atomic<bool> isValid;

    /// PURE FUNCTION ///
    virtual TYPE_OF_ACTION getTypeAction() const = 0;
    virtual bool isActionValid(void *arg) const = 0;

    void setToSkip(bool val){mToBeSkipped.store(val);}
    void setIsValid(bool val){isValid.store(val);}
    bool getToSkip(){ return mToBeSkipped.load();}
    bool getIsValid(){return isValid.load();}

};
class ActionNone : public Action { // should not do anything
    public:
    ActionNone():Action(){}
    ActionNone(const ActionNone &e):Action(e){}
    virtual ~ActionNone(){}

        virtual Action::TYPE_OF_ACTION getTypeAction() const override { return TYPE_OF_ACTION::NONE; }
        virtual bool isActionValid(void *arg) const override {return true;} 
};
