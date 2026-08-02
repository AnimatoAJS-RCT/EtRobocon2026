#include "ScenarioTracer.h"
#include "Log.h"

ScenarioTracer::ScenarioTracer(Walker* walker, int leftPwm, int rightPwm)
    : mWalker(walker),
      mLeftPwm(leftPwm),
      mRightPwm(rightPwm),
      mStartLeftCount(0),
      mStartRightCount(0),
      mIsInitialized(false),
      mLastLoggedState(-1)
{
    mState = UNDEFINED;
}

void ScenarioTracer::run()
{
    if(mLastLoggedState != mState) {
        LOGD("[SCENARIO] state %d -> %d\n", mLastLoggedState, mState);
        mLastLoggedState = mState;
    }

    switch(mState) {
        case UNDEFINED:
            mIsInitialized = true;
            mState = WAITING_FOR_START;
            break;
        case WAITING_FOR_START:
            if(mStarterList.empty()) {
                for(auto terminator : mTerminatorList) terminator->init();
                startWalking();
                return;
            }
            for(auto starter : mStarterList) {
                if(starter->isPushed()) {
                    for(auto terminator : mTerminatorList) terminator->init();
                    startWalking();
                    return;
                }
            }
            break;
        case WALKING:
            execWalking();
            break;
        case TERMINATED:
        default:
            break;
    }
}

void ScenarioTracer::execWalking()
{
    mWalker->runStraight(mLeftPwm, mRightPwm);
    for(auto terminator : mTerminatorList) {
        if(terminator->isToBeTerminate()) {
            mWalker->stop();
            mState = TERMINATED;
            LOGI("Stop: LC=%d, RC=%d\n", mWalker->getLeftCount(), mWalker->getRightCount());
            return;
        }
    }
}

void ScenarioTracer::startWalking()
{
    mStartLeftCount = mWalker->getLeftCount();
    mStartRightCount = mWalker->getRightCount();
    mWalker->beginStraightControl();
    mState = WALKING;
}
