// Actual generated callbacks against unchanged Oil Panic source bodies.
// Food allocation/presentation and shared motion/animation remain controlled
// engine boundaries. The food constructor body is tested in the separate
// root-owned engine fixture; this test verifies its generated call/lifetime.
#include "behavior.generated.h"
#include <melee/ft/fighter.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/inlines.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/kinds/ftCommon/inlines.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_target.h>
#include <melee/ft/kinds/ftGameWatch/forward.h>
#include <melee/ft/kinds/ftGameWatch/types.h>
#include <melee/it/item.h>
#include <melee/it/inlines.h>
#include <melee/it/kinds/itgamewatchpanic.h>
#include <sysdolphin/baselib/gobjplink.h>
#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static Fighter fighters[2];
static HSD_GObj owners[2], item_object, food_object;
static Item item;
static HSD_GObj* heads[32];
HSD_GObj** HSD_GObjPLinkHead = heads;
static struct ftCommonData common;
struct ftCommonData* p_ftCommonData = &common;
static ftGameWatchAttributes tuning;
static MotionState motions[1];
static const NativeFighterDefinition* definition;
static unsigned family, checks, pairs, absorb_pairs, turn_pairs, pickup_pairs;
static unsigned spawn_calls, invalidation;
static bool food_success;
static Vec3 supplied_position;

typedef struct ContractEvent { int code, a, b; float x, y; } ContractEvent;
static ContractEvent events[2][16];
static unsigned event_count[2];
static void check_at(bool value, unsigned line)
{ ++checks; if (!value) fprintf(stderr, "Contract assertion at line%u family%u pair%u check%u\n", line, family, pairs, checks); assert(value); }
#define check(value) check_at((value), __LINE__)
static void event(int code, int a, int b, float x, float y)
{
    check(event_count[family] < 16);
    events[family][event_count[family]++] = (ContractEvent){code, a, b, x, y};
}
static unsigned offset(const char* name)
{
    for (unsigned i = 0; i < definition->state_field_count; ++i)
        if (!strcmp(definition->state_fields[i].name, name)) return definition->state_fields[i].offset;
    assert(0); return 0;
}
static void state_int(const char* name, int32_t value)
{ memcpy(fighters[1].native_module_state + script_state_base + offset(name), &value, 4); }
static int32_t read_int(const char* name)
{ int32_t value; memcpy(&value, fighters[1].native_module_state + script_state_base + offset(name), 4); return value; }
static bool read_bool(const char* name)
{ return fighters[1].native_module_state[script_state_base + offset(name)] != 0; }
static float read_float(const char* name)
{ float value; memcpy(&value, fighters[1].native_module_state + script_state_base + offset(name), 4); return value; }

void Fighter_ChangeMotionState(Fighter_GObj* gobj, FtMotionId motion,
    MotionFlags flags, float frame, float rate, float blend, Fighter_GObj* donor)
{
    check(gobj == &owners[family] && flags == 0 && frame == 0 && rate == 1 && blend == 0 && !donor);
    fighters[family].motion_id = motion; event('M', motion, 0, 0, 0);
}
bool ftCo_800BFFD0(Fighter* fighter, enum_t id, bool arg)
{ check(fighter == &fighters[family] && !arg); event('C', id, 0, 0, 0); return true; }
void ftParts_80074B0C(Fighter_GObj* gobj, int group, int variant)
{ check(gobj == &owners[family]); event('V', group, variant, 0, 0); }
void ftCommon_UpdateFacing(Fighter* fighter)
{
    check(fighter == &fighters[family]); event('F', 0, 0, fighter->input.lstick[0].x, 0);
    if (fighter->input.lstick[0].x != 0) fighter->facing_dir = fighter->input.lstick[0].x > 0 ? 1 : -1;
}
s32 ftLib_800865CC(Fighter_GObj* owner)
{ check(owner == &owners[family]); return fighters[family].ground_or_air; }
void Item_80268E5C(Item_GObj* gobj, enum_t motion, Item_StateChangeFlags flags)
{ check(gobj == &item_object); item.msid = motion; event('I', motion, flags, 0, 0); }
void Item_802694CC(Item_GObj* gobj)
{ check(gobj == &item_object); event('A', 0, 0, 0, 0); }
static float common_attribute(NativeFighterCommonAttribute id)
{ check(id == NativeFighterCommon_HorizontalStickDeadzone); return common.horizontal_stick_deadzone; }
static HSD_GObj* spawn_food(Item_GObj* caller, Vec3* position)
{
    check(caller == &owners[1]); supplied_position = *position; ++spawn_calls;
    if (invalidation == 1) heads[HSD_GOBJ_PLINK_FIGHTER] = NULL;
    if (invalidation == 2) ++fighters[1].x8_spawnNum;
    if (invalidation == 3) { heads[HSD_GOBJ_PLINK_FIGHTER] = NULL; owners[1].user_data = (void*)1; }
    return food_success ? &food_object : NULL;
}
// Unused presentation dependencies of the actual target translation unit.
// No callback in this fixture queries camera, shield or effective model scale.
void ftLib_800866DC(Fighter_GObj* gobj, Vec3* position) { (void)gobj; (void)position; assert(0); }
bool ftLib_80086A18(Fighter_GObj* gobj) { (void)gobj; assert(0); return false; }
float ftLib_800869D4(Fighter_GObj* gobj) { (void)gobj; assert(0); return 0; }
float ftCo_80094098(Fighter_GObj* gobj, float* x) { (void)gobj; (void)x; assert(0); return 0; }

