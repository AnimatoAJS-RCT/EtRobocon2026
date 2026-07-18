#include "UltrasonicAlignTracer.h"

#include "Log.h"

#include <algorithm>
#include <cstdlib>

UltrasonicAlignTracer::UltrasonicAlignTracer(Walker* walker,
                                               const spikeapi::UltrasonicSensor* ultrasonicSensor,
                                               int halfSweepAngleDeg,
                                               int turnPwm,
                                               int stepAngleDeg,
                                               int sampleCount,
                                 int maxDistanceMm,
                                 double wheelDegreesPerBodyDegree,
                                 int centerBandMm)
    : mWalker(walker),
      mUltrasonicSensor(ultrasonicSensor),
    mHalfSweepWheelDegrees(0),
    mStepWheelDegrees(0),
    mFineStepWheelDegrees(0),
    mWheelDegreesPerBodyDegree(std::max(0.1, wheelDegreesPerBodyDegree)),
    mTurnPwm(std::max(1, std::abs(turnPwm))),
      mSampleCount(std::max(1, std::min(sampleCount, MAX_SAMPLES))),
    mMaxDistanceMm(std::max(1, maxDistanceMm)),
    mCenterBandMm(std::max(1, centerBandMm)),
      mStartLeftCount(0),
      mStartRightCount(0),
      mTargetWheelDegrees(0),
    mSweepEndWheelDegrees(0),
    mFineSweepEndWheelDegrees(0),
    mLastSampleWheelDegrees(0),
      mBestWheelDegrees(0),
      mBestDistanceMm(0),
      mSampleAttempts(0),
      mValidSampleCount(0),
        mFineMeasurementCount(0),
        mFinePass(0),
      mFoundObject(false),
    mPhase(TURNING_TO_COARSE_START)
{
    mHalfSweepWheelDegrees = static_cast<int>(std::abs(halfSweepAngleDeg)
                                               * mWheelDegreesPerBodyDegree);
    mStepWheelDegrees = std::max(
        1, static_cast<int>(std::abs(stepAngleDeg) * mWheelDegreesPerBodyDegree));
    mFineStepWheelDegrees = std::max(1, mStepWheelDegrees / 2);
    for(int pass = 0; pass < FINE_SWEEP_PASSES; pass++) {
        mFineCenterWheelDegrees[pass] = 0;
        mFineCenterFound[pass] = false;
    }
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
            if(mPhase == TURNING_TO_COARSE_START || mPhase == TURNING_TO_FINE_START
               || mPhase == TURNING_TO_BEST) {
                turnToTarget();
            } else {
                sweepAndMeasure();
            }
            break;
        case TERMINATED:
            break;
        default:
            break;
    }
}

void UltrasonicAlignTracer::startSearching()
{
    mStartLeftCount = mWalker->getLeftCount();
    mStartRightCount = mWalker->getRightCount();
    mTargetWheelDegrees = mHalfSweepWheelDegrees;
    mPhase = TURNING_TO_COARSE_START;
    mState = WALKING;
        LOGI("[ULTRA_ALIGN] start: half=%d wheelDeg step=%d wheelDeg pwm=%d samples=%d max=%dmm ratio=%.2f band=%dmm\n",
            mHalfSweepWheelDegrees, mStepWheelDegrees, mTurnPwm, mSampleCount, mMaxDistanceMm,
            mWheelDegreesPerBodyDegree, mCenterBandMm);
}

void UltrasonicAlignTracer::turnToTarget()
{
    int currentWheelDegrees = getTurnWheelDegrees();
    int remaining = mTargetWheelDegrees - currentWheelDegrees;
    if(remaining > TURN_TOLERANCE_DEG) {
        mWalker->setPwm(-mTurnPwm, mTurnPwm);
        mWalker->run();
        return;
    }
    if(remaining < -TURN_TOLERANCE_DEG) {
        mWalker->setPwm(mTurnPwm, -mTurnPwm);
        mWalker->run();
        return;
    }

    mWalker->brake();
    if(mPhase == TURNING_TO_BEST) {
        LOGI("[ULTRA_ALIGN] complete: found=%d distance=%dmm target=%d wheelDeg\n",
             mFoundObject ? 1 : 0, mBestDistanceMm, mTargetWheelDegrees);
        mState = TERMINATED;
        return;
    }

    if(mPhase == TURNING_TO_COARSE_START) {
        startSweep(COARSE_SWEEPING, -mHalfSweepWheelDegrees);
    } else {
        startSweep(FINE_SWEEPING, mFineSweepEndWheelDegrees);
    }
}

