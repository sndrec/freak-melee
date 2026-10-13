#include "../../tools/native_fighter_script.h"
#include "../../tools/native_fighter_script_functions.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(value) do { ++checks; assert(value); } while (0)

static bool generate(const char* source, const char* path, bool inspect, const NativeFighterScriptConstruction* construction)
{
    FILE* file = path ? fopen(path, "w+") : tmpfile(); CHECK(file);
    NativeFighterScriptError error = { 0 };
    bool valid = nativeFighterScript_GenerateC(source, strlen(source),
        "item_pause_queries", NULL, NULL, construction, file, &error);
    if (!valid && inspect)
        fprintf(stderr, "Pause/query source at %u: %s\n", error.line, error.message);
    if (valid && inspect) {
        char code[65536]; rewind(file);
        size_t length = fread(code, 1, sizeof(code) - 1, file);
        CHECK(feof(file)); code[length] = 0;
        CHECK(strstr(code, "engine->item_pause(gobj);") != NULL);
        CHECK(strstr(code, "engine->item_resume(gobj);") != NULL);
        CHECK(strstr(code, "host->item_pause == 0") != NULL);
        CHECK(strstr(code, "host->item_resume == 0") != NULL);
        CHECK(strstr(code, "fighter->take_dmg_cb == NULL") != NULL);
        CHECK(strstr(code, "fighter->death2_cb == NULL") != NULL);
        CHECK(strstr(code, "fighter->reflecting") != NULL);
        CHECK(strstr(code, "scripted_article_creator(item)->x34_scale.y") != NULL);
        CHECK(strstr(code, "host->item_creator == 0") != NULL);
        CHECK(strstr(code, "host->fighter_model_scale == 0") == NULL);
        CHECK(strstr(code, "host->fighter_root_scale_y == 0") == NULL);
        CHECK(strstr(code, "host->callback_source_alive == 0") == NULL);
    }
    fclose(file); return valid;
}

static void positive(bool callbacks, const char* path)
{
    char source[4096];
    snprintf(source, sizeof(source),
        "%sstate bool damage_empty\nstate bool death_empty\nstate bool reflected\n"
        "%s"
        "        let bool damage = callbacks.damage_is(None)\n"
        "        let bool death = callbacks.death2_is(None)\n"
        "        let bool reflected = reflector.active\n"
        "        state.damage_empty = damage\n"
        "        state.death_empty = death\n"
        "        state.reflected = reflected\n"
        "article It_Kind_Link_HShot states 1:\n"
        "    state float scale\n"
        "    on created:\n"
        "            let float scale = creator.instance_scale_y\n"
        "            item.state.scale = scale\n"
        "    on signal pause:\n"
        "            item.pause()\n"
        "    on signal resume:\n"
        "            item.resume()\n"
        "    state 0 anim -1:\n"
        "item PauseProbe damage 0 angle 0 lifetime 1:\n"
        "    on thrown:\n        item.pause()\n"
        "    on tick:\n        item.resume()\n",
        callbacks ? "" : "",
        callbacks ? "move Queries:\n    on state_341_physics:\n" :
                    "action Queries:\n    on load:\n");
    CHECK(generate(source, path, true, callbacks ? &(const NativeFighterScriptConstruction){"ftDataLink",341,1} : NULL));
}

