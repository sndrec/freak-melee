#include "native_rollback_regions.h"

#ifdef MELEE_NATIVE

#include "native_rollback_state.h"
#include "native_match_arena.h"
#include <sysdolphin/baselib/native_hsd_arena.h>
#include <sysdolphin/baselib/native_animation_pool.h>
#include <dolphin/gx/GXTexture.h>
#include <dolphin/os/OSAlloc.h>
#include <melee/lb/lbheap.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static void sort_regions(NativeRollbackPagedRegion* regions, uint32_t count)
{
    for (uint32_t i = 1; i < count; ++i) {
        NativeRollbackPagedRegion value = regions[i];
        uint32_t at = i;
        while (at > 0 &&
               (uintptr_t) regions[at - 1].memory > (uintptr_t) value.memory)
        {
            regions[at] = regions[at - 1];
            --at;
        }
        regions[at] = value;
    }
}

static uint32_t merge_regions(NativeRollbackPagedRegion* regions,
                              uint32_t count)
{
    uint32_t merged_count = 0;
    for (uint32_t i = 0; i < count; ++i) {
        uintptr_t start = (uintptr_t) regions[i].memory;
        uintptr_t end = start + regions[i].size;
        if (merged_count != 0) {
            NativeRollbackPagedRegion* previous = &regions[merged_count - 1];
            uintptr_t previous_start = (uintptr_t) previous->memory;
            uintptr_t previous_end = previous_start + previous->size;
            // Touching VirtualAlloc reservations can have different AllocationBase
            // values. VirtualProtect cannot span them; merge only overlaps.
            if (start < previous_end) {
                if (end > previous_end) {
                    previous->size = end - previous_start;
                }
                continue;
            }
        }
        regions[merged_count++] = regions[i];
    }
    return merged_count;
}

