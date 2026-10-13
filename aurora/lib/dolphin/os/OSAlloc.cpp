#include <dolphin/os.h>

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <dolphin/os/OSHeapSnapshot.h>

#include "../../logging.hpp"

extern "C" volatile OSHeapHandle __OSCurrHeap = -1;

namespace {

constexpr u32 kAlignment = 32;
constexpr u32 kHeaderSize = 32;
constexpr u32 kMinObjectSize = 64;

struct HeapDesc;

struct alignas(32) Cell {
  Cell* prev;
  Cell* next;
  s32 size;
  HeapDesc* owner;
};

static_assert(sizeof(Cell) == kHeaderSize, "Cell header must stay 32 bytes");

struct HeapDesc {
  s32 size;
  Cell* freeList;
  Cell* allocated;
};

static aurora::Module AllocLog("aurora::os::alloc");

static HeapDesc* sHeapArray = nullptr;
static int sNumHeaps = 0;
static u8* sArenaStart = nullptr;
static u8* sArenaEnd = nullptr;
static uint64_t sSnapshotEpoch;
// Host ownership lease. Heap topology is construction state during retained
// copy history; runtime object allocation belongs to the game's fixed pools.
static OSHeapHandle sHistoryHeap = -1;
static void historyMutation(const char* operation, OSHeapHandle heap) {
  std::fprintf(stderr, "[os-heap] %s refused during retained history: heap=%d\n", operation, heap);
  std::abort();
}

static void retireSnapshots() {
  if (sHistoryHeap != -1) historyMutation("geometry change", sHistoryHeap);
  if (sSnapshotEpoch == UINT64_MAX) std::abort();
  ++sSnapshotEpoch;
}

static uintptr_t roundUp32(const uintptr_t value) {
  return (value + (kAlignment - 1)) & ~(static_cast<uintptr_t>(kAlignment - 1));
}

static uintptr_t roundDown32(const uintptr_t value) {
  return value & ~(static_cast<uintptr_t>(kAlignment - 1));
}

static bool inArena(const void* ptr) {
  if (sArenaStart == nullptr || sArenaEnd == nullptr) {
    return false;
  }
  const auto p = reinterpret_cast<uintptr_t>(ptr);
  return p >= reinterpret_cast<uintptr_t>(sArenaStart) && p < reinterpret_cast<uintptr_t>(sArenaEnd);
}

static bool validHeapHandle(const OSHeapHandle heap) {
  return sHeapArray != nullptr && heap >= 0 && heap < sNumHeaps && sHeapArray[heap].size >= 0;
}

static Cell* addFront(Cell* list, Cell* cell) {
  cell->prev = nullptr;
  cell->next = list;
  if (list != nullptr) {
    list->prev = cell;
  }
  return cell;
}

static Cell* extract(Cell* list, Cell* cell) {
  if (cell->next != nullptr) {
    cell->next->prev = cell->prev;
  }
  if (cell->prev != nullptr) {
    cell->prev->next = cell->next;
    return list;
  }
  return cell->next;
}

static bool containsCell(Cell* list, Cell* cell) {
  for (Cell* it = list; it != nullptr; it = it->next) {
    if (it == cell) {
      return true;
    }
  }
  return false;
}

static Cell* insertAndCoalesce(Cell* list, Cell* cell) {
  Cell* prev = nullptr;
  Cell* next = list;
  while (next != nullptr && next < cell) {
    prev = next;
    next = next->next;
  }

  cell->prev = prev;
  cell->next = next;
  if (prev != nullptr) {
    prev->next = cell;
  } else {
    list = cell;
  }
  if (next != nullptr) {
    next->prev = cell;
  }

  if (cell->next != nullptr) {
    auto* right = cell->next;
    if (reinterpret_cast<u8*>(cell) + cell->size == reinterpret_cast<u8*>(right)) {
      cell->size += right->size;
      cell->next = right->next;
      if (right->next != nullptr) {
        right->next->prev = cell;
      }
    }
  }

  if (cell->prev != nullptr) {
    auto* left = cell->prev;
    if (reinterpret_cast<u8*>(left) + left->size == reinterpret_cast<u8*>(cell)) {
      left->size += cell->size;
      left->next = cell->next;
      if (cell->next != nullptr) {
        cell->next->prev = left;
      }
      return list;
    }
  }

  return list;
}

static bool validateBlockRange(const uintptr_t start, const uintptr_t end) {
  if (start >= end) {
    return false;
  }
  if (sArenaStart == nullptr || sArenaEnd == nullptr) {
    return false;
  }
  return start >= reinterpret_cast<uintptr_t>(sArenaStart)
      && end <= reinterpret_cast<uintptr_t>(sArenaEnd)
      && (end - start) >= kMinObjectSize;
}

static void dropTinyCell(HeapDesc& hd, Cell* cell) {
  hd.freeList = extract(hd.freeList, cell);
  hd.size -= cell->size;
}

static void carveRangeFromHeap(HeapDesc& hd, uintptr_t carveStart, uintptr_t carveEnd) {
  Cell* cell = hd.freeList;
  while (cell != nullptr) {
    Cell* nextCell = cell->next;
    const auto cellStart = reinterpret_cast<uintptr_t>(cell);
    const auto cellEnd = cellStart + static_cast<uintptr_t>(cell->size);

    const auto overlapStart = carveStart > cellStart ? carveStart : cellStart;
    const auto overlapEnd = carveEnd < cellEnd ? carveEnd : cellEnd;
    if (overlapStart >= overlapEnd) {
      cell = nextCell;
      continue;
    }

    const auto removed = static_cast<s32>(overlapEnd - overlapStart);
    hd.size -= removed;

    const bool cutHead = overlapStart == cellStart;
    const bool cutTail = overlapEnd == cellEnd;

    if (cutHead && cutTail) {
      hd.freeList = extract(hd.freeList, cell);
    } else if (cutHead) {
      auto* newCell = reinterpret_cast<Cell*>(overlapEnd);
      newCell->size = static_cast<s32>(cellEnd - overlapEnd);
      newCell->owner = nullptr;
      newCell->prev = cell->prev;
      newCell->next = cell->next;
      if (newCell->prev != nullptr) {
        newCell->prev->next = newCell;
      } else {
        hd.freeList = newCell;
      }
      if (newCell->next != nullptr) {
        newCell->next->prev = newCell;
      }
      if (newCell->size < static_cast<s32>(kMinObjectSize)) {
        dropTinyCell(hd, newCell);
      }
    } else if (cutTail) {
      cell->size = static_cast<s32>(overlapStart - cellStart);
      if (cell->size < static_cast<s32>(kMinObjectSize)) {
        dropTinyCell(hd, cell);
      }
    } else {
      const auto leftSize = static_cast<s32>(overlapStart - cellStart);
      const auto rightSize = static_cast<s32>(cellEnd - overlapEnd);
      if (leftSize >= static_cast<s32>(kMinObjectSize) && rightSize >= static_cast<s32>(kMinObjectSize)) {
        auto* right = reinterpret_cast<Cell*>(overlapEnd);
        right->size = rightSize;
        right->owner = nullptr;
        right->prev = cell;
        right->next = cell->next;
        if (right->next != nullptr) {
          right->next->prev = right;
        }
        cell->next = right;
        cell->size = leftSize;
      } else if (leftSize >= static_cast<s32>(kMinObjectSize)) {
        cell->size = leftSize;
      } else if (rightSize >= static_cast<s32>(kMinObjectSize)) {
        auto* right = reinterpret_cast<Cell*>(overlapEnd);
        right->size = rightSize;
        right->owner = nullptr;
        right->prev = cell->prev;
        right->next = cell->next;
        if (right->prev != nullptr) {
          right->prev->next = right;
        } else {
          hd.freeList = right;
        }
        if (right->next != nullptr) {
          right->next->prev = right;
        }
      } else {
        hd.freeList = extract(hd.freeList, cell);
      }
    }

    cell = nextCell;
  }
}

struct OwnedRange { uintptr_t start, end; };
// Allocator is already single-threaded. Only idle snapshot operations use this
// host scratch; it is not payload ownership or rollback state.
static struct {
  Cell* cells[OS_HEAP_SNAPSHOT_MAX_CELLS];
  OwnedRange ranges[OS_HEAP_SNAPSHOT_MAX_CELLS];
  u32 count, rangeCount;
} sSnapshotScratch;

// Immutable physical ownership, distinct from mutable Cell/free-list state.
// Allocation and coalescing cannot change these extents. Geometry operations
// retire the epoch before changing them. No allocations or tick-time sorting.
struct SnapshotOwnedRange { uintptr_t start, end; OSHeapHandle heap; };
static struct {
  SnapshotOwnedRange ranges[OS_HEAP_SNAPSHOT_MAX_CELLS];
  uint64_t epoch;
  u32 count;
} sSnapshotOwnership;

// Private, single-game-thread candidate index. Generation stamps avoid clearing
// a megabyte on each validation. It never enters saved state or survives a view.
constexpr u32 kSnapshotLookupCapacity = 2 * OS_HEAP_SNAPSHOT_MAX_CELLS;
static_assert(kSnapshotLookupCapacity == (1u << 17));
struct SnapshotLookupEntry { u32 row, generation; };
static struct {
  SnapshotLookupEntry entries[kSnapshotLookupCapacity];
  u32 generation;
} sSnapshotLookup;

static u32 snapshotLookupSlot(const void* address) {
  const u64 key = static_cast<u64>(reinterpret_cast<uintptr_t>(address)) >> 5;
  return static_cast<u32>((key * UINT64_C(0x9e3779b97f4a7c15)) >> 47);
}

static bool overlap(const void* a, size_t an, const void* b, size_t bn) {
  const auto x = reinterpret_cast<uintptr_t>(a), y = reinterpret_cast<uintptr_t>(b);
  return an > UINTPTR_MAX - x || bn > UINTPTR_MAX - y || (x < y + bn && y < x + an);
}

static bool collectSnapshotHeap(OSHeapHandle heap) {
  if (!validHeapHandle(heap) || !sSnapshotEpoch) return false;
  auto& scratch = sSnapshotScratch;
  auto& hd = sHeapArray[heap];
  scratch.count = scratch.rangeCount = 0;
  uint64_t total = 0;
  for (int list = 0; list < 2; ++list) {
    Cell* previous = nullptr;
    for (Cell* cell = list ? hd.allocated : hd.freeList; cell; cell = cell->next) {
      const auto at = reinterpret_cast<uintptr_t>(cell);
      if (scratch.count == OS_HEAP_SNAPSHOT_MAX_CELLS ||
          at < reinterpret_cast<uintptr_t>(sArenaStart) ||
          at > reinterpret_cast<uintptr_t>(sArenaEnd) - sizeof(Cell) ||
          (at & (kAlignment - 1))) return false;
      if (cell->prev != previous || cell->owner != (list ? &hd : nullptr) ||
          cell->size < static_cast<s32>(kMinObjectSize) ||
          (cell->size & (kAlignment - 1)) ||
          static_cast<uintptr_t>(cell->size) > reinterpret_cast<uintptr_t>(sArenaEnd) - at ||
          (!list && previous && reinterpret_cast<uintptr_t>(previous) >= at)) return false;
      total += static_cast<u32>(cell->size);
      if (total > static_cast<u32>(hd.size)) return false;
      scratch.cells[scratch.count++] = cell;
      previous = cell;
    }
  }
  if (total != static_cast<u32>(hd.size)) return false;
  std::sort(scratch.cells, scratch.cells + scratch.count,
            [](const Cell* a, const Cell* b) { return reinterpret_cast<uintptr_t>(a) < reinterpret_cast<uintptr_t>(b); });
  for (u32 i = 0; i < scratch.count; ++i) {
    auto* cell = scratch.cells[i];
    const auto at = reinterpret_cast<uintptr_t>(cell);
    const auto end = at + static_cast<u32>(cell->size);
    if (scratch.rangeCount && at < scratch.ranges[scratch.rangeCount - 1].end) return false;
    if (scratch.rangeCount && at == scratch.ranges[scratch.rangeCount - 1].end)
      scratch.ranges[scratch.rangeCount - 1].end = end;
    else scratch.ranges[scratch.rangeCount++] = {at, end};
  }
  return true;
}

static bool independentSnapshot(const void* data, size_t bytes) {
  return data && bytes &&
      !overlap(data, bytes, sArenaStart, static_cast<size_t>(sArenaEnd - sArenaStart)) &&
      !overlap(data, bytes, sHeapArray, static_cast<size_t>(sNumHeaps) * sizeof(HeapDesc)) &&
      !overlap(data, bytes, &sSnapshotScratch, sizeof(sSnapshotScratch)) &&
      !overlap(data, bytes, &sSnapshotOwnership, sizeof(sSnapshotOwnership)) &&
      !overlap(data, bytes, &sSnapshotLookup, sizeof(sSnapshotLookup)) &&
      !overlap(data, bytes, &sHeapArray, sizeof(sHeapArray)) &&
      !overlap(data, bytes, &sNumHeaps, sizeof(sNumHeaps)) &&
      !overlap(data, bytes, &sArenaStart, sizeof(sArenaStart)) &&
      !overlap(data, bytes, &sArenaEnd, sizeof(sArenaEnd)) &&
      !overlap(data, bytes, &sSnapshotEpoch, sizeof(sSnapshotEpoch)) &&
      !overlap(data, bytes, const_cast<OSHeapHandle*>(&__OSCurrHeap), sizeof(__OSCurrHeap));
}

static OSHeapSnapshotCell snapshotCell(const unsigned char* rows, u32 index) {
  OSHeapSnapshotCell record;
  std::memcpy(&record, rows + static_cast<size_t>(index) * sizeof(record), sizeof(record));
  return record;
}

static bool snapshotList(const unsigned char* rows, u32 count, void* head,
                         HeapDesc* owner, u32 expected) {
  void* previous = nullptr;
  u32 visited = 0;
  while (head) {
    if (visited == expected) return false;
    u32 slot = snapshotLookupSlot(head);
    while (sSnapshotLookup.entries[slot].generation == sSnapshotLookup.generation &&
           snapshotCell(rows, sSnapshotLookup.entries[slot].row).address != head) {
      slot = (slot + 1) & (kSnapshotLookupCapacity - 1);
    }
    if (sSnapshotLookup.entries[slot].generation != sSnapshotLookup.generation) return false;
    const u32 row = sSnapshotLookup.entries[slot].row;
    if (row >= count) return false;
    auto record = snapshotCell(rows, row);
    Cell state;
    std::memcpy(&state, record.state, sizeof(state));
    if (record.address != head || state.owner != owner || state.prev != previous) return false;
    previous = head;
    head = state.next;
    ++visited;
  }
  return visited == expected;
}

} // namespace

