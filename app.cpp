/******************************************************************************
 *  app.cpp (for SPIKE)
 *  Created on: 2025/01/05
 *  Implementation of the Task main_task
 *  Author: Kazuhiro.Kawachi
 *  Modifier: Yuki Tsuchitoi
 *  Copyright (c) 2025 Embedded Technology Software Design Robot Contest
 *****************************************************************************/

#include "app.h"
#include "kernel_cfg.h"
#include "Tracer.h"
#include "LineMonitor.h"
#include "LineTracer.h"
#include "ArmTracer.h"
#include "ScenarioTracer.h"
#include "Walker.h"
#include "DistanceTerminator.h"
#include "ColorTerminator.h"
#include "Calibrator.h"
#include "Util.h"
#include "Log.h"

#include "Light.h"
#include "Button.h"
#include "Display.h"

#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <string>

#define SIZE_OF_ARRAY(array) (sizeof(array) / sizeof(array[0]))

// デストラクタ問題の回避
// https://github.com/ETrobocon/etroboEV3/wiki/problem_and_coping
#ifndef MAKE_RASPIKE
extern "C" void* __dso_handle = 0;
extern "C" void _fini(void) {}
#else
// RASPIKE環境では不要
#endif

bool IS_LEFT_COURSE = false;  // Rコース。デフォルトはRコース。キャリブレーションで変更される。

using namespace spikeapi;

// Device objects
// オブジェクトを静的に確保する
ColorSensor gColorSensor(EPort::PORT_E);
ForceSensor gForceSensor(EPort::PORT_D);
Motor gLeftWheel(EPort::PORT_B, Motor::EDirection::COUNTERCLOCKWISE, true);
Motor gRightWheel(EPort::PORT_A, Motor::EDirection::CLOCKWISE, true);
Motor gArmMotor(EPort::PORT_C, Motor::EDirection::CLOCKWISE, true);

// オブジェクトの定義
static LineMonitor* gLineMonitor;
static Walker* gWalker;
static LineTracer* gLineTracer;
static ArmTracer* gArmTracer;
static ScenarioTracer* gScenarioTracer;
static Starter* gStarter;
static DistanceTerminator* gDistanceTerminator;
static ColorTerminator* gColorTerminator;
static Calibrator* gCalibrator;
static bool gCalibratorWakeSent;

std::vector<Tracer*> tracerList;
int tracerListSize;

static const char* tracerTypeName(const Tracer* tracer)
{
    if(dynamic_cast<const ScenarioTracer*>(tracer) != nullptr) {
        return "ScenarioTracer";
    }
    if(dynamic_cast<const LineTracer*>(tracer) != nullptr) {
        return "LineTracer";
    }
    if(dynamic_cast<const ArmTracer*>(tracer) != nullptr) {
        return "ArmTracer";
    }

    return "UnknownTracer";
}

static void trimLineEnd(char* line)
{
    size_t len = strlen(line);
    while(len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
        line[len - 1] = '\0';
        len--;
    }
}

