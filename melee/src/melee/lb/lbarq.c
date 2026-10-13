#include "lbarq.h"

#include <dolphin/ar.h>
#include <string.h>
#include <sysdolphin/baselib/debug.h>

void lbArq_Read(unsigned int source, void* destination, size_t length)
{
    size_t capacity = ARGetSize();
    const unsigned char* storage = ARGetStorageAddress();
    HSD_ASSERT(0, source <= capacity && length <= capacity - source);
    HSD_ASSERT(0, storage != NULL && destination != NULL);
    memcpy(destination, storage + source, length);
}
