#undef NDEBUG
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if ENGINE_ITEMS_CALLBACKS
#include "script_engine_items_callback_fixture.h"
#else
#include "script_engine_items_action_fixture.h"
#endif

static HSD_GObj source;
static uint32_t source_generation, result_id, held_id, auxiliary_id;
static bool live, recycle;
static unsigned assertions, stop_at, create_calls, attach_calls, inherit_calls;
static unsigned bind_calls, clear_calls, id_calls, alive_calls;
static unsigned existing_calls, visible_calls, pause_calls, drop_calls;
#define CHECK(value) do { ++assertions; assert(value); } while (0)

static void invalidate(unsigned phase)
{
    if (stop_at != phase) return;
    live = false;
    if (recycle) {
        ++((Fighter*)source.user_data)->x8_spawnNum;
        live = true;
    }
}

static bool alive(HSD_GObj* object, uint32_t generation, uint32_t source_class)
{
    ++alive_calls;
    CHECK(object == &source && source_class == NativeCallbackSource_Fighter);
    CHECK(generation == source_generation);
    return live && ((Fighter*)source.user_data)->x8_spawnNum == generation;
}

static int32_t create(Fighter_GObj* object, ItemKind kind, Vec3* pos, Vec3* vel,
                      float facing, uint32_t group, float damage)
{
    ++create_calls;
    CHECK(object == &source && kind == It_Kind_Bat);
    CHECK(pos && pos->x == 1 && pos->y == 2 && pos->z == 3);
    CHECK(vel && vel->x == 4 && vel->y == 5 && vel->z == 6);
    CHECK(facing == -1 && damage == 2.5f && group == UINT32_C(0xFEED1234));
    invalidate(1);
    return (int32_t)result_id;
}

static bool attach(Fighter_GObj* object, uint32_t handle, int32_t part, bool notify_holder)
{
    ++attach_calls;
    CHECK(object == &source && handle == result_id && part == 7 && notify_holder);
    invalidate(2);
    return handle != UINT32_MAX;
}

static bool inherit(Fighter_GObj* object, uint32_t handle, bool remember)
{
    ++inherit_calls;
    CHECK(object == &source && handle == result_id && remember);
    return handle != UINT32_MAX;
}

static bool bind(Fighter_GObj* object, uint32_t handle, bool auxiliary)
{
    ++bind_calls; CHECK(object == &source && handle == result_id);
    if (handle == UINT32_MAX) return false;
    if (auxiliary) auxiliary_id = handle;
    else held_id = handle;
    return true;
}

static bool clear(Fighter_GObj* object, bool auxiliary)
{
    ++clear_calls; CHECK(object == &source);
    if (auxiliary) auxiliary_id = UINT32_MAX;
    else held_id = UINT32_MAX;
    return true;
}

static bool bind_existing(Fighter_GObj* object, uint32_t handle, bool auxiliary)
{
    ++existing_calls; CHECK(object == &source && handle == result_id);
    if (handle == UINT32_MAX) return false;
    if (auxiliary) auxiliary_id = handle;
    else held_id = handle;
    return true;
}

static bool visible(uint32_t handle, bool value)
{ ++visible_calls; CHECK(handle == result_id && !value); return handle != UINT32_MAX; }
static bool paused(uint32_t handle, bool value)
{ ++pause_calls; CHECK(handle == result_id && value); return handle != UINT32_MAX; }
static bool drop(Fighter_GObj* object, uint32_t handle, bool value)
{
    ++drop_calls; CHECK(object == &source && handle == result_id && value);
    invalidate(3);
    return handle != UINT32_MAX;
}

static int32_t id(Fighter_GObj* object, bool auxiliary)
{
    ++id_calls; CHECK(object == &source);
    return (int32_t)(auxiliary ? auxiliary_id : held_id);
}

static unsigned offset(const NativeFighterDefinition* definition, const char* name)
{
    for (unsigned i = 0; i < definition->state_field_count; ++i)
        if (!strcmp(definition->state_fields[i].name, name)) return definition->state_fields[i].offset;
    assert(0); return 0;
}

static uint32_t word(const Fighter* fighter, unsigned at)
{
    uint32_t value; memcpy(&value, fighter->native_module_state + at, 4); return value;
}

static void reset(Fighter* fighter, uint32_t handle, unsigned phase, bool reuse)
{
    memset(fighter, 0, sizeof(*fighter));
    source_generation = UINT32_C(0xA0000033);
    fighter->x8_spawnNum = source_generation;
    source.user_data = fighter;
    result_id = handle; stop_at = phase; recycle = reuse; live = true;
    held_id = auxiliary_id = UINT32_MAX;
    create_calls = attach_calls = inherit_calls = bind_calls = clear_calls = id_calls = alive_calls = 0;
    existing_calls = visible_calls = pause_calls = drop_calls = 0;
}

