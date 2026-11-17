#ifndef MELEE_FT_NATIVE_FIGHTER_FAMILY_PROJECT_H
#define MELEE_FT_NATIVE_FIGHTER_FAMILY_PROJECT_H

#include <melee/ft/native_fighter_project.h>

#ifdef MELEE_NATIVE
#ifdef __cplusplus
extern "C" {
#endif

enum { NATIVE_FIGHTER_FAMILY_PROJECT_PATH_BYTES = 512 };

typedef struct NativeFighterDraft {
    char path[NATIVE_FIGHTER_FAMILY_PROJECT_PATH_BYTES];
    char title[64];
} NativeFighterDraft;

bool nativeFighterFamilyProject_LocalRoot(char* path, size_t capacity);
// NULL drafts and zero capacity query the full count without loading assets.
bool nativeFighterFamilyProject_ListLocal(NativeFighterDraft* drafts,
                                         size_t capacity, size_t* count);

typedef struct NativeFighterFamilyProject {
    NativeFighterFamilyMode mode;
    unsigned member_count;
    NativeFighterProject members[NATIVE_FIGHTER_FAMILY_MEMBERS];
    char keys[NATIVE_FIGHTER_FAMILY_MEMBERS][NATIVE_FIGHTER_CODE_KEY_BYTES];
    char paths[NATIVE_FIGHTER_FAMILY_MEMBERS][NATIVE_FIGHTER_FAMILY_PROJECT_PATH_BYTES];
} NativeFighterFamilyProject;

typedef struct NativeFighterFamilyProjectInfo {
    NativeFighterFamilyMode mode;
    unsigned member_count;
    NativeFighterProjectInfo members[NATIVE_FIGHTER_FAMILY_MEMBERS];
    char keys[NATIVE_FIGHTER_FAMILY_MEMBERS][NATIVE_FIGHTER_CODE_KEY_BYTES];
    char paths[NATIVE_FIGHTER_FAMILY_MEMBERS][NATIVE_FIGHTER_FAMILY_PROJECT_PATH_BYTES];
} NativeFighterFamilyProjectInfo;

// Root files are member0; member1 occupies the fixed partner subtree. family.txt
// owns stable keys/mode, not mutable player roles. Missing means single/main.
// creation.pending keeps interrupted construction unlisted and unopened.
bool nativeFighterFamilyProject_ReadInfo(const char* path,
                                        NativeFighterFamilyProjectInfo* info);
bool nativeFighterFamilyProject_Open(const char* path,
                                    NativeFighterFamilyProject* project);
void nativeFighterFamilyProject_Close(NativeFighterFamilyProject* project);

// Prepare every independent source member at a stable candidate address before
// publishing one canonical packet. Failure preserves all caller outputs.
bool nativeFighterFamilyProject_Build(const NativeFighterFamilyProject*,
    const void* portrait_png, size_t portrait_size, uint8_t** package,
    size_t* package_size, NativeContentId* id);

// Caller chooses member order and mode. Construction copies assets/CPU only;
// neither function compiles templates, registers fighters, or installs content.
// Failed staging trees remain marked creation.pending for explicit recovery.
bool nativeFighterFamilyProject_CreateStockFromDisc(
    const char* path, const char* title, NativeFighterFamilyMode mode,
    FighterKind primary, FighterKind partner);
bool nativeFighterFamilyProject_CreateFromPackage(
    const char* path, const char* title, const void* package, size_t package_size);

#ifdef __cplusplus
}
#endif
#endif
#endif
