#include <melee/ft/native_source_package.h>
#include <melee/ft/ftdata.h>
// Item-only binding oracle: no selected Fighter source, actor, data or motions.
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];
MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind)
{(void)kind;return NULL;}
const NativeScriptFighterProgram* nativeScriptFighter_Program(FighterKind kind)
{(void)kind;return NULL;}
NativeScriptVMStatus nativeScriptFighter_Status(Fighter_GObj* object)
{(void)object;return NativeScriptVM_SourceRetired;}
bool nativeScriptFighter_View(Fighter_GObj* object,uint32_t generation,NativeScriptReceiverView* view)
{(void)object;(void)generation;(void)view;return false;}
