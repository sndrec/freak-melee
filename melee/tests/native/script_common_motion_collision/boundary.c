#include "fixture.h"
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/inlines.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/ftchangeparam.h>
#include <melee/ft/kinds/ftCommon/ftCo_HammerLanding.h>
#include <melee/ft/kinds/ftCommon/ftCo_HammerWait.h>
#include <melee/ft/kinds/ftCommon/ftCo_DownSpot.h>
#include <melee/ft/kinds/ftCommon/ftCo_0C60.h>
#include <melee/pl/plbonuslib.h>
#include <melee/sfx/crowdsfx.h>
#include <melee/ft/ftcpuattack.h>
#include <melee/ft/ftcmdscript.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_0819.h>
#include <melee/ft/kinds/ftPeach/ftpeachspecialhi.h>
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
READ(nativeScriptBindingKinetics_Read) WRITE(nativeScriptBindingKinetics_Write)
READ(nativeScriptBindingCombat_Read) WRITE(nativeScriptBindingCombat_Write) CALL(nativeScriptBindingCombat_Invoke)
READ(nativeScriptBindingPresentation_Read) CALL(nativeScriptBindingSetup_Invoke) CALL(nativeScriptBindingTransition_Invoke)
READ(nativeScriptBindingItem_Read) WRITE(nativeScriptBindingItem_Write) CALL(nativeScriptBindingItem_Invoke)
bool nativeScriptBindingPresentation_Invoke(NativeScriptRuntimeContext* c,uint32_t s,NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
bool nativeScriptBindingSetup_Read(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
bool nativeScriptBindingSetup_Write(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,const NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
#include <melee/mp/mpcoll.h>
#include <melee/ft/kinds/ftCommon/ftCo_Ottotto.h>
#include "native_bodies.inc"
CALL(nativeScriptBindingState_Invoke)
CALL(nativeScriptBindingKinetics_Invoke)
struct Fighter_804D64FC_t* Fighter_804D64FC;
void ftCo_800B4880(Fighter* f,int id){(void)f;(void)id;abort();}
void ftCo_CpuAddAttackCandidate(Fighter* f,int id){(void)f;(void)id;abort();}
bool ftCo_CpuRangedAttackUnsafe(Fighter* f,Fighter* t,float x){(void)f;(void)t;(void)x;abort();}
void ftCo_Wait_IASA(Fighter_GObj* g){(void)g;abort();}
bool ftCo_Fall_IASA_Inner(Fighter_GObj* g){(void)g;abort();}
void ftCommon_UpdateFacing(Fighter* f){(void)f;abort();}
void ftParts_80074A4C(Fighter_GObj* g,int a,int b){(void)g;(void)a;(void)b;abort();}

void ftCommon_8007DA24(Fighter* f){(void)f;abort();}
void ftCommon_8007D60C(Fighter* f){(void)f;abort();}
void ft_80084104(Fighter_GObj* o){(void)o;abort();}
void ft_80083B68(Fighter_GObj* o){(void)o;abort();}
bool ftWallJump_8008169C(HSD_GObj* o){(void)o;abort();}
bool ftCliffCommon_80081298(Fighter_GObj* o){(void)o;abort();}
void ftCo_LandingFallSpecial_Enter(Fighter_GObj* o,bool b,float f){(void)o;(void)b;(void)f;abort();}
bool ftCo_80096CC8(Fighter_GObj* o,int i){(void)o;(void)i;abort();}
bool mpColl_80047AC8(CollData* c,bool (*f)(Fighter_GObj*,enum_t),Fighter_GObj* o){(void)c;(void)f;(void)o;abort();}
bool mpColl_80047E14(CollData* c,bool (*f)(Fighter_GObj*,enum_t),Fighter_GObj* o){(void)c;(void)f;(void)o;abort();}

void ftCo_AppealS_IASA(Fighter_GObj* o){(void)o;abort();}

// Controlled geometry/response leaves. The selected common policies and hooks
// are actual bodies; these leaves do not claim stage or renderer acceptance.
bool mpColl_800471F8(CollData* c){return tether_geometry(c);}
bool ft_80081A00(Fighter_GObj* o){return tether_environment(o);}
static struct Fighter_804D6524_t scale_modifiers;
struct Fighter_804D6524_t* Fighter_804D6524=&scale_modifiers;
bool ftCo_800C5240(Fighter_GObj* o){return tether_hammer(o);}
u32 ftCo_800C54C4(Fighter* f){(void)f;return 0;}
float ftCo_800C548C(Fighter* f){(void)f;return 0;}
void ftCo_800C4E94(Fighter* f){tether_cleanup(f);}
bool ftCo_8009A134(Fighter_GObj* o){return tether_hook(o);}
void Fighter_ChangeMotionState(Fighter_GObj* o,FtMotionId m,MotionFlags f,float a,float s,float b,Fighter_GObj* other){tether_motion(o,m,f,a,s,b,other);}
void ft_8008A2BC(Fighter_GObj* o){tether_neutral(o);}
void ftCommon_ClampGroundVel(Fighter* f,float v){(void)v;f->self_vel.x=1;}
void ftCommon_UnlockECB(Fighter* f){f->ecb_lock=0;}
bool ft_80084A18(Fighter_GObj* o){(void)o;return true;}
bool un_803224DC(s32 a,float b,float c){(void)a;(void)b;(void)c;return false;}
void pl_8003FC44(int p,int b){(void)p;(void)b;}
void ftPe_8011D598(Fighter_GObj* o){(void)o;abort();}

// Unselected direct catalog/kernel branches abort; they cannot fake success.
bool nativeFighterGeometry_ProbeLedge(Fighter_GObj* o,float y,float h){(void)o;(void)y;(void)h;abort();}
void mpCopyCollData(CollData* a,CollData* b,int c){(void)a;(void)b;(void)c;abort();}
Fighter_GObj* ft_80082E3C(Fighter_GObj* o){(void)o;abort();}
bool ft_80082708(Fighter_GObj* o){(void)o;abort();}
HSD_GObjEvent ftData_OnLanding[Ft_Kind_RuntimeCapacity];