void generateTracerList()
{
    // iniファイル読み込み
    std::vector<std::string> spl;
    size_t result_size;

#ifndef MAKE_RASPIKE
    LOGI("sim\n");
    // シミュレーター環境でファイルを読み込めないため固定文字列で設定値を読み込む
    const std::string lines[] = { "ArmTracer -50 500",
                                  "ScenarioTracer 1000 50 50 RED",
                                  "ArmTracer 50 500",
                                  "#end" };
    int idx = 0;
    // strcpy((char*)spl, lines[idx]);
    while(lines[idx] != "#end") {
        LOGD("readini: %s\n", lines[idx].c_str());
        if(lines[idx][0] == '#') {
            idx++;
            continue;
        }

        spl = split(lines[idx], " ");
#else
    LOGI("notsim\n");
    char currentDir[512];
    if(getcwd(currentDir, sizeof(currentDir)) == nullptr) {
        LOGI("failed to get current directory.\n");
        return;
    }

    char iniPath[512];
    snprintf(iniPath, sizeof(iniPath), "%s/workspace/EtRobocon2026/tracer.ini", currentDir);

    LOGI("tracer.ini読み取り:%s\n", iniPath);
    FILE* file;
    file = fopen(iniPath, "r");  // ファイル読み込み
    if(file == nullptr) {
        LOGI("failed to open tracer.ini:%s\n", iniPath);
        return;
    }
    char line[512];

    // 1行ずつ値を読み取り使用
    if(fgets(line, sizeof(line), file) == nullptr) {
        LOGI("tracer.ini is empty:%s\n", iniPath);
        fclose(file);
        return;
    }
    trimLineEnd(line);
    while(strcmp(line, "#end") != 0) {
        // LOGD("readini: a%sa, %d\n", line, strcmp(line, "#end"));
        if(line[0] == '#') {
            if(fgets(line, sizeof(line), file) == nullptr) {
                break;
            }
            trimLineEnd(line);
            continue;
        }

        spl = split(line, " ");
#endif
        result_size = spl.size();
        for(size_t i = 0; i < result_size; ++i) {
            LOGD("%lu: %s\n", (unsigned long)i, spl[i].c_str());
        }
        LOGD("\n");

        if(spl[0] == "ScenarioTracer") {
            double targetDistance;
            int leftPwm, rightPwm;
            targetDistance = atof(spl[1].c_str());
            leftPwm = atof(spl[2].c_str());
            rightPwm = atof(spl[3].c_str());

            // Lコースの場合、左右のPWMを入れ替える
            if (IS_LEFT_COURSE) {
                int tmp = leftPwm;
                leftPwm = rightPwm;
                rightPwm = tmp;
            }
            gScenarioTracer = new ScenarioTracer(gWalker, leftPwm, rightPwm);
            gScenarioTracer->addStarter(gStarter);

            gDistanceTerminator = new DistanceTerminator(gWalker, targetDistance);
            gScenarioTracer->addTerminator(gDistanceTerminator);

            if(result_size >= 5) {
                eColor stopColor = BLACK;
                bool hasStopColor = true;
                if(spl[4] == "BLACK") {
                    stopColor = BLACK;
                } else if(spl[4] == "BLUE") {
                    stopColor = BLUE;
                } else if(spl[4] == "RED") {
                    stopColor = RED;
                } else if(spl[4] == "GREEN") {
                    stopColor = GREEN;
                } else if(spl[4] == "YELLOW") {
                    stopColor = YELLOW;
                } else {
                    hasStopColor = false;
                    LOGI("Unknown stopColor: %s\n", spl[4].c_str());
                }

                if(hasStopColor) {
                    gColorTerminator = new ColorTerminator(&gColorSensor, stopColor);
                    gScenarioTracer->addTerminator(gColorTerminator);
                }
            }
            tracerList.push_back(gScenarioTracer);

        } else if(spl[0] == "LineTracer") {
            // TODO:targetDistanceでのterminateを実装
            double targetDistance, p, i, d;
            int targetBrightness, pwm, maxPwm;
            bool isLeftEdge;
            PidGain* pidGain;
            targetDistance = atof(spl[1].c_str());
            gDistanceTerminator = new DistanceTerminator(gWalker, targetDistance);
            targetBrightness = atof(spl[2].c_str());
            pwm = atof(spl[3].c_str());
            maxPwm = atof(spl[4].c_str());
            isLeftEdge = (strcmp(spl[5].c_str(), "LEFT_EDGE") == 0);

            // Lコースの場合、エッジを反転させる
            if (IS_LEFT_COURSE) {
                isLeftEdge = !isLeftEdge;
            }

            p = atof(spl[6].c_str());
            i = atof(spl[7].c_str());
            d = atof(spl[8].c_str());
            LOGI("LineTracer(%lf, %d, %d, %d, %s, PidGain(%lf, %lf, %lf)): push\n",
                 targetDistance, targetBrightness, pwm, maxPwm,
                 isLeftEdge ? "LEFT_EDGE" : "RIGHT_EDGE", p, i, d);
            pidGain = new PidGain(p, i, d);
            gLineTracer
                = new LineTracer(gLineMonitor, gWalker, targetBrightness, pwm, isLeftEdge, pidGain);
            gLineTracer->addStarter(gStarter);
            gLineTracer->addTerminator(gDistanceTerminator);
            if(result_size >= 10) {
                // 色で停止
                eColor stopColor = BLUE;
                bool hasStopColor = true;
                if(spl[9] == "BLUE") {
                    stopColor = BLUE;
                } else if(spl[9] == "RED") {
                    stopColor = RED;
                } else if(spl[9] == "GREEN") {
                    stopColor = GREEN;
                } else if(spl[9] == "YELLOW") {
                    stopColor = YELLOW;
                } else {
                    hasStopColor = false;
                    LOGI("Unknown stopColor: %s\n", spl[9].c_str());
                }

                if(hasStopColor) {
                    LOGI("stopColor: %s\n", colorToString(stopColor));
                    gColorTerminator = new ColorTerminator(&gColorSensor, stopColor);
                    gLineTracer->addTerminator(gColorTerminator);
                }
            }
            tracerList.push_back(gLineTracer);
        } else if(spl[0] == "ArmTracer") {
            if(result_size < 3) {
                LOGI("ArmTracer requires 2 params: ArmTracer <pwm> <duration_ms>\n");
#ifndef MAKE_RASPIKE
                idx++;
#else
                if(fgets(line, sizeof(line), file) == nullptr) {
                    break;
                }
                trimLineEnd(line);
#endif
                continue;
            }

            int armPwm = atoi(spl[1].c_str());
            int durationMs = atoi(spl[2].c_str());
            LOGI("ArmTracer(%d, %d): push\n", armPwm, durationMs);
            gArmTracer = new ArmTracer(&gArmMotor, armPwm, durationMs);
            gArmTracer->addStarter(gStarter);
            tracerList.push_back(gArmTracer);
        }
        // TODO:難所トレーサーの実装
        //        else if (spl[0] == "RotateTracer")
        //        {
        //            int direction, degree, pwm;
        //            if (spl[1] == "TURN_RIGHT")
        //            { // ^ (!IS_LEFT_COURSE)){
        //                direction = 1;
        //            }
        //            else
        //            {
        //                direction = -1;
        //            }
        //            if (spl[1] == "TURN_LEFT")
        //            { // ^ (!IS_LEFT_COURSE)){
        //                direction = -1;
        //            }
        //            else
        //            {
        //                direction = 1;
        //            }
        //            degree = atof(spl[2].c_str());
        //            if (result_size >= 4)
        //            {
        //                pwm = atof(spl[3].c_str());
        //                LOGD("RotateTracer(%d, %d, %d): push\n", direction, degree, pwm);
        //                courseList.push_back(new RotateTracer(direction, degree, pwm));
        //            }
        //            else
        //            {
        //                LOGD("RotateTracer(%d, %d): push\n", direction, degree);
        //                courseList.push_back(new RotateTracer(direction, degree));
        //            }
        //        }
        //        else if (spl[0] == "DifficultScenarioTracer")
        //        {
        //            int direction, maxTimer, maxCnt;
        //            if ((spl[1] == "TURN_RIGHT"))
        //            { // ^ (!IS_LEFT_COURSE)){
        //                direction = 1;
        //            }
        //            else
        //            {
        //                direction = -1;
        //            }
        //            if ((spl[1] == "TURN_LEFT"))
        //            { // ^ (!IS_LEFT_COURSE)){
        //                direction = -1;
        //            }
        //            else
        //            {
        //                direction = 1;
        //            }
        //
        //            if (result_size == 3)
        //            {
        //                maxTimer = atof(spl[2].c_str());
        //                LOGD("DifficultScenarioTracer(%d, %d): push\n", direction, maxTimer);
        //                courseList.push_back(new DifficultScenarioTracer(direction, maxTimer));
        //            }
        //            else if (result_size == 4)
        //            {
        //                maxTimer = atof(spl[2].c_str());
        //                maxCnt = atof(spl[3].c_str());
        //                LOGD("DifficultScenarioTracer(%d, %d, %d): push\n", direction, maxTimer,
        //                maxCnt); courseList.push_back(new DifficultScenarioTracer(direction,
        //                maxTimer, maxCnt));
        //            }
        //            else
        //            {
        //                LOGD("DifficultScenarioTracer(%d): push\n", direction);
        //                courseList.push_back(new DifficultScenarioTracer(direction));
        //            }
        //        }
        else {
            // Tracer名にマッチしなかったらなにもしない
        }
#ifndef MAKE_RASPIKE
        idx++;
#else
    if(fgets(line, sizeof(line), file) == nullptr) {
        break;
    }
    trimLineEnd(line);
#endif
    }
#ifdef MAKE_RASPIKE
    fclose(file);  // ファイルを閉じる
#endif

    tracerListSize = tracerList.size();
}

/**
 * システム生成
 */
static void user_system_create()
{

    // コース設定
    // IS_LEFT_COURSE はキャリブレーション時に設定する

    // オブジェクトの作成
    gWalker = new Walker(gLeftWheel, gRightWheel);
    gLineMonitor = new LineMonitor(gColorSensor);
    gStarter = new Starter(gForceSensor);
    gCalibrator = new Calibrator(gColorSensor, gForceSensor);

    generateTracerList();

    // 初期化完了通知
    Light light;
    light.turnOnColor(Light::EColor::ORANGE);
}

/**
 * システム破棄
 */
static void user_system_destroy()
{
    gLeftWheel.stop();
    gRightWheel.stop();
    gArmMotor.stop();
    gLeftWheel.resetCount();
    gRightWheel.resetCount();
    gArmMotor.resetCount();

    delete gLineTracer;
    delete gStarter;
    delete gLineMonitor;
    delete gWalker;
    delete gCalibrator;
}

/**
 * メインタスク
 */
void main_task(intptr_t unused)
{
    ER ercd;
    user_system_create();  // センサやモータの初期化処理
    LOGI("[MAIN] user_system_create done\n");
    gCalibratorWakeSent = false;

    // 周期ハンドラ開始
    ercd = sta_cyc(CYC_CALIBRATOR);
    LOGI("[MAIN] sta_cyc(CYC_CALIBRATOR)=%d\n", ercd);

    LOGI("[MAIN] waiting calibration completion...\n");
    ercd = slp_tsk();  // キャリブレーション完了まで待つ
    LOGI("[MAIN] woke from calibration wait, slp_tsk()=%d\n", ercd);

    // 周期ハンドラ停止
    ercd = stp_cyc(CYC_CALIBRATOR);
    LOGI("[MAIN] stp_cyc(CYC_CALIBRATOR)=%d\n", ercd);

    int black = gCalibrator->getBlack();
    int white = gCalibrator->getWhite();

    // 各LineTracerの目標輝度をキャリブレーション結果に基づいて補正する
    for(auto tracer : tracerList) {
        LineTracer* lineTracer = dynamic_cast<LineTracer*>(tracer);
        if(lineTracer != nullptr) {
            int normalizedTarget = lineTracer->getNormalizedTargetBrightness();
            int scaledTarget = black + (white - black) * normalizedTarget / 100;
            lineTracer->setTargetBrightness(scaledTarget);
            LOGI("[MAIN] LineTracer target: normalized=%d scaled=%d (black=%d white=%d)\n",
                 normalizedTarget, scaledTarget, black, white);
        }
    }

    // 周期ハンドラ開始
    ercd = sta_cyc(CYC_TRACER);
    LOGI("[MAIN] sta_cyc(CYC_TRACER)=%d\n", ercd);

    LOGI("[MAIN] waiting tracer completion or left button...\n");
    ercd = slp_tsk();  // トレース完了 or レフトボタン押下まで待つ
    LOGI("[MAIN] woke from tracer wait, slp_tsk()=%d\n", ercd);

    // 周期ハンドラ停止
    ercd = stp_cyc(CYC_TRACER);
    LOGI("[MAIN] stp_cyc(CYC_TRACER)=%d\n", ercd);

    user_system_destroy();  // 終了処理

    ext_tsk();
}

/**
 * キャリブレーション
 */
void calibrator_task(intptr_t exinf)
{
    ER ercd;
    (void)exinf;
    LOGD("[CAL_TASK] start\n");
    gCalibrator->run();
    LOGI("[CAL_TASK] finished calibrator run\n");

    if(!gCalibratorWakeSent && gCalibrator->isFinished()) {
        gCalibratorWakeSent = true;
        ercd = wup_tsk(MAIN_TASK);
        LOGI("[CAL_TASK] wup_tsk(MAIN_TASK)=%d\n", ercd);
    } else {
        LOGD("[CAL_TASK] skip wake (already sent or not finished)\n");
    }

    ext_tsk();
}

/**
 * ライントレースタスク
 */
void tracer_task(intptr_t exinf)
{
    static int traceLogCounter = 0;
    static int lastLoggedIndex = -1;
    ER ercd;
    Button button;

    if(button.isLeftPressed()) {
        ercd = wup_tsk(MAIN_TASK);  // レフトボタン押下
        LOGI("[TRACER_TASK] left button: wup_tsk(MAIN_TASK)=%d\n", ercd);
    } else {
        if(!tracerList.empty() && tracerList.front() != nullptr
           && tracerList.front()->isTerminated()) {
            LOGI("remove\n");
            Tracer* tracer = tracerList.front();
            tracerList.erase(tracerList.begin());
            delete tracer;

            if(!tracerList.empty() && tracerList.front() != nullptr) {
                int nextIndex = tracerListSize - tracerList.size() + 1;
                LOGI("[TRACER_TASK] next tracer: %d/%d %s\n", nextIndex, tracerListSize,
                     tracerTypeName(tracerList.front()));
            } else {
                LOGI("[TRACER_TASK] no next tracer\n");
            }
        }

        if(tracerList.empty()) {
            ercd = wup_tsk(MAIN_TASK);
            LOGI("[TRACER_TASK] tracerList empty: wup_tsk(MAIN_TASK)=%d\n", ercd);
            ext_tsk();
        }

        if(!tracerList.empty() && tracerList.front() != nullptr) {
            int currentIndex = tracerListSize - tracerList.size() + 1;
            if(lastLoggedIndex != currentIndex || (traceLogCounter % 100) == 0) {
                LOGD("[TRACER_TASK] tracerList: %d/%d\n", currentIndex, tracerListSize);
                lastLoggedIndex = currentIndex;
            }
            traceLogCounter++;
            tracerList.front()->run();
        }
    }

    ext_tsk();
}
