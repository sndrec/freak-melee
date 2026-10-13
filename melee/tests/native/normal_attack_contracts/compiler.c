#include "../../../tools/native_fighter_script.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; assert(x); } while (0)

static char* read(FILE* file)
{
    CHECK(fseek(file, 0, SEEK_END) == 0);
    long size = ftell(file); CHECK(size >= 0);
    rewind(file);
    char* text = malloc((size_t)size + 1); CHECK(text);
    CHECK(fread(text, 1, size, file) == (size_t)size);
    text[size] = 0;
    return text;
}

static bool generate(const char* source, FILE* output, const NativeFighterScriptConstruction* construction)
{
    NativeFighterScriptError error = { 0 };
    bool valid = nativeFighterScript_GenerateC(source, strlen(source),
        "normal_attack_contracts", NULL, NULL, construction, output, &error);
    if (!valid) fprintf(stderr, "Line %u: %s\n", error.line, error.message);
    return valid;
}

int main(int argc, char** argv)
{
    CHECK(argc == 4);
    FILE* input = fopen(argv[1], "rb"); CHECK(input);
    char* source = read(input); fclose(input);
    for (unsigned mode = 0; mode < 2; ++mode) {
        char* program = malloc(strlen(source) + 64); CHECK(program);
        if (mode) {
            strcpy(program, ""); strcat(program, source);
            char* action = strstr(program, "action NormalEntries:"); CHECK(action);
            memmove(action + 4, action + 6, strlen(action + 6) + 1);
            memcpy(action, "move", 4);
        } else strcpy(program, source);
        FILE* output = fopen(argv[mode + 2], "w+b"); CHECK(output);
        CHECK(generate(program, output, mode ? &(const NativeFighterScriptConstruction){"ftDataMario",341,12} : NULL));
        char* code = read(output);
        const char* expected[] = { "on_jab_enter", "on_rapid_jab_enter",
            "on_down_tilt_enter", "on_aerial_attack_motion", "return motion;",
            "engine->enter_jab", "engine->enter_rapid_jab",
            "engine->update_rapid_jab", "engine->try_ground_pickup",
            "engine->air_aerial_landing_collision", "engine->enter_landing_aerial",
            "host->debug_level == 0", "host->callback_source_alive == 0",
            "fighter->x8_spawnNum", "script_fn_FinishRapid", "script_fn_Land" };
        for (unsigned i = 0; i < sizeof(expected)/sizeof(expected[0]); ++i)
            CHECK(strstr(code, expected[i]));
        CHECK(!strstr(code, "ftGw_"));
        free(code); free(program); fclose(output);
    }
    const char* invalid[] = {
        "action X:\n    on aerial_attack_motion:\n        return 1.5\n",
        "action X:\n    on aerial_attack_motion:\n        return event.victim_motion\n",
        "action X:\n    on jab_enter:\n        let bool x = attack.enter_jab(1.5)\n",
        "action X:\n    on rapid_jab_enter:\n        attack.enter_rapid_jab(1.5)\n",
        "action X:\n    on load:\n        state.landing_aerial(1.5, 4)\n",
        "action X:\n    on load:\n        attack.update_rapid_jab(None)\n",
        "action X:\n    on load:\n        collision.air_aerial_landing_callback(None)\n",
        "func End(int value):\n    pass\naction X:\n    on load:\n        attack.update_rapid_jab(End)\n",
        "func End() -> int:\n    return 1\naction X:\n    on load:\n        collision.air_aerial_landing_callback(End)\n",
        "action X:\n    on load:\n        fighter.creation_id = 1\n",
        "action X:\n    on load:\n        engine.debug_level = 1\n",
        "action X:\n    on load:\n        stats.landing_air_neutral_lag = 1\n",
        "action X:\n    on load:\n        let int x = engine.debug_level()\n",
        "article It_Kind_Ness_PKFire states 1:\n    state 0 anim -1:\n        on physics:\n            let bool x = attack.enter_jab(341)\n",
        "article It_Kind_Ness_PKFire states 1:\n    state 0 anim -1:\n        on physics:\n            let int x = fighter.creation_id\n",
    };
    for (unsigned i = 0; i < sizeof(invalid)/sizeof(invalid[0]); ++i) {
        FILE* output = tmpfile(); CHECK(output);
        CHECK(!generate(invalid[i], output, NULL)); fclose(output);
    }
    free(source);
    printf("PASS %u normal-entry compiler checks\n", checks);
    return 0;
}
