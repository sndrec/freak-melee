#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include PARTNER_TRANSFER_MODULE
__declspec(dllimport) void* __stdcall VirtualAlloc(void*,size_t,unsigned long,unsigned long);
__declspec(dllimport) int __stdcall VirtualProtect(void*,size_t,unsigned long,unsigned long*);
__declspec(dllimport) int __stdcall VirtualFree(void*,size_t,unsigned long);

static void* pages;
static HSD_GObj* object;
static Fighter* fighter;
static const NativeFighterDefinition* definition;
static unsigned checks, mode, calls;
static bool live, result;
static uint32_t generation;
#define CHECK(v) do { ++checks; assert(v); } while (0)
static bool alive(HSD_GObj* source, uint32_t id, uint32_t cls)
{
    CHECK(source == object && id == generation && cls == NativeCallbackSource_Fighter);
    return live && fighter->x8_spawnNum == generation;
}
static bool transfer(HSD_GObj* source, HSD_GObjEvent stock_finish)
{
    CHECK(source == object && stock_finish == NULL); ++calls;
    if (mode == 1) {
        live = false; unsigned long old; CHECK(VirtualProtect(pages, 65536, 1, &old));
    } else if (mode == 2) ++fighter->x8_spawnNum;
    return result;
}
static unsigned offset(const char* name)
{
    for (unsigned i = 0; i < definition->state_field_count; ++i)
        if (!strcmp(definition->state_fields[i].name, name)) return definition->state_fields[i].offset;
    assert(0); return 0;
}
static int32_t integer(const char* name) { int32_t value; memcpy(&value, fighter->native_module_state + offset(name), 4); return value; }
static bool flag(const char* name) { return fighter->native_module_state[offset(name)] != 0; }
static void reset(void)
{
    unsigned long old; CHECK(VirtualProtect(pages, 65536, 4, &old)); memset(pages, 0, 65536);
    object = pages; fighter = (Fighter*)((unsigned char*)pages + 4096);
    object->user_data = fighter; generation = UINT32_C(0x80000001);
    fighter->x8_spawnNum = generation; live = true; calls = 0;
}
int main(void)
{
    _Static_assert(sizeof(Fighter) <= 65536 - 4096, "Protected Fighter allocation");
    pages = VirtualAlloc(NULL, 65536, 0x3000, 4); CHECK(pages);
    MotionState motion = {0}; NativeFighterDefinition base = {.fighter_root = "ftDataTest",
        .motion_state_first = 341, .motion_state_count = 1, .motion_states = &motion};
    NativeFighterHost host = {.activate_partner = transfer, .callback_source_alive = alive};
    definition = FreakMelee_GetFighterModule(&base, &host);
    CHECK(definition && definition->on_form_activated);
    NativeFighterHost missing = host; missing.activate_partner = NULL;
    CHECK(!FreakMelee_GetFighterModule(&base, &missing));
    missing = host; missing.callback_source_alive = NULL;
    CHECK(!FreakMelee_GetFighterModule(&base, &missing));
    CHECK(FreakMelee_GetFighterModule(&base, &host));
    for (mode = 0; mode < 3; ++mode) for (unsigned outcome = 0; outcome < 2; ++outcome) {
        reset(); result = outcome != 0;
#if PARTNER_TRANSFER_CALLBACKS
        definition->motion_states[0].phys_cb(object);
#else
        definition->on_load(object);
#endif
        unsigned long old; CHECK(VirtualProtect(pages, 65536, 4, &old));
        CHECK(calls == 1 && integer("after") == (mode ? 0 : 99));
        CHECK(flag("switched") == (mode ? false : result));
    }
    reset(); definition->on_form_activated(object);
    CHECK(integer("activated") == 77 && calls == 0);
    CHECK(VirtualFree(pages, 0, 0x8000));
    printf("PASS %u generated transfer checks; full engine transfer comparison is separate\n", checks);
    return 0;
}
