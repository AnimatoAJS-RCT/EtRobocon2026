#include "UltrasonicProbeTracer.h"

#include "Log.h"

#include <algorithm>

UltrasonicProbeTracer::UltrasonicProbeTracer(Walker* walker, const std::string& modeName,
                                             int sampleCount, int sampleIntervalMs)
    : mWalker(walker),
      mDevice(pup_ultrasonic_sensor_get_device(PBIO_PORT_ID_F)),
      mModeCount(0),
      mSampleCount(std::max(1, sampleCount)),
      mSampleIntervalCycles(
          std::max(1, (sampleIntervalMs + CONTROL_PERIOD_MS - 1) / CONTROL_PERIOD_MS)),
      mIntervalCycles(0),
      mLoggedSampleCount(0),
      mModeIndex(0),
      mStaleWaitCycles(0),
      mElapsedMs(0),
      mSampleStartElapsedMs(0),
      mActiveMode(DISTL),
      mHasActiveMode(false),
      mReadState(REQUEST_MODE)
{
    if(modeName == "ALL") {
        mModes[0] = DISTL;
        mModes[1] = TRAW;
        mModes[2] = ADRAW;
        mModeCount = 3;
    } else if(modeName == "DISTS") {
        mModes[0] = DISTS;
        mModeCount = 1;
    } else if(modeName == "TRAW") {
        mModes[0] = TRAW;
        mModeCount = 1;
    } else if(modeName == "ADRAW") {
        mModes[0] = ADRAW;
        mModeCount = 1;
    } else {
        mModes[0] = DISTL;
        mModeCount = 1;
    }
    mState = UNDEFINED;
}

bool UltrasonicProbeTracer::isSupportedMode(const std::string& modeName)
{
#ifdef SPIKERT
    return modeName == "DISTL" || modeName == "DISTS" || modeName == "TRAW"
           || modeName == "ADRAW" || modeName == "ALL";
#else
    (void)modeName;
    return false;
#endif
}

void UltrasonicProbeTracer::run()
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
                LOGI("[ULTRA_PROBE] start: modes=%d samples=%d interval=%dms\n", mModeCount,
                     mSampleCount, mSampleIntervalCycles * CONTROL_PERIOD_MS);
                startSample();
            }
            break;
        case WALKING:
            mElapsedMs += CONTROL_PERIOD_MS;
            if(mLoggedSampleCount >= mSampleCount) {
                mState = TERMINATED;
                LOGI("[ULTRA_PROBE] complete\n");
                break;
            }
            if(mIntervalCycles > 0) {
                mIntervalCycles--;
                if(mIntervalCycles == 0) {
                    startSample();
                }
                break;
            }
            switch(mReadState) {
                case REQUEST_MODE:
                    beginMode();
                    break;
                case WAIT_STALE:
                    if(++mStaleWaitCycles >= MODE_STALE_WAIT_MS / CONTROL_PERIOD_MS) {
                        mReadState = DISCARD_STALE;
                    }
                    break;
                case DISCARD_STALE:
                    readAndDiscard();
                    mReadState = LOG_VALUE;
                    break;
                case LOG_VALUE:
                    logValue();
                    advanceMode();
                    break;
            }
            break;
        case TERMINATED:
            break;
        default:
            break;
    }
}

void UltrasonicProbeTracer::startSample()
{
    mModeIndex = 0;
    mSampleStartElapsedMs = mElapsedMs;
    mReadState = REQUEST_MODE;
}

void UltrasonicProbeTracer::beginMode()
{
#ifdef SPIKERT
    ProbeMode desiredMode = mModes[mModeIndex];
    if(mHasActiveMode && desiredMode == mActiveMode) {
        mReadState = LOG_VALUE;
        return;
    }

    int32_t ignoredValue = 0;
    pup_device_get_values(mDevice, modeToPbio(desiredMode), &ignoredValue);
    mActiveMode = desiredMode;
    mHasActiveMode = true;
    mStaleWaitCycles = 0;
    mReadState = WAIT_STALE;
#else
    mReadState = LOG_VALUE;
#endif
}

void UltrasonicProbeTracer::readAndDiscard()
{
#ifdef SPIKERT
    int32_t ignoredValue = 0;
    pup_device_get_values(mDevice, modeToPbio(mModes[mModeIndex]), &ignoredValue);
#endif
}

void UltrasonicProbeTracer::logValue()
{
#ifdef SPIKERT
    int32_t value = 0;
    pbio_error_t error = pup_device_get_values(mDevice, modeToPbio(mModes[mModeIndex]), &value);
    LOGI("[ULTRA_PROBE]\t%d\t%s\t%d\t%ld\t%d\n", mLoggedSampleCount + 1,
         modeToName(mModes[mModeIndex]), static_cast<int>(error), static_cast<long>(value),
         mElapsedMs);
#else
    LOGI("[ULTRA_PROBE] unavailable in simulator\n");
#endif
}

void UltrasonicProbeTracer::advanceMode()
{
    mModeIndex++;
    if(mModeIndex < mModeCount) {
        mReadState = REQUEST_MODE;
        return;
    }

    mLoggedSampleCount++;
    LOGI("[ULTRA_PROBE_CYCLE]\t%d\t%d\n", mLoggedSampleCount,
         mElapsedMs - mSampleStartElapsedMs);
    if(mLoggedSampleCount < mSampleCount) {
        mIntervalCycles = mSampleIntervalCycles;
    }
}

int UltrasonicProbeTracer::modeToPbio(ProbeMode mode) const
{
#ifdef SPIKERT
    switch(mode) {
        case DISTL:
            return PBIO_IODEV_MODE_PUP_ULTRASONIC_SENSOR__DISTL;
        case DISTS:
            return PBIO_IODEV_MODE_PUP_ULTRASONIC_SENSOR__DISTS;
        case TRAW:
            return PBIO_IODEV_MODE_PUP_ULTRASONIC_SENSOR__TRAW;
        case ADRAW:
            return PBIO_IODEV_MODE_PUP_ULTRASONIC_SENSOR__ADRAW;
    }
    return PBIO_IODEV_MODE_PUP_ULTRASONIC_SENSOR__DISTL;
#else
    (void)mode;
    return 0;
#endif
}

const char* UltrasonicProbeTracer::modeToName(ProbeMode mode) const
{
    switch(mode) {
        case DISTL:
            return "DISTL";
        case DISTS:
            return "DISTS";
        case TRAW:
            return "TRAW";
        case ADRAW:
            return "ADRAW";
    }
    return "DISTL";
}