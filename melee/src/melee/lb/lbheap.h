#ifndef MELEE_LB_HEAP_H
#define MELEE_LB_HEAP_H

#include <Runtime/platform.h>

typedef enum LbHeapType {
    LbHeapType_Hsd = 0,
    LbHeapType_MainLow = 1,
    LbHeapType_MainHigh = 2,
    LbHeapType_Aram = 3,
    LbHeapType_AramLow = 4,
} LbHeapType;

/// @remarks @c LbHeapStatus_Create is named by an assert in #lbHeap_80015CA8.
typedef enum LbHeapStatus {
    /* 0x00 */ LbHeapStatus_Create,
    /* 0x01 */ LbHeapStatus_Destroy,
} LbHeapStatus;

typedef struct LbHeapMemoryRegion {
    void* memory;
    size_t size;
    int allocator_heap;
} LbHeapMemoryRegion;

/* 0158D0 */ void lbHeap_800158D0(int, int);
/* 0158E8 */ int lbHeap_800158E8(int);
/* 015900 */ void lbHeap_80015900(void);
/* 015BB8 */ LbHeapStatus lbHeap_80015BB8(int);
/* 015BD0 */ void* lbHeap_80015BD0(int heap_id, size_t size);
/* 015CA8 */ void lbHeap_80015CA8(int, void*);
void lbHeap_Compact(u32 heap_id);
size_t lbHeap_GetActiveMainMemoryRegions(LbHeapMemoryRegion* regions,
                                         size_t capacity);
/* 015DF8 */ void lbHeap_80015DF8(void);
/* 015F3C */ void lbHeap_80015F3C(void);

#endif