void UltrasonicAlignTracer::startSweep(Phase phase, int endWheelDegrees)
{
    mPhase = phase;
    mSweepEndWheelDegrees = endWheelDegrees;
    int sampleStep = (phase == FINE_SWEEPING) ? mFineStepWheelDegrees : mStepWheelDegrees;
    mLastSampleWheelDegrees = getTurnWheelDegrees() - sampleStep;
    mSampleAttempts = 0;
    mValidSampleCount = 0;
    if(phase == FINE_SWEEPING) {
        mFineMeasurementCount = 0;
    }
}

void UltrasonicAlignTracer::sweepAndMeasure()
{
    int wheelDegrees = getTurnWheelDegrees();
    bool hasReachedEnd = (mSweepEndWheelDegrees < wheelDegrees)
                             ? (wheelDegrees <= mSweepEndWheelDegrees)
                             : (wheelDegrees >= mSweepEndWheelDegrees);
    if(hasReachedEnd) {
        mWalker->brake();
        if(mPhase == COARSE_SWEEPING) {
            startFineSweep();
        } else {
            calculateFineCenter();
            mFinePass++;
            if(mFinePass < FINE_SWEEP_PASSES) {
                startFineSweepPass();
            } else {
                selectFinalAngle();
                mPhase = TURNING_TO_BEST;
            }
        }
        return;
    }

    if(mSweepEndWheelDegrees < wheelDegrees) {
        mWalker->setPwm(mTurnPwm, -mTurnPwm);
    } else {
        mWalker->setPwm(-mTurnPwm, mTurnPwm);
    }
    mWalker->run();

    int sampleStep = (mPhase == FINE_SWEEPING) ? mFineStepWheelDegrees : mStepWheelDegrees;
    if(std::abs(wheelDegrees - mLastSampleWheelDegrees) < sampleStep
       && mSampleAttempts == 0) {
        return;
    }

    collectMeasurement(wheelDegrees);
}

void UltrasonicAlignTracer::collectMeasurement(int wheelDegrees)
{
    int distance = mUltrasonicSensor->getDistance();
    if(distance > 0 && distance <= mMaxDistanceMm) {
        mSamples[mValidSampleCount] = distance;
        mSampleWheelDegrees[mValidSampleCount] = wheelDegrees;
        mValidSampleCount++;
    }
    mSampleAttempts++;
    if(mSampleAttempts < mSampleCount) {
        return;
    }

    evaluateSamples();
    mLastSampleWheelDegrees = wheelDegrees;
    mSampleAttempts = 0;
    mValidSampleCount = 0;
}

void UltrasonicAlignTracer::evaluateSamples()
{
    if(mValidSampleCount == 0) {
        LOGD("[ULTRA_ALIGN] position=%d wheelDeg no valid measurement\n", mTargetWheelDegrees);
        return;
    }

    int medianIndex = getMedianSampleIndex();
    int distance = mSamples[medianIndex];
    int wheelDegreeSum = 0;
    for(int index = 0; index < mValidSampleCount; index++) {
        wheelDegreeSum += mSampleWheelDegrees[index];
    }
    int wheelDegrees = wheelDegreeSum / mValidSampleCount;
    LOGD("[ULTRA_ALIGN] position=%d wheelDeg distance=%dmm samples=%d\n", wheelDegrees, distance,
         mValidSampleCount);
    if(mPhase == COARSE_SWEEPING && (!mFoundObject || distance < mBestDistanceMm)) {
        mFoundObject = true;
        mBestDistanceMm = distance;
        mBestWheelDegrees = wheelDegrees;
    }
    if(mPhase == FINE_SWEEPING && mFineMeasurementCount < MAX_FINE_MEASUREMENTS) {
        mFineDistances[mFineMeasurementCount] = distance;
        mFineWheelDegrees[mFineMeasurementCount] = wheelDegrees;
        mFineMeasurementCount++;
    }
}

