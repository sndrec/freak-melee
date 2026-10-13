#include <melee/ft/native_fighter_module.h>
#include <melee/ft/types.h>
#include <sysdolphin/baselib/gobj.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static void stock_ground(HSD_GObj* object)
{
    Fighter* fighter = object->user_data;
    fighter->native_module_state[0] = 7;
}

static void stock_air(HSD_GObj* object)
{
    Fighter* fighter = object->user_data;
    fighter->native_module_state[0] = 9;
}

static HSD_GObjEvent ground_entry(const NativeFighterDefinition* definition,
                                 unsigned kind)
{
    return kind == 0 ? definition->special_s
         : kind == 1 ? definition->special_hi : definition->special_lw;
}

static HSD_GObjEvent air_entry(const NativeFighterDefinition* definition,
                              unsigned kind)
{
    return kind == 0 ? definition->special_air_s
         : kind == 1 ? definition->special_air_hi
                     : definition->special_air_lw;
}

static void clear_ground(NativeFighterDefinition* definition, unsigned kind)
{
    if (kind == 0) definition->special_s = NULL;
    if (kind == 1) definition->special_hi = NULL;
    if (kind == 2) definition->special_lw = NULL;
}

int main(int argc, char** argv)
{
    if (argc != 4) return 2;
    MotionState state = { .anim_id = 0 };
    NativeFighterDefinition base = {
        .code_key = "mario_clone",
        .source_kind = Ft_Kind_Mario,
        .animation_source_kind = Ft_Kind_Mario,
        .fighter_root = "ftDataMario",
        .costume_joint = "PlyMario5K_Share_joint",
        .costume_matanim = "PlyMario5K_Share_matanim_joint",
        .action_count = 303,
        .demo_count = 16,
        .motion_state_count = 1,
        .motion_states = &state,
        .joint_count = 61,
        .special_n = stock_ground,
        .special_air_n = stock_air,
        .special_s = stock_ground,
        .special_air_s = stock_air,
        .special_hi = stock_ground,
        .special_air_hi = stock_air,
        .special_lw = stock_ground,
        .special_air_lw = stock_air,
    };
    Fighter* fighters = calloc(2, sizeof(*fighters));
    if (fighters == NULL) return 1;
    HSD_GObj objects[2] = { 0 };
    objects[0].user_data = &fighters[0];
    objects[1].user_data = &fighters[1];
    bool ok = true;
    for (unsigned kind = 0; kind < 3 && ok; ++kind) {
        NativeFighterDefinition missing = base;
        clear_ground(&missing, kind);
        NativeFighterModule rejected = { 0 };
        if (nativeFighterModule_Open(argv[kind + 1], &missing, &rejected)) {
            nativeFighterModule_Close(&rejected);
            ok = false;
            break;
        }
        NativeFighterModule module = { 0 };
        if (!nativeFighterModule_Open(argv[kind + 1], &base, &module)) {
            ok = false;
            break;
        }
        fighters[0] = (Fighter){ 0 };
        fighters[1] = (Fighter){ 0 };
        fighters[0].facing_dir = 1.0f;
        fighters[1].facing_dir = -1.0f;
        ground_entry(module.definition, kind)(&objects[0]);
        air_entry(module.definition, kind)(&objects[1]);
        ok = ground_entry(module.definition, kind) != stock_ground &&
             air_entry(module.definition, kind) != stock_air &&
             module.definition->special_n == stock_ground &&
             module.definition->special_air_n == stock_air &&
             fabsf(fighters[0].gr_vel - 0.5f) < 0.00001f &&
             fabsf(fighters[1].self_vel.x - 0.25f) < 0.00001f &&
             fighters[0].native_module_state[0] == 7 &&
             fighters[1].native_module_state[0] == 9 &&
             fighters[0].self_vel.x == 0.0f &&
             fabsf(fighters[1].self_vel.y -
                   (kind == 1 ? 0.75f : 0.0f)) < 0.00001f &&
             fighters[1].gr_vel == 0.0f;
        nativeFighterModule_Close(&module);
    }
    free(fighters);
    if (!ok) {
        fputs("Stock special script changed the wrong callback or velocity.\n",
              stderr);
        return 1;
    }
    puts("Side, up, and down specials preserved stock entry before speed edits.");
    return 0;
}
