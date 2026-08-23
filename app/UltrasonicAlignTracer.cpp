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
      mStartLeftCount(0),
      mStartRightCount(0),
      mPhase(TURN_TO_SWEEP_START),
      mFoundObject(false),
      mTargetTurnWdeg(0),
      mSweepEndWdeg(0),
      mSweepCenterWdeg(0),
      mPrimarySweep(true),
            mReverseSweep(false),
    mTargetVerifyScan(false),
            mNearAlignScan(false),
            mNearAlignDone(false),
            mNearAlignFallbackWdeg(0),
      mSettleRemaining(0),
      mSampleWait(0),
      mSampleAttempts(0),
      mValidCount(0),
      mMinValidMm(0),
    mNoEchoCount(0),
      mBestFound(false),
            mBestMedianMm(0),
      mBestCenterWdeg(0),
            mSweepValidSamples(0),
            mSweepNoEchoSamples(0),
            mSweepOutOfRangeSamples(0),
            mScanPwm(SCAN_PWM_INITIAL),
            mScanSpeedTicks(0),
            mScanSpeedStartWdeg(0),
      mLastValidMm(-1),
      mLastValidForwardWdeg(0),
    mApproachStartForwardWdeg(0),
      mInvalidRounds(0),
    mTargetVerifyAttempted(false),
      mRescanAttempts(0),
      mCreepAttempts(0),
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
                case TURN_TO_TARGET:
                    runTurn();
                    break;
                case SCAN_SWEEP:
                    runScanSweep();
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
            primeDistance, primeDistance < 0 ? "no-echo" : "ready");
        mReverseSweep = false;
    mState = WALKING;
        LOGI("[ULTRA_ALIGN] start: half=%d wheelDeg speed=%ddeg/s max=%dmm push=%d wheelDeg\n",
            mHalfSweepWdeg, SCAN_TARGET_BODY_DEG_PER_SEC, mMaxDistanceMm, mPushWdeg);
    startSweep(mHalfSweepWdeg, -mHalfSweepWdeg, true);
}

void UltrasonicAlignTracer::startSweep(int startWdeg, int endWdeg, bool primary)
{
    mTargetTurnWdeg = startWdeg;
    mSweepEndWdeg = endWdeg;
    mSweepCenterWdeg = (startWdeg + endWdeg) / 2;
    mPrimarySweep = primary;
    mTargetVerifyScan = false;
    mNearAlignScan = false;
    if(!mReverseSweep) {
        resetHistogram();
    }
    mSweepValidSamples = 0;
    mSweepNoEchoSamples = 0;
    mSweepOutOfRangeSamples = 0;
    mPhase = TURN_TO_SWEEP_START;
    LOGI("[ULTRA_ALIGN] sweep plan: %d -> %d wheelDeg primary=%d\n",
         startWdeg, endWdeg, primary ? 1 : 0);
}

void UltrasonicAlignTracer::startTargetVerifyScan()
{
    int halfWdeg = static_cast<int>(TARGET_VERIFY_HALF_BODY_DEG * WHEEL_DEG_PER_BODY_DEG);
    int centerWdeg = mTargetTurnWdeg;
    mTargetTurnWdeg = std::min(BIN_MAX_WDEG, centerWdeg + halfWdeg);
    mSweepEndWdeg = std::max(-BIN_MAX_WDEG, centerWdeg - halfWdeg);
    mSweepCenterWdeg = centerWdeg;
    mTargetVerifyScan = true;
    mReverseSweep = false;
    resetHistogram();
    mSweepValidSamples = 0;
    mSweepNoEchoSamples = 0;
    mSweepOutOfRangeSamples = 0;
    mPhase = TURN_TO_SWEEP_START;
    LOGI("[ULTRA_ALIGN] target verify: %d -> %d wheelDeg around=%d\n",
         mTargetTurnWdeg, mSweepEndWdeg, centerWdeg);
}

