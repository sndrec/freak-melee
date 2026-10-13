#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "script_lifetime_fixture.h"

#include <melee/ft/native_fighter_module.h>
#include <melee/ft/types.h>
#include <sysdolphin/baselib/gobj.h>

#include <stdio.h>

typedef const NativeFighterDefinition* (__cdecl* ModuleEntry)(
    const NativeFighterDefinition*, const NativeFighterHost*);

static unsigned articles;

static void change_motion(Fighter_GObj* object, FtMotionId state,
                          MotionFlags flags, float start, float speed,
                          float blend, Fighter_GObj* other)
{
    Fighter* fighter = object->user_data;
    (void) flags;
    (void) start;
    (void) speed;
    (void) blend;
    (void) other;
    fighter->motion_id = state;
    if (fighter->x21EC != NULL) {
        HSD_GObjEvent exit = fighter->x21EC;
        exit(object);
        fighter->x21EC = NULL;
    }
    fighter->accessory4_cb = NULL;
}

static void start_animation(Fighter_GObj* object) { (void) object; }
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
}
static void air_to_ground(Fighter* fighter)
{
    fighter->ground_or_air = GA_Ground;
}
static void stock_article(Fighter_GObj* object)
{
    Fighter* fighter = object->user_data;
    if (fighter->throw_flags_b0) {
        ++articles;
        fighter->throw_flags_b0 = 0;
    }
}

int main(int argc, char** argv)
{
    if (argc != 2) return 2;
    HMODULE library = LoadLibraryExA(argv[1], NULL,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (library == NULL) return 1;
    ModuleEntry entry = (ModuleEntry) GetProcAddress(
        library, "FreakMelee_GetFighterModule");
    MotionState states[2] = { 0 };
    NativeFighterDefinition base = {
        .motion_states = states,
        .motion_state_count = 2,
        .motion_state_first = 343,
        .special_ground_state = { [NativeFighterSpecial_Neutral] = 343 },
        .special_air_state = { [NativeFighterSpecial_Neutral] = 344 },
        .neutral_article = stock_article,
    };
    NativeFighterHost host = {
        .callback_source_alive = script_lifetime_alive,
        .change_motion_state = change_motion,
        .start_animation = start_animation,
        .ground_collision = ground_collision,
        .air_collision = air_collision,
        .ground_to_air = ground_to_air,
        .air_to_ground = air_to_ground,
    };
    const NativeFighterDefinition* authored = entry != NULL
        ? entry(&base, &host) : NULL;
    Fighter fighter = {
        .facing_dir = 1,
        .ground_or_air = GA_Ground,
        .gr_vel = 3,
        .self_vel.x = 4,
    };
    HSD_GObj object = { .user_data = &fighter };
    script_lifetime_register(&object, NativeCallbackSource_Fighter);
    bool ok = authored != NULL && authored->state_bytes == 1 &&
              authored->special_n != NULL &&
              authored->special_air_n != NULL;
    if (ok) {
        authored->special_n(&object);
        HSD_GObjEvent cue = fighter.accessory4_cb;
        ok = fighter.motion_id == 343 && cue != NULL &&
             fighter.gr_vel == 3 && fighter.self_vel.x == 4;
        if (ok) {
            fighter.throw_flags_b0 = 1;
            cue(&object);
            fighter.throw_flags_b0 = 1;
            cue(&object);
            ok = articles == 1 && !fighter.throw_flags_b0 &&
                 fighter.native_module_state[0] == 1 &&
                 fighter.gr_vel == 3 && fighter.self_vel.x == 4;
            change_motion(&object, 14, 0, 0, 1, 0, NULL);
            authored->special_n(&object);
            fighter.throw_flags_b0 = 1;
            fighter.accessory4_cb(&object);
            ok = ok && articles == 2 && fighter.gr_vel == 3 &&
                 fighter.self_vel.x == 4;
            fighter.ground_or_air = GA_Air;
            authored->special_air_n(&object);
            ok = ok && fighter.motion_id == 344 &&
                 fighter.gr_vel == 3 && fighter.self_vel.x == 4;
        }
    }
    FreeLibrary(library);
    if (!ok) fputs("Article-only script changed movement or cue lifetime.\n",
                   stderr);
    return ok ? 0 : 1;
}
