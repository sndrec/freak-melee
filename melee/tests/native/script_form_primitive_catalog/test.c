#include <melee/ft/native_script_bindings.h>
#include <melee/ft/native_script_binding_effect.h>
#include <melee/ft/native_script_expression.h>
#include <melee/ft/native_script_statement.h>
#include <melee/ft/native_script_bytecode.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/types.h>
#include <melee/ef/forward.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,calls;static uint32_t observed_semantic,observed_count;
static NativeScriptValue observed[16];static NativeScriptDiagnostic diagnostic;
static Fighter actor;static HSD_GObj object;static HSD_GObj* heads[32];HSD_GObj** HSD_GObjPLinkHead=heads;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"form catalog %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
bool nativeModelEffect_Snapshot(HSD_GObj* g,uint32_t* n){(void)g;(void)n;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* g,uint32_t n){(void)g;(void)n;return NULL;}
bool nativeScriptBindings_SourceAlive(NativeScriptRuntimeContext* c)
{return c&&nativeCallbackSource_Alive(c->source,c->generation,c->source_class);}
static bool unavailable(uint32_t s,const NativeScriptValue* a,uint32_t n)
{++calls;observed_semantic=s;observed_count=n;for(uint32_t i=0;i<n;++i)observed[i]=a[i];return false;}
bool nativeScriptBindingFeedback_Invoke(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* a,uint32_t n,NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)c;(void)v;(void)d;return unavailable(s,a,n);}
bool nativeScriptBindingTransition_Invoke(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* a,uint32_t n,NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)c;(void)v;(void)d;return unavailable(s,a,n);}
bool nativeScriptBindingEffect_Read(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* a,uint32_t n,NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)a;(void)n;(void)v;(void)d;return false;}
bool nativeScriptBindingEffect_Write(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* a,uint32_t n,const NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)a;(void)n;(void)v;(void)d;return false;}
bool nativeScriptBindingEffect_Invoke(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* selectors,uint32_t count,NativeScriptValue* a,uint32_t n,NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)c;(void)selectors;(void)count;(void)v;(void)d;return unavailable(s,a,n);}
static void compile(const char* text,unsigned mask,bool accept)
{
    NativeScriptSourceNode syntax[64];NativeScriptIRNode nodes[256];uint32_t args[256];NativeScriptLocal locals[8];
    NativeScriptSourceTree tree={.source=text,.source_bytes=(uint32_t)strlen(text),.nodes=syntax,.capacity=64};CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
    NativeScriptIRArena arena={.source=text,.source_bytes=tree.source_bytes,.nodes=nodes,.node_capacity=256,.arguments=args,.argument_capacity=256};
    NativeScriptBindingResolverContext context={mask};NativeScriptStatementCompiler compiler={.tree=&tree,.expressions=&arena,.bindings=nativeScriptBindings_Resolve,.binding_context=&context,
        .locals=locals,.local_capacity=8,.return_type=NativeScriptType_Void};
    CHECK(nativeScriptStatement_Compile(&compiler,1,&diagnostic)==accept);if(!accept)return;
    NativeScriptInstruction instructions[256];NativeScriptFunctionCode code[1];NativeScriptBytecodeBuilder builder={.instructions=instructions,.instruction_capacity=256,.functions=code,.function_count=1,
        .describe_call=nativeScriptBindings_DescribeCall,.binding_context=&context};CHECK(nativeScriptBytecode_Compile(&builder,&compiler,1,0,0,&diagnostic));
    const uint32_t ids[]={NativeScriptBinding_EffectsSpawnOnPart,NativeScriptBinding_EffectsWindAtPosition,NativeScriptBinding_StateEnterAtSpeed,NativeScriptBinding_EffectsSpawnWithScalar,NativeScriptBinding_EffectsSpawnOnJointWithScalar};
    for(unsigned i=0;i<5;++i){bool found=false;for(uint32_t n=0;n<builder.instruction_count;++n)if(instructions[n].semantic==ids[i])found=true;CHECK(found);}
}
int main(void)
{
    const uint32_t ids[]={NativeScriptBinding_EffectsSpawnOnPart,NativeScriptBinding_EffectsWindAtPosition,NativeScriptBinding_StateEnterAtSpeed,NativeScriptBinding_EffectsSpawnWithScalar,NativeScriptBinding_EffectsSpawnOnJointWithScalar};
    NativeScriptBindingResolverContext resolver={NativeScriptBindings_Fighter};NativeScriptCallLayout layout;
    for(unsigned i=0;i<5;++i){const NativeScriptBindingDescriptor* d=nativeScriptBindings_Descriptor(ids[i]);CHECK(d->result_type==NativeScriptType_Void&&(d->flags&NativeScriptFlag_MayInvalidateSource));CHECK(nativeScriptBindings_DescribeCall(&resolver,ids[i],&layout,&diagnostic));CHECK(layout.argument_count==(i==0?2:i==1?7:i==2?4:3));}
    CHECK(nativeScriptBindings_Descriptor(ids[0])->arguments[0].numeric_any&&nativeScriptBindings_Descriptor(ids[0])->arguments[1].type==NativeScriptType_I32);
    CHECK(nativeScriptBindings_Descriptor(ids[1])->arguments[3].type==NativeScriptType_I32);
    for(unsigned i=3;i<5;++i)CHECK(nativeScriptBindings_Descriptor(ids[i])->arguments[0].type==NativeScriptType_I32&&nativeScriptBindings_Descriptor(ids[i])->arguments[1].type==NativeScriptType_U32&&nativeScriptBindings_Descriptor(ids[i])->arguments[2].type==NativeScriptType_F32);
    CHECK(nativeScriptBindings_DescribeCall(&resolver,ids[2],&layout,&diagnostic)&&layout.argument_types[0]==NativeScriptType_Invalid&&layout.argument_types[1]==NativeScriptType_U32&&layout.argument_types[2]==NativeScriptType_F32&&layout.argument_types[3]==NativeScriptType_F32);
    compile("func Slice():\n    effects.spawn_on_part(1276, 104)\n    effects.wind_at_position(position.x, position.y, position.z, 120, 0.9, 0.02, 1.0)\n    state.enter_at_speed(343, uint(0x80000000), frame, 0)\n    effects.spawn_with_scalar(1146, 1, facing.direction)\n    effects.spawn_on_joint_with_scalar(1146, 2, -1)\n",NativeScriptBindings_Fighter,true);
    compile("func Bad():\n    state.enter_at_speed(343.0, uint(0), 1, 0)\n",NativeScriptBindings_Fighter,false);
    compile("func Bad():\n    effects.spawn_on_part(1276, 1.5)\n",NativeScriptBindings_Fighter,false);
    compile("func Bad():\n    effects.wind_at_position(1, 2, 3, 4.5, 1, 1, 1)\n",NativeScriptBindings_Fighter,false);
    compile("func Bad():\n    effects.spawn_on_part(1276, 104)\n",NativeScriptBindings_Article,false);
    actor=(Fighter){.x8_spawnNum=17};object=(HSD_GObj){.user_data=&actor};actor.gobj=&object;heads[HSD_GOBJ_PLINK_FIGHTER]=&object;
    NativeScriptRuntimeContext context={.source=&object,.source_class=NativeCallbackSource_Fighter,.generation=17};
    for(unsigned i=0;i<5;++i){NativeScriptValue args[7]={0};const NativeScriptBindingDescriptor* d=nativeScriptBindings_Descriptor(ids[i]);
        for(unsigned a=0;a<d->argument_count;++a){args[a].type=d->arguments[a].type;if(args[a].type==NativeScriptType_F32)args[a].as.f32=1.25f;else if(args[a].type==NativeScriptType_I32)args[a].as.i64=1276;else args[a].as.u64=UINT32_C(0x80000000);}
        if(i==0)args[0]=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0x1000004FC)};
        if(i==2)args[0]=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000155)};
        NativeScriptValue before[7];memcpy(before,args,sizeof(args));NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};calls=0;
        CHECK(!nativeScriptBindings_Invoke(&context,ids[i],NULL,0,args,d->argument_count,&result,&diagnostic));CHECK(calls==1&&observed_semantic==ids[i]&&observed_count==d->argument_count);
        CHECK(!memcmp(before,args,sizeof(args))&&!memcmp(args,observed,d->argument_count*sizeof(*args)));CHECK(result.type==NativeScriptType_U64&&result.as.u64==UINT64_MAX);
    }
    printf("{\"status\":\"PASS form primitive catalog/bytecode/dispatch\",\"checks\":%u,\"native_leaves_unavailable\":true}\n",checks);return 0;
}
