#include "../../tools/native_fighter_script.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned checks;
#define CHECK(value) do { ++checks; assert(value); } while (0)

static bool generate(const char* source, const char* path, bool inspect, const NativeFighterScriptConstruction* construction)
{
    FILE* file = path ? fopen(path, "w+") : tmpfile(); CHECK(file);
    NativeFighterScriptError error = { 0 };
    bool valid = nativeFighterScript_GenerateC(source, strlen(source),
        "engine_items", NULL, NULL, construction, file, &error);
    if (!valid && inspect)
        fprintf(stderr, "Engine-item source at %u: %s\n", error.line, error.message);
    if (valid && inspect) {
        char code[65536]; rewind(file);
        size_t count = fread(code, 1, sizeof(code) - 1, file);
        CHECK(feof(file)); code[count] = 0;
        const char* fields[] = { "create_engine_item", "attach_created_item",
            "inherit_created_item_scale", "bind_fighter_item", "clear_fighter_item",
            "held_item_id", "callback_source_alive", "bind_existing_item",
            "set_item_visible", "set_item_paused", "notify_item_drop" };
        for (unsigned i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i) {
            char host[96]; snprintf(host, sizeof(host), "host->%s == 0", fields[i]);
            CHECK(strstr(code, host));
        }
        CHECK(strstr(code, "engine->create_engine_item(gobj, (ItemKind)kind, &pos, &vel, facing, group, damage)"));
        CHECK(strstr(code, "engine->attach_created_item(gobj, handle, part, notify_holder)"));
        CHECK(strstr(code, "engine->held_item_id(gobj, false)"));
        CHECK(strstr(code, "engine->held_item_id(gobj, true)"));
        CHECK(strstr(code, "__auto_type callback_result"));
        CHECK(strstr(code, "engine->bind_existing_item(gobj, handle, false)"));
        CHECK(strstr(code, "engine->bind_existing_item(gobj, handle, true)"));
        CHECK(strstr(code, "engine->set_item_visible(handle, option)"));
        CHECK(strstr(code, "engine->set_item_paused(handle, option)"));
        CHECK(strstr(code, "__auto_type callback_result = engine->notify_item_drop"));
    }
    fclose(file); return valid;
}

static void positive(bool callbacks, const char* path)
{
    char source[8192];
    snprintf(source, sizeof(source),
        "%sstate int created\nstate int held_id\nstate int auxiliary_id\n"
        "state bool attached\nstate bool scaled\nstate bool held\nstate bool auxiliary\n"
        "state bool cleared_held\nstate bool cleared_auxiliary\n"
        "state int inner_after_create\nstate int helper_after_create\nstate int outer_after_create\n"
        "state int helper_after_attach\nstate int outer_after_attach\nstate int finished\n"
        "state bool notified\nstate int after_drop\n"
        "func CreateInner():\n"
        "    state.created = items.create_engine(It_Kind_Bat, 1, 2, 3, 4, 5, 6, -1, int(uint(0xFEED1234)), 2.5)\n"
        "    state.inner_after_create = 1\n"
        "func Create():\n    CreateInner()\n    state.helper_after_create = 1\n"
        "func Attach():\n    state.attached = items.attach(state.created, int(7), true)\n"
        "    state.helper_after_attach = 1\n"
        "%s"
        "        Create()\n        state.outer_after_create = 1\n"
        "        Attach()\n        state.outer_after_attach = 1\n"
        "        state.scaled = items.inherit_owner_scale(state.created, true)\n"
        "        state.held = items.bind_held(state.created)\n"
        "        state.auxiliary = items.bind_auxiliary(state.created)\n"
        "        let int held = items.held_id\n        let int auxiliary = items.auxiliary_id\n"
        "        state.held_id = held\n        state.auxiliary_id = auxiliary\n"
        "        state.cleared_held = items.clear_held()\n"
        "        state.cleared_auxiliary = items.clear_auxiliary()\n"
        "        items.bind_existing_held(state.created)\n"
        "        items.bind_existing_auxiliary(state.created)\n"
        "        items.set_visible(state.created, false)\n"
        "        items.set_paused(state.created, true)\n"
        "        state.finished = 1\n"
        "        state.notified = items.notify_drop(state.created, true)\n"
        "        state.after_drop = 1\n",
        callbacks ? "" : "",
        callbacks ? "move Items:\n    on state_341_physics:\n" :
                    "action Items:\n    on load:\n");
    CHECK(generate(source, path, true, callbacks ? &(const NativeFighterScriptConstruction){"ftDataLink",341,1} : NULL));
}

