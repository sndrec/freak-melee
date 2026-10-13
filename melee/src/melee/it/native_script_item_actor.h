#ifndef MELEE_IT_NATIVE_SCRIPT_ITEM_ACTOR_H
#define MELEE_IT_NATIVE_SCRIPT_ITEM_ACTOR_H

#include <melee/ft/native_script_compile.h>
#include <melee/ft/native_script_binding_item_model.h>
#include <melee/it/forward.h>
#include <melee/ft/forward.h>
#include <melee/gm/native_match_pool.h>

enum {
    NativeScriptItemHook_Spawned, NativeScriptItemHook_Destroyed,
    NativeScriptItemHook_PickedUp, NativeScriptItemHook_Dropped,
    NativeScriptItemHook_Thrown, NativeScriptItemHook_DamageDealt,
    NativeScriptItemHook_DamageReceived, NativeScriptItemHook_EnteredAir,
    NativeScriptItemHook_Reflected, NativeScriptItemHook_Clanked,
    NativeScriptItemHook_Absorbed, NativeScriptItemHook_ShieldBounced,
    NativeScriptItemHook_HitShield, NativeScriptItemHook_Interaction,
    NativeScriptItemHook_Created, NativeScriptItemHook_OutOfBounds,
    NativeScriptItemHook_Accessory, NativeScriptItemHook_EnteredHitlag,
    NativeScriptItemHook_ExitedHitlag, NativeScriptItemHook_Touched,
    NativeScriptItemHook_JumpedOn, NativeScriptItemHook_GrabDealt,
    NativeScriptItemHook_GrabbedForVictim, NativeScriptItemHook_Launched,
    NativeScriptItemHook_StageContact, NativeScriptItemHook_StageLanded,
    NativeScriptItemHook_StageLeft, NativeScriptItemHook_HeldStatus,
    NativeScriptItemHook_HeldRequest, NativeScriptItemHook_HolderNeutral,
    NativeScriptItemHook_WorkshopThrown, NativeScriptItemHook_WorkshopTick,
    NativeScriptItemHook_WorkshopHit, NativeScriptItemHook_WorkshopGroundCollision,
    NativeScriptItemHook_Count
};
enum {
    NativeScriptItemPhase_Animation, NativeScriptItemPhase_Physics,
    NativeScriptItemPhase_Collision, NativeScriptItemPhase_Count
};
enum {
    NativeScriptItemCallback_Accessory, NativeScriptItemCallback_EnteredHitlag,
    NativeScriptItemCallback_ExitedHitlag, NativeScriptItemCallback_Touched,
    NativeScriptItemCallback_JumpedOn, NativeScriptItemCallback_GrabDealt,
    NativeScriptItemCallback_GrabbedForVictim, NativeScriptItemCallback_Count
};
enum {
    NATIVE_SCRIPT_ITEM_ACTORS=128, NATIVE_SCRIPT_ITEM_STATE_BYTES=8192,
    NATIVE_SCRIPT_ITEM_STATES=32, NATIVE_SCRIPT_ITEM_SIGNALS=256
};

// Native identity and physical scene/slot lifetime are independent. Relations
// may outlive a Fighter; resolving a stale relation never selects its replacement.
typedef struct NativeScriptItemRelation {
    NativeMatchPoolReference object;
    const void* data;
    uint32_t generation,kind;
} NativeScriptItemRelation;

// Root-owned immutable publication. Every numeric state is represented; no
// machine-code callbacks or ItemStateTable pointers select interpreted behavior.
typedef struct NativeScriptItemState {
    int32_t animation_id;
    uint32_t functions[NativeScriptItemPhase_Count];
} NativeScriptItemState;
typedef struct NativeScriptItemSignal { uint32_t signal_id, function; } NativeScriptItemSignal;
struct NativeItemDefinition;
struct NativeScriptItemProjectile;
struct NativeScriptItemAttachment;
struct NativeSourceItemVisual;
struct NativeScriptItemModelRootAsset;
struct NativeScriptItemModelAnimationAsset;
struct NativeArticleAnimationOrigin;
struct NativeScriptItemResourceSelector;
struct NativeScriptItemAnimationDescriptor;
struct ItemStateArray;

