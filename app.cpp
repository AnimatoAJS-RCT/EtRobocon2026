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
// tracer.ini is converted to this header by Makefile.inc at build time.
// Both the firmware and the simulator read the scenario from it.
#include "TracerConfig.h"
#include "Tracer.h"
#include "LineMonitor.h"
#include "LineTracer.h"
#include "ArmTracer.h"
#include "RotateTracer.h"
#include "ScenarioTracer.h"
#include "UltrasonicAlignTracer.h"
#include "UltrasonicDistanceLoggerTracer.h"
#include "UltrasonicProbeTracer.h"
#include "RallyRouteSolver.h"
#include "RallyTracer.h"
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
#include <sstream>

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
UltrasonicSensor gUltrasonicSensor(EPort::PORT_F);
Light gStatusLight;  // キャリブレーションの状態表示に使うステータスライト

// オブジェクトの定義
static LineMonitor* gLineMonitor;
static Walker* gWalker;
static LineTracer* gLineTracer;
static ArmTracer* gArmTracer;
static RotateTracer* gRotateTracer;
static ScenarioTracer* gScenarioTracer;
static UltrasonicAlignTracer* gUltrasonicAlignTracer;
static UltrasonicDistanceLoggerTracer* gUltrasonicDistanceLoggerTracer;
static UltrasonicProbeTracer* gUltrasonicProbeTracer;
static RallyTracer* gRallyTracer;
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
    if(dynamic_cast<const RotateTracer*>(tracer) != nullptr) {
        return "RotateTracer";
    }
    if(dynamic_cast<const UltrasonicAlignTracer*>(tracer) != nullptr) {
        return "UltrasonicAlignTracer";
    }
    if(dynamic_cast<const UltrasonicDistanceLoggerTracer*>(tracer) != nullptr) {
        return "UltrasonicDistanceLoggerTracer";
    }
    if(dynamic_cast<const UltrasonicProbeTracer*>(tracer) != nullptr) {
        return "UltrasonicProbeTracer";
    }
    if(dynamic_cast<const RallyTracer*>(tracer) != nullptr) {
        return "RallyTracer";
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

    // Neither the firmware nor the athrill simulator can read the host
    // workspace at runtime, so read the tracer.ini content embedded at build time.
    LOGI("embedded tracer.ini\n");
    std::istringstream configStream(kTracerIni);
    std::string line;
    if(!std::getline(configStream, line)) {
        LOGI("embedded tracer.ini is empty.\n");
        return;
    }
    while(line != "#end") {
        if(line.empty() || line[0] == '#') {
            if(!std::getline(configStream, line)) {
                break;
            }
            continue;
        }

        spl = split(line, " ");
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
            if(IS_LEFT_COURSE) {
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
            if(IS_LEFT_COURSE) {
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
                = new LineTracer(gLineMonitor, gWalker, targetBrightness, pwm, maxPwm, isLeftEdge, pidGain);
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
                LOGI("ArmTracer requires 2 params: ArmTracer <pwm> <target_angle_deg>\n");
                if(!std::getline(configStream, line)) {
                    break;
                }
                continue;
            }

            int armPwm = atoi(spl[1].c_str());
            int targetAngle = atoi(spl[2].c_str());
            LOGI("ArmTracer(%d, %ddeg): push\n", armPwm, targetAngle);
            gArmTracer = new ArmTracer(&gArmMotor, armPwm, targetAngle);
            gArmTracer->addStarter(gStarter);
            tracerList.push_back(gArmTracer);
        } else if(spl[0] == "RotateTracer") {
            if(result_size != 4) {
                LOGI("RotateTracer requires 3 params: RotateTracer <TURN_LEFT|TURN_RIGHT> "
                     "<angle_deg> <pwm>\n");
                if(!std::getline(configStream, line)) {
                    break;
                }
                continue;
            }

            int direction = 0;
            if(spl[1] == "TURN_RIGHT") {
                direction = 1;
            } else if(spl[1] == "TURN_LEFT") {
                direction = -1;
            } else {
                LOGI("RotateTracer direction must be TURN_LEFT or TURN_RIGHT: %s\n",
                     spl[1].c_str());
                if(!std::getline(configStream, line)) {
                    break;
                }
                continue;
            }

            int angleDeg = atoi(spl[2].c_str());
            int pwm = atoi(spl[3].c_str());
            if(angleDeg <= 0 || pwm <= 0 || pwm > 100) {
                LOGI("RotateTracer requires angle_deg > 0 and 0 < pwm <= 100\n");
                if(!std::getline(configStream, line)) {
                    break;
                }
                continue;
            }

            LOGI("RotateTracer(direction=%s, angle=%ddeg, pwm=%d): push\n",
                 spl[1].c_str(), angleDeg, pwm);
            gRotateTracer = new RotateTracer(gWalker, direction, angleDeg, pwm);
            gRotateTracer->addStarter(gStarter);
            tracerList.push_back(gRotateTracer);
        } else if(spl[0] == "UltrasonicAlignTracer") {
            if(result_size != 4) {
                LOGI("UltrasonicAlignTracer requires 3 params: <half_sweep_deg> <max_distance_mm> "
                     "<push_distance_mm>\n");
                if(!std::getline(configStream, line)) {
                    break;
                }
                continue;
            }

            int halfSweepAngleDeg = atoi(spl[1].c_str());
            int maxDistanceMm = atoi(spl[2].c_str());
            int pushDistanceMm = atoi(spl[3].c_str());
            if(gUltrasonicSensor.hasError()) {
                LOGI("UltrasonicAlignTracer skipped: ultrasonic sensor is unavailable on PORT_F\n");
            } else {
                 LOGI("UltrasonicAlignTracer(half=%ddeg, max=%dmm, push=%dmm): push\n",
                     halfSweepAngleDeg, maxDistanceMm, pushDistanceMm);
                gUltrasonicAlignTracer = new UltrasonicAlignTracer(
                    gWalker, &gUltrasonicSensor, halfSweepAngleDeg, maxDistanceMm,
                    pushDistanceMm);
                gUltrasonicAlignTracer->addStarter(gStarter);
                tracerList.push_back(gUltrasonicAlignTracer);
            }
        } else if(spl[0] == "RallyTracer") {
            if(result_size < 19) {
                LOGI("RallyTracer requires 18 params: <move_pwm> <turn_pwm> <start_x> <start_y> "
                     "<start_heading_deg> <lap_count> "
                     "<red_gx1> <red_gy1> <red_gx2> <red_gy2> "
                     "<blue_gx1> <blue_gy1> <blue_gx2> <blue_gy2> "
                     "<yellow_gx1> <yellow_gy1> <yellow_gx2> <yellow_gy2> "
                     "[marker_enable] [marker_reflection_threshold] "
                     "[marker_snap_window_deg] [marker_cooldown_ticks]\n");
                if(!std::getline(configStream, line)) {
                    break;
                }
                continue;
            }

            int movePwm = atoi(spl[1].c_str());
            int turnPwm = atoi(spl[2].c_str());
            QRPos startPos = {atoi(spl[3].c_str()), atoi(spl[4].c_str())};
            int startHeadingDeg = atoi(spl[5].c_str());
            int lapCount = atoi(spl[6].c_str());

            GateInfo gates;
            gates.red.gx1 = atoi(spl[7].c_str());
            gates.red.gy1 = atoi(spl[8].c_str());
            gates.red.gx2 = atoi(spl[9].c_str());
            gates.red.gy2 = atoi(spl[10].c_str());
            gates.blue.gx1 = atoi(spl[11].c_str());
            gates.blue.gy1 = atoi(spl[12].c_str());
            gates.blue.gx2 = atoi(spl[13].c_str());
            gates.blue.gy2 = atoi(spl[14].c_str());
            gates.yellow.gx1 = atoi(spl[15].c_str());
            gates.yellow.gy1 = atoi(spl[16].c_str());
            gates.yellow.gx2 = atoi(spl[17].c_str());
            gates.yellow.gy2 = atoi(spl[18].c_str());

            bool enableMarkerCorrection = result_size >= 20 ? (atoi(spl[19].c_str()) != 0) : false;
            int markerReflectionThreshold = result_size >= 21 ? atoi(spl[20].c_str()) : 20;
            int markerSnapWindowDeg = result_size >= 22 ? atoi(spl[21].c_str()) : 180;
            int markerCooldownTicks = result_size >= 23 ? atoi(spl[22].c_str()) : 25;

            if(IS_LEFT_COURSE) {
                auto mirrorQrX = [](int x) { return 5 - x; };
                auto mirrorGateX = [](int gx) { return 6 - gx; };
                auto mirrorHeading = [](int heading) {
                    int normalized = ((heading % 360) + 360) % 360;
                    return (180 - normalized + 360) % 360;
                };

                startPos.x = mirrorQrX(startPos.x);
                startHeadingDeg = mirrorHeading(startHeadingDeg);

                gates.red.gx1 = mirrorGateX(gates.red.gx1);
                gates.red.gx2 = mirrorGateX(gates.red.gx2);
                gates.blue.gx1 = mirrorGateX(gates.blue.gx1);
                gates.blue.gx2 = mirrorGateX(gates.blue.gx2);
                gates.yellow.gx1 = mirrorGateX(gates.yellow.gx1);
                gates.yellow.gx2 = mirrorGateX(gates.yellow.gx2);

                LOGI("RallyTracer mirrored for L-course: start=(%d,%d) heading=%d\n",
                     startPos.x, startPos.y, startHeadingDeg);
            }

            RallyRouteSolver::Config cfg;
            cfg.startPos = startPos;
            cfg.lapCount = lapCount;
            RallyRoute route = RallyRouteSolver::solve(gates, cfg);

              LOGI("RallyTracer(move=%d turn=%d start=(%d,%d) heading=%d lap=%d "
                  "R=(%d,%d)-(%d,%d) B=(%d,%d)-(%d,%d) Y=(%d,%d)-(%d,%d) "
                  "marker=%d thr=%d snap=%d cd=%d): push\n",
                 movePwm, turnPwm, startPos.x, startPos.y, startHeadingDeg, lapCount,
                 gates.red.gx1, gates.red.gy1, gates.red.gx2, gates.red.gy2,
                 gates.blue.gx1, gates.blue.gy1, gates.blue.gx2, gates.blue.gy2,
                  gates.yellow.gx1, gates.yellow.gy1, gates.yellow.gx2, gates.yellow.gy2,
                  enableMarkerCorrection ? 1 : 0, markerReflectionThreshold,
                  markerSnapWindowDeg, markerCooldownTicks);

            gRallyTracer = new RallyTracer(gWalker, route, movePwm, turnPwm, startPos,
                                       startHeadingDeg, &gColorSensor,
                                       enableMarkerCorrection,
                                       markerReflectionThreshold,
                                       markerSnapWindowDeg,
                                       markerCooldownTicks);
            gRallyTracer->addStarter(gStarter);
            tracerList.push_back(gRallyTracer);
        } else if(spl[0] == "UltrasonicDistanceLoggerTracer") {
            if(result_size != 2) {
                LOGI("UltrasonicDistanceLoggerTracer requires 1 param: <sample_count>\n");
                if(!std::getline(configStream, line)) {
                    break;
                }
                continue;
            }

            int sampleCount = atoi(spl[1].c_str());
            if(gUltrasonicSensor.hasError()) {
                LOGI("UltrasonicDistanceLoggerTracer skipped: ultrasonic sensor is unavailable on PORT_F\n");
            } else {
                LOGI("UltrasonicDistanceLoggerTracer(samples=%d): push\n", sampleCount);
                gUltrasonicDistanceLoggerTracer = new UltrasonicDistanceLoggerTracer(
                    gWalker, &gUltrasonicSensor, sampleCount);
                gUltrasonicDistanceLoggerTracer->addStarter(gStarter);
                tracerList.push_back(gUltrasonicDistanceLoggerTracer);
            }
        } else if(spl[0] == "UltrasonicProbeTracer") {
            if(result_size != 3 && result_size != 4) {
                LOGI("UltrasonicProbeTracer requires: <DISTL|DISTS|TRAW|ADRAW|ALL> "
                     "<sample_count> [sample_interval_ms]\n");
                if(!std::getline(configStream, line)) {
                    break;
                }
                continue;
            }

            const std::string& modeName = spl[1];
            int sampleCount = atoi(spl[2].c_str());
            int sampleIntervalMs = result_size == 4 ? atoi(spl[3].c_str()) : 100;
            if(!UltrasonicProbeTracer::isSupportedMode(modeName) || sampleCount <= 0
               || sampleIntervalMs <= 0) {
                LOGI("UltrasonicProbeTracer requires a supported mode, sample_count > 0, and "
                     "sample_interval_ms > 0\n");
            } else if(gUltrasonicSensor.hasError()) {
                LOGI("UltrasonicProbeTracer skipped: ultrasonic sensor is unavailable on PORT_F\n");
            } else {
                LOGI("UltrasonicProbeTracer(mode=%s, samples=%d, interval=%dms): push\n",
                     modeName.c_str(), sampleCount, sampleIntervalMs);
                gUltrasonicProbeTracer = new UltrasonicProbeTracer(
                    gWalker, modeName, sampleCount, sampleIntervalMs);
                gUltrasonicProbeTracer->addStarter(gStarter);
                tracerList.push_back(gUltrasonicProbeTracer);
            }
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
    if(!std::getline(configStream, line)) {
        break;
    }
    }

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
    gCalibrator = new Calibrator(gColorSensor, gForceSensor, gStatusLight);

    // 初期化完了通知（キャリブレーション開始前は ORANGE で待機中を示す）
    gStatusLight.turnOnColor(Light::EColor::ORANGE);
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

    // Course selection is completed by calibration, so create tracers afterward
    // to apply the selected left/right reversal to every scenario.
    generateTracerList();

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

    if(!gCalibratorWakeSent) {
        gCalibrator->run();

        if(gCalibrator->isFinished()) {
            gCalibratorWakeSent = true;
            ercd = wup_tsk(MAIN_TASK);
            LOGI("[CAL_TASK] wup_tsk(MAIN_TASK)=%d\n", ercd);
        }
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
