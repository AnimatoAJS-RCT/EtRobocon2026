#include "UltrasonicAlignTracer.h"

#include "Log.h"

#include <algorithm>
#include <cstdlib>

UltrasonicAlignTracer::UltrasonicAlignTracer(Walker* walker,
                                             const spikeapi::UltrasonicSensor* ultrasonicSensor,
                                             int halfSweepAngleDeg,
                                             int maxDistanceMm,
                                             int pushDistanceMm)
    : mWalker(walker),
      mUltrasonicSensor(ultrasonicSensor),
      mHalfSweepWdeg(static_cast<int>(std::abs(halfSweepAngleDeg) * WHEEL_DEG_PER_BODY_DEG)),
      mMaxDistanceMm(std::max(100, maxDistanceMm)),
      mPushWdeg(static_cast<int>(std::abs(pushDistanceMm) * WHEEL_DEG_PER_MM)),
      mMaxApproachWdeg(static_cast<int>((std::max(100, maxDistanceMm) + 300) * WHEEL_DEG_PER_MM)),
    mRoughScanStepWdeg(std::max(1, static_cast<int>(ROUGH_SCAN_STEP_BODY_DEG * WHEEL_DEG_PER_BODY_DEG))),
    mScanStepWdeg(mRoughScanStepWdeg),
      mStartLeftCount(0),
      mStartRightCount(0),
      mPhase(TURN_TO_SWEEP_START),
      mFoundObject(false),
      mTargetTurnWdeg(0),
      mSweepEndWdeg(0),
      mSweepCenterWdeg(0),
      mPrimarySweep(true),
            mReverseSweep(false),
    mPrecisionScan(false),
    mTargetVerifyScan(false),
            mNearAlignScan(false),
            mNearAlignDone(false),
            mNearAlignFallbackWdeg(0),
      mSettleRemaining(0),
      mSampleWait(0),
      mSampleAttempts(0),
      mValidCount(0),
      mMinValidMm(0),
    mMeasurementErrorCount(0),
      mClusterOpen(false),
      mClusterMinMm(0),
      mClusterStartWdeg(0),
      mClusterLastWdeg(0),
      mClusterSampleCount(0),
      mBestFound(false),
      mBestMinMm(0),
    mBestMinWdeg(0),
      mBestCenterWdeg(0),
    mSweepValidSamples(0),
            mSweepErrorSamples(0),
            mSweepBridgedErrorSamples(0),
            mSweepOutOfRangeSamples(0),
      mLastValidMm(-1),
      mLastValidForwardWdeg(0),
    mApproachStartForwardWdeg(0),
      mInvalidRounds(0),
    mTargetVerifyAttempted(false),
      mRescanAttempts(0),
      mCreepAttempts(0),
      mCreepStartForwardWdeg(-1),
      mCreepTargetWdeg(0),
      mBackupStartForwardWdeg(0),
      mBackupTargetWdeg(0),
      mPendingRescanHalfWdeg(0),
      mPulseStartForwardWdeg(0),
      mPulseTargetWdeg(0),
      mPushStartForwardWdeg(0),
      mPushTargetWdeg(0),
    mReturnTargetForwardWdeg(0),
      mLeftStallTicks(0),
      mRightStallTicks(0),
      mPrevLeftCount(0),
      mPrevRightCount(0)
{
    mState = UNDEFINED;
}

void UltrasonicAlignTracer::run()
{
    switch(mState) {
        case UNDEFINED:
            mState = WAITING_FOR_START;
            break;
        case WAITING_FOR_START:
            if(mStarterList.empty()) {
                startSearching();
            } else {
                for(auto starter : mStarterList) {
                    if(starter->isPushed()) {
                        startSearching();
                        break;
                    }
                }
            }
            break;
        case WALKING:
            switch(mPhase) {
                case TURN_TO_SWEEP_START:
                case SCAN_STEP_TURN:
                case TURN_TO_TARGET:
                    runTurn();
                    break;
                case SCAN_SWEEP:
                    runScanSweep();
                    break;
                case SCAN_SETTLE:
                case SCAN_SAMPLE:
                    runScanMeasure();
                    break;
                case APPROACH_SETTLE:
                case APPROACH_SAMPLE:
                    runApproachMeasure();
                    break;
                case APPROACH_PULSE:
                    runApproachPulse();
                    break;
                case BACKING:
                    runBackup();
                    break;
                case CREEPING:
                    runCreep();
                    break;
                case PUSHING:
                    runPush();
                    break;
                case RETURNING:
                    runReturn();
                    break;
                case RETURN_TURNING:
                    if(driveTurnTo(0, TURN_PWM_MAX)) {
                        LOGI("[ULTRA_ALIGN] returned: forward=%d heading=%d wheelDeg\n",
                             getForwardWdeg(), getTurnWdeg());
                        mState = TERMINATED;
                    }
                    break;
            }
            break;
        case TERMINATED:
        default:
            break;
    }
}

