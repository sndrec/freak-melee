/* Float/SideB actual-source comparison: controlled boundaries are explicit.
 * The final generated matching-SDK module executes in this console fixture.
 * Source-only composition: actual frozen bodies and final generated callbacks.
 * Animation/collision/allocation/AB54 are explicit controlled engine
 * boundaries. The real generic item owner backend and full-generation registry
 * are linked. */
#include <melee/ft/ftchangeparam.h>
#include <melee/ft/ft_0CDD.h>
#include <melee/ft/ft_0DF1.h>
#include <melee/ft/kinds/ftCommon/ftCo_AttackAir.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallAerial.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallSpecial.h>
#include <melee/ft/kinds/ftCommon/ftCo_SpecialAir.h>
#include <melee/ft/kinds/ftCommon/ftCo_JumpAerial.h>
#include <melee/ft/kinds/ftCommon/ftCo_Landing.h>
#include <melee/ft/kinds/ftCommon/ftCo_LandingAir.h>
#include <melee/ft/kinds/ftPeach/ftpeachfloat.h>
#include <melee/ft/kinds/ftPeach/ftpeachfloatfall.h>
#include <melee/ft/kinds/ftPeach/ftpeachfloatattack.h>
#include <melee/ft/kinds/ftPeach/ftpeachspecials.h>
#include <melee/ft/native_fighter_contact.h>
#include <melee/ef/efasync.h>
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
typedef struct CompositionEvent {
    int op, a, b;
    float x, y, z, w;
} CompositionEvent;
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
static CompositionEvent events[128], expected[128];
static unsigned count, expected_count;
static void record(int op, int a, int b, float x, float y, float z, float w)
{
    CHECK(count < 128);
    events[count++] = (CompositionEvent){ op, a, b, x, y, z, w };
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
static const NativeFighterDefinition* recipient_definition;
static const NativeFighterArticleLogic *turnip, *explode;
static bool scripted, allocation_failure, frames_remaining, ground_hit,
    air_hit;
static int invalidate_item, invalidate_fighter;
static unsigned draws[12], draw_count, draw_cursor;
static SpawnItem observed;
static const uint32_t handle = UINT32_C(0xfedcba98);
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind k)
{
    if (k == Ft_Kind_Mars) return recipient_definition;
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
    if (fighter.x21EC) { HSD_GObjEvent cb = fighter.x21EC; fighter.x21EC = NULL; cb(g); }
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
#include "extra_boundaries.inc"
#include "actual.inc"
#include "generated.inc"
#include "parameters.inc"
#include "attributes.inc"
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

static void reset_case(bool script)
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

static float floatvar(const char* n) { return script_state_get_float(&fighter, field(n)); }
static void setfloat(const char* n, float v) { script_state_set_float(&fighter, field(n), v); }
static bool boolvar(const char* n) { uint8_t v; memcpy(&v, fighter.native_module_state+field(n), 1); return v != 0; }
static void setbool(const char* n, bool v) { uint8_t b = v; memcpy(fighter.native_module_state+field(n), &b, 1); }
static void seed_case(bool script, unsigned v, unsigned id)
{
 reset_case(script);
 memcpy(&attrs, dat_attribute_bits, sizeof dat_attribute_bits);
 attrs.floatfallf_anim_start = 23; attrs.floatfallb_anim_start = 31;
 setfloat("float_fall_forward_end", 23); setfloat("float_fall_backward_end", 31);
 bones[1].joint = bones[4].joint = &joint;
 fighter.u.pe.has_float = (v & 1) != 0;
 setbool("has_float", fighter.u.pe.has_float);
 float timers[] = {-1, 0, .25f, 1, 2, 150};
 fighter.u.pe.x4 = timers[v % 6]; setfloat("float_timer", fighter.u.pe.x4);
 fighter.mv.pe.floatattack.x0 = (v & 4) != 0;
 setbool("float_attack_released", fighter.mv.pe.floatattack.x0);
 setbool("side_smash", fighter.mv.pe.specials.x0);
 fighter.motion_id = id;
 fighter.ground_or_air = (v & 8) ? GA_Ground : GA_Air;
 fighter.cur_pos = (Vec3){1, 2, 3};
 fighter.self_vel = (Vec3){(v&1)?-3.25f:2.75f,(v&2)?-2.5f:1.75f,.125f};
 fighter.gr_vel = (v&2)?-.8f:.8f;
 fighter.xE4_ground_accel_1 = .625f;
 fighter.x74_self_accel = (Vec3){.1f,.2f,.3f};
 fighter.x6A4_transNOffset.x = 7.25f;
 fighter.facing_dir = (v&8) ? 1 : -1;
 fighter.x673 = v % 6;
 fighter.input.lstick[0] = (Vec2){(v&16)?-.75f:.75f,(float)((int)(v%7)-3)/3};
 fighter.input.cstick[0] = (Vec2){(v&32)?.75f:0,(v&64)?-.75f:0};
 fighter.input.cstick[1] = (Vec2){(v&128)?.75f:0,0};
 fighter.input.held_buttons[0] = (v&16)?HSD_PAD_X:0;
 fighter.input.pressed_buttons = (v&32)?HSD_PAD_A:0;
 fighter.allow_interrupt = (v&64) != 0;
 fighter.item_gobj = (v&4) ? &object : NULL;
 use_category = (v&8) ? 3 : 2;
 fighter.throw_flags = v&1 ? 8 : 0;
 fighter.cmd_vars[0] = v&1; fighter.cmd_vars[1] = (v>>1)&1;
 fighter.cmd_vars[2] = (v>>2)&1; fighter.cmd_vars[3] = (v>>3)&1;
 fighter.coll_data.env_flags = (v&16 ? Collide_RightWallMask : 0) |
   (v&32 ? Collide_LeftWallMask : 0) | (v&64 ? Collide_CeilingMask : 0);
 fighter.coll_data.floor.normal = (Vec3){.6f,.8f,0};
 common_fighter.tap_jump_threshold = .3f;
 common_fighter.x88 = .5f; common_fighter.x78 = .25f;
 common_fighter.xDC = .6f; common_fighter.xE0 = .6f; common_fighter.x20_radians = .7f;
 ground_friction = v&128 ? .75f : 1.25f;
 landing_threshold = -.25f;
 frames_remaining = (v&1) != 0; ground_hit = (v&2) != 0;
 air_hit = (v&4) != 0; special_ground_hit = (v&8) != 0;
 special_input = (v&128) != 0; item_throw_input = (v&32) != 0; jump_input = (v&16) != 0;
 recipient_definition = NULL; contact_mutation = 0;
 foreign.kind = Ft_Kind_Mars; foreign.motion_id = v&1 ? 369 : 368;
 fighter.unk_gobj = &other; fighter.x221C_b5 = (v&64) != 0;
 selector_calls = 0;
}
typedef struct Projection {
 Vec3 position, velocity, acceleration, animation_delta;
 float ground_velocity, ground_acceleration, timer, facing;
 int motion, commands[4], selector_calls, item_state;
 uint32_t throw_flags;
 bool has_float, released, side_smash, allowed, effects_active, thrown,
      facing_callback, hit_callback, state_callback;
} Projection;
static Projection project(bool side)
{
 Projection p = {0};
 p.position=fighter.cur_pos; p.velocity=fighter.self_vel; p.acceleration=fighter.x74_self_accel;
 p.animation_delta=fighter.x6A4_transNOffset;
 p.ground_velocity=fighter.gr_vel; p.ground_acceleration=fighter.xE4_ground_accel_1;
 p.timer=scripted ? floatvar("float_timer") : fighter.u.pe.x4;
 p.facing=fighter.facing_dir; p.motion=fighter.motion_id;
 for (unsigned i=0;i<4;i++) p.commands[i]=fighter.cmd_vars[i];
 p.selector_calls=selector_calls; p.item_state=item.msid;
 p.throw_flags=fighter.throw_flags;
 p.has_float=scripted ? boolvar("has_float") : fighter.u.pe.has_float;
 p.released=side ? false : (scripted ? boolvar("float_attack_released") : fighter.mv.pe.floatattack.x0);
 p.side_smash=side ? (scripted ? boolvar("side_smash") : fighter.mv.pe.specials.x0) : false;
 p.allowed=fighter.allow_interrupt; p.effects_active=fighter.x2219_b0;
 p.thrown=fighter.x2070.count_thrown_items;
 p.facing_callback=fighter.x21F8!=NULL; p.hit_callback=fighter.hurtbox_detect_cb!=NULL;
 p.state_callback=fighter.x21EC!=NULL;
 return p;
}
static HSD_GObjEvent original_phase(unsigned id, unsigned phase)
{
 const MotionState* m=&retail_slots[id-341];
 switch(phase) {case 0:return m->anim_cb;case 1:return m->input_cb;case 2:return m->phys_cb;default:return m->coll_cb;}
}
static HSD_GObjEvent generated_phase(unsigned id, unsigned phase)
{
 const MotionState* m=motion(id);
 switch(phase) {case 0:return m->anim_cb;case 1:return m->input_cb;case 2:return m->phys_cb;default:return m->coll_cb;}
}
static void pair_phase(unsigned id, unsigned phase, unsigned v)
{
 seed_case(false,v,id); original_phase(id,phase)(&source); Projection expected_projection=project(id>=354); save();
 seed_case(true,v,id); generated_phase(id,phase)(&source); Projection actual_projection=project(id>=354);
 if (memcmp(&expected_projection,&actual_projection,sizeof actual_projection)) {
  const float* a=(const float*)&expected_projection;const float*b=(const float*)&actual_projection;
  for(unsigned i=0;i<sizeof actual_projection/4;i++) if(memcmp(a+i,b+i,4)) fprintf(stderr,"projection id%u phase%u variant%u word%u old%g new%g\n",id,phase,v,i,a[i],b[i]);
 }
 CHECK(!memcmp(&expected_projection,&actual_projection,sizeof actual_projection)); compare();
}
int main(void)
{
 MotionState states[30]={0};
 for(unsigned i=0;i<30;i++){states[i].anim_id=400+i;states[i].x4_flags=0x100+i;}
 for(unsigned i=0;i<20;i++) states[i]=retail_slots[i];
 NativeFighterDefinition base={.source_kind=Ft_Kind_Peach,.fighter_root="ftDataPeach",
 .motion_state_first=341,.motion_state_count=30,.motion_states=states,.joint_count=114};
 NativeFighterHost host={0};
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

 host.held_item_id=held_id;
 host.resolve_bone=resolve_bone;host.spawn_async_effect=efAsync_Spawn;
 host.common_attribute=common_attribute;
 host.air_accel_normal=ftCommon_CalcSelfAccel_Drift;
 host.air_drift=ft_80084DB0;host.animation_velocity_y_adjusted=ft_800851D0;
 host.enter_fall_aerial=ftCo_FallAerial_Enter;
 host.enter_landing_basic=ftCo_Landing_Enter_Basic;
 host.basic_landing_velocity_threshold=ftCo_800D0EC8;
 host.air_special_landing_collision=ft_800831CC;
 host.fall_special_ledge_check=ftCo_80096CC8;
 host.cstick_attack_edge=ftCo_800DF478;
 host.aerial_attack_motion=ftCo_AttackAir_GetMsidFromCStick;
 host.item_use_category=itGetUseKind;host.enter_air_gun=ftCo_800CDDA0;
 host.check_air_item_throw=ftCo_80095328;host.check_air_special=ftCo_SpecialAir_CheckInput;
 host.aerial_attack_actions=ftCo_800CB870;
 host.special_ground_collision=ft_800827A0;
 host.air_decel=ftCommon_CalcSelfAccel_Deaccel;host.fall=ftCommon_Fall;
 host.ground_to_self_movement=ftCommon_SetSelfMovementFromGroundedMovement;
 host.contact_rebound_allowed=nativeFighterContact_ReboundAllowed;
 CHECK(FreakMelee_SetFighterParameters(parameter_bits,sizeof parameter_bits/4,parameter_schema));
 definition=FreakMelee_GetFighterModule(&base,&host);
 CHECK(definition && definition->state_bytes==69 && definition->state_field_count==18);
 CHECK(definition->source_kind==Ft_Kind_Peach && definition->article_logic_count==5);
 turnip=article(It_Kind_Peach_Turnip);explode=article(It_Kind_Peach_Explode);
 CHECK(definition->special_s && definition->special_air_s);
 CHECK(definition->on_aerial_action_manual && definition->on_aerial_action_automatic && definition->on_grounded_motion_setup);
 CHECK(!motion(356)->anim_cb && !motion(356)->input_cb && !motion(356)->phys_cb && !motion(356)->coll_cb);
 for(unsigned id=341;id<371;id++) {
  CHECK(motion(id)->anim_id==states[id-341].anim_id && motion(id)->x4_flags==states[id-341].x4_flags);
  if(id>=361 && id<=364 || id>=369) CHECK(!memcmp(motion(id),states+id-341,sizeof(MotionState)));
 }
 for(unsigned id=341;id<=360;id++) {
  if(id>=349 && id<=353 || id==356) continue;
  for(unsigned phase=0;phase<4;phase++) {
   CHECK(original_phase(id,phase) && generated_phase(id,phase));
   for(unsigned v=0;v<256;v++) pair_phase(id,phase,v);
  }
 }
 for(unsigned v=0;v<256;v++) {
  seed_case(false,v,341);ftPe_8011BB6C(&source,(v&2)!=0);Projection e=project(false);save();
  seed_case(true,v,341);script_fn_enter_float(&source,(v&2)!=0);Projection a=project(false);
  CHECK(!memcmp(&e,&a,sizeof a));compare();
  seed_case(false,v,341);bool eb=ftPe_8011BE80(&source);e=project(false);save();
  seed_case(true,v,341);bool ab=script_fn_try_float_attack(&source);a=project(false);
  CHECK(eb==ab && !memcmp(&e,&a,sizeof a));compare();
  for(unsigned air=0;air<2;air++) {
   seed_case(false,v,air?357:354);(air?ftPe_SpecialAirS_Enter:ftPe_SpecialS_Enter)(&source);e=project(true);save();
   seed_case(true,v,air?357:354);(air?definition->special_air_s:definition->special_s)(&source);a=project(true);
   CHECK(!memcmp(&e,&a,sizeof a));compare();
  }
  seed_case(false,v,360);doAirEnd0(&source);e=project(true);save();
  seed_case(true,v,360);authored_hurtbox_detect(&source);a=project(true);
  CHECK(!memcmp(&e,&a,sizeof a));compare();
  for(unsigned reverse=0;reverse<2;reverse++) {
   seed_case(false,v,360);(reverse?ftCommon_8007F7B4:ftCommon_8007F76C)(&source);e=project(true);save();
   seed_case(true,v,360);(reverse?script_fn_side_reverse_velocity:script_fn_side_face_velocity)(&source);a=project(true);
   CHECK(!memcmp(&e,&a,sizeof a));compare();
  }
 }

 /* Full selector domain (including neutral), both live gun categories and
  * failure allocation in the composed explosion constructor. */
 const Vec2 sticks[]={{0,0},{1,0},{-1,0},{0,1},{0,-1}};
 for(unsigned selected=0;selected<5;selected++) for(unsigned held=0;held<2;held++)
 for(unsigned category=2;category<=3;category++) for(unsigned facing=0;facing<2;facing++) {
  Projection e,a;bool eb,ab;
  for(unsigned script=0;script<2;script++) {
   seed_case(script,0,341);fighter.facing_dir=facing?1:-1;
   fighter.input.lstick[0]=sticks[selected];fighter.input.cstick[0]=(Vec2){0,0};
   fighter.input.pressed_buttons=HSD_PAD_A;fighter.item_gobj=held?&object:NULL;
   use_category=category;fighter.u.pe.x4=2;setfloat("float_timer",2);
   if(!script){eb=ftPe_8011BE80(&source);e=project(false);save();}
   else{ab=script_fn_try_float_attack(&source);a=project(false);}
  }
  CHECK(eb==ab && !memcmp(&e,&a,sizeof a));compare();
 }
 for(unsigned v=0;v<64;v++) for(unsigned phase=0;phase<2;phase++) {
  unsigned id=phase?360:354;
  seed_case(false,v,id);allocation_failure=true;
  (phase?ftPe_SpecialAirSJump_Coll:ftPe_SpecialSStart_Anim)(&source);
  Projection e=project(true);save();
  seed_case(true,v,id);allocation_failure=true;
  generated_phase(id,phase?3:0)(&source);Projection a=project(true);
  CHECK(!memcmp(&e,&a,sizeof a));compare();
 }
 /* Valid recipient predicates retain exact old branch results and ordering. */
 for(unsigned allow=0;allow<2;allow++) for(unsigned v=0;v<32;v++) {
  Projection e,a;
  for(unsigned script=0;script<2;script++) {
   seed_case(script,v,360);fighter.x221C_b5=true;
   recipient_policy=(NativeFighterDefinition){.on_body_rebound=recipient_rebound};
   recipient_definition=&recipient_policy;contact_mutation=allow?0:9;
   if(!script){doAirEnd0(&source);e=project(true);save();}
   else{authored_hurtbox_detect(&source);a=project(true);}
  }
  CHECK(!memcmp(&e,&a,sizeof a));compare();
 }
 const FighterKind target_kinds[]={Ft_Kind_Peach,Ft_Kind_Mars,Ft_Kind_Emblem,Ft_Kind_Mario};
 for(unsigned k=0;k<4;k++) for(unsigned id=364;id<=372;id++) {
  Projection e,a;
  for(unsigned script=0;script<2;script++) {
   seed_case(script,0,360);fighter.x221C_b5=true;
   foreign.kind=target_kinds[k];foreign.motion_id=id;
   if(!script){doAirEnd0(&source);e=project(true);save();}
   else{authored_hurtbox_detect(&source);a=project(true);}
  }
  CHECK(!memcmp(&e,&a,sizeof a));compare();
 }
 /* Exact strict threshold and input equality cases are additional actual
  * landing/continue-input body comparisons. */
 const float edges[]={-.5f,-.3f,-.25f,0,.25f,.3f,.5f};
 for(unsigned i=0;i<7;i++) for(unsigned held=0;held<2;held++) {
  seed_case(false,0,341);fighter.self_vel.y=edges[i];ft_80082B1C(&source);Projection e=project(false);save();
  seed_case(true,0,341);fighter.self_vel.y=edges[i];script_fn_float_landing(&source);Projection a=project(false);
  CHECK(!memcmp(&e,&a,sizeof a));compare();
  seed_case(false,0,341);fighter.input.lstick[0].y=edges[i];fighter.input.held_buttons[0]=held?HSD_PAD_Y:0;
  bool eb=ftPe_Float_CheckContinueInput(&fighter);save();
  seed_case(true,0,341);fighter.input.lstick[0].y=edges[i];fighter.input.held_buttons[0]=held?HSD_PAD_Y:0;
  CHECK(eb==script_fn_continue_float_input(&source));compare();
 }
 /* Entry-hook policy checks are explicit expected conditions, not counted as
  * original-C pairs; root separately tests actual ordered common callers. */
 for(unsigned v=0;v<256;v++) {
  seed_case(true,v,29);
  bool expected_manual=fighter.input.lstick[0].y<=-common_fighter.x88 &&
      (fighter.input.held_buttons[0]&HSD_PAD_XY) && boolvar("has_float");
  CHECK(definition->on_aerial_action_manual(&source)==expected_manual);
  CHECK(!expected_manual || fighter.motion_id==341 && floatvar("float_timer")==attrs.xC && !boolvar("has_float"));
  seed_case(true,v,29);
  bool expected_auto=fighter.self_vel.y<=0 && boolvar("has_float") &&
     (fighter.input.lstick[0].y>=common_fighter.tap_jump_threshold || fighter.input.held_buttons[0]&HSD_PAD_XY);
  CHECK(definition->on_aerial_action_automatic(&source)==expected_auto);
  CHECK(!expected_auto || fighter.motion_id==341 && floatvar("float_timer")==attrs.xC && !boolvar("has_float"));
  definition->on_grounded_motion_setup(&source);CHECK(boolvar("has_float"));
 }
 /* Safety beyond valid retail contact pointers: real generation registry and
  * actual generic dispatcher; changed identity stops authored rebound tail. */
 for(int mutation=1;mutation<=9;mutation++) {
  seed_case(true,64,360);fighter.x221C_b5=true;
  recipient_policy=(NativeFighterDefinition){.on_body_rebound=recipient_rebound};
  recipient_definition=&recipient_policy;contact_mutation=mutation;
  Vec3 before=fighter.self_vel;int before_command=fighter.cmd_vars[2];
  authored_hurtbox_detect(&source);
  CHECK(!memcmp(&before,&fighter.self_vel,sizeof before) && fighter.cmd_vars[2]==before_command && fighter.motion_id==360);
  CHECK(count==1 && events[0].op==Interact);safety++;
 }
 seed_case(true,64,360);fighter.unk_gobj=NULL;authored_hurtbox_detect(&source);CHECK(fighter.motion_id==359);safety++;
 seed_case(true,64,360);source.next=NULL;authored_hurtbox_detect(&source);CHECK(fighter.motion_id==360 && count==0);safety++;

 for(unsigned recycled=0;recycled<2;recycled++) for(unsigned entry=0;entry<3;entry++) {
  seed_case(true,0,341);invalidate_fighter=recycled?2:1;
  if(entry==0) script_fn_enter_float(&source,true);
  else (entry==1?definition->special_s:definition->special_air_s)(&source);
  CHECK(count==1 && events[0].op==Motion);
  CHECK(!boolvar("has_float") && floatvar("float_timer")==-1);
  safety++;
 }
 printf("PASS %u original/generated comparisons, %u checks, %u authored safety cases\n",pairs,checks,safety);
 return 0;
}