// Original bodies are copied mechanically, not rewritten expected behavior.
#include "original.h"

static void reset(void)
{
    memset(fighters, 0, sizeof(fighters)); memset(owners, 0, sizeof(owners));
    memset(events, 0, sizeof(events)); memset(event_count, 0, sizeof(event_count));
    memset(heads, 0, sizeof(heads)); memset(&item, 0, sizeof(item));
    item_object.user_data = &item; item.entity = &item_object; item.x1C = UINT32_C(0x80000005);
    heads[HSD_GOBJ_PLINK_ITEM] = &item_object;
    for (unsigned i = 0; i < 2; ++i) {
        owners[i].user_data = &fighters[i]; fighters[i].gobj = &owners[i];
        fighters[i].x8_spawnNum = (int32_t)UINT32_C(0xFEDCBA98);
        fighters[i].dat_attrs = &tuning; fighters[i].facing_dir = 1;
        fighters[i].x5AC.model_num = 11;
    }
    invalidation = spawn_calls = 0;
}
static void select(unsigned which)
{ family = which; heads[HSD_GOBJ_PLINK_FIGHTER] = &owners[which]; }
static void equal_events(void)
{
    check(event_count[0] == event_count[1]);
    for (unsigned i = 0; i < event_count[0]; ++i) {
        ContractEvent* a = &events[0][i]; ContractEvent* b = &events[1][i];
        check(a->code == b->code && a->a == b->a && a->b == b->b && a->x == b->x && a->y == b->y);
    }
    ++pairs;
}
static void absorption(void)
{
    const int damage[] = {0, 1, 31, 16777217, 16777219};
    for (unsigned air = 0; air < 2; ++air) for (int charge = 0; charge < 5; ++charge)
    for (int hits = 0; hits < 5; ++hits) for (unsigned d = 0; d < 5; ++d) {
        reset(); state_int("panic_charge", charge); state_int("panic_damage", 7);
        for (unsigned i = 0; i < 2; ++i) {
            fighters[i].u.gw.x2238_panicCharge = charge; fighters[i].u.gw.x223C_panicDamage = 7;
            fighters[i].AbsorbAttr.x1A48_hitsTaken = hits;
            fighters[i].AbsorbAttr.x1A44_damageTaken = damage[d];
            fighters[i].ground_or_air = air ? GA_Air : GA_Ground;
        }
        select(0); ftGw_SpecialLw_AbsorbThink_DecideAction(&owners[0]);
        select(1); definition->on_absorb(&owners[1]); equal_events();
        check(read_int("panic_charge") == fighters[0].u.gw.x2238_panicCharge);
        check(read_int("panic_damage") == fighters[0].u.gw.x223C_panicDamage);
        check(fighters[0].motion_id == fighters[1].motion_id); ++absorb_pairs;
    }
}
static void turning(void)
{
    const float deadzones[] = {0, .3f, .65f, 1};
    for (unsigned dz = 0; dz < 4; ++dz) for (unsigned direction = 0; direction < 2; ++direction)
    for (int turn = -1; turn < 3; ++turn) for (unsigned held = 0; held < 2; ++held)
    for (int side = -3; side <= 3; ++side) {
        reset(); common.horizontal_stick_deadzone = deadzones[dz];
        tuning.x7C_GAMEWATCH_PANIC_TURN_FRAMES = 3.5f;
        float stick = side == -1 ? -deadzones[dz] : side == 1 ? deadzones[dz] : .4f * side;
        for (unsigned i = 0; i < 2; ++i) {
            fighters[i].facing_dir = direction ? -1 : 1;
            fighters[i].input.lstick[0].x = stick;
            fighters[i].input.held_buttons[0] = held ? HSD_PAD_B : 0;
            fighters[i].mv.gw.SpecialLw.turnFrames = turn;
        }
        state_int("panic_turn", turn);
        select(0); ftGw_SpecialLw_IASA(&owners[0]);
        select(1); definition->motion_states[0].input_cb(&owners[1]); equal_events();
        check(fighters[0].facing_dir == fighters[1].facing_dir);
        check(fighters[0].mv.gw.SpecialLw.turnFrames == read_int("panic_turn"));
        check(fighters[0].mv.gw.SpecialLw.isRelease == read_bool("panic_release")); ++turn_pairs;
    }
}
static void pickup(void)
{
    for (unsigned owner = 0; owner < 3; ++owner) for (unsigned air = 0; air < 2; ++air) {
        reset(); fighters[0].ground_or_air = fighters[1].ground_or_air = air ? GA_Air : GA_Ground;
        for (unsigned f = 0; f < 2; ++f) {
            select(f); memset(event_count + f, 0, sizeof(event_count[f]));
            item.xDAC_itcmd_var0 = 9; item.xDB0_itcmd_var1 = 8; item.msid = 7; item.ground_or_air = GA_Air;
            item.owner = owner == 0 ? NULL : &owners[f];
            if (owner == 2 && f == 0) continue; // Original dereferences stale owners; excluded undefined input.
            if (owner == 2) { heads[HSD_GOBJ_PLINK_FIGHTER] = NULL; owners[f].user_data = (void*)1; }
            if (f == 0) itGameWatchPanic_Logic78_PickedUp(&item_object);
            else definition->article_logics[0].logic->picked_up(&item_object);
            check(item.xDAC_itcmd_var0 == 0 && item.xDB0_itcmd_var1 == 0);
            if (owner == 1) check(item.ground_or_air == (air ? GA_Air : GA_Ground) && item.msid == (int)air);
            else check(item.ground_or_air == GA_Air && item.msid == 7 && event_count[f] == 0);
        }
        if (owner != 2) { equal_events(); ++pickup_pairs; }
    }
}
static void food_boundary(void)
{
    const int hits[] = {0, 1, 16777217, INT32_MAX, INT32_MIN};
    for (unsigned h = 0; h < 5; ++h) for (unsigned success = 0; success < 2; ++success)
    for (unsigned retired = 0; retired < 4; ++retired) {
        reset(); select(1); food_success = success; invalidation = retired;
        fighters[1].AbsorbAttr.x1A48_hitsTaken = hits[h]; fighters[1].ground_or_air = GA_Air;
        common.horizontal_stick_deadzone = .375f;
        state_int("continued", 99);
        definition->on_load(&owners[1]);
        check(spawn_calls == 1 && supplied_position.x == 1.25f && supplied_position.y == 2.25f && supplied_position.z == 3.25f);
        check(read_int("hits") == hits[h] && read_float("deadzone") == .375f && read_bool("target_air"));
        check(read_int("coordinate_order") == 123);
        check(read_int("continued") == (retired ? 99 : 1));
        check(read_bool("spawned") == (retired ? false : success));
    }
}
static void typed_queries(void)
{
    const uint32_t ids[] = {UINT32_C(16777217), UINT32_C(2147483649), UINT32_MAX, UINT32_C(0xFEDCBA98)};
    const NativeFighterArticleLogic* logic = &definition->article_logics[0];
    check(logic->state_field_count == 1 && logic->state_fields[0].type == NativeFighterState_Int);
    for (unsigned i = 0; i < 4; ++i) for (unsigned mode = 0; mode < 4; ++mode) {
        reset(); select(1); fighters[1].x8_spawnNum = (int32_t)ids[i];
        fighters[1].ground_or_air = mode ? GA_Air : GA_Ground;
        memcpy(item.xDD4_itemVar.native_article.state + logic->state_fields[0].offset, &ids[i], 4);
        if (mode == 2) { heads[HSD_GOBJ_PLINK_FIGHTER] = NULL; owners[1].user_data = (void*)1; }
        if (mode == 3) ++fighters[1].x8_spawnNum; // Same address, new creation ID.
        check(script_fn_Observe(&owners[0], (int32_t)ids[i]) == (mode == 1));
        logic->logic->states[0].physics_updated(&item_object);
        check((item.ground_or_air == GA_Air) == (mode == 1));
    }
}
int main(void)
{
    NativeFighterHost host = { .callback_source_alive = nativeCallbackSource_Alive,
        .common_attribute = common_attribute, .spawn_food = spawn_food,
        .target_airborne = nativeFighterTarget_Airborne,
        .target_is_fighter = nativeFighterTarget_IsFighter,
        .item_current_owner = nativeFighterTarget_CurrentOwner,
        .change_motion_state = Fighter_ChangeMotionState, .start_color = ftCo_800BFFD0,
        .set_visibility_group = ftParts_80074B0C, .update_facing = ftCommon_UpdateFacing,
        .abs_float = fabsf, .item_change_state_flags = Item_80268E5C,
        .item_advance_animation = Item_802694CC };
    NativeFighterDefinition base = {.fighter_root = "ftDataGamewatch", .motion_state_first = 375,
        .motion_state_count = 1, .motion_states = motions};
    NativeFighterHost missing = host; missing.spawn_food = NULL;
    check(!FreakMelee_GetFighterModule(&base, &missing));
    missing = host; missing.target_airborne = NULL; check(!FreakMelee_GetFighterModule(&base, &missing));
    missing = host; missing.common_attribute = NULL; check(!FreakMelee_GetFighterModule(&base, &missing));
    missing = host; missing.callback_source_alive = NULL; check(!FreakMelee_GetFighterModule(&base, &missing));
    definition = FreakMelee_GetFighterModule(&base, &host); check(definition && definition->on_absorb && definition->on_load);
    absorption(); turning(); pickup(); food_boundary(); typed_queries();
    printf("Contracts source: %u checks, %u pairs; absorb%u turn%u pickup%u food40\n",
        checks, pairs, absorb_pairs, turn_pairs, pickup_pairs);
    return 0;
}