void UltrasonicAlignTracer::startSearching()
{
    mStartLeftCount = mWalker->getLeftCount();
    mStartRightCount = mWalker->getRightCount();
    mPrevLeftCount = mStartLeftCount;
    mPrevRightCount = mStartRightCount;
        // 静止中の初回読取りでDISTLモードへの自動切替を完了させてから旋回する。
        int primeDistance = mUltrasonicSensor->getDistance();
        LOGI("[ULTRA_ALIGN] sensor prime: raw=%dmm status=%s\n",
            primeDistance, primeDistance < 0 ? "driver-error" : "ready");
        mReverseSweep = false;
    mState = WALKING;
    LOGI("[ULTRA_ALIGN] start: half=%d wheelDeg step=%d max=%dmm push=%d wheelDeg\n",
         mHalfSweepWdeg, mScanStepWdeg, mMaxDistanceMm, mPushWdeg);
    startSweep(mHalfSweepWdeg, -mHalfSweepWdeg, true);
}

void UltrasonicAlignTracer::startSweep(int startWdeg, int endWdeg, bool primary)
{
    mTargetTurnWdeg = startWdeg;
    mSweepEndWdeg = endWdeg;
    mSweepCenterWdeg = (startWdeg + endWdeg) / 2;
    mPrimarySweep = primary;
    mScanStepWdeg = mRoughScanStepWdeg;
    mPrecisionScan = false;
    mTargetVerifyScan = false;
    mNearAlignScan = false;
    mClusterOpen = false;
    mBestFound = false;
    mSweepValidSamples = 0;
    mSweepErrorSamples = 0;
    mSweepBridgedErrorSamples = 0;
    mSweepOutOfRangeSamples = 0;
    mPhase = TURN_TO_SWEEP_START;
    LOGI("[ULTRA_ALIGN] sweep plan: %d -> %d wheelDeg primary=%d\n",
         startWdeg, endWdeg, primary ? 1 : 0);
}

void UltrasonicAlignTracer::startPrecisionScan(int centerWdeg)
{
    int halfWdeg = static_cast<int>(PRECISION_HALF_BODY_DEG * WHEEL_DEG_PER_BODY_DEG);
    mTargetTurnWdeg = centerWdeg + halfWdeg;
    mSweepEndWdeg = centerWdeg - halfWdeg;
    mSweepCenterWdeg = centerWdeg;
    mPrecisionScan = true;
    mScanStepWdeg = std::max(1, static_cast<int>(PRECISION_SCAN_STEP_BODY_DEG * WHEEL_DEG_PER_BODY_DEG));
    mTargetVerifyScan = false;
    mClusterOpen = false;
    mBestFound = false;
    mSweepValidSamples = 0;
    mSweepErrorSamples = 0;
    mSweepBridgedErrorSamples = 0;
    mSweepOutOfRangeSamples = 0;
    mPhase = TURN_TO_SWEEP_START;
    LOGI("[ULTRA_ALIGN] precision scan: %d -> %d wheelDeg around=%d\n",
         mTargetTurnWdeg, mSweepEndWdeg, centerWdeg);
}

void UltrasonicAlignTracer::startTargetVerifyScan()
{
    int halfWdeg = static_cast<int>(TARGET_VERIFY_HALF_BODY_DEG * WHEEL_DEG_PER_BODY_DEG);
    int centerWdeg = mTargetTurnWdeg;
    mTargetTurnWdeg = centerWdeg + halfWdeg;
    mSweepEndWdeg = centerWdeg - halfWdeg;
    mSweepCenterWdeg = centerWdeg;
    mPrecisionScan = true;
    mScanStepWdeg = std::max(1, static_cast<int>(PRECISION_SCAN_STEP_BODY_DEG * WHEEL_DEG_PER_BODY_DEG));
    mTargetVerifyScan = true;
    mClusterOpen = false;
    mBestFound = false;
    mSweepValidSamples = 0;
    mSweepErrorSamples = 0;
    mSweepBridgedErrorSamples = 0;
    mSweepOutOfRangeSamples = 0;
    mPhase = TURN_TO_SWEEP_START;
    LOGI("[ULTRA_ALIGN] target verify: %d -> %d wheelDeg around=%d\n",
         mTargetTurnWdeg, mSweepEndWdeg, centerWdeg);
}

