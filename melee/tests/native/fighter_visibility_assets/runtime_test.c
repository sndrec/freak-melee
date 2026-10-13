// Actual extracted engine/import bodies; private slot binding is controlled.
#include <melee/ft/ftcharacter_assets.h>
#include <melee/ft/ftcostume_assets.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftdat.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_cpu.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/types.h>
#include <melee/ef/native_model_effect.h>
#include <melee/it/native_article_resource.h>
#include <melee/it/native_holdable.h>
#include <sysdolphin/baselib/dat_import_internal.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)
static HSD_GObj* heads[32];
HSD_GObj** HSD_GObjPLinkHead = heads;
static unsigned retirement_steps;
void nativeArticleResource_CancelDefinition(const NativeFighterDefinition* d)
{ CHECK(d != NULL); CHECK(retirement_steps++ == 0); }
void nativeHoldableVisual_Clear(int k)
{ CHECK(k == Ft_Kind_CustomFirst); CHECK(retirement_steps++ == 1); }
void nativeFighterModule_Close(NativeFighterModule* module)
{ CHECK(module != NULL); CHECK(retirement_steps++ == 2); }
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* source, uint32_t id)
{ (void)source; (void)id; CHECK(false); return NULL; }
bool nativeModelEffect_Snapshot(HSD_GObj* source, uint32_t* id)
{ (void)source; (void)id; CHECK(false); return false; }

#include "visibility_actual.inc"

typedef struct VisibilityFixture {
    unsigned char dat_bytes[344];
    unsigned char arena[8192];
    HSD_Dat dat;
    HSD_DatImport import;
    FtCharacterAssets assets;
    HSD_Joint* costume;
    FtCharacterAssetLayout layout;
} VisibilityFixture;

static void put32(unsigned char* at, uint32_t value)
{ at[0] = value >> 24; at[1] = value >> 16; at[2] = value >> 8; at[3] = value; }

static void seed_dat(VisibilityFixture* f)
{
    memset(f, 0, sizeof(*f));
    // Real relocated DAT: slot15 -> two groups, two variants (including an
    // empty selection), duplicate DObj ordinal1, two actual DObj descriptors.
    unsigned char* data = f->dat_bytes + 32;
    const uint32_t relocations[] = {60,68,84,144,228};
    put32(f->dat_bytes, sizeof(f->dat_bytes));
    put32(f->dat_bytes + 4, 292);
    put32(f->dat_bytes + 8, 5);
    for (unsigned i = 0; i < 5; ++i)
        put32(f->dat_bytes + 324 + 4*i, relocations[i]);
    put32(data + 60, 64);
    put32(data + 64, 2); put32(data + 68, 80);
    put32(data + 80, 3); put32(data + 84, 104);
    data[104] = 0; data[105] = 1; data[106] = 1;
    // Native graph importer resolves DAT_JOINT128 with draw descriptors at
    // 224/240; scale1 is ordinary descriptor data, not a fake pointer graph.
    put32(data + 144, 224);
    put32(data + 156, 0x3f800000);
    put32(data + 160, 0x3f800000);
    put32(data + 164, 0x3f800000);
    put32(data + 228, 240);
    f->layout = (FtCharacterAssetLayout){ .visibility_mask = 0x8000 };
    f->assets.model_data.x0.model_num = 2;
}

static bool decode(VisibilityFixture* f)
{
    CHECK(HSD_DatOpen(&f->dat, f->dat_bytes, sizeof(f->dat_bytes)) == HSD_DAT_OK);
    CHECK(HSD_DatImportInit(&f->import, &f->dat, f->arena, sizeof(f->arena), 64) == HSD_DAT_OK);
    f->costume = HSD_DatImportGraph(&f->import, 128, DAT_JOINT);
    CHECK(f->costume != NULL);
    return import_fighter_visibility(&f->import, &f->layout, 0, &f->assets);
}

