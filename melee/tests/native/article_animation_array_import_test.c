#include <melee/it/itarticle_data.h>
#include <melee/ft/native_fighter_runtime.h>
#include <sysdolphin/baselib/dat_import_internal.h>
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void put32(u8* p, u32 value)
{
    p[0] = (u8)(value >> 24); p[1] = (u8)(value >> 16);
    p[2] = (u8)(value >> 8); p[3] = (u8)value;
}

static size_t fixture(u8 bytes[1024], u32 external_slot, u32 invalid_slot)
{
    static const u32 slots[] = { 0, 48, 52, 56, 60, 64, 68, 96, 100, 112, 116, 128, 132, 160, 192, 224 };
    memset(bytes, 0, 1024);
    u8* data = bytes + 32;
    put32(data, 32);
    put32(data + 48, 96); put32(data + 52, 112); put32(data + 56, 128);
    for (unsigned i = 0; i < 3; ++i) {
        put32(data + 60 + i * 4, 160 + i * 32);
        put32(data + 96 + i * 16, 160 + i * 32);
        put32(data + 100 + i * 16, 160 + i * 32);
        put32(data + 160 + i * 32, 256 + i * 32);
    }
    put32(data + 176, 0x123); put32(data + 272, 0x456);
    unsigned count = 0;
    for (unsigned i = 0; i < sizeof(slots) / sizeof(*slots); ++i) {
        if (slots[i] == external_slot || slots[i] == invalid_slot) continue;
        put32(bytes + 32 + 384 + 4 * count++, slots[i]);
    }
    size_t end = 32 + 384 + 4 * count;
    if (external_slot != UINT32_MAX) {
        put32(data + external_slot, UINT32_MAX);
        put32(bytes + end, external_slot); put32(bytes + end + 4, 0);
        end += 8;
        memcpy(bytes + end, "ExternalAnimation", sizeof("ExternalAnimation"));
        end += sizeof("ExternalAnimation");
        put32(bytes + 16, 1);
    }
    put32(bytes, (u32)end); put32(bytes + 4, 384); put32(bytes + 8, count);
    return end;
}

void test_native_article_animation_array_import(void)
{
    u8 bytes[1024], original[1024], arena[8192];
    size_t size = fixture(bytes, UINT32_MAX, UINT32_MAX);
    memcpy(original, bytes, sizeof(bytes));
    const NativeFighterArticleModelAnimation recipes[] = {
        { 1, { {16, 0}, {20, 1}, {24, 0} } },
        { 2, { {28, UINT32_MAX}, {32, UINT32_MAX}, {36, UINT32_MAX} } }
    };
    HSD_Dat dat; HSD_DatImport import;
    assert(HSD_DatOpen(&dat, bytes, size) == HSD_DAT_OK);
    assert(HSD_DatImportInit(&import, &dat, arena, sizeof(arena), 32) == HSD_DAT_OK);
    Article article = { .native_special_attribute_source = 32 };
    assert(itArticle_ImportModelAnimations(&import, &article, recipes, 2));
    assert(article.native_model_animation_count == 2);
    assert(article.native_model_animations[0].anim == article.native_model_animations[1].anim);
    assert(article.native_model_animations[0].matanim == article.native_model_animations[1].matanim);
    assert(article.native_model_animations[0].shapeanim == article.native_model_animations[1].shapeanim);
    assert(article.native_model_animations[0].anim->flags == 0x123);
    assert(article.native_model_animations[0].anim->child->flags == 0x456);
    assert(article.native_model_animations[0].matanim->child && article.native_model_animations[0].shapeanim->child);
    unsigned records = import.record_count;
    assert(itArticle_ImportModelAnimations(&import, &article, recipes, 2));
    assert(import.record_count == records && memcmp(bytes, original, sizeof(bytes)) == 0);

    for (unsigned scenario = 0; scenario < 10; ++scenario) {
        u32 external = scenario == 5 ? 52 : scenario == 6 ? 116 : UINT32_MAX;
        u32 invalid = scenario == 7 ? 52 : scenario == 8 ? 116 : UINT32_MAX;
        size = fixture(bytes, external, invalid);
        NativeFighterArticleModelAnimation selected = recipes[0];
        if (scenario == 0) selected.components[1].array_index = 4; // Next array is a distinct target.
        if (scenario == 1) selected.components[1].array_index = 1073741823;
        if (scenario == 2) selected.components[1].attribute_offset = UINT32_MAX; // Noncanonical omission.
        if (scenario == 3) selected.components[1].attribute_offset = 21;
        if (scenario == 4) selected.components[1].attribute_offset = 64; // Attribute block ends at96.
        assert(HSD_DatOpen(&dat, bytes, size) == HSD_DAT_OK);
        assert(HSD_DatImportInit(&import, &dat, arena, sizeof(arena), 32) == HSD_DAT_OK);
        if (scenario == 9) import.arena_size = import.arena_used; // Graph allocation failure.
        article = (Article){ .native_special_attribute_source = 32,
            .native_model_animation_count = 1 };
        article.native_model_animations[0].name_hash = 0xCAFE;
        article.native_model_animations[0].anim = (void*)(uintptr_t)0x100;
        assert(!itArticle_ImportModelAnimations(&import, &article, &selected, 1));
        assert(article.native_model_animation_count == 1 && article.native_model_animations[0].name_hash == 0xCAFE &&
               article.native_model_animations[0].anim == (void*)(uintptr_t)0x100);
        assert(import.status == (scenario == 5 || scenario == 6 ? HSD_DAT_EXTERNAL :
            scenario == 7 || scenario == 8 ? HSD_DAT_RELOCATION :
            scenario == 9 ? HSD_DAT_CAPACITY : HSD_DAT_DESCRIPTOR));
        if (scenario == 5 || scenario == 7) assert(import.error_offset == 52);
        if (scenario == 6 || scenario == 8) assert(import.error_offset == 116);
    }
    // Explicit null element and null pointer array stay null; neither is an
    // unresolved external reference. Preserve local references at offset zero.
    for (unsigned slot = 52; slot <= 116; slot += 64) {
        size = fixture(bytes, UINT32_MAX, slot);
        put32(bytes + 32 + slot, 0);
        assert(HSD_DatOpen(&dat, bytes, size) == HSD_DAT_OK);
        assert(HSD_DatImportInit(&import, &dat, arena, sizeof(arena), 32) == HSD_DAT_OK);
        article = (Article){ .native_special_attribute_source = 32 };
        assert(itArticle_ImportModelAnimations(&import, &article, recipes, 1));
        assert(article.native_model_animation_count == 1 && article.native_model_animations[0].matanim == NULL);
    }
    size = fixture(bytes, UINT32_MAX, UINT32_MAX);
    put32(bytes + 32 + 96, 0); // Still relocated: references an actual graph at data offset0.
    assert(HSD_DatOpen(&dat, bytes, size) == HSD_DAT_OK);
    assert(HSD_DatImportInit(&import, &dat, arena, sizeof(arena), 32) == HSD_DAT_OK);
    article = (Article){ .native_special_attribute_source = 32 };
    assert(itArticle_ImportModelAnimations(&import, &article, recipes, 1));
    assert(article.native_model_animations[0].anim && article.native_model_animations[0].anim->child);
    puts("Direct and indexed article animation graphs share owned identities; bounds, externals and failures never publish partial tables");
}
