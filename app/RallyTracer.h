/**
 * @file RallyTracer.h
 * @brief ETラリー走行トレーサー
 *
 * RallyRoute に従ってロボットを動かす。
 * 各ステップで「旋回 → 軸方向への移動」を繰り返し、
 * VIRTUAL_DETOUR ステップでは追加で反対向きに戻るフェーズを実行する。
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
      * @param markerSearchAngleDeg マーカー未検出時に首振り探索する片側の車体角 [度]
     * @param finalHeadingDeg   終了時の機体向き。負値の場合は最終旋回を行わない
     */
    RallyTracer(Walker* walker, const RallyRoute& route,
                int movePwm, int turnPwm,
                QRPos initialPos = {1, 1},
                int initialHeadingDeg = 0,
                int finalHeadingDeg = -1,
                const spikeapi::ColorSensor* colorSensor = nullptr,
                bool enableMarkerCorrection = false,
                int markerReflectionThreshold = 20,
                int markerSnapWindowDegrees = 180,
                int markerCooldownTicks = 25,
                int markerSearchAngleDeg = 15);

    void run() override;

    // ---- 調整可能な物理定数 ----

    /// QR コード格子 1 マス分の移動に必要なホイール回転角 [度]
    /// QR 間隔 200 mm, ホイール径 56 mm として計算: 200 / (56π) × 360 ≈ 410
    static const int QR_GRID_WHEEL_DEGREES = 510;

    /// 車体 1 度旋回に必要なホイール回転角（右輪-左輪の平均変化量）
    /// UltrasonicAlignTracer と同じ 14/9 ≈ 1.556 を使用
    static constexpr double WHEEL_DEGREES_PER_BODY_DEGREE = 2.106;

    /// 実測した車体角に合わせて、方向ごとの旋回目標を校正する
    /// targetWdeg = requestedBodyDeg * WHEEL_DEGREES_PER_BODY_DEGREE * scale
    static constexpr double RIGHT_TURN_SCALE = 1.0;   // 時計回り
    static constexpr double LEFT_TURN_SCALE = 1.0;    // 反時計回り

    /// 旋回完了の許容誤差 [ホイール度]
    static const int TURN_TOLERANCE = 1;

    /// 前回旋回の誤差を次回目標へ反映する最大補正量 [ホイール度]
    static const int TURN_CORRECTION_LIMIT = 20;

    /// 直進完了の許容誤差 [ホイール度]
    static const int MOVE_TOLERANCE = 2;

    /// 目標直前に低速へ切り替える残り距離 [ホイール度]
    static const int APPROACH_WINDOW = 100;

    /// モーターが動き続ける最低付近のアプローチPWM
    static const int APPROACH_PWM = 40;

    /// 前方カメラがマーカーを検出してから車体中心が通過するまでのホイール角 [度]
    static const int MARKER_TO_CENTER_WHEEL_DEGREES = 180;

    /// マーカー未検出時に片側へ探索する車体角のデフォルト値 [度]
    static const int MARKER_SEARCH_ANGLE_DEGREES_DEFAULT = 15;

    /// マーカー探索で前後にクロールする1段あたりの距離 [ホイール度]
    /// 前方センサーは車軸中心からタイヤ半径の1.5倍前方にあるため、
    /// その場旋回だけでは前後方向のずれを拾えない。
    /// タイヤ径56mm・半径28mmとして、センサーオフセット 1.5×28=42mm を
    /// ホイール回転角に換算した値（42/(56π)×360 ≈ 86°）を目安に設定。
    static const int MARKER_SEARCH_CRAWL_STEP_WHEEL_DEGREES = 90;

    /// マーカー探索で前後にクロールする最大段数
    /// 直進中に見逃したマーカーは目標地点より手前（後方）にある可能性が高いため、
    /// 後方は前方より広い範囲まで探索する
    static const int MARKER_SEARCH_MAX_CRAWL_STAGES_FORWARD = 2;
    static const int MARKER_SEARCH_MAX_CRAWL_STAGES_BACKWARD = 4;

