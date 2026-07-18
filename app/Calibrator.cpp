#include "Calibrator.h"
#include "Log.h"

#include "app.h"

#include "kernel.h"

Calibrator::Calibrator(const spikeapi::ColorSensor& colorSensor,
                       const spikeapi::ForceSensor& forceSensor,
                       spikeapi::Light& light)
    : mColorSensor(colorSensor), mForceSensor(forceSensor), mLight(light)
{
}

void Calibrator::run()
{
    updateButtonState();

    switch(mState) {
        case UNDEFINED:
            execUndefined();
            break;
        case WAITING_FOR_START:
            execWaitingForStart();
            break;
        case SETTING_COURSE:
            execSettingCourse();
            break;
        case CALIBRATING_BLACK:
            execCalibratingBlack();
            break;
        case CALIBRATING_WHITE:
            execCalibratingWhite();
            break;
        case WAITING_FOR_START_CONFIRMATION:
            execWaitingForStartConfirmation();
            break;
        case TERMINATED:
            break;
        default:
            break;
    }
}

int Calibrator::getBlack()
{
    return mBlack;
}

int Calibrator::getWhite()
{
    return mWhite;
}

int Calibrator::getTarget()
{
    return (mWhite + mBlack) / 2;
}

bool Calibrator::isFinished()
{
    return mState == TERMINATED;
}

// Calibration flow on the hub:
// 1) ORANGE: press force sensor to start calibration.
// 2) RED/BLUE: choose course with LEFT/RIGHT arrow, then press CENTER to confirm.
// 3) LED OFF: place sensor on black and press force sensor.
// 4) WHITE: place sensor on white and press force sensor.
// 5) GREEN: confirm the values, then press force sensor to start.

void Calibrator::execUndefined()
{
    if(mIsInitialized == false) {
        // 初期化処理
        mIsInitialized = true;
    }
#ifndef SPIKERT
    // シミュレータ: フォースセンサ/ボタン操作なしで即座に完了させる。
    // 黒/白はデフォルト値(0/100)のままなので、tracer.ini の目標輝度が
    // 正規化値のまま使われる。コースはシミュレータ既定の L コースに合わせる。
    IS_LEFT_COURSE = true;
    LOGI("[CAL] simulator: skip calibration (course=LEFT, black=%d, white=%d)\n", mBlack,
         mWhite);
    mState = TERMINATED;
#else
    mState = WAITING_FOR_START;
#endif
}

void Calibrator::execWaitingForStart()
{
    mLight.turnOnColor(spikeapi::Light::EColor::ORANGE);
    if(consumePress()) {
        LOGI("[CAL] start pressed\n");
        mState = SETTING_COURSE;
    }
}

void Calibrator::execSettingCourse()
{
    mLight.turnOnColor(IS_LEFT_COURSE ? spikeapi::Light::EColor::RED
                                      : spikeapi::Light::EColor::BLUE);
    if(mButton.isLeftPressed()) {
        IS_LEFT_COURSE = true;
        LOGI("[CAL] course: %s\n", IS_LEFT_COURSE ? "LEFT" : "RIGHT");
    } else if(mButton.isRightPressed()) {
        IS_LEFT_COURSE = false;
        LOGI("[CAL] course: %s\n", IS_LEFT_COURSE ? "LEFT" : "RIGHT");
    } else if(mButton.isCenterPressed()) {
        LOGI("[CAL] course confirmed: %s\n", IS_LEFT_COURSE ? "LEFT" : "RIGHT");
        mState = CALIBRATING_BLACK;
    }
}

void Calibrator::execCalibratingBlack()
{
    mLight.turnOff();
    if(consumePress()) {
        mBlack = mColorSensor.getReflection();
        LOGI("[CAL] black: %d\n", mBlack);
        mState = CALIBRATING_WHITE;
    }
}

void Calibrator::execCalibratingWhite()
{
    mLight.turnOnColor(spikeapi::Light::EColor::WHITE);
    if(consumePress()) {
        mWhite = mColorSensor.getReflection();
        LOGI("[CAL] white: %d, target: %d\n", mWhite, getTarget());
        mState = WAITING_FOR_START_CONFIRMATION;
    }
}

void Calibrator::execWaitingForStartConfirmation()
{
    mLight.turnOnColor(spikeapi::Light::EColor::GREEN);
    if(consumePress()) {
        LOGI("[CAL] calibration confirmed\n");
        mState = TERMINATED;
    }
}

void Calibrator::updateButtonState()
{
    bool isTouched = mForceSensor.isTouched();
    if(isTouched && !mWasTouched) {
        mPressPending = true;
    }
    mWasTouched = isTouched;
}

bool Calibrator::consumePress()
{
    bool pressed = mPressPending;
    mPressPending = false;
    return pressed;
}
