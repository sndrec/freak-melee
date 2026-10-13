#include "../../tools/native_fighter_script.h"
#include "../../tools/native_fighter_script_functions.h"
#include "../../tools/native_fighter_script_signal.h"
#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static bool generate(const char* source, const char* path, char* code, size_t capacity, const NativeFighterScriptConstruction* construction)
{
    FILE* output = path ? fopen(path, "w+") : tmpfile();
    assert(output);
    NativeFighterScriptError error = { 0 };
    bool valid = nativeFighterScript_GenerateC(source, strlen(source),
        "callback_exports_probe", NULL, NULL, construction, output, &error);
    if (!valid && code) fprintf(stderr, "Exports at%u: %s\n", error.line, error.message);
    if (valid && code) {
        rewind(output);
        size_t size = fread(code, 1, capacity - 1, output);
        assert(feof(output)); code[size] = 0;
    }
    fclose(output);
    return valid;
}

static void positive(bool callbacks, const char* path)
{
    char source[4096], code[65536];
    snprintf(source, sizeof(source),
        "%sstate int received\nstate int visits\n"
        "export func clear_resource():\n"
        "    state.received = 16777217\n    record()\n"
        "func record():\n    state.visits = state.visits + 1\n"
        "export func apply_friction():\n    physics.ground_friction()\n"
        "    let int recipient_motion = targets.motion(16777217)\n"
        "export func acknowledge():\n    state.received = -1\n"
        "export func is_ready() -> bool:\n    return state.received != 0\n"
        "export func precise_id() -> int:\n    return 16777217\n"
        "func scale(float amount) -> float:\n    return amount * 1.25\n"
        "export func speed() -> float:\n    return scale(1.6)\n"
        "func next() -> int:\n    state.received = state.received + 1\n    return state.received\n"
        "func combine(int a, int b) -> int:\n    return a * 10 + b\n"
        "export func ordered() -> int:\n    state.received = 0\n    return combine(next(), next())\n"
        "%s        state.visits = 7\n",
        callbacks ? "" : "",
        callbacks ? "move Exports:\n    on state_341_physics:\n" :
                    "action Exports:\n    on load:\n");
    assert(generate(source, path, code, sizeof(code), callbacks ? &(const NativeFighterScriptConstruction){"ftDataNess",341,1} : NULL));
    assert(strstr(code, "generated.callback_export_count = 7;"));
    assert(strstr(code, "generated.callback_exports = script_callback_exports;"));
    assert(strstr(code, "host->ground_friction == 0")); // Export has no local caller.
    const char* table = strstr(code, "static const NativeFighterCallbackExport script_callback_exports[]");
    assert(table);
    const char* previous = table;
    const char* names[] = { "acknowledge", "apply_friction", "clear_resource" };
    unsigned order[] = { 0, 1, 2 };
    for (unsigned i = 0; i < 3; ++i)
        for (unsigned j = i + 1; j < 3; ++j)
            if (nativeScriptSignal_Hash(names[order[j]]) < nativeScriptSignal_Hash(names[order[i]])) {
                unsigned swap = order[i]; order[i] = order[j]; order[j] = swap;
            }
    for (unsigned i = 0; i < 3; ++i) {
        char entry[128];
        snprintf(entry, sizeof(entry), ".name_hash = UINT32_C(%u), .callback = script_fn_%s,",
            nativeScriptSignal_Hash(names[order[i]]), names[order[i]]);
        const char* at = strstr(table, entry);
        assert(at && at > previous); previous = at;
    }
    assert(!strstr(table, "script_fn_record"));
    assert(strstr(code, "static void script_fn_clear_resource(HSD_GObj* gobj)"));
    assert(strstr(code, "script_fn_record(gobj);"));

    snprintf(source, sizeof(source), "%sfunc private_only():\n    pass\n%s        pass\n",
        callbacks ? "" : "",
        callbacks ? "move Exports:\n    on state_341_physics:\n" :
                    "action Exports:\n    on load:\n");
    assert(generate(source, NULL, code, sizeof(code), callbacks ? &(const NativeFighterScriptConstruction){"ftDataNess",341,1} : NULL));
    assert(strstr(code, "generated.callback_export_count = 0;"));
    assert(strstr(code, "generated.callback_exports = NULL;"));
    assert(!strstr(code, "static const NativeFighterCallbackExport"));

    // An export-only action program still binds its engine and storage.
    assert(generate("state int received\nexport func receive():\n"
                    "    physics.ground_friction()\n    state.received = 1\n",
                    NULL, code, sizeof(code), NULL));
    assert(strstr(code, "static const NativeFighterHost* engine;"));
    assert(strstr(code, "engine = host;"));
}