typedef enum NativeScriptItemDrawing {
    NativeScriptItemDrawing_Standard,
    NativeScriptItemDrawing_Billboard
} NativeScriptItemDrawing;
typedef struct NativeScriptItemProgram {
    const NativeScriptCompileBank* bank;
    uint64_t identity;
    FighterKind author_kind;
    ItemKind kind;
    uint32_t recipe_index, owner;
    NativeScriptItemDrawing drawing;
    uint16_t resource_capacity;
    int16_t resource_reserved_part; // -1 when no external part is reserved.
    const struct NativeItemDefinition* recipe;
    const Article* article;
    const struct NativeScriptItemProjectile* projectile;
    const struct NativeScriptItemAttachment* attachment;
    const struct NativeSourceItemVisual* visual;
    const struct NativeScriptItemModelRootAsset* model_roots;
    uint32_t model_root_count;
    bool clones_primary_model; // Derived from this owner's prepared bytecode.
    const struct NativeScriptItemModelAnimationAsset* model_animations;
    uint32_t model_animation_count;
    const struct NativeArticleAnimationOrigin* animation_origins;
    uint32_t animation_origin_count;
    const struct ItemStateArray* animation_states;
    const struct NativeScriptItemAnimationDescriptor* animation_descriptors;
    uint32_t animation_descriptor_count;
    bool animation_descriptors_prepared;
    const NativeScriptItemModelSelector* model_selectors;
    uint32_t model_selector_count;
    const struct NativeScriptItemResourceSelector* resource_selectors;
    uint32_t resource_selector_count;
    uint32_t hooks[NativeScriptItemHook_Count];
    const NativeScriptItemState* states;
    uint32_t state_count;
    const NativeScriptItemSignal* signals;
    uint32_t signal_count;
} NativeScriptItemProgram;

// Implemented by root-owned preparation/publication. Authored source owner is
// the discriminator; multiple entities may share physical kind/recipe assets.
const NativeScriptItemProgram* nativeScriptItem_Program(FighterKind author,
    uint32_t owner_node);
bool nativeScriptItem_HasActors(FighterKind author);
bool nativeScriptItem_ReferencesProgram(const NativeScriptCompileBank*);
bool nativeScriptItem_IsAttached(Item_GObj*);
// Logical execution status; unlike Spec, this rejects retired storage/faults.
NativeScriptVMStatus nativeScriptItem_Status(Item_GObj*);
const NativeScriptItemProgram* nativeScriptItem_Spec(Item_GObj*);
Fighter_GObj* nativeScriptItem_Creator(Item_GObj*);
Fighter_GObj* nativeScriptItem_AttachmentOwner(Item_GObj*);
bool nativeScriptItem_ClearAttachmentOwner(Item_GObj*);
bool nativeScriptItem_GetCreationValue(Item_GObj*,float* output);
bool nativeScriptItem_SetCreationValue(Item_GObj*,float value);
// Factory initialization before Created. NULL deliberately revokes control;
// this never changes immutable original creator or current damage ownership.
bool nativeScriptItem_SetAttachmentOwner(Item_GObj*,Fighter_GObj*);
bool nativeScriptItem_View(Item_GObj*,uint32_t generation,NativeScriptReceiverView*);
// Attach selects/pins storage only; Spawned/Created keep their actual source
// timing. Destroyed retires the storage view after its callback, retaining the
// registry/program pin through deferred cleanup. Detach releases it at final free.
bool nativeScriptItem_Attach(Item_GObj*, const NativeScriptItemProgram*);
void nativeScriptItem_Detach(Item_GObj*);
bool nativeScriptItem_StateAnimation(Item_GObj*, uint32_t state, int32_t* animation);
bool nativeScriptItem_SelectState(Item_GObj*, uint32_t state);
uint64_t nativeScriptItem_Revision(Item_GObj*);
// Advance once at the actual Item update boundary; this executes no script.
void nativeScriptItem_BeginTick(Item_GObj*);
bool nativeScriptItem_HookBound(Item_GObj*, uint32_t hook);
bool nativeScriptItem_PhaseBound(Item_GObj*, uint32_t phase);
bool nativeScriptItem_BindCallback(Item_GObj*, uint32_t slot, uint32_t function);
uint32_t nativeScriptItem_CallbackFunction(Item_GObj*, uint32_t slot);
void nativeScriptItem_ResetStateCallbacks(Item_GObj*);
// Missing hooks/phases produce the typed neutral null-slot result, never a
// native fallback. Faulted actors remain attached; Error/SourceRetired require
// stopping before further source userdata access. Bool true retains native
// removal meaning even after an ordinary state change.
NativeScriptVMStatus nativeScriptItem_RunHook(Item_GObj*, uint32_t hook,
    const NativeScriptValue* arguments, uint32_t count, NativeScriptValue* result);
NativeScriptVMStatus nativeScriptItem_RunPhase(Item_GObj*, uint32_t phase,
    NativeScriptValue* result);
NativeScriptVMStatus nativeScriptItem_RunSignal(Item_GObj*, uint32_t signal_id,
    float value, NativeScriptValue* result);
bool nativeScriptItem_CanBeginScene(uint64_t epoch);
bool nativeScriptItem_BeginScene(uint64_t epoch);
struct NativeMatchArena;
bool nativeScriptItem_AdmitHistory(struct NativeMatchArena*);
bool nativeScriptItem_RetireScene(void);
bool nativeScriptItem_Save(void* snapshot, size_t capacity, size_t* written);
bool nativeScriptItem_ValidateSnapshot(const void* snapshot, size_t bytes);
bool nativeScriptItem_Load(const void* snapshot, size_t bytes);

#endif
