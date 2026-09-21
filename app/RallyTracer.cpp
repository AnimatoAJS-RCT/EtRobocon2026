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
                         int finalHeadingDeg,
                         const spikeapi::ColorSensor* colorSensor,
                         bool enableMarkerCorrection,
                         int markerReflectionThreshold,
                         int markerSnapWindowDegrees,
                         int markerCooldownTicks,
                         int markerSearchAngleDeg)
    : mWalker(walker),
      mRoute(route),
      mMovePwm(std::max(1, std::abs(movePwm))),
      mTurnPwm(std::max(1, std::abs(turnPwm))),
      mCurrentStepIndex(0),
      mPhase(TURNING),
      mCurrentPos(initialPos),
      mHeadingDeg(((initialHeadingDeg % 360) + 360) % 360),
      mTargetHeadingDeg(0),
            mFinalHeadingDeg(finalHeadingDeg < 0 ? -1 : ((finalHeadingDeg % 360) + 360) % 360),
            mIsFinalTurning(false),
      mPhaseStartLeftCount(0),
      mPhaseStartRightCount(0),
      mTargetWheelDegrees(0),
            mPreviousTurnError(0),
    mMoveDirection(1),      mBrakeCountdown(0),
    mMarkerCorrectionAllowedThisStep(true),
    mColorSensor(colorSensor),
      mEnableMarkerCorrection(enableMarkerCorrection),
      mMarkerReflectionThreshold(markerReflectionThreshold),
      mMarkerSnapWindowDegrees(std::max(1, markerSnapWindowDegrees)),
      mMarkerCooldownTicks(std::max(0, markerCooldownTicks)),
      mMarkerCooldownRemaining(0),
      mMarkerSearchAngleDeg(std::max(1, markerSearchAngleDeg)),
    mMarkerDetectedInPhase(false),
    mMarkerSearchRing(0),
    mMarkerSearchSubStage(MARKER_SEARCH_SWEEP_PLUS),
    mMarkerSearchHeadingDeg(0),
    mMarkerSearchTurnWheelDegrees(0),
    mMarkerSearchCrawlOffset(0),
    mMarkerSearchCrawlStartOffset(0),
    mMarkerSearchCrawlTargetOffset(0),
    mMarkerSearchNextRing(0),
    mIsMarkerSearchRecoveryTurning(false)
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
                mWalker->init();
                LOGI("[RALLY] wheel encoders reset at start: wheel=(%d,%d)\n",
                     mWalker->getLeftCount(), mWalker->getRightCount());
                startNextStep();
            } else {
                for(auto starter : mStarterList) {
                    if(starter->isPushed()) {
                        mWalker->init();
                        LOGI("[RALLY] wheel encoders reset at start: wheel=(%d,%d)\n",
                             mWalker->getLeftCount(), mWalker->getRightCount());
                        startNextStep();
                        return;
                    }
                }
            }
            break;

        case WALKING:
            switch(mPhase) {
                case TURNING:   execTurning();   break;
                case BRAKING:   execBraking();   break;
                case MOVING:    execMoving();    break;
                case RETURNING: execReturning(); break;
                case MARKER_OFFSET: execMarkerOffset(); break;
                case MARKER_SEARCH: execMarkerSearch(); break;
                default:
                    LOGE("[RALLY] invalid phase=%d step=%u; stopping\n",
                         static_cast<int>(mPhase),
                         static_cast<unsigned>(mCurrentStepIndex));
                    mWalker->brake();
                    mState = TERMINATED;
                    break;
            }
            break;

        case TERMINATED:
            break;

        default:
            LOGE("[RALLY] invalid state=%d; stopping\n", static_cast<int>(mState));
            mWalker->brake();
            mState = TERMINATED;
            break;
    }
}

// ---------------------------------------------------------------------------
// フェーズ遷移
// ---------------------------------------------------------------------------