bool nativeRollbackRegions_Build(NativeRollbackRegionSet* result)
{
    if (result == NULL) {
        return false;
    }
    *result = (NativeRollbackRegionSet){ 0 };

    LbHeapMemoryRegion heaps[NATIVE_ROLLBACK_HEAP_REGION_CAPACITY];
    size_t heap_count = lbHeap_GetActiveMainMemoryRegions(
        heaps, NATIVE_ROLLBACK_HEAP_REGION_CAPACITY);
    if (heap_count == 0 || heap_count > NATIVE_ROLLBACK_HEAP_REGION_CAPACITY) {
        return false;
    }

    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    uintptr_t page_mask = (uintptr_t) system_info.dwPageSize - 1;
    for (size_t i = 0; i < heap_count; ++i) {
        uintptr_t start = (uintptr_t) heaps[i].memory & ~page_mask;
        uintptr_t end =
            ((uintptr_t) heaps[i].memory + heaps[i].size + page_mask) &
            ~page_mask;
        result->regions[i] = (NativeRollbackPagedRegion){
            .memory = (void*) start,
            .size = end - start,
        };
    }

    NativeRollbackStateRegion global;
    if (nativeRollbackState_GetGlobalRegions(&global, 1) != 1) {
        return false;
    }
    uintptr_t global_start = (uintptr_t) global.memory & ~page_mask;
    uintptr_t global_end =
        ((uintptr_t) global.memory + global.size + page_mask) & ~page_mask;
    result->globals = result->regions[heap_count] =
        (NativeRollbackPagedRegion){
            .memory = (void*) global_start,
            .size = global_end - global_start,
        };

    for (size_t i = 0; i < heap_count; ++i) {
        if (heaps[i].allocator_heap < 0) {
            continue;
        }
        if (result->exact_count >= NATIVE_ROLLBACK_EXACT_REGION_CAPACITY) {
            return false;
        }
        OSAllocStateRegion allocator_region;
        if (!OSGetHeapStateRegion(heaps[i].allocator_heap,
                                  &allocator_region) ||
            allocator_region.memory == NULL || allocator_region.size == 0)
        {
            return false;
        }
        result->exact_regions[result->exact_count++] =
            (NativeRollbackExactRegion){
                .memory = allocator_region.memory,
                .size = allocator_region.size,
            };
        result->tracked_bytes += allocator_region.size;
    }
    OSAllocStateRegion current_heap_region;
    if (result->exact_count >= NATIVE_ROLLBACK_EXACT_REGION_CAPACITY ||
        !OSGetCurrentHeapStateRegion(&current_heap_region) ||
        current_heap_region.memory == NULL || current_heap_region.size == 0)
    {
        return false;
    }
    result->exact_regions[result->exact_count++] = (NativeRollbackExactRegion){
        .memory = current_heap_region.memory,
        .size = current_heap_region.size,
    };
    result->tracked_bytes += current_heap_region.size;

    GXTextureStateRegion texture_regions[2];
    uint32_t texture_count = GXGetTextureStateRegions(texture_regions, 2);
    if (texture_count != 2 || result->exact_count + texture_count >
                                  NATIVE_ROLLBACK_EXACT_REGION_CAPACITY)
    {
        return false;
    }
    for (uint32_t i = 0; i < texture_count; ++i) {
        if (texture_regions[i].memory == NULL || texture_regions[i].size == 0)
        {
            return false;
        }
        result->exact_regions[result->exact_count++] =
            (NativeRollbackExactRegion){
                .memory = texture_regions[i].memory,
                .size = texture_regions[i].size,
            };
        result->tracked_bytes += texture_regions[i].size;
    }

    size_t identity_count = nativeAssetIdentity_GetRegions(
        result->identity_regions, NATIVE_ASSET_IDENTITY_REGION_CAPACITY);
    if (identity_count > NATIVE_ASSET_IDENTITY_REGION_CAPACITY) {
        return false;
    }
    result->identity_count = (uint32_t) identity_count;

    uint32_t unmerged_count = (uint32_t) heap_count + 1;
    // Until all owners migrate, retain the broad oracle over both old heaps and
    // the new live arena. Include capacity so post-start allocations are covered.
    if(nativeMatchSceneArena.lifetime){
        uintptr_t start=(uintptr_t)nativeMatchSceneArena.memory & ~page_mask;
        uintptr_t end=((uintptr_t)nativeMatchSceneArena.memory+nativeMatchSceneArena.capacity+page_mask)&~page_mask;
        result->regions[unmerged_count++]=(NativeRollbackPagedRegion){(void*)start,end-start};
    }
    if(nativeMatchSceneArena.lifetime && nativeMatchActorStorage.memory){
        uintptr_t start=(uintptr_t)nativeMatchActorStorage.memory & ~page_mask;
        uintptr_t end=((uintptr_t)nativeMatchActorStorage.memory+nativeMatchActorStorage.bytes+page_mask)&~page_mask;
        result->regions[unmerged_count++]=(NativeRollbackPagedRegion){(void*)start,end-start};
    }
    if(nativeHsdArena.lifetime){
        uintptr_t start=(uintptr_t)nativeHsdArena.memory & ~page_mask;
        uintptr_t end=((uintptr_t)nativeHsdArena.memory+nativeHsdArena.capacity+page_mask)&~page_mask;
        result->regions[unmerged_count++]=(NativeRollbackPagedRegion){(void*)start,end-start};
    }
    if(nativeAnimationGlobals.heap_epoch && nativeAnimationStorage.memory){
        uintptr_t start=(uintptr_t)nativeAnimationStorage.memory & ~page_mask;
        uintptr_t end=((uintptr_t)nativeAnimationStorage.memory+nativeAnimationStorage.bytes+page_mask)&~page_mask;
        result->regions[unmerged_count++]=(NativeRollbackPagedRegion){(void*)start,end-start};
    }
    sort_regions(result->regions, unmerged_count);
    result->count = merge_regions(result->regions, unmerged_count);
    for (uint32_t i = 0; i < result->count; ++i) {
        result->tracked_bytes += result->regions[i].size;
    }
    return true;
}

uint64_t nativeRollbackRegions_Hash(const NativeRollbackRegionSet* regions)
{
    uint64_t hash = UINT64_C(14695981039346656037);
    for (uint32_t region = 0; region < regions->count; ++region) {
        const uint8_t* bytes = regions->regions[region].memory;
        for (size_t i = 0; i < regions->regions[region].size; ++i) {
            hash ^= bytes[i];
            hash *= UINT64_C(1099511628211);
        }
    }
    for (uint32_t region = 0; region < regions->exact_count; ++region) {
        const uint8_t* bytes = regions->exact_regions[region].memory;
        for (size_t i = 0; i < regions->exact_regions[region].size; ++i) {
            hash ^= bytes[i];
            hash *= UINT64_C(1099511628211);
        }
    }
    return hash;
}

#endif
