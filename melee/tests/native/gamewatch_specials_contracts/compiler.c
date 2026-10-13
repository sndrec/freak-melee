#include "../../../tools/native_fighter_script.h"
#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static bool generate(const char* source, FILE* file, bool diagnose)
{
    NativeFighterScriptError error = { 0 };
    bool ok = nativeFighterScript_GenerateC(source, strlen(source),
        "gamewatch_contract_probe", NULL, NULL, NULL, file, &error);
    if (!ok && diagnose) fprintf(stderr, "%u: %s\n", error.line, error.message);
    return ok;
}

int main(int argc, char** argv)
{
    const char* positive =
        "state int hits\nstate bool air\nstate bool success\nstate float deadzone\n"
        "action Contract:\n    on load:\n"
        "        state.hits = absorber.hits\n"
        "        state.hits = absorber.damage\n"
        "        state.deadzone = controls.horizontal_stick_deadzone\n"
        "        state.air = targets.airborne(uint(0xFEDCBA98))\n"
        "        state.success = items.spawn_food(1.25, 2, -3)\n"
        "article It_Kind_GameWatch_Panic states 1:\n    state 0 anim -1:\n"
        "        on physics:\n            item.airborne = targets.airborne(uint(0xFEDCBA98))\n";
    FILE* file = tmpfile(); assert(file && generate(positive, file, true));
    assert(!fseek(file, 0, SEEK_END)); long n = ftell(file); assert(n > 0);
    rewind(file); char* output = malloc((size_t)n + 1); assert(output);
    assert(fread(output, 1, (size_t)n, file) == (size_t)n); output[n] = 0;
    assert(strstr(output, "AbsorbAttr.x1A48_hitsTaken"));
    assert(strstr(output, "NativeFighterCommon_HorizontalStickDeadzone"));
    assert(strstr(output, "4275878552"));
    assert(strstr(output, "host->target_airborne == 0"));
    assert(strstr(output, "host->spawn_food == 0"));
    assert(strstr(output, "host->common_attribute == 0"));
    assert(strstr(output, "host->callback_source_alive == 0"));
    const char* spawn = strstr(output, "engine->spawn_food(gobj"); assert(spawn);
    assert(strstr(spawn, "callback_source_alive(gobj, callback_generation, NativeCallbackSource_Fighter"));
    free(output); fclose(file);
    const char* invalid[] = {
        "action Bad:\n    on load:\n        absorber.hits = 2\n",
        "action Bad:\n    on load:\n        let int value = absorber.hits()\n",
        "action Bad:\n    on load:\n        controls.horizontal_stick_deadzone = 0.2\n",
        "action Bad:\n    on load:\n        let float value = controls.horizontal_stick_deadzone()\n",
        "action Bad:\n    on load:\n        let bool value = targets.airborne(1.5)\n",
        "action Bad:\n    on load:\n        let bool value = targets.airborne()\n",
        "action Bad:\n    on load:\n        let bool value = targets.airborne(1, 2)\n",
        "action Bad:\n    on load:\n        let bool value = items.spawn_food(1, 2)\n",
        "action Bad:\n    on load:\n        let bool value = items.spawn_food(1, 2, 3, 4)\n",
        "article It_Kind_GameWatch_Panic states 1:\n    state 0 anim -1:\n        on physics:\n            item.airborne = targets.airborne(1.5)\n",
        "article It_Kind_GameWatch_Panic states 1:\n    state 0 anim -1:\n        on physics:\n            item.airborne = targets.airborne(1, 2)\n",
        "article It_Kind_GameWatch_Panic states 1:\n    state 0 anim -1:\n        on physics:\n            let bool value = items.spawn_food(1, 2, 3)\n",
        "model_effect Bad 0x509:\n    on update:\n        let bool value = targets.airborne(1)\naction Probe:\n    on load:\n        pass\n",
        "model_effect Bad 0x509:\n    on update:\n        let bool value = items.spawn_food(1, 2, 3)\naction Probe:\n    on load:\n        pass\n",
    };
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        file = tmpfile(); assert(file); assert(!generate(invalid[i], file, false)); fclose(file);
    }
    if (argc == 3) {
        file = fopen(argv[1], "rb"); assert(file);
        assert(!fseek(file, 0, SEEK_END)); n = ftell(file); assert(n > 0); rewind(file);
        char* source = malloc((size_t)n + 1); assert(source);
        assert(fread(source, 1, (size_t)n, file) == (size_t)n); source[n] = 0; fclose(file);
        file = fopen(argv[2], "wb"); assert(file);
        bool ok = generate(source, file, true); fclose(file); free(source); if (!ok) return 1;
    } else assert(argc == 1);
    puts("Absorber/common input/live target/ordinary food compiler contracts PASS");
    return 0;
}
