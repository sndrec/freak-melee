#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "script_lifetime_fixture.h"

#include "../../tools/native_fighter_script.h"
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/types.h>
#include <melee/ft/kinds/ftMario/types.h>
#include <melee/ft/kinds/ftCommon/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/controller.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

typedef const NativeFighterDefinition* (__cdecl* ModuleEntry)(
    const NativeFighterDefinition*, const NativeFighterHost*);
typedef unsigned long long (__cdecl* SourceHash)(void);

static unsigned articles;
static unsigned root_effects;
static int last_root_effect;
static unsigned reflector_creates, reflector_updates;
static bool animation_active = true;
static unsigned freefall_calls;
static float freefall_mobility, freefall_lag;
static unsigned ground_motion_calls, air_motion_calls;
static unsigned fall_calls, quick_air_decel_calls;
static float fall_gravity, fall_terminal_velocity;
static float up_entry_x, up_entry_y;
static unsigned standard_air_calls, standard_ground_calls;
static unsigned ground_box_calls, air_box_calls;
static ftCollisionBox ground_box_seen, air_box_seen;
static unsigned special_landing_calls, landing_enter_calls;
static bool landing_interrupt;
static float landing_lag;
static unsigned facing_updates, root_rotations;
static float root_rotation_y;
static unsigned root_x_rotations;
static float root_rotation_x;
static unsigned stock_entries;
static char movement_trace[16];
static unsigned movement_count;
static float ground_accel_mul, ground_accel_max;
static float air_accel_mul, air_accel_max;
static float ascend_accel, ascend_max;
static float horizontal_limit, ground_limit, fall_limit;

static void stock_entry(HSD_GObj* object)
{
    (void) object;
    ++stock_entries;
}

static void stock_article(HSD_GObj* object)
{
    ++articles;
    ((Fighter*) object->user_data)->throw_flags_b0 = 0;
}
static EF_SpawnResult spawn_effect(s32 effect, HSD_GObj* object, ...)
{
    (void) object;
    ++root_effects;
    last_root_effect = effect;
    return (EF_SpawnResult){ 0 };
}
static void pause_effects(HSD_GObj* object) { (void) object; }
static void resume_effects(HSD_GObj* object) { (void) object; }
static void create_reflector(Fighter_GObj* object, ReflectDesc* description,
                             HSD_GObjEvent on_reflect)
{
    (void) object; (void) description; (void) on_reflect;
    ++reflector_creates;
}
static void update_reflector(Fighter_GObj* object)
{ (void) object; ++reflector_updates; }

static void change_motion(Fighter_GObj* object, FtMotionId state,
                          MotionFlags flags, float start, float speed,
                          float blend, Fighter_GObj* other)
{
    (void) flags; (void) start; (void) speed; (void) blend; (void) other;
    Fighter* fighter = object->user_data;
    if (state == 348) {
        up_entry_x = fighter->self_vel.x;
        up_entry_y = fighter->self_vel.y;
    }
    fighter->motion_id = state;
}

