#include "Calibrator.h"
#include "Util.h"
#include "Log.h"

#include "kernel.h"

Calibrator::Calibrator(const spikeapi::ColorSensor& colorSensor,
                                             const spikeapi::ForceSensor& forceSensor)
    : mColorSensor(colorSensor), mForceSensor(forceSensor)
{
}

void Calibrator::run()
{
#ifndef ETROBO_PHYSICAL_BUILD
    LOGI("[CAL] run entry: sim branch\n");
#else
    LOGI("[CAL] run entry: physical branch\n");
#endif
#ifndef ETROBO_PHYSICAL_BUILD
    mState = TERMINATED;
    return;
#endif

    while(!ettr_log_wait_for_bluetooth()) {
        if(mForceSensor.isTouched()) {
            LOGI("[CAL] skip bluetooth wait by force sensor\n");
            break;
        }
        tslp_tsk(100);
    }

    while(1) {
        switch(mState) {
            case UNDEFINED:
                execUndefined();
                break;
            case WAITING_FOR_START:
                execWaitingForStart();
                break;
            case CALIBRATING_BLACK:
                execCalibratingBlack();
                break;
            case WAITING_FOR_WHITE:
                execWaitingForWhite();
                break;
            case CALIBRATING_WHITE:
                execCalibratingWhite();
                break;
            case WAITING_FOR_FINISH:
                execWaitingForFinish();
                break;
            case TERMINATED:
                // Do nothing
                return;
            default:
                break;
        }
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

void Calibrator::execUndefined()
{
    if(mIsInitialized == false) {
        // 初期化処理
        mIsInitialized = true;
    }
    mState = WAITING_FOR_START;
}

void Calibrator::execWaitingForStart()
{
    LOGI("Calibrate: Push to start\n");
    while(!mForceSensor.isTouched()) {
        tslp_tsk(500);  // 500ms wait
    }

    while(mForceSensor.isTouched()) {
        tslp_tsk(50);
    }

    mState = CALIBRATING_BLACK;
}

void Calibrator::execCalibratingBlack()
{
    LOGI("Calibrating black...\n");
    mBlack = mColorSensor.getReflection();
    LOGI("Black: %d\n", mBlack);
    tslp_tsk(1000);  // 1s wait
    mState = WAITING_FOR_WHITE;
}

void Calibrator::execWaitingForWhite()
{
    static int waitLoop = 0;
    if(waitLoop % 20 == 0) {
        LOGD("[CAL] WAITING_FOR_WHITE: touched=%d force=%.2f\n",
             mForceSensor.isTouched() ? 1 : 0, mForceSensor.getForce());
        LOGI("Set white\n");
        LOGI("Push to start\n");
    }

    if(mForceSensor.isTouched()) {
        LOGI("[CAL] WAITING_FOR_WHITE -> CALIBRATING_WHITE\n");
        while(mForceSensor.isTouched()) {
            tslp_tsk(50);
        }
        mState = CALIBRATING_WHITE;
        tslp_tsk(500);  // 500ms wait
        waitLoop = 0;
        return;
    }

    waitLoop++;
    tslp_tsk(50);
}

void Calibrator::execCalibratingWhite()
{
    LOGI("Calibrating white...\n");
    mWhite = mColorSensor.getReflection();
    LOGI("White: %d\n", mWhite);
    tslp_tsk(1000);  // 1s wait
    mState = WAITING_FOR_FINISH;
}

void Calibrator::execWaitingForFinish()
{
    static int finishLoop = 0;
    if(finishLoop == 0) {
        LOGI("Finished.\n");
        LOGI("Target: %d\n", getTarget());
    }

    if(finishLoop % 20 == 0) {
        LOGD("[CAL] WAITING_FOR_FINISH: touched=%d force=%.2f\n",
             mForceSensor.isTouched() ? 1 : 0, mForceSensor.getForce());
    }

    if(mForceSensor.isTouched()) {
        LOGI("[CAL] WAITING_FOR_FINISH -> TERMINATED\n");
        while(mForceSensor.isTouched()) {
            tslp_tsk(50);
        }
        mState = TERMINATED;
        finishLoop = 0;
        return;
    }

    finishLoop++;
    tslp_tsk(50);
}
