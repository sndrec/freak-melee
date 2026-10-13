#include <stdlib.h>
#include <stdio.h>
#include <melee/ft/native_script_binding_cpu_control.h>
#include <melee/ft/native_script_binding_targets.h>
#include <melee/ft/native_script_binding_item_model_resolve.h>
#include <melee/ft/native_script_binding_article_reference_resolve.h>
#include <melee/ft/native_script_binding_signal_resolve.h>
#include <melee/ft/native_script_expression.h>
#include <melee/ft/native_script_statement.h>
#include <melee/ft/native_script_bytecode.h>
#include <melee/ft/native_script_receiver_semantics.h>
#include <sysdolphin/baselib/psstructs.h>
#include <sysdolphin/baselib/psappsrt.h>
#include <melee/ef/eflib.h>
#include <melee/cm/camera.h>
#include <sysdolphin/baselib/generator.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/particle.h>
#include <sysdolphin/baselib/class.h>
#include "controlled_source.inc"
#include "catalog_frontend.inc"

HSD_JObjInfo hsdJObj;
u8 HSD_GObj_JObjKind;
HSD_ObjAllocData efAsync_AllocData;
HSD_GObjProc* HSD_GObj_CurrentInvokedProc;
static HSD_GObjProc proc;
static EF_QueuedEffect nodes[8];
static bool occupied[8],allocation_failure,generator_failure;
static unsigned allocations,frees,peak,used,deliveries,generator_count;
static HSD_JObj* delivered[32];
static HSD_Generator generators[32];
static unsigned admission_change;
static unsigned scalar_deliveries;
static int scalar_id;
static HSD_JObj* scalar_joint;
static float scalar_value;
static void class_init(void)
{ hsdJObj.parent.parent.head.flags|=1; if(admission_change==1)storage.live=false;
  if(admission_change==2)parts[11].joint=&joints[12]; }

void* HSD_ObjAlloc(HSD_ObjAllocData* data)
{
    CHECK(data==&efAsync_AllocData);++allocations;
    if(allocation_failure)return NULL;
    for(unsigned i=0;i<8;++i)if(!occupied[i]){occupied[i]=true;++used;if(used>peak)peak=used;memset(&nodes[i],0xCD,sizeof(nodes[i]));return &nodes[i];}
    return NULL;
}
void HSD_ObjFree(HSD_ObjAllocData* data,void* value)
{CHECK(data==&efAsync_AllocData);for(unsigned i=0;i<8;++i)if(value==&nodes[i]){CHECK(occupied[i]);occupied[i]=false;--used;++frees;return;}abort();}
void HSD_ObjAllocInit(HSD_ObjAllocData* data,size_t size,u32 align)
{CHECK(data==&efAsync_AllocData&&size==sizeof(EF_QueuedEffect)&&align==sizeof(EF_QueuedEffect*));}
HSD_Generator* hsd_8039F05C(s32 link,s32 bank,s32 id)
{CHECK(link==0&&bank==0&&id==286);if(generator_failure)return NULL;CHECK(generator_count<32);HSD_Generator* gen=&generators[generator_count++];memset(gen,0,sizeof(*gen));return gen;}
void nativeModelEffect_Spawn(Fighter_GObj* actor,const NativeFighterModelEffect* policy,s32 part,Vec3* pos,bool deferred)
{(void)actor;(void)policy;(void)part;(void)pos;(void)deferred;abort();}
void Camera_RequestQuake(CmQuakeKind id,Vec3* position){(void)id;(void)position;abort();}
void __assert(char* file,u32 line,char* message){fprintf(stderr,"assert %s:%lu %s\n",file,line,message);abort();}
void OSReport(char* format,...){(void)format;abort();}
#include "native_queue.inc"
EF_SpawnResult efSync_Spawn(s32 id,HSD_GObj* actor,...)
{
    CHECK(actor==object);EF_SpawnResult spawned={0};va_list vlist;va_start(vlist,actor);
    switch(id){
    // Controlled rendering leaves; the native queue must copy and deliver the
    // joint and Float according to the actual dispatcher admission catalog.
    case 0x490:case 0x50c:
        scalar_id=id;scalar_joint=va_arg(vlist,HSD_JObj*);
        scalar_value=*va_arg(vlist,float*);++scalar_deliveries;break;
#include "native_sync_case.inc"
    default:abort();}
    va_end(vlist);CHECK(deliveries<32);delivered[deliveries++]=spawned.object.generator?spawned.object.generator->jobj:NULL;
    change(OnEffect);return spawned;
}
// Unrelated encoded effect/CPU families are absent, never successful leaves.
bool nativeScriptBindingEffect_DescribeCall(void* c,uint32_t id,NativeScriptCallLayout* l,NativeScriptDiagnostic* d)
{(void)c;(void)id;(void)l;(void)d;return false;}

