#include <melee/ft/kinds/ftCommon/ftCo_ItemThrow.h>
// Actual generated Purin callbacks versus unchanged baseline15 stock bodies.
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/types.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ft_0877.h>
#include <melee/ft/ft_0881.h>
#include <melee/ft/ft_0C8C.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/kinds/ftPurin/forward.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/controller.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)
static unsigned checks, stock_comparisons;
static Fighter fighter;
static HSD_GObj source;
static HSD_JObj primary, secondary;
static struct FighterBone parts[50];
static struct Fighter_x2D0_t jump_stats;
static ftCommonData common;
ftCommonData* p_ftCommonData = &common;
static NativeFighterParameters parameters;
static const NativeFighterDefinition* module;
static bool live, use_reference, finished, contact, wall, cliff;
static int interrupt_exit, interrupt_index, remove_interrupt, remove_transition;
static unsigned logs[96], log_count;
static float physics_values[3], landing_threshold;
void __assert(char* file, u32 line, char* message)
{ fprintf(stderr, "%s:%u: engine assertion: %s\n", file, line, message); exit(1); }
static void record(unsigned value) { CHECK(log_count < 96); logs[log_count++] = value; }
static void fixture_jump_entry(Fighter_GObj*);
static void reference_ftCo_800D74A4(Fighter_GObj*);
void ftCo_JumpAerial_Begin(Fighter_GObj*, FtMotionId, Vec3*, bool, bool, bool);
void ftCo_JumpAerial_IASA(Fighter_GObj*);
void ftCo_FallAerial_Enter(Fighter_GObj*);
void ftCo_800D74A4(Fighter_GObj*);
void ftCo_80098B20(Fighter_GObj*);

