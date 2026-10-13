#include "../../tools/native_fighter_script.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool emit(bool callbacks, const char* path)
{
    char source[8192];
    snprintf(source, sizeof(source),
        "%sstate int cursor\nstate float values[16]\nstate bool enabled[2]\nstate bool latched\n"
        "func CreatorTouch(float amount):\n    state.values[2] = amount\n"
        "%s"
        "        state.values[state.cursor] = velocity.x\n"
        "        state.enabled[0] = input.attack_pressed\n"
        "        state.latched = input.attack_pressed\n"
        "        guard.front_only = input.attack_pressed\n"
        "        velocity.y = state.values[state.cursor]\n"
        "        state.cursor = state.cursor + 1\n"
        "article It_Kind_Mario_Fire states 1:\n"
        "    state float history[128]\n"
        "    state 0 anim -1:\n"
        "        on physics:\n"
        "            item.state.history[0x7F] = owner.state.values[0x0]\n"
        "            owner.state.values[1] = item.state.history[127] + 1\n"
        "            velocity.x = item.state.history[127]\n"
        "            creator.state.values[3] = creator.state.values[0] + 3\n"
        "            creator.state.cursor = creator.state.cursor\n"
        "            creator.state.enabled[1] = creator.exists\n"
        "            creator.state.latched = item.damage_behavior\n"
        "            item.harm_owner = item.damage_behavior\n"
        "            creator.call(CreatorTouch, creator.position.x)\n",
        callbacks ? "" : "",
        callbacks ? "move Arrays:\n    on state_341_physics:\n" : "action Arrays:\n    on load:\n");
    FILE* output = path ? fopen(path, "w") : tmpfile();
    if (!output) return false;
    NativeFighterScriptError error = { 0 };
    bool result = nativeFighterScript_GenerateC(source, strlen(source),
        "state_array_probe", NULL, NULL, callbacks ? &(const NativeFighterScriptConstruction){"ftDataMario",341,1} : NULL, output, &error);
    if (!result) fprintf(stderr, "State array %s fixture failed at %u: %s\n",
                         callbacks ? "callback" : "action", error.line, error.message);
    fclose(output);
    return result;
}

static void invalid(const NativeFighterScriptConstruction* construction)
{
    static const char* lines[] = {
        "item.state.history", "item.state.history[-1]", "item.state.history[16]",
        "item.state.history[0.5]", "item.state.scalar[0]", "item.state.history[]",
        "item.state.history[unknown]", "item.state.history[0", "item.state.history[0]]"
    };
    for (unsigned i = 0; i < sizeof(lines) / sizeof(lines[0]); ++i) {
        char source[2048];
        snprintf(source, sizeof(source),
            "article It_Kind_Mario_Fire states 1:\n"
            "    state float history[16]\n    state bool scalar\n"
            "    state 0 anim -1:\n        on physics:\n"
            "            velocity.x = %s\n", lines[i]);
        FILE* output = tmpfile(); assert(output);
        NativeFighterScriptError error = { 0 };
        assert(!nativeFighterScript_GenerateC(source, strlen(source),
            "state_array_invalid", NULL, NULL, construction, output, &error));
        fclose(output);
    }
}

int main(int argc, char** argv)
{
    if (argc != 1 && argc != 3) return 2;
    assert(emit(false, argc == 3 ? argv[1] : NULL));
    assert(emit(true, argc == 3 ? argv[2] : NULL));
    invalid(NULL);
    puts("Fighter/article typed arrays generate in both compiler modes and reject invalid accesses");
    return 0;
}
