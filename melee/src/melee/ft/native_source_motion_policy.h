#ifndef MELEE_FT_NATIVE_SOURCE_MOTION_POLICY_H
#define MELEE_FT_NATIVE_SOURCE_MOTION_POLICY_H
#include "native_source_assets.h"
#include "native_script_source.h"

bool nativeSourceMotionPolicy_Owns(const NativeScriptSourceTree*,uint32_t);
bool nativeSourceMotionPolicy_Prepare(const NativeScriptSourceTree*,
    const MotionState* common,NativeSourceImportedAssets*,NativeScriptDiagnostic*);
bool nativeSourceMotionPolicy_Validate(const NativeScriptSourceTree*,uint32_t,
    const NativeSourceImportedAssets*,NativeScriptDiagnostic*);
#endif