const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ return kind == 0x100 ? module : NULL; }
bool nativeCallbackSource_Alive(HSD_GObj* gobj, uint32_t id, uint32_t object_class)
{
    return live && gobj == &source && gobj->user_data == &fighter &&
        id == (uint32_t)fighter.x8_spawnNum && object_class == NativeCallbackSource_Fighter;
}
bool nativeCallbackSource_Snapshot(HSD_GObj* gobj, uint32_t object_class, uint32_t* id)
{
    if (gobj != &source || !nativeCallbackSource_Alive(gobj, fighter.x8_spawnNum, object_class)) return false;
    *id = fighter.x8_spawnNum; return true;
}
void HSD_JObjSetMtxDirtySub(HSD_JObj* joint) { joint->flags |= JOBJ_MTX_DIRTY; }
void Fighter_ChangeMotionState(Fighter_GObj* gobj, FtMotionId motion, MotionFlags flags,
                               float start, float speed, float blend, Fighter_GObj* other)
{
    CHECK(gobj == &source && start == 0 && speed == 1 && blend == 0 && !other);
    record((unsigned)motion); record((unsigned)flags);
    fighter.motion_id = motion; fighter.x2222_b3 = false;
    if (remove_transition) { live = false; fighter.x8_spawnNum++; }
}
void ft_80088770(Fighter* fp) { CHECK(fp == &fighter); record(80088770); }
void ft_80088328(Fighter* fp, int id, u8 volume, u8 pan)
{ CHECK(fp == &fighter && volume == 127 && pan == 64); record(80088328); record(id); }
void ft_PlaySFX(Fighter* fp, enum_t id, u8 volume, u8 pan)
{ CHECK(fp == &fighter && volume == 127 && pan == 64); record(808148); record(id); }
void ftCo_JumpAerial_Phys_Cb(Fighter_GObj* gobj) { (void)gobj; CHECK(0); }
void ftCommon_8007D5D4(Fighter* fp) { CHECK(fp == &fighter); record(0x8007D5D4); }
void ftAnim_8006EBA4(Fighter_GObj* gobj) { CHECK(gobj == &source); record(0x8006EBA4); }
void ftCommon_8007EBAC(Fighter* fp, u32 a, u32 b)
{ CHECK(fp == &fighter); record(a); record(b); }
void ftColl_8007B62C(Fighter_GObj* gobj, enum_t status)
{ CHECK(gobj == &source); record(status); }
static void ftCo_SpawnEf(Fighter_GObj* gobj, HSD_JObj* joint, u32 a, enum_t effect)
{ CHECK(gobj == &source && joint == &primary); record(a); record(effect); }
s32 ftCo_800D7268(Fighter* fp) { CHECK(fp == &fighter); return 0; }
bool ft_800D2D0C(Fighter_GObj* gobj) { CHECK(gobj == &source); return false; }
void ft_800D2E7C(Fighter_GObj* gobj, Vec3* velocity) { (void)gobj; (void)velocity; CHECK(0); }
bool ftAnim_IsFramesRemaining(Fighter_GObj* gobj) { CHECK(gobj == &source); return !finished; }
void ftCo_Fall_Enter(Fighter_GObj* gobj) { CHECK(gobj == &source); record(29); fighter.motion_id = 29; }
void ft_80084E1C(Fighter_GObj* gobj, float threshold, float acceleration, float speed)
{ CHECK(gobj == &source); physics_values[0] = threshold; physics_values[1] = acceleration; physics_values[2] = speed; record(84); }
bool ft_CheckGroundAndLedge(Fighter_GObj* gobj, int direction)
{ CHECK(gobj == &source && direction == ftGetFacingDirInt2(gobj)); record(81); return contact; }
float ftCo_800D0EC8(Fighter* fp) { CHECK(fp == &fighter); record(82); return landing_threshold; }
void ft_8008A2BC(Fighter_GObj* gobj) { CHECK(gobj == &source); record(83); }
void ftCo_Landing_Enter_Basic(Fighter_GObj* gobj) { CHECK(gobj == &source); record(85); }
bool ftWallJump_8008169C(Fighter_GObj* gobj) { CHECK(gobj == &source); record(86); return wall; }
bool ftCliffCommon_80081298(Fighter_GObj* gobj) { CHECK(gobj == &source); record(87); return cliff; }
static bool interrupt(Fighter_GObj* gobj)
{
    CHECK(gobj == &source); int index = interrupt_index++; record(100 + index);
    if (index == remove_interrupt) { live = false; fighter.x8_spawnNum++; }
    return index == interrupt_exit;
}
bool ftCo_SpecialAir_CheckInput(Fighter_GObj* gobj) { return interrupt(gobj); }
bool ftCo_80095328(Fighter_GObj* gobj, bool* unused, struct NativeItemLifetime* retained) { CHECK(!unused&&!retained); return interrupt(gobj); }
bool ftCo_800D7100(Fighter_GObj* gobj) { return interrupt(gobj); }
bool ftCo_800C3B10(Fighter_GObj* gobj) { return interrupt(gobj); }
bool ftCo_80099A58(Fighter_GObj* gobj) { return interrupt(gobj); }
bool ftCo_AttackAir_CheckItemThrowInput(Fighter_GObj* gobj) { return interrupt(gobj); }
bool ftCo_800D705C(Fighter_GObj* gobj) { return interrupt(gobj); }
bool ftPe_8011BA54(Fighter_GObj* gobj) { return interrupt(gobj); }
bool ftCo_800CB870(Fighter_GObj* gobj) { return interrupt(gobj); }
bool ftPe_8011BAD8(Fighter_GObj* gobj) { return interrupt(gobj); }
bool ftCo_800CEE70(Fighter_GObj* gobj) { return interrupt(gobj); }

