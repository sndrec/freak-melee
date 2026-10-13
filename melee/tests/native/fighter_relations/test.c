#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <melee/ft/inlines.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/ftcommon_assets.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_relations.h>
#include <melee/ft/native_fighter_target.h>
#include <melee/ef/native_model_effect.h>
#include <melee/mp/mpcoll.h>
#include <sysdolphin/baselib/gobjplink.h>

static unsigned checks, scenarios, pose_calls, pose_invalidation;
#define CHECK(e) do { ++checks; if (!(e)) { \
    fprintf(stderr, "%s:%d scenario %u: %s\n", __FILE__, __LINE__, scenarios, #e); exit(1); \
} } while (0)
HSD_GObj** HSD_GObjPLinkHead;
static HSD_GObj* heads[32];
static FighterPartsTable* part_tables[Ft_Kind_RuntimeCapacity];
FighterPartsTable** ftPartsTable = part_tables;
static Fighter fighters[4], replacement;
static HSD_GObj objects[4];
static FighterBone bones[4];
static HSD_JObj joint;
static FighterPartsTable table;
static ftData data, changed_data;
static HSD_GObj* pose_object;
static Fighter* pose_fighter;
static unsigned pre_calls[5], post_calls[5], rotation_calls;
static HSD_GObj* callback_target;
static unsigned callback_phase, callback_action;
static HSD_GObj* protected_object;
static Fighter* protected_fighter;
static HSD_GObj* clear_marker_object;
static unsigned callback_trace[32], callback_trace_count;

void ftPartSetRotX(Fighter* fighter, int part, float rotation)
{
    CHECK(fighter != NULL && part == 0 && rotation == 0); ++rotation_calls;
}

// Model-effect lifetime is outside this fighter-only source matrix.
bool nativeModelEffect_Snapshot(HSD_GObj* object, uint32_t* generation)
{ (void)object; (void)generation; return false; }
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* object, uint32_t generation)
{ (void)object; (void)generation; return NULL; }

// The actual BonePosition operation owns all validation and continuation.
// World-matrix evaluation is a controlled boundary which can retire its caller.
void lb_8000B1CC(HSD_JObj* input, Vec3* offset, Vec3* output)
{
    CHECK(input == &joint && offset == NULL); ++pose_calls;
    *output = (Vec3){12.5f, -7.25f, 99.0f};
    if (pose_invalidation == 1) heads[HSD_GOBJ_PLINK_FIGHTER] = NULL;
    if (pose_invalidation == 2) ++fighters[0].x8_spawnNum;
    if (pose_invalidation == 3) objects[0].user_data = &replacement;
    if (pose_invalidation == 4) fighters[0].kind = Ft_Kind_Mario;
    if (pose_invalidation == 5) fighters[0].ft_data = &changed_data;
    if (pose_invalidation == 6) {
        heads[HSD_GOBJ_PLINK_FIGHTER] = NULL;
        DWORD previous;
        CHECK(VirtualProtect(pose_object, 4096, PAGE_NOACCESS, &previous));
        CHECK(VirtualProtect(pose_fighter, sizeof(Fighter), PAGE_NOACCESS, &previous));
    }
}

#include "bodies.h"

static void reset(uint32_t first, uint32_t second)
{
    ++scenarios; memset(fighters, 0, sizeof(fighters)); memset(objects, 0, sizeof(objects));
    memset(heads, 0, sizeof(heads)); memset(part_tables, 0, sizeof(part_tables));
    memset(bones, 0, sizeof(bones)); memset(&replacement, 0, sizeof(replacement));
    HSD_GObjPLinkHead = heads; ftPartsTable = part_tables;
    for (unsigned i = 0; i < 4; ++i) {
        fighters[i].x8_spawnNum = (int32_t)(i == 0 ? first : i == 1 ? second : i + 5);
        fighters[i].kind = Ft_Kind_CustomFirst; fighters[i].ft_data = &data;
        objects[i].user_data = &fighters[i];
        objects[i].next = i < 3 ? &objects[i + 1] : NULL;
    }
    heads[HSD_GOBJ_PLINK_FIGHTER] = &objects[0];
    table.parts_num = 4; part_tables[Ft_Kind_CustomFirst] = &table;
    fighters[0].parts = bones; bones[2].joint = &joint;
    pose_calls = pose_invalidation = 0;
    memset(pre_calls, 0, sizeof(pre_calls)); memset(post_calls, 0, sizeof(post_calls));
    rotation_calls = callback_phase = callback_action = 0; callback_target = NULL;
    clear_marker_object = NULL; callback_trace_count = 0;
}

