/******************************************************************************
 *  ColorTerminator.cpp (for SPIKE )
 *  Created on: 2025/01/05
 *  Definition of the Class ColorTerminator
 *  Author: Kazuhiro Kawachi
 *  Modifier : Yuki Tsuchitoi
 *  Copyright (c) 2025 Embedded Technology Software Design Robot Contest
 *****************************************************************************/

#include "ColorTerminator.h"
#include "ColorSensor.h"
#include "Util.h"
#include "Log.h"

/**
 * コンストラクタ
 * @param colorSensor ColorSensor
 * @param termColor 停止する色
 */
ColorTerminator::ColorTerminator(const spikeapi::ColorSensor* colorSensor, eColor termColor)
  : mColorSensor(colorSensor),
    mTermColor(termColor),
    mLogCounter(0),
    mHasLastLoggedColor(false),
    mLastLoggedColor(BLACK)
{
}

bool ColorTerminator::isToBeTerminate()
{
    spikeapi::ColorSensor::HSV hsv;
    mColorSensor->getHSV(hsv);
    eColor c = getColor(hsv.h, hsv.s, hsv.v);
    bool isTerminate = (c == mTermColor);

    // Reduce periodic log flooding: print on color change, periodic sample, or terminate match.
    if(!mHasLastLoggedColor || (mLastLoggedColor != c) || (mLogCounter % 100 == 0)
       || isTerminate) {
        LOGD("[COLOR_TERM] h=%u\ts=%u\tv=%u\tcolor=%s\ttarget=%s\tmatch=%d\n", hsv.h,
             hsv.s, hsv.v, colorToString(c), colorToString(mTermColor), isTerminate ? 1 : 0);
        mLastLoggedColor = c;
        mHasLastLoggedColor = true;
    }

    mLogCounter++;
    return isTerminate;
}
