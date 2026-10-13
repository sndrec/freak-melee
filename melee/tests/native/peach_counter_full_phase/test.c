/* Actual original files + actual final generated module. Controlled boundaries
 * are motion animation/probes, item allocation/AB54, effects/model rendering.
 * Shared gravity/deceleration/guard/item scale/timer/signal/recipient/identity
 * bodies run directly. Original invalid pointers are not parity references. */
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <melee/db/db.h>
#include <melee/ef/eflib.h>
#include <melee/ef/efsync.h>
#include <melee/ef/native_model_effect.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcamera.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/ft/kinds/ftCommon/inlines.h>
#include <melee/ft/kinds/ftKirby/ftkirbyspecialpeach.h>
#include <melee/ft/kinds/ftPeach/ftpeach.h>
#include <melee/ft/kinds/ftPeach/ftpeachspecialn.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_events.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_target.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/it/inlines.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_2725.h>
#include <melee/it/itCharItems.h>
#include <melee/it/itcoll.h>
#include <melee/it/kinds/inlines.h>
#include <melee/it/kinds/itpeachtoad.h>
#include <melee/it/kinds/itpeachtoadspore.h>
#include <melee/it/native_article.h>
#include <melee/it/native_article_refs.h>
#include <melee/it/native_item_owner.h>
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/random.h>

