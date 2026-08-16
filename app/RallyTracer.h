/**
 * @file RallyTracer.h
 * @brief ETラリー走行トレーサー
 *
 * RallyRoute に従ってロボットを動かす。
 * 各ステップで「旋回 → 前進」を繰り返し、
 * VIRTUAL_DETOUR ステップでは追加で「後退（戻り）」フェーズを実行する。
 *
 * まずは RallyRoute を手入力して動作確認するための実装。
 */

#ifndef ETTR_APP_RALLYTRACER_H_
#define ETTR_APP_RALLYTRACER_H_

#include "RallyRoute.h"
#include "ColorSensor.h"
#include "Tracer.h"
#include "Walker.h"

#include <cstddef>

class RallyTracer : public Tracer {
public:
    /**
     * @param walker            Walker インスタンス
     * @param route             走行する経路
     * @param movePwm           直進時の PWM 値 (正値)
     * @param turnPwm           旋回時の PWM 値 (正値)
     * @param initialPos        走行開始時の QR 座標
     * @param initialHeadingDeg 走行開始時の機体向き (0=東, 90=北, 180=西, 270=南)
      * @param colorSensor       床面マーカ検出用センサ（未使用時は nullptr）
      * @param enableMarkerCorrection 黒マーカ検出で自己位置補正を有効化するか
      * @param markerReflectionThreshold 黒マーカ判定の反射光しきい値（以下で検出）
      * @param markerSnapWindowDegrees 目標残距離がこの値以下のときだけ補正を許可
      * @param markerCooldownTicks 連続誤検出を防ぐクールダウン周期数
     */
    RallyTracer(Walker* walker, const RallyRoute& route,
                int movePwm, int turnPwm,
                QRPos initialPos = {1, 1},
                     int initialHeadingDeg = 0,
                     const spikeapi::ColorSensor* colorSensor = nullptr,
                     bool enableMarkerCorrection = false,
                     int markerReflectionThreshold = 20,
                     int markerSnapWindowDegrees = 180,
                     int markerCooldownTicks = 25);

    void run() override;

    // ---- 調整可能な物理定数 ----

    /// QR コード格子 1 マス分の移動に必要なホイール回転角 [度]
    /// QR 間隔 200 mm, ホイール径 56 mm として計算: 200 / (56π) × 360 ≈ 410
    static const int QR_GRID_WHEEL_DEGREES = 410;

    /// 車体 1 度旋回に必要なホイール回転角（右輪-左輪の平均変化量）
    /// UltrasonicAlignTracer と同じ 14/9 ≈ 1.556 を使用
    static constexpr double WHEEL_DEGREES_PER_BODY_DEGREE = 14.0 / 9.0;

    /// 旋回完了の許容誤差 [ホイール度]
    static const int TURN_TOLERANCE = 3;

    /// 直進完了の許容誤差 [ホイール度]
    static const int MOVE_TOLERANCE = 10;

private:
    enum Phase {
        TURNING,    ///< 目標方向へ旋回中
        MOVING,     ///< 目標 QR へ前進中
        RETURNING,  ///< 仮想 QR から実 QR へ後退中 (VIRTUAL_DETOUR 専用)
    };

    Walker* mWalker;
    RallyRoute mRoute;
    int mMovePwm;
    int mTurnPwm;

    std::size_t mCurrentStepIndex;  ///< 現在処理中のステップ番号
    Phase mPhase;
    QRPos mCurrentPos;              ///< 現在の QR 座標
    int mHeadingDeg;                ///< 現在の機体向き [度]
    int mTargetHeadingDeg;          ///< 旋回目標の向き（旋回完了時に mHeadingDeg へコピー）

    int mPhaseStartLeftCount;       ///< 現フェーズ開始時の左モーター値
    int mPhaseStartRightCount;      ///< 現フェーズ開始時の右モーター値
    int mTargetWheelDegrees;        ///< 現フェーズの目標ホイール変化量（絶対値）

    const spikeapi::ColorSensor* mColorSensor;
    bool mEnableMarkerCorrection;
    int mMarkerReflectionThreshold;
    int mMarkerSnapWindowDegrees;
    int mMarkerCooldownTicks;
    int mMarkerCooldownRemaining;
    bool mMarkerDetectedInPhase;

    // ---- フェーズ遷移 ----
    void startNextStep();
    void beginTurning(int targetHeadingDeg);
    void beginMoving(int wheelDegrees);
    void beginReturning(int wheelDegrees);
    void finishStep();

    // ---- フェーズ実行 ----
    void execTurning();
    void execMoving();
    void execReturning();

    bool isMarkerSnapTriggered(int remainingDegrees);

    // ---- ユーティリティ ----
    /// 旋回量を計測 [ホイール度、反時計回りが正]
    int getTurnWheelDegrees() const;
    /// 直進量を計測 [ホイール度、前進が正]
    int getMoveWheelDegrees() const;
    /// フェーズ開始カウンタをリセット
    void resetPhaseCounters();

    /// fromDeg から toDeg への最短旋回量 [-180, 180] を返す
    static int shortestTurn(int fromDeg, int toDeg);
    /// from → to 方向の角度 [度、0=東、90=北] を返す
    static int calcHeadingDeg(const QRPos& from, const QRPos& to);
    /// from → to のユークリッド距離に対応するホイール回転角 [度] を返す
    static int calcMoveWheelDegrees(const QRPos& from, const QRPos& to);
};

#endif  // ETTR_APP_RALLYTRACER_H_
