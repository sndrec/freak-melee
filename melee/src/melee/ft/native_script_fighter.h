#ifndef MELEE_FT_NATIVE_SCRIPT_FIGHTER_H
#define MELEE_FT_NATIVE_SCRIPT_FIGHTER_H

#include "native_script_compile.h"
#include "native_script_item_program.h"
#include "native_script_effect_program.h"
#include "forward.h"

enum { NativeScriptFighter_Load,NativeScriptFighter_Tick,
    NativeScriptFighter_Remove,NativeScriptFighter_Death,NativeScriptFighter_LateUpdate,
    NativeScriptFighter_SpecialN,NativeScriptFighter_SpecialS,NativeScriptFighter_SpecialHi,NativeScriptFighter_SpecialLw,
    NativeScriptFighter_SpecialAirN,NativeScriptFighter_SpecialAirS,NativeScriptFighter_SpecialAirHi,NativeScriptFighter_SpecialAirLw,
    NativeScriptFighter_CpuCaptureDecision,NativeScriptFighter_CpuRangedCandidates,
    NativeScriptFighter_LoadSpecialAttrs,NativeScriptFighter_LandingAction,
    NativeScriptFighter_ItemPickupExt,NativeScriptFighter_ItemDropExt,
    NativeScriptFighter_ItemPickup,NativeScriptFighter_ItemDrop,
    NativeScriptFighter_ItemVisible,NativeScriptFighter_ItemInvisible,
    NativeScriptFighter_KnockbackEnter,NativeScriptFighter_KnockbackExit,NativeScriptFighter_Landing,
    NativeScriptFighter_AerialJump,
    NativeScriptFighter_FormActivated,
    NativeScriptFighter_CpuInitialize,NativeScriptFighter_CpuModeUpdate,NativeScriptFighter_CpuOutput,
    NativeScriptFighter_CpuPrepare,NativeScriptFighter_CpuDropCommand,
    NativeScriptFighter_CpuStickStep,NativeScriptFighter_CpuStickLimit,NativeScriptFighter_CpuPendingAction,
    NativeScriptFighter_CpuSpacingUpdate,NativeScriptFighter_CpuAttackMotion,NativeScriptFighter_CpuGrabMotion,
    NativeScriptFighter_CpuAttackDecision,NativeScriptFighter_CpuTravelDecision,
    NativeScriptFighter_CpuFallingRecoveryAllowed,NativeScriptFighter_CpuFallingDestinationReached,
    NativeScriptFighter_CpuAvailableJump,NativeScriptFighter_CpuProjectileDefense,NativeScriptFighter_CpuPummelSpecial,
    NativeScriptFighter_CpuRangedAllowed,NativeScriptFighter_CpuSpecialCandidate,NativeScriptFighter_CpuProjectileHit,
    NativeScriptFighter_CpuThreat,NativeScriptFighter_CpuChargeDecision,NativeScriptFighter_CpuChargeCancel,
    NativeScriptFighter_CpuRecovery,
    NativeScriptFighter_ColorAnimationReset,NativeScriptFighter_PresentationItem,
    NativeScriptFighter_DemoEnter,NativeScriptFighter_Taunt,
    NativeScriptFighter_HeldMotion,NativeScriptFighter_NeutralEnter,
    NativeScriptFighter_AirResumeMotion,NativeScriptFighter_HeldTimerExpired,
    NativeScriptFighter_GroundedMotionSetup,
    NativeScriptFighter_AerialActionManual,NativeScriptFighter_AerialActionAutomatic,
    NativeScriptFighter_WaitEnter,NativeScriptFighter_CrouchWaitEnter,
    NativeScriptFighter_JabEnter,NativeScriptFighter_RapidJabEnter,NativeScriptFighter_DownTiltEnter,
    NativeScriptFighter_AerialAttackMotion,NativeScriptFighter_AerialAttackEnter,
    NativeScriptFighter_ForwardSmashStart,NativeScriptFighter_ForwardSmashEnter,
    NativeScriptFighter_UpSmashEnter,NativeScriptFighter_DownSmashEnter,
    NativeScriptFighter_Absorb,
    NativeScriptFighter_BodyRebound,
    NativeScriptFighter_DynamicsInteractionStart,
    NativeScriptFighter_AnimationDrivenJumpPhysics,
    NativeScriptFighter_Trail,
    NativeScriptFighter_CatchAllowed,NativeScriptFighter_CatchStart,NativeScriptFighter_CatchFrame,
    NativeScriptFighter_CatchExit,NativeScriptFighter_CatchWaitRelease,
    NativeScriptFighter_CatchPullFrame,NativeScriptFighter_CatchPullAnchor,NativeScriptFighter_CatchPullReady,NativeScriptFighter_CatchPullFollow,
    NativeScriptFighter_ThrowVictimSetup,
    NativeScriptFighter_ThrowPresentation,
    NativeScriptFighter_CaptureWaitSetup,NativeScriptFighter_CaptureWaitEnter,NativeScriptFighter_AirTetherInput,
    NativeScriptFighter_GrabCatch,NativeScriptFighter_ItemCatch,NativeScriptFighter_Grabbed,
    NativeScriptFighter_ActionMetadata,
    NativeScriptFighter_ItemSwingMotion,
    NativeScriptFighter_JabRestart,
    NativeScriptFighter_ShieldStart,NativeScriptFighter_ShieldReflectStart,
    NativeScriptFighter_ShieldTransition,
    NativeScriptFighter_ShieldScale,NativeScriptFighter_ShieldAlpha,
    NativeScriptFighter_RollEnter,NativeScriptFighter_SpotdodgeEnter,
    NativeScriptFighter_DynamicsWind,
    NativeScriptFighter_HeavyThrowEnter,
    NativeScriptFighter_ThrowVictimMotion,
    NativeScriptFighter_ThrowAnimation,NativeScriptFighter_ThrowFinish,
    NativeScriptFighter_HeavyPickupFinish,NativeScriptFighter_HeavyItemDamage,
    NativeScriptFighter_DynamicsGroundCollision,
    NativeScriptFighter_IdleVariants,NativeScriptFighter_MotionIsAngled,
    NativeScriptFighter_WornItemJumpSound,
    NativeScriptFighter_ShieldBreak,
    NativeScriptFighter_WaitInput,NativeScriptFighter_ActionStatus,
    NativeScriptFighter_AuxiliaryItemAttach,
    NativeScriptFighter_AuxiliaryItemVisibilityReset,
    NativeScriptFighter_JabStart,
    NativeScriptFighter_HookCount };