void UltrasonicAlignTracer::startNearAlignScan()
{
    int halfWdeg = static_cast<int>(NEAR_ALIGN_HALF_BODY_DEG * WHEEL_DEG_PER_BODY_DEG);
    mNearAlignFallbackWdeg = mTargetTurnWdeg;
    mTargetTurnWdeg = std::min(BIN_MAX_WDEG, mNearAlignFallbackWdeg + halfWdeg);
    mSweepEndWdeg = std::max(-BIN_MAX_WDEG, mNearAlignFallbackWdeg - halfWdeg);
    mSweepCenterWdeg = mNearAlignFallbackWdeg;
    mPrimarySweep = false;
    mReverseSweep = false;
    mTargetVerifyScan = false;
    mNearAlignScan = true;
    resetHistogram();
    mSweepValidSamples = 0;
    mSweepNoEchoSamples = 0;
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
    } else if(mPhase == TURN_TO_SWEEP_START) {
        // 旋回中心から離れたセンサを連続移動させ、マルチパス位相を掃引する。
        mSampleWait = 0;
        mScanPwm = SCAN_PWM_INITIAL;
        mScanSpeedTicks = 0;
        mScanSpeedStartWdeg = getTurnWdeg();
        mPhase = SCAN_SWEEP;
    }
}

