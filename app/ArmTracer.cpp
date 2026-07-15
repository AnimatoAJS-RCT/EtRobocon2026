#include "ArmTracer.h"

#include "Log.h"

ArmTracer::ArmTracer(spikeapi::Motor* armMotor, int armPwm, int durationMs)
    : mArmMotor(armMotor),
      mArmPwm(armPwm),
      mDurationMs(durationMs > 0 ? durationMs : 0),
      mElapsedMs(0),
      mIsInitialized(false)
{
    mState = UNDEFINED;
}

void ArmTracer::run()
{
    switch(mState) {
        case UNDEFINED:
            if(!mIsInitialized) {
                mElapsedMs = 0;
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
        case WALKING:
            mArmMotor->setPower(mArmPwm);
            mElapsedMs += LOOP_INTERVAL_MS;

            for(auto terminator : mTerminatorList) {
                if(terminator->isToBeTerminate()) {
                    mArmMotor->stop();
                    mState = TERMINATED;
                    LOGI("[ARM] terminated by terminator (pwm=%d, elapsed=%dms)\n", mArmPwm,
                         mElapsedMs);
                    return;
                }
            }

            if(mElapsedMs >= mDurationMs) {
                mArmMotor->stop();
                mState = TERMINATED;
                LOGI("[ARM] completed (pwm=%d, duration=%dms)\n", mArmPwm, mDurationMs);
            }
            break;
        case TERMINATED:
            break;
        default:
            break;
    }
}