void RallyTracer::startNextStep()
{
    LOGD("[RALLY] startNextStep: index=%u size=%u state=%d phase=%d\n",
         static_cast<unsigned>(mCurrentStepIndex),
         static_cast<unsigned>(mRoute.size()),
         static_cast<int>(mState), static_cast<int>(mPhase));

    if(mCurrentStepIndex >= mRoute.size()) {
        if(mFinalHeadingDeg >= 0 && !mIsFinalTurning) {
            int turnBodyDeg = shortestTurn(mHeadingDeg, mFinalHeadingDeg);
            if(std::abs(turnBodyDeg) >= 5) {
                mIsFinalTurning = true;
                LOGI("[RALLY] final heading: %d->%d\n", mHeadingDeg, mFinalHeadingDeg);
                beginTurning(mFinalHeadingDeg);
                mState = WALKING;
                return;
            }
            mHeadingDeg = mFinalHeadingDeg;
        }
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
        beginMoving(calcMoveWheelDegrees(mCurrentPos, step.destination),
                calcMoveDirection(mCurrentPos, step.destination));
    } else {
        beginTurning(targetHeading);
    }

    mState = WALKING;
}

void RallyTracer::beginTurning(int targetHeadingDeg)
{
    mTargetHeadingDeg  = ((targetHeadingDeg % 360) + 360) % 360;
    int turnBodyDeg    = shortestTurn(mHeadingDeg, mTargetHeadingDeg);
    
    // 旋回方向に応じた補正係数を適用
    double scale = turnBodyDeg > 0 ? LEFT_TURN_SCALE : RIGHT_TURN_SCALE;
    int nominalWheelDegrees = static_cast<int>(turnBodyDeg
                                               * WHEEL_DEGREES_PER_BODY_DEGREE
                                               * scale);
    int turnCorrection = std::max(-TURN_CORRECTION_LIMIT,
                                  std::min(TURN_CORRECTION_LIMIT,
                                           -mPreviousTurnError));
    mTargetWheelDegrees = nominalWheelDegrees + turnCorrection;
    
    resetPhaseCounters();
    mWalker->beginEncoderCorrection();
    mPhase = TURNING;
        LOGD("[RALLY] begin turning: bodyDeg=%d scale=%g nominal=%d correction=%d wheelDeg=%d startEncoder=(%d,%d)\n",
            turnBodyDeg, scale, nominalWheelDegrees, turnCorrection,
            mTargetWheelDegrees,
         mPhaseStartLeftCount, mPhaseStartRightCount);
}

void RallyTracer::beginMoving(int wheelDegrees, int moveDirection)
{
    mTargetWheelDegrees = wheelDegrees;
    mMoveDirection = moveDirection >= 0 ? 1 : -1;
    resetPhaseCounters();
    mWalker->beginEncoderCorrection();
    mPhase = MOVING;
    mMarkerDetectedInPhase = false;
    mIsMarkerSearchRecoveryTurning = false;
    // 仮想QR（外周ゲート）への移動中は実 QR がないのでマーカー補正を無効化する
    mMarkerCorrectionAllowedThisStep = mCurrentStepIndex < mRoute.size()
        && mRoute[mCurrentStepIndex].type == RouteStepType::MOVE;
    LOGD("[RALLY] begin moving: target=%d dir=%d wheel=(%d,%d) markerCorrection=%d\n",
         wheelDegrees, mMoveDirection,
         mWalker->getLeftCount(), mWalker->getRightCount(),
         mMarkerCorrectionAllowedThisStep ? 1 : 0);
}

void RallyTracer::beginReturning(int wheelDegrees)
{
    mTargetWheelDegrees = wheelDegrees;
    mMoveDirection = -mMoveDirection;
    resetPhaseCounters();
    mWalker->beginEncoderCorrection();
    mPhase = RETURNING;
    mMarkerDetectedInPhase = false;
    // 必ず仮想QRからの後退なのでマーカー補正は常に無効
    mMarkerCorrectionAllowedThisStep = false;
    LOGD("[RALLY] begin returning: wheelDeg=%d\n", wheelDegrees);
}

