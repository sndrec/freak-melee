#include "native_rollback_pages.h"

#ifdef MELEE_NATIVE

#define WIN32_LEAN_AND_MEAN
#include <limits.h>
#include <string.h>
#include <windows.h>

typedef struct NativeRollbackPage {
    uint32_t region;
    uint32_t page;
    uint8_t bytes[];
} NativeRollbackPage;

typedef struct NativeRollbackFrame {
    uint64_t frame;
    uint32_t dirty_count;
    bool valid;
    bool overflow;
} NativeRollbackFrame;

typedef struct NativeRollbackTrackedRegion {
    uint8_t* memory;
    size_t size;
    uint32_t page_count;
    uint32_t* page_generation;
} NativeRollbackTrackedRegion;

typedef struct NativeRollbackTrackedExactRegion {
    uint8_t* memory;
    size_t size;
    size_t snapshot_offset;
} NativeRollbackTrackedExactRegion;

static struct NativeRollbackPageState {
    size_t page_size;
    uint32_t region_count;
    uint32_t frame_capacity;
    uint32_t dirty_page_capacity;
    size_t page_record_size;
    size_t exact_snapshot_size;
    NativeRollbackTrackedRegion* regions;
    NativeRollbackTrackedExactRegion* exact_regions;
    uint32_t exact_region_count;
    NativeRollbackFrame* frames;
    uint8_t* page_records;
    uint8_t* exact_snapshots;
    uint32_t* page_generations;
    uint32_t generation;
    NativeRollbackFrame* active_frame;
    uint8_t* active_records;
    void* exception_handler;
    DWORD owner_thread;
} rollback;

static NativeRollbackPage* page_record(uint8_t* records, uint32_t index)
{
    return (NativeRollbackPage*) (records + index * rollback.page_record_size);
}

static bool multiply_size(size_t first, size_t second, size_t* result)
{
    if (first != 0 && second > SIZE_MAX / first) {
        return false;
    }
    *result = first * second;
    return true;
}

static NativeRollbackTrackedRegion* find_region(uintptr_t address,
                                                uint32_t* region_index)
{
    for (uint32_t i = 0; i < rollback.region_count; ++i) {
        uintptr_t start = (uintptr_t) rollback.regions[i].memory;
        if (address >= start && address - start < rollback.regions[i].size) {
            *region_index = i;
            return &rollback.regions[i];
        }
    }
    return NULL;
}

static LONG CALLBACK rollback_exception(EXCEPTION_POINTERS* exception)
{
    EXCEPTION_RECORD* record = exception->ExceptionRecord;
    if (record->ExceptionCode != EXCEPTION_ACCESS_VIOLATION ||
        record->NumberParameters < 2 || record->ExceptionInformation[0] != 1 ||
        rollback.active_frame == NULL ||
        GetCurrentThreadId() != rollback.owner_thread)
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    uint32_t region_index;
    uintptr_t address = record->ExceptionInformation[1];
    NativeRollbackTrackedRegion* region = find_region(address, &region_index);
    if (region == NULL) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    uint32_t page = (uint32_t) ((address - (uintptr_t) region->memory) /
                                rollback.page_size);
    if (region->page_generation[page] != rollback.generation) {
        region->page_generation[page] = rollback.generation;
        uint32_t dirty = rollback.active_frame->dirty_count;
        if (dirty == rollback.dirty_page_capacity) {
            rollback.active_frame->overflow = true;
        } else {
            NativeRollbackPage* saved =
                page_record(rollback.active_records, dirty);
            saved->region = region_index;
            saved->page = page;
            memcpy(saved->bytes,
                   region->memory + (size_t) page * rollback.page_size,
                   rollback.page_size);
            rollback.active_frame->dirty_count = dirty + 1;
        }
    }

    DWORD old_protection;
    if (!VirtualProtect(region->memory + (size_t) page * rollback.page_size,
                        rollback.page_size, PAGE_READWRITE, &old_protection))
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    return EXCEPTION_CONTINUE_EXECUTION;
}

static void make_regions_writable(void)
{
    for (uint32_t i = 0; i < rollback.region_count; ++i) {
        DWORD old_protection;
        VirtualProtect(rollback.regions[i].memory, rollback.regions[i].size,
                       PAGE_READWRITE, &old_protection);
    }
}

