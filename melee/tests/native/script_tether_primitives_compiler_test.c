#include "../../tools/native_fighter_script.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; assert(x); } while (0)

static bool generate(const char* source, const char* path, bool inspect, const NativeFighterScriptConstruction* construction)
{
    FILE* file = path ? fopen(path, "w+") : tmpfile(); CHECK(file);
    NativeFighterScriptError error = {0};
    bool ok = nativeFighterScript_GenerateC(source, strlen(source),
        "tether_primitives", NULL, NULL, construction, file, &error);
    if (!ok && inspect) fprintf(stderr, "Tether primitives line %u: %s\n", error.line, error.message);
    if (ok && inspect) {
        char code[131072]; rewind(file);
        size_t count = fread(code, 1, sizeof(code)-1, file); CHECK(feof(file)); code[count] = 0;
        const char* required[] = { "stage_segment_intersection", "play_global_sound", "check_fastfall",
            "fastfall", "probe_ledge", "find_ledge_occupant", "enter_cliff_jump_phase2",
            "air_collision", "enter_damage_fall", "enter_freefall", "callback_source_alive" };
        for (unsigned i = 0; i < sizeof(required)/sizeof(*required); ++i) {
            char needle[128]; snprintf(needle, sizeof(needle), "host->%s == 0", required[i]);
            CHECK(strstr(code, needle));
        }
        CHECK(strstr(code, "engine->stage_segment_intersection(NULL, NULL, NULL, NULL, -1, -1,"));
        CHECK(strstr(code, "engine->find_ledge_occupant(gobj) == NULL"));
        CHECK(strstr(code, "engine->air_collision(gobj) != GA_Ground"));
        CHECK(strstr(code, "fighter->pos_delta.z ="));
        CHECK(strstr(code, "fighter->x74_self_accel.z ="));
        CHECK(strstr(code, "Collide_LeftWallMask") && strstr(code, "Collide_RightWallMask"));
        CHECK(!strstr(code, "ftCo_800C3B10") && !strstr(code, "ftCo_8009B390"));
    }
    fclose(file); return ok;
}

static void positive(bool callbacks, const char* path)
{
    char source[8192];
    snprintf(source, sizeof(source),
        "%sstate bool continued\nstate bool blocked\nstate bool fast\nstate bool available\nstate bool in_air\n"
        "state bool shield\nstate bool was_used\nstate bool was_fast\nstate int sound\nstate int masks\n"
        "state float lag\nstate float cx\nstate float cy\nstate float cz\n"
        "func Read():\n"
        "    state.was_used = movement.used_tether\n"
        "    state.was_fast = movement.fastfall\n"
        "    state.shield = input.shield_held\n"
        "    state.lag = movement.special_landing_lag\n"
        "    state.cx = collision.position.x\n    state.cy = collision.position.y\n    state.cz = collision.position.z\n"
        "    state.blocked = stage.segment_blocked(1,2,3,4)\n"
        "    state.sound = audio.play_global(int(701),int(127),int(64))\n"
        "    state.fast = physics.check_fastfall()\n"
        "    state.available = collision.probe_ledge(5,6) and collision.ledge_available()\n"
        "    state.in_air = collision.air()\n"
        "    state.masks = Collision.left_wall_mask | Collision.right_wall_mask\n"
        "    position.delta.x = 9\n    position.delta.y = 10\n    position.delta.z = 11\n"
        "    acceleration.z = 12\n    movement.used_tether = true\n"
        "    physics.fastfall()\n    callbacks.death2(Finish)\n"
        "func Finish():\n    state.freefall(int(1), int(0), input.shield_held, 1, movement.special_landing_lag)\n    state.continued = true\n"
        "%s        Read()\n"
        "    on damage:\n        state.damage_fall()\n"
        "    on death1:\n        state.cliff_jump_phase2(1.5)\n        state.continued = true\n"
        "    on air_tether_input:\n        state.damage_fall()\n        movement.used_tether = false\n        return true\n"
        "article It_Kind_Link_HShot states 1:\n"
        "    state bool blocked\n    state int sound\n"
        "    on created:\n"
        "            let bool blocked = stage.segment_blocked(1,2,3,4)\n"
        "            let int sound = audio.play_global(int(702),int(100),int(32))\n"
        "            item.state.blocked = blocked\n            item.state.sound = sound\n"
        "            stage.segment_blocked(1,2,3,4)\n"
        "            audio.play_global(int(703),int(101),int(33))\n"
        "    state 0 anim -1:\n",
        callbacks ? "" : "",
        callbacks ? "move Probe:\n    on state_341_physics:\n" : "action Probe:\n    on load:\n");
    CHECK(generate(source,path,true, callbacks ? &(const NativeFighterScriptConstruction){"ftDataLink",341,1} : NULL));
}