void UltrasonicAlignTracer::startNearAlignScan()
{
    int halfWdeg = static_cast<int>(NEAR_ALIGN_HALF_BODY_DEG * WHEEL_DEG_PER_BODY_DEG);
    mNearAlignFallbackWdeg = mTargetTurnWdeg;
    mTargetTurnWdeg = mNearAlignFallbackWdeg + halfWdeg;
    mSweepEndWdeg = mNearAlignFallbackWdeg - halfWdeg;
    mSweepCenterWdeg = mNearAlignFallbackWdeg;
    mPrimarySweep = false;
    mReverseSweep = false;
    mPrecisionScan = false;
    mTargetVerifyScan = false;
    mNearAlignScan = true;
    mClusterOpen = false;
    mBestFound = false;
    mSweepValidSamples = 0;
    mSweepErrorSamples = 0;
    mSweepBridgedErrorSamples = 0;
    mSweepOutOfRangeSamples = 0;
    mPhase = TURN_TO_SWEEP_START;
    LOGI("[ULTRA_ALIGN] near align: %d -> %d wheelDeg around=%d\n",
         mTargetTurnWdeg, mSweepEndWdeg, mNearAlignFallbackWdeg);
}

void UltrasonicAlignTracer::runTurn()
{
    if(!driveTurnTo(mTargetTurnWdeg, TURN_PWM_MAX)) {
        return;
    }
    if(mPhase == TURN_TO_TARGET) {
        startApproach();
    } else if(mPhase == TURN_TO_SWEEP_START && !mPrecisionScan) {
        // 粗探索は反射帯を素早く見つけるため、停止せず連続旋回しながら測定する。
        mSampleWait = 0;
        mPhase = SCAN_SWEEP;
    } else {
        beginMeasurement(SCAN_SETTLE);
    }
}

void UltrasonicAlignTracer::runScanSweep()
{
    if(mSampleWait > 0) {
        mSampleWait--;
    } else {
        bool valid = false;
        int distance = readDistanceMm(&valid);
        int turnWdeg = getTurnWdeg();
        if(valid && mNearAlignScan) {
            int expected = mLastValidMm
                         - static_cast<int>((getForwardWdeg() - mLastValidForwardWdeg)
                                            / WHEEL_DEG_PER_MM);
            int maximum = std::max(MIN_VALID_MM,
                                   expected + NEAR_ALIGN_FAR_TOLERANCE_MM);
            if(distance > maximum) {
                valid = false;
                LOGI("[ULTRA_ALIGN] near align reject: pos=%d distance=%dmm expected=%dmm max=%dmm\n",
                     turnWdeg, distance, expected, maximum);
            }
        }
        // Bluetoothログ出力は走査ループを遅らせるため、有効エコーだけ記録する。
        if(valid) {
            LOGI("[ULTRA_ALIGN] sweep hit: pos=%d distance=%dmm\n", turnWdeg, distance);
        } else if(distance < 0) {
            mSweepErrorSamples++;
        } else {
            mSweepOutOfRangeSamples++;
        }
        bool bridgeError = distance < 0 && mClusterOpen
                           && std::abs(turnWdeg - mClusterLastWdeg)
                                  <= CLUSTER_ERROR_BRIDGE_WDEG;
        if(bridgeError) {
            // 短い取得エラーでは反応帯を閉じず、次の正常値を待つ。
            mSweepBridgedErrorSamples++;
        } else {
            feedCluster(distance, valid, turnWdeg);
        }
        if(valid) {
            mSweepValidSamples++;
        }
        mSampleWait = SCAN_SAMPLE_INTERVAL_TICKS - 1;
    }

    int scanPwmLimit = mReverseSweep ? REVERSE_SCAN_PWM_MAX : TURN_PWM_MAX;
    if(driveTurnTo(mSweepEndWdeg, scanPwmLimit)) {
        finishScan();
    }
}

void UltrasonicAlignTracer::beginMeasurement(Phase settlePhase)
{
    mWalker->brake();
    mSettleRemaining = SETTLE_TICKS;
    mSampleWait = 0;
    mSampleAttempts = 0;
    mValidCount = 0;
    mMinValidMm = 0;
    mMeasurementErrorCount = 0;
    mPhase = settlePhase;
}

bool UltrasonicAlignTracer::stepMeasurement(int maxSamples)
{
    if(mSettleRemaining > 0) {
        mSettleRemaining--;
        return false;
    }
    if(mSampleWait > 0) {
        mSampleWait--;
        return false;
    }

    bool valid = false;
    int distance = readDistanceMm(&valid);
    LOGD("[ULTRA_ALIGN] sample: pos=%d raw=%dmm valid=%d %d/%d\n",
         getTurnWdeg(), distance, valid ? 1 : 0, mSampleAttempts + 1, maxSamples);
    if(valid) {
        if(mValidCount == 0 || distance < mMinValidMm) {
            mMinValidMm = distance;
        }
        mValidCount++;
    } else if(distance < 0) {
        mMeasurementErrorCount++;
    }
    mSampleAttempts++;
    if(mSampleAttempts >= maxSamples || mValidCount >= SCAN_SAMPLES) {
        return true;
    }
    mSampleWait = SAMPLE_TICKS;
    return false;
}

