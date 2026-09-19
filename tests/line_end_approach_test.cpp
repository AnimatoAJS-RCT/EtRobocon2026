#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

#define TRACER_H
#define ETTR_UNIT_WALKER_H_
#define SPIKE_CPP_API_COLOR_SENSOR_H_
#define ETTR_LOG_LEVEL 0

class Starter {
public:
    bool isPushed() { return true; }
};

class Tracer {
public:
    enum State { UNDEFINED, WAITING_FOR_START, WALKING, TERMINATED };
    State mState = UNDEFINED;
    std::vector<Starter*> mStarterList;
    virtual void run() = 0;
    virtual ~Tracer() = default;
    bool isTerminated() const { return mState == TERMINATED; }
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
    void move(double millimeters) {
        int degrees = static_cast<int>(std::round(millimeters * 360 / (55 * 3.1415926535)));
        left += degrees;
        right += degrees;
    }
    void heading(double degrees) {
        int center = (left + right) / 2;
        left = center - static_cast<int>(degrees);
        right = center + static_cast<int>(degrees);
    }
};

namespace spikeapi {
class ColorSensor {
public:
    struct HSV { int h; int s; int v; };
    int reflection = 100;
    HSV hsv = {0, 0, 100};
    int getReflection() const { return reflection; }
    void getHSV(HSV& value) const { value = hsv; }
};
}

void ettr_log_write(const char*, ...) {}

#include "../app/Pid.cpp"
#include "../unit/Util.cpp"
#define private public
#include "../app/RotateTracer.cpp"
#include "../app/LineEndApproachTracer.cpp"
#undef private

