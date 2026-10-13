#include <melee/ft/native_script_binding_actions.h>
#include <melee/ft/native_script_bindings.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftcpuattack.h>
#include <melee/ft/ftcmdscript.h>
#include <melee/ft/ftcpu_profile.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/types.h>
#include <melee/ef/native_model_effect.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Fighter fighter,target;
static HSD_GObj object,target_object;
static HSD_GObj* heads[32];HSD_GObj** HSD_GObjPLinkHead=heads;
static struct Fighter_804D64FC_t common;struct Fighter_804D64FC_t* Fighter_804D64FC=&common;
static unsigned checks,commands,ground_queries,ray_queries;
static bool obstructed,grounded;
static unsigned command_log[32];
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"action %u: %s\n",__LINE__,#x);exit(1);}}while(0)
void ftCo_800B463C(Fighter* fp,u8 command){(void)fp;if(commands>=32)abort();command_log[commands++]=command;}
bool nativeModelEffect_Snapshot(HSD_GObj* o,uint32_t* g){(void)o;(void)g;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* o,uint32_t g){(void)o;(void)g;return NULL;}
bool nativeScriptBindings_SourceAlive(NativeScriptRuntimeContext* c){return nativeCallbackSource_Alive(c->source,c->generation,c->source_class);}
// This primitive fixture does not schedule actor actions. Those operations
// remain explicitly unavailable; CPU/common scalar behavior is actual source.
void ftCo_Wait_IASA(Fighter_GObj* o){(void)o;abort();}
bool ftCo_Fall_IASA_Inner(Fighter_GObj* o){(void)o;abort();}
bool nativeScriptFighter_BindMotionPhase(Fighter_GObj* o,uint32_t p,uint32_t f,bool cue){(void)o;(void)p;(void)f;(void)cue;return false;}
bool nativeScriptBindingFeedback_Invoke(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* a,uint32_t n,NativeScriptValue* v,NativeScriptDiagnostic* d){(void)c;(void)s;(void)a;(void)n;(void)v;(void)d;return false;}
bool nativeScriptBindingCollision_Invoke(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* a,uint32_t n,NativeScriptValue* v,NativeScriptDiagnostic* d){(void)c;(void)s;(void)a;(void)n;(void)v;(void)d;return false;}
#include "actual_cpu.h"
static NativeScriptRuntimeContext context;
static NativeScriptDiagnostic diagnostic;
static NativeScriptValue integer(int value){return (NativeScriptValue){.type=NativeScriptType_I32,.as.i64=value};}
static void setup(void)
{
    fighter=(Fighter){0};target=(Fighter){0};object=(HSD_GObj){.user_data=&fighter,.next=&target_object};target_object=(HSD_GObj){.user_data=&target};
    fighter.gobj=&object;fighter.x8_spawnNum=17;target.gobj=&target_object;target.x8_spawnNum=18;heads[HSD_GOBJ_PLINK_FIGHTER]=&object;
    context=(NativeScriptRuntimeContext){.source=&object,.source_class=NativeCallbackSource_Fighter,.generation=17};commands=ground_queries=ray_queries=0;
}
int main(void)
{
    setup();NativeScriptValue value;
    static u8 sequence[]={CpuCmd_PressA,CpuCmd_PressB,CpuCmd_Done};static u8* scripts[FT_CPU_SCRIPT_COUNT];
    for(unsigned i=0;i<FT_CPU_SCRIPT_COUNT;++i)scripts[i]=sequence;common.cmdscripts=scripts;
    for(int index=-2;index<65;++index){setup();unsigned expected[32],expected_count=0;
        if(index>0&&index<FT_CPU_SCRIPT_COUNT){ftCo_800B4880(&fighter,index);expected_count=commands;memcpy(expected,command_log,sizeof(expected));}
        commands=0;NativeScriptValue in=integer(index),out={.type=NativeScriptType_Void};
        CHECK(nativeScriptBindingActions_Invoke(&context,NativeScriptBinding_CPURunScript,&in,1,&out,&diagnostic));CHECK(commands==expected_count);
        if(expected_count)CHECK(!memcmp(expected,command_log,expected_count*sizeof(expected[0])));
    }
    for(int level=-2;level<20;++level)for(int attempt=-1;attempt<10;++attempt){fighter.cpu.level=level;fighter.cpu.x94=attempt;CHECK(nativeScriptBindingActions_Read(&context,NativeScriptBinding_CPULevel,&value,&diagnostic));CHECK(value.as.i64==fighter.cpu.level);CHECK(nativeScriptBindingActions_Read(&context,NativeScriptBinding_CPUGrabAttempts,&value,&diagnostic));CHECK(value.as.i64==attempt);}
    for(int selected=-2;selected<65;++selected)for(unsigned used=0;used<8;++used){setup();fighter.cpu.xEC=used;for(unsigned i=0;i<8;++i)fighter.cpu.xCC_array[i]=0x100+i;
        Fighter initial=fighter;ftCo_CpuAddAttackCandidate(&fighter,selected);Fighter expected=fighter;fighter=initial;
        NativeScriptValue input=integer(selected),out={.type=NativeScriptType_Void};CHECK(nativeScriptBindingActions_Invoke(&context,NativeScriptBinding_CPUAddCandidate,&input,1,&out,&diagnostic));CHECK(!memcmp(&fighter,&expected,sizeof(fighter)));}
    for(unsigned mask=0;mask<8;++mask){setup();fighter.facing_dir=mask&1?-1:1;fighter.cur_pos=(Vec3){3,5,0};target.cur_pos=(Vec3){12,7,0};fighter.cpu.x44=mask&2?&target:NULL;obstructed=(mask&4)!=0;grounded=false;
        bool expected=ftCo_CpuRangedAttackUnsafe(&fighter,fighter.cpu.x44,22);unsigned rays=ray_queries,ground=ground_queries;ray_queries=ground_queries=0;
        NativeScriptValue input={.type=NativeScriptType_F32,.as.f32=22},out={.type=NativeScriptType_Bool};CHECK(nativeScriptBindingActions_Invoke(&context,NativeScriptBinding_CPURangedUnsafe,&input,1,&out,&diagnostic));CHECK(out.as.u64==expected&&ray_queries==rays&&ground_queries==ground);}
    for(unsigned i=0;i<17;++i){setup();fighter.input.lstick[0].x=((int)i-8)*.125f;Fighter initial=fighter;ftCommon_UpdateFacing(&fighter);Fighter expected=fighter;fighter=initial;NativeScriptValue out={.type=NativeScriptType_Void};CHECK(nativeScriptBindingActions_Invoke(&context,NativeScriptBinding_FacingFromInput,NULL,0,&out,&diagnostic));CHECK(!memcmp(&fighter,&expected,sizeof(fighter)));}
    setup();fighter.x5F4_arr[0].prev=9;fighter.x5F4_arr[1].prev=7;NativeScriptValue input=integer(0),out={.type=NativeScriptType_Void};CHECK(nativeScriptBindingActions_Invoke(&context,NativeScriptBinding_VisualsHideGroup,&input,1,&out,&diagnostic));CHECK(fighter.x5F4_arr[0].prev==0&&fighter.x5F4_arr[1].prev==7&&fighter.x221D_b2);
    setup();++fighter.x8_spawnNum;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_MAX};input=integer(31);CHECK(!nativeScriptBindingActions_Invoke(&context,NativeScriptBinding_CPUAddCandidate,&input,1,&out,&diagnostic));CHECK(!fighter.cpu.xEC&&out.type==NativeScriptType_U64&&out.as.u64==UINT64_MAX);
    printf("{\"status\":\"PASS actual CPU/action bodies\",\"checks\":%u}\n",checks);return 0;
}
// This actual-body primitive oracle has no selected source package/actor.
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind)
{(void)kind;return NULL;}
const NativeScriptMoveLayout* nativeScriptBindings_SourceFighterLayout(NativeScriptRuntimeContext* context,NativeScriptDiagnostic* diagnostic)
{(void)context;(void)diagnostic;return NULL;}
