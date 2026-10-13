// Fighter/item-only fixtures have no effect registry. The dedicated model
// effect fixture links the real backend and exercises that domain separately.
#include <melee/ef/forward.h>
#include <sysdolphin/baselib/forward.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

EF_Effect* nativeModelEffect_Resolve(HSD_GObj* source, uint32_t generation)
{
    (void)source;
    (void)generation;
    return NULL;
}

bool nativeModelEffect_Snapshot(HSD_GObj* source, uint32_t* generation)
{
    (void)source;
    if (generation != NULL) *generation = 0;
    return false;
}