void UltrasonicAlignTracer::runScanMeasure()
{
    mWalker->brake();
    if(mPhase == SCAN_SETTLE && mSettleRemaining == 0) {
        mPhase = SCAN_SAMPLE;
    }
    if(!stepMeasurement(SCAN_SAMPLES)) {
        return;
    }

    int turnWdeg = getTurnWdeg();
    bool valid = mValidCount > 0;
    mSweepValidSamples += mValidCount;
    LOGI("[ULTRA_ALIGN] scan: pos=%d min=%dmm valid=%d/%d\n",
         turnWdeg, valid ? mMinValidMm : -1, mValidCount, mSampleAttempts);
    feedCluster(mMinValidMm, valid, turnWdeg);

    // 近距離クラスタが確定したら残りを走査せず打ち切る
    if(mBestFound && !mClusterOpen && mBestMinMm <= EARLY_ACCEPT_MM) {
        LOGI("[ULTRA_ALIGN] early accept: min=%dmm\n", mBestMinMm);
        finishScan();
        return;
    }

    int direction = mSweepEndWdeg >= mTargetTurnWdeg ? 1 : -1;
    int next = mTargetTurnWdeg + direction * mScanStepWdeg;
    bool passed = direction > 0 ? next > mSweepEndWdeg : next < mSweepEndWdeg;
    if(passed) {
        finishScan();
        return;
    }
    mTargetTurnWdeg = next;
    mPhase = SCAN_STEP_TURN;
}

void UltrasonicAlignTracer::finishScan()
{
    closeCluster();
        LOGI("[ULTRA_ALIGN] sweep summary: direction=%s valid=%d errors=%d bridged=%d outOfRange=%d best=%d\n",
            mReverseSweep ? "reverse" : "forward", mSweepValidSamples,
            mSweepErrorSamples, mSweepBridgedErrorSamples, mSweepOutOfRangeSamples,
            mBestFound ? mBestMinMm : -1);
    if(mBestFound) {
        if(mNearAlignScan) {
            mNearAlignScan = false;
            mNearAlignDone = true;
            mTargetTurnWdeg = mBestCenterWdeg;
            mLastValidMm = mBestMinMm;
            mLastValidForwardWdeg = getForwardWdeg();
            mPhase = TURN_TO_TARGET;
            LOGI("[ULTRA_ALIGN] near align target: center=%d distance=%dmm\n",
                 mBestCenterWdeg, mBestMinMm);
            return;
        }
        if(!mPrecisionScan) {
            // 粗探索で得た反応帯の中央をそのまま正対方位として使う。
            // ボトルのような丸い対象は最短値より、反射が始まって終わる角度の
            // 中央の方が、接近後の姿勢誤差に強い。
            mFoundObject = true;
            mTargetTurnWdeg = mBestCenterWdeg;
            mLastValidMm = mBestMinMm;
            mPhase = TURN_TO_TARGET;
            LOGI("[ULTRA_ALIGN] target from sweep: center=%d wheelDeg distance=%dmm\n",
                 mBestCenterWdeg, mBestMinMm);
            return;
        }
        mFoundObject = true;
        mTargetTurnWdeg = mBestCenterWdeg;
        mLastValidMm = mBestMinMm;
        mPhase = TURN_TO_TARGET;
        LOGI("[ULTRA_ALIGN] target: center=%d wheelDeg distance=%dmm\n",
             mBestCenterWdeg, mBestMinMm);
    } else if(mNearAlignScan) {
        if(!mReverseSweep) {
            int reverseStartWdeg = getTurnWdeg();
            int reverseEndWdeg = 2 * mSweepCenterWdeg - reverseStartWdeg;
            mReverseSweep = true;
            LOGI("[ULTRA_ALIGN] near align reverse: %d -> %d wheelDeg\n",
                 reverseStartWdeg, reverseEndWdeg);
            mTargetTurnWdeg = reverseStartWdeg;
            mSweepEndWdeg = reverseEndWdeg;
            mClusterOpen = false;
            mBestFound = false;
            mSweepValidSamples = 0;
            mSweepErrorSamples = 0;
            mSweepBridgedErrorSamples = 0;
            mSweepOutOfRangeSamples = 0;
            mPhase = TURN_TO_SWEEP_START;
        } else {
            mNearAlignScan = false;
            mNearAlignDone = true;
            mTargetTurnWdeg = mNearAlignFallbackWdeg;
            mPhase = TURN_TO_TARGET;
            LOGI("[ULTRA_ALIGN] near align found nothing; keep heading=%d\n",
                 mTargetTurnWdeg);
        }
    } else if(!mPrecisionScan && !mReverseSweep) {
        // 同じ位置を逆方向にも走査する。センサ更新遅延・機体振動・円筒面の
        // 反射方向が走査方向に依存しても、片道だけで対象を捨てない。
        int reverseStartWdeg = getTurnWdeg();
        int reverseEndWdeg = 2 * mSweepCenterWdeg - reverseStartWdeg;
        mReverseSweep = true;
        LOGI("[ULTRA_ALIGN] reverse sweep: %d -> %d wheelDeg\n",
             reverseStartWdeg, reverseEndWdeg);
        startSweep(reverseStartWdeg, reverseEndWdeg, mPrimarySweep);
    } else if(mTargetVerifyScan) {
        LOGI("[ULTRA_ALIGN] target verify found nothing; rescan\n");
        startRescan();
    } else if(mPrecisionScan) {
        LOGI("[ULTRA_ALIGN] precision scan found nothing; resume rough search\n");
        startSweep(mSweepCenterWdeg + mHalfSweepWdeg,
                   mSweepCenterWdeg - mHalfSweepWdeg, mPrimarySweep);
    } else if(mPrimarySweep) {
        mReverseSweep = false;
        LOGI("[ULTRA_ALIGN] no object found in primary sweep\n");
        startCreep();
    } else {
        mReverseSweep = false;
        LOGI("[ULTRA_ALIGN] rescan found nothing\n");
        startRescan();
    }
}

