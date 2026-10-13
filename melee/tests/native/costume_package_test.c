#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>

#include <aurora/dvd.h>
#include <dolphin/dvd.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/native_costume_package.h>
#include <melee/ft/native_costume_import.h>
#include <melee/ft/native_costume_runtime.h>
#include <melee/gm/forward.h>
#include <melee/gm/native_content_store.h>
#include <melee/lb/lbfile.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void require(bool condition, const char* message)
{
    if (!condition) {
        fputs(message, stderr);
        exit(1);
    }
}

int main(int argc, char** argv)
{
    require(argc == 2 && aurora_dvd_open(argv[1]),
            "open original USA v1.02 ISO\n");
    const Fighter_CostumeStrings* names =
        &ftData_803C2360[Ft_Kind_Mario][0];
    char disc_path[64];
    snprintf(disc_path, sizeof(disc_path), "/%s",
             lbFileGetFullName(names->dat_filename));
    DVDFileInfo file;
    require(DVDOpen(disc_path, &file), "open stock Mario costume DAT\n");
    size_t dat_size = file.length;
    size_t padded = (dat_size + 31) & ~(size_t) 31;
    void* dat = _aligned_malloc(padded, 32);
    require(dat != NULL &&
                DVDReadPrio(&file, dat, padded, 0, 2) >= (s32) dat_size &&
                DVDClose(&file),
            "read stock Mario costume DAT\n");

    NativeCostumePackage metadata = {
        .character = CKind_Mario,
        .base_costume = 0,
        .title = "Mario - Original outfit",
    };
    NativeCostumeValidation validation;
    require(nativeCostumePackage_Validate(&metadata, dat, dat_size, NULL,
                                          &validation) &&
                validation.imported_records != 0,
            "validate original DAT against Mario's typed model graph\n");
    metadata.character = CKind_Fox;
    require(!nativeCostumePackage_Validate(&metadata, dat, dat_size, NULL,
                                           &validation),
            "reject costume DAT for wrong fighter's required symbols\n");
    metadata.character = CKind_Mario;
    require(!nativeCostumePackage_Validate(&metadata, dat, 31, NULL,
                                           &validation),
            "reject truncated costume DAT\n");

    uint8_t* package = NULL;
    size_t package_size = 0;
    NativeContentId id;
    require(nativeContentStore_BuildCostume(&metadata, dat, dat_size,
                                            &package, &package_size, &id),
            "package original DAT\n");
    char temporary[MAX_PATH];
    DWORD length = GetTempPathA(sizeof(temporary), temporary);
    require(length > 0 && length < sizeof(temporary), "temporary path\n");
    char root[MAX_PATH];
    int written = snprintf(root, sizeof(root),
                           "%sFreakMeleeCostume-%lu-%lu", temporary,
                           GetCurrentProcessId(), GetTickCount());
    require(written > 0 && written < sizeof(root), "package root\n");
    NativeContentId cached;
    require(nativeContentStore_Cache(root, package, package_size, NULL,
                                     &cached) &&
                !nativeContentStore_IsInstalled(root, &id),
            "stock DAT cached but not selectable\n");
    uint8_t* reread = NULL;
    size_t reread_size = 0;
    NativeCostumePackage reread_metadata;
    const uint8_t* reread_dat;
    size_t reread_dat_size;
    require(nativeContentStore_Read(root, &id, &reread, &reread_size) &&
                nativeContentStore_ParseCostume(
                    reread, reread_size, &reread_metadata, &reread_dat,
                    &reread_dat_size) &&
                nativeCostumePackage_Validate(&reread_metadata, reread_dat,
                                               reread_dat_size, root,
                                               &validation) &&
                nativeContentStore_Install(root, &id) &&
                nativeContentStore_IsInstalled(root, &id),
            "reimport and explicitly install cached original DAT\n");

    require(!nativeCostumeRuntime_LoadInstalled(0, &id, CKind_Fox, 0, root) &&
                !nativeCostumeRuntime_LoadInstalled(0, &id, CKind_Mario, 1,
                                                    root) &&
                nativeCostumeRuntime_LoadInstalled(0, &id, CKind_Mario, 0,
                                                   root),
            "only the matching installed costume loads for the fighter\n");
    static Fighter fighter;
    fighter.player_id = 0;
    fighter.kind = Ft_Kind_Mario;
    fighter.x619_costume_id = 0;
    const UnkCostumeStruct* selected =
        nativeCostumeRuntime_ForFighter(&fighter);
    require(selected != NULL && selected->joint != NULL &&
                selected->native_assets != NULL,
            "fighter resolves installed cosmetic model\n");
    fighter.x619_costume_id = 1;
    require(nativeCostumeRuntime_ForFighter(&fighter) == NULL,
            "other gameplay color still resolves stock assets\n");
    nativeCostumeRuntime_Clear();

    char hex[65];
    nativeContentStore_IdHex(&id, hex);
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s\\installed\\%s.fmi", root, hex);
    require(DeleteFileA(path), "remove installed marker\n");
    require(!nativeCostumeRuntime_LoadInstalled(0, &id, CKind_Mario, 0, root),
            "cached but uninstalled costume is not selectable\n");
    require(nativeCostumeRuntime_LoadCached(0, &id, CKind_Mario, 0, root) &&
                nativeCostumeRuntime_ForFighter(&fighter) == NULL,
            "cached costume loads for a match without collection membership\n");
    fighter.x619_costume_id = 0;
    require(nativeCostumeRuntime_ForFighter(&fighter) != NULL,
            "cached costume resolves for the matching match fighter\n");
    require(!nativeCostumeRuntime_LoadInstalled(0, &id, CKind_Mario, 0, root),
            "cached match costume does not become installed\n");
    nativeCostumeRuntime_Clear();
    snprintf(path, sizeof(path), "%s\\cache\\%s.fmc", root, hex);
    require(DeleteFileA(path), "remove cached package\n");
    metadata.character = CKind_Fox;
    require(!nativeCostumeImport_Install(root, &metadata, dat, dat_size,
                                        &cached, &validation) &&
                !nativeContentStore_IsCached(root, &id),
            "in-client import rejects a DAT for the wrong fighter\n");
    metadata.character = CKind_Mario;
    require(nativeCostumeImport_Install(root, &metadata, dat, dat_size,
                                       &cached, &validation) &&
                memcmp(&cached, &id, sizeof(id)) == 0 &&
                nativeContentStore_IsInstalled(root, &id),
            "in-client import validates and installs the DAT\n");
    snprintf(path, sizeof(path), "%s\\installed\\%s.fmi", root, hex);
    require(DeleteFileA(path), "remove imported marker\n");
    snprintf(path, sizeof(path), "%s\\cache\\%s.fmc", root, hex);
    require(DeleteFileA(path), "remove imported package\n");
    snprintf(path, sizeof(path), "%s\\installed", root);
    require(RemoveDirectoryA(path), "remove installed directory\n");
    snprintf(path, sizeof(path), "%s\\cache", root);
    require(RemoveDirectoryA(path) && RemoveDirectoryA(root),
            "remove package directories\n");
    free(reread);
    free(package);
    _aligned_free(dat);
    aurora_dvd_close();
    printf("original Mario DAT packaged: %zu bytes, %zu imported records\n",
           dat_size, validation.imported_records);
    return 0;
}
