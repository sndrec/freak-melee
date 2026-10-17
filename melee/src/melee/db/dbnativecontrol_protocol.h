#ifndef MELEE_DB_NATIVE_CONTROL_PROTOCOL_H
#define MELEE_DB_NATIVE_CONTROL_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>

#include <dolphin/types.h>

#define DB_NATIVE_CONTROL_CHANNEL "\\\\.\\mailslot\\freak-melee-control"
#define DB_NATIVE_CONTROL_MAGIC 0x464D4354U

enum {
    DB_NATIVE_CONTROL_ID_BYTES = 32,
    DB_NATIVE_CONTROL_CHANNEL_BYTES = 96,
};

typedef enum DbNativeControlCommand {
    DB_NATIVE_CONTROL_SET_PAD = 1,
    DB_NATIVE_CONTROL_CLEAR_PAD = 2,
    DB_NATIVE_CONTROL_SCREENSHOT = 3,
    DB_NATIVE_CONTROL_UI_CLICK = 4,
    DB_NATIVE_CONTROL_UI_TEXT = 5,
    DB_NATIVE_CONTROL_UI_DRAG = 6,
    DB_NATIVE_CONTROL_ROOM_ROTATION = 7,
    DB_NATIVE_CONTROL_UI_KEY = 8,
    DB_NATIVE_CONTROL_UI_MOVE = 9,
    DB_NATIVE_CONTROL_UI_INSPECT = 10,
    DB_NATIVE_CONTROL_ROOM_STAGE = 11,
} DbNativeControlCommand;

typedef struct DbNativeControlMessage {
    u32 magic;
    u32 command;
    u32 port;
    u32 frames;
    u32 buttons;
    s32 stick_x;
    s32 stick_y;
    s32 cstick_x;
    s32 cstick_y;
    u32 trigger_left;
    u32 trigger_right;
    u32 analog_a;
    u32 analog_b;
    u32 extensions;
    s32 ui_x;
    s32 ui_y;
    s32 ui_end_x;
    s32 ui_end_y;
    u32 rotation_policy;
    char text[128];
} DbNativeControlMessage;

bool dbNativeControl_FormatChannel(
    char output[DB_NATIVE_CONTROL_CHANNEL_BYTES], const char* id);

#endif