int main(void)
{
    MotionState motion = { 0 };
    NativeFighterDefinition base = { .fighter_root = "ftDataLink",
        .motion_state_first = 341, .motion_state_count = 1, .motion_states = &motion };
    NativeFighterHost host = { .create_engine_item = create, .attach_created_item = attach,
        .inherit_created_item_scale = inherit, .bind_fighter_item = bind,
        .clear_fighter_item = clear, .held_item_id = id, .callback_source_alive = alive,
        .bind_existing_item=bind_existing, .set_item_visible=visible,
        .set_item_paused=paused, .notify_item_drop=drop };
    const NativeFighterDefinition* definition = FreakMelee_GetFighterModule(&base, &host);
    CHECK(definition);
    for (unsigned missing = 0; missing < 11; ++missing) {
        NativeFighterHost incomplete = host;
        switch (missing) {
        case 0: incomplete.create_engine_item = NULL; break;
        case 1: incomplete.attach_created_item = NULL; break;
        case 2: incomplete.inherit_created_item_scale = NULL; break;
        case 3: incomplete.bind_fighter_item = NULL; break;
        case 4: incomplete.clear_fighter_item = NULL; break;
        case 5: incomplete.held_item_id = NULL; break;
        case 6: incomplete.callback_source_alive = NULL; break;
        case 7: incomplete.bind_existing_item = NULL; break;
        case 8: incomplete.set_item_visible = NULL; break;
        case 9: incomplete.set_item_paused = NULL; break;
        case 10: incomplete.notify_item_drop = NULL; break;
        }
        CHECK(!FreakMelee_GetFighterModule(&base, &incomplete));
    }
    definition = FreakMelee_GetFighterModule(&base, &host); CHECK(definition);
    Fighter* fighter = calloc(1, sizeof(*fighter)); CHECK(fighter);
    const uint32_t ids[] = { UINT32_C(16777217), UINT32_C(0x80000001),
                            UINT32_C(0xFFFFFFFE), UINT32_MAX };
    unsigned cases = 0;
    for (unsigned i = 0; i < sizeof(ids) / sizeof(ids[0]); ++i)
    for (unsigned stop = 0; stop <= 3; ++stop)
    for (unsigned reuse = 0; reuse < 2; ++reuse) {
        reset(fighter, ids[i], stop, reuse); ++cases;
#if ENGINE_ITEMS_CALLBACKS
        definition->motion_states[0].phys_cb(&source);
#else
        definition->on_load(&source);
#endif
#define WORD(name) word(fighter, offset(definition, name))
#define BOOL(name) fighter->native_module_state[offset(definition, name)]
        CHECK(create_calls == 1 && alive_calls != 0);
        if (stop == 1) {
            CHECK(WORD("created") == 0);
            CHECK(WORD("inner_after_create") == 0 && WORD("helper_after_create") == 0);
            CHECK(WORD("outer_after_create") == 0 && WORD("finished") == 0);
            CHECK(attach_calls == 0 && inherit_calls == 0 && bind_calls == 0);
            continue;
        }
        CHECK(WORD("created") == ids[i]);
        CHECK(WORD("inner_after_create") == 1 && WORD("helper_after_create") == 1);
        CHECK(WORD("outer_after_create") == 1 && attach_calls == 1);
        if (stop == 2) {
            CHECK(!BOOL("attached") && WORD("helper_after_attach") == 0);
            CHECK(WORD("outer_after_attach") == 0 && WORD("finished") == 0);
            CHECK(inherit_calls == 0 && bind_calls == 0 && clear_calls == 0 && id_calls == 0);
            continue;
        }
        CHECK(WORD("helper_after_attach") == 1 && WORD("outer_after_attach") == 1);
        CHECK(WORD("finished") == 1);
        CHECK(BOOL("attached") == (ids[i] != UINT32_MAX));
        CHECK(BOOL("scaled") == (ids[i] != UINT32_MAX));
        CHECK(BOOL("held") == (ids[i] != UINT32_MAX));
        CHECK(BOOL("auxiliary") == (ids[i] != UINT32_MAX));
        CHECK(WORD("held_id") == ids[i] && WORD("auxiliary_id") == ids[i]);
        CHECK(BOOL("cleared_held"));
        CHECK(BOOL("cleared_auxiliary"));
        CHECK(held_id == ids[i] && auxiliary_id == ids[i]);
        CHECK(inherit_calls == 1 && bind_calls == 2 && clear_calls == 2 && id_calls == 2);
        CHECK(existing_calls == 2 && visible_calls == 1 && pause_calls == 1 && drop_calls == 1);
        CHECK(BOOL("notified") == (stop != 3 && ids[i] != UINT32_MAX));
        CHECK(WORD("after_drop") == (stop == 3 ? 0 : 1));
    }
    free(fighter);
    printf("Generated engine-item module: %u cases, %u assertions; controlled Host/liveness boundaries.\n",
           cases, assertions);
    return 0;
}
