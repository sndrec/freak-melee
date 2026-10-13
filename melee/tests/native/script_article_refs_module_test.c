#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if ARTICLE_REFS_CALLBACKS
#include "script_article_refs_callback_fixture.h"
#else
#include "script_article_refs_action_fixture.h"
#endif

static bool live = true;
static unsigned calls, negative_indices;
static unsigned charge_calls;
static bool exists(HSD_GObj*, uint32_t, ItemKind);

static uint32_t owner(HSD_GObj* caller, uint32_t id, ItemKind kind)
{
    return exists(caller, id, kind) ? UINT32_C(0xFFFFFFF1) : 0;
}

static bool charging(uint32_t id)
{
    ++charge_calls;
    assert(id == (live ? UINT32_C(0xFFFFFFF1) : 0));
    return live;
}

static bool exists(HSD_GObj* caller, uint32_t id, ItemKind kind)
{
    assert(caller && caller->user_data);
    assert(id == UINT32_C(0xFEDCBA98) && kind == It_Kind_Ness_PKFire_Flame);
    ++calls; return live;
}

static int32_t integer(HSD_GObj* caller, uint32_t id, ItemKind kind,
                       unsigned field, int32_t index)
{
    assert(field == 2 && index == 0);
    return exists(caller, id, kind) ? INT32_MIN + 5 : 0;
}

static bool boolean(HSD_GObj* caller, uint32_t id, ItemKind kind,
                    unsigned field, int32_t index)
{
    assert(field == 0 && index == 0); return exists(caller, id, kind);
}

static float floating(HSD_GObj* caller, uint32_t id, ItemKind kind,
                      unsigned field, int32_t index)
{
    assert(field == 1 && (index == -1 || index == 2));
    negative_indices += index == -1;
    return exists(caller, id, kind) && index == 2 ? -0.0f : 0.0f;
}

static unsigned offset(const NativeFighterDefinition* definition,
                        const char* name)
{
    for (unsigned i = 0; i < definition->state_field_count; ++i)
        if (!strcmp(definition->state_fields[i].name, name))
            return definition->state_fields[i].offset;
    assert(0); return 0;
}

static uint32_t word(const uint8_t* bytes, unsigned at)
{
    uint32_t value; memcpy(&value, bytes + at, 4); return value;
}

int main(void)
{
    NativeFighterHost host = { .article_ref_exists = exists,
        .article_ref_int = integer, .article_ref_bool = boolean,
        .article_ref_float = floating, .article_ref_current_owner = owner,
        .target_smash_charge_active = charging };
    MotionState motion = { 0 };
    NativeFighterDefinition base = { .fighter_root = "ftDataNess",
        .motion_state_first = 341, .motion_state_count = 1,
        .motion_states = &motion };
    NativeFighterHost missing = host;
    missing.article_ref_exists = NULL; assert(!FreakMelee_GetFighterModule(&base, &missing));
    missing = host; missing.article_ref_int = NULL; assert(!FreakMelee_GetFighterModule(&base, &missing));
    missing = host; missing.article_ref_bool = NULL; assert(!FreakMelee_GetFighterModule(&base, &missing));
    missing = host; missing.article_ref_float = NULL; assert(!FreakMelee_GetFighterModule(&base, &missing));
    missing = host; missing.article_ref_current_owner = NULL; assert(!FreakMelee_GetFighterModule(&base, &missing));
    missing = host; missing.target_smash_charge_active = NULL; assert(!FreakMelee_GetFighterModule(&base, &missing));
    const NativeFighterDefinition* definition = FreakMelee_GetFighterModule(&base, &host);
    assert(definition && definition->article_logic_count == 2);
    const NativeFighterArticleLogic* ball = &definition->article_logics[0];
    const NativeFighterArticleLogic* trail = &definition->article_logics[1];
    assert(ball->state_field_count == 5 && trail->state_field_count == 3);
    assert(!strcmp(ball->state_fields[0].name, "parent"));
    assert(!strcmp(trail->state_fields[0].name, "active"));
    assert(trail->state_fields[1].count == 8 && trail->state_fields[1].offset == 4);
    Fighter* fighter = calloc(1, sizeof(*fighter));
    Item* item = calloc(1, sizeof(*item)); assert(fighter && item);
    HSD_GObj fg = { .user_data = fighter }, ig = { .user_data = item };
    item->entity = &ig;
    for (unsigned pass = 0; pass < 2; ++pass) {
        live = pass == 0; calls = negative_indices = charge_calls = 0;
        fighter->input.held_buttons[0] = pass == 0 ? HSD_PAD_DPADUP : HSD_PAD_Z;
#if ARTICLE_REFS_CALLBACKS
        definition->motion_states[0].phys_cb(&fg);
#else
        definition->on_load(&fg);
#endif
        const uint8_t* state = fighter->native_module_state;
        assert(word(state, offset(definition, "handle")) == UINT32_C(0xFEDCBA98));
        assert(word(state, offset(definition, "value")) == (live ? UINT32_C(0x80000005) : 0));
        assert(word(state, offset(definition, "helper_value")) == (live ? UINT32_C(0x80000005) : 0));
        assert(state[offset(definition, "active")] == live);
        assert(word(state, offset(definition, "sampled")) == (live ? UINT32_C(0x80000000) : 0));
        assert(word(state, offset(definition, "owner")) == (live ? UINT32_C(0xFFFFFFF1) : 0));
        assert(word(state, offset(definition, "helper_owner")) == (live ? UINT32_C(0xFFFFFFF1) : 0));
        assert(state[offset(definition, "charging")] == live);
        assert(state[offset(definition, "up_held")] == (pass == 0));
        assert(state[offset(definition, "z_held")] == (pass == 1));
        ball->logic->states[0].physics_updated(&ig);
        state = item->xDD4_itemVar.native_article.state;
        assert(word(state, ball->state_fields[0].offset) == (live ? UINT32_C(0x80000005) : 0));
        assert(state[ball->state_fields[1].offset] == live);
        assert(word(state, ball->state_fields[2].offset) == (live ? UINT32_C(0x80000000) : 0));
        assert(word(state, ball->state_fields[3].offset) == (live ? UINT32_C(0xFFFFFFF1) : 0));
        assert(state[ball->state_fields[4].offset] == live);
        assert(calls == 13 && negative_indices == 2 && charge_calls == 2);
    }
    free(item); free(fighter);
    puts("Generated fighter/item article refs preserve high-bit IDs, typed results, descriptor indices and Host requirements");
    return 0;
}
