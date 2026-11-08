#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far

#include <melee/gm/native_content_store.h>
#include <melee/ft/native_source_assets.h>
#include <melee/ft/native_fighter_pose.h>
#include <melee/ft/kinds/ftCommon/forward.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codec_dependencies.inc"

static void require(bool condition, const char* message)
{
    if (!condition) {
        fputs(message, stderr);
        exit(1);
    }
}

static void family_outfit_codec(void)
{
    NativeCostumePackage metadata = {.member_count=2, .model_variant=7};
    metadata.fighter_id.bytes[0] = 1;
    strcpy(metadata.title, "Coordinated outfit");
    strcpy(metadata.member_keys[0], "main");
    strcpy(metadata.member_keys[1], "partner");
    uint8_t first[48] = {1}, second[64] = {2};
    NativeCostumePackageData data = {.members={{first,sizeof(first)},{second,sizeof(second)}}};
    uint8_t* bytes;
    size_t size;
    NativeContentId id;
    require(nativeContentStore_BuildCostume(&metadata, &data, &bytes, &size, &id), "build paired outfit\n");
    NativeCostumePackage parsed;
    NativeCostumePackageData spans;
    require(nativeContentStore_ParseCostume(bytes, size, &parsed, &spans) &&
            parsed.member_count == 2 && !strcmp(parsed.member_keys[1], "partner") &&
            spans.members[0].size == sizeof(first) && spans.members[1].size == sizeof(second) &&
            !memcmp(spans.members[0].dat, first, sizeof(first)) &&
            !memcmp(spans.members[1].dat, second, sizeof(second)), "paired outfit exact spans\n");
    NativeCostumePackage untouched = parsed;
    NativeCostumePackageData untouched_spans = spans;
    for (size_t cut = 0; cut < size; ++cut) {
        require(!nativeContentStore_ParseCostume(bytes, cut, &parsed, &spans) &&
                !memcmp(&parsed, &untouched, sizeof(parsed)) &&
                !memcmp(&spans, &untouched_spans, sizeof(spans)), "truncation preserves parser outputs\n");
    }
    bytes[6] = 0;
    require(!nativeContentStore_ParseCostume(bytes, size, &parsed, &spans), "reject empty family\n");
    bytes[6] = 3;
    require(!nativeContentStore_ParseCostume(bytes, size, &parsed, &spans), "reject excessive family\n");
    bytes[6] = 2;
    memcpy(bytes + 144, bytes + 108, 32);
    require(!nativeContentStore_ParseCostume(bytes, size, &parsed, &spans), "reject duplicate member keys\n");
    memcpy(bytes + 144, metadata.member_keys[1], 32);
    bytes[176]++;
    require(!nativeContentStore_ParseCostume(bytes, size, &parsed, &spans), "reject member span overflow\n");
    bytes[176]--;
    require(nativeContentStore_ParseCostume(bytes, size, &parsed, &spans), "restore paired payload\n");
    free(bytes);
    metadata.member_count = 1;
    require(!nativeContentStore_BuildCostume(&metadata, &data, &bytes, &size, &id), "reject unused member payload\n");
    metadata.member_count = 2;
    strcpy(metadata.member_keys[1], "main");
    require(!nativeContentStore_BuildCostume(&metadata, &data, &bytes, &size, &id), "reject duplicate build keys\n");
}

