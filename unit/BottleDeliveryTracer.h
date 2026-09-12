#ifndef ETTR_APP_BOTTLEDELIVERYTRACER_H_
#define ETTR_APP_BOTTLEDELIVERYTRACER_H_

#include "Tracer.h"
#include "Walker.h"
#include "LineMonitor.h"
#include "Pid.h"
#include "ColorSensor.h"
#include "UltrasonicSensor.h"
#include "Motor.h"
#include "DistanceTerminator.h"
#include "Util.h"
#include "LineTracer.h"

class BottleDeliveryTracer : public Tracer {
public:
    BottleDeliveryTracer(Walker* walker, // 左右モーターを制御する走行体
                         LineMonitor* lineMonitor, // ライン反射量を測定するモニター
                         spikeapi::UltrasonicSensor* ultrasonicSensor, // 距離センサー
                         spikeapi::ColorSensor* colorSensor, // ボトルや床の色を検出するセンサー
                         spikeapi::Motor* armMotor, // ボトルを掴むアームのモーター
                         double targetDistance, // 目標走行距離
                         int targetBrightness, // ライン判定に使う反射輝度のしきい値
                         int pwm, // 通常走行時の基本PWM
                         int maxPwm, // 左右モーターに設定できるPWMの上限
                         bool isLeftEdge, // 左エッジ走行ならtrue
                         double kp, // PID制御の比例ゲイン
                         double ki, // PID制御の積分ゲイン
                         double kd); // PID制御の微分ゲイン
    ~BottleDeliveryTracer();

    void run() override;

private:
    enum DetectedColor {
        COLOR_UNKNOWN, // 色をまだ判定できない
        COLOR_RED, // 赤色
        COLOR_BLUE, // 青色
        COLOR_YELLOW, // 黄色
        COLOR_WHITE, // 白色
        COLOR_BLACK // 黒色
    };

    enum Stage {
        STAGE_APPROACH_BOTTLE,
        STAGE_FORWARD_TO_BLACK_BEFORE_COLOR_CHECK,
        STAGE_ARM_UP,
        STAGE_COLOR_CHECK,
        STAGE_FORWARD_1CM_FOR_COLOR_RETRY,
        STAGE_BACKWARD_1CM_AFTER_COLOR_CHECK,
        STAGE_ARM_DOWN,
        STAGE_BLUE_LINE_COUNT,
        STAGE_FIRST_BLUE_FORWARD,
        STAGE_BLUE_LINE_HIGH_SPEED,
        STAGE_BLUE_LINE_STRONG_TRACE,
        STAGE_TURN_TO_TARGET,
        STAGE_FORWARD_TO_COLOR,
        STAGE_BACK_TO_LINE,
        STAGE_TURN_TO_RETURN,
        STAGE_RETURN_PATH,
        STAGE_DONE
    };

    Walker* mWalker; // 左右モーターを制御する走行体
    LineMonitor* mLineMonitor; // ライン反射量の計測・PID入力を担当
    spikeapi::ColorSensor* mColorSensor; // 色センサー
    spikeapi::Motor* mArmMotor; // アームモーター
    int mTargetBrightness; // ライン判定の基準輝度
    int mPwm; // 基本走行PWM
    int mMaxPwm; // PWMの最大値
    bool mIsLeftEdge; // 左エッジ走行かどうか
    PidGain* mPidGain; // ライン・トレース用PIDゲイン
    PidGain* mRturnPidGain;
    PidGain* mStrongTracePidGain; // 強補正区間用PIDゲイン
    LineTracer* mLineTracer; // 通常速度のライントレース
    LineTracer* mFastLineTracer; // 高速区間のライントレース
    LineTracer* mStrongTraceLineTracer; // 強補正区間のライントレース
    LineTracer* mReturnLineTracer; // 帰路のライントレース
    Stage mStage; // 現在実行中のミッション段階
    bool mStageInitialized; // 現在の段階を初期化済みかどうか
    int mArmStartCount; // アーム動作開始時のエンコーダー値
    int mStartLeftCount; // 回転・距離計測開始時の左モーター値
    int mStartRightCount; // 回転・距離計測開始時の右モーター値
    int mDeliveryStartLeftCount; // 納品場所探索開始時の左モーター値
    int mDeliveryStartRightCount; // 納品場所探索開始時の右モーター値
    int mBlueLineTouchCount; // 青線を踏んだ回数
    int mBlueLineTouchCountReturn; //帰りに青色を踏んだ回数
    int mColorCheckCycles; // ボトル色判定を待った制御周期数
    bool mDetectedBlueBottle; // 青ボトルを検出したかどうか
    bool mDetectedRedBottle;  
    bool mDetectedYellowBottle; // 
    DetectedColor mDetectedTargetColor; // 現在検出した納品場所の色
    DistanceTerminator* mColorRetryDistanceTerminator; // 色判定リトライ用の1cm距離判定
    DistanceTerminator* mColorBackDistanceTerminator; // 色判定後の10mm後退距離判定
    DistanceTerminator* mFirstBlueForwardTerminator; // 初回青線後の150mm距離判定
    DistanceTerminator* mBlueLineTraceTerminator; // 1100mmライントレースの距離判定
    DistanceTerminator* mBlueLineFinalTraceTerminator; // 追加200mmライントレースの距離判定
    DistanceTerminator* mDeliveryBackDistanceTerminator; // 納品場所からの後退距離判定
    int mBlueConsecutiveCount; // 青色を連続検出した回数
    eColor mLastDetectedColor; // 前回検出した色

    eColor getDetectedColorContinuous();
    eColor getDetectedColor() const;
};

#endif // ETTR_APP_BOTTLEDELIVERYTRACER_H_