int main()
{
    using Subject = LineEndApproachTracer;
    Subject::Config config;
    assert(config.isValid());
    auto invalid = config;
    invalid.pwm = 101;
    assert(!invalid.isValid());
    invalid = config;
    invalid.gapMm = NAN;
    assert(!invalid.isValid());

    Subject::Config parsed;
    auto tokens = split("LineEndApproachTracer 55 60 100 60 RIGHT_EDGE 0.6 0.01 0.017 "
                        "gapMm=25 sensorOffsetMm=50 timeoutTicks=2000", " ");
    assert(parsed.parse(tokens, false));
    assert(parsed.pwm == 60 && parsed.gapMm == 25 && parsed.sensorOffsetMm == 50);
    assert(parsed.timeoutTicks == 2000 && !parsed.isLeftEdge);
    assert(parsed.parse(tokens, true));
    assert(parsed.isLeftEdge && parsed.mirrorCourse);
    for(const char* suffix : {"unknown=1", "gapMm=-1", "gapMm=nan", "scanDeg=90",
                              "timeoutTicks=0", "gapMm=20oops", "gapMm", "timeoutTicks=1.5"}) {
        auto bad = tokens;
        bad.push_back(suffix);
        assert(!parsed.parse(bad, false) && !parsed.isValid());
    }
    assert(!parsed.parse({}, false));
    auto badPwm = tokens;
    badPwm[2] = "60.5";
    assert(!parsed.parse(badPwm, false));

    for(bool startsOnBlue : {false, true}) {
        Walker walker;
        spikeapi::ColorSensor sensor;
        Subject tracer(&walker, &sensor, config);
        tracer.run();
        tracer.run();
        sensor.reflection = 0;
        tracer.run();
        assert(tracer.mPhase == Subject::APPROACH);
        walker.move(45);
        sensor.reflection = 100;
        tracer.run();
        assert(walker.leftPwm == 60 && walker.rightPwm == 60);
        walker.move(80);
        sensor.reflection = startsOnBlue ? 90 : 0;
        sensor.hsv = startsOnBlue ? spikeapi::ColorSensor::HSV{220, 100, 100}
                                 : spikeapi::ColorSensor::HSV{0, 0, 0};
        tracer.run();
        tracer.run();
        assert(tracer.mPhase == Subject::CENTER_ON_LINE);
        walker.move(46);
        tracer.run();
        assert(tracer.mPhase == Subject::TURN_TO_LINE);
        tracer.run();
        tracer.run();
        tracer.run();
        assert(walker.leftPwm == -60 && walker.rightPwm == 60);
        walker.heading(tracer.mTurn.mTargetTurnWdeg);
        tracer.run();
        assert(tracer.mPhase == Subject::ADVANCE_AFTER_TURN);
        tracer.run();
        assert(walker.leftPwm == 60 && walker.rightPwm == 60);
        walker.move(51);
        tracer.run();
        assert(tracer.mPhase == Subject::ALIGN_SCAN);
        sensor.reflection = 0;
        sensor.hsv = {0, 0, 0};
        tracer.run();
        tracer.run();
        assert(tracer.mPhase == Subject::TRACE);
        if(startsOnBlue) {
            sensor.reflection = 90;
            sensor.hsv = {220, 100, 100};
            tracer.run();
            assert(tracer.mPhase == Subject::MARKER);
            tracer.run();
            assert(walker.leftPwm == 60 && walker.rightPwm == 60);
            walker.move(25);
            sensor.reflection = 30;
            sensor.hsv = {0, 0, 0};
            tracer.run();
            tracer.run();
        }
        sensor.reflection = 50;
        sensor.hsv = {0, 0, 0};
        walker.move(105);
        tracer.run();
        assert(tracer.mStable);
        sensor.reflection = 100;
        sensor.hsv = {0, 0, 100};
        tracer.run();
        assert(tracer.mPhase == Subject::GAP);
        double endpoint = tracer.mGapStartMm;
        walker.move(21);
        tracer.run();
        assert(tracer.mPhase == Subject::POSITION_END);
        tracer.run();
        assert(walker.leftPwm == -60 && walker.rightPwm == -60);
        walker.move(endpoint - tracer.distanceMm() - 1);
        tracer.run();
        assert(tracer.isTerminated());
        assert(walker.leftPwm == 0 && walker.rightPwm == 0);
    }

    Walker shortLineWalker;
    spikeapi::ColorSensor shortLineSensor;
    auto shortLineConfig = config;
    shortLineConfig.stableMm = 100;
    Subject shortLine(&shortLineWalker, &shortLineSensor, shortLineConfig);
    shortLine.mState = Tracer::WALKING;
    shortLine.mTraceStartMm = shortLine.distanceMm();
    shortLine.startTrace(true);
    shortLineSensor.reflection = 30;
    shortLineWalker.move(35);
    shortLine.run();
    assert(!shortLine.mStable);
    shortLineSensor.reflection = 100;
    shortLineSensor.hsv = {0, 0, 100};
    shortLine.run();
    assert(shortLine.mPhase == Subject::TRACE);
    assert(shortLineWalker.leftPwm == 60 && shortLineWalker.rightPwm == 60);

    Walker walker;
    spikeapi::ColorSensor sensor;
    Subject missed(&walker, &sensor, config);
    missed.run();
    missed.run();
    walker.move(501);
    missed.run();
    assert(missed.hasFailed() && !missed.isTerminated());
    missed.run();
    assert(walker.leftPwm == 0 && walker.rightPwm == 0);

    Subject timeout(&walker, &sensor, config);
    timeout.run();
    timeout.run();
    timeout.mTicks = config.timeoutTicks;
    timeout.run();
    assert(timeout.hasFailed());

    Subject recovery(&walker, &sensor, config);
    recovery.mState = Tracer::WALKING;
    recovery.mTraceStartMm = recovery.distanceMm();
    recovery.startTrace(true);
    recovery.mStable = true;
    recovery.run();
    assert(recovery.mPhase == Subject::GAP);
    sensor.reflection = 30;
    recovery.run();
    recovery.run();
    assert(recovery.mPhase == Subject::TRACE && !recovery.isTerminated());

    config.mirrorCourse = true;
    Subject mirrored(&walker, &sensor, config);
    assert(mirrored.mTurn.mDirection == -1);
    config.mirrorCourse = false;
    Subject rightCourse(&walker, &sensor, config);
    assert(rightCourse.mTurn.mDirection == 1);

    walker.heading(0);
    assert(!mirrored.turnTo(20));
    walker.heading(27);
    assert(mirrored.turnTo(20));
    assert(walker.leftPwm == 0 && walker.rightPwm == 0);
    assert(!mirrored.turnTo(-20));
    walker.heading(-27);
    assert(mirrored.turnTo(-20));

    sensor.reflection = 100;
    Subject unstable(&walker, &sensor, config);
    unstable.mState = Tracer::WALKING;
    unstable.mTraceStartMm = unstable.distanceMm();
    unstable.startScan(Subject::END_SCAN);
    walker.heading(unstable.mScanCenter + config.scanDeg * 14 / 9);
    unstable.run();
    walker.heading(unstable.mScanCenter - config.scanDeg * 14 / 9);
    unstable.run();
    assert(unstable.hasFailed() && !unstable.isTerminated());

    Subject reacquired(&walker, &sensor, config);
    reacquired.mState = Tracer::WALKING;
    reacquired.mTraceStartMm = reacquired.distanceMm();
    reacquired.startScan(Subject::END_SCAN);
    sensor.reflection = 30;
    reacquired.run();
    reacquired.run();
    assert(reacquired.mPhase == Subject::TRACE && !reacquired.isTerminated());
    std::puts("line_end_approach_test: PASS");
}