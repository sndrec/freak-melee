#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#undef near
#undef far
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/ftcharacter_assets.h>
#include <melee/ft/ftcostume_assets.h>
#include <melee/ft/ftcommon_assets.h>
#include <melee/ft/ftcpu_data.h>
#include <melee/ft/native_fighter_cpu.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ef/native_model_effect.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/types.h>
#include <melee/pl/player.h>
#include <melee/it/native_holdable.h>
#include <melee/it/native_article_resource.h>
#include <melee/gm/native_content_store.h>
#include <sysdolphin/baselib/gobjplink.h>

static unsigned checks, cases;
#define CHECK(e) do { ++checks; if (!(e)) { \
    fprintf(stderr, "%s:%d: case %u: %s\n", __FILE__, __LINE__, cases, #e); exit(1); \
} } while (0)

#include "types.h"

typedef struct Allocation {
    void* pointer;
    NativeFighterFamily* family;
    unsigned tag;
    bool live;
} Allocation;
static Allocation allocations[256];
static unsigned allocated, freed, package_allocated, package_freed;
static unsigned member_calls, bound, published, cancelled, cleared, closed, common_loads;
static int fail_member = -1, invalid_member = -1;
static unsigned failure_phase, invalid_policy;
static bool installed = true, default_available = true, read_available = true;
static bool malformed, common_available = true;
static NativeFighterDefinition definitions[8];
static NativeFighterFamily* reading_family;
static const NativeFighterDefinition* requested[8];
static bool saw_preflight[8];
static FighterKind bound_kinds[8];
static unsigned mapping_count[ChKind_RuntimeCapacity];
static FighterKind mapping_primary[ChKind_RuntimeCapacity], mapping_partner[ChKind_RuntimeCapacity];
static bool mapping_alternate[ChKind_RuntimeCapacity];
static FighterPartsTable* common_parts[Ft_Kind_RuntimeCapacity];
FighterPartsTable** ftPartsTable = common_parts;
static uint8_t* packet[4];
static size_t packet_size[4];
static NativeContentId ids[4];
HSD_GObj** HSD_GObjPLinkHead;
static HSD_GObj* live_heads[32];
static HSD_GObj* player_roles[Gm_Player_NumMax][2];
static CharacterKind player_characters[Gm_Player_NumMax];
static Fighter member_fighters[3];
static HSD_GObj member_objects[3];

static void tracked_free(void* pointer);
static bool prepare_member(NativeFighterSlot*, NativeFighterFamily*, unsigned,
    const NativeFighterDefinition*, FighterKind, const char*, bool);
static void bind_member(NativeFighterSlot*);

// These are the exact production bodies, including release_slot. Only its
// allocator call is routed to a counted fixture allocation boundary.
#define free tracked_free
#include "bodies.h"
#undef free
#include "hash.h"

static void activation(Fighter_GObj* object) { (void)object; }

bool nativeModelEffect_Snapshot(HSD_GObj* object, uint32_t* generation)
{ (void)object; (void)generation; return false; }
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* object, uint32_t generation)
{ (void)object; (void)generation; return NULL; }

HSD_GObj* Player_GetEntityAtIndex(int player, int role)
{
    CHECK((unsigned)player < Gm_Player_NumMax && (unsigned)role < 2);
    return player_roles[player][role];
}

CharacterKind Player_GetPlayerCharacter(int player)
{
    CHECK((unsigned)player < Gm_Player_NumMax);
    return player_characters[player];
}

static void* tracked_allocate(size_t size, NativeFighterFamily* family, unsigned tag)
{
    unsigned index = 0;
    while (index < 256 && allocations[index].live) ++index;
    CHECK(index < 256);
    void* result = malloc(size);
    CHECK(result != NULL);
    allocations[index] = (Allocation){result, family, tag, true};
    ++allocated;
    if (tag == 1) ++package_allocated;
    return result;
}