extern "C" {

void* OSInitAlloc(void* arenaStart, void* arenaEnd, int maxHeaps) {
  if (arenaStart == nullptr || arenaEnd == nullptr || maxHeaps <= 0) {
    return nullptr;
  }

  auto start = reinterpret_cast<uintptr_t>(arenaStart);
  auto end = reinterpret_cast<uintptr_t>(arenaEnd);
  if (start >= end) {
    return nullptr;
  }

  const auto arrayBytes = static_cast<uintptr_t>(maxHeaps) * sizeof(HeapDesc);
  if ((end - start) < arrayBytes + kMinObjectSize) {
    return nullptr;
  }

  retireSnapshots();
  sHeapArray = reinterpret_cast<HeapDesc*>(arenaStart);
  sNumHeaps = maxHeaps;
  for (int i = 0; i < sNumHeaps; ++i) {
    sHeapArray[i].size = -1;
    sHeapArray[i].freeList = nullptr;
    sHeapArray[i].allocated = nullptr;
  }

  __OSCurrHeap = -1;
  sArenaStart = reinterpret_cast<u8*>(roundUp32(start + arrayBytes));
  sArenaEnd = reinterpret_cast<u8*>(roundDown32(end));
  if (sArenaEnd <= sArenaStart || static_cast<uintptr_t>(sArenaEnd - sArenaStart) < kMinObjectSize) {
    sHeapArray = nullptr;
    sNumHeaps = 0;
    sArenaStart = nullptr;
    sArenaEnd = nullptr;
    return nullptr;
  }

  return sArenaStart;
}

OSHeapHandle OSCreateHeap(void* start, void* end) {
  if (sHeapArray == nullptr) {
    return -1;
  }

  const auto blockStart = roundUp32(reinterpret_cast<uintptr_t>(start));
  const auto blockEnd = roundDown32(reinterpret_cast<uintptr_t>(end));
  if (!validateBlockRange(blockStart, blockEnd)) {
    return -1;
  }

  for (OSHeapHandle heap = 0; heap < sNumHeaps; ++heap) {
    auto& hd = sHeapArray[heap];
    if (hd.size >= 0) {
      continue;
    }

    retireSnapshots();
    hd.size = static_cast<s32>(blockEnd - blockStart);
    hd.allocated = nullptr;
    hd.freeList = reinterpret_cast<Cell*>(blockStart);
    hd.freeList->prev = nullptr;
    hd.freeList->next = nullptr;
    hd.freeList->size = hd.size;
    hd.freeList->owner = nullptr;
    return heap;
  }

  return -1;
}

void OSDestroyHeap(OSHeapHandle heap) {
  if (!validHeapHandle(heap)) {
    return;
  }

  auto& hd = sHeapArray[heap];
  retireSnapshots();
  hd.size = -1;
  hd.freeList = nullptr;
  hd.allocated = nullptr;
  if (__OSCurrHeap == heap) {
    __OSCurrHeap = -1;
  }
}

void OSAddToHeap(OSHeapHandle heap, void* start, void* end) {
  if (!validHeapHandle(heap)) {
    return;
  }

  const auto blockStart = roundUp32(reinterpret_cast<uintptr_t>(start));
  const auto blockEnd = roundDown32(reinterpret_cast<uintptr_t>(end));
  if (!validateBlockRange(blockStart, blockEnd)) {
    return;
  }

  auto& hd = sHeapArray[heap];
  retireSnapshots();
  auto* cell = reinterpret_cast<Cell*>(blockStart);
  cell->prev = nullptr;
  cell->next = nullptr;
  cell->size = static_cast<s32>(blockEnd - blockStart);
  cell->owner = nullptr;
  hd.freeList = insertAndCoalesce(hd.freeList, cell);
  hd.size += cell->size;
}

void* OSAllocFromHeap(OSHeapHandle heap, u32 size) {
  if (!validHeapHandle(heap) || size == 0) {
    return nullptr;
  }

  if (heap == sHistoryHeap) historyMutation("allocation", heap);
  auto& hd = sHeapArray[heap];
  const auto requested = static_cast<s32>(roundUp32(static_cast<uintptr_t>(size) + kHeaderSize));

  Cell* cell = hd.freeList;
  while (cell != nullptr && cell->size < requested) {
    cell = cell->next;
  }
  if (cell == nullptr) {
    return nullptr;
  }

  const auto leftover = cell->size - requested;
  if (leftover < static_cast<s32>(kMinObjectSize)) {
    hd.freeList = extract(hd.freeList, cell);
  } else {
    auto* split = reinterpret_cast<Cell*>(reinterpret_cast<u8*>(cell) + requested);
    split->size = leftover;
    split->owner = nullptr;
    split->prev = cell->prev;
    split->next = cell->next;
    if (split->prev != nullptr) {
      split->prev->next = split;
    } else {
      hd.freeList = split;
    }
    if (split->next != nullptr) {
      split->next->prev = split;
    }
    cell->size = requested;
  }

  cell->owner = &hd;
  hd.allocated = addFront(hd.allocated, cell);
  return reinterpret_cast<u8*>(cell) + kHeaderSize;
}

void OSFreeToHeap(OSHeapHandle heap, void* ptr) {
  if (!validHeapHandle(heap) || ptr == nullptr) {
    return;
  }
  if (!inArena(ptr) || (reinterpret_cast<uintptr_t>(ptr) & (kAlignment - 1)) != 0) {
    return;
  }

  auto& hd = sHeapArray[heap];
  auto* cell = reinterpret_cast<Cell*>(reinterpret_cast<u8*>(ptr) - kHeaderSize);
  if (cell->owner != &hd || !containsCell(hd.allocated, cell)) {
    return;
  }

  if (heap == sHistoryHeap) historyMutation("free", heap);
  hd.allocated = extract(hd.allocated, cell);
  cell->owner = nullptr;
  hd.freeList = insertAndCoalesce(hd.freeList, cell);
}

OSHeapHandle OSSetCurrentHeap(OSHeapHandle heap) {
  const auto prev = __OSCurrHeap;
  if (heap == -1 || validHeapHandle(heap)) {
    __OSCurrHeap = heap;
  }
  return prev;
}

void* OSAllocFixed(void* rstart, void* rend) {
  if (sHeapArray == nullptr || rstart == nullptr || rend == nullptr) {
    return nullptr;
  }

  for (int i = 0; i < sNumHeaps; ++i) {
    if (sHeapArray[i].size >= 0 && sHeapArray[i].allocated != nullptr) {
      return nullptr;
    }
  }

  const auto fixedStart = roundDown32(reinterpret_cast<uintptr_t>(rstart));
  const auto fixedEnd = roundUp32(reinterpret_cast<uintptr_t>(rend));
  if (fixedStart >= fixedEnd) {
    return nullptr;
  }
  if (fixedStart < reinterpret_cast<uintptr_t>(sArenaStart)
      || fixedEnd > reinterpret_cast<uintptr_t>(sArenaEnd)) {
    return nullptr;
  }

  retireSnapshots();
  for (int i = 0; i < sNumHeaps; ++i) {
    auto& hd = sHeapArray[i];
    if (hd.size >= 0) {
      carveRangeFromHeap(hd, fixedStart, fixedEnd);
    }
  }

  return reinterpret_cast<void*>(fixedStart);
}

s32 OSCheckHeap(OSHeapHandle heap) {
  if (!validHeapHandle(heap)) {
    return -1;
  }

  auto& hd = sHeapArray[heap];
  s32 total = 0;
  s32 freeBytes = 0;

  if (hd.allocated != nullptr && hd.allocated->prev != nullptr) {
    return -1;
  }

  for (Cell* cell = hd.allocated; cell != nullptr; cell = cell->next) {
    if (!inArena(cell)
        || (reinterpret_cast<uintptr_t>(cell) & (kAlignment - 1)) != 0
        || cell->size < static_cast<s32>(kMinObjectSize)
        || (cell->size & (kAlignment - 1)) != 0
        || cell->owner != &hd
        || (cell->next != nullptr && cell->next->prev != cell)) {
      return -1;
    }
    total += cell->size;
    if (total <= 0 || total > hd.size) {
      return -1;
    }
  }

  if (hd.freeList != nullptr && hd.freeList->prev != nullptr) {
    return -1;
  }

  for (Cell* cell = hd.freeList; cell != nullptr; cell = cell->next) {
    if (!inArena(cell)
        || (reinterpret_cast<uintptr_t>(cell) & (kAlignment - 1)) != 0
        || cell->size < static_cast<s32>(kMinObjectSize)
        || (cell->size & (kAlignment - 1)) != 0
        || cell->owner != nullptr
        || (cell->next != nullptr && cell->next->prev != cell)) {
      return -1;
    }
    if (cell->next != nullptr) {
      if (reinterpret_cast<uintptr_t>(cell) + static_cast<uintptr_t>(cell->size)
          > reinterpret_cast<uintptr_t>(cell->next)) {
        return -1;
      }
    }

    total += cell->size;
    freeBytes += cell->size - static_cast<s32>(kHeaderSize);
    if (total <= 0 || total > hd.size) {
      return -1;
    }
  }

  if (total != hd.size) {
    return -1;
  }
  return freeBytes;
}

u32 OSReferentSize(void* ptr) {
  if (ptr == nullptr || !inArena(ptr) || (reinterpret_cast<uintptr_t>(ptr) & (kAlignment - 1)) != 0) {
    return 0;
  }
  auto* cell = reinterpret_cast<Cell*>(reinterpret_cast<u8*>(ptr) - kHeaderSize);
  if (cell->owner == nullptr) {
    return 0;
  }
  return static_cast<u32>(cell->size - static_cast<s32>(kHeaderSize));
}

void OSDumpHeap(OSHeapHandle heap) {
  AllocLog.info("OSDumpHeap({})", heap);
  if (!validHeapHandle(heap)) {
    AllocLog.info("--------Invalid");
    return;
  }

  auto& hd = sHeapArray[heap];
  if (OSCheckHeap(heap) < 0) {
    AllocLog.info("--------Broken");
    return;
  }

  AllocLog.info("addr\tsize\t\tend\t\tprev\t\tnext");
  AllocLog.info("--------Allocated");
  for (Cell* cell = hd.allocated; cell != nullptr; cell = cell->next) {
    AllocLog.info("{}\t{}\t{}\t{}\t{}",
                  reinterpret_cast<void*>(cell),
                  cell->size,
                  reinterpret_cast<void*>(reinterpret_cast<u8*>(cell) + cell->size),
                  reinterpret_cast<void*>(cell->prev),
                  reinterpret_cast<void*>(cell->next));
  }

  AllocLog.info("--------Free");
  for (Cell* cell = hd.freeList; cell != nullptr; cell = cell->next) {
    AllocLog.info("{}\t{}\t{}\t{}\t{}",
                  reinterpret_cast<void*>(cell),
                  cell->size,
                  reinterpret_cast<void*>(reinterpret_cast<u8*>(cell) + cell->size),
                  reinterpret_cast<void*>(cell->prev),
                  reinterpret_cast<void*>(cell->next));
  }
}

void OSVisitAllocated(void (*visitor)(void*, u32)) {
  if (visitor == nullptr || sHeapArray == nullptr) {
    return;
  }

  for (int heap = 0; heap < sNumHeaps; ++heap) {
    auto& hd = sHeapArray[heap];
    if (hd.size < 0) {
      continue;
    }
    for (Cell* cell = hd.allocated; cell != nullptr; cell = cell->next) {
      visitor(reinterpret_cast<u8*>(cell) + kHeaderSize,
              static_cast<u32>(cell->size - static_cast<s32>(kHeaderSize)));
    }
  }
}

BOOL OSGetHeapStateRegion(OSHeapHandle heap, OSAllocStateRegion* region) {
  if (!validHeapHandle(heap) || region == nullptr) {
    return FALSE;
  }
  *region = {
      &sHeapArray[heap],
      static_cast<u32>(sizeof(sHeapArray[heap])),
  };
  return TRUE;
}

BOOL OSGetCurrentHeapStateRegion(OSAllocStateRegion* region) {
  if (region == nullptr) {
    return FALSE;
  }
  *region = {
      const_cast<OSHeapHandle*>(&__OSCurrHeap),
      static_cast<u32>(sizeof(__OSCurrHeap)),
  };
  return TRUE;
}

BOOL OSLockHeapForHistory(OSHeapHandle heap) {
  if (sHistoryHeap != -1 || !collectSnapshotHeap(heap)) return FALSE;
  sHistoryHeap = heap;
  return TRUE;
}

BOOL OSUnlockHeapForHistory(OSHeapHandle heap) {
  if (sHistoryHeap != heap || heap == -1) return FALSE;
  sHistoryHeap = -1;
  return TRUE;
}

uint64_t OSHeapSnapshotEpoch(void) {
  return sHeapArray ? sSnapshotEpoch : 0;
}

BOOL OSPrepareHeapSnapshotOwnership(void) {
  if (!sHeapArray || !sSnapshotEpoch) return FALSE;
  if (sSnapshotOwnership.epoch == sSnapshotEpoch) return TRUE;
  sSnapshotOwnership.count = 0;
  sSnapshotOwnership.epoch = 0;
  for (OSHeapHandle heap = 0; heap < sNumHeaps; ++heap) {
    if (!validHeapHandle(heap)) continue;
    if (!collectSnapshotHeap(heap) || sSnapshotScratch.rangeCount >
        OS_HEAP_SNAPSHOT_MAX_CELLS - sSnapshotOwnership.count) return FALSE;
    for (u32 i = 0; i < sSnapshotScratch.rangeCount; ++i) {
      const auto& range = sSnapshotScratch.ranges[i];
      sSnapshotOwnership.ranges[sSnapshotOwnership.count++] = {range.start, range.end, heap};
    }
  }
  std::sort(sSnapshotOwnership.ranges, sSnapshotOwnership.ranges + sSnapshotOwnership.count,
      [](const SnapshotOwnedRange& a, const SnapshotOwnedRange& b) { return a.start < b.start; });
  for (u32 i = 1; i < sSnapshotOwnership.count; ++i)
    if (sSnapshotOwnership.ranges[i].start < sSnapshotOwnership.ranges[i - 1].end) return FALSE;
  sSnapshotOwnership.epoch = sSnapshotEpoch;
  return TRUE;
}

BOOL OSHeapSnapshotOwnsRange(OSHeapHandle heap, const void* data, u32 bytes) {
  const auto at = reinterpret_cast<uintptr_t>(data);
  if (!data || !bytes || bytes > UINTPTR_MAX - at || !validHeapHandle(heap) ||
      !OSPrepareHeapSnapshotOwnership()) return FALSE;
  u32 low = 0, high = sSnapshotOwnership.count;
  while (low < high) {
    const u32 middle = low + (high - low) / 2;
    if (sSnapshotOwnership.ranges[middle].start <= at) low = middle + 1;
    else high = middle;
  }
  if (!low) return FALSE;
  const auto& range = sSnapshotOwnership.ranges[low - 1];
  return range.heap == heap && at + bytes <= range.end;
}

BOOL OSHeapSnapshotBufferIndependent(const void* data, u32 bytes) {
  return sHeapArray && independentSnapshot(data, bytes);
}

BOOL OSHeapSnapshotOwnsAllocation(OSHeapHandle heap, const void* data, u32 bytes) {
  const auto at = reinterpret_cast<uintptr_t>(data);
  if (at < kHeaderSize || (at & (kAlignment - 1)) ||
      !OSHeapSnapshotOwnsRange(heap, data, bytes)) return FALSE;
  const auto* cell = reinterpret_cast<const Cell*>(at - kHeaderSize);
  if (!OSHeapSnapshotOwnsRange(heap, cell, kHeaderSize) ||
      cell->owner != &sHeapArray[heap] || cell->size < static_cast<s32>(kMinObjectSize) ||
      (cell->size & (kAlignment - 1)) ||
      bytes > static_cast<u32>(cell->size) - kHeaderSize) return FALSE;
  if (!OSHeapSnapshotOwnsRange(heap, cell, static_cast<u32>(cell->size))) return FALSE;
  // Old Cell headers can survive inside a coalesced free block after metadata
  // restore. Owner/capacity fields alone do not establish live membership.
  const Cell* previous = nullptr;
  u32 visited = 0;
  for (const Cell* live = sHeapArray[heap].allocated; live; live = live->next) {
    if (++visited > OS_HEAP_SNAPSHOT_MAX_CELLS ||
        (reinterpret_cast<uintptr_t>(live) & (kAlignment - 1)) ||
        !OSHeapSnapshotOwnsRange(heap, live, kHeaderSize) ||
        live->owner != &sHeapArray[heap] || live->prev != previous) return FALSE;
    if (live == cell) return TRUE;
    previous = live;
  }
  return FALSE;
}

u32 OSHeapSnapshotBytes(OSHeapHandle heap) {
  if (!collectSnapshotHeap(heap)) return 0;
  return static_cast<u32>(sizeof(OSHeapSnapshotHeader) +
                         sSnapshotScratch.count * sizeof(OSHeapSnapshotCell));
}

BOOL OSSaveHeapSnapshot(OSHeapHandle heap, void* data, u32 capacity, u32* written) {
  if (!validHeapHandle(heap) || !OSPrepareHeapSnapshotOwnership() || !collectSnapshotHeap(heap)) return FALSE;
  const auto bytes = static_cast<u32>(sizeof(OSHeapSnapshotHeader) +
                                     sSnapshotScratch.count * sizeof(OSHeapSnapshotCell));
  if (capacity < bytes || !independentSnapshot(data, bytes) ||
      !independentSnapshot(written, sizeof(*written)) || overlap(data, bytes, written, sizeof(*written))) return FALSE;
  if (__OSCurrHeap != -1 && !validHeapHandle(__OSCurrHeap)) return FALSE;
  // Collection proves current list membership, prev/next order, counts, total
  // size, alignment and disjoint Cells. Check its merged extents against the
  // frozen geometry too. An idle writer then emits a valid candidate directly;
  // callers need not walk and index the same output a second time.
  for (u32 i = 0; i < sSnapshotScratch.rangeCount; ++i) {
    const auto& range = sSnapshotScratch.ranges[i];
    if (!OSHeapSnapshotOwnsRange(heap, reinterpret_cast<const void*>(range.start),
                                static_cast<u32>(range.end - range.start))) return FALSE;
  }
  auto& hd = sHeapArray[heap];
  OSHeapSnapshotHeader header{};
  header.descriptor = &hd; header.arena_start = sArenaStart; header.arena_end = sArenaEnd;
  header.free_head = hd.freeList; header.allocated_head = hd.allocated;
  header.epoch = sSnapshotEpoch; header.heap = heap; header.current_heap = __OSCurrHeap;
  header.heap_bytes = hd.size; header.cells = sSnapshotScratch.count;
  auto* cursor = static_cast<unsigned char*>(data);
  std::memcpy(cursor, &header, sizeof(header)); cursor += sizeof(header);
  for (u32 i = 0; i < header.cells; ++i) {
    OSHeapSnapshotCell record{};
    record.address = sSnapshotScratch.cells[i];
    std::memcpy(record.state, record.address, sizeof(Cell));
    std::memcpy(cursor, &record, sizeof(record)); cursor += sizeof(record);
  }
  *written = bytes;
  return TRUE;
}

BOOL OSValidateHeapSnapshot(OSHeapHandle heap, const void* data, u32 bytes) {
  // Heap geometry is immutable within this epoch. Normal allocation/free only
  // splits/coalesces Cells inside these already admitted ranges; sorting the
  // current lists again neither strengthens candidate ownership nor restores it.
  if (!validHeapHandle(heap) || !OSPrepareHeapSnapshotOwnership() || bytes < sizeof(OSHeapSnapshotHeader) ||
      !independentSnapshot(data, bytes)) return FALSE;
  OSHeapSnapshotHeader header;
  std::memcpy(&header, data, sizeof(header));
  auto& hd = sHeapArray[heap];
  if (header.descriptor != &hd || header.heap != heap || header.epoch != sSnapshotEpoch ||
      header.arena_start != sArenaStart || header.arena_end != sArenaEnd || header.heap_bytes != hd.size ||
      header.cells > OS_HEAP_SNAPSHOT_MAX_CELLS ||
      bytes != sizeof(header) + header.cells * sizeof(OSHeapSnapshotCell) ||
      (header.current_heap != -1 && !validHeapHandle(header.current_heap))) return FALSE;
  const auto* rows = static_cast<const unsigned char*>(data) + sizeof(header);
  uintptr_t previousEnd = 0;
  uint64_t total = 0;
  u32 range = 0, allocated = 0;
  if (!++sSnapshotLookup.generation) {
    std::memset(sSnapshotLookup.entries, 0, sizeof(sSnapshotLookup.entries));
    sSnapshotLookup.generation = 1;
  }
  for (u32 i = 0; i < header.cells; ++i) {
    auto record = snapshotCell(rows, i);
    Cell state;
    std::memcpy(&state, record.state, sizeof(state));
    const auto at = reinterpret_cast<uintptr_t>(record.address);
    if (state.size < static_cast<s32>(kMinObjectSize) || (state.size & (kAlignment - 1)) ||
        (at & (kAlignment - 1)) || at < previousEnd ||
        static_cast<uintptr_t>(state.size) > UINTPTR_MAX - at ||
        (state.owner && state.owner != &hd)) return FALSE;
    const auto end = at + static_cast<u32>(state.size);
    while (range < sSnapshotOwnership.count && at >= sSnapshotOwnership.ranges[range].end) ++range;
    if (range == sSnapshotOwnership.count || sSnapshotOwnership.ranges[range].heap != heap ||
        at < sSnapshotOwnership.ranges[range].start || end > sSnapshotOwnership.ranges[range].end) return FALSE;
    if (state.owner) ++allocated;
    // Addresses are already checked in increasing, disjoint order. At most
    // half the fixed table is occupied, so probing always reaches a free slot.
    u32 slot = snapshotLookupSlot(record.address);
    while (sSnapshotLookup.entries[slot].generation == sSnapshotLookup.generation)
      slot = (slot + 1) & (kSnapshotLookupCapacity - 1);
    sSnapshotLookup.entries[slot] = {i, sSnapshotLookup.generation};
    total += static_cast<u32>(state.size);
    previousEnd = end;
  }
  if (total != static_cast<u32>(hd.size) ||
      !snapshotList(rows, header.cells, header.allocated_head, &hd, allocated) ||
      !snapshotList(rows, header.cells, header.free_head, nullptr, header.cells - allocated)) return FALSE;
  // Native free-list order is increasing address; preserve that contract too.
  for (u32 i = 0; i < header.cells; ++i) {
    Cell state;
    auto record = snapshotCell(rows, i);
    std::memcpy(&state, record.state, sizeof(state));
    if (!state.owner && state.next &&
        reinterpret_cast<uintptr_t>(state.next) <= reinterpret_cast<uintptr_t>(record.address)) return FALSE;
  }
  return TRUE;
}

BOOL OSLoadHeapSnapshot(OSHeapHandle heap, const void* data, u32 bytes) {
  if (!OSValidateHeapSnapshot(heap, data, bytes)) return FALSE;
  OSHeapSnapshotHeader header;
  std::memcpy(&header, data, sizeof(header));
  const auto* rows = static_cast<const unsigned char*>(data) + sizeof(header);
  for (u32 i = 0; i < header.cells; ++i) {
    auto record = snapshotCell(rows, i);
    std::memcpy(record.address, record.state, sizeof(Cell));
  }
  auto& hd = sHeapArray[heap];
  hd.size = header.heap_bytes;
  hd.freeList = static_cast<Cell*>(header.free_head);
  hd.allocated = static_cast<Cell*>(header.allocated_head);
  __OSCurrHeap = header.current_heap;
  return TRUE;
}

} // extern "C"