void UltrasonicAlignTracer::runScanSweep()
{
    if(mSampleWait > 0) {
        mSampleWait--;
    } else {
        bool hasEcho = false;
        bool valid = false;
        int distance = readDistanceMm(&hasEcho, &valid);
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
        // Bluetoothログ出力で走査周期を乱さないよう、有効エコーも間引いて記録する。
        if(valid) {
            LOGD_EVERY(10, "[ULTRA_ALIGN] sweep hit: pos=%d distance=%dmm\n",
                       turnWdeg, distance);
        } else if(!hasEcho) {
            mSweepNoEchoSamples++;
        } else {
            mSweepOutOfRangeSamples++;
        }
        if(valid) {
            recordHit(distance, turnWdeg);
            mSweepValidSamples++;
        }
        mSampleWait = SCAN_SAMPLE_INTERVAL_TICKS - 1;
    }

    if(driveScanTo(mSweepEndWdeg)) {
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
    mNoEchoCount = 0;
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

    bool hasEcho = false;
    bool valid = false;
    int distance = readDistanceMm(&hasEcho, &valid);
    LOGD("[ULTRA_ALIGN] sample: pos=%d raw=%dmm valid=%d %d/%d\n",
         getTurnWdeg(), distance, valid ? 1 : 0, mSampleAttempts + 1, maxSamples);
    if(valid) {
        if(mValidCount == 0 || distance < mMinValidMm) {
            mMinValidMm = distance;
        }
        mValidCount++;
    } else if(!hasEcho) {
        mNoEchoCount++;
    }
    mSampleAttempts++;
    if(mSampleAttempts >= maxSamples || mValidCount > 0) {
        return true;
    }
    mSampleWait = SAMPLE_TICKS;
    return false;
}

void UltrasonicAlignTracer::finishScan()
{
    LOGI("[ULTRA_ALIGN] sweep summary: direction=%s valid=%d noEcho=%d outOfRange=%d\n",
         mReverseSweep ? "reverse" : "forward", mSweepValidSamples,
         mSweepNoEchoSamples, mSweepOutOfRangeSamples);
    if(!mReverseSweep) {
        int reverseStartWdeg = getTurnWdeg();
        int reverseEndWdeg = 2 * mSweepCenterWdeg - reverseStartWdeg;
        mReverseSweep = true;
        mTargetTurnWdeg = reverseStartWdeg;
        mSweepEndWdeg = reverseEndWdeg;
        mSweepValidSamples = 0;
        mSweepNoEchoSamples = 0;
        mSweepOutOfRangeSamples = 0;
        mPhase = TURN_TO_SWEEP_START;
        LOGI("[ULTRA_ALIGN] reverse sweep: %d -> %d wheelDeg\n",
             reverseStartWdeg, reverseEndWdeg);
        return;
    }
    selectBestCluster();
    if(mBestFound) {
        if(mNearAlignScan) {
            mNearAlignScan = false;
            mNearAlignDone = true;
            mTargetTurnWdeg = mBestCenterWdeg;
            mSweepCenterWdeg = mBestCenterWdeg;
            mLastValidMm = mBestMedianMm;
            mLastValidForwardWdeg = getForwardWdeg();
            mPhase = TURN_TO_TARGET;
            LOGI("[ULTRA_ALIGN] near align target: center=%d distance=%dmm\n",
                  mBestCenterWdeg, mBestMedianMm);
            return;
        }
           if(!mTargetVerifyScan && !mFoundObject) {
            mFoundObject = true;
            mTargetTurnWdeg = mBestCenterWdeg;
              mLastValidMm = mBestMedianMm;
              LOGI("[ULTRA_ALIGN] candidate: center=%d wheelDeg median=%dmm; verify\n",
                  mBestCenterWdeg, mBestMedianMm);
              startTargetVerifyScan();
            return;
        }
        mFoundObject = true;
        mTargetTurnWdeg = mBestCenterWdeg;
          mSweepCenterWdeg = mBestCenterWdeg;
           mLastValidMm = mBestMedianMm;
        mPhase = TURN_TO_TARGET;
           LOGI("[ULTRA_ALIGN] target: center=%d wheelDeg median=%dmm\n",
               mBestCenterWdeg, mBestMedianMm);
    } else if(mNearAlignScan) {
        mNearAlignScan = false;
        mNearAlignDone = true;
        mTargetTurnWdeg = mNearAlignFallbackWdeg;
        mPhase = TURN_TO_TARGET;
        LOGI("[ULTRA_ALIGN] near align found nothing; keep heading=%d\n",
             mTargetTurnWdeg);
    } else if(mTargetVerifyScan) {
        LOGI("[ULTRA_ALIGN] target verify found nothing; rescan\n");
        startRescan();
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

void UltrasonicAlignTracer::resetHistogram()
{
    for(int bin = 0; bin < BIN_COUNT; bin++) {
        mBinHitCount[bin] = 0;
        mBinNextDistance[bin] = 0;
    }
    mBestFound = false;
}

void UltrasonicAlignTracer::recordHit(int distanceMm, int turnWdeg)
{
    if(turnWdeg < -BIN_MAX_WDEG || turnWdeg > BIN_MAX_WDEG) {
        return;
    }
    int bin = (turnWdeg + BIN_MAX_WDEG) / BIN_WIDTH_WDEG;
    if(bin < 0 || bin >= BIN_COUNT) {
        return;
    }
    int slot = mBinNextDistance[bin];
    mBinDistance[bin][slot] = distanceMm;
    mBinNextDistance[bin] = (slot + 1) % BIN_DISTANCE_CAPACITY;
    mBinHitCount[bin]++;
}

int UltrasonicAlignTracer::medianForBin(int bin) const
{
    int count = std::min(mBinHitCount[bin], static_cast<int>(BIN_DISTANCE_CAPACITY));
    int sorted[BIN_DISTANCE_CAPACITY];
    for(int i = 0; i < count; i++) {
        sorted[i] = mBinDistance[bin][i];
    }
    std::sort(sorted, sorted + count);
    return sorted[count / 2];
}

void UltrasonicAlignTracer::selectBestCluster()
{
    mBestFound = false;
    for(int start = 0; start < BIN_COUNT;) {
        if(mBinHitCount[start] == 0) {
            start++;
            continue;
        }
        int end = start;
        while(end + 1 < BIN_COUNT && mBinHitCount[end + 1] > 0) {
            end++;
        }

        int distances[CLUSTER_MAX_SAMPLES];
        int count = 0;
        for(int bin = start; bin <= end && count < CLUSTER_MAX_SAMPLES; bin++) {
            int binCount = std::min(mBinHitCount[bin], static_cast<int>(BIN_DISTANCE_CAPACITY));
            for(int sample = 0; sample < binCount && count < CLUSTER_MAX_SAMPLES; sample++) {
                distances[count++] = mBinDistance[bin][sample];
            }
        }
        std::sort(distances, distances + count);
        int median = distances[count / 2];

        // 約100ms更新の同一値が端の複数ビンへ尾を引く分を中央計算から除く。
                int trimmedStart = start;
                while(trimmedStart + 1 < end
              && medianForBin(trimmedStart) == medianForBin(trimmedStart + 1)) {
            trimmedStart++;
        }
        int trimmedEnd = end;
                while(trimmedEnd - 1 > start
              && medianForBin(trimmedEnd) == medianForBin(trimmedEnd - 1)) {
            trimmedEnd--;
        }
                if(trimmedStart > trimmedEnd) {
                        trimmedStart = start;
                        trimmedEnd = end;
                }
        int centerBin = (trimmedStart + trimmedEnd) / 2;
        int centerWdeg = centerBin * BIN_WIDTH_WDEG - BIN_MAX_WDEG;
        LOGI("[ULTRA_ALIGN] cluster: bins=%d..%d trimmed=%d..%d center=%d median=%dmm hits=%d\n",
             start, end, trimmedStart, trimmedEnd, centerWdeg, median, count);
        if(!mBestFound || median < mBestMedianMm) {
            mBestFound = true;
            mBestMedianMm = median;
            mBestCenterWdeg = centerWdeg;
        }
        start = end + 1;
    }
}

void UltrasonicAlignTracer::startCreep()
{
    mWalker->brake();
    mReverseSweep = false;
    if(mCreepAttempts >= MAX_CREEP_ATTEMPTS) {
        LOGI("[ULTRA_ALIGN] expanded search attempts exhausted\n");
        finish(false);
        return;
    }
    mCreepAttempts++;
    int expandedHalfWdeg = std::min(
        static_cast<int>(MAX_SWEEP_HALF_BODY_DEG * WHEEL_DEG_PER_BODY_DEG),
        mHalfSweepWdeg + static_cast<int>(mCreepAttempts * 30 * WHEEL_DEG_PER_BODY_DEG));
    LOGI("[ULTRA_ALIGN] expanded search: attempt=%d half=%d wheelDeg\n",
         mCreepAttempts, expandedHalfWdeg);
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
    // 連続確認走査のエコーは停止測定より新鮮で、静止フェードの影響も受けにくい。
    // 実測距離から安全余裕を残す最初のパルスだけは、その値を直接使う。
    if(mLastValidMm > CONTACT_MM) {
        startPulse(mLastValidMm - PULSE_KEEP_MM);
        return;
    }
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
    int travelledSinceEchoMm =
        static_cast<int>((forward - mLastValidForwardWdeg) / WHEEL_DEG_PER_MM);
    if(forward > mMaxApproachWdeg) {
        // クリープや再探索の前進も上限に含まれるため、目前まで来ているなら
        // 中断せず押し切る
          if(predicted >= 0 && predicted <= 200
              && travelledSinceEchoMm <= RECENT_ECHO_TRAVEL_MM) {
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
        if(predicted <= BLIND_MM && travelledSinceEchoMm <= RECENT_ECHO_TRAVEL_MM) {
            // 直近100mm以内に実測があり、ボトルが盲域へ入った場合だけ接触寸前とみなす。
            LOGI("[ULTRA_ALIGN] blind contact: predicted=%dmm\n", predicted);
            startPush(std::max(0, predicted));
            return;
        }
        mInvalidRounds++;
           LOGI("[ULTRA_ALIGN] no target echo: predicted=%dmm noEcho=%d/%d round=%d\n",
               predicted, mNoEchoCount, mSampleAttempts, mInvalidRounds);
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
    int backupMm = std::min(RESCAN_STANDOFF_MM - predicted, RESCAN_BACKUP_MAX_MM);
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
    int center = std::max(-BIN_MAX_WDEG, std::min(mSweepCenterWdeg, BIN_MAX_WDEG));
    int start = std::min(BIN_MAX_WDEG, center + mPendingRescanHalfWdeg);
    int end = std::max(-BIN_MAX_WDEG, center - mPendingRescanHalfWdeg);
    mReverseSweep = false;
    LOGI("[ULTRA_ALIGN] rescan: attempt=%d center=%d range=%d..%d wheelDeg\n",
        mRescanAttempts, center, start, end);
    startSweep(start, end, false);
}

void UltrasonicAlignTracer::startPush(int remainingMm)
{
    mPushStartForwardWdeg = getForwardWdeg();
    mPushTargetWdeg = mPushWdeg + static_cast<int>(remainingMm * WHEEL_DEG_PER_MM);
    mSampleWait = SAMPLE_TICKS;
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

    if(--mSampleWait <= 0) {
        bool hasEcho = false;
        bool inRange = false;
        int distance = readDistanceMm(&hasEcho, &inRange);
        mSampleWait = SAMPLE_TICKS;
        if(hasEcho && inRange && distance > CONTACT_MM + PUSH_LOST_RISE_MM) {
            LOGI("[ULTRA_ALIGN] push lost: reading=%dmm; backup=%dmm then rescan\n",
                 distance, PUSH_LOST_BACKUP_MM);
            startPushLostRescan();
            return;
        }
    }
    driveForward(PUSH_PWM);
}

void UltrasonicAlignTracer::startPushLostRescan()
{
    mWalker->brake();
    mBackupStartForwardWdeg = getForwardWdeg();
    mBackupTargetWdeg = static_cast<int>(PUSH_LOST_BACKUP_MM * WHEEL_DEG_PER_MM);
    // 現在方位に依存せず、探索可能な全範囲を往復する。
    mPendingRescanHalfWdeg = BIN_MAX_WDEG * 2;
    mWalker->beginEncoderCorrection();
    mPhase = BACKING;
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

bool UltrasonicAlignTracer::driveScanTo(int targetWdeg)
{
    int error = targetWdeg - getTurnWdeg();
    if(std::abs(error) <= TURN_TOLERANCE_WDEG) {
        mWalker->brake();
        return true;
    }

    mScanSpeedTicks++;
    if(mScanSpeedTicks >= SCAN_SPEED_WINDOW_TICKS) {
        int travelled = std::abs(getTurnWdeg() - mScanSpeedStartWdeg);
        int targetTravel = static_cast<int>(SCAN_TARGET_BODY_DEG_PER_SEC
                                            * WHEEL_DEG_PER_BODY_DEG
                                            * SCAN_SPEED_WINDOW_TICKS / 100.0);
        mScanPwm += (targetTravel - travelled) / 2;
        mScanPwm = std::max(SCAN_PWM_MIN, std::min(mScanPwm, SCAN_PWM_MAX));
        int bodyDegPerSec = static_cast<int>(travelled * 100.0
                                            / WHEEL_DEG_PER_BODY_DEG
                                            / SCAN_SPEED_WINDOW_TICKS);
        LOGD_EVERY(10, "[ULTRA_ALIGN] scan drive: speed=%ddeg/s pwm=%d travelled=%d\n",
                   bodyDegPerSec, mScanPwm, travelled);
        mScanSpeedTicks = 0;
        mScanSpeedStartWdeg = getTurnWdeg();
    }

    int leftBoost = 0;
    int rightBoost = 0;
    updateStall(&leftBoost, &rightBoost);
    if(error > 0) {
        mWalker->setPwm(-(mScanPwm + leftBoost), mScanPwm + rightBoost);
    } else {
        mWalker->setPwm(mScanPwm + leftBoost, -(mScanPwm + rightBoost));
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

int UltrasonicAlignTracer::readDistanceMm(bool* hasEcho, bool* inRange) const
{
    int distance = mUltrasonicSensor->getDistance();
    *hasEcho = distance >= MIN_VALID_MM && distance < SENSOR_MAX_VALID_MM;
    *inRange = *hasEcho && distance <= mMaxDistanceMm;
    return distance;
}
