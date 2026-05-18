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
      mThreshold(30)
{
}

/**
 * ライン境界から外れた度合いを判定する
 * @retval ライン境界とセンサ値との差分
 */
int LineMonitor::calDiffReflection() const
{
    // 光センサからの取得値を見て
    // ライン境界の値との差分を算出して返す
    int diff = mColorSensor.getReflection() - (int)mThreshold;

    spikeapi::ColorSensor::HSV hsv;
    mColorSensor.getHSV(hsv);
    LOGD_EVERY(100, "[LINE_MON] diff=%d\th=%u\ts=%u\tv=%u\n", diff, hsv.h, hsv.s, hsv.v);

    return diff;
}

/**
 * ライン閾値を設定する
 * @param threshold ライン閾値
 */
void LineMonitor::setThreshold(int8_t threshold)
{
    mThreshold = threshold;
}
