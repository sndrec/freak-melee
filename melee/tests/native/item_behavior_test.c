#include <melee/ft/native_script_item_program.h>
// Actual asset/callback selector and constructors; allocator/model work is excluded.
#include <melee/it/native_item_behavior.h>
#include <melee/it/item.h>
#include <melee/it/types.h>
#include <melee/it/kinds/types.h>
#include <melee/it/itdraw.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/types.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
HSD_GObj** HSD_GObjPLinkHead;
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Article engine_assets, parent_assets, author_assets;
static Article* engine_table[It_Kind_Kuriboh];
Article** it_804D6D24 = engine_table;
static ItemAttr attrs;
static ItemModelDesc model;
static ItemLogicTable engine_logic, parent_logic, author_logic;
static NativeFighterArticleLogic parent_entry, author_entry;
static NativeFighterDefinition definition;
static Fighter parent;
static HSD_GObj parent_object, result_object;
static Item result;
static bool parent_has_assets, parent_has_logic, author_loaded, failed_allocation;
static unsigned queries, preparations, allocations, checks;
static SpawnItem observed;
static NativeItemBehaviorSelection observed_selection;
static FighterKind observed_author;

static void require(bool condition, const char* why)
{
    ++checks;
    if (!condition) { fprintf(stderr, "%s\n", why); exit(1); }
}
void __assert(char* file, u32 line, char* condition)
{ fprintf(stderr, "%s:%u %s\n", file, (unsigned)line, condition); abort(); }
void OSReport(char* format, ...) { (void) format; }
void it_8026EECC(HSD_GObj* object, int pass) { (void) object; (void) pass; }
static void engine_draw(HSD_GObj* object, int pass) { (void) object; (void) pass; }
bool ftLib_80086960(HSD_GObj* object)
{ ++queries; return object == &parent_object; }
bool nativeFighterRuntime_ResolveArticleForItem(const Fighter* fighter,
                                               ItemKind kind, Article** assets)
{
    ++queries; require(fighter == &parent && kind == It_Kind_BombHei, "parent asset identity");
    *assets = &parent_assets;
    return parent_has_assets;
}
const NativeFighterArticleLogic* nativeFighterRuntime_ArticleLogic(
    const Fighter* fighter, ItemKind kind)
{
    ++queries; require(fighter == &parent && kind == It_Kind_BombHei, "parent logic identity");
    return parent_has_logic ? &parent_entry : NULL;
}
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind author)
{
    ++queries;
    return author_loaded && author == Ft_Kind_Ness ? &definition : NULL;
}
Article* nativeFighterRuntime_ArticleAssets(FighterKind author, ItemKind kind)
{
    ++queries;
    return author_loaded && author == Ft_Kind_Ness && kind == It_Kind_BombHei ?
           &author_assets : NULL;
}
static void Item_802674AC(SpawnItem* spawn)
{
    ++preparations;
    // Hold-kind classification is deliberately an isolated engine boundary.
    spawn->hold_kind = 3;
}
static HSD_GObj* Item_8026862C(SpawnItem* spawn,
                              NativeItemBehaviorSelection selection, FighterKind author,uint32_t source_owner)
{
    if(source_owner)abort();
    ++allocations; observed = *spawn;
    observed_selection = selection; observed_author = author;
    if (failed_allocation) return NULL;
    nativeItemBehavior_Select(&result_object, spawn, selection, author,0);
    return &result_object;
}
#include "item_behavior_fixture.h"
#include "item_behavior_original.inc"

