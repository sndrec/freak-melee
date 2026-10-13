#include "../../tools/native_fighter_script.h"
#include "../../tools/native_fighter_script_functions.h"
#include "../../tools/native_fighter_script_signal.h"
#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static NativeFighterParameters parameters;
static unsigned checks;

static bool generate(const char* source, const char* path, char** generated,
                     unsigned* error_line, const NativeFighterScriptConstruction* construction)
{
    FILE* output = path ? fopen(path, "w+b") : tmpfile(); assert(output);
    NativeFighterScriptError error = { 0 };
    bool valid = nativeFighterScript_GenerateC(source, strlen(source),
        "model_effect_probe", NULL, &parameters, construction, output, &error);
    if (error_line) *error_line = error.line;
    if (valid && generated) {
        assert(!fseek(output, 0, SEEK_END)); long size = ftell(output);
        assert(size > 0 && !fseek(output, 0, SEEK_SET));
        *generated = malloc((size_t)size + 1); assert(*generated);
        assert(fread(*generated, 1, (size_t)size, output) == (size_t)size);
        (*generated)[size] = 0;
    }
    if (!valid && path) fprintf(stderr, "Model effect line %u: %s\n", error.line, error.message);
    fclose(output); ++checks; return valid;
}

static const char shared[] =
    "state bool collision\nstate int samples[2]\n"
    "func CancelValue() -> float:\n    if creator.command0: effect.cancel()\n    return 2.0\n"
    "func CancelLeaf():\n    let float ignored = CancelValue()\n"
    "func CancelBranch():\n    CancelLeaf()\n    effect.joint(\"\").translation.x = 99\n"
    "func SharedMath(int value) -> int:\n    let int next = value + 1\n    return next\n"
    "func UnusedValid():\n    let int current = creator.motion\n    pass\n"
    "model_effect VisualZ 0x509:\n    on update:\n"
    "        SharedMath(creator.command1)\n"
    "        effect.joint(\"1\").visible = creator.motion == attrs.air_motion\n"
    "        if creator.command3 and creator.state.collision:\n"
    "            effect.joint(\"\").rotation.z = -atan2_float(creator.floor.normal.x, creator.floor.normal.y)\n"
    "        else: effect.joint(\"\").rotation.z = 0\n"
    "        let int sampled = creator.state.samples[1]\n"
    "        effect.joint(\"0/255\").quaternion.w = attrs.amount\n"
    "        let bool present = effect.joint(\"0\").exists\n"
    "        effect.joint(\"0\").self_visible = present\n"
    "        CancelBranch()\n"
    "        effect.joint(\"\").rotation.y = creator.facing\n"
    "        effect.joint(\"\").translation.y = 100\n"
    "model_effect VisualA 0x47C:\n    on update:\n        pass\n";

static void positive(bool states, const char* path)
{
    char source[8192];
    snprintf(source, sizeof(source), "%s%s%s\n"
        "        SharedMath(2)\n        effects.spawn_root(VisualZ)\n"
        "        effects.spawn_on_bone(VisualA, Top)\n",
        states ? "" : "", shared,
        states ? "move Effects:\n    on state_341_physics:" : "action Effects:\n    on load:");
    char* code; assert(generate(source, path, &code, NULL, states ? &(const NativeFighterScriptConstruction){"ftDataLuigi",341,1} : NULL));
    assert(strstr(code, "static void script_model_effect_VisualZ(EF_Effect* effect)"));
    assert(strstr(code, "HSD_GObj* effect_gobj = effect->gobj;"));
    assert(strstr(code, "const uint32_t callback_generation = effect->native_generation;"));
    assert(strstr(code, "callback_source_alive(effect_gobj, callback_generation, NativeCallbackSource_ModelEffect)"));
    assert(strstr(code, "script_effect_fn_CancelBranch(effect);"));
    assert(strstr(code, "script_effect_fn_SharedMath(effect,"));
    assert(strstr(code, "script_fn_SharedMath(gobj,"));
    assert(!strstr(code, "script_fn_CancelLeaf"));
    assert(!strstr(code, "script_effect_fn_UnusedValid"));
    assert(strstr(code, "engine->model_joint_write(engine->model_joint_resolve(engine->model_effect_joint(effect, 0), \"1\"), 10,"));
    assert(strstr(code, "host->spawn_model_effect == 0"));
    assert(strstr(code, "host->model_effect_creator == 0"));
    assert(strstr(code, "host->model_joint_read == 0"));
    assert(strstr(code, "host->model_joint_write == 0"));
    assert(strstr(code, "generated.model_effect_count = 2;"));
    assert(strstr(code, "fighter->facing_dir"));
    assert(!strstr(code, "efLib_Cb_") && !strstr(code, "mv.co.common"));
    char first[128], second[128];
    const uint32_t a = nativeScriptSignal_Hash("VisualA"), z = nativeScriptSignal_Hash("VisualZ");
    snprintf(first, sizeof(first), "{ UINT32_C(%u)", a < z ? a : z);
    snprintf(second, sizeof(second), "{ UINT32_C(%u)", a < z ? z : a);
    assert(strstr(code, first) < strstr(code, second));
    free(code);
}