void UltrasonicAlignTracer::feedCluster(int distanceMm, bool valid, int turnWdeg)
{
    if(!valid) {
        closeCluster();
        return;
    }
    if(!mClusterOpen) {
        mClusterOpen = true;
        mClusterMinMm = distanceMm;
        mClusterStartWdeg = turnWdeg;
        mClusterLastWdeg = turnWdeg;
        mClusterSampleCount = 0;
    } else if(std::abs(distanceMm - mClusterMinMm) <= CLUSTER_GAP_MM) {
        mClusterLastWdeg = turnWdeg;
        if(distanceMm < mClusterMinMm) {
            mClusterMinMm = distanceMm;
        }
    } else {
        closeCluster();
        mClusterOpen = true;
        mClusterMinMm = distanceMm;
        mClusterStartWdeg = turnWdeg;
        mClusterLastWdeg = turnWdeg;
        mClusterSampleCount = 0;
    }
    if(mClusterSampleCount < CLUSTER_MAX_SAMPLES) {
        mClusterSampleWdeg[mClusterSampleCount] = turnWdeg;
        mClusterSampleMm[mClusterSampleCount] = distanceMm;
        mClusterSampleCount++;
    }
}

void UltrasonicAlignTracer::closeCluster()
{
    if(!mClusterOpen) {
        return;
    }
    mClusterOpen = false;
    int width = std::abs(mClusterLastWdeg - mClusterStartWdeg);
    // 連続旋回中は対象がビームを横切る時間が短く、1サンプルしか取れない
    // ことがある。候補は捨てず、TURN_TO_TARGET 後の静止測定で検証する。
    if(mClusterSampleCount == 0) {
        return;
    }
    if(!mBestFound || mClusterMinMm < mBestMinMm) {
        mBestFound = true;
        mBestMinMm = mClusterMinMm;
        mBestMinWdeg = mClusterStartWdeg;
        for(int i = 0; i < mClusterSampleCount; i++) {
            if(mClusterSampleMm[i] == mClusterMinMm) {
                mBestMinWdeg = mClusterSampleWdeg[i];
            }
        }
        // 30msポーリングではセンサ内部の同じ値を複数回読む。反応帯の端で
        // 同一値が連続すると帯が走査方向へ伸びるため、先頭側は最後の重複、
        // 末尾側は最初の重複を端点として中央を求める。
        int centerStartIndex = 0;
        while(centerStartIndex + 1 < mClusterSampleCount
              && mClusterSampleMm[centerStartIndex + 1] == mClusterSampleMm[0]) {
            centerStartIndex++;
        }
        int centerEndIndex = mClusterSampleCount - 1;
        while(centerEndIndex > 0
              && mClusterSampleMm[centerEndIndex - 1]
                     == mClusterSampleMm[mClusterSampleCount - 1]) {
            centerEndIndex--;
        }
        int centerStartWdeg = mClusterSampleWdeg[centerStartIndex];
        int centerEndWdeg = mClusterSampleWdeg[centerEndIndex];
        mBestCenterWdeg = (centerStartWdeg + centerEndWdeg) / 2;
        LOGI("[ULTRA_ALIGN] cluster: center=%d span=%d..%d trimmed=%d..%d minPos=%d width=%d min=%dmm\n",
             mBestCenterWdeg, mClusterStartWdeg, mClusterLastWdeg,
             centerStartWdeg, centerEndWdeg, mBestMinWdeg, width, mClusterMinMm);
    }
}

