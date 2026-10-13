#include "native_fighter_script_sound.h"
#include <stdio.h>
#include <string.h>

int nativeScriptSound_Action(const ScriptCodeBlock* block, const char* source,
                            char* output, size_t capacity)
{
    static const char name[] = "sound.channel";
    if (strncmp(source, name, sizeof(name) - 1) ||
        (source[sizeof(name) - 1] != '(' && source[sizeof(name) - 1] != 0))
        return 0;
    if (!block || block->item_event || block->model_effect) return -1;
    char args[4][192], values[4][192];
    const char* after;
    if (!nativeScriptCode_ParseArguments(source + sizeof(name) - 1, args, 4,
                                         &after) || *after) return -1;
    for (unsigned i = 0; i < 4; ++i) {
        if (nativeScriptCode_ExpressionType(block, args[i]) != ScriptType_Int ||
            !nativeScriptCode_Expression(block, args[i], values[i],
                                         sizeof(values[i]))) return -1;
    }
    // Separate declarations preserve source order and let the common lifetime
    // emitter guard each effectful argument before evaluating the next one.
    int length = snprintf(output, capacity,
        "{ int32_t sound_channel = (%s); int32_t sound_id = (%s); "
        "int32_t sound_volume = (%s); int32_t sound_pan = (%s); "
        "if ((uint32_t)sound_channel < NATIVE_FIGHTER_SOUND_CHANNEL_COUNT) "
        "engine->sound_channels[sound_channel](fighter,sound_id,"
        "(uint8_t)sound_volume,(uint8_t)sound_pan); }",
        values[0], values[1], values[2], values[3]);
    return length >= 0 && (size_t)length < capacity ? 1 : -1;
}