static void constructor_families(const NativeFighterScriptConstruction* construction)
{
    const char source[] =
        "model_effect Positioned 0x4CC:\n    on update:\n"
        "        effect.joint(\"0\", 3).translation.x = creator.facing\n"
        "model_effect Linked 0x4DA:\n    on update:\n"
        "        effect.joint(\"\", 1).visible = true\n"
        "action Test:\n    on load:\n"
        "        effects.spawn_at(Positioned, attrs.amount, 2, 3)\n"
        "        effects.spawn_async_on_bone(Positioned, Top)\n";
    char* code; assert(generate(source, NULL, &code, NULL, construction));
    assert(strstr(code, "engine->model_effect_joint(effect, 3)"));
    assert(strstr(code, "engine->model_effect_joint(effect, 1)"));
    assert(strstr(code, "model_position.x = "));
    assert(strstr(code, "model_position.x = ") < strstr(code, "model_position.y = "));
    assert(strstr(code, "model_position.y = ") < strstr(code, "model_position.z = "));
    assert(strstr(code, "-1, &model_position, false)"));
    assert(strstr(code, "NULL, true)"));
    assert(strstr(code, "host->model_effect_joint == 0"));
    free(code);
    const char* actions[] = {
        "effects.spawn_at(Positioned, 1, 2)",
        "effects.spawn_at(Positioned, 1, 2, 3, 4)",
        "effects.spawn_at(Positioned, unknown, 2, 3)",
        "effects.spawn_async_on_bone(Positioned, InvalidBone)"
    };
    for (unsigned i = 0; i < sizeof(actions)/sizeof(actions[0]); ++i) {
        char text[4096];
        snprintf(text,sizeof(text), "model_effect Positioned 0x4CC:\n    on update:\n        pass\naction Test:\n    on load:\n        %s\n",actions[i]);
        assert(!generate(text,NULL,NULL,NULL, construction));
    }
}

static void reject_body(const char* body, const NativeFighterScriptConstruction* construction)
{
    char source[4096]; unsigned line = 0;
    snprintf(source, sizeof(source), "model_effect Probe 0x509:\n    on update:\n        %s\n"
        "action Test:\n    on load:\n        pass\n", body);
    assert(!generate(source, NULL, NULL, &line, construction)); assert(line == 3);
}