void UltrasonicAlignTracer::startCreep()
{
    mWalker->brake();
    mReverseSweep = false;
    int creepMm = std::min(CREEP_FIRST_MM + mCreepAttempts * CREEP_INCREMENT_MM,
                           CREEP_MAX_MM);
    mCreepTargetWdeg = static_cast<int>(creepMm * WHEEL_DEG_PER_MM);
    if(mCreepAttempts >= MAX_CREEP_ATTEMPTS
       || getForwardWdeg() + mCreepTargetWdeg > mMaxApproachWdeg) {
        LOGI("[ULTRA_ALIGN] creep attempts exhausted\n");
        finish(false);
        return;
    }
    mCreepAttempts++;
    mTargetTurnWdeg = mSweepCenterWdeg;
    mCreepStartForwardWdeg = -1;  // 旋回完了後に前進開始位置を記録する
    mPhase = CREEPING;
    LOGI("[ULTRA_ALIGN] creep: attempt=%d heading=%d wheelDeg forward=%dmm then rescan\n",
            mCreepAttempts, mTargetTurnWdeg, creepMm);
}

void UltrasonicAlignTracer::runCreep()
{
    if(mCreepStartForwardWdeg < 0) {
        if(!driveTurnTo(mTargetTurnWdeg, TURN_PWM_MAX)) {
            return;
        }
        mCreepStartForwardWdeg = getForwardWdeg();
        mWalker->beginEncoderCorrection();
    }
    if(getForwardWdeg() - mCreepStartForwardWdeg < mCreepTargetWdeg) {
        driveForward(APPROACH_PWM);
        return;
    }
    mWalker->brake();
    int expandedHalfWdeg = std::min(
        static_cast<int>(MAX_SWEEP_HALF_BODY_DEG * WHEEL_DEG_PER_BODY_DEG),
        mHalfSweepWdeg + static_cast<int>(mCreepAttempts * 30 * WHEEL_DEG_PER_BODY_DEG));
    startSweep(expandedHalfWdeg, -expandedHalfWdeg, true);
}

void UltrasonicAlignTracer::startApproach()
{
    if(!mNearAlignDone) {
        mApproachStartForwardWdeg = getForwardWdeg();
    }
    mLastValidForwardWdeg = getForwardWdeg();
    mInvalidRounds = 0;
    mTargetVerifyAttempted = false;
    LOGI("[ULTRA_ALIGN] approach begin: heading=%d wheelDeg expect=%dmm\n",
         mTargetTurnWdeg, mLastValidMm);
    beginMeasurement(APPROACH_SETTLE);
}

void UltrasonicAlignTracer::runApproachMeasure()
{
    mWalker->brake();
    if(mPhase == APPROACH_SETTLE && mSettleRemaining == 0) {
        mPhase = APPROACH_SAMPLE;
    }
    if(!stepMeasurement(APPROACH_SAMPLES)) {
        return;
    }

    int forward = getForwardWdeg();
    int predicted = mLastValidMm
                    - static_cast<int>((forward - mLastValidForwardWdeg) / WHEEL_DEG_PER_MM);
    if(forward > mMaxApproachWdeg) {
        // クリープや再探索の前進も上限に含まれるため、目前まで来ているなら
        // 中断せず押し切る
        if(predicted >= 0 && predicted <= 200) {
            LOGI("[ULTRA_ALIGN] approach limit but target near: predicted=%dmm; push\n",
                 predicted);
            startPush(predicted);
        } else {
            LOGI("[ULTRA_ALIGN] approach limit reached without contact\n");
            finish(false);
        }
        return;
    }
    if(mValidCount == 0) {
        if(predicted <= BLIND_MM) {
            // ボトルがセンサ盲域に入った=接触寸前とみなす
            LOGI("[ULTRA_ALIGN] blind contact: predicted=%dmm\n", predicted);
            startPush(std::max(0, predicted));
            return;
        }
        if(mMeasurementErrorCount * 2 >= mSampleAttempts) {
            // 負値は無反射ではなくPUPドライバエラー。対象の反応帯と距離を
            // 既に確定できている間は、方位を維持して予測距離まで接近し、
            // 停止後に再測定する。
            LOGI("[ULTRA_ALIGN] distance driver errors=%d/%d predicted=%dmm; blind pulse\n",
                 mMeasurementErrorCount, mSampleAttempts, predicted);
            startPulse(predicted - PULSE_KEEP_MM);
            return;
        }
        mInvalidRounds++;
        LOGI("[ULTRA_ALIGN] no target echo: predicted=%dmm errors=%d/%d round=%d\n",
             predicted, mMeasurementErrorCount, mSampleAttempts, mInvalidRounds);
        if(mInvalidRounds >= 2) {
            if(!mTargetVerifyAttempted) {
                mTargetVerifyAttempted = true;
                startTargetVerifyScan();
            } else {
                startRescan();
            }
        } else {
            beginMeasurement(APPROACH_SETTLE);  // 一度だけ測り直す
        }
        return;
    }

    mInvalidRounds = 0;
    int distance = mMinValidMm;
    LOGI("[ULTRA_ALIGN] approach: distance=%dmm predicted=%dmm forward=%d\n",
         distance, predicted, forward);
    if(distance <= CONTACT_MM) {
        LOGI("[ULTRA_ALIGN] contact: distance=%dmm\n", distance);
        startPush(distance);
        return;
    }
    if(distance > predicted + LOST_RISE_MM) {
        LOGI("[ULTRA_ALIGN] lost: reading=%dmm predicted=%dmm\n", distance, predicted);
        startRescan();
        return;
    }
    mLastValidMm = distance;
    mLastValidForwardWdeg = forward;
    startPulse(distance - PULSE_KEEP_MM);
}

