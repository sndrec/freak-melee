#include "devcom.h"

#include "debug.h"
#include "devcom.static.h"
#include "synth.h"
#include <limits.h>
#include <stdatomic.h>

// One DVD read is in flight. The worker publishes only its result; queue and
// game state are consumed exclusively by the thread calling HSD_DevComPoll.
static _Atomic(s32) dvd_completion = INT_MIN;

static void HSD_DevComDVDReadComplete(s32 result, DVDFileInfo* info)
{
    atomic_store_explicit(&dvd_completion, result, memory_order_release);
}

bool HSD_DevComIsBusy(int idx)
{
    return (bool) devComStatus[idx];
}

static void HSD_DevComUnlink(HSD_DevCom* dc)
{
    HSD_DevCom* curr;
    int i = dc->dcReq & 3;

    if (devComStatus[i] == dc) {
        devComStatus[i] = dc->next;
        if (HSD_DevCom_804C6330[i] == dc) {
            HSD_DevCom_804C6330[i] = 0;
        }
        return;
    }

    for (curr = devComStatus[i]; curr->next != NULL; curr = curr->next) {
        if (curr->next == dc) {
            curr->next = dc->next;
            if (HSD_DevCom_804C6330[i] == dc) {
                HSD_DevCom_804C6330[i] = curr;
            }

            return;
        }
    }
    HSD_ASSERT(0x6E, 0);

}

static void HSD_DevComStdCallback(ARQRequest* request)
{
    int i;

    if (request == &devComARQR[0][0]) {
        i = 0;
    } else if (request == &devComARQR[1][0]) {
        i = 1;
    } else {
        HSD_ASSERT(0xA5, 0);
    }
    aramstate = 0;
    devComRelayBufFlag[i] = false;
    HSD_DevComDVDWakeUp();
    HSD_DevComARAMWakeUp();
}

static inline void HSD_DevComRecycle(HSD_DevCom* devcom)
{
    devcom->next = HSD_DevCom_804D77F0;
    HSD_DevCom_804D77F0 = devcom;
}

static void HSD_DevComARAMCallback(ARQRequest* request)
{
    int i;
    void* buf;

    if (aramDC->type == 0x1A) {
        if (request == &devComARQR[0][0]) {
            i = 0;
        } else {
            i = 1;
        }
        buf = HSD_DevCom_804C6330_bufs[i];
    } else {
        buf = NULL;
    }

    if (aramDC->callback != NULL) {
        aramDC->callback(aramDC->dcReq, aramDC->args, buf,
                         aramDC->cancelflag);
    }

    HSD_DevComUnlink(aramDC);
    HSD_DevComRecycle(aramDC);
    HSD_DevComStdCallback(request);
}

static inline int getRelayBufIdx(void)
{
    int i;
    for (i = 0; i < 2; i++) {
        if (!devComRelayBufFlag[i]) {
            devComRelayBufFlag[i] = true;
            return i;
        }
    }
    return -1;
}

