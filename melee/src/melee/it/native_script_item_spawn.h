#ifndef MELEE_IT_NATIVE_SCRIPT_ITEM_SPAWN_H
#define MELEE_IT_NATIVE_SCRIPT_ITEM_SPAWN_H
#include <melee/ft/native_script_vm.h>
#include <melee/it/forward.h>
#include <dolphin/mtx.h>

typedef struct NativeScriptItemSpawnData {
    Vec3 position, previous, velocity;
    float facing, creation_value;
    uint32_t collision_group;
    int32_t state; // -1 leaves selection to the actual Spawned callback.
    bool origin_at_owner, initial_collision, initial_update, damage_owned;
} NativeScriptItemSpawnData;
typedef struct NativeScriptItemLaunchData {
    float angle, speed;
    uint32_t state, collision_group;
    bool preserve_facing;
} NativeScriptItemLaunchData;

// Native output only. Script handle representation/publication belongs to the
// typed binding layer. Failure/retirement leaves output untouched and preserves
// completed native creation side effects; no implicit gameplay rollback.
NativeScriptVMStatus nativeScriptItemSpawn_Create(NativeScriptRuntimeContext*, uint32_t owner_node,
    const NativeScriptItemSpawnData*, Item_GObj** output, NativeScriptDiagnostic*);
// Exact positional constructor defaults: previous is the supplied position;
// owner-center changes only current spawn position. A live allocation failure
// succeeds with NULL output; retirement/error leaves output untouched. When
// gameplay_owned is false, both gameplay parents and the original creator are
// NULL from construction onward; the caller's code/asset author is retained.
NativeScriptVMStatus nativeScriptItemSpawn_At(NativeScriptRuntimeContext*,uint32_t owner_node,
    const Vec3* position,const Vec3* velocity,float facing,bool origin_at_owner,
    uint32_t collision_group,float creation_value,bool initial_collision,bool gameplay_owned,
    Item_GObj** output,NativeScriptDiagnostic*);
NativeScriptVMStatus nativeScriptItemSpawn_Child(NativeScriptRuntimeContext*,uint32_t child_owner,
    const Vec3* position,float facing,uint32_t damage_owner_handle,uint32_t collision_group,
    float creation_value,uint32_t backlink,uint32_t parent_slot,Item_GObj** output,NativeScriptDiagnostic*);
NativeScriptVMStatus nativeScriptItemSpawn_Projectile(NativeScriptRuntimeContext*, uint32_t owner_node,
    const Vec3* previous, float facing, int32_t variant, const NativeScriptItemLaunchData* optional_launch,
    Item_GObj** output, NativeScriptDiagnostic*);
// Supplied coordinates are previous position; current position is the Fighter
// ECB center. Missing raw parts and healthy constructor NULL publish NULL.
// Created precedes native pickup, and held registration follows both hooks.
NativeScriptVMStatus nativeScriptItemSpawn_HeldAtCenter(NativeScriptRuntimeContext*, uint32_t owner_node,
    const Vec3* previous, uint32_t raw_part, float facing, Item_GObj** output, NativeScriptDiagnostic*);
NativeScriptVMStatus nativeScriptItemSpawn_Attached(NativeScriptRuntimeContext*, uint32_t owner_node,
    uint32_t raw_part, Item_GObj** output, NativeScriptDiagnostic*);
NativeScriptVMStatus nativeScriptItemSpawn_AttachedPresentation(NativeScriptRuntimeContext*, uint32_t owner_node,
    uint32_t raw_part, int32_t variant, int32_t appearance, uint32_t initial_state,
    uint32_t attached_state, float lifetime, Item_GObj** output, NativeScriptDiagnostic*);
NativeScriptVMStatus nativeScriptItemSpawn_PullHoldable(NativeScriptRuntimeContext*, uint32_t owner_node,
    Item_GObj** output, NativeScriptDiagnostic*);
NativeScriptVMStatus nativeScriptItemSpawn_Food(NativeScriptRuntimeContext*, Item_GObj** output,
    NativeScriptDiagnostic*);
#endif
