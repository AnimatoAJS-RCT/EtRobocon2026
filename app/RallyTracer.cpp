/**
 * @file RallyTracer.cpp
 * @brief ETラリー走行トレーサー実装
 */

#include "RallyTracer.h"

#include "Log.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

// ---------------------------------------------------------------------------
// コンストラクタ
// ---------------------------------------------------------------------------

RallyTracer::RallyTracer(Walker* walker, const RallyRoute& route,
                         int movePwm, int turnPwm,
                                                 QRPos initialPos, int initialHeadingDeg,
                                                 const spikeapi::ColorSensor* colorSensor,
                                                 bool enableMarkerCorrection,
                                                 int markerReflectionThreshold,
                                                 int markerSnapWindowDegrees,
                                                 int markerCooldownTicks)
    : mWalker(walker),
      mRoute(route),
      mMovePwm(std::max(1, std::abs(movePwm))),
      mTurnPwm(std::max(1, std::abs(turnPwm))),
      mCurrentStepIndex(0),
      mPhase(TURNING),
      mCurrentPos(initialPos),
      mHeadingDeg(((initialHeadingDeg % 360) + 360) % 360),
      mTargetHeadingDeg(0),
      mPhaseStartLeftCount(0),
      mPhaseStartRightCount(0),
      mTargetWheelDegrees(0),
      mColorSensor(colorSensor),
      mEnableMarkerCorrection(enableMarkerCorrection),
      mMarkerReflectionThreshold(markerReflectionThreshold),
      mMarkerSnapWindowDegrees(std::max(1, markerSnapWindowDegrees)),
      mMarkerCooldownTicks(std::max(0, markerCooldownTicks)),
      mMarkerCooldownRemaining(0),
      mMarkerDetectedInPhase(false)
{
    mState = UNDEFINED;
}

// ---------------------------------------------------------------------------
// run()
// ---------------------------------------------------------------------------

void RallyTracer::run()
{
    switch(mState) {
        case UNDEFINED:
            mState = WAITING_FOR_START;
            break;

        case WAITING_FOR_START:
            if(mStarterList.empty()) {
                startNextStep();
            } else {
                for(auto starter : mStarterList) {
                    if(starter->isPushed()) {
                        startNextStep();
                        return;
                    }
                }
            }
            break;

        case WALKING:
            switch(mPhase) {
                case TURNING:   execTurning();   break;
                case MOVING:    execMoving();    break;
                case RETURNING: execReturning(); break;
            }
            break;

        case TERMINATED:
            break;

        default:
            break;
    }
}

// ---------------------------------------------------------------------------
// フェーズ遷移
// ---------------------------------------------------------------------------

void RallyTracer::startNextStep()
{
    if(mCurrentStepIndex >= mRoute.size()) {
        mWalker->stop();
        mState = TERMINATED;
        LOGI("[RALLY] all %u steps done\n",
             static_cast<unsigned>(mRoute.size()));
        return;
    }

    const RouteStep& step = mRoute[mCurrentStepIndex];
    int targetHeading = calcHeadingDeg(mCurrentPos, step.destination);
    int turnBodyDeg   = shortestTurn(mHeadingDeg, targetHeading);

    LOGI("[RALLY] step %u type=%d cur=(%d,%d) dest=(%d,%d) heading %d->%d\n",
         static_cast<unsigned>(mCurrentStepIndex),
         static_cast<int>(step.type),
         mCurrentPos.x, mCurrentPos.y,
         step.destination.x, step.destination.y,
         mHeadingDeg, targetHeading);

    if(std::abs(turnBodyDeg) < 5) {
        // すでにほぼ正しい方向を向いている → 旋回をスキップ
        mHeadingDeg = targetHeading;
        beginMoving(calcMoveWheelDegrees(mCurrentPos, step.destination));
    } else {
        beginTurning(targetHeading);
    }

    mState = WALKING;
}

void RallyTracer::beginTurning(int targetHeadingDeg)
{
    mTargetHeadingDeg  = ((targetHeadingDeg % 360) + 360) % 360;
    int turnBodyDeg    = shortestTurn(mHeadingDeg, mTargetHeadingDeg);
    mTargetWheelDegrees = static_cast<int>(turnBodyDeg * WHEEL_DEGREES_PER_BODY_DEGREE);
    resetPhaseCounters();
    mPhase = TURNING;
    LOGD("[RALLY] begin turning: bodyDeg=%d wheelDeg=%d\n",
         turnBodyDeg, mTargetWheelDegrees);
}

void RallyTracer::beginMoving(int wheelDegrees)
{
    mTargetWheelDegrees = wheelDegrees;
    resetPhaseCounters();
    mWalker->beginEncoderCorrection();
    mPhase = MOVING;
    mMarkerDetectedInPhase = false;
    LOGD("[RALLY] begin moving: wheelDeg=%d\n", wheelDegrees);
}

void RallyTracer::beginReturning(int wheelDegrees)
{
    mTargetWheelDegrees = wheelDegrees;
    resetPhaseCounters();
    mWalker->beginEncoderCorrection();
    mPhase = RETURNING;
    mMarkerDetectedInPhase = false;
    LOGD("[RALLY] begin returning: wheelDeg=%d\n", wheelDegrees);
}