static void release_storage(void)
{
    if (rollback.regions != NULL) {
        VirtualFree(rollback.regions, 0, MEM_RELEASE);
    }
    if (rollback.frames != NULL) {
        VirtualFree(rollback.frames, 0, MEM_RELEASE);
    }
    if (rollback.page_records != NULL) {
        VirtualFree(rollback.page_records, 0, MEM_RELEASE);
    }
    if (rollback.exact_regions != NULL) {
        VirtualFree(rollback.exact_regions, 0, MEM_RELEASE);
    }
    if (rollback.exact_snapshots != NULL) {
        VirtualFree(rollback.exact_snapshots, 0, MEM_RELEASE);
    }
    if (rollback.page_generations != NULL) {
        VirtualFree(rollback.page_generations, 0, MEM_RELEASE);
    }
}

bool nativeRollbackPages_Init(const NativeRollbackPagedRegion* regions,
                              uint32_t region_count,
                              const NativeRollbackExactRegion* exact_regions,
                              uint32_t exact_region_count,
                              uint32_t frame_capacity,
                              uint32_t dirty_page_capacity)
{
    nativeRollbackPages_Shutdown();
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    size_t page_size = system_info.dwPageSize;
    if (regions == NULL || region_count == 0 || frame_capacity == 0 ||
        dirty_page_capacity == 0 ||
        (exact_region_count != 0 && exact_regions == NULL))
    {
        return false;
    }

    size_t exact_snapshot_size = 0;
    for (uint32_t i = 0; i < exact_region_count; ++i) {
        if (exact_regions[i].memory == NULL || exact_regions[i].size == 0 ||
            exact_regions[i].size > SIZE_MAX - exact_snapshot_size)
        {
            return false;
        }
        exact_snapshot_size += exact_regions[i].size;
    }

    uint32_t total_pages = 0;
    for (uint32_t i = 0; i < region_count; ++i) {
        uintptr_t start = (uintptr_t) regions[i].memory;
        if (regions[i].memory == NULL || regions[i].size == 0 ||
            start % page_size != 0 || regions[i].size % page_size != 0 ||
            regions[i].size / page_size > UINT32_MAX - total_pages)
        {
            return false;
        }
        uintptr_t end = start + regions[i].size;
        if (end < start) {
            return false;
        }
        for (uint32_t j = 0; j < i; ++j) {
            uintptr_t other_start = (uintptr_t) regions[j].memory;
            uintptr_t other_end = other_start + regions[j].size;
            if (start < other_end && other_start < end) {
                return false;
            }
        }
        total_pages += (uint32_t) (regions[i].size / page_size);
    }

    size_t page_record_size = sizeof(uint32_t) * 2 + page_size;
    size_t records_per_frame;
    size_t record_bytes;
    size_t region_bytes;
    size_t frame_bytes;
    size_t generation_bytes;
    size_t exact_region_bytes;
    size_t exact_snapshot_bytes;
    if (!multiply_size(dirty_page_capacity, page_record_size,
                       &records_per_frame) ||
        !multiply_size(frame_capacity, records_per_frame, &record_bytes) ||
        !multiply_size(region_count, sizeof(*rollback.regions),
                       &region_bytes) ||
        !multiply_size(frame_capacity, sizeof(*rollback.frames),
                       &frame_bytes) ||
        !multiply_size(total_pages, sizeof(*rollback.page_generations),
                       &generation_bytes) ||
        !multiply_size(exact_region_count, sizeof(*rollback.exact_regions),
                       &exact_region_bytes) ||
        !multiply_size(frame_capacity, exact_snapshot_size,
                       &exact_snapshot_bytes))
    {
        return false;
    }

    rollback.regions = VirtualAlloc(NULL, region_bytes,
                                    MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    rollback.frames = VirtualAlloc(NULL, frame_bytes, MEM_RESERVE | MEM_COMMIT,
                                   PAGE_READWRITE);
    rollback.page_records = VirtualAlloc(
        NULL, record_bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (exact_region_bytes != 0) {
        rollback.exact_regions = VirtualAlloc(
            NULL, exact_region_bytes, MEM_RESERVE | MEM_COMMIT,
            PAGE_READWRITE);
    }
    if (exact_snapshot_bytes != 0) {
        rollback.exact_snapshots = VirtualAlloc(
            NULL, exact_snapshot_bytes, MEM_RESERVE | MEM_COMMIT,
            PAGE_READWRITE);
    }
    rollback.page_generations = VirtualAlloc(
        NULL, generation_bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (rollback.regions == NULL || rollback.frames == NULL ||
        rollback.page_records == NULL || rollback.page_generations == NULL ||
        (exact_region_bytes != 0 && rollback.exact_regions == NULL) ||
        (exact_snapshot_bytes != 0 && rollback.exact_snapshots == NULL))
    {
        release_storage();
        memset(&rollback, 0, sizeof(rollback));
        return false;
    }

    uint32_t generation_offset = 0;
    for (uint32_t i = 0; i < region_count; ++i) {
        uint32_t page_count = (uint32_t) (regions[i].size / page_size);
        rollback.regions[i] = (NativeRollbackTrackedRegion){
            .memory = regions[i].memory,
            .size = regions[i].size,
            .page_count = page_count,
            .page_generation = rollback.page_generations + generation_offset,
        };
        generation_offset += page_count;
    }
    size_t exact_offset = 0;
    for (uint32_t i = 0; i < exact_region_count; ++i) {
        rollback.exact_regions[i] = (NativeRollbackTrackedExactRegion) {
            .memory = exact_regions[i].memory,
            .size = exact_regions[i].size,
            .snapshot_offset = exact_offset,
        };
        exact_offset += exact_regions[i].size;
    }
    rollback.page_size = page_size;
    rollback.region_count = region_count;
    rollback.exact_region_count = exact_region_count;
    rollback.exact_snapshot_size = exact_snapshot_size;
    rollback.frame_capacity = frame_capacity;
    rollback.dirty_page_capacity = dirty_page_capacity;
    rollback.page_record_size = page_record_size;
    rollback.owner_thread = GetCurrentThreadId();
    rollback.exception_handler =
        AddVectoredExceptionHandler(1, rollback_exception);
    if (rollback.exception_handler == NULL) {
        release_storage();
        memset(&rollback, 0, sizeof(rollback));
        return false;
    }
    return true;
}

bool nativeRollbackPages_IsInitialized(void)
{
    return rollback.frames != NULL || rollback.active_frame != NULL;
}

void nativeRollbackPages_Shutdown(void)
{
    make_regions_writable();
    if (rollback.exception_handler != NULL) {
        RemoveVectoredExceptionHandler(rollback.exception_handler);
    }
    release_storage();
    memset(&rollback, 0, sizeof(rollback));
}

bool nativeRollbackPages_BeginFrame(uint64_t frame)
{
    if (rollback.regions == NULL || rollback.active_frame != NULL ||
        GetCurrentThreadId() != rollback.owner_thread)
    {
        return false;
    }
    if (++rollback.generation == 0) {
        uint32_t total_pages = 0;
        for (uint32_t i = 0; i < rollback.region_count; ++i) {
            total_pages += rollback.regions[i].page_count;
        }
        memset(rollback.page_generations, 0,
               (size_t) total_pages * sizeof(*rollback.page_generations));
        rollback.generation = 1;
    }

    uint32_t slot = (uint32_t) (frame % rollback.frame_capacity);
    NativeRollbackFrame* saved = &rollback.frames[slot];
    *saved = (NativeRollbackFrame){ .frame = frame };
    rollback.active_frame = saved;
    rollback.active_records =
        rollback.page_records + (size_t) slot * rollback.dirty_page_capacity *
                                    rollback.page_record_size;
    if (rollback.exact_region_count != 0) {
        uint8_t* exact_snapshot = rollback.exact_snapshots +
                                  (size_t) slot * rollback.exact_snapshot_size;
        for (uint32_t i = 0; i < rollback.exact_region_count; ++i) {
            NativeRollbackTrackedExactRegion* region =
                &rollback.exact_regions[i];
            memcpy(exact_snapshot + region->snapshot_offset, region->memory,
                   region->size);
        }
    }

    for (uint32_t i = 0; i < rollback.region_count; ++i) {
        DWORD old_protection;
        if (!VirtualProtect(rollback.regions[i].memory,
                            rollback.regions[i].size, PAGE_READONLY,
                            &old_protection))
        {
            make_regions_writable();
            rollback.active_frame = NULL;
            rollback.active_records = NULL;
            return false;
        }
    }
    return true;
}

bool nativeRollbackPages_EndFrame(void)
{
    if (rollback.active_frame == NULL) {
        return false;
    }
    bool protected = true;
    for (uint32_t i = 0; i < rollback.region_count; ++i) {
        DWORD old_protection;
        if (!VirtualProtect(rollback.regions[i].memory,
                            rollback.regions[i].size, PAGE_READWRITE,
                            &old_protection))
        {
            protected = false;
        }
    }
    rollback.active_frame->valid =
        protected && !rollback.active_frame->overflow;
    bool valid = rollback.active_frame->valid;
    rollback.active_frame = NULL;
    rollback.active_records = NULL;
    return valid;
}

bool nativeRollbackPages_ToFrame(uint64_t frame, uint64_t current_frame)
{
    if (rollback.regions == NULL || rollback.active_frame != NULL ||
        frame > current_frame ||
        current_frame - frame > rollback.frame_capacity)
    {
        return false;
    }
    if (frame == current_frame) {
        return true;
    }

    for (uint64_t at = current_frame; at > frame; --at) {
        uint64_t saved_frame = at - 1;
        uint32_t slot = (uint32_t) (saved_frame % rollback.frame_capacity);
        NativeRollbackFrame* saved = &rollback.frames[slot];
        if (!saved->valid || saved->frame != saved_frame) {
            return false;
        }
    }

    for (uint64_t at = current_frame; at > frame; --at) {
        uint64_t saved_frame = at - 1;
        uint32_t slot = (uint32_t) (saved_frame % rollback.frame_capacity);
        NativeRollbackFrame* saved = &rollback.frames[slot];
        uint8_t* records = rollback.page_records +
                           (size_t) slot * rollback.dirty_page_capacity *
                               rollback.page_record_size;
        for (uint32_t i = 0; i < saved->dirty_count; ++i) {
            NativeRollbackPage* page = page_record(records, i);
            if (page->region >= rollback.region_count ||
                page->page >= rollback.regions[page->region].page_count)
            {
                return false;
            }
            NativeRollbackTrackedRegion* region =
                &rollback.regions[page->region];
            memcpy(region->memory + (size_t) page->page * rollback.page_size,
                   page->bytes, rollback.page_size);
        }
        saved->valid = false;
    }
    if (rollback.exact_region_count != 0) {
        uint32_t target_slot = (uint32_t) (frame % rollback.frame_capacity);
        uint8_t* exact_snapshot =
            rollback.exact_snapshots +
            (size_t) target_slot * rollback.exact_snapshot_size;
        for (uint32_t i = 0; i < rollback.exact_region_count; ++i) {
            NativeRollbackTrackedExactRegion* region =
                &rollback.exact_regions[i];
            memcpy(region->memory, exact_snapshot + region->snapshot_offset,
                   region->size);
        }
    }
    return true;
}

bool nativeRollbackPages_VisitFrame(uint64_t frame,
                                    NativeRollbackPageVisitor visitor,
                                    void* user_data)
{
    if (rollback.regions == NULL || rollback.active_frame != NULL ||
        visitor == NULL)
    {
        return false;
    }
    uint32_t slot = (uint32_t) (frame % rollback.frame_capacity);
    NativeRollbackFrame* saved = &rollback.frames[slot];
    if (!saved->valid || saved->frame != frame) {
        return false;
    }
    uint8_t* records =
        rollback.page_records + (size_t) slot * rollback.dirty_page_capacity *
                                    rollback.page_record_size;
    for (uint32_t i = 0; i < saved->dirty_count; ++i) {
        NativeRollbackPage* page = page_record(records, i);
        if (page->region >= rollback.region_count ||
            page->page >= rollback.regions[page->region].page_count)
        {
            return false;
        }
        NativeRollbackTrackedRegion* region = &rollback.regions[page->region];
        visitor(page->region, page->page,
                region->memory + (size_t) page->page * rollback.page_size,
                user_data);
    }
    return true;
}

uint32_t nativeRollbackPages_DirtyPageCount(void)
{
    return rollback.active_frame != NULL ? rollback.active_frame->dirty_count
                                         : 0;
}

size_t nativeRollbackPages_PageSize(void)
{
    return rollback.page_size;
}

#endif
