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
    // 実測した車体角に合わせて、方向ごとの停止目標を校正する。
    // targetWdeg = requestedBodyDeg * WHEEL_DEG_PER_BODY_DEG * scale + offset
    static constexpr double RIGHT_TURN_SCALE = 1.0;
    static constexpr double LEFT_TURN_SCALE = 1.0;
    static constexpr int RIGHT_TURN_OFFSET_WDEG = 0;
    static constexpr int LEFT_TURN_OFFSET_WDEG = 0;

    Walker* mWalker;
    int mDirection;
    int mRequestedAngleDeg;
    int mTargetTurnWdeg;
    int mPwm;
    int mStartLeftCount;
    int mStartRightCount;

    int getTurnWdeg() const;
};

#endif  // ETTR_APP_ROTATETRACER_H_