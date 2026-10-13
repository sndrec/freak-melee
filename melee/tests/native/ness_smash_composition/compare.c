// Actual-source comparison with matching frozen43 or later reviewed inputs.
// Only the fighter slice is exercised; accepted resource components are
// separate.
#include <Runtime/platform.h>

#include <melee/ft/kinds/ftCommon/forward.h>

#include <placeholder.h>
#include <stdio.h>
#include <stdlib.h>

#include "behavior.generated.c"
#include <melee/ft/fighter.h>
#include <melee/ft/ft_07C1.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_0877.h>
#include <melee/ft/ft_0881.h>
#include <melee/ft/ft_0DF0.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcolanim.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/kinds/ftCommon/ftCo_Wait.h>
#include <melee/ft/kinds/ftNess/ftness.h>
#include <melee/ft/kinds/ftNess/ftnessattackhi4.h>
#include <melee/ft/kinds/ftNess/ftnessattacklw4.h>
#include <melee/ft/kinds/ftNess/ftnessattacks4.h>
#include <melee/ft/kinds/ftNess/types.h>
#include <melee/it/itYoyo.h>
#include <melee/it/kinds/itnessbat.h>
#include <melee/it/kinds/itnessyoyo.h>
#include <melee/it/types.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lbcollision.h>
#include <melee/lb/lbvector.h>
#include <melee/mp/mpcoll.h>

