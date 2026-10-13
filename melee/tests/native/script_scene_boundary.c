#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
// This GObj owner fixture has no interpreted actors or active VM invocation.
bool nativeScriptRuntime_Idle(void){return true;}
bool nativeScriptFighter_RetireScene(void){return true;}

bool nativeScriptFighter_CanBeginScene(uint64_t epoch){return epoch!=0;}
bool nativeScriptFighter_BeginScene(uint64_t epoch){return epoch!=0;}
bool nativeScriptItem_CanBeginScene(uint64_t epoch){return epoch!=0;}
bool nativeScriptItem_BeginScene(uint64_t epoch){return epoch!=0;}
bool nativeScriptItem_RetireScene(void){return true;}

// These lifecycle fixtures create no class-backed model graph. Abort if an
// unexpected active grid tries to resolve one; the connected reservation
// fixture links the real class owner instead.
#include <sysdolphin/baselib/native_class_snapshot.h>
#include <stdlib.h>
NativeClassReference nativeClass_Reference(const void* p,size_t n)
{ (void)p;(void)n;abort(); }
void* nativeClass_Resolve(NativeClassReference ref) { (void)ref;abort(); }
bool nativeClass_TypeKnown(const HSD_ClassInfo* info,size_t bytes)
{ (void)info;(void)bytes;abort(); }