void HSD_DevComARAMWakeUp(void)
{
    int req_idx;
    u32 xfer_size2;
    void (*arq_callback)(ARQRequest*);
    void (*arq_callback2)(ARQRequest*);

    if (aramstate != 0) {
        return;
    }
    aramDC = devComStatus[3];
    if (devComStatus[3] != NULL) {
        if (aramDC->cancelflag) {
            HSD_DevCom* canceled = aramDC;
            HSD_DevComUnlink(canceled);
            if (canceled->callback != NULL) {
                canceled->callback(canceled->dcReq, canceled->args, NULL, true);
            }
            HSD_DevComRecycle(canceled);

            HSD_DevComARAMWakeUp();
            return;
        }
        req_idx = getRelayBufIdx();
        if (req_idx >= 0) {
            if (aramDC->type == 3) {
                u32 xfer_size;
                if (aramDC->size > DEVCOM_BUF_SIZE) {
                    arq_callback = HSD_DevComStdCallback;
                    xfer_size = DEVCOM_BUF_SIZE;
                } else {
                    arq_callback = HSD_DevComARAMCallback;
                    xfer_size = aramDC->size;
                }
                {
                    int* p = HSD_DevCom_804C6330_bufs[req_idx];
                    int i;
                    for (i = 0x1000; i > 0; i--) {
                        *p++ = 0;
                    }
                }

                uintptr_t destination = aramDC->dest;
                aramDC->dest += xfer_size;
                aramDC->size -= xfer_size;
                aramstate = 1;
                ARQPostRequest(devComARQR[req_idx], 0, 0, 1,
                               (uintptr_t) HSD_DevCom_804C6330_bufs[req_idx],
                               destination, xfer_size, arq_callback);
            } else if (aramDC->type == 0xB) {
                aramstate = 1;
                ARQPostRequest(devComARQR[req_idx], 0, 0, 1, aramDC->src,
                               aramDC->dest, aramDC->size,
                               HSD_DevComARAMCallback);
            } else if (aramDC->type == 0x19) {
                aramstate = 1;
                ARQPostRequest(devComARQR[req_idx], 0, 1, 1, aramDC->src,
                               aramDC->dest, aramDC->size,
                               HSD_DevComARAMCallback);
            } else if (aramDC->type == 0x1A) {
                aramstate = 1;
                ARQPostRequest(devComARQR[req_idx], 0, 1, 1, aramDC->src,
                               (uintptr_t) HSD_DevCom_804C6330_bufs[req_idx],
                               aramDC->size, HSD_DevComARAMCallback);
            } else if (aramDC->type == 0x1B) {
                if (aramDC->size > DEVCOM_BUF_SIZE) {
                    arq_callback2 = HSD_DevComStdCallback;
                    xfer_size2 = DEVCOM_BUF_SIZE;
                } else {
                    arq_callback2 = HSD_DevComARAMCallback;
                    xfer_size2 = aramDC->size;
                }
                ARQPostRequest(&devComARQR[req_idx][1], 0, 1, 1, aramDC->src,
                               (uintptr_t) HSD_DevCom_804C6330_bufs[req_idx],
                               xfer_size2, NULL);
                uintptr_t destination = aramDC->dest;
                aramDC->src += xfer_size2;
                aramDC->dest += xfer_size2;
                aramDC->size -= xfer_size2;
                aramstate = 1;
                ARQPostRequest(&devComARQR[req_idx][0], 0, 0, 1,
                               (uintptr_t) HSD_DevCom_804C6330_bufs[req_idx],
                               destination, xfer_size2, arq_callback2);
            }
        }
    }

}

static void HSD_DevComDVDStdCallback(ARQRequest* request)
{
    int i;
    if (request == &devComARQR[0][0]) {
        i = 0;
    } else if (request == &devComARQR[1][0]) {
        i = 1;
    } else {
        HSD_ASSERT(0x158, 0);
    }
    devComRelayBufFlag[i] = false;
    HSD_DevComDVDWakeUp();
    HSD_DevComARAMWakeUp();
}

static void HSD_DevComDVDARAMEndCallback(ARQRequest* request)
{
    int i;
    if (request == &devComARQR[0][0]) {
        i = 0;
    } else if (request == &devComARQR[1][0]) {
        i = 1;
    } else {
        HSD_ASSERT(0x158, 0);
    }
    HSD_DevCom* completed = HSD_DevCom_804D77FC[i];
    HSD_DevCom_804D77FC[i] = NULL;
    if (completed->callback != NULL && HSD_DevCom_804D7804 == 0) {
        completed->callback(completed->dcReq, completed->args, NULL,
                            completed->cancelflag);
    }
    HSD_DevComRecycle(completed);
    HSD_DevComDVDStdCallback(request);
}

static void HSD_DevComFinishDVDMemRead(s32 result)
{
    HSD_DevCom* dc;

    if (result == -1) {
        HSD_DevCom_804D7804 = 1;
    }
    if (dvdDC->size > 0x80000) {
        dvdDC->src += 0x80000;
        dvdDC->dest += 0x80000;
        dvdDC->size -= 0x80000;
        HSD_DevCom_804D77F5 = 0;
        HSD_DevComDVDWakeUp();
        return;
    }
    if (dvdDC->callback != NULL && HSD_DevCom_804D7804 == 0) {
        dvdDC->callback(dvdDC->dcReq, dvdDC->args, NULL,
                        dvdDC->cancelflag);
    }
    HSD_DevComUnlink(dvdDC);
    dc = dvdDC;

    dc->next = HSD_DevCom_804D77F0;
    HSD_DevCom_804D77F0 = dc;

    HSD_DevCom_804D77F5 = 0;
    HSD_DevComDVDWakeUp();
}

