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

    Walker grayWalker;
    spikeapi::ColorSensor graySensor;
    Subject grayStart(&grayWalker, &graySensor, config);
    grayStart.run();
    grayStart.run();
    grayWalker.move(50);
    graySensor.reflection = 100;
    graySensor.hsv = {200, 20, 100};
    grayStart.run();
    graySensor.reflection = 20;
    graySensor.hsv = {200, 20, 80};
    grayStart.run();
    grayStart.run();
    assert(grayStart.mPhase == Subject::APPROACH);
    assert(grayStart.mMatchCount == 0);

    graySensor.reflection = 20;
    graySensor.hsv = {200, 37, 9};
    grayStart.run();
    grayStart.run();
    assert(grayStart.mPhase == Subject::CENTER_ON_LINE);

    for(int scenario = 0; scenario < 6; scenario++) {
        int startPosition = scenario % 3;
        config.mirrorCourse = scenario >= 3;
        bool startsOnBlue = startPosition == 1;
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
         int turnDirection = config.mirrorCourse ? -1 : 1;
         assert(walker.leftPwm == -60 * turnDirection
             && walker.rightPwm == 60 * turnDirection);
         walker.heading(turnDirection * tracer.mTurn.mTargetTurnWdeg);
        tracer.run();
        assert(tracer.mPhase == Subject::CHECK_AFTER_TURN);
        tracer.run();
        assert(walker.leftPwm == 0 && walker.rightPwm == 0);
        tracer.run();
        assert(tracer.mPhase == (startsOnBlue ? Subject::MARKER : Subject::TRACE));
        assert(!tracer.mStable);
        if(startPosition != 0) {
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
            assert(tracer.mPhase == Subject::MARKER);
            tracer.run();
            assert(tracer.mPhase == Subject::TRACE && !tracer.mStable);
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
    config.mirrorCourse = false;

    for(bool gray : {false, true}) {
        Walker searchWalker;
        spikeapi::ColorSensor searchSensor;
        Subject search(&searchWalker, &searchSensor, config);
        search.mState = Tracer::WALKING;
        search.enter(Subject::CHECK_AFTER_TURN);
        searchSensor.reflection = gray ? 20 : 100;
        searchSensor.hsv = {200, 20, gray ? 80 : 100};
        search.run();
        assert(search.mPhase == Subject::ALIGN_SCAN);
        assert(searchWalker.leftPwm == 0 && searchWalker.rightPwm == 0);
        search.run();
        assert(search.mPhase == Subject::ALIGN_SCAN);
        assert(searchWalker.leftPwm == -searchWalker.rightPwm);

        searchSensor.reflection = 20;
        searchSensor.hsv = {200, 37, 9};
        search.run();
        assert(search.mPhase == Subject::ALIGN_SCAN);
        searchSensor.hsv = {220, 100, 100};
        search.run();
        assert(search.mPhase == Subject::ALIGN_SCAN);
        search.run();
        assert(search.mPhase == Subject::MARKER);
        search.run();
        assert(searchWalker.leftPwm == 60 && searchWalker.rightPwm == 60);
        searchWalker.move(10);
        searchSensor.reflection = gray ? 20 : 100;
        searchSensor.hsv = {200, 20, gray ? 80 : 100};
        search.run();
        assert(search.mPhase == Subject::ALIGN_SCAN);
        assert(searchWalker.leftPwm == 0 && searchWalker.rightPwm == 0);
        searchSensor.reflection = 20;
        searchSensor.hsv = {200, 37, 9};
        search.run();
        search.run();
        assert(search.mPhase == Subject::TRACE && !search.mStable);
    }

    Walker markerWalker;
    spikeapi::ColorSensor markerSensor;
    Subject markerLimit(&markerWalker, &markerSensor, config);
    markerLimit.mState = Tracer::WALKING;
    markerLimit.enter(Subject::CHECK_AFTER_TURN);
    markerSensor.hsv = {220, 100, 100};
    markerLimit.run();
    markerLimit.run();
    assert(markerLimit.mPhase == Subject::MARKER);
    markerWalker.move(config.markerMaxMm + 1);
    markerLimit.run();
    assert(markerLimit.hasFailed() && markerLimit.isTerminated());
    assert(markerWalker.leftPwm == 0 && markerWalker.rightPwm == 0);

    Walker shortLineWalker;
    spikeapi::ColorSensor shortLineSensor;
    auto shortLineConfig = config;
    shortLineConfig.stableMm = 100;
    Subject shortLine(&shortLineWalker, &shortLineSensor, shortLineConfig);
    shortLine.mState = Tracer::WALKING;
    shortLine.mTraceStartMm = shortLine.distanceMm();
    shortLine.startTrace(true);
    shortLineSensor.reflection = 30;
    shortLineSensor.hsv = {200, 37, 9};
    shortLineWalker.move(35);
    shortLine.run();
    assert(!shortLine.mStable);
    shortLineSensor.reflection = 100;
    shortLineSensor.hsv = {0, 0, 100};
    shortLine.run();
    assert(shortLine.mPhase == Subject::TRACE);
    assert(shortLineWalker.leftPwm == 60 && shortLineWalker.rightPwm == 60);
    shortLineWalker.move(80);
    shortLine.run();
    shortLineSensor.reflection = 30;
    shortLineSensor.hsv = {200, 37, 9};
    shortLineWalker.move(35);
    shortLine.run();
    assert(!shortLine.mStable);

    Walker walker;
    spikeapi::ColorSensor sensor;
    Subject missed(&walker, &sensor, config);
    missed.run();
    missed.run();
    walker.move(501);
    missed.run();
    assert(missed.hasFailed() && missed.isTerminated());
    missed.run();
    assert(walker.leftPwm == 0 && walker.rightPwm == 0);

    Subject timeout(&walker, &sensor, config);
    timeout.run();
    timeout.run();
    timeout.mTicks = config.timeoutTicks;
    timeout.run();
    assert(timeout.hasFailed() && timeout.isTerminated());

    Subject recovery(&walker, &sensor, config);
    recovery.mState = Tracer::WALKING;
    recovery.mTraceStartMm = recovery.distanceMm();
    recovery.startTrace(true);
    recovery.mStable = true;
    recovery.run();
    assert(recovery.mPhase == Subject::GAP);
    sensor.reflection = 30;
    sensor.hsv = {200, 37, 9};
    recovery.run();
    recovery.run();
    assert(recovery.mPhase == Subject::TRACE && !recovery.isTerminated());

    config.mirrorCourse = true;
    Subject mirrored(&walker, &sensor, config);
    assert(mirrored.mTurn.mDirection == -1);
    config.mirrorCourse = false;
    Subject rightCourse(&walker, &sensor, config);
    assert(rightCourse.mTurn.mDirection == 1);
    assert(rightCourse.mTurn.mTargetTurnWdeg == 161);

    walker.heading(0);
    assert(!mirrored.turnTo(20));
    walker.heading(27);
    assert(mirrored.turnTo(20));
    assert(walker.leftPwm == 0 && walker.rightPwm == 0);
    assert(!mirrored.turnTo(-20));
    walker.heading(-27);
    assert(mirrored.turnTo(-20));

    sensor.reflection = 100;
    sensor.hsv = {200, 20, 100};
    Subject unstable(&walker, &sensor, config);
    unstable.mState = Tracer::WALKING;
    unstable.mTraceStartMm = unstable.distanceMm();
    unstable.startScan(Subject::END_SCAN);
    walker.heading(unstable.mScanCenter + config.scanDeg * 14 / 9);
    unstable.run();
    walker.heading(unstable.mScanCenter - config.scanDeg * 14 / 9);
    unstable.run();
    assert(unstable.hasFailed() && unstable.isTerminated());

    Subject reacquired(&walker, &sensor, config);
    reacquired.mState = Tracer::WALKING;
    reacquired.mTraceStartMm = reacquired.distanceMm();
    reacquired.startScan(Subject::END_SCAN);
    sensor.reflection = 30;
    sensor.hsv = {200, 37, 9};
    reacquired.run();
    reacquired.run();
    assert(reacquired.mPhase == Subject::TRACE && !reacquired.isTerminated());
    std::puts("line_end_approach_test: PASS");
}