void RallyTracer::beginMarkerOffset()
{
    mTargetWheelDegrees = MARKER_TO_CENTER_WHEEL_DEGREES;
    resetPhaseCounters();
    mWalker->beginEncoderCorrection();
    mPhase = MARKER_OFFSET;
    LOGI("[RALLY] marker offset: wheelDeg=%d\n", mTargetWheelDegrees);
}

void RallyTracer::beginMarkerSearch()
{
    mMarkerSearchRing = 0;
    mMarkerSearchHeadingDeg = mHeadingDeg;
    mMarkerSearchTurnWheelDegrees = static_cast<int>(mMarkerSearchAngleDeg
                                                      * WHEEL_DEGREES_PER_BODY_DEGREE * RIGHT_TURN_SCALE);
    mMarkerSearchCrawlOffset = 0;
    mPhase = MARKER_SEARCH;
    beginMarkerSearchSweep();
    LOGI("[RALLY] marker search: heading=%d range=+-%ddeg crawlStep=%d fwdStages=%d backStages=%d\n",
         mMarkerSearchHeadingDeg, mMarkerSearchAngleDeg,
         MARKER_SEARCH_CRAWL_STEP_WHEEL_DEGREES,
         MARKER_SEARCH_MAX_CRAWL_STAGES_FORWARD, MARKER_SEARCH_MAX_CRAWL_STAGES_BACKWARD);
}

void RallyTracer::beginMarkerSearchSweep()
{
    mMarkerSearchSubStage = MARKER_SEARCH_SWEEP_PLUS;
    mTargetWheelDegrees = mMarkerSearchTurnWheelDegrees;
    resetPhaseCounters();
    mWalker->beginEncoderCorrection();
    LOGD("[RALLY] marker search: ring=%d sweep at crawlOffset=%d\n",
         mMarkerSearchRing, mMarkerSearchCrawlOffset);
}

void RallyTracer::completeMovingStep()
{
    const RouteStep& step = mRoute[mCurrentStepIndex];
    if(step.type == RouteStepType::VIRTUAL_DETOUR) {
        beginReturning(calcMoveWheelDegrees(mCurrentPos, step.destination));
    } else {
        finishStep();
    }
}

void RallyTracer::finishStep()
{
    LOGD("[RALLY] finishStep: index=%u size=%u\n",
         static_cast<unsigned>(mCurrentStepIndex),
         static_cast<unsigned>(mRoute.size()));

    if(mCurrentStepIndex >= mRoute.size()) {
        LOGE("[RALLY] finishStep out of range: index=%u size=%u; stopping\n",
             static_cast<unsigned>(mCurrentStepIndex),
             static_cast<unsigned>(mRoute.size()));
        mWalker->brake();
        mState = TERMINATED;
        return;
    }

    const RouteStep& step = mRoute[mCurrentStepIndex];

    // 現在位置を更新
    if(step.type == RouteStepType::VIRTUAL_DETOUR) {
        mCurrentPos = step.returnPos;  // 後退して戻った実 QR 座標
        // 向きは前進方向のまま（後退しても車体は向きを変えていない）
    } else {
        mCurrentPos = step.destination;
    }

    int leftTotal = mWalker->getLeftCount();
    int rightTotal = mWalker->getRightCount();
    LOGI("[RALLY] step %u done, now at (%d,%d) heading=%d wheel=(%d,%d)\n",
         static_cast<unsigned>(mCurrentStepIndex),
         mCurrentPos.x, mCurrentPos.y, mHeadingDeg,
         leftTotal, rightTotal);

    mCurrentStepIndex++;
    LOGD("[RALLY] finishStep: advancing to index=%u\n",
         static_cast<unsigned>(mCurrentStepIndex));
    startNextStep();
}

// ---------------------------------------------------------------------------
// フェーズ実行
// ---------------------------------------------------------------------------

