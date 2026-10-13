#include <melee/gm/native_match_control_history.h>
#include "gm_1A36.h"

#include "gm_1A36.static.h"
#include "gmscdata.h"
#include "types.h"
#include <sysdolphin/baselib/controller.h>
#include <stdint.h>
#include <string.h>

static unsigned input_activity;

u64 gm_GetButtonsPressed(u8 idx)
{
    return controller_map.ports[idx].button;
}

u64 gm_GetButtonsTriggered(u8 idx)
{
    return controller_map.ports[idx].trigger;
}

u64 gm_801A36C0(u8 idx)
{
    return controller_map.ports[idx].repeat2;
}

void gm_801A36E0(s32 idx, s32 arg1)
{
    int i;
    if (idx == PAD_MAX_CONTROLLERS) {
        for (i = 0; i < PAD_MAX_CONTROLLERS; i++) {
            controller_map.ports[i].repeat_timer = arg1;
        }
        return;
    }
    controller_map.ports[idx].repeat_timer = arg1;
}

void gm_801A3714(s32 idx, u64 arg1, u64 arg2)
{
    if (arg1 & controller_map.ports[idx].button) {
        controller_map.ports[idx].button |= arg2;
    }
    if (arg1 & controller_map.ports[idx].repeat) {
        controller_map.ports[idx].repeat |= arg2;
    }
    if (arg1 & controller_map.ports[idx].trigger) {
        controller_map.ports[idx].trigger |= arg2;
    }
    if (arg1 & controller_map.ports[idx].release) {
        controller_map.ports[idx].release |= arg2;
    }
}

static void gm_801A3820(s32 idx, u64 arg1, u64 arg2)
{
    struct gm_controller_map* controller = controller_map.ports;
    if (arg1 == (arg1 & controller[idx].button)) {
        controller[idx].button |= arg2;
    }
    if (arg1 == (arg1 & controller[idx].repeat)) {
        controller[idx].repeat |= arg2;
    }
    if (arg1 == (arg1 & controller[idx].button)) {
        if (arg1 & controller[idx].trigger) {
            controller[idx].trigger |= arg2;
        }
    }
    if (arg1 == (arg1 & controller[idx].button)) {
        if (arg1 & controller[idx].release) {
            controller[idx].release |= arg2;
        }
    }
}

static void fn_801A396C(int idx)
{
    NativeGmInputSnapshot* controller = &controller_map;
    if (controller_map.ports[idx].trigger || controller_map.ports[idx].release) {
        controller_map.ports[idx].repeat2 = controller->ports[idx].trigger;
        controller_map.ports[idx].repeat_timer = controller->repeat_delay;
        controller_map.ports[idx].held_frames = 0;
        return;
    }
    if (controller_map.ports[idx].held_frames < controller->fast_threshold) {
        controller_map.ports[idx].held_frames++;
    }

    if (controller_map.ports[idx].repeat_timer) {
        controller_map.ports[idx].repeat_timer--;
        controller_map.ports[idx].repeat2 = 0;
        return;
    }

    controller_map.ports[idx].repeat2 = controller_map.ports[idx].button;

    if (controller_map.ports[idx].held_frames >= controller_map.fast_threshold) {
        controller_map.ports[idx].repeat_timer = controller_map.fast_interval;
    } else if (controller_map.ports[idx].held_frames >= controller_map.middle_threshold) {
        controller_map.ports[idx].repeat_timer = controller_map.middle_interval;
    } else {
        controller_map.ports[idx].repeat_timer = controller_map.early_interval;
    }
}

