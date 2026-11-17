#ifndef NATIVE_FIGHTER_MATERIAL_ANIMATION_H
#define NATIVE_FIGHTER_MATERIAL_ANIMATION_H
#include "native_fighter_model_editor.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct NativeMaterialTextureSequence {
    uint32_t offset;
    unsigned animation_id, frames;
} NativeMaterialTextureSequence;
unsigned nativeFighterMaterial_Sequences(const NativeFighterModelEditor*,
                                         unsigned material,
                                         NativeMaterialTextureSequence*,
                                         unsigned capacity);
#ifdef __cplusplus
}
#endif
#endif
