#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include ARTICLE_SURFACE_MODULE

static HSD_GObj object;
static Item item;
static ItemAttr attributes;
static unsigned checks, walls, ceiling, floor_calls, reflection_calls, effect_calls;
static bool reflection;
static const NativeFighterArticleLogic* article;
#define CHECK(v) do { ++checks; assert(v); } while (0)

static bool alive(HSD_GObj* source, uint32_t generation, uint32_t cls)
{
    CHECK(source == &object && generation == UINT32_C(0x80000001));
    CHECK(cls == NativeCallbackSource_Item); return true;
}
static s32 wall_flags(Item_GObj* source) { CHECK(source == &object); return walls; }
static s32 ceiling_flags(Item_GObj* source) { CHECK(source == &object); return ceiling; }
static bool reflect(Item_GObj* source) { CHECK(source == &object); ++reflection_calls; return reflection; }
static void align(Item_GObj* source) { CHECK(source == &object); ++floor_calls; }
static void effect(Item_GObj* source, int id, const char* path, unsigned mode)
{
    CHECK(source == &object);
    if (effect_calls == 0) { CHECK(id == 16777217 && !strcmp(path, "0/1") && mode == 0); }
    else { CHECK(effect_calls == 1 && id == 16777218 && !strcmp(path, "") && mode == 3); }
    ++effect_calls;
}
static unsigned offset(const char* name)
{
    for (unsigned i = 0; i < article->state_field_count; ++i)
        if (!strcmp(article->state_fields[i].name, name)) return article->state_fields[i].offset;
    assert(0); return 0;
}
static int32_t integer(const char* name) { int32_t value; memcpy(&value, item.xDD4_itemVar.native_article.state + offset(name), 4); return value; }
static float real(const char* name) { float value; memcpy(&value, item.xDD4_itemVar.native_article.state + offset(name), 4); return value; }
static bool flag(const char* name) { return item.xDD4_itemVar.native_article.state[offset(name)] != 0; }

int main(void)
{
    MotionState motion = {0};
    NativeFighterDefinition base = {.fighter_root = "ftDataTest", .motion_state_first = 341,
        .motion_state_count = 1, .motion_states = &motion};
    NativeFighterHost host = {.callback_source_alive = alive,
        .item_wall_contact_flags = wall_flags, .item_ceiling_contact_flags = ceiling_flags,
        .item_reflect_from_wall = reflect, .item_align_to_floor = align,
        .item_spawn_async_joint_effect = effect};
    const NativeFighterDefinition* definition = FreakMelee_GetFighterModule(&base, &host);
    CHECK(definition && definition->article_logic_count == 1);
    article = definition->article_logics;
    NativeFighterHost missing = host;
    missing.item_wall_contact_flags = NULL; CHECK(!FreakMelee_GetFighterModule(&base, &missing));
    missing = host; missing.item_ceiling_contact_flags = NULL; CHECK(!FreakMelee_GetFighterModule(&base, &missing));
    missing = host; missing.item_align_to_floor = NULL; CHECK(!FreakMelee_GetFighterModule(&base, &missing));
    missing = host; missing.item_reflect_from_wall = NULL; CHECK(!FreakMelee_GetFighterModule(&base, &missing));
    missing = host; missing.item_spawn_async_joint_effect = NULL; CHECK(!FreakMelee_GetFighterModule(&base, &missing));
    CHECK(FreakMelee_GetFighterModule(&base, &host));
    const float coefficients[] = {0, 0.25f, 1, 1.5f};
    for (walls = 0; walls < 16; ++walls) for (ceiling = 0; ceiling < 8; ++ceiling)
    for (unsigned r = 0; r < 2; ++r) for (unsigned c = 0; c < 4; ++c) {
        memset(&item, 0, sizeof(item)); object.user_data = &item;
        item.entity = &object; item.x1C = (s32)UINT32_C(0x80000001);
        item.xCC_item_attr = &attributes; attributes.x58 = coefficients[c];
        int32_t id = 16777217; memcpy(item.xDD4_itemVar.native_article.state + offset("effect"), &id, 4);
        reflection = r != 0; reflection_calls = floor_calls = effect_calls = 0;
        item.xDCD_flag.b3 = 1; item.xDCD_flag.b5 = 1;
        article->logic->states[0].physics_updated(&object);
        CHECK(integer("walls") == (int32_t)walls && integer("ceiling") == (int32_t)ceiling);
        CHECK(flag("reflected") == reflection && floor_calls == 1 && reflection_calls == 1);
        CHECK(item.xDCD_flag.b4 && item.xDCD_flag.b3 && item.xDCD_flag.b5 && flag("stopped"));
        CHECK(real("restitution") == coefficients[c] && effect_calls == 2);
    }
    printf("PASS %u generated article surface/effect checks; native surface and queue bodies are separate engine fixtures\n", checks);
    return 0;
}
