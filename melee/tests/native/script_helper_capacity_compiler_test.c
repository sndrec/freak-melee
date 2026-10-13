#include "../../tools/native_fighter_script_functions.h"
#include "../../tools/native_fighter_script_lifetime.h"
#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static bool generate(const char* source, char** generated)
{
    FILE* output = tmpfile(); assert(output);
    NativeFighterScriptError error = { 0 };
    bool valid = nativeFighterScript_GenerateC(source, strlen(source),
        "helper_capacity_probe", NULL, NULL, NULL, output, &error);
    if (!valid && generated) fprintf(stderr, "Helpers line %u: %s\n", error.line, error.message);
    if (valid && generated) {
        assert(!fseek(output, 0, SEEK_END));
        long size = ftell(output); assert(size > 0);
        *generated = malloc((size_t)size + 1); assert(*generated);
        rewind(output);
        assert(fread(*generated, 1, (size_t)size, output) == (size_t)size);
        (*generated)[size] = 0;
    }
    fclose(output); return valid;
}

// A forward chain crosses both word boundaries and the final supported index.
// The leaf can retire its caller: every ancestor must inherit that hazard.
static void fighter_dependencies(void)
{
    char source[16384]; size_t used = 0;
    for (unsigned i = 0; i < SCRIPT_FUNCTIONS; ++i) {
        if (i + 1 == SCRIPT_FUNCTIONS)
            used += snprintf(source + used, sizeof(source) - used,
                "func F%u():\n    let bool sent = targets.call(1, \"receive\")\n", i);
        else used += snprintf(source + used, sizeof(source) - used,
                "func F%u():\n    F%u()\n", i, i + 1);
    }
    assert(used < sizeof(source));
    char* parsed = malloc(used + 1); assert(parsed); memcpy(parsed, source, used + 1);
    NativeFighterScriptError error = { 0 };
    ScriptFunctionProgram* program = nativeScriptFunctions_Parse(parsed, used, &error);
    assert(program && program->count == 128);
    ScriptStateTable state = { 0 };
    assert(nativeScriptFunctions_Compile(program, &state, NULL, false, 0, 0, &error));
    for (unsigned i = 0; i < program->count; ++i) assert(program->functions[i].may_invalidate_source);
    ScriptCodeBlock caller = { .functions = program };
    caller.function_calls = nativeScriptFunctions_References(program, "F127()");
    assert(nativeScriptLifetime_BlockMayInvalidate(&caller));
    assert(nativeScriptDependencies_Test(caller.function_calls, 127));
    assert(!nativeScriptDependencies_Test(caller.function_calls, 63));
    free(program); free(parsed);

    strcpy(source + used, "action Probe:\n    on load:\n        F0()\n");
    char* code = NULL; assert(generate(source, &code));
    assert(strstr(code, "script_fn_F127(gobj);"));
    assert(strstr(code, "({ script_fn_F0(gobj);"));
    assert(strstr(code, "host->callback_source_alive == 0"));
    free(code);
    strcpy(source + used, "func Overflow():\n    pass\n");
    assert(!generate(source, NULL));

    // A cycle between the low and high words must remain a compile error.
    used = 0;
    for (unsigned i = 0; i < 128; ++i)
        used += snprintf(source + used, sizeof(source) - used,
            "func F%u():\n    F%u()\n", i, (i + 1) % 128);
    assert(!generate(source, NULL));
}

static void effect_dependencies(void)
{
    char source[16384]; size_t used = 0;
    for (unsigned i = 0; i < 128; ++i) {
        if (i == 127)
            used += snprintf(source + used, sizeof(source) - used,
                "func E127():\n    effect.cancel()\n");
        else used += snprintf(source + used, sizeof(source) - used,
                "func E%u():\n    E%u()\n", i, i + 1);
    }
    strcpy(source + used,
        "model_effect Visual 0x509:\n    on update:\n        E0()\n"
        "action Probe:\n    on load:\n        effects.spawn_root(Visual)\n");
    char* code = NULL; assert(generate(source, &code));
    assert(strstr(code, "script_effect_fn_E127(effect);"));
    assert(strstr(code, "script_effect_fn_E63(effect);"));
    assert(strstr(code, "host->model_effect_cancel == 0"));
    assert(!strstr(code, "script_fn_E0"));
    free(code);
    // The low-index caller cannot hide its high-index effect-only dependency.
    strcpy(source + used,
        "model_effect Visual 0x509:\n    on update:\n        E0()\n"
        "action Probe:\n    on load:\n        E0()\n");
    assert(!generate(source, NULL));
}

int main(void)
{
    fighter_dependencies(); effect_dependencies();
    puts("128 helper dependency capacity: transitive lifetime, recursion and effect context checked across both words.");
    return 0;
}