void RallyTracer::execTurning()
{
    int current   = getTurnWheelDegrees();
    int remaining = mTargetWheelDegrees - current;
    int leftDelta = mWalker->getLeftCount() - mPhaseStartLeftCount;
    int rightDelta = mWalker->getRightCount() - mPhaseStartRightCount;

    if(std::abs(remaining) <= TURN_TOLERANCE) {
        mPreviousTurnError = current - mTargetWheelDegrees;
        mWalker->brake();
        mWalker->setPwm(0, 0);
        mBrakeCountdown = 30;  // 停止を2フレーム確認
        mPhase = BRAKING;
           LOGI("[RALLY] turn complete: target=%d actual=%d wheel=(%d,%d) delta=(%d,%d) error=%d nextCorrection=%d, entering brake\n",
             mTargetWheelDegrees, current,
             mWalker->getLeftCount(), mWalker->getRightCount(),
             leftDelta, rightDelta,
               mTargetWheelDegrees - current, -mPreviousTurnError);
        return;
    }

    // 残量が正 → 反時計回り (CCW): 左後退, 右前進
    // 残量が負 →     時計回り  (CW): 左前進, 右後退
    int turnPwm = std::abs(remaining) <= APPROACH_WINDOW
        ? std::min(mTurnPwm, APPROACH_PWM) : mTurnPwm;
    if(remaining > 0) {
        mWalker->setPwm(-turnPwm, turnPwm);
    } else {
        mWalker->setPwm(turnPwm, -turnPwm);
    }
    LOGD_EVERY(5,
               "[RALLY] turning: target=%d current=%d remaining=%d wheel=(%d,%d) delta=(%d,%d) pwm=(%d,%d)\n",
               mTargetWheelDegrees, current, remaining,
               mWalker->getLeftCount(), mWalker->getRightCount(),
               leftDelta, rightDelta,
               remaining > 0 ? -turnPwm : turnPwm,
               remaining > 0 ? turnPwm : -turnPwm);
    mWalker->runWithEncoderCorrection(remaining > 0 ? -turnPwm : turnPwm,
                                      remaining > 0 ? turnPwm : -turnPwm);
}

void RallyTracer::execBraking()
{
    mBrakeCountdown--;

    if(mBrakeCountdown <= 0) {
        mHeadingDeg = mTargetHeadingDeg;
        if(mIsFinalTurning) {
            mWalker->stop();
            mState = TERMINATED;
            LOGI("[RALLY] final heading reached: %d\n", mHeadingDeg);
            return;
        }
        if(mIsMarkerSearchRecoveryTurning) {
            mIsMarkerSearchRecoveryTurning = false;
            completeMovingStep();
            return;
        }
        const RouteStep& step = mRoute[mCurrentStepIndex];
        beginMoving(calcMoveWheelDegrees(mCurrentPos, step.destination),
                calcMoveDirection(mCurrentPos, step.destination));
        return;
    }

    // ブレーキカウント中も能動ブレーキを継続し、惰性によるずれを抑える
    mWalker->brake();
    mWalker->setPwm(0, 0);
}

void RallyTracer::execMoving()
{
    int leftDelta  = mWalker->getLeftCount()  - mPhaseStartLeftCount;
    int rightDelta = mWalker->getRightCount() - mPhaseStartRightCount;
    int current   = mMoveDirection * getMoveWheelDegrees();
    int remaining = mTargetWheelDegrees - current;

    if(remaining <= MOVE_TOLERANCE) {
        mWalker->brake();
        LOGI("[RALLY] move complete: target=%d actual=%d wheel=(%d,%d) delta=(%d,%d) error=%d\n",
             mTargetWheelDegrees, current,
             mWalker->getLeftCount(), mWalker->getRightCount(),
             leftDelta, rightDelta,
             mTargetWheelDegrees - current);
        if(mEnableMarkerCorrection && mColorSensor != nullptr && mMarkerCorrectionAllowedThisStep) {
            beginMarkerSearch();
        } else {
            completeMovingStep();
        }
        return;
    }

    if(isMarkerSnapTriggered(remaining)) {
        mWalker->brake();
        beginMarkerOffset();
        return;
    }

    int movePwm = remaining <= APPROACH_WINDOW
        ? std::min(mMovePwm, APPROACH_PWM) : mMovePwm;
    LOGD_EVERY(10,
               "[RALLY] moving: target=%d current=%d remaining=%d wheel=(%d,%d) delta=(%d,%d) pwm=%d\n",
               mTargetWheelDegrees, current, remaining,
               mWalker->getLeftCount(), mWalker->getRightCount(),
               leftDelta, rightDelta, movePwm);
    mWalker->runWithEncoderCorrection(mMoveDirection * movePwm,
                                      mMoveDirection * movePwm);
}

