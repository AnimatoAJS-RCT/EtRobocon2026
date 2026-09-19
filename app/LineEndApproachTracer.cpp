#include "LineEndApproachTracer.h"
#include "Log.h"
#include "Util.h"

#include <algorithm>
#include <cmath>
#include <sstream>

bool LineEndApproachTracer::Config::parse(const std::vector<std::string>& tokens, bool mirror)
{
    Config candidate;
    auto readInt = [](const std::string& token, int& value) {
        std::istringstream input(token);
        return static_cast<bool>(input >> value) && input.eof();
    };
    auto readDouble = [](const std::string& token, double& value) {
        std::istringstream input(token);
        return static_cast<bool>(input >> value) && input.eof() && std::isfinite(value);
    };
    bool valid = tokens.size() >= 9 && tokens[0] == "LineEndApproachTracer";
    if(valid) {
        valid = readInt(tokens[1], candidate.targetBrightness)
            && readInt(tokens[2], candidate.pwm)
            && readInt(tokens[3], candidate.maxPwm)
            && readInt(tokens[4], candidate.turnPwm)
            && (tokens[5] == "LEFT_EDGE" || tokens[5] == "RIGHT_EDGE")
            && readDouble(tokens[6], candidate.kp)
            && readDouble(tokens[7], candidate.ki)
            && readDouble(tokens[8], candidate.kd);
        candidate.isLeftEdge = (tokens[5] == "LEFT_EDGE") != mirror;
        candidate.mirrorCourse = mirror;
    }
    struct Option {
        const char* name;
        double* value;
    };
    Option options[] = {
        {"qrClearMm", &candidate.qrClearMm},
        {"approachMaxMm", &candidate.approachMaxMm},
        {"sensorOffsetMm", &candidate.sensorOffsetMm},
        {"traceMaxMm", &candidate.traceMaxMm},
        {"stableMm", &candidate.stableMm},
        {"gapMm", &candidate.gapMm},
        {"markerMaxMm", &candidate.markerMaxMm},
        {"scanDeg", &candidate.scanDeg},
        {"endOffsetMm", &candidate.endOffsetMm}
    };
    for(size_t index = 9; valid && index < tokens.size(); index++) {
        size_t separator = tokens[index].find('=');
        if(separator == std::string::npos) {
            valid = false;
            break;
        }
        std::string name = tokens[index].substr(0, separator);
        std::string value = tokens[index].substr(separator + 1);
        bool found = false;
        for(const auto& option : options) {
            if(name == option.name) {
                found = true;
                valid = readDouble(value, *option.value);
                break;
            }
        }
        if(name == "timeoutTicks") {
            found = true;
            valid = readInt(value, candidate.timeoutTicks);
        }
        valid = valid && found;
    }
    if(!valid || !candidate.isValid()) {
        pwm = 0;
        return false;
    }
    *this = candidate;
    return true;
}

bool LineEndApproachTracer::Config::isValid() const
{
    return targetBrightness > 0 && targetBrightness < 80
        && pwm > 0 && pwm <= maxPwm && maxPwm <= 100
        && turnPwm > 0 && turnPwm <= 100
        && std::isfinite(kp) && std::isfinite(ki) && std::isfinite(kd)
        && kp >= 0 && ki >= 0 && kd >= 0
        && std::isfinite(qrClearMm) && qrClearMm >= 0
        && std::isfinite(approachMaxMm) && approachMaxMm > qrClearMm
        && std::isfinite(sensorOffsetMm) && sensorOffsetMm > 0
        && std::isfinite(traceMaxMm) && traceMaxMm > stableMm + gapMm
        && std::isfinite(stableMm) && stableMm > 0
        && std::isfinite(gapMm) && gapMm > 0
        && std::isfinite(markerMaxMm) && markerMaxMm > 0
        && std::isfinite(scanDeg) && scanDeg > 0 && scanDeg <= 45
        && std::isfinite(endOffsetMm) && std::abs(endOffsetMm) <= traceMaxMm
        && timeoutTicks > 0;
}

LineEndApproachTracer::LineEndApproachTracer(Walker* walker,
    spikeapi::ColorSensor* sensor, const Config& config)
    : mWalker(walker), mSensor(sensor), mConfig(config),
      mGain(config.kp, config.ki, config.kd), mPid(&mGain, 2),
            mTurn(walker, config.mirrorCourse ? -1 : 1, 90, config.turnPwm)
{
    mState = UNDEFINED;
    if(config.ki > 0) {
        mPid.setIntegralLimit(config.maxPwm / config.ki);
    }
}

void LineEndApproachTracer::setCalibration(int black, int white)
{
    mBlack = black;
    mWhite = white;
}

