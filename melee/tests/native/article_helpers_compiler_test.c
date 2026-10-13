#include "../../tools/native_fighter_script.h"
#include "../../tools/native_fighter_item_functions.h"
#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static unsigned cases;
static char generated[1048576];

static bool generate(const char* source, const char* path,
                     NativeFighterScriptError* error, const NativeFighterScriptConstruction* construction)
{
    FILE* output = path ? fopen(path, "w+") : tmpfile();
    assert(output);
    memset(error, 0, sizeof(*error));
    ++cases;
    bool valid = nativeFighterScript_GenerateC(source, strlen(source),
        "article_helpers_probe", NULL, NULL, construction, output, error);
    generated[0] = 0;
    if (valid) {
        rewind(output);
        size_t size = fread(generated, 1, sizeof(generated) - 1, output);
        assert(feof(output)); generated[size] = 0;
    }
    fclose(output);
    return valid;
}

static void accept(const char* source, const char* path, const NativeFighterScriptConstruction* construction)
{
    NativeFighterScriptError error;
    if (!generate(source, path, &error, construction)) {
        fprintf(stderr, "Unexpected rejection at line %u: %s\n%s\n",
                error.line, error.message, source);
        abort();
    }
}

static void reject(const char* source, unsigned line, const char* diagnostic, const NativeFighterScriptConstruction* construction)
{
    NativeFighterScriptError error;
    if (generate(source, NULL, &error, construction)) {
        fprintf(stderr, "Unexpected acceptance:\n%s\n", source);
        abort();
    }
    if (!error.line || !error.message[0] || (line && error.line != line) ||
        (diagnostic && !strstr(error.message, diagnostic))) {
        fprintf(stderr, "Unexpected diagnostic at line %u: %s (expected line %u, %s)\n%s\n",
                error.line, error.message, line, diagnostic ? diagnostic : "any diagnostic", source);
        abort();
    }
}

static void positive(bool callbacks, const char* path)
{
    char source[12288];
    snprintf(source, sizeof(source),
        "%sstate int received\nstate bool ready\nstate float amount\n"
        "func Store(int id, bool active, float amount):\n"
        "    state.received = id\n    state.ready = active\n    state.amount = amount\n"
        "%s        return\n"
        "article It_Kind_Ness_PKFire states 1:\n"
        "    state int result\n    state bool active\n    state float amount\n"
        "    func Outer(int id, bool active, float amount):\n"
        "        let int copy = id\n        let bool enabled = active\n"
        "        let float scaled = amount\n        copy = copy + 2\n"
        "        if enabled:\n            let int scratch = copy + 1\n"
        "            copy = scratch\n"
        "        if not enabled:\n            let int scratch = copy - 1\n"
        "            copy = scratch\n"
        "        Inner(copy, enabled, scaled)\n"
        "    func Inner(int id, bool active, float amount):\n"
        "        item.state.result = id\n        item.state.active = active\n"
        "        item.state.amount = amount\n        owner.call(Store, id, active, amount)\n"
        "    func Pure(float amount):\n        item.state.amount = amount\n"
        "    func SignalValue(float value):\n        item.state.amount = value\n"
        "    func PureZero():\n        pass\n"
        "    on signal receive:\n            SignalValue(signal.value)\n"
        "    on created:\n            Outer(16777217, true, 1.25)\n"
        "            Pure(2)\n            PureZero()\n            PureZero(   )\n"
        "    state 0 anim -1:\n        on physics:\n"
        "            Outer(item.creation_id, creator.exists, 2.5)\n"
        "        on animation:\n            Pure(3.5)\n            return false\n"
        "article It_Kind_Ness_PKFlash states 1:\n"
        "    state int result\n"
        "    func Outer(int id):\n        item.state.result = id\n"
        "    on created:\n            Outer(int(0x80000001))\n"
        "    state 0 anim -1:\n",
        callbacks ? "" : "",
        callbacks ? "move Helpers:\n    on state_341_physics:\n" :
                    "action Helpers:\n    on load:\n");
    accept(source, path, callbacks ? &(const NativeFighterScriptConstruction){"ftDataNess",341,1} : NULL);
    assert(strstr(generated, "static void scripted_article_0_mutating_helper_0(HSD_GObj* gobj, int32_t local_id, bool local_active, float local_amount);"));
    assert(strstr(generated, "static void scripted_article_0_mutating_helper_1("));
    assert(strstr(generated, "static void scripted_article_0_helper_2(HSD_GObj* gobj, float local_amount);"));
    assert(strstr(generated, "static void scripted_article_1_helper_0(HSD_GObj* gobj, int32_t local_id);"));
    assert(strstr(generated, "local_copy = local_copy + 2;"));
    assert(strstr(generated, "script_fn_Store(recipient, local_id, local_active, local_amount)"));
    assert(strstr(generated, "host->callback_source_alive == 0"));
    assert(strstr(generated, "scripted_article_0_mutating_helper_0(gobj, 16777217, true, 1.25f)"));
    assert(strstr(generated, "scripted_article_0_helper_3(gobj, signal_value)"));
    assert(strstr(generated, "static void scripted_article_0_helper_4(HSD_GObj* gobj);"));
    assert(strstr(generated, "scripted_article_0_helper_4(gobj);"));
    const char* pure = strstr(generated, "static void scripted_article_0_helper_4(HSD_GObj* gobj) {");
    const char* pure_end = pure ? strstr(pure, "}\n") : NULL;
    const char* pure_guard = pure ? strstr(pure, "callback_generation") : NULL;
    assert(pure_end && (!pure_guard || pure_guard > pure_end));
    const char* prototype = strstr(generated, "static void scripted_article_0_mutating_helper_1(");
    const char* definition = strstr(prototype, ") {\n");
    assert(prototype && definition && prototype < definition);
}

