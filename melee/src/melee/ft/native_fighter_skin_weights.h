#ifndef MELEE_FT_NATIVE_FIGHTER_SKIN_WEIGHTS_H
#define MELEE_FT_NATIVE_FIGHTER_SKIN_WEIGHTS_H

#include <stdbool.h>
#include <stdint.h>

enum { NATIVE_SKIN_MAX_INFLUENCES = 16 };

typedef struct NativeSkinWeights {
    unsigned count;
    uint16_t joints[NATIVE_SKIN_MAX_INFLUENCES];
    float weights[NATIVE_SKIN_MAX_INFLUENCES];
} NativeSkinWeights;

#ifdef __cplusplus
extern "C" {
#endif

// Asset editing: merge duplicate joints, remove zero weights, sort by joint
// and normalize. Invalid input leaves the supplied weights unchanged.
bool nativeSkinWeights_Normalize(NativeSkinWeights* skin);
float nativeSkinWeights_Get(const NativeSkinWeights* skin, uint16_t joint);
bool nativeSkinWeights_Equal(const NativeSkinWeights* a,
                             const NativeSkinWeights* b);
// Assign one influence and distribute the remainder proportionally among
// the others. A sole influence remains fully weighted. Capacity failure is
// reported without discarding another bone or changing the original weights.
bool nativeSkinWeights_Set(NativeSkinWeights* skin, uint16_t joint, float weight);

#ifdef __cplusplus
}
#endif
#endif
