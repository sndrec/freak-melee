#include "fixture.h"
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/kinds/ftCommon/ftCo_AppealS.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallAerial.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallSpecial.h>
#include <melee/ft/kinds/ftCommon/ftCo_Landing.h>
#include <melee/ft/kinds/ftCommon/ftCo_LandingAir.h>
#include <melee/pl/player.h>
#include <melee/db/db.h>
#include <melee/it/itgroundcoll.h>
#include <melee/ft/native_fighter_target.h>
#include <melee/ft/fighter.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/it/types.h>
#include <melee/ef/native_model_effect.h>
#include <melee/ef/efsync.h>
#include <sysdolphin/baselib/gobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#pragma clang section data = ""
void OSReport(char* f,...){va_list args;va_start(args,f);vfprintf(stderr,f,args);va_end(args);}
__declspec(noreturn) void __assert(char* f,u32 l,char* r){fprintf(stderr,"%s:%u %s\n",f,(unsigned)l,r);abort();}
__declspec(noreturn) void HSD_Panic(char* f,u32 l,char* r){__assert(f,l,r);}
bool nativeFighterPool_CanBeginScene(uint64_t e){return e!=0;}
bool nativeFighterPool_BeginScene(uint64_t e){return e!=0;}
bool nativeFighterPool_RetireScene(void){return true;}
bool nativeModelEffect_Snapshot(HSD_GObj* o,uint32_t* g){(void)o;(void)g;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* o,uint32_t g){(void)o;(void)g;return NULL;}
const EF_ModelRecipe* efSync_ModelRecipe(s32 id){(void)id;return NULL;}
void HSD_JObjRemoveAll(HSD_JObj* o){(void)o;abort();}
void HSD_LObjRemoveAll(HSD_LObj* o){(void)o;abort();}
void HSD_CObjFree(HSD_CObj* o){(void)o;abort();}
void HSD_FogFree(HSD_Fog* o){(void)o;abort();}
void HSD_LObj_803668EC(HSD_LObj* o){(void)o;abort();}
HSD_CObj* HSD_CObjGetCurrent(void){abort();}
void HSD_LObjSetupInit(HSD_CObj* o){(void)o;abort();}
void HSD_JObjDispAll(HSD_JObj* o,Mtx m,u32 f,u32 d){(void)o;(void)m;(void)f;(void)d;abort();}
void HSD_FogSet(HSD_Fog* o){(void)o;abort();}
bool HSD_CObjSetCurrent(HSD_CObj* o){(void)o;abort();}
void HSD_CObjEndCurrent(void){abort();}
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind k){(void)k;return NULL;}
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind k){return k==target_package.target?&target_package:NULL;}
// All unrelated native families are explicitly unavailable. Target output
// publication goes through real Execution/Storage, without a successful stub.
#define READ(n) bool n(NativeScriptRuntimeContext* c,uint32_t s,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)r;(void)d;return false;}
#define WRITE(n) bool n(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)r;(void)d;return false;}
#define CALL(n) bool n(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t z,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)z;(void)r;(void)d;return false;}
READ(nativeScriptBindingKinetics_Read) WRITE(nativeScriptBindingKinetics_Write) CALL(nativeScriptBindingKinetics_Invoke)
READ(nativeScriptBindingCombat_Read) WRITE(nativeScriptBindingCombat_Write) CALL(nativeScriptBindingCombat_Invoke)
READ(nativeScriptBindingPresentation_Read) CALL(nativeScriptBindingSetup_Invoke) CALL(nativeScriptBindingTransition_Invoke)
READ(nativeScriptBindingItem_Read) WRITE(nativeScriptBindingItem_Write) CALL(nativeScriptBindingItem_Invoke)
bool nativeScriptBindingPresentation_Invoke(NativeScriptRuntimeContext* c,uint32_t s,NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
bool nativeScriptBindingSetup_Read(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
bool nativeScriptBindingSetup_Write(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,const NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
#include "native_target.inc"
#include "state_absent.inc"
Gm_PKind Player_GetPlayerSlotType(s32 index){(void)index;abort();}
void Fighter_ChangeMotionState(Fighter_GObj* o,FtMotionId motion,MotionFlags flags,float frame,float rate,float blend,Fighter_GObj* other)
{taunt_change(o,motion,flags,frame,rate,blend,other);}
void db_80225DD8(Item_GObj* a,Fighter_GObj* b){(void)a;(void)b;abort();}
bool it_8026D9A0(Item_GObj* a){(void)a;abort();}
bool it_8026DA08(Item_GObj* a){(void)a;abort();}
