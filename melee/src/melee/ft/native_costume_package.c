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
                                   const NativeCostumePackageData* data,
                                   const char* root,
                                   NativeCostumeValidation* result)
{
    if (result != NULL) {
        memset(result, 0, sizeof(*result));
    }
    if (costume == NULL || data == NULL || !costume->member_count ||
        costume->member_count > NATIVE_FIGHTER_FAMILY_MEMBERS) {
        return false;
    }
    const NativeContentId empty_id = { 0 };
    bool custom = memcmp(&costume->fighter_id, &empty_id,
                         sizeof(empty_id)) != 0;
    const char* joint_name;
    const char* matanim_name;
    NativeFighterCostumeFamilyInfo custom_info;
    FtCostumeBindings stock_bindings;
    if (custom) {
        if (costume->character != 0 || costume->base_costume != 0 ||
            !nativeFighterRuntime_CostumeInfoCached(
                &costume->fighter_id, root, costume->model_variant, &custom_info)) {
            return false;
        }
        if (costume->member_count != custom_info.member_count) return false;
        for (unsigned m = 0; m < costume->member_count; ++m)
            if (memcmp(costume->member_keys[m], custom_info.members[m].key,
                       NATIVE_FIGHTER_CODE_KEY_BYTES)) return false;
    } else {
        if (costume->member_count != 1) return false;
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
                CostumeListsForeachCharacter[kind].numCostumes ||
            !ftCostumeBindings_Stock(kind, costume->base_costume, &stock_bindings)) {
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
    bool valid = true;
    for (unsigned m = 0; valid && m < costume->member_count; ++m) {
        FtCostumeAssets assets = {0};
        if (custom) {
            joint_name = custom_info.members[m].joint_name;
            matanim_name = custom_info.members[m].matanim_name;
        }
        valid = data->members[m].dat && data->members[m].size >= 32 &&
            ftCostumeAssets_Load(data->members[m].dat, data->members[m].size,
                joint_name, matanim_name, &assets) &&
            ftCostumeBindings_Fit(custom ? &custom_info.members[m].bindings : &stock_bindings,
                                  &assets);
        if (result != NULL) {
            result->member = m;
            result->import_status = !valid && assets.import.status == HSD_DAT_OK
                                        ? HSD_DAT_DESCRIPTOR : assets.import.status;
            result->error_offset = assets.import.error_offset;
            result->imported_records += assets.import.record_count;
            result->arena_used += assets.import.arena_used;
        }
        free(assets.arena);
    }
    return valid;
}

#endif
