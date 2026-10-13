#ifndef MELEE_IT_NATIVE_ITEM_OWNER_H
#define MELEE_IT_NATIVE_ITEM_OWNER_H

#include <melee/it/forward.h>
#include <dolphin/mtx.h>
#include <stdint.h>
#ifdef MELEE_NATIVE
#include <melee/gm/native_match_pool.h>
#endif

// Creator identity is separate from current damage/attachment ownership.
void nativeItem_CaptureCreator(Item_GObj*, Fighter_GObj*);
Fighter_GObj* nativeItem_Creator(Item_GObj*);
#ifdef MELEE_NATIVE
// Failed construction only: preserve transferred and unrelated live Items.
void nativeItem_RemoveConstructingOwnerChildren(Fighter_GObj*,
    NativeMatchPoolReference object_reference,
    NativeMatchPoolReference fighter_reference);
#endif
bool nativeItem_BindFighter(Fighter_GObj*, uint32_t handle, bool auxiliary);
bool nativeItem_BindExisting(Fighter_GObj*, uint32_t handle, bool auxiliary);
bool nativeItem_SetVisible(uint32_t handle, bool visible);
bool nativeItem_SetPaused(uint32_t handle, bool paused);
bool nativeItem_NotifyDrop(Fighter_GObj*, uint32_t handle, bool normal);
bool nativeItem_ClearBinding(Fighter_GObj*, bool auxiliary);
int32_t nativeItem_HeldId(Fighter_GObj*, bool auxiliary);
int32_t nativeItem_CreateEngine(Fighter_GObj*, ItemKind, Vec3*, Vec3*, float,
                                uint32_t collision_group, float damage);
bool nativeItem_Attach(Fighter_GObj*, uint32_t handle, int32_t raw_part,
                       bool notify_holder);
bool nativeItem_InheritOwnerScale(Fighter_GObj*, uint32_t handle,
                                 bool remember_throw_scale);

#endif
