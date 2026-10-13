#ifndef MELEE_EF_NATIVE_SCRIPT_EFFECT_ACTOR_H
#define MELEE_EF_NATIVE_SCRIPT_EFFECT_ACTOR_H
#include <melee/ft/native_script_effect_program.h>
#include <melee/ft/native_script_receiver.h>
#include <melee/ft/native_script_vm.h>

typedef struct NativeScriptEffectOrigin {
    HSD_JObj* creator_root;
    struct _HSD_ClassInfo* info;
    struct FighterBone* parts;
    const struct NativeSourcePackage* package;
    uint32_t part;
    uint32_t on_part;
} NativeScriptEffectOrigin;
enum { NATIVE_SCRIPT_EFFECT_ACTORS=128,NATIVE_SCRIPT_EFFECT_STATE_BYTES=8192 };
bool nativeScriptEffect_CanBeginScene(uint64_t epoch);
bool nativeScriptEffect_BeginScene(uint64_t epoch);
struct NativeMatchArena;
bool nativeScriptEffect_AdmitHistory(struct NativeMatchArena*);
bool nativeScriptEffect_RetireScene(void);
bool nativeScriptEffect_HasActors(FighterKind author);
bool nativeScriptEffect_ReferencesProgram(const NativeScriptCompileBank*);
bool nativeScriptEffect_IsAttached(HSD_GObj*);
const NativeScriptEffectProgram* nativeScriptEffect_Spec(HSD_GObj*);
EF_Effect* nativeScriptEffect_Resolve(HSD_GObj*,uint32_t generation);
bool nativeScriptEffect_Snapshot(HSD_GObj*,uint32_t* generation);
NativeScriptVMStatus nativeScriptEffect_Status(HSD_GObj*);
bool nativeScriptEffect_View(HSD_GObj*,uint32_t generation,NativeScriptReceiverView*);
Fighter_GObj* nativeScriptEffect_Creator(HSD_GObj*,uint32_t generation);
HSD_JObj* nativeScriptEffect_Joint(HSD_GObj*,uint32_t generation,unsigned member);
NativeScriptVMStatus nativeScriptEffect_Run(HSD_GObj*);
// Actual userdata cleanup retires storage for the complete group; its program
// stays pinned until every native member reaches its final userdata cleanup.
void nativeScriptEffect_Detach(HSD_GObj*);
NativeScriptVMStatus nativeScriptEffect_Spawn(NativeScriptRuntimeContext*,uint32_t owner,bool on_part,int32_t raw_part,bool deferred,NativeScriptDiagnostic*);
struct EF_QueuedEffect;
NativeScriptVMStatus nativeScriptEffect_ProcessDeferred(Fighter_GObj*,const struct EF_QueuedEffect*);
#endif
