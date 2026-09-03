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
    ArmTracer(spikeapi::Motor* armMotor, int armPwm, int direction, int targetAngle);
    void run() override;

   private:
    spikeapi::Motor* mArmMotor;
    int mArmPwm;
    int mDirection;
    int mTargetAngle;
    int mStartCount;
    bool mIsInitialized;
};

#endif  // ETTR_APP_ARMTRACER_H_