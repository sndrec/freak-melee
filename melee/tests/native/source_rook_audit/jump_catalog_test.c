#include <melee/ft/native_script_bindings.h>
#include <melee/ft/native_script_expression.h>
#include <melee/ft/native_script_statement.h>
#include <melee/ft/native_script_bytecode.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"jump catalog %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
static void body(const char* source,unsigned mask,bool accept)
{
    NativeScriptSourceNode syntax[128];NativeScriptIRNode nodes[512];uint32_t arguments[512];NativeScriptLocal locals[32];
    NativeScriptSourceTree tree={.source=source,.source_bytes=(uint32_t)strlen(source),.nodes=syntax,.capacity=128};
    NativeScriptIRArena arena={.source=source,.source_bytes=tree.source_bytes,.nodes=nodes,.node_capacity=512,.arguments=arguments,.argument_capacity=512};
    CHECK(nativeScriptSource_Parse(&tree,&diagnostic));uint32_t at=0;
    while(at<tree.count&&tree.nodes[at].kind!=NativeScriptSource_Function)++at;CHECK(at<tree.count);
    NativeScriptBindingResolverContext context={mask};
    NativeScriptStatementCompiler compiler={.tree=&tree,.expressions=&arena,.bindings=nativeScriptBindings_Resolve,.binding_context=&context,
        .locals=locals,.local_capacity=32,.return_type=NativeScriptType_Void};
    CHECK(nativeScriptStatement_Compile(&compiler,at,&diagnostic)==accept);
    if(!accept)return;
    NativeScriptInstruction instructions[1024];NativeScriptFunctionCode functions[1];
    NativeScriptBytecodeBuilder builder={.instructions=instructions,.instruction_capacity=1024,.functions=functions,.function_count=1,
        .describe_call=nativeScriptBindings_DescribeCall,.binding_context=&context};
    CHECK(nativeScriptBytecode_Compile(&builder,&compiler,at,0,0,&diagnostic));
    const uint32_t ids[]={NativeScriptBinding_JumpBegin,NativeScriptBinding_BonesRootAddRotationY,
        NativeScriptBinding_ControlsAerialJumpActions,NativeScriptBinding_CollisionTryWallJump,
        NativeScriptBinding_JumpsUsed,NativeScriptBinding_JumpsMax,NativeScriptBinding_CollisionBasicLandingVelocityThreshold};
    for(unsigned i=0;i<sizeof(ids)/sizeof(*ids);++i){bool found=false;
        for(uint32_t n=0;n<builder.instruction_count;++n)if(instructions[n].semantic==ids[i])found=true;
        CHECK(found);
    }
}
int main(void)
{
    NativeScriptBindingResolverContext context={NativeScriptBindings_Fighter};NativeScriptCallLayout layout;
    CHECK(nativeScriptBindings_DescribeCall(&context,NativeScriptBinding_JumpBegin,&layout,&diagnostic));
    CHECK(layout.argument_count==7&&nativeScriptBindings_Descriptor(NativeScriptBinding_JumpBegin)->result_type==NativeScriptType_Void);
    CHECK(layout.argument_types[0]==NativeScriptType_I32);
    for(unsigned i=1;i<4;++i)CHECK(layout.argument_types[i]==NativeScriptType_F32);
    for(unsigned i=4;i<7;++i)CHECK(layout.argument_types[i]==NativeScriptType_Bool);
    CHECK(nativeScriptBindings_Descriptor(NativeScriptBinding_ControlsAerialJumpActions)->result_type==NativeScriptType_Void);
    CHECK(nativeScriptBindings_Descriptor(NativeScriptBinding_ControlsTryAerialJump)->result_type==NativeScriptType_Bool);
    CHECK((nativeScriptBindings_Descriptor(NativeScriptBinding_JumpsUsed)->flags&NativeScriptFlag_Writable)!=0);
    CHECK((nativeScriptBindings_Descriptor(NativeScriptBinding_JumpsMax)->flags&NativeScriptFlag_Writable)==0);
    body("func Slice():\n    let int used = jumps.used\n    jumps.used = used + 1\n    let int maximum = jumps.max\n    let float threshold = collision.basic_landing_velocity_threshold\n    bones.root.add_rotation_y(0.05)\n    if collision.try_wall_jump(): return\n    controls.aerial_jump_actions()\n    jump.begin(341, input.x, 3, 0, false, false, true)\n",NativeScriptBindings_Fighter,true);
    body("func Bad():\n    jumps.max = 4\n",NativeScriptBindings_Fighter,false);
    body("func Bad():\n    collision.basic_landing_velocity_threshold = 1\n",NativeScriptBindings_Fighter,false);
    body("func Bad():\n    jump.begin(341, 1, 3, 0, 1, false, true)\n",NativeScriptBindings_Fighter,false);
    body("func Bad():\n    if controls.aerial_jump_actions(): pass\n",NativeScriptBindings_Fighter,false);
    body("func Bad():\n    jump.begin(341, 1, 3, 0, false, false, true)\n",NativeScriptBindings_Article,false);
    printf("{\"status\":\"PASS generic jump catalog/typed bytecode mapping\",\"checks\":%u}\n",checks);return 0;
}