static void unlink_object(HSD_GObj* object)
{
    HSD_GObj** link = &heads[HSD_GOBJ_PLINK_FIGHTER];
    while (*link != NULL && *link != object) link = &(*link)->next;
    CHECK(*link == object); *link = object->next; object->next = NULL;
}

static void callback_change(HSD_GObj* object)
{
    Fighter* fighter = object->user_data;
    switch (callback_action) {
    case 1: unlink_object(object); object->user_data = NULL; break;
    case 2: ++fighter->x8_spawnNum; break;
    case 3:
        fighter->x1A5C = &objects[2];
        fighter->native_hitlag_link_generation = (uint32_t)fighters[2].x8_spawnNum; break;
    case 4:
        unlink_object(fighter->x1A5C); fighter->x1A5C->user_data = NULL; break;
    case 5: ++((Fighter*)fighter->x1A5C->user_data)->x8_spawnNum; break;
    case 6: {
        unlink_object(protected_object); DWORD previous;
        CHECK(VirtualProtect(protected_object, 4096, PAGE_NOACCESS, &previous));
        CHECK(VirtualProtect(protected_fighter, sizeof(Fighter), PAGE_NOACCESS, &previous)); break;
    }
    }
}

static unsigned callback_index(HSD_GObj* object)
{
    for (unsigned i = 0; i < 4; ++i) if (object == &objects[i]) return i;
    CHECK(object == protected_object); return 4;
}

static void pre_callback(HSD_GObj* object)
{
    unsigned index = callback_index(object); ++pre_calls[index];
    CHECK(callback_trace_count < 32); callback_trace[callback_trace_count++] = index * 2;
    if (callback_phase == 1 && callback_target == object) callback_change(object);
}

static void post_callback(HSD_GObj* object)
{
    unsigned index = callback_index(object); ++post_calls[index];
    CHECK(callback_trace_count < 32); callback_trace[callback_trace_count++] = index * 2 + 1;
    if (object == clear_marker_object) ((Fighter*)object->user_data)->x2219_b7 = 0;
    if (callback_phase == 2 && callback_target == object) callback_change(object);
}