typedef bool (*CounterPredicate)(Item_GObj*);
static unsigned pairs, checks, safety;
#define CHECK(x)                                                              \
    do {                                                                      \
        ++checks;                                                             \
        if (!(x)) {                                                           \
            fprintf(stderr, "pair%u safety%u line%d %s\n", pairs, safety,     \
                    __LINE__, #x);                                            \
            exit(1);                                                          \
        }                                                                     \
    } while (0)
// C-reference actors have no interpreted slots: native guard construction may
// clear the source slot, but source helper registration is unavailable here.
bool nativeScriptFighter_BindCallback(Fighter_GObj* g,uint32_t slot,uint32_t function)
{
    CHECK(g && slot == NativeScriptFighterCallback_ShieldHit && function == UINT32_MAX);
    return false;
}

typedef struct CounterEvent {
    int op, a, b;
    float x, y, z, w;
} CounterEvent;
enum {
    Motion,
    Start,
    Finish,
    Fall,
    GroundPhysics,
    GroundProbe,
    AirProbe,
    ToAir,
    ToGround,
    Position,
    Allocate,
    Attach,
    ItemState,
    Advance,
    Hide,
    Show,
    Scale,
    Geometry,
    Rate,
    AdvanceModel,
    Debug,
    Random,
    Effect,
    DestroyEffects,
    Reflect,
    ShieldBounce,
    Interact,
    Free,
    Pause,
    Resume
};
static CounterEvent events[128], expected[128];
static unsigned count, expected_count;
static void record(int op, int a, int b, float x, float y, float z, float w)
{
    CHECK(count < 128);
    events[count++] = (CounterEvent){ op, a, b, x, y, z, w };
}
static void save(void)
{
    expected_count = count;
    memcpy(expected, events, sizeof events);
}
static void compare(void)
{
    if (count != expected_count ||
        memcmp(events, expected, count * sizeof(*events)))
    {
        for (unsigned i = 0; i < count || i < expected_count; i++) {
            fprintf(stderr, "%u old%d/%d/%g new%d/%d/%g\n", i, expected[i].op,
                    expected[i].a, expected[i].x, events[i].op, events[i].a,
                    events[i].x);
        }
    }
    CHECK(count == expected_count &&
          !memcmp(events, expected, count * sizeof(*events)));
    ++pairs;
}
static Fighter fighter, foreign;
static HSD_GObj source, other, object, *heads[32];
HSD_GObj** HSD_GObjPLinkHead = heads;
static Item item;
static HSD_JObj root, joint;
static FighterBone bones[MAX_FT_PARTS];
static FighterPartsTable rig, *rigs[Ft_Kind_RuntimeCapacity];
FighterPartsTable** ftPartsTable = rigs;
static ftPe_DatAttrs attrs;
static Article article_asset;
static itPeachToadSporeAttributes spore_attrs;
static ItemCommonData common;
ItemCommonData* it_804D6D28 = &common;
static const NativeFighterDefinition* definition;
static const NativeFighterArticleLogic *toad, *spore;
static bool scripted, allocation_failure, frames_remaining, ground_hit,
    air_hit, bounce_result;
static int invalidate_item, invalidate_fighter;
static unsigned float_cursor;
static float float_draws[2];
static SpawnItem observed;
static const uint32_t handle = UINT32_C(0xfedcba98);
const NativeFighterDefinition*
nativeFighterRuntime_Definition(FighterKind kind)
{
    return kind == Ft_Kind_CustomFirst || kind == Ft_Kind_CustomFirst + 1
               ? definition
               : NULL;
}
bool nativeModelEffect_Snapshot(HSD_GObj* g, uint32_t* id)
{
    (void) g;
    *id = 0;
    return false;
}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* g, uint32_t id)
{
    (void) g;
    (void) id;
    return NULL;
}
static void unexpected_boundary(void)
{
    fprintf(stderr, "Uncontrolled boundary\n");
    exit(1);
}
void __assert(char* f, u32 l, char* s)
{
    fprintf(stderr, "%s:%lu %s", f, l, s);
    abort();
}
HSD_GObj* ftLib_8008741C(u32 id)
{
    for (HSD_GObj* g = heads[HSD_GOBJ_PLINK_FIGHTER]; g; g = g->next) {
        if (((Fighter*) g->user_data)->x8_spawnNum == id) {
            return g;
        }
    }
    return NULL;
}
uint32_t nativeFighterTarget_Id(HSD_GObj* g)
{
    return g ? ((Fighter*) g->user_data)->x8_spawnNum : 0;
}
static uint32_t current_owner(Item_GObj* g)
{
    HSD_GObj* owner = ((Item*) g->user_data)->owner;
    uint32_t id;
    return nativeCallbackSource_Snapshot(owner, NativeCallbackSource_Fighter,
                                         &id)
               ? id
               : 0;
}
static uint32_t name_hash(const char* text)
{
    uint32_t h = 2166136261u;
    for (; *text; text++) {
        h = (h ^ (unsigned char) *text) * 16777619u;
    }
    return h;
}
static int32_t target_motion(uint32_t id)
{
    HSD_GObj* g = ftLib_8008741C(id);
    return g ? ((Fighter*) g->user_data)->motion_id : -1;
}
static void mutate(void)
{
    if (invalidate_item == 1) {
        heads[HSD_GOBJ_PLINK_ITEM] = NULL;
    }
    if (invalidate_item == 2) {
        item.x1C++;
    }
    if (invalidate_fighter == 1) {
        heads[HSD_GOBJ_PLINK_FIGHTER] = &other;
    }
    if (invalidate_fighter == 2) {
        fighter.x8_spawnNum++;
    }
}
float HSD_Randf(void)
{
    CHECK(float_cursor < 2);
    float v = float_draws[float_cursor++];
    record(Random, float_cursor, 0, v, 0, 0, 0);
    return v;
}
void Fighter_ChangeMotionState(Fighter_GObj* g, FtMotionId m,
                               MotionFlags flags, float start, float rate,
                               float blend, Fighter_GObj* donor)
{
    CHECK(g == &source && !blend && !donor);
    record(Motion, m, flags, start, rate, 0, 0);
    fighter.motion_id = m;
    fighter.cur_anim_frame = start;
    mutate();
}
void ftAnim_8006EBA4(Fighter_GObj* g)
{
    CHECK(g == &source);
    record(Start, 0, 0, 0, 0, 0, 0);
}
bool ftAnim_IsFramesRemaining(Fighter_GObj* g)
{
    CHECK(g == &source);
    return frames_remaining;
}
void ft_8008A2BC(Fighter_GObj* g)
{
    CHECK(g == &source);
    record(Finish, 0, 0, 0, 0, 0, 0);
    fighter.motion_id = 14;
}
void ftCo_Fall_Enter(Fighter_GObj* g)
{
    CHECK(g == &source);
    record(Fall, 0, 0, 0, 0, 0, 0);
    fighter.motion_id = 29;
}
void ft_80084F3C(Fighter_GObj* g)
{
    CHECK(g == &source);
    record(GroundPhysics, 0, 0, 0, 0, 0, 0);
    fighter.self_vel.x *= .75f;
}
bool ft_800827A0(Fighter_GObj* g)
{
    CHECK(g == &source);
    record(GroundProbe, 0, 0, 0, 0, 0, 0);
    return ground_hit;
}
GroundOrAir ft_80081D0C(Fighter_GObj* g)
{
    CHECK(g == &source);
    record(AirProbe, 0, 0, 0, 0, 0, 0);
    return air_hit;
}
void ftCommon_8007D5D4(Fighter* f)
{
    CHECK(f == &fighter);
    record(ToAir, 0, 0, 0, 0, 0, 0);
    f->ground_or_air = GA_Air;
}
void ftCommon_8007D7FC(Fighter* f)
{
    CHECK(f == &fighter);
    record(ToGround, 0, 0, 0, 0, 0, 0);
    f->ground_or_air = GA_Ground;
}
void lb_8000B1CC(HSD_JObj* j, Vec3* offset, Vec3* pos)
{
    CHECK(j == &joint && !offset);
    *pos = (Vec3){ 11, 22, 33 };
    record(Position, 109, 0, pos->x, pos->y, pos->z, 0);
}
static unsigned bone_count(Fighter* f)
{
    CHECK(f == &fighter);
    return rig.parts_num;
}
static HSD_GObj* allocate(SpawnItem* s)
{
    observed = *s;
    record(Allocate, s->kind, s->x44_flag.b0, s->pos.x, s->pos.y, s->pos.z,
           s->facing_dir);
    CHECK(s->x0_parent_gobj == &source && s->x4_parent_gobj2 == &source &&
          !s->x40 && !s->x3C_damage);
    CHECK(!memcmp(&s->pos, &s->prev_pos, sizeof(Vec3)));
    CHECK(s->vel.x == 0 && s->vel.y == 0 && s->vel.z == 0);
    if (allocation_failure) {
        return NULL;
    }
    heads[HSD_GOBJ_PLINK_ITEM] = &object;
    item.x1C = handle;
    item.owner = &source;
    item.pos = s->pos;
    item.facing_dir = s->facing_dir;
    item.x40_vel = s->vel;
    item.kind = s->kind;
    item.xB8_itemLogicTable =
        scripted ? (s->kind == It_Kind_Peach_Toad ? toad->logic : spore->logic)
                 : NULL;
    nativeItem_CaptureCreator(&object, &source);
    return &object;
}
Item_GObj* Item_80268B18(SpawnItem* s)
{
    return allocate(s);
}
Item_GObj* Item_CreateEngine(SpawnItem* s)
{
    (void) s;
    CHECK(false);
    return NULL;
}
Item_GObj* Item_CreateAuthored(SpawnItem* s, FighterKind author, uint32_t source_owner)
{
    CHECK(source_owner == 0);
    Item_GObj* g = allocate(s);
    if (g) {
        item.native_article_author_kind = author;
    }
    return g;
}
Article* nativeFighterRuntime_ArticleAssets(FighterKind author, ItemKind kind)
{
    CHECK(author == fighter.kind &&
          (kind == It_Kind_Peach_Toad || kind == It_Kind_Peach_ToadSpore));
    return &article_asset;
}
void it_8026BB68(Fighter_GObj* g, Vec3* p)
{
    (void) g;
    (void) p;
    CHECK(false);
}
void Item_8026AB54(Item_GObj* g, HSD_GObj* owner, Fighter_Part part)
{
    CHECK(g == &object && owner == &source && part == 109);
    record(Attach, part, 0, 0, 0, 0, 0);
    item.owner = owner;
    item.xDC4 = part;
    if (scripted) {
        toad->logic->picked_up(g);
    } else {
        itPeachToad_Logic91_PickedUp(g);
    }
}
void db_80225DD8(Item_GObj* g, Fighter_GObj* owner)
{
    CHECK(g == &object && owner == &source);
    record(Debug, 0, 0, 0, 0, 0, 0);
}
void Item_80268E5C(Item_GObj* g, enum_t state, Item_StateChangeFlags flags)
{
    CHECK(g == &object);
    record(ItemState, state, flags, item.xD44_lifeTimer, 0, 0, 0);
    item.msid = state;
    mutate();
}
static void item_change(Item_GObj* g, int state)
{
    Item_80268E5C(g, state, ITEM_ANIM_UPDATE);
}
void Item_802694CC(Item_GObj* g)
{
    CHECK(g == &object);
    record(Advance, 0, 0, 0, 0, 0, 0);
}
void lb_8000BA0C(HSD_JObj* j, float rate)
{
    CHECK(j == &root);
    record(Rate, 0, 0, rate, 0, 0, 0);
}
void HSD_JObjAnimAll(HSD_JObj* j)
{
    CHECK(j == &root);
    record(AdvanceModel, 0, 0, 0, 0, 0, 0);
}
void HSD_JObjClearFlagsAll(HSD_JObj* j, u32 flags)
{
    CHECK(j == &root && flags == JOBJ_HIDDEN);
    record(Show, 0, 0, 0, 0, 0, 0);
    j->flags &= ~flags;
}
void HSD_JObjSetFlagsAll(HSD_JObj* j, u32 flags)
{
    CHECK(j == &root && flags == JOBJ_HIDDEN);
    record(Hide, 0, 0, 0, 0, 0, 0);
    j->flags |= flags;
}
float ftLib_80086A0C(Fighter_GObj* g)
{
    return g ? ((Fighter*) g->user_data)->x34_scale.y : 1;
}
void(HSD_JObjSetMtxDirty)(HSD_JObj* j)
{
    CHECK(j == &root);
    record(Scale, 0, 0, j->scale.x, j->scale.y, j->scale.z, 0);
}
void it_80275534(Item_GObj* g, int a)
{
    CHECK(g == &object);
    record(Geometry, 1, a, 0, 0, 0, 0);
}
void it_80274DFC(Item_GObj* g)
{
    CHECK(g == &object);
    record(Geometry, 2, 0, 0, 0, 0, 0);
}
void it_80274E44(Item_GObj* g)
{
    CHECK(g == &object);
    record(Geometry, 3, 0, 0, 0, 0, 0);
}
bool it_80273030(Item_GObj* g)
{
    CHECK(g == &object);
    record(Reflect, 0, 0, 0, 0, 0, 0);
    item.x40_vel.x = -item.x40_vel.x;
    item.owner = &other;
    item.xD44_lifeTimer = 101;
    mutate();
    return false;
}
bool itColl_BounceOffShield(Item_GObj* g)
{
    CHECK(g == &object);
    record(ShieldBounce, 0, 0, 0, 0, 0, 0);
    return bounce_result;
}
bool it_8026B894(Item_GObj* g, Item_GObj* recipient)
{
    CHECK(g == &object);
    record(Interact, recipient == &other, 0, 0, 0, 0, 0);
    return true;
}
void efLib_DestroyAll(HSD_GObj* g)
{
    CHECK(g == &object);
    record(DestroyEffects, 0, 0, 0, 0, 0, 0);
}
EF_SpawnResult efSync_Spawn(s32 id, HSD_GObj* g, ...)
{
    CHECK(g == &object);
    va_list a;
    va_start(a, g);
    HSD_JObj* j = va_arg(a, HSD_JObj*);
    va_end(a);
    CHECK(j == &root);
    record(Effect, id, 0, item.x40_vel.x, item.x40_vel.y, item.x40_vel.z,
           item.xD44_lifeTimer);
    return (EF_SpawnResult){ 0 };
}
void ftKb_SpecialNPe_8010C3C0(HSD_GObj* g)
{
    (void) g;
    CHECK(false);
}
bool ftKb_SpecialNPe_8010C4AC(HSD_GObj* g)
{
    (void) g;
    CHECK(false);
    return false;
}
void ftCommon_8007E6DC(Fighter_GObj* g, Item_GObj* i, s32 b)
{
    (void) g;
    (void) i;
    (void) b;
    CHECK(false);
}
static void sentinel(Fighter_GObj* g)
{
    (void) g;
}
void ftPe_Init_OnDeath2(Fighter_GObj* g)
{
    CHECK(g == &source);
    ftPe_SpecialN_OnDeath2(g);
}
void Item_8026A8EC(Item_GObj* g)
{
    CHECK(g == &object);
    if (scripted) {
        if (item.xB8_itemLogicTable && item.xB8_itemLogicTable->destroyed) {
            item.xB8_itemLogicTable->destroyed(g);
        }
    } else if (item.kind == It_Kind_Peach_Toad) {
        itPeachToad_Logic91_Destroyed(g);
    }
    record(Free, 0, 0, 0, 0, 0, 0);
    heads[HSD_GOBJ_PLINK_ITEM] = NULL;
}
#include "actual.inc"
#include "generated.inc"
#include "parameters.inc"
#include "retail_logic.inc"
#include "retail_slots.inc"

