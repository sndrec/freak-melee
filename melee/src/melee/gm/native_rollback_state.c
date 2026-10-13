#include "native_rollback_state.h"

#ifdef MELEE_NATIVE

#include <stdint.h>

#pragma section(".mstate$a", read, write)
#pragma section(".mstate$z", read, write)

__declspec(allocate(".mstate$a")) static uint8_t rollback_state_begin;
__declspec(allocate(".mstate$z")) static uint8_t rollback_state_end;

size_t nativeRollbackState_GetGlobalRegions(NativeRollbackStateRegion* regions,
                                             size_t capacity)
{
    uintptr_t begin = (uintptr_t) &rollback_state_begin + 1;
    uintptr_t end = (uintptr_t) &rollback_state_end;
    if (end <= begin) {
        return 0;
    }
    if (capacity != 0 && regions != NULL) {
        regions[0] = (NativeRollbackStateRegion){
            .memory = (void*) begin,
            .size = end - begin,
        };
    }
    return 1;
}

#endif