typedef struct NativeScriptFighterHookContract {
    const char* name;
    uint8_t result_type,argument_count;
    uint8_t arguments[2];
    uint16_t record_bytes;
} NativeScriptFighterHookContract;
extern const NativeScriptFighterHookContract nativeScriptFighterHookContracts[NativeScriptFighter_HookCount];
enum { NativeScriptFighterCallback_Damage,NativeScriptFighterCallback_Death1,
    NativeScriptFighterCallback_Death2,NativeScriptFighterCallback_Death3,
    NativeScriptFighterCallback_PreHitlag,NativeScriptFighterCallback_PostHitlag,
    NativeScriptFighterCallback_DealDamage,NativeScriptFighterCallback_StateChange,
    NativeScriptFighterCallback_ReflectHit,NativeScriptFighterCallback_Reflect,
    NativeScriptFighterCallback_HurtboxDetect,NativeScriptFighterCallback_ShieldHit,
    NativeScriptFighterCallback_AccessoryBefore,NativeScriptFighterCallback_AccessoryHitlag,
    NativeScriptFighterCallback_SecondaryDamage,
    NativeScriptFighterCallback_Count };
enum { NATIVE_SCRIPT_FIGHTER_ACTORS=16,NATIVE_SCRIPT_FIGHTER_STATE_BYTES=8192 };

enum { NATIVE_SCRIPT_MOTION_PATCHES=1024 };
typedef struct NativeScriptMotionPatch {
    uint32_t motion,functions[NativeScriptMotion_Count];
    bool accessory_cue;
} NativeScriptMotionPatch;
typedef struct NativeScriptFighterProgram {
    const NativeScriptCompileBank* bank;
    uint64_t identity;
    uint32_t hooks[NativeScriptFighter_HookCount];
    uint32_t callbacks[NativeScriptFighterCallback_Count];
    NativeScriptMotionPatch motions[NATIVE_SCRIPT_MOTION_PATCHES];uint32_t motion_count;
    NativeScriptItemPrograms items;
    NativeScriptEffectPrograms effects;
} NativeScriptFighterProgram;

// Program publication is a transition operation: any live actor of this kind
// prevents replacement. Unsupported root hooks are rejected until their actual
// scheduler sites migrate; no callback is silently discarded.
bool nativeScriptFighter_PrepareProgram(FighterKind,const NativeScriptCompileBank*,
    const NativeScriptMoveLayout* owned_layout,const NativeScriptItemMetadataContext*,
    NativeScriptFighterProgram*,NativeScriptDiagnostic*);
