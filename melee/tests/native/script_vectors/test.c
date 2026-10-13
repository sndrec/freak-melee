#include <melee/ft/native_script_bindings.h>
#include <melee/ft/native_script_binding_vectors.h>
#include <melee/ft/native_script_expression.h>
#include <melee/ft/native_script_statement.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/lb/lbvector.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,pairs,alive_calls,retire_at;static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"vectors %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
bool nativeScriptBindings_SourceAlive(NativeScriptRuntimeContext* c)
{++alive_calls;return c&&c->generation==17&&(!retire_at||alive_calls<retire_at);}
static uint32_t bits(float value){uint32_t word;memcpy(&word,&value,4);return word;}
static float expected(uint32_t id,Vec3 a,Vec3 b)
{
    if(id==NativeScriptBinding_VectorsAngleXY)return lbVector_AngleXY(&a,&b);
    if(id==NativeScriptBinding_VectorsAngle)return lbVector_Angle(&a,&b);
    Vec3 result;lbVector_CrossprodNormalized(&a,&b,&result);
    return id==NativeScriptBinding_VectorsCrossNormalizedX?result.x:id==NativeScriptBinding_VectorsCrossNormalizedY?result.y:result.z;
}
static unsigned pack(uint32_t id,Vec3 a,Vec3 b,NativeScriptValue args[6])
{
    const float values[6]={a.x,a.y,a.z,b.x,b.y,b.z};unsigned count=id==NativeScriptBinding_VectorsAngleXY?4:6;
    for(unsigned i=0;i<count;++i)args[i]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=count==4?values[(const unsigned[]){0,1,3,4}[i]]:values[i]};return count;
}
static void numeric(void)
{
    float denormal;uint32_t tiny=1;memcpy(&denormal,&tiny,4);
    const Vec3 values[]={{0,0,0},{-0.0f,0,-0.0f},{1,0,0},{0,1,0},{0,0,1},{-1,0,0},{1,2,3},{3,-5,7},{1e-6f,0,0},{0,1e-6f,0},{FLT_MIN,0,0},{denormal,denormal,0},{FLT_MAX,FLT_MAX,1},{INFINITY,1,0},{NAN,0,1},{1,2,INFINITY}};
    for(unsigned c=0;c<2;++c)for(unsigned a=0;a<sizeof(values)/sizeof(*values);++a)for(unsigned b=0;b<sizeof(values)/sizeof(*values);++b)
        for(uint32_t id=NativeScriptBinding_VectorsAngleXY;id<=NativeScriptBinding_VectorsCrossNormalizedZ;++id){
            NativeScriptRuntimeContext context={.source_class=c?NativeCallbackSource_Item:NativeCallbackSource_Fighter,.generation=17};NativeScriptValue args[6],out={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};unsigned count=pack(id,values[a],values[b],args);alive_calls=0;retire_at=0;
            float reference=expected(id,values[a],values[b]);CHECK(nativeScriptBindings_Invoke(&context,id,NULL,0,args,count,&out,&diagnostic));CHECK(out.type==NativeScriptType_F32);CHECK(bits(out.as.f32)==bits(reference));++pairs;
        }
    CHECK(expected(NativeScriptBinding_VectorsAngle,(Vec3){1e-6f,0,0},(Vec3){0,1e-6f,0})==0);
    CHECK(expected(NativeScriptBinding_VectorsAngleXY,(Vec3){1e-6f,0,99},(Vec3){0,1e-6f,-99})>1.5f);
    CHECK(expected(NativeScriptBinding_VectorsCrossNormalizedZ,(Vec3){1,0,0},(Vec3){0,1,0})==1);
    CHECK(expected(NativeScriptBinding_VectorsCrossNormalizedZ,(Vec3){0,1,0},(Vec3){1,0,0})==-1);
}
static void source_vm(const char* call,uint32_t mask,float reference,bool accepted,const char* setup)
{
    char source[512];snprintf(source,sizeof(source),"func Sample() -> float:\n%s    return %s\n",setup?setup:"",call);NativeScriptSourceNode syntax[32];NativeScriptIRNode nodes[128];uint32_t args[128];NativeScriptLocal locals[4];NativeScriptSourceTree tree={.source=source,.source_bytes=(uint32_t)strlen(source),.nodes=syntax,.capacity=32};CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
    NativeScriptIRArena arena={.source=source,.source_bytes=tree.source_bytes,.nodes=nodes,.node_capacity=128,.arguments=args,.argument_capacity=128};NativeScriptBindingResolverContext context={mask};NativeScriptStatementCompiler body={.tree=&tree,.expressions=&arena,.bindings=nativeScriptBindings_Resolve,.binding_context=&context,.locals=locals,.local_capacity=4,.return_type=NativeScriptType_F32};
    CHECK(nativeScriptStatement_Compile(&body,1,&diagnostic)==accepted);if(!accepted)return;
    NativeScriptInstruction instructions[128];NativeScriptFunctionCode code[1];NativeScriptBytecodeBuilder builder={.instructions=instructions,.instruction_capacity=128,.functions=code,.function_count=1,.describe_call=nativeScriptBindings_DescribeCall,.binding_context=&context};CHECK(nativeScriptBytecode_Compile(&builder,&body,1,0,0,&diagnostic));
    NativeScriptBytecodeProgram program={source,tree.source_bytes,instructions,builder.instruction_count,code,1,false}; { NativeScriptDiagnostic preparation={0}; if(!nativeScriptBytecode_Prepare(&program,&preparation)){fprintf(stderr,"bytecode preparation: %s\n",preparation.message);abort();} }NativeScriptValue registers[64];NativeScriptVMFrame frames[4];NativeScriptVM vm={.registers=registers,.register_capacity=64,.frames=frames,.frame_capacity=4};NativeScriptRuntimeContext runtime={.source_class=mask==NativeScriptBindings_Article?NativeCallbackSource_Item:NativeCallbackSource_Fighter,.generation=17};NativeScriptValue out;retire_at=0;alive_calls=0;
    CHECK(nativeScriptVM_Run(&vm,&program,&nativeScriptBindingHost,&runtime,0,NULL,0,&out)==NativeScriptVM_Ok);CHECK(out.type==NativeScriptType_F32&&bits(out.as.f32)==bits(reference));
}
static void guarded(void)
{
    NativeScriptRuntimeContext context={.source_class=NativeCallbackSource_Fighter,.generation=17};NativeScriptValue args[6],out={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};unsigned count=pack(NativeScriptBinding_VectorsAngle,(Vec3){1,2,3},(Vec3){4,5,6},args);
    retire_at=2;alive_calls=0;CHECK(!nativeScriptBindingVectors_Invoke(&context,NativeScriptBinding_VectorsAngle,args,count,&out,&diagnostic));CHECK(out.type==NativeScriptType_U64&&out.as.u64==UINT64_MAX);
    retire_at=4;alive_calls=0;CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_VectorsAngle,NULL,0,args,count,&out,&diagnostic));CHECK(out.type==NativeScriptType_U64&&out.as.u64==UINT64_MAX);
    retire_at=0;context.generation=16;CHECK(!nativeScriptBindingVectors_Invoke(&context,NativeScriptBinding_VectorsAngle,args,count,&out,&diagnostic));context.generation=17;args[0].type=NativeScriptType_F64;CHECK(!nativeScriptBindingVectors_Invoke(&context,NativeScriptBinding_VectorsAngle,args,count,&out,&diagnostic));CHECK(out.type==NativeScriptType_U64&&out.as.u64==UINT64_MAX);
    args[0].type=NativeScriptType_F32;CHECK(!nativeScriptBindingVectors_Invoke(&context,NativeScriptBinding_VectorsAngle,args,4,&out,&diagnostic));context.source_class=NativeCallbackSource_ModelEffect;CHECK(!nativeScriptBindingVectors_Invoke(&context,NativeScriptBinding_VectorsAngle,args,count,&out,&diagnostic));
}
static void normalization(void)
{
    const Vec3 vectors[]={{0,0,0},{-0.0f,0,-0.0f},{3,4,9},{-1,2,-3},
        {FLT_MIN,FLT_MIN,1},{FLT_MAX,FLT_MAX,1},{INFINITY,1,0},{NAN,1,2}};
    for(unsigned actor=0;actor<2;++actor)for(unsigned xy=0;xy<2;++xy) {
        uint32_t id=xy?NativeScriptBinding_VectorsNormalizeXY:NativeScriptBinding_VectorsNormalize;
        const NativeScriptBindingDescriptor* descriptor=nativeScriptBindings_Descriptor(id);
        CHECK(descriptor&&descriptor->result_type==NativeScriptType_Void&&descriptor->contexts==3);
        CHECK(descriptor->publication==NativeScriptOutputs_AlwaysAfterGuard);
        for(unsigned i=0;i<3;++i)CHECK(descriptor->arguments[i].mode==NativeScriptArgument_InputOutput);
        for(unsigned sample=0;sample<sizeof(vectors)/sizeof(*vectors);++sample) {
            Vec3 reference=vectors[sample];if(xy)lbVector_NormalizeXY(&reference);else lbVector_Normalize(&reference);
            NativeScriptValue args[3]={{.type=NativeScriptType_F32,.as.f32=vectors[sample].x},
                {.type=NativeScriptType_F32,.as.f32=vectors[sample].y},{.type=NativeScriptType_F32,.as.f32=vectors[sample].z}};
            NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
            NativeScriptRuntimeContext context={.source_class=actor?NativeCallbackSource_Item:NativeCallbackSource_Fighter,.generation=17};
            retire_at=0;alive_calls=0;
            CHECK(nativeScriptBindings_Invoke(&context,id,NULL,0,args,3,&result,&diagnostic));
            CHECK(result.type==NativeScriptType_Void);
            CHECK(bits(args[0].as.f32)==bits(reference.x)&&bits(args[1].as.f32)==bits(reference.y)&&bits(args[2].as.f32)==bits(reference.z));++pairs;
            NativeScriptValue before[3];memcpy(before,args,sizeof(args));result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
            retire_at=4;alive_calls=0;
            CHECK(!nativeScriptBindings_Invoke(&context,id,NULL,0,args,3,&result,&diagnostic));
            CHECK(!memcmp(before,args,sizeof(args))&&result.type==NativeScriptType_U64&&result.as.u64==UINT64_MAX);
        }
        const char* setup=xy?"    let float x = 3\n    let float y = 4\n    let float z = 9\n    vectors.normalize_xy(x, y, z)\n":
            "    let float x = 3\n    let float y = 4\n    let float z = 9\n    vectors.normalize(x, y, z)\n";
        Vec3 reference={3,4,9};if(xy)lbVector_NormalizeXY(&reference);else lbVector_Normalize(&reference);
        source_vm("x + y + z",actor?NativeScriptBindings_Article:NativeScriptBindings_Fighter,
            (reference.x+reference.y)+reference.z,true,setup);
    }
    source_vm("1",NativeScriptBindings_Fighter,0,false,"    vectors.normalize(3, 4, 9)\n");
    source_vm("1",NativeScriptBindings_Article,0,false,"    let int x = 3\n    let float y = 4\n    let float z = 9\n    vectors.normalize(x, y, z)\n");
    source_vm("1",NativeScriptBindings_Effect,0,false,"    let float x = 3\n    let float y = 4\n    let float z = 9\n    vectors.normalize(x, y, z)\n");
    NativeScriptValue args[3]={{.type=NativeScriptType_F64,.as.f64=3},
        {.type=NativeScriptType_F32,.as.f32=4},{.type=NativeScriptType_F32,.as.f32=9}};
    NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
    NativeScriptRuntimeContext context={.source_class=NativeCallbackSource_Fighter,.generation=17};
    retire_at=0;alive_calls=0;
    CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_VectorsNormalize,NULL,0,args,3,&result,&diagnostic));
    args[0]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=3};
    CHECK(!nativeScriptBindingVectors_Invoke(&context,NativeScriptBinding_VectorsNormalize,args,2,&result,&diagnostic));
    CHECK(result.type==NativeScriptType_U64&&result.as.u64==UINT64_MAX);
}
int main(void)
{
    numeric();const char* calls[]={"vectors.angle_xy(1,2,3,4)","vectors.angle(1,2,3,4,5,6)","vectors.cross_normalized_x(1,2,3,4,5,6)","vectors.cross_normalized_y(1,2,3,4,5,6)","vectors.cross_normalized_z(1,2,3,4,5,6)"};
    for(unsigned i=0;i<5;++i){const NativeScriptBindingDescriptor* d=nativeScriptBindings_Descriptor(NativeScriptBinding_VectorsAngleXY+i);CHECK(d&&d->result_type==NativeScriptType_F32&&d->contexts==3&&(d->flags&NativeScriptFlag_Pure));
        float reference=expected(NativeScriptBinding_VectorsAngleXY+i,(Vec3){1,2,i?3:0},(Vec3){i?4:3,i?5:4,i?6:0});source_vm(calls[i],NativeScriptBindings_Fighter,reference,true,NULL);source_vm(calls[i],NativeScriptBindings_Article,reference,true,NULL);source_vm(calls[i],NativeScriptBindings_Effect,0,false,NULL);}
    source_vm("vectors.angle_xy(1,2,3,4,5,6)",NativeScriptBindings_Article,0,false,NULL);source_vm("vectors.angle(1,2,3,4)",NativeScriptBindings_Fighter,0,false,NULL);source_vm("vectors.angle_xy(None,1,2,3)",NativeScriptBindings_Fighter,0,false,NULL);guarded();
    normalization();printf("{\"status\":\"PASS native vector numerical/typed VM comparisons\",\"checks\":%u,\"native_pairs\":%u,\"game_claim\":false}\n",checks,pairs);return 0;
}
