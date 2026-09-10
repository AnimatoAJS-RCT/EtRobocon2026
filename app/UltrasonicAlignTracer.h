#ifndef ETTR_APP_ULTRASONICALIGNTRACER_H_
#define ETTR_APP_ULTRASONICALIGNTRACER_H_

#include "Tracer.h"
#include "UltrasonicSensor.h"
#include "Walker.h"

// ET相撲用: ボトルを検出し、正対して接近し、押し出す。
//
// DISTLの-1はドライバエラーではなく、有効な距離を取得できなかった無反射値。
//
// 動作の流れ:
//   1. SCAN: 約35deg/sで連続往復旋回し、2度ごとの方位ビンへエコーを累積する。
//      1回以上ヒットした連続ビンを対象とし、距離中央値が最小の塊を選ぶ。
//   2. TURN_TO_TARGET: クラスタ中心へ旋回
//   3. APPROACH: 残距離に応じた長さの前進パルス→停止→測定を繰り返す。
//      距離が想定より増えたら範囲を広げて再スキャン(最大3回)。
//   4. 接触判定: 有効距離が閾値以下、または「予測距離」(最後の有効距離 -
//      その後の走行距離)が盲域に入ったら押し出しへ
//   5. PUSH: 高PWM・方位保持で指定距離を押し切る
//
// 低PWM停動対策として、エンコーダが動かない間はPWMを自動ブーストする。
class UltrasonicAlignTracer : public Tracer {
public:
    UltrasonicAlignTracer(Walker* walker,
                          const spikeapi::UltrasonicSensor* ultrasonicSensor,
                          int halfSweepAngleDeg,
                          int maxDistanceMm,
                          int pushDistanceMm,
                          int scanTurnDegPerSec,
                          int approachPwm,
                          int pushPwm,
                          int measureHz,
                          int scanMeasureHz);
    void run() override;

private:
    enum Phase {
        TURN_TO_SWEEP_START,
        SCAN_SWEEP,
        TURN_TO_TARGET,
        APPROACH_SETTLE,
        APPROACH_SAMPLE,
        APPROACH_PULSE,
        BACKING,
        PUSHING,
        RETURNING,
        RETURN_TURNING,
    };

    // 単位換算(車体角1度・直進1mmあたりの車輪角)
    static constexpr double WHEEL_DEG_PER_BODY_DEG = 14.0 / 9.0;
    static constexpr double WHEEL_DEG_PER_MM = 2.05;

    // 旋回: 比例制御 + 静止摩擦に負けない下限PWM
    static const int TURN_PWM_MIN = 22;
    static const int TURN_PWM_MAX = 40;
    static const int TURN_TOLERANCE_WDEG = 3;

    // 30msごとの読取りはセンサ内部の約100ms更新より速く、ピング数とは一致しない。
    static const int SCAN_SPEED_WINDOW_TICKS = 10;
    static const int SCAN_PWM_MIN = 22;
    static const int SCAN_PWM_MAX = 35;
    static const int SCAN_PWM_INITIAL = 25;
    static const int TARGET_VERIFY_HALF_BODY_DEG = 30;
    static const int NEAR_ALIGN_HALF_BODY_DEG = 20;
    static const int SETTLE_TICKS = 8;        // ブレーキ後の整定 (80ms)
    static const int APPROACH_SAMPLES = 5;

    // 接近・押し出し
    static const int HEADING_DIFF_MAX = 10;   // 方位保持の左右差PWM上限
    static const int PULSE_MIN_MM = 30;       // 前進パルス長の下限
    static const int PULSE_MAX_MM = 150;      // 前進パルス長の上限
    static const int PULSE_KEEP_MM = 120;     // 測定距離からこの分を残してパルス
                                              // (最短検知9cmより手前で止まり測定を維持する)
    static const int NEAR_ALIGN_AFTER_MM = 120;
    static const int NEAR_ALIGN_FAR_TOLERANCE_MM = 150;

