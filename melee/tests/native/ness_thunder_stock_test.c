// Actual original ball/trail/lbvector bodies link separately. Three fighter
// cleanup predicates and Dolphin's cross product are mechanically extracted.
// All external engine boundaries below are explicit; no game is launched.
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "behavior.generated.c"
#include "ness_thunder_raw.generated.h"
#include <melee/db/db.h>
#include <melee/ef/eflib.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/kinds/ftNess/ftness.h>
#include <melee/ft/kinds/ftNess/ftnessspecialhi.h>
#include <melee/ft/kinds/ftNess/types.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/gr/stage.h>
#include <melee/it/inlines.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_2725.h>
#include <melee/it/item.h>
#include <melee/it/itgroundcoll.h>
#include <melee/it/kinds/inlines.h>
#include <melee/it/kinds/itnesspkthunderball.h>
#include <melee/it/kinds/itnesspkthundertrail.h>
#include <melee/lb/lbvector.h>
#include <sysdolphin/baselib/jobj.h>

// Assertions remain active in original source; all vector inputs are nonNULL.
#ifndef ASSERTMSGLINE
#define ASSERTMSGLINE(line, condition, message)                               \
    do {                                                                      \
        if (!(condition))                                                     \
            abort();                                                          \
    } while (0)
#endif
#include "ness_thunder_original.generated.h"

enum {
    Create = 1,
    Motion,
    Life,
    Manual,
    Visible,
    Debug,
    Disable,
    Stage,
    CameraRelease,
    Effects,
    Rotate,
    Interact
};
typedef struct ThunderEvent {
    int op, a, b;
    float x, y, z, w;
} ThunderEvent;
static ThunderEvent events[2][128];
static unsigned counts[2], lane, checks, failures;
static Fighter fighters[2];
static HSD_GObj owners[2], foreign[2], objects[2][7];
static FighterBone bones[2][63];
static HSD_JObj bones_joints[2][63], models[2][7], children[2][7];
static Item items[2][7];
static Article article;
static ItemAttr common;
static itPKThunderAttributes tuning;
static SpawnItem spawns[2][7];
static unsigned spawn_count[2];
static const NativeFighterDefinition* definition;
static const NativeFighterArticleLogic* logics[5];
static const char* phase = "entry";
static bool allocation = true, controller = true;
static StKind stage_kind;
static unsigned child_slot[2];

