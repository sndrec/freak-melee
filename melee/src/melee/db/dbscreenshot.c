#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>
#include <time.h>
#include <aurora/aurora.h>
#include <melee/ft/native_file_publish.h>

#include "db.h"
#include <melee/gm/gm_unsplit.h>
#include <sysdolphin/baselib/controller.h>
#include <sysdolphin/baselib/debug.h>

/* 4D6B94 */ int db_ScreenshotNumber;
/* 4D6B90 */ unsigned int db_ScreenshotPending;
/* 4D6B8C */ UnkFlagStruct db_5xSpeedStatus;
/* 4D6B88 */ int db_804D6B88;

void fn_Setup5xSpeed(void)
{
    db_5xSpeedStatus.b0 = false;
}

void fn_Check5xSpeed(int player)
{
    if ((db_ButtonsDown(player) & HSD_PAD_A) &&
        (db_ButtonsPressed(player) & HSD_PAD_DPADRIGHT))
    {
        fn_Toggle5xSpeed();
    }
}

void fn_Toggle5xSpeed(void)
{
    db_5xSpeedStatus.b0 ^= 1;
    if (db_5xSpeedStatus.b0) {
        gm_SetGameSpeed(5.0f);
    } else {
        gm_ResetGameSpeed();
    }
}

void db_InitScreenshot(void)
{
    db_ScreenshotNumber = 0;
    db_ScreenshotPending = 0;
}

void db_RequestScreenshot(void)
{
    db_ScreenshotPending = 1;
}

static inline HSD_PadStatus* get_pad(u8 i)
{
    return &HSD_PadMasterStatus[i];
}

void db_CheckScreenshot(void)
{
    int i;

    for (i = 0; i < 4; i++) {
        if ((get_pad(i)->button & HSD_PAD_Y) &&
            (get_pad(i)->trigger & HSD_PAD_DPADUP))
        {
            db_RequestScreenshot();
        }
    }
}

void db_TakeScreenshotIfPending(void)
{
    static char path[128];
    static uint64_t capture_id;
    if (capture_id) {
        AuroraPresentedCapture result;
        aurora_presented_capture_status(&result);
        if (result.capture_id == capture_id && result.status == AURORA_CAPTURE_PENDING) return;
        size_t size;
        const void* png = aurora_presented_png(capture_id, &size);
        if (png && !nativeFile_PublishLocal(path, png, size, NATIVE_FILE_PRESERVE, NULL))
            fprintf(stderr, "Could not save screenshot %s\n", path);
        aurora_release_presented_capture(capture_id);
        capture_id = 0;
    }
    if (db_ScreenshotPending == 0) {
        return;
    }
    _mkdir("screenshots");
    snprintf(path, sizeof(path), "screenshots/melee-%lld-%lu-%04d.png",
             (long long) time(NULL), (unsigned long) GetCurrentProcessId(),
             db_ScreenshotNumber);
    AuroraPresentedCapture requested;
    if (aurora_capture_presented(false, &requested)) {
        capture_id = requested.capture_id;
        db_ScreenshotNumber++;
        db_ScreenshotPending = 0;
    }
}