static void tracked_free(void* pointer)
{
    if (pointer == NULL) return;
    unsigned index = 0;
    while (index < 256 && (!allocations[index].live || allocations[index].pointer != pointer)) ++index;
    CHECK(index < 256); // Includes double-free and foreign allocation rejection.
    Allocation* allocation = &allocations[index];
    if (allocation->tag == 1) {
        // The package owns all spans; no member allocation may outlive it.
        for (unsigned i = 0; i < 256; ++i)
            CHECK(!allocations[i].live || allocations[i].family != allocation->family ||
                  allocations[i].tag == 1);
        ++package_freed;
    }
    allocation->live = false;
    ++freed;
    free(pointer);
}

void nativeFighterModule_Close(NativeFighterModule* module)
{
    if (module->library != NULL) ++closed;
    *module = (NativeFighterModule){0};
}

void nativeArticleResource_CancelDefinition(const NativeFighterDefinition* definition)
{
    CHECK(definition != NULL); ++cancelled;
}

void nativeHoldableVisual_Clear(int fighter_kind)
{
    CHECK(fighter_kind >= Ft_Kind_CustomFirst && fighter_kind < Ft_Kind_RuntimeCapacity);
    ++cleared;
}

void Fighter_LoadCommonData(void)
{
    ++common_loads;
    if (common_available) ftPartsTable = common_parts;
}

bool nativeContentStore_DefaultRoot(char* path, size_t capacity)
{
    if (!default_available) return false;
    CHECK(capacity >= 9); strcpy(path, "fixture"); return true;
}

bool nativeContentStore_IsInstalled(const char* root, const NativeContentId* id)
{
    CHECK(root != NULL && id != NULL); return installed;
}

bool nativeContentStore_Read(const char* root, const NativeContentId* id,
                             uint8_t** package, size_t* size)
{
    CHECK(root != NULL && id != NULL);
    int selected = -1;
    for (unsigned i = 0; i < 4; ++i)
        if (memcmp(id, &ids[i], sizeof(*id)) == 0) selected = (int)i;
    if (!read_available || selected < 0) return false;
    // prepare_family has retained the exact family before calling this sink.
    reading_family = NULL;
    for (unsigned bank = 0; bank < 2; ++bank)
        for (unsigned f = 0; f < 4; ++f)
            if (&family_banks[bank][f].package == package) reading_family = &family_banks[bank][f];
    // Detached validation families live on the caller's stack. Its retained
    // allocation owner is supplied by this field address, without fake Items.
    if (reading_family == NULL)
        reading_family = (NativeFighterFamily*)((char*)package - offsetof(NativeFighterFamily, package));
    CHECK(memcmp(&reading_family->id, id, sizeof(*id)) == 0);
    *size = packet_size[selected];
    *package = tracked_allocate(*size, reading_family, 1);
    memcpy(*package, packet[selected], *size);
    if (malformed) (*package)[7] = 1;
    return true;
}

