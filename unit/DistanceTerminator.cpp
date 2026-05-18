/******************************************************************************
 *  DistanceTerminator.cpp (for SPIKE)
 *  Created on: 2025/08/23
 *  Definition of the Class DistanceTerminator
 *  Author: Your Name
 *  Copyright (c) 2025 Embedded Technology Software Design Robot Contest
 *****************************************************************************/

#include "DistanceTerminator.h"
#include "Log.h"

const double DistanceTerminator::TIRE_DIAMETER = 5.5;
const double DistanceTerminator::PI = 3.1415926535;

/**
 * コンストラクタ
 * @param walker Walker
 * @param targetDistance 目標距離 (mm)
 */
DistanceTerminator::DistanceTerminator(Walker* walker, double targetDistance)
    : mWalker(walker), mTargetDistance(targetDistance), mInitialDistance(0.0), mCheckCount(0)
{
}

void DistanceTerminator::init()
{
    mInitialDistance = calcCurrentDistance();
        mCheckCount = 0;
        LOGI("[DIST_TERM] init: initial=%f target=%f\n", mInitialDistance, mTargetDistance);
}

bool DistanceTerminator::isToBeTerminate()
{
    double currentDistance = calcCurrentDistance() - mInitialDistance;
    bool isTerminate = currentDistance >= mTargetDistance;

    if(isTerminate) {
        LOGI("[DIST_TERM] current=%f target=%f match=1\n", currentDistance, mTargetDistance);
    } else if(mCheckCount < 5) {
        LOGD("[DIST_TERM] first-check[%d]: current=%f target=%f match=0\n", mCheckCount,
             currentDistance, mTargetDistance);
    } else {
        LOGD_EVERY(50, "[DIST_TERM] current=%f target=%f match=0\n", currentDistance,
                   mTargetDistance);
    }

    mCheckCount++;

    return isTerminate;
}


double DistanceTerminator::calcCurrentDistance()
{
    double wheelCircumference = TIRE_DIAMETER * PI;
    double leftDistance = (double)mWalker->getLeftCount() * wheelCircumference / 360.0;
    double rightDistance = (double)mWalker->getRightCount() * wheelCircumference / 360.0;
    return (leftDistance + rightDistance) / 2.0;
}

