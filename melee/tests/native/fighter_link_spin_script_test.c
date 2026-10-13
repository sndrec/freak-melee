#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <melee/ft/native_fighter_module.h>
#include <melee/ft/kinds/ftLink/ftlinkspecialhi.h>
#include <melee/ft/kinds/ftLink/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

_Static_assert(offsetof(ftLk_DatAttrs, x30) == 48,
    "Spin landing lag must use the owned attribute at offset 48");

typedef const NativeFighterDefinition* (__cdecl* ModuleEntry)(
    const NativeFighterDefinition*, const NativeFighterHost*);
typedef int (__cdecl* ParameterEntry)(const uint32_t*, unsigned, uint64_t);

typedef struct Trace {
    unsigned state_changes, animation_starts, effects, friction, falls, drifts;
    unsigned ground_checks, ledge_checks, cliff_checks, freefalls, landings;
    unsigned forced_air, waits;
    int state, flags, effect, first_joint, second_joint, ledge_direction;
    float frame, rate, gravity, terminal, threshold, drift, speed, lag;
    int freefall_a, freefall_b;
    bool freefall_c;
} Trace;
static Trace* trace;
static bool remaining, ground_contact, ledge_contact, cliff_contact;
static FighterBone parts[80];

void Fighter_ChangeMotionState(Fighter_GObj* gobj, FtMotionId state,
    MotionFlags flags, float frame, float rate, float blend, Fighter_GObj* other)
{
    (void) blend; (void) other;
    Fighter* fp = gobj->user_data;
    ++trace->state_changes;
    trace->state = state; trace->flags = flags;
    trace->frame = frame; trace->rate = rate;
    fp->motion_id = state; fp->cur_anim_frame = frame;
}
void ftAnim_8006EBA4(Fighter_GObj* gobj)
{ (void) gobj; ++trace->animation_starts; }
bool ftAnim_IsFramesRemaining(Fighter_GObj* gobj)
{ (void) gobj; return remaining; }
void ft_8008A2BC(Fighter_GObj* gobj)
{ (void) gobj; ++trace->waits; }
void ft_80084F3C(Fighter_GObj* gobj)
{ (void) gobj; ++trace->friction; }
void ftCommon_Fall(Fighter* fp, float gravity, float terminal)
{
    ++trace->falls; trace->gravity = gravity; trace->terminal = terminal;
    fp->self_vel.y -= gravity;
    if (fp->self_vel.y < -terminal) fp->self_vel.y = -terminal;
}
void ftCommon_CalcSelfAccel_DriftSimple(Fighter* fp, float threshold,
    float drift, float speed)
{
    (void) fp; ++trace->drifts;
    trace->threshold = threshold; trace->drift = drift; trace->speed = speed;
}
void ftCommon_8007D60C(Fighter* fp)
{
    ++trace->forced_air;
    fp->ground_or_air = GA_Air; fp->gr_vel = 0;
    fp->x74_self_accel.y = 0; fp->x1968_jumpsUsed = fp->co_attrs.max_jumps;
    fp->ecb_lock = 5; fp->coll_data.x130_flags |= CollData_X130_Locked;
}
bool ft_80082708(Fighter_GObj* gobj)
{ (void) gobj; ++trace->ground_checks; return ground_contact; }
bool ft_CheckGroundAndLedge(Fighter_GObj* gobj, int direction)
{
    (void) gobj; ++trace->ledge_checks; trace->ledge_direction = direction;
    return ledge_contact;
}
bool ftCliffCommon_80081298(Fighter_GObj* gobj)
{ (void) gobj; ++trace->cliff_checks; return cliff_contact; }
void ftCo_LandingFallSpecial_Enter(Fighter_GObj* gobj, bool flag, float lag)
{ (void) gobj; (void) flag; ++trace->landings; trace->lag = lag; }
void ftCo_80096900(Fighter_GObj* gobj, int a, int b, bool c,
    float drift, float lag)
{
    (void) gobj; ++trace->freefalls;
    trace->freefall_a = a; trace->freefall_b = b; trace->freefall_c = c;
    trace->drift = drift; trace->lag = lag;
}
Fighter_Part ftParts_GetBoneIndex(Fighter* fp, Fighter_Part part)
{ (void) fp; return part == FtPart_TransN ? 3 : part; }
void efLib_PauseAll(Fighter_GObj* gobj) { (void) gobj; }
void efLib_ResumeAll(Fighter_GObj* gobj) { (void) gobj; }
EF_SpawnResult efSync_Spawn(s32 effect, HSD_GObj* gobj, ...)
{
    (void) gobj;
    va_list args; va_start(args, gobj);
    HSD_JObj* first = va_arg(args, HSD_JObj*);
    HSD_JObj* second = va_arg(args, HSD_JObj*);
    va_end(args);
    ++trace->effects; trace->effect = effect;
    trace->first_joint = trace->second_joint = -1;
    for (unsigned i = 0; i < 80; ++i) {
        if (parts[i].joint == first) trace->first_joint = i;
        if (parts[i].joint == second) trace->second_joint = i;
    }
    return (EF_SpawnResult){ 0 };
}
static void effect_between(Fighter_GObj* gobj, s32 effect, s32 first, s32 second)
{ efSync_Spawn(effect, gobj, parts[first].joint, parts[second].joint); }
static void untouched(Fighter_GObj* gobj) { (void) gobj; }

