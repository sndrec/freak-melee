#include <melee/ft/native_script_binding_actions.h>
#include <melee/ft/native_script_bindings.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_0819.h>
#include <melee/ft/ftcliffcommon.h>
#include <melee/ft/ftwalljump.h>
#include <melee/ft/types.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallSpecial.h>
#include <melee/ft/kinds/ftCommon/ftCo_Landing.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/mp/mpcoll.h>
#include <melee/ef/native_model_effect.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static Fighter fighter;static HSD_GObj object;static HSD_GObj* heads[32];HSD_GObj** HSD_GObjPLinkHead=heads;
static unsigned checks,trace[16],trace_count;static bool contact,interrupt,wall,cliff,retire;
static bool allow;static float lag;static bool observed_allow;static float observed_lag;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"collision %u: %s\n",__LINE__,#x);exit(1);}}while(0)
static void log_step(unsigned value){if(trace_count>=16)abort();trace[trace_count++]=value;}
bool nativeModelEffect_Snapshot(HSD_GObj* o,uint32_t* g){(void)o;(void)g;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* o,uint32_t g){(void)o;(void)g;return NULL;}
bool nativeScriptBindings_SourceAlive(NativeScriptRuntimeContext* c){return nativeCallbackSource_Alive(c->source,c->generation,c->source_class);}
bool ftCo_80096CC8(Fighter_GObj* o,int line){(void)o;(void)line;return false;}
static bool probe(CollData* c,unsigned step){log_step(step);c->cur_pos=(Vec3){8,9,10};if(retire)++fighter.x8_spawnNum;return contact;}
bool mpColl_80047AC8(CollData* c,bool (*fn)(HSD_GObj*,int),HSD_GObj* o){(void)fn;(void)o;return probe(c,2);}
bool mpColl_80047E14(CollData* c,bool (*fn)(HSD_GObj*,int),HSD_GObj* o){(void)fn;(void)o;return probe(c,3);}
void mpCollSetFacingDir(CollData* c,int dir){(void)c;CHECK(dir==((fighter.facing_dir<0)?-1:1));log_step(1);}
bool ft_80081A00(HSD_GObj* o){(void)o;log_step(4);return interrupt;}
bool ftWallJump_8008169C(HSD_GObj* o){(void)o;log_step(6);return wall;}
bool ftCliffCommon_80081298(HSD_GObj* o){(void)o;log_step(7);return cliff;}
void ftCo_LandingFallSpecial_Enter(HSD_GObj* o,bool enabled,float frames){(void)o;log_step(5);observed_allow=enabled;observed_lag=frames;}
bool ft_80082578(HSD_GObj* o){(void)o;log_step(8);return contact;}
bool ft_800827A0(HSD_GObj* o){(void)o;log_step(9);return contact;}
void ftCo_Fall_Enter(HSD_GObj* o){(void)o;log_step(10);}
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#include "actual_collision.h"
#pragma clang diagnostic pop
static void landing(HSD_GObj* o){ftCo_LandingFallSpecial_Enter(o,allow,lag);}
static NativeScriptRuntimeContext context;
static NativeScriptDiagnostic diagnostic;
static void setup(unsigned mask)
{
    fighter=(Fighter){0};object=(HSD_GObj){.user_data=&fighter};fighter.gobj=&object;fighter.x8_spawnNum=17;heads[HSD_GOBJ_PLINK_FIGHTER]=&object;
    fighter.cur_pos=(Vec3){1,2,3};fighter.coll_data.cur_pos=(Vec3){4,5,6};fighter.facing_dir=mask&1?-1:1;fighter.x2064_ledgeCooldown=mask&2;fighter.x2224_b2=(mask>>2)&1;
    contact=mask&8;interrupt=mask&16;wall=mask&32;cliff=mask&64;allow=mask&128;lag=(float)mask*.125f;
    observed_lag=-1;observed_allow=false;retire=false;trace_count=0;context=(NativeScriptRuntimeContext){.source=&object,.source_class=NativeCallbackSource_Fighter,.generation=17};
}
int main(void)
{
    for(unsigned mask=0;mask<256;++mask){setup(mask);Fighter initial=fighter;ft_800831CC(&object,ftCo_80096CC8,landing);Fighter expected=fighter;unsigned expected_trace[16],count=trace_count;memcpy(expected_trace,trace,sizeof(trace));float expected_lag=observed_lag;bool expected_allow=observed_allow;
        fighter=initial;trace_count=0;observed_lag=-1;observed_allow=false;NativeScriptValue args[]={{.type=NativeScriptType_Bool,.as.u64=allow},{.type=NativeScriptType_F32,.as.f32=lag}},out={.type=NativeScriptType_Void};
        CHECK(nativeScriptBindingCollision_Invoke(&context,NativeScriptBinding_CollisionAirSpecialLanding,args,2,&out,&diagnostic));CHECK(!memcmp(&fighter,&expected,sizeof(fighter)));CHECK(trace_count==count&&!memcmp(trace,expected_trace,count*sizeof(unsigned)));CHECK(observed_lag==expected_lag&&observed_allow==expected_allow);
    }
    for(unsigned c=0;c<2;++c){setup(c?8:0);ft_80084104(&object);unsigned expected[16],count=trace_count;memcpy(expected,trace,sizeof(trace));trace_count=0;NativeScriptValue out={.type=NativeScriptType_Void};CHECK(nativeScriptBindingCollision_Invoke(&context,NativeScriptBinding_CollisionGroundStandard,NULL,0,&out,&diagnostic));CHECK(trace_count==count&&!memcmp(trace,expected,count*sizeof(unsigned)));trace_count=0;CHECK(nativeScriptBindingCollision_Invoke(&context,NativeScriptBinding_CollisionAirStandard,NULL,0,&out,&diagnostic));CHECK(trace_count==1&&trace[0]==8);}
    setup(8);retire=true;NativeScriptValue args[]={{.type=NativeScriptType_Bool},{.type=NativeScriptType_F32,.as.f32=5}},out={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};CHECK(!nativeScriptBindingCollision_Invoke(&context,NativeScriptBinding_CollisionAirSpecialLanding,args,2,&out,&diagnostic));CHECK(fighter.cur_pos.x==1&&trace_count==2&&out.type==NativeScriptType_U64&&out.as.u64==UINT64_MAX);
    printf("{\"status\":\"PASS actual collision body ordering\",\"checks\":%u}\n",checks);return 0;
}
// No source metadata is published in this controlled stock collision oracle.
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind)
{(void)kind;return NULL;}
const NativeScriptMoveLayout* nativeScriptBindings_SourceFighterLayout(NativeScriptRuntimeContext* context,NativeScriptDiagnostic* diagnostic)
{(void)context;(void)diagnostic;return NULL;}
