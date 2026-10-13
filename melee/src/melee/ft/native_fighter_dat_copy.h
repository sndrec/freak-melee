#ifndef MELEE_FT_NATIVE_FIGHTER_DAT_COPY_H
#define MELEE_FT_NATIVE_FIGHTER_DAT_COPY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum NativeFighterDatCopyResult {
    NativeFighterDatCopy_Ok,
    NativeFighterDatCopy_Input,
    NativeFighterDatCopy_References,
    NativeFighterDatCopy_Slot,
    NativeFighterDatCopy_Capacity,
    NativeFighterDatCopy_Overlap,
    NativeFighterDatCopy_Allocation,
    NativeFighterDatCopy_Output,
} NativeFighterDatCopyResult;

typedef struct NativeFighterDatCopyStats {
    NativeFighterDatCopyResult result;
    uint32_t source_base;
    uint32_t copied_data_bytes;
    uint32_t relocation_count;
    uint32_t external_count;
    uint32_t merged_external_count;
    size_t output_bytes;
} NativeFighterDatCopyStats;

// Construction only. Both serialized slots must already resolve LOCAL.
// Output NULL performs validated sizing. A failed call never changes output.
// Output storage must not overlap either immutable input. required_size/stats
// are independent writable records, not part of any input/output byte buffer.
bool nativeFighterDatCopy_PointerField(
    const void* destination, size_t destination_size, uint32_t destination_slot,
    const void* source, size_t source_size, uint32_t source_slot,
    void* output, size_t output_capacity, size_t* required_size,
    NativeFighterDatCopyStats* stats);

#ifdef __cplusplus
}
#endif

#endif
