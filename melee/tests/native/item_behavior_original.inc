// Actual native selector from commit 1f2df74d6, before explicit policy split.
// Initialization and the ordinary item table are separate fixture boundaries.
static void original_item_select(Item_GObj* gobj, const SpawnItem* spawnItem, FighterKind author)
{
    Item* item_data = gobj->user_data;
    // Explicit authors survive damage-owner transfers such as reflection.
    // Resolve through the live owner only for ordinary item constructors.
    if (author == Ft_Kind_None && spawnItem->x0_parent_gobj != NULL &&
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
    if (author != Ft_Kind_None) {
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
}