static bool prepare_member(NativeFighterSlot* slot, NativeFighterFamily* family,
    unsigned member_index, const NativeFighterDefinition* requested_definition,
    FighterKind fighter_kind, const char* root, bool preflight)
{
    CHECK(root != NULL && member_index < family->metadata.member_count);
    CHECK(fighter_kind >= Ft_Kind_CustomFirst && fighter_kind < Ft_Kind_RuntimeCapacity);
    unsigned actor = (unsigned)(fighter_kind - Ft_Kind_CustomFirst);
    unsigned source_family = 4;
    for (unsigned f = 0; f < 4; ++f)
        if (memcmp(&family->id, &ids[f], sizeof(family->id)) == 0) source_family = f;
    CHECK(source_family < 4);
    unsigned source_actor = source_family * 2 + member_index;
    CHECK(actor < 8 && !slot->active);
    CHECK(family->data.members[member_index].fighter_dat >= family->package &&
          family->data.members[member_index].fighter_dat < family->package + family->package_size);
    ++member_calls;
    requested[actor] = requested_definition;
    saw_preflight[actor] = preflight;
    if ((int)actor == fail_member && failure_phase == 0) return false;
    slot->family = family; slot->member_index = member_index;
    slot->fighter_kind = fighter_kind; slot->character_kind = family->character_kind;
    slot->definition = requested_definition != NULL ? requested_definition : &definitions[source_actor];
    slot->module.library = (void*)slot->definition;
    slot->character_arena = tracked_allocate(1, family, 2);
    if ((int)actor == fail_member && failure_phase == 1) {
        release_slot(slot); return false;
    }
    slot->animation_arena = tracked_allocate(1, family, 3);
    slot->costume_arena = tracked_allocate(1, family, 4);
    slot->motion_states = tracked_allocate(sizeof(MotionState), family, 5);
    if ((int)actor == fail_member && failure_phase == 2) {
        release_slot(slot); return false;
    }
    if ((int)actor == invalid_member) {
        if (invalid_policy == 0) slot->module.library = NULL;
        if (invalid_policy == 1) definitions[source_actor].source_kind = Ft_Kind_Mario;
        if (invalid_policy == 2) definitions[source_actor].on_form_activated = NULL;
    }
    return true;
}

static void bind_member(NativeFighterSlot* slot)
{
    CHECK(slot->family != NULL && slot->definition != NULL && !slot->active);
    CHECK(mapping_count[slot->family->character_kind] == 0);
    CHECK(bound < 8); bound_kinds[bound++] = slot->fighter_kind;
    slot->active = true;
}

void Player_RegisterNativeCharacter(CharacterKind character, FighterKind primary,
                                    FighterKind partner, bool alternate)
{
    NativeFighterFamily* family = NULL;
    for (unsigned f = 0; f < 4; ++f)
        if (families[f].character_kind == character && families[f].members != NULL) family = &families[f];
    CHECK(family != NULL && !family->active);
    for (unsigned m = 0; m < family->metadata.member_count; ++m)
        CHECK(family->members[m].active);
    CHECK(primary == family->members[0].fighter_kind);
    CHECK(partner == (family->metadata.member_count == 2 ? family->members[1].fighter_kind : Ft_Kind_None));
    CHECK(alternate == (family->metadata.mode == NativeFighterFamily_AlternateForm));
    ++mapping_count[character]; mapping_primary[character] = primary;
    mapping_partner[character] = partner; mapping_alternate[character] = alternate;
    ++published;
}

static void cleanup(void)
{
    for (unsigned bank = 0; bank < 2; ++bank)
        for (unsigned f = 0; f < 4; ++f) release_family(&family_banks[bank][f]);
    CHECK(allocated == freed && package_allocated == package_freed);
    memset(mapping_count, 0, sizeof(mapping_count));
    staged_count = 0; bound = published = 0;
    load_context_count = 0;
}

static void reset(void)
{
    cleanup(); ++cases;
    memset(allocations, 0, sizeof(allocations));
    allocated = freed = package_allocated = package_freed = 0;
    member_calls = bound = published = cancelled = cleared = closed = common_loads = 0;
    fail_member = invalid_member = -1; failure_phase = invalid_policy = 0;
    installed = default_available = read_available = common_available = true;
    malformed = false; ftPartsTable = common_parts;
    memset(requested, 0, sizeof(requested)); memset(saw_preflight, 0, sizeof(saw_preflight));
    memset(definitions, 0, sizeof(definitions));
    memset(live_heads, 0, sizeof(live_heads)); HSD_GObjPLinkHead = live_heads;
    memset(player_roles, 0, sizeof(player_roles));
    memset(player_characters, 0, sizeof(player_characters));
    memset(member_fighters, 0, sizeof(member_fighters));
    memset(member_objects, 0, sizeof(member_objects));
    for (unsigned i = 0; i < 8; ++i) {
        definitions[i].source_kind = Ft_Kind_None;
        definitions[i].sound_banks = UINT64_C(1) << (i + 1);
        definitions[i].on_form_activated = activation;
    }
}

