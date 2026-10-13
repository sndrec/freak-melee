#include <sysdolphin/baselib/devcom.h>
#include <dolphin/ar.h>
#include <dolphin/dvd.h>
#include <dolphin/os.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static unsigned char aram[0x20000], disk[0x10000];
static unsigned char host[0x10000] ATTRIBUTE_ALIGN(32);
static unsigned callbacks, transfers;
static unsigned pool_allocations, cancellations;
static bool defer_dvd;
static bool dvd_worker;
static DWORD owner_thread;
static bool cancel_again;
static int replacement_context;
static DVDCallback pending_callback;
static DVDFileInfo* pending_info;
static void* pending_destination;
static s32 pending_size, pending_offset;
static int context;
static void require(bool ok, const char* message)
{
    if (!ok) { fputs(message, stderr); exit(1); }
}
void __assert(char* file, u32 line, char* expression)
{
    fprintf(stderr, "%s:%u: %s\n", file, line, expression); abort();
}
void* HSD_AudioMalloc(size_t size) { ++pool_allocations; return calloc(1, size); }

// Deterministic synchronous provider boundaries model Aurora's ARAM behavior
// and also stress the earliest possible DVD completion. No thread-race proof.
void ARQPostRequest(ARQRequest* request, u32 owner, u32 type, u32 priority,
                   uintptr_t source, uintptr_t destination, u32 size, ARQCallback cb)
{
    require(++transfers < 100, "transfer restarted without advancing\n");
    if (type == ARAM_DIR_MRAM_TO_ARAM) {
        require(destination + size <= sizeof(aram), "ARAM write bounds\n");
        memcpy(aram + destination, (void*) source, size);
    } else {
        require(source + size <= sizeof(aram), "ARAM read bounds\n");
        memcpy((void*) destination, aram + source, size);
    }
    if (cb) cb(request);
}
BOOL DVDFastOpen(s32 entry, DVDFileInfo* info) { return true; }
BOOL DVDReadAsyncPrio(DVDFileInfo* info, void* destination, s32 size,
                      s32 offset, DVDCallback cb, s32 priority)
{
    require(offset >= 0 && (size_t) offset + size <= sizeof(disk), "DVD read bounds\n");
    if (defer_dvd) {
        require(!pending_callback, "overlapping DVD submissions\n");
        pending_callback = cb;
        pending_info = info;
        pending_destination = destination;
        pending_size = size;
        pending_offset = offset;
        return true;
    }
    memcpy(destination, disk + offset, size);
    cb(size, info);
    return true;
}
struct CompletedRead {
    DVDCallback callback;
    DVDFileInfo* info;
    void* destination;
    s32 size, offset;
};
static DWORD WINAPI finish_on_worker(void* parameter)
{
    struct CompletedRead* read = parameter;
    require(GetCurrentThreadId() != owner_thread, "missing DVD worker\n");
    memcpy(read->destination, disk + read->offset, read->size);
    read->callback(read->size, read->info);
    return 0;
}
static void finish_dvd(void)
{
    DVDCallback cb = pending_callback;
    require(cb != NULL, "missing delayed DVD read\n");
    pending_callback = NULL;
    unsigned before = callbacks + cancellations;
    if (dvd_worker) {
        struct CompletedRead read = { cb, pending_info, pending_destination,
                                      pending_size, pending_offset };
        HANDLE thread = CreateThread(NULL, 0, finish_on_worker, &read, 0, NULL);
        require(thread != NULL, "create DVD test worker\n");
        require(WaitForSingleObject(thread, 5000) == WAIT_OBJECT_0, "DVD worker timeout\n");
        CloseHandle(thread);
    } else {
        memcpy(pending_destination, disk + pending_offset, pending_size);
        cb(pending_size, pending_info);
    }
    require(callbacks + cancellations == before, "worker ran game completion\n");
    HSD_DevComPoll();
}
static void canceled_request(int request, void* args, void* buffer, bool canceled)
{
    require(GetCurrentThreadId() == owner_thread, "cancellation ran off owner thread\n");
    require(args == &context && canceled, "cancellation context\n");
    ++cancellations;
    if (cancel_again) {
        cancel_again = false;
        HSD_DevComCancelEx(request, 0, NULL, NULL);
    }
}
static void replacement_callback(int request, void* args, void* buffer, bool canceled)
{
    require(args == &replacement_context && canceled, "replacement cancellation context\n");
    ++cancellations;
}
static void complete(int request, void* args, void* buffer, bool canceled)
{
    require(GetCurrentThreadId() == owner_thread, "completion ran off owner thread\n");
    require(args == &context && !canceled, "completion context\n");
    ++callbacks;
}
static void read_buffer(int request, void* args, void* buffer, bool canceled)
{
    complete(request, args, buffer, canceled);
    require(buffer && memcmp(buffer, host, 32) == 0, "scratch callback data\n");
}
static void submit_from_callback(int request, void* args, void* buffer, bool canceled)
{
    complete(request, args, buffer, canceled);
    HSD_DevComRequest(0, 32, 0x6020, 32, 0x23, 1, complete, &context);
}
static void submit(int type, uintptr_t source, uintptr_t destination, size_t size,
                   HSD_DevComCallback cb)
{
    unsigned before = callbacks;
    HSD_DevComRequest(0, source, destination, size, type, 1, cb, &context);
    HSD_DevComPoll();
    require(callbacks == before + 1, "completion count\n");
    require(!HSD_DevComIsBusy((type & 0x38) == 0x20 ? 1 : 3), "queue left busy\n");
}
int main(void)
{
    owner_thread = GetCurrentThreadId();
    memset(aram, 0xFF, sizeof(aram));
    submit(3, 0, 0x4000, 0xA000, complete);
    for (unsigned i = 0x4000; i < 0xE000; ++i) require(aram[i] == 0, "chunked clear\n");
    for (unsigned i = 0; i < sizeof(host); ++i) host[i] = disk[i] = (u8) (i * 13);
    submit(0xB, (uintptr_t) host, 0x4000, 0xA000, complete);
    memset(host, 0, sizeof(host));
    submit(0x19, 0x4000, (uintptr_t) host, 0xA000, complete);
    require(memcmp(host, disk, 0xA000) == 0, "host transfer round trip\n");
    submit(0x1A, 0x4000, 0, 32, read_buffer);
    submit(0x1B, 0x4000, 0x10000, 0xA000, complete);
    require(memcmp(aram + 0x10000, disk, 0xA000) == 0, "chunked ARAM copy\n");
    submit(0x23, 0, 0x4000, 0xA000, complete);
    require(memcmp(aram + 0x4000, disk, 0xA000) == 0, "chunked DVD to ARAM\n");
    submit(0x21, 0, (uintptr_t) host, 0xA000, complete);
    submit(0x22, 0, 0, 32, read_buffer);
    unsigned before = callbacks;
    HSD_DevComRequest(0, 0, 0x6000, 32, 0x23, 1, submit_from_callback, &context);
    HSD_DevComPoll();
    require(callbacks == before + 2 && !HSD_DevComIsBusy(1), "nested completion\n");
    require(memcmp(aram + 0x6000, disk, 64) == 0, "nested transfer bytes\n");
    defer_dvd = true;
    int active = HSD_DevComRequest(0, 0, (uintptr_t) host, 32, 0x21, 1,
                                  canceled_request, &context);
    unsigned allocations_before = pool_allocations;
    for (unsigned i = 0; i < 64; ++i) {
        int queued = HSD_DevComRequest(0, 32, (uintptr_t) (host + 32), 32,
                                      0x21, 1, canceled_request, &context);
        cancel_again = true;
        HSD_DevComCancelEx(queued, 0, NULL, NULL);
    }
    require(cancellations == 64, "queued cancellation count\n");
    require(pool_allocations == allocations_before, "canceled nodes were not recycled\n");
    int replaced = HSD_DevComRequest(0, 32, (uintptr_t) (host + 32), 32,
                                    0x21, 1, complete, &context);
    HSD_DevComCancelEx(replaced, 3, replacement_callback, &replacement_context);
    require(cancellations == 65, "replacement callback count\n");
    HSD_DevComCancelEx(active, 0, NULL, NULL);
    require(cancellations == 65 && HSD_DevComIsBusy(1), "in-flight lifetime\n");
    finish_dvd();
    require(cancellations == 66 && !HSD_DevComIsBusy(1), "in-flight cancellation completion\n");
    // A canceled head in a lower-priority queue has not started its read yet.
    // Draining the active queue must retire it without allocating replacement nodes.
    for (unsigned i = 0; i < 32; ++i) {
        HSD_DevComRequest(0, 0, (uintptr_t) host, 32, 0x21, 0, complete, &context);
        int waiting_head = HSD_DevComRequest(0, 32, (uintptr_t) (host + 32), 32,
                                            0x21, 1, canceled_request, &context);
        HSD_DevComCancelEx(waiting_head, 0, NULL, NULL);
        finish_dvd();
        require(!pending_callback && !HSD_DevComIsBusy(0) && !HSD_DevComIsBusy(1),
                "canceled lower-priority head\n");
    }
    require(cancellations == 98 && pool_allocations == allocations_before,
            "canceled head recycling\n");
    dvd_worker = true;
    before = callbacks;
    HSD_DevComRequest(0, 0, (uintptr_t) host, 32, 0x21, 1, complete, &context);
    finish_dvd();
    require(callbacks == before + 1 && !HSD_DevComIsBusy(1), "worker handoff\n");
    puts("Native synchronous transfer ordering checks passed");
    return 0;
}
