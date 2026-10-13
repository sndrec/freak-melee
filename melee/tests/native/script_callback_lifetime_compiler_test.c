#include "../../tools/native_fighter_script.h"
#include "../../tools/native_fighter_script_lifetime.h"
#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static bool generate(const char* source, const char* path, bool inspect, const NativeFighterScriptConstruction* construction)
{
    FILE* output = path ? fopen(path, "w+") : tmpfile(); assert(output);
    NativeFighterScriptError error = { 0 };
    bool valid = nativeFighterScript_GenerateC(source, strlen(source),
        "callback_lifetime_probe", NULL, NULL, construction, output, &error);
    if (!valid && inspect) fprintf(stderr, "Lifetime line %u: %s\n", error.line, error.message);
    if (valid && inspect) {
        char code[65536]; rewind(output);
        size_t size = fread(code, 1, sizeof(code) - 1, output);
        assert(feof(output)); code[size] = 0;
        assert(strstr(code, "host->callback_source_alive == 0"));
        assert(strstr(code, "host->target_call == 0"));
        assert(strstr(code, "__auto_type callback_result = engine->target_call"));
        assert(strstr(code, "fighter->x8_spawnNum"));
        assert(strstr(code, "item->x1C"));
        assert(strstr(code, "NativeCallbackSource_Fighter"));
        assert(strstr(code, "NativeCallbackSource_Item"));
        assert(strstr(code, "return 0.0f;"));
        assert(strstr(code, "return false;"));
        assert(strstr(code, "({ script_fn_Outer(gobj);"));
    }
    fclose(output); return valid;
}

static void positive(bool callbacks, const char* path)
{
    char source[8192];
    snprintf(source, sizeof(source),
        "%sstate int target\nstate int visits\nstate int before\nstate int after\n"
        "state bool ready\nstate bool skipped\nstate bool flags[2]\n"
        "func Outer():\n    state.visits = state.visits + 1\n    Inner()\n"
        "    state.visits = state.visits + 100\n"
        "func Inner():\n    let bool sent = targets.call(state.target, \"receive\")\n"
        "    state.ready = sent\n"
        "    state.visits = state.visits + 10\n"
        "func ResultId() -> int:\n    return targets.call_int(state.target, \"receive\")\n"
        "func ResultFloat() -> float:\n    return targets.call_float(state.target, \"receive\")\n"
        "func ResultBool() -> bool:\n    return targets.call_bool(state.target, \"receive\")\n"
        "%s"
        "        state.before = 1\n"
        "        let bool skipped = false and controls.try_ground_jump()\n"
        "        state.skipped = skipped\n"
        "        state.basic_landing()\n        Outer()\n        state.after = 9\n"
        "    on jab_start:\n        return ResultId()\n"
        "    on shield_scale:\n        return ResultFloat()\n"
        "    on worn_item_jump_sound:\n        return ResultBool()\n"
        "article It_Kind_Ness_PKFire states 1:\n"
        "    state bool ready\n    state int after\n"
        "    on signal typed_int:\n            item.state.after = targets.call_int(item.current_owner, \"receive\")\n"
        "    on signal typed_float:\n            let float value = targets.call_float(item.current_owner, \"receive\")\n            item.state.after = 15\n"
        "    on signal typed_bool:\n            item.state.ready = targets.call_bool(item.current_owner, \"receive\")\n"
        "    func RemoveOuter():\n        RemoveInner()\n        item.state.after = 100\n"
        "    func RemoveInner():\n        item.remove()\n        item.state.after = 101\n"
        "    func Relay():\n        item.signal(\"branch\", 0)\n        item.state.after = 102\n"
        "    on signal leaf:\n            targets.call(item.current_owner, \"receive\")\n"
        "    on signal branch:\n            item.signal(\"leaf\", 0)\n            item.state.after = 4\n"
        "    on created:\n"
        "            let bool sent = targets.call(item.current_owner, \"receive\")\n"
        "            item.state.ready = sent\n"
        "            creator.call(Outer)\n            item.state.after = 2\n"
        "    on spawned:\n            Relay()\n            item.state.after = 3\n"
        "    on damage_dealt:\n"
        "            if not targets.call(item.current_owner, \"receive\"):\n"
        "                item.state.after = 5\n"
        "            return true\n"
        "    on destroyed:\n            RemoveOuter()\n            item.state.after = 6\n"
        "    on picked_up:\n            RemoveOuter()\n            item.state.after = 7\n"
        "    on dropped:\n            creator.state.ready = targets.call(item.current_owner, \"receive\")\n"
        "            item.state.after = 11\n"
        "    on thrown:\n            creator.state.flags[0] = targets.call(item.current_owner, \"receive\")\n"
        "            item.state.after = 12\n"
        "    on entered_air:\n            owner.state.ready = targets.call(item.current_owner, \"receive\")\n"
        "            item.state.after = 13\n"
        "    state 0 anim -1:\n        on animation:\n"
        "            owner.state.flags[0] = targets.call(item.current_owner, \"receive\")\n"
        "            item.state.after = 14\n"
        "        on physics:\n"
        "            let bool expired = false and item.expire_tick()\n"
        "            item.state.ready = expired\n"
        "            item.grounded = targets.call(item.current_owner, \"receive\")\n"
        "            item.state.after = 8\n",
        callbacks ? "" : "",
        callbacks ? "move Lifetime:\n    on state_341_physics:\n" :
                    "action Lifetime:\n    on load:\n");
    assert(generate(source, path, true, callbacks ? &(const NativeFighterScriptConstruction){"ftDataNess",341,1} : NULL));
}

