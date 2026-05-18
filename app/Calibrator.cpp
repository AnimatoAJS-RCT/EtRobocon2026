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
        mState = CALIBRATING_BLACK;
        tslp_tsk(500);  // 500ms wait
    }
}

void Calibrator::execCalibratingBlack()
{
    LOGI("Calibrating black...\n");
    mBlack = mColorSensor.getReflection();
    char msg[32];
    sprintf(msg, "Black: %d", mBlack);
    LOGI("%s\n", msg);
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
    char msg[32];
    sprintf(msg, "White: %d", mWhite);
    LOGI("%s\n", msg);
    tslp_tsk(1000);  // 1s wait
    mState = WAITING_FOR_FINISH;
}

void Calibrator::execWaitingForFinish()
{
    static int finishLoop = 0;
    if(finishLoop == 0) {
        LOGI("Finished.\n");
        char msg[32];
        sprintf(msg, "Target: %d", getTarget());
        LOGI("%s\n", msg);
    }

    if(finishLoop % 20 == 0) {
        LOGD("[CAL] WAITING_FOR_FINISH: touched=%d force=%.2f\n",
             mForceSensor.isTouched() ? 1 : 0, mForceSensor.getForce());
    }

    if(mForceSensor.isTouched()) {
        LOGI("[CAL] WAITING_FOR_FINISH -> TERMINATED\n");
        mState = TERMINATED;
        finishLoop = 0;
        return;
    }

    finishLoop++;
    tslp_tsk(50);
}