/* Bomb is exercised in fighter_link_bomb_script_test. Its required primitives
 * are present here solely so this test can load the complete shared template;
 * any accidental Bomb invocation fails immediately. */
static void unused_object(HSD_GObj* object) { (void)object;abort(); }
static bool unused_predicate(HSD_GObj* object) { (void)object;abort(); }
static bool unused_model(HSD_JObj* joint) { (void)joint;abort(); }
static void unused_model_advance(HSD_JObj* joint) { (void)joint;abort(); }
static void unused_fighter(Fighter* fighter) { (void)fighter;abort(); }
static GroundOrAir unused_air(HSD_GObj* object) { (void)object;abort(); }
static unsigned runtime_bones(Fighter* fighter) { (void)fighter;return 80; }
static ItemKind unused_kind(Item_GObj* object) { (void)object;abort(); }
static void unused_motion(HSD_GObj* object,FtMotionId id) { (void)object;(void)id;abort(); }
static void unused_visibility(HSD_GObj* object,int group,int value) { (void)object;(void)group;(void)value;abort(); }
static void unused_position(HSD_JObj* joint,Vec3* offset,Vec3* position) { (void)joint;(void)offset;(void)position;abort(); }
static int32_t unused_create(HSD_GObj* owner,ItemKind kind,Vec3* previous,unsigned bone,float facing)
{ (void)owner;(void)kind;(void)previous;(void)bone;(void)facing;abort(); }
static bool unused_owned(HSD_GObj* owner,s32 id) { (void)owner;(void)id;abort(); }
static void unused_notify(HSD_GObj* owner,bool held) { (void)owner;(void)held;abort(); }
static void unused_fall(Item_GObj* object,float gravity,float terminal) { (void)object;(void)gravity;(void)terminal;abort(); }
static void unused_float(Item_GObj* object,float value) { (void)object;(void)value;abort(); }
static void unused_sound(Item_GObj* object,s32 sound,u8 pan,u8 volume) { (void)object;(void)sound;(void)pan;(void)volume;abort(); }
static bool unused_interaction(Item_GObj* object,HSD_GObj* other) { (void)object;(void)other;abort(); }
static float unused_joint(Item_GObj* object,unsigned index,unsigned channel) { (void)object;(void)index;(void)channel;abort(); }
static void unused_joint_set(Item_GObj* object,unsigned index,unsigned channel,float value) { (void)object;(void)index;(void)channel;(void)value;abort(); }
static void unused_index(Item_GObj* object,unsigned index) { (void)object;(void)index;abort(); }
static s32 unused_flags(Item_GObj* object) { (void)object;abort(); }
static void unused_ecb(Item_GObj* object,itECB* ecb) { (void)object;(void)ecb;abort(); }
static Fighter_GObj* unused_creator(Item_GObj* item) { (void)item;abort(); }
static void unused_item(Item* item) { (void)item;abort(); }
static void unused_callback(Item_GObj* object,HSD_GObjEvent callback) { (void)object;(void)callback;abort(); }
static void unused_callbacks(Item_GObj* object,HSD_GObjEvent landed,HSD_GObjEvent left) { (void)object;(void)landed;(void)left;abort(); }
static void unused_facing(CollData* coll,int direction) { (void)coll;(void)direction;abort(); }
static void unused_wind(Vec3* position,int kind,float strength,float radius,float angle) { (void)position;(void)kind;(void)strength;(void)radius;(void)angle;abort(); }
static float unused_random(void) { abort(); }
static void unused_state(Item_GObj* object,enum_t state,Item_StateChangeFlags flags) { (void)object;(void)state;(void)flags;abort(); }
static void unused_release(Item_GObj* object,Vec3* velocity) { (void)object;(void)velocity;abort(); }

