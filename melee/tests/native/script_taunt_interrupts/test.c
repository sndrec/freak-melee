#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "fixture.h"
#include <melee/ft/native_script_binding_actions.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_source_bank.h>
#include <melee/ft/kinds/ftCommon/ftCo_AppealS.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftdata.h>
#include <melee/lb/lbanim.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""

NativeSourcePackage target_package;
static MotionState common[341],motions[2];static Fighter fighters[256],replacement_data;
static struct Fighter_WaitAnimData clips[2];static FigaTree clip;
static FighterPartsTable* parts[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=parts;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
ftCommonData* p_ftCommonData;
unsigned guard_calls,mutate_at,mutation;HSD_GObj* query_caller,*query_target,*replacement;HSD_GObj*(*target_reuse)(void);
static unsigned checks,cases,comparisons,created,order[14],order_count,success_at,trigger_at,change,callbacks;
static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"taunt interrupts %u: %s (%u:%u %s)\n",__LINE__,#x,diagnostic.source.line,diagnostic.source.column,diagnostic.message);exit(1);}}while(0)
void target_guard_probe(NativeScriptRuntimeContext* c){(void)c;}
void taunt_change(Fighter_GObj* g,FtMotionId m,MotionFlags f,float a,float b,float c,Fighter_GObj* o)
{(void)g;(void)m;(void)f;(void)a;(void)b;(void)c;(void)o;abort();}
static uint32_t function(const char* name)
{
    const NativeScriptProgramIndex* p=&target_package.compiler.bank.declarations;
    for(unsigned i=0;i<p->function_count;++i)if(nativeScriptSource_SpanEquals(p->tree,p->functions[i].name,name))return i;
    CHECK(false);return 0;
}
static void prepare(void)
{
    CHECK(nativeSourceBank_Allocate(&target_package.compiler,&diagnostic));
    const char* source="fighter:\n    behavior independent\nstate int divisor\nstate int after\nfunc Step():\n    controls.taunt_actions()\n    state.after = state.after + 1\nfunc Clip(int index) -> float:\n    return animation.clip_length(index)\nfunc Fault() -> int:\n    return 1 / state.divisor\naction Common:\n    on cpu_capture_decision:\n        return false\n    on tick:\n        Step()\n";
    NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=2};
    NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout};
    const NativeScriptCompileBank* accepted=NULL;
    CHECK(nativeScriptCompile_Build(&target_package.compiler.bank,&request,&accepted,&diagnostic));
    CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,accepted,&layout,NULL,&target_package.program,&diagnostic));
    target_package.address=&target_package;target_package.target=Ft_Kind_CustomFirst;target_package.ready=target_package.assets.ready=true;
    target_package.assets.facts.moves=layout;target_package.assets.common_motions=common;target_package.assets.motions=motions;
    target_package.assets.facts.actions=2;target_package.assets.character.data.xC=clips;clips[0].x14=&clip;
    target_package.assets.parts.parts_num=4;target_package.assets.facts.joints=4;parts[Ft_Kind_CustomFirst]=&target_package.assets.parts;
    gFtDataList[Ft_Kind_CustomFirst]=&target_package.assets.character.data;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=motions;
    nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&target_package.program);
}
static void remove_fighter(void* value){nativeScriptFighter_Detach(((Fighter*)value)->gobj);}
static HSD_GObj* fighter(bool owned)
{
    CHECK(created<256);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(o);
    Fighter* f=&fighters[created++];*f=(Fighter){.gobj=o,.kind=owned?Ft_Kind_CustomFirst:Ft_Kind_Link,
        .behavior_kind=owned?Ft_Kind_CustomFirst:Ft_Kind_Link,.x8_spawnNum=created+10,.ft_data=&target_package.assets.character.data,.allow_interrupt=true};
    f->x24=clips;f->x58C=2;
    GObj_InitUserData(o,4,remove_fighter,f);if(owned)CHECK(nativeScriptFighter_Attach(o));return o;
}
static NativeScriptRuntimeContext context(HSD_GObj* o)
{uint32_t id=((Fighter*)o->user_data)->x8_spawnNum;return(NativeScriptRuntimeContext){.source=o,.generation=id,.actor_id=id,.source_class=NativeCallbackSource_Fighter,.scene_epoch=nativeGObjPool.lifetime};}
static int64_t after(HSD_GObj* o)
{
    NativeScriptReceiverView v;CHECK(nativeScriptFighter_View(o,((Fighter*)o->user_data)->x8_spawnNum,&v));
    for(unsigned i=0;i<v.program->value_count;++i)if(nativeScriptSource_SpanEquals(v.program->tree,v.program->values[i].name,"after")){
        NativeScriptValue value;CHECK(nativeScriptStorage_Read(v.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&value,&diagnostic));return value.as.i64;
    }CHECK(false);return 0;
}
bool input_check(Fighter_GObj* o,unsigned index)
{
    CHECK(order_count<14);order[order_count++]=index;
    if(change && index==trigger_at) {
        unsigned mode=change;change=0;NativeScriptValue output={.type=NativeScriptType_U64,.as.u64=99};
        CHECK(nativeScriptFighter_Run(o,NativeScriptFighter_CpuCaptureDecision,NULL,0, NULL,&output)==NativeScriptVM_Ok);
        CHECK(output.type==NativeScriptType_Bool&&!output.as.u64);++callbacks;
        Fighter* f=o->user_data;
        if(mode==1)HSD_GObjFree(o);
        if(mode==2){CHECK(nativeScriptFighter_Helper(o,f->x8_spawnNum,function("Fault"),NULL,0,&output,&diagnostic)!=NativeScriptVM_Ok);CHECK(nativeScriptFighter_Status(o)==NativeScriptVM_Error);}
        if(mode==3)++f->x8_spawnNum;
        if(mode==4)++((NativeScriptFighterProgram*)nativeScriptFighter_Program(f->kind))->identity;
        if(mode==5){replacement_data=*f;o->user_data=&replacement_data;}
        return false;
    }
    return index==success_at;
}
static void reset(unsigned success){order_count=0;success_at=success;trigger_at=UINT32_MAX;change=0;memset(order,0,sizeof(order));}
static void compare(void)
{
    HSD_GObj* o=fighter(true);NativeScriptRuntimeContext c=context(o);NativeScriptValue output;
    for(unsigned enabled=0;enabled<2;++enabled)for(unsigned success=0;success<=14;++success){
        ((Fighter*)o->user_data)->allow_interrupt=enabled;reset(success);reference_taunt_inputs(o);
        unsigned count=order_count,expected[14];memcpy(expected,order,sizeof(expected));reset(success);
        CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_ControlsTauntActions,NULL,0,NULL,0,&output,&diagnostic));
        CHECK(output.type==NativeScriptType_Void&&order_count==count&&!memcmp(order,expected,count*sizeof(*order)));
        reset(success);int64_t prior=after(o);CHECK(nativeScriptFighter_Run(o,NativeScriptFighter_Tick,NULL,0, NULL,&output)==NativeScriptVM_Ok);
        CHECK(order_count==count&&!memcmp(order,expected,count*sizeof(*order))&&after(o)==prior+1);++comparisons;++cases;
    }
    output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=77};CHECK(!nativeScriptBindingActions_Invoke(&c,NativeScriptBinding_ControlsTauntActions,NULL,1,&output,&diagnostic)&&output.as.u64==77);
    HSD_GObjFree(o);
    // Unattached native fighters still use the unchanged native input ordering.
    o=fighter(false);reset(14);ftCo_AppealS_IASA(o);CHECK(order_count==14);HSD_GObjFree(o);++cases;
}
static void retirements(void)
{
    for(unsigned phase=0;phase<14;++phase)for(unsigned mode=1;mode<=5;++mode) {
        HSD_GObj* o=fighter(true);NativeScriptRuntimeContext c=context(o);reset(14);trigger_at=phase;change=mode;
        NativeScriptFighterProgram* selected=(NativeScriptFighterProgram*)nativeScriptFighter_Program(Ft_Kind_CustomFirst);
        uint64_t identity=selected->identity;Fighter* data=o->user_data;
        NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=77};
        CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_ControlsTauntActions,NULL,0,NULL,0,&result,&diagnostic));
        CHECK(change==0&&order_count==phase+1&&result.type==NativeScriptType_U64&&result.as.u64==77);
        for(unsigned i=0;i<order_count;++i)CHECK(order[i]==i);
        selected->identity=identity;
        if(mode!=1){if(mode==5)o->user_data=data;HSD_GObjFree(o);}++cases;
    }
    // Repeat physical retirement and real arithmetic fault through an outer
    // authored VM helper. Its post-native state write must never execute.
    for(unsigned mode=1;mode<=2;++mode){HSD_GObj* o=fighter(true);reset(14);trigger_at=5;change=mode;
        NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=77};uint32_t id=((Fighter*)o->user_data)->x8_spawnNum;
        CHECK(nativeScriptFighter_Helper(o,id,function("Step"),NULL,0,&result,&diagnostic)!=NativeScriptVM_Ok);CHECK(order_count==6&&result.type==NativeScriptType_U64&&result.as.u64==77);
        if(mode!=1)HSD_GObjFree(o);++cases;
    }
}
static void type_cases(void)
{
    const char* invalid[]={"fighter:\n    behavior independent\nfunc Bad():\n    controls.taunt_actions(1)\n",
        "fighter:\n    behavior independent\nfunc Bad() -> bool:\n    return controls.taunt_actions()\n"};
    for(unsigned i=0;i<2;++i){NativeSourceBank scratch={0};CHECK(nativeSourceBank_Allocate(&scratch,&diagnostic));
        const NativeScriptCompileBank* accepted=&target_package.compiler.bank;
        NativeScriptCompileRequest request={.source=invalid[i],.source_bytes=(uint32_t)strlen(invalid[i]),.motion_layout=&accepted->motion_layout};
        CHECK(!nativeScriptCompile_Build(&scratch.bank,&request,&accepted,&diagnostic)&&accepted==&target_package.compiler.bank);nativeSourceBank_Release(&scratch);++cases;
    }
}
static void clip_cases(void)
{
    HSD_GObj* o=fighter(true);NativeScriptRuntimeContext c=context(o);NativeScriptValue result;
    const uint32_t patterns[]={0,0x80000000,0x3f800000,0x418c0000,0x7f800000,0xff800000,0x7fc01234};
    for(unsigned i=0;i<7;++i){memcpy(&clip.frames,&patterns[i],4);for(unsigned index=0;index<2;++index){
        NativeScriptValue arg={.type=NativeScriptType_I32,.as.i64=index};float expected=lbAnim_8001E8F8(ftData_80085E50(o->user_data,(int)index));
        CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_AnimationClipLength,NULL,0,&arg,1,&result,&diagnostic));
        CHECK(result.type==NativeScriptType_F32&&!memcmp(&result.as.f32,&expected,4));
        CHECK(nativeScriptFighter_Helper(o,c.generation,function("Clip"),&arg,1,&result,&diagnostic)==NativeScriptVM_Ok);CHECK(!memcmp(&result.as.f32,&expected,4));++comparisons;
    }}
    NativeScriptValue invalid[]={{.type=NativeScriptType_I64,.as.i64=-1},{.type=NativeScriptType_U64,.as.u64=2},{.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000000)},{.type=NativeScriptType_F32,.as.f32=1}};
    for(unsigned i=0;i<4;++i){result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=77};CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_AnimationClipLength,NULL,0,&invalid[i],1,&result,&diagnostic)&&result.type==NativeScriptType_U64&&result.as.u64==77);}
    ((Fighter*)o->user_data)->x24=NULL;NativeScriptValue arg={.type=NativeScriptType_I32,.as.i64=0};CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_AnimationClipLength,NULL,0,&arg,1,&result,&diagnostic));
    HSD_GObjFree(o);++cases;
}
int main(void)
{
    HSD_GObjLibInitDataType setup={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&setup);prepare();
    compare();retirements();type_cases();clip_cases();CHECK(callbacks==72);CHECK(nativeScriptRuntime_Idle());
    nativeSourceBank_Release(&target_package.compiler);
    printf("{\"status\":\"PASS taunt interrupts\",\"checks\":%u,\"cases\":%u,\"native_comparisons\":%u,\"nested_callbacks\":%u}\n",checks,cases,comparisons,callbacks);return 0;
}
