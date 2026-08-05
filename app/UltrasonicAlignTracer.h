#ifndef ETTR_APP_ULTRASONICALIGNTRACER_H_
#define ETTR_APP_ULTRASONICALIGNTRACER_H_

#include "Tracer.h"
#include "UltrasonicSensor.h"
#include "Walker.h"

// ET相撲用: ボトルを検出し、正対して接近し、押し出す。
//
// 実測知見: SPIKEの超音波センサは回転・移動しながらだと有効なエコーが
// ほぼ取れない(2026-07-19ログで有効測定0件)。静止時は安定して取れるため、
// スキャン・接近とも「動く→止まる→測る」のステップ方式で構成する。
//
// 動作の流れ:
//   1. SCAN: 小刻みに旋回→ブレーキ→整定→静止測定を繰り返し、
//      有効測定のクラスタ(=ボトル)のうち最も近いものの中心角を求める。
//      近距離クラスタが確定したら全範囲を回りきらず打ち切って高速化。
//   2. TURN_TO_TARGET: クラスタ中心へ旋回
//   3. APPROACH: 残距離に応じた長さの前進パルス→停止→測定を繰り返す。
//      距離が想定より増えたら範囲を広げて再スキャン(最大3回)。
//   4. 接触判定: 有効距離が閾値以下、または「予測距離」(最後の有効距離 -
//      その後の走行距離)が盲域に入ったら押し出しへ
//   5. PUSH: 高PWM・方位保持で指定距離を押し切る
//
// 999mm前後の「エコーなし」センチネルと負値は常に無効扱い。
// 低PWM停動対策として、エンコーダが動かない間はPWMを自動ブーストする。
class UltrasonicAlignTracer : public Tracer {
public:
    UltrasonicAlignTracer(Walker* walker,
                          const spikeapi::UltrasonicSensor* ultrasonicSensor,
                          int halfSweepAngleDeg,
                          int maxDistanceMm,
                          int pushDistanceMm);
    void run() override;

private:
    enum Phase {
        TURN_TO_SWEEP_START,
        SCAN_SWEEP,
        SCAN_STEP_TURN,
        SCAN_SETTLE,
        SCAN_SAMPLE,
        TURN_TO_TARGET,
        APPROACH_SETTLE,
        APPROACH_SAMPLE,
        APPROACH_PULSE,
        BACKING,
        CREEPING,
        PUSHING,
        RETURNING,
        RETURN_TURNING,
    };

    // 単位換算(車体角1度・直進1mmあたりの車輪角)
    static constexpr double WHEEL_DEG_PER_BODY_DEG = 14.0 / 9.0;
    static constexpr double WHEEL_DEG_PER_MM = 2.05;

    // 旋回: 比例制御 + 静止摩擦に負けない下限PWM
    static const int TURN_PWM_MIN = 17;
    static const int TURN_PWM_MAX = 30;
    static const int REVERSE_SCAN_PWM_MAX = 20;
    static const int TURN_TOLERANCE_WDEG = 3;

    // スキャン: 1ステップの角度と静止測定の設定
    static const int ROUGH_SCAN_STEP_BODY_DEG = 8;
    static const int PRECISION_SCAN_STEP_BODY_DEG = 3;
    static const int SCAN_SAMPLE_INTERVAL_TICKS = 3;   // 30ms（粗探索中）
    static const int PRECISION_HALF_BODY_DEG = 25;
    static const int TARGET_VERIFY_HALF_BODY_DEG = 25;
    static const int NEAR_ALIGN_HALF_BODY_DEG = 20;
    static const int SETTLE_TICKS = 8;        // ブレーキ後の整定 (80ms)
    static const int SAMPLE_TICKS = 10;       // サンプル間隔 (100ms; センサ更新≒10Hz)
    static const int SCAN_SAMPLES = 2;        // ステップごとの測定回数
    static const int APPROACH_SAMPLES = 5;    // 接近時の測定回数(有効3つで打ち切り)
    static const int EARLY_ACCEPT_MM = 320;   // これより近いクラスタ確定で走査打ち切り

    // 接近・押し出し
    static const int APPROACH_PWM = 35;
    static const int PUSH_PWM = 35;
    static const int HEADING_DIFF_MAX = 10;   // 方位保持の左右差PWM上限
    static const int PULSE_MIN_MM = 30;       // 前進パルス長の下限
    static const int PULSE_MAX_MM = 150;      // 前進パルス長の上限
    static const int PULSE_KEEP_MM = 120;     // 測定距離からこの分を残してパルス
                                              // (最短検知9cmより手前で止まり測定を維持する)
    static const int NEAR_ALIGN_AFTER_MM = 120;
    static const int NEAR_ALIGN_FAR_TOLERANCE_MM = 150;

    // 距離の有効範囲。999は「エコーなし」センチネルなので常に除外する
    static const int MIN_VALID_MM = 25;
    static const int SENSOR_INVALID_MM = 900;

    // クラスタ判定: クラスタ内最小距離との許容差 / ノイズ棄却の最小角幅
    static const int CLUSTER_GAP_MM = 80;
    static const int CLUSTER_MIN_WIDTH_WDEG = 5;
    static const int CLUSTER_ERROR_BRIDGE_WDEG = 16;  // 約10度以内の取得エラーを同一反応帯として接続
    // クラスタ弧全体はビーム幅で実際より広がる(左右非対称だと中心もずれる)ため、
    // 狙う角度は「最小距離+この値」以内の測定が集まる狭い範囲の中央にする
    static const int CLUSTER_NEAR_BAND_MM = 30;
    static const int CLUSTER_MAX_SAMPLES = 64;