void RallyTracer::execMarkerOffset()
{
    int leftDelta  = mWalker->getLeftCount()  - mPhaseStartLeftCount;
    int rightDelta = mWalker->getRightCount() - mPhaseStartRightCount;
    // センサーは常に車軸より +方位側にあるので、後退中(mMoveDirection=-1)でも進行方向は常に +方位側
    int current = getMoveWheelDegrees();
    if(current >= mTargetWheelDegrees - MOVE_TOLERANCE) {
        mWalker->brake();
        LOGI("[RALLY] marker offset complete: target=%d actual=%d wheel=(%d,%d) delta=(%d,%d) error=%d\n",
             mTargetWheelDegrees, current,
             mWalker->getLeftCount(), mWalker->getRightCount(),
             leftDelta, rightDelta,
             mTargetWheelDegrees - current);
        if(mIsMarkerSearchRecoveryTurning) {
            beginTurning(mMarkerSearchHeadingDeg);
        } else {
            completeMovingStep();
        }
        return;
    }

    LOGD_EVERY(10,
               "[RALLY] marker offset: target=%d current=%d remaining=%d wheel=(%d,%d) delta=(%d,%d)\n",
               mTargetWheelDegrees, current, mTargetWheelDegrees - current,
               mWalker->getLeftCount(), mWalker->getRightCount(),
               leftDelta, rightDelta);
    mWalker->runWithEncoderCorrection(mMovePwm, mMovePwm);
}

void RallyTracer::execMarkerSearch()
{
    if(isMarkerDetected()) {
        int relativeWheelDeg = 0;
        switch(mMarkerSearchSubStage) {
            case MARKER_SEARCH_SWEEP_PLUS:
                // 基準方位(0)から開始するので相対角はそのまま現在値
                relativeWheelDeg = getTurnWheelDegrees();
                break;
            case MARKER_SEARCH_SWEEP_MINUS:
                // +A から開始して -A へ向かうので +A を起点に加算する
                relativeWheelDeg = mMarkerSearchTurnWheelDegrees + getTurnWheelDegrees();
                break;
            case MARKER_SEARCH_RETURN_HEADING:
                // -A から開始して基準方位へ戻るので -A を起点に加算する
                relativeWheelDeg = -mMarkerSearchTurnWheelDegrees + getTurnWheelDegrees();
                break;
            case MARKER_SEARCH_CRAWL:
                relativeWheelDeg = 0;  // クロール中は基準方位のまま
                break;
        }
        int bodyDeg = static_cast<int>(relativeWheelDeg / WHEEL_DEGREES_PER_BODY_DEGREE);
        mHeadingDeg = ((mMarkerSearchHeadingDeg + bodyDeg) % 360 + 360) % 360;
        mIsMarkerSearchRecoveryTurning = true;
        mWalker->brake();
        LOGI("[RALLY] marker found during search: ring=%d crawlOffset=%d heading=%d\n",
             mMarkerSearchRing, mMarkerSearchCrawlOffset, mHeadingDeg);
        beginMarkerOffset();
        return;
    }

    if(mMarkerSearchSubStage == MARKER_SEARCH_CRAWL) {
        execMarkerSearchCrawl();
    } else {
        execMarkerSearchTurn();
    }
}

