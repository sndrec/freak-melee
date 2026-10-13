#include <melee/ef/native_script_effect_actor.h>
// These isolated fixtures have no effect actors; scene bookkeeping is empty.
bool nativeScriptEffect_View(HSD_GObj* a,uint32_t g,NativeScriptReceiverView* v)
{(void)a;(void)g;(void)v;return false;}
bool nativeScriptEffect_CanBeginScene(uint64_t e){return e!=0;}
bool nativeScriptEffect_BeginScene(uint64_t e){return e!=0;}
bool nativeScriptEffect_RetireScene(void){return true;}
bool nativeScriptEffect_HasActors(FighterKind k){(void)k;return false;}