static void packets(NativeFighterFamilyMode mode)
{
    static const uint8_t portrait[45] = {
        137, 'P', 'N', 'G', 13, 10, 26, 10, 0, 0, 0, 13, 'I', 'H', 'D', 'R',
        0, 0, 0, 64, 0, 0, 0, 56, 8, 6,
        [37] = 'I', [38] = 'E', [39] = 'N', [40] = 'D',
    };
    static const uint8_t bytes[32] = {0x20};
    for (unsigned f = 0; f < 4; ++f) {
        free(packet[f]); packet[f] = NULL;
        NativeFighterPackage metadata = {.mode = mode,
            .member_count = mode == NativeFighterFamily_Single ? 1 : 2};
        snprintf(metadata.title, sizeof(metadata.title), "Family %u", f);
        NativeFighterPackageData data = {.portrait_png = portrait, .portrait_size = 45};
        for (unsigned m = 0; m < metadata.member_count; ++m) {
            strcpy(metadata.members[m].key, m == 0 ? "main" : "partner");
            snprintf(metadata.members[m].code_key, 32, "actor_%u", f * 2 + m);
            data.members[m] = (NativeFighterMemberData){
                .fighter_dat = bytes, .fighter_size = 32,
                .animation_dat = bytes, .animation_size = 32,
                .costume_dat = bytes, .costume_size = 32,
                .module_dll = bytes, .module_size = 32};
        }
        CHECK(nativeContentStore_BuildFighter(&metadata, &data, &packet[f], &packet_size[f], &ids[f]));
    }
}

static void healthy(void)
{
    for (unsigned mode = 0; mode < 3; ++mode) {
        reset(); packets((NativeFighterFamilyMode)mode);
        CHECK(nativeFighterRuntime_StageCached(ids, 4, NULL, true));
        CHECK(staged_count == 4 && published == 0 && bound == 0 && package_allocated == 4);
        for (unsigned f = 0; f < 4; ++f) {
            NativeFighterFamily* family = &staged_families[f];
            CHECK(family->members == &staged_slots[f * 2] && family->package != NULL && !family->active);
            uint64_t expected = definitions[f * 2].sound_banks;
            if (mode != 0) expected |= definitions[f * 2 + 1].sound_banks;
            CHECK(nativeFighterRuntime_CharacterSoundBanks(ChKind_CustomFirst + f) == expected);
            for (unsigned m = 0; m < family->metadata.member_count; ++m)
                CHECK(family->members[m].fighter_kind == Ft_Kind_CustomFirst + f * 2 + m);
        }
        CHECK(!nativeFighterRuntime_ActivateStaged(3) && published == 0);
        CHECK(nativeFighterRuntime_ActivateStaged(4));
        CHECK(published == 4 && bound == (mode == 0 ? 4 : 8) && staged_count == 0);
        for (unsigned f = 0; f < 4; ++f) {
            CHECK(families[f].active && mapping_count[ChKind_CustomFirst + f] == 1);
            uint64_t expected = definitions[f * 2].sound_banks;
            if (mode != 0) expected |= definitions[f * 2 + 1].sound_banks;
            CHECK(nativeFighterRuntime_CharacterSoundBanks(ChKind_CustomFirst + f) == expected);
            CHECK(mapping_primary[ChKind_CustomFirst + f] == Ft_Kind_CustomFirst + f * 2);
            CHECK(mapping_partner[ChKind_CustomFirst + f] ==
                  (mode == 0 ? Ft_Kind_None : Ft_Kind_CustomFirst + f * 2 + 1));
            CHECK(mapping_alternate[ChKind_CustomFirst + f] == (mode == 1));
        }
        CHECK(nativeFighterRuntime_CharacterSoundBanks(ChKind_RuntimeCapacity) == 0);
        cleanup(); CHECK(cancelled == (mode == 0 ? 4 : 8));
        CHECK(cleared == cancelled && closed == cancelled);
    }
}

