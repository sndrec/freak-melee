#include "native_costume_runtime.h"

#ifdef MELEE_NATIVE

#include "ftcostume_assets.h"
#include "ftdata.h"
#include "native_fighter_runtime.h"
#include "types.h"

#include <melee/gm/gm_1601.h>
#include <melee/gm/native_stock_content.h>
#include <melee/pl/player.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct NativeCostumeSlot {
    NativeContentId id;
    uint8_t* package;
    void* arena;
    FtCostumeAssets assets;
    UnkCostumeStruct costume;
    FighterKind kind;
    uint8_t base_costume;
    uint8_t model_variant;
    bool active;
} NativeCostumeSlot;

static NativeCostumeSlot slots[4];

static bool load_costume(unsigned port, const NativeContentId* id,
                         uint8_t character, uint8_t base_costume,
                         const NativeContentId* fighter_id,
                         FighterKind custom_kind, const char* root,
                         bool require_installed)
{
    if (port >= 4 || id == NULL) {
        return false;
    }
    char default_root[512];
    if (root == NULL) {
        if (!nativeContentStore_DefaultRoot(default_root,
                                            sizeof(default_root))) {
            return false;
        }
        root = default_root;
    }
    if (require_installed && !nativeContentStore_IsInstalled(root, id)) {
        fprintf(stderr, "Selected costume is not installed.\n");
        return false;
    }
    const NativeContentId empty_id = { 0 };
    bool custom = fighter_id != NULL &&
                  memcmp(fighter_id, &empty_id, sizeof(empty_id)) != 0;
    FighterKind kind = custom ? custom_kind
                              : Player_800325C8(character, false);
    if (slots[port].active) {
        return slots[port].base_costume == base_costume &&
               slots[port].kind == kind &&
               memcmp(slots[port].id.bytes, id->bytes,
                      NATIVE_CONTENT_ID_BYTES) == 0;
    }

    uint8_t* package = NULL;
    size_t package_size;
    NativeCostumePackage metadata;
    const uint8_t* dat;
    size_t dat_size;
    if (!nativeContentStore_Read(root, id, &package, &package_size) ||
        !nativeContentStore_ParseCostume(package, package_size, &metadata,
                                         &dat, &dat_size) ||
        metadata.character != character ||
        metadata.base_costume != base_costume ||
        (custom ? memcmp(&metadata.fighter_id, fighter_id,
                         sizeof(*fighter_id)) != 0
                : memcmp(&metadata.fighter_id, &empty_id,
                         sizeof(empty_id)) != 0))
    {
        fprintf(stderr, "Selected costume metadata or fighter dependency does not match.\n");
        free(package);
        return false;
    }
    const char* joint_name;
    const char* matanim_name;
    NativeFighterCostumeInfo custom_info;
    if (custom) {
        if (character != 0 || base_costume != 0 ||
            kind < Ft_Kind_CustomFirst || kind >= Ft_Kind_RuntimeCapacity ||
            !nativeFighterRuntime_CostumeInfoCached(fighter_id, root,
                                                    &custom_info)) {
            fprintf(stderr, "Selected costume's parent fighter could not be validated.\n");
            free(package);
            return false;
        }
        joint_name = custom_info.joint_name;
        matanim_name = custom_info.matanim_name;
    } else {
        uint8_t decoded_character;
        if (!nativeStockContent_CharacterFromId(
                nativeStockContent_CharacterId(character),
                &decoded_character) ||
            decoded_character != character ||
            base_costume >= gm_GetNumCostumesForCKind(character) ||
            kind < 0 || kind >= Ft_Kind_StockCount ||
            base_costume >= CostumeListsForeachCharacter[kind].numCostumes) {
            free(package);
            return false;
        }
        const Fighter_CostumeStrings* names =
            &ftData_803C2360[kind][base_costume];
        joint_name = names->joint_name;
        matanim_name = names->matanim_joint_name;
    }
    void* arena = malloc(FT_COSTUME_IMPORT_ARENA_SIZE);
    if (arena == NULL) {
        free(package);
        return false;
    }
    NativeCostumeSlot* slot = &slots[port];
    if (!ftCostumeAssets_Import(dat, dat_size, joint_name,
                                matanim_name, arena,
                                FT_COSTUME_IMPORT_ARENA_SIZE,
                                FT_COSTUME_IMPORT_RECORD_CAPACITY,
                                &slot->assets) ||
        (custom && slot->assets.joint_count != custom_info.joint_count))
    {
        fprintf(stderr, "Selected costume model failed native import.\n");
        free(arena);
        free(package);
        return false;
    }
    slot->package = package;
    slot->id = *id;
    slot->arena = arena;
    slot->assets.owned_bytes = package;
    slot->assets.arena = arena;
    slot->costume.joint = slot->assets.joint;
    slot->costume.x4 = slot->assets.matanim_joint;
    slot->costume.native_assets = &slot->assets;
    slot->kind = kind;
    slot->base_costume = base_costume;
    slot->model_variant = metadata.model_variant;
    slot->active = true;
    return true;
}

bool nativeCostumeRuntime_LoadInstalled(unsigned port,
                                        const NativeContentId* id,
                                        uint8_t character,
                                        uint8_t base_costume,
                                        const char* root)
{
    return load_costume(port, id, character, base_costume, NULL, Ft_Kind_None,
                        root, true);
}

bool nativeCostumeRuntime_LoadCached(unsigned port,
                                     const NativeContentId* id,
                                     uint8_t character,
                                     uint8_t base_costume,
                                     const char* root)
{
    return load_costume(port, id, character, base_costume, NULL, Ft_Kind_None,
                        root, false);
}

bool nativeCostumeRuntime_LoadCustom(unsigned port,
                                     const NativeContentId* id,
                                     const NativeContentId* fighter_id,
                                     FighterKind fighter_kind,
                                     const char* root,
                                     bool require_installed)
{
    if (fighter_id == NULL) return false;
    return load_costume(port, id, 0, 0, fighter_id, fighter_kind, root,
                        require_installed);
}

const UnkCostumeStruct* nativeCostumeRuntime_ForFighter(const Fighter* fp)
{
    if (fp == NULL || fp->player_id >= 4) {
        return NULL;
    }
    const NativeCostumeSlot* slot = &slots[fp->player_id];
    return slot->active && fp->kind == slot->kind &&
                   fp->x619_costume_id == slot->base_costume
               ? &slot->costume
               : NULL;
}

uint8_t nativeCostumeRuntime_ModelVariant(Fighter* fp)
{
    if (fp == NULL) return 0;
    if (fp->player_id < 4) {
        const NativeCostumeSlot* slot = &slots[fp->player_id];
        if (slot->active && fp->kind == slot->kind &&
            fp->x619_costume_id == slot->base_costume)
            return slot->model_variant;
    }
    return fp->x619_costume_id;
}

void nativeCostumeRuntime_Clear(void)
{
    // Call only after fighters from the previous scene have been destroyed.
    for (unsigned port = 0; port < 4; ++port) {
        free(slots[port].arena);
        free(slots[port].package);
    }
    memset(slots, 0, sizeof(slots));
}

#endif