static void import_cases(VisibilityFixture* f)
{
    seed_dat(f); CHECK(decode(f));
    CHECK(f->assets.visibility_assets[15] != NULL);
    CHECK(f->assets.fighter_items[15] == f->assets.visibility_assets[15]);
    CHECK(f->assets.visibility_assets[0] == NULL);
    CHECK(f->assets.visibility_assets[15][0].x0 == 2);
    CHECK(f->assets.visibility_assets[15][1].x0 == 0);
    CHECK(ftCharacterAssets_ValidateVisibility(&f->assets, 0x8000, f->costume));
    CHECK(!ftCharacterAssets_ValidateVisibility(&f->assets, 1, f->costume));
    CHECK(!ftCharacterAssets_ValidateVisibility(&f->assets, 0x8000, NULL));
    CHECK(!ftCharacterAssets_ValidateVisibility(NULL, 0x8000, f->costume));
    FtPartsVisLookup* lookup = f->assets.visibility_assets[15];
    lookup[0].x4[0].x4[1] = 2;
    CHECK(!ftCharacterAssets_ValidateVisibility(&f->assets, 0x8000, f->costume));
    lookup[0].x4[0].x4[1] = 1;
    lookup[0].x4[0].x0 = 257;
    CHECK(!ftCharacterAssets_ValidateVisibility(&f->assets, 0x8000, f->costume));
    lookup[0].x4[0].x0 = 3;
    lookup[0].x0 = -1;
    CHECK(!ftCharacterAssets_ValidateVisibility(&f->assets, 0x8000, f->costume));
    seed_dat(f); put32(f->dat_bytes + 32 + 64, 257); CHECK(!decode(f));
    seed_dat(f); put32(f->dat_bytes + 32 + 80, 257); CHECK(!decode(f));
    seed_dat(f); put32(f->dat_bytes + 32 + 84, 290); CHECK(!decode(f));
    seed_dat(f); put32(f->dat_bytes + 32 + 60, 290); CHECK(!decode(f));
    seed_dat(f); f->assets.model_data.x0.model_num = 0; CHECK(!decode(f));
    seed_dat(f); CHECK(decode(f));
    CHECK(ftCharacterAssetLayout_IsValid(&f->layout));
    for (unsigned slot = 0; slot < 16; ++slot) {
        FtCharacterAssetLayout l = { .visibility_mask = (uint16_t)(1u << slot) };
        CHECK(ftCharacterAssetLayout_IsValid(&l));
        l.article_mask = l.visibility_mask; CHECK(!ftCharacterAssetLayout_IsValid(&l));
        l.article_mask = 0; l.joint_graph_mask = l.visibility_mask;
        CHECK(!ftCharacterAssetLayout_IsValid(&l));
    }
    FtCharacterAssetLayout l = { .visibility_mask = 2, .item_format = FtCharacterItems_ModelParts };
    CHECK(!ftCharacterAssetLayout_IsValid(&l));
    l = (FtCharacterAssetLayout){ .visibility_mask = 0x400, .article_mask = 0x3ff,
                                .item_format = FtCharacterItems_PaperArticles };
    CHECK(ftCharacterAssetLayout_IsValid(&l));
    l.joint_graph_mask = 0x400; CHECK(!ftCharacterAssetLayout_IsValid(&l));
}