static void failures(void)
{
    for (unsigned mode = 1; mode < 3; ++mode)
        for (int actor = 0; actor < 8; ++actor)
            for (unsigned phase = 0; phase < 3; ++phase) {
                reset(); packets((NativeFighterFamilyMode)mode);
                fail_member = actor; failure_phase = phase;
                CHECK(!nativeFighterRuntime_StageCached(ids, 4, "fixture", false));
                CHECK(member_calls == (unsigned)actor + 1 && published == 0 && bound == 0 && staged_count == 0);
                CHECK(allocated == freed && package_allocated == package_freed && cancelled == 0);
                for (unsigned f = 0; f < 4; ++f)
                    CHECK(staged_families[f].package == NULL && !families[f].active);
            }
    for (unsigned policy = 0; policy < 3; ++policy)
        for (int actor = 0; actor < 8; ++actor) {
            reset(); packets(NativeFighterFamily_AlternateForm);
            invalid_member = actor; invalid_policy = policy;
            CHECK(!nativeFighterRuntime_StageCached(ids, 4, "fixture", false));
            CHECK(allocated == freed && package_allocated == package_freed && published == 0 && cancelled == 0);
        }
    for (unsigned failure = 0; failure < 5; ++failure) {
        reset(); packets(NativeFighterFamily_AlternateForm);
        if (failure == 0) installed = false;
        if (failure == 1) default_available = false;
        if (failure == 2) read_available = false;
        if (failure == 3) malformed = true;
        if (failure == 4) { ftPartsTable = NULL; common_available = false; }
        CHECK(!nativeFighterRuntime_StageCached(ids, 4, NULL, true));
        CHECK(allocated == freed && package_allocated == package_freed && member_calls == 0 && published == 0);
    }
}

static void outgoing_and_detached(void)
{
    reset(); packets(NativeFighterFamily_AlternateForm);
    CHECK(nativeFighterRuntime_Load(&ids[0], NULL, ChKind_CustomFirst, Ft_Kind_CustomFirst,
                                   "fixture", false, false));
    unsigned outgoing_published = published, outgoing_bound = bound;
    NativeFighterFamily saved = families[0];
    void* primary_arena = saved.members[0].character_arena;
    CHECK(nativeFighterRuntime_ValidateCached(&ids[0], &definitions[0], NULL));
    CHECK(cancelled == 0 && cleared == 0 && families[0].package == saved.package &&
          saved.members[0].character_arena == primary_arena);
    CHECK(requested[0] == &definitions[0] && requested[1] == NULL &&
          saw_preflight[0] && saw_preflight[1]);
    fail_member = 3; failure_phase = 2;
    CHECK(!nativeFighterRuntime_StageCached(ids, 4, "fixture", false));
    CHECK(published == outgoing_published && bound == outgoing_bound && cancelled == 0);
    CHECK(families[0].active && families[0].package == saved.package &&
          families[0].members[0].character_arena == primary_arena);
    fail_member = -1; bound = 0;
    // Staging must not publish mappings or mutate outgoing imported members.
    CHECK(nativeFighterRuntime_StageCached(&ids[1], 1, "fixture", false));
    definitions[2].sound_banks = UINT64_C(0x10000000000);
    definitions[3].sound_banks = UINT64_C(0x40000000000);
    CHECK((saved.members[0].definition->sound_banks | saved.members[1].definition->sound_banks) == 6);
    CHECK(nativeFighterRuntime_CharacterSoundBanks(ChKind_CustomFirst) == UINT64_C(0x50000000000));
    CHECK(!nativeFighterRuntime_ActivateStaged(1) && published == outgoing_published);
    // Release outgoing family, then the prepared family can publish.
    release_family(&families[0]); mapping_count[ChKind_CustomFirst] = 0;
    CHECK(nativeFighterRuntime_ActivateStaged(1));
    CHECK(nativeFighterRuntime_CharacterSoundBanks(ChKind_CustomFirst) == UINT64_C(0x50000000000));
    cleanup(); CHECK(allocated == freed && package_allocated == package_freed);
}

