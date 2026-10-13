#include <melee/gm/native_match_control_history.h>
#include "native_controller_snapshot.h"
#include "native_controller_snapshot_internal.h"
#include <stdlib.h>
#include <string.h>

extern HSD_RumbleData HSD_Rumble_804C22E0[4];

// Immutable backing/filter lifetime and idle scratch; excluded from .mstate.
HSD_PadRawFilter nativeControllerRawFilter;
u32 nativeControllerInputDepth;
static struct {
    uint64_t epoch;
    HSD_PadData* queue;
    HSD_PadRumbleListData* rumble;
    u16 rumble_count;
    u8 queue_count;
} backing;
static unsigned char visited[8192];

static void retire_history(void)
{
    if (backing.epoch == UINT64_MAX) abort();
    ++backing.epoch;
}

void HSD_PadSetRawFilter(HSD_PadRawFilter filter)
{
    if (filter != nativeControllerRawFilter) retire_history();
    nativeControllerRawFilter = filter;
}

void nativeController_Bind(u8 queue_count, HSD_PadData* queue,
                           u16 rumble_count, HSD_PadRumbleListData* rumble)
{
    retire_history();
    backing.queue = queue;
    backing.queue_count = queue_count;
    backing.rumble = rumble;
    backing.rumble_count = rumble_count;
}

static bool overlap(const void* a, size_t an, const void* b, size_t bn)
{
    uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    return an > UINTPTR_MAX - x || bn > UINTPTR_MAX - y ||
        (x < y + bn && y < x + an);
}

static bool independent(const void* data, size_t bytes)
{
    return data && bytes &&
        !overlap(data, bytes, &backing, sizeof(backing)) &&
        !overlap(data, bytes, visited, sizeof(visited)) &&
        !overlap(data, bytes, &nativeControllerRawFilter, sizeof(nativeControllerRawFilter)) &&
        !overlap(data, bytes, &nativeControllerInputDepth, sizeof(nativeControllerInputDepth)) &&
        !overlap(data, bytes, &HSD_PadLibData, sizeof(HSD_PadLibData)) &&
        !overlap(data, bytes, HSD_PadMasterStatus, sizeof(HSD_PadMasterStatus)) &&
        !overlap(data, bytes, HSD_PadCopyStatus, sizeof(HSD_PadCopyStatus)) &&
        !overlap(data, bytes, HSD_PadGameStatus, sizeof(HSD_PadGameStatus)) &&
        !overlap(data, bytes, HSD_Rumble_804C22E0, sizeof(HSD_Rumble_804C22E0)) &&
        !overlap(data, bytes, backing.queue, backing.queue_count * sizeof(*backing.queue)) &&
        !overlap(data, bytes, backing.rumble, backing.rumble_count * sizeof(*backing.rumble));
}

static size_t packed_bytes(void)
{
    if (!backing.epoch || !backing.queue_count || !backing.queue ||
        (backing.rumble_count && !backing.rumble) || nativeControllerInputDepth) return 0;
    return sizeof(NativeControllerSnapshotHeader) +
        backing.queue_count * sizeof(*backing.queue) +
        backing.rumble_count * sizeof(*backing.rumble);
}

static int rumble_index(const HSD_PadRumbleListData* pointer)
{
    uintptr_t at = (uintptr_t)pointer, start = (uintptr_t)backing.rumble;
    size_t bytes = backing.rumble_count * sizeof(*backing.rumble);
    if (!pointer || at < start || at - start >= bytes ||
        (at - start) % sizeof(*backing.rumble)) return -1;
    return (int)((at - start) / sizeof(*backing.rumble));
}

static bool topology(const NativeControllerSnapshotHeader* header,
                     const unsigned char* records)
{
    const PadLibData* library = &header->library;
    if (header->epoch != backing.epoch || library->queue != backing.queue ||
        library->qnum != backing.queue_count || library->qread >= library->qnum ||
        library->qwrite >= library->qnum || library->qcount > library->qnum ||
        library->qtype > 2 || library->qwrite !=
        (library->qread + library->qcount) % library->qnum ||
        library->rumble_info.max_list != backing.rumble_count) return false;
    memset(visited, 0, (backing.rumble_count + 7u) / 8u);
    unsigned total = 0;
    for (int channel = -1; channel < 4; ++channel) {
        const HSD_RumbleData* state = channel < 0 ? NULL : &header->rumble[channel];
        if (state && (state->last_status > 2 || state->status > 2 || state->direct_status > 1)) return false;
        HSD_PadRumbleListData* cursor = state ? state->listdatap : library->rumble_info.listdatap;
        unsigned count = 0;
        while (cursor) {
            int index = rumble_index(cursor);
            if (index < 0 || (visited[index / 8] & (1u << (index % 8)))) return false;
            visited[index / 8] |= (unsigned char)(1u << (index % 8));
            HSD_PadRumbleListData record;
            memcpy(&record, records + index * sizeof(record), sizeof(record));
            // Free records can retain old command pointers and counters; copy
            // them intact rather than interpreting unused bytes as live state.
            if (state && (!record.headp || !record.listp ||
                (((uintptr_t)record.headp | (uintptr_t)record.listp |
                  (uintptr_t)record.stack) & 1))) return false;
            cursor = record.next;
            ++count;
            ++total;
        }
        if (state && count != state->nb_list) return false;
    }
    return total == backing.rumble_count;
}

