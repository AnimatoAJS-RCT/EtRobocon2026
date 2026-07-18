/******************************************************************************
 *  Starter.cpp (for SPIKE )
 *  Created on: 2025/01/05
 *  Definition of the Class Starter
 *  Author: Kazuhiro Kawachi
 *  Modifier : Yuki Tsuchitoi
 *  Copyright (c) 2025 Embedded Technology Software Design Robot Contest
 *****************************************************************************/

#include "Starter.h"

#include <stdio.h>

/**
 * コンストラクタ
 * @param forceSensor フォースセンサ
 */
Starter::Starter(const spikeapi::ForceSensor& forceSensor) : mForceSensor(forceSensor), mIsStarted(false) {}

/**
 * 押下中か否か
 * @retval true  押下している
 * @retval false 押下していない
 */
bool Starter::isPushed()
{
#ifndef SPIKERT
    // シミュレータ: フォースセンサの代わりに、シミュレータの計測タイム
    // (athrill RXデータ領域 0x090F0000+500、GOの瞬間に0から増加するms値)を監視し、
    // カウントダウン終了(GO)と同時に自動スタートする。
    if(*(volatile unsigned int*)(0x090F0000 + 500) != 0) {
        mIsStarted = true;
    }
#endif
    if (mForceSensor.isTouched()) {
        mIsStarted = true;
    }
    //printf("Starter::isPushed(): %s\n", mIsStarted ? "true" : "false");
    return mIsStarted;
}
