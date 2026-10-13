#undef NDEBUG
#include <assert.h>
#include <string.h>
#if PARAMETER_CAPACITY_CALLBACKS
#include "parameter_capacity_callback_fixture.h"
#else
#include "parameter_capacity_action_fixture.h"
#endif

static void verify(const NativeFighterDefinition* definition, HSD_GObj* object,
                   float lift, int number, int ready, int byte)
{
#if PARAMETER_CAPACITY_CALLBACKS
    definition->motion_states[0].phys_cb(object);
#else
    definition->on_load(object);
#endif
    Fighter* fighter = object->user_data;
    assert(fighter->self_vel.x == lift && fighter->self_vel.y == number);
    assert(fighter->cmd_vars[0] == ready && fighter->cmd_vars[1] == byte);
}

int main(void)
{
    uint32_t values[256];
    for (unsigned i = 0; i < 256; ++i) {
        static const uint32_t words[] = { 0x3fa00000, 0xffffffef, 1, 233 };
        values[i] = words[i % 4];
    }
    // The setter's schema/count are emitted from the real imported table.
    assert(FreakMelee_SetFighterParameters(values, 256, test_parameter_schema));
    MotionState motion = { 0 };
    NativeFighterDefinition base = { .fighter_root = "ftDataMario",
        .motion_state_first = 341, .motion_state_count = 1, .motion_states = &motion };
    NativeFighterHost host = { 0 };
    const NativeFighterDefinition* definition = FreakMelee_GetFighterModule(&base, &host);
    assert(definition);
    Fighter fighter = { 0 }; HSD_GObj object = { .user_data = &fighter };
    verify(definition, &object, 1.25f, -17, 1, 233);
    values[0] = 0x40200000; values[129] = 19; values[254] = 0; values[255] = 254;
    assert(!FreakMelee_SetFighterParameters(values, 255, test_parameter_schema));
    assert(!FreakMelee_SetFighterParameters(values, 257, test_parameter_schema));
    assert(!FreakMelee_SetFighterParameters(values, 256, test_parameter_schema ^ 1));
    verify(definition, &object, 1.25f, -17, 1, 233);
    assert(FreakMelee_SetFighterParameters(values, 256, test_parameter_schema));
    verify(definition, &object, 2.5f, 19, 0, 254);
    puts("Actual generated parameter callbacks use field255 and reload without recompilation");
    return 0;
}