static void callback_queries(void)
{
    ScriptCodeBlock caller = { 0 };
    char code[192];
    const char* names[] = { "callbacks.damage_is", "callbacks.death2_is" };
    const char* fields[] = { "take_dmg_cb", "death2_cb" };
    for (unsigned i = 0; i < 2; ++i) {
        const char* source = "( None ) and false";
        CHECK(nativeScriptFunctions_CallbackExpression(&caller, names[i],
            &source, code, sizeof(code)));
        char expected[96]; snprintf(expected, sizeof(expected),
            "(fighter->%s == NULL)", fields[i]);
        CHECK(strcmp(code, expected) == 0);
        CHECK(strcmp(source, " and false") == 0);
        CHECK(!nativeScriptDependencies_Any(caller.function_calls) && !caller.may_invalidate_source);
        const char* invalid[] = { "()", "(1)", "(None, None)", "(missing)",
                                  "(\"None\")", "(None())" };
        for (unsigned j = 0; j < sizeof(invalid) / sizeof(invalid[0]); ++j) {
            source = invalid[j];
            CHECK(!nativeScriptFunctions_CallbackExpression(&caller, names[i],
                &source, code, sizeof(code)));
        }
        caller.model_effect = true; source = "(None)";
        CHECK(!nativeScriptFunctions_CallbackExpression(&caller, names[i],
            &source, code, sizeof(code)));
        caller.model_effect = false;
    }
}

static void negative(const NativeFighterScriptConstruction* construction)
{
    const char* fighter[] = {
        "item.pause()", "item.resume()", "state.scale = creator.instance_scale_y",
        "let bool bad = callbacks.damage_is(int(0))",
        "let bool bad = callbacks.death2_is(None, None)",
        "let bool bad = reflector.active()", "reflector.active = true"
    };
    const char* article[] = {
        "item.pause(1)", "item.resume(false)", "let bool bad = item.pause()",
        "let float bad = item.resume()", "item.state.scale = creator.instance_scale_y()",
        "creator.instance_scale_y = 2", "let bool bad = callbacks.damage_is(None)",
        "let bool bad = callbacks.death2_is(None)", "let bool bad = reflector.active"
    };
    char source[2048];
    for (unsigned i = 0; i < sizeof(fighter) / sizeof(fighter[0]); ++i) {
        snprintf(source, sizeof(source), "state float scale\naction Test:\n"
            "    on load:\n        %s\n", fighter[i]);
        CHECK(!generate(source, NULL, false, construction));
    }
    for (unsigned i = 0; i < sizeof(article) / sizeof(article[0]); ++i) {
        snprintf(source, sizeof(source), "article It_Kind_Link_HShot states 1:\n"
            "    state float scale\n    on created:\n            %s\n"
            "    state 0 anim -1:\n"
        "item PauseProbe damage 0 angle 0 lifetime 1:\n"
        "    on thrown:\n        item.pause()\n"
        "    on tick:\n        item.resume()\n", article[i]);
        CHECK(!generate(source, NULL, false, construction));
    }
    const char* effect[] = { "item.pause()", "item.resume()",
        "let bool bad = callbacks.damage_is(None)",
        "let bool bad = callbacks.death2_is(None)", "let bool bad = reflector.active",
        "let float bad = creator.instance_scale_y" };
    for (unsigned i = 0; i < sizeof(effect) / sizeof(effect[0]); ++i) {
        snprintf(source, sizeof(source), "model_effect Probe 0x509:\n"
            "    on update:\n        %s\naction Test:\n    on load:\n        pass\n", effect[i]);
        CHECK(!generate(source, NULL, false, construction));
    }
    // Named zero-argument pointers retain the same existing slot-query contract.
    CHECK(generate("state bool vacant\nfunc cleanup():\n    pass\n"
        "action Test:\n    on load:\n        callbacks.damage(cleanup)\n"
        "        callbacks.death2(cleanup)\n"
        "        state.vacant = callbacks.damage_is(cleanup) and callbacks.death2_is(cleanup)\n",
        NULL, false, construction));
    CHECK(!generate("state bool vacant\nfunc cleanup(int value):\n    pass\n"
        "action Test:\n    on load:\n        state.vacant = callbacks.damage_is(cleanup)\n",
        NULL, false, construction));
}

int main(int argc, char** argv)
{
    callback_queries(); negative(NULL);
    positive(false, argc > 1 ? argv[1] : NULL);
    positive(true, argc > 2 ? argv[2] : NULL);
    printf("{\"compiler_checks\":%u}\n", checks);
    return 0;
}
