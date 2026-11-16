#if defined(MELEE_NATIVE)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "dbnativecontrol.h"
#include "dbnativecontrol_protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <melee/db/db.h>
#include <melee/ft/native_costume_runtime.h>
#include <melee/gm/native_online_session.h>
#include <melee/gm/native_spectator_match.h>
#include <melee/gm/native_signaling_wire.h>
#include <melee/gm/native_netplay_match.h>
#include <melee/gr/grpstadium.h>
#include <melee/lb/lbshadow.h>
#include <melee/mn/mnstagesel.h>
#include <melee/ui/native_shell.h>
#include <melee/ui/native_shell_workshop.h>
#include <sysdolphin/baselib/debug.h>
#include <dolphin/pad.h>
#include <aurora/aurora.h>
#include <melee/gm/native_studio.h>
#include <sysdolphin/baselib/audio_capture.h>
#include <SDL3/SDL.h>

typedef struct DbNativePadState {
    u32 remaining_frames;
    bool active;
} DbNativePadState;

static HANDLE control_channel = INVALID_HANDLE_VALUE;
static DbNativePadState virtual_pads[PAD_MAX_CONTROLLERS];
static struct { SDL_JoystickID id; SDL_Gamepad* gamepad; } devices[4];

// These devices pass through SDL and the ordinary Aurora input provider. The
// older PAD override remains useful for deterministic post-mapping input.
static bool device_command(const char* command)
{
    unsigned slot, port, button, down; int values[6], end = 0;
    if (sscanf(command,"attach,%u%n",&slot,&end)==1 && end && !command[end] && slot<4) {
        if (devices[slot].id) return false;
        SDL_VirtualJoystickDesc desc; SDL_INIT_INTERFACE(&desc);
        desc.type=SDL_JOYSTICK_TYPE_GAMEPAD; desc.naxes=SDL_GAMEPAD_AXIS_COUNT;
        desc.nbuttons=SDL_GAMEPAD_BUTTON_COUNT;
        desc.axis_mask=(1u<<SDL_GAMEPAD_AXIS_COUNT)-1;
        desc.button_mask=(1u<<SDL_GAMEPAD_BUTTON_COUNT)-1;
        desc.name="Freak Melee virtual controller";
        SDL_JoystickID id=SDL_AttachVirtualJoystick(&desc);
        SDL_Gamepad* pad=id?SDL_OpenGamepad(id):NULL;
        if (!pad) { if (id) SDL_DetachVirtualJoystick(id); return false; }
        devices[slot].id=id; devices[slot].gamepad=pad;
        SDL_Joystick* joystick=SDL_GetGamepadJoystick(pad);
        for (unsigned axis=0;axis<SDL_GAMEPAD_AXIS_COUNT;++axis)
            SDL_SetJoystickVirtualAxis(joystick,axis,axis>=4?-32768:0);
        SDL_UpdateGamepads(); return true;
    }
    end=0;
    if (sscanf(command,"detach,%u%n",&slot,&end)==1 && end && !command[end] && slot<4 && devices[slot].id) {
        SDL_CloseGamepad(devices[slot].gamepad); SDL_DetachVirtualJoystick(devices[slot].id);
        devices[slot].gamepad=NULL; devices[slot].id=0; return true;
    }
    end=0;
    if (sscanf(command,"port,%u,%u%n",&slot,&port,&end)==2 && end && !command[end] && slot<4 && port<4 && devices[slot].gamepad)
        return SDL_SetGamepadPlayerIndex(devices[slot].gamepad,port);
    end=0;
    if (sscanf(command,"button,%u,%u,%u%n",&slot,&button,&down,&end)==3 && end && !command[end] && slot<4 &&
        button<SDL_GAMEPAD_BUTTON_COUNT && down<2 && devices[slot].gamepad) {
        bool ok=SDL_SetJoystickVirtualButton(SDL_GetGamepadJoystick(devices[slot].gamepad),button,down!=0);
        SDL_UpdateGamepads(); return ok;
    }
    end=0;
    if (sscanf(command,"axes,%u,%d,%d,%d,%d,%d,%d%n",&slot,values,values+1,values+2,values+3,values+4,values+5,&end)==7 &&
        end && !command[end] && slot<4 && devices[slot].gamepad) {
        for (unsigned i=0;i<6;++i) if (values[i]<-32768 || values[i]>32767) return false;
        bool ok=true;
        for (unsigned i=0;i<6;++i) ok=SDL_SetJoystickVirtualAxis(SDL_GetGamepadJoystick(devices[slot].gamepad),i,(Sint16)values[i]) && ok;
        SDL_UpdateGamepads(); return ok;
    }
    return false;
}

