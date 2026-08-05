#include "UltrasonicDistanceLoggerTracer.h"

#include "Log.h"

#include <algorithm>

UltrasonicDistanceLoggerTracer::UltrasonicDistanceLoggerTracer(
    Walker* walker,
    const spikeapi::UltrasonicSensor* ultrasonicSensor,
    int sampleCount)
    : mWalker(walker),
      mUltrasonicSensor(ultrasonicSensor),
      mSampleCount(std::max(1, sampleCount)),
      mLoggedSampleCount(0),
      mIntervalCycles(0)
{
    mState = UNDEFINED;
}

void UltrasonicDistanceLoggerTracer::run()
{
    mWalker->brake();

    switch(mState) {
        case UNDEFINED:
            mState = WAITING_FOR_START;
            break;
        case WAITING_FOR_START:
            if(mStarterList.empty()) {
                mState = WALKING;
            } else {
                for(auto starter : mStarterList) {
                    if(starter->isPushed()) {
                        mState = WALKING;
                        break;
                    }
                }
            }
            if(mState == WALKING) {
                LOGI("[ULTRA_DIAG] start: samples=%d interval=%dms; keep the robot still\n",
                     mSampleCount, LOG_INTERVAL_CYCLES * 10);
            }
            break;
        case WALKING:
            mIntervalCycles++;
            if(mIntervalCycles < LOG_INTERVAL_CYCLES) {
                break;
            }
            mIntervalCycles = 0;
            mLoggedSampleCount++;
            LOGI("[ULTRA_DIAG] sample=%d/%d raw=%dmm\n", mLoggedSampleCount, mSampleCount,
                 mUltrasonicSensor->getDistance());
            if(mLoggedSampleCount >= mSampleCount) {
                LOGI("[ULTRA_DIAG] complete\n");
                mState = TERMINATED;
            }
            break;
        case TERMINATED:
            break;
        default:
            break;
    }
}