#ifndef SOURCE_FORM_TRANSITION_FIXTURE_H
#define SOURCE_FORM_TRANSITION_FIXTURE_H
#include <melee/ft/native_source_package.h>
#include <melee/ft/types.h>
#include <sysdolphin/baselib/gobj.h>
enum { ChangeHealthy,ChangePhysicalRemove,ChangeGeneration,ChangeLogicalRetire,ChangeFault,ChangeDataReplace };
typedef struct ChangeRecord {
    Fighter_GObj* object;FtMotionId motion;MotionFlags flags;
    float frame,rate,blend;Fighter_GObj* target;
} ChangeRecord;
extern NativeSourcePackage package;
extern NativeScriptFighterProgram selected;
extern HSD_GObj object;
extern Fighter fighter;
extern NativeScriptStorageInstance storage;
extern NativeScriptVMStatus actor_status;
extern const NativeScriptProgramIndex* view_program;
extern unsigned change_mode,change_count,start_count,order_count,order[8];
extern ChangeRecord change;
void fixture_select(void);
#endif
