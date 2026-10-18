#ifndef MELEE_FT_NATIVE_SOURCE_PACKAGE_H
#define MELEE_FT_NATIVE_SOURCE_PACKAGE_H
#include "native_source_assets.h"
#include "native_source_bank.h"
#include "native_source_parameters.h"
#include "native_script_fighter.h"
#include "native_source_resources.h"
#include "native_source_entity_assets.h"
#include <melee/it/native_source_item_visual.h>

enum {NATIVE_SOURCE_DAT_BYTES=32*1024*1024};
typedef enum NativeSourcePackageFamily {NativeSourcePackage_Single=0} NativeSourcePackageFamily;
typedef struct NativeSourcePackageInput {
    NativeSourcePackageFamily family;
    const void* facts;size_t facts_bytes;
    NativeSourceAssetInput assets;
    const char* source;size_t source_bytes;
    const char* parameters;size_t parameter_bytes;
    const void* resources;size_t resource_bytes;
    NativeScriptCompileDiagnostics* diagnostics;
} NativeSourcePackageInput;
typedef struct NativeSourcePackage {
    const struct NativeSourcePackage* address;
    NativeSourcePackageInput owned_input;
    void* input_memory;size_t input_bytes;
    NativeSourceImportedAssets assets;
    NativeSourceBank compiler;
    NativeSourceParameters parameters;
    NativeSourceResources resources;
    NativeSourceItemVisuals visuals;
    NativeSourceEntityAssets entities;
    NativeScriptItemMetadataContext metadata;
    NativeScriptFighterProgram program;
    NativeSourceBankStats stats;
    FighterKind target;
    uint32_t leases;
    bool ready;
} NativeSourcePackage;
// Output must be zero-initialized/vacant at its final stable address. Accepted
// is read-only and separate; failed preparation leaves it unchanged and returns
// the attempted candidate to vacant. No global program/assets are published.
bool nativeSourcePackage_Prepare(NativeSourcePackage*,const NativeSourcePackageInput*,FighterKind target,
    const NativeSourcePackage* accepted,NativeScriptDiagnostic*);
bool nativeSourcePackage_CanActivate(const NativeSourcePackage*);
// Explicit transition leases for future actual table/resource installations.
// They supplement actual published-bank and actor-view lifetime checks.
bool nativeSourcePackage_Pin(NativeSourcePackage*);
bool nativeSourcePackage_Unpin(NativeSourcePackage*);
bool nativeSourcePackage_CanRelease(const NativeSourcePackage*);
bool nativeSourcePackage_Release(NativeSourcePackage*);
#endif