#include "purin_jump_original.inc"
#include "purin_jump_current.inc"
static void stock_slot(HSD_GObj* gobj) { (void)gobj; CHECK(0); }
#include "purin_jump_base.inc"
#include "purin_jump_generated.inc"
static void unused_host(void) { CHECK(0); }
static float drift_threshold(void) { return 0.3f; }
static void reload_attributes(Fighter_GObj* gobj) { CHECK(gobj == &source); }
static void fixture_jump_entry(Fighter_GObj* gobj)
{ if (use_reference) reference_ftCo_800D74A4(gobj); else ftCo_800D74A4(gobj); }
static unsigned turn_offset(void)
{
    for (unsigned i = 0; i < module->state_field_count; ++i)
        if (!strcmp(module->state_fields[i].name, "multi_turn_remaining")) return module->state_fields[i].offset;
    CHECK(0); return 0;
}
static int turn(bool reference)
{
    int value;
    if (reference) return fighter.mv.co.jumpaerial.x0;
    memcpy(&value, fighter.native_module_state + turn_offset(), sizeof(value)); return value;
}
static void set_turn(bool reference, int value)
{
    if (reference) fighter.mv.co.jumpaerial.x0 = value;
    else memcpy(fighter.native_module_state + turn_offset(), &value, sizeof(value));
}
static void load_parameters(const char* path)
{
    char text[32768]; FILE* file = fopen(path, "rb"); CHECK(file);
    size_t length = fread(text, 1, sizeof(text), file); CHECK(feof(file) && !ferror(file)); fclose(file);
    unsigned error = 0; CHECK(nativeFighterParameters_Parse(text, length, &parameters, &error));
    uint32_t words[NATIVE_FIGHTER_PARAMETER_CAPACITY];
    for (unsigned i = 0; i < parameters.count; ++i) words[i] = parameters.fields[i].bits;
    CHECK(FreakMelee_SetFighterParameters(words, parameters.count, parameters.schema_hash));
}
static void edit(const char* name, uint32_t value)
{
    bool found = false; uint32_t words[NATIVE_FIGHTER_PARAMETER_CAPACITY];
    for (unsigned i = 0; i < parameters.count; ++i) {
        if (!strcmp(parameters.fields[i].name, name)) { parameters.fields[i].bits = value; found = true; }
        words[i] = parameters.fields[i].bits;
    }
    CHECK(found && FreakMelee_SetFighterParameters(words, parameters.count, parameters.schema_hash));
}
static uint32_t float_bits(float value) { uint32_t bits; memcpy(&bits, &value, sizeof(bits)); return bits; }
static void reset(bool reference, int used, float facing, float stick)
{
    static struct ftData data;
    static FtSFX sfx;
    static ftData_x8 bone_data;
    memset(&fighter, 0, sizeof(fighter)); memset(&primary, 0, sizeof(primary));
    memset(&secondary, 0, sizeof(secondary)); memset(&data, 0, sizeof(data));
    fighter.kind = 0x100; fighter.x8_spawnNum = UINT32_C(0x80000027);
    fighter.parts = parts; parts[0].joint = &primary; parts[0].x4_jobj2 = &secondary;
    source.user_data = &fighter; fighter.gobj = &source; live = true;
    fighter.dat_attrs = fighter.x2D0 = &jump_stats; fighter.can_multijump = true;
    fighter.ft_data = &data; data.x4C_sfx = &sfx; data.x8 = &bone_data;
    sfx.x14 = 909; bone_data.x11 = 0; common.x258 = .3f;
    module->on_load_special_attrs(&source);
    fighter.x1968_jumpsUsed = used; fighter.facing_dir = facing;
    fighter.motion_id = 341;
    fighter.input.lstick[0].x = stick; fighter.self_vel = (Vec3){ 3.5f, -2.5f, 1.25f };
    fighter.input.held_buttons[0] = HSD_PAD_X; fighter.input.pressed_buttons = HSD_PAD_X;
    fighter.x671_timer_lstick_tilt_y = 10; fighter.co_attrs.air_drift_stick_mul = .9f;
    fighter.co_attrs.air_drift_max = 1.5f; primary.rotate.y = .875f; secondary.rotate.y = -1.25f;
    fighter.kind = reference ? Ft_Kind_Purin : 0x100;
    use_reference = reference; finished = contact = wall = cliff = false;
    log_count = interrupt_index = 0; interrupt_exit = remove_interrupt = -1; remove_transition = 0;
    memset(physics_values, 0, sizeof(physics_values)); landing_threshold = -1.25f;
}
typedef struct Result {
    int motion, used, countdown;
    float facing, primary_rotation, secondary_rotation;
    Vec3 velocity;
    unsigned dirty, count, events[96];
    float physics[3];
    bool top_ko;
} Result;
static Result result(bool reference)
{
    Result r = { 0 }; r.motion = fighter.motion_id; r.used = fighter.x1968_jumpsUsed;
    // The original countdown shares a motion union with FallAerial. Its bytes
    // cease to describe a countdown after leaving a jump motion.
    if (fighter.motion_id >= 341 && fighter.motion_id <= 345)
        r.countdown = turn(reference);
    r.facing = fighter.facing_dir; r.velocity = fighter.self_vel;
    r.primary_rotation = primary.rotate.y; r.secondary_rotation = secondary.rotate.y;
    r.dirty = primary.flags; r.count = log_count; r.top_ko = fighter.x2222_b3;
    memcpy(r.events, logs, log_count * sizeof(logs[0])); memcpy(r.physics, physics_values, sizeof(r.physics)); return r;
}
static void same(Result expected, Result actual)
{ CHECK(!memcmp(&expected, &actual, sizeof(expected))); ++checks; ++stock_comparisons; }
static void removed_shield(Fighter_GObj* gobj)
{ CHECK(gobj == &source); fighter.x8_spawnNum++; live = false; }

