#include "ScenarioTracer.h"
#include "Log.h"
#include <algorithm>  // for std::max, std::min

// 回転数差を補正するためのPゲイン
const double ScenarioTracer::Kp = 0.05;
// PWM補正値のクリッピング割合
const double ScenarioTracer::PWM_CORRECTION_LIMIT_RATIO = 0.2;

ScenarioTracer::ScenarioTracer(Walker* walker, int leftPwm, int rightPwm)
    : mWalker(walker),
        mLeftPwm(leftPwm),
        mRightPwm(rightPwm),
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
                mWalker->setPwm(mLeftPwm, mRightPwm);
                mState = WALKING;
                return;
            }
            for(auto starter : mStarterList) {
                if(starter->isPushed()) {
                    for(auto terminator : mTerminatorList) {
                        terminator->init();
                    }
                    mWalker->setPwm(mLeftPwm, mRightPwm);
                    mState = WALKING;
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
    // 目標のPWM比に従って回転数が追従するように補正をかける
    int leftCount = mWalker->getLeftCount();
    int rightCount = mWalker->getRightCount();

    // 目標回転数比と現在回転数比の誤差を計算
    // error = leftCount * mRightPwm - rightCount * mLeftPwm
    // この値が0になるように制御する
    double error = static_cast<double>(leftCount) * mRightPwm - static_cast<double>(rightCount) * mLeftPwm;

    // 補正量を計算 (P制御)
    double correction = Kp * error;

    // 補正後のPWM値を計算
    int correctedLeftPwm;
    int correctedRightPwm;

    // 前進・ほぼ直進と後退で補正の向きを変える
    if (mLeftPwm + mRightPwm >= 0) { // Forward
        correctedLeftPwm = mLeftPwm - static_cast<int>(correction);
        correctedRightPwm = mRightPwm + static_cast<int>(correction);
    } else { // Backward
        correctedLeftPwm = mLeftPwm + static_cast<int>(correction);
        correctedRightPwm = mRightPwm - static_cast<int>(correction);
    }

    // 補正後のPWM値が目標値から大きく外れないように、目標値の±10%の範囲にクリッピングする
    // 1. 左右それぞれの変動幅（マージン）を計算
    int leftMargin = static_cast<int>(std::abs(mLeftPwm) * PWM_CORRECTION_LIMIT_RATIO);
    int rightMargin = static_cast<int>(std::abs(mRightPwm) * PWM_CORRECTION_LIMIT_RATIO);

    // 2. 左右それぞれのPWM値の最小値と最大値を計算
    int minLeftPwm = mLeftPwm - leftMargin;
    int maxLeftPwm = mLeftPwm + leftMargin;
    int minRightPwm = mRightPwm - rightMargin;
    int maxRightPwm = mRightPwm + rightMargin;

    // 3. 計算した範囲内にクリッピング
    correctedLeftPwm = std::max(minLeftPwm, std::min(correctedLeftPwm, maxLeftPwm));
    correctedRightPwm = std::max(minRightPwm, std::min(correctedRightPwm, maxRightPwm));

    printf("Correction: L/R Cnt=%d/%d, Err=%.1f, Corr=%.1f, PWM L/R=%d/%d -> %d/%d\n",
           leftCount, rightCount, error, correction, mLeftPwm, mRightPwm, correctedLeftPwm, correctedRightPwm);

    mWalker->setPwm(correctedLeftPwm, correctedRightPwm);
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
