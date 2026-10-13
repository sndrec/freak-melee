#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if STATE_ARRAY_CALLBACKS
#include "script_state_callback_fixture.h"
#else
#include "script_state_action_fixture.h"
#endif

static bool creator_valid = true;
static HSD_GObj* lifetime_fighters[2];
static HSD_GObj* lifetime_items[2];
static bool fixture_source_alive(HSD_GObj* object, uint32_t generation, uint32_t kind)
{
    for (unsigned i = 0; i < 2; ++i) {
        if (kind == NativeCallbackSource_Fighter && object == lifetime_fighters[i])
            return ((Fighter*)object->user_data)->x8_spawnNum == generation;
        if (kind == NativeCallbackSource_Item && object == lifetime_items[i])
            return ((Item*)object->user_data)->x1C == generation;
    }
    assert(0); return false;
}
static Fighter_GObj* fixture_creator(Item_GObj* object)
{ return creator_valid && object ? ((Item*)object->user_data)->xDD4_itemVar.native_article.attachment_owner : NULL; }

static const NativeFighterStateField* field(const NativeFighterDefinition* definition,
                                           const char* name)
{
    for (unsigned i = 0; i < definition->state_field_count; ++i)
        if (strcmp(definition->state_fields[i].name, name) == 0)
            return &definition->state_fields[i];
    assert(0); return NULL;
}

int main(void)
{
    NativeFighterHost host = { .item_creator = fixture_creator, .callback_source_alive = fixture_source_alive };
    MotionState motion = { 0 };
    NativeFighterStateField base_field = { "base_value", 0, 1, NativeFighterState_Int, 4, false };
    NativeFighterDefinition base = { .fighter_root = "ftDataMario",
        .motion_state_first = 341, .motion_state_count = 1, .motion_states = &motion,
        .state_bytes = 8, .state_field_count = 1, .state_fields = &base_field };
    NativeFighterHost missing = host; missing.item_creator = NULL;
    assert(FreakMelee_GetFighterModule(&base, &missing) == NULL);
    missing = host; missing.callback_source_alive = NULL;
    assert(FreakMelee_GetFighterModule(&base, &missing) == NULL);
    const NativeFighterDefinition* definition = FreakMelee_GetFighterModule(&base, &host);
    assert(definition && definition->state_field_count == 5);
    unsigned values_offset = field(definition, "values")->offset;
    unsigned cursor_offset = field(definition, "cursor")->offset;
    unsigned enabled_offset = field(definition, "enabled")->offset;
    unsigned latched_offset = field(definition, "latched")->offset;
    assert(field(definition, "values")->count == 16 && field(definition, "values")->stride == 4);
    assert(field(definition, "base_value")->offset == 0);
    assert(values_offset == 16 && cursor_offset == 12);
    Fighter* fighters = calloc(2, sizeof(*fighters)); assert(fighters);
    Item* items = calloc(2, sizeof(*items)); assert(items);
    HSD_GObj fighter_objects[2] = { 0 }, item_objects[2] = { 0 };
    for (unsigned i = 0; i < 2; ++i) {
        fighter_objects[i].user_data = &fighters[i];
        item_objects[i].user_data = &items[i];
        lifetime_fighters[i] = &fighter_objects[i]; lifetime_items[i] = &item_objects[i];
        fighters[i].x8_spawnNum = UINT32_C(0x80000001) + i;
        items[i].x1C = UINT32_C(16777217) + i;
        items[i].entity = &item_objects[i];
        items[i].owner = &fighter_objects[i];
        items[i].xDD4_itemVar.native_article.attachment_owner = &fighter_objects[i];
        fighters[i].self_vel.x = i ? -7.0f : 3.0f;
        fighters[i].cur_pos.x = i ? -42 : 42;
        fighters[i].input.pressed_buttons = i ? 0 : HSD_PAD_A;
#if STATE_ARRAY_CALLBACKS
        definition->motion_states[0].phys_cb(&fighter_objects[i]);
#else
        definition->on_load(&fighter_objects[i]);
#endif
        assert(fighters[i].self_vel.y == fighters[i].self_vel.x);
        assert(fighters[i].native_module_state[latched_offset] == (i == 0));
        assert(fighters[i].native_module_state[enabled_offset] == (i == 0));
        assert(fighters[i].x221B_b3 == (i == 0));
        int32_t cursor; memcpy(&cursor, fighters[i].native_module_state + cursor_offset, 4);
        assert(cursor == 1);
        const NativeFighterArticleLogic* article = &definition->article_logics[0];
        assert(article->local_state_bytes == 512 && article->state_field_count == 1);
        assert(article->state_fields[0].count == 128 && article->state_fields[0].offset == 0);
        items[i].xC98 = i ? 0 : 0x100;
        article->logic->states[0].physics_updated(&item_objects[i]);
        assert(fighters[i].native_module_state[latched_offset] == (i == 0));
        assert(items[i].xDCD_flag.b5 == (i == 0));
        assert(items[i].x40_vel.x == fighters[i].self_vel.x);
        float history, transferred;
        memcpy(&history, items[i].xDD4_itemVar.native_article.state + 508, 4);
        memcpy(&transferred, fighters[i].native_module_state + values_offset + 4, 4);
        assert(history == fighters[i].self_vel.x && transferred == history + 1);
        memcpy(&history, fighters[i].native_module_state + values_offset + 8, 4);
        assert(history == fighters[i].cur_pos.x);
    }
    for (int32_t index = -1; index <= 16; index += 17) {
        memcpy(fighters[0].native_module_state + cursor_offset, &index, 4);
        uint8_t before[64]; memcpy(before, fighters[0].native_module_state + values_offset, 64);
#if STATE_ARRAY_CALLBACKS
        definition->motion_states[0].phys_cb(&fighter_objects[0]);
#else
        definition->on_load(&fighter_objects[0]);
#endif
        assert(fighters[0].self_vel.y == 0);
        assert(memcmp(before, fighters[0].native_module_state + values_offset, 64) == 0);
    }
    // No current matching owner: reads return typed zero and writes leave the former owner untouched.
    items[0].owner = &fighter_objects[1];
    definition->article_logics[0].logic->states[0].physics_updated(&item_objects[0]);
    assert(items[0].x40_vel.x == 0 && fighters[0].self_vel.y == 0);
    float creator_value;
    memcpy(&creator_value, fighters[0].native_module_state + values_offset + 12, 4);
    assert(creator_value == 6); // Transfer does not redirect creator state/functions.
    creator_valid = false;
    definition->article_logics[0].logic->states[0].physics_updated(&item_objects[0]);
    float after_invalid;
    memcpy(&after_invalid, fighters[0].native_module_state + values_offset + 12, 4);
    assert(after_invalid == creator_value);
    creator_valid = true;
    // Repeated open starts from declaration offsets; switching bases never accumulates them.
    base.state_bytes = 3;
    definition = FreakMelee_GetFighterModule(&base, &host);
    assert(definition && field(definition, "values")->offset == 12);
    base.state_bytes = 12;
    definition = FreakMelee_GetFighterModule(&base, &host);
    assert(definition && field(definition, "values")->offset == 20);
    definition = FreakMelee_GetFighterModule(&base, &host);
    assert(definition && field(definition, "values")->offset == 20);
    base_field.name = "values";
    assert(FreakMelee_GetFighterModule(&base, &host) == NULL);
    base_field.name = "base_value"; base.state_field_count = 62;
    assert(FreakMelee_GetFighterModule(&base, &host) == NULL);
    free(items); free(fighters);
    puts("Actual generated arrays preserve fighter/article instances and metadata across module opens");
    return 0;
}
