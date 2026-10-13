#include "boundary.h"
#include <melee/ft/native_script_binding_jump.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/kinds/ftCommon/ftCo_JumpAerial.h>
#include <melee/ft/kinds/ftCommon/ftCo_Attack100.h>
#include <melee/ft/ftwalljump.h>
#include <dolphin/gx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum{N=512,F=16,V=32};
static char source_storage[8192];static NativeScriptSourceNode nodes[N];static NativeScriptIRNode expressions[N];static uint32_t arguments[N];
static NativeScriptFunctionDeclaration functions[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];
static NativeScriptLocal locals[16];static NativeScriptInstruction instructions[1024];static NativeScriptFunctionCode code[F];static NativeScriptExecutionFunction execution[F];
static NativeScriptCompileFunctionScratch graph[F];static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[4];static NativeScriptParameterSlot slots[V];
static NativeScriptValue registers[1024];static NativeScriptVMFrame frames[16];static uint8_t bytes[64];
static NativeScriptDiagnostic diagnostic;static unsigned checks;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"jump %u %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
static bool alive(NativeScriptRuntimeContext* context){return nativeScriptBindings_SourceAlive(context) && jump_storage.live;}
static bool read_value(NativeScriptRuntimeContext*c,uint32_t semantic,const NativeScriptValue*s,uint32_t count,NativeScriptValue*v,NativeScriptDiagnostic*d)
{
    if((semantic&UINT32_C(0xf0000000))==NATIVE_SCRIPT_STATE_SEMANTIC)
        return nativeScriptStorage_Read(&jump_storage,semantic,s,count,v,d);
    return nativeScriptBindings_Read(c,semantic,s,count,v,d);
}
static bool write_value(NativeScriptRuntimeContext*c,uint32_t semantic,const NativeScriptValue*s,uint32_t count,const NativeScriptValue*v,NativeScriptDiagnostic*d)
{
    if((semantic&UINT32_C(0xf0000000))==NATIVE_SCRIPT_STATE_SEMANTIC)
        return nativeScriptStorage_Write(&jump_storage,semantic,s,count,v,d);
    return nativeScriptBindings_Write(c,semantic,s,count,v,d);
}
static const NativeScriptVMHost host={.source_alive=alive,.read=read_value,.write=write_value,.invoke=nativeScriptBindings_Invoke};
NativeScriptVMStatus jumpFixture_Run(uint32_t function)
{NativeScriptValue result;return nativeScriptVM_Run(&jump_vm,&jump_package.compiler.bank.code,&host,&jump_context,function,NULL,0,&result);}
static uint32_t named(const char* name)
{
    NativeScriptCompileBank* bank=&jump_package.compiler.bank;
    for(unsigned i=0;i<bank->declarations.function_count;++i)
        if(nativeScriptSource_SpanEquals(&bank->tree,bank->declarations.functions[i].name,name))return i;
    abort();
}
static int32_t state(const char* name)
{
    NativeScriptCompileBank* bank=&jump_package.compiler.bank;
    for(unsigned i=0;i<bank->declarations.value_count;++i)if(nativeScriptSource_SpanEquals(&bank->tree,bank->declarations.values[i].name,name)) {
        NativeScriptValue value;CHECK(nativeScriptStorage_Read(&jump_storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&value,&diagnostic));return (int32_t)value.as.i64;
    }
    abort();
}
static void prepare(void)
{
    NativeScriptCompileBank* bank=&jump_package.compiler.bank;
    *bank=(NativeScriptCompileBank){.source_storage=source_storage,.source_capacity=sizeof(source_storage),.tree={.nodes=nodes,.capacity=N},
        .expressions={.nodes=expressions,.node_capacity=N,.arguments=arguments,.argument_capacity=N},
        .declarations={.functions=functions,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},
        .locals=locals,.local_capacity=16,.instructions=instructions,.instruction_capacity=1024,.function_code=code,.function_code_capacity=F,
        .execution_functions=execution,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,
        .storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=4},.parameter_slots=slots,.parameter_slot_capacity=V};
    const char* source="fighter:\n    behavior independent\nstate int started\nstate int finished\nfunc Counter():\n    jumps.used = jumps.max\nfunc Turn():\n    bones.root.add_rotation_y(-0.25)\nfunc Unsupported():\n    jump.begin(341, 1, 3, 0, false, true, true)\n    state.finished = 999\naction Events:\n    on aerial_jump:\n        state.started = state.started + 1\n        jump.begin(341, input.x, 3, 0, false, false, true)\n        state.finished = state.finished + 1\naction JumpPhases:\n    on state_341_interrupt:\n        controls.aerial_jump_actions()\n        state.finished = state.finished + 1\n";
    NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=12};
    NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout};const NativeScriptCompileBank* accepted=NULL;
    CHECK(nativeScriptCompile_Build(bank,&request,&accepted,&diagnostic));
    CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,bank,&layout,NULL,&jump_package.program,&diagnostic));
    CHECK(jump_package.program.hooks[NativeScriptFighter_AerialJump]!=UINT32_MAX);
    CHECK(jump_package.program.motion_count==1 && jump_package.program.motions[0].motion==341);
    CHECK(jump_package.program.motions[0].functions[NativeScriptMotion_Interrupt]!=UINT32_MAX);
    nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&jump_package.program);
    jump_vm=(NativeScriptVM){.registers=registers,.register_capacity=1024,.frames=frames,.frame_capacity=16};
}
static void reset(void)
{
    jumpFixture_Reset();NativeScriptCompileBank* bank=&jump_package.compiler.bank;
    CHECK(nativeScriptStorage_Create(&bank->storage_layout,&bank->storage_parameters,0,17,17,bytes,sizeof(bytes),&jump_storage,&diagnostic));
}
static NativeScriptValue integer(int64_t n){return (NativeScriptValue){.type=NativeScriptType_I32,.as.i64=n};}
static NativeScriptValue real(float n){return (NativeScriptValue){.type=NativeScriptType_F32,.as.f32=n};}
static NativeScriptValue flag(bool value){return (NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=value};}
static bool begin(float x,float y,float z,bool timer,bool animation,bool sound)
{
    NativeScriptValue args[]={integer(341),real(x),real(y),real(z),flag(timer),flag(animation),flag(sound)},out={0};
    return nativeScriptBindings_Invoke(&jump_context,NativeScriptBinding_JumpBegin,NULL,0,args,7,&out,&diagnostic);
}
int main(void)
{
    prepare();
    for(unsigned timer=0;timer<2;++timer)for(unsigned worn=0;worn<2;++worn) {
        reset();Vec3 velocity={-0.8125f,3.0000002384185791f,-0.125f};
        ftCo_JumpAerial_Begin(&jump_reference_object,341,&velocity,timer!=0,false,worn!=0);
        CHECK(begin(velocity.x,velocity.y,velocity.z,timer!=0,false,worn!=0));
        CHECK(!memcmp(&jump_actor.self_vel,&jump_reference.self_vel,sizeof(Vec3)));
        CHECK(jump_actor.motion_id==jump_reference.motion_id && jump_actor.cur_anim_frame==jump_reference.cur_anim_frame);
        CHECK(jump_actor.x1968_jumpsUsed==jump_reference.x1968_jumpsUsed && jump_actor.x2221_b7==jump_reference.x2221_b7);
        CHECK(jump_actor.x671_timer_lstick_tilt_y==jump_reference.x671_timer_lstick_tilt_y);
        CHECK(jump_changes==2 && jump_reset==2 && jump_sounds==2 && jump_worn==2*worn);
    }
    reset();CHECK(!begin(1,3,0,false,true,true));CHECK(jump_changes==0 && jump_actor.x1968_jumpsUsed==1);
    CHECK(jumpFixture_Run(named("Unsupported"))==NativeScriptVM_Error);CHECK(jump_changes==0 && state("finished")==0);
    reset();NativeScriptValue bad[]={integer(353),real(1),real(3),real(0),flag(false),flag(false),flag(true)},out={.type=NativeScriptType_U64,.as.u64=91};
    CHECK(!nativeScriptBindingJump_Invoke(&jump_context,NativeScriptBinding_JumpBegin,bad,7,&out,&diagnostic));CHECK(jump_changes==0 && out.as.u64==91);
    reset();NativeScriptValue used=integer(255);
    CHECK(nativeScriptBindings_Write(&jump_context,NativeScriptBinding_JumpsUsed,NULL,0,&used,&diagnostic));
    CHECK(begin(1,3,0,false,false,false));CHECK(jump_actor.x1968_jumpsUsed==0);
    used=integer(-1);CHECK(!nativeScriptBindings_Write(&jump_context,NativeScriptBinding_JumpsUsed,NULL,0,&used,&diagnostic));
    used=integer(256);CHECK(!nativeScriptBindings_Write(&jump_context,NativeScriptBinding_JumpsUsed,NULL,0,&used,&diagnostic));
    CHECK(!nativeScriptBindings_Write(&jump_context,NativeScriptBinding_JumpsMax,NULL,0,&used,&diagnostic));
    reset();CHECK(jumpFixture_Run(named("Counter"))==NativeScriptVM_Ok && jump_actor.x1968_jumpsUsed==2);
    CHECK(jumpFixture_Run(named("Turn"))==NativeScriptVM_Ok && jump_root.rotate.y==1.75f);
    jump_root.flags|=JOBJ_USE_QUATERNION;CHECK(jumpFixture_Run(named("Turn"))==NativeScriptVM_Error && jump_root.rotate.y==1.75f);
    reset();jump_actor.x34_scale.y=1.25f;
    CHECK(nativeScriptBindings_Read(&jump_context,NativeScriptBinding_CollisionBasicLandingVelocityThreshold,NULL,0,&out,&diagnostic));
    CHECK(out.type==NativeScriptType_F32 && out.as.f32==-2.5f);

    // Both actual native entry sites dispatch the prepared code-ID event.
    reset();jump_actor.input.pressed_buttons=HSD_PAD_X;
    CHECK(ftCo_JumpAerial_CheckInput(&jump_object,false));
    CHECK(jump_dispatches==1 && jump_actor.motion_id==341 && jump_actor.self_vel.x==0.5f && jump_actor.self_vel.y==3 && jump_actor.x1968_jumpsUsed==2);
    CHECK(state("started")==1 && state("finished")==1);
    CHECK(!ftCo_JumpAerial_CheckInput(&jump_object,false) && jump_dispatches==1);
    reset();ftCo_800D74A4(&jump_object);
    CHECK(jump_dispatches==1 && jump_actor.self_vel.y==3 && state("finished")==1);
    reset();jump_hook=false;jump_actor.input.pressed_buttons=HSD_PAD_X;
    CHECK(ftCo_JumpAerial_CheckInput(&jump_object,false));CHECK(jump_dispatches==0 && jump_actor.self_vel.x==1.5f && jump_actor.self_vel.y==9);
    reset();jump_hook=false;jump_actor.can_multijump=true;jump_actor.input.pressed_buttons=HSD_PAD_X;
    struct Fighter_x2D0_t raw={.x0=1073741824,.x4=1,.x8=1,.x14={3,3,3,3,3},.x28=128,.x2C=341,.x30=-1};
    jump_actor.dat_attrs=&raw;jump_actor.x2D0=&raw;
    CHECK(ftCo_800D730C(&jump_object,false));CHECK(jump_dispatches==0 && jump_actor.motion_id==341 && jump_actor.self_vel.x==0.5f && jump_actor.self_vel.y==3);
    CHECK(!ftCo_800D730C(&jump_object,false));
    reset();jump_fault_at=20;ftCo_800D74A4(&jump_object);
    CHECK(jump_dispatches==1 && jump_changes==1 && jump_status==NativeScriptVM_Error && jump_reset==0 && state("finished")==0);
    reset();jump_retire_at=21;ftCo_800D74A4(&jump_object);
    CHECK(jump_dispatches==1 && jump_reset==1 && jump_sounds==0 && jump_status==NativeScriptVM_SourceRetired);
    reset();jump_retire_at=22;ftCo_800D74A4(&jump_object);
    CHECK(jump_sounds==1 && jump_worn==0 && jump_status==NativeScriptVM_SourceRetired);
    reset();jump_retire_at=20;ftCo_800D74A4(&jump_object);
    CHECK(jump_changes==1 && jump_reset==0 && jump_sounds==0 && jump_status==NativeScriptVM_SourceRetired);

    // Shared IASA order and command gating, plus terminal logical faults.
    reset();jump_actor.cmd_vars[0]=1;jump_actor.x1968_jumpsUsed=2;
    jump_reference.cmd_vars[0]=1;jump_reference.x1968_jumpsUsed=2;
    ftCo_JumpAerial_IASA(&jump_reference_object);char reference_order[64];strcpy(reference_order,jump_order);
    jump_stage=0;jump_order[0]=0;
    CHECK(jumpFixture_Run(named("state_341_interrupt"))==NativeScriptVM_Ok);
    CHECK(!strcmp(jump_order,reference_order) && !strcmp(jump_order,"ABCDEFGHIJK") && state("finished")==1);
    reset();jump_actor.x1968_jumpsUsed=2;
    CHECK(jumpFixture_Run(named("state_341_interrupt"))==NativeScriptVM_Ok);
    CHECK(!strcmp(jump_order,"ABCDEFGHI"));
    for(unsigned stop=1;stop<=8;++stop) {
        reset();jump_stop_at=stop;
        CHECK(jumpFixture_Run(named("state_341_interrupt"))==NativeScriptVM_Ok && jump_stage==stop);
    }
    reset();jump_fault_at=3;CHECK(jumpFixture_Run(named("state_341_interrupt"))==NativeScriptVM_Error);
    CHECK(!strcmp(jump_order,"ABC") && state("finished")==0);
    reset();jump_retire_at=4;CHECK(jumpFixture_Run(named("state_341_interrupt"))==NativeScriptVM_SourceRetired);
    CHECK(!strcmp(jump_order,"ABCD"));

    // Actual walljump arithmetic/entry; retire before the timer/used tail.
    reset();jump_actor.can_walljump=true;jump_actor.coll_data.env_flags=Collide_RightWallHug;
    jump_actor.x2110_walljumpWallSide=-1;jump_actor.input.lstick[0].x=0.75f;
    CHECK(nativeScriptBindingJump_Invoke(&jump_context,NativeScriptBinding_CollisionTryWallJump,NULL,0,&out,&diagnostic));
    CHECK(out.type==NativeScriptType_Bool && out.as.u64 && jump_changes==1 && jump_actor.wall_jump_input_timer==254 && jump_actor.x1969_walljumpUsed==1);
    reset();jump_actor.can_walljump=true;jump_actor.coll_data.env_flags=Collide_RightWallHug;
    jump_actor.x2110_walljumpWallSide=-1;jump_actor.input.lstick[0].x=0.75f;jump_retire_at=30;
    out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=91};
    CHECK(!nativeScriptBindingJump_Invoke(&jump_context,NativeScriptBinding_CollisionTryWallJump,NULL,0,&out,&diagnostic));
    CHECK(out.as.u64==91 && jump_actor.wall_jump_input_timer==1 && jump_actor.x1969_walljumpUsed==0);
    reset();jump_actor.can_walljump=true;jump_actor.coll_data.env_flags=Collide_RightWallHug;
    jump_actor.x2110_walljumpWallSide=-1;jump_actor.input.lstick[0].x=0.75f;jump_retire_at=31;
    out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=91};
    CHECK(!nativeScriptBindingJump_Invoke(&jump_context,NativeScriptBinding_CollisionTryWallJump,NULL,0,&out,&diagnostic));
    CHECK(out.as.u64==91 && jump_actor.wall_jump_input_timer==1 && jump_actor.x1969_walljumpUsed==0);
    printf("{\"status\":\"PASS generic source jump\",\"checks\":%u,\"functions\":%u,\"instructions\":%u,\"game\":false}\n",checks,jump_package.compiler.bank.declarations.function_count,jump_package.compiler.bank.code.instruction_count);
    return 0;
}
