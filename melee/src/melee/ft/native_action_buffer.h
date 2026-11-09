#ifndef MELEE_FT_NATIVE_ACTION_BUFFER_H
#define MELEE_FT_NATIVE_ACTION_BUFFER_H

#include <melee/ft/forward.h>
#include <sysdolphin/baselib/controller.h>
#include <dolphin/mtx.h>

typedef struct NativeActionStick {
    Vec2 position;
    u8 tilt_x;
    u8 tilt_y;
    u8 frames;
} NativeActionStick;

typedef struct NativeActionBuffer {
    NativeActionStick movement;
    NativeActionStick direction;
    Vec2 cstick;
    HSD_Pad pressed;
    HSD_Pad held;
    float trigger;
    bool cstick_command;
    bool checking;
    bool replaying_x;
} NativeActionBuffer;

typedef struct NativeActionInput {
    Vec2 lstick;
    Vec2 cstick;
    Vec2 previous_cstick;
    HSD_Pad pressed;
    HSD_Pad held;
    float trigger;
    u8 tilt_x;
    u8 tilt_y;
    u8 special_up;
    u8 special_down;
    u8 special_side;
    u8 special_neutral;
} NativeActionInput;

void nativeActionBuffer_Record(Fighter* fp, unsigned frames);
bool nativeActionBuffer_Begin(Fighter* fp, NativeActionInput* live,
                             NativeActionInput* offered);
void nativeActionBuffer_End(Fighter* fp, const NativeActionInput* live,
                           const NativeActionInput* offered);

#endif
