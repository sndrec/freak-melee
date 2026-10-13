#ifndef MELEE_GM_NATIVE_MATCH_OBJECT_CENSUS_H
#define MELEE_GM_NATIVE_MATCH_OBJECT_CENSUS_H
#include <stdbool.h>
#include <stdint.h>

typedef struct NativeMatchObjectCensusStats {
    uint32_t scene, object_peak, proc_peak, linked_object_peak, linked_proc_peak;
    uint32_t allocator_types, class_types, classifier_types, errors;
    uint64_t samples, replay_samples, payload_peak, allocator_payload_peak, class_payload_peak;
} NativeMatchObjectCensusStats;

// Read-only host diagnostic, fixed storage and no allocation during Sample.
// Begin does not inspect old scene registries, which preload may invalidate.
void nativeMatchObjectCensus_BeginScene(uint32_t scene);
void nativeMatchObjectCensus_Sample(uint64_t frame, uint64_t input_sample, bool replaying);
void nativeMatchObjectCensus_FinishScene(void);
void nativeMatchObjectCensus_Close(void);
void nativeMatchObjectCensus_Abort(const char* reason);
const NativeMatchObjectCensusStats* nativeMatchObjectCensus_Stats(void);
bool nativeMatchObjectCensus_HasFailed(void);
#endif