static void HSD_DevComFinishDVDRelayRead(s32 result)
{
    HSD_DevCom* dc;
    u16 type;

    PAD_STACK(8);

    if (result == -1) {
        HSD_DevCom_804D7804 = 1;
    }
    type = dvdDC->type;
    if (type == 0x22) {
        HSD_ASSERT(0x18C, dvdDC->size <= DEVCOM_BUF_SIZE);
        HSD_ASSERT(0x18D, dvdDC->callback);
        if (HSD_DevCom_804D7804 == 0) {
            dvdDC->callback(dvdDC->dcReq, dvdDC->args,
                            HSD_DevCom_804C6330_bufs[HSD_DevCom_804D77F6],
                            dvdDC->cancelflag);
        }
        HSD_DevComUnlink(dvdDC);
        dc = dvdDC;

        dc->next = HSD_DevCom_804D77F0;
        HSD_DevCom_804D77F0 = dc;

        HSD_DevCom_804D77F5 = 0;
        devComRelayBufFlag[HSD_DevCom_804D77F6] = false;
        HSD_DevComDVDWakeUp();
        HSD_DevComARAMWakeUp();
    } else if (type == 0x23) {
        HSD_DevCom_804D77F7 = HSD_DevCom_804D77F6;
        uintptr_t destination = dvdDC->dest;
        if (dvdDC->size > DEVCOM_BUF_SIZE) {
            dvdDC->src += DEVCOM_BUF_SIZE;
            dvdDC->dest += DEVCOM_BUF_SIZE;
            dvdDC->size -= DEVCOM_BUF_SIZE;
            HSD_DevCom_804D77F5 = 0;
            ARQPostRequest(
                devComARQR[HSD_DevCom_804D77F7], 0, 0, 1,
                (uintptr_t) HSD_DevCom_804C6330_bufs[HSD_DevCom_804D77F7],
                destination, DEVCOM_BUF_SIZE, HSD_DevComDVDStdCallback);
        } else {
            size_t length = dvdDC->size;
            HSD_DevCom_804D77FC[HSD_DevCom_804D77F7] = dvdDC;
            HSD_DevComUnlink(dvdDC);
            HSD_DevCom_804D77F5 = 0;
            ARQPostRequest(
                devComARQR[HSD_DevCom_804D77F7], 0, 0, 1,
                (uintptr_t) HSD_DevCom_804C6330_bufs[HSD_DevCom_804D77F7],
                destination, length, HSD_DevComDVDARAMEndCallback);
        }
    }
}

void HSD_DevComPoll(void)
{
    s32 result;
    while ((result = atomic_exchange_explicit(&dvd_completion, INT_MIN,
                                               memory_order_acquire)) != INT_MIN) {
        HSD_ASSERT(0, dvdDC && HSD_DevCom_804D77F5);
        if (dvdDC->type == 0x21) {
            HSD_DevComFinishDVDMemRead(result);
        } else {
            HSD_DevComFinishDVDRelayRead(result);
        }
    }
}

void HSD_DevComDVDWakeUp(void)
{
    int i;
    int buf_idx;

    if (HSD_DevCom_804D77F5 != 0) {
        return;
    }
    for (i = 0; i < 3; i++) {
        if ((dvdDC = devComStatus[i])) {
            if (dvdDC->cancelflag) {
                HSD_DevCom* canceled = dvdDC;
                HSD_DevComUnlink(canceled);
                if (canceled->callback != NULL) {
                    canceled->callback(canceled->dcReq, canceled->args, NULL, true);
                }
                HSD_DevComRecycle(canceled);

                HSD_DevComDVDWakeUp();
                return;
            }
            DVDFastOpen(dvdDC->file, &fileinfo);
            if (dvdDC->type == 0x21) {
                HSD_DevCom_804D77F5 = 1;
                DVDReadAsyncPrio(&fileinfo, (void*) dvdDC->dest,
                                 MIN(dvdDC->size, 0x80000), (s32) dvdDC->src,
                                 HSD_DevComDVDReadComplete, 2);
                return;
            }
            buf_idx = getRelayBufIdx();
            if (buf_idx >= 0) {
                HSD_DevCom_804D77F6 = buf_idx;
                HSD_DevCom_804D77F5 = 1;
                DVDReadAsyncPrio(&fileinfo, HSD_DevCom_804C6330_bufs[buf_idx],
                                 MIN(dvdDC->size, DEVCOM_BUF_SIZE), dvdDC->src,
                                 HSD_DevComDVDReadComplete, 2);
                return;
            }
        }
    }

}