static HSD_GObj* fixture_source;
static bool callback_source_alive(HSD_GObj* object, uint32_t generation, uint32_t source_class)
{
    return source_class == NativeCallbackSource_Fighter && object != NULL &&
        object == fixture_source && object->user_data != NULL &&
        (uint32_t)((Fighter*)object->user_data)->x8_spawnNum == generation;
}

static bool compare(Fighter* stock, Fighter* script,
    HSD_GObjEvent native, HSD_GObjEvent authored, const char* phase)
{
    if (native == NULL || authored == NULL) {
        fprintf(stderr, "Missing Spin callback: %s\n", phase);
        return false;
    }
    Trace expected = { 0 }, actual = { 0 };
    HSD_GObj objects[2] = { { .user_data = stock }, { .user_data = script } };
    trace = &expected; native(&objects[0]);
    fixture_source = &objects[1];
    trace = &actual; authored(&objects[1]);
    fixture_source = NULL;
    bool ok = memcmp(&expected, &actual, sizeof(expected)) == 0 &&
        stock->motion_id == script->motion_id &&
        stock->ground_or_air == script->ground_or_air &&
        stock->self_vel.x == script->self_vel.x &&
        stock->self_vel.y == script->self_vel.y &&
        stock->gr_vel == script->gr_vel &&
        stock->x74_self_accel.y == script->x74_self_accel.y &&
        stock->x1968_jumpsUsed == script->x1968_jumpsUsed &&
        stock->ecb_lock == script->ecb_lock &&
        stock->coll_data.x130_flags == script->coll_data.x130_flags &&
        stock->x2219_b0 == script->x2219_b0 &&
        stock->cur_anim_frame == script->cur_anim_frame &&
        (stock->accessory4_cb != NULL) == (script->accessory4_cb != NULL) &&
        stock->pre_hitlag_cb == script->pre_hitlag_cb &&
        stock->post_hitlag_cb == script->post_hitlag_cb;
    if (!ok) {
        fprintf(stderr, "Spin parity failed: kind=%d phase=%s\n",
                stock->kind, phase);
        fprintf(stderr, "  state: %d/%d flags: %#x/%#x frame: %g/%g\n",
            expected.state, actual.state, expected.flags, actual.flags,
            expected.frame, actual.frame);
        fprintf(stderr, "  landing calls: %u/%u lag: %g/%g drift: %g/%g\n",
            expected.landings, actual.landings, expected.lag, actual.lag,
            expected.drift, actual.drift);
        fprintf(stderr, "  effects: %u/%u joints: %d,%d/%d,%d\n",
            expected.effects, actual.effects, expected.first_joint,
            expected.second_joint, actual.first_joint, actual.second_joint);
    }
    return ok;
}