static void reset(void)
{
    memset(&result, 0x5a, sizeof(result));
    result.kind = It_Kind_BombHei;
    result.native_article_author_kind = Ft_Kind_None;
    result.native_script_owner = 0;
    result.xC4_article_data = &engine_assets;
    result.xB8_itemLogicTable = &engine_logic;
    result.xBC_itemStateContainer = engine_logic.states;
    result_object = (HSD_GObj){ .user_data = &result, .render_cb = engine_draw };
    parent = (Fighter){ .kind = Ft_Kind_Mario };
    parent_object = (HSD_GObj){ .user_data = &parent };
    parent_entry = (NativeFighterArticleLogic){ .kind = It_Kind_BombHei,
                                             .logic = &parent_logic };
    author_entry = (NativeFighterArticleLogic){ .kind = It_Kind_BombHei,
                                             .logic = &author_logic };
    definition = (NativeFighterDefinition){ .article_logics = &author_entry,
                                          .article_logic_count = 1 };
    queries = preparations = allocations = 0;
    author_loaded = true; failed_allocation = false;
}
int main(void)
{
    engine_assets.x0_common_attr = &attrs;
    engine_assets.x10_modelDesc = &model;
    engine_table[It_Kind_BombHei] = &engine_assets;
    require(!nativeItemBehavior_EngineAssetsAvailable((ItemKind)-1) &&
            !nativeItemBehavior_EngineAssetsAvailable(It_Kind_Kuriboh) &&
            !nativeItemBehavior_EngineAssetsAvailable(It_Kind_Dosei), "invalid/missing common assets");
    require(nativeItemBehavior_EngineAssetsAvailable(It_Kind_BombHei), "loaded engine assets");
    engine_assets.x0_common_attr = NULL;
    require(!nativeItemBehavior_EngineAssetsAvailable(It_Kind_BombHei), "missing common attributes");
    engine_assets.x0_common_attr = &attrs;
    engine_assets.x10_modelDesc = NULL;
    require(!nativeItemBehavior_EngineAssetsAvailable(It_Kind_BombHei), "missing model descriptor");
    engine_assets.x10_modelDesc = &model;
    it_804D6D24 = NULL;
    require(!nativeItemBehavior_EngineAssetsAvailable(It_Kind_BombHei), "unloaded common table");
    it_804D6D24 = engine_table;
    for (unsigned assets = 0; assets < 2; ++assets)
        for (unsigned logic = 0; logic < 2; ++logic)
            for (unsigned render = 0; render < 2; ++render)
                for (unsigned creator = 0; creator < 3; ++creator) {
                    reset(); parent_has_assets = assets; parent_has_logic = logic;
                    parent_entry.standard_render = render; author_entry.standard_render = render;
                    SpawnItem spawn = { .kind = It_Kind_BombHei,
                        .x0_parent_gobj = creator == 0 ? NULL : creator == 1 ? &parent_object : &result_object };
                    Item before = result;
                    result.native_script_owner = UINT32_C(0xdeadbeef);
                    nativeItemBehavior_Select(&result_object, &spawn, NativeItemBehavior_Engine, Ft_Kind_None,0);
                    require(!memcmp(&before, &result, sizeof(result)) && queries == 0 &&
                            result_object.render_cb == engine_draw, "engine selection retains private union/assets/logic/render without parent lookup");
                    nativeItemBehavior_Select(&result_object, &spawn, NativeItemBehavior_Parent, Ft_Kind_None,0);
                    Item candidate = result;
                    GObj_RenderFunc candidate_draw = result_object.render_cb;
                    reset(); parent_entry.standard_render = render;
                    original_item_select(&result_object, &spawn, Ft_Kind_None);
                    require(!memcmp(&candidate, &result, sizeof(result)) &&
                            candidate_draw == result_object.render_cb, "parent selection differs from previous actual source");
                    reset(); author_entry.standard_render = render;
                    nativeItemBehavior_Select(&result_object, &spawn, NativeItemBehavior_Author, Ft_Kind_Ness,0);
                    candidate = result; candidate_draw = result_object.render_cb;
                    reset(); author_entry.standard_render = render;
                    original_item_select(&result_object, &spawn, Ft_Kind_Ness);
                    require(!memcmp(&candidate, &result, sizeof(result)) &&
                            candidate_draw == result_object.render_cb, "explicit author selection differs from previous actual source");
                }
    reset(); parent_has_assets = parent_has_logic = true;
    SpawnItem spawn = { .kind = It_Kind_BombHei, .x0_parent_gobj = &parent_object,
        .x4_parent_gobj2 = &parent_object, .pos = {1,2,3}, .prev_pos = {4,5,6},
        .vel = {7,8,9}, .facing_dir = -1, .x40 = 0x80000001, .x3C_damage = 11, .x10 = 17 };
    SpawnItem before = spawn;
    require(Item_CreateEngine(NULL) == NULL, "null engine request");
    spawn.kind = It_Kind_Dosei;
    SpawnItem missing = spawn;
    require(Item_CreateEngine(&spawn) == NULL && !memcmp(&spawn, &missing, sizeof(spawn)) &&
            preparations == 0 && allocations == 0, "missing assets reject before observable writes/allocation");
    spawn = before;
    require(Item_CreateEngine(&spawn) == &result_object && preparations == 1 && allocations == 1 &&
            observed_selection == NativeItemBehavior_Engine && observed_author == Ft_Kind_None &&
            queries == 0 && result.xC4_article_data == &engine_assets &&
            result.xB8_itemLogicTable == &engine_logic, "engine constructor bypasses conflicting parent declarations");
    before.x48_ground_or_air = GA_Air; before.x10 = 0; before.hold_kind = 3;
    require(!memcmp(&observed, &before, sizeof(before)), "engine construction preserves complete caller spawn data");
    reset(); failed_allocation = true;
    require(Item_CreateEngine(&spawn) == NULL && preparations == 1 && allocations == 1,
            "allocator failure returns absent item");
    reset();
    require(Item_80268B18(&spawn) == &result_object && observed_selection == NativeItemBehavior_Parent,
            "airborne parent constructor policy");
    require(Item_80268B5C(&spawn) == &result_object && observed_selection == NativeItemBehavior_Parent &&
            observed.x48_ground_or_air == GA_Ground && observed.x10 == 0, "grounded parent constructor policy");
    Item_80268B9C(&spawn);
    require(observed_selection == NativeItemBehavior_Parent && observed.x48_ground_or_air == GA_Ground &&
            observed.x10 == 1, "grounded flagged parent constructor policy");
    require(Item_CreateAuthored(&spawn, Ft_Kind_Ness,0) == &result_object &&
            observed_selection == NativeItemBehavior_Author && observed_author == Ft_Kind_Ness,
            "explicit article constructor policy");
    reset(); author_loaded = false; before = spawn;
    require(Item_CreateAuthored(&spawn, Ft_Kind_Ness,0) == NULL && !memcmp(&before, &spawn, sizeof(spawn)) &&
            preparations == 0 && allocations == 0, "missing explicit author rejects before mutation");
    printf("Ordinary/parent/authored item policies: %u checks, 24 source configurations; allocation/model work excluded\n", checks);
    return 0;
}

// This asset-selection fixture deliberately contains no interpreted Item program.
const NativeScriptItemProgram* nativeScriptItem_Program(FighterKind kind,uint32_t owner){(void)kind;(void)owner;return NULL;}
bool nativeScriptItem_Attach(Item_GObj* object,const NativeScriptItemProgram* program){(void)object;(void)program;return false;}