static unsigned field(const char* name)
{
    for (unsigned i = 0; i < definition->state_field_count; i++) {
        if (!strcmp(name, definition->state_fields[i].name)) {
            return definition->state_fields[i].offset;
        }
    }
    CHECK(false);
    return 0;
}
static void setvar(Fighter* f, const char* n, int32_t value)
{
    script_state_set_int(f, field(n), value);
}
static int32_t variable(Fighter* f, const char* n)
{
    return script_state_get_int(f, field(n));
}
static void setboost(Fighter* f, bool value)
{
    f->u.pe.specialairn_used = value;
    memcpy(f->native_module_state + field("toad_air_boost_used"), &value, 1);
}
static const MotionState* motion(unsigned id)
{
    return &definition->motion_states[id - 341];
}
static const NativeFighterArticleLogic* lookup(ItemKind kind)
{
    for (unsigned i = 0; i < definition->article_logic_count; i++) {
        if (definition->article_logics[i].kind == kind) {
            return &definition->article_logics[i];
        }
    }
    CHECK(false);
    return NULL;
}
static void reset_case(bool script, int motion_id, bool present, int owner)
{
    scripted = script;
    count = float_cursor = 0;
    memset(events, 0, sizeof events);
    memset(&fighter, 0, sizeof fighter);
    memset(&foreign, 0, sizeof foreign);
    memset(&item, 0, sizeof item);
    memset(&root, 0, sizeof root);
    memset(heads, 0, sizeof heads);
    source = (HSD_GObj){ .user_data = &fighter };
    other = (HSD_GObj){ .user_data = &foreign };
    object = (HSD_GObj){ .user_data = &item, .hsd_obj = &root };
    heads[HSD_GOBJ_PLINK_FIGHTER] = &source;
    source.next = &other;
    fighter.kind = Ft_Kind_CustomFirst;
    foreign.kind = Ft_Kind_CustomFirst + 1;
    fighter.gobj = &source;
    foreign.gobj = &other;
    fighter.x8_spawnNum = UINT32_C(0xf0000001);
    foreign.x8_spawnNum = UINT32_C(0x90000002);
    fighter.parts = bones;
    fighter.dat_attrs = &attrs;
    fighter.motion_id = foreign.motion_id = motion_id;
    fighter.cur_anim_frame = 17.25f;
    fighter.facing_dir = -1;
    fighter.specialn_facing_dir = .75f;
    foreign.death2_cb = foreign.take_dmg_cb = sentinel;
    fighter.self_vel = (Vec3){ 3.25f, -2.5f, 7 };
    fighter.ground_or_air =
        motion_id == 367 || motion_id == 368 ? GA_Air : GA_Ground;
    fighter.x34_scale.y = 1.75f;
    foreign.x34_scale.y = .375f;
    fighter.co_attrs.gravity = .19f;
    fighter.co_attrs.terminal_velocity = 2.1f;
    fighter.shield_hit.skip_update_pos = true;
    fighter.x221B_b0 = true;
    fighter.x221B_b1 = true;
    fighter.x221B_b2 = true;
    fighter.x221B_b3 = false;
    fighter.x221B_b4 = true;
    fighter.shield_unk0 = 8.5f;
    fighter.shield_unk1 = 9.5f;
    fighter.cmd_vars[0] = fighter.cmd_vars[1] = fighter.cmd_vars[2] =
        fighter.cmd_vars[3] = 7;
    fighter.accessory4_cb = fighter.pre_hitlag_cb = fighter.post_hitlag_cb =
        fighter.death2_cb = fighter.take_dmg_cb = sentinel;
    if (present) {
        heads[HSD_GOBJ_PLINK_ITEM] = &object;
        fighter.u.pe.toad_gobj = &object;
        foreign.u.pe.toad_gobj = &object;
        fighter.x1984_heldItemSpec = &object;
    }
    item.kind = It_Kind_Peach_Toad;
    item.entity = &object;
    item.x1C = handle;
    item.owner = owner == 0 ? NULL : owner == 1 ? &source : &other;
    item.scl = 1.25f;
    item.x3C = 7;
    item.xC4_article_data = &article_asset;
    item.x40_vel = (Vec3){ 2, -3, 17 };
    item.xD44_lifeTimer = 40.75f;
    item.xDAC_itcmd_var0 = 0;
    item.native_article_author_kind = script ? fighter.kind : Ft_Kind_None;
    item.xDC8_word.flags.x15 = true;
    item.xDD4_itemVar.native_article.attachment_owner = &source;
    item.xDD4_itemVar.native_article.creator_spawn_id = fighter.x8_spawnNum;
    item.xDD4_itemVar.native_article.attachment_facing = -1;
    item.xB8_itemLogicTable = script ? toad->logic : NULL;
    root.scale = (Vec3){ 1, 1, 1 };
    root.flags = JOBJ_HIDDEN;
    article_asset.x4_specialAttributes = &spore_attrs;
    bones[109].joint = &joint;
    bones[attrs.xAC.x0_bone_id].joint = &joint;
    rig.parts_num = 114;
    rigs[fighter.kind] = &rig;
    rigs[foreign.kind] = &rig;
    setvar(&fighter, "toad_handle", present ? (int32_t) handle : -1);
    setvar(&foreign, "toad_handle", present ? (int32_t) handle : -1);
    setvar(&fighter, "parasol_handle", -1);
    setvar(&fighter, "saved_handle", -1);
    setvar(&fighter, "vegetable_handle", -1);
    setboost(&fighter, false);
    common.x4C_float = .5f;
    frames_remaining = ground_hit = true;
    air_hit = false;
    bounce_result = false;
    allocation_failure = false;
    invalidate_item = invalidate_fighter = 0;
    float_draws[0] = .25f;
    float_draws[1] = .75f;
}
typedef struct Projection {
    int motion, ground, commands[4], saved_facing, toad, aux, state;
    bool boost;
    unsigned callbacks, shieldflags, itemflags;
    bool guard_callback, guard_bone;
    int recipient_toad;
    unsigned recipient_callbacks, pre, post;
    unsigned draws;
    Vec3 velocity, accel, shieldoffset, scale, itemvel;
    float frame, hitlag0, hitlag1, radius, scl, life, half;
    unsigned rootflags;
    bool skip, manual;
    int owner;
} Projection;
static int callback_role(HSD_GObjEvent cb)
{
    if (!cb) {
        return 0;
    }
    if (cb == sentinel) {
        return 1;
    }
    if (cb == onAccessory4 || cb == script_fn_create_toad) {
        return 2;
    }
    if (cb == onHitAccessory4 || cb == script_fn_create_spore) {
        return 3;
    }
    if (cb == onEnterHitlag || cb == script_fn_pause_toad) {
        return 5;
    }
    if (cb == onExitHitlag || cb == script_fn_resume_toad) {
        return 6;
    }
    return 4;
}
static Projection project(void)
{
    Projection p = { 0 };
    p.motion = fighter.motion_id;
    p.ground = fighter.ground_or_air;
    memcpy(p.commands, fighter.cmd_vars, sizeof p.commands);
    p.saved_facing = scripted ? variable(&fighter, "toad_saved_facing")
                              : fighter.mv.pe.specialn.facing_dir;
    p.toad = scripted ? (variable(&fighter, "toad_handle") != -1)
                      : (fighter.u.pe.toad_gobj != NULL);
    p.aux = fighter.x1984_heldItemSpec != NULL;
    p.state = item.msid;
    p.boost =
        scripted
            ? (fighter.native_module_state[field("toad_air_boost_used")] != 0)
            : fighter.u.pe.specialairn_used;
    p.callbacks = callback_role(fighter.accessory4_cb) |
                  ((fighter.death2_cb != NULL) << 3) |
                  ((fighter.take_dmg_cb != NULL) << 4) |
                  ((fighter.pre_hitlag_cb != NULL) << 5) |
                  ((fighter.post_hitlag_cb != NULL) << 6);
    p.recipient_toad = scripted ? (variable(&foreign, "toad_handle") != -1)
                                : (foreign.u.pe.toad_gobj != NULL);
    p.recipient_callbacks =
        (foreign.death2_cb != NULL) | ((foreign.take_dmg_cb != NULL) << 1);
    p.pre = callback_role(fighter.pre_hitlag_cb);
    p.post = callback_role(fighter.post_hitlag_cb);
    p.draws = float_cursor;
    memcpy(&p.itemflags, &item.xDC8_word, sizeof p.itemflags);
    p.guard_callback = fighter.shield_hit_cb != NULL;
    p.guard_bone = fighter.shield_hit.bone == &joint;
    p.shieldflags = fighter.x221B_b0 | (fighter.x221B_b1 << 1) |
                    (fighter.x221B_b2 << 2) | (fighter.x221B_b3 << 3) |
                    (fighter.x221B_b4 << 4);
    p.velocity = fighter.self_vel;
    p.accel = fighter.x74_self_accel;
    p.shieldoffset = fighter.shield_hit.offset;
    p.radius = fighter.shield_hit.size;
    p.skip = fighter.shield_hit.skip_update_pos;
    p.frame = fighter.cur_anim_frame;
    p.hitlag0 = fighter.shield_unk0;
    p.hitlag1 = fighter.shield_unk1;
    p.scale = root.scale;
    p.itemvel = item.x40_vel;
    p.scl = item.scl;
    p.life = item.xD44_lifeTimer;
    p.half = item.xD48_halfLifeTimer;
    p.rootflags = root.flags;
    p.manual = item.xDC8_word.flags.x15;
    p.owner = item.owner == &source ? 1 : item.owner == &other ? 2 : 0;
    return p;
}
static void project_compare(Projection* want)
{
    Projection got = project();
    if (memcmp(&got, want, sizeof got)) {
        for (unsigned i = 0; i < sizeof got; i++) {
            if (((unsigned char*) &got)[i] != ((unsigned char*) want)[i]) {
                fprintf(stderr,
                        "projection byte %u old%u new%u motion%d owner%d "
                        "frame%g\n",
                        i, ((unsigned char*) want)[i],
                        ((unsigned char*) &got)[i], fighter.motion_id,
                        item.owner == &source  ? 1
                        : item.owner == &other ? 2
                                               : 0,
                        item.x5CC_currentAnimFrame);
            }
        }
    }
    CHECK(!memcmp(&got, want, sizeof got));
    compare();
}
void ftCamera_UpdateCameraBox(Fighter_GObj* g)
{
    (void) g;
    CHECK(false);
}

