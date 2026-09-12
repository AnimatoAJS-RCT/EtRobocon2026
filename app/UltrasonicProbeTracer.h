#ifndef ETTR_APP_ULTRASONICPROBETRACER_H_
#define ETTR_APP_ULTRASONICPROBETRACER_H_

#include "Tracer.h"
#include "Walker.h"

extern "C" {
#include <spike/pup/ultrasonicsensor.h>
}

#include <string>

class UltrasonicProbeTracer : public Tracer {
public:
    UltrasonicProbeTracer(Walker* walker, const std::string& modeName, int sampleCount,
                          int sampleIntervalMs);
    void run() override;

    static bool isSupportedMode(const std::string& modeName);

private:
    enum ProbeMode { DISTL, DISTS, TRAW, ADRAW };
    enum ReadState { REQUEST_MODE, WAIT_STALE, DISCARD_STALE, LOG_VALUE };

    static const int CONTROL_PERIOD_MS = 10;
    static const int MODE_STALE_WAIT_MS = 50;

    void startSample();
    void beginMode();
    void readAndDiscard();
    void logValue();
    void advanceMode();
    int modeToPbio(ProbeMode mode) const;
    const char* modeToName(ProbeMode mode) const;

    Walker* mWalker;
    pup_device_t* mDevice;
    ProbeMode mModes[3];
    int mModeCount;
    int mSampleCount;
    int mSampleIntervalCycles;
    int mIntervalCycles;
    int mLoggedSampleCount;
    int mModeIndex;
    int mStaleWaitCycles;
    int mElapsedMs;
    int mSampleStartElapsedMs;
    ProbeMode mActiveMode;
    bool mHasActiveMode;
    ReadState mReadState;
};

#endif  // ETTR_APP_ULTRASONICPROBETRACER_H_