#ifndef _memory_h_
#define _memory_h_

#include <Runtime/platform.h>
#include <dolphin/os/OSAlloc.h>

OSHeapHandle HSD_GetHeap(void);
void HSD_SetHeap(OSHeapHandle handle);

void HSD_Free(void* ptr);
void* HSD_MemAlloc(ssize_t size);

#endif
