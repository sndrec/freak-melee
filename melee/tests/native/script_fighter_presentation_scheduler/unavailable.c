#include <melee/ft/native_script_bindings.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/ef/native_script_effect_actor.h>
#include <melee/ef/native_model_effect.h>
// There are no Item or Effect actors in this Fighter-only fixture. The scene
// boundaries reserve no such storage and every actor/view/helper lookup rejects.
bool nativeScriptItem_HasActors(FighterKind k){(void)k;return false;}
bool nativeScriptItem_ReferencesProgram(const NativeScriptCompileBank* p){(void)p;return false;}
bool nativeScriptItem_View(Item_GObj* g,uint32_t n,NativeScriptReceiverView* v){(void)g;(void)n;(void)v;return false;}
const NativeScriptItemProgram* nativeScriptItem_Spec(Item_GObj* g){(void)g;return NULL;}
NativeScriptVMStatus nativeScriptItem_Status(Item_GObj* g){(void)g;return NativeScriptVM_SourceRetired;}
Fighter_GObj* nativeScriptItem_Creator(Item_GObj* g){(void)g;return NULL;}
Fighter_GObj* nativeScriptItem_AttachmentOwner(Item_GObj* g){(void)g;return NULL;}
bool nativeScriptItem_CanBeginScene(uint64_t e){return e!=0;}
bool nativeScriptItem_BeginScene(uint64_t e){return e!=0;}
bool nativeScriptItem_RetireScene(void){return true;}
bool nativeScriptEffect_HasActors(FighterKind k){(void)k;return false;}
bool nativeScriptEffect_ReferencesProgram(const NativeScriptCompileBank* p){(void)p;return false;}
bool nativeScriptEffect_View(HSD_GObj* g,uint32_t n,NativeScriptReceiverView* v){(void)g;(void)n;(void)v;return false;}
const NativeScriptEffectProgram* nativeScriptEffect_Spec(HSD_GObj* g){(void)g;return NULL;}
EF_Effect* nativeScriptEffect_Resolve(HSD_GObj* g,uint32_t n){(void)g;(void)n;return NULL;}
Fighter_GObj* nativeScriptEffect_Creator(HSD_GObj* g,uint32_t n){(void)g;(void)n;return NULL;}
bool nativeScriptEffect_CanBeginScene(uint64_t e){return e!=0;}
bool nativeScriptEffect_BeginScene(uint64_t e){return e!=0;}
bool nativeScriptEffect_RetireScene(void){return true;}
bool nativeModelEffect_Snapshot(HSD_GObj* g,uint32_t* n){(void)g;(void)n;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* g,uint32_t n){(void)g;(void)n;return NULL;}
#define CALL(name) bool name(NativeScriptRuntimeContext*c,uint32_t s,const NativeScriptValue*a,uint32_t n,NativeScriptValue*v,NativeScriptDiagnostic*d){(void)c;(void)s;(void)a;(void)n;(void)v;(void)d;return false;}
CALL(nativeScriptBindingKinetics_Invoke) CALL(nativeScriptBindingState_Invoke)
CALL(nativeScriptBindingSetup_Invoke) CALL(nativeScriptBindingCombat_Invoke)
CALL(nativeScriptBindingTransition_Invoke)
