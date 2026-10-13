#include <melee/ft/native_script_bindings.h>
#include <melee/ft/native_script_binding_common_movement.h>
#include <melee/ft/native_script_binding_visuals.h>
#include <melee/ft/native_script_expression.h>
#include <melee/ft/native_script_statement.h>
#include <melee/ft/native_callback_lifetime.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks,calls;static uint32_t selected,captured_count;static NativeScriptValue captured[2];static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"common catalog %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
bool nativeScriptBindings_SourceAlive(NativeScriptRuntimeContext* c){return c&&c->generation==17;}
bool nativeScriptBindingCommonMovement_Invoke(NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    (void)c;(void)args;(void)out;(void)d;CHECK(semantic>=NativeScriptBinding_FacingFromInputThreshold&&semantic<=NativeScriptBinding_CollisionSkipSoftFloor);CHECK(count==0);
    selected=semantic;captured_count=count;++calls;return false; // Engine primitive intentionally unavailable.
}
bool nativeScriptBindingVisuals_Invoke(NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    (void)c;(void)out;(void)d;CHECK(semantic==NativeScriptBinding_VisualsSetVisible||semantic==NativeScriptBinding_VisualsSetGroup||semantic==NativeScriptBinding_ColorStart);CHECK(count<=2);
    selected=semantic;captured_count=count;for(unsigned i=0;i<count;++i)captured[i]=args[i];++calls;return false;
}
static NativeScriptIRNode expression(const char* source,uint32_t mask,bool accepted)
{
    NativeScriptIRNode nodes[128];uint32_t args[128],result=0;NativeScriptIRArena arena={.source=source,.source_bytes=(uint32_t)strlen(source),.nodes=nodes,.node_capacity=128,.arguments=args,.argument_capacity=128};
    NativeScriptBindingResolverContext context={mask};diagnostic=(NativeScriptDiagnostic){0};
    CHECK(nativeScriptExpression_Parse(&arena,(NativeScriptSourceSpan){.length=arena.source_bytes,.line=1,.column=1},nativeScriptBindings_Resolve,&context,&result,&diagnostic)==accepted);
    return accepted?nodes[result]:(NativeScriptIRNode){0};
}
static void vm(const char* call,uint32_t expected,unsigned argc)
{
    char source[512];snprintf(source,sizeof(source),"func Slice():\n    %s\n",call);
    NativeScriptSourceNode syntax[32];NativeScriptIRNode nodes[128];uint32_t args[128];NativeScriptLocal locals[4];
    NativeScriptSourceTree tree={.source=source,.source_bytes=(uint32_t)strlen(source),.nodes=syntax,.capacity=32};CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
    NativeScriptIRArena arena={.source=source,.source_bytes=tree.source_bytes,.nodes=nodes,.node_capacity=128,.arguments=args,.argument_capacity=128};
    NativeScriptBindingResolverContext context={NativeScriptBindings_Fighter};NativeScriptStatementCompiler body={.tree=&tree,.expressions=&arena,.bindings=nativeScriptBindings_Resolve,.binding_context=&context,.locals=locals,.local_capacity=4,.return_type=NativeScriptType_Void};
    CHECK(nativeScriptStatement_Compile(&body,1,&diagnostic));
    NativeScriptInstruction instructions[128];NativeScriptFunctionCode code[1];NativeScriptBytecodeBuilder builder={.instructions=instructions,.instruction_capacity=128,.functions=code,.function_count=1,.describe_call=nativeScriptBindings_DescribeCall,.binding_context=&context};
    CHECK(nativeScriptBytecode_Compile(&builder,&body,1,0,0,&diagnostic));
    NativeScriptBytecodeProgram program={source,tree.source_bytes,instructions,builder.instruction_count,code,1,false}; { NativeScriptDiagnostic preparation={0}; if(!nativeScriptBytecode_Prepare(&program,&preparation)){fprintf(stderr,"bytecode preparation: %s\n",preparation.message);abort();} }NativeScriptValue registers[64];NativeScriptVMFrame frames[4];NativeScriptVM machine={.registers=registers,.register_capacity=64,.frames=frames,.frame_capacity=4};
    NativeScriptRuntimeContext runtime={.source_class=NativeCallbackSource_Fighter,.generation=17};NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};calls=0;
    CHECK(nativeScriptVM_Run(&machine,&program,&nativeScriptBindingHost,&runtime,0,NULL,0,&result)==NativeScriptVM_Error);
    CHECK(calls==1&&selected==expected&&captured_count==argc);CHECK(result.type==NativeScriptType_U64&&result.as.u64==UINT64_MAX);
}
int main(void)
{
    const struct{const char* name;uint32_t id;unsigned count;} methods[]={
        {"facing.from_input_threshold()",NativeScriptBinding_FacingFromInputThreshold,0},
        {"physics.force_air_and_use_jumps()",NativeScriptBinding_PhysicsForceAirAndUseJumps,0},
        {"physics.switch_to_ground()",NativeScriptBinding_PhysicsSwitchToGround,0},
        {"collision.skip_soft_floor()",NativeScriptBinding_CollisionSkipSoftFloor,0},
        {"visuals.set_visible(false)",NativeScriptBinding_VisualsSetVisible,1},
        {"visuals.set_group(0x8000000000000000, -1)",NativeScriptBinding_VisualsSetGroup,2},
        {"color.start(0x8000000000000000, true)",NativeScriptBinding_ColorStart,2},
    };
    for(unsigned i=0;i<sizeof(methods)/sizeof(*methods);++i){
        NativeScriptIRNode node=expression(methods[i].name,NativeScriptBindings_Fighter,true);CHECK(node.semantic==methods[i].id);CHECK(node.type==(i==3?NativeScriptType_Bool:NativeScriptType_Void));
        CHECK(node.flags&NativeScriptFlag_MayInvalidateSource);expression(methods[i].name,NativeScriptBindings_Article,false);expression(methods[i].name,NativeScriptBindings_Effect,false);
        vm(methods[i].name,methods[i].id,methods[i].count);
        if(i==5){CHECK(captured[0].type==NativeScriptType_U64&&captured[0].as.u64==UINT64_C(9223372036854775808));CHECK(captured[1].type==NativeScriptType_I32&&captured[1].as.i64==-1);}
        if(i==6){CHECK(captured[0].type==NativeScriptType_U64&&captured[0].as.u64==UINT64_C(9223372036854775808));CHECK(captured[1].type==NativeScriptType_Bool&&captured[1].as.u64==1);}
    }
    const struct{const char* name;uint32_t id;uint8_t type;} fields[]={
        {"air.terminal_velocity",NativeScriptBinding_AirTerminalVelocity,NativeScriptType_F32},
        {"controls.stick_smoothing",NativeScriptBinding_ControlsStickSmoothing,NativeScriptType_F32},
        {"model.instance_scale_y",NativeScriptBinding_ModelInstanceScaleY,NativeScriptType_F32},
        {"model.scale",NativeScriptBinding_ModelScale,NativeScriptType_F32},
        {"ceiling.normal.x",NativeScriptBinding_CeilingNormalX,NativeScriptType_F32},
        {"ceiling.normal.y",NativeScriptBinding_CeilingNormalY,NativeScriptType_F32},
        {"wall.left.normal.x",NativeScriptBinding_WallLeftNormalX,NativeScriptType_F32},
        {"wall.left.normal.y",NativeScriptBinding_WallLeftNormalY,NativeScriptType_F32},
        {"wall.right.normal.x",NativeScriptBinding_WallRightNormalX,NativeScriptType_F32},
        {"wall.right.normal.y",NativeScriptBinding_WallRightNormalY,NativeScriptType_F32},
        {"ecb.top.y",NativeScriptBinding_ECBTopY,NativeScriptType_F32},
        {"ecb.bottom.y",NativeScriptBinding_ECBBottomY,NativeScriptType_F32},
        {"fighter.creation_id",NativeScriptBinding_FighterCreationID,NativeScriptType_U32},
    };
    for(unsigned i=0;i<sizeof(fields)/sizeof(*fields);++i){NativeScriptIRNode node=expression(fields[i].name,NativeScriptBindings_Fighter,true);CHECK(node.semantic==fields[i].id&&node.type==fields[i].type);
        CHECK((node.flags&(NativeScriptFlag_Readable|NativeScriptFlag_Pure|NativeScriptFlag_Writable))==(NativeScriptFlag_Readable|NativeScriptFlag_Pure));expression(fields[i].name,NativeScriptBindings_Article,false);expression(fields[i].name,NativeScriptBindings_Effect,false);}
    NativeScriptIRNode pickup=expression("items.block_ground_pickup",NativeScriptBindings_Fighter,true);CHECK(pickup.type==NativeScriptType_Bool);CHECK((pickup.flags&(NativeScriptFlag_Addressable|NativeScriptFlag_Writable))==(NativeScriptFlag_Addressable|NativeScriptFlag_Writable));
    expression("items.block_ground_pickup(true)",NativeScriptBindings_Fighter,false);
    const char* invalid[]={"visuals.set_group(0.5,1)","visuals.set_group(0,1.5)","color.start(2.5,true)","color.start(2,1)","visuals.set_visible(1)","physics.switch_to_ground(0)"};
    for(unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);++i)expression(invalid[i],NativeScriptBindings_Fighter,false);
    NativeScriptRuntimeContext runtime={.source_class=NativeCallbackSource_Fighter,.generation=17};NativeScriptValue input[2]={{.type=NativeScriptType_F32,.as.f32=0.5f},{.type=NativeScriptType_I32,.as.i64=1}},out={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};calls=0;
    CHECK(!nativeScriptBindings_Invoke(&runtime,NativeScriptBinding_VisualsSetGroup,NULL,0,input,2,&out,&diagnostic));CHECK(calls==0&&out.type==NativeScriptType_U64&&out.as.u64==UINT64_MAX);
    printf("{\"status\":\"PASS common typed catalog/VM routing\",\"checks\":%u,\"native_leaves_unavailable\":true}\n",checks);return 0;
}
