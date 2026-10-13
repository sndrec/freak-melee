/* Source-only composition: actual frozen bodies and final generated callbacks.
 * Animation/collision/allocation/AB54 are explicit controlled engine
 * boundaries. The real generic item owner backend and full-generation registry
 * are linked. */
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <melee/db/db.h>
#include <melee/ef/efsync.h>
#include <melee/ef/native_model_effect.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcamera.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/ft/kinds/ftCommon/ftCo_ItemThrow.h>
#include <melee/ft/kinds/ftCommon/ftCo_Wait.h>
#include <melee/ft/kinds/ftCommon/ftpickupitem.h>
#include <melee/ft/kinds/ftCommon/inlines.h>
#include <melee/ft/kinds/ftPeach/ftpeach.h>
#include <melee/ft/kinds/ftPeach/ftpeachattacks4.h>
#include <melee/ft/kinds/ftPeach/ftpeachspeciallw.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_target.h>
#include <melee/it/inlines.h>
#include <melee/it/itCharItems.h>
#include <melee/it/kinds/inlines.h>
#include <melee/it/kinds/itpeachexplode.h>
#include <melee/it/kinds/itpeachturnip.h>
#include <melee/it/native_article.h>
#include <melee/it/native_item_owner.h>
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/random.h>

static unsigned pairs, checks, safety;
#define CHECK(x)                                                              \
    do {                                                                      \
        checks++;                                                             \
        if (!(x)) {                                                           \
            fprintf(stderr, "pair%u line%d %s\n", pairs, __LINE__, #x);       \
            exit(1);                                                          \
        }                                                                     \
    } while (0)
typedef struct PluckEvent {
    int op, a, b;
    float x, y, z, w;
} PluckEvent;
enum {
    Random,
    Motion,
    Start,
    Finish,
    Fall,
    Actions,
    GroundPhysics,
    AirPhysics,
    GroundProbe,
    AirProbe,
    ToAir,
    ToGround,
    Throw,
    Position,
    Allocate,
    Lifetime,
    Attach,
    Appearance,
    ItemState,
    Advance,
    Scale,
    Geometry,
    Pickup,
    Effect,
    Hide,
    Sound,
    Debug,
    Interact
};
static PluckEvent events[128], expected[128];
static unsigned count, expected_count;
static void record(int op, int a, int b, float x, float y, float z, float w)
{
    CHECK(count < 128);
    events[count++] = (PluckEvent){ op, a, b, x, y, z, w };
}
static void save(void)
{
    expected_count = count;
    memcpy(expected, events, sizeof(events));
    count = 0;
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
    CHECK(count == expected_count);
    CHECK(!memcmp(events, expected, count * sizeof(*events)));
    pairs++;
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
static ftData data;
static struct ftData_x8 attachment;
static ItemStateArray appearances;
static Article turnip_asset;
static union {
    itPeachTurnipAttributes attr;
    unsigned char storage[88];
} faces;
static ItemLogicTable picked_logic;
static ItemCommonData common;
ItemCommonData* it_804D6D28 = &common;
static const NativeFighterDefinition* definition;
static const NativeFighterArticleLogic *turnip, *explode;
static bool scripted, allocation_failure, frames_remaining, ground_hit,
    air_hit;
static int invalidate_item, invalidate_fighter;
static unsigned draws[12], draw_count, draw_cursor;
static SpawnItem observed;
static const uint32_t handle = UINT32_C(0xfedcba98);
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind k)
{
    return k == fighter.kind ? definition : NULL;
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
    exit(1);
}
s32 HSD_Randi(s32 n)
{
    CHECK(n > 0 && draw_cursor < draw_count &&
          draws[draw_cursor] < (unsigned) n);
    int v = draws[draw_cursor++];
    record(Random, n, 0, v, 0, 0, 0);
    return v;
}
void Fighter_ChangeMotionState(Fighter_GObj* g, FtMotionId m, MotionFlags f,
                               float start, float rate, float blend,
                               Fighter_GObj* donor)
{
    CHECK(g == &source && blend == 0 && !donor);
    record(Motion, m, f, start, rate, 0, 0);
    fighter.motion_id = m;
    fighter.cur_anim_frame = start;
    if (invalidate_fighter == 1) {
        heads[HSD_GOBJ_PLINK_FIGHTER] = NULL;
    }
    if (invalidate_fighter == 2) {
        fighter.x8_spawnNum++;
    }
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
    if (invalidate_fighter == 1) {
        heads[HSD_GOBJ_PLINK_FIGHTER] = NULL;
    }
    if (invalidate_fighter == 2) {
        fighter.x8_spawnNum++;
    }
}
void ftCo_Fall_Enter(Fighter_GObj* g)
{
    CHECK(g == &source);
    record(Fall, 0, 0, 0, 0, 0, 0);
    fighter.motion_id = 29;
}
void ftCo_Wait_IASA(Fighter_GObj* g)
{
    CHECK(g == &source);
    record(Actions, 0, 0, 0, 0, 0, 0);
}
void ft_80084FA8(Fighter_GObj* g)
{
    CHECK(g == &source);
    record(GroundPhysics, 1, 0, 0, 0, 0, 0);
}
void ft_80084F3C(Fighter_GObj* g)
{
    CHECK(g == &source);
    record(GroundPhysics, 2, 0, 0, 0, 0, 0);
}
void ft_80084EEC(Fighter_GObj* g)
{
    CHECK(g == &source);
    record(AirPhysics, 0, 0, 0, 0, 0, 0);
}
bool ft_80082708(Fighter_GObj* g)
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
void ft_80084104(Fighter_GObj* g)
{
    CHECK(g == &source);
    record(GroundProbe, 1, 0, 0, 0, 0, 0);
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
void ftCo_800957F4(Fighter_GObj* g, FtMotionId m)
{
    CHECK(g == &source);
    record(Throw, m, 0, 0, 0, 0, 0);
}
ItemKind itGetKind(Item_GObj* g)
{
    CHECK(g == &object);
    return item.kind;
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
    return 114;
}
static HSD_GObj* allocate(SpawnItem* s)
{
    observed = *s;
    record(Allocate, s->kind, s->x44_flag.b0, s->pos.x, s->pos.y, s->pos.z,
           s->facing_dir);
    CHECK(s->x0_parent_gobj == &source && s->x4_parent_gobj2 == &source &&
          s->x40 == 0 && s->x3C_damage == 0);
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
    picked_logic.picked_up = s->kind == It_Kind_Peach_Turnip
                                 ? (scripted ? turnip->logic->picked_up
                                             : itPeachTurnip_Logic56_PickedUp)
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
    return allocate(s);
}
Item_GObj* Item_CreateAuthored(SpawnItem* s, FighterKind author)
{
    Item_GObj* g = allocate(s);
    if (g) {
        item.native_article_author_kind = author;
    }
    return g;
}
Article* nativeFighterRuntime_ArticleAssets(FighterKind author, ItemKind kind)
{
    CHECK(author == fighter.kind);
    CHECK(kind == It_Kind_Peach_Turnip || kind == It_Kind_Peach_Explode);
    return &turnip_asset;
}
uint32_t nativeFighterTarget_Id(HSD_GObj* g)
{
    CHECK(g == &source);
    return (uint32_t) fighter.x8_spawnNum;
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
void it_8026BB68(Fighter_GObj* g, Vec3* p)
{
    (void) g;
    (void) p;
    CHECK(false);
}
void Item_8026A8EC(Item_GObj* g)
{
    CHECK(g == &object);
    heads[HSD_GOBJ_PLINK_ITEM] = NULL;
}
void Item_8026AB54(Item_GObj* g, HSD_GObj* owner, Fighter_Part part)
{
    CHECK(g == &object && owner == &source && part == 109);
    record(Attach, part, 0, 0, 0, 0, 0);
    item.owner = owner;
    item.xDC4 = part;
    if (picked_logic.picked_up) {
        picked_logic.picked_up(g);
    }
}
void db_80225DD8(Item_GObj* g, Fighter_GObj* owner)
{
    CHECK(g == &object && owner == &source);
    record(Debug, 0, 0, 0, 0, 0, 0);
}
void it_8026BD0C(Item_GObj* g)
{
    CHECK(g == &object);
    item.xDCD_flag.b2 = true;
}
void it_8026BCF4(Item_GObj* g)
{
    CHECK(g == &object);
    item.xDCD_flag.b2 = false;
}
void it_80273670(Item_GObj* g, int d, float f)
{
    CHECK(g == &object);
    record(Appearance, d, 0, f, 0, 0, 0);
}
void Item_80268E5C(Item_GObj* g, enum_t state, Item_StateChangeFlags flags)
{
    CHECK(g == &object);
    record(ItemState, state, flags, item.xD44_lifeTimer,
           item.xDC8_word.flags.xC, item.xDC8_word.flags.x15,
           item.xDD0_flag.b3);
    item.msid = state;
    if (invalidate_item == 1) {
        heads[HSD_GOBJ_PLINK_ITEM] = NULL;
    }
    if (invalidate_item == 2) {
        item.x1C++;
    }
}
static void item_change(Item_GObj* g, int s)
{
    Item_80268E5C(g, s, ITEM_ANIM_UPDATE);
}
void Item_802694CC(Item_GObj* g)
{
    CHECK(g == &object);
    record(Advance, 0, 0, 0, 0, 0, 0);
}
void ftpickupitem_80094818(Fighter_GObj* g, bool flag)
{
    CHECK(g == &source && !flag && fighter.item_gobj == &object);
    record(Pickup, 0, 0, 0, 0, 0, 0);
}
void ftCommon_8007E6DC(Fighter_GObj* g, Item_GObj* i, s32 b)
{
    (void) g;
    (void) i;
    (void) b;
    CHECK(false);
}
void ftPe_Init_OnDeath2(Fighter_GObj* g)
{
    (void) g;
    CHECK(false);
} /* Registration only. */
EF_SpawnResult efSync_Spawn(s32 effect, HSD_GObj* g, ...)
{
    va_list a;
    va_start(a, g);
    Vec3* pos = va_arg(a, Vec3*);
    va_end(a);
    CHECK(g == &source || g == &object);
    record(Effect, effect, g == &object, pos->x, pos->y, pos->z,
           g == &object ? item.xD44_lifeTimer : 0);
    return (EF_SpawnResult){ 0 };
}
void it_80272A3C(HSD_JObj* j)
{
    CHECK(j == &root);
    record(Hide, 0, 0, item.xD44_lifeTimer, item.xDAC_itcmd_var0, 0, 0);
    j->flags |= JOBJ_HIDDEN;
}
void Item_8026AE84(Item* i, enum_t sound, u8 pan, u8 volume)
{
    CHECK(i == &item);
    record(Sound, sound, pan, volume, item.xD44_lifeTimer, 0, 0);
}
static void play_sound(Item_GObj* g, s32 sound, u8 p, u8 v)
{
    CHECK(g == &object);
    Item_8026AE84(&item, sound, p, v);
}
float ftLib_80086A0C(Fighter_GObj* g)
{
    CHECK(g == &source || g == &other || g == NULL);
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
bool it_8026B894(Item_GObj* g, Item_GObj* o)
{
    CHECK(g == &object);
    record(Interact, o == &source, 0, 0, 0, 0, 0);
    return false;
}
void it_8026BB20(Item_GObj* g)
{
    (void) g;
    CHECK(false);
}
void it_8026B724(Item_GObj* g)
{
    (void) g;
    CHECK(false);
}
void it_8026B73C(Item_GObj* g)
{
    (void) g;
    CHECK(false);
}
#include "actual.inc"
#include "generated.inc"
#include "parameters.inc"
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
static int32_t variable(const char* n)
{
    return script_state_get_int(&fighter, field(n));
}
static void setvar(const char* n, int32_t v)
{
    script_state_set_int(&fighter, field(n), v);
}

static void reset(bool script)
{
    scripted = script;
    count = draw_cursor = 0;
    draw_count = 12;
    memset(draws, 0, sizeof draws);
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
    fighter.gobj = &source;
    fighter.kind = Ft_Kind_Peach;
    fighter.x8_spawnNum = UINT32_C(0xf0000001);
    foreign.x8_spawnNum = 2;
    fighter.parts = bones;
    bones[109].joint = &joint;
    fighter.dat_attrs = &attrs;
    fighter.ft_data = &data;
    data.x8 = &attachment;
    attachment.x10 = 109;
    fighter.x34_scale.y = 1.75f;
    foreign.x34_scale.y = .375f;
    fighter.facing_dir = -1;
    fighter.cur_pos = (Vec3){ 1, 2, 3 };
    fighter.cur_anim_frame = 17.25f;
    fighter.motion_id = 352;
    fighter.u.pe.veg_gobj = &object;
    fighter.u.pe.attacks4_motion_id = -1;
    fighter.allow_interrupt = true;
    fighter.cmd_vars[0] = 17;
    fighter.throw_flags = UINT32_MAX;
    setvar("last_smash_motion", -1);
    setvar("vegetable_handle", (int32_t) handle);
    item.entity = &object;
    item.x1C = handle;
    item.scl = 1.25f;
    item.throw_scale = 1;
    item.x3C = 7;
    item.xC4_article_data = &turnip_asset;
    item.xB8_itemLogicTable = &picked_logic;
    item.native_article_author_kind = Ft_Kind_None;
    item.xDC8_word.flags.xC = 1;
    item.xDC8_word.flags.x15 = 1;
    item.xDC8_word.flags.x1A = 1;
    item.xDAC_itcmd_var0 = 9;
    root.scale = (Vec3){ 1, 1, 1 };
    turnip_asset.x4_specialAttributes = &faces.attr;
    turnip_asset.xC_itemStates = &appearances;
    appearances.count = 1;
    faces.attr.x0_lifetime = 140;
    faces.attr.x4_length = 8;
    int w[] = { 35, 6, 5, 3, 3, 4, 1, 1 }, d[] = { 2, 2, 2, 2, 2, 6, 12, 30 };
    for (int i = 0; i < 8; i++) {
        faces.attr.x8[i].x0_odds = w[i];
        faces.attr.x8[i].x4_damage = d[i];
    }
    attrs.x14 = 128;
    attrs.speciallw_item_table_count = 3;
    int weights[] = { 2, 3, 1 },
        kinds[] = { It_Kind_BombHei, It_Kind_Dosei, It_Kind_Sword };
    for (int i = 0; i < 3; i++) {
        attrs.speciallw_item_table[i].randi_max = weights[i];
        attrs.speciallw_item_table[i].kind = kinds[i];
    }
    picked_logic.picked_up =
        script ? turnip->logic->picked_up : itPeachTurnip_Logic56_PickedUp;
    rig.parts_num = 114;
    rigs[fighter.kind] = &rig;
    frames_remaining = true;
    ground_hit = true;
    air_hit = false;
    allocation_failure = false;
    invalidate_item = invalidate_fighter = 0;
    common.x4C_float = .5f;
    common.xF8 = 33.5f;
}
void ftCamera_UpdateCameraBox(Fighter_GObj* g)
{
    (void) g;
    CHECK(false);
}
#include "retail_slots.inc"
static const MotionState* motion(unsigned id)
{
    return &definition->motion_states[id - 341];
}
static const NativeFighterArticleLogic* article(ItemKind kind)
{
    for (unsigned i = 0; i < definition->article_logic_count; i++) {
        if (definition->article_logics[i].kind == kind) {
            return &definition->article_logics[i];
        }
    }
    CHECK(false);
    return NULL;
}
int main(void)
{
    MotionState states[30] = { 0 };
    for (unsigned i = 0; i < 30; i++) {
        states[i].anim_id = i + 400;
        states[i].x4_flags = i + 0x100;
    }
    for (unsigned i = 0; i < 6; i++) {
        states[retail_ids[i] - 341] = retail_slots[i];
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
    host.random_int = HSD_Randi;
    host.change_motion_state = Fighter_ChangeMotionState;
    host.start_animation = ftAnim_8006EBA4;
    host.animation_frames_remaining = ftAnim_IsFramesRemaining;
    host.finish_ground_action = ft_8008A2BC;
    host.enter_fall = ftCo_Fall_Enter;
    host.ground_actions = ftCo_Wait_IASA;
    host.ground_motion = ft_80084FA8;
    host.ground_friction = ft_80084F3C;
    host.air_gravity_and_friction = ft_80084EEC;
    host.ground_collision = ft_80082708;
    host.air_collision = ft_80081D0C;
    host.ground_standard_collision = ft_80084104;
    host.ground_to_air = ftCommon_8007D5D4;
    host.air_to_ground = ftCommon_8007D7FC;
    host.held_item_id = nativeItem_HeldId;
    host.held_item_kind = itGetKind;
    host.throw_held_item = ftCo_800957F4;
    host.bone_count = bone_count;
    host.joint_position = lb_8000B1CC;
    host.create_article_at = nativeArticle_Create;
    host.create_engine_item = nativeItem_CreateEngine;
    host.attach_created_item = nativeItem_Attach;
    host.inherit_created_item_scale = nativeItem_InheritOwnerScale;
    host.bind_fighter_item = nativeItem_BindFighter;
    host.clear_fighter_item = nativeItem_ClearBinding;
    host.notify_item_pickup = ftpickupitem_80094818;
    host.spawn_effect = efSync_Spawn;
    host.item_creator = nativeArticle_Creator;
    host.item_set_lifetime = it_80275158;
    host.item_set_appearance = it_80273670;
    host.item_change_state = item_change;
    host.item_advance_animation = Item_802694CC;
    host.item_change_state_flags = Item_80268E5C;
    host.item_use_manual_lifetime = it_8026B3A8;
    host.item_hide_model = it_8026BB44;
    host.item_play_sound = play_sound;
    host.item_begin_expiry = it_8027518C;
    host.item_expire_tick = it_802751D8;
    host.item_inherit_owner_scale = it_80274594;
    host.item_set_debug_owner = db_80225DD8;
    host.item_interact = it_8026B894;
    CHECK(FreakMelee_SetFighterParameters(
        parameter_bits, sizeof(parameter_bits) / 4, parameter_schema));
    definition = FreakMelee_GetFighterModule(&base, &host);
    CHECK(definition && definition->state_bytes == 61 &&
          definition->state_field_count == 15);
    turnip = article(It_Kind_Peach_Turnip);
    explode = article(It_Kind_Peach_Explode);
    CHECK(definition->article_logic_count == 5 && explode->state_count == 2 &&
          definition->callback_export_count == 7);
    CHECK(definition->source_kind == Ft_Kind_Peach);
    CHECK(definition->on_forward_smash_enter && definition->special_lw &&
          definition->special_air_lw);
    CHECK(ftCo_MS_LightThrowF4 == 108 && ftCo_MS_LightThrowAirF4 == 112 &&
          coll_mf == UINT32_C(0x0c4c5080));
    for (unsigned id = 341; id < 371; id++) {
        const MotionState* m = motion(id);
        CHECK(m->anim_id == states[id - 341].anim_id &&
              m->x4_flags == states[id - 341].x4_flags);
        if ((id < 349 || id > 353) && (id < 365 || id > 368)) {
            CHECK(!memcmp(m, &states[id - 341], sizeof *m));
        } else {
            CHECK(m->anim_cb && m->phys_cb && m->coll_cb &&
                  ((m->input_cb != NULL) == (id <= 351 || id >= 365)));
        }
    }
    CHECK(!motion(356)->anim_cb && !motion(356)->input_cb &&
          !motion(356)->phys_cb && !motion(356)->coll_cb);
    for (unsigned i = 0; i < 2; i++) {
        CHECK(explode->logic->states[i].anim_id == (int) i &&
              explode->logic->states[i].animated &&
              !explode->logic->states[i].physics_updated &&
              !explode->logic->states[i].collided);
    }
    CHECK(!explode->logic->destroyed && !explode->logic->picked_up &&
          !explode->logic->dropped && !explode->logic->thrown &&
          explode->logic->dmg_dealt && explode->logic->evt_unk);
    CHECK(!explode->logic->spawned && !explode->logic->dmg_received &&
          !explode->logic->entered_air && !explode->logic->reflected &&
          !explode->logic->clanked && !explode->logic->absorbed &&
          !explode->logic->shield_bounced && !explode->logic->hit_shield);
    CHECK(retail_explode_logic.states == it_803F7488 &&
          retail_explode_logic.dmg_dealt && retail_explode_logic.evt_unk);
    for (unsigned i = 0; i < 2; i++) {
        CHECK(it_803F7488[i].anim_id == explode->logic->states[i].anim_id);
        CHECK((it_803F7488[i].animated != NULL) ==
              (explode->logic->states[i].animated != NULL));
        CHECK(it_803F7488[i].physics_updated ==
                  explode->logic->states[i].physics_updated &&
              it_803F7488[i].collided == explode->logic->states[i].collided);
    }
    for (int prev = -1; prev <= 351; prev++) {
        if (prev == -1 || prev >= 349) {
            for (unsigned next = 0; next < 3; next++) {
                if ((int) next + 349 != prev) {
                    for (unsigned rejects = 0; rejects <= 5; rejects++) {
                        reset(false);
                        fighter.u.pe.attacks4_motion_id = prev;
                        for (unsigned i = 0; i < rejects; i++) {
                            draws[i] =
                                prev == -1 ? next : (unsigned) (prev - 349);
                        }
                        draws[rejects] = next;
                        ftPe_AttackS4_Enter(&source);
                        int m = fighter.motion_id;
                        unsigned consumed = draw_cursor;
                        save();
                        reset(true);
                        setvar("last_smash_motion", prev);
                        for (unsigned i = 0; i < rejects; i++) {
                            draws[i] =
                                prev == -1 ? next : (unsigned) (prev - 349);
                        }
                        draws[rejects] = next;
                        definition->on_forward_smash_enter(&source, 123.5f);
                        compare();
                        CHECK(fighter.motion_id == m &&
                              variable("last_smash_motion") == m &&
                              draw_cursor == consumed &&
                              !fighter.allow_interrupt &&
                              fighter.cmd_vars[0] == 0);
                    }
                }
            }
        }
    }
    for (unsigned id = 349; id <= 353; id++) {
        for (int finish = 0; finish < 2; finish++) {
            reset(false);
            frames_remaining = !finish;
            if (id <= 351) {
                ftPe_AttackS4_Anim(&source);
            } else if (id == 352) {
                ftPe_SpecialLw_Anim(&source);
            } else {
                ftPe_SpecialAirLw_Anim(&source);
            }
            bool cleared = fighter.u.pe.veg_gobj == NULL;
            int m = fighter.motion_id;
            save();
            reset(true);
            frames_remaining = !finish;
            motion(id)->anim_cb(&source);
            compare();
            CHECK(fighter.motion_id == m &&
                  ((variable("vegetable_handle") == -1) == cleared));
        }
    }
    for (unsigned id = 349; id <= 351; id++) {
        for (int allow = 0; allow < 2; allow++) {
            reset(false);
            fighter.allow_interrupt = allow;
            ftPe_AttackS4_IASA(&source);
            save();
            reset(true);
            fighter.allow_interrupt = allow;
            motion(id)->input_cb(&source);
            compare();
        }
    }
    for (unsigned id = 349; id <= 353; id++) {
        reset(false);
        if (id <= 351) {
            ftPe_AttackS4_Phys(&source);
        } else if (id == 352) {
            ftPe_SpecialLw_Phys(&source);
        } else {
            ftPe_SpecialAirLw_Phys(&source);
        }
        save();
        reset(true);
        motion(id)->phys_cb(&source);
        compare();
    }
    for (unsigned id = 349; id <= 353; id++) {
        for (int hit = 0; hit < 2; hit++) {
            reset(false);
            fighter.motion_id = id;
            ground_hit = air_hit = hit;
            if (id <= 351) {
                ftPe_AttackS4_Coll(&source);
            } else if (id == 352) {
                ftPe_SpecialLw_Coll(&source);
            } else {
                ftPe_SpecialAirLw_Coll(&source);
            }
            int m = fighter.motion_id;
            bool accessory = fighter.accessory4_cb != NULL;
            GroundOrAir ga = fighter.ground_or_air;
            save();
            reset(true);
            fighter.motion_id = id;
            ground_hit = air_hit = hit;
            motion(id)->coll_cb(&source);
            compare();
            CHECK(fighter.motion_id == m && fighter.ground_or_air == ga &&
                  (fighter.accessory4_cb != NULL) == accessory);
        }
    }
    for (int air = 0; air < 2; air++) {
        for (int held = 0; held < 3; held++) {
            reset(false);
            if (held) {
                fighter.item_gobj = &object;
                heads[HSD_GOBJ_PLINK_ITEM] = &object;
                item.kind = held == 1 ? It_Kind_Peach_Turnip : It_Kind_Sword;
            }
            if (air) {
                ftPe_SpecialAirLw_Enter(&source);
            } else {
                ftPe_SpecialLw_Enter(&source);
            }
            u32 flags = fighter.throw_flags;
            bool accessory = fighter.accessory4_cb != NULL;
            save();
            reset(true);
            if (held) {
                fighter.item_gobj = &object;
                heads[HSD_GOBJ_PLINK_ITEM] = &object;
                item.kind = held == 1 ? It_Kind_Peach_Turnip : It_Kind_Sword;
            }
            if (air) {
                definition->special_air_lw(&source);
            } else {
                definition->special_lw(&source);
            }
            compare();
            CHECK(fighter.throw_flags == flags &&
                  (fighter.accessory4_cb != NULL) == accessory);
        }
    }
    /* Actual entry/offledge callbacks, then actual accessory dispatch; no
     * on_tick surrogate. */
    for (int route = 0; route < 3; route++) {
        for (int cue = 0; cue < 2; cue++) {
            for (int rare = -1; rare < 6; rare++) {
                for (int fail = 0; fail < 2; fail++) {
                    for (unsigned face_draw = 0; face_draw < 58; face_draw++) {
                        reset(false);
                        if (route == 0) {
                            ftPe_SpecialLw_Enter(&source);
                        } else if (route == 1) {
                            ground_hit = false;
                            ftPe_SpecialLw_Coll(&source);
                        } else {
                            air_hit = true;
                            ftPe_SpecialAirLw_Coll(&source);
                        }
                        CHECK(fighter.accessory4_cb);
                        count = 0;
                        fighter.throw_flags_b0 = cue;
                        draws[0] = rare < 0 ? 1 : 0;
                        draws[1] = rare < 0 ? face_draw : rare;
                        draws[2] = face_draw;
                        allocation_failure = fail;
                        fighter.accessory4_cb(&source);
                        unsigned consumed = draw_cursor;
                        ItemKind kind = item.kind;
                        float scl = item.scl, remember = item.throw_scale;
                        bool held = fighter.item_gobj != NULL,
                             published = fighter.u.pe.veg_gobj != NULL;
                        save();
                        reset(true);
                        if (route == 0) {
                            definition->special_lw(&source);
                        } else if (route == 1) {
                            ground_hit = false;
                            motion(352)->coll_cb(&source);
                        } else {
                            air_hit = true;
                            motion(353)->coll_cb(&source);
                        }
                        CHECK(fighter.accessory4_cb);
                        count = 0;
                        fighter.throw_flags_b0 = cue;
                        draws[0] = rare < 0 ? 1 : 0;
                        draws[1] = rare < 0 ? face_draw : rare;
                        draws[2] = face_draw;
                        allocation_failure = fail;
                        fighter.accessory4_cb(&source);
                        compare();
                        CHECK(draw_cursor == consumed &&
                              !fighter.throw_flags_b0 && item.kind == kind &&
                              item.scl == scl &&
                              item.throw_scale == remember &&
                              (fighter.item_gobj != NULL) == held);
                        CHECK((variable("vegetable_handle") ==
                               (int32_t) handle) == published);
                        if (held) {
                            CHECK(fighter.death2_cb && fighter.take_dmg_cb);
                        }
                    }
                }
            }
        }
    }
    /* Complete Explode constructor composition, current-owner scale and
     * timers. */
    for (int variant = 0; variant < 2; variant++) {
        for (int fail = 0; fail < 2; fail++) {
            for (int scale = 0; scale < 5; scale++) {
                for (int face = -1; face <= 1; face += 2) {
                    reset(false);
                    allocation_failure = fail;
                    fighter.facing_dir = face;
                    fighter.x34_scale.y = .25f + scale * .5f;
                    Vec3 pos = { 4, 5, -3 };
                    Item_GObj* result =
                        it_802BD158(&source, &pos, variant, face);
                    bool success = result != NULL;
                    float scl = item.scl, life = item.xD44_lifeTimer;
                    int state = item.msid;
                    save();
                    reset(true);
                    allocation_failure = fail;
                    fighter.facing_dir = face;
                    fighter.x34_scale.y = .25f + scale * .5f;
                    int32_t id =
                        script_fn_create_explosion(&source, 4, 5, -3, variant);
                    compare();
                    CHECK((id != -1) == success);
                    if (success) {
                        CHECK((uint32_t) id == handle &&
                              item.owner == &source && item.scl == scl &&
                              item.xD44_lifeTimer == life &&
                              item.msid == state && life == common.xF8);
                        CHECK(item.pos.x == 4 && item.pos.y == 5 &&
                              item.pos.z == -3 && item.facing_dir == face);
                        CHECK(!item.xDC8_word.flags.xC &&
                              !item.xDC8_word.flags.x15 && item.xDD0_flag.b3 &&
                              item.xDCF_flag.b2 && item.xDD1_flag.b0 &&
                              !item.xDC8_word.flags.x1A);
                    }
                }
            }
        }
    }
    for (int owner = 0; owner < 3; owner++) {
        for (int variant = 0; variant < 2; variant++) {
            reset(false);
            heads[HSD_GOBJ_PLINK_ITEM] = &object;
            item.owner = owner == 0 ? NULL : owner == 1 ? &source : &other;
            it_80274594(&object);
            float scale = item.scl;
            save();
            reset(true);
            heads[HSD_GOBJ_PLINK_ITEM] = &object;
            item.owner = owner == 0 ? NULL : owner == 1 ? &source : &other;
            host.item_inherit_owner_scale(&object);
            compare();
            CHECK(item.scl == scale);
        }
    }
    const float lives[] = { -1, 0, .25f, 1, 1.25f, 2, 33.5f, 60 };
    for (int state = 0; state < 2; state++) {
        for (unsigned i = 0; i < sizeof(lives) / sizeof(*lives); i++) {
            reset(false);
            heads[HSD_GOBJ_PLINK_ITEM] = &object;
            item.xD44_lifeTimer = lives[i];
            bool a = itPeachexplode_UnkMotion1_Anim(&object);
            float life = item.xD44_lifeTimer;
            save();
            reset(true);
            heads[HSD_GOBJ_PLINK_ITEM] = &object;
            item.xD44_lifeTimer = lives[i];
            bool b = explode->logic->states[state].animated(&object);
            compare();
            CHECK(a == b && item.xD44_lifeTimer == life);
        }
    }
    reset(false);
    bool damage = itPeachExplode_Logic55_DmgDealt(&object);
    save();
    reset(true);
    CHECK(explode->logic->dmg_dealt(&object) == damage);
    compare();
    for (int recipient = 0; recipient < 2; recipient++) {
        reset(false);
        heads[HSD_GOBJ_PLINK_ITEM] = &object;
        itPeachExplode_Logic55_EvtUnk(&object, recipient ? &source : NULL);
        save();
        reset(true);
        heads[HSD_GOBJ_PLINK_ITEM] = &object;
        explode->logic->evt_unk(&object, recipient ? &source : NULL);
        compare();
    }
    /* Safety is authored behavior, separate from healthy stock pairs. */
    for (int mode = 1; mode <= 2; mode++) {
        reset(true);
        invalidate_fighter = mode;
        definition->special_lw(&source);
        CHECK(count == 1 && !fighter.accessory4_cb);
        safety++;
        reset(true);
        invalidate_fighter = mode;
        frames_remaining = false;
        motion(352)->anim_cb(&source);
        CHECK(variable("vegetable_handle") == (int32_t) handle && count == 1);
        safety++;
        reset(true);
        invalidate_item = mode;
        int32_t id = script_fn_create_explosion(&source, 4, 5, -3, 0);
        CHECK((uint32_t) id == handle &&
              !nativeCallbackSource_Alive(&object, handle,
                                          NativeCallbackSource_Item));
        CHECK(events[count - 1].op == ItemState);
        safety++;
    }
    printf("PASS %u original/generated comparisons, %u checks; %u authored "
           "safety cases\n",
           pairs, checks, safety);
    return 0;
}