    // 接触・ロスト判定
    static const int CONTACT_MM = 90;         // 実機では80mm前後で値が飽和するため、この値以下で接触
    static const int BLIND_MM = 100;          // 予測距離がこれ以下なら盲域(最短検知9cm)=接触寸前
    static const int LOST_RISE_MM = 150;      // 予測よりこれ以上遠い値はロスト
    static const int MAX_RESCAN_ATTEMPTS = 3;
    static const int RESCAN_HALF_BODY_DEG = 35;  // 再スキャン半角(試行ごとに拡大)
    // 最短検知距離(9cm)より近い位置で旋回するとアームがボトルに当たるため、
    // 再スキャン前にこの距離まで後退して離れる
    static const int RESCAN_STANDOFF_MM = 350;

    // クリープ探索: スキャンで見つからないとき前進して再スキャンする
    // (センサはボトルが近いほど確実に見えるため、近づいて検出圏内に入れる)
    static const int CREEP_FIRST_MM = 200;
    static const int CREEP_INCREMENT_MM = 50;
    static const int CREEP_MAX_MM = 300;
    static const int MAX_CREEP_ATTEMPTS = 2;
    static const int MAX_SWEEP_HALF_BODY_DEG = 175;

    // 終了時は開始地点より少し後方まで退避して、次のゴール走行の方位を固定する
    static const int RETURN_STANDOFF_MM = 150;
    static const int RETURN_PWM = 35;

    // 停動検出: エンコーダが動かないまま続いたらPWMをブーストする
    static const int STALL_BOOST_DIV = 4;   // boost = stallTicks / DIV
    static const int STALL_BOOST_MAX = 25;

    Walker* mWalker;
    const spikeapi::UltrasonicSensor* mUltrasonicSensor;

    int mHalfSweepWdeg;
    int mMaxDistanceMm;
    int mPushWdeg;
    int mMaxApproachWdeg;
    int mRoughScanStepWdeg;
    int mScanStepWdeg;

    int mStartLeftCount;
    int mStartRightCount;

    Phase mPhase;
    bool mFoundObject;

    // 旋回目標(車輪角) / スキャン範囲
    int mTargetTurnWdeg;
    int mSweepEndWdeg;
    int mSweepCenterWdeg;
    bool mPrimarySweep;  // 初回スキャンか(false=再スキャン)
    bool mReverseSweep;
    bool mPrecisionScan;
    bool mTargetVerifyScan;
    bool mNearAlignScan;
    bool mNearAlignDone;
    int mNearAlignFallbackWdeg;

    // 静止測定の進行状態
    int mSettleRemaining;
    int mSampleWait;
    int mSampleAttempts;
    int mValidCount;
    int mMinValidMm;
    int mMeasurementErrorCount;

    // オンラインクラスタリング
    bool mClusterOpen;
    int mClusterMinMm;
    int mClusterStartWdeg;
    int mClusterLastWdeg;
    int mClusterSampleCount;
    int mClusterSampleWdeg[CLUSTER_MAX_SAMPLES];
    int mClusterSampleMm[CLUSTER_MAX_SAMPLES];
    bool mBestFound;
    int mBestMinMm;
    int mBestMinWdeg;
    int mBestCenterWdeg;
    int mSweepValidSamples;
    int mSweepErrorSamples;
    int mSweepBridgedErrorSamples;
    int mSweepOutOfRangeSamples;

    // 接近時の追跡状態
    int mLastValidMm;
    int mLastValidForwardWdeg;
    int mApproachStartForwardWdeg;
    int mInvalidRounds;
    bool mTargetVerifyAttempted;
    int mRescanAttempts;
    int mCreepAttempts;
    int mCreepStartForwardWdeg;
    int mCreepTargetWdeg;
    int mBackupStartForwardWdeg;
    int mBackupTargetWdeg;
    int mPendingRescanHalfWdeg;

    // 前進パルス・押し出し
    int mPulseStartForwardWdeg;
    int mPulseTargetWdeg;
    int mPushStartForwardWdeg;
    int mPushTargetWdeg;

    // 終了後の退避・初期方位復帰
    int mReturnTargetForwardWdeg;

    // 停動検出(左右独立: 片輪だけ止まると機体が平行移動して方位がずれるため)
    int mLeftStallTicks;
    int mRightStallTicks;
    int mPrevLeftCount;
    int mPrevRightCount;

    void startSearching();
    void startSweep(int startWdeg, int endWdeg, bool primary);
    void startPrecisionScan(int centerWdeg);
    void startTargetVerifyScan();
    void startNearAlignScan();
    void runTurn();
    void runScanSweep();
    void runScanStepTurn();
    void runScanMeasure();
    void finishScan();
    void beginMeasurement(Phase settlePhase);
    bool stepMeasurement(int maxSamples);  // trueで測定一巡完了
    void runApproachMeasure();
    void runApproachPulse();
    void runPush();
    void runReturn();
    void startReturn(bool pushed);
    void startApproach();
    void startPulse(int distanceMm);
    void startPush(int remainingMm);
    void startRescan();
    void doRescanSweep();
    void runBackup();
    void startCreep();
    void runCreep();
    void feedCluster(int distanceMm, bool valid, int turnWdeg);
    void closeCluster();
    void finish(bool pushed);
    bool driveTurnTo(int targetWdeg, int pwmLimit);
    void driveForward(int basePwm);
    void updateStall(int* leftBoost, int* rightBoost);
    int getTurnWdeg() const;
    int getForwardWdeg() const;
    int readDistanceMm(bool* valid) const;
};

#endif  // ETTR_APP_ULTRASONICALIGNTRACER_H_