int main(void)
{
    MotionState states[30] = { 0 };
    for (unsigned i = 0; i < 30; i++) {
        states[i].anim_id = 400 + i;
        states[i].x4_flags = 0x100 + i;
    }
    for (unsigned i = 0; i < 4; i++) {
        states[24 + i] = retail_slots[i];
    }
    NativeFighterDefinition base = { .source_kind = Ft_Kind_Peach,
                                     .fighter_root = "ftDataPeach",
                                     .motion_state_first = 341,
                                     .motion_state_count = 30,
                                     .motion_states = states,
                                     .joint_count = 114 };
    NativeFighterHost host = { 0 };
#include "hosts.inc"
    host.callback_source_alive = nativeCallbackSource_Alive;
    host.change_motion_state = Fighter_ChangeMotionState;
    host.start_animation = ftAnim_8006EBA4;
    host.animation_frames_remaining = ftAnim_IsFramesRemaining;
    host.finish_ground_action = ft_8008A2BC;
    host.enter_fall = ftCo_Fall_Enter;
    host.ground_friction = ft_80084F3C;
    host.special_ground_collision = ft_800827A0;
    host.air_collision = ft_80081D0C;
    host.ground_to_air = ftCommon_8007D5D4;
    host.air_to_ground = ftCommon_8007D7FC;
    host.fall = ftCommon_Fall;
    host.fall_basic = ftCommon_FallBasic;
    host.air_decel = ftCommon_CalcSelfAccel_Deaccel;
    host.create_guard = ftColl_8007B1B8;
    host.bone_count = bone_count;
    host.joint_position = lb_8000B1CC;
    host.create_article_at = nativeArticle_Create;
    host.attach_created_item = nativeItem_Attach;
    host.bind_fighter_item = nativeItem_BindFighter;
    host.clear_fighter_item = nativeItem_ClearBinding;
    host.set_item_paused = nativeItem_SetPaused;
    host.item_creator = nativeArticle_Creator;
    host.item_current_owner = current_owner;
    host.target_motion = target_motion;
    host.target_call = nativeFighterEvent_Call;
    host.target_call_bool = nativeFighterEvent_CallBool;
    host.signal_article = signal_article;
    host.item_remove = Item_8026A8EC;
    host.item_change_state = item_change;
    host.item_advance_animation = Item_802694CC;
    host.item_hide_model = it_8026BB44;
    host.item_show_model = it_8026BB20;
    host.item_inherit_owner_scale = it_80274594;
    host.model_animation_rate = lb_8000BA0C;
    host.model_advance_animation = HSD_JObjAnimAll;
    host.item_interact = it_8026B894;
    host.random_float = HSD_Randf;
    host.sin_float = sinf;
    host.cos_float = cosf;
    host.item_set_lifetime = it_80275158;
    host.item_use_manual_lifetime = it_8026B3A8;
    host.spawn_effect = efSync_Spawn;
    host.item_set_debug_owner = db_80225DD8;
    host.destroy_effects = efLib_DestroyAll;
    host.article_ref_exists = nativeArticleRef_Exists;
    host.item_reflect = it_80273030;
    host.item_shield_bounce = itColl_BounceOffShield;
    CHECK(FreakMelee_SetFighterParameters(
        parameter_bits, sizeof parameter_bits / 4, parameter_schema));
    definition = FreakMelee_GetFighterModule(&base, &host);
    CHECK(definition && definition->state_field_count == 15 &&
          definition->state_bytes == 61 &&
          definition->source_kind == Ft_Kind_Peach);
    toad = lookup(It_Kind_Peach_Toad);
    spore = lookup(It_Kind_Peach_ToadSpore);
    CHECK(toad->state_count == 2 && spore->state_count == 1 &&
          definition->special_n &&
          definition->special_air_n);
    CHECK(nativeFighterEvents_Validate(definition));
    for (unsigned i = 0;
         i < (sizeof fixture_parameters / sizeof fixture_parameters[0]); i++)
    {
        const NativeFighterParameter* p = &fixture_parameters[i];
        if (!strncmp(p->name, "toad_", 5)) {
            float f;
            int32_t v;
            memcpy(&f, &parameter_bits[i], 4);
            memcpy(&v, &parameter_bits[i], 4);
            if (!strcmp(p->name, "toad_air_horizontal_divisor")) {
                attrs.specialairn_vel_x_div = f;
            } else if (!strcmp(p->name, "toad_air_deceleration")) {
                attrs.x98 = f;
            } else if (!strcmp(p->name, "toad_air_initial_vertical_speed")) {
                attrs.specialairn_vel_y = f;
            } else if (!strcmp(p->name, "toad_gravity")) {
                attrs.xA0 = f;
            } else if (!strcmp(p->name, "toad_terminal_velocity")) {
                attrs.xA4 = f;
            } else if (!strcmp(p->name, "toad_guard_hitlag")) {
                attrs.xA8 = f;
            } else if (!strcmp(p->name, "toad_guard_joint")) {
                attrs.xAC.x0_bone_id = v;
            } else if (!strcmp(p->name, "toad_guard_x")) {
                attrs.xAC.x4_offset.x = f;
            } else if (!strcmp(p->name, "toad_guard_y")) {
                attrs.xAC.x4_offset.y = f;
            } else if (!strcmp(p->name, "toad_guard_z")) {
                attrs.xAC.x4_offset.z = f;
            } else if (!strcmp(p->name, "toad_guard_radius")) {
                attrs.xAC.x10_size = f;
            }
        } else if (!strncmp(p->name, "It_Kind_Peach_ToadSpore.", 23)) {
            float f;
            memcpy(&f, &parameter_bits[i], 4);
            if (strstr(p->name, "minimum_speed")) {
                spore_attrs.x0_min_speed = f;
            } else if (strstr(p->name, "speed_range")) {
                spore_attrs.x4_max_speed_offset = f;
            } else if (strstr(p->name, "speed_decay")) {
                spore_attrs.x8_speed_decay_rate = f;
            } else if (strstr(p->name, "spread_angle")) {
                spore_attrs.xc_angle = f;
            }
        }
    }
    CHECK(coll_mf == UINT32_C(0x0c4c508c));
    CHECK(attrs.specialairn_vel_x_div > 0 && attrs.xAC.x0_bone_id >= 0 &&
          attrs.xAC.x0_bone_id < 114);
    CHECK(spore_attrs.x0_min_speed > 0);
    for (unsigned id = 341; id < 371; id++) {
        const MotionState* m = motion(id);
        CHECK(m->anim_id == states[id - 341].anim_id &&
              m->x4_flags == states[id - 341].x4_flags);
        if (id < 365 || id > 368) {
            CHECK(!memcmp(m, &states[id - 341], sizeof *m));
        } else {
            CHECK(m->anim_cb && m->input_cb && m->phys_cb && m->coll_cb);
        }
    }
    /* Every non-null state/lifecycle slot matches the actual retail rows. */
    CHECK(!toad->on_created && !toad->logic->spawned &&
          !toad->logic->dropped && !toad->logic->thrown &&
          !toad->logic->dmg_dealt && !toad->logic->dmg_received &&
          !toad->logic->entered_air && !toad->logic->reflected &&
          !toad->logic->clanked && !toad->logic->absorbed &&
          !toad->logic->shield_bounced && !toad->logic->hit_shield);
    CHECK(toad->logic->destroyed && toad->logic->picked_up &&
          toad->logic->evt_unk);
    CHECK(retail_toad_logic.states == it_803F7528 &&
          retail_spore_logic.states == it_803F7548);
    for (unsigned i = 0; i < 2; i++) {
        CHECK(toad->logic->states[i].anim_id == it_803F7528[i].anim_id &&
              toad->logic->states[i].animated &&
              !toad->logic->states[i].physics_updated &&
              !toad->logic->states[i].collided);
    }
    CHECK(spore->on_created && spore->logic->states[0].animated &&
          spore->logic->states[0].physics_updated &&
          !spore->logic->states[0].collided);
    CHECK(!spore->logic->spawned && !spore->logic->destroyed &&
          !spore->logic->picked_up && !spore->logic->dropped &&
          !spore->logic->thrown && !spore->logic->dmg_received &&
          !spore->logic->entered_air);
    CHECK(spore->logic->dmg_dealt && spore->logic->clanked &&
          spore->logic->hit_shield && spore->logic->absorbed &&
          spore->logic->shield_bounced && spore->logic->reflected &&
          spore->logic->evt_unk);
    for (unsigned id = 365; id <= 368; id++) {
        Projection want;
        reset_case(false, id, true, 1);
        retail_slots[id - 365].input_cb(&source);
        want = project();
        save();
        reset_case(true, id, true, 1);
        motion(id)->input_cb(&source);
        project_compare(&want);
    }
    const float facing[] = { -1, 1, -.75f, .75f },
                vels[] = { -10, -.125f, 0, .125f, 10 };
    for (int air = 0; air < 2; air++) {
        for (unsigned face = 0; face < 4; face++) {
            for (unsigned v = 0; v < 5; v++) {
                Projection want;
                for (int mode = 0; mode < 2; mode++) {
                    reset_case(mode, air ? 367 : 365, false, 1);
                    fighter.facing_dir = facing[face];
                    fighter.self_vel.x = vels[v];
                    if (!mode) {
                        if (air) {
                            ftPe_SpecialAirN_Enter(&source);
                        } else {
                            ftPe_SpecialN_Enter(&source);
                        }
                        want = project();
                        save();
                    } else {
                        (air ? definition->special_air_n
                             : definition->special_n)(&source);
                        project_compare(&want);
                    }
                }
            }
        }
    }
    for (unsigned id = 365; id <= 368; id++) {
        for (int command = -1; command <= 3; command++) {
            for (int remaining = 0; remaining < 2; remaining++) {
                Projection want;
                for (int mode = 0; mode < 2; mode++) {
                    reset_case(mode, id, true, 1);
                    fighter.cmd_vars[1] = command;
                    fighter.cmd_vars[3] = command;
                    frames_remaining = remaining;
                    if (!mode) {
                        retail_slots[id - 365].anim_cb(&source);
                        want = project();
                        save();
                    } else {
                        motion(id)->anim_cb(&source);
                        project_compare(&want);
                    }
                }
            }
        }
    }
    for (unsigned id = 365; id <= 368; id++) {
        for (int command = -1; command <= 3; command++) {
            for (int used = 0; used < 2; used++) {
                for (unsigned x = 0; x < 5; x++) {
                    for (unsigned y = 0; y < 5; y++) {
                        Projection want;
                        for (int mode = 0; mode < 2; mode++) {
                            reset_case(mode, id, true, 1);
                            fighter.cmd_vars[0] = command;
                            fighter.self_vel.x = vels[x];
                            fighter.self_vel.y = vels[y];
                            setboost(&fighter, used);
                            if (!mode) {
                                retail_slots[id - 365].phys_cb(&source);
                                want = project();
                                save();
                            } else {
                                motion(id)->phys_cb(&source);
                                project_compare(&want);
                            }
                        }
                    }
                }
            }
        }
    }
    for (unsigned id = 365; id <= 368; id++) {
        for (int contact = 0; contact < 2; contact++) {
            for (int present = 0; present < 2; present++) {
                for (int anim = -1; anim <= 3; anim++) {
                    for (int phys = -1; phys <= 3; phys++) {
                        Projection want;
                        for (int mode = 0; mode < 2; mode++) {
                            reset_case(mode, id, present, 1);
                            fighter.cmd_vars[0] = phys;
                            fighter.cmd_vars[1] = anim;
                            setboost(&fighter, true);
                            ground_hit = !contact;
                            air_hit = contact;
                            if (!mode) {
                                retail_slots[id - 365].coll_cb(&source);
                                want = project();
                                save();
                            } else {
                                motion(id)->coll_cb(&source);
                                project_compare(&want);
                            }
                        }
                    }
                }
            }
        }
    }
    for (int air = 0; air < 2; air++) {
        for (int present = 0; present < 2; present++) {
            for (unsigned face = 0; face < 4; face++) {
                Projection want;
                for (int mode = 0; mode < 2; mode++) {
                    reset_case(mode, air ? 367 : 365, present, 1);
                    // The normal animation command constructs the live guard
                    // and installs its explicit reaction helper in both paths.
                    fighter.cmd_vars[1] = 1;
                    if (!mode) {
                        retail_slots[air ? 2 : 0].anim_cb(&source);
                    } else {
                        motion(air ? 367 : 365)->anim_cb(&source);
                    }
                    CHECK(fighter.x221B_b0 && fighter.shield_hit_cb);
                    fighter.specialn_facing_dir = facing[face];
                    if (!mode) {
                        fighter.shield_hit_cb(&source);
                        want = project();
                        save();
                    } else {
                        fighter.shield_hit_cb(&source);
                        project_compare(&want);
                    }
                }
            }
        }
    }
    for (int cmd = 0; cmd < 3; cmd++) {
        for (int fail = 0; fail < 2; fail++) {
            for (unsigned face = 0; face < 4; face++) {
                Projection want;
                for (int mode = 0; mode < 2; mode++) {
                    reset_case(mode, 365, false, 1);
                    fighter.cmd_vars[2] = cmd;
                    fighter.facing_dir = facing[face];
                    allocation_failure = fail;
                    if (!mode) {
                        onAccessory4(&source);
                        want = project();
                        save();
                    } else {
                        script_fn_create_toad(&source);
                        project_compare(&want);
                    }
                }
            }
        }
    }
    for (int fail = 0; fail < 2; fail++) {
        for (unsigned face = 0; face < 4; face++) {
            for (int a = 0; a < 5; a++) {
                for (int b = 0; b < 5; b++) {
                    Projection want;
                    for (int mode = 0; mode < 2; mode++) {
                        reset_case(mode, 366, false, 1);
                        fighter.facing_dir = facing[face];
                        allocation_failure = fail;
                        float_draws[0] = a * .25f;
                        float_draws[1] = b * .25f;
                        if (!mode) {
                            onHitAccessory4(&source);
                            want = project();
                            save();
                        } else {
                            script_fn_create_spore(&source);
                            project_compare(&want);
                        }
                    }
                }
            }
        }
    }
    const int motions[] = { 350, 364, 365, 366, 367, 368, 369, 400 };
    const float frames[] = {
        0, 6.999f, 7, 7.001f, 52.999f, 53, 59.999f, 60, 80
    };
    for (unsigned state = 0; state < 2; state++) {
        for (unsigned m = 0; m < 8; m++) {
            for (unsigned f = 0; f < 9; f++) {
                for (int owner = 0; owner < 3; owner++) {
                    Projection want;
                    bool result = false;
                    int32_t other_handle = 0;
                    for (int mode = 0; mode < 2; mode++) {
                        reset_case(mode, motions[m], true, owner);
                        item.x5CC_currentAnimFrame = frames[f];
                        if (!mode) {
                            result = it_803F7528[state].animated(&object);
                            other_handle = foreign.u.pe.toad_gobj != NULL;
                            want = project();
                            save();
                        } else {
                            CHECK(toad->logic->states[state].animated(
                                      &object) == result);
                            CHECK((variable(&foreign, "toad_handle") != -1) ==
                                  (owner != 2 || other_handle));
                            project_compare(&want);
                        }
                    }
                }
            }
        }
    }
    for (int owner = 0; owner < 3; owner++) {
        for (int action = 0; action < 5; action++) {
            Projection want;
            for (int mode = 0; mode < 2; mode++) {
                reset_case(mode, 365, true, owner);
                if (action == 0) {
                    if (!mode) {
                        itPeachToad_Logic91_PickedUp(&object);
                    } else {
                        toad->logic->picked_up(&object);
                    }
                } else if (action == 1) {
                    if (!mode) {
                        it_802BE100(&object);
                    } else {
                        signal_article(&source, handle, name_hash("respond"),
                                       0);
                    }
                } else if (action == 2) {
                    if (!mode) {
                        itPeachToad_Logic91_Destroyed(&object);
                    } else {
                        toad->logic->destroyed(&object);
                    }
                } else if (action == 3) {
                    if (!mode) {
                        it_802BDF40(&object);
                    } else {
                        signal_article(&source, handle,
                                       name_hash("remove_cleanup"), 0);
                    }
                } else {
                    if (!mode) {
                        itPeachToad_Logic91_EvtUnk(&object, &other);
                    } else {
                        toad->logic->evt_unk(&object, &other);
                    }
                }
                if (!mode) {
                    want = project();
                    save();
                } else {
                    project_compare(&want);
                }
            }
        }
    }
    for (int present = 0; present < 2; present++) {
        for (int paused = 0; paused < 2; paused++) {
            Projection want;
            for (int mode = 0; mode < 2; mode++) {
                reset_case(mode, 365, present, 1);
                if (!mode) {
                    if (paused) {
                        onEnterHitlag(&source);
                    } else {
                        onExitHitlag(&source);
                    }
                    want = project();
                    save();
                } else {
                    if (paused) {
                        script_fn_pause_toad(&source);
                    } else {
                        script_fn_resume_toad(&source);
                    }
                    project_compare(&want);
                }
            }
        }
    }
    const float lives[] = { -1, 0, .5f, 1, 1.25f, 60 };
    for (unsigned l = 0; l < 6; l++) {
        for (int cmd = -1; cmd < 3; cmd++) {
            Projection want;
            bool result = false;
            for (int mode = 0; mode < 2; mode++) {
                reset_case(mode, 366, true, 1);
                item.kind = It_Kind_Peach_ToadSpore;
                item.xB8_itemLogicTable = mode ? spore->logic : NULL;
                item.xD44_lifeTimer = lives[l];
                item.xDAC_itcmd_var0 = cmd;
                if (!mode) {
                    result = itPeachtoadspore_UnkMotion0_Anim(&object);
                    want = project();
                    save();
                } else {
                    CHECK(spore->logic->states[0].animated(&object) == result);
                    project_compare(&want);
                }
            }
        }
    }
    for (unsigned x = 0; x < 5; x++) {
        for (unsigned y = 0; y < 5; y++) {
            Projection want;
            for (int mode = 0; mode < 2; mode++) {
                reset_case(mode, 366, true, 1);
                item.kind = It_Kind_Peach_ToadSpore;
                item.x40_vel = (Vec3){ vels[x], vels[y], 17 };
                if (!mode) {
                    itPeachtoadspore_UnkMotion0_Phys(&object);
                    want = project();
                    save();
                } else {
                    spore->logic->states[0].physics_updated(&object);
                    project_compare(&want);
                }
            }
        }
    }
    CounterPredicate stock_events[] = { itPeachToadSpore_Logic92_DmgDealt,
                                        itPeachToadSpore_Logic68_Clanked,
                                        itPeachToadSpore_Logic68_HitShield,
                                        itPeachToadSpore_Logic68_Absorbed,
                                        itPeachToadSpore_Logic68_ShieldBounced,
                                        itPeachToadSpore_Logic68_Reflected };
    CounterPredicate generated_events[] = {
        spore->logic->dmg_dealt,      spore->logic->clanked,
        spore->logic->hit_shield,     spore->logic->absorbed,
        spore->logic->shield_bounced, spore->logic->reflected
    };
    for (unsigned event = 0; event < 6; event++) {
        for (unsigned l = 0; l < 6; l++) {
            for (int bounce = 0; bounce < 2; bounce++) {
                Projection want;
                bool result = false;
                for (int mode = 0; mode < 2; mode++) {
                    reset_case(mode, 366, true, 1);
                    item.kind = It_Kind_Peach_ToadSpore;
                    item.xD44_lifeTimer = lives[l];
                    bounce_result = bounce;
                    if (!mode) {
                        result = stock_events[event](&object);
                        want = project();
                        save();
                    } else {
                        CHECK(generated_events[event](&object) == result);
                        project_compare(&want);
                    }
                }
            }
        }
    }
    reset_case(false, 366, true, 1);
    itPeachToadSpore_Logic92_EvtUnk(&object, &other);
    save();
    reset_case(true, 366, true, 1);
    spore->logic->evt_unk(&object, &other);
    compare();
    /* Defined authored invalidation policy; not original stale-pointer parity.
     */
    for (int mutation = 1; mutation <= 2; mutation++) {
        reset_case(true, 367, false, 1);
        invalidate_fighter = mutation;
        definition->special_air_n(&source);
        CHECK(count == 1 && events[0].op == Motion &&
              fighter.cmd_vars[0] == 7);
        ++safety;
    }
    for (int mutation = 1; mutation <= 2; mutation++) {
        reset_case(true, 366, true, 1);
        invalidate_item = mutation;
        item.xB8_itemLogicTable = spore->logic;
        spore->on_created(&object);
        CHECK(events[count - 1].op == ItemState);
        ++safety;
    }
    reset_case(true, 365, false, 1);
    rig.parts_num = 109;
    fighter.cmd_vars[2] = 0;
    script_fn_create_toad(&source);
    CHECK(variable(&fighter, "toad_handle") == -1 &&
          !heads[HSD_GOBJ_PLINK_ITEM]);
    ++safety;
    reset_case(true, 365, true, 1);
    item.x1C++;
    script_fn_pause_toad(&source);
    CHECK(count == 0);
    ++safety;
    for (int mutation = 1; mutation <= 2; mutation++) {
        reset_case(true, 365, false, 1);
        fighter.cmd_vars[2] = 0;
        invalidate_item = mutation;
        script_fn_create_toad(&source);
        CHECK(variable(&fighter, "toad_handle") == -1 &&
              fighter.x1984_heldItemSpec == NULL &&
              events[count - 1].op == ItemState);
        ++safety;
    }
    for (int mutation = 1; mutation <= 2; mutation++) {
        reset_case(true, 366, true, 1);
        invalidate_fighter = mutation;
        script_fn_create_spore(&source);
        CHECK(fighter.accessory4_cb == sentinel &&
              events[count - 1].op == Debug);
        ++safety;
    }
    reset_case(true, 350, true, 1);
    heads[HSD_GOBJ_PLINK_FIGHTER] = &other;
    source.user_data = (void*) (uintptr_t) 1;
    CHECK(toad->logic->states[0].animated(&object));
    CHECK(variable(&fighter, "toad_handle") == (int32_t) handle);
    ++safety;
    printf("Peach counter: %u original/generated comparisons, %u checks, %u "
           "authored safety cases passed.\n",
           pairs, checks, safety);
    return 0;
}