static void protected_allocate(void)
{
    protected_object = VirtualAlloc(NULL, 4096, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    protected_fighter = VirtualAlloc(NULL, sizeof(Fighter), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    CHECK(protected_object != NULL && protected_fighter != NULL);
    *protected_fighter = fighters[1]; *protected_object = objects[1];
    protected_object->user_data = protected_fighter;
    protected_fighter->pre_hitlag_cb = pre_callback; protected_fighter->post_hitlag_cb = post_callback;
}

static void protected_release(void)
{
    CHECK(VirtualFree(protected_object, 0, MEM_RELEASE));
    CHECK(VirtualFree(protected_fighter, 0, MEM_RELEASE));
    protected_object = NULL; protected_fighter = NULL;
}

static void links(void)
{
    const uint32_t ids[] = {1, 16777217u, 0x80000001u, UINT32_MAX};
    for (unsigned a = 0; a < 4; ++a) for (unsigned b = 0; b < 4; ++b) {
        if (a == b) continue;
        reset(ids[a], ids[b]);
        CHECK(ftLib_8008741C(ids[a]) == &objects[0]);
        CHECK(ftLib_8008741C(ids[b]) == &objects[1]);
        CHECK(nativeFighterRelations_Link(&objects[0], ids[b]));
        CHECK(fighters[0].x1A5C == &objects[1]);
        CHECK(fighters[0].native_hitlag_link_generation == ids[b]);
        CHECK(nativeFighterRelations_LinkedId(&objects[0]) == ids[b]);
        CHECK(nativeFighterRelations_LinkedId(&objects[1]) == 0); // One-way until explicitly authored.
        CHECK(nativeFighterRelations_Link(&objects[1], ids[a]));
        CHECK(nativeFighterRelations_LinkedId(&objects[1]) == ids[a]);
        CHECK(!nativeFighterRelations_Link(&objects[0], ids[a]));
        CHECK(!nativeFighterRelations_Link(&objects[0], 0x77777777u));
        CHECK(nativeFighterRelations_LinkedId(&objects[0]) == ids[b]); // Rejection preserves prior link.
        fighters[1].x8_spawnNum = 0x55555555;
        CHECK(nativeFighterRelations_LinkedId(&objects[0]) == 0); // Same address, new creation.
        CHECK(!nativeFighterRelations_Link(&objects[0], ids[b]));
        CHECK(nativeFighterRelations_LinkedId(&objects[1]) == ids[a]);
        CHECK(nativeFighterRelations_Link(&objects[0], 0));
        CHECK(fighters[0].x1A5C == NULL && fighters[0].native_hitlag_link_generation == 0);
        CHECK(nativeFighterRelations_LinkedId(&objects[0]) == 0);
    }
    reset(101, 102); objects[1].user_data = NULL;
    CHECK(ftLib_8008741C(102) == NULL);
    CHECK(!nativeFighterRelations_Link(&objects[0], 102));
    objects[0].user_data = NULL;
    CHECK(!nativeFighterRelations_Link(&objects[0], 0));
    CHECK(nativeFighterRelations_LinkedId(&objects[0]) == 0);
    reset(101, 102); HSD_GObjPLinkHead = NULL;
    CHECK(!nativeFighterRelations_Link(&objects[0], 102));
    CHECK(!nativeFighterRelations_CopyCollision(&objects[0], 102));
    CHECK(nativeFighterRelations_LinkedId(&objects[0]) == 0);
}

static void poisoned_retirement(void)
{
    reset(101, 102);
    HSD_GObj* retired = VirtualAlloc(NULL, 4096, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    Fighter* retired_data = VirtualAlloc(NULL, sizeof(Fighter), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    CHECK(retired != NULL && retired_data != NULL);
    retired_data->x8_spawnNum = 0x76543210; retired->user_data = retired_data;
    retired->next = &objects[0]; heads[HSD_GOBJ_PLINK_FIGHTER] = retired;
    CHECK(nativeFighterRelations_Link(&objects[0], 0x76543210));
    heads[HSD_GOBJ_PLINK_FIGHTER] = &objects[0];
    DWORD previous;
    CHECK(VirtualProtect(retired, 4096, PAGE_NOACCESS, &previous));
    CHECK(VirtualProtect(retired_data, sizeof(Fighter), PAGE_NOACCESS, &previous));
    CHECK(nativeFighterRelations_LinkedId(&objects[0]) == 0);
    CHECK(!nativeFighterRelations_Link(retired, 102));
    CHECK(nativeFighterRelations_LinkedId(retired) == 0);
    CHECK(!nativeFighterRelations_CopyCollision(retired, 102));
    CHECK(!nativeFighterRelations_CopyCollision(&objects[0], 0x76543210));
    CHECK(ftLib_8008741C(0x76543210) == NULL);
    CHECK(nativeFighterRelations_Link(&objects[0], 0));
    CHECK(VirtualFree(retired, 0, MEM_RELEASE)); CHECK(VirtualFree(retired_data, 0, MEM_RELEASE));
}

static void collision_copy(void)
{
    for (unsigned seed = 1; seed <= 256; ++seed) {
        reset(101, 0x80000001u);
        CollData* src = &fighters[1].coll_data;
        CollData* dst = &fighters[0].coll_data;
        memset(src, 0x20 + seed % 64, sizeof(*src));
        memset(dst, 0x80 + seed % 64, sizeof(*dst));
        src->cur_pos = (Vec3){seed, seed * -0.5f, seed * 0.25f};
        src->floor.index = (int)seed; src->left_facing_wall.index = -1;
        src->right_facing_wall.index = 37; src->ceiling.index = INT_MAX;
        CollData original = *dst, source_before = *src;
        Fighter fighter_before = fighters[0];
        CHECK(nativeFighterRelations_CopyCollision(&objects[0], 0x80000001u));
#define COPIED(field) CHECK(memcmp(&dst->field, &src->field, sizeof(dst->field)) == 0)
        COPIED(cur_pos); COPIED(prev_pos); COPIED(last_pos); COPIED(facing_dir);
        COPIED(x38); COPIED(floor_skip); COPIED(ledge_id_left); COPIED(ledge_id_right);
        COPIED(joint_id_skip); COPIED(lstick_x); COPIED(x64_ecb); COPIED(desired_ecb);
        COPIED(ecb); COPIED(prev_ecb); COPIED(xE4_ecb); COPIED(x130_flags);
        COPIED(env_flags); COPIED(prev_env_flags); COPIED(x13C); COPIED(contact);
        COPIED(floor); COPIED(left_facing_wall); COPIED(right_facing_wall); COPIED(ceiling);
#undef COPIED
        CHECK(dst->x34_flags.b0 == src->x34_flags.b0 && dst->x34_flags.b1234 == src->x34_flags.b1234);
        CHECK(dst->x34_flags.b5 == src->x34_flags.b5 && dst->x34_flags.b6 == src->x34_flags.b6);
        CHECK(dst->x34_flags.b7 == original.x34_flags.b7);
#define RETAINED(field) CHECK(memcmp(&dst->field, &original.field, sizeof(dst->field)) == 0)
        RETAINED(x0_gobj); RETAINED(x28_vec); RETAINED(x35_flags); RETAINED(joint_id_only);
        RETAINED(x50); RETAINED(ledge_snap_x); RETAINED(ledge_snap_y);
        RETAINED(ledge_snap_height); RETAINED(ecb_source);
#undef RETAINED
        CHECK(memcmp(src, &source_before, sizeof(*src)) == 0);
        fighter_before.coll_data = *dst;
        CHECK(memcmp(&fighters[0], &fighter_before, sizeof(Fighter)) == 0);
        CollData unchanged = *dst;
        CHECK(!nativeFighterRelations_CopyCollision(&objects[0], 0));
        CHECK(!nativeFighterRelations_CopyCollision(&objects[0], 0x99999999u));
        CHECK(memcmp(dst, &unchanged, sizeof(*dst)) == 0);
        CHECK(nativeFighterRelations_CopyCollision(&objects[0], 101)); // Safe same-object copy.
        CHECK(memcmp(dst, &unchanged, sizeof(*dst)) == 0);
    }
}

static void bone_query(void)
{
    for (unsigned failure = 0; failure <= 12; ++failure) {
        reset(101, 102); Vec3 output = {7, 8, 9}, saved = output;
        if (failure == 1) HSD_GObjPLinkHead = NULL;
        if (failure == 2) objects[0].user_data = NULL;
        if (failure == 3) fighters[0].kind = (FighterKind)-1;
        if (failure == 4) fighters[0].kind = Ft_Kind_RuntimeCapacity;
        if (failure == 5) ftPartsTable = NULL;
        if (failure == 6) part_tables[Ft_Kind_CustomFirst] = NULL;
        if (failure == 7) table.parts_num = 2;
        if (failure == 8) fighters[0].parts = NULL;
        if (failure == 9) bones[2].joint = NULL;
        uint32_t target = failure == 10 ? 0 : failure == 11 ? 0x99999999u : 101;
        Vec3* result = failure == 12 ? NULL : &output;
        bool valid = nativeFighterTarget_BonePosition(target, 2, result);
        CHECK(valid == (failure == 0)); CHECK(pose_calls == (failure == 0 ? 1 : 0));
        if (failure == 0) CHECK(output.x == 12.5f && output.y == -7.25f && output.z == 99);
        else CHECK(memcmp(&output, &saved, sizeof(output)) == 0);
    }
    reset(101, 102); Vec3 output = {7, 8, 9};
    CHECK(!nativeFighterTarget_BonePosition(101, UINT_MAX, &output) && pose_calls == 0);
    for (unsigned mode = 1; mode <= 5; ++mode) {
        reset(0x80000001u, 102); output = (Vec3){7, 8, 9}; Vec3 saved = output;
        pose_invalidation = mode;
        CHECK(!nativeFighterTarget_BonePosition(0x80000001u, 2, &output));
        CHECK(pose_calls == 1 && memcmp(&output, &saved, sizeof(output)) == 0);
    }
    reset(101, 102);
    pose_object = VirtualAlloc(NULL, 4096, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    pose_fighter = VirtualAlloc(NULL, sizeof(Fighter), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    CHECK(pose_object != NULL && pose_fighter != NULL);
    *pose_fighter = fighters[0]; *pose_object = objects[0];
    pose_object->user_data = pose_fighter; pose_object->next = NULL;
    heads[HSD_GOBJ_PLINK_FIGHTER] = pose_object; pose_invalidation = 6;
    output = (Vec3){7, 8, 9}; Vec3 saved = output;
    CHECK(!nativeFighterTarget_BonePosition(101, 2, &output));
    CHECK(pose_calls == 1 && memcmp(&output, &saved, sizeof(output)) == 0);
    CHECK(VirtualFree(pose_object, 0, MEM_RELEASE));
    CHECK(VirtualFree(pose_fighter, 0, MEM_RELEASE));
}

static void hitlag_healthy(void)
{
    for (unsigned topology = 0; topology < 4; ++topology)
        for (unsigned callbacks = 0; callbacks < 4; ++callbacks)
            for (unsigned gate = 0; gate < 4; ++gate) {
                reset(101, 102);
                for (unsigned i = 0; i < 4; ++i) {
                    fighters[i].pre_hitlag_cb = callbacks & 1 ? pre_callback : NULL;
                    fighters[i].post_hitlag_cb = callbacks & 2 ? post_callback : NULL;
                }
                if (topology != 0) CHECK(nativeFighterRelations_Link(&objects[0], 102));
                if (topology == 2) CHECK(nativeFighterRelations_Link(&objects[1], 101));
                if (topology == 3) {
                    CHECK(nativeFighterRelations_Link(&objects[1], 7));
                    CHECK(nativeFighterRelations_Link(&objects[2], 101));
                }
                Fighter_UnkRecursiveFunc_8006D044(&objects[0]);
                CHECK(fighters[0].x2219_b5 && !fighters[0].x2219_b7);
                CHECK(fighters[1].x2219_b5 == (topology != 0));
                CHECK(fighters[1].x2219_b7 == (topology != 0));
                CHECK(!fighters[2].x2219_b5 && !fighters[2].x2219_b7);
                CHECK(pre_calls[0] == (callbacks & 1 ? 1 : 0));
                CHECK(pre_calls[1] == ((callbacks & 1) && topology != 0 ? 1 : 0));
                CHECK(pre_calls[2] == 0);
                if (callbacks & 1) {
                    CHECK(callback_trace[0] == 0);
                    if (topology != 0) CHECK(callback_trace[1] == 2);
                }
                Fighter_UnkSetFlag_8006CFBC(&objects[0]);
                Fighter_UnkSetFlag_8006CFBC(&objects[1]);
                CHECK(!fighters[0].x221A_b1 && fighters[1].x221A_b1 == (topology != 0));
                fighters[1].allow_sdi = gate & 1 ? true : false;
                fighters[1].dmg.x1954 = gate & 2 ? 1 : 0;
                unsigned before = callback_trace_count;
                Fighter_8006D10C(&objects[0]);
                CHECK(!fighters[0].x2219_b5 && !fighters[0].x2219_b7);
                CHECK(!fighters[1].x2219_b7);
                CHECK(fighters[1].x2219_b5 == (topology != 0 && gate != 0));
                CHECK(post_calls[0] == (callbacks & 2 ? 1 : 0));
                CHECK(post_calls[1] == ((callbacks & 2) && topology != 0 && gate == 0 ? 1 : 0));
                CHECK(post_calls[2] == 0);
                if (callbacks & 2) {
                    CHECK(callback_trace[before] == 1);
                    if (topology != 0 && gate == 0) CHECK(callback_trace[before + 1] == 3);
                }
            }
    for (unsigned gate = 0; gate < 4; ++gate) {
        reset(101, 102); fighters[0].x2219_b7 = 1; fighters[0].x2219_b5 = 1;
        fighters[0].pre_hitlag_cb = pre_callback; fighters[0].post_hitlag_cb = post_callback;
        CHECK(nativeFighterRelations_Link(&objects[0], 102));
        Fighter_UnkRecursiveFunc_8006D044(&objects[0]);
        CHECK(pre_calls[0] == 1 && pre_calls[1] == 0 && fighters[0].x2219_b7);
        fighters[0].allow_sdi = gate & 1 ? true : false;
        fighters[0].dmg.x1954 = gate & 2 ? 1 : 0;
        Fighter_8006CFE0(&objects[0]);
        CHECK(!fighters[0].x2219_b7 && post_calls[0] == (gate == 0 ? 1 : 0));
        CHECK(fighters[0].x2219_b5 == (gate != 0));
    }
}

static void hitlag_callbacks(void)
{
    for (unsigned phase = 1; phase <= 2; ++phase)
        for (unsigned action = 1; action <= 5; ++action) {
            reset(101, 102);
            for (unsigned i = 0; i < 4; ++i) {
                fighters[i].pre_hitlag_cb = pre_callback; fighters[i].post_hitlag_cb = post_callback;
            }
            CHECK(nativeFighterRelations_Link(&objects[0], 102));
            if (phase == 2) {
                Fighter_UnkRecursiveFunc_8006D044(&objects[0]);
                memset(pre_calls, 0, sizeof(pre_calls)); callback_trace_count = 0;
            }
            callback_phase = phase; callback_action = action; callback_target = &objects[0];
            if (phase == 1) Fighter_UnkRecursiveFunc_8006D044(&objects[0]);
            else Fighter_8006D10C(&objects[0]);
            if (action <= 2) {
                CHECK(fighters[0].x2219_b5 == (phase == 2));
                CHECK(pre_calls[1] == 0 && post_calls[1] == 0);
            } else if (action == 3) {
                if (phase == 1) CHECK(pre_calls[1] == 0 && pre_calls[2] == 1 && fighters[2].x2219_b7);
                else CHECK(post_calls[1] == 0 && post_calls[2] == 0 && fighters[1].x2219_b7);
            } else {
                CHECK(pre_calls[1] == 0 && post_calls[1] == 0);
                CHECK(fighters[0].x2219_b5 == (phase == 1));
            }
        }
    // A child may remove its parent. Neither entry nor exit reads the parent's
    // cached userdata after that final recursive call.
    for (unsigned phase = 1; phase <= 2; ++phase) {
        reset(101, 102); protected_allocate();
        protected_fighter->x8_spawnNum = (int32_t)0x800000A1u;
        protected_object->next = &objects[1];
        heads[HSD_GOBJ_PLINK_FIGHTER] = protected_object;
        protected_fighter->x1A5C = &objects[1]; protected_fighter->native_hitlag_link_generation = 102;
        fighters[1].pre_hitlag_cb = pre_callback; fighters[1].post_hitlag_cb = post_callback;
        if (phase == 2) Fighter_UnkRecursiveFunc_8006D044(protected_object);
        callback_phase = phase; callback_action = 6; callback_target = &objects[1];
        if (phase == 1) Fighter_UnkRecursiveFunc_8006D044(protected_object);
        else Fighter_8006D10C(protected_object);
        CHECK(!nativeCallbackSource_Alive(protected_object, 102, NativeCallbackSource_Fighter));
        protected_release();
    }
    // Retirement from the direct caller's own callback must prevent both the
    // hitlag flag update and any linked traversal, with poisoned GObj/Fighter.
    for (unsigned phase = 1; phase <= 2; ++phase) {
        reset(101, 102); protected_allocate();
        protected_fighter->x8_spawnNum = (int32_t)0x800000A1u;
        protected_object->next = &objects[0]; heads[HSD_GOBJ_PLINK_FIGHTER] = protected_object;
        protected_fighter->x1A5C = &objects[0]; protected_fighter->native_hitlag_link_generation = 101;
        protected_fighter->x2219_b5 = phase == 2;
        callback_phase = phase; callback_action = 6; callback_target = protected_object;
        if (phase == 1) Fighter_UnkRecursiveFunc_8006D044(protected_object);
        else Fighter_8006D10C(protected_object);
        CHECK(pre_calls[0] == 0 && post_calls[0] == 0);
        protected_release();
    }
    // A selected child can also retire itself from its pre callback or from
    // Inline1's post callback; neither path may touch its cached flag storage.
    for (unsigned phase = 1; phase <= 2; ++phase) {
        reset(101, 102); protected_allocate();
        objects[0].next = protected_object; protected_object->next = &objects[2];
        CHECK(nativeFighterRelations_Link(&objects[0], 102));
        if (phase == 2) Fighter_UnkRecursiveFunc_8006D044(&objects[0]);
        callback_phase = phase; callback_action = 6; callback_target = protected_object;
        if (phase == 1) Fighter_UnkRecursiveFunc_8006D044(&objects[0]);
        else Fighter_8006D10C(&objects[0]);
        CHECK(pre_calls[4] == 1 && post_calls[4] == (phase == 2 ? 1 : 0));
        CHECK(fighters[0].x2219_b5 == (phase == 1));
        protected_release();
    }
    // Unlike an unmarked replacement, a currently marked replacement receives
    // the exit callback and marker cleanup; the old endpoint is untouched.
    reset(101, 102);
    CHECK(nativeFighterRelations_Link(&objects[0], 102));
    fighters[0].pre_hitlag_cb = pre_callback; fighters[0].post_hitlag_cb = post_callback;
    fighters[1].pre_hitlag_cb = pre_callback; fighters[1].post_hitlag_cb = post_callback;
    fighters[2].post_hitlag_cb = post_callback;
    Fighter_UnkRecursiveFunc_8006D044(&objects[0]);
    fighters[2].x2219_b7 = 1; fighters[2].x2219_b5 = 1;
    callback_phase = 2; callback_action = 3; callback_target = &objects[0];
    Fighter_8006D10C(&objects[0]);
    CHECK(post_calls[0] == 1 && post_calls[1] == 0 && post_calls[2] == 1);
    CHECK(fighters[1].x2219_b7 && fighters[1].x2219_b5);
    CHECK(!fighters[2].x2219_b7 && !fighters[2].x2219_b5);
    // Exercise Inline1's post-nested-call guard. The child clears its marker,
    // allowing Inline2 -> CFE0 -> D10C to enter a marked third fighter whose
    // callback removes the child before Inline1's final marker write.
    reset(101, 102); protected_allocate();
    objects[0].next = protected_object; protected_object->next = &objects[2];
    CHECK(nativeFighterRelations_Link(&objects[0], 102));
    protected_fighter->x1A5C = &objects[2]; protected_fighter->native_hitlag_link_generation = 7;
    protected_fighter->x2219_b7 = 1; protected_fighter->x2219_b5 = 1;
    fighters[2].x2219_b7 = 1; fighters[2].post_hitlag_cb = post_callback;
    clear_marker_object = protected_object; callback_phase = 2;
    callback_action = 6; callback_target = &objects[2];
    Fighter_8006D10C(&objects[0]);
    CHECK(post_calls[4] == 1 && post_calls[2] == 1);
    CHECK(!fighters[2].x2219_b7 && !fighters[2].x2219_b5);
    protected_release();
}

static void hitlag_stale_and_cleanup(void)
{
    for (unsigned invalid = 0; invalid < 3; ++invalid) {
        reset(101, 102); fighters[0].pre_hitlag_cb = pre_callback;
        fighters[0].post_hitlag_cb = post_callback;
        fighters[1].pre_hitlag_cb = pre_callback; fighters[1].post_hitlag_cb = post_callback;
        CHECK(nativeFighterRelations_Link(&objects[0], 102));
        if (invalid == 0) fighters[0].native_hitlag_link_generation = 444;
        if (invalid == 1) ++fighters[1].x8_spawnNum;
        if (invalid == 2) { unlink_object(&objects[1]); objects[1].user_data = (void*)1; }
        Fighter_UnkRecursiveFunc_8006D044(&objects[0]); Fighter_8006D10C(&objects[0]);
        CHECK(pre_calls[0] == 1 && post_calls[0] == 1 && pre_calls[1] == 0 && post_calls[1] == 0);
        CHECK(!fighters[1].x2219_b5 && !fighters[1].x2219_b7);
    }
    for (unsigned operation = 0; operation < 3; ++operation)
        for (unsigned invalid = 0; invalid < 4; ++invalid)
            for (unsigned markers = 0; markers < 4; ++markers) {
                reset(101, 102);
                CHECK(nativeFighterRelations_Link(&objects[0], 102));
                CHECK(nativeFighterRelations_Link(&objects[1], 101));
                fighters[0].x2219_b7 = markers & 1 ? 1 : 0;
                fighters[1].x2219_b7 = markers & 2 ? 1 : 0;
                if (invalid == 1) fighters[0].native_hitlag_link_generation = 444;
                if (invalid == 2) ++fighters[1].x8_spawnNum;
                if (invalid == 3) {
                    protected_allocate(); protected_fighter->x1A5C = &objects[0];
                    fighters[0].x1A5C = protected_object;
                    DWORD previous;
                    CHECK(VirtualProtect(protected_object, 4096, PAGE_NOACCESS, &previous));
                    CHECK(VirtualProtect(protected_fighter, sizeof(Fighter), PAGE_NOACCESS, &previous));
                }
                if (operation == 0) setRefGObjFlagAndClear(&fighters[0]);
                if (operation == 1) ftPp_SpecialS_8011F68C(&objects[0]);
                if (operation == 2) ftNn_Init_801238E4(&objects[0]);
                CHECK(fighters[0].x1A5C == NULL && fighters[0].native_hitlag_link_generation == 0);
                CHECK(fighters[0].x221A_b1 == (operation != 0 && (markers & 1) ? 1 : 0));
                CHECK(rotation_calls == (operation != 0 ? 1 : 0));
                if (invalid == 0) {
                    CHECK(fighters[1].x1A5C == NULL && fighters[1].native_hitlag_link_generation == 0);
                    CHECK(fighters[1].x221A_b1 == (markers & 2 ? 1 : 0));
                } else {
                    CHECK(fighters[1].x1A5C == &objects[0] && fighters[1].native_hitlag_link_generation == 101);
                    CHECK(!fighters[1].x221A_b1);
                }
                if (invalid == 3) protected_release();
            }
    reset(101, 102); protected_allocate();
    fighters[0].x1A5C = protected_object; fighters[0].native_hitlag_link_generation = 102;
    DWORD previous;
    CHECK(VirtualProtect(protected_object, 4096, PAGE_NOACCESS, &previous));
    CHECK(VirtualProtect(protected_fighter, sizeof(Fighter), PAGE_NOACCESS, &previous));
    fighters[0].pre_hitlag_cb = pre_callback; fighters[0].post_hitlag_cb = post_callback;
    Fighter_UnkRecursiveFunc_8006D044(&objects[0]); Fighter_8006D10C(&objects[0]);
    CHECK(pre_calls[0] == 1 && post_calls[0] == 1 && !fighters[0].x2219_b5);
    protected_release();
}

int main(void)
{
    links(); poisoned_retirement(); collision_copy(); bone_query();
    hitlag_healthy(); hitlag_callbacks(); hitlag_stale_and_cleanup();
    printf("PASS fighter relations: %u scenarios / %u checks\n", scenarios, checks);
    return 0;
}
