#include <sysdolphin/baselib/native_memory_snapshot.h>
#include <sysdolphin/baselib/native_robj_snapshot.h>
#include <sysdolphin/baselib/native_hsd_reservation.h>
#include <sysdolphin/baselib/robj.h>
#include <sysdolphin/baselib/jobj.h>
#include <dolphin/os/OSHeapSnapshot.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks, refusals, evaluations, update_calls;
#define CHECK(test) do { ++checks; if (!(test)) { fprintf(stderr, "line %d: %s\n", __LINE__, #test); exit(1); } } while (0)
static unsigned char arena[1024 * 1024] ATTRIBUTE_ALIGN(64);
static unsigned char saved[1024], empty[1024], after[1024], malformed[1025];
static unsigned char heap_before[2048], heap_after[2048];
static NativeMemorySnapshot memory;
static NativeHsdReservation construction;
static unsigned char construction_backing[4096] ATTRIBUTE_ALIGN(64);
static OSHeapHandle main_heap, other_heap;
static float result, tail;
static bool inspect_active;

static void reject_active(void)
{
    size_t written = 991;
    CHECK(!nativeRObj_SnapshotBytes());
    CHECK(!nativeRObj_Save(after, sizeof(after), &written) && written == 991);
    CHECK(!nativeRObj_ValidateSnapshot(empty, sizeof(NativeRObjSnapshotHeader)));
    CHECK(!nativeRObj_Load(empty, sizeof(NativeRObjSnapshotHeader)));
}

static float expression(void* data)
{
    float* arguments = data;
    ++evaluations;
    if (inspect_active) reject_active();
    // The real evaluator supplies joint channels; explicitly touch the tail
    // so loss of the persistent buffer cannot hide behind identical poses.
    for (unsigned i = 3; i < 100; ++i) arguments[i] = tail + (float)i;
    return arguments[0] + arguments[1] * 2 + arguments[2] * 3;
}

static void update(void* object, int type, HSD_ObjData* value)
{
    (void)object;
    CHECK(type == 0x10 && value);
    result = value->fv;
    ++update_calls;
    if (inspect_active) reject_active();
}

static void evaluate(float x, float y, float z)
{
    // A resolved clean joint exercises actual expEvaluate without making a
    // synthetic object impersonate a class-managed dirty matrix owner.
    HSD_JObj joint = {0};
    joint.translate = (Vec3){x, y, z};
    HSD_Rvalue source = {NULL, 0x70, &joint};
    HSD_RObj constraint = {0};
    constraint.flags = 0x80000010;
    constraint.u.exp.expr.func = expression;
    constraint.u.exp.rvalue = &source;
    constraint.u.exp.nb_args = (u32)-1;
    HSD_RObjUpdateAll(&constraint, &joint, update);
    CHECK(constraint.u.exp.nb_args == 3);
    CHECK(result == x + y * 2 + z * 3);
}

static void reject_candidate(size_t bytes)
{
    size_t before_size, after_size;
    CHECK(nativeRObj_Save(after, sizeof(after), &before_size));
    unsigned char before[1024];
    memcpy(before, after, before_size);
    CHECK(!nativeRObj_ValidateSnapshot(malformed, bytes));
    CHECK(!nativeRObj_Load(malformed, bytes));
    CHECK(nativeRObj_Save(after, sizeof(after), &after_size));
    CHECK(before_size == after_size && !memcmp(before, after, before_size));
    ++refusals;
}

