#ifndef MELEE_EF_NATIVE_MODEL_EFFECT_H
#define MELEE_EF_NATIVE_MODEL_EFFECT_H

#include <melee/ft/native_fighter_runtime.h>
#include <melee/ef/forward.h>
#include <dolphin/types.h>
#include <stdint.h>
#include <stdbool.h>

// Registry lookups compare retained addresses before reading their contents.
EF_Effect* nativeModelEffect_Resolve(HSD_GObj*, uint32_t generation);
bool nativeModelEffect_Snapshot(HSD_GObj*, uint32_t* generation);
Fighter_GObj* nativeModelEffect_Creator(EF_Effect*);
bool nativeModelEffect_Validate(const NativeFighterDefinition*);
void nativeModelEffect_Spawn(Fighter_GObj*, const NativeFighterModelEffect*, s32 part,
                             Vec3* position, bool deferred);
HSD_JObj* nativeModelEffect_Joint(EF_Effect*, unsigned member);
void nativeModelEffect_Remove(EF_Effect*);
void nativeModelEffect_Cancel(EF_Effect*);
void nativeModelEffect_CancelGeneration(HSD_GObj*, uint32_t generation);
void nativeModelEffect_Clear(void);

#endif
