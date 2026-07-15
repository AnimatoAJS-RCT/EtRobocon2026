#ifndef ETTR_APP_CALIBRATOR_H_
#define ETTR_APP_CALIBRATOR_H_

#include "ColorSensor.h"
#include "ForceSensor.h"
#include "Light.h"
#include "Button.h"

class Calibrator {
public:
    explicit Calibrator(const spikeapi::ColorSensor& colorSensor,
                        const spikeapi::ForceSensor& forceSensor,
                        spikeapi::Light& light);
    void run();
    int getBlack();
    int getWhite();
    int getTarget();
    bool isFinished();

private:
    void execUndefined();
    void execWaitingForStart();
    void execSettingCourse();
    void execCalibratingBlack();
    void execCalibratingWhite();
    void execWaitingForStartConfirmation();

    enum State {
        UNDEFINED,
        WAITING_FOR_START,
        // LEFT/RIGHT selects the course and CENTER confirms it.
        SETTING_COURSE,
        CALIBRATING_BLACK,
        CALIBRATING_WHITE,
        WAITING_FOR_START_CONFIRMATION,
        TERMINATED
    };
    State mState = UNDEFINED;

    const spikeapi::ColorSensor& mColorSensor;
    const spikeapi::ForceSensor& mForceSensor;
    spikeapi::Light& mLight;
    spikeapi::Button mButton;
    int mBlack = 0;
    int mWhite = 100;
    bool mIsInitialized = false;
    bool mWasTouched = false;
    bool mPressPending = false;

    void updateButtonState();
    bool consumePress();
};

#endif  // ETTR_APP_CALIBRATOR_H_
