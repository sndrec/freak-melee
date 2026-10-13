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
#include <melee/ef/eflib.h>
#include <sysdolphin/baselib/robj.h>
#include "controlled_source.inc"
#include "catalog_frontend.inc"

HSD_JObjInfo hsdJObj;
u8 HSD_GObj_JObjKind=3;
static HSD_JObj root,replacement_root,effect_roots[16];
static HSD_GObj effect_objects[16];static EF_Effect native_effects[16];
static HSD_RObj constraints[32];
static unsigned attempts,created,constraint_count,drains,vm_calls,comparisons;
static unsigned failure_at,admission_change,substitution;
static uint32_t constructor_ids[16],drain_ids[16];
volatile s32 efLib_AnimCount;u8 efLib_LoadKind;
HSD_JObj* efLib_AnimQueue[32];
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
    if(admission_change==3)object->hsd_obj=&replacement_root;
}
void HSD_JObjSetMtxDirtySub(HSD_JObj* joint){joint->flags|=JOBJ_MTX_DIRTY;}
void HSD_JObjSetupMatrixSub(HSD_JObj* joint){(void)joint;abort();}
void C_MTXMultVec(Mtx m,Vec3* v,Vec3* o){(void)m;(void)v;(void)o;abort();}
void HSD_GObjFree(HSD_GObj* actor){(void)actor;abort();}
HSD_RObj* HSD_RObjAlloc(void)
{CHECK(constraint_count<32);return &constraints[constraint_count++];}
void HSD_RObjSetFlags(HSD_RObj* r,u32 flags){r->flags=flags;}
void HSD_RObjSetConstraintObj(HSD_RObj* r,void* value)
{r->u.jobj=value;if(value)++HSD_OBJ((HSD_JObj*)value)->ref_count;}
void HSD_JObjPrependRObj(HSD_JObj* joint,HSD_RObj* r){r->next=joint->robj;joint->robj=r;}
void efLib_Cb_SetRotY_FromFighterDir(EF_Effect* value){(void)value;abort();}
EF_Effect* efLib_Create(int id,HSD_GObj* parent)
{
    ++attempts;if(attempts==failure_at)return NULL;CHECK(created<16);
    unsigned i=created++;constructor_ids[i]=(uint32_t)id;
    native_effects[i].gobj=&effect_objects[i];native_effects[i].parent_gobj=parent;
    native_effects[i].native_generation=i+1;
    effect_objects[i].hsd_obj=&effect_roots[i];
    effect_roots[i].flags=JOBJ_MTX_DIRTY;
    efLib_AnimQueue[efLib_AnimCount++]=&effect_roots[i];
    return &native_effects[i];
}
#include "native_contract.inc"
void HSD_JObjAnimAll(HSD_JObj* joint)
{
    CHECK(joint>=effect_roots&&joint<effect_roots+16);CHECK(drains<16);
    drain_ids[drains++]=(uint32_t)(joint-effect_roots);
    if(substitution==1)parts[43].joint=&joints[44];
    if(substitution==2)HSD_CLASS_METHOD(&joints[43])=NULL;
    if(substitution==3)object->hsd_obj=&replacement_root;
    if(substitution==4)HSD_CLASS_METHOD(&root)=NULL;
    if(substitution==5)object->obj_kind=9;
    change(OnEffect);
}
#define EFALT_VA_ARG(type) va_arg(vlist,type)
EF_SpawnResult efSync_Spawn(s32 id,HSD_GObj* actor,...)
{
    CHECK(actor==object);++effects;last_effect=id;
    HSD_GObj* gobj=actor;EF_SpawnResult spawned={0};s32 cnt_2;efLib_AnimCount=0;efLib_LoadKind=EF_LOADKIND_SYNC;
    va_list vlist;va_start(vlist,actor);
    switch(id){
#include "native_sync_cases.inc"
    default:abort();}
#include "native_drain.inc"
    va_end(vlist);return spawned;
}
static void setup(void)
{
    base_setup();object->classifier=HSD_GOBJ_CLASS_FIGHTER;
    memset(&hsdJObj,0,sizeof(hsdJObj));hsdJObj.parent.parent.head.obj_size=sizeof(HSD_JObj);
    hsdJObj.parent.parent.head.flags=1;hsdJObj.parent.parent.head.info_init=class_init;
    for(unsigned i=0;i<140;++i)HSD_CLASS_METHOD(&joints[i])=HSD_CLASS_INFO(&hsdJObj);
    memset(&root,0,sizeof(root));memset(&replacement_root,0,sizeof(replacement_root));
    HSD_CLASS_METHOD(&root)=HSD_CLASS_INFO(&hsdJObj);root.scale=(Vec3){2,-3,4};
    object->hsd_obj=&root;object->obj_kind=HSD_GObj_JObjKind;
    memset(native_effects,0,sizeof(native_effects));memset(effect_roots,0,sizeof(effect_roots));
    memset(effect_objects,0,sizeof(effect_objects));memset(constraints,0,sizeof(constraints));
    attempts=created=constraint_count=drains=failure_at=admission_change=substitution=0;
    memset(constructor_ids,0,sizeof(constructor_ids));memset(drain_ids,0,sizeof(drain_ids));
}
static bool call(NativeScriptValue id,NativeScriptValue first,NativeScriptValue second,NativeScriptValue* out)
{NativeScriptValue args[]={id,first,second};return nativeScriptBindingEffect_Invoke(&context,NativeScriptBinding_EffectsSpawnBetween,NULL,0,args,3,out,&diagnostic);}
static void denied(NativeScriptValue id,NativeScriptValue first,NativeScriptValue second)
{
    NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};unsigned before=effects;
    CHECK(!call(id,first,second,&out));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&effects==before);++cases;
}
typedef struct Observation {
    uint32_t attempts,created,constraints,drains,ids[16],order[16];
    Vec3 translation[16],scale[16];int32_t attached[16];uint32_t callback[16],flags[32],target[32],refs[140];
} Observation;
static Observation observe(void)
{
    Observation o={.attempts=attempts,.created=created,.constraints=constraint_count,.drains=drains};
    memcpy(o.ids,constructor_ids,sizeof(o.ids));memcpy(o.order,drain_ids,sizeof(o.order));
    for(unsigned i=0;i<created;++i){o.translation[i]=effect_roots[i].translate;o.scale[i]=effect_roots[i].scale;
        o.attached[i]=(int32_t)(native_effects[i].attach_jobj-joints);
        o.callback[i]=native_effects[i].update==efLib_Cb_SetRotY_FromFighterDir;
        CHECK(native_effects[i].parent_gobj==object);}
    for(unsigned i=0;i<constraint_count;++i){o.flags[i]=constraints[i].flags;o.target[i]=(uint32_t)(constraints[i].u.jobj-joints);}
    for(unsigned i=0;i<140;++i)o.refs[i]=HSD_OBJ(&joints[i])->ref_count;
    return o;
}
static void native_comparison(void)
{
    const int ids[]={1211,1212,1167,1291};
    for(unsigned id=0;id<4;++id)for(unsigned fail=0;fail<3;++fail)for(unsigned same=0;same<2;++same){
        int second=same?29:43;setup();failure_at=fail;reference_spawn_between(object,ids[id],29,second);Observation expected=observe();
        setup();failure_at=fail;NativeScriptValue out={0};CHECK(call(i32(ids[id]),i32(29),i32(second),&out));
        Observation actual=observe();CHECK(!memcmp(&expected,&actual,sizeof(actual)));++comparisons;
        CHECK(out.type==NativeScriptType_Void&&!fighter.x60C&&!fighter.x2219_b0);
        if(!fail){CHECK(created==2&&drains==2&&drain_ids[0]==1&&drain_ids[1]==0);
            CHECK(native_effects[0].next==&native_effects[1]);CHECK(effect_roots[0].scale.y==-3&&effect_roots[1].scale.x==-3);}
        if(fail==1)CHECK(attempts==1&&!created&&!drains);
        if(fail==2)CHECK(attempts==2&&created==1&&drains==1&&!native_effects[0].next);
        ++cases;
    }
    for(unsigned i=0;i<6;++i){setup();int first=29,second=43;if(i==0)first=-1;if(i==1)second=-1;
        if(i==2)first=140;if(i==3)second=140;if(i==4)parts[first].joint=NULL;if(i==5)parts[second].joint=NULL;
        NativeScriptValue out={0};CHECK(call(i32(1211),i32(first),i32(second),&out)&&out.type==NativeScriptType_Void&&!effects&&!attempts);++cases;}
    setup();NativeScriptValue out={0};CHECK(call((NativeScriptValue){.type=NativeScriptType_U64,.as.u64=1211},i32(29),i32(43),&out));++cases;
}
static void negative_contract(void)
{
    setup();denied(float_value(1211),i32(29),i32(43));denied(i32(-1),i32(29),i32(43));denied(u32v(65536),i32(29),i32(43));
    denied((NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0x1000004BB)},i32(29),i32(43));
    denied((NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=2},i32(29),i32(43));
    denied(i32(0x4EC),i32(29),i32(43));denied(i32(0x47A),i32(29),i32(43));denied(i32(0x505),i32(29),i32(43));
    denied(i32(1211),u32v(29),i32(43));denied(i32(1211),i32(29),u32v(43));
    denied(i32(1211),i32(29),(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=INT64_C(0x10000002B)});
    setup();context.scene_epoch++;denied(i32(1211),i32(29),i32(43));
    setup();actor_status=NativeScriptVM_Error;denied(i32(1211),i32(29),i32(43));
    setup();object->hsd_obj=NULL;denied(i32(1211),i32(29),i32(43));
    setup();object->obj_kind=9;denied(i32(1211),i32(29),i32(43));
    setup();hsdJObj.parent.parent.head.obj_size=sizeof(HSD_JObj)-1;denied(i32(1211),i32(29),i32(43));
    setup();HSD_CLASS_METHOD(&joints[43])=NULL;denied(i32(1211),i32(29),i32(43));
    for(unsigned m=1;m<=3;++m){setup();hsdJObj.parent.parent.head.flags=0;admission_change=m;denied(i32(1211),i32(29),i32(43));}
    for(unsigned m=1;m<=ReuseSlot;++m){setup();mutation=m;mutate_on=OnEffect;NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
        CHECK(!call(i32(1211),i32(29),i32(43),&out));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&created==2&&drains==2);++cases;}
    for(unsigned m=1;m<=5;++m){setup();substitution=m;NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
        CHECK(!call(i32(1211),i32(29),i32(43),&out));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&created==2);++cases;}
    for(int32_t id=0;id<=65535;++id){bool expected=id==1211||id==1212||id==1167||id==1291;
        CHECK((nativeScriptEffectDispatch_Arguments(id)==NativeScriptEffectArguments_TwoJoint)==expected);}
    CHECK(nativeScriptEffectDispatch_Arguments(-1)==NativeScriptEffectArguments_Unsupported);
    CHECK(nativeScriptEffectDispatch_Arguments(65536)==NativeScriptEffectArguments_Unsupported);
}
static bool vm_invoke(NativeScriptRuntimeContext* c,uint32_t op,const NativeScriptValue* s,uint32_t ns,NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{++vm_calls;return nativeScriptBindingEffect_Invoke(c,op,s,ns,args,count,out,d);}
static void source_vm(void)
{
    static const char source[]="func Pulse():\n    effects.spawn_between(1211, 29, 43)\n    effects.spawn_between(1211, 29, 43)\n";
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
    setup();vm_calls=0;CHECK(nativeScriptVM_Run(&vm,&program,&host,&context,0,NULL,0,&out)==NativeScriptVM_Ok);
    CHECK(vm_calls==2&&created==4&&HSD_OBJ(&joints[29])->ref_count==2&&HSD_OBJ(&joints[43])->ref_count==4);++cases;
    setup();vm_calls=0;mutation=RetireStorage;mutate_on=OnEffect;
    CHECK(nativeScriptVM_Run(&vm,&program,&host,&context,0,NULL,0,&out)!=NativeScriptVM_Ok&&vm_calls==1);++cases;
    const char* invalid[]={"effects.spawn_between(1211.0, 29, 43)","effects.spawn_between(1211, 29)","effects.spawn_between(1211, 29, 43, 0)","effects.spawn_between_missing(1211, 29, 43)"};
    for(unsigned i=0;i<4;++i){arena.source=invalid[i];arena.source_bytes=(uint32_t)strlen(invalid[i]);arena.node_count=arena.argument_count=0;uint32_t node;
        CHECK(!nativeScriptExpression_Parse(&arena,(NativeScriptSourceSpan){0,arena.source_bytes,1,1},nativeScriptBindings_Resolve,&resolver,&node,&diagnostic));++cases;}
    arena.source="effects.spawn_between(1211, 29, 43)";arena.source_bytes=(uint32_t)strlen(arena.source);arena.node_count=arena.argument_count=0;
    resolver.context_mask=NativeScriptBindings_Article;uint32_t node;
    CHECK(!nativeScriptExpression_Parse(&arena,(NativeScriptSourceSpan){0,arena.source_bytes,1,1},nativeScriptBindings_Resolve,&resolver,&node,&diagnostic));++cases;
}
int main(void)
{native_comparison();negative_contract();source_vm();printf("{\"status\":\"PASS synchronous two-joint effect\",\"checks\":%u,\"cases\":%u,\"native_comparisons\":%u,\"retirement_mutations\":%u}\n",checks,cases,comparisons,mutations);return 0;}
