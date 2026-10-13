#include <melee/ft/native_script_binding_signal.h>
#include <melee/ef/native_script_effect_actor.h>
// This fixture has no Effect actors or source signals. Native Item/Article
// execution never succeeds through these absent recipients.
bool nativeScriptBindingSignal_Invoke(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* a,
    uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)a;(void)n;(void)r;(void)d;return false;}
bool nativeScriptEffect_View(HSD_GObj* o,uint32_t g,NativeScriptReceiverView* v)
{(void)o;(void)g;(void)v;return false;}
bool nativeScriptEffect_CanBeginScene(uint64_t epoch){return epoch!=0;}
bool nativeScriptEffect_BeginScene(uint64_t epoch){return epoch!=0;}
bool nativeScriptEffect_RetireScene(void){return true;}
