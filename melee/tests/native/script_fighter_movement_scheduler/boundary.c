#include <melee/ft/kinds/ftCommon/ftCo_ItemThrow.h>
#include "fixture.h"
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/native_fighter_lifetime.h>
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
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind k){return k==movement_package.target?&movement_package:NULL;}
bool nativeScriptBindings_Context(void* o,uint32_t k,uint32_t g,NativeScriptRuntimeContext* c,NativeScriptDiagnostic* d)
{(void)d;if(!nativeCallbackSource_Alive(o,g,k))return false;*c=(NativeScriptRuntimeContext){.source=o,.source_class=k,.generation=g,.actor_id=g};return true;}
#include "source_guards.inc"
// All unrelated native families are explicitly unavailable. Target output
// publication goes through real Execution/Storage, without a successful stub.
bool nativeScriptBindings_Read(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
bool nativeScriptBindings_Write(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,const NativeScriptValue* r,NativeScriptDiagnostic* d){(void)v;(void)d;if(n||s!=NativeScriptBinding_PositionX||r->type!=NativeScriptType_F32)return false;((Fighter*)((HSD_GObj*)c->source)->user_data)->cur_pos.x=r->as.f32;++write_count;trace=trace*10+(unsigned)r->as.f32;bool fail=mutation_mode==5&&(!mutation_point||mutation_point==(unsigned)r->as.f32);movement_mutate(c->source,(unsigned)r->as.f32);return !fail;}
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

#include <melee/ft/ft_0892.h>
#include <melee/ft/kinds/ftCommon/ftCo_DownSpot.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallAerial.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallSpecial.h>
#include <melee/ft/kinds/ftPeach/ftpeachfloat.h>
#include <melee/ft/kinds/ftCommon/inlines.h>
#include <melee/it/native_held_article.h>
#include <sysdolphin/baselib/controller.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_0877.h>
#include <melee/ft/kinds/ftLink/ftlinkattackair.h>
#include <melee/ft/kinds/ftCLink/ftclink.h>
#include <melee/it/kinds/itpeachparasol.h>
ftCommonData common_data;ftCommonData* p_ftCommonData=&common_data;
void ftCo_DownSpot_Enter(Fighter_GObj* o){(void)o;trace=trace*10+7;}
bool ftCo_800C5240(Fighter_GObj* o){(void)o;return false;}
void ftCo_800C4ED8(Fighter_GObj* o){(void)o;abort();}
void ftCommon_8007D7FC(Fighter* f){trace=trace*10+1;f->ground_or_air=GA_Ground;}
void Fighter_ChangeMotionState(Fighter_GObj* o,FtMotionId motion,MotionFlags flags,float frame,float rate,float blend,Fighter_GObj* donor){(void)flags;(void)frame;(void)rate;(void)donor;trace=trace*10+3;((Fighter*)o->user_data)->motion_id=motion;((Fighter*)o->user_data)->cur_pos.z=blend;if(invoke_grounded)movement_grounded(o);}
bool ftCo_8008A698(Fighter* f){(void)f;return dead_fall;}
struct Fighter_WaitAnimData* ftData_80085FD4(Fighter* f,FtMotionId id){(void)f;(void)id;static struct Fighter_WaitAnimData state={.x8=1};return &state;}
void ftCo_8008A6D8(Fighter_GObj* o,s32 id){(void)o;(void)id;trace=trace*10+4;}
void ftCommon_8007EFC0(Fighter* f,u32 value){(void)f;(void)value;trace=trace*10+5;}
void ftLk_AttackAir_800EB3BC(Fighter_GObj* o){(void)o;++donor_calls;}
void ftCl_Init_8014919C(Fighter_GObj* o){(void)o;++donor_calls;}
bool nativeHeldArticle_NotifyNeutral(Fighter_GObj* o,bool* claimed){(void)o;++donor_calls;*claimed=true;return true;}
void it_802BDB94(Item_GObj* o){(void)o;++donor_calls;}
ItemKind itGetKind(Item_GObj* o){return ((Item*)o->user_data)->kind;}
void ftPe_8011BB6C(Fighter_GObj* o,bool manual){(void)o;(void)manual;++float_calls;}
void ftCo_Fall_Enter(Fighter_GObj* o){(void)o;abort();}
void ftCo_FallAerial_Enter(Fighter_GObj* o){(void)o;abort();}
void ftCo_800968C8(Fighter_GObj* o){(void)o;++donor_calls;}
void ftCo_80095744(Fighter_GObj* o,bool* flag,struct NativeItemLifetime* retained){(void)o;(void)flag;(void)retained;++drop_calls;}
void ftAnim_8006EBA4(Fighter_GObj* o){++animation_calls;movement_mutate(o,99);}
void pl_8003FE1C(int p,int sub){(void)p;(void)sub;}
void pl_80040330(int p,int sub,float value){(void)p;(void)sub;(void)value;}
#include "native_movement.inc"

#include "head_reference.inc"

#include <melee/ft/kinds/ftCommon/ftCo_Landing.h>
#include <melee/ft/kinds/ftCommon/ftCo_LandingAir.h>
bool ft_80082708(Fighter_GObj* o){(void)o;abort();}
bool ft_800827A0(Fighter_GObj* o){(void)o;abort();}
bool ft_80082888(Fighter_GObj* o,ftCollisionBox* b){(void)o;(void)b;abort();}
bool ft_800824A0(Fighter_GObj* o,ftCollisionBox* b){(void)o;(void)b;abort();}
GroundOrAir ft_80081D0C(Fighter_GObj* o){(void)o;abort();}
void ftCo_80096900(Fighter_GObj* o,int a,int b,bool c,float d,float e){(void)o;(void)a;(void)b;(void)c;(void)d;(void)e;abort();}
void ftCo_800969D8(Fighter_GObj* o,int a,int b,int c,float d,float e,float f){(void)o;(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;abort();}
void ftCo_Landing_Enter(Fighter_GObj* o,FtMotionId id,bool b,MotionFlags flags,float frame,float rate){(void)o;(void)id;(void)b;(void)flags;(void)frame;(void)rate;abort();}
void ftCo_Landing_Enter_Basic(Fighter_GObj* o){(void)o;abort();}
void ftCo_LandingFallSpecial_Enter(Fighter_GObj* o,bool b,float f){(void)o;(void)b;(void)f;abort();}
void ftCo_LandingAir_EnterWithMsidLag(Fighter_GObj* o,FtMotionId id,float f){(void)o;(void)id;(void)f;abort();}
