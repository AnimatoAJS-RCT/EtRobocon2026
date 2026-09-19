#ifndef ETTR_APP_LINEENDAPPROACHTRACER_H_
#define ETTR_APP_LINEENDAPPROACHTRACER_H_

#include "Tracer.h"
#include "Walker.h"
#include "ColorSensor.h"
#include "Pid.h"
#include "RotateTracer.h"

#include <string>
#include <vector>

class LineEndApproachTracer : public Tracer {
public:
    struct Config {
        int targetBrightness = 55;
        int pwm = 60;
        int maxPwm = 100;
        int turnPwm = 60;
        bool isLeftEdge = false;
        bool mirrorCourse = false;
        double kp = 0.6;
        double ki = 0.01;
        double kd = 0.017;
        double qrClearMm = 40;
        double approachMaxMm = 500;
        double sensorOffsetMm = 45;
        double traceMaxMm = 700;
        double stableMm = 30;
        double gapMm = 20;
        double markerMaxMm = 80;
        double scanDeg = 20;
        double endOffsetMm = 0;
        int timeoutTicks = 10000;

        bool isValid() const;
        bool parse(const std::vector<std::string>& tokens, bool mirror);
    };

    LineEndApproachTracer(Walker* walker, spikeapi::ColorSensor* sensor, const Config& config);
    void run() override;
    void setCalibration(int black, int white);
    bool hasFailed() const;

private:
    enum Phase {
        APPROACH, CENTER_ON_LINE, TURN_TO_LINE, CHECK_AFTER_TURN, ALIGN_SCAN,
        TRACE, MARKER, GAP, END_SCAN, RESTORE_HEADING,
        VERIFY_BACK, POSITION_END, FAILED
    };

    Walker* mWalker;
    spikeapi::ColorSensor* mSensor;
    Config mConfig;
    PidGain mGain;
    Pid mPid;
    RotateTracer mTurn;
    Phase mPhase = APPROACH;
    int mBlack = 0;
    int mWhite = 100;
    int mTicks = 0;
    int mMatchCount = 0;
    int mBlackCount = 0;
    int mMarkerCount = 0;
    bool mSawWhite = false;
    bool mStable = false;
    double mPhaseStartMm = 0;
    double mTraceStartMm = 0;
    double mStableStartMm = 0;
    double mGapStartMm = 0;
    double mGapHeading = 0;
    double mScanCenter = 0;
    int mScanLeg = 0;
    double mEndTargetMm = 0;
    int mPositionDirection = 1;
    bool mTurnTargetActive = false;
    double mTurnTarget = 0;
    int mTurnDirection = 1;

    double distanceMm() const;
    double headingWdeg() const;
    void enter(Phase phase);
    void fail(const char* reason);
    void driveStraight(int direction = 1);
    bool turnTo(double target);
    void startScan(Phase phase);
    bool scanFinished();
    void startTrace(bool resetStability);
    void followLine(int reflection);
};

#endif