static void negative(const NativeFighterScriptConstruction* construction)
{
    const char* invalid[] = {
        "collision.air_landing_callback(None)",
        "collision.air_landing_callback(Missing)",
        "collision.air_landing_callback()",
        "collision.air_landing_callback(1)",
        "collision.air_landing_callback(Missing, Missing)",
        "let bool result = collision.contact_rebound_allowed(1)",
        "let int result = controls.aerial_attack_motion(1)",
        "let bool result = input.cstick_attack_edge(1)",
        "let bool result = controls.try_air_item_throw(1)",
        "let bool result = targets.call(1.5, \"receive\")",
        "let bool result = targets.call(1, \"bad-name\")",
        "let int result = targets.call(1, \"receive\")",
        "let result = targets.call(1, \"receive\")",
        "let bool result = targets.call(1, \"receive\") and true",
        "let bool result = not (targets.call(1, \"receive\"))",
        "let bool result = targets.call(targets.call(1, \"receive\"), \"receive\")",
        "velocity.x = targets.call(1, \"receive\")",
        "if true and targets.call(1, \"receive\"):\n            velocity.x = 2",
        "while targets.call(1, \"receive\"):\n            velocity.x = 2",
        "return targets.call(1, \"receive\")",
        "physics.scale_velocity(targets.call(1, \"receive\"))",
    };
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        char source[4096];
        snprintf(source, sizeof(source), "action Probe:\n    on load:\n        %s\n", invalid[i]);
        assert(!generate(source, NULL, false, construction));
        char indented[2048]; size_t used = 0;
        for (const char* p = invalid[i]; *p; ++p) {
            indented[used++] = *p;
            if (*p == '\n') { memcpy(indented + used, "    ", 4); used += 4; }
        }
        indented[used] = 0;
        snprintf(source, sizeof(source), "article It_Kind_Ness_PKFire states 1:\n"
            "    on created:\n            %s\n    state 0 anim -1:\n", indented);
        assert(!generate(source, NULL, false, construction));
    }
    assert(!generate("state int flags[2]\naction Probe:\n    on load:\n"
        "        state.flags[0] = targets.call(1, \"receive\")\n", NULL, false, construction));
    assert(!generate("article It_Kind_Ness_PKFire states 1:\n    state int flags[2]\n"
        "    on created:\n            item.state.flags[0] = targets.call(1, \"receive\")\n"
        "    state 0 anim -1:\n", NULL, false, construction));
    assert(!generate("article It_Kind_Ness_PKFire states 1:\n"
        "    projectile 0 0 targets.call(1, \"receive\") 1 1\n"
        "    state 0 anim -1:\n", NULL, false, construction));
    // Direct local signal edges only target preceding declarations: self and
    // forward edges are rejected, so a local signal cycle cannot be emitted.
    assert(!generate("article It_Kind_Ness_PKFire states 1:\n"
        "    on signal cycle:\n            item.signal(\"cycle\", 0)\n"
        "    state 0 anim -1:\n", NULL, false, construction));
}

int main(int argc, char** argv)
{
    if (argc != 1 && argc != 3) return 2;
    negative(NULL);
    positive(false, argc == 3 ? argv[1] : NULL);
    positive(true, argc == 3 ? argv[2] : NULL);
    puts("Synchronous effects lower at exact calls, preserve helper closure and reject unordered target dispatch.");
    return 0;
}
