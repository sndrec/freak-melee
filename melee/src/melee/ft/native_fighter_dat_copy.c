#include "native_fighter_dat_copy.h"

#include <melee/gm/native_content_store.h>
#include <sysdolphin/baselib/dat.h>
#include <stdlib.h>
#include <string.h>

typedef struct External {
    const char* name;
    uint32_t head;
    uint32_t tail;
    uint32_t index;
} External;

typedef struct CopyInput {
    HSD_Dat dat;
    External* external; // Sorted by name, preserving original table indices.
} CopyInput;

static uint32_t be32(const void* value)
{
    const unsigned char* p = value;
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 |
           (uint32_t)p[2] << 8 | p[3];
}

static void put32(void* value, uint32_t word)
{
    unsigned char* p = value;
    p[0] = (unsigned char)(word >> 24);
    p[1] = (unsigned char)(word >> 16);
    p[2] = (unsigned char)(word >> 8);
    p[3] = (unsigned char)word;
}

static int external_order(const void* a, const void* b)
{
    return strcmp(((const External*)a)->name, ((const External*)b)->name);
}

static int word_order(const void* a, const void* b)
{
    uint32_t x = *(const uint32_t*)a, y = *(const uint32_t*)b;
    return x < y ? -1 : x != y;
}

static External* find_external(CopyInput* input, const char* name)
{
    uint32_t lo = 0, hi = input->dat.extern_count;
    while (lo < hi) {
        uint32_t mid = lo + (hi - lo) / 2;
        int order = strcmp(input->external[mid].name, name);
        if (order < 0) lo = mid + 1;
        else hi = mid;
    }
    return lo < input->dat.extern_count &&
           !strcmp(input->external[lo].name, name) ? &input->external[lo] : NULL;
}

// HSD_DatOpen validates each chain, but permits duplicate relocation slots,
// duplicate external names and shared chain slots. Those are ambiguous when
// rebuilding metadata. Mark every slot once, including relocated target zero.
static NativeFighterDatCopyResult read_input(CopyInput* input,
                                             const void* bytes, size_t size)
{
    if (size > NATIVE_CONTENT_MAX_PACKAGE_BYTES ||
        HSD_DatOpen(&input->dat, bytes, size) != HSD_DAT_OK)
        return NativeFighterDatCopy_Input;
    HSD_Dat* dat = &input->dat;
    if (dat->extern_count > NATIVE_CONTENT_MAX_PACKAGE_BYTES / sizeof(External))
        return NativeFighterDatCopy_Capacity;
    unsigned char* used = calloc(dat->data_size / 4 + 1, 1);
    input->external = calloc(dat->extern_count ? dat->extern_count : 1,
                             sizeof(External));
    if (!used || !input->external) {
        free(used);
        return NativeFighterDatCopy_Allocation;
    }
    NativeFighterDatCopyResult result = NativeFighterDatCopy_References;
    for (uint32_t i = 0; i < dat->relocation_count; ++i) {
        uint32_t slot = be32(dat->relocations + (size_t)i * 4);
        if (used[slot / 4]) goto done;
        used[slot / 4] = 1;
    }
    for (uint32_t i = 0; i < dat->extern_count; ++i) {
        External* e = &input->external[i];
        e->name = dat->strings + be32(dat->externs + (size_t)i * 8 + 4);
        e->head = be32(dat->externs + (size_t)i * 8);
        e->tail = UINT32_MAX;
        e->index = i;
        uint32_t slot = e->head;
        while (slot != UINT32_MAX) {
            if (used[slot / 4]) goto done;
            used[slot / 4] = 1;
            e->tail = slot;
            slot = be32(dat->data + slot);
        }
    }
    qsort(input->external, dat->extern_count, sizeof(External), external_order);
    for (uint32_t i = 1; i < dat->extern_count; ++i)
        if (!strcmp(input->external[i - 1].name, input->external[i].name))
            goto done;
    result = NativeFighterDatCopy_Ok;
done:
    free(used);
    return result;
}

static bool overlaps(const void* a, size_t as, const void* b, size_t bs)
{
    uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    if (as > UINTPTR_MAX - x || bs > UINTPTR_MAX - y) return true;
    return as && bs && x < y + bs && y < x + as;
}