static void reject(const char* statement, unsigned context, const NativeFighterScriptConstruction* construction)
{
    char source[2048];
    const char* prefix = context == 0 ? "state int id\naction Test:\n    on load:\n        " :
        context == 1 ? "article It_Kind_Link_HShot states 1:\n    on created:\n            " :
        "model_effect Test 0x509:\n    on update:\n        ";
    const char* suffix = context == 1 ? "\n    state 0 anim -1:\n" :
        context == 2 ? "\naction Test:\n    on load:\n        pass\n" : "\n";
    snprintf(source, sizeof(source), "%s%s%s", prefix, statement, suffix);
    CHECK(!generate(source, NULL, false, construction));
}

static void negative(const NativeFighterScriptConstruction* construction)
{
    const char* malformed[] = {
        "items.create_engine(It_Kind_Bat, 1,2,3,4,5,6,1,int(0))",
        "items.create_engine(1.5, 1,2,3,4,5,6,1,int(0),0)",
        "items.create_engine(It_Kind_Bat, 1,2,3,4,5,6,1,1.5,0)",
        "items.create_engine(It_Kind_Bat, 1,2,3,4,5,6,1,true,0)",
        "items.attach(16777217.0, int(7), true)", "items.attach(int(1), 7.5, true)",
        "items.attach(int(1))", "items.attach(1, 7)", "items.attach(1, 7, 1)", "items.attach(1, 7, 0.5)", "items.inherit_owner_scale(1.0, true)",
        "items.inherit_owner_scale(int(1), 1)", "items.inherit_owner_scale(int(1), 0.5)",
        "items.bind_held(1.0)", "items.bind_auxiliary(true)",
        "items.clear_held(int(0))", "items.clear_auxiliary(false)",
        "let int value = items.held_id()", "let int value = items.auxiliary_id()",
        "items.held_id = int(1)", "items.auxiliary_id = int(2)",
        "items.bind_existing_held(1.0)", "items.bind_existing_auxiliary(true)",
        "items.bind_existing_held(1, false)", "items.set_visible(1, 1)",
        "items.set_paused(1.5, true)", "items.notify_drop(1, 1.0)",
        "items.notify_drop()", "items.set_visible(1, true, false)"
    };
    for (unsigned i = 0; i < sizeof(malformed) / sizeof(malformed[0]); ++i)
        reject(malformed[i], 0, construction);
    const char* fighter_only[] = {
        "items.create_engine(It_Kind_Bat, 1,2,3,4,5,6,1,int(0),0)",
        "items.attach(int(1), int(7), true)", "items.inherit_owner_scale(int(1), true)",
        "items.bind_held(int(1))", "items.bind_auxiliary(int(1))",
        "items.clear_held()", "items.clear_auxiliary()",
        "let int value = items.held_id", "let int value = items.auxiliary_id",
        "items.bind_existing_held(1)", "items.bind_existing_auxiliary(1)",
        "items.set_visible(1, true)", "items.set_paused(1, false)",
        "items.notify_drop(1, true)"
    };
    for (unsigned i = 0; i < sizeof(fighter_only) / sizeof(fighter_only[0]); ++i) {
        reject(fighter_only[i], 1, construction); reject(fighter_only[i], 2, construction);
    }
    CHECK(generate("state int id\naction Statements:\n    on load:\n"
        "        items.create_engine(It_Kind_Bat,1,2,3,4,5,6,1,int(0),0)\n"
        "        items.attach(state.id, int(7), true)\n"
        "        items.inherit_owner_scale(state.id, false)\n"
        "        items.bind_held(state.id)\n        items.bind_auxiliary(state.id)\n"
        "        items.clear_held( )\n        items.clear_auxiliary()\n",
        NULL, false, construction));
    // Pure ID reads do not require callback liveness or item construction.
    CHECK(generate("state int held\naction Read:\n    on load:\n"
        "        let int value = items.held_id\n        state.held = value\n",
        NULL, false, construction));
}

int main(int argc, char** argv)
{
    negative(NULL); positive(false, argc > 1 ? argv[1] : NULL);
    positive(true, argc > 2 ? argv[2] : NULL);
    printf("Engine item compiler checks: %u\n", checks);
    return 0;
}
