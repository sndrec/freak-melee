#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if HELD_MOTION_CALLBACKS
#include "script_held_motion_callback_fixture.h"
#else
#include "script_held_motion_action_fixture.h"
#endif

static HSD_GObj* live;
static uint32_t generation = UINT32_C(0x80000017);
static unsigned calls, invalidation, checks;
#define CHECK(x) do { ++checks; assert(x); } while (0)
static bool alive(HSD_GObj* object, uint32_t id, uint32_t type)
{
    return object == live && object->user_data != NULL &&
        type == NativeCallbackSource_Fighter && id == generation &&
        ((Fighter*)object->user_data)->x8_spawnNum == generation;
}
static void enter(HSD_GObj* object, float blend)
{
    CHECK(object == live && blend == 1.5f);
    ++calls;
    if (invalidation == 1) object->user_data = NULL;
    if (invalidation == 2) ((Fighter*)object->user_data)->x8_spawnNum++;
}
static unsigned offset(const NativeFighterDefinition* d, const char* name)
{
    for (unsigned i = 0; i < d->state_field_count; ++i)
        if (!strcmp(d->state_fields[i].name, name)) return d->state_fields[i].offset;
    abort();
}
int main(void)
{
    MotionState motions[2] = { 0 };
    NativeFighterDefinition base = { .fighter_root = "ftDataMario",
        .motion_state_first = 341, .motion_state_count = 2, .motion_states = motions };
    NativeFighterHost host = { .enter_held_open = enter, .callback_source_alive = alive };
    const NativeFighterDefinition* d = FreakMelee_GetFighterModule(&base, &host);
    CHECK(d && d->on_held_motion && d->on_load);
    NativeFighterHost bad = host;
    bad.enter_held_open = NULL;
    CHECK(!FreakMelee_GetFighterModule(&base, &bad));
    bad = host; bad.callback_source_alive = NULL;
    CHECK(!FreakMelee_GetFighterModule(&base, &bad));
    d = FreakMelee_GetFighterModule(&base, &host);
    Fighter* fighter = calloc(1, sizeof(*fighter));
    CHECK(fighter);
    HSD_GObj object = { .user_data = fighter }; live = &object;
    fighter->x8_spawnNum = generation;
    int values[] = { -1, 0, 20, ftCo_MS_Count - 1, ftCo_MS_Count,
                     341, 342, 343, INT32_MAX, INT32_MIN };
    for (unsigned i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
        int value = values[i];
        memcpy(fighter->native_module_state + offset(d, "published"), &value, 4);
        fighter->motion_id = 77;
        d->on_load(&object);
        bool valid = value >= 0 && (value < ftCo_MS_Count || value == 341 || value == 342);
        CHECK(fighter->motion_id == (valid ? value : 77));
    }
    CHECK(d->on_held_motion(&object, 38, ftCo_MS_FallSpecial) == 341);
    CHECK(d->on_held_motion(&object, 38, 56) == 56);
    CHECK(d->on_held_motion(&object, 38, 19) == 38);
    int published;
    memcpy(&published, fighter->native_module_state + offset(d, "published"), 4);
    CHECK(published == 19);
    for (invalidation = 0; invalidation < 3; ++invalidation) {
        object.user_data = fighter; fighter->x8_spawnNum = generation;
        fighter->native_module_state[offset(d, "continued")] = 0;
        int result = d->on_held_motion(&object, 38, 55);
        CHECK(result == (invalidation == 0 ? 342 : 0));
        CHECK(fighter->native_module_state[offset(d, "continued")] == (invalidation == 0));
    }
    CHECK(calls == 3);
    free(fighter);
    printf("Held motion generated module PASS %u checks\n", checks);
    return 0;
}
