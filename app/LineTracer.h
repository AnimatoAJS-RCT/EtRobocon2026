/******************************************************************************
 *  LineTracer.h (for SPIKE )
 *  Created on: 2025/01/05
 *  Definition of the Class LineTracer
 *  Author: Kazuhiro Kawachi
 *  Modifier: Yuki Tsuchitoi
 *  Copyright (c) 2025 Embedded Technology Software Design Robot Contest
 *****************************************************************************/

#ifndef ETTR_APP_LINETRACER_H_
#define ETTR_APP_LINETRACER_H_

#include "LineMonitor.h"
#include "Walker.h"
#include "Pid.h"
#include "Tracer.h"

class LineTracer : public Tracer
{
public:
    static const float Kp;
    static const int bias;

    /**
     * コンストラクタ
     * @param lineMonitor LineMonitor
     * @param walker Walker
     * @param targetBrightness 目標輝度
     * @param pwm PWM値
      * @param _isLeftEdge エッジの左右どちらを走るか(true:左エッジ, false:右エッジ)
     * @param _gain PIDゲイン
      * @param _stopColor 停止条件の色 //TODO:未実装
     */
    LineTracer(LineMonitor *lineMonitor,
               Walker *walker,
               int targetBrightness,
               int pwm,
               int maxPwm,
               bool _isLeftEdge,
               PidGain *_gain);

    void run();
    void setTargetBrightness(int targetBrightness);
    int getNormalizedTargetBrightness() const;

private:
    LineMonitor *mLineMonitor;
    Walker *mWalker;
    int mTargetBrightness;
    int mPwm;
    int mMaxPwm;
    int mNormalizedTargetBrightness; // iniファイルから読み込んだ目標輝度(0-100)
    bool mIsLeftEdge;
    PidGain *mPidGain;
    bool mIsInitialized;
    Pid mPid;

    float calcPropValue(int diffReflection);

    void execUndefined();
    void execWaitingForStart();
    void execWalking();
};

#endif // ETTR_APP_LINETRACER_H_