static void negative(const NativeFighterScriptConstruction* construction)
{
    const char* bodies[] = {
        "creator.motion = 1", "creator.facing = 1", "creator.state.bad = true", "state.bad = true",
        "velocity.x = 1", "physics.air_basic()", "effects.spawn_root(Probe)",
        "targets.call(1, \"receive\")", "let bool ok = targets.call(1, \"receive\")",
        "effect.joint("", 4).rotation.z = 0", "effect.joint("", -1).rotation.z = 0",
        "effect.joint("", 1.5).rotation.z = 0", "effect.joint("", creator.command0).rotation.z = 0",
        "effect.joint("", 0, 1).rotation.z = 0", "effect.cancel(1)", "effect.joint(\"\").exists = true",
        "effect.joint(\"256\").rotation.z = 0", "effect.joint(\"0/\").rotation.z = 0",
        "effect.joint(\"0/0/0/0/0/0/0/0/0/0/0/0/0\").rotation.z = 0",
        "effect.joint(\"\").invalid = 0", "let int bad = effect.joint(\"\").rotation.z",
        "let int bad = creator.state.unknown", "let int bad = creator.facing",
        "let bad = creator.facing.extra", "let old = owner.motion", "unknown()",
    };
    for (unsigned i = 0; i < sizeof(bodies) / sizeof(bodies[0]); ++i) reject_body(bodies[i], construction);
    const char* invalid[] = {
        "model_effect Probe -1:\n    on update:\n        pass\n",
        "model_effect Probe 0x80000000:\n    on update:\n        pass\n",
        "model_effect Probe 0x509:\n    on physics:\n        pass\n",
        "model_effect Probe 0x509:\n    on update:\n        pass\n    on update:\n        pass\n",
        "model_effect Probe 0x509:\n    on update:\n",
        "model_effect Probe 0x509:\n    on update:\n        pass\nmodel_effect Probe 0x47C:\n    on update:\n        pass\n",
        // Distinct names collide under the actual declaration hash.
        "model_effect EjppvBnoqdu 0x509:\n    on update:\n        pass\nmodel_effect EaxToXlnift 0x47C:\n    on update:\n        pass\n",
        "func Bad():\n    effect.nonsense()\n",
        "func Bad():\n    effect.nonsense()\nmodel_effect Probe 0x509:\n    on update:\n        pass\n",
        "export func Bad():\n    effect.cancel()\nmodel_effect Probe 0x509:\n    on update:\n        pass\n",
        "func Bad():\n    effect.cancel()\nfunc Outer():\n    Bad()\nmodel_effect Probe 0x509:\n    on update:\n        pass\naction Extra:\n    on tick:\n        Outer()\n",
        "func Bad():\n    effect.cancel()\nmodel_effect Probe 0x509:\n    on update:\n        pass\narticle It_Kind_Luigi_Fireball states 1:\n    on created:\n            creator.call(Bad)\n",
        "func Bad():\n    effect.cancel()\nmodel_effect Probe 0x509:\n    on update:\n        pass\narticle It_Kind_Luigi_Fireball states 1:\n    on created:\n            owner.call(Bad)\n",
        "func Loop():\n    Loop()\n    effect.cancel()\nmodel_effect Probe 0x509:\n    on update:\n        pass\n",
    };
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        char source[8192]; snprintf(source, sizeof(source), "%saction Test:\n    on load:\n        pass\n", invalid[i]);
        assert(!generate(source, NULL, NULL, NULL, construction));
    }
    char source[8192]; size_t used = 0;
    for (unsigned i = 0; i < 32; ++i)
        used += (size_t)snprintf(source + used, sizeof(source) - used,
            "model_effect E%u 0x509:\n    on update:\n        pass\n", i);
    strcpy(source + used, "action Test:\n    on load:\n        pass\n");
    assert(generate(source, NULL, NULL, NULL, construction));
    strcpy(source + used, "model_effect TooMany 0x509:\n    on update:\n        pass\naction Test:\n    on load:\n        pass\n");
    assert(!generate(source, NULL, NULL, NULL, construction));
}

static void lexical_context(void)
{
    assert(!nativeScriptFunctions_ModelEffectContext("let item_effect = 1 # effect.cancel()"));
    assert(!nativeScriptFunctions_ModelEffectContext("effects.fake(\"creator.motion\", \"effect.joint\")"));
    assert(!nativeScriptFunctions_ModelEffectContext("other.effect.cancel()"));
    assert(nativeScriptFunctions_ModelEffectContext("if creator.command3: effect.cancel()"));
    ScriptFunctionProgram* program = calloc(1, sizeof(*program)); assert(program);
    program->count = 1; strcpy(program->functions[0].name, "Leaf");
    assert(nativeScriptDependencies_Test(nativeScriptFunctions_References(program, "Leaf()"), 0));
    assert(!nativeScriptDependencies_Any(nativeScriptFunctions_References(program, "OtherLeaf()")));
    assert(!nativeScriptDependencies_Any(nativeScriptFunctions_References(program, "other.Leaf()")));
    assert(!nativeScriptDependencies_Any(nativeScriptFunctions_References(program, "fake(\"Leaf()\") # Leaf()")));
    free(program); checks += 8;
}

int main(int argc, char** argv)
{
    if (argc != 1 && argc != 3) return 2;
    unsigned line = 0;
    const char values[] = "int air_motion = 357\nfloat amount = 0.5\n";
    assert(nativeFighterParameters_Parse(values, strlen(values), &parameters, &line));
    lexical_context(); negative(NULL); constructor_families(NULL);
    positive(false, argc == 3 ? argv[1] : NULL);
    positive(true, argc == 3 ? argv[2] : NULL);
    printf("Model effect compiler: %u context, declaration and emission cases passed.\n", checks);
    return 0;
}
