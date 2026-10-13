#include <melee/ft/native_fighter_module.h>
#include <melee/ft/types.h>
#include <melee/it/types.h>
#include <melee/it/kinds/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjobject.h>
#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HELD_PROTOCOL_DLL
#include <windows.h>
#else
#include "held_article_protocol_fixture.h"
#endif

static unsigned checks, queries[4], owner_reads, alive_reads;
#define CHECK(value) do { ++checks; assert(value); } while (0)
static HSD_GObj* source;
static bool live, target_live;
static unsigned invalidation;
static uint32_t owner_id = UINT32_C(0xFEDCBA98);
static bool source_alive(HSD_GObj* object, uint32_t generation, uint32_t class_id)
{
    ++alive_reads;
    return live && class_id == 1 && object == source &&
        object->user_data != NULL && ((Item*)object->user_data)->x1C == generation;
}
static uint32_t current_owner(Item_GObj* object)
{
    CHECK(object == source); ++owner_reads; return owner_id;
}
static bool valid_target(uint32_t id) { return target_live && id == UINT32_C(0xFEDCBA98); }
static int32_t held_id(uint32_t id)
{
    ++queries[0]; return valid_target(id) ? (int32_t)UINT32_C(0x89ABCDEF) : -1;
}
static int32_t held_kind(uint32_t id)
{
    ++queries[1]; return valid_target(id) ? It_Kind_Peach_Parasol : -1;
}
static int32_t held_motion(uint32_t id)
{
    ++queries[2]; return valid_target(id) ? 2 : -1;
}
static float held_frame(uint32_t id)
{
    ++queries[3]; return valid_target(id) ? 16.25f : 0;
}
static void change_state(Item_GObj* object, enum_t motion, Item_StateChangeFlags flags)
{
    CHECK(object == source); CHECK(motion == 1); CHECK(flags == 2);
    Item* item = object->user_data; item->msid = motion;
    if (invalidation == 1) live = false;
    if (invalidation == 2) ++item->x1C;
}
static uint32_t bits_float(float value) { uint32_t bits; memcpy(&bits, &value, 4); return bits; }
static unsigned field(const NativeFighterArticleLogic* logic, const char* name)
{
    for (unsigned i = 0; i < logic->state_field_count; ++i)
        if (!strcmp(logic->state_fields[i].name, name)) return logic->state_fields[i].offset;
    CHECK(false); return 0;
}
static uint32_t word(const Item* item, unsigned offset)
{
    uint32_t value; memcpy(&value, item->xDD4_itemVar.native_article.state + offset, 4); return value;
}
int main(int argc, char** argv)
{
#ifdef HELD_PROTOCOL_DLL
    if (argc != 2) return 2;
    HMODULE library = LoadLibraryA(argv[1]); CHECK(library);
    const NativeFighterDefinition* (*get_module)(const NativeFighterDefinition*, const NativeFighterHost*) =
        (void*)GetProcAddress(library, "FreakMelee_GetFighterModule"); CHECK(get_module);
#else
    (void)argc; (void)argv;
    const NativeFighterDefinition* (*get_module)(const NativeFighterDefinition*, const NativeFighterHost*) = FreakMelee_GetFighterModule;
#endif
    NativeFighterHost host = { .target_held_id = held_id, .target_held_kind = held_kind,
        .target_held_motion = held_motion, .target_held_frame = held_frame,
        .item_current_owner = current_owner, .item_change_state_flags = change_state,
        .callback_source_alive = source_alive };
    MotionState motion = {0};
    NativeFighterDefinition base = { .fighter_root = "ftDataPeach", .source_kind = Ft_Kind_Peach,
        .motion_state_first = 341, .motion_state_count = 1, .motion_states = &motion };
    for (unsigned missing = 0; missing < 7; ++missing) {
        NativeFighterHost broken = host;
        switch (missing) {
        case 0: broken.target_held_id = NULL; break;
        case 1: broken.target_held_kind = NULL; break;
        case 2: broken.target_held_motion = NULL; break;
        case 3: broken.target_held_frame = NULL; break;
        case 4: broken.item_current_owner = NULL; break;
        case 5: broken.item_change_state_flags = NULL; break;
        case 6: broken.callback_source_alive = NULL; break;
        }
        CHECK(get_module(&base, &broken) == NULL);
    }
    const NativeFighterDefinition* definition = get_module(&base, &host);
    CHECK(definition); CHECK(definition->article_logic_count == 3);
    const NativeFighterArticleLogic* policy = &definition->article_logics[0];
    CHECK(policy->on_held_status); CHECK(policy->on_held_request);
    CHECK(definition->article_logics[1].on_held_status); CHECK(definition->article_logics[1].on_held_request);
    CHECK(!definition->article_logics[2].on_held_status); CHECK(!definition->article_logics[2].on_held_request);
    Item* item = calloc(1, sizeof(*item)); CHECK(item);
    HSD_GObj object = { .user_data = item }; source = &object; item->entity = source;
    item->x1C = UINT32_C(0xFEDCBA98); live = target_live = true;
    const int states[] = { -1, 0, 1, 2, 3, 61 };
    const int expected[] = { -1, 6, 4, 6, -1, -1 };
    for (unsigned i = 0; i < 6; ++i) {
        item->msid = states[i]; CHECK(policy->on_held_status(source) == expected[i]);
        CHECK(owner_reads == 0 && alive_reads == 0);
    }
    CHECK(definition->article_logics[1].on_held_status(source) == -1);
    CHECK(!definition->article_logics[1].on_held_request(source, 4, 1, 0));
    CHECK(policy->logic->dmg_dealt(source)); // Ordinary Bool callbacks retain existing numeric truth.
    const float rates[] = { -0.0f, 0.5f, 1.0f, -2.0f };
    const float durations[] = { 0, 1.0f, 2.5f, -3.0f };
    unsigned id_at = field(policy, "observed_id"), kind_at = field(policy, "observed_kind");
    unsigned motion_at = field(policy, "observed_motion"), frame_at = field(policy, "observed_frame");
    unsigned continued_at = field(policy, "continued");
    for (unsigned absent = 0; absent < 2; ++absent)
        for (unsigned r = 0; r < 4; ++r)
            for (unsigned d = 0; d < 4; ++d) {
                target_live = absent == 0; live = true; invalidation = 0;
                memset(queries, 0, sizeof(queries)); owner_reads = alive_reads = 0;
                memset(item->xDD4_itemVar.native_article.state, 0, sizeof(item->xDD4_itemVar.native_article.state));
                CHECK(policy->on_held_request(source, 4, rates[r], durations[d]));
                float expected_rate = durations[d] == 0 ? rates[r] : rates[r] * (15.0f / durations[d]);
                CHECK(bits_float(item->x5D0_animFrameSpeed) == bits_float(expected_rate));
                CHECK(word(item, id_at) == (target_live ? UINT32_C(0x89ABCDEF) : UINT32_MAX));
                CHECK((int32_t)word(item, kind_at) == (target_live ? It_Kind_Peach_Parasol : -1));
                CHECK((int32_t)word(item, motion_at) == (target_live ? 2 : -1));
                CHECK(word(item, frame_at) == bits_float(target_live ? 16.25f : 0));
                CHECK(word(item, continued_at) == 4);
                CHECK(owner_reads == 3 && alive_reads != 0);
                for (unsigned q = 0; q < 4; ++q) CHECK(queries[q] == 1);
            }
    for (unsigned mode = 1; mode <= 2; ++mode) {
        live = target_live = true; invalidation = mode;
        memset(item->xDD4_itemVar.native_article.state, 0, sizeof(item->xDD4_itemVar.native_article.state));
        CHECK(!policy->on_held_request(source, 4, 1, 0));
        CHECK(word(item, continued_at) == 0);
    }
    live = target_live = true; invalidation = 0;
    for (int request = -1; request <= 7; ++request) if (request != 4) {
        memset(queries, 0, sizeof(queries)); owner_reads = 0;
        CHECK(!policy->on_held_request(source, request, 1, 0));
        CHECK(owner_reads == 0 && queries[0] == 0 && queries[3] == 0);
    }
    free(item);
#ifdef HELD_PROTOCOL_DLL
    CHECK(FreeLibrary(library));
#endif
    printf("Generated held article protocol PASS %u checks (32 rate/observation cases + 2 invalidation cases)\n", checks);
    return 0;
}