static void bounds_and_retained_load(void)
{
    reset(); packets(NativeFighterFamily_SimultaneousCompanion);
    CHECK(nativeFighterRuntime_StageCached(ids, 1, "fixture", false));
    void* staged_package = staged_families[0].package;
    NativeContentId duplicate[2] = {ids[0], ids[0]};
    CHECK(!nativeFighterRuntime_StageCached(duplicate, 2, "fixture", false));
    CHECK(staged_count == 1 && staged_families[0].package == staged_package);
    CHECK(!nativeFighterRuntime_StageCached(ids, 5, "fixture", false));
    CHECK(!nativeFighterRuntime_StageCached(NULL, 1, "fixture", false));
    CHECK(nativeFighterRuntime_StageCached(NULL, 0, NULL, false));
    CHECK(staged_count == 0 && allocated == freed && package_allocated == package_freed);
    CHECK(nativeFighterRuntime_ActivateStaged(0));
    for (unsigned invalid = 0; invalid < 4; ++invalid) {
        NativeFighterFamily family = {0}; NativeFighterSlot members[2] = {0};
        CharacterKind character = invalid < 2 ?
            (invalid == 0 ? ChKind_CustomFirst - 1 : ChKind_RuntimeCapacity) : ChKind_CustomFirst;
        FighterKind fighter = invalid < 2 ? Ft_Kind_CustomFirst :
            (invalid == 2 ? Ft_Kind_CustomFirst - 1 : Ft_Kind_RuntimeCapacity - 1);
        CHECK(!prepare_family(&family, members, &ids[0], NULL, character, fighter, NULL, false, false));
        CHECK(family.package == NULL);
    }
    CHECK(nativeFighterRuntime_Load(&ids[0], NULL, ChKind_CustomFirst, Ft_Kind_CustomFirst,
                                   "fixture", false, false));
    CHECK(!nativeFighterRuntime_Load(&ids[1], NULL, ChKind_CustomFirst, Ft_Kind_CustomFirst + 2,
                                    "fixture", false, false));
    CHECK(!nativeFighterRuntime_Load(&ids[1], NULL, ChKind_CustomFirst + 1, Ft_Kind_CustomFirst + 1,
                                    "fixture", false, false));
    load_context_count = 1;
    load_contexts[0].slot = &families[0].members[0];
    load_contexts[0].definition = families[0].members[0].definition;
    release_family(&families[0]);
    CHECK(load_contexts[0].slot == NULL && load_contexts[0].definition == NULL);
    CHECK(package_allocated == package_freed && allocated == freed && cancelled == 2 && cleared == 2);
    CHECK(nativeFighterRuntime_CharacterSoundBanks(ChKind_CustomFirst) == 0);
}

static void member_setup(NativeFighterFamilyMode mode)
{
    reset(); packets(mode);
    CHECK(nativeFighterRuntime_StageCached(ids, 4, "fixture", false));
    CHECK(nativeFighterRuntime_ActivateStaged(4));
    NativeFighterFamily* family = &families[0];
    player_characters[1] = family->character_kind;
    for (unsigned m = 0; m < family->metadata.member_count; ++m) {
        member_fighters[m].x8_spawnNum = (int32_t)(m == 0 ? 0x80000001u : 16777217u);
        member_fighters[m].player_id = 1;
        member_fighters[m].kind = family->members[m].fighter_kind;
        member_fighters[m].ft_data = &family->members[m].character.data;
        member_objects[m].user_data = &member_fighters[m];
        member_objects[m].next = m + 1 < family->metadata.member_count ? &member_objects[m + 1] : NULL;
        player_roles[1][m] = &member_objects[m];
    }
    live_heads[HSD_GOBJ_PLINK_FIGHTER] = &member_objects[0];
}

