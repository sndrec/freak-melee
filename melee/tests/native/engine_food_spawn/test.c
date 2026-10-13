#include <melee/it/types.h>
#include <melee/it/item.h>
#include <melee/it/itspawn.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_2725.h>
#include <melee/it/it_3F14.h>
#include <sysdolphin/baselib/gobj.h>
#undef NDEBUG
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static unsigned cases, checks, event_count;
#define CHECK(value) do { ++checks; assert(value); } while (0)
s32 it_804D6D0C;
static HSD_GObj callers[2], result_object;
static HSD_GObj* expected_caller;
static Vec3 supplied, center, supplied_before;
static bool available, allocated;
static int events[3];
static s32 initial_counter;

typedef struct SpawnProjection {
    ItemKind kind;
    Vec3 position, previous, velocity;
    float facing;
    s16 damage;
    s32 group;
    HSD_GObj* parent;
    HSD_GObj* secondary_parent;
    bool collision_flag;
} SpawnProjection;
static SpawnProjection observed;

static void event(int kind)
{
    CHECK(event_count < 3);
    events[event_count++] = kind;
}
bool it_8026D324(ItemKind kind)
{
    CHECK(kind == It_Kind_Foods);
    CHECK(it_804D6D0C == initial_counter);
    event(1);
    return available;
}
void it_8026BB68(Item_GObj* caller, Vec3* position)
{
    CHECK(caller == expected_caller && caller != NULL);
    CHECK(position != NULL && event_count == 1);
    CHECK(it_804D6D0C == initial_counter);
    event(2);
    *position = center;
}
Item_GObj* Item_80268B18(SpawnItem* spawn)
{
    CHECK(spawn != NULL);
    CHECK(event_count == (expected_caller ? 2 : 1));
    CHECK(it_804D6D0C == initial_counter);
    event(3);
    // Only fields explicitly initialized by the real helper are observed.
    // No read or normalization of unspecified SpawnItem fields/padding.
    observed = (SpawnProjection) {
        .kind = spawn->kind, .position = spawn->pos,
        .previous = spawn->prev_pos, .velocity = spawn->vel,
        .facing = spawn->facing_dir, .damage = spawn->x3C_damage,
        .group = spawn->x40, .parent = spawn->x0_parent_gobj,
        .secondary_parent = spawn->x4_parent_gobj2,
        .collision_flag = spawn->x44_flag.b0
    };
    return allocated ? &result_object : NULL;
}
#include "food_spawn.inc"

static void vec_equal(Vec3 actual, Vec3 expected)
{
    CHECK(!memcmp(&actual.x, &expected.x, sizeof(float)));
    CHECK(!memcmp(&actual.y, &expected.y, sizeof(float)));
    CHECK(!memcmp(&actual.z, &expected.z, sizeof(float)));
}
static void run_case(unsigned caller_index, Vec3 previous, Vec3 owner_center,
                     s32 counter, bool gate, bool success)
{
    supplied = supplied_before = previous;
    center = owner_center;
    expected_caller = caller_index ? &callers[caller_index - 1] : NULL;
    available = gate;
    allocated = success;
    initial_counter = it_804D6D0C = counter;
    event_count = 0;
    memset(events, 0, sizeof(events));
    HSD_GObj* result = it_8028FAF4(expected_caller, &supplied);
    CHECK(result == (gate && success ? &result_object : NULL));
    CHECK(it_804D6D0C == counter + (gate && success));
    CHECK(events[0] == 1);
    if (!gate) {
        CHECK(event_count == 1);
    } else {
        CHECK(event_count == (expected_caller ? 3 : 2));
        if (expected_caller) CHECK(events[1] == 2);
        CHECK(events[event_count - 1] == 3);
        CHECK(observed.kind == It_Kind_Foods);
        vec_equal(observed.previous, previous);
        vec_equal(observed.position, expected_caller ? owner_center : previous);
        vec_equal(observed.velocity, (Vec3){0, 0, 0});
        CHECK(observed.facing == -1.0f);
        CHECK(observed.damage == 0 && observed.group == 0);
        CHECK(observed.parent == NULL && observed.secondary_parent == NULL);
        CHECK(observed.collision_flag);
    }
    vec_equal(supplied, supplied_before);
    ++cases;
}
int main(void)
{
    const Vec3 previous[] = {{0,0,0}, {-0.0f,12.5f,-5.5f},
                             {1000,-0.125f,99}, {-300,45,7}};
    const Vec3 centers[] = {{11,22,33}, {-47,9.5f,81}, {0,-0.0f,-1}};
    const s32 counters[] = {-5,0,7,20000000};
    for (unsigned caller=0; caller<3; ++caller)
        for (unsigned p=0; p<4; ++p)
            for (unsigned c=0; c<3; ++c)
                for (unsigned n=0; n<4; ++n)
                    for (unsigned gate=0; gate<2; ++gate)
                        for (unsigned success=0; success<2; ++success)
                            run_case(caller,previous[p],centers[c],counters[n],gate,success);
    // The original unavailable gate runs before dereferencing either input.
    available=false;initial_counter=it_804D6D0C=19;event_count=0;
    CHECK(it_8028FAF4((Item_GObj*)(uintptr_t)1,NULL)==NULL);
    CHECK(event_count==1 && it_804D6D0C==19);++cases;
    printf("Food spawn actual source PASS %u cases %u checks\n",cases,checks);
    return 0;
}