bool LineEndApproachTracer::hasFailed() const
{
    return mPhase == FAILED;
}

double LineEndApproachTracer::distanceMm() const
{
    return (static_cast<double>(mWalker->getLeftCount()) + mWalker->getRightCount())
        * 0.5 * 55.0 * 3.1415926535 / 360.0;
}

double LineEndApproachTracer::headingWdeg() const
{
    return (static_cast<double>(mWalker->getRightCount()) - mWalker->getLeftCount()) * 0.5;
}

void LineEndApproachTracer::enter(Phase phase)
{
    LOGI("[LINE_END] phase=%d -> %d distance=%.1f heading=%.1f\n",
         static_cast<int>(mPhase), static_cast<int>(phase), distanceMm(), headingWdeg());
    mWalker->brake();
    mPhase = phase;
    mPhaseStartMm = distanceMm();
    mMatchCount = 0;
    mBlackCount = 0;
    mMarkerCount = 0;
    mTurnTargetActive = false;
    mWalker->beginEncoderCorrection();
}

void LineEndApproachTracer::fail(const char* reason)
{
    LOGE("[LINE_END] failed: %s; continue to next tracer\n", reason);
    mWalker->brake();
    mPhase = FAILED;
    mState = TERMINATED;
}

void LineEndApproachTracer::driveStraight(int direction)
{
    mWalker->runWithEncoderCorrection(direction * mConfig.pwm, direction * mConfig.pwm);
}

bool LineEndApproachTracer::turnTo(double target)
{
    double error = target - headingWdeg();
    if(!mTurnTargetActive || target != mTurnTarget) {
        mTurnTargetActive = true;
        mTurnTarget = target;
        mTurnDirection = error >= 0 ? 1 : -1;
    }
    if(std::abs(error) <= 3.0 || mTurnDirection * error <= 0) {
        mWalker->brake();
        return true;
    }
    int power = mTurnDirection * mConfig.turnPwm;
    mWalker->setPwm(-power, power);
    mWalker->run();
    return false;
}

void LineEndApproachTracer::startScan(Phase phase)
{
    mScanCenter = headingWdeg();
    mScanLeg = 0;
    enter(phase);
}

bool LineEndApproachTracer::scanFinished()
{
    double span = mConfig.scanDeg * 14.0 / 9.0;
    double target = mScanCenter + (mScanLeg == 0 ? span : -span);
    if(turnTo(target)) {
        mScanLeg++;
        return mScanLeg == 2;
    }
    return false;
}

void LineEndApproachTracer::startTrace(bool resetStability)
{
    if(resetStability) {
        mStable = false;
        mStableStartMm = distanceMm();
    }
    mPid.reset();
    enter(TRACE);
}

void LineEndApproachTracer::followLine(int reflection)
{
    double target = mBlack + (mWhite - mBlack) * mConfig.targetBrightness / 100.0;
    double turn = mPid.calculatePid(reflection - target);
    if(mConfig.isLeftEdge) {
        turn = -turn;
    }
    int left = std::max(-mConfig.maxPwm,
                       std::min(mConfig.maxPwm, static_cast<int>(mConfig.pwm - turn)));
    int right = std::max(-mConfig.maxPwm,
                        std::min(mConfig.maxPwm, static_cast<int>(mConfig.pwm + turn)));
    mWalker->setPwm(left, right);
    mWalker->run();
}

