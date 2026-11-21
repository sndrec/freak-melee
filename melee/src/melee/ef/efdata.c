#include "efdata.h"
#include "eflib.h"

#if defined(MELEE_NATIVE)
// Initialized storage belongs to the rollback-managed game data section.
#pragma clang section data = ".mstate$m"
u32 efLib_AllocationSerial = 0;
EF_EffectUsage efLib_Usage = {0};
#endif

/* 458EB0 */ HSD_ObjAllocData efLib_AllocData;

// .sbss (mwcc allocates .sbss in reverse definition order ? sometimes...)
/* 4D64F0 */ s32 efLib_AnimCount;
/* 4D64EC */ u32 efLib_EffectCount;
/* 4D64E8 */ u32 efLib_LoadKind;

// Actual effect-global storage. Consumers access these tables directly.
#ifdef MELEE_NATIVE
HSD_JObj* efLib_AnimQueue[32];
#else
EF_ParamEntry efLib_AnimQueue[0x10];
#endif
EF_ParamEntry efLib_ParamTable[8];
