#include "../../../tools/native_fighter_script.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned programs;
static void generate(const char* source, bool expected, const char* path, const NativeFighterScriptConstruction* construction)
{
    FILE* output = path ? fopen(path, "w") : tmpfile(); assert(output);
    NativeFighterScriptError error = {0}; ++programs;
    bool ok = nativeFighterScript_GenerateC(source, strlen(source), "relations", NULL, NULL, construction, output, &error);
    if (ok != expected) fprintf(stderr, "Unexpected %s at %u: %s\n%s\n", ok ? "accept" : "reject", error.line, error.message, source);
    assert(ok == expected); if (!ok) assert(error.line); fclose(output);
}
int main(int argc, char** argv)
{
    assert(argc == 1 || argc == 3);
    const char* invalid[] = {
        "hitlag.link(1.25)", "family.member_id()", "family.member_id(1.0)", "collision.copy_from(false)",
        "airborne = 1", "spawn.horizontal_offset = true", "cpu.pending_action = 1", "cpu.travel_frames = 1.5",
        "hitlag.active = true", "fighter.incapacitated = false", "hitlag.linked_id = 3",
        "targets.read_bone_position(1, 0, state.x, state.x, state.z)",
        "targets.read_bone_position(1, 0, state.peer, state.y, state.z)",
        "targets.read_bone_position(1, 0.5, state.x, state.y, state.z)",
        "targets.read_bone_position(1.5, 0, state.x, state.y, state.z)",
        "targets.read_bone_position(1, 0, 0, state.y, state.z)",
        "hitlag.request_release(1)", "hitlag.unlink(1)",
    };
    for (unsigned route = 0; route < 2; ++route) {
        const char* prefix = route ? "" : "";
        const char* entry = route ? "move Relations:\n    on state_341_physics:\n" : "action Relations:\n    on load:\n";
        char source[4096];
        snprintf(source, sizeof(source), "%sstate int peer\nstate int linked\nstate int after\nstate bool bound\n"
            "state bool copied\nstate bool found\nstate bool active\nstate bool incapacitated\n"
            "state float x\nstate float y\nstate float z\n%s"
            "        state.peer = family.member_id(1)\n        state.bound = hitlag.link(state.peer)\n"
            "        state.linked = hitlag.linked_id\n        state.copied = collision.copy_from(state.peer)\n"
            "        state.active = hitlag.active\n        state.incapacitated = fighter.incapacitated\n"
            "        spawn.horizontal_offset = 7.25\n        airborne = true\n"
            "        cpu.travel_frames = cpu.travel_frames + 1\n        cpu.pending_action = true\n"
            "        state.found = targets.read_bone_position(state.peer, 2, state.x, state.y, state.z)\n"
            "        hitlag.request_release()\n        hitlag.unlink()\n        state.after = 99\n"
            "%s    on cpu_pending_action:\n        return cpu.pending_action\n",
            prefix, entry, route ? "move Pending:\n" : "action Pending:\n");
        generate(source, true, argc == 3 ? argv[route+1] : NULL, route ? &(const NativeFighterScriptConstruction){"ftDataTest",341,1} : NULL);
        // These properties are emitted as integers and must also type-check as
        // integers without a cast (float-to-int assignment is deliberately rejected).
        snprintf(source, sizeof(source), "%s%s        let int category = action.category\n"
            "        let int move_id = action.move_id\n", prefix, entry);
        generate(source, true, NULL, route ? &(const NativeFighterScriptConstruction){"ftDataTest",341,1} : NULL);
        for (unsigned i = 0; i < sizeof(invalid)/sizeof(*invalid); ++i) {
            snprintf(source, sizeof(source), "%sstate int peer\nstate float x\nstate float y\nstate float z\n%s        %s\n", prefix, entry, invalid[i]);
            generate(source, false, NULL, route ? &(const NativeFighterScriptConstruction){"ftDataTest",341,1} : NULL);
        }
        snprintf(source, sizeof(source), "%s%s        let float x = 7\n        let float y = 8\n        let float z = 9\n"
            "        let bool found = targets.read_bone_position(1, 0, x, y, z)\n", prefix, entry);
        generate(source, true, NULL, route ? &(const NativeFighterScriptConstruction){"ftDataTest",341,1} : NULL);
    }
    const char* foreign[] = {"hitlag.link(1)", "family.member_id(1)", "collision.copy_from(1)",
        "hitlag.request_release()", "hitlag.unlink()", "cpu.pending_action = true", "cpu.travel_frames = 1"};
    for (unsigned i=0; i<sizeof(foreign)/sizeof(*foreign); ++i) {
        char source[1024]; snprintf(source,sizeof(source),"article It_Kind_Link_HShot states 1:\n    state 0 anim -1:\n        on physics:\n            %s\n", foreign[i]);
        generate(source, false, NULL, NULL);
    }
    printf("PASS %u paired relations compiler programs in both generator routes\n", programs);
}
