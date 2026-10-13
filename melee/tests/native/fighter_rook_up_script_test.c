#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "script_lifetime_fixture.h"

#include <melee/ft/native_fighter_module.h>
#include <sysdolphin/baselib/gobj.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef const NativeFighterDefinition* (__cdecl* ModuleEntry)(
    const NativeFighterDefinition*, const NativeFighterHost*);

static float rotation_y;
static unsigned extra_updates;

static void change_motion(Fighter_GObj* object, FtMotionId state,
                          MotionFlags flags, float frame, float speed,
                          float blend, Fighter_GObj* other)
{
    Fighter* fighter = object->user_data;
    (void) flags; (void) blend; (void) other;
    fighter->motion_id = state;
    // Fighter_ChangeMotionState evaluates the requested animation frame before
    // returning. ftAnim_8006EBA4 would then advance it by another tick.
    fighter->cur_anim_frame = frame;
    fighter->frame_speed_mul = speed;
    memset(fighter->cmd_vars, 0, sizeof(fighter->cmd_vars));
}

static void start_animation(Fighter_GObj* object)
{
    Fighter* fighter = object->user_data;
    fighter->cur_anim_frame += fighter->frame_speed_mul;
    ++extra_updates;
}

static void rotate_y(Fighter* fighter, Fighter_Part part, float angle)
{
    (void) fighter; (void) part;
    rotation_y = angle;
}

static void rotate_x(Fighter* fighter, Fighter_Part part, float angle)
{ (void) fighter; (void) part; (void) angle; }

static bool frames_remaining(Fighter_GObj* object)
{ (void) object; return true; }
static void air_drift(Fighter_GObj* object) { (void) object; }
static bool ground_ledge(Fighter_GObj* object, int direction)
{ (void) object; (void) direction; return false; }
static bool cliff_catch(Fighter_GObj* object)
{ (void) object; return false; }
static void freefall(Fighter_GObj* object, int a, int b, bool c,
                    float mobility, float lag)
{ (void) object; (void) a; (void) b; (void) c; (void) mobility; (void) lag; }
static void landing(Fighter_GObj* object, bool interrupt, float lag)
{ (void) object; (void) interrupt; (void) lag; }

int main(int argc, char** argv)
{
    if (argc != 2) return 2;
    HMODULE library = LoadLibraryExA(argv[1], NULL,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!library) return 2;
    ModuleEntry entry = (ModuleEntry) GetProcAddress(
        library, "FreakMelee_GetFighterModule");
    MotionState states[12] = { 0 };
    NativeFighterDefinition base = {
        .fighter_root = "ftDataRook",
        .motion_states = states,
        .motion_state_first = 341,
        .motion_state_count = 12,
    };
    NativeFighterHost host = {
        .callback_source_alive = script_lifetime_alive,
        .change_motion_state = change_motion,
        .start_animation = start_animation,
        .set_root_rotation_y = rotate_y,
        .set_root_rotation_x = rotate_x,
        .animation_frames_remaining = frames_remaining,
        .air_drift = air_drift,
        .check_ground_and_ledge = ground_ledge,
        .check_cliff_catch = cliff_catch,
        .enter_freefall = freefall,
        .enter_landing_fall_special = landing,
    };
    const NativeFighterDefinition* definition = entry ? entry(&base, &host) : NULL;
    if (!definition) return 2;
    bool ok = true;
    for (unsigned air = 0; air < 2; ++air) {
        for (int facing = -1; facing <= 1; facing += 2) {
            Fighter fighter = { 0 };
            Fighter_GObj object = { .user_data = &fighter };
            script_lifetime_register(&object, NativeCallbackSource_Fighter);
            fighter.facing_dir = (float) facing;
            fighter.ground_or_air = air ? GA_Air : GA_Ground;
            (air ? definition->special_air_hi : definition->special_hi)(&object);
            ok &= fighter.motion_id == 350 && fighter.cur_anim_frame == 1;
            // Startup signals the travel state from its animation callback;
            // the new state's physics runs later in that same game tick.
            fighter.cur_anim_frame = 12;
            fighter.cmd_vars[1] = 1;
            definition->motion_states[9].anim_cb(&object);
            definition->motion_states[10].phys_cb(&object);
            float speed = 1.5f - 0.5f / 110.0f;
            ok &= fighter.motion_id == 351 && fighter.cur_anim_frame == 1;
            ok &= fighter.facing_dir == facing;
            ok &= fabsf(fighter.self_vel.x - facing * speed) < 0.00001f;
            ok &= fabsf(rotation_y) < 0.00001f;

            // A later deliberate reversal still interpolates through 16 frames
            // and remains relative to this particular entry's facing direction.
            fighter.cur_anim_frame = 10;
            fighter.self_vel.x = -facing * 0.5f;
            definition->motion_states[10].phys_cb(&object);
            ok &= fighter.facing_dir == -facing && fabsf(rotation_y) < 0.00001f;
            fighter.cur_anim_frame = 18;
            definition->motion_states[10].phys_cb(&object);
            ok &= fabsf(rotation_y - 3.14159f * 0.5f) < 0.00001f;
            fighter.cur_anim_frame = 26;
            definition->motion_states[10].phys_cb(&object);
            ok &= fabsf(rotation_y - 3.14159f) < 0.00001f;
        }
    }
    ok &= extra_updates == 0;
    printf("Rook up-special ground/air, left/right entry and reversal: %s\n",
           ok ? "PASS" : "FAIL");
    FreeLibrary(library);
    return ok ? 0 : 1;
}
