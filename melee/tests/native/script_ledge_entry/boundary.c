#include "fixture.h"
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/kinds/ftCommon/ftCo_JumpAerial.h>
#include <melee/ft/kinds/ftCommon/ftCo_HammerWait.h>
#include <melee/ft/ftcmdscript.h>
#include <melee/ft/kinds/ftCommon/ftCo_0A01.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/ft/kinds/ftDonkey/forward.h>
#include <melee/ft/kinds/ftKirby/forward.h>
#include <melee/ft/kinds/ftPeach/forward.h>
#include <melee/ft/kinds/ftKoopa/forward.h>
#include <sysdolphin/baselib/random.h>
#include <melee/cm/camera.h>
#include <melee/gr/grlib.h>
#include <melee/gr/stage.h>
#include <melee/sc/types.h>
#include <melee/pl/player.h>
#include <melee/lb/lb_013B.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/rumble.h>
#include <melee/it/itgroundcoll.h>
#include <melee/mp/mpcoll.h>

#include <melee/it/itcoll.h>
#include <melee/it/it_2725.h>
#include <melee/it/inlines.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lbcollision.h>
#include <melee/db/db.h>

#include <melee/ft/fighter.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/it/types.h>
#include <melee/ef/native_model_effect.h>
#include <melee/ef/efsync.h>
#include <sysdolphin/baselib/gobj.h>
#include <stdio.h>
#include <stdlib.h>
#pragma clang section data = ""
void OSReport(char* f,...){(void)f;}
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
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind k){return k==ledge_package.target?&ledge_package:NULL;}
bool nativeScriptBindings_Context(void* o,uint32_t k,uint32_t g,NativeScriptRuntimeContext* c,NativeScriptDiagnostic* d)
{(void)d;if(!nativeCallbackSource_Alive(o,g,k))return false;*c=(NativeScriptRuntimeContext){.source=o,.source_class=k,.generation=g,.actor_id=g};return true;}
#include "source_guards.inc"
// All unrelated native families are explicitly unavailable. Target output
// publication goes through real Execution/Storage, without a successful stub.
bool nativeScriptBindings_Read(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
bool nativeScriptBindings_Write(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,const NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
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

CALL(nativeScriptBindingState_Invoke)
#include <melee/ft/ft_0877.h>
#include <melee/ft/ft_0881.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftaction.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftcliffcommon.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/ft/kinds/ftCommon/ftCo_Attack100.h>
#include <melee/ef/efasync.h>
#include <melee/mp/mplib.h>
#include <melee/pl/plbonuslib.h>
#include <stdarg.h>
#include <string.h>
ftCommonData common_data;ftCommonData* p_ftCommonData=&common_data;
void Fighter_ChangeMotionState(Fighter_GObj* o,FtMotionId motion,MotionFlags flags,float frame,float rate,float blend,Fighter_GObj* donor){
 if(motion!=252||flags||frame!=0||rate!=1||blend!=0||donor)abort();
 ((Fighter*)o->user_data)->motion_id=motion;ledge_observe(o,1);
}
void ftAnim_8006E9B4(Fighter_GObj* o){ledge_observe(o,2);}
void ftAction_80073240(Fighter_GObj* o){ledge_observe(o,3);}
void ftAnim_800707B0(Fighter_GObj* o){ledge_observe(o,4);}
void ftCo_800DB500(Fighter_GObj* o){ledge_observe(o,5);}
void ftCo_Fall_Enter(Fighter_GObj* o){((Fighter*)o->user_data)->motion_id=29;ledge_observe(o,6);}
bool mpLib_80054ED8(int line){(void)line;return valid_floor;}
void mpLib_80053ECC_Floor(int line,Vec3* v){(void)line;*v=(Vec3){100.25f,-5.75f,3};}
void mpLib_80053DA4_Floor(int line,Vec3* v){(void)line;*v=(Vec3){-200.5f,7.125f,-4};}
void ft_800881D8(Fighter* f,int id,u8 vol,u8 pan){if(id!=42||vol!=127||pan!=64)abort();ledge_observe(f->gobj,7);}
void ftCommon_8007EBAC(Fighter* f,u32 pattern,u32 duration){if(pattern!=12||duration)abort();ledge_observe(f->gobj,8);}
void efAsync_Spawn(HSD_GObj* o,void* queue,u32 kind,u32 id,HSD_JObj* joint,...){
 if(queue!=&((Fighter*)o->user_data)->x60C||kind!=2||id!=1052||joint)abort();
 va_list ap;va_start(ap,joint);effect_position=*va_arg(ap,Vec3*);va_end(ap);ledge_observe(o,9);
}
void ft_PlaySFX(Fighter* f,enum_t id,u8 vol,u8 pan){if(id!=4||vol!=127||pan!=64)abort();ledge_observe(f->gobj,10);}
bool un_803224DC(u32 id,float x,float knockback){(void)id;(void)x;(void)knockback;return false;}
void pl_8003FC44(int p,int sub){(void)p;(void)sub;}
HSD_GObj* ft_80082E3C(HSD_GObj* o){(void)o;return NULL;}
void pl_80040048(int p,int sub){(void)p;(void)sub;}
void pl_8003FFDC(int a,int b,int c,int d,int e){(void)a;(void)b;(void)c;(void)d;(void)e;}
void mpCollSetFacingDir(CollData* data,int mode){(void)data;(void)mode;}
bool mpColl_800471F8(CollData* data){data->cur_pos.x+=1;return valid_floor;}
bool mpColl_800473CC(CollData* data){data->cur_pos.y+=1;return valid_floor;}
bool ft_80081A00(Fighter_GObj* o){ledge_observe(o,11);return false;}
bool ftCo_800C5240(Fighter_GObj* o){(void)o;return false;}
bool ftCo_800D730C(Fighter_GObj* o,bool input){if(input)abort();ledge_observe(o,12);return valid_floor;}
bool ftCo_JumpAerial_CheckInput(Fighter_GObj* o,bool input){if(input)abort();ledge_observe(o,12);return valid_floor;}
#include "native_ledge.inc"
#include "reference_ledge.inc"
