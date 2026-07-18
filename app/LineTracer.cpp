/******************************************************************************
 *  LineTracer.cpp (for SPIKE)
 *  Created on: 2025/01/05
 *  Implementation of the Class LineTracer
 *  Author: Kazuhiro Kawachi
 *  Modifier: Yuki Tsuchitoi
 *  Copyright (c) 2025 Embedded Technology Software Design Robot Contest
 *****************************************************************************/

#include "LineTracer.h"

// 定数宣言
const float LineTracer::Kp = 0.83;
const int LineTracer::bias = 0;

/**
 * コンストラクタ
 * @param lineMonitor     ライン判定
 * @param walker 走行
 */
LineTracer::LineTracer(LineMonitor* lineMonitor, Walker* walker, int targetBrightness, int pwm, int maxPwm,
                                             bool isLeftEdge, PidGain* pidGain)
  : mLineMonitor(lineMonitor),
    mWalker(walker),
    mTargetBrightness(targetBrightness),
    mPwm(pwm),
        mMaxPwm(maxPwm),
        mNormalizedTargetBrightness(targetBrightness),
    mIsLeftEdge(isLeftEdge),
    mPidGain(pidGain),
    mIsInitialized(false),
    mPid(pidGain, 2)
{
    if(pidGain->ki != 0.0) {
        mPid.setIntegralLimit(maxPwm / (pidGain->ki > 0.0 ? pidGain->ki : -pidGain->ki));
    }
}

/**
 * ライントレースする
 */
void LineTracer::run()
{
    switch(mState) {
        case UNDEFINED:
            execUndefined();
            break;
        case WAITING_FOR_START:
            execWaitingForStart();
            break;
        case WALKING:
            execWalking();
            break;
        case TERMINATED:
            // 何もしない
            break;
        default:
            break;
    }
}

void LineTracer::setTargetBrightness(int targetBrightness)
{
    mTargetBrightness = targetBrightness;
}

int LineTracer::getNormalizedTargetBrightness() const
{
    return mNormalizedTargetBrightness;
}

/**
 * 走行体の操作量を計算する
 * @param diffBrightness ラインから外れた度合い（ライン閾値との差）
 */
float LineTracer::calcPropValue(int diffBrightness)
{
    // float turn = LineTracer::Kp * diffBrightness + LineTracer::bias;
    double d = mPid.calculatePid(diffBrightness);
    float turn = d;
    return turn;
}

/**
 * 未定義状態の処理
 */
void LineTracer::execUndefined()
{
    if(mIsInitialized == false) {
        mIsInitialized = true;
    }
    mState = WAITING_FOR_START;
}

/**
 * 開始待ち状態の処理
 */
void LineTracer::execWaitingForStart()
{
    // 開始条件を満たしているか確認する
    if(mStarterList.empty()) {
        // 開始条件を満たした場合の処理
        for(auto terminator : mTerminatorList) {
            terminator->init();
        }
        mPid.reset();
        mLineMonitor->setThreshold(mTargetBrightness);
        mState = WALKING;
        return;
    }
    for(auto starter : mStarterList) {
        if(starter->isPushed()) {
            // 開始条件を満たした場合の処理
            for(auto terminator : mTerminatorList) {
                terminator->init();
            }
            mPid.reset();
            mLineMonitor->setThreshold(mTargetBrightness);
            mState = WALKING;
            return;
        }
    }
}

/**
 * 走行中状態の処理
 */
void LineTracer::execWalking()
{
    int diffReflection = mLineMonitor->calDiffReflection();

    // 走行体の操作量を計算する
    float turn = calcPropValue(diffReflection);
    if (mIsLeftEdge) {
        turn = -turn;
    }
    int leftPwm = mPwm - turn;
    int rightPwm = mPwm + turn;
    if(leftPwm > mMaxPwm) leftPwm = mMaxPwm;
    if(leftPwm < -mMaxPwm) leftPwm = -mMaxPwm;
    if(rightPwm > mMaxPwm) rightPwm = mMaxPwm;
    if(rightPwm < -mMaxPwm) rightPwm = -mMaxPwm;
    mWalker->setPwm(leftPwm, rightPwm);

    // 走行を行う
    mWalker->run();

    // 終了条件を満たしているか確認する
    for(auto terminator : mTerminatorList) {
        if(terminator->isToBeTerminate()) {
            // 終了条件を満たした場合の処理
            mWalker->stop();
            mState = TERMINATED;
            return;
        }
    }
}