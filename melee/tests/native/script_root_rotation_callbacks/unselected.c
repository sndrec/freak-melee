#include <melee/ft/native_script_bindings.h>
#include <melee/ft/ftcommon.h>
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/jobj.h>
#include <stdlib.h>

// The selected raw-root getter/setter never uses these unrelated dispatch paths.
// Fail loudly if a future case enters one rather than pretending it succeeded.
HSD_JObjInfo hsdJObj;
bool hsdObjIsDescendantOf(HSD_Obj* object,HSD_ClassInfo* type)
{(void)object;(void)type;abort();}
const struct NativeScriptMoveLayout* nativeScriptBindings_SourceFighterLayout(
    NativeScriptRuntimeContext* context,NativeScriptDiagnostic* diagnostic)
{(void)context;(void)diagnostic;abort();}
void ftCommon_8007F76C(Fighter_GObj* object){(void)object;abort();}
void ftCommon_8007F5CC(Item_GObj* object,bool visible){(void)object;(void)visible;abort();}
void ftCommon_UnlockECB(Fighter* fighter){(void)fighter;abort();}
