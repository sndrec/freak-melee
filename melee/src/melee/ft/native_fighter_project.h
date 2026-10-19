#ifndef MELEE_FT_NATIVE_FIGHTER_PROJECT_H
#define MELEE_FT_NATIVE_FIGHTER_PROJECT_H

#ifdef MELEE_NATIVE

#include <melee/ft/forward.h>
#include <melee/ft/native_fighter_cpu.h>
#include <melee/ft/native_source_asset_facts.h>
#include <melee/gm/native_content_store.h>
#include "native_fighter_attributes.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NativeFighterProject {
    FighterKind base_kind;
    char fighter_root[NATIVE_SOURCE_NAME_BYTES];
    unsigned action_count;
    unsigned demo_count;
    char title[64];
    char template_key[NATIVE_FIGHTER_CODE_KEY_BYTES]; // Initial authoring hint only.
    NativeSourceAssetFacts facts;
    uint8_t* fighter_dat;
    size_t fighter_size;
    uint8_t* animation_dat;
    size_t animation_size;
    uint8_t* costume_dat;
    size_t costume_size;
    uint8_t* asset_record;
    size_t asset_record_size;
    uint8_t* resources;
    size_t resource_size;
    uint8_t* script_text;
    size_t script_size;
    uint8_t* parameter_text;
    size_t parameter_size;
    uint8_t* cpu_text;
    size_t cpu_size;
    struct NativeFighterAuthoring* authoring;
    // In-memory editor history tokens. Serial stays monotonic through Undo;
    // revision identifies the current skeleton layout. Neither is serialized.
    uint64_t rig_edit_revision, rig_edit_serial;
} NativeFighterProject;

typedef struct NativeFighterProjectInfo {
    FighterKind base_kind;
    char fighter_root[NATIVE_SOURCE_NAME_BYTES];
    unsigned action_count;
    unsigned demo_count;
    char title[64];
    char template_key[NATIVE_FIGHTER_CODE_KEY_BYTES];
    NativeSourceAssetFacts facts;
} NativeFighterProjectInfo;



const char* nativeFighterProject_StockBaseName(FighterKind source_kind);

// Creates an independent, editable draft from a supported stock fighter.
// The destination must not already exist. A completed manifest marks a draft
// ready to open; interrupted creations remain unlisted until repaired.
bool nativeFighterProject_CreateStock(const char* path, const char* title,
                                      FighterKind source_kind,
                                      const void* fighter, size_t fighter_size,
                                      const void* animation,
                                      size_t animation_size,
                                      const void* costume, size_t costume_size,
                                      const NativeFighterCpuProfile* cpu);
// Explicit one-time import for an existing draft; never used during a match.
bool nativeFighterProject_ImportCpuFromDisc(const char* path, FighterKind kind);
// Copies the original stock archives from the currently open game disc.
// The caller owns disc initialization and the destination directory must not exist.
bool nativeFighterProject_CreateStockFromDisc(const char* path,
                                              const char* title,
                                              FighterKind source_kind);
// Copies one validated package member into its editable project directory.
// The family importer owns package parsing and shared portrait lifetime.
// Member assets and code are copied; the completed manifest is written last.
bool nativeFighterProject_CreateFromMember(const char* path, const char* title,
    const NativeFighterPackageMember* member, const NativeFighterMemberData* data,
    const void* portrait_png, size_t portrait_size);
// Reads only the manifest and asset sizes for project-library listing.
bool nativeFighterProject_ReadInfo(const char* path,
                                   NativeFighterProjectInfo* info);
// Rename the primary project manifest. Family titles come from their primary member.
bool nativeFighterProject_Rename(const char* path, const char* title);
bool nativeFighterProject_Open(const char* path, NativeFighterProject* project);
bool nativeFighterProject_GetAttribute(const NativeFighterProject* project,
                                       size_t property, double* value);
bool nativeFighterProject_SetAttribute(NativeFighterProject* project,
                                       size_t property, double value);
// Save fighter DAT edits atomically and retain the immediately previous file.
bool nativeFighterProject_Save(const char* path,
                               const NativeFighterProject* project);
// Save the matching fighter, animation, costume, rig facts and editor metadata together.
// Validates the complete candidate before staging files. A failed replacement
// restores the previous files; a successful save retains one complete asset
// predecessor. This is operation-level rollback, not power-loss recovery.
bool nativeFighterProject_SaveAssets(const char* path,
    const NativeFighterProject* project, char* error, size_t error_size);
// Admit a complete in-memory asset set before an authoring transaction is
// published. Checks the canonical facts record and imports the actual graphs.
bool nativeFighterProject_ValidateAssets(const NativeFighterProject*,
    char* error, size_t error_size);
bool nativeFighterProject_UndoLastAssetSave(const char* path,
    char* error, size_t error_size);
// Swap the saved fighter DAT with the previous saved version. Repeating this
// operation toggles between the two states; current animation addresses are
// retained and other projects are never affected.
bool nativeFighterProject_UndoLastSave(const char* path);
// Save the animation bank with matching fighter descriptor addresses; retain
// an independent predecessor pair. Animation undo restores addresses only,
// preserving unrelated attributes, commands and playback flags.
bool nativeFighterProject_SaveAnimation(const char* path,
                                        const NativeFighterProject* project);
bool nativeFighterProject_UndoLastAnimationSave(const char* path);
// Costume/model edits have their own saved predecessor, independent of move data.
bool nativeFighterProject_SaveCostume(const char* path,
                                     const NativeFighterProject* project);
bool nativeFighterProject_UndoLastCostumeSave(const char* path);

// Install a cached build from this draft, replacing its previous installed
// build and preserving its CSS tile. Old package bytes remain cached.
bool nativeFighterProject_Install(const char* path, const char* content_root,
                                  const NativeContentId* id,
                                  NativeContentId* replaced);
void nativeFighterProject_Close(NativeFighterProject* project);

#ifdef __cplusplus
}
#endif

#endif
#endif