static void apply_message(const DbNativeControlMessage* message)
{
    if (message->magic != DB_NATIVE_CONTROL_MAGIC) {
        return;
    }
    if (message->command==DB_NATIVE_CONTROL_DEVICE) {
        if (memchr(message->text,0,sizeof(message->text)))
            OSReport("[native-control] Device request accepted=%u\n",device_command(message->text));
        return;
    }
    if (message->command == DB_NATIVE_CONTROL_SCREENSHOT) {
        db_RequestScreenshot();
        return;
    }
    if (message->command == DB_NATIVE_CONTROL_VIDEO_START || message->command == DB_NATIVE_CONTROL_VIDEO_STOP) {
        bool accepted=false;
        if (message->command==DB_NATIVE_CONTROL_VIDEO_STOP) accepted=aurora_request_video_capture(NULL);
        else if (memchr(message->text,0,sizeof(message->text))) accepted=aurora_request_video_capture(message->text);
        OSReport("[native-control] Video request accepted=%u\n",accepted);
        return;
    }
    if (message->command==DB_NATIVE_CONTROL_WORKSHOP_IMPORT) {
        bool ok = memchr(message->text, 0, sizeof(message->text)) &&
                  nativeShellWorkshop_ImportFighter(message->text);
        OSReport("[native-control] Workshop fighter import accepted=%u\n", ok);
        return;
    }
    if (message->command==DB_NATIVE_CONTROL_WORKSHOP_IMAGE) {
        bool ok = memchr(message->text, 0, sizeof(message->text)) &&
                  nativeShellWorkshop_ImportHudImage(message->port, message->text);
        OSReport("[native-control] Workshop HUD image import accepted=%u\n", ok);
        return;
    }
    if (message->command==DB_NATIVE_CONTROL_STUDIO) {
        if (memchr(message->text,0,sizeof(message->text))) nativeStudio_Command(message->text);
        return;
    }
    if (message->command==DB_NATIVE_CONTROL_AUDIO_START || message->command==DB_NATIVE_CONTROL_AUDIO_STOP) {
        bool accepted=false;
        if (message->command==DB_NATIVE_CONTROL_AUDIO_STOP) accepted=HSD_AudioCaptureSetFile(NULL);
        else if (memchr(message->text,0,sizeof(message->text))) accepted=HSD_AudioCaptureSetFile(message->text);
        OSReport("[native-control] Audio request accepted=%u\n",accepted);
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
    if (message->command == DB_NATIVE_CONTROL_ROOM_WATCH) {
        if(message->buttons==0)nativeOnlineSession_StopWatching();
        else if(message->buttons==1) {
            bool started=nativeOnlineSession_WatchMatch();
            OSReport("[native-control] Watch match %s.\n",started ? "requested" : "rejected");
        }
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
    if (message->command == DB_NATIVE_CONTROL_UI_DOWN ||
        message->command == DB_NATIVE_CONTROL_UI_UP) {
        if (message->buttons <= 2)
            nativeShell_PointerButtonAt(message->ui_x, message->ui_y,
                message->buttons, message->command == DB_NATIVE_CONTROL_UI_DOWN);
        return;
    }
    if (message->command == DB_NATIVE_CONTROL_UI_KEY) {
        if (memchr(message->text, '\0', sizeof(message->text)))
            nativeShell_PressKey(message->text, message->buttons);
        return;
    }
    if (message->command == DB_NATIVE_CONTROL_UI_DRAG) {
        if (message->buttons <= 2 && message->extensions <= 7)
            nativeShell_DragAt(message->ui_x, message->ui_y,
                               message->ui_end_x, message->ui_end_y,
                               message->buttons, message->extensions);
        return;
    }
    if (message->command == DB_NATIVE_CONTROL_UI_WHEEL) {
        if (message->stick_y >= -100 && message->stick_y <= 100)
            nativeShell_WheelAt(message->ui_x, message->ui_y, message->stick_y);
        return;
    }
    if (message->command == DB_NATIVE_CONTROL_UI_INSPECT) {
        if (memchr(message->text, '\0', sizeof(message->text))) {
            if (!strcmp(message->text,"stadium-state")) grStadium_Inspect();
            else if (!strcmp(message->text,"shadow-state")) lbShadow_Inspect();
            else if (!strcmp(message->text,"netplay-state")) nativeNetplayMatch_Inspect();
            else if (!strcmp(message->text,"costume-state")) nativeCostumeRuntime_Inspect();
            else if (!strcmp(message->text,"sss-state")) mnStageSel_Inspect();
            else if (!strcmp(message->text,"room-state")) nativeOnlineSession_Inspect();
            else if (!strcmp(message->text,"watch-playback")) nativeSpectatorMatch_Inspect();
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
    for (unsigned i=0;i<4;++i) if (devices[i].id) {
        SDL_CloseGamepad(devices[i].gamepad); SDL_DetachVirtualJoystick(devices[i].id);
        devices[i].gamepad=NULL; devices[i].id=0;
    }
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
