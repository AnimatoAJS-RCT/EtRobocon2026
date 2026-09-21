#include "RotateTracer.h"

#include "Log.h"

#include <algorithm>
#include <cstdlib>

RotateTracer::RotateTracer(Walker* walker, int direction, int angleDeg, int pwm)
    : mWalker(walker),
      mDirection(direction),
    mRequestedAngleDeg(std::abs(angleDeg)),
    mTargetTurnWdeg(0),
      mPwm(std::abs(pwm)),
      mStartLeftCount(0),
    mStartRightCount(0),
    mBrakeCountdown(0),
    mIsBraking(false)
{
    double scale = mDirection > 0 ? LEFT_TURN_SCALE : RIGHT_TURN_SCALE;
    int offset = mDirection > 0 ? LEFT_TURN_OFFSET_WDEG : RIGHT_TURN_OFFSET_WDEG;
    mTargetTurnWdeg = std::max(0, static_cast<int>(mRequestedAngleDeg
                                     * WHEEL_DEG_PER_BODY_DEG * scale)
                          + offset);
    mState = UNDEFINED;
}

void RotateTracer::run()
{
    switch(mState) {
        case UNDEFINED:
            mState = WAITING_FOR_START;
            break;
        case WAITING_FOR_START:
            if(mStarterList.empty()) {
                mStartLeftCount = mWalker->getLeftCount();
                mStartRightCount = mWalker->getRightCount();
                mWalker->beginEncoderCorrection();
                mIsBraking = false;
                mState = WALKING;
                LOGI("[ROTATE] start: direction=%s requested=%ddeg target=%d wheelDeg pwm=%d\n",
                     mDirection > 0 ? "LEFT" : "RIGHT", mRequestedAngleDeg,
                     mTargetTurnWdeg, mPwm);
                return;
            }
            for(auto starter : mStarterList) {
                if(starter->isPushed()) {
                    mStartLeftCount = mWalker->getLeftCount();
                    mStartRightCount = mWalker->getRightCount();
                    mWalker->beginEncoderCorrection();
                    mIsBraking = false;
                    mState = WALKING;
                    LOGI("[ROTATE] start: direction=%s requested=%ddeg target=%d wheelDeg pwm=%d\n",
                        mDirection > 0 ? "LEFT" : "RIGHT", mRequestedAngleDeg,
                         mTargetTurnWdeg, mPwm);
                    return;
                }
            }
            break;
        case WALKING: {
            if(mIsBraking) {
                mBrakeCountdown--;
                if(mBrakeCountdown <= 0) {
                    mWalker->stop();
                    mState = TERMINATED;
                    LOGI("[ROTATE] completed: direction=%s target=%d wheelDeg moved=%d wheelDeg\n",
                         mDirection > 0 ? "LEFT" : "RIGHT", mTargetTurnWdeg,
                         mDirection * getTurnWdeg());
                    return;
                }
                mWalker->brake();
                mWalker->setPwm(0, 0);
                return;
            }

            int current = mDirection * getTurnWdeg();
            int remaining = mTargetTurnWdeg - current;
            if(std::abs(remaining) <= TURN_TOLERANCE) {
                mWalker->brake();
                mWalker->setPwm(0, 0);
                mBrakeCountdown = BRAKE_TICKS;
                mIsBraking = true;
                LOGI("[ROTATE] turn complete: direction=%s target=%d actual=%d error=%d, entering brake\n",
                     mDirection > 0 ? "LEFT" : "RIGHT", mTargetTurnWdeg,
                     current, mTargetTurnWdeg - current);
                return;
            }

            int turnPwm = std::abs(remaining) <= APPROACH_WINDOW
                ? std::min(mPwm, APPROACH_PWM) : mPwm;
            int turnDirection = remaining > 0 ? mDirection : -mDirection;
            mWalker->setPwm(-turnDirection * turnPwm, turnDirection * turnPwm);
            mWalker->runWithEncoderCorrection(-turnDirection * turnPwm,
                                              turnDirection * turnPwm);
            break;
        }
        case TERMINATED:
        default:
            break;
    }
}

int RotateTracer::getTurnWdeg() const
{
    int leftDelta = mWalker->getLeftCount() - mStartLeftCount;
    int rightDelta = mWalker->getRightCount() - mStartRightCount;
    return (rightDelta - leftDelta) / 2;
}