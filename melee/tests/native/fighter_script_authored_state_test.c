#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "script_lifetime_fixture.h"

#include <melee/ft/native_fighter_module.h>
#include <melee/ft/types.h>
#include <sysdolphin/baselib/gobj.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef const NativeFighterDefinition* (__cdecl* ModuleEntry)(
    const NativeFighterDefinition*, const NativeFighterHost*);

static unsigned changed[2];
static unsigned started[2];
static unsigned ground_to_air_calls;
static unsigned air_to_ground_calls;
static unsigned article_calls[2];
static unsigned effect_calls[2];
static bool wrong_resource;
static unsigned finished_ground[2];
static unsigned entered_fall[2];
static unsigned ground_actions[2];
static unsigned air_actions[2];
static unsigned ground_physics[2];
static unsigned air_physics[2];
static bool frames_remaining = true;
static MotionFlags last_flags[2];
static float last_frame[2];

static void change_motion(Fighter_GObj* object, FtMotionId state,
                          MotionFlags flags, float start, float speed,
                          float blend, Fighter_GObj* other)
{
    Fighter* fighter = object->user_data;
    unsigned slot = fighter->facing_dir < 0 ? 1 : 0;
    if (speed != 1 || blend != 0 || other != NULL)
        changed[slot] = 99;
    else
        ++changed[slot];
    last_flags[slot] = flags;
    last_frame[slot] = start;
    fighter->motion_id = state;
    if (fighter->x21EC != NULL) {
        HSD_GObjEvent exit = fighter->x21EC;
        exit(object);
        fighter->x21EC = NULL;
    }
    fighter->accessory4_cb = NULL;
}

static void start_animation(Fighter_GObj* object)
{
    Fighter* fighter = object->user_data;
    ++started[fighter->facing_dir < 0 ? 1 : 0];
}

static void old_stock_entry(Fighter_GObj* object)
{
    Fighter* fighter = object->user_data;
    fighter->native_module_state[0] = 1;
}

static Fighter_Part resolve_bone(Fighter* fighter, Fighter_Part part)
{
    (void) fighter;
    if (part != FtPart_L1stNb) wrong_resource = true;
    return 0;
}

static void joint_position(HSD_JObj* joint, Vec3* origin, Vec3* result)
{
    if (joint == NULL || origin != NULL) wrong_resource = true;
    *result = (Vec3){ 1, 2, 3 };
}

static void spawn_fireball(Fighter_GObj* object, Vec3* position,
                           ItemKind kind, s32 variant, float facing)
{
    Fighter* fighter = object->user_data;
    if (kind != It_Kind_Mario_Fire || position->x != 1 ||
        position->y != 2 || variant != 0 ||
        facing != fighter->facing_dir ||
        fighter->throw_flags_b0) wrong_resource = true;
    ++article_calls[fighter->facing_dir < 0 ? 1 : 0];
}

static EF_SpawnResult spawn_effect(s32 effect, HSD_GObj* object, ...)
{
    Fighter* fighter = object->user_data;
    if (effect != 1146) wrong_resource = true;
    ++effect_calls[fighter->facing_dir < 0 ? 1 : 0];
    return (EF_SpawnResult){ 0 };
}

static bool ground_collision(Fighter_GObj* object)
{
    (void) object;
    return false;
}

static GroundOrAir air_collision(Fighter_GObj* object)
{
    (void) object;
    return GA_Air;
}

static void ground_to_air(Fighter* fighter)
{
    fighter->ground_or_air = GA_Air;
    ++ground_to_air_calls;
}

static void air_to_ground(Fighter* fighter)
{
    fighter->ground_or_air = GA_Ground;
    ++air_to_ground_calls;
}

static bool animation_frames_remaining(Fighter_GObj* object)
{
    (void) object;
    return frames_remaining;
}

static void finish_ground_action(Fighter_GObj* object)
{
    Fighter* fighter = object->user_data;
    ++finished_ground[fighter->facing_dir < 0 ? 1 : 0];
}

static void enter_fall(Fighter_GObj* object)
{
    Fighter* fighter = object->user_data;
    ++entered_fall[fighter->facing_dir < 0 ? 1 : 0];
}

