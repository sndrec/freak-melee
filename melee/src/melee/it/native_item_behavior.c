#include "native_item_behavior.h"
#include "it_3F14.h"
#include "itdraw.h"
#include "types.h"
#include "native_script_item_actor.h"
#include <melee/it/kinds/types.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/native_fighter_runtime.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/gobj.h>
#include <string.h>

bool nativeItemBehavior_EngineAssetsAvailable(ItemKind kind)
{
    if (kind < 0 || kind >= It_Kind_Kuriboh || it_804D6D24 == NULL)
        return false;
    const Article* article = it_804D6D24[kind];
    return article != NULL && article->x0_common_attr != NULL &&
           article->x10_modelDesc != NULL;
}

bool nativeItemBehavior_Select(Item_GObj* gobj, const SpawnItem* spawnItem,
                               NativeItemBehaviorSelection selection,
                               FighterKind author,uint32_t source_owner)
{
    Item* item_data = gobj->user_data;
    item_data->native_script_owner=0;
    if(source_owner) {
        FighterKind source_author=author;
        if(selection==NativeItemBehavior_Parent) {
            uint32_t generation;
            if(nativeCallbackSource_Snapshot(spawnItem->x0_parent_gobj,NativeCallbackSource_Fighter,&generation))
                source_author=((Fighter*)spawnItem->x0_parent_gobj->user_data)->kind;
            else if(nativeCallbackSource_Snapshot(spawnItem->x0_parent_gobj,NativeCallbackSource_Item,&generation))
                source_author=((Item*)spawnItem->x0_parent_gobj->user_data)->native_article_author_kind;
            else return false;
        } else if(selection!=NativeItemBehavior_Author)return false;
        const NativeScriptItemProgram* program=nativeScriptItem_Program(source_author,source_owner);
        if(!program || program->kind!=item_data->kind || !program->article)return false;
        // Empty data table keeps common capability queries safe. No callback or
        // state-table pointer can select authored native behavior for this item.
        static ItemLogicTable source_logic;
        item_data->native_article_author_kind=source_author;
        item_data->native_script_owner=program->owner;
        item_data->xC4_article_data=(Article*)program->article;
        item_data->xB8_itemLogicTable=&source_logic;item_data->xBC_itemStateContainer=NULL;
        item_data->msid=-1;
        if(program->recipe) {
            memset(&item_data->xDD4_itemVar.native_holdable,0,sizeof(item_data->xDD4_itemVar.native_holdable));
            item_data->xDD4_itemVar.native_holdable.owner_kind=source_author;
            item_data->xDD4_itemVar.native_holdable.definition_index=(u16)program->recipe_index;
        } else memset(&item_data->xDD4_itemVar.native_article,0,sizeof(item_data->xDD4_itemVar.native_article));
        return nativeScriptItem_Attach(gobj,program);
    }
    // Explicit authors survive damage-owner transfers such as reflection.
    if (selection == NativeItemBehavior_Parent && spawnItem->x0_parent_gobj != NULL &&
        ftLib_80086960(spawnItem->x0_parent_gobj)) {
        const Fighter* owner =
            HSD_GObjGetUserData(spawnItem->x0_parent_gobj);
        Article* owned;
        if (nativeFighterRuntime_ResolveArticleForItem(owner,
                                                        item_data->kind,
                                                        &owned)) {
            HSD_ASSERTREPORT(578, owned != NULL,
                             "custom fighter article is missing\n");
            item_data->xC4_article_data = owned;
        }
        const NativeFighterArticleLogic* authored = nativeFighterRuntime_ArticleLogic(
            owner, item_data->kind);
        if (authored != NULL) {
            item_data->native_article_author_kind = owner->kind;
            item_data->xB8_itemLogicTable = authored->logic;
            item_data->xBC_itemStateContainer = authored->logic->states;
            if (authored->standard_render) gobj->render_cb = it_8026EECC;
            item_data->xDD4_itemVar.native_article.attachment_owner = NULL;
            item_data->xDD4_itemVar.native_article.creator_spawn_id = 0;
            item_data->xDD4_itemVar.native_article.attachment_facing = 0;
            item_data->xDD4_itemVar.native_article.launch_angle = 0;
            item_data->xDD4_itemVar.native_article.launch_speed = 0;
            item_data->xDD4_itemVar.native_article.launch_origin = (Vec3) { 0 };
            item_data->xDD4_itemVar.native_article.surface_normal = (Vec3) { 0 };
            memset(item_data->xDD4_itemVar.native_article.state, 0,
                   sizeof(item_data->xDD4_itemVar.native_article.state));
        }
    }
    if (selection == NativeItemBehavior_Author) {
        const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(author);
        Article* owned = nativeFighterRuntime_ArticleAssets(author, item_data->kind);
        HSD_ASSERTREPORT(578, definition != NULL && owned != NULL,
                         "authored item has no package assets\n");
        item_data->xC4_article_data = owned;
        for (unsigned i = 0; i < definition->article_logic_count; ++i) {
            const NativeFighterArticleLogic* logic = &definition->article_logics[i];
            if (logic->kind != item_data->kind) continue;
            item_data->native_article_author_kind = author;
            item_data->xB8_itemLogicTable = logic->logic;
            item_data->xBC_itemStateContainer = logic->logic->states;
            if (logic->standard_render) gobj->render_cb = it_8026EECC;
            memset(&item_data->xDD4_itemVar.native_article, 0,
                   sizeof(item_data->xDD4_itemVar.native_article));
            break;
        }
    }
    return true;
}