static void reject_resource(const NativeFighterMultiJumpStats* stats)
{
    struct Fighter_x2D0_t before = jump_stats;
    int maximum = fighter.co_attrs.max_jumps;
    int used = fighter.x1968_jumpsUsed;
    CHECK(!nativeFighterJump_ConfigureMulti(&fighter, stats));
    CHECK(!memcmp(&before, &jump_stats, sizeof(before)));
    CHECK(fighter.co_attrs.max_jumps == maximum && fighter.x1968_jumpsUsed == used);
    ++checks;
}

static void resource_validation(void)
{
    reset(false, 3, 1, 0);
    NativeFighterMultiJumpStats valid = {
        .turn_frames = 11, .reverse_threshold = .4f, .horizontal_impulse = .6f,
        .acceleration_multiplier = .7f, .speed_multiplier = .9f,
        .vertical_impulses = { 2, 1.8f, 1.6f, 1.4f, 1.2f },
        .count = 5, .first_motion = 341, .helmet_first_motion = -1,
    };
    CHECK(nativeFighterJump_ConfigureMulti(&fighter, &valid));
    CHECK(fighter.co_attrs.max_jumps == 6 && fighter.x1968_jumpsUsed == 3);
    CHECK(nativeFighterJump_MultiVerticalSpeed(&fighter, 0) == 2);
    CHECK(nativeFighterJump_MultiVerticalSpeed(&fighter, 4) == 1.2f);
    CHECK(nativeFighterJump_MultiVerticalSpeed(&fighter, -1) == 0);
    CHECK(nativeFighterJump_MultiVerticalSpeed(&fighter, 5) == 0);
    ++checks;
    NativeFighterMultiJumpStats invalid;
    for (unsigned field = 0; field < 9; ++field) {
        for (unsigned nonfinite = 0; nonfinite < 3; ++nonfinite) {
            invalid = valid;
            float* values[] = { &invalid.reverse_threshold, &invalid.horizontal_impulse,
                &invalid.acceleration_multiplier, &invalid.speed_multiplier,
                &invalid.vertical_impulses[0], &invalid.vertical_impulses[1],
                &invalid.vertical_impulses[2], &invalid.vertical_impulses[3],
                &invalid.vertical_impulses[4] };
            *values[field] = nonfinite == 0 ? NAN : nonfinite == 1 ? INFINITY : -INFINITY;
            reject_resource(&invalid);
        }
    }
    invalid = valid; invalid.turn_frames = 0; reject_resource(&invalid);
    invalid = valid; invalid.count = 0; reject_resource(&invalid);
    invalid = valid; invalid.count = 6; reject_resource(&invalid);
    invalid = valid; invalid.first_motion = 340; reject_resource(&invalid);
    invalid = valid; invalid.first_motion = 369; reject_resource(&invalid);
    invalid = valid; invalid.helmet_first_motion = -2; reject_resource(&invalid);
    invalid = valid; invalid.helmet_first_motion = 340; reject_resource(&invalid);
    invalid = valid; invalid.helmet_first_motion = 369; reject_resource(&invalid);
    reject_resource(NULL);
    fighter.kind = Ft_Kind_Purin; reject_resource(&valid); fighter.kind = 0x100;
    fighter.can_multijump = false; reject_resource(&valid); fighter.can_multijump = true;
    fighter.x2D0 = NULL; reject_resource(&valid); fighter.x2D0 = &jump_stats;
    fighter.dat_attrs = NULL; reject_resource(&valid); fighter.dat_attrs = &jump_stats;
    // A declared invalid count also makes the scaled-resource read unavailable.
    jump_stats.x28 = 6;
    CHECK(nativeFighterJump_MultiVerticalSpeed(&fighter, 0) == 0);
    ++checks;
}