void LineEndApproachTracer::run()
{
    if(mState == TERMINATED || hasFailed()) {
        mWalker->brake();
        return;
    }
    if(mState == UNDEFINED) {
        mState = WAITING_FOR_START;
        return;
    }
    if(mState == WAITING_FOR_START) {
        bool ready = mStarterList.empty();
        for(auto starter : mStarterList) {
            ready = ready || starter->isPushed();
        }
        if(!ready) {
            return;
        }
        mState = WALKING;
        if(!mConfig.isValid() || mWhite <= mBlack) {
            fail("invalid configuration or calibration");
            return;
        }
        enter(APPROACH);
        return;
    }
    if(++mTicks > mConfig.timeoutTicks) {
        fail("timeout");
        return;
    }

    int reflection = mSensor->getReflection();
    double normalized = 100.0 * (reflection - mBlack) / (mWhite - mBlack);
    spikeapi::ColorSensor::HSV hsv;
    mSensor->getHSV(hsv);
    eColor color = getColor(hsv.h, hsv.s, hsv.v);
    bool marker = color == BLUE || color == RED || color == YELLOW || color == GREEN;
    bool line = color == BLACK || marker;
    bool white = normalized >= 85 && !marker;
    if(!line && normalized <= mConfig.targetBrightness) {
        LOGD_EVERY(10, "[LINE_END] ignore non-line: raw=%d normalized=%.1f h=%d s=%d v=%d\n",
                   reflection, normalized, hsv.h, hsv.s, hsv.v);
    }
    mMatchCount = line ? mMatchCount + 1 : 0;
    mBlackCount = color == BLACK ? mBlackCount + 1 : 0;
    mMarkerCount = marker ? mMarkerCount + 1 : 0;
    double distance = distanceMm();
    double moved = distance - mPhaseStartMm;

    if(mPhase >= TRACE && mPhase <= VERIFY_BACK
       && distance - mTraceStartMm > mConfig.traceMaxMm) {
        fail("trace distance limit");
        return;
    }

    switch(mPhase) {
    case APPROACH:
        if(moved >= mConfig.approachMaxMm) {
            fail("approach distance limit");
        } else if(moved >= mConfig.qrClearMm) {
            if(white) {
                mSawWhite = true;
            }
            if(mSawWhite && mMatchCount >= 2) {
                enter(CENTER_ON_LINE);
            } else {
                driveStraight();
            }
        } else {
            mMatchCount = 0;
            driveStraight();
        }
        break;
    case CENTER_ON_LINE:
        if(moved >= mConfig.sensorOffsetMm) {
            enter(TURN_TO_LINE);
        } else {
            driveStraight();
        }
        break;
    case TURN_TO_LINE:
        mTurn.run();
        if(mTurn.isTerminated()) {
            mTraceStartMm = distance;
            enter(CHECK_AFTER_TURN);
        }
        break;
    case CHECK_AFTER_TURN:
    case ALIGN_SCAN:
        if(mBlackCount >= 2) {
            LOGI("[LINE_END] black acquired: start tracking\n");
            startTrace(true);
        } else if(mMarkerCount >= 2) {
            LOGI("[LINE_END] marker acquired: cross until black\n");
            enter(MARKER);
        } else if(line) {
            mWalker->brake();
        } else if(mPhase == CHECK_AFTER_TURN) {
            startScan(ALIGN_SCAN);
        } else if(scanFinished()) {
            fail("line not found after turn");
        }
        break;
    case TRACE:
        if(marker) {
            enter(MARKER);
        } else if(white) {
            if(mStable) {
                mGapStartMm = distance;
                mGapHeading = headingWdeg();
                enter(GAP);
            } else {
                mStableStartMm = distance;
                driveStraight();
            }
        } else {
            if(distance - mStableStartMm >= mConfig.stableMm) {
                mStable = true;
            }
            followLine(reflection);
        }
        break;
    case MARKER:
        if(moved >= mConfig.markerMaxMm) {
            fail("marker distance limit");
        } else if(mBlackCount >= 2) {
            startTrace(true);
        } else if(!line) {
            startScan(ALIGN_SCAN);
        } else {
            driveStraight();
        }
        break;
    case GAP:
        if(mMatchCount >= 2) {
            startTrace(true);
        } else if(moved >= mConfig.gapMm) {
            mEndTargetMm = mGapStartMm + mConfig.endOffsetMm;
            mPositionDirection = mEndTargetMm >= distance ? 1 : -1;
            LOGI("[LINE_END] endpoint confirmed: target=%.1f current=%.1f\n",
                 mEndTargetMm, distance);
            enter(POSITION_END);
        } else {
            driveStraight();
        }
        break;
    case END_SCAN:
        if(mMatchCount >= 2) {
            startTrace(true);
        } else if(scanFinished()) {
            if(!mStable) {
                fail("line lost before stable tracking");
            } else {
                enter(RESTORE_HEADING);
            }
        }
        break;
    case RESTORE_HEADING:
        if(turnTo(mGapHeading)) {
            enter(VERIFY_BACK);
        }
        break;
    case VERIFY_BACK:
        if(mMatchCount >= 2 && !marker) {
            mEndTargetMm = mGapStartMm + mConfig.endOffsetMm;
            mPositionDirection = mEndTargetMm >= distance ? 1 : -1;
            enter(POSITION_END);
        } else if(-moved >= mConfig.gapMm + mConfig.stableMm) {
            fail("no line behind endpoint candidate");
        } else {
            driveStraight(-1);
        }
        break;
    case POSITION_END:
        if(mPositionDirection * (mEndTargetMm - distance) <= 0) {
            mWalker->brake();
            mState = TERMINATED;
            LOGI("[LINE_END] complete distance=%.1f heading=%.1f\n", distance, headingWdeg());
        } else {
            driveStraight(mPositionDirection);
        }
        break;
    case FAILED:
        mWalker->brake();
        break;
    }
}