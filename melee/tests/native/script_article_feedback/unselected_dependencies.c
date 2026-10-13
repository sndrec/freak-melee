#include <melee/ef/efasync.h>
#include <stdio.h>
#include <stdlib.h>

// Async enqueue is outside this synchronous effect argument/lifetime fixture.
void efAsync_Spawn(HSD_GObj* object, void* queue, u32 mode, u32 effect,
                   HSD_JObj* joint, ...)
{
    (void)object; (void)queue; (void)mode; (void)effect; (void)joint;
    fputs("Unselected async effect enqueue reached\n", stderr);
    abort();
}
