#if defined(MELEE_NATIVE)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "dbnativecontrol.h"
#include "dbnativecontrol_protocol.h"

#include <stdlib.h>
#include <string.h>

#include <melee/db/db.h>
#include <melee/ft/native_costume_runtime.h>
#include <melee/gm/native_online_session.h>
#include <melee/gm/native_signaling_wire.h>
#include <melee/gm/native_netplay_match.h>
#include <melee/gr/grpstadium.h>
#include <melee/mn/mnstagesel.h>
#include <melee/ui/native_shell.h>
#include <sysdolphin/baselib/debug.h>
#include <dolphin/pad.h>

typedef struct DbNativePadState {
    u32 remaining_frames;
    bool active;
} DbNativePadState;

static HANDLE control_channel = INVALID_HANDLE_VALUE;
static DbNativePadState virtual_pads[PAD_MAX_CONTROLLERS];

static void apply_message(const DbNativeControlMessage* message)
{
    if (message->magic != DB_NATIVE_CONTROL_MAGIC) {
        return;
    }
    if (message->command == DB_NATIVE_CONTROL_SCREENSHOT) {
        db_RequestScreenshot();
        return;
    }
    if (message->command == DB_NATIVE_CONTROL_ROOM_ROTATION) {
        if (message->rotation_policy <= NativeRoomRotation_WinnerOut) {
            bool queued = nativeOnlineSession_SetRotation(
                (NativeRoomRotationPolicy) message->rotation_policy);
            OSReport("[native-control] Room rotation %u %s.\n",
                     message->rotation_policy,
                     queued ? "queued" : "rejected");
        }
        return;
    }
    if (message->command == DB_NATIVE_CONTROL_ROOM_STAGE) {
        bool queued = message->port < 2 && (message->ui_x >= 0 && message->ui_x < 6 || message->ui_x == 255) &&
            nativeOnlineSession_PickQuickStage(message->port, (uint8_t) message->ui_x);
        OSReport("[native-control] Stage vote seat=%u stage=%d %s.\n", message->port, message->ui_x,
                 queued ? "queued" : "rejected");
        return;
    }
    if (message->port >= PAD_MAX_CONTROLLERS) {
        return;
    }
    if (message->command == DB_NATIVE_CONTROL_UI_CLICK) {
        if (message->buttons <= 2)
            nativeShell_ClickAt(message->ui_x, message->ui_y, message->buttons);
        return;
    }
    if (message->command == DB_NATIVE_CONTROL_UI_MOVE) {
        nativeShell_MovePointer(message->ui_x, message->ui_y);
        return;
    }
    if (message->command == DB_NATIVE_CONTROL_UI_KEY) {
        if (memchr(message->text, '\0', sizeof(message->text)))
            nativeShell_PressKey(message->text, message->buttons);
        return;
    }
    if (message->command == DB_NATIVE_CONTROL_UI_DRAG) {
        nativeShell_DragAt(message->ui_x, message->ui_y,
                           message->ui_end_x, message->ui_end_y);
        return;
    }
    if (message->command == DB_NATIVE_CONTROL_UI_INSPECT) {
        if (memchr(message->text, '\0', sizeof(message->text))) {
            if (!strcmp(message->text,"stadium-state")) grStadium_Inspect();
            else if (!strcmp(message->text,"netplay-state")) nativeNetplayMatch_Inspect();
            else if (!strcmp(message->text,"costume-state")) nativeCostumeRuntime_Inspect();
            else if (!strcmp(message->text,"sss-state")) mnStageSel_Inspect();
            else nativeShell_InspectElement(message->text);
        }
        return;
    }
    if (message->command == DB_NATIVE_CONTROL_UI_TEXT) {
        if (memchr(message->text, '\0', sizeof(message->text)) != NULL) {
            nativeShell_EnterText(message->text);
        }
        return;
    }
    if (message->command == DB_NATIVE_CONTROL_CLEAR_PAD) {
        PADClearVirtualStatus(message->port);
        virtual_pads[message->port].active = false;
        virtual_pads[message->port].remaining_frames = 0;
        return;
    }
    if (message->command == DB_NATIVE_CONTROL_SET_PAD) {
        PADStatus status = { 0 };
        status.button = (u16) message->buttons;
        status.stickX = (s8) message->stick_x;
        status.stickY = (s8) message->stick_y;
        status.substickX = (s8) message->cstick_x;
        status.substickY = (s8) message->cstick_y;
        status.triggerLeft = (u8) message->trigger_left;
        status.triggerRight = (u8) message->trigger_right;
        status.analogA = (u8) message->analog_a;
        status.analogB = (u8) message->analog_b;
        status.extButton = message->extensions;
        PADSetVirtualStatus(message->port, &status);
        virtual_pads[message->port].active = true;
        virtual_pads[message->port].remaining_frames = message->frames;
    }
}

void dbNativeControl_Init(void)
{
    char channel[DB_NATIVE_CONTROL_CHANNEL_BYTES];
    const char* id = getenv("FREAK_MELEE_CONTROL_ID");
    if (!dbNativeControl_FormatChannel(channel, id)) {
        OSReport("Invalid FREAK_MELEE_CONTROL_ID; native control disabled\n");
        return;
    }
    control_channel = CreateMailslotA(channel,
                                     sizeof(DbNativeControlMessage), 0, NULL);
    if (control_channel == INVALID_HANDLE_VALUE) {
        OSReport("Native control channel unavailable (%lu)\n", GetLastError());
        return;
    }
    OSReport("Native control channel ready: %s\n", channel);
}

void dbNativeControl_Poll(void)
{
    if (control_channel == INVALID_HANDLE_VALUE) {
        return;
    }
    for (;;) {
        DbNativeControlMessage message;
        DWORD next_size = MAILSLOT_NO_MESSAGE;
        DWORD message_count = 0;
        if (!GetMailslotInfo(control_channel, NULL, &next_size, &message_count,
                             NULL))
        {
            return;
        }
        if (message_count == 0 || next_size == MAILSLOT_NO_MESSAGE) {
            return;
        }
        DWORD bytes_read = 0;
        if (!ReadFile(control_channel, &message, sizeof(message), &bytes_read,
                      NULL) || bytes_read != sizeof(message))
        {
            return;
        }
        apply_message(&message);
    }
}

void dbNativeControl_OnInputSample(void)
{
    for (u32 port = 0; port < PAD_MAX_CONTROLLERS; ++port) {
        DbNativePadState* pad = &virtual_pads[port];
        if (!pad->active || pad->remaining_frames == 0) {
            continue;
        }
        pad->remaining_frames -= 1;
        if (pad->remaining_frames == 0) {
            PADStatus neutral = { 0 };
            PADSetVirtualStatus(port, &neutral);
            pad->active = false;
        }
    }
}

void dbNativeControl_Shutdown(void)
{
    PADClearAllVirtualStatus();
    if (control_channel != INVALID_HANDLE_VALUE) {
        CloseHandle(control_channel);
        control_channel = INVALID_HANDLE_VALUE;
    }
    for (u32 port = 0; port < PAD_MAX_CONTROLLERS; ++port) {
        virtual_pads[port].active = false;
        virtual_pads[port].remaining_frames = 0;
    }
}

#endif
