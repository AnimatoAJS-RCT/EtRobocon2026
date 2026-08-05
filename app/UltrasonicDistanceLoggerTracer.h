#ifndef ETTR_APP_ULTRASONICDISTANCELOGGERTRACER_H_
#define ETTR_APP_ULTRASONICDISTANCELOGGERTRACER_H_

#include "Tracer.h"
#include "UltrasonicSensor.h"
#include "Walker.h"

class UltrasonicDistanceLoggerTracer : public Tracer {
public:
    UltrasonicDistanceLoggerTracer(Walker* walker,
                                   const spikeapi::UltrasonicSensor* ultrasonicSensor,
                                   int sampleCount);
    void run() override;

private:
    static const int LOG_INTERVAL_CYCLES = 10;

    Walker* mWalker;
    const spikeapi::UltrasonicSensor* mUltrasonicSensor;
    int mSampleCount;
    int mLoggedSampleCount;
    int mIntervalCycles;
};

#endif  // ETTR_APP_ULTRASONICDISTANCELOGGERTRACER_H_