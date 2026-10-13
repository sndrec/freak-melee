#include "native_costume_package.h"

#ifdef MELEE_NATIVE

#include <melee/ft/ftcostume_assets.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/gm/gm_1601.h>
#include <melee/gm/native_stock_content.h>
#include <melee/pl/player.h>

#include <stdlib.h>
#include <string.h>

bool nativeCostumePackage_Validate(const NativeCostumePackage* costume,
                                   const void* dat, size_t dat_size,
                                   const char* root,
                                   NativeCostumeValidation* result)
{
    if (result != NULL) {
        memset(result, 0, sizeof(*result));
    }
    if (costume == NULL || dat == NULL || dat_size < 32) {
        return false;
    }
    const NativeContentId empty_id = { 0 };
    bool custom = memcmp(&costume->fighter_id, &empty_id,
                         sizeof(empty_id)) != 0;
    const char* joint_name;
    const char* matanim_name;
    uint16_t joint_count = 0;
    NativeFighterCostumeInfo custom_info;
    if (custom) {
        if (costume->character != 0 || costume->base_costume != 0 ||
            !nativeFighterRuntime_CostumeInfoCached(
                &costume->fighter_id, root, &custom_info)) {
            return false;
        }
        joint_name = custom_info.joint_name;
        matanim_name = custom_info.matanim_name;
        joint_count = custom_info.joint_count;
    } else {
        uint8_t character;
        if (!nativeStockContent_CharacterFromId(
                nativeStockContent_CharacterId(costume->character),
                &character) ||
            character != costume->character ||
            costume->base_costume >= gm_GetNumCostumesForCKind(character)) {
            return false;
        }
        FighterKind kind = Player_800325C8(character, false);
        if (kind < 0 || kind >= Ft_Kind_StockCount ||
            costume->base_costume >=
                CostumeListsForeachCharacter[kind].numCostumes) {
            return false;
        }
        const Fighter_CostumeStrings* names =
            &ftData_803C2360[kind][costume->base_costume];
        if (names->joint_name == NULL) {
            return false;
        }
        joint_name = names->joint_name;
        matanim_name = names->matanim_joint_name;
    }
    void* arena = malloc(FT_COSTUME_IMPORT_ARENA_SIZE);
    if (arena == NULL) {
        return false;
    }
    FtCostumeAssets assets;
    bool valid = ftCostumeAssets_Import(
        dat, dat_size, joint_name, matanim_name, arena,
        FT_COSTUME_IMPORT_ARENA_SIZE, FT_COSTUME_IMPORT_RECORD_CAPACITY,
        &assets) &&
        (!custom || assets.joint_count == joint_count);
    if (result != NULL) {
        result->import_status = assets.import.status;
        result->error_offset = assets.import.error_offset;
        result->imported_records = assets.import.record_count;
        result->arena_used = assets.import.arena_used;
    }
    free(arena);
    return valid;
}

#endif
