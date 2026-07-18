/******************************************************************************
 *  app.h (for SPIKE)
 *  Created on: 2025/01/05
 *  Definition of the Task main_task
 *  Author: Kazuhiro.Kawachi
 *  Modifier: Yuki Tsuchitoi
 *  Copyright (c) 2025 Embedded Technology Software Design Robot Contest
 *****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include "spikeapi.h"
#include "kernel.h"

#ifdef __cplusplus
#ifdef alignof
#undef alignof
#endif
#endif

#include <stdbool.h>

/*
 *  各タスクの優先度の定義
 */
#ifdef SPIKERT
#define MAIN_PRIORITY       5  /* メインタスク */
#define CALIBRATOR_PRIORITY 6
#define TRACER_PRIORITY     7
#define BT_SENDER_PRIORITY  8
#else
/* シミュレータ(athrill): APP_INIT_TASK(TMIN_TPRI+6)がヒープ初期化と
 * グローバルコンストラクタを実行するため、それより低い優先度にしないと
 * 最初の new でクラッシュしてリブートループになる */
#define MAIN_PRIORITY       (TMIN_APP_TPRI + 1)
#define CALIBRATOR_PRIORITY (TMIN_APP_TPRI + 2)
#define TRACER_PRIORITY     (TMIN_APP_TPRI + 3)
#endif

/*
 *  ターゲットに依存する可能性のある定数の定義
 */
#ifndef STACK_SIZE
#define STACK_SIZE      4096        /* タスクのスタックサイズ */
#endif /* STACK_SIZE */


extern bool IS_LEFT_COURSE; // Lコース

/*
 *  関数のプロトタイプ宣言
 */
#ifndef TOPPERS_MACRO_ONLY

extern void main_task(intptr_t exinf);
extern void calibrator_task(intptr_t exinf);
extern void tracer_task(intptr_t exinf);
extern void sender_task(intptr_t exinf);
extern void ev3_cyc_tracer(intptr_t exinf);

#endif /* TOPPERS_MACRO_ONLY */

#ifdef __cplusplus
}
#endif
