#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <vector>

#define TRACER_H
#define ETTR_UNIT_WALKER_H_
#define SPIKE_CPP_API_ULTRASONIC_SENSOR_H_
#define ETTR_LOG_LEVEL 0

class Starter {
public:
    bool isPushed() { return true; }
};

class Tracer {
public:
    enum State { UNDEFINED, WAITING_FOR_START, WALKING, TERMINATED };
    State mState;
    std::vector<Starter*> mStarterList;
    virtual void run() = 0;
    virtual ~Tracer() = default;
};

class Walker {
public:
    int left = 0;
    int right = 0;
    int leftPwm = 0;
    int rightPwm = 0;
    int getLeftCount() { return left; }
    int getRightCount() { return right; }
    void brake() { setPwm(0, 0); }
    void beginEncoderCorrection() {}
    void setPwm(int leftValue, int rightValue) {
        leftPwm = leftValue;
        rightPwm = rightValue;
    }
    void run() {}
    void runWithEncoderCorrection(int leftValue, int rightValue) {
        setPwm(leftValue, rightValue);
    }
};

namespace spikeapi {
class UltrasonicSensor {
public:
    int distance = -1;
    int getDistance() const { return distance; }
};
}

void ettr_log_write(const char*, ...) {}

#define private public
#include "../app/UltrasonicAlignTracer.cpp"
#undef private

const int UltrasonicAlignTracer::BIN_MAX_WDEG;
const int UltrasonicAlignTracer::HEADING_DIFF_MAX;
const int UltrasonicAlignTracer::SCAN_PWM_MIN;
const int UltrasonicAlignTracer::SCAN_PWM_MAX;
const int UltrasonicAlignTracer::RESCAN_BACKUP_MAX_MM;
const int UltrasonicAlignTracer::MIN_VALID_MM;