static void unexpected(void)
{
    fprintf(stderr, "Unexpected Thunder boundary in %s lane%u child%u\n",
            phase, lane, child_slot[lane]);
    fprintf(stderr, "events=%u last=%d motion=%d\n", counts[lane],
            counts[lane] ? events[lane][counts[lane] - 1].op : 0,
            fighters[lane].motion_id);
    abort();
}
void __assert(char* f, u32 l, char* m)
{
    fprintf(stderr, "%s:%lu %s\n", f, l, m);
    abort();
}
void HSD_JObjSetMtxDirtySub(HSD_JObj* j)
{
    j->flags |= JOBJ_MTX_DIRTY;
}
static void record(int op, int a, int b, float x, float y, float z, float w)
{
    if (counts[lane] >= 128) {
        abort();
    }
    events[lane][counts[lane]++] = (ThunderEvent){ op, a, b, x, y, z, w };
}
static void check(bool condition, const char* name)
{
    ++checks;
    if (!condition) {
        if (failures < 25) {
            fprintf(stderr, "%s differs at check %u\n", name, checks);
        }
        ++failures;
    }
}
static void eqf(float a, float b, const char* name)
{
    bool same = (a == b) || (isnan(a) && isnan(b)) ||
                fabsf(a - b) <= 2e-6f * fmaxf(1, fmaxf(fabsf(a), fabsf(b)));
    if (!same && failures < 25)
        fprintf(stderr, "%s: original %.9g, authored %.9g (%s)\n",
                name, a, b, phase);
    check(same, name);
}
static uint32_t bits(float v)
{
    uint32_t b;
    memcpy(&b, &v, 4);
    return b;
}
static const NativeFighterStateField* find(const NativeFighterStateField* f,
                                           unsigned n, const char* name)
{
    for (unsigned i = 0; i < n; ++i) {
        if (!strcmp(f[i].name, name)) {
            return f + i;
        }
    }
    fprintf(stderr, "Missing state field: %s\n", name);
    unexpected();
    return NULL;
}
static const NativeFighterArticleLogic* item_logic(unsigned index)
{
    return index ? logics[index <= 3 ? 1 : index - 2] : logics[0];
}
static uint32_t get(unsigned index, const char* name, unsigned element)
{
    const NativeFighterArticleLogic* l = item_logic(index);
    const NativeFighterStateField* f =
        find(l->state_fields, l->state_field_count, name);
    if (element >= f->count) {
        unexpected();
    }
    uint32_t v = 0;
    memcpy(&v,
           items[1][index].xDD4_itemVar.native_article.state + f->offset +
               element * f->stride,
           f->stride);
    return v;
}
static void set(unsigned index, const char* name, unsigned element, uint32_t v)
{
    const NativeFighterArticleLogic* l = item_logic(index);
    const NativeFighterStateField* f =
        find(l->state_fields, l->state_field_count, name);
    if (element >= f->count) {
        unexpected();
    }
    memcpy(items[1][index].xDD4_itemVar.native_article.state + f->offset +
               element * f->stride,
           &v, f->stride);
}
static float value(unsigned index, const char* name, unsigned element)
{
    uint32_t b = get(index, name, element);
    float f;
    memcpy(&f, &b, 4);
    return f;
}
static void fighter_set(const char* name, uint32_t v)
{
    const NativeFighterStateField* f =
        find(definition->state_fields, definition->state_field_count, name);
    memcpy(fighters[1].native_module_state + f->offset, &v, f->stride);
}
static uint32_t fighter_get(const char* name)
{
    const NativeFighterStateField* f =
        find(definition->state_fields, definition->state_field_count, name);
    uint32_t v = 0;
    memcpy(&v, fighters[1].native_module_state + f->offset, f->stride);
    return v;
}
static void trace(void)
{
    check(counts[0] == counts[1], "ordered call count");
    if (counts[0] == counts[1]) {
        for (unsigned i = 0; i < counts[0]; ++i) {
            ThunderEvent* a = &events[0][i];
            ThunderEvent* b = &events[1][i];
            check(a->op == b->op && a->a == b->a && a->b == b->b,
                  "ordered call identity");
            eqf(a->x, b->x, "call x");
            eqf(a->y, b->y, "call y");
            eqf(a->z, b->z, "call z");
            eqf(a->w, b->w, "call w");
        }
    }
}
static void compare(unsigned i)
{
    Item* a = &items[0][i];
    Item* b = &items[1][i];
    check(a->msid == b->msid, "motion");
    eqf(a->xD44_lifeTimer, b->xD44_lifeTimer, "lifetime");
    eqf(a->xD48_halfLifeTimer, b->xD48_halfLifeTimer, "half lifetime");
    eqf(a->pos.x, b->pos.x, "position x");
    eqf(a->pos.y, b->pos.y, "position y");
    eqf(a->pos.z, b->pos.z, "position z");
    eqf(a->x40_vel.x, b->x40_vel.x, "velocity x");
    eqf(a->x40_vel.y, b->x40_vel.y, "velocity y");
    eqf(a->x40_vel.z, b->x40_vel.z, "velocity z");
    check((a->owner == &owners[0]) == (b->owner == &owners[1]),
          "current damage owner");
    check((a->owner == NULL) == (b->owner == NULL), "owner absence");
    check(a->xDC8_word.flags.x13 == b->xDC8_word.flags.x13, "owner bound");
    check(a->xDCC_flag.b3 == b->xDCC_flag.b3, "bounds flag");
    check(a->xDAC_itcmd_var0 == b->xDAC_itcmd_var0 &&
              a->xDB0_itcmd_var1 == b->xDB0_itcmd_var1 &&
              a->xDB4_itcmd_var2 == b->xDB4_itcmd_var2 &&
              a->xDB8_itcmd_var3 == b->xDB8_itcmd_var3,
          "commands");
    check((children[0][i].flags & JOBJ_HIDDEN) ==
              (children[1][i].flags & JOBJ_HIDDEN),
          "recursive child visibility");
    eqf(models[0][i].scale.z, models[1][i].scale.z, "trail root radius");
    eqf(models[0][i].rotate.x, models[1][i].rotate.x, "trail root rotation");
    if (i) {
        check((a->xDD4_itemVar.nesspkthundertrail.x0 != NULL) ==
                  (get(i, "parent_enabled", 0) != 0),
              "trail parent enable");
        check((uint32_t) a->xDD4_itemVar.nesspkthundertrail.x4 ==
                  get(i, "sample_index", 0),
              "trail index");
        check((uint32_t) a->xDD4_itemVar.nesspkthundertrail.x8 ==
                  get(i, "blink_frame", 0),
              "trail blink");
    } else {
        check((a->xDD4_itemVar.pkthunder.xF04 != NULL) ==
                  (get(0, "controller_enabled", 0) != 0),
              "ball controller");
        check((uint32_t) a->xDD4_itemVar.pkthunder.xF00 ==
                  get(0, "reflected", 0),
              "ball reflected");
        check((uint32_t) a->xDD4_itemVar.pkthunder.xEF8 == get(0, "age", 0),
              "ball age");
        check((uint32_t) a->xDD4_itemVar.pkthunder.xEFC ==
                  get(0, "next_trail", 0),
              "next trail");
        eqf(a->xDD4_itemVar.pkthunder.xEEC, value(0, "speed", 0),
            "ball speed");
        for (unsigned j = 0; j < 16; ++j) {
            Vec3* v = &a->xDD4_itemVar.pkthunder.positions[j];
            eqf(v->x, value(0, "history_x", j), "history x");
            eqf(v->y, value(0, "history_y", j), "history y");
            eqf(v->z, value(0, "history_z", j), "history z");
            eqf(a->xDD4_itemVar.pkthunder.angles[j], value(0, "angles", j),
                "angle history");
        }
        for (unsigned j = 0; j < 6; ++j) {
            check((a->xDD4_itemVar.pkthunder.xDD4[j] != NULL) ==
                      (get(0, "trail_handles", j) != UINT32_MAX),
                  "trail handle presence");
        }
    }
}
static void compare_fighter(void)
{
    check((fighters[0].u.ns.pkthunder_gobj != NULL) ==
              (fighter_get("thunder_ball_handle") != UINT32_MAX),
          "fighter ball handle");
    check((fighters[0].u.ns.pkthunder_gfx != 0) ==
              (fighter_get("thunder_effects_active") != 0),
          "fighter effect flag");
    check((fighters[0].death2_cb != NULL) == (fighters[1].death2_cb != NULL) &&
              (fighters[0].take_dmg_cb != NULL) ==
                  (fighters[1].take_dmg_cb != NULL),
          "cleanup callbacks");
    eqf(bones_joints[0][0].rotate.x, bones_joints[1][0].rotate.x,
        "fighter root rotation");
}
void HSD_JObjSetFlagsAll(HSD_JObj* j, u32 flags)
{
    if (!j) {
        return;
    }
    j->flags |= flags;
    HSD_JObjSetFlagsAll(j->child, flags);
    HSD_JObjSetFlagsAll(j->next, flags);
}
void HSD_JObjClearFlagsAll(HSD_JObj* j, u32 flags)
{
    if (!j) {
        return;
    }
    j->flags &= ~flags;
    HSD_JObjClearFlagsAll(j->child, flags);
    HSD_JObjClearFlagsAll(j->next, flags);
}
void it_80272940(HSD_GObj* g)
{
    HSD_JObjClearFlagsAll(((HSD_JObj*) g->hsd_obj)->child, JOBJ_HIDDEN);
    record(Visible, 1, 0, 0, 0, 0, 0);
}
void Item_80268E5C(HSD_GObj* g, enum_t motion, Item_StateChangeFlags flags)
{
    ((Item*) g->user_data)->msid = motion;
    record(Motion, motion, flags, 0, 0, 0, 0);
}
void it_80275158(HSD_GObj* g, float life)
{
    Item* i = g->user_data;
    i->xD44_lifeTimer = life;
    i->xD48_halfLifeTimer = life * .5f;
    record(Life, 0, 0, life, 0, 0, 0);
}
void it_8026B3A8(HSD_GObj* g)
{
    (void) g;
    record(Manual, 0, 0, 0, 0, 0, 0);
}
void db_80225DD8(HSD_GObj* g, HSD_GObj* owner)
{
    (void) g;
    record(Debug, owner == &owners[lane], 0, 0, 0, 0, 0);
}
void it_802725D4(HSD_GObj* g)
{
    (void) g;
    record(Disable, 0, 0, 0, 0, 0, 0);
}
bool it_80273130(HSD_GObj* g)
{
    return Item_TickLifetime(g->user_data);
}
bool it_8026DA08(HSD_GObj* g)
{
    (void) g;
    record(Stage, 0, 0, 0, 0, 0, 0);
    return false;
}
void Item_80267454(Item_GObj* g)
{
    (void) g;
    record(CameraRelease, 0, 0, 0, 0, 0, 0);
}
StKind Stage_80225194(void)
{
    return stage_kind;
}
void efLib_DestroyAll(HSD_GObj* g)
{
    (void) g;
    record(Effects, 0, 0, 0, 0, 0, 0);
}
void ftPartSetRotX(Fighter* f, int part, float angle)
{
    f->parts[part].joint->rotate.x = angle;
    record(Rotate, part, 0, angle, 0, 0, 0);
}
void it_8026BB68(HSD_GObj* owner, Vec3* out)
{
    *out = ((Fighter*) owner->user_data)->cur_pos;
}
u32 Item_8026AE60(void)
{
    return UINT32_C(0xf0123456);
}
void ftLib_800865D8(HSD_GObj* g, float* x, float* y)
{
    Fighter* f = g->user_data;
    *x = f->input.lstick[0].x;
    *y = f->input.lstick[0].y;
}
bool it_8026B894(HSD_GObj* g, HSD_GObj* other)
{
    (void) g;
    (void) other;
    record(Interact, 0, 0, 0, 0, 0, 0);
    return false;
}
static bool alive(HSD_GObj* g, uint32_t id, uint32_t cls)
{
    if (cls == NativeCallbackSource_Fighter) {
        return g == &owners[1] && fighters[1].x8_spawnNum == id;
    }
    if (cls == NativeCallbackSource_Item) {
        for (unsigned i = 0; i < 7; ++i) {
            if (g == &objects[1][i]) {
                return items[1][i].x1C == id;
            }
        }
    }
    return false;
}
static Fighter_GObj* creator(Item_GObj* g)
{
    Item* i = g->user_data;
    return controller &&
                   i->xDD4_itemVar.native_article.attachment_owner ==
                       &owners[1] &&
                   i->xDD4_itemVar.native_article.creator_spawn_id ==
                       fighters[1].x8_spawnNum
               ? &owners[1]
               : NULL;
}
static uint32_t current_owner(Item_GObj* g)
{
    return ((Item*) g->user_data)->owner == &owners[1]
               ? fighters[1].x8_spawnNum
               : 0;
}
static unsigned bone_count(Fighter* f)
{
    (void) f;
    return 63;
}
static bool signal_article(HSD_GObj* caller, uint32_t id, uint32_t name,
                           float payload)
{
    (void) caller;
    for (unsigned i = 0; i < 7; ++i) {
        if (id == items[1][i].x1C) {
            const NativeFighterArticleLogic* l = item_logic(i);
            for (unsigned s = 0; s < l->signal_count; ++s) {
                if (l->signals[s].name_hash == name) {
                    l->signals[s].callback(&objects[1][i], payload);
                    return true;
                }
            }
        }
    }
    return false;
}
static uint32_t signal_hash(const char* name)
{
    uint32_t h = UINT32_C(2166136261);
    for (; *name; ++name) {
        h ^= (unsigned char) *name;
        h *= UINT32_C(16777619);
    }
    return h;
}
static void signal(unsigned i, const char* name)
{
    const NativeFighterArticleLogic* l = item_logic(i);
    for (unsigned s = 0; s < l->signal_count; ++s) {
        if (l->signals[s].name_hash == signal_hash(name)) {
            l->signals[s].callback(&objects[1][i], 0);
            return;
        }
    }
    unexpected();
}
static int32_t link_id(Item_GObj* g, unsigned slot)
{
    if (slot) {
        unexpected();
    }
    return ((Item*) g->user_data)->native_article_links[0];
}
static float ref_float(HSD_GObj* caller, uint32_t id, ItemKind kind,
                       unsigned field, int32_t index)
{
    (void) caller;
    if (id != items[1][0].x1C || kind != It_Kind_Ness_PKThunder ||
        field >= logics[0]->state_field_count)
    {
        return 0;
    }
    const NativeFighterStateField* f = &logics[0]->state_fields[field];
    if (index < 0 || (unsigned) index >= f->count) {
        return 0;
    }
    float v;
    memcpy(&v,
           items[1][0].xDD4_itemVar.native_article.state + f->offset +
               index * f->stride,
           4);
    return v;
}
static void joint_write(Item_GObj* g, unsigned model, const char* path,
                        unsigned channel, float v)
{
    if (model) {
        unexpected();
    }
    HSD_JObj* j = g->hsd_obj;
    if (!strcmp(path, "0") && channel == 10) {
        if (v) {
            it_80272940(g);
        } else {
            HSD_JObjSetFlagsAll(j->child, JOBJ_HIDDEN);
        }
        return;
    }
    if (*path) {
        unexpected();
    }
    if (channel == 8) {
        HSD_JObjSetScaleZ(j, v);
    } else if (channel == 3) {
        HSD_JObjSetRotationX(j, v);
    } else {
        unexpected();
    }
}
static bool shield_bounce(Item_GObj* g)
{
    Item* i = g->user_data;
    lbVector_Mirror(&i->x40_vel, &i->xC58);
    return false;
}
HSD_GObj* Item_80268B18(SpawnItem* spawn)
{
    unsigned index =
        spawn->kind == It_Kind_Ness_PKThunder ? 0 : ++child_slot[lane];
    if (index > 6) {
        unexpected();
    }
    spawns[lane][spawn_count[lane]++] = *spawn;
    record(Create, spawn->kind, spawn->x40, spawn->prev_pos.x,
           spawn->prev_pos.y, spawn->prev_pos.z, spawn->facing_dir);
    if (!allocation) {
        return NULL;
    }
    Item* i = &items[lane][index];
    i->kind = spawn->kind;
    i->pos = spawn->pos;
    i->x40_vel = spawn->vel;
    i->owner = spawn->x0_parent_gobj;
    i->facing_dir = spawn->facing_dir;
    i->xAC4_ignoreItemID = spawn->x40;
    return &objects[lane][index];
}
static int32_t create_article(Fighter_GObj* owner, ItemKind kind,
                              Vec3* position, Vec3* vel, float facing,
                              bool origin, uint32_t group, float value, bool initial_collision)
{
    SpawnItem spawn = { 0 };
    spawn.kind = kind;
    Item_InitSpawn(&spawn, owner, position, facing);
    spawn.vel = *vel;
    spawn.x40 = group;
    spawn.x44_flag.b0 = initial_collision;
    if (origin) {
        it_8026BB68(owner, &spawn.pos);
    }
    HSD_GObj* g = Item_80268B18(&spawn);
    if (!g) {
        return -1;
    }
    Item* i = g->user_data;
    i->xDD4_itemVar.native_article.attachment_owner = owner;
    i->xDD4_itemVar.native_article.creator_spawn_id = fighters[1].x8_spawnNum;
    i->xDD4_itemVar.native_article.creation_value = value;
    logics[0]->on_created(g);
    return i->x1C;
}
static int32_t create_child(Item_GObj* parent, ItemKind kind, Vec3* p,
                            float facing, uint32_t owner, uint32_t group,
                            float value, unsigned backlink)
{
    if (backlink) {
        unexpected();
    }
    SpawnItem spawn = { 0 };
    spawn.kind = kind;
    spawn.pos = spawn.prev_pos = *p;
    spawn.facing_dir = facing;
    spawn.x0_parent_gobj =
        owner == fighters[1].x8_spawnNum ? &owners[1] : NULL;
    spawn.x4_parent_gobj2 = parent;
    spawn.x40 = group;
    spawn.x44_flag.b0 = true;
    HSD_GObj* g = Item_80268B18(&spawn);
    if (!g) {
        return -1;
    }
    Item* i = g->user_data;
    i->native_article_links[0] = items[1][0].x1C;
    i->xDD4_itemVar.native_article.attachment_owner = &owners[1];
    i->xDD4_itemVar.native_article.creator_spawn_id = fighters[1].x8_spawnNum;
    i->xDD4_itemVar.native_article.creation_value = value;
    item_logic(child_slot[lane])->on_created(g);
    return i->x1C;
}
static void dummy(HSD_GObj* g)
{
    (void) g;
}
static void reset(int motion, float facing)
{
    memset(items, 0, sizeof(items));
    memset(fighters, 0, sizeof(fighters));
    memset(models, 0, sizeof(models));
    memset(children, 0, sizeof(children));
    memset(counts, 0, sizeof(counts));
    memset(spawn_count, 0, sizeof(spawn_count));
    allocation = controller = true;
    stage_kind = 0;
    child_slot[0] = child_slot[1] = 0;
    for (unsigned l = 0; l < 2; ++l) {
        owners[l].user_data = &fighters[l];
        Fighter* f = &fighters[l];
        f->parts = bones[l];
        f->x8_spawnNum = UINT32_C(0xf1234567);
        f->motion_id = motion;
        f->cur_pos = (Vec3){ 10, 20, 3 };
        f->death2_cb = f->take_dmg_cb = dummy;
        f->u.ns.pkthunder_gfx = true;
        for (unsigned j = 0; j < 63; ++j) {
            bones[l][j].joint = &bones_joints[l][j];
            bones_joints[l][j].rotate.x = 2;
        }
        for (unsigned i = 0; i < 7; ++i) {
            Item* p = &items[l][i];
            objects[l][i].user_data = p;
            objects[l][i].hsd_obj = &models[l][i];
            models[l][i].child = &children[l][i];
            models[l][i].scale = (Vec3){ 1, 1, 1 };
            p->entity = &objects[l][i];
            p->owner = &owners[l];
            p->kind = i ? i <= 3 ? It_Kind_Ness_PKThunder1
                                 : It_Kind_Ness_PKThunder2 + i - 4
                        : It_Kind_Ness_PKThunder;
            p->x1C = UINT32_C(0xfedcba98) + i;
            p->xC4_article_data = &article;
            p->xCC_item_attr = &common;
            p->facing_dir = facing;
            p->xDC8_word.flags.x13 = 1;
            p->xDCC_flag.b3 = 1;
            p->xAC4_ignoreItemID = UINT32_C(0xf0123456);
            p->pos = (Vec3){ 7, 9, 1 };
            p->xD44_lifeTimer = 20;
            p->x40_vel = (Vec3){ 1, 2, 0 };
            if (l == 1) {
                p->xDD4_itemVar.native_article.attachment_owner = &owners[l];
                p->xDD4_itemVar.native_article.creator_spawn_id =
                    f->x8_spawnNum;
                p->native_article_links[0] = UINT32_C(0xfedcba98);
            }
        }
    }
    fighters[0].u.ns.pkthunder_gobj = &objects[0][0];
    fighter_set("thunder_ball_handle", items[1][0].x1C);
    fighter_set("thunder_effects_active", 1);
    Item* ball = &items[0][0];
    ball->xDD4_itemVar.pkthunder.xF04 = &owners[0];
    ball->xDD4_itemVar.pkthunder.xEEC = tuning.x4_PKTHUNDER_SPEED;
    set(0, "controller_enabled", 0, 1);
    set(0, "speed", 0, bits(tuning.x4_PKTHUNDER_SPEED));
    for (unsigned j = 0; j < 16; ++j) {
        ball->xDD4_itemVar.pkthunder.positions[j] =
            (Vec3){ j * .5f, j * -.25f, j * .125f };
        ball->xDD4_itemVar.pkthunder.angles[j] = .35f;
        set(0, "history_x", j, bits(j * .5f));
        set(0, "history_y", j, bits(j * -.25f));
        set(0, "history_z", j, bits(j * .125f));
        set(0, "angles", j, bits(.35f));
    }
    for (unsigned j = 0; j < 6; ++j) {
        set(0, "trail_handles", j, UINT32_MAX);
    }
    for (unsigned i = 1; i < 7; ++i) {
        items[0][i].xDD4_itemVar.nesspkthundertrail.x0 = &objects[0][0];
        items[0][i].xDD4_itemVar.nesspkthundertrail.x4 = i - 1;
        set(i, "parent_enabled", 0, 1);
        set(i, "sample_index", 0, i - 1);
    }
}
static void attach_trails(void)
{
    for (unsigned j = 0; j < 6; ++j) {
        items[0][0].xDD4_itemVar.pkthunder.xDD4[j] = &objects[0][j + 1];
        set(0, "trail_handles", j, items[1][j + 1].x1C);
    }
}
static void compare_spawns(void)
{
    check(spawn_count[0] == spawn_count[1], "spawn count");
    for (unsigned n = 0; n < spawn_count[0] && n < spawn_count[1]; ++n) {
        SpawnItem* a = &spawns[0][n];
        SpawnItem* b = &spawns[1][n];
        check(a->kind == b->kind && a->x40 == b->x40, "spawn kind/group");
        check(a->x44_flag.b0 == b->x44_flag.b0, "spawn interaction flag");
        eqf(a->facing_dir, b->facing_dir, "spawn facing");
        eqf(a->x3C_damage, b->x3C_damage, "spawn damage");
        eqf(a->pos.x, b->pos.x, "spawn pos x");
        eqf(a->pos.y, b->pos.y, "spawn pos y");
        eqf(a->pos.z, b->pos.z, "spawn pos z");
        check(!memcmp(&a->prev_pos, &b->prev_pos, sizeof(Vec3)),
              "spawn previous origin");
        check(!memcmp(&a->vel, &b->vel, sizeof(Vec3)), "spawn velocity");
        check((a->x0_parent_gobj == NULL) == (b->x0_parent_gobj == NULL),
              "spawn damage owner");
        check((a->x4_parent_gobj2 == &objects[0][0]) ==
                  (b->x4_parent_gobj2 == &objects[1][0]),
              "spawn ball parent");
    }
}
static char* read(const char* path, size_t* n)
{
    FILE* f = fopen(path, "rb");
    if (!f) {
        abort();
    }
    fseek(f, 0, SEEK_END);
    *n = ftell(f);
    rewind(f);
    char* p = malloc(*n + 1);
    if (!p || fread(p, 1, *n, f) != *n) {
        abort();
    }
    p[*n] = 0;
    fclose(f);
    return p;
}
#ifdef THUNDER_FIGHTER_CASES
#include "ness_thunder_fighter_cases.h"
#endif