void UltrasonicAlignTracer::startPulse(int distanceMm)
{
    int pulseMm = distanceMm;
    if(pulseMm < PULSE_MIN_MM) pulseMm = PULSE_MIN_MM;
    if(pulseMm > PULSE_MAX_MM) pulseMm = PULSE_MAX_MM;
    mPulseStartForwardWdeg = getForwardWdeg();
    mPulseTargetWdeg = static_cast<int>(pulseMm * WHEEL_DEG_PER_MM);
    mWalker->beginEncoderCorrection();
    mPhase = APPROACH_PULSE;
    LOGD("[ULTRA_ALIGN] pulse: %dmm (%d wheelDeg)\n", pulseMm, mPulseTargetWdeg);
}

void UltrasonicAlignTracer::runApproachPulse()
{
    int travelled = getForwardWdeg() - mPulseStartForwardWdeg;
    if(travelled >= mPulseTargetWdeg) {
        int approachTravelled = getForwardWdeg() - mApproachStartForwardWdeg;
        if(!mNearAlignDone
           && approachTravelled >= static_cast<int>(NEAR_ALIGN_AFTER_MM * WHEEL_DEG_PER_MM)) {
            startNearAlignScan();
            return;
        }
        beginMeasurement(APPROACH_SETTLE);
        return;
    }
    driveForward(APPROACH_PWM);
}

void UltrasonicAlignTracer::startRescan()
{
    mWalker->brake();
    if(mRescanAttempts >= MAX_RESCAN_ATTEMPTS) {
        LOGI("[ULTRA_ALIGN] rescan attempts exhausted\n");
        finish(false);
        return;
    }
    mRescanAttempts++;
    mPendingRescanHalfWdeg =
        static_cast<int>(RESCAN_HALF_BODY_DEG * mRescanAttempts * WHEEL_DEG_PER_BODY_DEG);

    // ボトルの至近距離で旋回するとアームが当たって弾き飛ばすため、
    // 予測距離が近いときはまず安全距離まで後退してからスキャンする
    int predicted = 0;
    if(mLastValidMm >= 0) {
        predicted = mLastValidMm
                    - static_cast<int>((getForwardWdeg() - mLastValidForwardWdeg)
                                       / WHEEL_DEG_PER_MM);
        if(predicted < 0) predicted = 0;
    }
    int backupMm = RESCAN_STANDOFF_MM - predicted;
    if(backupMm > 0) {
        mBackupStartForwardWdeg = getForwardWdeg();
        mBackupTargetWdeg = static_cast<int>(backupMm * WHEEL_DEG_PER_MM);
        mWalker->beginEncoderCorrection();
        mPhase = BACKING;
        LOGI("[ULTRA_ALIGN] backup before rescan: attempt=%d predicted=%dmm backup=%dmm\n",
             mRescanAttempts, predicted, backupMm);
        return;
    }
    doRescanSweep();
}

void UltrasonicAlignTracer::runBackup()
{
    int travelled = mBackupStartForwardWdeg - getForwardWdeg();
    if(travelled < mBackupTargetWdeg) {
        int leftBoost = 0;
        int rightBoost = 0;
        updateStall(&leftBoost, &rightBoost);
        mWalker->runWithEncoderCorrection(-(APPROACH_PWM + leftBoost), -(APPROACH_PWM + rightBoost));
        return;
    }
    mWalker->brake();
    doRescanSweep();
}

void UltrasonicAlignTracer::doRescanSweep()
{
    int center = getTurnWdeg();
    mReverseSweep = false;
    LOGI("[ULTRA_ALIGN] rescan: attempt=%d center=%d half=%d wheelDeg\n",
         mRescanAttempts, center, mPendingRescanHalfWdeg);
    startSweep(center + mPendingRescanHalfWdeg, center - mPendingRescanHalfWdeg, false);
}

void UltrasonicAlignTracer::startPush(int remainingMm)
{
    mPushStartForwardWdeg = getForwardWdeg();
    mPushTargetWdeg = mPushWdeg + static_cast<int>(remainingMm * WHEEL_DEG_PER_MM);
    mWalker->beginEncoderCorrection();
    mPhase = PUSHING;
    LOGI("[ULTRA_ALIGN] push begin: %d wheelDeg (remaining=%dmm)\n",
         mPushTargetWdeg, remainingMm);
}