static void negatives(const NativeFighterScriptConstruction* construction)
{
    static const struct { const char* body; unsigned line; const char* diagnostic; } cases[] = {
        { "    func Same():\n        Same()\n", 2, "Recursive" },
        { "    func First():\n        Second()\n    func Second():\n        First()\n", 2, "Recursive" },
        { "    func Same():\n        return\n    func Same():\n        return\n", 4, "twice" },
        { "    func Wrong(value):\n        return\n", 2, "typed arguments" },
        { "    func Wrong(int value, int value):\n        return\n", 2, "typed arguments" },
        { "    func Wrong(int a, int b, int c, int d, int e, int f, int g, int h, int i):\n        return\n", 2, "typed arguments" },
        { "    func item():\n        return\n", 2, "typed arguments" },
        { "    func Wrong():\n", 2, "complete body" },
        { "    func Wrong():\n      pass\n", 3, "helper statement" },
        { "    func Wrong(int value):\n        value = 1.5\n", 3, "helper statement" },
        { "    func Wrong():\n        let int value = 1.5\n", 3, "helper statement" },
        { "    func Wrong(int value):\n        let int value = 1\n", 3, "helper statement" },
        { "    func Wrong():\n        if true:\n            let int hidden = 1\n        item.state.result = hidden\n", 5, "helper statement" },
        { "    func Wrong():\n        item.state.result = signal.value\n", 3, "helper statement" },
        { "    func Wrong():\n        if item.attachment_owner_is(other):\n            return\n", 3, "helper statement" },
        { "    func Wrong():\n        return true\n", 3, "helper statement" },
        { "    func Wrong(int id):\n        return\n    on created:\n            Wrong()\n", 0, NULL },
        { "    func Wrong():\n        return\n    on created:\n            Wrong(1)\n", 0, NULL },
        { "    func Wrong(int id):\n        return\n    on created:\n            Wrong(1.5)\n", 0, NULL },
        { "    func Wrong():\n        Missing()\n", 3, "helper statement" },
        { "    func Wrong():\n        return\n    on created:\n            let int value = Wrong()\n", 0, NULL },
        { "    func Wrong():\n        return\n    on created:\n            Wrong() trailing\n", 0, NULL },
        { "    func Wrong():\n        item.signal(\"loop\", 0)\n    on signal loop:\n            Wrong()\n", 0, "Recursive" },
        { "    func Wrong():\n        item.state.result = signal.value\n    on signal receive:\n            Wrong()\n", 3, "helper statement" },
    };
    for (unsigned i = 0; i < sizeof(cases) / sizeof(*cases); ++i) {
        char source[8192];
        snprintf(source, sizeof(source), "article It_Kind_Ness_PKFire states 1:\n"
            "    state int result\n%s    state 0 anim -1:\n", cases[i].body);
        reject(source, cases[i].line ? cases[i].line + 1 : 0, cases[i].diagnostic, construction);
    }
    reject("article It_Kind_Ness_PKFire states 1:\n    func Private():\n        return\n"
           "    state 0 anim -1:\narticle It_Kind_Ness_PKFlash states 1:\n"
           "    on created:\n            Private()\n    state 0 anim -1:\n", 7, NULL, construction);
    reject("article It_Kind_Ness_PKFire states 1:\n    func Private():\n        return\n"
           "    state 0 anim -1:\naction Probe:\n    on load:\n        Private()\n", 7, NULL, construction);
}

