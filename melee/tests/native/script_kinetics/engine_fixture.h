#ifndef SCRIPT_KINETICS_ENGINE_FIXTURE_H
#define SCRIPT_KINETICS_ENGINE_FIXTURE_H

#include <melee/ft/types.h>
#include <melee/ft/native_fighter_runtime.h>
#include <sysdolphin/baselib/gobj.h>

void scriptKineticsFixture_Reset(void);
void scriptKineticsFixture_Definition(const NativeFighterDefinition*);
void scriptKineticsFixture_RetireAt(unsigned stage,unsigned mode);
#ifdef SCRIPT_FIXTURE_SOURCE_ACTOR
#include <melee/ft/native_source_package.h>
NativeScriptCompileBank* scriptKineticsFixture_OwnSource(Fighter*,const NativeScriptCompileBank*,NativeScriptStorageInstance*);
void scriptKineticsFixture_AddSource(Fighter*,NativeScriptStorageInstance*);
NativeSourcePackage* scriptKineticsFixture_Source(void);
extern NativeScriptVMStatus script_fixture_source_status;
extern const NativeScriptProgramIndex* script_fixture_source_view_program;
#endif
void ftAnim_8006EBA4(HSD_GObj*);
extern unsigned script_kinetic_stage_count,script_kinetic_stages[16];
extern unsigned script_kinetic_sounds;

#endif
