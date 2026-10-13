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

static void animation_import(void);
void test_native_article_animation_array_import(void);

void test_native_article_model_roots_import(void)
{
    // Attribute references occupy console offsets36/40. Two names share an
    // immutable joint graph; its child proves graph import rather than a cast.
    u8 bytes[280] = { 0 }, original[sizeof(bytes)], arena[8192];
    u8* data = bytes + 32;
    static const u32 relocations[] = { 0, 68, 72, 104 };
    put32(bytes, sizeof(bytes)); put32(bytes + 4, 232);
    put32(bytes + 8, 4);
    for (unsigned i = 0; i < 4; ++i)
        put32(bytes + 264 + i * 4, relocations[i]);
    put32(data, 32); put32(data + 68, 96); put32(data + 72, 96);
    put32(data + 104, 160);
    for (unsigned i = 0; i < 3; ++i) {
        put32(data + 96 + 32 + i * 4, 0x3F800000);
        put32(data + 160 + 32 + i * 4, 0x3F800000);
    }
    put32(data + 160 + 44, 0x40000000);
    memcpy(original, bytes, sizeof(bytes));
    HSD_Dat dat;
    HSD_DatImport import;
    NativeFighterArticleModelRoot roots[] = { { 12345, 36 }, { 54321, 40 } };
    assert(HSD_DatOpen(&dat, bytes, sizeof(bytes)) == HSD_DAT_OK);
    assert(HSD_DatImportInit(&import, &dat, arena, sizeof(arena), 32) == HSD_DAT_OK);
    Article article = { .native_special_attribute_source = 32 };
    // Existing character allocations must survive extension of the same arena.
    u32* marker = HSD_DatImportAlloc(&import, sizeof(*marker));
    *marker = 0xC0FFEE;
    assert(itArticle_ImportModelRoots(&import, &article, roots, 2));
    assert(*marker == 0xC0FFEE && article.native_model_root_count == 2);
    HSD_Joint* root = article.native_model_roots[0].root;
    assert(root == article.native_model_roots[1].root && root->child);
    assert(root->child->position.x == 2 && root->scale.y == 1);
    assert((u8*)root >= arena && (u8*)root < arena + sizeof(arena));
    assert(memcmp(bytes, original, sizeof(bytes)) == 0);
    unsigned records = import.record_count;
    assert(itArticle_ImportModelRoots(&import, &article, roots, 2));
    assert(import.record_count == records); // Reuses imported graph identities.
    for (unsigned failure = 0; failure < 5; ++failure) {
        assert(HSD_DatImportInit(&import, &dat, arena, sizeof(arena), 32) == HSD_DAT_OK);
        article = (Article){ .native_special_attribute_source = 32 };
        NativeFighterArticleModelRoot invalid[] = { roots[0], roots[1] };
        unsigned count = 2;
        if (failure == 0) invalid[1].name_hash = invalid[0].name_hash;
        if (failure == 1) invalid[1].attribute_offset = 37;
        if (failure == 2) invalid[1].attribute_offset = 64;
        if (failure == 3) invalid[1].name_hash = 0;
        if (failure == 4) count = 9;
        assert(!itArticle_ImportModelRoots(&import, &article, invalid, count));
        assert(article.native_model_root_count == 0 && import.status != HSD_DAT_OK);
    }
    // A declared null root remains named and safely unloadable at runtime.
    put32(data + 72, 0);
    // A null field has no relocation record; relocation-to-offset0 is a
    // different, valid DAT graph reference and must not be treated as null.
    put32(bytes + 8, 3); put32(bytes + 272, 104);
    assert(HSD_DatOpen(&dat, bytes, sizeof(bytes)) == HSD_DAT_OK);
    assert(HSD_DatImportInit(&import, &dat, arena, sizeof(arena), 32) == HSD_DAT_OK);
    article = (Article){ .native_special_attribute_source = 32 };
    assert(itArticle_ImportModelRoots(&import, &article, roots, 2));
    assert(article.native_model_root_count == 2 && article.native_model_roots[1].root == NULL);
    put32(data + 72, 96);
    assert(HSD_DatImportInit(&import, &dat, arena, sizeof(arena), 32) == HSD_DAT_OK);
    article = (Article){ .native_special_attribute_source = 32 };
    assert(!itArticle_ImportModelRoots(&import, &article, roots, 2));
    assert(import.status == HSD_DAT_RELOCATION && article.native_model_root_count == 0);
    animation_import();
    test_native_article_animation_array_import();
    puts("Article named roots import into owned arena with relocation/bounds validation");
}