    // DISTLの物理レンジと、近距離で飽和する実機値を含む下限。
    static const int MIN_VALID_MM = 50;
    static const int SENSOR_MAX_VALID_MM = 2500;

    // 方位占有ヒストグラム。1ビンでもヒットすれば反射ありとする(1-of-M)。
    static const int BIN_WIDTH_WDEG = 3;  // 車体角約2度
    static const int BIN_MAX_WDEG = 140;  // 車体角約90度
    static const int BIN_COUNT = BIN_MAX_WDEG * 2 / BIN_WIDTH_WDEG + 1;
    static const int BIN_DISTANCE_CAPACITY = 8;
    static const int CLUSTER_MAX_SAMPLES = 64;

    // 接触・ロスト判定
    static const int CONTACT_MM = 90;         // 実機では80mm前後で値が飽和するため、この値以下で接触
    static const int BLIND_MM = 100;          // 予測距離がこれ以下なら盲域(最短検知9cm)=接触寸前
    static const int RECENT_ECHO_TRAVEL_MM = 100;
    static const int LOST_RISE_MM = 150;      // 予測よりこれ以上遠い値はロスト
    static const int PUSH_LOST_RISE_MM = 150; // 押出し中にこの分以上遠い反射は対象喪失
    static const int MAX_RESCAN_ATTEMPTS = 3;
    static const int RESCAN_HALF_BODY_DEG = 35;  // 再スキャン半角(試行ごとに拡大)
    // 最短検知距離(9cm)より近い位置で旋回するとアームがボトルに当たるため、
    // 再スキャン前にこの距離まで後退して離れる
    static const int RESCAN_STANDOFF_MM = 150;
    static const int RESCAN_BACKUP_MAX_MM = 50;

    // 未検出時は前進せず、同じ位置で探索範囲だけを段階的に拡大する。
    static const int MAX_CREEP_ATTEMPTS = 2;
    static const int MAX_SWEEP_HALF_BODY_DEG = 90;
    static const int PUSH_LOST_BACKUP_MM = 100;

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
    int mScanTargetBodyDegPerSec;
    int mApproachPwm;
    int mPushPwm;
    int mSampleTicks;
    int mScanSampleIntervalTicks;

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
    int mNoEchoCount;

    // 往路・復路で共有する方位占有ヒストグラム
    int mBinHitCount[BIN_COUNT];
    int mBinDistance[BIN_COUNT][BIN_DISTANCE_CAPACITY];
    int mBinNextDistance[BIN_COUNT];
    bool mBestFound;
    int mBestMedianMm;
    int mBestCenterWdeg;
    int mSweepValidSamples;
    int mSweepNoEchoSamples;
    int mSweepOutOfRangeSamples;
    int mScanPwm;
    int mScanSpeedTicks;
    int mScanSpeedStartWdeg;

    // 接近時の追跡状態
    int mLastValidMm;
    int mLastValidForwardWdeg;
    int mApproachStartForwardWdeg;
    int mInvalidRounds;
    bool mTargetVerifyAttempted;
    int mRescanAttempts;
    int mCreepAttempts;
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
    void startTargetVerifyScan();
    void startNearAlignScan();
    void runTurn();
    void runScanSweep();
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
    void startPushLostRescan();
    void startRescan();
    void doRescanSweep();
    void runBackup();
    void startCreep();
    void resetHistogram();
    void recordHit(int distanceMm, int turnWdeg);
    void selectBestCluster();
    int medianForBin(int bin) const;
    void finish(bool pushed);
    bool driveTurnTo(int targetWdeg, int pwmLimit);
    bool driveScanTo(int targetWdeg);
    void driveForward(int basePwm);
    void updateStall(int* leftBoost, int* rightBoost);
    int getTurnWdeg() const;
    int getForwardWdeg() const;
    int readDistanceMm(bool* hasEcho, bool* inRange) const;
};

#endif  // ETTR_APP_ULTRASONICALIGNTRACER_H_
