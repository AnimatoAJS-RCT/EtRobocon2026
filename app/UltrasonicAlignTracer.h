#ifndef ETTR_APP_ULTRASONICALIGNTRACER_H_
#define ETTR_APP_ULTRASONICALIGNTRACER_H_

#include "Tracer.h"
#include "UltrasonicSensor.h"
#include "Walker.h"

class UltrasonicAlignTracer : public Tracer {
public:
    UltrasonicAlignTracer(Walker* walker,
                          const spikeapi::UltrasonicSensor* ultrasonicSensor,
                          int halfSweepAngleDeg,
                          int turnPwm,
                          int stepAngleDeg,
                          int sampleCount,
                          int maxDistanceMm,
                          double wheelDegreesPerBodyDegree,
                          int centerBandMm);
    void run() override;

private:
    enum Phase {
        TURNING_TO_COARSE_START,
        COARSE_SWEEPING,
        TURNING_TO_FINE_START,
        FINE_SWEEPING,
        TURNING_TO_BEST,
    };

    static const int TURN_TOLERANCE_DEG = 2;
    static const int FINE_SWEEP_HALF_ANGLE_DEG = 8;
    static const int FINE_SWEEP_PASSES = 2;
    static const int MAX_SAMPLES = 5;
    static const int MAX_FINE_MEASUREMENTS = 24;

    Walker* mWalker;
    const spikeapi::UltrasonicSensor* mUltrasonicSensor;
    int mHalfSweepWheelDegrees;
    int mStepWheelDegrees;
    int mFineStepWheelDegrees;
    double mWheelDegreesPerBodyDegree;
    int mTurnPwm;
    int mSampleCount;
    int mMaxDistanceMm;
    int mCenterBandMm;
    int mStartLeftCount;
    int mStartRightCount;
    int mTargetWheelDegrees;
    int mSweepEndWheelDegrees;
    int mFineSweepEndWheelDegrees;
    int mLastSampleWheelDegrees;
    int mBestWheelDegrees;
    int mBestDistanceMm;
    int mFineCenterWheelDegrees[FINE_SWEEP_PASSES];
    bool mFineCenterFound[FINE_SWEEP_PASSES];
    int mSamples[MAX_SAMPLES];
    int mSampleWheelDegrees[MAX_SAMPLES];
    int mFineDistances[MAX_FINE_MEASUREMENTS];
    int mFineWheelDegrees[MAX_FINE_MEASUREMENTS];
    int mSampleAttempts;
    int mValidSampleCount;
    int mFineMeasurementCount;
    int mFinePass;
    bool mFoundObject;
    Phase mPhase;

    void startSearching();
    void turnToTarget();
    void startSweep(Phase phase, int endWheelDegrees);
    void sweepAndMeasure();
    void collectMeasurement(int wheelDegrees);
    void evaluateSamples();
    void startFineSweep();
    void startFineSweepPass();
    void calculateFineCenter();
    void selectFinalAngle();
    int getTurnWheelDegrees() const;
    int getMedianSampleIndex() const;
};

#endif  // ETTR_APP_ULTRASONICALIGNTRACER_H_