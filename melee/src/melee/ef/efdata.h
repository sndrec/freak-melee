#ifndef MELEE_EF_DATA_H
#define MELEE_EF_DATA_H

#include <dolphin/types.h>
#include <sysdolphin/baselib/objalloc.h>
#ifdef MELEE_NATIVE
#include "native_effect_limits.h"
#endif

extern HSD_ObjAllocData efLib_AllocData;
extern HSD_ObjAllocData efAsync_AllocData;

extern u32 efLib_LoadKind;
extern u32 efLib_EffectCount;
extern s32 efLib_AnimCount;
#if defined(MELEE_NATIVE)
extern u32 efLib_AllocationSerial;
#endif

#endif