static void rejection(const NativeFighterScriptConstruction* construction)
{
    const char* declarations[] = {
        "export func transfer(int id):\n    pass\n",
        "export func transfer(float amount):\n    pass\n",
        "export func transfer(bool flag):\n    pass\n",
        "export func bad-name():\n    pass\n",
        "export func export():\n    pass\n",
        "export func missing():\n",
        "export func same():\n    pass\nfunc same():\n    pass\n",
        "func same():\n    pass\nexport func same():\n    pass\n",
        "export func same():\n    pass\nexport func same():\n    pass\n",
        "export func costarring():\n    pass\nexport func liquid():\n    pass\n",
        "export func zero_hash_50Fbk5():\n    pass\n",
        "export func first():\n    pass\nfighter:\n    behavior independent\n",
        "export func recursive():\n    recursive()\n",
        "func invalid() -> text:\n    return 1\n",
        "func no_result() -> int:\n    pass\n",
        "func partial() -> int:\n    if grounded: return 1\n",
        "func bare() -> bool:\n    return\n",
        "func narrow() -> int:\n    return 1.5\n",
        "func void_value():\n    return 1\n",
        "func typed() -> bool:\n    return true\nfunc bind():\n    callbacks.physics(typed)\n",
        "func Value() -> int:\n    return 7\nfunc invalid():\n    let float bad = Value().frame\n",
        "func Value() -> bool:\n    return true\nfunc invalid():\n    let bool bad = Value()[0]\n",
        "func invalid():\n    let int value = targets.motion(1.5)\n",
        "func invalid():\n    let int value = targets.motion(true)\n",
    };
    assert(nativeScriptSignal_Hash("costarring") == nativeScriptSignal_Hash("liquid"));
    assert(nativeScriptSignal_Hash("zero_hash_50Fbk5") == 0);
    for (unsigned i = 0; i < sizeof(declarations) / sizeof(declarations[0]); ++i) {
        char source[4096];
        snprintf(source, sizeof(source), "%saction Probe:\n    on load:\n        pass\n", declarations[i]);
        assert(!generate(source, NULL, NULL, 0, construction));
    }
    char source[16384]; size_t used = 0;
    for (unsigned i = 0; i < SCRIPT_FUNCTIONS; ++i)
        used += snprintf(source + used, sizeof(source) - used,
                         "export func callback_%u():\n    pass\n", i);
    assert(generate(source, NULL, NULL, 0, construction));
    snprintf(source + used, sizeof(source) - used, "export func overflow():\n    pass\n");
    assert(!generate(source, NULL, NULL, 0, construction));
}

static void raw_emission(void)
{
    char code[65536];
    assert(generate("move Hit:\n    on state_341_physics:\n"
                    "        callbacks.on_hit_state(341)\n", NULL, code, sizeof(code), &(const NativeFighterScriptConstruction){"ftDataNess",341,1}));
    const char* hit = strstr(code, "static void authored_on_hit(HSD_GObj* gobj) {");
    assert(hit && strstr(hit, "const uint32_t callback_generation"));
    assert(strstr(hit, "engine->change_motion_state(gobj, target"));
    assert(strstr(hit, "engine->callback_source_alive(gobj, callback_generation"));
    assert(strstr(code, "host->callback_source_alive == 0"));

    const char* cue =
        "move Cue:\n    on special_n:\n        state.enter(NeutralSpecial)\n"
        "    on special_air_n:\n        state.enter(NeutralSpecial)\n"
        "    on ground_animation:\n        pass\n    on air_animation:\n        pass\n"
        "    on ground_interrupt:\n        pass\n    on air_interrupt:\n        pass\n"
        "    on ground_physics:\n        pass\n    on air_physics:\n        pass\n"
        "    on ground_collision:\n        pass\n    on air_collision:\n        pass\n"
        "    on neutral_special_cue:\n        articles.spawn(MarioFireball, LeftHand)\n"
        "        let bool sent = targets.call(0, \"missing\")\n"
        "        motion.set_facing_speed(1.5)\n";
    assert(generate(cue, NULL, code, sizeof(code), NULL));
    const char* spawn = strstr(code, "engine->spawn_projectile(gobj, &article_position");
    assert(spawn);
    const char* guard = strstr(spawn, "engine->callback_source_alive(gobj, callback_generation");
    const char* speed = strstr(spawn, "fighter->gr_vel =");
    assert(guard && speed && guard < speed);
}

static void effects(void)
{
    char source[] =
        "export func entry():\n    middle()\n"
        "func unrelated():\n    pass\n"
        "func middle():\n    leaf()\n"
        "func leaf():\n    pass\n";
    NativeFighterScriptError error = { 0 };
    ScriptFunctionProgram* functions = nativeScriptFunctions_Parse(source, strlen(source), &error);
    assert(functions && functions->count == 4 && functions->export_count == 1);
    // Direct-effect recognition is tested by the lifetime compiler suite. This
    // checks graph propagation independently, including forward declarations.
    functions->functions[3].code.may_invalidate_source = true;
    ScriptStateTable state = { 0 };
    assert(nativeScriptFunctions_Compile(functions, &state, NULL, false, 0, 0, &error));
    assert(functions->functions[0].may_invalidate_source);
    assert(!functions->functions[1].may_invalidate_source);
    assert(functions->functions[2].may_invalidate_source);
    assert(functions->functions[3].may_invalidate_source);
    free(functions);
}

static void bounded_output(void)
{
    char source[]="func Value() -> int:\n    return 7\n"
        "func Pair(int a, int b) -> int:\n    return a + b\n";
    NativeFighterScriptError error={0};
    ScriptFunctionProgram* functions=nativeScriptFunctions_Parse(source,strlen(source),&error);
    assert(functions);
    ScriptCodeBlock caller={.functions=functions};
    for (size_t capacity=1; capacity<=3; ++capacity) {
        unsigned char guarded[16]; memset(guarded,0xDA,sizeof(guarded));
        assert(!nativeScriptFunctions_Call(&caller,"Pair(Value(), Value())",
            (char*)guarded,capacity));
        for (size_t i=capacity; i<sizeof(guarded); ++i) assert(guarded[i]==0xDA);
    }
    free(functions);
}

int main(int argc, char** argv)
{
    assert(argc == 1 || argc == 3);
    positive(false, argc == 3 ? argv[1] : NULL);
    positive(true, argc == 3 ? argv[2] : NULL);
    rejection(NULL); raw_emission(); effects(); bounded_output();
    puts("Recipient exports compile into bounded sorted direct tables in both routes; invalid declarations and transitive effects checked.");
    return 0;
}
