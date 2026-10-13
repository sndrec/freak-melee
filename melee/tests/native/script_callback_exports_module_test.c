#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#if CALLBACK_EXPORTS_CALLBACKS
#include "script_callback_exports_callback_fixture.h"
#else
#include "script_callback_exports_action_fixture.h"
#endif

static HSD_GObj* last_friction;
static void friction(HSD_GObj* object) { last_friction = object; }
static int32_t target_motion(uint32_t target) { assert(target==16777217); return 342; }
static void inherited_callback(HSD_GObj* object) { assert(!"Inherited exports must be replaced"); }
static const NativeFighterCallbackExport inherited_export = { .name_hash=1, .callback=inherited_callback, .result_type=NativeFighterReturn_Void };

static unsigned offset(const NativeFighterDefinition* definition, const char* name)
{
    for (unsigned i = 0; i < definition->state_field_count; ++i)
        if (!strcmp(definition->state_fields[i].name, name)) return definition->state_fields[i].offset;
    assert(0); return 0;
}
static int32_t read_word(Fighter* fighter, unsigned at)
{
    int32_t value; memcpy(&value, fighter->native_module_state + at, sizeof(value)); return value;
}
static uint32_t hash(const char* name)
{
    uint32_t value = UINT32_C(2166136261);
    for (; *name; ++name) value = (value ^ (unsigned char)*name) * UINT32_C(16777619);
    return value;
}
static HSD_GObjEvent callback(const NativeFighterDefinition* definition, const char* name)
{
    for (unsigned i = 0; i < definition->callback_export_count; ++i)
        if (definition->callback_exports[i].name_hash == hash(name)) return definition->callback_exports[i].callback;
    assert(0); return NULL;
}

int main(void)
{
    MotionState motion = { 0 };
    NativeFighterDefinition base = { .fighter_root = "ftDataNess",
        .motion_state_first = 341, .motion_state_count = 1, .motion_states = &motion,
        .state_bytes = 3, .callback_export_count = 1, .callback_exports = &inherited_export };
    NativeFighterHost host = { .ground_friction = friction, .target_motion=target_motion };
    const NativeFighterDefinition* definition = FreakMelee_GetFighterModule(&base, &host);
    assert(definition && definition->callback_export_count == 7);
    for (unsigned i = 0; i < definition->callback_export_count; ++i) {
        assert(definition->callback_exports[i].name_hash && definition->callback_exports[i].callback);
        if (i) assert(definition->callback_exports[i - 1].name_hash < definition->callback_exports[i].name_hash);
    }
    NativeFighterHost missing = { 0 };
    assert(!FreakMelee_GetFighterModule(&base, &missing)); // Export-only dependency.
    assert(FreakMelee_GetFighterModule(&base, &host) == definition);
    Fighter* caller = calloc(1, sizeof(*caller));
    Fighter* recipient = calloc(1, sizeof(*recipient)); assert(caller && recipient);
    HSD_GObj cg = { .user_data = caller }, rg = { .user_data = recipient };
    unsigned received = offset(definition, "received"), visits = offset(definition, "visits");
    assert(received >= 4 && visits >= 4);
    memset(recipient->native_module_state, 0x5A, base.state_bytes);
#if CALLBACK_EXPORTS_CALLBACKS
    definition->motion_states[0].phys_cb(&cg);
#else
    definition->on_load(&cg);
#endif
    assert(read_word(caller, visits) == 7);
    callback(definition, "clear_resource")(&rg);
    assert(read_word(recipient, received) == 16777217);
    assert(read_word(recipient, visits) == 1);
    assert(read_word(caller, received) == 0 && read_word(caller, visits) == 7);
    for (unsigned i=0; i<definition->callback_export_count; ++i) {
        const NativeFighterCallbackExport* value=&definition->callback_exports[i];
        if (value->name_hash==hash("is_ready")) {
            assert(value->result_type==NativeFighterReturn_Bool);
            assert(value->bool_callback(&rg) && !value->bool_callback(&cg));
        } else if (value->name_hash==hash("precise_id")) {
            assert(value->result_type==NativeFighterReturn_Int);
            assert(value->int_callback(&rg)==16777217);
        } else if (value->name_hash==hash("speed")) {
            assert(value->result_type==NativeFighterReturn_Float);
            assert(value->float_callback(&rg)==2.0f);
        }
    }
    for (unsigned i = 0; i < base.state_bytes; ++i) assert(recipient->native_module_state[i] == 0x5A);
    for (unsigned i=0; i<definition->callback_export_count; ++i) {
        const NativeFighterCallbackExport* value=&definition->callback_exports[i];
        if (value->name_hash==hash("ordered")) assert(value->int_callback(&rg)==12);
    }
    callback(definition, "acknowledge")(&rg);
    assert(read_word(recipient, received) == -1);
    callback(definition, "apply_friction")(&rg);
    assert(last_friction == &rg);
    // Reopening at an authoring transition rebinds offsets, without mutating the
    // immutable base table or appending exports repeatedly.
    base.state_bytes = 7;
    assert(FreakMelee_GetFighterModule(&base, &host) == definition);
    assert(definition->callback_export_count == 7 && offset(definition, "received") == received + 4);
    memset(recipient->native_module_state, 0, sizeof(recipient->native_module_state));
    memset(recipient->native_module_state, 0x3C, base.state_bytes);
    callback(definition, "clear_resource")(&rg);
    assert(read_word(recipient, offset(definition, "received")) == 16777217);
    for (unsigned i = 0; i < base.state_bytes; ++i) assert(recipient->native_module_state[i] == 0x3C);
    assert(base.callback_exports == &inherited_export && base.callback_export_count == 1);
    free(recipient); free(caller);
    puts("Generated recipient callbacks use the recipient object and bound module storage; both generation routes retain exact integer state.");
    return 0;
}