int main()
{
    Walker walker;
    spikeapi::UltrasonicSensor sensor;
    auto makeTracer = [&]() {
        return UltrasonicAlignTracer(&walker, &sensor, 60, 500, 280, 35, 50, 50, 10, 33);
    };

    for(int direction : {-1, 1}) {
        for(int drift : {-20, 20}) {
            walker.left = walker.right = 0;
            auto tracer = makeTracer();
            tracer.driveRotation(direction * 25);
            walker.left = drift - direction * 10;
            walker.right = drift + direction * 10;
            tracer.driveRotation(direction * 25);
            assert((walker.leftPwm + walker.rightPwm) * drift < 0);
            assert(walker.leftPwm * direction < 0);
            assert(walker.rightPwm * direction > 0);
        }
    }
    walker.left = walker.right = 0;

    auto fineScan = makeTracer();
    fineScan.mScanTargetBodyDegPerSec = 75;
    assert(fineScan.scanTargetBodyDegPerSec() == 75);
    fineScan.mTargetVerifyScan = true;
    assert(fineScan.scanTargetBodyDegPerSec() == 35);
    fineScan.mTargetVerifyScan = false;
    fineScan.mNearAlignScan = true;
    assert(fineScan.scanTargetBodyDegPerSec() == 35);
    fineScan.mScanTargetBodyDegPerSec = 20;
    assert(fineScan.scanTargetBodyDegPerSec() == 20);

    auto tiedClusters = makeTracer();
    tiedClusters.resetHistogram();
    tiedClusters.mSweepCenterWdeg = -2;
    for(int bin : {37, 38, 39, 40, 41, 43, 44, 46, 47, 48, 50, 52, 53, 55, 56}) {
        tiedClusters.recordHit(294, bin * 3 - 140);
    }
    tiedClusters.selectBestCluster();
    assert(tiedClusters.mBestCenterWdeg == 1);
    auto fastClusters = makeTracer();
    fastClusters.resetHistogram();
    fastClusters.mSweepCenterWdeg = -50;
    for(int bin : {20, 22, 23, 25, 26, 28, 29, 31, 32, 34, 36, 37, 39}) {
        fastClusters.recordHit(294, bin * 3 - 140);
    }
    fastClusters.selectBestCluster();
    assert(fastClusters.mBestCenterWdeg == -47);
    for(int center : {-32, 28}) {
        auto tie = makeTracer();
        tie.resetHistogram();
        tie.mSweepCenterWdeg = center;
        tie.recordHit(294, -32);
        tie.recordHit(294, 28);
        tie.selectBestCluster();
        assert(tie.mBestCenterWdeg == center);
        tie.recordHit(200, 70);
        tie.selectBestCluster();
        assert(tie.mBestCenterWdeg == 70);
        assert(tie.mBestMedianMm == 200);
    }

    auto hit = makeTracer();
    hit.startSearching();
    hit.recordHit(280, 0);
    hit.finishScan();
    assert(hit.mTargetVerifyScan && !hit.mReverseSweep);
    hit.recordHit(275, 0);
    hit.finishScan();
    assert(hit.mPhase == hit.TURN_TO_TARGET);
    assert(hit.mLastValidMm == 275);

    struct VerifyCase {
        int distanceMm;
        int forwardWdeg;
        int creepAttempts;
        bool approached;
        bool translates;
        bool backs;
    };
    for(const VerifyCase& scenario : {
            VerifyCase{300, 0, 1, false, true, false},
            VerifyCase{201, 0, 0, false, true, false},
            VerifyCase{200, 0, 0, false, false, false},
            VerifyCase{120, 0, 0, false, false, true},
            VerifyCase{-1, 0, 0, false, false, true},
            VerifyCase{300, 307, 0, false, false, false},
            VerifyCase{300, 0, 5, false, false, false},
            VerifyCase{300, 0, 0, true, false, false},
            VerifyCase{2000, 1600, 0, false, false, false}}) {
        auto verifyMiss = makeTracer();
        verifyMiss.startSearching();
        if(scenario.approached) {
            verifyMiss.mLastValidMm = 300;
            verifyMiss.startApproach();
            assert(verifyMiss.mApproachStarted);
        }
        verifyMiss.mLastValidMm = scenario.distanceMm;
        verifyMiss.mLastValidForwardWdeg = 0;
        verifyMiss.mCreepAttempts = scenario.creepAttempts;
        walker.left = walker.right = scenario.forwardWdeg;
        verifyMiss.startTargetVerifyScan();
        verifyMiss.finishScan();
        assert(verifyMiss.mReverseSweep);
        assert(verifyMiss.mCreepAttempts == scenario.creepAttempts);
        verifyMiss.finishScan();
        if(scenario.translates) {
            assert(verifyMiss.mPhase == verifyMiss.CREEP_TURN);
            assert(verifyMiss.mCreepAttempts == scenario.creepAttempts + 1);
            assert(verifyMiss.mRescanAttempts == 0);
            assert(verifyMiss.mCreepTargetWdeg == 102);
            assert(verifyMiss.mLastValidMm == -1);
            assert(verifyMiss.mTargetTurnWdeg == 0);
            assert(walker.leftPwm == 0 && walker.rightPwm == 0);
        } else {
            assert(verifyMiss.mPhase == (scenario.backs
                ? verifyMiss.BACKING : verifyMiss.TURN_TO_SWEEP_START));
            assert(verifyMiss.mRescanAttempts == 1);
            assert(verifyMiss.mCreepAttempts == scenario.creepAttempts);
        }
        walker.left = walker.right = 0;
    }
    auto recoveringVerify = makeTracer();
    recoveringVerify.startSearching();
    recoveringVerify.mLastValidMm = 300;
    recoveringVerify.mLostRecovery = true;
    recoveringVerify.startTargetVerifyScan();
    recoveringVerify.finishScan();
    recoveringVerify.finishScan();
    assert(recoveringVerify.mPhase == recoveringVerify.RETURNING);
    assert(recoveringVerify.mCreepAttempts == 0);

    auto missed = makeTracer();
    missed.startSearching();
    missed.finishScan();
    assert(missed.mPhase == missed.CREEP_TURN);
    assert(missed.mCreepTargetWdeg == 102);
    for(int attempt = 1; attempt < missed.MAX_CREEP_ATTEMPTS; attempt++) {
        missed.startCreep();
    }
    assert(missed.mCreepAttempts == 5);
    missed.startCreep();
    assert(missed.mPhase == missed.RETURNING);

    auto finalSweep = makeTracer();
    finalSweep.startSearching();
    finalSweep.mCreepAttempts = finalSweep.MAX_CREEP_ATTEMPTS;
    finalSweep.finishScan();
    assert(finalSweep.mReverseSweep);
    finalSweep.finishScan();
    assert(finalSweep.mPhase == finalSweep.RETURNING);

    for(int distance : {120, 280}) {
        auto creep = makeTracer();
        creep.startSearching();
        creep.startCreep();
        sensor.distance = distance;
        creep.mSampleWait = 0;
        creep.runCreep();
        assert(walker.leftPwm == 0 && walker.rightPwm == 0);
        assert(creep.mLastValidMm == distance);
        if(distance <= 150) {
            assert(creep.mPhase == creep.TURN_TO_TARGET);
        } else {
            assert(creep.mTargetVerifyScan);
        }
    }
    sensor.distance = -1;

    auto rescan = makeTracer();
    rescan.startSearching();
    rescan.mLastValidMm = 120;
    rescan.startRescan();
    assert(rescan.mPhase == rescan.BACKING);
    rescan.mLastValidMm = 400;
    rescan.startRescan();
    assert(rescan.mPhase == rescan.TURN_TO_SWEEP_START);
    rescan.finishScan();
    assert(rescan.mReverseSweep);
    rescan.finishScan();
    assert(rescan.mPhase == rescan.CREEP_TURN);
    assert(rescan.mLastValidMm == -1);

    auto measurement = makeTracer();
    measurement.beginMeasurement(measurement.APPROACH_SETTLE);
    measurement.mSettleRemaining = 0;
    assert(!measurement.stepMeasurement(5));
    for(int tick = 0; tick < 9; tick++) {
        assert(!measurement.stepMeasurement(5));
    }
    assert(measurement.mSampleAttempts == 1);
    assert(!measurement.stepMeasurement(5));
    assert(measurement.mSampleAttempts == 2);

    auto staleApproach = makeTracer();
    staleApproach.startSearching();
    staleApproach.mLastValidMm = 352;
    staleApproach.mLastValidForwardWdeg = 0;
    walker.left = walker.right = 320;
    staleApproach.startApproach();
    assert(staleApproach.mLastValidForwardWdeg == 0);
    assert(staleApproach.mPhase == staleApproach.APPROACH_SETTLE);
    assert(walker.leftPwm == 0 && walker.rightPwm == 0);
    walker.left = walker.right = 0;

    auto nearLost = makeTracer();
    nearLost.startSearching();
    nearLost.mLastValidMm = 352;
    nearLost.mTargetTurnWdeg = -50;
    nearLost.mPulseStartForwardWdeg = 0;
    walker.left = walker.right = 320;
    nearLost.startNearAlignScan();
    nearLost.finishScan();
    nearLost.finishScan();
    assert(nearLost.mPhase == nearLost.LOST_RETURN_TURN);
    assert(nearLost.mTargetTurnWdeg == -50);
    assert(nearLost.mPendingLostBackupMm == 150);
    nearLost.startLostBackup(nearLost.mPendingLostBackupMm);
    assert(nearLost.mPhase == nearLost.BACKING);
    assert(nearLost.mBackupTargetWdeg == 307);
    nearLost.doRescanSweep();
    nearLost.finishScan();
    nearLost.finishScan();
    assert(nearLost.mPhase == nearLost.RETURNING);
    assert(nearLost.mCreepAttempts == 0);
    walker.left = walker.right = 0;

    auto pushLost = makeTracer();
    pushLost.startSearching();
    pushLost.startPush(80);
    sensor.distance = 900;
    pushLost.mSampleWait = 1;
    pushLost.runPush();
    assert(pushLost.mPhase == pushLost.BACKING);
    assert(pushLost.mBackupTargetWdeg == 204);
    assert(walker.leftPwm == 0 && walker.rightPwm == 0);
    pushLost.mLostRecoveryAttempts = pushLost.MAX_RESCAN_ATTEMPTS;
    pushLost.startPushLostRescan();
    assert(pushLost.mPhase == pushLost.RETURNING);
    sensor.distance = -1;

    for(int reverseCenter : {13, -14, -29, 1, -32, 4, 999}) {
        auto edgeHit = makeTracer();
        edgeHit.startSearching();
        edgeHit.mTargetTurnWdeg = -14;
        edgeHit.startNearAlignScan();
        edgeHit.recordHit(286, 13);
        edgeHit.recordHit(286, 16);
        walker.left = 45;
        walker.right = -45;
        edgeHit.finishScan();
        assert(edgeHit.mReverseSweep);
        assert(edgeHit.mNearAlignVerifyRequired);
        assert(edgeHit.mPhase == edgeHit.TURN_TO_SWEEP_START);
        edgeHit.selectBestCluster();
        assert(!edgeHit.mBestFound);
        if(reverseCenter != 999) {
            edgeHit.recordHit(285, reverseCenter);
        }
        edgeHit.finishScan();
        if(reverseCenter == 13 || reverseCenter == -14
           || reverseCenter == -29 || reverseCenter == 1) {
            assert(edgeHit.mPhase == edgeHit.TURN_TO_TARGET);
            assert(edgeHit.mTargetTurnWdeg == reverseCenter);
            assert(edgeHit.mLastValidMm == 285);
            assert(edgeHit.mNearAlignDone);
            assert(!edgeHit.mNearAlignScan);
            assert(edgeHit.mLostRecoveryAttempts == 0);
        } else {
            assert(edgeHit.mPhase == edgeHit.LOST_RETURN_TURN);
            assert(edgeHit.mTargetTurnWdeg == -14);
        }
        walker.left = walker.right = 0;
    }
    auto loggedCorrection = makeTracer();
    loggedCorrection.startSearching();
    loggedCorrection.mTargetTurnWdeg = -68;
    loggedCorrection.mLastValidMm = 325;
    loggedCorrection.startNearAlignScan();
    loggedCorrection.recordHit(141, -77);
    loggedCorrection.recordHit(126, -41);
    walker.left = 718;
    walker.right = 526;
    loggedCorrection.finishScan();
    assert(loggedCorrection.mNearAlignVerifyRequired);
    assert(loggedCorrection.mNearAlignVerifyCenterWdeg == -41);
    assert(loggedCorrection.mReverseSweep);
    loggedCorrection.recordHit(137, -71);
    walker.left = 658;
    walker.right = 572;
    loggedCorrection.finishScan();
    assert(loggedCorrection.mPhase == loggedCorrection.TURN_TO_TARGET);
    assert(loggedCorrection.mTargetTurnWdeg == -71);
    assert(loggedCorrection.mLastValidMm == 137);
    assert(loggedCorrection.mLastValidForwardWdeg == 615);
    assert(loggedCorrection.mNearAlignDone);
    assert(loggedCorrection.mLostRecoveryAttempts == 0);
    walker.left = walker.right = 0;

    auto smallCorrection = makeTracer();
    smallCorrection.startSearching();
    smallCorrection.mTargetTurnWdeg = -14;
    smallCorrection.startNearAlignScan();
    smallCorrection.recordHit(308, -23);
    smallCorrection.finishScan();
    assert(smallCorrection.mPhase == smallCorrection.TURN_TO_TARGET);
    assert(!smallCorrection.mReverseSweep);

    for(int attempt : {4, 5}) {
        auto translated = makeTracer();
        translated.startSearching();
        translated.mCreepAttempts = attempt;
        translated.mCreepStartForwardWdeg = 0;
        translated.mCreepTargetWdeg = 102;
        walker.left = walker.right = 102;
        sensor.distance = -1;
        translated.runCreep();
        int expectedHalf = attempt == 5 ? 140 : 93;
        assert(translated.mTargetTurnWdeg == expectedHalf);
        assert(translated.mSweepEndWdeg == -expectedHalf);
        if(attempt == 5) {
            translated.recordHit(220, -124);
            translated.finishScan();
            assert(translated.mTargetVerifyScan);
            assert(translated.mLastValidMm == 220);
        }
        walker.left = walker.right = 0;
    }
    auto wideMiss = makeTracer();
    wideMiss.startSearching();
    wideMiss.mCreepAttempts = 5;
    wideMiss.mCreepTargetWdeg = 102;
    walker.left = walker.right = 102;
    wideMiss.runCreep();
    walker.left = 242;
    walker.right = -38;
    wideMiss.finishScan();
    assert(wideMiss.mReverseSweep);
    assert(wideMiss.mSweepEndWdeg == 140);
    wideMiss.finishScan();
    assert(wideMiss.mPhase == wideMiss.RETURNING);
    assert(wideMiss.mCreepAttempts == 5);
    walker.left = walker.right = 0;

    auto limit = makeTracer();
    limit.startSearching();
    limit.startCreep();
    walker.left = walker.right = limit.mMaxApproachWdeg;
    limit.runCreep();
    assert(limit.mPhase == limit.RETURNING);
    std::puts("ultrasonic_align_test: all checks passed");
}