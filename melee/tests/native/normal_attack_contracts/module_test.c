#include <melee/ft/native_fighter_module.h>
#include <melee/ef/native_model_effect.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef NORMAL_BASE
#include "normal_base.h"
#else
#include "normal_action.h"
#endif

HSD_GObj** HSD_GObjPLinkHead;
static HSD_GObj* heads[32];
static unsigned invalidation, calls, checks;
static bool pickup;
#define CHECK(x) do { ++checks; assert(x); } while (0)

// This fixture contains only fighters. Reaching the effect branch is an error.
bool nativeModelEffect_Snapshot(HSD_GObj* object, uint32_t* generation)
{ (void)object; (void)generation; abort(); }
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* object, uint32_t generation)
{ (void)object; (void)generation; abort(); }

static void invalidate(HSD_GObj* object)
{
    ++calls;
    if (invalidation == 1) {
        heads[HSD_GOBJ_PLINK_FIGHTER] = NULL;
        object->user_data = (void*)1;
    } else if (invalidation == 2) ++((Fighter*)object->user_data)->x8_spawnNum;
}
static bool jab(HSD_GObj* object, FtMotionId motion)
{
    CHECK(motion == 341); invalidate(object); return !pickup;
}
static void rapid(HSD_GObj* object, enum_t motion)
{
    CHECK(motion == 342); invalidate(object);
}
static bool ground_pickup(HSD_GObj* object)
{
    invalidate(object); return pickup;
}
static bool should_finish(HSD_GObj* object)
{
    invalidate(object); return true;
}
static void collision(HSD_GObj* object, HSD_GObjEvent land)
{
    CHECK(land); land(object);
}
static void landing(HSD_GObj* object, FtMotionId motion, float lag)
{
    CHECK(motion == 350 && lag == 8); invalidate(object);
}
static unsigned field(const NativeFighterDefinition* definition, const char* name)
{
    for (unsigned i = 0; i < definition->state_field_count; ++i)
        if (!strcmp(name, definition->state_fields[i].name)) return definition->state_fields[i].offset;
    CHECK(false); return 0;
}
static uint32_t word(const Fighter* fighter, unsigned offset)
{
    uint32_t value; memcpy(&value, fighter->native_module_state + offset, sizeof(value)); return value;
}
int main(void)
{
    DbLKind debug = DbLKind_DebugRom;
    NativeFighterHost host = { .enter_jab = jab, .enter_rapid_jab = rapid,
        .rapid_jab_should_finish = should_finish, .try_ground_pickup = ground_pickup,
        .air_aerial_landing_collision = collision, .enter_landing_aerial = landing,
        .debug_level = &debug, .callback_source_alive = nativeCallbackSource_Alive };
    MotionState motions[12] = { 0 };
    NativeFighterDefinition base = { .fighter_root = "ftDataMario",
        .motion_state_first = 341, .motion_state_count = 12, .motion_states = motions };
    NativeFighterHost missing = host;
#define MISSING(member) do { missing = host; missing.member = NULL; CHECK(!FreakMelee_GetFighterModule(&base, &missing)); } while (0)
    MISSING(enter_jab); MISSING(enter_rapid_jab); MISSING(rapid_jab_should_finish);
    MISSING(try_ground_pickup); MISSING(air_aerial_landing_collision);
    MISSING(enter_landing_aerial); MISSING(debug_level); MISSING(callback_source_alive);
    const NativeFighterDefinition* definition = FreakMelee_GetFighterModule(&base, &host);
    CHECK(definition && definition->on_jab_enter && definition->on_rapid_jab_enter &&
          definition->on_down_tilt_enter && definition->on_aerial_attack_motion);
    Fighter* fighter = calloc(1, sizeof(*fighter)); CHECK(fighter);
    HSD_GObj object = { .user_data = fighter };
    HSD_GObjPLinkHead = heads;
    HSD_GObjEvent callbacks[] = { definition->on_jab_enter, definition->on_rapid_jab_enter,
        definition->on_down_tilt_enter, definition->motion_states[0].anim_cb,
        definition->motion_states[0].coll_cb };
    unsigned tail = field(definition, "tail");
    for (unsigned selected = 0; selected < 5; ++selected)
        for (unsigned picked = 0; picked < 2; ++picked)
            for (invalidation = 0; invalidation < 3; ++invalidation) {
                memset(fighter, 0, sizeof(*fighter)); object.user_data = fighter;
                fighter->x8_spawnNum = (s32)UINT32_C(0xFEDCBA98);
                heads[HSD_GOBJ_PLINK_FIGHTER] = &object; pickup = picked; calls = 0;
                callbacks[selected](&object);
                CHECK(calls == 1);
                // Rapid query invalidation stops before the separately owned
                // completion. Landing stops after its nested transition.
                CHECK(word(fighter, tail) == (selected == 3 ? (invalidation ? 0u : 2u) :
                    selected == 4 ? (invalidation ? 0u : 2u) : (invalidation ? 0u : 1u)));
                if (selected == 0) CHECK(fighter->native_module_state[field(definition, "entered")] == (!invalidation && !pickup));
                if (selected == 2) CHECK(fighter->native_module_state[field(definition, "picked_up")] == (!invalidation && pickup));
            }
    object.user_data = fighter; heads[HSD_GOBJ_PLINK_FIGHTER] = &object;
    fighter->x8_spawnNum = (s32)UINT32_C(0xFEDCBA98);
    fighter->co_attrs.landingairn_lag = 1;
    fighter->co_attrs.landingairf_lag = 2;
    fighter->co_attrs.landingairb_lag = 4;
    fighter->co_attrs.landingairhi_lag = 8;
    fighter->co_attrs.landingairlw_lag = 16;
    for (int level = DbLKind_Master; level <= DbLKind_Develop; ++level) {
        debug = (DbLKind)level; definition->on_load(&object);
        CHECK(word(fighter, field(definition, "debug")) == (uint32_t)level);
        CHECK(word(fighter, field(definition, "self_id")) == UINT32_C(0xFEDCBA98));
        float lag;
        memcpy(&lag, fighter->native_module_state + field(definition, "landing_lag"), sizeof(lag));
        CHECK(lag == 31);
    }
    for (int motion = 65; motion <= 69; ++motion)
        CHECK(definition->on_aerial_attack_motion(&object, motion) == (motion == 65 ? 347 : motion));
    free(fighter);
    printf("PASS %u generated normal-entry checks\n", checks);
    return 0;
}
