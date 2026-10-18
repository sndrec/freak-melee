#include <sysdolphin/baselib/native_callback_profile.h>
#include <sysdolphin/baselib/native_random_snapshot.h>
#include <sysdolphin/baselib/native_class_snapshot.h>
#include <melee/it/item.h>
#include <melee/gr/stage.h>
#include <stdlib.h>

// Graph/VM fixtures run with profiling disabled. Reaching its timing hooks is
// an unexpected operation. RNG diagnostics observe object lifetime events;
// these fixtures do not initialize the optional trace sink.
bool nativeCallbackProfileEnabled;
NativeCallbackProfileStamp nativeCallbackProfile_Begin(uintptr_t callback,
    NativeCallbackProfileKind kind)
{ (void)callback; (void)kind; abort(); }
void nativeCallbackProfile_End(NativeCallbackProfileStamp stamp)
{ (void)stamp; abort(); }
void nativeRandom_TraceEvent(const char* name,void* caller,const void* object,int value)
{ (void)name; (void)caller; (void)object; (void)value; }
StKind Stage_80225194(void) { return (StKind)0; }

// Scene construction reserves the real optional Ground/Item pools. No grid or
// native item is constructed here; their class leases/removal are unselected.
NativeClassReference nativeClass_Reference(const void* p,size_t n)
{ (void)p; (void)n; abort(); }
void* nativeClass_Resolve(NativeClassReference ref) { (void)ref; abort(); }
bool nativeClass_TypeKnown(const HSD_ClassInfo* info,size_t bytes)
{ (void)info; (void)bytes; abort(); }
void Item_OnUserDataRemove(void* object) { (void)object; abort(); }
