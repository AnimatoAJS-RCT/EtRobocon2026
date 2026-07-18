/******************************************************************************
 *  LineMonitor.h (for SPIKE )
 *  Created on: 2025/01/05
 *  Definition of the Class LineMonitor
 *  Author: Kazuhiro Kawachi
 *  Modifier : Yuki Tsuchitoi
 *  Copyright (c) 2025 Embedded Technology Software Design Robot Contest
 *****************************************************************************/

#ifndef ETTR_UNIT_LINEMONITOR_H_
#define ETTR_UNIT_LINEMONITOR_H_

#include "ColorSensor.h"

// 定義
class LineMonitor {
public:
    explicit LineMonitor(const spikeapi::ColorSensor& colorSensor);

    int calDiffReflection();
    void setThreshold(int8_t threshold);

private:
    static const int8_t INITIAL_THRESHOLD_BLACK;
    static const int8_t INITIAL_THRESHOLD_WHITE;
    static const int FILTER_WINDOW = 3;

    const spikeapi::ColorSensor& mColorSensor;
    int8_t mThreshold;
    int mReflections[FILTER_WINDOW];
    int mReflectionCount;
    int mNextReflectionIndex;

    int filterReflection(int reflection);
};

#endif  // ETTR_UNIT_LINEMONITOR_H_
