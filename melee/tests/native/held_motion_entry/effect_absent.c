// This Fighter-only fixture has no model-effect registry; link real Fighter/Item checks.
#include <melee/ef/forward.h>
#include <sysdolphin/baselib/forward.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* source, uint32_t generation)
{
    (void)source; (void)generation; return NULL;
}
bool nativeModelEffect_Snapshot(HSD_GObj* source, uint32_t* generation)
{
    (void)source; if(generation) *generation=0; return false;
}