void UltrasonicAlignTracer::startFineSweep()
{
    if(!mFoundObject) {
        mTargetWheelDegrees = 0;
        mPhase = TURNING_TO_BEST;
        return;
    }

    mFinePass = 0;
    startFineSweepPass();
}

void UltrasonicAlignTracer::startFineSweepPass()
{
    int fineOffset = static_cast<int>(FINE_SWEEP_HALF_ANGLE_DEG
                                      * mWheelDegreesPerBodyDegree);
    int fineStart = mBestWheelDegrees;
    if(mFinePass == 0) {
        fineStart += fineOffset;
        mFineSweepEndWheelDegrees = mBestWheelDegrees - fineOffset;
    } else {
        fineStart -= fineOffset;
        mFineSweepEndWheelDegrees = mBestWheelDegrees + fineOffset;
    }
    int fineLimit = mHalfSweepWheelDegrees;
    if(fineStart > fineLimit) fineStart = fineLimit;
    if(fineStart < -fineLimit) fineStart = -fineLimit;
    if(mFineSweepEndWheelDegrees > fineLimit) mFineSweepEndWheelDegrees = fineLimit;
    if(mFineSweepEndWheelDegrees < -fineLimit) mFineSweepEndWheelDegrees = -fineLimit;
    mTargetWheelDegrees = fineStart;
    mPhase = TURNING_TO_FINE_START;
}

void UltrasonicAlignTracer::calculateFineCenter()
{
    if(mFineMeasurementCount < 3) return;

    int bestIndex = 0;
    for(int index = 1; index < mFineMeasurementCount; index++) {
        if(mFineDistances[index] < mFineDistances[bestIndex]) {
            bestIndex = index;
        }
    }
    int firstIndex = bestIndex;
    int lastIndex = bestIndex;
    int distanceLimit = mFineDistances[bestIndex] + mCenterBandMm;
    while(firstIndex > 0 && mFineDistances[firstIndex - 1] <= distanceLimit) {
        firstIndex--;
    }
    while(lastIndex + 1 < mFineMeasurementCount
          && mFineDistances[lastIndex + 1] <= distanceLimit) {
        lastIndex++;
    }

    int firstAngle = mFineWheelDegrees[firstIndex];
    int lastAngle = mFineWheelDegrees[lastIndex];
    mFineCenterWheelDegrees[mFinePass] = (firstAngle + lastAngle) / 2;
    mFineCenterFound[mFinePass] = true;
    LOGD("[ULTRA_ALIGN] pass=%d center=%d wheelDeg band=%dmm range=%d..%d\n", mFinePass,
         mFineCenterWheelDegrees[mFinePass], mCenterBandMm, firstAngle, lastAngle);
}

void UltrasonicAlignTracer::selectFinalAngle()
{
    if(!mFoundObject) {
        mTargetWheelDegrees = 0;
        return;
    }

    if(mFineCenterFound[0] && mFineCenterFound[1]) {
        mTargetWheelDegrees = (mFineCenterWheelDegrees[0] + mFineCenterWheelDegrees[1]) / 2;
    } else if(mFineCenterFound[0]) {
        mTargetWheelDegrees = mFineCenterWheelDegrees[0];
    } else if(mFineCenterFound[1]) {
        mTargetWheelDegrees = mFineCenterWheelDegrees[1];
    } else {
        mTargetWheelDegrees = mBestWheelDegrees;
    }
    LOGI("[ULTRA_ALIGN] selected center=%d wheelDeg coarseBest=%d wheelDeg\n",
         mTargetWheelDegrees, mBestWheelDegrees);
}

int UltrasonicAlignTracer::getTurnWheelDegrees() const
{
    int leftDelta = mWalker->getLeftCount() - mStartLeftCount;
    int rightDelta = mWalker->getRightCount() - mStartRightCount;
    return (rightDelta - leftDelta) / 2;
}

int UltrasonicAlignTracer::getMedianSampleIndex() const
{
    int sortedIndexes[MAX_SAMPLES];
    for(int index = 0; index < mValidSampleCount; index++) {
        sortedIndexes[index] = index;
    }
    std::sort(sortedIndexes, sortedIndexes + mValidSampleCount,
              [this](int left, int right) { return mSamples[left] < mSamples[right]; });
    return sortedIndexes[mValidSampleCount / 2];
}