int main(int argc, char** argv)
{
    CHECK(argc == 2); load_parameters(argv[1]);
    NativeFighterHost host = { 0 };
#include "purin_jump_host.inc"
    host.callback_source_alive = nativeCallbackSource_Alive;
    host.begin_aerial_jump = ftCo_JumpAerial_Begin;
    host.add_joint_rotation_y = HSD_JObjAddRotationY;
    host.reload_ext_attributes = reload_attributes;
    host.multi_jump_vertical_speed = nativeFighterJump_MultiVerticalSpeed;
    host.configure_multi_jump = nativeFighterJump_ConfigureMulti;
    host.aerial_jump_actions = ftCo_JumpAerial_IASA;
    host.enter_fall_aerial = ftCo_FallAerial_Enter;
    host.enter_fall = ftCo_Fall_Enter;
    host.finish_ground_action = ft_8008A2BC;
    host.enter_landing_basic = ftCo_Landing_Enter_Basic;
    host.check_ground_and_ledge = ft_CheckGroundAndLedge;
    host.try_wall_jump = ftWallJump_8008169C;
    host.check_cliff_catch = ftCliffCommon_80081298;
    host.basic_landing_velocity_threshold = ftCo_800D0EC8;
    host.air_drift_threshold = drift_threshold;
    host.air_drift_with_limits = ft_80084E1C;
    host.animation_frames_remaining = ftAnim_IsFramesRemaining;
    module = FreakMelee_GetFighterModule(&ftPurinClone_Definition, &host); CHECK(module);
    CHECK(module->source_kind == Ft_Kind_None);
    CHECK(module->bone_count == 26 && module->joint_count == 50);
    CHECK(module->on_aerial_jump && module->on_shield_break);
    CHECK(module->on_item_pickup_ext && module->on_item_drop_ext);
    CHECK(module->motion_state_count == ftPr_MS_SelfCount);
    CHECK(module->demo_motion_names == ftPurinClone_Definition.demo_motion_names);
    CHECK(module->bones == ftPurinClone_Definition.bones);
    CHECK(module->sound_banks == ftPurinClone_Definition.sound_banks);
    for (unsigned row = 0; row < module->motion_state_count; ++row) {
        MotionState* now = &module->motion_states[row]; MotionState* old = &ftPr_Init_MotionStateTable[row];
        if (old->anim_cb) CHECK(now->anim_cb && now->anim_cb != old->anim_cb);
        if (old->input_cb) CHECK(now->input_cb && now->input_cb != old->input_cb);
        if (old->phys_cb) CHECK(now->phys_cb && now->phys_cb != old->phys_cb);
        if (old->coll_cb) CHECK(now->coll_cb && now->coll_cb != old->coll_cb);
        CHECK(now->cam_cb == old->cam_cb);
    }
    for (int frames = 11; frames <= 13; ++frames) {
        edit("multi_turn_frames", frames);
        for (int used = 1; used <= 5; ++used) for (int facing = -1; facing <= 1; facing += 2)
            for (int x = -4; x <= 4; ++x) for (int worn = 0; worn < 2; ++worn) {
                Result pair[2];
                for (int side = 0; side < 2; ++side) {
                    reset(side == 0, used, facing, x * .25f);
                    fighter.x197C = worn ? &source : NULL;
                    if (side == 0) reference_ftCo_800D74A4(&source); else ftCo_800D74A4(&source);
                    pair[side] = result(side == 0);
                }
                same(pair[0], pair[1]);
            }
        for (unsigned row = 0; row < 5; ++row) for (int counter = 0; counter <= frames; ++counter)
            for (int end = 0; end < 2; ++end) for (int used = 5; used <= 6; ++used) {
                Result pair[2];
                for (int side = 0; side < 2; ++side) {
                    reset(side == 0, used, -1, .4f); set_turn(side == 0, counter); finished = end;
                    if (side == 0) reference_ftCo_JumpAerialF1_Anim(&source);
                    else module->motion_states[row].anim_cb(&source);
                    pair[side] = result(side == 0);
                }
                same(pair[0], pair[1]);
            }
    }
    edit("multi_turn_frames", 12);
    for (unsigned row = 0; row < 5; ++row) {
        for (int exit = -1; exit < 11; ++exit) for (int command = 0; command < 2; ++command) {
            Result pair[2];
            for (int side = 0; side < 2; ++side) {
                reset(side == 0, 2, 1, .4f); fighter.cmd_vars[0] = command; interrupt_exit = exit;
                if (side == 0) reference_ftCo_JumpAerialF1_IASA(&source); else module->motion_states[row].input_cb(&source);
                pair[side] = result(side == 0);
            }
            same(pair[0], pair[1]);
        }
        Result old, now;
        reset(true, 2, 1, .4f); reference_ftCo_JumpAerialF1_Phys(&source); old = result(true);
        reset(false, 2, 1, .4f); module->motion_states[row].phys_cb(&source); now = result(false); same(old, now);
        for (int touch = 0; touch < 2; ++touch) for (int walls = 0; walls < 2; ++walls)
            for (int cliffs = 0; cliffs < 2; ++cliffs) for (int height = -1; height <= 1; ++height) {
                Result pair[2];
                for (int side = 0; side < 2; ++side) {
                    reset(side == 0, 2, -1, .4f); contact = touch; wall = walls; cliff = cliffs;
                    fighter.self_vel.y = landing_threshold + height;
                    if (side == 0) reference_ftCo_JumpAerialF1_Coll(&source); else module->motion_states[row].coll_cb(&source);
                    pair[side] = result(side == 0);
                }
                same(pair[0], pair[1]);
            }
    }
    for (int used = 1; used <= 6; ++used) for (int locked = 0; locked < 2; ++locked) {
        Result pair[2]; int consumed[2];
        for (int side = 0; side < 2; ++side) {
            reset(side == 0, used, -1, .5f); fighter.motion_id = 342; fighter.cmd_vars[0] = locked;
            consumed[side] = ftCo_800D730C(&source, false); pair[side] = result(side == 0);
        }
        CHECK(consumed[0] == consumed[1]); same(pair[0], pair[1]);
    }
    Result pair[2];
    for (int side = 0; side < 2; ++side) {
        reset(side == 0, 1, 1, 0); fighter.co_attrs.shield_break_initial_velocity = 4.25f;
        if (side == 0) reference_ftCo_80098B20(&source); else ftCo_80098B20(&source);
        pair[side] = result(side == 0);
    }
    same(pair[0], pair[1]); CHECK(pair[1].top_ko);
    // Surviving stock decisions are compared above; removed/recycled entities
    // must stop before writes, and are deliberately not compared to stock UB.
    for (int index = 0; index < 11; ++index) {
        reset(false, 2, 1, 0); fighter.cmd_vars[0] = 1; remove_interrupt = index;
        ftCo_JumpAerial_IASA(&source); CHECK(!live && interrupt_index == index + 1); ++checks;
    }
    reset(false, 2, 1, 0); remove_transition = 1;
    Vec3 velocity = { 7, 8, 9 }; ftCo_JumpAerial_Begin(&source, 341, &velocity, false, false, true);
    CHECK(!live && fighter.x1968_jumpsUsed == 2 && fighter.self_vel.x == 3.5f); ++checks;
    reset(false, 2, 1, 0); remove_transition = 1; fighter.mv.co.fallaerial.smid = 777;
    ftCo_FallAerial_Enter(&source); CHECK(!live && fighter.mv.co.fallaerial.smid == 777); ++checks;
    reset(false, 2, 1, 0); NativeFighterDefinition removed = *module; removed.on_shield_break = removed_shield;
    const NativeFighterDefinition* saved = module; module = &removed; ftCo_80098B20(&source); module = saved;
    CHECK(!live && log_count == 4); ++checks;
    // Editable FMP values feed the real resource; shared input observes the
    // changed maximum, and the generated entry reads the changed scaled row.
    edit("multi_count", 3); edit("multi_vertical_1", float_bits(2.5f));
    reset(false, 1, 1, .5f); CHECK(jump_stats.x28 == 3 && fighter.co_attrs.max_jumps == 4);
    CHECK(ftCo_800D730C(&source, false)); CHECK(fighter.self_vel.y == 2.5f); ++checks;
    reset(false, 4, 1, .5f); CHECK(!ftCo_800D730C(&source, false)); ++checks;
    resource_validation();
    printf("Purin jump/shield checks passed: %u (%u stock comparisons + %u lifetime/resource cases); 20 owned jump phases\n",
           checks, stock_comparisons, checks - stock_comparisons);
    return 0;
}