static Fighter fighters[2];
static HSD_GObj owners[2], items[2][2];
static Item item_data[2][2];
static Article yoyo_article;
static itYoyoAttributes yoyo;
static ftNessAttributes attributes;
static FighterBone bones[2][62];
static HSD_JObj joints[2][62];
static bool created, airborne_result, finished, alive[2], item_alive[2][2];
static unsigned side, checks, failures, pairs, entry_pairs, helper_pairs;
static unsigned expected_cleanup_deltas;
static unsigned coverage[7][4], sweep_calls;
static int case_motion, case_phase, case_variant;
static u32 sweep_flags[2];
static const NativeFighterDefinition* definition;
static NativeFighterParameters parameters;
static uint32_t words[NATIVE_FIGHTER_PARAMETER_CAPACITY];
static struct Fighter_ShakeTable_t shake_table = { .x4 = 5 };
struct Fighter_ShakeTable_t* Fighter_SmashChargeShakeTable = &shake_table;
static struct ftCommonData common = { .x7C8 = 30 };
struct ftCommonData* p_ftCommonData = &common;
typedef struct SweepDescriptor {
    Vec3 previous, current;
    ftCollisionBox box;
    int category;
} SweepDescriptor;
static SweepDescriptor probes[2][2];
static unsigned probe_counts[2], common_pairs;
static bool apply_transition_cleanup;
static MotionFlags cleanup_transition_flags;
static void transition_hitbox_cleanup(Fighter_GObj*, MotionFlags);
enum {
    InvalidateNone,
    InvalidateTransition,
    InvalidateConstructor,
    InvalidateSignal
};
static unsigned invalidation, safety_cases;
static bool recycle_source;
static void invalidate(unsigned boundary)
{
    if (side == 1 && invalidation == boundary) {
        if (recycle_source) {
            ++fighters[1].x8_spawnNum;
        } else {
            alive[1] = false;
        }
    }
}
typedef struct SmashEvent {
    int kind, id;
    float x, y, z;
} SmashEvent;
static SmashEvent events[2][64];
static unsigned counts[2];
enum {
    Change = 1,
    Start,
    Rate,
    Wait,
    WaitInput,
    Friction,
    GroundMotion,
    ReflectUpdate,
    Collision,
    Create,
    Remove,
    Pause,
    Resume,
    Material,
    Released,
    Held,
    Retract,
    Sound,
    Color,
    Rumble,
    Damage,
    Position,
    Sweep
};
enum {
    Bat,
    Yoyo
};
static void check(bool value, const char* message)
{
    ++checks;
    if (!value && failures++ < 12) {
        fprintf(stderr, "motion%d phase%d variant%d: %s\n", case_motion,
                case_phase, case_variant, message);
    }
}
static void event(int kind, int id, float x, float y, float z)
{
    check(counts[side] < 64, "event capacity");
    if (counts[side] < 64) {
        events[side][counts[side]++] = (SmashEvent){ kind, id, x, y, z };
    }
}
static unsigned state_offset(const char* name)
{
    for (unsigned i = 0; i < definition->state_field_count; ++i) {
        if (!strcmp(name, definition->state_fields[i].name)) {
            return definition->state_fields[i].offset;
        }
    }
    abort();
}
static void integer(const char* name, int32_t value)
{
    memcpy(fighters[1].native_module_state + state_offset(name), &value, 4);
}
static int32_t get_int(const char* name)
{
    int32_t value;
    memcpy(&value, fighters[1].native_module_state + state_offset(name), 4);
    return value;
}
static void number(const char* name, float value)
{
    memcpy(fighters[1].native_module_state + state_offset(name), &value, 4);
}
static float get_float(const char* name)
{
    float value;
    memcpy(&value, fighters[1].native_module_state + state_offset(name), 4);
    return value;
}
static void boolean(const char* name, bool value)
{
    fighters[1].native_module_state[state_offset(name)] = value;
}
static bool get_bool(const char* name)
{
    return fighters[1].native_module_state[state_offset(name)] != 0;
}
static uint32_t hash(const char* name)
{
    uint32_t value = 2166136261u;
    while (*name) {
        value = (value ^ (uint8_t) *name++) * 16777619u;
    }
    return value;
}
static uint32_t handle(unsigned kind)
{
    return UINT32_C(0x80000005) + kind;
}
static float parameter(const char* name)
{
    for (unsigned i = 0; i < parameters.count; ++i) {
        if (!strcmp(name, parameters.fields[i].name)) {
            float value;
            memcpy(&value, &words[i], 4);
            return parameters.fields[i].type == NativeFighterParameter_Float
                       ? value
                       : (float) (int32_t) words[i];
        }
    }
    abort();
}
static void edit(const char* name, float value)
{
    for (unsigned i = 0; i < parameters.count; ++i) {
        if (!strcmp(name, parameters.fields[i].name)) {
            if (parameters.fields[i].type == NativeFighterParameter_Float) {
                memcpy(&words[i], &value, 4);
            } else {
                words[i] = (uint32_t) (int32_t) value;
            }
            return;
        }
    }
    abort();
}
static void unexpected(void)
{
    fprintf(stderr, "Unexpected unexercised Host boundary\n");
    abort();
}
static bool source_alive(HSD_GObj* object, uint32_t generation, uint32_t kind)
{
    for (unsigned i = 0; i < 2; ++i) {
        if (object == &owners[i]) {
            return kind == NativeCallbackSource_Fighter && alive[i] &&
                   generation == fighters[i].x8_spawnNum;
        }
    }
    return false;
}
void Fighter_ChangeMotionState(Fighter_GObj* object, FtMotionId motion,
                               MotionFlags flags, float start, float rate,
                               float blend, Fighter_GObj* donor)
{
    Fighter* fighter = object->user_data;
    check(!donor && blend == 0, "motion donor/blend");
    event(Change, motion, start, rate, (float) flags);
    fighter->motion_id = motion;
    fighter->cur_anim_frame = start;
    fighter->frame_speed_mul = rate;
    fighter->smash_attrs.state = SmashState_None;
    invalidate(InvalidateTransition);
}
void ftAnim_8006EBA4(Fighter_GObj* object)
{
    (void) object;
    event(Start, 0, 0, 0, 0);
}
void ftAnim_SetAnimRate(Fighter_GObj* object, float rate)
{
    ((Fighter*) object->user_data)->frame_speed_mul = rate;
    event(Rate, 0, rate, 0, 0);
}
bool ftAnim_IsFramesRemaining(Fighter_GObj* object)
{
    (void) object;
    return !finished;
}
void ft_8008A2BC(Fighter_GObj* object)
{
    ((Fighter*) object->user_data)->motion_id = ftCo_MS_Wait;
    if (apply_transition_cleanup) {
        transition_hitbox_cleanup(object, cleanup_transition_flags);
    }
    event(Wait, 0, 0, 0, 0);
}
void ftCo_Wait_IASA(Fighter_GObj* object)
{
    if (apply_transition_cleanup) {
        ((Fighter*) object->user_data)->motion_id = ftCo_MS_KneeBend;
        transition_hitbox_cleanup(object, cleanup_transition_flags);
    }
    event(WaitInput, 0, 0, 0, 0);
}
void ft_8007C114(Fighter_GObj* object)
{
    (void) object;
    check(false, "unexercised hurtbox-cleanup branch");
}
void ft_80084F3C(Fighter_GObj* object)
{
    (void) object;
    event(Friction, 0, 0, 0, 0);
}
void ft_80084FA8(Fighter_GObj* object)
{
    (void) object;
    event(GroundMotion, 0, 0, 0, 0);
}
void ftColl_8007AEF8(Fighter_GObj* object)
{
    (void) object;
    event(ReflectUpdate, 0, 0, 0, 0);
}
void ft_80084104(Fighter_GObj* object)
{
    ((Fighter*) object->user_data)->ground_or_air =
        airborne_result ? GA_Air : GA_Ground;
    event(Collision, 0, 0, 0, 0);
}
void ftColl_CreateReflectHit(Fighter_GObj* object, ReflectDesc* desc,
                             HSD_GObjEvent callback)
{
    Fighter* fighter = object->user_data;
    ReflectDesc* expected = &attributes.xB8_BASEBALL_BAT;
    fighter->reflecting = true;
    fighter->reflect_hit_cb = callback;
    check(desc->x0_bone_id == expected->x0_bone_id &&
              desc->x4_max_damage == expected->x4_max_damage &&
              !memcmp(&desc->x8_offset, &expected->x8_offset, sizeof(Vec3)) &&
              desc->x14_size == expected->x14_size &&
              desc->x18_damage_mul == expected->x18_damage_mul &&
              desc->x1C_speed_mul == expected->x1C_speed_mul &&
              desc->x20_behavior == expected->x20_behavior,
          "owned reflector descriptor");
    event(ReflectUpdate, desc->x0_bone_id, desc->x14_size,
          desc->x18_damage_mul, desc->x1C_speed_mul);
}
void ftColl_8007ABD0(HitCapsule* hit, u32 damage, Fighter_GObj* object)
{
    (void) object;
    hit->damage = (float) damage;
    event(Damage, 0, (float) damage, 0, 0);
}
void lb_8000B1CC(HSD_JObj* joint, Vec3* offset, Vec3* result)
{
    check(offset == NULL, "joint origin query");
    *result = joint->translate;
}
void ft_80088510(Fighter* fighter, int id, u8 volume, u8 pan)
{
    (void) fighter;
    event(Sound, id, volume, pan, 1);
}
void ft_PlaySFX(Fighter* fighter, enum_t id, u8 volume, u8 pan)
{
    (void) fighter;
    event(Sound, id, volume, pan, 0);
}
bool ftCo_800BFFD0(Fighter* fighter, enum_t id, bool value)
{
    (void) fighter;
    event(Color, id, value, 0, 0);
    return true;
}
void ftCommon_8007EBAC(Fighter* fighter, u32 id, u32 value)
{
    (void) fighter;
    event(Rumble, id, value, 0, 0);
}
void ftCo_800C0200(Fighter* fighter, int color)
{
    (void) fighter;
    event(Color, color, 0, 0, 1);
}
void ftCommon_8007ECD4(Fighter* fighter, s32 id)
{
    (void) fighter;
    event(Rumble, id, 0, 0, 1);
}
void mpColl_80041EE4(CollData* collision)
{
    memset(collision, 0, sizeof(*collision));
}
bool mpColl_8004730C(CollData* collision, ftCollisionBox* box)
{
    check(sweep_calls < 2, "sweep count");
    event(Sweep, collision->x34_flags.b1234, box->top, collision->last_pos.y,
          collision->cur_pos.y);
    check(box->top == -box->bottom && box->left.x == box->bottom &&
              box->right.x == box->top,
          "sweep symmetric box");
    if (sweep_calls < 2) {
        probes[side][sweep_calls] =
            (SweepDescriptor){ collision->last_pos, collision->cur_pos, *box,
                               collision->x34_flags.b1234 };
    }
    collision->env_flags = sweep_flags[sweep_calls++];
    probe_counts[side] = sweep_calls;
    return collision->env_flags != 0;
}
void ftNs_Init_OnDamage(Fighter_GObj* object)
{
    (void) object;
    abort();
}
static void marker(Fighter_GObj* object)
{
    (void) object;
}
Item_GObj* it_802AD478(Item_GObj* object, Vec3* position, Fighter_Part part,
                       float facing)
{
    (void) object;
    check(part == 42, "Bat raw part42");
    event(Create, Bat, position->x, position->y, facing);
    item_alive[side][Bat] = created;
    return created ? &items[side][Bat] : NULL;
}
HSD_GObj* it_802BE9D8(HSD_GObj* object, float facing, Vec3* position,
                      s32 motion)
{
    (void) object;
    check(motion == fighters[side].motion_id, "YoYo source motion");
    event(Create, Yoyo, position->x, position->y, facing);
    item_alive[side][Yoyo] = created;
    return created ? &items[side][Yoyo] : NULL;
}
void it_802AD6B8(Item_GObj* object)
{
    check(object == &items[side][Bat], "Bat remove identity");
    event(Remove, Bat, 0, 0, 0);
    item_alive[side][Bat] = false;
}
void it_802BE598(Item_GObj* object)
{
    check(object == &items[side][Yoyo], "YoYo pause identity");
    event(Pause, Yoyo, 0, 0, 0);
}
void it_802BE5B8(Item_GObj* object)
{
    check(object == &items[side][Yoyo], "YoYo resume identity");
    event(Resume, Yoyo, 0, 0, 0);
}
void it_802BE958(Item_GObj* object)
{
    check(object == &items[side][Yoyo], "YoYo remove identity");
    event(Remove, Yoyo, 0, 0, 0);
    ftNs_AttackHi4_YoyoItemSetFlag(&owners[side]);
    item_alive[side][Yoyo] = false;
}
void it_802BE5D8(void* object, float frame)
{
    (void) object;
    event(Material, Yoyo, frame, 0, 0);
}
void it_802BFE5C(Item_GObj* object, Vec3* velocity, float unused)
{
    (void) unused;
    (void) object;
    event(Released, Yoyo,
          fabsf(velocity->x) * item_data[side][Yoyo].facing_dir, velocity->y,
          velocity->z);
}
void it_802C0010(Item_GObj* object, Vec3* velocity)
{
    (void) object;
    event(Held, Yoyo, velocity->x, velocity->y, velocity->z);
}
void it_802BFEC4(Item_GObj* object)
{
    (void) object;
    event(Retract, Yoyo, 0, 0, 0);
}
static unsigned bone_count(Fighter* fighter)
{
    (void) fighter;
    return 62;
}
static float common_attribute(NativeFighterCommonAttribute kind)
{
    check(kind == NativeFighterCommon_SmashShakeCount, "shake count key");
    return 5;
}
static int32_t create_article(Fighter_GObj* owner, ItemKind kind,
                              Vec3* position, Vec3* velocity, float facing,
                              bool at_owner, uint32_t group, float value,
                              bool collision)
{
    (void) owner;
    check(at_owner && group == 0, "constructor author origin/group");
    unsigned item = kind == It_Kind_Ness_Bat ? Bat : Yoyo;
    check(velocity->x == 0 && velocity->y == 0 && velocity->z == 0,
          "constructor initial velocity");
    check(collision == (item == Yoyo), "constructor initial collision");
    check(item == Bat ? value == 0 : value == fighters[side].motion_id,
          "constructor authored value");
    event(Create, item, position->x, position->y, facing);
    item_alive[side][item] = created;
    invalidate(InvalidateConstructor);
    return created ? (int32_t) handle(item) : -1;
}
static bool bind_item(Fighter_GObj* object, uint32_t id, bool auxiliary)
{
    check(auxiliary && id == handle(Yoyo), "auxiliary publication");
    ((Fighter*) object->user_data)->x1984_heldItemSpec = &items[side][Yoyo];
    return true;
}
static bool clear_item(Fighter_GObj* object, bool auxiliary)
{
    check(auxiliary, "auxiliary failure clearing");
    ((Fighter*) object->user_data)->x1984_heldItemSpec = NULL;
    return true;
}
static bool signal_article(HSD_GObj* source, uint32_t id, uint32_t name,
                           float payload)
{
    check(source == &owners[side], "signal source");
    unsigned item = id == handle(Bat) ? Bat : Yoyo;
    if (id != handle(item) || !item_alive[side][item]) {
        return false;
    }
    if (name == hash("remove")) {
        event(Remove, item, 0, 0, 0);
        if (item == Yoyo) {
            script_fn_yoyo_forget(source);
        }
        item_alive[side][item] = false;
    } else if (name == hash("resume")) {
        event(Resume, item, 0, 0, 0);
    } else if (name == hash("pause")) {
        event(Pause, item, 0, 0, 0);
    } else if (name == hash("material")) {
        event(Material, item, payload, 0, 0);
    } else if (name == hash("held")) {
        event(Held, item, 0, 0, 0);
    } else if (name == hash("retract")) {
        event(Retract, item, 0, 0, 0);
    } else if (name == hash("charge_release")) {
        event(Released, item,
              item_data[side][Yoyo].facing_dir *
                  fabsf(yoyo.release_velocity_x * payload),
              yoyo.release_velocity_y, 0);
    } else {
        abort();
    }
    invalidate(InvalidateSignal);
    return true;
}
static void load_values(void)
{
    memset(&attributes, 0, sizeof(attributes));
    memset(&yoyo, 0, sizeof(yoyo));
    attributes.xAC_YOYO_CHARGE_DURATION = parameter("yoyo_charge_duration");
    attributes.xB0_YOYO_DAMAGE_MUL = parameter("yoyo_damage_multiplier");
    attributes.xB4_YOYO_REHIT_RATE = parameter("yoyo_rehit_rate");
    attributes.xB8_BASEBALL_BAT =
        (ReflectDesc){ (u32) parameter("bat_reflect_joint"),
                       (s32) parameter("bat_reflect_max_damage"),
                       { parameter("bat_reflect_x"),
                         parameter("bat_reflect_y"),
                         parameter("bat_reflect_z") },
                       parameter("bat_reflect_radius"),
                       parameter("bat_reflect_damage_multiplier"),
                       parameter("bat_reflect_speed_multiplier"),
                       (u8) parameter("bat_reflect_behavior") };
    yoyo.released_spin_rate =
        parameter("It_Kind_Ness_Yoyo.released_spin_rate");
    yoyo.charged_spin_rate = parameter("It_Kind_Ness_Yoyo.charged_spin_rate");
    yoyo.initial_charge_spin_rate =
        parameter("It_Kind_Ness_Yoyo.initial_charge_spin_rate");
    yoyo.release_velocity_x =
        parameter("It_Kind_Ness_Yoyo.release_velocity_x");
    yoyo.release_velocity_y =
        parameter("It_Kind_Ness_Yoyo.release_velocity_y");
    yoyo.up_smash_rotate_frame =
        (s32) parameter("It_Kind_Ness_Yoyo.up_smash_rotate_frame");
    yoyo.up_smash_retract_frame =
        (s32) parameter("It_Kind_Ness_Yoyo.up_smash_retract_frame");
    yoyo.down_smash_rotate_frame =
        (s32) parameter("It_Kind_Ness_Yoyo.down_smash_rotate_frame");
    yoyo.down_smash_retract_frame =
        (s32) parameter("It_Kind_Ness_Yoyo.down_smash_retract_frame");
    yoyo_article.x4_specialAttributes = &yoyo;
}
static void seed(int motion, int variant)
{
    memset(fighters, 0, sizeof(fighters));
    memset(events, 0, sizeof(events));
    counts[0] = counts[1] = 0;
    memset(probes, 0, sizeof(probes));
    probe_counts[0] = probe_counts[1] = 0;
    invalidation = InvalidateNone;
    static const int frames[] = { 0,  1,  2,  3,  4,  12, 13, 14,
                                  18, 19, 23, 24, 48, 49, 59, 60 };
    int current = frames[variant % 16];
    created = (variant & 16) != 0;
    airborne_result = (variant & 2) != 0;
    finished = (variant & 4) != 0;
    for (unsigned i = 0; i < 2; ++i) {
        owners[i].user_data = &fighters[i];
        alive[i] = true;
        fighters[i].x8_spawnNum = UINT32_C(0xFEDCBA98) + i;
        fighters[i].motion_id = motion;
        fighters[i].dat_attrs = &attributes;
        fighters[i].parts = bones[i];
        fighters[i].facing_dir = (variant & 1) ? -1 : 1;
        fighters[i].x34_scale = (Vec3){ 1, 1.25f, 1 };
        fighters[i].cur_pos = (Vec3){ 3, -4, 5 };
        fighters[i].frame_speed_mul = 0;
        fighters[i].ground_or_air = GA_Ground;
        fighters[i].cmd_vars[0] = variant % 2;
        fighters[i].cmd_vars[1] = variant % 3;
        fighters[i].allow_interrupt = variant % 2;
        fighters[i].input.held_buttons[0] = (variant & 1) ? HSD_PAD_A : 0;
        fighters[i].coll_data.floor.normal = (Vec3){ 0.25f, 1, 0 };
        fighters[i].coll_data.ecb.top.y = 4;
        fighters[i].coll_data.ecb.bottom.y = -2;
        fighters[i].smash_attrs.state = (SmashState) 4;
        fighters[i].smash_attrs.x2118_frames = variant * 3;
        fighters[i].smash_attrs.x2120_damageMul = 1.4f;
        fighters[i].smash_attrs.x2130_sfxBool = variant % 2;
        fighters[i].x914[0].state = (HitCapsuleState) (variant % 5);
        fighters[i].x914[0].damage = 10.75f;
        fighters[i].x914[0].x44 = 3;
        fighters[i].x914[0].x45 = 4;
        for (unsigned n = 0; n < 12; ++n) {
            fighters[i].x914[0].victims_1[n].victim =
                (void*) (uintptr_t) (n + 1);
            fighters[i].x914[0].victims_2[n].victim =
                (void*) (uintptr_t) (n + 20);
            fighters[i].x914[0].victims_1[n].x4 = 12 + n;
            fighters[i].x914[0].victims_2[n].x4 = 24 + n;
        }
        for (unsigned b = 0; b < 62; ++b) {
            bones[i][b].joint = &joints[i][b];
            joints[i][b].translate = (Vec3){ b * 0.1f, -b * 0.2f, 0.75f };
        }
        for (unsigned n = 0; n < 2; ++n) {
            items[i][n].user_data = &item_data[i][n];
            item_alive[i][n] = true;
            item_data[i][n].xC4_article_data = &yoyo_article;
            item_data[i][n].facing_dir = motion >= 345
                                             ? -fighters[i].facing_dir
                                             : fighters[i].facing_dir;
        }
        fighters[i].x1984_heldItemSpec = &items[i][Yoyo];
        if (variant & 32) {
            fighters[i].death2_cb = marker;
            fighters[i].take_dmg_cb = marker;
        }
    }
    fighters[0].u.ns.yoyo_gobj = &items[0][Yoyo];
    fighters[0].u.ns.bat_gobj = &items[0][Bat];
    fighters[0].mv.ns.attackhi4.yoyoCurrentFrame = current;
    fighters[0].mv.ns.attackhi4.yoyoRehitTimer = variant % 3;
    fighters[0].mv.ns.attackhi4.isChargeDisable = (variant & 2) != 0;
    fighters[0].mv.ns.attackhi4.isPosUpdateMod = (variant & 4) != 0;
    fighters[0].u.ns.yoyo_hitbox_pos = (Vec3){ 2, -3, 1 };
    fighters[0].u.ns.x223C = 0.5f;
    integer("yoyo_handle", (int32_t) handle(Yoyo));
    integer("bat_handle", (int32_t) handle(Bat));
    integer("yoyo_frame", current);
    integer("yoyo_rehit_timer", variant % 3);
    boolean("yoyo_charge_disabled", (variant & 2) != 0);
    boolean("yoyo_slope_correct", (variant & 4) != 0);
    number("yoyo_hitbox_x", 2);
    number("yoyo_hitbox_y", -3);
    number("yoyo_hitbox_z", 1);
    number("yoyo_spin_rate", 0.5f);
    sweep_flags[0] = (variant & 8) ? Collide_LeftWallMask : 0;
    sweep_flags[1] = Collide_FloorPush;
}
static void compare(void)
{
    Fighter* source = &fighters[0];
    Fighter* authored = &fighters[1];
    check(source->motion_id == authored->motion_id, "motion");
    check(source->allow_interrupt == authored->allow_interrupt,
          "interrupt flag");
    check(!memcmp(source->cmd_vars, authored->cmd_vars,
                  sizeof(source->cmd_vars)),
          "commands");
    check(source->mv.ns.attackhi4.yoyoCurrentFrame == get_int("yoyo_frame"),
          "owned frame");
    check(source->mv.ns.attackhi4.yoyoRehitTimer ==
              get_int("yoyo_rehit_timer"),
          "owned rehit timer");
    check((source->mv.ns.attackhi4.isChargeDisable != 0) ==
              get_bool("yoyo_charge_disabled"),
          "owned charge flag");
    check((source->mv.ns.attackhi4.isPosUpdateMod != 0) ==
              get_bool("yoyo_slope_correct"),
          "owned slope flag");
    Vec3 position = { get_float("yoyo_hitbox_x"), get_float("yoyo_hitbox_y"),
                      get_float("yoyo_hitbox_z") };
    check(!memcmp(&source->u.ns.yoyo_hitbox_pos, &position, sizeof(position)),
          "owned hit XYZ");
    check(source->u.ns.x223C == get_float("yoyo_spin_rate"), "owned spin");
    check((source->u.ns.yoyo_gobj != NULL) == (get_int("yoyo_handle") != -1),
          "published YoYo");
    check((source->u.ns.bat_gobj != NULL) == (get_int("bat_handle") != -1),
          "published Bat");
    check(!memcmp(&source->smash_attrs, &authored->smash_attrs,
                  sizeof(SmashAttr)),
          "charge storage");
    check(source->frame_speed_mul == authored->frame_speed_mul,
          "animation rate");
    check(source->x914[0].state == authored->x914[0].state &&
              source->x914[0].damage == authored->x914[0].damage,
          "hitbox state/damage");
    check(source->x2222_b2 == authored->x2222_b2, "Song immunity");
    check(!memcmp(source->x914[0].victims_1, authored->x914[0].victims_1,
                  sizeof(source->x914[0].victims_1)) &&
              !memcmp(source->x914[0].victims_2, authored->x914[0].victims_2,
                      sizeof(source->x914[0].victims_2)) &&
              source->x914[0].x44 == authored->x914[0].x44 &&
              source->x914[0].x45 == authored->x914[0].x45,
          "victim history and retained timers");
    check(source->reflecting == authored->reflecting, "reflector flag");
    // The deliberate generic-enable correction is the only expected delta:
    // original delayed refresh clears victims and enables without this latch.
    // Transition behavior itself is tested separately with its real source
    // block.
    bool refreshed = source->x914[0].x44 == 0 && source->x914[0].x45 == 0 &&
                     source->x914[0].victims_1[0].victim == NULL &&
                     source->x914[0].victims_2[0].victim == NULL;
    check(authored->x2219_b3 == (source->x2219_b3 || refreshed),
          "only delayed-refresh cleanup latch may differ");
    if (refreshed && !source->x2219_b3) {
        ++expected_cleanup_deltas;
    }
    check((source->death2_cb == marker) == (authored->death2_cb == marker) &&
              (source->death2_cb != NULL) == (authored->death2_cb != NULL),
          "death2 preservation/binding");
    check((source->take_dmg_cb == marker) ==
                  (authored->take_dmg_cb == marker) &&
              (source->take_dmg_cb != NULL) == (authored->take_dmg_cb != NULL),
          "damage preservation/binding");
    check((source->pre_hitlag_cb != NULL) ==
                  (authored->pre_hitlag_cb != NULL) &&
              (source->post_hitlag_cb != NULL) ==
                  (authored->post_hitlag_cb != NULL),
          "hitlag binding");
    check((source->deal_dmg_cb != NULL) == (authored->deal_dmg_cb != NULL) &&
              (source->accessory4_cb != NULL) ==
                  (authored->accessory4_cb != NULL),
          "deal/accessory binding");
    if (source->death2_cb == ftNs_Init_OnDamage) {
        check(authored->death2_cb == script_fn_SmashResourceDamage,
              "death2 exact authored cleanup binding");
    }
    if (source->take_dmg_cb == ftNs_Init_OnDamage) {
        check(authored->take_dmg_cb == script_fn_SmashResourceDamage,
              "damage exact authored cleanup binding");
    }
    if (source->pre_hitlag_cb == ftNs_AttackHi4_YoyoItemSetUnk) {
        check(authored->pre_hitlag_cb == authored_pre_hitlag,
              "pre-hitlag exact binding");
    }
    if (source->post_hitlag_cb == ftNs_AttackHi4_YoyoItemSetUnk2) {
        check(authored->post_hitlag_cb == authored_post_hitlag,
              "post-hitlag exact binding");
    }
    if (source->deal_dmg_cb == ftNs_AttackHi4_YoyoStartTimedRehit) {
        check(authored->deal_dmg_cb == authored_deal_damage,
              "deal-damage exact binding");
    }
    if (source->accessory4_cb == ftNs_AttackHi4_YoyoUpdateHitPos) {
        check(authored->accessory4_cb == script_fn_YoyoUpdateHitPos,
              "accessory exact binding");
    }
    if (source->reflect_hit_cb == ftNs_AttackS4_OnReflect) {
        check(authored->reflect_hit_cb == authored_reflect_hit,
              "reflect-hit exact binding");
    }
    check((source->x1984_heldItemSpec == &items[0][Yoyo]) ==
              (authored->x1984_heldItemSpec == &items[1][Yoyo]),
          "auxiliary resource publication");
    check(!memcmp(&source->x914[0].b_offset, &authored->x914[0].b_offset,
                  sizeof(Vec3)) &&
              source->x914[0].jobj == authored->x914[0].jobj,
          "world hitbox attachment");
    check(probe_counts[0] == probe_counts[1] &&
              !memcmp(probes[0], probes[1],
                      probe_counts[0] * sizeof(SweepDescriptor)),
          "full swept ECB descriptors/order");
    check(counts[0] == counts[1], "event count");
    check(!memcmp(events[0], events[1], counts[0] * sizeof(SmashEvent)),
          "ordered events");
}
#include "cleanup_cases.h"
#include "original_helpers.h"

