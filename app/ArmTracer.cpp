#include "ArmTracer.h"

#include "Log.h"

ArmTracer::ArmTracer(spikeapi::Motor* armMotor, int armPwm, int direction, int targetAngle)
    : mArmMotor(armMotor),
      mArmPwm(armPwm),
            mDirection(direction),
            mTargetAngle(targetAngle),
    mStartCount(0),
      mIsInitialized(false)
{
    mState = UNDEFINED;
}

void ArmTracer::run()
{
    switch(mState) {
        case UNDEFINED:
            if(!mIsInitialized) {
                mStartCount = mArmMotor->getCount();
                mIsInitialized = true;
            }
            mState = WAITING_FOR_START;
            break;
        case WAITING_FOR_START:
            if(mStarterList.empty()) {
                for(auto terminator : mTerminatorList) {
                    terminator->init();
                }
                mState = WALKING;
                return;
            }
            for(auto starter : mStarterList) {
                if(starter->isPushed()) {
                    for(auto terminator : mTerminatorList) {
                        terminator->init();
                    }
                    mState = WALKING;
                    return;
                }
            }
            break;
        case WALKING: {
            mArmMotor->setPower(mDirection * mArmPwm);

            for(auto terminator : mTerminatorList) {
                if(terminator->isToBeTerminate()) {
                    mArmMotor->brake();
                    mState = TERMINATED;
                    LOGI("[ARM] terminated by terminator (pwm=%d, angle=%ddeg)\n", mArmPwm,
                         mArmMotor->getCount() - mStartCount);
                    return;
                }
            }

              int movedAngle = mArmMotor->getCount() - mStartCount;
              int directedAngle = mDirection * movedAngle;
              bool isTargetReached = directedAngle >= mTargetAngle;
            if(isTargetReached) {
                mArmMotor->brake();
                mState = TERMINATED;
                 LOGI("[ARM] completed (pwm=%d, direction=%s, target=%ddeg, moved=%ddeg)\n",
                     mArmPwm, mDirection > 0 ? "DOWN" : "UP", mTargetAngle, movedAngle);
            } else if(mArmMotor->isStalled()) {
                mArmMotor->brake();
                mState = TERMINATED;
                 LOGI("[ARM] stalled (pwm=%d, direction=%s, target=%ddeg, moved=%ddeg)\n",
                     mArmPwm, mDirection > 0 ? "DOWN" : "UP", mTargetAngle, movedAngle);
            }
            break;
            }
        case TERMINATED:
            break;
        default:
            break;
    }
}