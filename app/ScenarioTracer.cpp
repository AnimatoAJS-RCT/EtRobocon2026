#include "ScenarioTracer.h"
#include "Log.h"

// 旧来の LineTracer と同じく、シンプルな直進制御を採用する。
// ここでは回転数差の補正を行わず、目標PWMをそのまま適用する。
const double ScenarioTracer::Kp = 0.02;
const double ScenarioTracer::PWM_CORRECTION_LIMIT_RATIO = 0.2;

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
            if(!mIsInitialized) {
                mIsInitialized = true;
            }
            mState = WAITING_FOR_START;
            break;
        case WAITING_FOR_START:
            if(mStarterList.empty()) {
                for(auto terminator : mTerminatorList) {
                    terminator->init();
                }
                startWalking();
                return;
            }
            for(auto starter : mStarterList) {
                if(starter->isPushed()) {
                    for(auto terminator : mTerminatorList) {
                        terminator->init();
                    }
                    startWalking();
                    return;
                }
            }
            break;
        case WALKING:
            execWalking();
            break;
        case TERMINATED:
            // Do nothing
            break;
        default:
            break;
    }
}

void ScenarioTracer::execWalking()
{
    // 旧来の LineTracer 同様、開始時の基準値を記録してから
    // 目標PWMをそのまま適用して直進する。
    mWalker->setPwm(mLeftPwm, mRightPwm);
    mWalker->run();

    for(auto terminator : mTerminatorList) {
        if(terminator->isToBeTerminate()) {
            mWalker->stop();
            mState = TERMINATED;
            int l = mWalker->getLeftCount();
            int r = mWalker->getRightCount();
            LOGI("Stop: LC=%d, RC=%d\n", l, r);
            return;
        }
    }
}

void ScenarioTracer::startWalking()
{
    mStartLeftCount = mWalker->getLeftCount();
    mStartRightCount = mWalker->getRightCount();
    mWalker->setPwm(mLeftPwm, mRightPwm);
    mState = WALKING;
}
