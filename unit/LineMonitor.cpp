/******************************************************************************
 *  LineMonitor.cpp (for SPIKE)
 *  Created on: 2025/01/05
 *  Implementation of the Class LineMonitor
 *  Author: Kazuhiro.Kawachi
 *  Modifier: Yuki Tsuchitoi
 *  Copyright (c) 2025 Embedded Technology Software Design Robot Contest
 *****************************************************************************/

#include "LineMonitor.h"

#include "ColorSensor.h"
#include "Log.h"

// 定数宣言
const int8_t LineMonitor::INITIAL_THRESHOLD_BLACK = 5;  // 黒色の光センサ値
const int8_t LineMonitor::INITIAL_THRESHOLD_WHITE = 35; // 白色の光センサ値

/**
 * コンストラクタ
 * @param colorSensor カラーセンサ
 */
LineMonitor::LineMonitor(const spikeapi::ColorSensor &colorSensor)
    : mColorSensor(colorSensor),
      //mThreshold((INITIAL_THRESHOLD_BLACK + INITIAL_THRESHOLD_WHITE) / 2)
    mThreshold(30),
    mReflectionCount(0),
    mNextReflectionIndex(0)
{
}

/**
 * ライン境界から外れた度合いを判定する
 * @retval ライン境界とセンサ値との差分
 */
int LineMonitor::calDiffReflection()
{
    // 光センサからの取得値を見て
    // ライン境界の値との差分を算出して返す
    int reflection = mColorSensor.getReflection();
    int filteredReflection = filterReflection(reflection);
    int diff = filteredReflection - (int)mThreshold;

    spikeapi::ColorSensor::HSV hsv;
    mColorSensor.getHSV(hsv);
    LOGD_EVERY(100, "[LINE_MON] raw=%d\tfiltered=%d\tdiff=%d\th=%u\ts=%u\tv=%u\n", reflection,
               filteredReflection, diff, hsv.h, hsv.s, hsv.v);

    return diff;
}

/**
 * ライン閾値を設定する
 * @param threshold ライン閾値
 */
void LineMonitor::setThreshold(int8_t threshold)
{
    mThreshold = threshold;
    mReflectionCount = 0;
    mNextReflectionIndex = 0;
}

int LineMonitor::filterReflection(int reflection)
{
    mReflections[mNextReflectionIndex] = reflection;
    mNextReflectionIndex = (mNextReflectionIndex + 1) % FILTER_WINDOW;
    if(mReflectionCount < FILTER_WINDOW) {
        mReflectionCount++;
        return reflection;
    }

    int sorted[FILTER_WINDOW];
    for(int i = 0; i < FILTER_WINDOW; i++) {
        sorted[i] = mReflections[i];
    }
    for(int i = 0; i < FILTER_WINDOW - 1; i++) {
        for(int j = i + 1; j < FILTER_WINDOW; j++) {
            if(sorted[i] > sorted[j]) {
                int temporary = sorted[i];
                sorted[i] = sorted[j];
                sorted[j] = temporary;
            }
        }
    }
    return sorted[FILTER_WINDOW / 2];
}
