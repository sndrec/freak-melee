#ifndef MELEE_FT_NATIVE_SCRIPT_HITBOX_METADATA_H
#define MELEE_FT_NATIVE_SCRIPT_HITBOX_METADATA_H

#include "native_script_source.h"
struct NativeScriptCompileBank;

// Load-time immutable, semantic capsule data. No packed commands, pointers,
// actor state or tick allocation. Bone is a bounded raw owned/reserved part.
typedef struct NativeScriptHitboxDefinition {
    NativeScriptSourceSpan name,source;
    uint32_t node;
    int32_t bone,damage,angle,growth,weight_knockback,base_knockback,group;
    int32_t element,shield_damage,sound_severity,sound_kind;
    float radius,offset_x,offset_y,offset_z;
    bool item_interaction,ignore_fighter_scale,clank,rebound;
    bool grounded,aerial,victim_only,require_throw_owner;
} NativeScriptHitboxDefinition;

bool nativeScriptHitboxMetadata_Owns(const NativeScriptSourceTree*,uint32_t);
bool nativeScriptHitboxMetadata_Prepare(struct NativeScriptCompileBank*,NativeScriptDiagnostic*);
bool nativeScriptHitboxMetadata_Validate(const struct NativeScriptCompileBank*,
    const NativeScriptSourceTree*,uint32_t,NativeScriptDiagnostic*);
bool nativeScriptHitboxMetadata_CheckSymbols(const struct NativeScriptCompileBank*,NativeScriptDiagnostic*);
int nativeScriptHitboxMetadata_TryResolve(const struct NativeScriptCompileBank*,uint32_t context,
    uint32_t owner,const NativeScriptIRArena*,const NativeScriptIRNode*,
    NativeScriptIRResolution*,NativeScriptDiagnostic*);
#endif