static void member_identity(void)
{
    for (unsigned mode = 0; mode < 3; ++mode) {
        member_setup((NativeFighterFamilyMode)mode);
        unsigned count = families[0].metadata.member_count;
        for (unsigned swap = 0; swap < 2; ++swap) {
            if (swap) {
                HSD_GObj* previous = player_roles[1][0];
                player_roles[1][0] = player_roles[1][1]; player_roles[1][1] = previous;
            }
            for (unsigned source = 0; source < count; ++source) {
                CHECK(nativeFighterRuntime_MemberId(&member_objects[source], 0) == 0x80000001u);
                CHECK(nativeFighterRuntime_MemberId(&member_objects[source], 1) ==
                      (count == 2 ? 16777217u : 0));
                CHECK(nativeFighterRuntime_MemberId(&member_objects[source], 2) == 0);
            }
        }
        // Same live member address with a new generation resolves its current
        // full identity; immutable membership is not a retained entity handle.
        member_fighters[0].x8_spawnNum = (int32_t)UINT32_MAX;
        CHECK(nativeFighterRuntime_MemberId(&member_objects[0], 0) == UINT32_MAX);
        if (count == 2) {
            member_fighters[1].x8_spawnNum = (int32_t)0xFEDCBA98u;
            CHECK(nativeFighterRuntime_MemberId(&member_objects[0], 1) == 0xFEDCBA98u);
        }
    }
    for (unsigned failure = 0; failure < 23; ++failure) {
        member_setup(NativeFighterFamily_AlternateForm);
        HSD_GObj* source = &member_objects[0]; unsigned index = 1;
        switch (failure) {
        case 0: HSD_GObjPLinkHead = NULL; break;
        case 1: live_heads[HSD_GOBJ_PLINK_FIGHTER] = NULL; break;
        case 2: member_objects[0].user_data = NULL; break;
        case 3: member_fighters[0].player_id = Gm_Player_NumMax; break;
        case 4: member_fighters[0].kind = Ft_Kind_Mario; break;
        case 5: member_fighters[0].ft_data = &families[1].members[0].character.data; break;
        case 6: families[0].members[0].active = false; break;
        case 7: families[0].active = false; break;
        case 8: player_characters[1] = (CharacterKind)0; break;
        case 9: player_roles[1][0] = NULL; break;
        case 10: player_roles[1][1] = NULL; break;
        case 11: member_objects[0].next = NULL; break;
        case 12: member_objects[1].user_data = NULL; break;
        case 13: member_fighters[1].player_id = 2; break;
        case 14: member_fighters[1].kind = Ft_Kind_CustomFirst + 2; break;
        case 15: member_fighters[1].ft_data = &families[0].members[0].character.data; break;
        case 16: families[0].members[1].active = false; break;
        case 17: families[0].metadata.member_count = 1; break;
        case 18: index = UINT_MAX; break;
        case 19: member_fighters[1].ft_data = &families[1].members[1].character.data; break;
        case 20:
            // Registry membership and matching kind/data do not make an
            // unrelated same-player entity a member of the actual player roles.
            member_fighters[2] = member_fighters[0]; member_fighters[2].x8_spawnNum = 444;
            member_objects[2].user_data = &member_fighters[2];
            member_objects[2].next = &member_objects[0];
            live_heads[HSD_GOBJ_PLINK_FIGHTER] = &member_objects[2];
            source = &member_objects[2]; break;
        case 21: member_fighters[0].player_id = 0; break;
        case 22: families[0].members[0].family = &families[1]; break;
        }
        CHECK(nativeFighterRuntime_MemberId(source, index) == 0);
    }
    member_setup(NativeFighterFamily_AlternateForm);
    HSD_GObj* retired = VirtualAlloc(NULL, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    CHECK(retired != NULL); DWORD previous;
    CHECK(VirtualProtect(retired, 4096, PAGE_NOACCESS, &previous));
    CHECK(nativeFighterRuntime_MemberId(retired, 0) == 0);
    member_objects[0].next = NULL; player_roles[1][1] = retired;
    CHECK(nativeFighterRuntime_MemberId(&member_objects[0], 1) == 0);
    CHECK(nativeFighterRuntime_MemberId(NULL, 0) == 0);
    CHECK(VirtualFree(retired, 0, MEM_RELEASE));
}

static void cpu_history_identity(void)
{
    for (unsigned mode = 0; mode < 3; ++mode) {
        member_setup((NativeFighterFamilyMode)mode); ++cases;
        unsigned count = families[0].metadata.member_count;
        CHECK(nativeFighterRuntime_CpuHistorySource(&member_objects[0]) == (count == 2 ? 16777217u : 0));
        if (count == 1) continue;
        CHECK(nativeFighterRuntime_CpuHistorySource(&member_objects[1]) == 0x80000001u);
        // The first actor has no published counterpart yet; the constructing
        // second actor gets the primary hint before its own Player publication.
        player_roles[1][0] = NULL; player_roles[1][1] = NULL;
        CHECK(nativeFighterRuntime_CpuHistorySource(&member_objects[0]) == 0);
        player_roles[1][0] = &member_objects[0];
        CHECK(nativeFighterRuntime_CpuHistorySource(&member_objects[1]) == 0x80000001u);
        CHECK(nativeFighterRuntime_MemberId(&member_objects[1], 0) == 0);
        player_roles[1][0] = &member_objects[1]; player_roles[1][1] = &member_objects[0];
        CHECK(nativeFighterRuntime_CpuHistorySource(&member_objects[1]) == 0x80000001u);
        member_fighters[0].x8_spawnNum = -1;
        CHECK(nativeFighterRuntime_CpuHistorySource(&member_objects[1]) == UINT32_MAX);
    }
    for (unsigned bad = 0; bad < 16; ++bad) {
        member_setup(NativeFighterFamily_SimultaneousCompanion); ++cases;
        Fighter_GObj* source = &member_objects[1];
        switch (bad) {
        case 0: source = NULL; break;
        case 1: live_heads[HSD_GOBJ_PLINK_FIGHTER] = NULL; break;
        case 2: member_objects[1].user_data = NULL; break;
        case 3: member_fighters[1].player_id = Gm_Player_NumMax; break;
        case 4: member_fighters[1].ft_data = NULL; break;
        case 5: member_fighters[1].kind = Ft_Kind_Mario; break;
        case 6: player_characters[1] = CKind_Mario; break;
        case 7: families[0].active = false; break;
        case 8: families[0].members[1].active = false; break;
        case 9: families[0].members[1].member_index = 2; break;
        case 10: families[0].members[1].family = &families[1]; break;
        case 11: families[0].members[0].active = false; break;
        case 12: member_fighters[0].ft_data = NULL; break;
        case 13: member_fighters[0].kind = Ft_Kind_Mario; break;
        case 14: member_fighters[0].player_id = 0; break;
        case 15: member_objects[0].user_data = NULL; break;
        }
        CHECK(nativeFighterRuntime_CpuHistorySource(source) == 0);
    }
    member_setup(NativeFighterFamily_SimultaneousCompanion); ++cases;
    HSD_GObj* retired = VirtualAlloc(NULL, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_NOACCESS);
    CHECK(retired != NULL);
    CHECK(nativeFighterRuntime_CpuHistorySource(retired) == 0);
    member_objects[0].next = &member_objects[1];
    live_heads[HSD_GOBJ_PLINK_FIGHTER] = &member_objects[1];
    player_roles[1][0] = retired;
    CHECK(nativeFighterRuntime_CpuHistorySource(&member_objects[1]) == 0);
    CHECK(VirtualFree(retired, 0, MEM_RELEASE));
}

int main(void)
{
    healthy(); failures(); outgoing_and_detached(); bounds_and_retained_load(); member_identity(); cpu_history_identity(); cleanup();
    for (unsigned f = 0; f < 4; ++f) free(packet[f]);
    printf("PASS family orchestration: %u cases, %u checks\n", cases, checks);
    return 0;
}
