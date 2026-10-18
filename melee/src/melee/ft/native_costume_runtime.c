#include "native_costume_runtime.h"

#ifdef MELEE_NATIVE

#include "ftcostume_assets.h"
#include "ftdata.h"
#include "fighter.h"
#include "native_fighter_runtime.h"
#include "types.h"

#include <melee/gm/gm_1601.h>
#include <melee/gm/native_stock_content.h>
#include <melee/pl/player.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct NativeCostumeSlot {
    void* arena;
    FtCostumeAssets assets;
    UnkCostumeStruct costume;
    FighterKind kind;
    uint8_t base_costume;
    uint8_t model_variant;
    bool active;
} NativeCostumeSlot;

typedef struct NativeCostumePlayer {
    NativeContentId id;
    NativeContentId fighter_id;
    uint8_t* package;
    unsigned member_count;
    NativeCostumeSlot members[NATIVE_FIGHTER_FAMILY_MEMBERS];
} NativeCostumePlayer;

static NativeCostumePlayer players[4];

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
    if (players[port].member_count) {
        const NativeCostumeSlot* first = &players[port].members[0];
        return first->base_costume == base_costume && first->kind == kind &&
            !memcmp(&players[port].id, id, sizeof(*id)) &&
            !memcmp(&players[port].fighter_id, custom ? fighter_id : &empty_id,
                    sizeof(empty_id));
    }

    uint8_t* package = NULL;
    size_t package_size;
    NativeCostumePackage metadata;
    NativeCostumePackageData data;
    if (!nativeContentStore_Read(root, id, &package, &package_size) ||
        !nativeContentStore_ParseCostume(package, package_size, &metadata,
                                         &data) ||
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
    NativeFighterCostumeFamilyInfo custom_info;
    if (custom) {
        if (character != 0 || base_costume != 0 ||
            kind < Ft_Kind_CustomFirst || kind >= Ft_Kind_RuntimeCapacity ||
            (kind - Ft_Kind_CustomFirst) % NATIVE_FIGHTER_FAMILY_MEMBERS ||
            !nativeFighterRuntime_CostumeInfoCached(fighter_id, root,
                                                    &custom_info)) {
            fprintf(stderr, "Selected costume's parent fighter could not be validated.\n");
            free(package);
            return false;
        }
        if (metadata.member_count != custom_info.member_count ||
            (unsigned)kind + metadata.member_count > Ft_Kind_RuntimeCapacity) {
            free(package);
            return false;
        }
        for (unsigned m = 0; m < metadata.member_count; ++m) {
            if (memcmp(metadata.member_keys[m], custom_info.members[m].key,
                       NATIVE_FIGHTER_CODE_KEY_BYTES)) {
                free(package);
                return false;
            }
        }
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
    NativeCostumePlayer candidate = {0};
    candidate.id = *id;
    candidate.fighter_id = metadata.fighter_id;
    candidate.package = package;
    candidate.member_count = metadata.member_count;
    bool valid = true;
    for (unsigned m = 0; valid && m < metadata.member_count; ++m) {
        NativeCostumeSlot* slot = &candidate.members[m];
        if (custom) {
            joint_name = custom_info.members[m].joint_name;
            matanim_name = custom_info.members[m].matanim_name;
        }
        slot->arena = malloc(FT_COSTUME_IMPORT_ARENA_SIZE);
        valid = slot->arena && ftCostumeAssets_Import(data.members[m].dat,
            data.members[m].size, joint_name, matanim_name, slot->arena,
            FT_COSTUME_IMPORT_ARENA_SIZE, FT_COSTUME_IMPORT_RECORD_CAPACITY,
            &slot->assets) &&
            (!custom || slot->assets.joint_count == custom_info.members[m].joint_count);
        slot->kind = (FighterKind)(kind + m);
        slot->base_costume = base_costume;
        slot->model_variant = metadata.model_variant;
        slot->active = valid;
    }
    if (!valid) {
        fprintf(stderr, "Selected outfit model failed native import.\n");
        for (unsigned m = 0; m < candidate.member_count; ++m) free(candidate.members[m].arena);
        free(package);
        return false;
    }
    players[port] = candidate;
    for (unsigned m = 0; m < candidate.member_count; ++m) {
        NativeCostumeSlot* slot = &players[port].members[m];
        slot->assets.owned_bytes = package;
        slot->assets.arena = slot->arena;
        slot->costume.joint = slot->assets.joint;
        slot->costume.x4 = slot->assets.matanim_joint;
        slot->costume.native_assets = &slot->assets;
    }
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
    for (unsigned m = 0; m < players[fp->player_id].member_count; ++m) {
        const NativeCostumeSlot* slot = &players[fp->player_id].members[m];
        if (slot->active && fp->kind == slot->kind &&
            fp->x619_costume_id == slot->base_costume) return &slot->costume;
    }
    return NULL;
}

uint8_t nativeCostumeRuntime_ModelVariant(Fighter* fp)
{
    if (fp == NULL) return 0;
    if (fp->player_id < 4) {
        for (unsigned m = 0; m < players[fp->player_id].member_count; ++m) {
            const NativeCostumeSlot* slot = &players[fp->player_id].members[m];
            if (slot->active && fp->kind == slot->kind &&
                fp->x619_costume_id == slot->base_costume) return slot->model_variant;
        }
    }
    return fp->x619_costume_id;
}

void nativeCostumeRuntime_Inspect(void)
{
    for (unsigned port = 0; port < 4; ++port) {
        const NativeCostumePlayer* player = &players[port];
        char hex[65];
        nativeContentStore_IdHex(&player->id, hex);
        fprintf(stderr, "[costume-player] port=%u outfit=%s members=%u\n",
                port, hex, player->member_count);
        for (unsigned m = 0; m < player->member_count; ++m) {
            const NativeCostumeSlot* slot = &player->members[m];
            Fighter* actor = NULL;
            for (unsigned role = 0; role < NATIVE_FIGHTER_FAMILY_MEMBERS; ++role) {
                Fighter_GObj* object = Player_GetEntityAtIndex(port, role);
                Fighter* candidate = object ? GET_FIGHTER(object) : NULL;
                if (candidate && candidate->kind == slot->kind) actor = candidate;
            }
            fprintf(stderr, "[costume-member] port=%u member=%u kind=%u variant=%u actor=%u dormant=%u actual=%u motion=%d\n",
                    port, m, slot->kind, slot->model_variant, actor != NULL,
                    actor ? actor->x221F_b3 : 0,
                    actor && actor->x108_costume_joint == slot->costume.joint,
                    actor ? actor->motion_id : -1);
        }
    }
}

void nativeCostumeRuntime_Clear(void)
{
    // Call only after fighters from the previous scene have been destroyed.
    for (unsigned port = 0; port < 4; ++port) {
        for (unsigned m = 0; m < players[port].member_count; ++m)
            free(players[port].members[m].arena);
        free(players[port].package);
    }
    memset(players, 0, sizeof(players));
}

#endif
