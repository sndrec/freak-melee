#include "native_rollback_census.h"

#ifdef MELEE_NATIVE

#include "native_rollback_pages.h"
#include "native_rollback_regions.h"
#include <dolphin/os/OSAlloc.h>
#include <melee/lb/lbheap.h>
#include <sysdolphin/baselib/debug.h>

#include <stdint.h>

enum {
    CENSUS_DIRTY_PAGE_CAPACITY = 8192,
};

typedef struct NativeRollbackCensusPhase {
    uint64_t frames;
    uint64_t dirty_pages;
    uint64_t global_dirty_pages;
    uint32_t max_dirty_pages;
    uint32_t max_global_dirty_pages;
    uint32_t overflows;
} NativeRollbackCensusPhase;

typedef struct NativeRollbackAllocationStats {
    uint32_t count;
    size_t payload_bytes;
} NativeRollbackAllocationStats;

static struct NativeRollbackCensus {
    bool active;
    bool frame_active;
    uint64_t sequence;
    uint32_t region_count;
    NativeRollbackPagedRegion regions[NATIVE_ROLLBACK_REGION_CAPACITY];
    NativeRollbackPagedRegion globals;
    NativeRollbackAllocationStats allocation_stats;
    NativeRollbackCensusPhase simulation;
    NativeRollbackCensusPhase render;
} census;

typedef struct NativeRollbackDirtyClassification {
    uint32_t globals;
} NativeRollbackDirtyClassification;

static void classify_dirty_page(uint32_t region, uint32_t page, void* memory,
                                void* user_data)
{
    (void) region;
    (void) page;
    NativeRollbackDirtyClassification* classification = user_data;
    uintptr_t address = (uintptr_t) memory;
    uintptr_t global_start = (uintptr_t) census.globals.memory;
    if (address >= global_start && address - global_start < census.globals.size) {
        ++classification->globals;
    }
}

static bool allocation_in_tracked_region(void* memory, size_t size)
{
    uintptr_t start = (uintptr_t) memory;
    uintptr_t end = start + size;
    if (end < start) {
        return false;
    }
    for (uint32_t i = 0; i < census.region_count; ++i) {
        uintptr_t region_start = (uintptr_t) census.regions[i].memory;
        uintptr_t region_end = region_start + census.regions[i].size;
        if (start >= region_start && end <= region_end) {
            return true;
        }
    }
    return false;
}

static void visit_allocation(void* memory, unsigned long size)
{
    if (!allocation_in_tracked_region(memory, size)) {
        return;
    }
    ++census.allocation_stats.count;
    census.allocation_stats.payload_bytes += size;
}

static void report_allocations(const char* point)
{
    census.allocation_stats = (NativeRollbackAllocationStats){ 0 };
    OSVisitAllocated(visit_allocation);
    OSReport("[rollback-census] %s: %u OS allocations, %zu payload bytes "
             "inside tracked heaps.\n",
             point, census.allocation_stats.count,
             census.allocation_stats.payload_bytes);
}

bool nativeRollbackCensus_Init(void)
{
    census = (struct NativeRollbackCensus){ 0 };
    NativeRollbackRegionSet region_set;
    if (!nativeRollbackRegions_Build(&region_set)) {
        OSReport("[rollback-census] Could not locate rollback regions.\n");
        return false;
    }
    census.globals = region_set.globals;
    census.region_count = region_set.count;
    for (uint32_t i = 0; i < region_set.count; ++i) {
        census.regions[i] = region_set.regions[i];
    }

    for (uint32_t i = 0; i < region_set.count; ++i) {
        OSReport("[rollback-census] region %u: %p + %zu bytes\n", i,
                 region_set.regions[i].memory, region_set.regions[i].size);
    }
    if (!nativeRollbackPages_Init(region_set.regions, region_set.count,
                                  region_set.exact_regions,
                                  region_set.exact_count, 1,
                                  CENSUS_DIRTY_PAGE_CAPACITY))
    {
        OSReport("[rollback-census] Could not initialize page tracker.\n");
        return false;
    }
    census.active = true;
    OSReport("[rollback-census] Tracking %zu game-state bytes.\n",
             region_set.tracked_bytes);
    report_allocations("match-loop entry");
    return true;
}

static void begin_phase(void)
{
    if (!census.active || census.frame_active) {
        return;
    }
    census.frame_active = nativeRollbackPages_BeginFrame(census.sequence++);
}

static void end_phase(NativeRollbackCensusPhase* phase)
{
    if (!census.frame_active) {
        return;
    }
    uint32_t dirty = nativeRollbackPages_DirtyPageCount();
    uint64_t frame = census.sequence - 1;
    bool complete = nativeRollbackPages_EndFrame();
    census.frame_active = false;
    NativeRollbackDirtyClassification classification = { 0 };
    if (complete) {
        complete = nativeRollbackPages_VisitFrame(
            frame, classify_dirty_page, &classification);
    }
    ++phase->frames;
    phase->dirty_pages += dirty;
    phase->global_dirty_pages += classification.globals;
    if (dirty > phase->max_dirty_pages) {
        phase->max_dirty_pages = dirty;
    }
    if (classification.globals > phase->max_global_dirty_pages) {
        phase->max_global_dirty_pages = classification.globals;
    }
    if (!complete) {
        ++phase->overflows;
    }
}

void nativeRollbackCensus_BeginSimulation(void)
{
    begin_phase();
}

void nativeRollbackCensus_EndSimulation(void)
{
    end_phase(&census.simulation);
}

void nativeRollbackCensus_BeginRender(void)
{
    begin_phase();
}

void nativeRollbackCensus_EndRender(void)
{
    end_phase(&census.render);
}

static void report_phase(const char* name,
                         const NativeRollbackCensusPhase* phase)
{
    double average =
        phase->frames == 0 ? 0.0 : (double) phase->dirty_pages / phase->frames;
    double global_average =
        phase->frames == 0
            ? 0.0
            : (double) phase->global_dirty_pages / phase->frames;
    OSReport("[rollback-census] %s: %llu frames, %.2f average dirty pages, "
             "%u maximum; globals %.2f average, %u maximum; %u "
             "overflowed.\n",
             name, (unsigned long long) phase->frames, average,
             phase->max_dirty_pages, global_average,
             phase->max_global_dirty_pages, phase->overflows);
}

void nativeRollbackCensus_Shutdown(void)
{
    if (!census.active) {
        return;
    }
    if (census.frame_active) {
        nativeRollbackPages_EndFrame();
    }
    report_allocations("match-loop exit");
    nativeRollbackPages_Shutdown();
    report_phase("simulation", &census.simulation);
    report_phase("render", &census.render);
    census = (struct NativeRollbackCensus){ 0 };
}

#endif
