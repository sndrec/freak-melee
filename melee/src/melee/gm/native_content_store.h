#ifndef MELEE_GM_NATIVE_CONTENT_STORE_H
#define MELEE_GM_NATIVE_CONTENT_STORE_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    NATIVE_CONTENT_ID_BYTES = 32,
    NATIVE_CONTENT_TITLE_BYTES = 64,
    NATIVE_CONTENT_MAX_PACKAGE_BYTES = 64 * 1024 * 1024,
};

typedef struct NativeContentId {
    uint8_t bytes[NATIVE_CONTENT_ID_BYTES];
} NativeContentId;

typedef struct NativeCostumePackage {
    uint8_t character;
    uint8_t base_costume;
    // Script-visible variant of a custom skin; separate from its asset slot.
    uint8_t model_variant;
    // Zero for a stock fighter. Custom costumes belong to this exact bundle.
    NativeContentId fighter_id;
    char title[NATIVE_CONTENT_TITLE_BYTES];
} NativeCostumePackage;

enum {
    NATIVE_FIGHTER_CODE_KEY_BYTES = 32,
    NATIVE_FIGHTER_FAMILY_MEMBERS = 2,
};

typedef enum NativeFighterFamilyMode {
    NativeFighterFamily_Single,
    NativeFighterFamily_AlternateForm,
    NativeFighterFamily_SimultaneousCompanion,
} NativeFighterFamilyMode;

typedef struct NativeFighterPackageMember {
    // Stable authoring/cache identity; independent of mutable player roles.
    char key[NATIVE_FIGHTER_CODE_KEY_BYTES];
} NativeFighterPackageMember;

typedef struct NativeFighterPackage {
    char title[NATIVE_CONTENT_TITLE_BYTES];
    NativeFighterFamilyMode mode;
    uint8_t member_count;
    NativeFighterPackageMember members[NATIVE_FIGHTER_FAMILY_MEMBERS];
} NativeFighterPackage;

typedef struct NativeFighterMemberData {
    const uint8_t* fighter_dat;
    size_t fighter_size;
    const uint8_t* animation_dat;
    size_t animation_size;
    const uint8_t* costume_dat;
    size_t costume_size;
    const uint8_t* asset_record;
    size_t asset_record_size;
    const uint8_t* script_text;
    size_t script_size;
    const uint8_t* parameter_text;
    size_t parameter_size;
    const uint8_t* cpu_text;
    size_t cpu_size;
    const uint8_t* resources;
    size_t resource_size;
} NativeFighterMemberData;

typedef struct NativeFighterPackageData {
    const uint8_t* portrait_png;
    size_t portrait_size;
    NativeFighterMemberData members[NATIVE_FIGHTER_FAMILY_MEMBERS];
} NativeFighterPackageData;

// A package contains one original-format costume DAT. Stock packages keep
// their original encoding; custom packages carry the owning fighter ID.
// Building does not validate the DAT graph; importers must do that first.
bool nativeContentStore_BuildCostume(const NativeCostumePackage* costume,
                                    const void* dat, size_t dat_size,
                                    uint8_t** package, size_t* package_size,
                                    NativeContentId* id);
bool nativeContentStore_ParseCostume(const void* package, size_t package_size,
                                    NativeCostumePackage* costume,
                                    const uint8_t** dat, size_t* dat_size);

// One content identity owns the portrait and source member payloads. The
// canonical source format carries Single or two-member alternate/companion families.
// Returned parse spans borrow the package allocation. Parsing validates the
// complete canonical record before publishing either output. Source graph /
// asset/FMS validation belongs to candidate preparation before activation.
bool nativeContentStore_BuildFighter(const NativeFighterPackage* fighter,
                                    const NativeFighterPackageData* data,
                                    uint8_t** package, size_t* package_size,
                                    NativeContentId* id);
bool nativeContentStore_ParseFighter(const void* package, size_t package_size,
                                    NativeFighterPackage* fighter,
                                    NativeFighterPackageData* data);

// The cache is non-selectable. Installed membership is a separate, explicit
// per-installation marker referring to a verified cached package.
bool nativeContentStore_Cache(const char* root, const void* package,
                              size_t package_size,
                              const NativeContentId* expected_id,
                              NativeContentId* id);
// Explicit acceptance also retains a costume's exact cached fighter parent.
// The caller validates source behavior and asset graphs before installation.
bool nativeContentStore_Install(const char* root, const NativeContentId* id);
// Remove local selectability while retaining cached bytes for existing peers.
bool nativeContentStore_Uninstall(const char* root, const NativeContentId* id);
bool nativeContentStore_IsCached(const char* root, const NativeContentId* id);
bool nativeContentStore_IsInstalled(const char* root, const NativeContentId* id);
bool nativeContentStore_Read(const char* root, const NativeContentId* id,
                             uint8_t** package, size_t* package_size);
bool nativeContentStore_ListInstalled(const char* root, NativeContentId* ids,
                                      size_t capacity, size_t* count);
bool nativeContentStore_DefaultRoot(char* path, size_t capacity);
void nativeContentStore_IdHex(const NativeContentId* id, char hex[65]);
bool nativeContentStore_IdFromHex(const char* hex, NativeContentId* id);

#ifdef __cplusplus
}
#endif

#endif
#endif
