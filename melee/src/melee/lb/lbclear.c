#include "lb_00B0.h"

void memzero(void* mem, ssize_t size)
{
    u8* bytes = mem;
    while (size--) {
        *bytes++ = 0;
    }
}
