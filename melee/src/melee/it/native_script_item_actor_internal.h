#ifndef MELEE_IT_NATIVE_SCRIPT_ITEM_ACTOR_INTERNAL_H
#define MELEE_IT_NATIVE_SCRIPT_ITEM_ACTOR_INTERNAL_H
#include "native_script_item_actor.h"
#include <melee/gm/native_match_pool.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>

typedef struct NativeScriptItemActor {
    Item_GObj* object;
    const NativeScriptItemProgram* program;
    NativeMatchPoolReference object_reference;
    NativeScriptItemRelation creator,attachment_owner;
    uint64_t revision, program_identity;
    uint32_t generation, tick, completed_callbacks, selected_state, running;
    uint32_t phases[NativeScriptItemPhase_Count];
    uint32_t callbacks[NativeScriptItemCallback_Count];
    float creation_value;
    bool active, faulted;
    uint8_t bytes[NATIVE_SCRIPT_ITEM_STATE_BYTES];
    NativeScriptStorageInstance storage;
    NativeScriptExecution execution;
} NativeScriptItemActor;
extern NativeScriptItemActor nativeScriptItemActors[NATIVE_SCRIPT_ITEM_ACTORS];
extern uint32_t nativeScriptItemInitializedCount;
extern uint64_t nativeScriptItemSceneEpoch;
extern uint16_t nativeScriptItemActorByObject[NATIVE_GOBJ_CAPACITY];
bool nativeScriptItem_Boundary(void);
bool nativeScriptItem_ProgramValid(const NativeScriptItemProgram*);
bool nativeScriptItem_NativeIdentity(const Item_GObj*, const NativeScriptItemProgram*, uint32_t generation);
bool nativeScriptItem_CallbackValid(const NativeScriptItemProgram*, uint32_t slot, uint32_t function);
#endif
