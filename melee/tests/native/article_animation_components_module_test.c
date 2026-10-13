#undef NDEBUG
#include <assert.h>
#include <string.h>
#include <windows.h>
#if ARTICLE_COMPONENTS_CALLBACKS
#include "article_components_callback_fixture.h"
#else
#include "article_components_action_fixture.h"
#endif

static HSD_GObj* registered;
static Item* registered_item;
static uint32_t registered_generation;
static unsigned calls, retire_at;
static bool fail_add;
static void observed(Item_GObj* object, unsigned model, const char* path)
{
    assert(object == registered && model == 0);
    assert(strcmp(path, calls < 4 ? "" : "0") == 0);
    ++calls;
    if (calls == retire_at) {
        registered = NULL;
        DWORD old;
        assert(VirtualProtect(registered_item, sizeof(*registered_item), PAGE_NOACCESS, &old));
    }
}
static bool add_animation(Item_GObj* object, unsigned root, const char* path, uint32_t name)
{
    assert(name == scripted_article_0_model_animations[calls < 4 ? 0 : 1].name_hash);
    observed(object, root, path);
    return !(fail_add && calls == 5);
}
static void request_animation(Item_GObj* object, unsigned root, const char* path, float frame)
{
    assert(frame == (calls < 4 ? 2 : 9)); observed(object, root, path);
}
static void advance_animation(Item_GObj* object, unsigned root, const char* path)
{
    observed(object, root, path);
}
static void remove_animation(Item_GObj* object, unsigned root, const char* path)
{
    observed(object, root, path);
}
static bool alive(HSD_GObj* object, uint32_t generation, uint32_t kind)
{
    assert(kind == NativeCallbackSource_Item);
    // Registry lookup precedes any item read; retired userdata is PAGE_NOACCESS.
    return object && object == registered && generation == registered_generation &&
           object->user_data == registered_item && registered_item->x1C == generation;
}

int main(void)
{
    MotionState motion = { 0 };
    NativeFighterDefinition base = { .fighter_root = "ftDataNess",
        .motion_state_first = 341, .motion_state_count = 1, .motion_states = &motion };
    NativeFighterHost host = { .item_add_model_animation = add_animation,
        .item_request_model_animation = request_animation,
        .item_advance_model_animation = advance_animation,
        .item_remove_model_animation = remove_animation,
        .callback_source_alive = alive };
    const NativeFighterDefinition* definition = FreakMelee_GetFighterModule(&base, &host);
    assert(definition && definition->article_logic_count == 1);
    const NativeFighterArticleLogic* article = &definition->article_logics[0];
    assert(article->model_animation_count == 4);
    const NativeFighterArticleModelAnimation* models = article->model_animations;
    assert(models[0].components[0].attribute_offset == UINT32_MAX && models[0].components[1].attribute_offset == 88 &&
           models[0].components[2].attribute_offset == UINT32_MAX);
    assert(models[1].components[0].attribute_offset == 80 && models[1].components[1].attribute_offset == 84 && models[1].components[2].attribute_offset == 88);
    assert(models[2].components[0].attribute_offset == 8 && models[2].components[1].attribute_offset == UINT32_MAX && models[2].components[2].attribute_offset == 24);
    assert(models[3].components[0].attribute_offset == 116 && models[3].components[0].array_index == 0);
    assert(models[3].components[1].attribute_offset == 120 && models[3].components[1].array_index == 2);
    assert(models[3].components[2].attribute_offset == 124 && models[3].components[2].array_index == 1);
    assert(models[0].components[1].array_index == UINT32_MAX);
    Item* item = VirtualAlloc(NULL, sizeof(*item), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    assert(item); HSD_GObj object = { .user_data = item };
    static const unsigned retirement[] = { 0, 1, 3, 4, 5, 7, 8 };
    for (unsigned i = 0; i < sizeof(retirement)/sizeof(*retirement); ++i) {
        memset(item, 0, sizeof(*item)); item->x1C = 0x80000001u;
        registered = &object; registered_item = item; registered_generation = item->x1C;
        calls = 0; retire_at = retirement[i];
        article->logic->states[0].physics_updated(&object);
        DWORD old; assert(VirtualProtect(item, sizeof(*item), PAGE_READWRITE, &old));
        assert(calls == (retire_at ? retire_at : 8));
        assert(scripted_article_state_int(item, 0) == (retire_at ? 0 : 23));
        assert(scripted_article_state_bool(item, 4) == (!retire_at || retire_at > 5));
    }
    memset(item, 0, sizeof(*item)); item->x1C = registered_generation;
    registered = &object; calls = 0; retire_at = 0; fail_add = true;
    article->logic->states[0].physics_updated(&object);
    assert(calls == 5 && scripted_article_state_int(item, 0) == -9);
    assert(!scripted_article_state_bool(item, 4));
    assert(VirtualFree(item, 0, MEM_RELEASE));
    puts("Actual generated module publishes component offsets, ordered root/subtree calls and six retired-source stops");
    return 0;
}