int main(void)
{
    family_outfit_codec();
    char temporary[MAX_PATH];
    DWORD length = GetTempPathA(sizeof(temporary), temporary);
    require(length > 0 && length < sizeof(temporary), "temporary path\n");
    char root[MAX_PATH];
    int written = snprintf(root, sizeof(root), "%sFreakMeleeContent-%lu-%lu",
                           temporary, GetCurrentProcessId(), GetTickCount());
    require(written > 0 && written < sizeof(root), "store root\n");

    NativeCostumePackageData metadata_data;
    NativeCostumePackage metadata = { .member_count = 1, .character = 8, .base_costume = 0 };
    strcpy(metadata.title, "Mario - Test skin");
    uint8_t dat[96];
    for (size_t i = 0; i < sizeof(dat); ++i) {
        dat[i] = (uint8_t) (i * 17);
    }
    uint8_t* built = NULL;
    size_t built_size = 0;
    NativeContentId id;
    require(nativeContentStore_BuildCostume(&metadata, &(NativeCostumePackageData){.members={{dat,sizeof(dat)}}},
                                            &built, &built_size, &id),
            "build costume package\n");
    NativeCostumePackage parsed;
    NativeCostumePackageData costume_parsed_data;
    require(nativeContentStore_ParseCostume(built, built_size, &parsed, &costume_parsed_data) &&
                parsed.character == metadata.character &&
                parsed.base_costume == metadata.base_costume &&
                strcmp(parsed.title, metadata.title) == 0 &&
                costume_parsed_data.members[0].size == sizeof(dat) &&
                memcmp(costume_parsed_data.members[0].dat, dat, sizeof(dat)) == 0,
            "canonical costume package round trip\n");

    char hex[65];
    NativeContentId decoded;
    nativeContentStore_IdHex(&id, hex);
    require(nativeContentStore_IdFromHex(hex, &decoded) &&
                memcmp(&decoded, &id, sizeof(id)) == 0,
            "stable package ID encoding\n");
    hex[0] = 'G';
    require(!nativeContentStore_IdFromHex(hex, &decoded),
            "reject invalid package ID\n");
    nativeContentStore_IdHex(&id, hex);

    NativeContentId wrong = id;
    wrong.bytes[0] ^= 0x80;
    NativeContentId cached;
    require(!nativeContentStore_Cache(root, built, built_size, &wrong,
                                      &cached) &&
                !nativeContentStore_IsCached(root, &id),
            "reject a transfer with the wrong requested identity\n");
    require(nativeContentStore_Cache(root, built, built_size, NULL, &cached) &&
                memcmp(&cached, &id, sizeof(id)) == 0 &&
                nativeContentStore_IsCached(root, &id) &&
                !nativeContentStore_IsInstalled(root, &id),
            "received package stays non-selectable in cache\n");
    NativeContentId installed[2];
    size_t count = 99;
    require(nativeContentStore_ListInstalled(root, installed, 2, &count) &&
                count == 0,
            "cache does not enter installed collection\n");
    require(nativeContentStore_Install(root, &id) &&
                nativeContentStore_IsInstalled(root, &id) &&
                nativeContentStore_ListInstalled(root, installed, 2,
                                                 &count) &&
                count == 1 && memcmp(installed, &id, sizeof(id)) == 0,
            "explicit acceptance installs cached package\n");

    uint8_t* loaded = NULL;
    size_t loaded_size = 0;
    require(nativeContentStore_Read(root, &id, &loaded, &loaded_size) &&
                loaded_size == built_size &&
                memcmp(loaded, built, built_size) == 0,
            "read verified cached package\n");
    free(loaded);

    require(nativeContentStore_Uninstall(root, &id) &&
                !nativeContentStore_IsInstalled(root, &id) &&
                nativeContentStore_IsCached(root, &id) &&
                nativeContentStore_Install(root, &id),
            "removing an install retains cached package bytes\n");

    NativeCostumePackage other_metadata = metadata;
    strcpy(other_metadata.title, "Mario - Another title");
    uint8_t* other = NULL;
    size_t other_size = 0;
    NativeContentId other_id;
    require(nativeContentStore_BuildCostume(&other_metadata, &(NativeCostumePackageData){.members={{dat,sizeof(dat)}}},
                                            &other, &other_size, &other_id) &&
                memcmp(&other_id, &id, sizeof(id)) != 0,
            "package identity covers metadata as well as DAT bytes\n");
    free(other);

    NativeCostumePackage custom_metadata = { .member_count = 1 };
    strcpy(custom_metadata.member_keys[0], "main");
    custom_metadata.model_variant = 3;
    custom_metadata.fighter_id.bytes[0] = 0xA5;
    custom_metadata.fighter_id.bytes[31] = 0x5A;
    strcpy(custom_metadata.title, "Clone costume");
    uint8_t* custom = NULL;
    size_t custom_size = 0;
    NativeContentId custom_id;
    require(nativeContentStore_BuildCostume(&custom_metadata, &(NativeCostumePackageData){.members={{dat,sizeof(dat)}}}, &custom,
                &custom_size, &custom_id) &&
                custom_size == sizeof(dat) + 180 &&
                nativeContentStore_ParseCostume(custom, custom_size, &parsed, &costume_parsed_data) &&
                memcmp(&parsed.fighter_id, &custom_metadata.fighter_id,
                       sizeof(parsed.fighter_id)) == 0 &&
                costume_parsed_data.members[0].size == sizeof(dat) &&
                parsed.model_variant == 3 && parsed.character == 0 && parsed.base_costume == 0 &&
                memcmp(costume_parsed_data.members[0].dat, dat, sizeof(dat)) == 0,
            "custom costume retains its exact parent bundle\n");
    NativeCostumePackage changed_variant = custom_metadata;
    changed_variant.model_variant = 2;
    uint8_t* variant_package = NULL;
    size_t variant_size;
    NativeContentId variant_id;
    require(nativeContentStore_BuildCostume(&changed_variant, &(NativeCostumePackageData){.members={{dat,sizeof(dat)}}},
                &variant_package, &variant_size, &variant_id) &&
                memcmp(&variant_id, &custom_id, sizeof(variant_id)) != 0,
            "costume variant participates in package identity\n");
    free(variant_package);
    custom[76] = 0;
    custom[107] = 0;
    require(!nativeContentStore_ParseCostume(custom, custom_size, &parsed, &costume_parsed_data),
            "custom costume requires parent identity\n");
    free(custom);
    custom_metadata.fighter_id.bytes[0] ^= 1;
    require(nativeContentStore_BuildCostume(&custom_metadata, &(NativeCostumePackageData){.members={{dat,sizeof(dat)}}}, &custom,
                &custom_size, &decoded) &&
                memcmp(&decoded, &custom_id, sizeof(decoded)) != 0,
            "changing the parent fighter changes costume identity\n");
    free(custom);

    char cache_path[MAX_PATH];
    written = snprintf(cache_path, sizeof(cache_path), "%s\\cache\\%s.fmc",
                       root, hex);
    require(written > 0 && written < sizeof(cache_path), "cache path\n");
    HANDLE file = CreateFileA(cache_path, GENERIC_WRITE, 0, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    require(file != INVALID_HANDLE_VALUE, "open cached package for test\n");
    LARGE_INTEGER offset = { .QuadPart = 80 };
    DWORD changed = 0;
    uint8_t corrupt = 0xFF;
    require(SetFilePointerEx(file, offset, NULL, FILE_BEGIN) &&
                WriteFile(file, &corrupt, 1, &changed, NULL) && changed == 1 &&
                CloseHandle(file),
            "modify cached package\n");
    require(!nativeContentStore_IsCached(root, &id) &&
                !nativeContentStore_IsInstalled(root, &id),
            "reject corrupted cached dependency and collection membership\n");
    require(nativeContentStore_Cache(root, built, built_size, NULL, &cached) &&
                nativeContentStore_IsInstalled(root, &id),
            "valid transfer repairs corrupted cache without new acceptance\n");

    char marker_path[MAX_PATH];
    written = snprintf(marker_path, sizeof(marker_path),
                       "%s\\installed\\%s.fmi", root, hex);
    require(written > 0 && written < sizeof(marker_path), "marker path\n");
    require(DeleteFileA(marker_path) && DeleteFileA(cache_path),
            "remove test package files\n");
    char installed_directory[MAX_PATH];
    char cache_directory[MAX_PATH];
    snprintf(installed_directory, sizeof(installed_directory),
             "%s\\installed", root);
    snprintf(cache_directory, sizeof(cache_directory), "%s\\cache", root);
    require(RemoveDirectoryA(installed_directory) &&
                RemoveDirectoryA(cache_directory) && RemoveDirectoryA(root),
            "remove test package directories\n");
    free(built);

    char fighter_root[MAX_PATH];
    written = snprintf(fighter_root, sizeof(fighter_root), "%s-fighter", root);
    require(written > 0 && written < sizeof(fighter_root),
            "fighter store root\n");
    uint8_t fighter_dat[96], animation_dat[64], costume_dat[80];
    uint8_t portrait_png[45] = {
        137, 'P', 'N', 'G', 13, 10, 26, 10,
        0, 0, 0, 13, 'I', 'H', 'D', 'R',
        0, 0, 0, 64, 0, 0, 0, 56, 8, 6,
        [37] = 'I', [38] = 'E', [39] = 'N', [40] = 'D',
    };
    for (size_t i = 0; i < sizeof(fighter_dat); ++i) {
        fighter_dat[i] = (uint8_t) (i * 3);
    }
    memset(animation_dat, 0xA5, sizeof(animation_dat));
    memset(costume_dat, 0x5A, sizeof(costume_dat));
    NativeFighterPackage fighter = { .mode = NativeFighterFamily_Single, .member_count = 1 };
    strcpy(fighter.members[0].key, "main");
    NativeSourceAssetFacts facts = { .actions=1,.demo_count=1,.joints=FT_COMMON_PART_COUNT,
        .bone_count=FT_COMMON_PART_COUNT,.effect_bank=UINT8_MAX,
        .moves={.common_count=ftCo_MS_Count,.first=ftCo_MS_Count,.count=1} };
    strcpy(facts.fighter_root,"ftDataTest");strcpy(facts.costume_joint,"PlyTest_Share_joint");
    for(unsigned i=0;i<FT_COMMON_PART_COUNT;++i)facts.bones[i]=(NativeFighterBone){(Fighter_Part)i,(uint16_t)i};
    uint8_t asset_record[65536];size_t asset_bytes=0;NativeScriptDiagnostic diagnostic={0};
    require(nativeSourceAssets_Write(&facts,asset_record,sizeof(asset_record),&asset_bytes,&diagnostic),"actual source facts codec write\n");
    static const uint8_t script_text[] = "fighter:\n    behavior independent\naction Idle:\n    on tick:\n        pass\n";
    strcpy(fighter.title, "Rook");
    strcpy(fighter.tile_name, "ROOK!");
    NativeFighterPackageData source = {
        .portrait_png = portrait_png,
        .portrait_size = sizeof(portrait_png),
        .members = {{
            .fighter_dat = fighter_dat,
            .fighter_size = sizeof(fighter_dat),
            .animation_dat = animation_dat,
            .animation_size = sizeof(animation_dat),
            .costume_dat = costume_dat,
            .costume_size = sizeof(costume_dat),
            .asset_record=asset_record,.asset_record_size=asset_bytes,
            .script_text=script_text,.script_size=sizeof(script_text)-1,
        }},
    };
    uint8_t* fighter_package = NULL;
    size_t fighter_package_size = 0;
    NativeContentId fighter_id;
    require(nativeContentStore_BuildFighter(&fighter, &source,
                                            &fighter_package,
                                            &fighter_package_size,
                                            &fighter_id),
            "build fighter bundle\n");
    portrait_png[0] = 0;
    uint8_t* rejected_package = NULL;
    size_t rejected_size = 0;
    NativeContentId rejected_id;
    require(!nativeContentStore_BuildFighter(&fighter, &source,
                                             &rejected_package,
                                             &rejected_size, &rejected_id),
            "reject invalid portrait in fighter bundle\n");
    portrait_png[0] = 137;
    NativeFighterPackage parsed_fighter;
    NativeFighterPackageData parsed_data;
    require(nativeContentStore_ParseFighter(fighter_package,
                                            fighter_package_size,
                                            &parsed_fighter, &parsed_data) &&
                strcmp(parsed_fighter.members[0].key, "main") == 0 &&
                strcmp(parsed_fighter.title, "Rook") == 0 &&
                strcmp(parsed_fighter.tile_name, "ROOK!") == 0 &&
                parsed_data.members[0].fighter_size == sizeof(fighter_dat) &&
                parsed_data.members[0].animation_size == sizeof(animation_dat) &&
                parsed_data.members[0].costume_size == sizeof(costume_dat) &&
                parsed_data.portrait_size == sizeof(portrait_png) &&
                memcmp(parsed_data.members[0].fighter_dat, fighter_dat,
                       sizeof(fighter_dat)) == 0 &&
                memcmp(parsed_data.members[0].animation_dat, animation_dat,
                       sizeof(animation_dat)) == 0 &&
                memcmp(parsed_data.members[0].costume_dat, costume_dat,
                       sizeof(costume_dat)) == 0 &&
                memcmp(parsed_data.portrait_png, portrait_png,
                       sizeof(portrait_png)) == 0 &&
                !nativeContentStore_ParseCostume(fighter_package, fighter_package_size, &metadata, &metadata_data),
            "typed fighter bundle round trip\n");
    uint8_t saved_label_byte = fighter_package[208 + sizeof(portrait_png)];
    fighter_package[208 + sizeof(portrait_png)] = '\n';
    require(!nativeContentStore_ParseFighter(fighter_package, fighter_package_size,
                                            &parsed_fighter, &parsed_data), "reject control bytes in a tile name\n");
    fighter_package[208 + sizeof(portrait_png)] = saved_label_byte;
    fighter_package[12] = 255;
    require(!nativeContentStore_ParseFighter(fighter_package, fighter_package_size,
                                            &parsed_fighter, &parsed_data), "reject oversized tile names\n");
    fighter_package[12] = 5;
    static const uint8_t parameter_text[] = "float Lift = 1.25\n";
    source.members[0].parameter_text = parameter_text;
    source.members[0].parameter_size = sizeof(parameter_text) - 1;
    static const uint8_t cpu_text[] = "distance 22\nground 1 5 -2 15 -5 7 1 1 0\n";
    source.members[0].cpu_text = cpu_text;
    source.members[0].cpu_size = sizeof(cpu_text) - 1;
    uint8_t* tuned_package = NULL;
    size_t tuned_package_size = 0;
    NativeContentId tuned_id;
    require(nativeContentStore_BuildFighter(&fighter, &source,
                                            &tuned_package,
                                            &tuned_package_size, &tuned_id),
            "build source fighter with parameters and CPU values\n");
    require(tuned_package_size == fighter_package_size +
                                           source.members[0].parameter_size + source.members[0].cpu_size &&
                memcmp(&tuned_id, &fighter_id, sizeof(tuned_id)) != 0 &&
                nativeContentStore_ParseFighter(tuned_package,
                                                tuned_package_size,
                                                &parsed_fighter, &parsed_data),
            "parse source fighter with parameters and CPU values\n");
    require(
                parsed_data.members[0].asset_record_size == asset_bytes &&
                memcmp(parsed_data.members[0].asset_record, asset_record, asset_bytes) == 0 &&
                parsed_data.members[0].script_size == source.members[0].script_size &&
                memcmp(parsed_data.members[0].script_text, script_text,
                       source.members[0].script_size) == 0 &&
                parsed_data.members[0].parameter_size == source.members[0].parameter_size &&
                memcmp(parsed_data.members[0].parameter_text, parameter_text,
                       source.members[0].parameter_size) == 0 &&
                parsed_data.members[0].cpu_size == source.members[0].cpu_size &&
                memcmp(parsed_data.members[0].cpu_text, cpu_text, source.members[0].cpu_size) == 0,
            "fighter bundle carries actual facts, FMS, parameters, and CPU tuning\n");
    tuned_package[124]++;
    require(!nativeContentStore_ParseFighter(tuned_package,
                                             tuned_package_size,
                                             &parsed_fighter, &parsed_data),
            "reject mismatched source facts length\n");
    tuned_package[124]--;
    tuned_package[128]++;
    require(!nativeContentStore_ParseFighter(tuned_package,
                                             tuned_package_size,
                                             &parsed_fighter, &parsed_data),
            "reject mismatched fighter script length\n");
    tuned_package[128]--;
    tuned_package[132]++;
    require(!nativeContentStore_ParseFighter(tuned_package,
                                             tuned_package_size,
                                             &parsed_fighter, &parsed_data),
            "reject mismatched fighter parameter length\n");
    tuned_package[132]--;
    tuned_package[136]++;
    require(!nativeContentStore_ParseFighter(tuned_package, tuned_package_size,
                                             &parsed_fighter, &parsed_data),
            "reject mismatched fighter CPU length\n");
    tuned_package[136]--;
    source.members[0].asset_record_size = 0;source.members[0].asset_record = NULL;
    require(!nativeContentStore_BuildFighter(&fighter,&source,&rejected_package,&rejected_size,&rejected_id),"reject source fighter without owned facts\n");
    source.members[0].asset_record_size=asset_bytes;source.members[0].asset_record=asset_record;
    source.members[0].script_size = 0;
    source.members[0].script_text = NULL;
    source.members[0].parameter_size = 0;
    source.members[0].parameter_text = NULL;
    require(!nativeContentStore_BuildFighter(&fighter,&source,&rejected_package,&rejected_size,&rejected_id),"reject source fighter without FMS\n");
    source.members[0].script_text=script_text;source.members[0].script_size=sizeof(script_text)-1;
    fighter.mode=NativeFighterFamily_AlternateForm;fighter.member_count=2;
    require(!nativeContentStore_BuildFighter(&fighter,&source,&rejected_package,&rejected_size,&rejected_id),"reject unsupported paired source family\n");
    fighter.mode=NativeFighterFamily_SimultaneousCompanion;
    require(!nativeContentStore_BuildFighter(&fighter,&source,&rejected_package,&rejected_size,&rejected_id),"reject unsupported linked source family\n");
    fighter.mode=NativeFighterFamily_Single;fighter.member_count=1;
    free(tuned_package);
    fighter_package[5] = 1;
    require(!nativeContentStore_ParseFighter(fighter_package,
                                             fighter_package_size,
                                             &parsed_fighter, &parsed_data),
            "reject noncanonical fighter header\n");
    fighter_package[5] = 0;
    fighter_package[80] = 'R';
    require(!nativeContentStore_ParseFighter(fighter_package,
                                             fighter_package_size,
                                             &parsed_fighter, &parsed_data),
            "reject invalid source member key\n");
    fighter_package[80] = 'm';
    fighter_package[8] = 0xFF;
    require(!nativeContentStore_ParseFighter(fighter_package,
                                             fighter_package_size,
                                             &parsed_fighter, &parsed_data),
            "reject fighter payload length mismatch\n");
    fighter_package[8] = sizeof(portrait_png);
    fighter_package[208] = 0;
    require(!nativeContentStore_ParseFighter(fighter_package,
                                             fighter_package_size,
                                             &parsed_fighter, &parsed_data),
            "reject damaged fighter portrait\n");
    fighter_package[208] = 137;
    NativeCostumePackage outfit = {.fighter_id=fighter_id, .member_count=1};
    strcpy(outfit.member_keys[0], "main");
    strcpy(outfit.title, "Owned outfit");
    uint8_t* outfit_package = NULL;
    size_t outfit_size;
    NativeContentId outfit_id;
    require(nativeContentStore_BuildCostume(&outfit, &(NativeCostumePackageData){.members={{costume_dat,sizeof(costume_dat)}}},
                &outfit_package,&outfit_size,&outfit_id) &&
            nativeContentStore_Cache(fighter_root,outfit_package,outfit_size,NULL,&cached) &&
            !nativeContentStore_Install(fighter_root,&outfit_id) &&
            !nativeContentStore_IsInstalled(fighter_root,&outfit_id),
            "outfit acceptance requires its exact cached parent\n");
    require(nativeContentStore_Cache(fighter_root, fighter_package,
                                     fighter_package_size, NULL, &cached) &&
                memcmp(&cached, &fighter_id, sizeof(cached)) == 0 &&
                nativeContentStore_IsCached(fighter_root, &fighter_id) &&
                !nativeContentStore_IsInstalled(fighter_root, &fighter_id),
            "fighter bundle caches without installation\n");
    snprintf(installed_directory,sizeof(installed_directory),"%s\\installed",fighter_root);
    char outfit_hex[65], outfit_marker[MAX_PATH], outfit_cache[MAX_PATH];
    nativeContentStore_IdHex(&outfit_id,outfit_hex);
    snprintf(outfit_marker,sizeof(outfit_marker),"%s\\%s.fmi",installed_directory,outfit_hex);
    snprintf(outfit_cache,sizeof(outfit_cache),"%s\\cache\\%s.fmc",fighter_root,outfit_hex);
    require(CreateDirectoryA(installed_directory,NULL) && CreateDirectoryA(outfit_marker,NULL),
            "block outfit marker with a directory\n");
    require(!nativeContentStore_Install(fighter_root,&outfit_id) &&
                !nativeContentStore_IsInstalled(fighter_root,&fighter_id),
            "failed outfit publication removes its newly installed dependency\n");
    require(nativeContentStore_Install(fighter_root,&fighter_id) &&
                !nativeContentStore_Install(fighter_root,&outfit_id) &&
                nativeContentStore_IsInstalled(fighter_root,&fighter_id),
            "failed outfit publication preserves a previously installed dependency\n");
    require(RemoveDirectoryA(outfit_marker) && nativeContentStore_Uninstall(fighter_root,&fighter_id) &&
                nativeContentStore_Install(fighter_root,&outfit_id) &&
                nativeContentStore_IsInstalled(fighter_root,&fighter_id) &&
                nativeContentStore_IsInstalled(fighter_root,&outfit_id) &&
                nativeContentStore_Install(fighter_root,&outfit_id),
            "explicit outfit acceptance installs its cached dependency and repeats safely\n");
    require(nativeContentStore_Uninstall(fighter_root,&outfit_id) && DeleteFileA(outfit_cache),
            "remove outfit fixture\n");
    free(outfit_package);
    require(nativeContentStore_Install(fighter_root, &fighter_id) &&
                nativeContentStore_IsInstalled(fighter_root, &fighter_id),
            "fighter bundle installation marker\n");
    nativeContentStore_IdHex(&fighter_id, hex);
    written = snprintf(cache_path, sizeof(cache_path),
                       "%s\\cache\\%s.fmc", fighter_root, hex);
    require(written > 0 && written < sizeof(cache_path),
            "fighter cache path\n");
    file = CreateFileA(cache_path, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    offset.QuadPart = 120;
    require(file != INVALID_HANDLE_VALUE &&
                SetFilePointerEx(file, offset, NULL, FILE_BEGIN) &&
                WriteFile(file, &corrupt, 1, &changed, NULL) && changed == 1 &&
                CloseHandle(file) &&
                !nativeContentStore_IsCached(fighter_root, &fighter_id) &&
                !nativeContentStore_IsInstalled(fighter_root, &fighter_id),
            "fighter asset corruption invalidates identity\n");
    written = snprintf(marker_path, sizeof(marker_path),
                       "%s\\installed\\%s.fmi", fighter_root, hex);
    require(written > 0 && written < sizeof(marker_path) &&
                DeleteFileA(marker_path) && DeleteFileA(cache_path),
            "remove fighter package files\n");
    snprintf(installed_directory, sizeof(installed_directory),
             "%s\\installed", fighter_root);
    snprintf(cache_directory, sizeof(cache_directory),
             "%s\\cache", fighter_root);
    require(RemoveDirectoryA(installed_directory) &&
                RemoveDirectoryA(cache_directory) &&
                RemoveDirectoryA(fighter_root),
            "remove fighter package directories\n");
    free(fighter_package);
    puts("Native costume and fighter content store passed");
    return 0;
}
