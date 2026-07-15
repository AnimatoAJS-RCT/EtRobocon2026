/******************************************************************************
 *  ArmTracer.h (for SPIKE)
 *  Created on: 2026/07/15
 *  Definition of the Class ArmTracer
 *****************************************************************************/

#ifndef ETTR_APP_ARMTRACER_H_
#define ETTR_APP_ARMTRACER_H_

#include "Motor.h"
#include "Tracer.h"

class ArmTracer : public Tracer {
   public:
    ArmTracer(spikeapi::Motor* armMotor, int armPwm, int durationMs);
    void run() override;

   private:
    static const int LOOP_INTERVAL_MS = 10;

    spikeapi::Motor* mArmMotor;
    int mArmPwm;
    int mDurationMs;
    int mElapsedMs;
    bool mIsInitialized;
};

#endif  // ETTR_APP_ARMTRACER_H_