static void try_ground_actions(Fighter_GObj* object)
{
    Fighter* fighter = object->user_data;
    ++ground_actions[fighter->facing_dir < 0 ? 1 : 0];
}

static bool try_air_actions(Fighter_GObj* object)
{
    Fighter* fighter = object->user_data;
    ++air_actions[fighter->facing_dir < 0 ? 1 : 0];
    return true;
}

static void ground_friction(Fighter_GObj* object)
{
    Fighter* fighter = object->user_data;
    ++ground_physics[fighter->facing_dir < 0 ? 1 : 0];
}

static void air_drift(Fighter_GObj* object)
{
    Fighter* fighter = object->user_data;
    ++air_physics[fighter->facing_dir < 0 ? 1 : 0];
}

int main(int argc, char** argv)
{
    if (argc != 2) return 2;
    HMODULE library = LoadLibraryExA(argv[1], NULL,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (library == NULL) return 1;
    ModuleEntry entry = (ModuleEntry) GetProcAddress(
        library, "FreakMelee_GetFighterModule");
    MotionState states[2] = {
        { .anim_cb = old_stock_entry, .input_cb = old_stock_entry,
          .phys_cb = old_stock_entry, .coll_cb = old_stock_entry },
        { .anim_cb = old_stock_entry, .input_cb = old_stock_entry,
          .phys_cb = old_stock_entry, .coll_cb = old_stock_entry },
    };
    NativeFighterDefinition base = {
        .motion_states = states,
        .motion_state_count = 2,
        .motion_state_first = 343,
        .special_ground_state = { [NativeFighterSpecial_Neutral] = 343 },
        .special_air_state = { [NativeFighterSpecial_Neutral] = 344 },
        .special_transition_flags = {
            [NativeFighterSpecial_Neutral] = Ft_MF_SkipColAnim | Ft_MF_UpdateCmd,
        },
        .special_n = old_stock_entry,
        .special_air_n = old_stock_entry,
    };
    NativeFighterHost host = {
        .callback_source_alive = script_lifetime_alive,
        .change_motion_state = change_motion,
        .start_animation = start_animation,
        .ground_collision = ground_collision,
        .air_collision = air_collision,
        .ground_to_air = ground_to_air,
        .air_to_ground = air_to_ground,
        .animation_frames_remaining = animation_frames_remaining,
        .finish_ground_action = finish_ground_action,
        .enter_fall = enter_fall,
        .ground_actions = try_ground_actions,
        .air_actions = try_air_actions,
        .ground_friction = ground_friction,
        .air_drift = air_drift,
        .resolve_bone = resolve_bone,
        .joint_position = joint_position,
        .spawn_projectile = spawn_fireball,
        .spawn_effect = spawn_effect,
    };
    NativeFighterDefinition missing = base;
    missing.special_ground_state[NativeFighterSpecial_Neutral] = 0;
    bool ok = entry != NULL && entry(&missing, &host) == NULL;
    missing = base;
    NativeFighterHost no_article = host;
    no_article.spawn_projectile = NULL;
    ok = ok && entry(&missing, &no_article) == NULL;
    NativeFighterHost no_animation = host;
    no_animation.start_animation = NULL;
    ok = ok && entry(&base, &no_animation) == NULL;
    no_animation = host;
    no_animation.animation_frames_remaining = NULL;
    ok = ok && entry(&base, &no_animation) == NULL;
    no_animation = host;
    no_animation.air_drift = NULL;
    ok = ok && entry(&base, &no_animation) == NULL;
    const NativeFighterDefinition* authored = ok ? entry(&base, &host) : NULL;
    ok = authored != NULL && authored->special_n != NULL &&
         authored->special_air_n != NULL &&
         authored->state_bytes == 1 &&
         authored->motion_states != states &&
         authored->motion_states[0].coll_cb != old_stock_entry &&
         authored->motion_states[1].coll_cb != old_stock_entry &&
         authored->motion_states[0].anim_cb != old_stock_entry &&
         authored->motion_states[1].anim_cb != old_stock_entry &&
         authored->motion_states[0].input_cb != old_stock_entry &&
         authored->motion_states[1].input_cb != old_stock_entry &&
         authored->motion_states[0].phys_cb != old_stock_entry &&
         authored->motion_states[1].phys_cb != old_stock_entry &&
         states[0].anim_cb == old_stock_entry &&
         states[1].anim_cb == old_stock_entry &&
         states[0].input_cb == old_stock_entry &&
         states[1].input_cb == old_stock_entry &&
         states[0].phys_cb == old_stock_entry &&
         states[1].phys_cb == old_stock_entry &&
         states[0].coll_cb == old_stock_entry &&
         states[1].coll_cb == old_stock_entry;
    Fighter* fighters = ok ? calloc(2, sizeof(*fighters)) : NULL;
    ok = ok && fighters != NULL;
    if (ok) {
        HSD_GObj objects[2] = { 0 };
        objects[0].user_data = &fighters[0];
        script_lifetime_register(&objects[0], NativeCallbackSource_Fighter);
        objects[1].user_data = &fighters[1];
        script_lifetime_register(&objects[1], NativeCallbackSource_Fighter);
        fighters[0].facing_dir = 1;
        fighters[1].facing_dir = -1;
        FighterBone bones[2] = {
            { .joint = (HSD_JObj*) &objects[0] },
            { .joint = (HSD_JObj*) &objects[1] },
        };
        fighters[0].parts = &bones[0];
        fighters[1].parts = &bones[1];
        fighters[0].ground_or_air = GA_Ground;
        fighters[1].ground_or_air = GA_Air;
        fighters[0].cmd_vars[0] = 4;
        fighters[1].cmd_vars[0] = 5;
        fighters[0].throw_flags = 7;
        fighters[1].throw_flags = 8;
        fighters[0].accessory4_cb = old_stock_entry;
        fighters[1].accessory4_cb = old_stock_entry;
        authored->special_n(&objects[0]);
        authored->special_air_n(&objects[1]);
        if (ok) {
            authored->motion_states[0].anim_cb(&objects[0]);
            authored->motion_states[1].anim_cb(&objects[1]);
            frames_remaining = false;
            authored->motion_states[0].anim_cb(&objects[0]);
            authored->motion_states[1].anim_cb(&objects[1]);
            ok = finished_ground[0] == 1 && entered_fall[1] == 1 &&
                 finished_ground[1] == 0 && entered_fall[0] == 0;
            authored->motion_states[0].input_cb(&objects[0]);
            authored->motion_states[1].input_cb(&objects[1]);
            fighters[0].cmd_vars[0] = 1;
            fighters[1].cmd_vars[0] = 1;
            authored->motion_states[0].input_cb(&objects[0]);
            authored->motion_states[1].input_cb(&objects[1]);
            authored->motion_states[0].phys_cb(&objects[0]);
            authored->motion_states[1].phys_cb(&objects[1]);
            ok = ok && ground_actions[0] == 1 && air_actions[1] == 1 &&
                 ground_actions[1] == 0 && air_actions[0] == 0 &&
                 ground_physics[0] == 1 && air_physics[1] == 1;
            fighters[0].cmd_vars[0] = 0;
            fighters[1].cmd_vars[0] = 0;
        }
        ok = ok && changed[0] == 1 && changed[1] == 1 &&
             started[0] == 1 && started[1] == 1 &&
             fighters[0].motion_id == 343 &&
             fighters[1].motion_id == 344 &&
             last_flags[0] == 0 && last_flags[1] == 0 &&
             fighters[0].cmd_vars[0] == 0 &&
             fighters[1].cmd_vars[0] == 0 &&
             fighters[0].throw_flags == 0 &&
             fighters[1].throw_flags == 0 &&
             fighters[0].accessory4_cb != NULL &&
             fighters[0].accessory4_cb == fighters[1].accessory4_cb &&
             fighters[0].accessory4_cb != old_stock_entry &&
             fighters[0].x21EC != NULL &&
             fighters[0].x21EC == fighters[1].x21EC &&
             fabsf(fighters[0].gr_vel - 0.8f) < 0.00001f &&
             fabsf(fighters[1].self_vel.x + 0.4f) < 0.00001f &&
             fighters[0].self_vel.y == 0 &&
             fabsf(fighters[1].self_vel.y - 0.25f) < 0.00001f &&
             fighters[0].native_module_state[0] == 0 &&
             fighters[1].native_module_state[0] == 0;
        if (ok) {
            HSD_GObjEvent cue = fighters[0].accessory4_cb;
            cue(&objects[0]);
            cue(&objects[1]);
            ok = fabsf(fighters[0].gr_vel - 0.8f) < 0.00001f &&
                 fabsf(fighters[1].self_vel.x + 0.4f) < 0.00001f;
            fighters[0].throw_flags_b0 = 1;
            fighters[1].throw_flags_b0 = 1;
            cue(&objects[0]);
            cue(&objects[1]);
            cue(&objects[0]);
            cue(&objects[1]);
            ok = ok && !fighters[0].throw_flags_b0 &&
                 !fighters[1].throw_flags_b0 &&
                 article_calls[0] == 1 && article_calls[1] == 1 &&
                 effect_calls[0] == 1 && effect_calls[1] == 1 &&
                 !wrong_resource &&
                 fighters[0].native_module_state[0] == 1 &&
                 fighters[1].native_module_state[0] == 1 &&
                 fabsf(fighters[0].gr_vel - 1.5f) < 0.00001f &&
                 fabsf(fighters[1].self_vel.x + 1.5f) < 0.00001f &&
                 fighters[0].self_vel.y == 0 &&
                 fabsf(fighters[1].self_vel.y - 0.5f) < 0.00001f;
            fighters[0].cur_anim_frame = 17;
            fighters[1].cur_anim_frame = 18;
            fighters[0].accessory4_cb = old_stock_entry;
            fighters[1].accessory4_cb = old_stock_entry;
            authored->motion_states[0].coll_cb(&objects[0]);
            authored->motion_states[1].coll_cb(&objects[1]);
            ok = changed[0] == 2 && changed[1] == 2 &&
                 ground_to_air_calls == 1 && air_to_ground_calls == 1 &&
                 fighters[0].motion_id == 344 &&
                 fighters[1].motion_id == 343 &&
                 last_flags[0] == base.special_transition_flags[NativeFighterSpecial_Neutral] &&
                 last_flags[1] == base.special_transition_flags[NativeFighterSpecial_Neutral] &&
                 last_frame[0] == 17 && last_frame[1] == 18 &&
                 fighters[0].accessory4_cb == cue &&
                 fighters[1].accessory4_cb == cue &&
                 fighters[0].x21EC != NULL &&
                 fighters[1].x21EC != NULL &&
                 fighters[0].native_module_state[0] == 1 &&
                 fighters[1].native_module_state[0] == 1;
            fighters[0].throw_flags_b0 = 1;
            fighters[1].throw_flags_b0 = 1;
            cue(&objects[0]);
            cue(&objects[1]);
            ok = ok && fighters[0].self_vel.x == 0 &&
                 fighters[1].gr_vel == 0 &&
                 !fighters[0].throw_flags_b0 &&
                 !fighters[1].throw_flags_b0 &&
                 article_calls[0] == 1 && article_calls[1] == 1 &&
                 effect_calls[0] == 1 && effect_calls[1] == 1;
            change_motion(&objects[0], 14, 0, 0, 1, 0, NULL);
            ok = ok && fighters[0].native_module_state[0] == 0 &&
                 fighters[1].native_module_state[0] == 1 &&
                 fighters[0].x21EC == NULL;
            fighters[0].ground_or_air = GA_Ground;
            authored->special_n(&objects[0]);
            fighters[0].throw_flags_b0 = 1;
            fighters[0].accessory4_cb(&objects[0]);
            ok = ok && article_calls[0] == 2 && article_calls[1] == 1 &&
                 fighters[0].native_module_state[0] == 1;
        }
    }
    free(fighters);
    FreeLibrary(library);
    if (!ok) fputs("Authored state entry did not use the host correctly.\n",
                   stderr);
    return ok ? 0 : 1;
}