void RallyTracer::finishStep()
{
    const RouteStep& step = mRoute[mCurrentStepIndex];

    // 現在位置を更新
    if(step.type == RouteStepType::VIRTUAL_DETOUR) {
        mCurrentPos = step.returnPos;  // 後退して戻った実 QR 座標
        // 向きは前進方向のまま（後退しても車体は向きを変えていない）
    } else {
        mCurrentPos = step.destination;
    }

    LOGI("[RALLY] step %u done, now at (%d,%d) heading=%d\n",
         static_cast<unsigned>(mCurrentStepIndex),
         mCurrentPos.x, mCurrentPos.y, mHeadingDeg);

    mCurrentStepIndex++;
    startNextStep();
}

// ---------------------------------------------------------------------------
// フェーズ実行
// ---------------------------------------------------------------------------

void RallyTracer::execTurning()
{
    int current   = getTurnWheelDegrees();
    int remaining = mTargetWheelDegrees - current;

    if(std::abs(remaining) <= TURN_TOLERANCE) {
        mWalker->brake();
        mHeadingDeg = mTargetHeadingDeg;
        const RouteStep& step = mRoute[mCurrentStepIndex];
        beginMoving(calcMoveWheelDegrees(mCurrentPos, step.destination));
        return;
    }

    // 残量が正 → 反時計回り (CCW): 左後退, 右前進
    // 残量が負 →     時計回り  (CW): 左前進, 右後退
    if(remaining > 0) {
        mWalker->setPwm(-mTurnPwm, mTurnPwm);
    } else {
        mWalker->setPwm(mTurnPwm, -mTurnPwm);
    }
    mWalker->run();
}

void RallyTracer::execMoving()
{
    int current   = getMoveWheelDegrees();
    int remaining = mTargetWheelDegrees - current;

    if(remaining <= MOVE_TOLERANCE || isMarkerSnapTriggered(remaining)) {
        mWalker->brake();
        const RouteStep& step = mRoute[mCurrentStepIndex];
        if(step.type == RouteStepType::VIRTUAL_DETOUR) {
            // 仮想 QR に到達 → 同じ距離だけ後退して戻る
            beginReturning(mTargetWheelDegrees);
        } else {
            finishStep();
        }
        return;
    }

    mWalker->runWithEncoderCorrection(mMovePwm, mMovePwm);
}

void RallyTracer::execReturning()
{
    // 後退量は getMoveWheelDegrees() の符号反転で取得
    int traveled  = -getMoveWheelDegrees();
    int remaining = mTargetWheelDegrees - traveled;

    if(remaining <= MOVE_TOLERANCE || isMarkerSnapTriggered(remaining)) {
        mWalker->brake();
        finishStep();
        return;
    }

    mWalker->runWithEncoderCorrection(-mMovePwm, -mMovePwm);
}

// ---------------------------------------------------------------------------
// ユーティリティ
// ---------------------------------------------------------------------------

int RallyTracer::getTurnWheelDegrees() const
{
    int leftDelta  = mWalker->getLeftCount()  - mPhaseStartLeftCount;
    int rightDelta = mWalker->getRightCount() - mPhaseStartRightCount;
    return (rightDelta - leftDelta) / 2;
}

int RallyTracer::getMoveWheelDegrees() const
{
    int leftDelta  = mWalker->getLeftCount()  - mPhaseStartLeftCount;
    int rightDelta = mWalker->getRightCount() - mPhaseStartRightCount;
    return (leftDelta + rightDelta) / 2;
}

void RallyTracer::resetPhaseCounters()
{
    mPhaseStartLeftCount  = mWalker->getLeftCount();
    mPhaseStartRightCount = mWalker->getRightCount();
}

bool RallyTracer::isMarkerSnapTriggered(int remainingDegrees)
{
    if(!mEnableMarkerCorrection || mColorSensor == nullptr) {
        return false;
    }
    if(mMarkerDetectedInPhase) {
        return false;
    }
    if(remainingDegrees > mMarkerSnapWindowDegrees) {
        return false;
    }
    if(mMarkerCooldownRemaining > 0) {
        mMarkerCooldownRemaining--;
        return false;
    }

    int reflection = mColorSensor->getReflection();
    if(reflection > mMarkerReflectionThreshold) {
        return false;
    }

    mMarkerDetectedInPhase = true;
    mMarkerCooldownRemaining = mMarkerCooldownTicks;
    LOGI("[RALLY] marker snap: reflection=%d remain=%d window=%d\n",
         reflection, remainingDegrees, mMarkerSnapWindowDegrees);
    return true;
}

// static
int RallyTracer::shortestTurn(int fromDeg, int toDeg)
{
    int diff = ((toDeg - fromDeg) % 360 + 360) % 360;
    if(diff > 180) {
        diff -= 360;
    }
    return diff;  // 正: 反時計回り, 負: 時計回り
}

// static
int RallyTracer::calcHeadingDeg(const QRPos& from, const QRPos& to)
{
    int dx = to.x - from.x;
    int dy = to.y - from.y;
    double rad = std::atan2(static_cast<double>(dy), static_cast<double>(dx));
    int deg = static_cast<int>(std::round(rad * 180.0 / M_PI));
    return ((deg % 360) + 360) % 360;
}

// static
int RallyTracer::calcMoveWheelDegrees(const QRPos& from, const QRPos& to)
{
    int dx = to.x - from.x;
    int dy = to.y - from.y;
    double dist = std::sqrt(static_cast<double>(dx * dx + dy * dy));
    return static_cast<int>(dist * QR_GRID_WHEEL_DEGREES);
}