bool nativeFighterDatCopy_PointerField(
    const void* destination, size_t destination_size, uint32_t destination_slot,
    const void* source, size_t source_size, uint32_t source_slot,
    void* output, size_t output_capacity, size_t* required_size,
    NativeFighterDatCopyStats* stats)
{
    NativeFighterDatCopyStats info = { .result = NativeFighterDatCopy_Input };
    CopyInput dst = {0}, src = {0};
    unsigned char* scratch = NULL;
    uint32_t* relocations = NULL;
    if (required_size) *required_size = 0;
    info.result = read_input(&dst, destination, destination_size);
    if (info.result != NativeFighterDatCopy_Ok) goto done;
    info.result = read_input(&src, source, source_size);
    if (info.result != NativeFighterDatCopy_Ok) goto done;
    u32 dst_target, src_target;
    const char* external;
    if (HSD_DatResolve(&dst.dat, destination_slot, &dst_target, &external) != HSD_DAT_REF_LOCAL ||
        HSD_DatResolve(&src.dat, source_slot, &src_target, &external) != HSD_DAT_REF_LOCAL) {
        info.result = NativeFighterDatCopy_Slot;
        goto done;
    }
    // Every total is computed in size_t under a 64 MiB package limit, before
    // narrowing to serialized 32-bit offsets or allocating output storage.
    size_t base = ((size_t)dst.dat.data_size + 31) & ~(size_t)31;
    size_t data_size = base + src.dat.data_size;
    size_t relocation_count = (size_t)dst.dat.relocation_count + src.dat.relocation_count;
    size_t external_count = dst.dat.extern_count;
    size_t strings_size = dst.dat.strings_size;
    for (uint32_t i = 0; i < src.dat.extern_count; ++i) {
        External* e = &src.external[i];
        if (find_external(&dst, e->name)) ++info.merged_external_count;
        else {
            ++external_count;
            strings_size += strlen(e->name) + 1;
        }
    }
    size_t total = 32 + data_size + relocation_count * 4 +
                   (size_t)dst.dat.root_count * 8 + external_count * 8 + strings_size;
    if (total > NATIVE_CONTENT_MAX_PACKAGE_BYTES || total > UINT32_MAX) {
        info.result = NativeFighterDatCopy_Capacity;
        goto done;
    }
    info.source_base = (uint32_t)base;
    info.copied_data_bytes = src.dat.data_size;
    info.relocation_count = (uint32_t)relocation_count;
    info.external_count = (uint32_t)external_count;
    info.output_bytes = total;
    if (required_size) *required_size = total;
    if (output && (overlaps(output, output_capacity, destination, destination_size) ||
                   overlaps(output, output_capacity, source, source_size))) {
        info.result = NativeFighterDatCopy_Overlap;
        goto done;
    }
    if (output && output_capacity < total) {
        info.result = NativeFighterDatCopy_Capacity;
        goto done;
    }
    // Sizing performs the same completed archive validation as real output.
    scratch = calloc(total, 1);
    relocations = malloc((relocation_count ? relocation_count : 1) * sizeof(uint32_t));
    if (!scratch || !relocations) {
        info.result = NativeFighterDatCopy_Allocation;
        goto done;
    }
    memcpy(scratch, destination, 32);
    put32(scratch, (uint32_t)total);
    put32(scratch + 4, (uint32_t)data_size);
    put32(scratch + 8, (uint32_t)relocation_count);
    put32(scratch + 16, (uint32_t)external_count);
    unsigned char* data = scratch + 32;
    memcpy(data, dst.dat.data, dst.dat.data_size);
    memcpy(data + base, src.dat.data, src.dat.data_size);
    for (uint32_t i = 0; i < dst.dat.relocation_count; ++i)
        relocations[i] = be32(dst.dat.relocations + (size_t)i * 4);
    for (uint32_t i = 0; i < src.dat.relocation_count; ++i) {
        uint32_t slot = be32(src.dat.relocations + (size_t)i * 4);
        relocations[dst.dat.relocation_count + i] = (uint32_t)base + slot;
        put32(data + base + slot, (uint32_t)base + be32(src.dat.data + slot));
    }
    put32(data + destination_slot, (uint32_t)base + src_target);
    qsort(relocations, relocation_count, sizeof(uint32_t), word_order);
    unsigned char* table = data + data_size;
    for (size_t i = 0; i < relocation_count; ++i) put32(table + i * 4, relocations[i]);
    table += relocation_count * 4;
    memcpy(table, dst.dat.roots, (size_t)dst.dat.root_count * 8);
    table += (size_t)dst.dat.root_count * 8;
    unsigned char* externs = table;
    memcpy(externs, dst.dat.externs, (size_t)dst.dat.extern_count * 8);
    unsigned char* strings = externs + external_count * 8;
    memcpy(strings, dst.dat.strings, dst.dat.strings_size);
    size_t next_string = dst.dat.strings_size;
    uint32_t next_external = dst.dat.extern_count;
    for (uint32_t i = 0; i < src.dat.extern_count; ++i) {
        External* e = &src.external[i];
        uint32_t slot = e->head;
        while (slot != UINT32_MAX) {
            uint32_t next = be32(src.dat.data + slot);
            put32(data + base + slot, next == UINT32_MAX ? next : (uint32_t)base + next);
            slot = next;
        }
        External* old = find_external(&dst, e->name);
        uint32_t head = e->head == UINT32_MAX ? UINT32_MAX : (uint32_t)base + e->head;
        if (old) {
            if (e->tail != UINT32_MAX) {
                put32(data + base + e->tail, old->head);
                put32(externs + (size_t)old->index * 8, head);
            }
        } else {
            put32(externs + (size_t)next_external * 8, head);
            put32(externs + (size_t)next_external * 8 + 4, (uint32_t)next_string);
            size_t length = strlen(e->name) + 1;
            memcpy(strings + next_string, e->name, length);
            next_string += length;
            ++next_external;
        }
    }
    HSD_Dat completed;
    if (HSD_DatOpen(&completed, scratch, total) != HSD_DAT_OK) {
        info.result = NativeFighterDatCopy_Output;
        goto done;
    }
    if (output) memcpy(output, scratch, total);
    info.result = NativeFighterDatCopy_Ok;
done:
    free(relocations);
    free(scratch);
    free(src.external);
    free(dst.external);
    if (stats) *stats = info;
    return info.result == NativeFighterDatCopy_Ok;
}