void RallyTracer::execMarkerSearchTurn()
{
    int current   = getTurnWheelDegrees();
    int remaining = mTargetWheelDegrees - current;

    if(std::abs(remaining) <= TURN_TOLERANCE) {
        advanceMarkerSearchStage();
        return;
    }

    if(remaining > 0) {
        mWalker->setPwm(-mTurnPwm, mTurnPwm);
    } else {
        mWalker->setPwm(mTurnPwm, -mTurnPwm);
    }
    mWalker->runWithEncoderCorrection(remaining > 0 ? -mTurnPwm : mTurnPwm,
                                      remaining > 0 ? mTurnPwm : -mTurnPwm);
}

void RallyTracer::execMarkerSearchCrawl()
{
    int current   = mMarkerSearchCrawlStartOffset + getMoveWheelDegrees();
    int remaining = mMarkerSearchCrawlTargetOffset - current;

    if(std::abs(remaining) <= MOVE_TOLERANCE) {
        mMarkerSearchCrawlOffset = mMarkerSearchCrawlTargetOffset;
        advanceMarkerSearchStage();
        return;
    }

    int crawlPwm = std::min(mMovePwm, APPROACH_PWM);
    int direction = remaining > 0 ? 1 : -1;
    mWalker->runWithEncoderCorrection(direction * crawlPwm, direction * crawlPwm);
}

void RallyTracer::advanceMarkerSearchStage()
{
    switch(mMarkerSearchSubStage) {
        case MARKER_SEARCH_SWEEP_PLUS:
            mMarkerSearchSubStage = MARKER_SEARCH_SWEEP_MINUS;
            mTargetWheelDegrees = -2 * mMarkerSearchTurnWheelDegrees;
            resetPhaseCounters();
            mWalker->beginEncoderCorrection();
            LOGD("[RALLY] marker search: ring=%d sweep opposite direction\n", mMarkerSearchRing);
            return;

        case MARKER_SEARCH_SWEEP_MINUS:
            mMarkerSearchSubStage = MARKER_SEARCH_RETURN_HEADING;
            mTargetWheelDegrees = mMarkerSearchTurnWheelDegrees;
            resetPhaseCounters();
            mWalker->beginEncoderCorrection();
            LOGD("[RALLY] marker search: ring=%d return to base heading\n", mMarkerSearchRing);
            return;

        case MARKER_SEARCH_RETURN_HEADING: {
            int maxRing = MARKER_SEARCH_MAX_CRAWL_STAGES_FORWARD + MARKER_SEARCH_MAX_CRAWL_STAGES_BACKWARD;
            int nextRing = mMarkerSearchRing + 1;
            int targetOffset = (nextRing <= maxRing) ? markerSearchRingOffset(nextRing) : 0;

            if(targetOffset == mMarkerSearchCrawlOffset) {
                // 既に目標オフセットにいるのでクロール不要
                if(nextRing <= maxRing) {
                    mMarkerSearchRing = nextRing;
                    beginMarkerSearchSweep();
                } else {
                    finishMarkerSearchNotFound();
                }
                return;
            }

            mMarkerSearchSubStage = MARKER_SEARCH_CRAWL;
            mMarkerSearchCrawlStartOffset = mMarkerSearchCrawlOffset;
            mMarkerSearchCrawlTargetOffset = targetOffset;
            mMarkerSearchNextRing = nextRing;
            resetPhaseCounters();
            mWalker->beginEncoderCorrection();
            LOGD("[RALLY] marker search: crawl from=%d to=%d (nextRing=%d)\n",
                 mMarkerSearchCrawlStartOffset, targetOffset, nextRing);
            return;
        }

        case MARKER_SEARCH_CRAWL: {
            int maxRing = MARKER_SEARCH_MAX_CRAWL_STAGES_FORWARD + MARKER_SEARCH_MAX_CRAWL_STAGES_BACKWARD;
            if(mMarkerSearchNextRing <= maxRing) {
                mMarkerSearchRing = mMarkerSearchNextRing;
                beginMarkerSearchSweep();
            } else {
                finishMarkerSearchNotFound();
            }
            return;
        }
    }
}