void UltrasonicAlignTracer::runPush()
{
    int travelled = getForwardWdeg() - mPushStartForwardWdeg;
    if(travelled >= mPushTargetWdeg) {
        finish(true);
        return;
    }
    driveForward(PUSH_PWM);
}

void UltrasonicAlignTracer::finish(bool pushed)
{
    startReturn(pushed);
}

void UltrasonicAlignTracer::startReturn(bool pushed)
{
    mWalker->brake();
    mReturnTargetForwardWdeg =
        getForwardWdeg() - static_cast<int>(RETURN_STANDOFF_MM * WHEEL_DEG_PER_MM);
    mWalker->beginEncoderCorrection();
    mPhase = RETURNING;
    LOGI("[ULTRA_ALIGN] return begin: found=%d pushed=%d lastDistance=%dmm targetForward=%d\n",
            mFoundObject ? 1 : 0, pushed ? 1 : 0, mLastValidMm, mReturnTargetForwardWdeg);
}

void UltrasonicAlignTracer::runReturn()
{
    if(getForwardWdeg() > mReturnTargetForwardWdeg) {
        mWalker->runWithEncoderCorrection(-RETURN_PWM, -RETURN_PWM);
        return;
    }
    mWalker->brake();
    mPhase = RETURN_TURNING;
}

bool UltrasonicAlignTracer::driveTurnTo(int targetWdeg, int pwmLimit)
{
    int error = targetWdeg - getTurnWdeg();
    if(std::abs(error) <= TURN_TOLERANCE_WDEG) {
        mWalker->brake();
        mLeftStallTicks = 0;
        mRightStallTicks = 0;
        return true;
    }
    int leftBoost = 0;
    int rightBoost = 0;
    updateStall(&leftBoost, &rightBoost);
    int pwm = TURN_PWM_MIN + std::abs(error) / 3;
    if(pwm > pwmLimit) {
        pwm = pwmLimit;
    }
    // 片輪だけ止まると機体が平行移動して方位がずれるため、止まっている側だけ強める
    if(error > 0) {
        mWalker->setPwm(-(pwm + leftBoost), pwm + rightBoost);
    } else {
        mWalker->setPwm(pwm + leftBoost, -(pwm + rightBoost));
    }
    mWalker->run();
    return false;
}

void UltrasonicAlignTracer::driveForward(int basePwm)
{
    int leftBoost = 0;
    int rightBoost = 0;
    updateStall(&leftBoost, &rightBoost);
    int error = mTargetTurnWdeg - getTurnWdeg();
    int diff = error / 2;
    if(diff > HEADING_DIFF_MAX) diff = HEADING_DIFF_MAX;
    if(diff < -HEADING_DIFF_MAX) diff = -HEADING_DIFF_MAX;
    mWalker->runWithEncoderCorrection(basePwm - diff + leftBoost, basePwm + diff + rightBoost);
}

void UltrasonicAlignTracer::updateStall(int* leftBoost, int* rightBoost)
{
    int left = mWalker->getLeftCount();
    int right = mWalker->getRightCount();
    if(std::abs(left - mPrevLeftCount) < 1) {
        mLeftStallTicks++;
    } else {
        mLeftStallTicks = 0;
    }
    if(std::abs(right - mPrevRightCount) < 1) {
        mRightStallTicks++;
    } else {
        mRightStallTicks = 0;
    }
    mPrevLeftCount = left;
    mPrevRightCount = right;
    *leftBoost = std::min(mLeftStallTicks / STALL_BOOST_DIV, static_cast<int>(STALL_BOOST_MAX));
    *rightBoost = std::min(mRightStallTicks / STALL_BOOST_DIV, static_cast<int>(STALL_BOOST_MAX));
}

int UltrasonicAlignTracer::getTurnWdeg() const
{
    int leftDelta = mWalker->getLeftCount() - mStartLeftCount;
    int rightDelta = mWalker->getRightCount() - mStartRightCount;
    return (rightDelta - leftDelta) / 2;
}

int UltrasonicAlignTracer::getForwardWdeg() const
{
    int leftDelta = mWalker->getLeftCount() - mStartLeftCount;
    int rightDelta = mWalker->getRightCount() - mStartRightCount;
    return (leftDelta + rightDelta) / 2;
}

int UltrasonicAlignTracer::readDistanceMm(bool* valid) const
{
    int distance = mUltrasonicSensor->getDistance();
    int upperLimit = std::min(mMaxDistanceMm, SENSOR_INVALID_MM);
    *valid = distance >= MIN_VALID_MM && distance < upperLimit;
    return distance;
}
