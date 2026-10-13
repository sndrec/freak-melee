#include <melee/ft/native_script_bindings.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/types.h>
#include <melee/ef/forward.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const char source[]="func Pulse():\n    pass\nfunc Other():\n    pass\nfunc Wrong(int value):\n    pass\nfunc Result() -> int:\n    return 1\narticle Foreign states 1:\n    func Alien():\n        pass\nfunc Bind():\n    callbacks.animation(Pulse)\n    callbacks.interrupt(Pulse)\n    callbacks.physics(Pulse)\n    callbacks.collision(Pulse)\n    callbacks.accessory(Pulse)\n    callbacks.accessory_cue(Other)\n    callbacks.accessory(None)\n";
static NativeScriptSourceNode syntax[128];static NativeScriptIRNode nodes[512];static uint32_t arguments[512];
static NativeScriptFunctionDeclaration functions[16];static NativeScriptParameterDeclaration parameters[16];static NativeScriptValueDeclaration values[16];
static NativeScriptSourceTree tree;static NativeScriptIRArena arena;static NativeScriptProgramIndex index;
static Fighter actor;static HSD_GObj object;static HSD_GObj* heads[32];HSD_GObj** HSD_GObjPLinkHead=heads;
static unsigned checks,calls,last_phase;static uint32_t last_function;static bool last_cue,attached,retire;
static uint32_t phases[NativeScriptMotion_Count];static NativeScriptDiagnostic diagnostic;
static NativeScriptRuntimeContext context;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"motion binding %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
bool nativeModelEffect_Snapshot(HSD_GObj* g,uint32_t* n){(void)g;(void)n;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* g,uint32_t n){(void)g;(void)n;return NULL;}
bool nativeScriptBindings_SourceAlive(NativeScriptRuntimeContext* c)
{return c&&nativeCallbackSource_Alive(c->source,c->generation,c->source_class)&&actor.gobj==c->source;}
bool nativeScriptFighter_BindMotionPhase(Fighter_GObj* g,uint32_t phase,uint32_t function,bool cue)
{
    CHECK(g==&object);++calls;last_phase=phase;last_function=function;last_cue=cue;
    if(!attached||phase>=NativeScriptMotion_Count||(cue&&phase!=NativeScriptMotion_Accessory))return false;
    if(function!=UINT32_MAX&&(function>=index.function_count||functions[function].owner||functions[function].callback||
        functions[function].parameter_count||functions[function].result_type!=NativeScriptType_Void))return false;
    phases[phase]=function;
    if(retire)++actor.x8_spawnNum;
    return true;
}
static uint32_t function(const char* name)
{for(uint32_t i=0;i<index.function_count;++i)if(nativeScriptSource_SpanEquals(&tree,functions[i].name,name))return i;CHECK(false);return UINT32_MAX;}
static void setup(void)
{
    actor=(Fighter){0};object=(HSD_GObj){.user_data=&actor};memset(heads,0,sizeof(heads));heads[HSD_GOBJ_PLINK_FIGHTER]=&object;
    actor.gobj=&object;actor.x8_spawnNum=17;context=(NativeScriptRuntimeContext){.source=&object,.source_class=NativeCallbackSource_Fighter,.generation=17};
    attached=true;retire=false;calls=0;for(unsigned i=0;i<NativeScriptMotion_Count;++i)phases[i]=UINT32_MAX;
}
static bool invoke(uint32_t semantic,NativeScriptValue input,NativeScriptValue* result)
{return nativeScriptBindings_Invoke(&context,semantic,NULL,0,&input,1,result,&diagnostic);}
static void semantics(void)
{
    tree=(NativeScriptSourceTree){.source=source,.source_bytes=sizeof(source)-1,.nodes=syntax,.capacity=128};CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
    arena=(NativeScriptIRArena){.source=source,.source_bytes=tree.source_bytes,.nodes=nodes,.node_capacity=512,.arguments=arguments,.argument_capacity=512};
    index=(NativeScriptProgramIndex){.tree=&tree,.expressions=&arena,.functions=functions,.function_capacity=16,.parameters=parameters,.parameter_capacity=16,.values=values,.value_capacity=16};CHECK(nativeScriptProgram_Index(&index,&diagnostic));
    NativeScriptBindingResolverContext native={NativeScriptBindings_Fighter};NativeScriptLocal locals[16];
    NativeScriptProgramCompiler compiler={.index=&index,.native_bindings=nativeScriptBindings_Resolve,.native_binding_context=&native,.locals=locals,.local_capacity=16};
    NativeScriptStatementCompiler body;uint32_t id=function("Bind");CHECK(nativeScriptProgram_CompileBody(&compiler,id,&body,&diagnostic));
    NativeScriptInstruction instructions[256];NativeScriptFunctionCode code[16];NativeScriptBytecodeBuilder builder={.instructions=instructions,.instruction_capacity=256,.functions=code,.function_count=index.function_count,.describe_call=nativeScriptBindings_DescribeCall,.binding_context=&native};
    CHECK(nativeScriptBytecode_Compile(&builder,&body,functions[id].source_node,id,0,&diagnostic));
    for(uint32_t semantic=NativeScriptBinding_CallbackAnimation;semantic<=NativeScriptBinding_CallbackAccessoryCue;++semantic){
        bool found=false;for(uint32_t n=0;n<builder.instruction_count;++n)if(instructions[n].semantic==semantic)found=true;CHECK(found);
        const NativeScriptBindingDescriptor* d=nativeScriptBindings_Descriptor(semantic);CHECK(d->result_type==NativeScriptType_Void&&d->argument_count==1&&d->arguments[0].mode==NativeScriptArgument_CodeRef);
    }
    const char* bad[]={"callbacks.animation(4)","callbacks.physics(0.5)","callbacks.accessory(\"Pulse\")"};
    for(unsigned i=0;i<3;++i){NativeScriptIRNode n[32];uint32_t a[32];NativeScriptIRArena one={.source=bad[i],.source_bytes=(uint32_t)strlen(bad[i]),.nodes=n,.node_capacity=32,.arguments=a,.argument_capacity=32};uint32_t expression;
        CHECK(!nativeScriptExpression_Parse(&one,(NativeScriptSourceSpan){.offset=0,.length=one.source_bytes,.line=1,.column=1},nativeScriptBindings_Resolve,&native,&expression,&diagnostic));}
    // Every CodeRef family rejects Callable values from a receiver/local/call
    // even if their flattened integer happens to match an owned helper ID.
    const uint32_t refs[]={NativeScriptBinding_CallbackAnimation,NativeScriptBinding_CallbackDamage};
    for(unsigned family=0;family<2;++family)for(unsigned variant=0;variant<5;++variant){
        NativeScriptIRNode value={.kind=NativeScriptIR_Name,.type=NativeScriptType_Callable,.semantic=NATIVE_SCRIPT_FUNCTION_SEMANTIC};
        if(variant==1)value.semantic=UINT32_C(0x01700001);
        if(variant==2)value.semantic=NATIVE_SCRIPT_LOCAL_SEMANTIC;
        if(variant==3)value.kind=NativeScriptIR_Call;
        if(variant==4){value.kind=NativeScriptIR_Null;value.type=NativeScriptType_Null;value.semantic=NATIVE_SCRIPT_SEMANTIC_NONE;}
        uint32_t slot=0;NativeScriptIRArena one={.nodes=&value,.node_count=1,.arguments=&slot,.argument_count=1};
        NativeScriptIRNode call={.kind=NativeScriptIR_Call,.argument_first=0,.argument_count=1};NativeScriptIRResolution resolved;
        CHECK(nativeScriptBindings_ResolveCall(&native,&one,&call,refs[family],&resolved,&diagnostic)==(variant==0||variant==4));
    }
}
int main(void)
{
    semantics();
    for(uint32_t semantic=NativeScriptBinding_CallbackAnimation;semantic<=NativeScriptBinding_CallbackAccessoryCue;++semantic){setup();NativeScriptValue helper={.type=NativeScriptType_Callable,.as.u64=function("Pulse")},result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
        CHECK(invoke(semantic,helper,&result));CHECK(result.type==NativeScriptType_Void&&calls==1&&last_function==helper.as.u64);
        CHECK(last_phase==(semantic==NativeScriptBinding_CallbackAccessoryCue?NativeScriptMotion_Accessory:semantic-NativeScriptBinding_CallbackAnimation));CHECK(last_cue==(semantic==NativeScriptBinding_CallbackAccessoryCue));
        helper=(NativeScriptValue){.type=NativeScriptType_Null};CHECK(invoke(semantic,helper,&result));CHECK(last_function==UINT32_MAX);
    }
    const char* bad_helpers[]={"Wrong","Result","Alien"};
    for(unsigned i=0;i<3;++i){setup();NativeScriptValue helper={.type=NativeScriptType_Callable,.as.u64=function(bad_helpers[i])},result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
        CHECK(!invoke(NativeScriptBinding_CallbackAnimation,helper,&result));CHECK(result.type==NativeScriptType_U64&&result.as.u64==UINT64_MAX&&phases[0]==UINT32_MAX);}
    setup();NativeScriptValue helper={.type=NativeScriptType_Callable,.as.u64=UINT64_C(0x100000000)},result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};CHECK(!invoke(NativeScriptBinding_CallbackAnimation,helper,&result));CHECK(calls==0);
    setup();attached=false;helper.as.u64=function("Pulse");CHECK(!invoke(NativeScriptBinding_CallbackAnimation,helper,&result));CHECK(result.type==NativeScriptType_U64);
    setup();retire=true;CHECK(!invoke(NativeScriptBinding_CallbackAccessoryCue,helper,&result));CHECK(result.type==NativeScriptType_U64&&result.as.u64==UINT64_MAX);
    printf("{\"status\":\"PASS motion callback code refs/dispatch\",\"checks\":%u,\"actor_leaf_controlled\":true}\n",checks);return 0;
}
