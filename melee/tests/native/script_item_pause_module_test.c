#undef NDEBUG
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if PAUSE_QUERY_CALLBACKS
#include "script_item_pause_callback_fixture.h"
#else
#include "script_item_pause_action_fixture.h"
#endif

#include "script_item_pause_originals.h"

static HSD_GObj* creator_object;
static unsigned assertions, pause_calls, resume_calls;
#define CHECK(value) do { ++assertions; assert(value); } while (0)

static HSD_GObj* creator(Item_GObj* object)
{
    CHECK(object && object->user_data);
    return creator_object;
}

static void occupied(HSD_GObj* object) { (void)object; assert(0); }

static unsigned offset(const NativeFighterStateField* fields, unsigned count,
                       const char* name)
{
    for (unsigned i = 0; i < count; ++i)
        if (!strcmp(fields[i].name, name)) return fields[i].offset;
    assert(0); return 0;
}

static const NativeFighterArticleSignal* signal(const NativeFighterArticleLogic* article,
                          uint32_t hash)
{
    for (unsigned i = 0; i < article->signal_count; ++i)
        if (article->signals[i].name_hash == hash) return &article->signals[i];
    assert(0); return NULL;
}

static uint32_t hash(const char* text)
{
    uint32_t value = UINT32_C(2166136261);
    while (*text) { value ^= (unsigned char)*text++; value *= UINT32_C(16777619); }
    return value;
}

int main(void)
{
    MotionState motion = { 0 };
    NativeFighterDefinition base = { .fighter_root = "ftDataLink",
        .motion_state_first = 341, .motion_state_count = 1, .motion_states = &motion };
    NativeFighterHost host = { .item_creator = creator,
        .item_pause = it_8026B724, .item_resume = it_8026B73C };
    const NativeFighterDefinition* definition = FreakMelee_GetFighterModule(&base, &host);
    CHECK(definition && definition->article_logic_count == 1 && definition->item_count == 1);
    NativeFighterHost missing = host; missing.item_pause = NULL;
    CHECK(!FreakMelee_GetFighterModule(&base, &missing));
    missing = host; missing.item_resume = NULL;
    CHECK(!FreakMelee_GetFighterModule(&base, &missing));
    missing = host; missing.item_creator = NULL;
    CHECK(!FreakMelee_GetFighterModule(&base, &missing));
    // Restore the exact accepted host after rejection; no stock handler delegation.
    definition = FreakMelee_GetFighterModule(&base, &host); CHECK(definition);
    Fighter* fighter = calloc(1, sizeof(*fighter));
    Item* item = calloc(1, sizeof(*item)); CHECK(fighter && item);
    HSD_GObj fg = { .user_data = fighter }, ig = { .user_data = item };
    item->entity = &ig;
    const NativeFighterArticleLogic* article = &definition->article_logics[0];
    const NativeFighterArticleSignal* pause = signal(article, hash("pause"));
    const NativeFighterArticleSignal* resume = signal(article, hash("resume"));
    // Exhaust all eight relevant original flag combinations, plus preserved bits.
    for (unsigned x7 = 0; x7 < 2; ++x7)
    for (unsigned x5 = 0; x5 < 2; ++x5)
    for (unsigned x3 = 0; x3 < 2; ++x3) {
        memset(&item->xDC8_word, 0, sizeof(item->xDC8_word));
        item->xDC8_word.flags.x7 = x7; item->xDC8_word.flags.x5 = x5;
        item->xDC8_word.flags.x3 = x3;
        item->xDC8_word.flags.x6 = true; item->xDC8_word.flags.x4 = true;
        resume->callback(&ig, 0); ++resume_calls;
        CHECK(item->xDC8_word.flags.x3 == 0);
        CHECK(item->xDC8_word.flags.x5 == (x7 || x5));
        CHECK(item->xDC8_word.flags.x7 == x7);
        CHECK(item->xDC8_word.flags.x6 && item->xDC8_word.flags.x4);
        pause->callback(&ig, 0); ++pause_calls;
        CHECK(item->xDC8_word.flags.x3 == 1);
        CHECK(item->xDC8_word.flags.x5 == (x7 || x5));
        CHECK(item->xDC8_word.flags.x7 == x7);
        resume->callback(&ig, 0); ++resume_calls;
        CHECK(item->xDC8_word.flags.x3 == 0);
        CHECK(item->xDC8_word.flags.x5 == (x7 || x5));
    }
    CHECK(definition->items[0].on_thrown && definition->items[0].on_tick);
    item->xDC8_word.flags.x3 = false; item->xDC8_word.flags.x5 = false;
    item->xDC8_word.flags.x7 = true;
    definition->items[0].on_thrown(&ig); ++pause_calls;
    CHECK(item->xDC8_word.flags.x3 && !item->xDC8_word.flags.x5);
    definition->items[0].on_tick(&ig); ++resume_calls;
    CHECK(!item->xDC8_word.flags.x3 && item->xDC8_word.flags.x5);
    unsigned scale_offset = offset(article->state_fields, article->state_field_count, "scale");
    for (unsigned present = 0; present < 2; ++present) {
        creator_object = present ? &fg : NULL;
        // The current damage owner is deliberately foreign; immutable creator wins.
        item->owner = NULL;
        fighter->x34_scale.y = 2.25f;
        article->on_created(&ig);
        float scale; memcpy(&scale, item->xDD4_itemVar.native_article.state + scale_offset, 4);
        CHECK(scale == (present ? 2.25f : 1.0f));
    }
    unsigned damage_offset = offset(definition->state_fields, definition->state_field_count, "damage_empty");
    unsigned death_offset = offset(definition->state_fields, definition->state_field_count, "death_empty");
    unsigned reflecting_offset = offset(definition->state_fields, definition->state_field_count, "reflected");
    for (unsigned occupied_damage = 0; occupied_damage < 2; ++occupied_damage)
    for (unsigned occupied_death = 0; occupied_death < 2; ++occupied_death)
    for (unsigned reflecting = 0; reflecting < 2; ++reflecting) {
        fighter->take_dmg_cb = occupied_damage ? occupied : NULL;
        fighter->death2_cb = occupied_death ? occupied : NULL;
        fighter->reflecting = reflecting;
#if PAUSE_QUERY_CALLBACKS
        definition->motion_states[0].phys_cb(&fg);
#else
        definition->on_load(&fg);
#endif
        CHECK(fighter->native_module_state[damage_offset] == !occupied_damage);
        CHECK(fighter->native_module_state[death_offset] == !occupied_death);
        CHECK(fighter->native_module_state[reflecting_offset] == reflecting);
        CHECK(fighter->take_dmg_cb == (occupied_damage ? occupied : NULL));
        CHECK(fighter->death2_cb == (occupied_death ? occupied : NULL));
    }
    free(item); free(fighter);
    printf("{\"module_assertions\":%u,\"actual_pause_calls\":%u,\"actual_resume_calls\":%u}\n",
           assertions, pause_calls, resume_calls);
    return 0;
}
