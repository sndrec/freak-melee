#include "fixture.h"
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/kinds/ftCommon/ftCo_Guard.h>
#include <melee/ft/kinds/ftCommon/ftCo_0A01.h>
#include <melee/ft/ftdevice.h>
#include <melee/ft/ft_0877.h>
#include <melee/gr/stage.h>
#include <melee/ft/native_fighter_target.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/ftchangeparam.h>
#include <melee/ft/ft_0DF0.h>
#include <melee/it/kinds/itsamusgrapple.h>
#include <melee/it/native_article_resource_internal.h>
#include <melee/db/db.h>
#include <melee/pl/player.h>
#include <melee/pl/types.h>
#include <melee/lb/lb_00B0.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/it/types.h>
#include <melee/it/it_26B1.h>
#include <melee/it/inlines.h>
#include <melee/gm/gmvs.h>
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
// Physical graphs are fixture-owned fixed storage, separate from GObj actors.
void HSD_JObjRemoveAll(HSD_JObj* o){(void)o;}
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
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind k){for(unsigned i=0;i<2;++i)if(packages[i].ready&&k==packages[i].target)return &packages[i];return NULL;}
bool nativeScriptBindings_Context(void* o,uint32_t k,uint32_t g,NativeScriptRuntimeContext* c,NativeScriptDiagnostic* d)
{(void)d;if(!nativeCallbackSource_Alive(o,g,k))return false;*c=(NativeScriptRuntimeContext){.source=o,.source_class=k,.generation=g,.actor_id=g};return true;}
#include "source_guards.inc"
#include "native_resource_lookup.inc"
// Descriptor creation observes common-helper inputs here. Separate damage/
// angle/staleness tests remain responsible for those helpers' internal policy.
DbLKind DbLevel=DbLKind_DebugRom;
struct Fighter_804D6524_t* Fighter_804D6524;
float* Fighter_804D6548;
float ftCo_CalcYScaledKnockback(float damage,float scale,float factor)
{(void)factor;return damage/scale;}
float ftCo_800DEEB8(Fighter* fighter,float damage){(void)fighter;return damage;}
StaleMoveTable* Player_GetStaleMoveTableIndexPtr(s32 slot){(void)slot;return NULL;}
Fighter_Part ftParts_GetBoneIndex(Fighter* fighter,Fighter_Part bone){(void)fighter;(void)bone;abort();}
void ftColl_8007ABD0(HitCapsule* hit,u32 damage,Fighter_GObj* object)
{(void)object;hit->damage=(float)damage;hit->unk_count=damage;}
void ftColl_8007AC9C(HitCapsule* hit,int angle,Fighter_GObj* object)
{(void)object;hit->kb_angle=angle;}
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wsign-compare"
#include "native_hitbox_create.inc"
#pragma clang diagnostic pop
// All unrelated native families are explicitly unavailable. Target output
// publication goes through real Execution/Storage, without a successful stub.
bool nativeScriptBindings_Read(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
bool nativeScriptBindings_Write(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,const NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
#define READ(n) bool n(NativeScriptRuntimeContext* c,uint32_t s,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)r;(void)d;return false;}
#define WRITE(n) bool n(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)r;(void)d;return false;}
#define CALL(n) bool n(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t z,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)z;(void)r;(void)d;return false;}
READ(nativeScriptBindingKinetics_Read) WRITE(nativeScriptBindingKinetics_Write) CALL(nativeScriptBindingKinetics_Invoke)
READ(nativeScriptBindingState_Read) CALL(nativeScriptBindingState_Invoke)
READ(nativeScriptBindingCombat_Read) WRITE(nativeScriptBindingCombat_Write) CALL(nativeScriptBindingCombat_Invoke)
CALL(nativeScriptBindingSetup_Invoke) CALL(nativeScriptBindingTransition_Invoke)
READ(nativeScriptBindingItem_Read) WRITE(nativeScriptBindingItem_Write) CALL(nativeScriptBindingItem_Invoke)
bool nativeScriptBindingSetup_Read(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
bool nativeScriptBindingSetup_Write(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,const NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
#include "native_target.inc"
// The unmodified decomp search retains an unused stack-padding declaration.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#include "native_ahead.inc"
#include "native_nearest.inc"
#pragma clang diagnostic pop
bool gm_8016B168(void){return ahead_teams;}
// Stage limits and device identity are controlled fixture leaves; the native
// availability decisions, timer, motion filtering and distance are unchanged.
float Stage_GetBlastZoneLeftOffset(void){return -100;}
float Stage_GetBlastZoneRightOffset(void){return 100;}
float Stage_GetBlastZoneBottomOffset(void){return -100;}
float Stage_GetBlastZoneTopOffset(void){return 100;}
int ftCo_800C06B4(Fighter* fighter){(void)fighter;return 0x7B;}