static NativeControllerSnapshotHeader current_header(void)
{
    NativeControllerSnapshotHeader header = {0};
    header.epoch = backing.epoch;
    memcpy(&header.library, &HSD_PadLibData, sizeof(header.library));
    memcpy(header.master, HSD_PadMasterStatus, sizeof(header.master));
    memcpy(header.copy, HSD_PadCopyStatus, sizeof(header.copy));
    memcpy(header.game, HSD_PadGameStatus, sizeof(header.game));
    memcpy(header.rumble, HSD_Rumble_804C22E0, sizeof(header.rumble));
    return header;
}

size_t nativeController_SnapshotBytes(void)
{
    size_t bytes = packed_bytes();
    if (!bytes) return 0;
    NativeControllerSnapshotHeader header = current_header();
    return topology(&header, (const unsigned char*)backing.rumble) ? bytes : 0;
}

bool nativeController_Save(void* data, size_t capacity, size_t* written)
{
    size_t bytes = nativeController_SnapshotBytes();
    if (!bytes || capacity < bytes || !independent(data, bytes) ||
        !independent(written, sizeof(*written)) ||
        overlap(data, bytes, written, sizeof(*written))) return false;
    NativeControllerSnapshotHeader header = current_header();
    memcpy(data, &header, sizeof(header));
    unsigned char* cursor = (unsigned char*)data + sizeof(header);
    size_t queue_bytes = backing.queue_count * sizeof(*backing.queue);
    size_t rumble_bytes = backing.rumble_count * sizeof(*backing.rumble);
    memcpy(cursor, backing.queue, queue_bytes);
    if (rumble_bytes) memcpy(cursor + queue_bytes, backing.rumble, rumble_bytes);
    *written = bytes;
    return true;
}

bool nativeController_ValidateSnapshot(const void* data, size_t bytes)
{
    if (!packed_bytes() || bytes != packed_bytes() || !independent(data, bytes)) return false;
    NativeControllerSnapshotHeader header;
    memcpy(&header, data, sizeof(header));
    return topology(&header, (const unsigned char*)data + sizeof(header) +
        backing.queue_count * sizeof(*backing.queue));
}

bool nativeController_Load(const void* data, size_t bytes)
{
    if (!nativeController_ValidateSnapshot(data, bytes)) return false;
    NativeControllerSnapshotHeader header;
    memcpy(&header, data, sizeof(header));
    const unsigned char* cursor = (const unsigned char*)data + sizeof(header);
    size_t queue_bytes = backing.queue_count * sizeof(*backing.queue);
    size_t rumble_bytes = backing.rumble_count * sizeof(*backing.rumble);
    memcpy(backing.queue, cursor, queue_bytes);
    if (rumble_bytes) memcpy(backing.rumble, cursor + queue_bytes, rumble_bytes);
    memcpy(&HSD_PadLibData, &header.library, sizeof(header.library));
    memcpy(HSD_PadMasterStatus, header.master, sizeof(header.master));
    memcpy(HSD_PadCopyStatus, header.copy, sizeof(header.copy));
    memcpy(HSD_PadGameStatus, header.game, sizeof(header.game));
    memcpy(HSD_Rumble_804C22E0, header.rumble, sizeof(header.rumble));
    return true;
}

#ifdef MELEE_NATIVE
bool nativeController_AdmitHistory(NativeMatchArena* arena)
{
    if(!arena || !arena->lifetime)return false;
#define ROOT(v) do { if(!nativeMatchArena_AddRoot(arena,&(v),sizeof(v)))return false; } while(0)
    ROOT(HSD_PadLibData);
    ROOT(HSD_PadMasterStatus);
    ROOT(HSD_PadCopyStatus);
    ROOT(HSD_PadGameStatus);
    ROOT(HSD_Rumble_804C22E0);
    if(backing.queue_count && !nativeMatchArena_AddRoot(arena,backing.queue,backing.queue_count*sizeof(*backing.queue)))return false;
    if(backing.rumble_count && !nativeMatchArena_AddRoot(arena,backing.rumble,backing.rumble_count*sizeof(*backing.rumble)))return false;
#undef ROOT
    return true;
}
#endif