#ifdef MUST_MATCH
#pragma push
#pragma dont_inline on
#endif
void gm_EvaluateAllControllerInputs(void)
{
    struct gm_controller_map* controller = controller_map.ports;
    int i;

    PAD_STACK(0x10);
    ++input_activity;

    for (i = 0; i < PAD_MAX_CONTROLLERS; i++) {
        controller_map.ports[i].button = HSD_PadCopyStatus[(u8) i].button;
        controller_map.ports[i].trigger = HSD_PadCopyStatus[(u8) i].trigger;
        controller_map.ports[i].repeat = HSD_PadCopyStatus[(u8) i].repeat;
        controller_map.ports[i].release = HSD_PadCopyStatus[(u8) i].release;
        gm_801A3714(i, PAD_BUTTON_A | PAD_BUTTON_START, PAD_CONFIRM);
        gm_801A3714(i, PAD_BUTTON_B, PAD_CANCEL);
        gm_801A3820(i, PAD_TRIGGER_L | PAD_TRIGGER_R | PAD_BUTTON_START,
                    PAD_LR_START);
        gm_801A3820(
            i, PAD_TRIGGER_L | PAD_TRIGGER_R | PAD_BUTTON_A | PAD_BUTTON_START,
            PAD_LRA_START);
        gm_801A3714(i, PAD_BUTTON_UP | PAD_STICK_UP, PAD_ANY_UP);
        gm_801A3714(i, PAD_BUTTON_DOWN | PAD_STICK_DOWN, PAD_ANY_DOWN);
        gm_801A3714(i, PAD_BUTTON_LEFT | PAD_STICK_LEFT, PAD_ANY_LEFT);
        gm_801A3714(i, PAD_BUTTON_RIGHT | PAD_STICK_RIGHT, PAD_ANY_RIGHT);
        controller_map.repeat_callback(i);
    }
    controller_map.ports[PAD_MAX_CONTROLLERS].button = 0;
    controller_map.ports[PAD_MAX_CONTROLLERS].trigger = 0;
    controller_map.ports[PAD_MAX_CONTROLLERS].repeat = 0;
    controller_map.ports[PAD_MAX_CONTROLLERS].release = 0;
    controller_map.ports[PAD_MAX_CONTROLLERS].repeat2 = 0;

    for (i = 0; i < PAD_MAX_CONTROLLERS; i++) {
        controller[PAD_MAX_CONTROLLERS].button |= controller[i].button;
        controller[PAD_MAX_CONTROLLERS].trigger |= controller[i].trigger;
        controller[PAD_MAX_CONTROLLERS].repeat |= controller[i].repeat;
        controller[PAD_MAX_CONTROLLERS].release |= controller[i].release;
        controller[PAD_MAX_CONTROLLERS].repeat2 |= controller[i].repeat2;
    }
    --input_activity;
}
#ifdef MUST_MATCH
#pragma pop
#endif

void gm_801A3E88(void)
{
    int i;
    static NativeGmInputSnapshot gm_803DA788 = {
        { 0 }, NULL, 0x14, 0x8, 0x28, 0x4, 0x64, 0x2,
    };

    controller_map = gm_803DA788;

    for (i = 0; i <= PAD_MAX_CONTROLLERS; i++) {
        controller_map.ports[i].repeat_timer = controller_map.repeat_delay;
    }
    controller_map.repeat_callback = fn_801A396C;
}

void gm_801A3EF4(void)
{
    GameMode* scene;
    for (scene = gm_GetAllGameModes(); scene->kind != GM_COUNT; scene++) {
        if (scene->on_init != NULL) {
            scene->on_init();
        }
    }
}

static bool mapped_input_buffer(const void* data, size_t bytes)
{
    uintptr_t at = (uintptr_t)data, root = (uintptr_t)&controller_map;
    uintptr_t activity = (uintptr_t)&input_activity;
    return data && bytes && bytes <= UINTPTR_MAX-at &&
        !(at < root+sizeof(controller_map) && root < at+bytes) &&
        !(at < activity+sizeof(input_activity) && activity < at+bytes);
}

bool nativeGmInput_ValidateSnapshot(const void* data, size_t bytes)
{
    if (input_activity || bytes != sizeof(NativeGmInputSnapshot) ||
        !mapped_input_buffer(data, bytes)) return false;
    NativeGmInputSnapshot state;
    memcpy(&state, data, sizeof(state));
    // The only assignment installs this algorithm. The all-zero pre-init state
    // is also capturable, but cannot become a callable custom code pointer.
    if (!state.repeat_callback) {
        const unsigned char* raw = (const unsigned char*)&state;
        for (size_t i = 0; i < sizeof(state); ++i) if (raw[i]) return false;
        return true;
    }
    return state.repeat_callback == fn_801A396C;
}

bool nativeGmInput_Save(void* data, size_t capacity, size_t* written)
{
    size_t bytes = sizeof(controller_map);
    if (input_activity || capacity < bytes || !mapped_input_buffer(data, bytes) ||
        !mapped_input_buffer(written, sizeof(*written))) return false;
    uintptr_t a = (uintptr_t)data, b = (uintptr_t)written;
    if (a < b+sizeof(*written) && b < a+bytes) return false;
    NativeGmInputSnapshot state;
    memcpy(&state, &controller_map, bytes);
    if (!nativeGmInput_ValidateSnapshot(&state, bytes)) return false;
    memcpy(data, &state, bytes);
    *written = bytes;
    return true;
}

bool nativeGmInput_Load(const void* data, size_t bytes)
{
    if (!nativeGmInput_ValidateSnapshot(data, bytes)) return false;
    memcpy(&controller_map, data, bytes);
    return true;
}

#ifdef MELEE_NATIVE
bool nativeGmInput_AdmitHistory(NativeMatchArena* arena)
{
    if(!arena || !arena->lifetime)return false;
#define ROOT(v) do { if(!nativeMatchArena_AddRoot(arena,&(v),sizeof(v)))return false; } while(0)
    ROOT(controller_map);
#undef ROOT
    return true;
}
#endif