int main(int argc, char** argv)
{
    if (argc != 2) return 2;
    HMODULE library = LoadLibraryExA(argv[1], NULL,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (library == NULL) return 2;
    ModuleEntry entry = (ModuleEntry) GetProcAddress(library,
        "FreakMelee_GetFighterModule");
    ParameterEntry set_parameters = (ParameterEntry) GetProcAddress(library,
        "FreakMelee_SetFighterParameters");
    for (unsigned i = 0; i < 80; ++i)
        parts[i].joint = (HSD_JObj*) (uintptr_t) (i + 1);
    NativeFighterHost host = {
        .callback_source_alive = callback_source_alive,
        .change_motion_state = Fighter_ChangeMotionState,
        .start_animation = ftAnim_8006EBA4,
        .animation_frames_remaining = ftAnim_IsFramesRemaining,
        .finish_ground_action = ft_8008A2BC,
        .ground_friction = ft_80084F3C,
        .fall = ftCommon_Fall,
        .air_accel_friction = ftCommon_CalcSelfAccel_DriftSimple,
        .force_air_and_use_jumps = ftCommon_8007D60C,
        .ground_collision = ft_80082708,
        .check_ground_and_ledge = ft_CheckGroundAndLedge,
        .check_cliff_catch = ftCliffCommon_80081298,
        .enter_landing_fall_special = ftCo_LandingFallSpecial_Enter,
        .enter_freefall = ftCo_80096900,
        .resolve_bone = ftParts_GetBoneIndex,
        .spawn_effect_between_parts = effect_between,
        .pause_effects = efLib_PauseAll, .resume_effects = efLib_ResumeAll,
        .set_visibility_group=unused_visibility,.set_current_visibility_group=unused_visibility,
        .bone_count=runtime_bones,.joint_position=unused_position,
        .create_held_article_at_center=unused_create,.owned_article_exists=unused_owned,
        .notify_item_pickup=unused_notify,.held_item_kind=unused_kind,.throw_held_item=unused_motion,
        .enter_fall=unused_object,.air_gravity_and_friction=unused_object,
        .ground_to_air=unused_fighter,.air_to_ground=unused_fighter,.air_collision=unused_air,
        .item_fall=unused_fall,.item_gravity=unused_object,.item_bounce=unused_object,
        .item_set_lifetime=unused_float,.item_reflect=unused_predicate,
        .item_shield_bounce=unused_predicate,.item_play_sound=unused_sound,.item_interact=unused_interaction,
        .item_dynamic_joint_value=unused_joint,.item_set_dynamic_joint_value=unused_joint_set,
        .item_set_animation_descriptor=unused_index,.item_sweep_stage_flags=unused_flags,
        .item_reset_default_ecb=unused_ecb,.item_set_grounded=unused_item,.item_set_airborne=unused_item,
        .item_bounce_stage_to_ground=unused_callback,.set_collision_facing=unused_facing,
        .spawn_wind=unused_wind,.abs_float=fabsf,.item_use_manual_lifetime=unused_object,
        .spawn_effect=efSync_Spawn,.random_float=unused_random,.item_change_state_flags=unused_state,
        .item_hide_model=unused_object,.item_release_attachment=unused_release,
        .item_begin_expiry=unused_object,.item_expire_tick=unused_predicate,.item_stop_motion=unused_object,
        .item_follow_ground=unused_callback,.item_slide_on_ground=unused_callbacks,
        .item_align_facing=unused_object,.model_animation_active=unused_model,
        .model_advance_animation=unused_model_advance,.item_creator=unused_creator,
    };
    bool ok = entry != NULL && set_parameters != NULL;
    Fighter* pair = calloc(2, sizeof(*pair));
    if (pair == NULL) return 2;
    for (unsigned family = 0; family < 2 && ok; ++family) {
        MotionState motions[21] = { 0 };
        for (unsigned i = 0; i < 21; ++i) motions[i].anim_cb = untouched;
        NativeFighterDefinition base = {
            .source_kind = family ? Ft_Kind_CLink : Ft_Kind_Link,
            .joint_count = family ? 79 : 75,
            .motion_state_first = 341, .motion_state_count = 21,
            .motion_states = motions,
            .special_ground_state = { [NativeFighterSpecial_Up] = 356,[NativeFighterSpecial_Down]=358 },
            .special_air_state = { [NativeFighterSpecial_Up] = 357,[NativeFighterSpecial_Down]=359 },
        };
        const NativeFighterDefinition* module = entry(&base, &host);
        NativeFighterParameters parameters;
        unsigned error_line;
        const char* values = family ? "int spin_effect_joint = 26\nint bomb_holding_joint = 68\n"
                                    : "int spin_effect_joint = 24\nint bomb_holding_joint = 64\n";
        uint32_t bits[2];
        ok = module != NULL &&
            nativeFighterParameters_Parse(values, strlen(values),
                &parameters, &error_line);
        if(ok) { bits[0]=parameters.fields[0].bits;bits[1]=parameters.fields[1].bits;
            ok=set_parameters(bits,2,parameters.schema_hash); }
        if (!ok) break;
        NativeFighterHost missing = host; missing.air_accel_friction = NULL;
        ok = entry(&base, &missing) == NULL;
        module = entry(&base, &host);
        ok = ok && module != NULL && module->article_logic_count == 2 &&
            module->motion_states[0].anim_cb == untouched &&
            module->motion_states[14].anim_cb == untouched &&
            module->motion_states[19].anim_cb == untouched;
        for (unsigned air = 0; air < 2 && ok; ++air) {
            ftLk_DatAttrs attrs = {
                .x30 = 13, .x34 = 0.25f,
                .specialairhi_drift_stick_mul = 0.5f,
                .specialairhi_drift_max_mul = 0.75f,
                .x40 = 3.5f, .specialhi_grav_mul = 2,
            };
            pair[0] = (Fighter){ .kind = base.source_kind, .x8_spawnNum = 101,
                .dat_attrs = &attrs, .parts = parts,
                .ground_or_air = air ? GA_Air : GA_Ground,
                .self_vel = { 8, -2, 0 }, .gr_vel = 1,
                .co_attrs = { .gravity = 0.125f, .terminal_velocity = 3,
                    .air_drift_stick_mul = 0.25f, .air_drift_max = 4,
                    .max_jumps = 2 },
            };
            pair[1] = pair[0]; remaining = true;
            HSD_GObjEvent native_entry = air ? ftLk_SpecialAirHi_Enter
                                            : ftLk_SpecialHi_Enter;
            HSD_GObjEvent script_entry = air ? module->special_air_hi
                                            : module->special_hi;
            unsigned state = air ? 16 : 15;
            ok &= compare(pair, pair + 1, native_entry, script_entry, "entry");
            ok &= pair[0].motion_id == (air ? 357 : 356) &&
                pair[0].self_vel.x == (air ? 2 : 8) &&
                pair[0].self_vel.y == (air ? 3.5f : -2);
            HSD_GObjEvent native_effect = pair[0].accessory4_cb;
            HSD_GObjEvent script_effect = pair[1].accessory4_cb;
            ok &= compare(pair, pair + 1, native_effect, script_effect, "effect");
            pair[0].accessory4_cb = native_effect;
            pair[1].accessory4_cb = script_effect;
            ok &= compare(pair, pair + 1, native_effect, script_effect,
                          "existing effect suppresses duplicate spawn");
            ok &= compare(pair, pair + 1,
                air ? ftLk_SpecialAirHi_Phys : ftLk_SpecialHi_Phys,
                module->motion_states[state].phys_cb, "physics");
            ground_contact = true; ledge_contact = false; cliff_contact = false;
            ok &= compare(pair, pair + 1,
                air ? ftLk_SpecialAirHi_Coll : ftLk_SpecialHi_Coll,
                module->motion_states[state].coll_cb, "no transition");
            remaining = false;
            ok &= compare(pair, pair + 1,
                air ? ftLk_SpecialAirHi_Anim : ftLk_SpecialHi_Anim,
                module->motion_states[state].anim_cb, "animation finish");
            if (!air) {
                pair[0].cur_anim_frame = pair[1].cur_anim_frame = 7.25f;
                ground_contact = false;
                ok &= compare(pair, pair + 1, ftLk_SpecialHi_Coll,
                    module->motion_states[15].coll_cb, "edge continuation");
                ok &= pair[0].motion_id == 357 && pair[0].ecb_lock == 5;
                ok &= compare(pair, pair + 1, ftLk_SpecialHi_Phys,
                    module->motion_states[15].phys_cb, "ground callback while airborne");
            }
            ledge_contact = true;
            ok &= compare(pair, pair + 1, ftLk_SpecialAirHi_Coll,
                module->motion_states[16].coll_cb, "special landing");
            ledge_contact = false; cliff_contact = true;
            ok &= compare(pair, pair + 1, ftLk_SpecialAirHi_Coll,
                module->motion_states[16].coll_cb, "cliff branch");
        }
    }
    free(pair); FreeLibrary(library);
    if (!ok) return 1;
    puts("Link and Young Link Spin Attack matches compiled stock callbacks, including effect joints and edge continuation.");
    return 0;
}