static void limits(const NativeFighterScriptConstruction* construction)
{
    char source[16384];
    size_t used = (size_t)snprintf(source, sizeof(source), "article It_Kind_Ness_PKFire states 1:\n");
    for (unsigned i = 0; i < ITEM_FUNCTION_LIMIT; ++i)
        used += (size_t)snprintf(source + used, sizeof(source) - used,
            "    func Helper%u():\n        return\n", i);
    strcpy(source + used, "    on created:\n            Helper31()\n    state 0 anim -1:\n");
    accept(source, NULL, construction);
    assert(strstr(generated, "scripted_article_0_helper_31(gobj);"));
    snprintf(source + used, sizeof(source) - used,
        "    func Overflow():\n        return\n    state 0 anim -1:\n");
    reject(source, 66, "At most 32", construction);
    accept("article It_Kind_Ness_PKFire states 1:\n"
        "    func Eight(int a, int b, int c, int d, int e, int f, int g, int h):\n"
        "        let int i = a\n        let int j = b\n        let int k = c\n"
        "        let int l = d\n        let int m = e\n        let int n = f\n"
        "        let int o = g\n        let int p = h\n"
        "    on created:\n            Eight(1, 2, 3, 4, 5, 6, 7, 8)\n"
        "    state 0 anim -1:\n", NULL, construction);
    reject("article It_Kind_Ness_PKFire states 1:\n"
        "    func Eight(int a, int b, int c, int d, int e, int f, int g, int h):\n"
        "        let int i = a\n        let int j = b\n        let int k = c\n"
        "        let int l = d\n        let int m = e\n        let int n = f\n"
        "        let int o = g\n        let int p = h\n        let int q = 0\n"
        "    state 0 anim -1:\n", 11, "helper statement", construction);
}

static void host_requirements(const NativeFighterScriptConstruction* construction)
{
    // An unused helper is still emitted and must bind its real engine operation.
    accept("article It_Kind_Ness_PKFire states 1:\n"
        "    func Advance():\n        item.advance_model_animation()\n"
        "    state 0 anim -1:\n", NULL, construction);
    assert(strstr(generated, "engine->model_advance_animation(gobj->hsd_obj)"));
    assert(strstr(generated, "host->model_advance_animation == 0"));
    // Model evaluation can invoke animation callbacks. It now shares the
    // source-lifetime classification used by explicit fighter pose evaluation.
    assert(strstr(generated, "host->callback_source_alive == 0"));
    assert(strstr(generated, "callback_generation"));
    accept("article It_Kind_Ness_PKFire states 1:\n    state int result\n"
        "    func Pure(int value):\n        let int copy = value + 1\n"
        "        item.state.result = copy\n"
        "    on created:\n            Pure(16777217)\n"
        "    state 0 anim -1:\n", NULL, construction);
    assert(!strstr(generated, "callback_generation"));
    assert(!strstr(generated, "host->callback_source_alive == 0"));
}