static void start_animation(Fighter_GObj* object) { (void) object; }
static bool ground_collision(Fighter_GObj* object)
{ (void) object; return false; }
static GroundOrAir air_collision(Fighter_GObj* object)
{ (void) object; return GA_Air; }
static bool ground_box_collision(Fighter_GObj* object, ftCollisionBox* box)
{ (void) object; ground_box_seen = *box; ++ground_box_calls; return true; }
static bool air_box_collision(Fighter_GObj* object, ftCollisionBox* box)
{ (void) object; air_box_seen = *box; ++air_box_calls; return true; }
static void ground_to_air(Fighter* fighter)
{ fighter->ground_or_air = GA_Air; }
static void air_to_ground(Fighter* fighter)
{ fighter->ground_or_air = GA_Ground; }
static bool frames_remaining(Fighter_GObj* object)
{ (void) object; return animation_active; }
static void empty_action(Fighter_GObj* object) { (void) object; }
static bool air_actions(Fighter_GObj* object)
{ (void) object; return false; }
static void ground_motion(Fighter_GObj* object)
{ (void) object; ++ground_motion_calls; }
static void air_motion(Fighter_GObj* object)
{ (void) object; ++air_motion_calls; }
static void fall(Fighter* fighter, float gravity, float terminal_velocity)
{
    (void) fighter;
    ++fall_calls;
    fall_gravity = gravity;
    fall_terminal_velocity = terminal_velocity;
}
static bool quick_air_decel(Fighter* fighter)
{ (void) fighter; ++quick_air_decel_calls; return false; }
static void fall_basic(Fighter* fighter)
{ (void) fighter; movement_trace[movement_count++] = 'F'; }
static void ascend(Fighter* fighter, float accel, float max)
{
    (void) fighter;
    movement_trace[movement_count++] = 'A';
    ascend_accel = accel;
    ascend_max = max;
}
static void ground_accel(Fighter* fighter, float threshold, float mul,
                         float max)
{
    (void) fighter;
    movement_trace[movement_count++] = threshold == 0.0f ? 'G' : '?';
    ground_accel_mul = mul;
    ground_accel_max = max;
}
static void air_accel(Fighter* fighter, float threshold, float mul,
                      float max)
{
    (void) fighter;
    movement_trace[movement_count++] = threshold == 0.0f ? 'D' : '?';
    air_accel_mul = mul;
    air_accel_max = max;
}
static void ground_to_self(Fighter_GObj* object)
{ (void) object; movement_trace[movement_count++] = 'S'; }
static void clamp_self_vel_x(Fighter* fighter, float max)
{ (void) fighter; movement_trace[movement_count++] = 'H'; horizontal_limit = max; }
static void clamp_ground_vel(Fighter* fighter, float max)
{ (void) fighter; movement_trace[movement_count++] = 'C'; ground_limit = max; }
static void clamp_fall_speed(Fighter* fighter, float max)
{ (void) fighter; movement_trace[movement_count++] = 'V'; fall_limit = max; }
static void standard_air_collision(Fighter_GObj* object)
{ (void) object; ++standard_air_calls; }
static void standard_ground_collision(Fighter_GObj* object)
{ (void) object; ++standard_ground_calls; }
static bool fall_special_ledge_check(Fighter_GObj* object, int line)
{ (void) object; (void) line; return false; }
static void special_landing_collision(Fighter_GObj* object,
                                      bool (*ledge_check)(Fighter_GObj*, int),
                                      void (*land)(Fighter_GObj*))
{
    if (ledge_check == fall_special_ledge_check) ++special_landing_calls;
    land(object);
    ((Fighter*) object->user_data)->ground_or_air = GA_Ground;
}
static void landing_fall_special(Fighter_GObj* object, bool allow_interrupt,
                                 float lag)
{
    (void) object;
    ++landing_enter_calls;
    landing_interrupt = allow_interrupt;
    landing_lag = lag;
}
static void update_facing(Fighter* fighter)
{
    ++facing_updates;
    fighter->facing_dir = fighter->input.lstick[0].x >= 0 ? 1.0f : -1.0f;
}
static void set_root_rotation_y(Fighter* fighter, int part, float angle)
{
    (void) fighter;
    if (part == 0) ++root_rotations;
    root_rotation_y = angle;
}
static void set_root_rotation_x(Fighter* fighter, int part, float angle)
{
    (void) fighter;
    if (part == 0) ++root_x_rotations;
    root_rotation_x = angle;
}
static void enter_freefall(Fighter_GObj* object, int first, int second,
                           bool allow_interrupt, float mobility, float lag)
{
    (void) object;
    if (first == 0 && second == 1 && !allow_interrupt) ++freefall_calls;
    freefall_mobility = mobility;
    freefall_lag = lag;
}

static bool invalid_script_rejected(const char* script, const NativeFighterScriptConstruction* construction)
{
    FILE* output = tmpfile();
    if (output == NULL) return false;
    NativeFighterScriptError error = { 0 };
    bool rejected = !nativeFighterScript_GenerateC(script, strlen(script),
                                                  "multi_probe", NULL, NULL, construction, output,
                                                  &error) &&
                    error.line != 0;
    if (!rejected) fprintf(stderr, "Expected rejection: %s\n", script);
    fclose(output);
    return rejected;
}

static bool removal_notification_accepted(const char* event, const NativeFighterScriptConstruction* construction)
{
    char script[256];
    snprintf(script, sizeof(script),
        "article It_Kind_Fox_Blaster states 1:\n"
        "    on %s:\n            item.remove()\n"
        "    state 0 anim 0:\n", event);
    FILE* output = tmpfile();
    if (output == NULL) return false;
    NativeFighterScriptError error = { 0 };
    bool accepted = nativeFighterScript_GenerateC(script, strlen(script),
        "multi_probe", NULL, NULL, construction, output, &error);
    if (!accepted) fprintf(stderr, "Removal notification rejected at %u: %s\n",
                            error.line, error.message);
    fclose(output);
    return accepted;
}

