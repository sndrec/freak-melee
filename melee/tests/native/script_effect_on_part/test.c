#include <stdlib.h>
#include <stdio.h>
#include <melee/ft/native_script_expression.h>
#include <melee/ft/native_script_statement.h>
#include <melee/ft/native_script_bytecode.h>
#include <melee/ft/native_script_binding_cpu_control.h>
#include <melee/ft/native_script_binding_item_model_resolve.h>
#include <melee/ft/native_script_binding_article_reference_resolve.h>
#include <melee/ft/native_script_binding_signal_resolve.h>
#include <sysdolphin/baselib/generator.h>
#include <sysdolphin/baselib/psstructs.h>
#include <sysdolphin/baselib/class.h>
#include <melee/ef/native_script_effect_actor.h>
#include <melee/lb/native_model_joint.h>
#include "controlled_source.inc"
#include "catalog_frontend.inc"

HSD_JObjInfo hsdJObj;
/* Controlled object-kind registry; paired-effect linkage shares this ABI. */
u8 HSD_GObj_JObjKind = 3;
static HSD_Generator generators[16];
static unsigned allocations,deliveries,generator_count,admission_change;
static unsigned substitution;
static Fighter substituted_fighter;
static bool allocation_failure;
static HSD_JObj* delivered;

bool nativeScriptFighter_IsAttached(Fighter_GObj* actor)
{return actor==object&&view_available!=0;}
NativeScriptVMStatus nativeScriptEffect_Spawn(NativeScriptRuntimeContext* c,uint32_t owner,bool on_part,int32_t raw,bool deferred,NativeScriptDiagnostic* d)
{(void)c;(void)owner;(void)on_part;(void)raw;(void)deferred;(void)d;return NativeScriptVM_Error;}
void nativeModelEffect_Cancel(EF_Effect* effect){(void)effect;abort();}
NativeScriptVMStatus nativeScriptEffect_Status(HSD_GObj* actor)
{(void)actor;return NativeScriptVM_SourceRetired;}
const NativeScriptEffectProgram* nativeScriptEffect_Spec(HSD_GObj* actor)
{(void)actor;return NULL;}
HSD_JObj* nativeScriptEffect_Joint(HSD_GObj* actor,uint32_t generation,unsigned member)
{(void)actor;(void)generation;(void)member;return NULL;}
float nativeModelJoint_Read(HSD_JObj* joint,unsigned channel)
{(void)joint;(void)channel;abort();}
void nativeModelJoint_Write(HSD_JObj* joint,unsigned channel,float value)
{(void)joint;(void)channel;(void)value;abort();}
void __assert(char* file,u32 line,char* message)
{fprintf(stderr,"assert %s:%lu %s\n",file,line,message);abort();}

static void class_init(void)
{
    hsdJObj.parent.parent.head.flags|=1;
    if(admission_change==1)storage.live=false;
    if(admission_change==2)parts[29].joint=&joints[30];
}
HSD_Generator* hsd_8039F05C(s32 link,s32 bank,s32 id)
{
    CHECK(link==0&&bank==0xE&&id==0x36B7);++allocations;
    if(allocation_failure)return NULL;
    CHECK(generator_count<16);HSD_Generator* gen=&generators[generator_count++];
    memset(gen,0,sizeof(*gen));return gen;
}
#include "native_contract.inc"