int main(int argc, char** argv)
{
    if (argc != 2) {
        return 2;
    }
    size_t n;
    char* text = read(argv[1], &n);
    NativeFighterParameters params;
    unsigned line = 0;
    if (!nativeFighterParameters_Parse(text, n, &params, &line)) {
        return 2;
    }
    free(text);
    uint32_t values[NATIVE_FIGHTER_PARAMETER_CAPACITY];
    for (unsigned i = 0; i < params.count; ++i) {
        values[i] = params.fields[i].bits;
    }
    if (!FreakMelee_SetFighterParameters(values, params.count,
                                         params.schema_hash))
    {
        return 2;
    }
    MotionState motions[36] = { 0 };
    NativeFighterDefinition base = { .code_key = "ness_clone",
                                     .source_kind = Ft_Kind_Ness,
                                     .action_count = 326,
                                     .joint_count = 63,
                                     .motion_state_first = 341,
                                     .motion_state_count = 36,
                                     .motion_states = motions };
    base.special_ground_state[0] = 348;
    base.special_air_state[0] = 352;
    base.special_ground_state[1] = 356;
    base.special_air_state[1] = 357;
    base.special_ground_state[2] = 358;
    base.special_air_state[2] = 362;
    base.special_ground_state[3] = 367;
    base.special_air_state[3] = 372;
    NativeFighterHost host = { 0 };
#include "ness_thunder_required_host.generated.h"
    host.callback_source_alive = alive;
    host.item_creator = creator;
    host.item_current_owner = current_owner;
    host.article_ref_float = ref_float;
    host.signal_article = signal_article;
    host.item_link_id = link_id;
    host.create_article_at = create_article;
    host.item_create_child = create_child;
    host.new_article_collision_group = Item_8026AE60;
    host.bone_count = bone_count;
    host.destroy_effects = efLib_DestroyAll;
    host.set_root_rotation_x = ftPartSetRotX;
    host.item_set_lifetime = it_80275158;
    host.item_change_state_flags = Item_80268E5C;
    host.item_use_manual_lifetime = it_8026B3A8;
    host.item_set_joint_value = joint_write;
    host.item_set_debug_owner = db_80225DD8;
    host.item_disable_hitboxes = it_802725D4;
    host.item_expire_tick = it_80273130;
    host.item_stage_collision_land = it_8026DA08;
    host.item_shield_bounce = shield_bounce;
    host.item_interact = it_8026B894;
    host.item_release_camera = Item_80267454;
    host.stage_internal_kind = Stage_80225194;
    host.vector_angle = lbVector_Angle;
    host.vector_mirror = lbVector_Mirror;
    host.vector_cross_normalized = lbVector_CrossprodNormalized;
    host.cos_float = cosf;
    host.sin_float = sinf;
    host.angle_atan2_float = atan2f;
    host.abs_float = fabsf;
#ifdef THUNDER_FIGHTER_CASES
    configure_fighter_host(&host);
#endif
    definition = FreakMelee_GetFighterModule(&base, &host);
    if (!definition) {
        return 2;
    }
    for (unsigned i = 0; i < definition->article_logic_count; ++i) {
        for (unsigned j = 0; j < 5; ++j) {
            if (definition->article_logics[i].kind ==
                It_Kind_Ness_PKThunder + j)
            {
                logics[j] = &definition->article_logics[i];
            }
        }
    }
    for (unsigned j = 0; j < 5; ++j) {
        if (!logics[j]) {
            return 2;
        }
    }
    memcpy(&tuning, thunder_raw, sizeof(thunder_raw));
    article.x4_specialAttributes = &tuning;
    phase = "constructors";
    // All three generated component queries call the same actual original
    // core.
    for (int ax = -2; ax <= 2; ++ax) {
        for (int by = -2; by <= 2; ++by) {
            for (unsigned zero = 0; zero < 2; ++zero) {
                reset(359, 1);
                Vec3 a = { ax, zero ? 0 : 1, zero ? 0 : -.5f },
                     b = { zero ? 0 : 2, by, zero ? 0 : .25f }, expected;
                lbVector_CrossprodNormalized(&a, &b, &expected);
                lane = 1;
                script_fn_thunder_cross_probe(&owners[1], a.x, a.y, a.z, b.x,
                                              b.y, b.z);
                uint32_t x = fighter_get("thunder_probe_x"),
                         y = fighter_get("thunder_probe_y"),
                         z = fighter_get("thunder_probe_z");
                check(x == bits(expected.x) && y == bits(expected.y) &&
                          z == bits(expected.z),
                      "all normalized-cross components exact");
            }
        }
    }
    // Constructors: success/failure, both facings, normal/exception camera
    // stage.
    for (unsigned face = 0; face < 2; ++face) {
        for (unsigned alloc = 0; alloc < 2; ++alloc) {
            for (unsigned camera = 0; camera < 2; ++camera) {
                reset(359, face ? -1 : 1);
                allocation = alloc;
                stage_kind = camera ? 84 : 0;
                Vec3 p = { 4, 6, 2 };
                lane = 0;
                HSD_GObj* a = it_802AB58C(&owners[0], &p, face ? -1 : 1);
                lane = 1;
                script_fn_thunder_create_ball(&owners[1], p.x, p.y, p.z,
                                              face ? -1 : 1);
                check((a != NULL) ==
                          (fighter_get("thunder_ball_handle") != UINT32_MAX),
                      "ball constructor failure");
                trace();
                compare_spawns();
                if (alloc) {
                    compare(0);
                }
            }
        }
    }
    // Direct constructor covers each indexed kind, owner absence and both
    // facings.
    static const ItemKind trail_kinds[] = {
        It_Kind_Ness_PKThunder1, It_Kind_Ness_PKThunder1,
        It_Kind_Ness_PKThunder1, It_Kind_Ness_PKThunder2,
        It_Kind_Ness_PKThunder3, It_Kind_Ness_PKThunder4
    };
    for (unsigned index = 0; index < 6; ++index) {
        for (unsigned face = 0; face < 2; ++face) {
            for (unsigned owner = 0; owner < 2; ++owner) {
                for (unsigned alloc = 0; alloc < 2; ++alloc) {
                    reset(359, face ? -1 : 1);
                    allocation = alloc;
                    child_slot[0] = child_slot[1] = index;
                    Vec3 p = { 4, 6, 0 };
                    lane = 0;
                    HSD_GObj* a = it_802AC43C(
                        owner ? &owners[0] : NULL, &objects[0][0], &p, index,
                        UINT32_C(0xf0123456), face ? -1 : 1);
                    lane = 1;
                    int32_t b = create_child(
                        &objects[1][0], trail_kinds[index], &p, face ? -1 : 1,
                        owner ? fighters[1].x8_spawnNum : 0,
                        UINT32_C(0xf0123456), (float) index, 0);
                    check((a != NULL) == (b != -1),
                          "trail constructor allocation");
                    trace();
                    compare_spawns();
                    if (alloc) {
                        compare(index + 1);
                    }
                }
            }
        }
    }
    phase = "spawn sequence";
    // Six-attempt ordered spawn sequence, reflected/unowned cases, failure
    // consumes attempt.
    for (unsigned reflected = 0; reflected < 2; ++reflected) {
        for (unsigned owner = 0; owner < 3; ++owner) {
            for (unsigned alloc = 0; alloc < 2; ++alloc) {
                reset(359, -1);
                allocation = alloc;
                items[0][0].xDD4_itemVar.pkthunder.xF00 = reflected;
                set(0, "reflected", 0, reflected);
                if (reflected) {
                    items[0][0].xDD4_itemVar.pkthunder.xF04 = NULL;
                    set(0, "controller_enabled", 0, 0);
                }
                for (unsigned l = 0; l < 2; ++l) {
                    if (owner) {
                        items[l][0].owner = owner == 1 ? NULL : &foreign[l];
                    }
                }
                for (unsigned age = 0; age < 12; ++age) {
                    lane = 0;
                    bool a =
                        itNesspkthunderball_UnkMotion0_Anim(&objects[0][0]);
                    lane = 1;
                    bool b =
                        logics[0]->logic->states[0].animated(&objects[1][0]);
                    check(a == b, "animation removal");
                    compare(0);
                    if (a) {
                        break;
                    }
                }
                trace();
                compare_spawns();
                for (unsigned i = 1; i <= child_slot[lane]; ++i) {
                    if (alloc) {
                        compare(i);
                    }
                }
            }
        }
    }
    // Nonspawning age and lifetime boundaries, invalid hold motion, inactive
    // control.
    for (int age = -1; age <= 13; age += 7) {
        for (int life = -1; life <= 2; ++life) {
            for (unsigned enabled = 0; enabled < 2; ++enabled) {
                for (unsigned hold = 0; hold < 2; ++hold) {
                    reset(hold ? 359 : 360, 1);
                    items[0][0].xDD4_itemVar.pkthunder.xEF8 = age;
                    set(0, "age", 0, age);
                    for (unsigned l = 0; l < 2; ++l) {
                        items[l][0].xD44_lifeTimer = life + .25f;
                    }
                    if (!enabled) {
                        items[0][0].xDD4_itemVar.pkthunder.xF04 = NULL;
                        set(0, "controller_enabled", 0, 0);
                    }
                    lane = 0;
                    bool a =
                        itNesspkthunderball_UnkMotion0_Anim(&objects[0][0]);
                    lane = 1;
                    bool b =
                        logics[0]->logic->states[0].animated(&objects[1][0]);
                    check(a == b, "age/lifetime removal");
                    compare(0);
                    trace();
                }
            }
        }
    }
    phase = "collision";
    // Original stage-contact truth table including zero signs.
    for (unsigned flags = 0; flags < 16; ++flags) {
        for (int x = -1; x <= 1; ++x) {
            for (int y = -1; y <= 1; ++y) {
                reset(359, 1);
                for (unsigned l = 0; l < 2; ++l) {
                    items[l][0].x40_vel = (Vec3){ x, y, 0 };
                    items[l][0].x378_itemColl.env_flags =
                        (flags & 1 ? Collide_FloorMask : 0) |
                        (flags & 2 ? Collide_CeilingMask : 0) |
                        (flags & 4 ? Collide_LeftWallMask : 0) |
                        (flags & 8 ? Collide_RightWallMask : 0);
                }
                lane = 0;
                bool a = it_803F6BC8[0].collided(&objects[0][0]);
                lane = 1;
                bool b = logics[0]->logic->states[0].collided(&objects[1][0]);
                check(a == b, "collision removal");
                trace();
                compare(0);
            }
        }
    }
    phase = "steering";
    // Steering through threshold, both signs, obtuse/acute/parallel/zero and
    // inactive paths.
    for (unsigned mode = 0; mode < 4; ++mode) {
        for (int sx = -8; sx <= 8; ++sx) {
            for (int sy = -8; sy <= 8; ++sy) {
                reset(mode == 3 ? 360 : 359, 1);
                for (unsigned l = 0; l < 2; ++l) {
                    fighters[l].input.lstick[0].x = sx * .125f;
                    fighters[l].input.lstick[0].y = sy * .125f;
                    items[l][0].x40_vel =
                        (Vec3){ sx & 1 ? 2 : 0, sy & 1 ? 2 : 0, .25f };
                }
                if (mode == 1) {
                    items[0][0].xDD4_itemVar.pkthunder.xF00 = 1;
                    set(0, "reflected", 0, 1);
                }
                if (mode == 2) {
                    for (unsigned l = 0; l < 2; ++l) {
                        items[l][0].owner = &foreign[l];
                    }
                }
                lane = 0;
                itNesspkthunderball_UnkMotion0_Phys(&objects[0][0]);
                lane = 1;
                logics[0]->logic->states[0].physics_updated(&objects[1][0]);
                compare(0);
            }
        }
    }
    phase = "trail transforms";
    // Every trail kind, facing, sample and blink state; exact root/child
    // transforms.
    for (unsigned i = 1; i < 7; ++i) {
        for (unsigned face = 0; face < 2; ++face) {
            for (unsigned blink = 0; blink < 4; ++blink) {
                for (unsigned parent = 0; parent < 2; ++parent) {
                    reset(359, face ? -1 : 1);
                    if (!parent) {
                        items[0][i].xDD4_itemVar.nesspkthundertrail.x0 = NULL;
                        set(i, "parent_enabled", 0, 0);
                    }
                    items[0][i].xDD4_itemVar.nesspkthundertrail.x8 = blink;
                    set(i, "blink_frame", 0, blink);
                    lane = 0;
                    bool a =
                        itNesspkthundertrail_UnkMotion0_Anim(&objects[0][i]);
                    itNesspkthundertrail_UnkMotion0_Phys(&objects[0][i]);
                    lane = 1;
                    bool b = item_logic(i)->logic->states[0].animated(
                        &objects[1][i]);
                    item_logic(i)->logic->states[0].physics_updated(
                        &objects[1][i]);
                    check(a == b, "trail removal");
                    compare(i);
                }
            }
        }
    }
    phase = "reflection/detach";
    // Reflection resets chain and history while damage detachment only clears
    // owner.
    for (unsigned owner = 0; owner < 3; ++owner) {
        for (unsigned reflected = 0; reflected < 2; ++reflected) {
            reset(363, 1);
            attach_trails();
            for (unsigned l = 0; l < 2; ++l) {
                if (owner) {
                    items[l][0].owner = owner == 1 ? NULL : &foreign[l];
                }
            }
            items[0][0].xDD4_itemVar.pkthunder.xF00 = reflected;
            set(0, "reflected", 0, reflected);
            lane = 0;
            it_802AB9C0(&objects[0][0]);
            lane = 1;
            signal(0, "detach_damage_owner");
            compare(0);
            for (unsigned i = 1; i < 7; ++i) {
                compare(i);
            }
            trace();
            reset(363, 1);
            attach_trails();
            for (unsigned l = 0; l < 2; ++l) {
                if (owner) {
                    items[l][0].owner = owner == 1 ? NULL : &foreign[l];
                }
            }
            lane = 0;
            bool a = it_802AC098(&objects[0][0]);
            lane = 1;
            bool b = logics[0]->logic->reflected(&objects[1][0]);
            check(a == b, "reflection return");
            compare(0);
            compare_fighter();
            for (unsigned i = 1; i < 7; ++i) {
                compare(i);
            }
            trace();
        }
    }
    phase = "cleanup";
    // All cleanup notifications and creator motion ranges.
    HSD_GObjPredicate original[] = { itNessPKThunderball_Logic26_DmgDealt,
                                     it_802AC074, it_802AC338, it_802AC3F8 };
    HSD_GObjPredicate scripted[] = { logics[0]->logic->dmg_dealt,
                                     logics[0]->logic->clanked,
                                     logics[0]->logic->absorbed,
                                     logics[0]->logic->hit_shield };
    for (unsigned n = 0; n < 4; ++n) {
        for (int motion = 357; motion <= 367; ++motion) {
            for (unsigned owner = 0; owner < 2; ++owner) {
                reset(motion, 1);
                attach_trails();
                if (owner) {
                    for (unsigned l = 0; l < 2; ++l) {
                        items[l][0].owner = &foreign[l];
                    }
                }
                lane = 0;
                bool a = original[n](&objects[0][0]);
                lane = 1;
                bool b = scripted[n](&objects[1][0]);
                check(a == b, "cleanup return");
                compare(0);
                compare_fighter();
                for (unsigned i = 1; i < 7; ++i) {
                    compare(i);
                }
                trace();
            }
        }
    }
    for (unsigned owner = 0; owner < 3; ++owner) {
        for (unsigned reflected = 0; reflected < 2; ++reflected) {
            reset(359, 1);
            attach_trails();
            for (unsigned l = 0; l < 2; ++l) {
                if (owner) {
                    items[l][0].owner = owner == 1 ? NULL : &foreign[l];
                }
            }
            items[0][0].xDD4_itemVar.pkthunder.xF00 = reflected;
            set(0, "reflected", 0, reflected);
            lane = 0;
            it_802AB90C(&objects[0][0]);
            lane = 1;
            logics[0]->logic->destroyed(&objects[1][0]);
            compare(0);
            compare_fighter();
            for (unsigned i = 1; i < 7; ++i) {
                compare(i);
            }
            trace();
        }
    }
    phase = "shield mirroring";
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            reset(359, 1);
            for (unsigned l = 0; l < 2; ++l) {
                items[l][0].xC58 = (Vec3){ x * .5f, y * .5f, 0 };
            }
            lane = 0;
            bool a = it_802AC35C(&objects[0][0]);
            lane = 1;
            bool b = logics[0]->logic->shield_bounced(&objects[1][0]);
            check(a == b, "shield return");
            compare(0);
        }
    }
    reset(359, 1);
    lane = 0;
    it_802AC41C(&objects[0][0], &objects[0][1]);
    lane = 1;
    logics[0]->logic->evt_unk(&objects[1][0], &objects[1][1]);
    trace();
#ifdef THUNDER_FIGHTER_CASES
    fighter_cases(&params);
#endif
    printf("Ness Thunder articles/fighter: %u checks, %u failures\n", checks,
           failures);
    return failures ? 1 : 0;
}
