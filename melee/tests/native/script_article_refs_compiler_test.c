#include "../../tools/native_fighter_script.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char articles[] =
    "article It_Kind_Ness_PKFire states 1:\n"
    "    state int parent\n    state bool active\n    state float sampled\n    state int owner\n    state bool charging\n"
    "    state 0 anim -1:\n        on physics:\n"
    "            let int id = uint(0xFEDCBA98)\n"
    "            item.state.parent = articles.ref(id, It_Kind_Ness_PKFire_Flame).state.parent\n"
    "            item.state.active = articles.ref(id, It_Kind_Ness_PKFire_Flame).state.active\n"
    "            item.state.sampled = articles.ref(id, It_Kind_Ness_PKFire_Flame).state.position[-1]\n"
    "            item.state.sampled = articles.ref(id, It_Kind_Ness_PKFire_Flame).state.position[2]\n"
    "            item.state.owner = articles.ref(id, It_Kind_Ness_PKFire_Flame).current_owner\n"
    "            item.state.charging = targets.smash_charge_active(item.state.owner)\n"
    "article It_Kind_Ness_PKFire_Flame states 1:\n"
    "    state bool active\n    state float position[8]\n    state int parent\n"
    "    state 0 anim -1:\n";

static bool generate(const char* source, const char* path, bool inspect, const NativeFighterScriptConstruction* construction)
{
    FILE* output = path ? fopen(path, "w+b") : tmpfile(); assert(output);
    NativeFighterScriptError error = { 0 };
    bool valid = nativeFighterScript_GenerateC(source, strlen(source),
        "article_refs_probe", NULL, NULL, construction, output, &error);
    if (!valid && inspect) fprintf(stderr, "Refs fixture line %u: %s\n", error.line, error.message);
    if (valid && inspect) {
        assert(!fseek(output, 0, SEEK_END)); long length = ftell(output);
        assert(length > 0 && !fseek(output, 0, SEEK_SET));
        char* code = malloc((size_t)length + 1); assert(code);
        size_t read = fread(code, 1, (size_t)length, output);
        assert(read && !ferror(output)); code[read] = 0;
        assert(strstr(code, "engine->article_ref_exists(gobj, (uint32_t)"));
        assert(strstr(code, "engine->article_ref_int(gobj, (uint32_t)"));
        assert(strstr(code, "engine->article_ref_bool(gobj, (uint32_t)"));
        assert(strstr(code, "engine->article_ref_float(gobj, (uint32_t)"));
        assert(strstr(code, "engine->article_ref_current_owner(gobj, (uint32_t)"));
        assert(strstr(code, "engine->target_smash_charge_active((uint32_t)"));
        assert(strstr(code, "It_Kind_Ness_PKFire_Flame, 2, (int32_t)(0)"));
        assert(strstr(code, "It_Kind_Ness_PKFire_Flame, 1, (int32_t)(2)"));
        free(code);
    }
    fclose(output); return valid;
}

static void positive(bool callbacks, const char* path)
{
    char source[8192];
    snprintf(source, sizeof(source),
        "%sstate int handle\nstate int value\nstate bool active\nstate float sampled\nstate int helper_value\nstate int owner\nstate int helper_owner\nstate bool charging\nstate bool up_held\nstate bool z_held\n"
        "func ReadParent(int id):\n"
        "    state.helper_value = articles.ref(id, It_Kind_Ness_PKFire_Flame).state.parent\n"
        "    state.helper_owner = articles.ref(id, It_Kind_Ness_PKFire_Flame).current_owner\n"
        "%s"
        "        let int id = uint(0xFEDCBA98)\n"
        "        state.handle = id\n"
        "        ReadParent(id)\n"
        "        state.active = articles.ref(id, It_Kind_Ness_PKFire_Flame).exists\n"
        "        state.value = articles.ref(id, It_Kind_Ness_PKFire_Flame).state.parent\n"
        "        state.active = articles.ref(id, It_Kind_Ness_PKFire_Flame).state.active\n"
        "        state.sampled = articles.ref(id, It_Kind_Ness_PKFire_Flame).state.position[-1]\n"
        "        state.sampled = articles.ref(id, It_Kind_Ness_PKFire_Flame).state.position[2]\n"
        "        state.owner = articles.ref(id, It_Kind_Ness_PKFire_Flame).current_owner\n"
        "        state.charging = targets.smash_charge_active(state.owner)\n"
        "        state.up_held = input.up_held\n"
        "        state.z_held = input.z_held\n%s",
        callbacks ? "" : "",
        callbacks ? "move Refs:\n    on state_341_physics:\n" : "action Refs:\n    on load:\n", articles);
    assert(generate(source, path, true, callbacks ? &(const NativeFighterScriptConstruction){"ftDataNess",341,1} : NULL));
}

