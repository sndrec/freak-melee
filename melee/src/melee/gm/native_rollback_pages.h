#ifndef MELEE_GM_NATIVE_ROLLBACK_PAGES_H
#define MELEE_GM_NATIVE_ROLLBACK_PAGES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef MELEE_NATIVE

typedef struct NativeRollbackPagedRegion {
    void* memory;
    size_t size;
} NativeRollbackPagedRegion;

typedef struct NativeRollbackExactRegion {
    void* memory;
    size_t size;
} NativeRollbackExactRegion;

typedef void (*NativeRollbackPageVisitor)(uint32_t region, uint32_t page,
                                          void* memory, void* user_data);

bool nativeRollbackPages_Init(const NativeRollbackPagedRegion* regions,
                              uint32_t region_count,
                              const NativeRollbackExactRegion* exact_regions,
                              uint32_t exact_region_count,
                              uint32_t frame_capacity,
                              uint32_t dirty_page_capacity);
void nativeRollbackPages_Shutdown(void);
// Retained completed frames pin their referenced scene/package addresses too.
bool nativeRollbackPages_IsInitialized(void);

// Each completed frame stores the original contents of pages first written
// during that frame. ToFrame restores the state at the start of `frame` from
// the state at the start of `current_frame` and discards the applied history.
bool nativeRollbackPages_BeginFrame(uint64_t frame);
bool nativeRollbackPages_EndFrame(void);
bool nativeRollbackPages_ToFrame(uint64_t frame, uint64_t current_frame);
bool nativeRollbackPages_VisitFrame(uint64_t frame,
                                    NativeRollbackPageVisitor visitor,
                                    void* user_data);

uint32_t nativeRollbackPages_DirtyPageCount(void);
size_t nativeRollbackPages_PageSize(void);

#endif

#endif