static void runtime_cases(VisibilityFixture* f)
{
    static Fighter fighter, child;
    static HSD_GObj source, nested;
    static HSD_DObj draws[2];
    static HSD_DObj* draw_list[2];
    static NativeFighterDefinition definition, replacement;
    static FtPartsVisLookup previous;
    memset(slots, 0, sizeof(slot_banks[0]));
    NativeFighterSlot* slot = &slots[0];
    slot->active = true; slot->fighter_kind = Ft_Kind_CustomFirst;
    definition.asset_layout = f->layout; slot->definition = &definition;
    slot->character = f->assets;
    fighter = (Fighter){ .kind = slot->fighter_kind, .x8_spawnNum = (s32)0x80000001u,
                         .ft_data = &slot->character.data };
    fighter.x5AC.model_num = 2; fighter.x5AC.xC[4] = &previous;
    fighter.x5AC.cleared[4] = true;
    fighter.dobj_list.count = 2; fighter.dobj_list.data = draw_list;
    draw_list[0] = &draws[0]; draw_list[1] = &draws[1];
    source = (HSD_GObj){ .user_data = &fighter };
    nested = (HSD_GObj){ .user_data = &child, .next = &source };
    child = fighter; heads[HSD_GOBJ_PLINK_FIGHTER] = &nested;
    CHECK(!nativeFighterRuntime_InstallOutlineVisibility(&source, 15));
    CHECK(!nativeFighterRuntime_EnterLoad(NULL));
    CHECK(nativeFighterRuntime_EnterLoad(&source));
    CHECK(!nativeFighterRuntime_InstallOutlineVisibility(&source, -1));
    CHECK(!nativeFighterRuntime_InstallOutlineVisibility(&source, 16));
    CHECK(!nativeFighterRuntime_InstallOutlineVisibility(&source, 0));
    CHECK(fighter.x5AC.xC[4] == &previous && fighter.x5AC.cleared[4]);
    CHECK(nativeFighterRuntime_InstallOutlineVisibility(&source, 15));
    CHECK(fighter.x5AC.xC[4] == f->assets.visibility_assets[15]);
    CHECK(fighter.x5AC.cleared[4]);
    fighter.x5AC.cleared[4] = false;
    CHECK(nativeFighterRuntime_InstallOutlineVisibility(&source, 15));
    CHECK(!fighter.x5AC.cleared[4]);
    CHECK(nativeFighterRuntime_EnterLoad(&nested));
    CHECK(!nativeFighterRuntime_InstallOutlineVisibility(&source, 15));
    nativeFighterRuntime_LeaveLoad(&source); CHECK(load_context_count == 2);
    nativeFighterRuntime_LeaveLoad(&nested); CHECK(load_context_count == 1);
    CHECK(nativeFighterRuntime_InstallOutlineVisibility(&source, 15));
    // Each failed check leaves all existing publication/bookkeeping intact.
    for (unsigned hazard = 0; hazard < 8; ++hazard) {
        fighter.x5AC.xC[4] = &previous; fighter.x5AC.cleared[4] = true;
        if (hazard == 0) fighter.x8_spawnNum++;
        if (hazard == 1) fighter.kind++;
        if (hazard == 2) fighter.ft_data = &f->assets.data;
        if (hazard == 3) slot->definition = &replacement;
        if (hazard == 4) slot->active = false;
        if (hazard == 5) draw_list[1] = NULL;
        if (hazard == 6) fighter.x5AC.model_num = 1;
        if (hazard == 7) definition.asset_layout.visibility_mask = 0;
        CHECK(!nativeFighterRuntime_InstallOutlineVisibility(&source, 15));
        CHECK(fighter.x5AC.xC[4] == &previous && fighter.x5AC.cleared[4]);
        fighter.x8_spawnNum = (s32)0x80000001u; fighter.kind = Ft_Kind_CustomFirst;
        fighter.ft_data = &slot->character.data; slot->definition = &definition;
        slot->active = true; draw_list[1] = &draws[1]; fighter.x5AC.model_num = 2;
        definition.asset_layout = f->layout;
    }
    // Retired source userdata is inaccessible. Registry membership rejects it
    // before dereference, and Leave compares addresses without reading userdata.
    heads[HSD_GOBJ_PLINK_FIGHTER] = &nested; nested.next = NULL;
    void* poison = VirtualAlloc(NULL, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_NOACCESS);
    CHECK(poison != NULL); source.user_data = poison;
    CHECK(!nativeFighterRuntime_InstallOutlineVisibility(&source, 15));
    nativeFighterRuntime_LeaveLoad(&source); CHECK(load_context_count == 0);
    CHECK(VirtualFree(poison, 0, MEM_RELEASE)); source.user_data = &fighter;
    nested.next = &source;
    CHECK(nativeFighterRuntime_EnterLoad(&source));
    release_slot(slot); CHECK(retirement_steps == 3);
    CHECK(!nativeFighterRuntime_InstallOutlineVisibility(&source, 15));
    // Rebinding the same address/Definition cannot revive the borrowed context.
    slot->active = true; slot->fighter_kind = Ft_Kind_CustomFirst;
    slot->definition = &definition; slot->character = f->assets;
    CHECK(!nativeFighterRuntime_InstallOutlineVisibility(&source, 15));
    nativeFighterRuntime_LeaveLoad(&source);
    for (unsigned i = 0; i < NATIVE_CALLBACK_DISPATCH_LIMIT; ++i)
        CHECK(nativeFighterRuntime_EnterLoad(&source));
    CHECK(!nativeFighterRuntime_EnterLoad(&source));
    CHECK(load_context_count == NATIVE_CALLBACK_DISPATCH_LIMIT);
    for (unsigned i = 0; i < NATIVE_CALLBACK_DISPATCH_LIMIT; ++i)
        nativeFighterRuntime_LeaveLoad(&source);
    CHECK(load_context_count == 0);
    for (unsigned i = 0; i < NATIVE_CALLBACK_DISPATCH_LIMIT; ++i)
        CHECK(nativeCallbackDispatch_Enter());
    CHECK(!nativeFighterRuntime_EnterLoad(&source));
    CHECK(load_context_count == 0);
    for (unsigned i = 0; i < NATIVE_CALLBACK_DISPATCH_LIMIT; ++i)
        nativeCallbackDispatch_Leave();
    CHECK(nativeFighterRuntime_EnterLoad(&source));
    nativeFighterRuntime_LeaveLoad(&source);
    CHECK(!nativeFighterRuntime_InstallOutlineVisibility(&source, 15));
    HSD_GObjPLinkHead = NULL;
    CHECK(!nativeFighterRuntime_EnterLoad(&source));
    CHECK(!nativeFighterRuntime_InstallOutlineVisibility(&source, 15));
    HSD_GObjPLinkHead = heads;
}

int main(void)
{
    VisibilityFixture* fixture = malloc(sizeof(*fixture));
    CHECK(fixture != NULL);
    import_cases(fixture); runtime_cases(fixture); free(fixture);
    printf("fighter visibility decoder/load publication: %u checks\n", checks);
    return 0;
}
