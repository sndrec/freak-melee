// Actual current collision bodies versus their original MSL int-bool bodies.
// Physics contact queries are controlled boundaries; their masks, call order,
// state writes, predicate returns and callbacks are compared without masking.
#include <melee/it/types.h>
#include <melee/it/item.h>
#include <melee/it/itgroundcoll.h>
#include <melee/it/itmaplib.h>
#include <melee/it/it_2725.h>
#include <melee/mp/mpcoll.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define GET_ITEM(gobj) ((Item*) HSD_GObjGetUserData(gobj))

void __assert(char* file, u32 line, char* expression)
{ fprintf(stderr, "%s:%lu: %s\n", file, (unsigned long)line, expression); abort(); }

static Item item;
static ItemAttr common;
static HSD_GObj object;
static HSD_JObj joint;
static int floor_contact, wall_flags, ceiling_flags, turnip_flags;
static bool allow_a, allow_b, allow_c;

enum CollisionStep {
    Prepare, Floor, Wall, Ceiling, RawContact, Bounce, WallResponse,
    ResolveFloor, GroundStep, Callback, Scale, QueryA, QueryB, QueryC, QueryD,
    UpdateFloor, State
};
typedef struct CollisionEvent { int step, arg, arg2; } CollisionEvent;
typedef struct CollisionResult {
    CollisionEvent events[48];
    unsigned count;
    int result, motion, line, ground;
    unsigned long lands;
    Vec3 position;
} CollisionResult;
static CollisionResult trace;
static unsigned comparisons;

static void record(int step, int arg, int arg2)
{
    assert(trace.count < sizeof(trace.events) / sizeof(trace.events[0]));
    trace.events[trace.count++] = (CollisionEvent){ step, arg, arg2 };
}
void it_80276214(Item_GObj* gobj) { assert(gobj == &object); record(Prepare, 0, 0); }
bool mpColl_800471F8(CollData* coll)
{ assert(coll == &item.x378_itemColl); record(Floor, floor_contact, 0); return floor_contact != 0; }
s32 it_80276308(Item_GObj* gobj)
{ assert(gobj == &object); record(Wall, wall_flags, 0); return wall_flags; }
s32 it_802763E0(Item_GObj* gobj)
{ assert(gobj == &object); record(Ceiling, ceiling_flags, 0); return ceiling_flags; }
s32 it_8026DAA8(Item_GObj* gobj)
{ assert(gobj == &object); record(RawContact, turnip_flags, 0); return turnip_flags; }
void it_80276FC4(Item_GObj* gobj, s32 flags)
{ assert(gobj == &object); record(Bounce, flags, 0); }
bool it_80276D9C(Item_GObj* gobj, enum_t flags)
{ assert(gobj == &object); record(WallResponse, flags, 0); return true; }
void it_80275DFC(Item_GObj* gobj)
{ assert(gobj == &object); record(ResolveFloor, 0, 0); }
void it_802762B0(Item* current)
{ assert(current == &item); record(GroundStep, 0, 0); current->ground_or_air = GA_Ground; }
void it_80272F7C(HSD_JObj* current, f32 scale)
{ assert(current == &joint && scale == 1.25f); record(Scale, 0, 0); }
bool it_8026DDFC(Item_GObj* gobj)
{ assert(gobj == &object); record(QueryA, allow_a, 0); return allow_a; }
bool it_8026DE98(Item_GObj* gobj)
{ assert(gobj == &object); record(QueryB, allow_a, 0); return allow_a; }
bool it_8026DC24(Item_GObj* gobj)
{ assert(gobj == &object); record(QueryC, allow_b, 0); return allow_b; }
bool it_8026DD5C(Item_GObj* gobj)
{ assert(gobj == &object); record(QueryD, allow_c, 0); return allow_c; }
void it_802725D4(Item_GObj* gobj)
{ assert(gobj == &object); record(UpdateFloor, 0, 0); }
void Item_80268E5C(HSD_GObj* gobj, enum_t motion, Item_StateChangeFlags flags)
{ assert(gobj == &object); record(State, motion, flags); item.msid = motion; }
static void landed(HSD_GObj* gobj)
{ assert(gobj == &object); record(Callback, 0, 0); item.xD50_landNum += 7; }

#include "collision_mask_original.inc"
#include "collision_mask_candidate.inc"

