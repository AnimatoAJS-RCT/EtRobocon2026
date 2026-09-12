#include "BottleDeliveryTracer.h"
#include "Log.h"
#include "Util.h"
#include <libcpp/spike/Clock.h>
#include <cstdlib>

/*
 * BottleDeliveryTracer の行動フロー:
 * 1. 初期化後、ライン・トレースで走行し、黒色を検出したら停止する。
 * 2. アームを上げ、色センサーで搬送するボトルの色を判定する。
 * 3. １秒後もボトルの色を判定できなかった場合一センチ進み再度ボトルの色判定に戻る
 * 4. アームを下げてボトルを保持する。
 * 5. ライン・トレースを再開し、青線を検出した回数が規定値に達するまで進む。
 *
 *    -
 * 最初の青色を踏んだらライントレースを止め、150ミリ直進しながら毎秒3度曲がる。その後ライントレースを再開する
 *    - その後1100ミリの間ライントレースする
 *    - その後200ミリの間ライントレースを続ける
 *    - その後通常の青線カウントへ戻る
 * - 情行動フロー中合計で青ボトルの場合は3回、赤ボトルの場合は4回青色を検出する。
 * 6. 初期エッジ側に応じて、納品エリア側へ90度回転する。
 * 7. 前進して納品場所の色を検出したら停止し、その前進距離を記録する。
 * 8. 納品場所から記録した距離と同じ距離だけ後退する。
 * 9. 納品場所へ向かったときと同じ向きに90度回転する。
 * 10. 帰路のライン・トレースを開始する。
 * 11. 帰路では基準エッジを反転してライン・トレースを継続する。
 */

// ボトル搬送ミッションの基本パラメータ。
static const int ARM_POWER = 90;                           // アームを上下させるときのモーター出力
static const int ARM_MOVE_ANGLE = 120;                     // アームを動かす目標角度
static const int BLUE_LINE_TOUCH_TARGET_YELLOW = 2;        // 黄ボトル時に必要な青線踏破回数
static const int BLUE_LINE_TOUCH_TARGET_BLUE = 3;          // 青ボトル時に必要な青線踏破回数
static const int BLUE_LINE_TOUCH_TARGET_RED = 4;           // 赤ボトル時に必要な青線踏破回数
static const int FIRST_BLUE_FORWARD_DISTANCE_MM = 200;     // 初回青線後に直進する距離(mm)
static const int BLUE_LINE_TRACE_DISTANCE_MM = 1050;       // ライントレースする距離(mm)
static const int BLUE_LINE_FINAL_TRACE_DISTANCE_MM = 200;  // 追加でライントレースする距離(mm)
static const int FIRST_BLUE_TURN_PWM = 12;          // 初回青線後の旋回量（3度/秒相当の校正値）
static const double STRONG_TRACE_PID_FACTOR = 2.0;  // 強補正区間のPIDゲイン倍率
static const int TURN_90_COUNT = 300;               // 90度回転とみなす左右輪の回転差
static const int CONTROL_CYCLE_MS = 10;             // run()が呼ばれる周期の想定値
static const int COLOR_CHECK_TIMEOUT_CYCLES = 1000 / CONTROL_CYCLE_MS;  // 色判定を待つ時間（1秒）
static const double COLOR_CHECK_RETRY_DISTANCE_MM = 10.0;     // 色判定失敗時に前進する距離(mm)
static const double COLOR_CHECK_BACKWARD_DISTANCE_MM = 10.0;  // 色判定後に後退する距離(mm)
static const double TIRE_DIAMETER_MM = 55.0;
static const double PI = 3.1415926535;