void RallyTracer::finishMarkerSearchNotFound()
{
    LOGI("[RALLY] marker search: not found after %d ring(s), resuming route\n",
         MARKER_SEARCH_MAX_CRAWL_STAGES_FORWARD + MARKER_SEARCH_MAX_CRAWL_STAGES_BACKWARD + 1);
    mWalker->brake();
    completeMovingStep();
}

// static
int RallyTracer::markerSearchRingOffset(int ring)
{
    if(ring <= 0) {
        return 0;
    }
    // 前方・後方を交互に1段ずつ広げるが、前方は早めに打ち切り、
    // 後方（直進中に見逃した側）はより遠くまで探索を続ける
    int pairCount = 2 * MARKER_SEARCH_MAX_CRAWL_STAGES_FORWARD;
    if(ring <= pairCount) {
        int magnitude = (ring + 1) / 2;
        return (ring % 2 == 1)
            ? magnitude * MARKER_SEARCH_CRAWL_STEP_WHEEL_DEGREES
            : -magnitude * MARKER_SEARCH_CRAWL_STEP_WHEEL_DEGREES;
    }
    int magnitude = MARKER_SEARCH_MAX_CRAWL_STAGES_FORWARD + (ring - pairCount);
    return -magnitude * MARKER_SEARCH_CRAWL_STEP_WHEEL_DEGREES;
}


void RallyTracer::execReturning()
{
    int leftDelta  = mWalker->getLeftCount()  - mPhaseStartLeftCount;
    int rightDelta = mWalker->getRightCount() - mPhaseStartRightCount;
    int traveled  = mMoveDirection * getMoveWheelDegrees();
    int remaining = mTargetWheelDegrees - traveled;

    if(remaining <= MOVE_TOLERANCE || isMarkerSnapTriggered(remaining)) {
        mWalker->brake();
        LOGI("[RALLY] return complete: target=%d actual=%d wheel=(%d,%d) delta=(%d,%d) error=%d\n",
             mTargetWheelDegrees, traveled,
             mWalker->getLeftCount(), mWalker->getRightCount(),
             leftDelta, rightDelta,
             mTargetWheelDegrees - traveled);
        finishStep();
        return;
    }

    LOGD_EVERY(10,
               "[RALLY] returning: target=%d traveled=%d remaining=%d wheel=(%d,%d) delta=(%d,%d)\n",
               mTargetWheelDegrees, traveled, remaining,
               mWalker->getLeftCount(), mWalker->getRightCount(),
               leftDelta, rightDelta);
    mWalker->runWithEncoderCorrection(mMoveDirection * mMovePwm,
                                      mMoveDirection * mMovePwm);
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
    if(!mEnableMarkerCorrection || mColorSensor == nullptr || !mMarkerCorrectionAllowedThisStep) {
        return false;
    }
    if(remainingDegrees > mMarkerSnapWindowDegrees) {
        return false;
    }
    if(!isMarkerDetected()) {
        return false;
    }

    LOGI("[RALLY] marker snap: remain=%d window=%d\n",
         remainingDegrees, mMarkerSnapWindowDegrees);
    return true;
}

bool RallyTracer::isMarkerDetected()
{
    if(!mEnableMarkerCorrection || mColorSensor == nullptr || mMarkerDetectedInPhase) {
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
        LOGI("[RALLY] marker detected: reflection=%d threshold=%d\n",
            reflection, mMarkerReflectionThreshold);
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
    if(dx != 0) {
        return 0;
    }
    if(dy != 0) {
        return 90;
    }
    return 0;
}

// static
int RallyTracer::calcMoveDirection(const QRPos& from, const QRPos& to)
{
    int dx = to.x - from.x;
    int dy = to.y - from.y;
    int delta = dx != 0 ? dx : dy;
    return delta >= 0 ? 1 : -1;
}

// static
int RallyTracer::calcMoveWheelDegrees(const QRPos& from, const QRPos& to)
{
    int dx = to.x - from.x;
    int dy = to.y - from.y;
    double dist = std::sqrt(static_cast<double>(dx * dx + dy * dy));
    return static_cast<int>(dist * QR_GRID_WHEEL_DEGREES);
}