int main(void)
{
    void* start = OSInitAlloc(arena, arena + sizeof(arena), 2);
    CHECK(start);
    main_heap = OSCreateHeap(start, arena + sizeof(arena) / 2);
    other_heap = OSCreateHeap(arena + sizeof(arena) / 2, arena + sizeof(arena));
    CHECK(main_heap >= 0 && other_heap >= 0 && main_heap != other_heap);
    CHECK(OSPrepareHeapSnapshotOwnership());
    HSD_SetHeap(main_heap);
    OSSetCurrentHeap(other_heap);
    size_t memory_bytes = 0, empty_bytes = 0, packed_bytes = 0, written = 991;
    CHECK(nativeMemory_Save(&memory, sizeof(memory), &memory_bytes));
    CHECK(memory_bytes == sizeof(memory) && memory.heap == main_heap);
    CHECK(nativeRObj_Save(empty, sizeof(empty), &empty_bytes));
    CHECK(empty_bytes == sizeof(NativeRObjSnapshotHeader));
    u32 before_bytes, after_bytes;
    CHECK(OSSaveHeapSnapshot(main_heap, heap_before, sizeof(heap_before), &before_bytes));
    // Leave the argument Cell inside the older free extent, rather than at its
    // beginning. Metadata rewind can leave a plausible stale interior header.
    void* leading = OSAllocFromHeap(main_heap, 96);
    CHECK(leading);
    inspect_active = true; tail = 100;
    evaluate(1, 2, 3);
    CHECK(nativeRObj_Save(saved, sizeof(saved), &packed_bytes));
    CHECK(packed_bytes == sizeof(NativeRObjSnapshotHeader) + 400);
    NativeRObjSnapshotHeader header;
    memcpy(&header, saved, sizeof(header));
    CHECK(header.heap == main_heap && header.count == 100 && header.arguments);
    CHECK(header.arguments[0] == 1 && header.arguments[99] == 199);
    CHECK(OSSaveHeapSnapshot(main_heap, heap_after, sizeof(heap_after), &after_bytes));

    // Selector ownership is separate: restoring HSD selection must not change
    // the OS selector, and neither selector should get inferred from the other.
    HSD_SetHeap(other_heap);
    CHECK(nativeMemory_Load(&memory, memory_bytes));
    CHECK(HSD_GetHeap() == main_heap && __OSCurrHeap == other_heap);
    NativeMemorySnapshot bad_memory = memory;
    bad_memory.heap = 9;
    CHECK(!nativeMemory_Load(&bad_memory, sizeof(bad_memory)) && HSD_GetHeap() == main_heap);
    bad_memory = memory; ++bad_memory.epoch;
    CHECK(!nativeMemory_Load(&bad_memory, sizeof(bad_memory)));
    bad_memory = memory; bad_memory.reserved = 1;
    CHECK(!nativeMemory_Load(&bad_memory, sizeof(bad_memory)));
    CHECK(!nativeMemory_Save(header.arguments, sizeof(memory), &written) && written == 991);
    CHECK(!nativeMemory_Save(&bad_memory, sizeof(bad_memory), (size_t*)&bad_memory));
    HSD_SetHeap(-1);
    CHECK(nativeMemory_Save(&bad_memory, sizeof(bad_memory), &written));
    HSD_SetHeap(main_heap);
    CHECK(nativeMemory_Load(&bad_memory, sizeof(bad_memory)) && HSD_GetHeap() == -1);
    CHECK(nativeMemory_Load(&memory, sizeof(memory)));

    tail = 500; evaluate(7, 8, 9);
    CHECK(header.arguments[99] == 599);
    CHECK(OSLoadHeapSnapshot(main_heap, heap_after, after_bytes));
    CHECK(nativeRObj_Load(saved, packed_bytes));
    CHECK(header.arguments[0] == 1 && header.arguments[99] == 199);
    CHECK(nativeRObj_Save(after, sizeof(after), &written));
    CHECK(written == packed_bytes && !memcmp(saved, after, written));

    NativeHsdDemand demand = {NULL, 32, 1};
    CHECK(nativeHsdReservation_Bind(&construction, construction_backing,
        sizeof(construction_backing), &demand, 1, 1));
    CHECK(!nativeMemory_Save(construction_backing, sizeof(memory), &written));
    CHECK(!nativeRObj_Save(construction_backing, sizeof(construction_backing), &written));
    CHECK(nativeHsdReservation_Begin(&construction, &demand, 1));
    CHECK(!nativeMemory_Save(&bad_memory, sizeof(bad_memory), &written));
    CHECK(!nativeMemory_Load(&memory, sizeof(memory)));
    CHECK(!nativeRObj_SnapshotBytes());
    CHECK(!nativeRObj_Save(after, sizeof(after), &written));
    CHECK(!nativeRObj_Load(saved, packed_bytes));
    CHECK(nativeHsdReservation_End(&construction));
    CHECK(nativeHsdReservation_Release(&construction));

    memcpy(malformed, saved, packed_bytes); reject_candidate(packed_bytes - 1);
    memcpy(malformed, saved, packed_bytes); reject_candidate(packed_bytes + 1);
    NativeRObjSnapshotHeader bad = header;
    ++bad.epoch;
    memcpy(malformed, saved, packed_bytes); memcpy(malformed, &bad, sizeof(bad)); reject_candidate(packed_bytes);
    bad = header; bad.count = 101;
    memcpy(malformed, saved, packed_bytes); memcpy(malformed, &bad, sizeof(bad)); reject_candidate(packed_bytes);
    bad = header; bad.heap = other_heap;
    memcpy(malformed, saved, packed_bytes); memcpy(malformed, &bad, sizeof(bad)); reject_candidate(packed_bytes);
    bad = header; bad.arguments = (float*)saved;
    memcpy(malformed, saved, packed_bytes); memcpy(malformed, &bad, sizeof(bad)); reject_candidate(packed_bytes);
    bad = header; bad.arguments = (float*)((uintptr_t)header.arguments + 1);
    memcpy(malformed, saved, packed_bytes); memcpy(malformed, &bad, sizeof(bad)); reject_candidate(packed_bytes);
    bad = header; bad.arguments = NULL;
    memcpy(malformed, saved, packed_bytes); memcpy(malformed, &bad, sizeof(bad)); reject_candidate(packed_bytes);
    written = 991;
    CHECK(!nativeRObj_Save(header.arguments, sizeof(saved), &written) && written == 991);
    CHECK(!nativeRObj_Save(after, sizeof(after), (size_t*)header.arguments));
    CHECK(!nativeRObj_Save(after, sizeof(after), (size_t*)after));
    CHECK(!nativeRObj_Save(after, packed_bytes - 1, &written) && written == 991);

    // Return to a boundary before the first lazy buffer allocation. No free or
    // constructor is run by this owner: allocator companions rewind that step.
    CHECK(OSLoadHeapSnapshot(main_heap, heap_before, before_bytes));
    CHECK(nativeRObj_Load(empty, empty_bytes));
    CHECK(nativeRObj_Save(after, sizeof(after), &written));
    CHECK(written == empty_bytes && !memcmp(after, empty, written));
    CHECK(OSAllocFromHeap(main_heap, 96) == leading);
    tail = 100; evaluate(1, 2, 3);
    CHECK(nativeRObj_Save(after, sizeof(after), &written));
    CHECK(written == packed_bytes && !memcmp(after, saved, written));
    // Preserve an allocated snapshot through the now-coalesced physical range.
    CHECK(OSLoadHeapSnapshot(main_heap, heap_before, before_bytes));
    CHECK(nativeRObj_ValidateSnapshot(saved, packed_bytes));
    CHECK(!nativeRObj_SnapshotBytes()); // Current root is between companion restores.
    CHECK(!nativeRObj_Load(saved, packed_bytes)); // Cell owner must restore first.
    CHECK(OSLoadHeapSnapshot(main_heap, heap_after, after_bytes));
    CHECK(nativeRObj_Load(saved, packed_bytes));
    CHECK(nativeRObj_Save(after, sizeof(after), &written));
    CHECK(!memcmp(after, saved, packed_bytes));

    uint64_t lifetime = OSHeapSnapshotEpoch();
    _HSD_RObjForgetMemory(start, arena + sizeof(arena) / 2);
    OSDestroyHeap(main_heap);
    CHECK(OSHeapSnapshotEpoch() != lifetime);
    CHECK(!nativeMemory_ValidateSnapshot(&memory, sizeof(memory)));
    CHECK(!nativeRObj_Load(saved, packed_bytes));
    CHECK(!nativeRObj_Load(empty, empty_bytes));
    printf("{\"status\":\"PASS\",\"checks\":%u,\"refusals\":%u,\"evaluations\":%u,\"update_calls\":%u,\"memory_bytes\":%zu,\"empty_expression_bytes\":%zu,\"expression_bytes\":%zu}\n",
        checks, refusals, evaluations, update_calls, memory_bytes, empty_bytes, packed_bytes);
    return 0;
}