static void animation_import(void)
{
    u8 bytes[392] = { 0 }, original[sizeof(bytes)], arena[8192];
    u8* data = bytes + 32;
    static const u32 slots[] = { 0, 48, 52, 56, 60, 64, 68, 96, 128, 160 };
    put32(bytes, sizeof(bytes)); put32(bytes + 4, 320); put32(bytes + 8, 10);
    for (unsigned i = 0; i < 10; ++i) put32(bytes + 352 + 4 * i, slots[i]);
    put32(data, 32);
    for (unsigned i = 0; i < 3; ++i) {
        put32(data + 48 + 4 * i, 96 + 32 * i);
        put32(data + 60 + 4 * i, 96 + 32 * i);
        put32(data + 96 + 32 * i, 192 + 32 * i);
    }
    put32(data + 112, 0x123); put32(data + 208, 0x456);
    memcpy(original, bytes, sizeof(bytes));
    HSD_Dat dat; HSD_DatImport import;
    NativeFighterArticleModelAnimation animations[] = {
        { 123, { {16, UINT32_MAX}, {20, UINT32_MAX}, {24, UINT32_MAX} } },
        { 456, { {28, UINT32_MAX}, {32, UINT32_MAX}, {36, UINT32_MAX} } }
    };
    assert(HSD_DatOpen(&dat, bytes, sizeof(bytes)) == HSD_DAT_OK);
    assert(HSD_DatImportInit(&import, &dat, arena, sizeof(arena), 32) == HSD_DAT_OK);
    Article article = { .native_special_attribute_source = 32 };
    assert(itArticle_ImportModelAnimations(&import, &article, animations, 2));
    assert(article.native_model_animation_count == 2);
    assert(article.native_model_animations[0].anim == article.native_model_animations[1].anim);
    assert(article.native_model_animations[0].anim->child->flags == 0x456);
    assert(article.native_model_animations[0].anim->flags == 0x123);
    assert(article.native_model_animations[0].matanim->child);
    assert(article.native_model_animations[0].shapeanim->child);
    assert((u8*)article.native_model_animations[0].anim >= arena &&
           (u8*)article.native_model_animations[0].anim < arena + sizeof(arena));
    assert(memcmp(original, bytes, sizeof(bytes)) == 0);
    unsigned records = import.record_count;
    assert(itArticle_ImportModelAnimations(&import, &article, animations, 2));
    assert(import.record_count == records);
    for (unsigned failure = 0; failure < 6; ++failure) {
        assert(HSD_DatImportInit(&import, &dat, arena, sizeof(arena), 32) == HSD_DAT_OK);
        article = (Article){ .native_special_attribute_source = 32 };
        NativeFighterArticleModelAnimation invalid[] = { animations[0], animations[1] };
        unsigned count = 2;
        if (failure == 0) invalid[1].name_hash = invalid[0].name_hash;
        if (failure == 1) invalid[1].components[1].attribute_offset = 17;
        if (failure == 2) invalid[1].components[2].attribute_offset = 64; // Pointer word exceeds extent.
        if (failure == 3) invalid[1].name_hash = 0;
        if (failure == 4) count = 9;
        if (failure == 5) invalid[1].components[0].attribute_offset = UINT32_MAX - 3;
        assert(!itArticle_ImportModelAnimations(&import, &article, invalid, count));
        assert(article.native_model_animation_count == 0 && import.status != HSD_DAT_OK);
    }
    // Three explicit null descriptors are a valid named bundle; unrelocated
    // nonzero pointer words fail atomically without publishing any bundle.
    put32(data + 60, 0); put32(data + 64, 0); put32(data + 68, 0);
    static const u32 null_slots[] = { 0, 48, 52, 56, 96, 128, 160 };
    put32(bytes + 8, 7);
    for (unsigned i = 0; i < 7; ++i) put32(bytes + 352 + 4 * i, null_slots[i]);
    assert(HSD_DatOpen(&dat, bytes, sizeof(bytes)) == HSD_DAT_OK);
    assert(HSD_DatImportInit(&import, &dat, arena, sizeof(arena), 32) == HSD_DAT_OK);
    article = (Article){ .native_special_attribute_source = 32 };
    assert(itArticle_ImportModelAnimations(&import, &article, animations, 2));
    assert(article.native_model_animation_count == 2 && !article.native_model_animations[1].anim &&
           !article.native_model_animations[1].matanim && !article.native_model_animations[1].shapeanim);
    put32(data + 68, 160);
    assert(HSD_DatImportInit(&import, &dat, arena, sizeof(arena), 32) == HSD_DAT_OK);
    article = (Article){ .native_special_attribute_source = 32 };
    assert(!itArticle_ImportModelAnimations(&import, &article, animations, 2));
    assert(import.status == HSD_DAT_RELOCATION && article.native_model_animation_count == 0);
    // Independent channels may be anywhere in the attribute extent. The
    // final pointer word needs only four bytes, not a guessed twelve-byte bundle.
    put32(data + 92, 128);
    static const u32 component_slots[] = { 0, 48, 52, 56, 92, 96, 128, 160 };
    put32(bytes + 8, 8);
    for (unsigned i = 0; i < 8; ++i) put32(bytes + 352 + 4 * i, component_slots[i]);
    assert(HSD_DatOpen(&dat, bytes, sizeof(bytes)) == HSD_DAT_OK);
    NativeFighterArticleModelAnimation components[] = {
        { 789, { {UINT32_MAX, UINT32_MAX}, {60, UINT32_MAX}, {UINT32_MAX, UINT32_MAX} } },
        { 987, { {16, UINT32_MAX}, {20, UINT32_MAX}, {24, UINT32_MAX} } },
        { 654, { {UINT32_MAX, UINT32_MAX}, {UINT32_MAX, UINT32_MAX}, {UINT32_MAX, UINT32_MAX} } }
    };
    assert(HSD_DatImportInit(&import, &dat, arena, sizeof(arena), 32) == HSD_DAT_OK);
    article = (Article){ .native_special_attribute_source = 32 };
    assert(itArticle_ImportModelAnimations(&import, &article, components, 3));
    assert(!article.native_model_animations[0].anim &&
           article.native_model_animations[0].matanim->child &&
           !article.native_model_animations[0].shapeanim);
    assert(article.native_model_animations[0].matanim == article.native_model_animations[1].matanim);
    assert(!article.native_model_animations[2].anim && !article.native_model_animations[2].matanim &&
           !article.native_model_animations[2].shapeanim);
    // The relocation at source+60 is the final word before the next graph
    // at96; offset64 would cross that attribute boundary.
    components[0].components[1].attribute_offset = 64;
    assert(HSD_DatImportInit(&import, &dat, arena, sizeof(arena), 32) == HSD_DAT_OK);
    article = (Article){ .native_special_attribute_source = 32 };
    assert(!itArticle_ImportModelAnimations(&import, &article, components, 3));
    assert(!article.native_model_animation_count && import.status == HSD_DAT_DESCRIPTOR);
    puts("Article animation bundles import owned shared typed graphs with bounds and atomic publication");
}
