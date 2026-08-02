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
      mStartRightCount(0)
{
    double scale = mDirection > 0 ? RIGHT_TURN_SCALE : LEFT_TURN_SCALE;
    int offset = mDirection > 0 ? RIGHT_TURN_OFFSET_WDEG : LEFT_TURN_OFFSET_WDEG;
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
                mState = WALKING;
                LOGI("[ROTATE] start: direction=%s requested=%ddeg target=%d wheelDeg pwm=%d\n",
                     mDirection > 0 ? "RIGHT" : "LEFT", mRequestedAngleDeg,
                     mTargetTurnWdeg, mPwm);
                return;
            }
            for(auto starter : mStarterList) {
                if(starter->isPushed()) {
                    mStartLeftCount = mWalker->getLeftCount();
                    mStartRightCount = mWalker->getRightCount();
                    mState = WALKING;
                    LOGI("[ROTATE] start: direction=%s requested=%ddeg target=%d wheelDeg pwm=%d\n",
                         mDirection > 0 ? "RIGHT" : "LEFT", mRequestedAngleDeg,
                         mTargetTurnWdeg, mPwm);
                    return;
                }
            }
            break;
        case WALKING:
            if(mDirection * getTurnWdeg() >= mTargetTurnWdeg) {
                mWalker->brake();
                mState = TERMINATED;
                LOGI("[ROTATE] completed: direction=%s target=%d wheelDeg moved=%d wheelDeg\n",
                     mDirection > 0 ? "RIGHT" : "LEFT", mTargetTurnWdeg, getTurnWdeg());
                return;
            }
            mWalker->setPwm(-mDirection * mPwm, mDirection * mPwm);
            mWalker->run();
            break;
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