#include "../../tools/native_fighter_script.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

static bool generate(const char* text, const char* path, const NativeFighterScriptConstruction* construction)
{
    FILE* output = path ? fopen(path, "w") : tmpfile();
    assert(output);
    NativeFighterScriptError error = { 0 };
    bool result = nativeFighterScript_GenerateC(text, strlen(text),
        "held_motion_probe", NULL, NULL, construction, output, &error);
    if (path && !result) fprintf(stderr, "%u: %s\n", error.line, error.message);
    fclose(output);
    return result;
}

int main(int argc, char** argv)
{
    assert(argc == 1 || argc == 3);
    for (unsigned form = 0; form < 2; ++form) {
        char text[4096];
        snprintf(text, sizeof(text),
            "%sstate int published\nstate bool continued\n"
            "%s"
            "    on held_motion:\n"
            "        if event.previous_motion == Motion.fall_special:\n"
            "            return 341\n"
            "        if event.previous_motion == 55:\n"
            "            state.held_open(1.5)\n"
            "            state.continued = true\n"
            "            return 342\n"
            "        if event.previous_motion == 56:\n"
            "            return event.previous_motion\n"
            "        state.published = event.previous_motion\n"
            "    on load:\n"
            "        motion.id = state.published\n",
            form ? "" : "",
            form ? "move HeldMotion:\n" : "action HeldMotion:\n");
        assert(generate(text, argc == 3 ? argv[form + 1] : NULL, form ? &(const NativeFighterScriptConstruction){"ftDataMario",341,2} : NULL));
    }
    const char* invalid[] = {
        "action Bad:\n    on held_motion:\n        return true\n",
        "action Bad:\n    on held_motion:\n        return 1.5\n",
        "action Bad:\n    on load:\n        motion.id = 1.5\n",
        "action Bad:\n    on load:\n        motion.id = true\n",
        "action Bad:\n    on load:\n        motion.id = event.previous_motion\n",
        "action Bad:\n    on load:\n        state.held_open()\n",
        "action Bad:\n    on load:\n        state.held_open(1, 2)\n"
    };
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
        assert(!generate(invalid[i], NULL, &(const NativeFighterScriptConstruction){"ftDataMario",341,2}));
    puts("Held motion callback, raw identity publication and shared entry compile in both modes");
    return 0;
}
