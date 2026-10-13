#ifndef SCRIPT_JUMP_BOUNDARY_H
#define SCRIPT_JUMP_BOUNDARY_H
#include <melee/ft/native_source_package.h>
#include <melee/ft/types.h>
#include <sysdolphin/baselib/gobj.h>
extern NativeSourcePackage jump_package;
extern NativeScriptStorageInstance jump_storage;
extern NativeScriptVM jump_vm;
extern NativeScriptRuntimeContext jump_context;
extern Fighter jump_actor,jump_reference;
extern HSD_GObj jump_object,jump_reference_object;
extern HSD_JObj jump_root;
extern NativeScriptVMStatus jump_status;
extern unsigned jump_changes,jump_reset,jump_sounds,jump_worn,jump_dispatches;
extern unsigned jump_stage,jump_stop_at,jump_fault_at,jump_retire_at;
extern bool jump_hook;
extern char jump_order[64];
void jumpFixture_Reset(void);
void jumpFixture_Retire(unsigned mode);
NativeScriptVMStatus jumpFixture_Run(uint32_t function);
#endif