int main(int argc, char** argv)
{
    if (argc != 3) return 2;
    FILE* source = fopen(argv[2], "rb");
    char bytes[65536];
    size_t size = source != NULL ? fread(bytes, 1, sizeof(bytes), source) : 0;
    if (source != NULL) fclose(source);
    if (size == 0 || size == sizeof(bytes)) return 1;

    HMODULE library = LoadLibraryExA(argv[1], NULL,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (library == NULL) return 1;
    ModuleEntry entry = (ModuleEntry) GetProcAddress(
        library, "FreakMelee_GetFighterModule");
    SourceHash source_hash = (SourceHash) GetProcAddress(
        library, "FreakMelee_GetScriptSourceHash");
    MotionState states[8] = { 0 };
    NativeFighterDefinition base = {
        .motion_states = states,
        .motion_state_count = 8,
        .motion_state_first = 343,
        .state_bytes = 3,
        .special_ground_state = {
            [NativeFighterSpecial_Neutral] = 343,
            [NativeFighterSpecial_Side] = 345,
            [NativeFighterSpecial_Up] = 347,
            [NativeFighterSpecial_Down] = 349,
        },
        .special_air_state = {
            [NativeFighterSpecial_Neutral] = 344,
            [NativeFighterSpecial_Side] = 346,
            [NativeFighterSpecial_Up] = 348,
            [NativeFighterSpecial_Down] = 350,
        },
        .neutral_article = stock_article,
        .special_n = stock_entry,
        .special_air_n = stock_entry,
        .special_s = stock_entry,
        .special_air_s = stock_entry,
        .special_hi = stock_entry,
        .special_air_hi = stock_entry,
        .special_lw = stock_entry,
        .special_air_lw = stock_entry,
    };
    NativeFighterHost host = {
        .callback_source_alive = script_lifetime_alive,
        .change_motion_state = change_motion,
        .start_animation = start_animation,
        .ground_collision = ground_collision,
        .air_collision = air_collision,
        .ground_box_collision = ground_box_collision,
        .air_box_collision = air_box_collision,
        .ground_to_air = ground_to_air,
        .air_to_ground = air_to_ground,
        .animation_frames_remaining = frames_remaining,
        .finish_ground_action = empty_action,
        .enter_fall = empty_action,
        .enter_freefall = enter_freefall,
        .ground_actions = empty_action,
        .air_actions = air_actions,
        .ground_friction = empty_action,
        .air_drift = empty_action,
        .ground_motion = ground_motion,
        .air_motion = air_motion,
        .fall = fall,
        .quick_air_decel = quick_air_decel,
        .fall_basic = fall_basic,
        .ascend = ascend,
        .ground_accel = ground_accel,
        .air_accel = air_accel,
        .ground_to_self_movement = ground_to_self,
        .clamp_self_vel_x = clamp_self_vel_x,
        .clamp_ground_vel = clamp_ground_vel,
        .clamp_fall_speed = clamp_fall_speed,
        .air_standard_collision = standard_air_collision,
        .ground_standard_collision = standard_ground_collision,
        .air_special_landing_collision = special_landing_collision,
        .fall_special_ledge_check = fall_special_ledge_check,
        .enter_landing_fall_special = landing_fall_special,
        .update_facing = update_facing,
        .set_root_rotation_y = set_root_rotation_y,
        .set_root_rotation_x = set_root_rotation_x,
        .angle_atan2 = atan2,
        .spawn_effect = spawn_effect,
        .pause_effects = pause_effects,
        .resume_effects = resume_effects,
        .create_reflector = create_reflector,
        .update_reflector = update_reflector,
    };
    bool ok = entry != NULL && source_hash != NULL &&
              source_hash() == nativeFighterScript_SourceHash(bytes, size);
    for (unsigned i = 0; i < 2 && ok; ++i) {
        NativeFighterDefinition missing = base;
        if (i == 0) missing.neutral_article = NULL;
        if (i == 1)
            missing.special_ground_state[NativeFighterSpecial_Down] = 0;
        ok = entry(&missing, &host) == NULL;
    }
    if (ok) {
        NativeFighterDefinition missing_state = base;
        missing_state.special_air_state[NativeFighterSpecial_Side] = 0;
        ok = entry(&missing_state, &host) == NULL;
        missing_state = base;
        missing_state.special_air_state[NativeFighterSpecial_Up] = 0;
        ok = ok && entry(&missing_state, &host) == NULL;
        NativeFighterHost missing_freefall = host;
        missing_freefall.enter_freefall = NULL;
        ok = ok && entry(&base, &missing_freefall) == NULL;
        NativeFighterHost missing_fall = host;
        missing_fall.fall = NULL;
        ok = ok && entry(&base, &missing_fall) == NULL;
        NativeFighterHost missing_landing = host;
        missing_landing.enter_landing_fall_special = NULL;
        ok = ok && entry(&base, &missing_landing) == NULL;
        NativeFighterHost missing_ground_box = host;
        missing_ground_box.ground_box_collision = NULL;
        ok = ok && entry(&base, &missing_ground_box) == NULL;
        NativeFighterHost missing_air_box = host;
        missing_air_box.air_box_collision = NULL;
        ok = ok && entry(&base, &missing_air_box) == NULL;
        NativeFighterHost missing_rotation = host;
        missing_rotation.set_root_rotation_y = NULL;
        ok = ok && entry(&base, &missing_rotation) == NULL;
        NativeFighterHost missing_root_x = host;
        missing_root_x.set_root_rotation_x = NULL;
        ok = ok && entry(&base, &missing_root_x) == NULL;
        NativeFighterHost missing_atan2 = host;
        missing_atan2.angle_atan2 = NULL;
        ok = ok && entry(&base, &missing_atan2) == NULL;
        NativeFighterHost missing_effect = host;
        missing_effect.spawn_effect = NULL;
        ok = ok && entry(&base, &missing_effect) == NULL;
        NativeFighterHost missing_pause = host;
        missing_pause.pause_effects = NULL;
        ok = ok && entry(&base, &missing_pause) == NULL;
        NativeFighterHost missing_resume = host;
        missing_resume.resume_effects = NULL;
        ok = ok && entry(&base, &missing_resume) == NULL;
        NativeFighterHost missing_reflector = host;
        missing_reflector.create_reflector = NULL;
        ok = ok && entry(&base, &missing_reflector) == NULL;
        NativeFighterHost missing_reflector_update = host;
        missing_reflector_update.update_reflector = NULL;
        ok = ok && entry(&base, &missing_reflector_update) == NULL;
        NativeFighterHost missing_ascend = host;
        missing_ascend.ascend = NULL;
        ok = ok && entry(&base, &missing_ascend) == NULL;
        NativeFighterDefinition full_state = base;
        full_state.state_bytes = 249;
        ok = ok && entry(&full_state, &host) == NULL;
    }
    const NativeFighterDefinition* module = ok ? entry(&base, &host) : NULL;
    ok = module != NULL && module->code_key != NULL &&
         strcmp(module->code_key, "multi_probe") == 0 &&
         module->state_bytes == 13 &&
         module->on_landing_action != NULL && module->on_death != NULL &&
         module->special_hi != stock_entry &&
         module->special_air_hi != stock_entry &&
          module->special_air_lw != stock_entry &&
         module->special_n != stock_entry &&
         module->special_s != stock_entry &&
         module->motion_states != states &&
         module->motion_states[2].anim_cb != NULL &&
         module->motion_states[2].coll_cb != NULL &&
         module->motion_states[3].coll_cb != NULL;
    ftMario_DatAttrs attributes = { 0 };
    attributes.specialhi.vel_x = 0.625f;
    attributes.specialhi.grav = 0.12f;
    attributes.specialhi.vel_mul = 0.75f;
    attributes.specialhi.momentum_stick_range = 0.5f;
    attributes.specialhi.reverse_stick_range = 0.3f;
    attributes.specialhi.angle_diff = 30.0f;
    attributes.specialhi.freefall_mobility = 0.8f;
    attributes.specialhi.landing_lag = 22.0f;
    attributes.speciallw.momentum_x = 1.5f;
    attributes.speciallw.momentum_x_mul = 0.6f;
    attributes.speciallw.air_momentum_x = 1.2f;
    attributes.speciallw.air_momentum_x_mul = 0.4f;
    attributes.speciallw.tap_y_vel_max = 2.2f;
    attributes.speciallw.tap_grav = 0.3f;
    Fighter fighter = {
        .facing_dir = 1,
        .ground_or_air = GA_Ground,
        .dat_attrs = &attributes,
    };
    fighter.native_module_state[0] = 0xA5;
    fighter.native_module_state[1] = 0x5A;
    fighter.native_module_state[2] = 0xC3;
    HSD_GObj object = { .user_data = &fighter };
    script_lifetime_register(&object, NativeCallbackSource_Fighter);
    if (ok) {
        module->special_n(&object);
        ok = fighter.motion_id == 343 && fighter.accessory4_cb != NULL;
        if (ok) {
            fighter.throw_flags_b0 = 1;
            fighter.accessory4_cb(&object);
            fighter.throw_flags_b0 = 1;
            fighter.accessory4_cb(&object);
            ok = articles == 1 && fighter.native_module_state[4] == 1;
        }
        module->special_s(&object);
        ok = ok && fighter.motion_id == 345 &&
             fabsf(fighter.gr_vel - 0.5f) < 0.00001f;
        fighter.cur_anim_frame = 7.0f;
        module->motion_states[2].coll_cb(&object);
        ok = ok && fighter.motion_id == 346 &&
             fighter.ground_or_air == GA_Air;
        module->special_hi(&object);
        ok = ok && fighter.motion_id == 347;
        fighter.input.lstick[0].x = 1.0f;
        fighter.throw_flags_b3 = 1;
        fighter.facing_dir = -1.0f;
        module->motion_states[4].input_cb(&object);
        ok = ok && fighter.throw_flags_b3 == 0 &&
             fabsf(fighter.lstick_angle + MTXDegToRad(30.0f)) < 0.00001f &&
             fighter.facing_dir == 1.0f && facing_updates == 1 &&
             root_rotations == 1 &&
             fabsf(root_rotation_y - (float) M_PI_2) < 0.00001f &&
             fighter.native_module_state[5] == 1;
        float saved_angle;
        memcpy(&saved_angle, fighter.native_module_state + 8,
               sizeof(saved_angle));
        ok = ok && saved_angle == fighter.lstick_angle;
        fighter.cmd_vars[0] = 1;
        fighter.lstick_angle = 0.0f;
        fighter.input.lstick[0].x = -1.0f;
        module->motion_states[5].input_cb(&object);
        ok = ok && fighter.lstick_angle == 0.0f &&
             facing_updates == 1 && root_rotations == 1 &&
             fighter.native_module_state[5] == 0;
        memcpy(&saved_angle, fighter.native_module_state + 8,
               sizeof(saved_angle));
        ok = ok && saved_angle != 0.0f;
        fighter.ground_or_air = GA_Ground;
        module->motion_states[4].phys_cb(&object);
        fighter.ground_or_air = GA_Air;
        module->motion_states[4].phys_cb(&object);
        ok = ok && ground_motion_calls == 1 && air_motion_calls == 1;
        fighter.cmd_vars[0] = 0;
        fighter.cmd_vars[2] = 0;
        fighter.self_vel.y = -1.0f;
        module->motion_states[4].coll_cb(&object);
        fighter.cmd_vars[0] = 1;
        fighter.self_vel.y = 1.0f;
        module->motion_states[5].coll_cb(&object);
        fighter.self_vel.y = -1.0f;
        module->motion_states[5].coll_cb(&object);
        fighter.ground_or_air = GA_Ground;
        module->motion_states[4].coll_cb(&object);
        ok = ok && standard_air_calls == 2 &&
             special_landing_calls == 1 && landing_enter_calls == 1 &&
             !landing_interrupt && landing_lag == 22.0f &&
             standard_ground_calls == 1;
        fighter.cmd_vars[2] = 1;
        module->motion_states[4].coll_cb(&object);
        module->motion_states[5].coll_cb(&object);
        ok = ok && ground_box_calls == 1 && air_box_calls == 1 &&
             ground_box_seen.top == 12.0f &&
             ground_box_seen.left.x == -6.0f &&
             ground_box_seen.right.y == 6.0f &&
             air_box_seen.bottom == 0.0f &&
             fighter.motion_id == 347 && fighter.ground_or_air == GA_Ground;
        fighter.cmd_vars[2] = 0;
        fighter.cmd_vars[3] = 1;
        fighter.coll_data.floor.normal.x = 1.0f;
        fighter.coll_data.floor.normal.y = 1.0f;
        fighter.facing_dir = -1.0f;
        module->motion_states[4].coll_cb(&object);
        ok = ok && root_x_rotations == 1 &&
             fabsf(root_rotation_x + (float) M_PI_4) < 0.00001f;
        fighter.cmd_vars[3] = 0;
        fighter.cmd_vars[1] = 7;
        module->special_lw(&object);
        ok = ok && stock_entries == 0 && fighter.motion_id == 350 &&
             fighter.ground_or_air == GA_Ground &&
             fighter.cmd_vars[1] == 0 && fighter.cmd_vars[2] == 1 &&
             fighter.self_vel.y == -2.2f &&
             root_effects == 1 && last_root_effect == 0x47c &&
             fighter.x2219_b0 == 1 &&
             fighter.pre_hitlag_cb == pause_effects &&
             fighter.post_hitlag_cb == resume_effects &&
             fighter.take_dmg_cb != NULL && fighter.death2_cb != NULL;
        unsigned rotations_before_damage = root_x_rotations;
        if (fighter.take_dmg_cb != NULL) fighter.take_dmg_cb(&object);
        ok = ok && root_x_rotations == rotations_before_damage + 1 &&
             root_rotation_x == 0.0f;
        fighter.cmd_vars[1] = 1;
        animation_active = false;
        module->motion_states[7].anim_cb(&object);
        animation_active = true;
        ok = ok && fighter.take_dmg_cb == NULL &&
             fighter.death2_cb == NULL &&
             fighter.u.mr.x2234_tornadoCharge == 0;
        fighter.cmd_vars[1] = 1;
        module->motion_states[6].phys_cb(&object);
        fighter.cmd_vars[1] = 0;
        fighter.reflecting = true;
        module->motion_states[6].phys_cb(&object);
        ok = ok && reflector_creates == 1 && reflector_updates == 2 &&
             !fighter.reflecting;
        module->special_air_lw(&object);
        ok = ok && fighter.motion_id == 350 && fighter.self_vel.y == 0.0f;
        module->on_landing_action(&object);
        // A legacy union value must not affect the script-owned charge.
        fighter.u.mr.x2234_tornadoCharge = 1;
        module->special_air_lw(&object);
        ok = ok && fighter.motion_id == 350 && fighter.self_vel.y == -2.2f;
        fighter.cmd_vars[1] = 1;
        module->motion_states[7].anim_cb(&object);
        module->on_death(&object);
        module->special_air_lw(&object);
        ok = ok && fighter.self_vel.y == -2.2f;
        fighter.facing_dir = -1;
        fighter.self_vel.x = 0.75f;
        fighter.self_vel.y = 3.0f;
        module->special_air_s(&object);
        ok = ok && stock_entries == 0 &&
             fighter.motion_id == 346 &&
             fabsf(fighter.self_vel.x - 0.25f) < 0.00001f &&
             fabsf(fighter.self_vel.y - 1.5f) < 0.00001f;
        fighter.self_vel.x = 4.0f;
        fighter.self_vel.y = 3.0f;
        module->special_air_hi(&object);
        ok = ok && stock_entries == 0 &&
             fighter.motion_id == 348 && fighter.self_vel.y == 0.0f &&
             fabsf(fighter.self_vel.x - 2.5f) < 0.00001f &&
             fabsf(up_entry_x - 2.5f) < 0.00001f && up_entry_y == 0.0f;
        fighter.co_attrs.terminal_velocity = 2.9f;
        fighter.cmd_vars[0] = 0;
        module->motion_states[5].phys_cb(&object);
        memcpy(&saved_angle, fighter.native_module_state + 8,
               sizeof(saved_angle));
        ok = ok && fall_calls == 1 && quick_air_decel_calls == 1 &&
             air_motion_calls == 1 &&
             saved_angle > 0.0f &&
             fabsf(fall_gravity - 0.12f) < 0.00001f &&
             fabsf(fall_terminal_velocity - 2.9f) < 0.00001f;
        fighter.cmd_vars[0] = 1;
        fighter.self_vel.x = 4.0f;
        fighter.self_vel.y = 2.0f;
        fighter.self_vel.z = -1.0f;
        module->motion_states[5].phys_cb(&object);
        ok = ok && air_motion_calls == 2 && fall_calls == 1 &&
             quick_air_decel_calls == 1 &&
             fabsf(fighter.self_vel.x - 3.0f) < 0.00001f &&
             fabsf(fighter.self_vel.y - 1.5f) < 0.00001f &&
             fabsf(fighter.self_vel.z + 0.75f) < 0.00001f;
        fighter.ground_or_air = GA_Ground;
        fighter.cmd_vars[3] = 1;
        module->motion_states[4].phys_cb(&object);
        fighter.ground_or_air = GA_Air;
        fighter.cmd_vars[3] = 1;
        module->motion_states[5].phys_cb(&object);
        ok = ok && movement_count == 8 &&
             memcmp(movement_trace, "GSCAFDHV", 8) == 0 &&
             fighter.cmd_vars[3] == 0 &&
             ground_accel_mul == 0.6f && ground_accel_max == 1.5f &&
             ground_limit == 1.5f &&
             ascend_accel == 2.2f && ascend_max == 0.3f &&
             air_accel_mul == 0.4f && air_accel_max == 1.2f &&
             horizontal_limit == 1.2f && fall_limit == 0.3f;
        fighter.cmd_vars[2] = 0;
        module->motion_states[4].anim_cb(&object);
        ok = ok && freefall_calls == 0 &&
             fighter.native_module_state[5] == 0;
        fighter.cmd_vars[2] = 1;
        module->motion_states[4].anim_cb(&object);
        ok = ok && freefall_calls == 0 &&
             fighter.native_module_state[5] == 1;
        fighter.cmd_vars[2] = 0;
        module->motion_states[4].anim_cb(&object);
        ok = ok && freefall_calls == 0 &&
             fighter.native_module_state[5] == 0;
        fighter.cmd_vars[1] = 1;
        fighter.input.pressed_buttons = HSD_PAD_B;
        animation_active = false;
        module->motion_states[4].anim_cb(&object);
        memcpy(&saved_angle, fighter.native_module_state + 8,
               sizeof(saved_angle));
        ok = ok && freefall_calls == 1 &&
             fighter.native_module_state[5] == 1 &&
             fighter.cmd_vars[1] == 0 && saved_angle >= 0.0f;
        module->motion_states[5].anim_cb(&object);
        ok = ok && freefall_calls == 2 &&
             fighter.native_module_state[5] == 0 &&
             freefall_mobility == 0.8f && freefall_lag == 22.0f;
    }
    ok = ok && fighter.native_module_state[0] == 0xA5 &&
         fighter.native_module_state[1] == 0x5A &&
         fighter.native_module_state[2] == 0xC3;
    ok = ok && invalid_script_rejected(
        "action MissingDamage:\n    on death:\n        callbacks.track_secondary_damage()\n", &(const NativeFighterScriptConstruction){"ftDataTest",341,1}) &&
        invalid_script_rejected(
        "func missing_damage():\n    callbacks.track_secondary_damage()\n"
        "move Probe:\n    on death:\n        missing_damage()\n", &(const NativeFighterScriptConstruction){"ftDataTest",341,1}) &&
        invalid_script_rejected(
        "action BadCue:\n    on death:\n        if cue.consume(8): pass\n", &(const NativeFighterScriptConstruction){"ftDataTest",341,1}) &&
        invalid_script_rejected(
        "action BadFloor:\n    on death:\n        let x = 0\n"
        "        stage.follow_floor(0, 0, 0, 0, 1, 3, x, x, x, x)\n", &(const NativeFighterScriptConstruction){"ftDataTest",341,1}) &&
        invalid_script_rejected(
        "action BadFloor:\n    on death:\n"
        "        stage.follow_floor(0, 0, 0, 0, 1, 3, found, x, y, z)\n", &(const NativeFighterScriptConstruction){"ftDataTest",341,1}) &&
        invalid_script_rejected(
        "move One:\n    on special_s:\n        stock.enter()\n"
        "move Two:\n    on special_air_s:\n        stock.enter()\n", &(const NativeFighterScriptConstruction){"ftDataTest",341,1}) &&
        invalid_script_rejected(
        "move One:\n    on special_n:\n        stock.enter()\n"
        "move Two:\n    on special_s:\n        stock.enter()\n"
        "move Three:\n    on special_hi:\n        stock.enter()\n"
        "move Four:\n    on special_lw:\n        stock.enter()\n"
        "move Five:\n    on special_n:\n        stock.enter()\n", &(const NativeFighterScriptConstruction){"ftDataTest",341,1}) &&
        invalid_script_rejected(
        "move One:\n    on special_s:\n        stock.enter()\n"
        "move Two:\n    on special_hi:\n        stock.enter()\n"
        "    on special_lw:\n        stock.enter()\n", &(const NativeFighterScriptConstruction){"ftDataTest",341,1}) &&
        invalid_script_rejected(
        "move Incomplete:\n"
        "    on special_n:\n        state.enter(NeutralSpecial)\n"
        "    on special_air_n:\n        state.enter(NeutralSpecial)\n"
        "    on ground_animation:\n"
        "        if animation.finished: state.wait()\n", &(const NativeFighterScriptConstruction){"ftDataTest",341,1}) &&
        invalid_script_rejected(
        "move WrongState:\n"
        "    on special_s:\n        state.enter(NeutralSpecial)\n"
        "    on special_air_s:\n        state.enter(SideSpecial)\n", &(const NativeFighterScriptConstruction){"ftDataTest",341,1}) &&
        invalid_script_rejected(
        "move IncompleteSide:\n"
        "    on special_s:\n        state.enter(SideSpecial)\n"
        "    on special_air_s:\n        state.enter(SideSpecial)\n"
        "    on ground_animation:\n"
        "        if animation.finished: state.wait()\n", &(const NativeFighterScriptConstruction){"ftDataTest",341,1}) &&
        invalid_script_rejected(
        "move BadAttribute:\n"
        "    on special_air_hi:\n        stock.enter()\n"
        "        motion.scale_horizontal_speed(attrs.specialhi.unknown)\n", &(const NativeFighterScriptConstruction){"ftDataTest",341,1}) &&
        invalid_script_rejected(
        "move BadFreefall:\n"
        "    on special_hi:\n        state.enter(UpSpecial)\n"
        "    on special_air_hi:\n        state.enter(UpSpecial)\n"
        "    on ground_animation:\n"
        "        if animation.finished: state.freefall(0, 1, false, attrs.specialhi.unknown, 22)\n", &(const NativeFighterScriptConstruction){"ftDataTest",341,1}) &&
        invalid_script_rejected(
        "move BadPhysics:\n"
        "    on special_hi:\n        state.enter(UpSpecial)\n"
        "    on special_air_hi:\n        state.enter(UpSpecial)\n"
        "    on ground_physics:\n"
        "        if airborne: physics.fall(attrs.specialhi.not_a_field)\n", &(const NativeFighterScriptConstruction){"ftDataTest",341,1}) &&
        invalid_script_rejected(
        "move EscapedLocal:\n"
        "    on special_hi:\n        state.enter(UpSpecial)\n"
        "    on ground_interrupt:\n"
        "        if command(0):\n"
        "            let angle = 1\n"
        "        motion.launch_angle = angle\n", &(const NativeFighterScriptConstruction){"ftDataTest",341,1}) &&
        invalid_script_rejected(
        "move InlineLocal:\n"
        "    on special_hi:\n        state.enter(UpSpecial)\n"
        "    on ground_interrupt:\n"
        "        if command(0): let angle = 1\n", &(const NativeFighterScriptConstruction){"ftDataTest",341,1}) &&
        invalid_script_rejected(
        "state float meter\n"
        "state bool meter\n"
        "move DuplicateState:\n"
        "    on special_hi:\n        state.enter(UpSpecial)\n", &(const NativeFighterScriptConstruction){"ftDataTest",341,1}) &&
        invalid_script_rejected(
        "state bool known\n"
        "move UnknownState:\n"
        "    on special_hi:\n        state.enter(UpSpecial)\n"
        "    on ground_interrupt:\n"
        "        state.missing = true\n", &(const NativeFighterScriptConstruction){"ftDataTest",341,1}) &&
        invalid_script_rejected(
        "move InvalidCommand:\n"
        "    on special_hi:\n        state.enter(UpSpecial)\n"
        "    on ground_interrupt:\n"
        "        command(4) = 1\n", &(const NativeFighterScriptConstruction){"ftDataTest",341,1});
    ok = ok && removal_notification_accepted("spawned", &(const NativeFighterScriptConstruction){"ftDataTest",341,1}) &&
        removal_notification_accepted("picked_up", &(const NativeFighterScriptConstruction){"ftDataTest",341,1}) &&
        removal_notification_accepted("destroyed", &(const NativeFighterScriptConstruction){"ftDataTest",341,1}) && invalid_script_rejected(
        "article It_Kind_Fox_Blaster states 1:\n"
        "    attachment origin owner plane maybe initial_collision off held off\n"
        "    state 0 anim 0:\n", &(const NativeFighterScriptConstruction){"ftDataTest",341,1}) && invalid_script_rejected(
        "article It_Kind_Fox_Blaster states 1:\n"
        "    attachment origin owner plane on initial_collision off held off\n"
        "    attachment origin bone plane off initial_collision on held on\n"
        "    state 0 anim 0:\n", &(const NativeFighterScriptConstruction){"ftDataTest",341,1}) && invalid_script_rejected(
        "article It_Kind_Fox_Blaster states 1:\n"
        "    on signal fire:\n            item.track_accessory()\n"
        "    state 0 anim 0:\n", &(const NativeFighterScriptConstruction){"ftDataTest",341,1}) && invalid_script_rejected(
        "article It_Kind_Fox_Blaster states 1:\n"
        "    on spawned:\n            item.change_state(1)\n"
        "    state 0 anim 0:\n", &(const NativeFighterScriptConstruction){"ftDataTest",341,1}) && invalid_script_rejected(
        "article It_Kind_Fox_Blaster states 1:\n"
        "    on spawned:\n            item.change_state(-1)\n"
        "    state 0 anim 0:\n", &(const NativeFighterScriptConstruction){"ftDataTest",341,1}) && invalid_script_rejected(
        "article It_Kind_Fox_Blaster states 1:\n"
        "    on accessory:\n            item.spawn_directional_effect(1166, UnknownBone, 0, 0, 1, 0)\n"
        "    state 0 anim 0:\n", &(const NativeFighterScriptConstruction){"ftDataTest",341,1});
    FreeLibrary(library);
    if (!ok) fputs("Multi-special module composition failed.\n", stderr);
    return ok ? 0 : 1;
}
