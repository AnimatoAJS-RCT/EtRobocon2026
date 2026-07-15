#include "ArmTracer.h"

#include "Log.h"

ArmTracer::ArmTracer(spikeapi::Motor* armMotor, int armPwm, int targetAngle)
    : mArmMotor(armMotor),
      mArmPwm(armPwm),
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
            mArmMotor->setPower(mArmPwm);

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
            bool isTargetReached = (mTargetAngle >= 0) ? (movedAngle >= mTargetAngle)
                                                       : (movedAngle <= mTargetAngle);
            if(isTargetReached) {
                mArmMotor->brake();
                mState = TERMINATED;
                LOGI("[ARM] completed (pwm=%d, target=%ddeg, moved=%ddeg)\n", mArmPwm,
                     mTargetAngle, movedAngle);
            } else if(mArmMotor->isStalled()) {
                mArmMotor->brake();
                mState = TERMINATED;
                LOGI("[ARM] stalled (pwm=%d, target=%ddeg, moved=%ddeg)\n", mArmPwm,
                     mTargetAngle, movedAngle);
            }
            break;
            }
        case TERMINATED:
            break;
        default:
            break;
    }
}