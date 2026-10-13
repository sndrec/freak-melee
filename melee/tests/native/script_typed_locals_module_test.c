#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#if TYPED_LOCALS_CALLBACKS
#include "script_typed_locals_callback_fixture.h"
#else
#include "script_typed_locals_action_fixture.h"
#endif

static HSD_GObj* creator_object;
static bool source_alive(HSD_GObj* object, uint32_t generation, uint32_t kind)
{
    assert(object && object->user_data);
    return kind == NativeCallbackSource_Item
        ? ((Item*)object->user_data)->x1C == generation
        : ((Fighter*)object->user_data)->x8_spawnNum == generation;
}
static HSD_GObj* creator(Item_GObj* object)
{
    assert(object && object->user_data);
    return creator_object;
}

static unsigned offset(const NativeFighterStateField* fields, unsigned count,
                        const char* name)
{
    for (unsigned i = 0; i < count; ++i)
        if (!strcmp(fields[i].name, name)) return fields[i].offset;
    assert(0); return 0;
}

static uint32_t word(const uint8_t* bytes, unsigned at)
{
    uint32_t value; memcpy(&value, bytes + at, 4); return value;
}

int main(void)
{
    MotionState motion = { 0 };
    NativeFighterDefinition base = { .fighter_root = "ftDataNess",
        .motion_state_first = 341, .motion_state_count = 1, .motion_states = &motion };
    NativeFighterHost host = { .item_creator = creator, .callback_source_alive = source_alive };
    const NativeFighterDefinition* definition = FreakMelee_GetFighterModule(&base, &host);
    assert(definition && definition->article_logic_count == 1);
    NativeFighterHost missing = { 0 };
    assert(!FreakMelee_GetFighterModule(&base, &missing));
    Fighter* fighter = calloc(1, sizeof(*fighter));
    Item* item = calloc(1, sizeof(*item)); assert(fighter && item);
    HSD_GObj fg = { .user_data = fighter }, ig = { .user_data = item };
    item->entity = &ig;
    item->xDD4_itemVar.native_article.attachment_owner = &fg;
    creator_object = &fg;
#if TYPED_LOCALS_CALLBACKS
    definition->motion_states[0].phys_cb(&fg);
#else
    definition->on_load(&fg);
#endif
#define FWORD(name) word(fighter->native_module_state, offset(definition->state_fields, definition->state_field_count, name))
    assert(FWORD("above") == UINT32_C(16777219));
    assert(FWORD("high") == UINT32_C(0x80000001));
    assert(FWORD("negative") == UINT32_MAX);
    assert(FWORD("quotient") == 4);
    assert(FWORD("received") == UINT32_C(0x80000001));
    assert(FWORD("old") == UINT32_C(0x4B800000)); // untyped let remains float, 16777216.
    assert(FWORD("amount") == UINT32_C(0x3FA00000));
    assert(fighter->native_module_state[offset(definition->state_fields, definition->state_field_count, "ready")] == 1);
    assert(definition->on_jab_start(&fg) == 16777217);
    const NativeFighterArticleLogic* article = &definition->article_logics[0];
    uint32_t ids[] = { UINT32_C(16777217), UINT32_C(0x80000001), UINT32_MAX };
    for (unsigned i = 0; i < sizeof(ids) / sizeof(ids[0]); ++i) {
        item->x1C = ids[i];
        article->on_created(&ig);
        const uint8_t* bytes = item->xDD4_itemVar.native_article.state;
#define IWORD(name) word(bytes, offset(article->state_fields, article->state_field_count, name))
        assert(IWORD("id") == ids[i]);
        assert(IWORD("high") == UINT32_C(0xDEADBEEF));
        assert(IWORD("negative") == UINT32_MAX);
        assert(IWORD("sum") == UINT32_C(16777219));
        assert(IWORD("old") == UINT32_C(0x4B800000));
        assert(bytes[offset(article->state_fields, article->state_field_count, "active")] == 1);
        assert(bytes[offset(article->state_fields, article->state_field_count, "nonzero")] == 1);
        assert(FWORD("received") == ids[i]);
    }
    free(item); free(fighter);
    puts("Generated fighter/article callbacks retain all32IDbits, typed helper arguments, bool truth and integer returns.");
    return 0;
}