private:
    enum Phase {
        TURNING,    ///< 目標方向へ旋回中
        BRAKING,    ///< 旋回完了後、停止を確認中
        MOVING,     ///< 目標 QR へ前進中
        RETURNING,  ///< 仮想 QR から実 QR へ後退中 (VIRTUAL_DETOUR 専用)
        MARKER_OFFSET, ///< マーカー検出後、車体中心をマーカー位置まで進める
        MARKER_SEARCH, ///< マーカー未検出時に首振り旋回と前後クロールで探し回る
    };

    /// マーカー探索中のサブフェーズ
    enum MarkerSearchSubStage {
        MARKER_SEARCH_SWEEP_PLUS,     ///< 基準方位から +A 度へ旋回しながら探索
        MARKER_SEARCH_SWEEP_MINUS,    ///< +A 度から -A 度へ旋回しながら探索
        MARKER_SEARCH_RETURN_HEADING, ///< -A 度から基準方位へ旋回を戻す
        MARKER_SEARCH_CRAWL,          ///< 次の探索リングへ前後にクロール移動する
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
    int mFinalHeadingDeg;           ///< 終了時の機体向き。負値なら指定なし
    bool mIsFinalTurning;           ///< 最終方位合わせ中かどうか

    int mPhaseStartLeftCount;       ///< 現フェーズ開始時の左モーター値
    int mPhaseStartRightCount;      ///< 現フェーズ開始時の右モーター値
    int mTargetWheelDegrees;        ///< 現フェーズの目標ホイール変化量（絶対値）
    int mPreviousTurnError;         ///< 前回旋回の実績値-目標値 [ホイール度]
    int mMoveDirection;              ///< 走行方向（+1=前進、-1=後退）
    int mBrakeCountdown;             ///< ブレーキフェーズの残りフレーム数
    bool mMarkerCorrectionAllowedThisStep; ///< 仮想QR（0/5）のゲート通過中はMARKER補正を無効化する

    const spikeapi::ColorSensor* mColorSensor;
    bool mEnableMarkerCorrection;
    int mMarkerReflectionThreshold;
    int mMarkerSnapWindowDegrees;
    int mMarkerCooldownTicks;
    int mMarkerCooldownRemaining;
    int mMarkerSearchAngleDeg;                ///< マーカー未検出時に首振り探索する片側の車体角 [度]
    bool mMarkerDetectedInPhase;
    int mMarkerSearchRing;                    ///< 探索リング番号 (0=その場旋回のみ、以降前後にクロール)
    MarkerSearchSubStage mMarkerSearchSubStage;
    int mMarkerSearchHeadingDeg;               ///< 探索開始時の基準方位 [度]
    int mMarkerSearchTurnWheelDegrees;         ///< 基準方位から ±A 度旋回するためのホイール度
    int mMarkerSearchCrawlOffset;               ///< 探索開始位置からの前後クロール量（前進が正）[ホイール度]
    int mMarkerSearchCrawlStartOffset;          ///< 現在のクロール動作開始時点のオフセット
    int mMarkerSearchCrawlTargetOffset;         ///< 現在のクロール動作の目標オフセット
    int mMarkerSearchNextRing;                   ///< クロール完了後に切り替えるリング番号
    bool mIsMarkerSearchRecoveryTurning;

    // ---- フェーズ遷移 ----
    void startNextStep();
    void beginTurning(int targetHeadingDeg);
    void beginMoving(int wheelDegrees, int moveDirection);
    void beginReturning(int wheelDegrees);
    void beginMarkerOffset();
    void beginMarkerSearch();
    void completeMovingStep();
    void finishStep();

    // ---- フェーズ実行 ----
    void execTurning();
    void execBraking();
    void execMoving();
    void execReturning();
    void execMarkerOffset();
    void execMarkerSearch();

    // ---- マーカー探索サブフェーズ ----
    /// 現在のリング位置で ±A 度の首振り旋回を開始する
    void beginMarkerSearchSweep();
    /// 旋回サブフェーズ (SWEEP_PLUS/MINUS/RETURN_HEADING) の1ティック分を実行
    void execMarkerSearchTurn();
    /// クロールサブフェーズの1ティック分を実行
    void execMarkerSearchCrawl();
    /// 現在のサブフェーズ完了後、次のサブフェーズ/リングへ遷移する
    void advanceMarkerSearchStage();
    /// 全リングでマーカーが見つからなかった場合の後処理
    void finishMarkerSearchNotFound();
    /// 探索リング番号から前後クロールオフセット目標値を返す
    /// (0, +1段, -1段, +2段, -2段, ... の順)
    static int markerSearchRingOffset(int ring);

    bool isMarkerSnapTriggered(int remainingDegrees);
    bool isMarkerDetected();

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
    static int calcMoveDirection(const QRPos& from, const QRPos& to);
    /// from → to のユークリッド距離に対応するホイール回転角 [度] を返す
    static int calcMoveWheelDegrees(const QRPos& from, const QRPos& to);
};

#endif  // ETTR_APP_RALLYTRACER_H_