static inline int HSD_DevComGetDestType(int type)
{
    return type & 7;
}

#define INIT_N_DEVCOMS 16

static inline void DevComLinkNext(HSD_DevCom* dc)
{
    int i;
    for (i = 1; i < INIT_N_DEVCOMS - 1; i++) {
        dc[i].next = &dc[i] + 1;
    }
    dc[i].next = NULL;
}

int HSD_DevComRequest(int file, uintptr_t src, uintptr_t dest, size_t size,
                      int type, int pri, HSD_DevComCallback cb, void* args)
{
    HSD_DevCom* dc;
    int result;


    if ((dc = HSD_DevCom_804D77F0)) {
        HSD_DevCom_804D77F0 = dc->next;

    } else {
        dc = HSD_AudioMalloc(sizeof(HSD_DevCom) * INIT_N_DEVCOMS);
        DevComLinkNext(dc);

        HSD_DevCom_804D77F0 = &dc[1];

    }

    HSD_ASSERT(0x1ED, dc);
    HSD_ASSERT(0x1EE,
        !(HSD_DevComGetDestType(type) == DEVCOMDEST_SBUF
            && size > DEVCOM_BUF_SIZE));

    // DVD sources are file offsets, not DMA buffer addresses. Native reads
    // support metadata starting inside a 32-byte block (SSM starts at 0x10).
    if ((type & 0x38) != 0x20) {
        HSD_ASSERT(0x1EF, src % 32 == 0);
    }
    HSD_ASSERT(0x1F0, dest % 32 == 0);
    HSD_ASSERT(0x1F1, size % 32 == 0);
    HSD_ASSERT(0x1F2, size != 0);

    pri = (type & 0x38) == 0x20 ? pri : 3;

    dc->file = file;
    dc->src = src;
    dc->dest = dest;
    dc->size = size;
    dc->type = type;
    dc->cancelflag = false;
    dc->callback = cb;
    dc->args = args;

    result = HSD_DevCom_804D6050 + pri;
    dc->dcReq = result;
    HSD_DevCom_804D6050 += 4;
    if (HSD_DevCom_804C6330[pri] != NULL) {
        HSD_DevCom_804C6330[pri]->next = dc;
    }
    HSD_DevCom_804C6330[pri] = dc;
    dc->next = NULL;
    if (devComStatus[pri] == NULL) {
        devComStatus[pri] = dc;
        HSD_DevComDVDWakeUp();
        HSD_DevComARAMWakeUp();
    }


    return result;
}

static inline HSD_DevCom* HSD_DevComCancelEx_inline(int dcReq)
{
    HSD_DevCom* cur = devComStatus[dcReq & 3];
    while (cur != NULL) {
        if (cur->dcReq == dcReq) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

int HSD_DevComCancelEx(int dcReq, u32 flags, HSD_DevComCallback cb, void* args)
{
    HSD_DevCom* dc;

    if ((dc = HSD_DevComCancelEx_inline(dcReq))) {
        int tmp = dcReq & 3;
        if (flags & 1) {
            dc->callback = cb;
        }
        if (flags & 2) {
            dc->args = args;
        }
        if (devComStatus[tmp] == dc) {
            dc->cancelflag = true;
        } else {
            HSD_DevComUnlink(dc);
            if (dc->callback != NULL) {
                dc->callback(dc->dcReq, dc->args, NULL, true);
            }
            HSD_DevComRecycle(dc);
        }
    } else {
        int i;
        for (i = 0; i < 2; i++) {
            HSD_DevCom* dc = HSD_DevCom_804D77FC[i];
            if (dc != NULL && dc->dcReq == dcReq) {
                if (flags & 1) dc->callback = cb;
                if (flags & 2) dc->args = args;
                dc->cancelflag = true;
            }
        }
    }

    return 0;
}