static void instance_scale_and_contact_mask(const NativeFighterScriptConstruction* construction)
{
    accept("article It_Kind_Peach_Turnip states 1:\n"
        "    func RestoreScale():\n"
        "        item.set_instance_scale(item.throw_scale)\n"
        "        let float holder_scale = holder.instance_scale_y\n"
        "        let int holder_motion = holder.motion\n"
        "        let int target_motion = targets.motion(item.current_owner)\n"
        "    on thrown:\n            RestoreScale()\n"
        "    state 0 anim -1:\n        on collision:\n"
        "            let int contact = item.sweep_stage_flags()\n"
        "            item.respond_stage_mask(contact)\n", NULL, construction);
    assert(strstr(generated, "float instance_scale = item->throw_scale"));
    assert(strstr(generated, "target_instance_scale_y(engine->item_current_owner(gobj))"));
    assert(strstr(generated, "host->target_instance_scale_y == 0"));
    assert(strstr(generated, "target_motion(engine->item_current_owner(gobj))"));
    assert(strstr(generated, "host->target_motion == 0"));
    assert(!strstr(generated, "scripted_article_holder_motion"));
    assert(strstr(generated, "item_set_instance_scale(gobj, gobj->hsd_obj, instance_scale)"));
    assert(strstr(generated, "item_respond_stage_mask(gobj, contact_mask)"));
    assert(strstr(generated, "host->item_set_instance_scale == 0"));
    assert(strstr(generated, "host->item_respond_stage_mask == 0"));
    const char* valid[] = {
        "item.set_instance_scale(1)", "item.set_instance_scale(1.5)",
        "item.respond_stage_mask(16777217)", "item.respond_stage_mask(-2147483648)",
    };
    const char* invalid[] = {
        "item.throw_scale = 2", "item.set_instance_scale(true)",
        "item.set_instance_scale()", "item.set_instance_scale(1, 2)",
        "item.respond_stage_mask(1.5)", "item.respond_stage_mask(false)",
        "item.respond_stage_mask()", "item.respond_stage_mask(1, 2)",
        "let int motion = targets.motion(1.5)", "let int motion = targets.motion(true)",
        "let int motion = targets.motion()", "let int motion = targets.motion(1, 2)",
    };
    char source[512];
    for (unsigned i=0;i<sizeof(valid)/sizeof(valid[0]);++i) {
        snprintf(source,sizeof(source),"article It_Kind_Peach_Turnip states 1:\n"
            "    on thrown:\n            %s\n    state 0 anim -1:\n",valid[i]);
        accept(source,NULL, construction);
    }
    for (unsigned i=0;i<sizeof(invalid)/sizeof(invalid[0]);++i) {
        snprintf(source,sizeof(source),"article It_Kind_Peach_Turnip states 1:\n"
            "    on thrown:\n            %s\n    state 0 anim -1:\n",invalid[i]);
        reject(source,0,NULL, construction);
    }
}

int main(int argc, char** argv)
{
    if (argc != 1 && argc != 3) return 2;
    positive(false, argc == 3 ? argv[1] : NULL);
    positive(true, argc == 3 ? argv[2] : NULL);
    negatives(NULL); limits(NULL); host_requirements(NULL); instance_scale_and_contact_mask(NULL);
    printf("Article-local helper compiler: %u programs checked in both generation routes.\n", cases);
    return 0;
}