int main(int argc, char** argv)
{
    if (argc != 2) {
        return 2;
    }
    FILE* input = fopen(argv[1], "rb");
    if (!input) {
        return 2;
    }
    char text[NATIVE_FIGHTER_PARAMETER_TEXT_CAPACITY + 1];
    size_t length = fread(text, 1, sizeof(text) - 1, input);
    fclose(input);
    text[length] = 0;
    check(nativeFighterParameters_Parse(text, length, &parameters, NULL),
          "owned parameter parse");
    for (unsigned i = 0; i < parameters.count; ++i) {
        words[i] = parameters.fields[i].bits;
    }
    check(FreakMelee_SetFighterParameters(words, parameters.count,
                                          parameters.schema_hash),
          "generated parameter binding");
    load_values();
    NativeFighterHost host = { 0 };
#include "required_host.h"
    host.callback_source_alive = source_alive;
    host.change_motion_state = Fighter_ChangeMotionState;
    host.start_animation = ftAnim_8006EBA4;
    host.set_animation_rate = ftAnim_SetAnimRate;
    host.animation_frames_remaining = ftAnim_IsFramesRemaining;
    host.finish_ground_action = ft_8008A2BC;
    host.ground_actions = ftCo_Wait_IASA;
    host.ground_friction = ft_80084F3C;
    host.ground_motion = ft_80084FA8;
    host.update_reflector = ftColl_8007AEF8;
    host.ground_standard_collision = ft_80084104;
    host.create_reflector = ftColl_CreateReflectHit;
    host.set_hitbox_damage = ftColl_8007ABD0;
    host.set_hitbox_position = ftColl_8007B8A8;
    host.joint_position = lb_8000B1CC;
    host.bone_count = bone_count;
    host.angle_atan2_float = atan2f;
    host.vector_rotate = lbVector_Rotate;
    host.loop_sound = ft_80088510;
    host.play_sound = ft_PlaySFX;
    host.start_color = ftCo_800BFFD0;
    host.rumble = ftCommon_8007EBAC;
    host.initialize_collision = mpColl_80041EE4;
    host.sweep_collision_box = mpColl_8004730C;
    host.common_attribute = common_attribute;
    host.create_article_at = create_article;
    host.bind_fighter_item = bind_item;
    host.clear_fighter_item = clear_item;
    host.signal_article = signal_article;
    host.clear_hitbox_victims = lbColl_80008440;
    MotionState motion[35] = { 0 };
    NativeFighterDefinition base = { .fighter_root = "ftDataNess",
                                     .motion_state_first = 341,
                                     .motion_state_count = 35,
                                     .motion_states = motion };
    definition = FreakMelee_GetFighterModule(&base, &host);
    check(definition != NULL, "module entry");
    if (!definition) {
        return 2;
    }
    HSD_GObjEvent originals[7][4] = {
        { ftNs_AttackS4_Anim, ftNs_AttackS4_IASA, ftNs_AttackS4_Phys,
          ftNs_AttackS4_Coll },
        { ftNs_AttackHi4_Anim, ftNs_AttackHi4_IASA, ftNs_AttackHi4_Phys,
          ftNs_AttackHi4_Coll },
        { ftNs_AttackHi4Charge_Anim, ftNs_AttackHi4Charge_IASA,
          ftNs_AttackHi4Charge_Phys, ftNs_AttackHi4Charge_Coll },
        { ftNs_AttackHi4Release_Anim, ftNs_AttackHi4Release_IASA,
          ftNs_AttackHi4Release_Phys, ftNs_AttackHi4Release_Coll },
        { ftNs_AttackLw4_Anim, ftNs_AttackLw4_IASA, ftNs_AttackLw4_Phys,
          ftNs_AttackLw4_Coll },
        { ftNs_AttackLw4Charge_Anim, ftNs_AttackLw4Charge_IASA,
          ftNs_AttackLw4Charge_Phys, ftNs_AttackLw4Charge_Coll },
        { ftNs_AttackLw4Release_Anim, ftNs_AttackLw4Release_IASA,
          ftNs_AttackLw4Release_Phys, ftNs_AttackLw4Release_Coll }
    };
    for (unsigned tuning = 0; tuning < 2; ++tuning) {
        if (tuning) {
            edit("yoyo_charge_duration", 45.5f);
            edit("yoyo_damage_multiplier", 480);
            edit("yoyo_rehit_rate", 7);
            edit("It_Kind_Ness_Yoyo.released_spin_rate", -0.25f);
            edit("It_Kind_Ness_Yoyo.charged_spin_rate", 0.3f);
            edit("It_Kind_Ness_Yoyo.initial_charge_spin_rate", 1.2f);
            edit("It_Kind_Ness_Yoyo.up_smash_rotate_frame", 6);
            edit("It_Kind_Ness_Yoyo.up_smash_retract_frame", 47);
            edit("It_Kind_Ness_Yoyo.down_smash_rotate_frame", 8);
            edit("It_Kind_Ness_Yoyo.down_smash_retract_frame", 57);
            check(FreakMelee_SetFighterParameters(words, parameters.count,
                                                  parameters.schema_hash),
                  "edited parameter binding");
            load_values();
        }
        for (unsigned state = 0; state < 7; ++state) {
            for (unsigned phase = 0; phase < 4; ++phase) {
                for (int variant = 0; variant < 64; ++variant) {
                    case_motion = 341 + state;
                    case_phase = phase;
                    case_variant = variant;
                    seed(341 + state, variant);
                    const MotionState* emitted =
                        &definition->motion_states[state];
                    HSD_GObjEvent callbacks[] = { emitted->anim_cb,
                                                  emitted->input_cb,
                                                  emitted->phys_cb,
                                                  emitted->coll_cb };
                    side = 0;
                    sweep_calls = 0;
                    originals[state][phase](&owners[0]);
                    side = 1;
                    sweep_calls = 0;
                    callbacks[phase](&owners[1]);
                    compare();
                    ++pairs;
                    ++coverage[state][phase];
                }
            }
        }
        HSD_GObjEvent entries[] = {
            ftNs_AttackS4_Enter,        ftNs_AttackHi4_Enter,
            ftNs_AttackLw4_Enter,       ftNs_AttackHi4Charge_Enter,
            ftNs_AttackLw4Charge_Enter, ftNs_AttackHi4Release_Enter,
            ftNs_AttackLw4Release_Enter
        };
        for (unsigned entry = 0; entry < 7; ++entry) {
            for (int variant = 0; variant < 64; ++variant) {
                case_motion = 341;
                case_phase = 4 + entry;
                case_variant = variant;
                seed(entry >= 2 ? 345 : 342, variant);
                side = 0;
                entries[entry](&owners[0]);
                side = 1;
                switch (entry) {
                case 0:
                    definition->on_forward_smash_enter(&owners[1], 0.35f);
                    break;
                case 1:
                    definition->on_up_smash_enter(&owners[1]);
                    break;
                case 2:
                    definition->on_down_smash_enter(&owners[1]);
                    break;
                case 3:
                    script_fn_YoyoChargeEnter(&owners[1], true);
                    break;
                case 4:
                    script_fn_YoyoChargeEnter(&owners[1], false);
                    break;
                case 5:
                    script_fn_YoyoReleaseEnter(&owners[1], true);
                    break;
                case 6:
                    script_fn_YoyoReleaseEnter(&owners[1], false);
                    break;
                }
                compare();
                ++entry_pairs;
            }
        }
        HSD_GObjEvent source_helpers[] = { ftNs_AttackHi4_YoyoApplySmash,
                                           ftNs_AttackHi4_YoyoSetChargeDamage,
                                           ftNs_AttackHi4_YoyoSetUnkRate,
                                           ftNs_AttackHi4_YoyoUpdateHitPos,
                                           ftNs_AttackHi4_YoyoCheckTimedRehit,
                                           ftNs_AttackHi4_YoyoStartTimedRehit,
                                           ftNs_AttackHi4_YoyoCreateItem,
                                           ftNs_AttackHi4_YoyoItemDespawn,
                                           ftNs_AttackHi4_YoyoItemSetUnk,
                                           ftNs_AttackHi4_YoyoItemSetUnk2 };
        HSD_GObjEvent generated_helpers[] = {
            script_fn_YoyoApplySmash,      script_fn_YoyoSetChargeDamage,
            script_fn_YoyoSetChargeSpin,   script_fn_YoyoUpdateHitPos,
            script_fn_YoyoCheckTimedRehit, script_fn_YoyoStartTimedRehit,
            script_fn_YoyoCreate,          script_fn_YoyoRemove,
            script_fn_YoyoPause,           script_fn_YoyoResume
        };
        for (unsigned helper = 0; helper < 10; ++helper) {
            for (int variant = 0; variant < 64; ++variant) {
                case_motion = 342;
                case_phase = 20 + helper;
                case_variant = variant;
                seed(342, variant);
                if ((variant & 8) && helper != 6) {
                    fighters[0].u.ns.yoyo_gobj = NULL;
                    integer("yoyo_handle", -1);
                }
                side = 0;
                source_helpers[helper](&owners[0]);
                side = 1;
                generated_helpers[helper](&owners[1]);
                compare();
                ++helper_pairs;
            }
        }
        const u32 masks[] = { Collide_FloorMask, Collide_LeftWallMask,
                              Collide_RightWallMask, Collide_CeilingMask };
        for (unsigned flags = 0; flags < 16; ++flags) {
            for (unsigned second = 0; second < 4; ++second) {
                case_motion = 342;
                case_phase = 30;
                case_variant = flags * 4 + second;
                seed(342, case_variant);
                sweep_flags[0] = 0;
                for (unsigned bit = 0; bit < 4; ++bit) {
                    if (flags & (1u << bit)) {
                        sweep_flags[0] |= masks[bit];
                    }
                }
                sweep_flags[1] = second == 1   ? Collide_FloorPush
                                 : second == 2 ? Collide_LeftWallMask
                                 : second == 3 ? Collide_CeilingMask
                                               : 0;
                side = 0;
                sweep_calls = 0;
                bool a = ftNs_AttackHi4_YoyoCheckNoObstruct(&owners[0]);
                side = 1;
                sweep_calls = 0;
                bool b = script_fn_YoyoCheckNoObstruct(&owners[1]);
                check(a == b, "two-sweep result");
                compare();
                ++helper_pairs;
            }
        }
        for (unsigned motion = 0; motion < 2; ++motion) {
            for (unsigned selected = 0; selected < 8; ++selected) {
                for (int variant = 0; variant < 12; ++variant) {
                    case_motion = motion ? 345 : 342;
                    case_phase = 31;
                    case_variant = variant;
                    seed(case_motion, variant);
                    int frames[] = { 1,
                                     2,
                                     motion ? yoyo.down_smash_rotate_frame
                                            : yoyo.up_smash_rotate_frame,
                                     motion ? yoyo.down_smash_retract_frame
                                            : yoyo.up_smash_retract_frame,
                                     motion ? 60 : 49,
                                     3,
                                     motion ? 61 : 50,
                                     13 };
                    fighters[0].mv.ns.attackhi4.yoyoCurrentFrame =
                        frames[selected];
                    integer("yoyo_frame", frames[selected]);
                    fighters[0].cmd_vars[0] = fighters[1].cmd_vars[0] =
                        variant % 2;
                    fighters[0].cmd_vars[1] = fighters[1].cmd_vars[1] =
                        variant % 3;
                    side = 0;
                    bool a = ftNs_AttackHi4_YoyoThink_IsRemove(&owners[0]);
                    side = 1;
                    bool b = script_fn_YoyoThink(&owners[1]);
                    check(a == b, "article think result");
                    compare();
                    ++helper_pairs;
                }
            }
        }
        for (unsigned motion = 0; motion < 2; ++motion) {
            for (int variant = 0; variant < 64; ++variant) {
                case_motion = motion ? 346 : 343;
                case_phase = 32;
                case_variant = variant;
                seed(case_motion, variant);
                for (unsigned i = 0; i < 2; ++i) {
                    fighters[i].smash_attrs.x212C = variant % 5;
                    fighters[i].smash_attrs.x212D = 5;
                }
                side = 0;
                ftCo_800DEF38(&owners[0]);
                originals[motion ? 5 : 2][0](&owners[0]);
                side = 1;
                ftCo_800DEF38(&owners[1]);
                definition->motion_states[motion ? 5 : 2].anim_cb(&owners[1]);
                compare();
                ++common_pairs;
            }
        }
    }
    for (unsigned state = 0; state < 7; ++state) {
        for (unsigned phase = 0; phase < 4; ++phase) {
            check(coverage[state][phase] == 128, "all28 phase coverage");
        }
    }
    for (unsigned recycled = 0; recycled < 2; ++recycled) {
        for (unsigned operation = 0; operation < 5; ++operation) {
            case_motion = 342;
            case_phase = 40 + operation;
            case_variant = recycled;
            seed(342, 16);
            side = 1;
            recycle_source = recycled != 0;
            if (operation == 0) {
                invalidation = InvalidateTransition;
                script_fn_BatEnter(&owners[1]);
                check(counts[1] == 1 && events[1][0].kind == Change,
                      "entry exits before animation/construction after "
                      "invalidation");
            } else if (operation == 1 || operation == 2) {
                invalidation = InvalidateConstructor;
                integer(operation == 1 ? "bat_handle" : "yoyo_handle", -1);
                if (operation == 1) {
                    script_fn_BatCreate(&owners[1]);
                } else {
                    script_fn_YoyoCreate(&owners[1]);
                }
                check(counts[1] == 1 && events[1][0].kind == Create,
                      "constructor called once");
                check(get_int(operation == 1 ? "bat_handle" : "yoyo_handle") ==
                          -1,
                      "constructor result not written after invalidation");
                check(!fighters[1].death2_cb && !fighters[1].take_dmg_cb &&
                          !fighters[1].pre_hitlag_cb &&
                          !fighters[1].post_hitlag_cb,
                      "no callback writes after invalidation");
            } else if (operation == 3) {
                invalidation = InvalidateSignal;
                script_fn_BatRemove(&owners[1]);
                check(counts[1] == 1 && events[1][0].kind == Remove &&
                          get_int("bat_handle") == (int32_t) handle(Bat),
                      "remove signal exits before following state write");
            } else {
                invalidation = InvalidateSignal;
                fighters[1].smash_attrs.state = SmashState_Charging;
                script_fn_YoyoApplySmash(&owners[1]);
                check(get_int("yoyo_frame") == 0,
                      "pre-signal frame reset preserved");
                check(counts[1] == 1 && events[1][0].kind == Released &&
                          fighters[1].smash_attrs.state == SmashState_Charging,
                      "charge exits before common state/presentation writes");
            }
            check(!source_alive(&owners[1], UINT32_C(0xFEDCBA99),
                                NativeCallbackSource_Fighter),
                  "entry generation rejected");
            ++safety_cases;
        }
    }
    check(safety_cases == 10, "source invalidation coverage");
    run_cleanup_regression();
    printf("%u checks, %u failures, %u phase source pairs, %u entry pairs, %u "
           "helper pairs, %u common-update pairs\n",
           checks, failures, pairs, entry_pairs, helper_pairs, common_pairs);
    printf("Authored-only lifetime: %u removed/recycled source cases\n",
           safety_cases);
    printf("Cleanup: %u explicit latch deltas; %u regression cases\n",
           expected_cleanup_deltas, cleanup_cases);
    return failures ? 1 : 0;
}
