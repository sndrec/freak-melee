#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "fixture.h"
#include <melee/ft/native_script_binding_targets.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_fighter_target.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
enum{N=2048,I=8192,F=64,V=128};
NativeSourcePackage target_package;
#define bank target_package.compiler.bank
static char source_bytes[32768];static NativeScriptSourceNode nodes[N];static NativeScriptIRNode ir[I];static uint32_t arguments[I];
static NativeScriptFunctionDeclaration functions[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];
static NativeScriptLocal locals[V];static NativeScriptInstruction instructions[I];static NativeScriptFunctionCode codes[F];static NativeScriptExecutionFunction entries[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[V];static NativeScriptParameterSlot slots[V];
static Article article;static ItemAttr attributes;static ItemModelDesc model;static Fighter fighters[256],alternate;static Item items[128];
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=tables;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
unsigned guard_calls,mutate_at,mutation;HSD_GObj* query_caller,*query_target,*replacement;HSD_GObj*(*target_reuse)(void);
static unsigned checks,fighter_count,item_count,comparisons;static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"target query %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
#undef bank
#include "foreign_source.h"
#define bank target_package.compiler.bank
static void prepare(void)
{
    bank=(NativeScriptCompileBank){.source_storage=source_bytes,.source_capacity=sizeof(source_bytes),.tree={.nodes=nodes,.capacity=N},.expressions={.nodes=ir,.node_capacity=I,.arguments=arguments,.argument_capacity=I},
        .declarations={.functions=functions,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=V,
        .instructions=instructions,.instruction_capacity=I,.function_code=codes,.function_code_capacity=F,.execution_functions=entries,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,
        .storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=V},.parameter_slots=slots,.parameter_slot_capacity=V};
    const char* source="fighter:\n    behavior independent\nstate int handle\nstate bool found\nstate bool air\nstate bool fighter\nstate float x\nstate float y\nstate float z\nstate bool delta_found\nstate float dx\nstate float dy\nstate float dz\nstate float damage\nstate float facing\nstate float width\nstate bool sleeping\nstate int floor\naction Queries:\n    on tick:\n        state.found = targets.read_position(state.handle, state.x, state.y, state.z)\n        state.delta_found = targets.read_position_delta(state.handle, state.dx, state.dy, state.dz)\n        state.air = targets.airborne(state.handle)\n        state.fighter = targets.is_fighter(state.handle)\n        state.damage = targets.damage(state.handle)\n        state.facing = targets.facing(state.handle)\n        state.width = targets.cpu_body_half_width(state.handle)\n        state.sleeping = targets.sleeping(state.handle)\n        state.floor = targets.floor_line(state.handle)\nfunc DeltaLocals(int reference) -> float:\n    let float x = 111\n    let float y = 112\n    let float z = 113\n    let bool found = targets.read_position_delta(reference, x, y, z)\n    return x\narticle Probe states 1:\n    state int handle\n    state bool found\n    state bool air\n    state bool fighter\n    state float x\n    state float y\n    state float z\n    state bool delta_found\n    state float dx\n    state float dy\n    state float dz\n    state float damage\n    state float facing\n    state float width\n    state bool sleeping\n    state int floor\n    on signal Query:\n        item.state.found = targets.read_position(item.state.handle, item.state.x, item.state.y, item.state.z)\n        item.state.delta_found = targets.read_position_delta(item.state.handle, item.state.dx, item.state.dy, item.state.dz)\n        item.state.air = targets.airborne(item.state.handle)\n        item.state.fighter = targets.is_fighter(item.state.handle)\n        item.state.damage = targets.damage(item.state.handle)\n        item.state.facing = targets.facing(item.state.handle)\n        item.state.width = targets.cpu_body_half_width(item.state.handle)\n        item.state.sleeping = targets.sleeping(item.state.handle)\n        item.state.floor = targets.floor_line(item.state.handle)\n    state 0 anim -1:\n";
    article=(Article){.x0_common_attr=&attributes,.x10_modelDesc=&model};NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&article};
    NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};
    NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout,.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};
    const NativeScriptCompileBank* accepted=NULL;CHECK(nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));
    CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,&bank,&layout,&metadata,&target_package.program,&diagnostic));
    target_package.address=&target_package;target_package.target=Ft_Kind_CustomFirst;target_package.ready=target_package.assets.ready=true;target_package.assets.facts.moves=layout;
    target_package.assets.parts.parts_num=4;target_package.assets.facts.joints=4;tables[Ft_Kind_CustomFirst]=&target_package.assets.parts;
    gFtDataList[Ft_Kind_CustomFirst]=&target_package.assets.character.data;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=target_package.assets.motions;
    nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&target_package.program);
}
static void fighter_remove(void* p){nativeScriptFighter_Detach(((Fighter*)p)->gobj);}
static void item_remove(void* p){nativeScriptItem_Detach(((Item*)p)->entity);}
static HSD_GObj* fighter(bool owned,uint32_t id)
{
    CHECK(fighter_count<256);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);Fighter* f=&fighters[fighter_count++];
    *f=(Fighter){.gobj=o,.kind=owned?Ft_Kind_CustomFirst:Ft_Kind_Link,.behavior_kind=owned?Ft_Kind_CustomFirst:Ft_Kind_Link,
        .x8_spawnNum=id,.ft_data=&target_package.assets.character.data,.cur_pos={1.25f,-2.5f,3.75f},.pos_delta={4.25f,-5.5f,6.75f}};
    GObj_InitUserData(o,4,fighter_remove,f);if(owned)CHECK(nativeScriptFighter_Attach(o));return o;
}
static HSD_GObj* item(void)
{
    CHECK(item_count<128);const NativeScriptItemProgram* p=&nativeScriptFighter_Program(Ft_Kind_CustomFirst)->items.programs[0];
    HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);Item* i=&items[item_count++];
    *i=(Item){.entity=o,.kind=p->kind,.msid=-1,.native_script_owner=p->owner,.native_article_author_kind=p->author_kind,.x1C=100+item_count,.xC4_article_data=&article};
    GObj_InitUserData(o,6,item_remove,i);CHECK(nativeScriptItem_Attach(o,p));return o;
}
static NativeScriptValue field(HSD_GObj* o,const char* name,const NativeScriptValue* input)
{
    bool f=o->classifier==HSD_GOBJ_CLASS_FIGHTER;uint32_t gen=f?((Fighter*)o->user_data)->x8_spawnNum:((Item*)o->user_data)->x1C;
    NativeScriptReceiverView view;CHECK(f?nativeScriptFighter_View(o,gen,&view):nativeScriptItem_View(o,gen,&view));
    for(unsigned n=0;n<bank.declarations.value_count;++n)if(values[n].owner==view.storage->owner&&nativeScriptSource_SpanEquals(&bank.tree,values[n].name,name)){
        NativeScriptValue result;if(input)CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,input,&diagnostic));CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&result,&diagnostic));return result;
    }CHECK(false);return(NativeScriptValue){0};
}
static NativeScriptRuntimeContext context(HSD_GObj* o)
{bool f=o->classifier==HSD_GOBJ_CLASS_FIGHTER;uint32_t gen=f?((Fighter*)o->user_data)->x8_spawnNum:((Item*)o->user_data)->x1C;return(NativeScriptRuntimeContext){.source=o,.source_class=f?NativeCallbackSource_Fighter:NativeCallbackSource_Item,.generation=gen,.actor_id=gen,.scene_epoch=nativeGObjPool.lifetime};}
static uint32_t bits(float value){uint32_t result;memcpy(&result,&value,4);return result;}
static float floating(uint32_t bits){float result;memcpy(&result,&bits,4);return result;}
static uint32_t hash(const char* name){uint32_t h=2166136261u;while(*name){h^=(unsigned char)*name++;h*=16777619u;}return h;}
static void args(NativeScriptValue* a,uint32_t id)
{a[0]=(NativeScriptValue){.type=NativeScriptType_U32,.as.u64=id};for(unsigned i=1;i<4;++i)a[i]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=111.0f+(float)i};}
static void delta_samples(HSD_GObj* caller,HSD_GObj* article_caller,HSD_GObj* target)
{
    NativeScriptRuntimeContext c=context(caller);NativeScriptValue a[4],out;
    uint32_t helper=UINT32_MAX;for(unsigned i=0;i<bank.declarations.function_count;++i)if(!functions[i].owner&&nativeScriptSource_SpanEquals(&bank.tree,functions[i].name,"DeltaLocals"))helper=i;CHECK(helper!=UINT32_MAX);
    uint32_t seed=UINT32_C(0x23456789);
    for(unsigned i=0;i<256;++i){Vec3 expected,legacy;Fighter* native=target->user_data;
        seed=seed*1664525u+1013904223u;native->pos_delta.x=floating(seed);
        seed=seed*1664525u+1013904223u;native->pos_delta.y=floating(seed);
        seed=seed*1664525u+1013904223u;native->pos_delta.z=floating(seed);
        ftLib_80086BEC(target,&expected);CHECK(nativeFighterTarget_PositionDelta(UINT32_C(0x80000005),&legacy)&&!memcmp(&expected,&legacy,sizeof(expected)));
        args(a,UINT32_C(0x80000005));CHECK(nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsReadPositionDelta,a,4,&out,&diagnostic)&&out.as.u64);
        CHECK(bits(a[1].as.f32)==bits(expected.x)&&bits(a[2].as.f32)==bits(expected.y)&&bits(a[3].as.f32)==bits(expected.z));++comparisons;
        NativeScriptValue id={.type=NativeScriptType_I32,.as.i64=-2147483643};CHECK(nativeScriptFighter_Helper(caller,c.generation,helper,&id,1,&out,&diagnostic)==NativeScriptVM_Ok&&out.type==NativeScriptType_F32&&bits(out.as.f32)==bits(expected.x));
    }
    const uint32_t patterns[]={0,UINT32_C(0x80000000),UINT32_C(0x7f800000),UINT32_C(0xff800000),UINT32_C(0x7fc01234),UINT32_C(0x7fa00001)};
    for(unsigned i=0;i<6;++i){Fighter* native=target->user_data;native->pos_delta=(Vec3){floating(patterns[i]),floating(patterns[(i+1)%6]),floating(patterns[(i+2)%6])};Vec3 expected;ftLib_80086BEC(target,&expected);args(a,UINT32_C(0x80000005));
        CHECK(nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsReadPositionDelta,a,4,&out,&diagnostic)&&out.as.u64);CHECK(bits(a[1].as.f32)==bits(expected.x)&&bits(a[2].as.f32)==bits(expected.y)&&bits(a[3].as.f32)==bits(expected.z));++comparisons;
    }
    for(unsigned i=0;i<3;++i){args(a,i==0?0:i==1?123456:UINT32_MAX);NativeScriptValue old[4];memcpy(old,a,sizeof(old));CHECK(nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsReadPositionDelta,a,4,&out,&diagnostic)&&!out.as.u64&&!memcmp(old,a,sizeof(old)));}
    NativeScriptValue missing={.type=NativeScriptType_I32,.as.i64=0};CHECK(nativeScriptFighter_Helper(caller,c.generation,helper,&missing,1,&out,&diagnostic)==NativeScriptVM_Ok&&out.type==NativeScriptType_F32&&out.as.f32==111);
    HSD_GObj* maximum=fighter(false,UINT32_MAX);args(a,UINT32_MAX);a[0]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=-1};CHECK(nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsReadPositionDelta,a,4,&out,&diagnostic)&&out.as.u64&&a[1].as.f32==4.25f);HSD_GObjFree(maximum);
    for(unsigned i=0;i<3;++i){args(a,5);if(i==0)a[0]=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000005)};if(i==1)a[0]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=5};if(i==2)a[2].type=NativeScriptType_I32;NativeScriptValue old[4];memcpy(old,a,sizeof(old));out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(!nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsReadPositionDelta,a,4,&out,&diagnostic)&&out.type==NativeScriptType_U64&&out.as.u64==99&&!memcmp(old,a,sizeof(old)));}
    // The caller belongs to the independent Item registry while the target
    // registry is deliberately malformed; no unrelated unsafe registry walk.
    c=context(article_caller);HSD_GObj* head=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];
    void* poison=VirtualAlloc(NULL,4096,MEM_COMMIT|MEM_RESERVE,PAGE_NOACCESS);CHECK(poison);
    HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER]=poison;args(a,123456);NativeScriptValue old[4];memcpy(old,a,sizeof(old));out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
    CHECK(!nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsReadPositionDelta,a,4,&out,&diagnostic)&&out.as.u64==99&&!memcmp(old,a,sizeof(old)));HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER]=head;CHECK(VirtualFree(poison,0,MEM_RELEASE));
    HSD_GObj* next=target->next;HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER]=target;target->next=target;CHECK(!nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsReadPositionDelta,a,4,&out,&diagnostic)&&out.as.u64==99&&!memcmp(old,a,sizeof(old)));target->next=next;HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER]=head;
    Fighter* native=target->user_data;native->gobj=NULL;args(a,UINT32_C(0x80000005));CHECK(!nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsReadPositionDelta,a,4,&out,&diagnostic)&&out.as.u64==99);native->gobj=target;
}
static void scalar_samples(HSD_GObj* caller,HSD_GObj* article_caller,HSD_GObj* target)
{
 const uint32_t patterns[]={0,0x80000000,0x3f800000,0xbf800000,0x7f800000,0xff800000,0x7fc12345,0x7fa00001};
 const uint32_t ids[]={NativeScriptBinding_TargetsDamage,NativeScriptBinding_TargetsFacing,NativeScriptBinding_TargetsCpuBodyHalfWidth,NativeScriptBinding_TargetsSleeping,NativeScriptBinding_TargetsFloorLine};
 const char* names[]={"damage","facing","width","sleeping","floor"};
 for(unsigned source=0;source<2;++source)for(unsigned recipient=0;recipient<2;++recipient)for(unsigned n=0;n<8;++n){
  HSD_GObj* from=source?article_caller:caller;HSD_GObj* selected=recipient?target:caller;Fighter* f=selected->user_data;NativeScriptRuntimeContext c=context(from);
  uint32_t id=(uint32_t)f->x8_spawnNum;memcpy(&f->dmg.x1830_percent,&patterns[n],4);memcpy(&f->facing_dir,&patterns[(n+1)%8],4);memcpy(&f->cpu.x564,&patterns[(n+2)%8],4);f->x221F_b3=n&1;f->coll_data.floor.index=n?INT32_MIN+(int)n:-1;
  NativeScriptValue handle={.type=NativeScriptType_I32,.as.i64=(int32_t)id},expected[5],actual;
  expected[0]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=nativeFighterTarget_Damage(id)};
  expected[1]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=nativeFighterTarget_Facing(id)};
  expected[2]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=nativeFighterTarget_CpuBodyHalfWidth(id)};
  expected[3]=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=nativeFighterTarget_Sleeping(id)};
  expected[4]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=nativeFighterTarget_FloorLine(id)};
  static Fighter before;before=*f;
  for(unsigned q=0;q<5;++q){CHECK(nativeScriptBindings_Invoke(&c,ids[q],NULL,0,&handle,1,&actual,&diagnostic));CHECK(actual.type==expected[q].type);CHECK(q<3?bits(actual.as.f32)==bits(expected[q].as.f32):actual.as.i64==expected[q].as.i64);++comparisons;}
  CHECK(!memcmp(f,&before,sizeof(*f)));field(from,"handle",&handle);
  NativeScriptValue output;if(source)CHECK(nativeScriptItem_RunSignal(from,hash("Query"),0,&output)==NativeScriptVM_Ok);else CHECK(nativeScriptFighter_Run(from,NativeScriptFighter_Tick,NULL,0,NULL,&output)==NativeScriptVM_Ok);
  for(unsigned q=0;q<5;++q){actual=field(from,names[q],NULL);CHECK(actual.type==expected[q].type);CHECK(q<3?bits(actual.as.f32)==bits(expected[q].as.f32):actual.as.i64==expected[q].as.i64);}
 }
 for(unsigned source=0;source<2;++source)for(unsigned missing=0;missing<2;++missing){HSD_GObj* from=source?article_caller:caller;NativeScriptRuntimeContext c=context(from);NativeScriptValue handle={.type=NativeScriptType_I32,.as.i64=missing?123456:0},actual;field(from,"handle",&handle);
  for(unsigned q=0;q<5;++q){CHECK(nativeScriptBindings_Invoke(&c,ids[q],NULL,0,&handle,1,&actual,&diagnostic));CHECK(q<3?actual.type==NativeScriptType_F32&&bits(actual.as.f32)==0:q==3?actual.type==NativeScriptType_Bool&&!actual.as.u64:actual.type==NativeScriptType_I32&&actual.as.i64==-1);++comparisons;}
  NativeScriptValue output;if(source)CHECK(nativeScriptItem_RunSignal(from,hash("Query"),0,&output)==NativeScriptVM_Ok);else CHECK(nativeScriptFighter_Run(from,NativeScriptFighter_Tick,NULL,0,NULL,&output)==NativeScriptVM_Ok);
  CHECK(field(from,"damage",NULL).as.f32==0&&field(from,"facing",NULL).as.f32==0&&field(from,"width",NULL).as.f32==0&&!field(from,"sleeping",NULL).as.u64&&field(from,"floor",NULL).as.i64==-1);
 }
}
static void frontend(void)
{
    const char* calls[]={"targets.damage(1.0)","targets.facing(1.0)","targets.cpu_body_half_width(1.0)","targets.sleeping(1.0)","targets.floor_line(1.0)","targets.damage()","targets.floor_line(1, 2)","targets.read_position_delta(1, x, x, z)","targets.read_position_delta(1, x, y, y)","targets.read_position_delta(1, x, y, 3.0)","targets.read_position_delta(1, x, y, integer)","targets.read_position_delta(1.0, x, y, z)","targets.read_position_delta(1, x, y)","targets.read_position_delta(1, x, y, z, x)"};
    for(unsigned i=0;i<sizeof(calls)/sizeof(*calls);++i){char source[1024];snprintf(source,sizeof(source),"fighter:\n    behavior independent\nfunc Probe():\n    let float x = 1\n    let float y = 2\n    let float z = 3\n    let int integer = 4\n    %s\n",calls[i]);NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.metadata=nativeScriptCompile_MinimalMetadata};const NativeScriptCompileBank* accepted=NULL;CHECK(!nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic)&&!accepted);}
}
static HSD_GObj* reuse(void){replacement=fighter(false,UINT32_C(0x80000005));return replacement;}
void target_guard_probe(NativeScriptRuntimeContext* c)
{
    if(c->source!=query_caller)return;++guard_calls;if(!mutation||guard_calls!=mutate_at)return;
    unsigned mode=mutation;mutation=0;
    if(mode==1)((Fighter*)query_target->user_data)->x8_spawnNum++;
    if(mode==2)((Fighter*)query_target->user_data)->kind=Ft_Kind_Zelda;
    if(mode==3){alternate=*(Fighter*)query_target->user_data;query_target->user_data=&alternate;}
    if(mode==4){HSD_GObjFree(query_target);if(target_reuse)target_reuse();}
    if(mode==5)HSD_GObjFree(query_caller);
    if(mode==6){NativeScriptValue result;CHECK(nativeScriptItem_RunHook(query_caller,NativeScriptItemHook_Destroyed,NULL,0,&result)==NativeScriptVM_Ok);}
    if(mode==7){for(unsigned i=0;i<NATIVE_SCRIPT_ITEM_ACTORS;++i)if(nativeScriptItemActors[i].active&&nativeScriptItemActors[i].object==query_caller)nativeScriptItemActors[i].faulted=true;}
    if(mode==8)query_target->classifier=HSD_GOBJ_CLASS_ITEM;
}
int main(void)
{
    prepare();prepare_foreign();HSD_GObjLibInitDataType setup={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&setup);
    HSD_GObj* f=fighter(true,21),*a=item(),*foreign=fighter(false,UINT32_C(0x80000005));
    NativeScriptValue values4[4],output;args(values4,UINT32_C(0x80000005));NativeScriptRuntimeContext c=context(f);
    CHECK(nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsReadPosition,values4,4,&output,&diagnostic)&&output.as.u64);
    CHECK(values4[1].as.f32==1.25f&&values4[2].as.f32==-2.5f&&values4[3].as.f32==3.75f);
    // Both real owned caller classes publish SuccessOnly outputs through VM storage.
    for(unsigned k=0;k<2;++k){HSD_GObj* caller=k?a:f;NativeScriptValue id={.type=NativeScriptType_I32,.as.i64=-2147483643};field(caller,"handle",&id);
        if(k)CHECK(nativeScriptItem_RunSignal(caller,hash("Query"),0,&output)==NativeScriptVM_Ok);else CHECK(nativeScriptFighter_Run(caller,NativeScriptFighter_Tick,NULL,0, NULL,&output)==NativeScriptVM_Ok);
        CHECK(field(caller,"found",NULL).as.u64&&field(caller,"fighter",NULL).as.u64&&!field(caller,"air",NULL).as.u64);
        CHECK(field(caller,"x",NULL).as.f32==1.25f&&field(caller,"y",NULL).as.f32==-2.5f&&field(caller,"z",NULL).as.f32==3.75f);
        CHECK(field(caller,"delta_found",NULL).as.u64&&field(caller,"dx",NULL).as.f32==4.25f&&field(caller,"dy",NULL).as.f32==-5.5f&&field(caller,"dz",NULL).as.f32==6.75f);
        id.as.i64=0;field(caller,"handle",&id);if(k)CHECK(nativeScriptItem_RunSignal(caller,hash("Query"),0,&output)==NativeScriptVM_Ok);else CHECK(nativeScriptFighter_Run(caller,NativeScriptFighter_Tick,NULL,0, NULL,&output)==NativeScriptVM_Ok);
        CHECK(!field(caller,"found",NULL).as.u64&&!field(caller,"fighter",NULL).as.u64&&field(caller,"x",NULL).as.f32==1.25f);
        CHECK(!field(caller,"delta_found",NULL).as.u64&&field(caller,"dx",NULL).as.f32==4.25f&&field(caller,"dy",NULL).as.f32==-5.5f&&field(caller,"dz",NULL).as.f32==6.75f);
    }
    static const uint32_t patterns[]={0,UINT32_C(0x80000000),UINT32_C(0x3f800000),UINT32_C(0xbf800000),UINT32_C(0x7f800000),UINT32_C(0xff800000),UINT32_C(0x7fc01234),UINT32_C(0x7fa00001)};
    for(unsigned n=0;n<64;++n){Fighter* target=foreign->user_data;target->cur_pos=(Vec3){floating(patterns[n%8]),floating(patterns[(n/8)%8]),floating(patterns[(n+3)%8])};target->ground_or_air=n%3;
        args(values4,UINT32_C(0x80000005));Vec3 native={0};CHECK(nativeFighterTarget_Position(UINT32_C(0x80000005),&native));
        CHECK(nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsReadPosition,values4,4,&output,&diagnostic)&&output.as.u64);
        CHECK(bits(values4[1].as.f32)==bits(native.x)&&bits(values4[2].as.f32)==bits(native.y)&&bits(values4[3].as.f32)==bits(native.z));
        CHECK(nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsAirborne,values4,1,&output,&diagnostic)&&output.as.u64==nativeFighterTarget_Airborne(UINT32_C(0x80000005)));
        CHECK(nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsIsFighter,values4,1,&output,&diagnostic)&&output.as.u64==nativeFighterTarget_IsFighter(UINT32_C(0x80000005)));++comparisons;
    }
    for(unsigned n=0;n<3;++n){args(values4,n==0?0:n==1?123456:UINT32_MAX);NativeScriptValue old[4];memcpy(old,values4,sizeof(old));CHECK(nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsReadPosition,values4,4,&output,&diagnostic)&&!output.as.u64);CHECK(memcmp(old,values4,sizeof(old))==0);}
    HSD_GObj* maximum=fighter(false,UINT32_MAX);args(values4,UINT32_MAX);values4[0]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=-1};
    CHECK(nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsReadPosition,values4,4,&output,&diagnostic)&&output.as.u64&&values4[1].as.f32==1.25f);HSD_GObjFree(maximum);
    HSD_GObj* wrong=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_FIGHTER,0);static Fighter wrongdata;wrongdata=(Fighter){.gobj=wrong,.x8_spawnNum=345};wrong->user_data=&wrongdata;args(values4,345);CHECK(nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsReadPosition,values4,4,&output,&diagnostic)&&!output.as.u64&&values4[1].as.f32==112);HSD_GObjFree(wrong);
    for(unsigned bad=0;bad<3;++bad){args(values4,5);if(bad==0)values4[0]=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000005)};if(bad==1)values4[0]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=5};if(bad==2)values4[2].type=NativeScriptType_I32;
        NativeScriptValue old[4];memcpy(old,values4,sizeof(old));output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(!nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsReadPosition,values4,4,&output,&diagnostic)&&output.type==NativeScriptType_U64&&output.as.u64==99&&memcmp(old,values4,sizeof(old))==0);}
    HSD_GObj* authored_target=fighter(false,UINT32_C(0x80000006));Fighter* authored=authored_target->user_data;authored->kind=authored->behavior_kind=Ft_Kind_CustomFirst+1;authored->ft_data=&target_foreign_package.assets.character.data;CHECK(nativeScriptFighter_Attach(authored_target));
    scalar_samples(f,a,authored_target);HSD_GObjFree(authored_target);scalar_samples(f,a,foreign);delta_samples(f,a,foreign);HSD_GObjFree(foreign);
    for(unsigned delta=0;delta<7;++delta)for(unsigned caller_kind=0;caller_kind<2;++caller_kind)for(unsigned mode=1;mode<=8;++mode){if(!caller_kind&&(mode==6||mode==7))continue;
        query_caller=caller_kind?item():fighter(true,30+fighter_count);query_target=fighter(false,UINT32_C(0x80000005));c=context(query_caller);args(values4,UINT32_C(0x80000005));NativeScriptValue old[4];memcpy(old,values4,sizeof(old));output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
        guard_calls=0;mutate_at=caller_kind?3:4;mutation=mode;target_reuse=mode==4?reuse:NULL;
        const uint32_t operations[]={NativeScriptBinding_TargetsReadPosition,NativeScriptBinding_TargetsReadPositionDelta,NativeScriptBinding_TargetsDamage,NativeScriptBinding_TargetsFacing,NativeScriptBinding_TargetsCpuBodyHalfWidth,NativeScriptBinding_TargetsSleeping,NativeScriptBinding_TargetsFloorLine};CHECK(!nativeScriptBindingTargets_Invoke(&c,operations[delta],values4,delta<2?4:1,&output,&diagnostic));CHECK(mutation==0&&output.type==NativeScriptType_U64&&output.as.u64==99&&memcmp(old,values4,sizeof(old))==0);
        if(mode==4){CHECK(replacement==query_target);HSD_GObjFree(replacement);}else HSD_GObjFree(query_target);if(mode!=5)HSD_GObjFree(query_caller);query_caller=NULL;target_reuse=NULL;
    }
    HSD_GObjFree(a);HSD_GObjFree(f);frontend();CHECK(nativeScriptRuntime_Idle());printf("{\"status\":\"PASS target queries\",\"checks\":%u,\"native_comparisons\":%u}\n",checks,comparisons);return 0;
}
