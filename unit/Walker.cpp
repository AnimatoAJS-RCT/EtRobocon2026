/******************************************************************************
 *  Walker.cpp (for SPIKE)
 *  Created on: 2025/01/05
 *  Implementation of the Class Walker
 *  Author: Kazuhiro.Kawachi
 *  Modifier: Yuki Tsuchitoi
 *  Copyright (c) 2025 Embedded Technology Software Design Robot Contest
 *****************************************************************************/

#include "Walker.h"
#include <stdio.h>

/**
 * コンストラクタ
 * @param leftWheel  左モータ
 * @param rightWheel 右モータ
 */
Walker::Walker(spikeapi::Motor& leftWheel, spikeapi::Motor& rightWheel)
  : mLeftWheel(leftWheel), mRightWheel(rightWheel), mLeftPwm(0), mRightPwm(0)
{
}

/**
 * 走行する
 */
void Walker::run()
{
    mLeftWheel.setPower(mLeftPwm);
    mRightWheel.setPower(mRightPwm);
}

/**
 * 走行を停止する
 */
void Walker::stop()
{
    mLeftWheel.stop();
    mRightWheel.stop();
}

void Walker::brake()
{
    mLeftWheel.brake();
    mRightWheel.brake();
}

/**
 * 走行に必要なものをリセットする
 */
void Walker::init()
{
    mLeftWheel.resetCount();
    mRightWheel.resetCount();
}

/**
 * PWMを設定する
 * @param leftPwm 左モーターのPWM値
 * @param rightPwm 右モーターのPWM値
 */
void Walker::setPwm(int leftPwm, int rightPwm)
{
    mLeftPwm = leftPwm;
    mRightPwm = rightPwm;
}

/**
 * 左タイヤの回転数を取得する
 */
int Walker::getLeftCount()
{
    return mLeftWheel.getCount();
}

/**
 * 右タイヤの回転数を取得する
 */
int Walker::getRightCount()
{
    return mRightWheel.getCount();
}