static void negatives(const NativeFighterScriptConstruction* construction)
{
    const char* bad[] = {
        "audio.play_global(1.0,2,3)", "audio.play_global(true,int(2),int(3))",
        "audio.play_global(int(1),2.5,int(3))", "audio.play_global(int(1),int(2))",
        "stage.segment_blocked(1,2,3)", "stage.segment_blocked(1,2,3,4,5)",
        "stage.segment_blocked(1,2,3,missing)", "collision.probe_ledge(5)",
        "collision.probe_ledge(5,6,7)", "collision.ledge_available(1)",
        "physics.check_fastfall(1)", "collision.air(1)", "physics.fastfall(1)",
        "state.damage_fall(1)", "state.cliff_jump_phase2()",
        "state.freefall(1,0,true,1)", "state.freefall(1,0,true,1,2,3)",
        "state.freefall(1.5,0,true,1,2)", "state.freefall(true,0,true,1,2)",
        "state.freefall(1,0,1,1,2)", "state.freefall(1,0,true,missing,2)",
        "let bool x = physics.fastfall()", "let bool x = state.damage_fall()",
        "movement.used_tether = 1", "movement.used_tether = 1.5", "position.delta.x = physics.fastfall()",
        "movement.fastfall = 1", "movement.fastfall = 0.5", "movement.special_landing_lag = 3",
        "collision.position.x = 3", "position.delta.w = 3", "acceleration.w = 3",
        "movement.used_tether()", "movement.fastfall()", "input.shield_held()",
        "collision.position.x()", "movement.special_landing_lag()",
        "let int x = stage.segment_blocked(1,2,3,4) + 0.5",
    };
    char source[2048];
    for (unsigned i=0;i<sizeof(bad)/sizeof(*bad);++i) {
        snprintf(source,sizeof(source),"action Bad:\n    on load:\n        %s\n",bad[i]);
        if (generate(source,NULL,false, construction)) { fprintf(stderr,"Accepted invalid fighter: %s\n",bad[i]); abort(); }
        ++checks;
    }
    const char* contexts[] = { "physics.check_fastfall()", "physics.fastfall()", "collision.probe_ledge(5,6)",
        "collision.ledge_available()", "collision.air()", "state.damage_fall()",
        "state.cliff_jump_phase2(1)", "let bool x = movement.used_tether", "let bool x = input.shield_held",
        "let float x = acceleration.z", "position.delta.x = 2", "movement.used_tether = true" };
    for (unsigned i=0;i<sizeof(contexts)/sizeof(*contexts);++i) {
        snprintf(source,sizeof(source),"article It_Kind_Link_HShot states 1:\n    on created:\n            %s\n    state 0 anim -1:\n",contexts[i]);
        CHECK(!generate(source,NULL,false, construction));
    }
    const char* effect[] = { "stage.segment_blocked(1,2,3,4)", "audio.play_global(int(1),int(2),int(3))",
        "physics.check_fastfall()", "physics.fastfall()", "collision.probe_ledge(5,6)",
        "collision.ledge_available()", "collision.air()", "position.delta.x = 2", "movement.used_tether = true" };
    for (unsigned i=0;i<sizeof(effect)/sizeof(*effect);++i) {
        snprintf(source,sizeof(source),"model_effect Bad 0x509:\n    on update:\n        %s\naction Probe:\n    on load:\n        pass\n",effect[i]);
        CHECK(!generate(source,NULL,false, construction));
    }
}

int main(int argc,char** argv)
{
    negatives(NULL); positive(false,argc>1?argv[1]:NULL); positive(true,argc>2?argv[2]:NULL);
    printf("{\"compiler_checks\":%u}\n",checks); return 0;
}
