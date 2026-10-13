// Called by the real retail archive-import test. Expected identities are read
// from serialized pointer slots and the import registry, independently of the
// accessory normalizer. These checks do not construct/render live JObjs.
static u32 accessory_local(const HSD_Dat* dat, u32 slot)
{
    u32 offset;
    const char* external;
    require(HSD_DatResolve(dat, slot, &offset, &external) == HSD_DAT_REF_LOCAL,
            "accessory retail pointer must resolve locally\n");
    return offset;
}

static void* accessory_expected(const HSD_DatImport* import, u32 slot, u32 type)
{
    u32 offset = accessory_local(import->dat, slot);
    for (u32 i = 0; i < import->record_count; ++i)
        if (import->records[i].offset == offset && import->records[i].type == type)
            return import->records[i].object;
    require(false, "accessory serialized graph must have a typed import record\n");
    return NULL;
}

static void validate_accessory_assets(FtCharacterAssets* assets)
{
    u32 root;
    require(HSD_DatFindRoot(&assets->dat, "ftDataSamus", &root), "Samus root\n");
    u32 table = accessory_local(&assets->dat, root + 72);
    u32 desc = accessory_local(&assets->dat, table + 16);
    u32 variants = accessory_local(&assets->dat, desc + 4);
    FtAccessoryAsset* asset = assets->accessory_assets[4];
    require(asset && assets->fighter_items[4] == asset && asset->variant_count == 4 &&
            ftAccessoryAssets_Validate(&assets->import, asset),
            "typed accessory owns its asset slot, extent and provenance\n");
    require(asset->root == accessory_expected(&assets->import, desc, DAT_JOINT) &&
            asset->base.joint == accessory_expected(&assets->import, desc + 8, DAT_ANIMJOINT) &&
            asset->base.material == accessory_expected(&assets->import, desc + 12, DAT_MATANIM_JOINT) &&
            !asset->base.shape, "accessory base equals serialized recipe\n");
    for (u32 i = 0; i < 4; ++i)
        require(asset->variants[i].joint == accessory_expected(&assets->import, variants + i * 4, DAT_ANIMJOINT) &&
                !asset->variants[i].material && !asset->variants[i].shape,
                "all four throw variants retain exact typed imported graphs\n");
    for (u32 i = 0; i < FT_CHARACTER_ITEM_CAPACITY; ++i)
        require(i == 4 || !assets->accessory_assets[i], "only declared accessory slot is populated\n");

    FtAccessoryAssetSource source = { .root = desc,
        .base = { desc + 8, desc + 12, UINT32_MAX } };
    for (u32 i = 0; i < FT_ACCESSORY_ANIMATION_VARIANTS; ++i)
        source.variants[i] = (FtAccessoryAnimationSource){
            variants + (i % 4) * 4, desc + 12, UINT32_MAX };
    const u32 counts[] = { 0, 1, 4, FT_ACCESSORY_ANIMATION_VARIANTS };
    for (u32 c = 0; c < ARRAY_SIZE(counts); ++c) {
        source.variant_count = counts[c];
        u32 records = assets->import.record_count;
        FtAccessoryAsset* imported = ftAccessoryAssets_Import(&assets->import, &source);
        require(imported && ftAccessoryAssets_Validate(&assets->import, imported) &&
                imported->root == asset->root && imported->variant_count == counts[c] &&
                assets->import.record_count == records,
                "generic accessory import reuses graphs without duplicating their records\n");
        for (u32 i = 0; i < counts[c]; ++i)
            require(imported->variants[i].joint == asset->variants[i % 4].joint &&
                    imported->variants[i].material == asset->base.material &&
                    !imported->variants[i].shape,
                    "generic variants can replace material as well as joint channels\n");
    }

    // Mutate the actual arena-owned recipe, then restore it for subsequent
    // archive tests. Out-of-arena copies are not an accepted ownership token.
    FtAccessoryAsset saved = *asset;
    for (unsigned bad = 0; bad < 7; ++bad) {
        if (bad == 0) asset->variant_count = FT_ACCESSORY_ANIMATION_VARIANTS + 1;
        if (bad == 1) asset->root = NULL;
        if (bad == 2) asset->base.joint = (HSD_AnimJoint*)asset->root;
        if (bad == 3) asset->variants[0].material = (HSD_MatAnimJoint*)asset->root;
        if (bad == 4) asset->variants[4].joint = asset->base.joint;
        if (bad == 5) asset->root = (HSD_Joint*)((u8*)assets->import.arena + assets->import.arena_used);
        if (bad == 6) asset->base.shape = (HSD_ShapeAnimJoint*)asset->base.material;
        require(!ftAccessoryAssets_Validate(&assets->import, asset), "reject malformed accessory provenance\n");
        *asset = saved;
    }
    require(!ftAccessoryAssets_Validate(&assets->import, &saved) &&
            !ftAccessoryAssets_Validate(NULL, asset), "reject foreign accessory ownership\n");

    void* scratch = malloc(FT_CHARACTER_IMPORT_ARENA_SIZE);
    require(scratch != NULL, "accessory negative import arena\n");
    source.variant_count = 4;
    for (unsigned bad = 0; bad < 7; ++bad) {
        HSD_DatImport import;
        require(HSD_DatImportInit(&import, &assets->dat, scratch,
                    FT_CHARACTER_IMPORT_ARENA_SIZE, FT_CHARACTER_IMPORT_RECORD_CAPACITY) == HSD_DAT_OK,
                "fresh accessory import context\n");
        FtAccessoryAssetSource malformed = source;
        if (bad == 0) malformed.variant_count = FT_ACCESSORY_ANIMATION_VARIANTS + 1;
        if (bad == 1) malformed.root = UINT32_MAX;
        if (bad == 2) ++malformed.root;
        if (bad == 3) ++malformed.base.material;
        if (bad == 4) ++malformed.variants[2].joint;
        if (bad == 5) malformed.root = UINT32_MAX - 3;
        if (bad == 6) malformed.variants[3].shape = UINT32_MAX - 3;
        require(!ftAccessoryAssets_Import(&import, &malformed) && import.status != HSD_DAT_OK,
                "invalid accessory recipe never publishes a partial root\n");
    }
    free(scratch);
    require(ftAccessoryAssets_Validate(&assets->import, asset), "retail accessory restored\n");
    puts("Accessory recipes: retail root/base/four variants match serialized graphs; generic channel and rejection checks pass");
}
