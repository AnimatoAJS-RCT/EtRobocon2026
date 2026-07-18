/******************************************************************************
 *  Walker.cpp (for SPIKE)
 *  Created on: 2025/01/05
 *  Implementation of the Class Walker
 *  Author: Kazuhiro.Kawachi
 *  Modifier: Yuki Tsuchitoi
 *  Copyright (c) 2025 Embedded Technology Software Design Robot Contest
 *****************************************************************************/

#ifndef ETTR_UNIT_WALKER_H_
#define ETTR_UNIT_WALKER_H_

#include "Motor.h"
using namespace spikeapi; 

class Walker {
public:
    Walker(spikeapi::Motor& leftWheel,
                    spikeapi::Motor& rightWheel);

    void init();
    void run();
    void stop();
    void brake();
    void setPwm(int leftPwm, int rightPwm);
    int getLeftCount();
    int getRightCount();

private:
    spikeapi::Motor& mLeftWheel;
    spikeapi::Motor& mRightWheel;
    int mLeftPwm;
    int mRightPwm;
};

#endif  // ETTR_UNIT_WALKER_H_