BottleDeliveryTracer::BottleDeliveryTracer(Walker* walker, LineMonitor* lineMonitor,
                                           spikeapi::UltrasonicSensor*,
                                           spikeapi::ColorSensor* colorSensor,
                                           spikeapi::Motor* armMotor, double, int targetBrightness,
                                           int pwm, int maxPwm, bool isLeftEdge, double kp,
                                           double ki, double kd)
  : mWalker(walker),
    mLineMonitor(lineMonitor),
    mColorSensor(colorSensor),
    mArmMotor(armMotor),
    mTargetBrightness(targetBrightness),
    mPwm(pwm),
    mMaxPwm(maxPwm),
    mIsLeftEdge(isLeftEdge),
    mPidGain(new PidGain(kp, ki, kd)),
    mRturnPidGain(new PidGain(0.6, 0.01, 0.017)),
    mStrongTracePidGain(new PidGain(kp * STRONG_TRACE_PID_FACTOR, ki, kd)),
    mLineTracer(
        new LineTracer(lineMonitor, walker, targetBrightness, pwm, maxPwm, isLeftEdge, mPidGain)),
    mFastLineTracer(new LineTracer(lineMonitor, walker, targetBrightness, pwm * 2, maxPwm,
                                   isLeftEdge, mPidGain)),
    mStrongTraceLineTracer(new LineTracer(lineMonitor, walker, targetBrightness, pwm * 0.6, maxPwm,
                                          isLeftEdge, mStrongTracePidGain)),
    mReturnLineTracer(new LineTracer(lineMonitor, walker, 55, 80, 100, isLeftEdge, mRturnPidGain)),
    mStage(STAGE_APPROACH_BOTTLE),
    mStageInitialized(false),
    mArmStartCount(0),
    mStartLeftCount(0),
    mStartRightCount(0),
    mDeliveryStartLeftCount(0),
    mDeliveryStartRightCount(0),
    mBlueLineTouchCount(0),
    mBlueLineTouchCountReturn(0),
    mColorCheckCycles(0),
    mDetectedBlueBottle(false),
    mDetectedRedBottle(false),
    mDetectedYellowBottle(false),
    mColorRetryDistanceTerminator(new DistanceTerminator(walker, COLOR_CHECK_RETRY_DISTANCE_MM)),
    mColorBackDistanceTerminator(new DistanceTerminator(walker, COLOR_CHECK_BACKWARD_DISTANCE_MM)),
    mFirstBlueForwardTerminator(new DistanceTerminator(walker, FIRST_BLUE_FORWARD_DISTANCE_MM)),
    mBlueLineTraceTerminator(new DistanceTerminator(walker, BLUE_LINE_TRACE_DISTANCE_MM)),
    mBlueLineFinalTraceTerminator(
        new DistanceTerminator(walker, BLUE_LINE_FINAL_TRACE_DISTANCE_MM)),
    mDeliveryBackDistanceTerminator(new DistanceTerminator(walker, 0.0)),
    mBlueConsecutiveCount(0),
    mLastDetectedColor(BLACK)
{
    mState = UNDEFINED;
}

BottleDeliveryTracer::~BottleDeliveryTracer()
{
    delete mLineTracer;
    delete mFastLineTracer;
    delete mStrongTraceLineTracer;
    delete mReturnLineTracer;
    delete mStrongTracePidGain;
    delete mColorRetryDistanceTerminator;
    delete mColorBackDistanceTerminator;
    delete mFirstBlueForwardTerminator;
    delete mBlueLineTraceTerminator;
    delete mBlueLineFinalTraceTerminator;
    delete mDeliveryBackDistanceTerminator;
}

