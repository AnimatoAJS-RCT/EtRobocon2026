#ifndef ETTR_APP_ROTATETRACER_H_
#define ETTR_APP_ROTATETRACER_H_

#include "Tracer.h"
#include "Walker.h"

class RotateTracer : public Tracer {
public:
    RotateTracer(Walker* walker, int direction, int angleDeg, int pwm);
    void run() override;

private:
    static constexpr double WHEEL_DEG_PER_BODY_DEG = 14.0 / 9.0;

    Walker* mWalker;
    int mDirection;
    int mTargetTurnWdeg;
    int mPwm;
    int mStartLeftCount;
    int mStartRightCount;

    int getTurnWdeg() const;
};

#endif  // ETTR_APP_ROTATETRACER_H_