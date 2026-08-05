#include "Log.h"
#include "app.h"
#include "kernel_cfg.h"

#include <kernel.h>
#ifdef SPIKERT
#include <syssvc/serial.h>
#include <serial/serial.h>
#endif

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#ifdef SPIKERT
static const uint_t BT_TX_QUEUE_LENGTH = 64U;
static const uint_t BT_TX_MESSAGE_SIZE = 512U;
#endif

static bool gBluetoothModeEnabled = false;
#ifdef ETROBO_PHYSICAL_BUILD
static bool gBluetoothPortOpened = false;
#endif
#ifdef SPIKERT
static char gBluetoothTxQueue[BT_TX_QUEUE_LENGTH][BT_TX_MESSAGE_SIZE];
static uint_t gBluetoothTxHead = 0U;
static uint_t gBluetoothTxTail = 0U;
static uint_t gBluetoothTxCount = 0U;
static uint_t gBluetoothTxDroppedCount = 0U;
#endif

void ettr_log_set_bluetooth_mode(bool enabled)
{
#ifdef SPIKERT
    gBluetoothModeEnabled = enabled;
#else
    (void)enabled;
#endif
}

bool ettr_log_wait_for_bluetooth(void)
{
#ifdef ETROBO_PHYSICAL_BUILD
    if(!gBluetoothPortOpened) {
        ER ercd = serial_opn_por(SIO_BLUETOOTH_PORTID);
        gBluetoothPortOpened = true;
        if(ercd == E_OK) {
            gBluetoothModeEnabled = true;
        }
    }
#endif

    return gBluetoothModeEnabled;
}

#ifdef SPIKERT
static void ettr_log_write_bt(const char* text)
{
    size_t length = strlen(text);

    if(length > 0U) {
        (void)serial_wri_dat(SIO_BLUETOOTH_PORTID, text, (uint_t)length);
    }
}

static bool ettr_log_enqueue_bt(const char* text)
{
    uint_t slot;
    size_t index = 0U;

    loc_cpu();
    if(gBluetoothTxCount >= BT_TX_QUEUE_LENGTH) {
        gBluetoothTxDroppedCount++;
        unl_cpu();
        return false;
    }

    slot = gBluetoothTxTail;
    while(index < (BT_TX_MESSAGE_SIZE - 1U) && text[index] != '\0') {
        gBluetoothTxQueue[slot][index] = text[index];
        index++;
    }
    gBluetoothTxQueue[slot][index] = '\0';

    gBluetoothTxTail = (gBluetoothTxTail + 1U) % BT_TX_QUEUE_LENGTH;
    gBluetoothTxCount++;
    unl_cpu();

    wup_tsk(BT_SENDER_TASK);
    return true;
}

extern "C" void sender_task(intptr_t exinf)
{
    uint_t slot;

    (void)exinf;

    while(1) {
        loc_cpu();
        if(gBluetoothTxCount == 0U) {
            unl_cpu();
            slp_tsk();
            continue;
        }

        slot = gBluetoothTxHead;
        unl_cpu();

        ettr_log_write_bt(gBluetoothTxQueue[slot]);

        loc_cpu();
        gBluetoothTxHead = (gBluetoothTxHead + 1U) % BT_TX_QUEUE_LENGTH;
        gBluetoothTxCount--;
        unl_cpu();
    }
}
#endif

void ettr_log_write_v(const char* fmt, va_list ap)
{
#ifdef SPIKERT
    char buffer[512];
    int written = vsnprintf(buffer, sizeof(buffer), fmt, ap);

    if(written < 0) {
        return;
    }

    if(gBluetoothModeEnabled) {
        (void)ettr_log_enqueue_bt(buffer);
        return;
    }

    fputs(buffer, stdout);
#else
    vfprintf(stdout, fmt, ap);
#endif
}

void ettr_log_write(const char* fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    ettr_log_write_v(fmt, ap);
    va_end(ap);
}