void BottleDeliveryTracer::run()
{
    switch(mState) {
        case UNDEFINED:
            // 初期化直後にそのまま走行開始する。
            LOGI("[BOTTLE] ミッションを初期化しました\n");
            // ミッションに登録された距離・色などの終了条件を初期化する。
            for(auto terminator : mTerminatorList) {
                terminator->init();
            }
            // 最初の走行段階を設定し、次回周期から実際の走行へ進む。
            mStage = STAGE_APPROACH_BOTTLE;
            mStageInitialized = false;
            mState = WALKING;
            return;
        case WALKING:
            switch(mStage) {
                case STAGE_APPROACH_BOTTLE: {
                    // 行動フロー開始。青色を検出して黒色になるまでの走行へ移る。
                    LOGI("[BOTTLE] 接近開始: 黒線までライントレース\n");
                    mStage = STAGE_FORWARD_TO_BLACK_BEFORE_COLOR_CHECK;
                    mStageInitialized = false;
                    break;
                }
                case STAGE_FORWARD_TO_BLACK_BEFORE_COLOR_CHECK: {
                    // 黒色を検出するまで進み続ける。
                    if(!mStageInitialized) {
                        // この段階に入った最初の周期だけ、青色の連続検出回数をリセットする。
                        mStageInitialized = true;
                        mBlueConsecutiveCount = 0;
                        LOGI("[BOTTLE] 前進ライントレースを開始\n");
                    }
                    // 通常速度でラインを追従しながら、色センサーで黒色を確認する。
                    mLineTracer->run();
                    eColor currentColor = getDetectedColorContinuous();
                    if(currentColor == BLACK) {
                        // 黒色に到達したので停止し、ボトル色の確認へ進む。
                        LOGI("[CAL] シミュレータ: 色判定前に黒を検出\n");
                        mWalker->stop();
                        mStage = STAGE_ARM_UP;
                        mStageInitialized = false;
                    }
                    break;
                }
                case STAGE_ARM_UP: {
                    // ボトルを掴むためにアームを上げる。
                    if(!mStageInitialized) {
                        // アームの現在位置を記録し、この段階の動作を開始する。
                        LOGI("[BOTTLE] アームを上げます\n");
                        mArmStartCount = mArmMotor->getCount();
                        mStageInitialized = true;
                        mArmMotor->setPower(-ARM_POWER);
                    }
                    if(std::abs(mArmMotor->getCount() - mArmStartCount) >= ARM_MOVE_ANGLE
                       || mArmMotor->isStalled()) {
                        // 目標角度またはストールに達したため、アームを安全に停止する。
                        mArmMotor->brake();
                        LOGI("[BOTTLE] アームの上げ動作が完了しました%s\n",
                             mArmMotor->isStalled() ? " (stalled)" : "");
                        mStage = STAGE_COLOR_CHECK;
                        mStageInitialized = false;
                    }
                    break;
                }
                case STAGE_COLOR_CHECK: {
                    // 1秒待っても色を判定できなければ、1cm前進してから再判定する。
                    if(!mStageInitialized) {
                        // 色判定の待ち時間を新たに数え始める。
                        mStageInitialized = true;
                        mColorCheckCycles = 0;
                        LOGI("[BOTTLE] ボトル色の確認を開始\n");
                    }
                    eColor detectedColor = getDetectedColor();
                    if(detectedColor == OTHER || detectedColor == BLACK) {
                        // 色を確定できない間は、タイムアウトまで同じ段階で待機する。
                        mColorCheckCycles++;
                        if(mColorCheckCycles >= COLOR_CHECK_TIMEOUT_CYCLES) {
                            // タイムアウトしたため、1cm前進してセンサー位置を変える。
                            LOGI(
                                "[BOTTLE] "
                                "ボトル色の確認がタイムアウトしたため、10mm前進して再確認します\n");
                            mColorRetryDistanceTerminator->init();
                            mStage = STAGE_FORWARD_1CM_FOR_COLOR_RETRY;
                            mStageInitialized = false;
                        }
                        break;
                    }
                    mDetectedBlueBottle = (detectedColor == BLUE);
                    mDetectedRedBottle = (detectedColor == RED);
                    mDetectedYellowBottle = (detectedColor == YELLOW);

                    // 検出色をボトル色として保存し、アームを下げる段階へ移る。
                    LOGI("[CAL] シミュレータ: ボトル色を認識しました: %s\n",
                         mDetectedBlueBottle ? "blue" : "red");
                    // 色を確定した位置から1cm後退してから、ボトルを保持する。
                    mStartLeftCount = mWalker->getLeftCount();
                    mStartRightCount = mWalker->getRightCount();
                    mStage = STAGE_BACKWARD_1CM_AFTER_COLOR_CHECK;
                    mStageInitialized = false;
                    break;
                }
                case STAGE_FORWARD_1CM_FOR_COLOR_RETRY: {
                    // ボトルの色を再判定するため、1cmだけ前進する。
                    if(!mStageInitialized) {
                        // リトライ走行の開始状態を記録する。
                        mStageInitialized = true;
                        LOGI("[BOTTLE] 色再確認のため前進を開始\n");
                    }
                    mWalker->setPwm(mPwm, mPwm);
                    // ラインを使わず、左右輪を同じ出力で1cm前進させる。
                    mWalker->run();
                    if(mColorRetryDistanceTerminator->isToBeTerminate()) {
                        // 1cm進んだため停止し、もう一度ボトル色を判定する。
                        mWalker->stop();
                        LOGI("[BOTTLE] 色再確認の前進が完了\n");
                        mStage = STAGE_COLOR_CHECK;
                        mStageInitialized = false;
                    }
                    break;
                }
                case STAGE_BACKWARD_1CM_AFTER_COLOR_CHECK: {
                    // 色判定後の位置を調整するため、ラインを使わず1cm後退する。
                    if(!mStageInitialized) {
                        mStageInitialized = true;
                        mColorBackDistanceTerminator->init();
                        LOGI("[BOTTLE] アーム下降前の10mm後退を開始\n");
                    }
                    mWalker->setPwm(-mPwm, -mPwm);
                    mWalker->run();
                    if(mColorBackDistanceTerminator->isToBeTerminate()) {
                        // 1cm後退したため停止し、アーム下降へ進む。
                        mWalker->stop();
                        LOGI("[BOTTLE] 10mm後退完了; アーム下降を開始\n");
                        mStage = STAGE_ARM_DOWN;
                        mStageInitialized = false;
                    }
                    break;
                }
                case STAGE_ARM_DOWN: {
                    // 掴んだボトルを保持したまま、青線踏破判定へ移る。
                    if(!mStageInitialized) {
                        // アームの現在位置を記録し、ボトルを保持する方向へ動かす。
                        mArmStartCount = mArmMotor->getCount();
                        mStageInitialized = true;
                        LOGI("[BOTTLE] アームを下ろします\n");
                        mArmMotor->setPower(ARM_POWER);
                    }
                    if(std::abs(mArmMotor->getCount() - mArmStartCount) >= ARM_MOVE_ANGLE
                       || mArmMotor->isStalled()) {
                        // アーム動作が完了したため停止し、青線のカウントを開始する。
                        mArmMotor->brake();
                        spikeapi::Clock clock;
                        clock.sleep(500 * 1000);
                        LOGI("[BOTTLE] アームの下降が完了; 青線カウントを開始\n");
                        mStage = STAGE_BLUE_LINE_COUNT;
                        mStageInitialized = false;
                        mBlueLineTouchCount = 0;
                    }
                    break;
                }
                case STAGE_BLUE_LINE_COUNT: {
                    // 青線を踏んだ回数で終了条件を判定し、指定回数に達したら回転へ進む。
                    if(!mStageInitialized) {
                        // 青線カウントと、初回青線後の特別走行用の状態を初期化する。
                        mStageInitialized = true;
                        mBlueConsecutiveCount = 0;
                        mLastDetectedColor = BLACK;
                        LOGI("[BOTTLE] 青線カウントを開始; 目標=%d\n",
                             mDetectedBlueBottle ? BLUE_LINE_TOUCH_TARGET_BLUE
                                                 : BLUE_LINE_TOUCH_TARGET_RED);
                    }
                    eColor onBlueLine = getDetectedColorContinuous();
                    // 青色の立ち上がりだけを数え、同じ青線を複数回カウントしない。
                    if(onBlueLine == BLUE && mLastDetectedColor != BLUE) {
                        mBlueLineTouchCount++;
                        LOGI("[CAL] シミュレータ: 青線に触れました (%d)\n", mBlueLineTouchCount);
                        mLastDetectedColor = onBlueLine;
                        if(mBlueLineTouchCount == 1) {
                            // 初回青線後だけ特別走行へ移り、距離計測を開始する。
                            mFirstBlueForwardTerminator->init();
                            mStage = STAGE_FIRST_BLUE_FORWARD;
                            mStageInitialized = false;
                            break;
                        }
                    }
                    mLastDetectedColor = onBlueLine;
                    // 特別走行中でない青線区間は通常のライン・トレースを続ける。
                    mLineTracer->run();

                    const int blueLineTouchTarget
                        = mDetectedBlueBottle     ? BLUE_LINE_TOUCH_TARGET_BLUE
                          : mDetectedYellowBottle ? BLUE_LINE_TOUCH_TARGET_YELLOW
                                                  : BLUE_LINE_TOUCH_TARGET_RED;

                    if(mBlueLineTouchCount >= blueLineTouchTarget) {
                        // 必要回数の青線を踏破したため、納品エリア側へ旋回する。
                        mWalker->stop();
                        LOGI("[BOTTLE] 青線の目標回数に到達; 納品エリアへ回転\n");
                        mStage = STAGE_TURN_TO_TARGET;
                        mStageInitialized = false;
                    }
                    break;
                }
                case STAGE_FIRST_BLUE_FORWARD: {
                    if(!mStageInitialized) {
                        // 初回青線後はラインを見ず、指定距離の緩い旋回走行を開始する。
                        mStageInitialized = true;
                        LOGI("[BOTTLE] 初回青線後の前進を開始: 150mm\n");
                    }
                    int leftPwm = mPwm;
                    int rightPwm = mPwm;
                    if(mIsLeftEdge) {
                        // 左エッジ走行では左へ寄せる向きに左右PWM差を付ける。
                        leftPwm -= FIRST_BLUE_TURN_PWM;
                        rightPwm += FIRST_BLUE_TURN_PWM;
                    } else {
                        // 右エッジ走行では右へ寄せる向きに左右PWM差を付ける。
                        leftPwm += FIRST_BLUE_TURN_PWM;
                        rightPwm -= FIRST_BLUE_TURN_PWM;
                    }
                    mWalker->setPwm(leftPwm, rightPwm);
                    mWalker->run();
                    if(mFirstBlueForwardTerminator->isToBeTerminate()) {
                        // 150mm進んだため停止し、1100mmのライントレースへ切り替える。
                        mWalker->stop();
                        mBlueLineTraceTerminator->init();
                        LOGI("[BOTTLE] 初回青線後の前進完了; 1100mmのライントレースを開始\n");
                        mStage = STAGE_BLUE_LINE_HIGH_SPEED;
                        mStageInitialized = false;
                    }
                    break;
                }
                case STAGE_BLUE_LINE_HIGH_SPEED: {
                    if(!mStageInitialized) {
                        // 1100mmのライントレース区間の開始を記録する。
                        mStageInitialized = true;
                        LOGI("[BOTTLE] 青線追従を開始: 1100mm\n");
                    }
                    mFastLineTracer->run();
                    if(mBlueLineTraceTerminator->isToBeTerminate()) {
                        // 1100mm走り終えたため、追加のライントレースへ切り替える。
                        mWalker->stop();
                        mBlueLineFinalTraceTerminator->init();
                        LOGI("[BOTTLE] 1100mm区間完了; 追加ライントレースを開始\n");
                        mStage = STAGE_BLUE_LINE_STRONG_TRACE;
                        mStageInitialized = false;
                    }
                    break;
                }
                case STAGE_BLUE_LINE_STRONG_TRACE: {
                    if(!mStageInitialized) {
                        // 200mmの追加ライントレース区間の開始を記録する。
                        mStageInitialized = true;
                        LOGI("[BOTTLE] 追加ライントレースを開始: 200mm\n");
                    }
                    mStrongTraceLineTracer->run();
                    if(mBlueLineFinalTraceTerminator->isToBeTerminate()) {
                        // 200mm走り終えたため、通常の青線カウントへ戻る。
                        mWalker->stop();
                        LOGI("[BOTTLE] 追加ライントレース完了; 青線カウントを再開\n");
                        mStage = STAGE_BLUE_LINE_COUNT;
                        mStageInitialized = false;
                    }
                    break;
                }
                case STAGE_TURN_TO_TARGET: {
                    // 初期エッジ側に応じて、対象エリア側へ90度回転する。
                    if(!mStageInitialized) {
                        // 左右輪の開始角度を記録し、納品エリア方向への旋回を開始する。
                        mStartLeftCount = mWalker->getLeftCount();
                        mStartRightCount = mWalker->getRightCount();
                        mStageInitialized = true;
                        LOGI("[BOTTLE] 納品エリア側へ回転を開始\n");
                    }
                    if(mIsLeftEdge) {
                        // 左エッジ時の納品エリア方向へ旋回する。
                        mWalker->setPwm(mPwm, -mPwm);
                    } else {
                        // 右エッジ時の納品エリア方向へ旋回する。
                        mWalker->setPwm(-mPwm, mPwm);
                    }
                    mWalker->run();

                    int delta = std::abs((mWalker->getLeftCount() - mStartLeftCount)
                                         - (mWalker->getRightCount() - mStartRightCount));
                    LOGI("[move] 回転量の差分 (%d)\n", delta);
                    LOGI("[move] delta=%d / target=%d\n", delta, TURN_90_COUNT);
                    if(delta >= TURN_90_COUNT) {
                        // 左右輪の回転差が90度分に達したため、前進して色を探す。
                        mWalker->stop();
                        LOGI("[BOTTLE] 納品エリア側への回転完了\n");
                        mStage = STAGE_FORWARD_TO_COLOR;
                        mStageInitialized = false;
                    }
                    break;
                }
                case STAGE_FORWARD_TO_COLOR: {
                    // 納品場所の色を検出するまで前進する。
                    if(!mStageInitialized) {
                        // 後退距離を求めるため、納品場所の探索開始位置を記録する。
                        mDeliveryStartLeftCount = mWalker->getLeftCount();
                        mDeliveryStartRightCount = mWalker->getRightCount();
                        mStageInitialized = true;
                        LOGI("[BOTTLE] 納品場所の探索を開始\n");
                    }
                    mWalker->setPwm(mPwm, mPwm);
                    mWalker->run();
                    eColor targetDetected = getDetectedColor();
                    if(targetDetected != WHITE && targetDetected != BLACK
                       && targetDetected != OTHER) {
                        // 白・黒・未確定以外の色を納品場所として受け付ける。
                        mDetectedTargetColor = (targetDetected == RED)      ? COLOR_RED
                                               : (targetDetected == YELLOW) ? COLOR_YELLOW
                                                                            : COLOR_UNKNOWN;
                        LOGI("[CAL] シミュレータ: 納品場所の色を検出: %d\n", mDetectedTargetColor);
                        mWalker->stop();
                        double wheelCircumference = TIRE_DIAMETER_MM * PI;
                        double leftDistance
                            = std::abs(mWalker->getLeftCount() - mDeliveryStartLeftCount)
                              * wheelCircumference / 360.0;
                        double rightDistance
                            = std::abs(mWalker->getRightCount() - mDeliveryStartRightCount)
                              * wheelCircumference / 360.0;
                        double deliveryDistance = (leftDistance + rightDistance) / 2.0;
                        mDeliveryBackDistanceTerminator->setTargetDistance(deliveryDistance);
                        mDeliveryBackDistanceTerminator->init();
                        LOGI("[BOTTLE] 納品場所までの距離=%fmm; 同じ距離を後退します\n",
                             deliveryDistance);
                        mStage = STAGE_BACK_TO_LINE;
                        mStageInitialized = false;
                    }
                    break;
                }
                case STAGE_BACK_TO_LINE: {
                    // 納品場所までの前進距離と同じ距離だけ後退する。
                    if(!mStageInitialized) {
                        mStageInitialized = true;
                        LOGI("[BOTTLE] 納品場所から同じ距離の後退を開始\n");
                    }
                    mWalker->setPwm(-mPwm, -mPwm);
                    mWalker->run();
                    if(mDeliveryBackDistanceTerminator->isToBeTerminate()) {
                        // 同じ距離を後退したため、最初と同じ向きに90度回転する。
                        mWalker->stop();
                        LOGI("[BOTTLE] 後退完了; 同じ向きに90度回転します\n");
                        mStage = STAGE_TURN_TO_RETURN;
                        mStageInitialized = false;
                    }
                    break;
                }
                case STAGE_TURN_TO_RETURN: {
                    // 納品場所へ向かったときと同じ向きに90度回転する。
                    if(!mStageInitialized) {
                        // 行きと同じ向きの90度旋回を開始する。
                        mStartLeftCount = mWalker->getLeftCount();
                        mStartRightCount = mWalker->getRightCount();
                        mStageInitialized = true;
                        LOGI("[BOTTLE] 帰路方向への回転を開始\n");
                    }
                    if(mIsLeftEdge) {
                        mWalker->setPwm(mPwm, -mPwm);
                    } else {
                        mWalker->setPwm(-mPwm, mPwm);
                    }
                    mWalker->run();
                    int delta = std::abs((mWalker->getLeftCount() - mStartLeftCount)
                                         - (mWalker->getRightCount() - mStartRightCount));
                    if(delta >= TURN_90_COUNT) {
                        // 帰路方向を向いたため、逆エッジでライン・トレースを開始する。
                        LOGI("[BOTTLE] 帰路方向への回転完了\n");
                        mStage = STAGE_RETURN_PATH;
                        mStageInitialized = false;
                    }
                    break;
                }
                case STAGE_RETURN_PATH: {
                    // 帰路では基準エッジを反転してライン・トレースする。
                    if(!mStageInitialized) {
                         mStageInitialized = true;
                         mLastDetectedColor = BLACK;
                         mBlueConsecutiveCount = 0;
                         LOGI("[BOTTLE] 帰路のライントレースを開始\n");
                        
                    }
                    mLineTracer->run();
                    eColor onBlueLine = getDetectedColorContinuous();
                    const int blueLineTouchTarget
                        = mDetectedBlueBottle     ? BLUE_LINE_TOUCH_TARGET_BLUE
                          : mDetectedYellowBottle ? BLUE_LINE_TOUCH_TARGET_YELLOW
                                                  : BLUE_LINE_TOUCH_TARGET_RED;

                    if(onBlueLine == BLUE && mLastDetectedColor != BLUE) {
                        mBlueLineTouchCountReturn++;
                        LOGI("[CAL] シミュレータ: 青線に触れました (%d)\n",
                             mBlueLineTouchCountReturn);
                    }

                    mLastDetectedColor = onBlueLine;

                    if(mBlueLineTouchCountReturn >= blueLineTouchTarget - 1) {
                        // 必要回数の青線を踏破したため、納品エリア側へ旋回する。
                        mWalker->stop();
                    }

                    break;
                }
                case STAGE_DONE:
                    // ミッション終了。状態を終了へ遷移する。
                    // 終了後に走行し続けないよう、毎周期モーターを停止する。
                    mWalker->stop();
                    LOGI("[BOTTLE] ミッション完了\n");
                    mState = TERMINATED;
                    break;
            }
            break;
        case TERMINATED:
            break;
    }
}


eColor BottleDeliveryTracer::getDetectedColorContinuous()
{
    if(!mColorSensor) {
        return BLACK;
    }
    spikeapi::ColorSensor::HSV hsv;
    mColorSensor->getHSV(hsv);
    eColor currentColor = getColor(hsv.h, hsv.s, hsv.v);

    // 連続検知ロジック（ColorTerminatorと同じ）
    if(currentColor == BLUE) {
        mBlueConsecutiveCount++;
    } else {
        mBlueConsecutiveCount = 0;
    }

    // 2回以上連続で同じ色（青の場合）を検知したら確定
    // 他の色の場合は1回で確定
    if(currentColor == BLUE) {
        return (mBlueConsecutiveCount >= 2) ? BLUE : OTHER;
    }
    return currentColor;
}

eColor BottleDeliveryTracer::getDetectedColor() const
{
    if(!mColorSensor) {
        return BLACK;
    }
    spikeapi::ColorSensor::HSV hsv;
    mColorSensor->getHSV(hsv);
    return getColor(hsv.h, hsv.s, hsv.v);
}