static void reset(void)
{
    memset(&item, 0, sizeof(item));
    memset(&object, 0, sizeof(object));
    memset(&common, 0, sizeof(common));
    memset(&trace, 0, sizeof(trace));
    object.user_data = &item;
    object.hsd_obj = &joint;
    item.xCC_item_attr = &common;
    common.x60_scale = 1.25f;
    item.x378_itemColl.cur_pos = (Vec3){ 2.0f, -3.0f, 5.0f };
    item.x378_itemColl.floor.index = 1234;
    item.pos = (Vec3){ 10.0f, 20.0f, 30.0f };
    item.xC30 = -777;
    item.msid = -8;
    item.ground_or_air = GA_Air;
    item.xD50_landNum = 11;
}
static CollisionResult capture(int result)
{
    trace.result = result;
    trace.motion = item.msid;
    trace.line = item.xC30;
    trace.ground = item.ground_or_air;
    trace.lands = item.xD50_landNum;
    trace.position = item.pos;
    return trace;
}
static int original(unsigned policy)
{
    switch (policy) {
    case 0: return original_it_8026DFB0(&object);
    case 1: return original_it_8026E058(&object);
    case 2: original_it_8026E15C(&object, landed); return 0;
    case 3: original_it_8026E248(&object, landed); return 0;
    case 4: return original_it_8026E32C(&object, landed);
    case 5: original_it_8026E414(&object, landed); return 0;
    case 6: original_it_8026E4D0(&object, landed); return 0;
    case 7: original_it_8026E5A0(&object, landed); return 0;
    case 8: original_it_8026E664(&object, landed); return 0;
    case 9: original_it_8026E71C(&object, landed); return 0;
    case 10: original_it_8026E7E0(&object, landed); return 0;
    case 11: return original_itPeachturnip_UnkMotion3_Coll(&object);
    default: abort();
    }
}
static int candidate(unsigned policy)
{
    switch (policy) {
    case 0: return it_8026DFB0(&object);
    case 1: return it_8026E058(&object);
    case 2: it_8026E15C(&object, landed); return 0;
    case 3: it_8026E248(&object, landed); return 0;
    case 4: return it_8026E32C(&object, landed);
    case 5: it_8026E414(&object, landed); return 0;
    case 6: it_8026E4D0(&object, landed); return 0;
    case 7: it_8026E5A0(&object, landed); return 0;
    case 8: it_8026E664(&object, landed); return 0;
    case 9: it_8026E71C(&object, landed); return 0;
    case 10: it_8026E7E0(&object, landed); return 0;
    case 11: return itPeachturnip_UnkMotion3_Coll(&object);
    default: abort();
    }
}
static void compare(unsigned policy)
{
    reset();
    CollisionResult expected = capture(original(policy));
    reset();
    CollisionResult actual = capture(candidate(policy));
    if (memcmp(&expected, &actual, sizeof(expected)) != 0) {
        fprintf(stderr, "policy=%u floor=%d wall=%d ceiling=%d raw=%d predicates=%d/%d/%d\n",
                policy, floor_contact, wall_flags, ceiling_flags, turnip_flags,
                allow_a, allow_b, allow_c);
        fprintf(stderr, "events=%u/%u result=%d/%d line=%d/%d lands=%lu/%lu\n",
                expected.count, actual.count, expected.result, actual.result,
                expected.line, actual.line, expected.lands, actual.lands);
        abort();
    }
    ++comparisons;
}
int main(void)
{
    for (floor_contact = 0; floor_contact <= 1; ++floor_contact) {
        for (wall_flags = 0; wall_flags < 16; ++wall_flags) {
            for (ceiling_flags = 0; ceiling_flags < 16; ++ceiling_flags) {
                for (unsigned predicates = 0; predicates < 8; ++predicates) {
                    allow_a = (predicates & 1) != 0;
                    allow_b = (predicates & 2) != 0;
                    allow_c = (predicates & 4) != 0;
                    for (unsigned policy = 0; policy < 11; ++policy) compare(policy);
                }
            }
        }
    }
    const int boundary_masks[] = { 0x10, 0x10000, 0x7FFFFFFF, -1, (-2147483647-1),
                                  (-2147483647-1)+2, 20, 24 };
    for (turnip_flags = 0; turnip_flags < 16; ++turnip_flags) compare(11);
    for (unsigned i = 0; i < sizeof(boundary_masks)/sizeof(boundary_masks[0]); ++i) {
        turnip_flags = boundary_masks[i]; compare(11);
    }
    turnip_flags = 8;
    reset();
    assert(candidate(11) == 0);
    assert(trace.count == 3 && trace.events[1].step == Bounce &&
           trace.events[1].arg == 8 && trace.events[2].step == State && item.msid == 1);
    floor_contact = 0; wall_flags = 8; ceiling_flags = 0;
    reset();
    candidate(5);
    assert(trace.count == 5 && trace.events[4].step == Bounce &&
           trace.events[4].arg == 8 && item.ground_or_air == GA_Air);
    printf("collision mask original-MSL comparisons: %u PASS\n", comparisons);
    return 0;
}
