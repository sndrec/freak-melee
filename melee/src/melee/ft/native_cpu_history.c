#include "native_cpu_history.h"
#include "ftlib.h"
#include "native_callback_lifetime.h"
#include "types.h"
#include <math.h>
#include <sysdolphin/baselib/gobj.h>

static bool cursor_index(const struct CpuFighter* data,
                         const struct Fighter_x1A88_xFC_t* cursor,
                         unsigned* index)
{
    uintptr_t start = (uintptr_t)data->xFC;
    uintptr_t address = (uintptr_t)cursor;
    if (address < start || address - start >= sizeof(data->xFC) ||
        (address - start) % sizeof(data->xFC[0]) != 0) return false;
    *index = (unsigned)((address - start) / sizeof(data->xFC[0]));
    return true;
}

static Fighter* live_receiver(Fighter_GObj* object)
{
    uint32_t generation;
    return nativeCallbackSource_Snapshot(object, NativeCallbackSource_Fighter,
                                         &generation) ? object->user_data : NULL;
}

bool nativeCpuHistory_Record(Fighter_GObj* receiver, uint32_t input_source)
{
    Fighter* fighter = live_receiver(receiver);
    Fighter_GObj* source = input_source != 0 ? ftLib_8008741C(input_source) : NULL;
    if (fighter == NULL || source == NULL || source->user_data == NULL) return false;
    const Fighter* input = source->user_data;
    struct CpuFighter* data = &fighter->cpu;
    unsigned write, read;
    if (!cursor_index(data, data->x444, &write) ||
        !cursor_index(data, data->x448, &read)) return false;
    const float axes[] = { input->input.lstick[0].x, input->input.lstick[0].y,
                           input->input.cstick[0].x, input->input.cstick[0].y };
    for (unsigned i = 0; i < 4; ++i)
        if (!isfinite(axes[i]) || axes[i] < -1.0f || axes[i] > 1.0f) return false;
    float trigger = input->input.triggers[0];
    if (!isfinite(trigger) || trigger <= -1.0f || trigger >= 256.0f ||
        !isfinite(input->cur_pos.x) || !isfinite(input->cur_pos.y) ||
        !isfinite(input->cur_pos.z) || !isfinite(input->facing_dir)) return false;
    write = (write + 1) % (sizeof(data->xFC) / sizeof(data->xFC[0]));
    read = (read + 1) % (sizeof(data->xFC) / sizeof(data->xFC[0]));
    // Preserve xA/xB: the original recording body never writes them.
    struct Fighter_x1A88_xFC_t record = data->xFC[write];
    s8 quantized[4];
    for (unsigned i = 0; i < 4; ++i) {
        // Native decompiled inlineM0 returned negative Float through u8 (UB).
        // Truncate signed arithmetic explicitly; ordinary signed input intent.
        quantized[i] = (s8)(int)(axes[i] * (axes[i] >= 0.0f ? 127.0f : 128.0f));
    }
    record.lstick = (S8Vec2){ quantized[0], quantized[1] };
    record.cstick = (S8Vec2){ quantized[2], quantized[3] };
    // Both source bytes use triggers[0], directly truncated; no255 scaling.
    // Fractional values in (-1,256) have a representable truncated u8 value.
    record.x4 = record.x5 = (u8)trigger;
    record.x0 = input->input.held_buttons[0];
    record.cur_pos = input->cur_pos;
    record.facing_dir = input->facing_dir;
    data->xFC[write] = record;
    data->x444 = &data->xFC[write];
    data->x448 = &data->xFC[read];
    return true;
}

bool nativeCpuHistory_ReadPose(Fighter_GObj* receiver, Vec3* position, float* facing)
{
    if (position == NULL || facing == NULL) return false;
    Fighter* fighter = live_receiver(receiver);
    unsigned index;
    if (fighter == NULL || !cursor_index(&fighter->cpu, fighter->cpu.x448, &index))
        return false;
    const struct Fighter_x1A88_xFC_t* record = &fighter->cpu.xFC[index];
    Vec3 value = record->cur_pos;
    float direction = record->facing_dir;
    if (!isfinite(value.x) || !isfinite(value.y) || !isfinite(value.z) ||
        !isfinite(direction)) return false;
    *position = value;
    *facing = direction;
    return true;
}

bool nativeCpuHistory_Apply(Fighter_GObj* receiver)
{
    Fighter* fighter = live_receiver(receiver);
    unsigned index;
    if (fighter == NULL || !cursor_index(&fighter->cpu, fighter->cpu.x448, &index))
        return false;
    struct CpuFighter* data = &fighter->cpu;
    const struct Fighter_x1A88_xFC_t* record = &data->xFC[index];
    data->lstick = record->lstick;
    data->cstick = record->cstick;
    data->buttons = record->x0;
    data->ltrigger = record->x4;
    data->rtrigger = record->x5;
    return true;
}