EF_SpawnResult efSync_Spawn(s32 id,HSD_GObj* actor,...)
{
    CHECK(actor==object);++effects;last_effect=(int)id;EF_SpawnResult spawned={0};
    va_list vlist;va_start(vlist,actor);
    switch(id){
#include "native_sync_case.inc"
    default:abort();}
    va_end(vlist);++deliveries;
    delivered=spawned.object.generator?spawned.object.generator->jobj:NULL;
    if(substitution==1)object->user_data=&substituted_fighter;
    if(substitution==2)object->classifier=HSD_GOBJ_CLASS_ITEM;
    if(substitution==3)HSD_CLASS_METHOD(&joints[29])=NULL;
    change(OnEffect);return spawned;
}
static void setup(void)
{
    base_setup();object->classifier=HSD_GOBJ_CLASS_FIGHTER;
    memset(&hsdJObj,0,sizeof(hsdJObj));
    hsdJObj.parent.parent.head.obj_size=sizeof(HSD_JObj);
    hsdJObj.parent.parent.head.flags=1;hsdJObj.parent.parent.head.info_init=class_init;
    for(unsigned i=0;i<140;++i)HSD_CLASS_METHOD(&joints[i])=HSD_CLASS_INFO(&hsdJObj);
    allocations=deliveries=generator_count=admission_change=substitution=0;allocation_failure=false;delivered=NULL;
}
static bool call(NativeScriptValue id,NativeScriptValue part,NativeScriptValue* output)
{
    NativeScriptValue args[]={id,part};
    return nativeScriptBindingEffect_Invoke(&context,NativeScriptBinding_EffectsSpawnOnPart,NULL,0,args,2,output,&diagnostic);
}
static void denied(NativeScriptValue id,NativeScriptValue part)
{
    NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};unsigned before=effects;
    CHECK(!call(id,part,&out));CHECK(out.type==NativeScriptType_U64&&out.as.u64==UINT64_MAX&&effects==before);++cases;
}
static void native_comparison(void)
{
    setup();reference_spawn_on_part(object,0x4EC,29);
    CHECK(delivered==&joints[29]&&HSD_OBJ(&joints[29])->ref_count==1&&allocations==1);++cases;
    setup();NativeScriptValue out={0};CHECK(call(i32(0x4EC),i32(29),&out));
    CHECK(out.type==NativeScriptType_Void&&delivered==&joints[29]&&HSD_OBJ(&joints[29])->ref_count==1);
    CHECK(package.assets.part_to_joint[29]!=29&&delivered!=&joints[package.assets.part_to_joint[29]]);
    CHECK(allocations==1&&deliveries==1&&!fighter.x60C);++cases;
    setup();allocation_failure=true;CHECK(call(i32(0x4EC),i32(29),&out));
    CHECK(out.type==NativeScriptType_Void&&effects==1&&allocations==1&&!delivered&&!HSD_OBJ(&joints[29])->ref_count);++cases;
    for(unsigned i=0;i<3;++i){setup();int32_t part=i==0?-1:i==1?140:29;if(i==2)parts[29].joint=NULL;
        CHECK(call(i32(0x4EC),i32(part),&out)&&out.type==NativeScriptType_Void&&!effects&&!allocations);++cases;}
    for(unsigned type=0;type<3;++type){setup();NativeScriptValue id=type==0?i32(0x4EC):type==1?u32v(0x4EC):(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=0x4EC};
        CHECK(call(id,i32(29),&out)&&delivered==&joints[29]);++cases;}
}
static void negative_contract(void)
{
    setup();denied(float_value(0x4EC),i32(29));denied(i32(-1),i32(29));denied(u32v(65536),i32(29));
    denied((NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0x1000004EC)},i32(29));
    denied((NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=2},i32(29));
    denied(i32(0x47A),i32(29));denied(i32(0x505),i32(29));denied(i32(0x4EC),u32v(29));
    denied(i32(0x4EC),(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=INT64_C(0x10000001D)});
    setup();context.scene_epoch++;denied(i32(0x4EC),i32(29));
    setup();object->classifier=HSD_GOBJ_CLASS_ITEM;denied(i32(0x4EC),i32(29));
    setup();storage.live=false;denied(i32(0x4EC),i32(29));
    setup();actor_status=NativeScriptVM_Error;denied(i32(0x4EC),i32(29));
    setup();package.assets.parts.parts_num=139;denied(i32(0x4EC),i32(29));
    setup();fighter.parts=NULL;denied(i32(0x4EC),i32(29));
    setup();HSD_CLASS_METHOD(&joints[29])=NULL;denied(i32(0x4EC),i32(29));
    setup();hsdJObj.parent.parent.head.obj_size=sizeof(HSD_JObj)-1;denied(i32(0x4EC),i32(29));
    for(unsigned m=1;m<=2;++m){setup();hsdJObj.parent.parent.head.flags=0;admission_change=m;denied(i32(0x4EC),i32(29));}
    for(unsigned m=1;m<=ReuseSlot;++m){setup();mutation=m;mutate_on=OnEffect;
        NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};CHECK(!call(i32(0x4EC),i32(29),&out));
        CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&effects==1&&allocations==1);++cases;}
    for(unsigned m=1;m<=3;++m){setup();substitution=m;
        NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};CHECK(!call(i32(0x4EC),i32(29),&out));
        CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&effects==1&&allocations==1);++cases;}
}
static bool vm_invoke(NativeScriptRuntimeContext* c,uint32_t op,const NativeScriptValue* s,uint32_t ns,NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{return nativeScriptBindingEffect_Invoke(c,op,s,ns,args,count,out,d);}
static void source_vm(void)
{
    static const char source[]="func Pulse():\n    effects.spawn_on_part(0x4EC, 29)\n    effects.spawn_on_part(0x4EC, 29)\n";
    NativeScriptSourceNode nodes[32];NativeScriptIRNode ir[64];uint32_t arguments[32];NativeScriptLocal locals[8];
    NativeScriptSourceTree tree={.source=source,.source_bytes=sizeof(source)-1,.nodes=nodes,.capacity=32};
    NativeScriptIRArena arena={.source=source,.source_bytes=sizeof(source)-1,.nodes=ir,.node_capacity=64,.arguments=arguments,.argument_capacity=32};
    NativeScriptBindingResolverContext resolver={NativeScriptBindings_Fighter};
    NativeScriptStatementCompiler statements={.tree=&tree,.expressions=&arena,.bindings=nativeScriptBindings_Resolve,.binding_context=&resolver,.locals=locals,.local_capacity=8,.return_type=NativeScriptType_Void};
    CHECK(nativeScriptSource_Parse(&tree,&diagnostic));unsigned fn=0;for(unsigned i=0;i<tree.count;++i)if(nodes[i].kind==NativeScriptSource_Function)fn=i;
    CHECK(nativeScriptStatement_Compile(&statements,fn,&diagnostic));NativeScriptInstruction instructions[64];NativeScriptFunctionCode functions[1]={0};
    NativeScriptBytecodeBuilder builder={.instructions=instructions,.instruction_capacity=64,.functions=functions,.function_count=1,.describe_call=nativeScriptBindings_DescribeCall,.binding_context=&resolver};
    CHECK(nativeScriptBytecode_Compile(&builder,&statements,fn,0,0,&diagnostic));
    NativeScriptBytecodeProgram program={source,sizeof(source)-1,instructions,builder.instruction_count,functions,1,false}; { NativeScriptDiagnostic preparation={0}; if(!nativeScriptBytecode_Prepare(&program,&preparation)){fprintf(stderr,"bytecode preparation: %s\n",preparation.message);abort();} }
    NativeScriptValue registers[64],out={0};NativeScriptVMFrame frames[4];
    NativeScriptVM vm={.registers=registers,.register_capacity=64,.frames=frames,.frame_capacity=4};
    NativeScriptVMHost host={.source_alive=nativeScriptBindings_SourceAlive,.invoke=vm_invoke};
    setup();CHECK(nativeScriptVM_Run(&vm,&program,&host,&context,0,NULL,0,&out)==NativeScriptVM_Ok);
    CHECK(deliveries==2&&delivered==&joints[29]&&HSD_OBJ(&joints[29])->ref_count==2);++cases;
    setup();mutation=RetireStorage;mutate_on=OnEffect;
    CHECK(nativeScriptVM_Run(&vm,&program,&host,&context,0,NULL,0,&out)!=NativeScriptVM_Ok&&deliveries==1);++cases;
    const char* invalid[]={"effects.spawn_on_part(1260.0, 29)","effects.spawn_on_part(0x4EC)","effects.spawn_on_part(0x4EC, 29, 0)","effects.spawn_on_joint(0x4EC, 29)"};
    for(unsigned i=0;i<4;++i){arena.source=invalid[i];arena.source_bytes=(uint32_t)strlen(invalid[i]);arena.node_count=arena.argument_count=0;uint32_t node;
        CHECK(!nativeScriptExpression_Parse(&arena,(NativeScriptSourceSpan){0,arena.source_bytes,1,1},nativeScriptBindings_Resolve,&resolver,&node,&diagnostic));++cases;}
    arena.source="effects.spawn_on_part(0x4EC, 29)";arena.source_bytes=(uint32_t)strlen(arena.source);arena.node_count=arena.argument_count=0;
    resolver.context_mask=NativeScriptBindings_Article;uint32_t node;
    CHECK(!nativeScriptExpression_Parse(&arena,(NativeScriptSourceSpan){0,arena.source_bytes,1,1},nativeScriptBindings_Resolve,&resolver,&node,&diagnostic));++cases;
}
int main(void)
{native_comparison();negative_contract();source_vm();printf("{\"status\":\"PASS canonical raw-part effect\",\"checks\":%u,\"cases\":%u,\"retirement_mutations\":%u}\n",checks,cases,mutations);return 0;}