static void setup(void)
{
    base_setup();memset(occupied,0,sizeof(occupied));memset(&hsdJObj,0,sizeof(hsdJObj));
    hsdJObj.parent.parent.head.obj_size=sizeof(HSD_JObj);hsdJObj.parent.parent.head.flags=1;hsdJObj.parent.parent.head.info_init=class_init;
    for(unsigned i=0;i<140;++i)HSD_CLASS_METHOD(&joints[i])=HSD_CLASS_INFO(&hsdJObj);
    allocations=frees=peak=used=deliveries=generator_count=admission_change=0;
    scalar_deliveries=0;scalar_joint=NULL;scalar_value=0;scalar_id=0;
    allocation_failure=generator_failure=false;HSD_GObj_CurrentInvokedProc=NULL;proc.s_link=8;
    efAsync_QueueInit();
}
static bool call(unsigned part,NativeScriptValue* output)
{NativeScriptValue args[]={i32(1236),u32v(part)};return invoke(NativeScriptBinding_EffectsSpawnAsyncAttached,args,2,output);}
static void queue_contract(void)
{
    for(unsigned mode=0;mode<3;++mode){setup();NativeScriptValue out={0};if(mode){proc.s_link=mode==1?8:9;HSD_GObj_CurrentInvokedProc=&proc;}
        fighter.x2219_b0=true;CHECK(call(4,&out));CHECK(out.type==NativeScriptType_Void&&fighter.x2219_b0);
        CHECK(mode==1?(used==1&&deliveries==0&&fighter.x60C):(used==0&&deliveries==1&&!fighter.x60C));
        if(mode==1){EF_QueuedEffect* n=fighter.x60C;CHECK(n->spawn_kind==EF_SPAWN_ATTACH&&n->gfx_id==1236&&n->jobj==&joints[11]&&!n->native_policy&&!n->native_author);CHECK(HSD_OBJ(&joints[11])->ref_count==0);efAsync_QueueFlush(object,&fighter.x60C);}
        CHECK(!fighter.x60C&&used==0&&allocations==1&&frees==1&&deliveries==1&&delivered[0]==&joints[11]&&HSD_OBJ(&joints[11])->ref_count==1);++cases;
    }
    setup();HSD_GObj_CurrentInvokedProc=&proc;NativeScriptValue out={0};
    for(unsigned p=0;p<8;++p)CHECK(call(p,&out));CHECK(peak==8&&used==8&&!deliveries);
    CHECK(call(8,&out)&&used==8&&allocations==9);efAsync_QueueFlush(object,&fighter.x60C);
    for(unsigned p=0;p<8;++p)CHECK(delivered[p]==&joints[package.assets.part_to_joint[7-p]]);
    CHECK(!used&&frees==8);++cases;
    setup();HSD_GObj_CurrentInvokedProc=&proc;CHECK(call(4,&out));CHECK(call(5,&out));efAsync_QueueClear(&fighter.x60C);
    CHECK(!fighter.x60C&&!used&&!deliveries&&frees==2&&HSD_OBJ(&joints[11])->ref_count==0);++cases;
    setup();allocation_failure=true;CHECK(call(4,&out)&&out.type==NativeScriptType_Void&&!used&&!deliveries);++cases;
    setup();generator_failure=true;CHECK(call(4,&out)&&deliveries==1&&!delivered[0]&&!used&&HSD_OBJ(&joints[11])->ref_count==0);++cases;
}
static void negative_contract(void)
{
    const NativeScriptValue ids[]={ {.type=NativeScriptType_I64,.as.i64=-1}, {.type=NativeScriptType_U64,.as.u64=UINT64_C(0x1000004d4)},
        {.type=NativeScriptType_I32,.as.i64=1146},{.type=NativeScriptType_I32,.as.i64=1234},{.type=NativeScriptType_F32,.as.f32=1236} };
    for(unsigned i=0;i<sizeof(ids)/sizeof(*ids);++i){setup();NativeScriptValue args[]={ids[i],u32v(4)};reject(NativeScriptBinding_EffectsSpawnAsyncAttached,args,2);CHECK(!allocations);++cases;}
    for(unsigned m=1;m<=13;++m){setup();mutation=m;mutate_on=OnEffect;NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};CHECK(!call(4,&out));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&deliveries==1&&!used);++cases;}
    for(unsigned m=1;m<=2;++m){setup();hsdJObj.parent.parent.head.flags=0;admission_change=m;NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};CHECK(!call(4,&out));CHECK(!allocations&&out.as.u64==99);++cases;}
    setup();HSD_CLASS_METHOD(&joints[11])=NULL;NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};CHECK(!call(4,&out)&&!allocations&&out.as.u64==99);++cases;
    setup();hsdJObj.parent.parent.head.obj_size=sizeof(HSD_JObj)-1;CHECK(!call(4,&out)&&!allocations);++cases;
}
static void scalar_contract(void)
{
    int ids[]={0x490,0x50c};float values[]={0.0f,-0.0f,0.25f,-1.75f};
    for(unsigned id=0;id<2;++id)for(unsigned v=0;v<4;++v)for(unsigned deferred=0;deferred<2;++deferred){
        setup();if(deferred)HSD_GObj_CurrentInvokedProc=&proc;
        NativeScriptValue args[]={i32(ids[id]),u32v(4),float_value(values[v])},out={0};
        CHECK(invoke(NativeScriptBinding_EffectsSpawnAsyncWithScalar,args,3,&out));
        CHECK(out.type==NativeScriptType_Void);
        if(deferred){EF_QueuedEffect* node=fighter.x60C;CHECK(node&&node->spawn_kind==EF_SPAWN_ATTACH_PARAM&&node->gfx_id==ids[id]);
            CHECK(node->jobj==&joints[11]&&!memcmp(&node->params.x,&values[v],sizeof(float))&&!scalar_deliveries);
            args[2].as.f32=99;efAsync_QueueFlush(object,&fighter.x60C);}
        CHECK(scalar_deliveries==1&&scalar_id==ids[id]&&scalar_joint==&joints[11]);
        CHECK(!memcmp(&scalar_value,&values[v],sizeof(float))&&!used&&!fighter.x60C);++cases;
    }
    for(unsigned invalid=0;invalid<4;++invalid){setup();NativeScriptValue args[]={i32(0x490),u32v(4),float_value(.25f)};
        if(invalid==0)args[0]=i32(0x4d4);if(invalid==1)args[2].as.f32=NAN;
        if(invalid==2)args[2].as.f32=INFINITY;if(invalid==3)args[2]=(NativeScriptValue){.type=NativeScriptType_F64,.as.f64=.25};
        reject(NativeScriptBinding_EffectsSpawnAsyncWithScalar,args,3);CHECK(!allocations&&!scalar_deliveries);++cases;}
    for(unsigned m=1;m<=13;++m){setup();mutation=m;mutate_on=OnEffect;
        NativeScriptValue args[]={i32(0x490),u32v(4),float_value(.25f)},out={.type=NativeScriptType_U64,.as.u64=99};
        CHECK(!invoke(NativeScriptBinding_EffectsSpawnAsyncWithScalar,args,3,&out));
        CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&scalar_deliveries==1&&!used);++cases;}
}
static bool vm_invoke(NativeScriptRuntimeContext* c,uint32_t op,const NativeScriptValue* selectors,uint32_t ns,NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{CHECK(!ns);(void)selectors;return nativeScriptBindingFeedback_Invoke(c,op,args,count,out,d);}
static void source_vm(void)
{
    static const char source[]="func Pulse():\n    effects.spawn_async_attached(1236, 4)\n    effects.spawn_async_with_scalar(1168, 4, 0.25)\n";
    NativeScriptSourceNode source_nodes[32];NativeScriptIRNode ir[64];uint32_t arguments[32];NativeScriptLocal locals[8];
    NativeScriptSourceTree tree={.source=source,.source_bytes=sizeof(source)-1,.nodes=source_nodes,.capacity=32};
    NativeScriptIRArena arena={.source=source,.source_bytes=sizeof(source)-1,.nodes=ir,.node_capacity=64,.arguments=arguments,.argument_capacity=32};
    NativeScriptBindingResolverContext resolver={NativeScriptBindings_Fighter};
    NativeScriptStatementCompiler statements={.tree=&tree,.expressions=&arena,.bindings=nativeScriptBindings_Resolve,.binding_context=&resolver,.locals=locals,.local_capacity=8,.return_type=NativeScriptType_Void};
    CHECK(nativeScriptSource_Parse(&tree,&diagnostic));unsigned fn=0;for(unsigned i=0;i<tree.count;++i)if(tree.nodes[i].kind==NativeScriptSource_Function)fn=i;
    CHECK(nativeScriptStatement_Compile(&statements,fn,&diagnostic));NativeScriptInstruction instructions[64];NativeScriptFunctionCode function[1]={0};
    NativeScriptBytecodeBuilder builder={.instructions=instructions,.instruction_capacity=64,.functions=function,.function_count=1,.describe_call=nativeScriptBindings_DescribeCall,.binding_context=&resolver};
    CHECK(nativeScriptBytecode_Compile(&builder,&statements,fn,0,0,&diagnostic));
    NativeScriptBytecodeProgram program={source,sizeof(source)-1,instructions,builder.instruction_count,function,1,false}; { NativeScriptDiagnostic preparation={0}; if(!nativeScriptBytecode_Prepare(&program,&preparation)){fprintf(stderr,"bytecode preparation: %s\n",preparation.message);abort();} }
    NativeScriptValue regs[64],out={0};NativeScriptVMFrame frames[4];NativeScriptVM vm={.registers=regs,.register_capacity=64,.frames=frames,.frame_capacity=4};
    NativeScriptVMHost host={.source_alive=nativeScriptBindings_SourceAlive,.invoke=vm_invoke};setup();HSD_GObj_CurrentInvokedProc=&proc;
    CHECK(nativeScriptVM_Run(&vm,&program,&host,&context,0,NULL,0,&out)==NativeScriptVM_Ok);CHECK(used==2&&!deliveries);
    efAsync_QueueFlush(object,&fighter.x60C);CHECK(deliveries==2&&delivered[1]==&joints[11]);
    CHECK(scalar_deliveries==1&&scalar_id==0x490&&scalar_joint==&joints[11]&&scalar_value==.25f);++cases;
    const char* invalid[]={"effects.spawn_async_attached(1236.0, 4)","effects.spawn_async_attached(1236)","effects.spawn_async_attached(1236, 4, 5)"};
    for(unsigned i=0;i<3;++i){arena.source=invalid[i];arena.source_bytes=(uint32_t)strlen(invalid[i]);arena.node_count=arena.argument_count=0;uint32_t node;
        CHECK(!nativeScriptExpression_Parse(&arena,(NativeScriptSourceSpan){0,arena.source_bytes,1,1},nativeScriptBindings_Resolve,&resolver,&node,&diagnostic));++cases;}
    arena.source="effects.spawn_async_attached(1236, 4)";arena.source_bytes=(uint32_t)strlen(arena.source);arena.node_count=arena.argument_count=0;
    resolver.context_mask=NativeScriptBindings_Article;uint32_t node;
    CHECK(!nativeScriptExpression_Parse(&arena,(NativeScriptSourceSpan){0,arena.source_bytes,1,1},nativeScriptBindings_Resolve,&resolver,&node,&diagnostic));++cases;
}
int main(void)
{queue_contract();negative_contract();scalar_contract();source_vm();printf("{\"status\":\"PASS async effect\",\"checks\":%u,\"cases\":%u,\"queue_capacity\":8}\n",checks,cases);return 0;}
