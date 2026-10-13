#include "native_rollback_hash.h"

#ifdef MELEE_NATIVE

#define WIN32_LEAN_AND_MEAN
#include <string.h>
#include <windows.h>

enum {
    NORMALIZED_RAW_WORD = 0,
    NORMALIZED_PAGED_POINTER = 1,
    NORMALIZED_EXACT_POINTER = 2,
    NORMALIZED_IMAGE_POINTER = 3,
    NORMALIZED_ASSET_POINTER = 4,
};

typedef struct NativeRollbackImageRange {
    uintptr_t start;
    size_t size;
} NativeRollbackImageRange;

static uint64_t hash_u64(uint64_t hash, uint64_t value)
{
    for (uint32_t byte = 0; byte < 8; ++byte) {
        hash ^= (uint8_t) (value >> (byte * 8));
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static NativeRollbackImageRange executable_image(void)
{
    HMODULE module = GetModuleHandleW(NULL);
    if (module == NULL) {
        return (NativeRollbackImageRange){ 0 };
    }
    const IMAGE_DOS_HEADER* dos = (const IMAGE_DOS_HEADER*) module;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0) {
        return (NativeRollbackImageRange){ 0 };
    }
    const IMAGE_NT_HEADERS* nt =
        (const IMAGE_NT_HEADERS*) ((const uint8_t*) module + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->OptionalHeader.SizeOfImage == 0)
    {
        return (NativeRollbackImageRange){ 0 };
    }
    return (NativeRollbackImageRange){
        .start = (uintptr_t) module,
        .size = nt->OptionalHeader.SizeOfImage,
    };
}

static bool address_offset(uintptr_t address, const void* memory, size_t size,
                           uintptr_t* offset)
{
    uintptr_t start = (uintptr_t) memory;
    if (address < start || address - start >= size) {
        return false;
    }
    *offset = address - start;
    return true;
}

static uint64_t hash_word(uint64_t hash,
                          const NativeRollbackRegionSet* regions,
                          NativeRollbackImageRange image, uintptr_t value)
{
    uintptr_t offset;
    for (uint32_t region = 0; region < regions->count; ++region) {
        if (address_offset(value, regions->regions[region].memory,
                           regions->regions[region].size, &offset))
        {
            hash = hash_u64(hash, NORMALIZED_PAGED_POINTER);
            hash = hash_u64(hash, region);
            return hash_u64(hash, offset);
        }
    }
    for (uint32_t region = 0; region < regions->exact_count; ++region) {
        if (address_offset(value, regions->exact_regions[region].memory,
                           regions->exact_regions[region].size, &offset))
        {
            hash = hash_u64(hash, NORMALIZED_EXACT_POINTER);
            hash = hash_u64(hash, region);
            return hash_u64(hash, offset);
        }
    }
    for (uint32_t region = 0; region < regions->identity_count; ++region) {
        if (address_offset(value, regions->identity_regions[region].memory,
                           regions->identity_regions[region].size, &offset))
        {
            hash = hash_u64(hash, NORMALIZED_ASSET_POINTER);
            hash = hash_u64(hash, regions->identity_regions[region].identity);
            return hash_u64(hash, offset);
        }
    }
    if (address_offset(value, (const void*) image.start, image.size, &offset))
    {
        hash = hash_u64(hash, NORMALIZED_IMAGE_POINTER);
        return hash_u64(hash, offset);
    }
    hash = hash_u64(hash, NORMALIZED_RAW_WORD);
    return hash_u64(hash, value);
}

static uint64_t hash_region(uint64_t hash,
                            const NativeRollbackRegionSet* regions,
                            NativeRollbackImageRange image, const void* memory,
                            size_t size)
{
    const uint8_t* bytes = memory;
    size_t offset = 0;
    for (; size - offset >= sizeof(uintptr_t); offset += sizeof(uintptr_t)) {
        uintptr_t word;
        memcpy(&word, bytes + offset, sizeof(word));
        hash = hash_word(hash, regions, image, word);
    }
    hash = hash_u64(hash, size - offset);
    for (; offset < size; ++offset) {
        hash ^= bytes[offset];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

uint64_t
nativeRollbackHash_AddressNormalized(const NativeRollbackRegionSet* regions)
{
    if (regions == NULL) {
        return 0;
    }
    uint64_t hash = UINT64_C(14695981039346656037);
    NativeRollbackImageRange image = executable_image();
    hash = hash_u64(hash, regions->count);
    for (uint32_t region = 0; region < regions->count; ++region) {
        hash = hash_u64(hash, region);
        hash = hash_u64(hash, regions->regions[region].size);
        hash =
            hash_region(hash, regions, image, regions->regions[region].memory,
                        regions->regions[region].size);
    }
    hash = hash_u64(hash, regions->exact_count);
    for (uint32_t region = 0; region < regions->exact_count; ++region) {
        hash = hash_u64(hash, region);
        hash = hash_u64(hash, regions->exact_regions[region].size);
        hash = hash_region(hash, regions, image,
                           regions->exact_regions[region].memory,
                           regions->exact_regions[region].size);
    }
    return hash;
}

uint64_t nativeRollbackHash_AddressNormalizedPagedRegion(
    const NativeRollbackRegionSet* regions, uint32_t region)
{
    if (regions == NULL || region >= regions->count) {
        return 0;
    }
    uint64_t hash = UINT64_C(14695981039346656037);
    hash = hash_u64(hash, NORMALIZED_PAGED_POINTER);
    hash = hash_u64(hash, region);
    hash = hash_u64(hash, regions->regions[region].size);
    return hash_region(hash, regions, executable_image(),
                       regions->regions[region].memory,
                       regions->regions[region].size);
}

uint64_t nativeRollbackHash_AddressNormalizedExactRegion(
    const NativeRollbackRegionSet* regions, uint32_t region)
{
    if (regions == NULL || region >= regions->exact_count) {
        return 0;
    }
    uint64_t hash = UINT64_C(14695981039346656037);
    hash = hash_u64(hash, NORMALIZED_EXACT_POINTER);
    hash = hash_u64(hash, region);
    hash = hash_u64(hash, regions->exact_regions[region].size);
    return hash_region(hash, regions, executable_image(),
                       regions->exact_regions[region].memory,
                       regions->exact_regions[region].size);
}

uint64_t nativeRollbackHash_AddressNormalizedRange(
    const NativeRollbackRegionSet* regions, const void* memory, size_t size)
{
    if (regions == NULL || memory == NULL || size == 0) {
        return 0;
    }
    uint64_t hash = UINT64_C(14695981039346656037);
    hash = hash_u64(hash, size);
    return hash_region(hash, regions, executable_image(), memory, size);
}

#endif