static void negative(const NativeFighterScriptConstruction* construction)
{
    const char* invalid[] = {
        "articles.ref(1.25, It_Kind_Ness_PKFire_Flame).exists",
        "articles.ref(1.25, It_Kind_Ness_PKFire_Flame).current_owner",
        "targets.smash_charge_active(1.25)",
        "articles.ref(1, It_Kind_Ness_PKFire_Flame).state.position[0.5]",
        "articles.ref(1, It_Kind_Ness_PKFire_Flame).state.missing",
        "articles.ref(1, It_Kind_Ness_PKFire_Flame).state.position",
        "articles.ref(1, It_Kind_Ness_PKFire_Flame).state.active[0]",
        "articles.ref(1, It_Kind_Mario_Fire).exists"
    };
    for (unsigned context = 0; context < 2; ++context)
        for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
            char source[8192];
            snprintf(source, sizeof(source), context ?
                "article It_Kind_Ness_PKFire states 1:\n    state 0 anim -1:\n"
                "        on physics:\n            velocity.x = %s\n"
                "article It_Kind_Ness_PKFire_Flame states 1:\n"
                "    state bool active\n    state float position[8]\n    state 0 anim -1:\n" :
                "action Refs:\n    on load:\n        velocity.x = %s\n%s",
                invalid[i], articles);
            assert(!generate(source, NULL, false, construction));
        }
    const char* readonly[] = {
        "action Refs:\n    on load:\n        articles.ref(1, It_Kind_Ness_PKFire_Flame).state.parent = 2\n",
        "article It_Kind_Ness_PKFire states 1:\n    state 0 anim -1:\n"
        "        on physics:\n            articles.ref(1, It_Kind_Ness_PKFire_Flame).state.parent = 2\n",
        "action Refs:\n    on load:\n        articles.ref(1, It_Kind_Ness_PKFire_Flame).current_owner = 2\n",
        "article It_Kind_Ness_PKFire states 1:\n    state 0 anim -1:\n"
        "        on physics:\n            articles.ref(1, It_Kind_Ness_PKFire_Flame).current_owner = 2\n",
        "action Refs:\n    on load:\n        input.up_held = true\n",
        "action Refs:\n    on load:\n        input.z_held = true\n"
    };
    for (unsigned i = 0; i < sizeof(readonly) / sizeof(readonly[0]); ++i) {
        char source[8192]; snprintf(source, sizeof(source), "%s"
            "article It_Kind_Ness_PKFire_Flame states 1:\n    state int parent\n"
            "    state 0 anim -1:\n", readonly[i]);
        assert(!generate(source, NULL, false, construction));
    }
}

int main(int argc, char** argv)
{
    if (argc != 1 && argc != 3) return 2;
    positive(false, argc == 3 ? argv[1] : NULL);
    positive(true, argc == 3 ? argv[2] : NULL);
    negative(NULL);
    puts("Forward article refs compile in fighter/item contexts with exact typed descriptor indices");
    return 0;
}
