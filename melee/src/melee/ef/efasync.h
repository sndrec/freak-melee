#ifndef MELEE_EF_EFASYNC_H
#define MELEE_EF_EFASYNC_H

#include <sysdolphin/baselib/objalloc.h>
extern HSD_ObjAllocData efAsync_AllocData;

#include <sysdolphin/baselib/forward.h>

#include <stdarg.h>

#include <melee/ef/types.h>
#ifdef MELEE_NATIVE
#include <melee/ef/native_effect_limits.h>
#endif

static inline EF_QueuedEffect* efAsync_AllocateRequest(void)
{
#ifdef MELEE_NATIVE
    // A slot can be reused immediately. Its allocation still counts for this tick.
    if (efLib_Usage.requests_this_tick >= EF_MAX_QUEUED_EFFECTS) return NULL;
    ++efLib_Usage.requests_this_tick;
#endif
    return HSD_ObjAlloc(&efAsync_AllocData);
}

/* 063930 */ EF_SpawnResult efAsync_Dispatch(s32 gfx_id, HSD_GObj* gobj,
                                             va_list vlist);
/* 06729C */ void efAsync_LoadAsync(int index);
/* 06730C */ void efAsync_OnLoad(HSD_Archive* archive, u8* data, u32 length,
                                 int index);
/* 06737C */ void efAsync_LoadSync(int index);
/* 06744C */ void efAsync_QueueProcessDeferred(HSD_GObj* gobj,
                                               EF_QueuedEffect* queued_effect);
/* 067624 */ void efAsync_QueueFlush(HSD_GObj* gobj, void* arg_struct);
/* 067688 */ void efAsync_QueueClear(void* arg_struct);
/* 0676F0 */ void efAsync_Spawn(HSD_GObj* gobj, void* queue_head,
                                u32 spawn_kind, u32 gfx_id, HSD_JObj* jobj,
                                ...);
/* 0676F0 */ void efAsync_QueueInit(void);

#endif
