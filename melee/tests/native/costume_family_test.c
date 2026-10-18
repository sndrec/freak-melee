#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far

#include <stdint.h>
#include <aurora/dvd.h>
#include <melee/ft/native_costume_package.h>
#include <melee/ft/native_costume_runtime.h>
#include <melee/ft/ftcostume_assets.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void require(bool condition, const char* message)
{
    if (!condition) { fputs(message, stderr); exit(1); }
}

int main(int argc, char** argv)
{
    require(argc == 5 && aurora_dvd_open(argv[1]), "ISO STORE FIGHTER_ID OUTFIT_ID required\n");
    const char* root = argv[2];
    NativeContentId parent, outfit;
    require(nativeContentStore_IdFromHex(argv[3], &parent) &&
            nativeContentStore_IdFromHex(argv[4], &outfit), "package identities\n");
    uint8_t* package = NULL;
    size_t size;
    NativeCostumePackage metadata;
    NativeCostumePackageData data;
    require(nativeContentStore_Read(root, &outfit, &package, &size) &&
            nativeContentStore_ParseCostume(package, size, &metadata, &data), "read outfit\n");
    NativeCostumeValidation validation;
    require(nativeCostumePackage_Validate(&metadata, &data, root, &validation), "validate every model\n");
    unsigned count = metadata.member_count;
    require(count >= 1 && count <= NATIVE_FIGHTER_FAMILY_MEMBERS, "member count\n");
    static Fighter fighter;
    fighter.player_id = 0;
    fighter.x619_costume_id = 0;
    if (count == 2) {
        NativeCostumePackage incomplete = metadata;
        incomplete.member_count = 1;
        require(!nativeCostumePackage_Validate(&incomplete, &data, root, NULL), "reject incomplete family\n");
        NativeCostumePackage swapped = metadata;
        memcpy(swapped.member_keys[0], metadata.member_keys[1], NATIVE_FIGHTER_CODE_KEY_BYTES);
        memcpy(swapped.member_keys[1], metadata.member_keys[0], NATIVE_FIGHTER_CODE_KEY_BYTES);
        require(!nativeCostumePackage_Validate(&swapped, &data, root, NULL), "reject wrong member mapping\n");
        // The first graph imports successfully; the second must prevent publication.
        uint8_t broken[32] = {0};
        NativeCostumePackageData invalid = data;
        invalid.members[1] = (NativeCostumeMemberData){broken, sizeof(broken)};
        uint8_t* failed_package = NULL;
        size_t failed_size;
        NativeContentId failed_id, cached;
        require(nativeContentStore_BuildCostume(&metadata, &invalid, &failed_package, &failed_size, &failed_id) &&
                nativeContentStore_Cache(root, failed_package, failed_size, NULL, &cached) &&
                !nativeCostumeRuntime_LoadCustom(0, &failed_id, &parent, Ft_Kind_CustomFirst, root, false),
                "failed second import rejects the whole outfit\n");
        for (unsigned m = 0; m < count; ++m) {
            fighter.kind = (FighterKind)(Ft_Kind_CustomFirst + m);
            require(nativeCostumeRuntime_ForFighter(&fighter) == NULL, "failed load publishes no member\n");
        }
        free(failed_package);
    }
    NativeContentId wrong = parent;
    wrong.bytes[0] ^= 1;
    require(!nativeCostumeRuntime_LoadCustom(0, &outfit, &wrong, Ft_Kind_CustomFirst, root, true), "reject wrong parent\n");
    require(nativeCostumeRuntime_LoadCustom(0, &outfit, &parent, Ft_Kind_CustomFirst, root, true) &&
            nativeCostumeRuntime_LoadCustom(0, &outfit, &parent, Ft_Kind_CustomFirst, root, true), "load installed coordinated outfit\n");
    require(!nativeCostumeRuntime_LoadCustom(0, &outfit, &wrong, Ft_Kind_CustomFirst, root, true),
            "loaded outfit still rejects another parent\n");
    const UnkCostumeStruct* previous = NULL;
    for (unsigned m = 0; m < count; ++m) {
        fighter.kind = (FighterKind)(Ft_Kind_CustomFirst + m);
        const UnkCostumeStruct* costume = nativeCostumeRuntime_ForFighter(&fighter);
        require(costume && costume != previous && costume->joint && costume->native_assets &&
                costume->native_assets->joint_count &&
                costume->native_assets->dat.data == data.members[m].dat - package + 32 +
                    (const uint8_t*)costume->native_assets->owned_bytes &&
                nativeCostumeRuntime_ModelVariant(&fighter) == metadata.model_variant,
                "member resolves its own DAT and shared variant\n");
        previous = costume;
        fighter.x619_costume_id = 1;
        require(!nativeCostumeRuntime_ForFighter(&fighter), "other color does not use this outfit\n");
        fighter.x619_costume_id = 0;
    }
    fighter.kind = (FighterKind)(Ft_Kind_CustomFirst + count);
    require(!nativeCostumeRuntime_ForFighter(&fighter), "unrelated member does not use this outfit\n");
    nativeCostumeRuntime_Clear();
    require(nativeContentStore_Uninstall(root, &outfit) &&
            !nativeCostumeRuntime_LoadCustom(0, &outfit, &parent, Ft_Kind_CustomFirst, root, true) &&
            nativeCostumeRuntime_LoadCustom(0, &outfit, &parent, Ft_Kind_CustomFirst, root, false) &&
            !nativeContentStore_IsInstalled(root, &outfit), "remote cached outfit remains uninstalled\n");
    for (unsigned m = 0; m < count; ++m) {
        fighter.kind = (FighterKind)(Ft_Kind_CustomFirst + m);
        require(nativeCostumeRuntime_ForFighter(&fighter), "remote member resolves its outfit\n");
    }
    nativeCostumeRuntime_Clear();
    require(nativeContentStore_Install(root, &outfit), "restore fixture outfit installation\n");
    free(package);
    aurora_dvd_close();
    printf("PASS %u coordinated outfit members\n", count);
    return 0;
}
