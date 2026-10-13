#include <melee/ft/native_script_binding_ledge.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ftcliffcommon.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/kinds/ftCommon/ftCo_JumpAerial.h>
#include <melee/pl/plbonuslib.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Fighter actor,other;static HSD_GObj object,other_object;static HSD_GObj* heads[32];
HSD_GObj** HSD_GObjPLinkHead=heads;
static ftCommonData common;ftCommonData* p_ftCommonData=&common;
static NativeSourcePackage vacant_source;
static unsigned checks,comparisons,trace[16],trace_count;
static bool contact,interrupt,occupied,captured,jump,retire,owned;
static int observed_mode;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"ledge %u: %s\n",__LINE__,#x);exit(1);}}while(0)
static void log_step(unsigned step){CHECK(trace_count<16);trace[trace_count++]=step;}
bool nativeModelEffect_Snapshot(HSD_GObj* g,uint32_t* n){(void)g;(void)n;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* g,uint32_t n){(void)g;(void)n;return NULL;}
bool nativeScriptBindings_SourceAlive(NativeScriptRuntimeContext* c)
{return c&&nativeCallbackSource_Alive(c->source,c->generation,c->source_class)&&actor.gobj==c->source;}
const struct NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind)
{(void)kind;return owned?&vacant_source:NULL;}
const NativeScriptMoveLayout* nativeScriptBindings_SourceFighterLayout(NativeScriptRuntimeContext* c,NativeScriptDiagnostic* d)
{(void)c;(void)d;return NULL;} // Selected source actor is genuinely absent here.
void mpCollSetFacingDir(CollData* c,int mode){(void)c;observed_mode=mode;log_step(1);}
bool mpColl_800471F8(CollData* c){log_step(2);c->cur_pos.x+=.25f;return contact;}
bool mpColl_800473CC(CollData* c){log_step(3);c->cur_pos.y-=.5f;return contact;}
bool ft_80081A00(HSD_GObj* g){(void)g;log_step(4);if(retire)++actor.x8_spawnNum;return interrupt;}
Fighter_GObj* ft_80082E3C(Fighter_GObj* g){(void)g;log_step(5);return occupied?&other_object:NULL;}
void pl_80040048(int p,int b){(void)p;(void)b;log_step(6);}
void pl_8003FFDC(int a,int b,int c,int d,int e){(void)a;(void)b;(void)c;(void)d;(void)e;log_step(7);}
void ftCliffCommon_80081370(Fighter_GObj* g){(void)g;log_step(8);actor.motion_id=252;if(retire)++actor.x8_spawnNum;}
bool ftCo_800C5240(Fighter_GObj* g){(void)g;log_step(9);return captured;}
bool ftCo_800D730C(Fighter_GObj* g,bool input){(void)g;CHECK(!input);log_step(10);if(retire)++actor.x8_spawnNum;return jump;}
bool ftCo_JumpAerial_CheckInput(Fighter_GObj* g,bool input){(void)g;CHECK(!input);log_step(11);if(retire)++actor.x8_spawnNum;return jump;}
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#include "actual_ledge.h"
#pragma clang diagnostic pop
static NativeScriptRuntimeContext context;static NativeScriptDiagnostic diagnostic;
static void setup(unsigned mask)
{
    memset(&actor,0,sizeof(actor));memset(&other,0,sizeof(other));memset(heads,0,sizeof(heads));
    object=(HSD_GObj){.user_data=&actor};other_object=(HSD_GObj){.user_data=&other};
    heads[HSD_GOBJ_PLINK_FIGHTER]=&object;actor.gobj=&object;actor.x8_spawnNum=17;actor.kind=Ft_Kind_Mario;
    common.x480=.25f;actor.cur_pos=(Vec3){1,2,3};actor.coll_data.cur_pos=(Vec3){4,5,6};
    actor.x2064_ledgeCooldown=mask&1;actor.x2224_b2=(mask>>1)&1;
    actor.input.lstick[0].y=mask&2?-.5f:0;
    actor.coll_data.env_flags=mask&4?Collide_LeftLedgeGrab:0;actor.is_sandbag=(mask>>3)&1;
    actor.can_multijump=(mask>>4)&1;contact=mask&8;interrupt=mask&16;occupied=mask&32;
    captured=mask&64;jump=mask&128;other.mv.co.cliff.ledge_id=37;
    retire=owned=false;trace_count=0;observed_mode=77;
    context=(NativeScriptRuntimeContext){.source=&object,.source_class=NativeCallbackSource_Fighter,.generation=17};
}
int main(void)
{
    const uint32_t ids[]={NativeScriptBinding_CollisionCheckGroundLedge,NativeScriptBinding_CollisionCliffCatch,NativeScriptBinding_ControlsTryAerialJump};
    for(unsigned method=0;method<3;++method)for(unsigned mask=0;mask<256;++mask)for(int mode=-1;mode<=1;++mode){
        setup(mask);Fighter initial=actor;bool expected=method==0?ft_CheckGroundAndLedge(&object,mode):method==1?ftCliffCommon_80081298(&object):ftCo_800CB870(&object);
        Fighter after=actor;unsigned steps[16],count=trace_count;int direction=observed_mode;memcpy(steps,trace,sizeof(steps));
        actor=initial;trace_count=0;observed_mode=77;
        NativeScriptValue arg={.type=NativeScriptType_I32,.as.i64=mode},result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
        CHECK(nativeScriptBindings_Invoke(&context,ids[method],NULL,0,&arg,method==0?1:0,&result,&diagnostic));
        CHECK(result.type==NativeScriptType_Bool&&result.as.u64==(uint64_t)expected);
        CHECK(!memcmp(&actor,&after,sizeof(actor))&&trace_count==count&&!memcmp(trace,steps,count*sizeof(*trace))&&observed_mode==direction);++comparisons;
    }
    for(unsigned method=0;method<3;++method){
        setup(4|128);retire=true;NativeScriptValue arg={.type=NativeScriptType_I32},result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
        CHECK(!nativeScriptBindings_Invoke(&context,ids[method],NULL,0,&arg,method==0?1:0,&result,&diagnostic));
        CHECK(result.type==NativeScriptType_U64&&result.as.u64==UINT64_MAX);
        setup(4|128);owned=true;CHECK(!nativeScriptBindings_Invoke(&context,ids[method],NULL,0,&arg,method==0?1:0,&result,&diagnostic));CHECK(trace_count==0);
    }
    printf("{\"status\":\"PASS actual ledge/cliff/aerial-jump bodies\",\"checks\":%u,\"comparisons\":%u}\n",checks,comparisons);return 0;
}