bool nativeScriptFighter_CanPublish(FighterKind);
// Requires CanPublish at the same quiescent transition. Prepared internal
// immutable pointers are rebound; no parsing or late asset checks occur here.
void nativeScriptFighter_PublishProgram(FighterKind,const NativeScriptFighterProgram*);
const NativeScriptFighterProgram* nativeScriptFighter_Program(FighterKind);
bool nativeScriptFighter_HasActors(FighterKind);
bool nativeScriptFighter_ReferencesProgram(const NativeScriptCompileBank*);
bool nativeScriptFighter_Attach(Fighter_GObj*);
void nativeScriptFighter_NewLife(Fighter_GObj*,uint32_t generation);
void nativeScriptFighter_Detach(Fighter_GObj*);
bool nativeScriptFighter_IsAttached(Fighter_GObj*);
// Only the initial attachment callback may allocate costume trees and cloth.
bool nativeScriptFighter_IsLoading(Fighter_GObj*);
bool nativeScriptFighter_HasHook(Fighter_GObj*,uint32_t hook);
NativeScriptVMStatus nativeScriptFighter_Status(Fighter_GObj*);
// A dormant destination must be live and have no suspended/running script
// before common form activation changes its generation and rebinds its VM.
bool nativeScriptFighter_CanActivateForm(Fighter_GObj*);
// Missing static slots return their typed neutral value; Error/SourceRetired
// leaves output untouched and stops native continuation. No stock fallback.
NativeScriptVMStatus nativeScriptFighter_Run(Fighter_GObj*,uint32_t hook,
    const NativeScriptValue* arguments,uint32_t count, NativeScriptEventRecordIO* record,NativeScriptValue* result);
// Bind/clear instance callbacks without installing machine-code pointers.
// UINT32_MAX clears a slot. Native shared-engine assignments supersede bindings.
bool nativeScriptFighter_BindCallback(Fighter_GObj*,uint32_t slot,uint32_t function);
bool nativeScriptFighter_CallbackBound(Fighter_GObj*,uint32_t slot);
bool nativeScriptFighter_RunCallback(Fighter_GObj*,uint32_t slot);
uint32_t nativeScriptFighter_CallbackFunction(Fighter_GObj*,uint32_t slot);
// Pure query of this actor's current interpreted slot. Native handlers never
// compare equal to an authored helper or to None; no pointer enters the VM.
bool nativeScriptFighter_CallbackMatches(Fighter_GObj*,uint32_t slot,uint32_t function,bool*);
void nativeScriptFighter_ResetMotionCallbacks(Fighter_GObj*);
void nativeScriptFighter_SelectMotion(Fighter_GObj*,uint32_t motion);
bool nativeScriptFighter_MotionBound(Fighter_GObj*,uint32_t phase);
bool nativeScriptFighter_RunMotion(Fighter_GObj*,uint32_t phase);
// Bind a same-program root helper to an actual motion phase. Helpers must be
// zero-argument Void functions. The prepared animation-driven jump hook may
// replace Physics after shared jump entry. UINT32_MAX clears; cue is accessory only.
bool nativeScriptFighter_BindMotionPhase(Fighter_GObj*,uint32_t phase,uint32_t function,bool cue);
bool nativeScriptFighter_CanBeginScene(uint64_t epoch);
bool nativeScriptFighter_BeginScene(uint64_t epoch);
struct NativeMatchArena;
bool nativeScriptFighter_AdmitHistory(struct NativeMatchArena*);
bool nativeScriptFighter_Save(void* snapshot,size_t capacity,size_t* written);
bool nativeScriptFighter_ValidateSnapshot(const void* snapshot,size_t bytes);
// Requires restored GObj/Fighter peers; never runs authored callbacks.
bool nativeScriptFighter_Load(const void* snapshot,size_t bytes);
bool nativeScriptFighter_RetireScene(void);
void nativeScriptFighter_Report(void);
bool nativeScriptFighter_View(Fighter_GObj*,uint32_t generation,NativeScriptReceiverView*);
NativeScriptVMStatus nativeScriptFighter_Helper(Fighter_GObj*,uint32_t generation,uint32_t function,
    const NativeScriptValue*,uint32_t count,NativeScriptValue*,NativeScriptDiagnostic*);

#endif
