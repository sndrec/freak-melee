#include "engine_fixture.h"
#include <melee/ft/ftaction.h>
#include <melee/ft/kinds/ftCommon/ftCo_Attack100.h>
#include <melee/ft/native_script_bindings.h>
#include <melee/ft/native_script_expression.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ftanim.h>
#include <melee/lb/lbanim.h>
#include <melee/ef/forward.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

HSD_GObj** HSD_GObjPLinkHead;
static HSD_GObj* heads[32];
static HSD_GObj object,reference_object;
static Fighter actor,reference;
static FighterBone parts[3];static HSD_JObj joints[6];
static NativeScriptRuntimeContext context;
static NativeScriptDiagnostic diagnostic;
static unsigned checks,pairs,double_differences;
#define CHECK(value) do { ++checks; if (!(value)) { fprintf(stderr,"Kinetic check %d: %s (%s)\n",__LINE__,#value,diagnostic.message);exit(1); } } while (0)

bool nativeModelEffect_Snapshot(HSD_GObj* g,uint32_t* n){(void)g;(void)n;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* g,uint32_t n){(void)g;(void)n;return NULL;}

static void setup(unsigned sample)
{
    scriptKineticsFixture_Reset();memset(&actor,0,sizeof(actor));memset(&object,0,sizeof(object));
    memset(heads,0,sizeof(heads));memset(parts,0,sizeof(parts));memset(joints,0,sizeof(joints));
    HSD_GObjPLinkHead=heads;heads[HSD_GOBJ_PLINK_FIGHTER]=&object;
    object.user_data=&actor;actor.gobj=&object;actor.x8_spawnNum=(int32_t)UINT32_C(0x80000001);actor.kind=Ft_Kind_Mario;
    actor.parts=parts;object.hsd_obj=&joints[0];actor.x8AC_animSkeleton=&joints[3];
    for(unsigned i=0;i<3;++i){parts[i].joint=&joints[i];parts[i].x4_jobj2=&joints[i+3];parts[i].flags_b1=true;joints[i].flags=(sample+i)%3==0;joints[i+3].flags=(sample+i)%4==0;}
    parts[1].flags_b0=(sample&1)!=0;parts[2].flags_b5=(sample&2)!=0;
    joints[0].scale.x=25.25f;joints[3].scale.x=47.5f;
    actor.x8A4_animBlendFrames=sample&1?0.5f:0;
    actor.co_attrs=(ftCo_DatAttrs){.gravity=0.0625f,.terminal_velocity=2.25f,.fast_fall_velocity=3.75f,
        .ground_friction=0.125f,.aerial_friction=0.09375f,.walk_max_vel=1.25f,
        .air_drift_max=1.375f,.air_drift_stick_mul=0.25f,.aerial_drift_base=0.03125f,.air_max_horizontal_velocity=2.5f};
    actor.self_vel=(Vec3){((int)(sample%37)-18)*0.15625f,((int)(sample%19)-9)*0.1875f,0.875f};
    actor.gr_vel=((int)(sample%29)-14)*0.1875f;actor.xE4_ground_accel_1=0.21875f;
    actor.xE8_ground_accel_2=0.3125f;actor.x74_self_accel=(Vec3){1,2,3};
    actor.x8c_kb_vel=(Vec3){4,5,6};actor.x98_atk_shield_kb=(Vec3){7,8,9};
    actor.xD4_unk_vel=(Vec3){10,11,12};actor.xF0_ground_kb_vel=13;
    actor.xF4_ground_attacker_shield_kb_vel=14;
    actor.input.lstick[0]=(Vec2){((int)(sample%17)-8)*0.125f,((int)(sample%9)-4)*0.25f};
    actor.x671_timer_lstick_tilt_y=(uint8_t)(sample%7);actor.fall_fast=(sample%11)==0;
    actor.facing_dir=sample&1?-1:1;actor.lstick_angle=((int)(sample%7)-3)*0.125f;
    actor.x6A4_transNOffset=(Vec3){0.875f,0.5625f,-0.375f};actor.x594_b0=(sample&2)!=0;
    actor.coll_data.floor.normal=(Vec3){0.5f,0.8660254f,0};
    actor.cur_anim_frame=17.5f;actor.frame_speed_mul=1.25f;actor.x5F4_arr[0].idx=23;actor.motion_id=347;
    actor.x2223_b0=(sample%5)==0;
    reference=actor;reference_object=object;reference_object.user_data=&reference;reference.gobj=&reference_object;
    CHECK(nativeScriptBindings_Context(&object,NativeCallbackSource_Fighter,(uint32_t)actor.x8_spawnNum,&context,&diagnostic));
}

typedef struct Observed {
    Vec3 velocity,acceleration,animation_delta,kb,shield_kb,other_velocity;
    float ground_kb,ground_shield_kb,ground_acceleration2;
    float ground_velocity,ground_acceleration,angle,frame,rate,saved_rate;
    uint32_t flags;
} Observed;
static Observed observed(const Fighter* fp)
{
    return (Observed){fp->self_vel,fp->x74_self_accel,fp->x6A4_transNOffset,
        fp->x8c_kb_vel,fp->x98_atk_shield_kb,fp->xD4_unk_vel,fp->xF0_ground_kb_vel,fp->xF4_ground_attacker_shield_kb_vel,fp->xE8_ground_accel_2,
        fp->gr_vel,fp->xE4_ground_accel_1,fp->lstick_angle,fp->cur_anim_frame,fp->frame_speed_mul,fp->x8A0_unk,
        fp->fall_fast|((uint32_t)fp->x671_timer_lstick_tilt_y<<8)};
}

static bool original(uint32_t id,float a,float b,float c)
{
    switch(id) {
    case NativeScriptBinding_PhysicsClearKinetics:ftCommon_8007E2FC(&reference_object);break;
    case NativeScriptBinding_PhysicsGroundFriction:ft_80084F3C(&reference_object);break;
    case NativeScriptBinding_PhysicsAirDrift:ft_80084DB0(&reference_object);break;
    case NativeScriptBinding_PhysicsGroundMotion:ft_80084FA8(&reference_object);break;
    case NativeScriptBinding_PhysicsAirMotion:ft_80085154(&reference_object);break;
    case NativeScriptBinding_PhysicsAirGravityAndFriction:ft_80084EEC(&reference_object);break;
    case NativeScriptBinding_PhysicsClampAirDrift:ftCommon_ClampAirDrift(&reference);break;
    case NativeScriptBinding_PhysicsQuickAirDecel:return ftCommon_CalcSelfAccel_DeaccelQuickAir(&reference);
    case NativeScriptBinding_PhysicsFallBasic:ftCommon_FallBasic(&reference);break;
    case NativeScriptBinding_PhysicsGroundToSelf:ftCommon_SetSelfMovementFromGroundedMovement(&reference_object);break;
    case NativeScriptBinding_PhysicsAscend:ftCommon_Ascend(&reference,a,b);break;
    case NativeScriptBinding_PhysicsGroundAccel:ftCommon_CalcGroundAccel_AccelToLStickX(&reference,0,a,b);break;
    case NativeScriptBinding_PhysicsAirAccel:ftCommon_CalcSelfAccel_DriftSimple_NoFriction(&reference,0,a,b);break;
    case NativeScriptBinding_PhysicsClampHorizontalSpeed:ftCommon_ClampSelfVelX(&reference,a);break;
    case NativeScriptBinding_PhysicsClampGroundSpeed:ftCommon_ClampGroundVel(&reference,a);break;
    case NativeScriptBinding_PhysicsClampFallSpeed:ftCommon_ClampFallSpeed(&reference,a);break;
    case NativeScriptBinding_PhysicsAirDecel:ftCommon_CalcSelfAccel_Deaccel(&reference,a);break;
    case NativeScriptBinding_PhysicsFall:ftCommon_Fall(&reference,a,reference.co_attrs.terminal_velocity);break;
    case NativeScriptBinding_PhysicsFallWithTerminal:ftCommon_Fall(&reference,a,b);break;
    case NativeScriptBinding_PhysicsAirDecelNormal:ftCommon_CalcSelfAccel_DeaccelAir(&reference);break;
    case NativeScriptBinding_PhysicsGroundDecelerate:ftCommon_CalcGroundAccel_Deaccel(&reference,a);break;
    case NativeScriptBinding_PhysicsAirAccelCommon:ftCommon_CalcSelfAccel_DriftSimple_NoFriction(&reference,0.1875f,a,b);break;
    case NativeScriptBinding_PhysicsAirAccelFriction:ftCommon_CalcSelfAccel_DriftSimple(&reference,a,b,c);break;
    case NativeScriptBinding_PhysicsAirAccelNormal:ftCommon_CalcSelfAccel_Drift(&reference);break;
    case NativeScriptBinding_PhysicsAirAccelFrom:ftCommon_CalcSelfAccel_DriftFrom(&reference,a);break;
    case NativeScriptBinding_PhysicsQuickDecel:return ftCommon_CalcSelfAccel_DeaccelQuick(&reference,a);
    case NativeScriptBinding_PhysicsAnimationVelocity:ft_800851C0(&reference_object);ft_80085134(&reference_object);break;
    case NativeScriptBinding_PhysicsAnimationVelocityPlain:ft_80085134(&reference_object);break;
    case NativeScriptBinding_PhysicsGroundAnimationVelocity:ft_80085088(&reference_object);break;
    case NativeScriptBinding_PhysicsFastfall:ftCommon_FallFast(&reference);break;
    case NativeScriptBinding_PhysicsCheckFastfall:return ftCommon_CheckFallFast(&reference);
    default:CHECK(false);
    }
    return false;
}

static void physics_pairs(void)
{
    const uint32_t ids[]={NativeScriptBinding_PhysicsClearKinetics,NativeScriptBinding_PhysicsGroundFriction,NativeScriptBinding_PhysicsAirDrift,
        NativeScriptBinding_PhysicsGroundMotion,NativeScriptBinding_PhysicsAirMotion,NativeScriptBinding_PhysicsAirGravityAndFriction,
        NativeScriptBinding_PhysicsClampAirDrift,NativeScriptBinding_PhysicsQuickAirDecel,NativeScriptBinding_PhysicsFallBasic,
        NativeScriptBinding_PhysicsGroundToSelf,NativeScriptBinding_PhysicsAscend,NativeScriptBinding_PhysicsGroundAccel,
        NativeScriptBinding_PhysicsAirAccel,NativeScriptBinding_PhysicsClampHorizontalSpeed,NativeScriptBinding_PhysicsClampGroundSpeed,
        NativeScriptBinding_PhysicsClampFallSpeed,NativeScriptBinding_PhysicsAirDecel,NativeScriptBinding_PhysicsFall,
        NativeScriptBinding_PhysicsFallWithTerminal,NativeScriptBinding_PhysicsAirDecelNormal,NativeScriptBinding_PhysicsGroundDecelerate,
        NativeScriptBinding_PhysicsAirAccelCommon,NativeScriptBinding_PhysicsAirAccelFriction,NativeScriptBinding_PhysicsAirAccelNormal,
        NativeScriptBinding_PhysicsAirAccelFrom,NativeScriptBinding_PhysicsQuickDecel,NativeScriptBinding_PhysicsAnimationVelocity,
        NativeScriptBinding_PhysicsAnimationVelocityPlain,NativeScriptBinding_PhysicsGroundAnimationVelocity,
        NativeScriptBinding_PhysicsFastfall,NativeScriptBinding_PhysicsCheckFastfall};
    for(unsigned sample=0;sample<128;++sample)for(unsigned op=0;op<sizeof(ids)/sizeof(*ids);++op) {
        setup(sample);uint32_t id=ids[op];const NativeScriptBindingDescriptor* descriptor=nativeScriptBindings_Descriptor(id);
        double a=0.1000000014901161+(sample%7)*0.00300000000000001,b=0.93750000000001,c=1.31250000000001;
        NativeScriptValue values[3]={{.type=NativeScriptType_F64,.as.f64=a},{.type=NativeScriptType_F64,.as.f64=b},{.type=NativeScriptType_F64,.as.f64=c}},result;
        bool expected=original(id,(float)a,(float)b,(float)c);
        CHECK(nativeScriptBindings_Invoke(&context,id,NULL,0,values,descriptor->argument_count,&result,&diagnostic));
        CHECK(result.type==descriptor->result_type);
        if(result.type==NativeScriptType_Bool)CHECK(result.as.u64==(uint64_t)expected);
        Observed actual=observed(&actor),wanted=observed(&reference);CHECK(!memcmp(&actual,&wanted,sizeof(actual)));++pairs;
    }
}

static void motion_pairs(void)
{
    const uint32_t ids[]={NativeScriptBinding_MotionSetVerticalSpeed,NativeScriptBinding_MotionAddVerticalSpeed,
        NativeScriptBinding_MotionScaleHorizontalSpeed,NativeScriptBinding_MotionDivideHorizontalSpeed,
        NativeScriptBinding_MotionScaleVelocity,NativeScriptBinding_MotionScalePlanarVelocity};
    for(unsigned sample=0;sample<512;++sample)for(unsigned op=0;op<6;++op) {
        setup(sample);uint32_t id=ids[op];double number=0.1000000014901161+(sample%7)*0.00300000000000001;
        NativeScriptValue argument={.type=NativeScriptType_F64,.as.f64=number},result;
        switch(id) {
        case NativeScriptBinding_MotionSetVerticalSpeed:reference.self_vel.y=(float)number;break;
        case NativeScriptBinding_MotionAddVerticalSpeed: {
            float alternative=reference.self_vel.y+(float)number;reference.self_vel.y+=number;if(alternative!=reference.self_vel.y)++double_differences;break;
        }
        case NativeScriptBinding_MotionScaleHorizontalSpeed: {
            float alternative=reference.self_vel.x*(float)number;reference.self_vel.x*=number;if(alternative!=reference.self_vel.x)++double_differences;break;
        }
        case NativeScriptBinding_MotionDivideHorizontalSpeed:reference.self_vel.x/=number;break;
        case NativeScriptBinding_MotionScaleVelocity: {
            float scale=(float)number;reference.self_vel.x*=scale;reference.self_vel.y*=scale;reference.self_vel.z*=scale;break;
        }
        case NativeScriptBinding_MotionScalePlanarVelocity: {
            float scale=(float)number;reference.self_vel.x*=scale;reference.self_vel.y*=scale;break;
        }
        default:CHECK(false);
        }
        CHECK(nativeScriptBindings_Invoke(&context,id,NULL,0,&argument,1,&result,&diagnostic));
        Observed actual=observed(&actor),wanted=observed(&reference);CHECK(!memcmp(&actual,&wanted,sizeof(actual)));++pairs;
    }
    CHECK(double_differences>0);
    setup(1);actor.self_vel.x=1;NativeScriptValue zero={.type=NativeScriptType_F64,.as.f64=0},result;
    CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_MotionDivideHorizontalSpeed,NULL,0,&zero,1,&result,&diagnostic));CHECK(isinf(actor.self_vel.x));
}

static NativeScriptIRNode expression_nodes[512];static uint32_t expression_arguments[256];
static void semantic(const char* text,unsigned family,bool expected,unsigned type,uint32_t id)
{
    NativeScriptIRArena arena={text,(uint32_t)strlen(text),expression_nodes,0,512,expression_arguments,0,256};
    NativeScriptBindingResolverContext binding={family};uint32_t root;
    CHECK(nativeScriptExpression_Parse(&arena,(NativeScriptSourceSpan){0,arena.source_bytes,5,1},nativeScriptBindings_Resolve,&binding,&root,&diagnostic)==expected);
    if(expected){CHECK(expression_nodes[root].type==type);if(id)CHECK(expression_nodes[root].semantic==id);}
}

static void animation_cases(void)
{
    NativeScriptValue result;
    for(unsigned sample=0;sample<16;++sample) {
        setup(sample);CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_AnimationFinished,NULL,0,&result,&diagnostic));CHECK(result.as.u64==(uint64_t)!ftAnim_IsFramesRemaining(&reference_object));
        CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_AnimationLength,NULL,0,&result,&diagnostic));CHECK(result.as.f32==ftAnim_8006F484(&reference_object));
        NativeScriptValue rate={.type=NativeScriptType_F64,.as.f64=1.10000000000001};ftAnim_SetAnimRate(&reference_object,(float)rate.as.f64);
        CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_AnimationSetRate,NULL,0,&rate,1,&result,&diagnostic));
        CHECK(actor.frame_speed_mul==reference.frame_speed_mul&&actor.x8A0_unk==reference.x8A0_unk);
        // Plain primitive fixture has no scene/actor. Compare the native leaf
        // sequence here; script_ledge_entry proves the guarded shared helper.
        ftAnim_8006E9B4(&reference_object);
        ftAction_80073240(&reference_object);
        ftAnim_800707B0(&reference_object);
        ftCo_800DB500(&reference_object);
        CHECK(script_kinetic_stage_count==4);
        scriptKineticsFixture_RetireAt(0,0);
        CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_AnimationStart,NULL,0,NULL,0,&result,&diagnostic));
        CHECK(script_kinetic_stage_count==4&&actor.cur_anim_frame==reference.cur_anim_frame);
        for(unsigned stage=0;stage<4;++stage)CHECK(script_kinetic_stages[stage]==stage+1);
    }
    for(unsigned stage=1;stage<=4;++stage)for(unsigned mode=0;mode<2;++mode) {
        setup(0);scriptKineticsFixture_RetireAt(stage,mode);
        result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
        CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_AnimationStart,NULL,0,NULL,0,&result,&diagnostic));
        CHECK(script_kinetic_stage_count==stage&&result.type==NativeScriptType_U64&&result.as.u64==UINT64_MAX);
    }
    semantic("animation.finished",NativeScriptBindings_Fighter,true,NativeScriptType_Bool,NativeScriptBinding_AnimationFinished);
    semantic("animation.rate + frame",NativeScriptBindings_Fighter,true,NativeScriptType_F32,0);
    semantic("physics.fall(0.1)",NativeScriptBindings_Fighter,true,NativeScriptType_Void,NativeScriptBinding_PhysicsFall);
    semantic("physics.fall(0.1,2.25)",NativeScriptBindings_Fighter,true,NativeScriptType_Void,NativeScriptBinding_PhysicsFallWithTerminal);
    semantic("physics.quick_decel(air.drift_max)",NativeScriptBindings_Fighter,true,NativeScriptType_Bool,NativeScriptBinding_PhysicsQuickDecel);
    semantic("motion.add_vertical_speed(0.1000000000000001)",NativeScriptBindings_Fighter,true,NativeScriptType_Void,NativeScriptBinding_MotionAddVerticalSpeed);
    const char* unsupported[]={"physics.switch_to_air()","state.change(1,0,0,1)","motion.add_facing_speed(1)","physics.fall()","physics.fall(1,2,3)","animation.frame","animation.remaining"};
    for(unsigned i=0;i<sizeof(unsupported)/sizeof(*unsupported);++i)semantic(unsupported[i],NativeScriptBindings_Fighter,false,0,0);
    semantic("physics.fall(1)",NativeScriptBindings_Article,false,0,0);semantic("animation.finished",NativeScriptBindings_Effect,false,0,0);
}

static void kinetic_properties(void)
{
    setup(7);NativeScriptValue value;
    const struct {uint32_t id;float expected;} scalar[]={
        {NativeScriptBinding_AnimationRate,actor.frame_speed_mul},
        {NativeScriptBinding_AnimationDeltaX,actor.x6A4_transNOffset.x},
        {NativeScriptBinding_AnimationDeltaY,actor.x6A4_transNOffset.y},
        {NativeScriptBinding_AnimationDeltaZ,actor.x6A4_transNOffset.z},
        {NativeScriptBinding_MotionLaunchAngle,actor.lstick_angle},
        {NativeScriptBinding_GroundVelocity,actor.gr_vel},
        {NativeScriptBinding_GroundAcceleration,actor.xE4_ground_accel_1},
        {NativeScriptBinding_GroundFriction,actor.co_attrs.ground_friction},
        {NativeScriptBinding_AirDriftMax,actor.co_attrs.air_drift_max},
        {NativeScriptBinding_AirDriftStickMultiplier,actor.co_attrs.air_drift_stick_mul}};
    for(unsigned i=0;i<sizeof(scalar)/sizeof(*scalar);++i) {
        CHECK(nativeScriptBindings_Read(&context,scalar[i].id,NULL,0,&value,&diagnostic));CHECK(value.type==NativeScriptType_F32&&value.as.f32==scalar[i].expected);
        const NativeScriptBindingDescriptor* descriptor=nativeScriptBindings_Descriptor(scalar[i].id);
        NativeScriptValue input={.type=NativeScriptType_F64,.as.f64=1.100000000000001};
        bool writable=(descriptor->flags&NativeScriptFlag_Writable)!=0;
        CHECK(nativeScriptBindings_Write(&context,scalar[i].id,NULL,0,&input,&diagnostic)==writable);
        if(writable){CHECK(nativeScriptBindings_Read(&context,scalar[i].id,NULL,0,&value,&diagnostic));CHECK(value.as.f32==(float)input.as.f64);}
    }
    CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_MotionID,NULL,0,&value,&diagnostic)&&value.type==NativeScriptType_I32&&value.as.i64==347);
    CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_AnimationID,NULL,0,&value,&diagnostic)&&value.type==NativeScriptType_I32&&value.as.i64==23);
    NativeScriptValue timer={.type=NativeScriptType_F64,.as.f64=2.100000000000001};
    CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_AnimationSetTimer,NULL,0,&timer,1,&value,&diagnostic));CHECK(actor.cur_anim_frame==(float)timer.as.f64);
    actor.x6D8=(Vec3){1,2,3};actor.x6C0=(Vec3){4,5,6};actor.x68C_transNPos=(Vec3){7,8,9};
    CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_AnimationClearMotionSamples,NULL,0,NULL,0,&value,&diagnostic));
    Vec3 zero={0};CHECK(!memcmp(&actor.x6D8,&zero,sizeof(zero))&&!memcmp(&actor.x6C0,&zero,sizeof(zero))&&!memcmp(&actor.x68C_transNPos,&zero,sizeof(zero))&&!memcmp(&actor.x6A4_transNOffset,&zero,sizeof(zero))&&actor.lstick_angle==0);
}

static void vm_case(void)
{
    const char* text="func CapeAndTornadoPhysics():\n"
        "    physics.clear_kinetics()\n"
        "    motion.divide_horizontal_speed(1.7)\n"
        "    if grounded:\n"
        "        physics.ground_friction()\n"
        "    else:\n"
        "        physics.fall(0.0625,2.25)\n"
        "        physics.air_decel(0.09375)\n"
        "    motion.add_vertical_speed(0.1000000000000001)\n"
        "    motion.scale_velocity(1.10000000000001)\n";
    NativeScriptSourceNode syntax[128];NativeScriptSourceTree tree={text,(uint32_t)strlen(text),syntax,0,128};CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
    NativeScriptIRArena arena={text,(uint32_t)strlen(text),expression_nodes,0,512,expression_arguments,0,256};
    NativeScriptLocal locals[32];NativeScriptBindingResolverContext binding={NativeScriptBindings_Fighter};
    NativeScriptStatementCompiler body={.tree=&tree,.expressions=&arena,.bindings=nativeScriptBindings_Resolve,.binding_context=&binding,.locals=locals,.local_capacity=32,.return_type=NativeScriptType_Void};
    uint32_t function=1;CHECK(tree.nodes[function].kind==NativeScriptSource_Function);CHECK(nativeScriptStatement_Compile(&body,function,&diagnostic));
    NativeScriptInstruction code[1024];NativeScriptFunctionCode fn[1];NativeScriptBytecodeBuilder builder={.instructions=code,.instruction_capacity=1024,.functions=fn,.function_count=1,.describe_call=nativeScriptBindings_DescribeCall,.binding_context=&binding};
    CHECK(nativeScriptBytecode_Compile(&builder,&body,function,0,0,&diagnostic));
    NativeScriptBytecodeProgram program={text,tree.source_bytes,code,builder.instruction_count,fn,1,false}; { NativeScriptDiagnostic preparation={0}; if(!nativeScriptBytecode_Prepare(&program,&preparation)){fprintf(stderr,"bytecode preparation: %s\n",preparation.message);abort();} }
    NativeScriptValue registers[512];NativeScriptVMFrame frames[8];NativeScriptVM vm={.registers=registers,.register_capacity=512,.frames=frames,.frame_capacity=8};
    for(unsigned sample=0;sample<64;++sample) {
        setup(sample);actor.ground_or_air=reference.ground_or_air=sample&1?GA_Air:GA_Ground;
        ftCommon_8007E2FC(&reference_object);
        reference.self_vel.x/=1.7;
        if(reference.ground_or_air==GA_Ground)ft_80084F3C(&reference_object);
        else {ftCommon_Fall(&reference,0.0625f,2.25f);ftCommon_CalcSelfAccel_Deaccel(&reference,0.09375f);}
        reference.self_vel.y+=0.1000000000000001;float scale=(float)1.10000000000001;
        reference.self_vel.x*=scale;reference.self_vel.y*=scale;reference.self_vel.z*=scale;
        NativeScriptValue result;CHECK(nativeScriptVM_Run(&vm,&program,&nativeScriptBindingHost,&context,0,NULL,0,&result)==NativeScriptVM_Ok);
        Observed actual=observed(&actor),wanted=observed(&reference);CHECK(!memcmp(&actual,&wanted,sizeof(actual)));CHECK(vm.depth==0&&vm.register_top==0);++pairs;
    }
}

static void vm_retirement(void)
{
    const char* text="func StartAndContinue() -> float:\n"
        "    animation.start()\n"
        "    position.x = 123.0\n"
        "    return position.x\n";
    NativeScriptSourceNode syntax[32];NativeScriptSourceTree tree={text,(uint32_t)strlen(text),syntax,0,32};CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
    NativeScriptIRArena arena={text,(uint32_t)strlen(text),expression_nodes,0,512,expression_arguments,0,256};NativeScriptLocal locals[16];
    NativeScriptBindingResolverContext binding={NativeScriptBindings_Fighter};
    NativeScriptStatementCompiler body={.tree=&tree,.expressions=&arena,.bindings=nativeScriptBindings_Resolve,.binding_context=&binding,.locals=locals,.local_capacity=16,.return_type=NativeScriptType_F32};
    CHECK(nativeScriptStatement_Compile(&body,1,&diagnostic));
    NativeScriptInstruction code[128];NativeScriptFunctionCode fn[1];NativeScriptBytecodeBuilder builder={.instructions=code,.instruction_capacity=128,.functions=fn,.function_count=1,.describe_call=nativeScriptBindings_DescribeCall,.binding_context=&binding};
    CHECK(nativeScriptBytecode_Compile(&builder,&body,1,0,0,&diagnostic));
    NativeScriptBytecodeProgram program={text,tree.source_bytes,code,builder.instruction_count,fn,1,false}; { NativeScriptDiagnostic preparation={0}; if(!nativeScriptBytecode_Prepare(&program,&preparation)){fprintf(stderr,"bytecode preparation: %s\n",preparation.message);abort();} }NativeScriptValue registers[128];NativeScriptVMFrame frames[8];
    NativeScriptVM vm={.registers=registers,.register_capacity=128,.frames=frames,.frame_capacity=8};
    for(unsigned stage=1;stage<=4;++stage)for(unsigned mode=0;mode<2;++mode) {
        setup(0);actor.cur_pos.x=17;scriptKineticsFixture_RetireAt(stage,mode);
        NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
        CHECK(nativeScriptVM_Run(&vm,&program,&nativeScriptBindingHost,&context,0,NULL,0,&result)==NativeScriptVM_SourceRetired);
        CHECK(actor.cur_pos.x==17&&result.type==NativeScriptType_U64&&result.as.u64==UINT64_MAX);
        CHECK(script_kinetic_stage_count==stage&&vm.depth==0&&vm.register_top==0);
    }
}

static void owned_clip_lengths(void)
{
    setup(0);
    scriptKineticsFixture_OwnSource(&actor,NULL,NULL);
    CHECK(nativeScriptBindings_Context(&object,NativeCallbackSource_Fighter,
        actor.x8_spawnNum,&context,&diagnostic));
    NativeSourcePackage* source=scriptKineticsFixture_Source();
    FigaTree gameplay_clip={.frames=48},demo_clip={.frames=19};
    struct Fighter_WaitAnimData gameplay[3]={{.x14=&gameplay_clip},{0},{.x14=&gameplay_clip}};
    struct Fighter_WaitAnimData demo[2]={{.x14=&demo_clip},{0}};
    source->assets.character.data.xC=gameplay;
    source->assets.character.data.x14=demo;
    source->assets.facts.actions=3;source->assets.facts.demo_count=2;
    // Load callbacks query gameplay IDs even when constructing a results actor.
    // The stock lookup returns zero for IDs absent from that active demo table.
    const unsigned indices[]={0,1,2,3,18,19,72,296,297,298,65535};
    for(unsigned bank=0;bank<2;++bank) {
        actor.x24=bank?demo:gameplay;actor.x58C=bank?2:3;
        for(unsigned i=0;i<sizeof(indices)/sizeof(*indices);++i) {
            NativeScriptValue argument={.type=NativeScriptType_I32,.as.i64=indices[i]},result;
            CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_AnimationClipLength,
                NULL,0,&argument,1,&result,&diagnostic));
            float expected=indices[i]<actor.x58C&&actor.x24[indices[i]].x14?
                ((FigaTree*)actor.x24[indices[i]].x14)->frames:0;
            CHECK(result.type==NativeScriptType_F32&&result.as.f32==expected);
        }
    }
    NativeScriptValue argument={.type=NativeScriptType_I32,.as.i64=0};
    NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=UINT64_MAX},result=sentinel;
    actor.x58C=3; // Forged count must not permit reading past the two demo entries.
    CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_AnimationClipLength,
        NULL,0,&argument,1,&result,&diagnostic));
    CHECK(result.type==sentinel.type&&result.as.u64==sentinel.as.u64);
    actor.x58C=2;actor.x24=gameplay; // Wrong owned bank/count pairing.
    CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_AnimationClipLength,
        NULL,0,&argument,1,&result,&diagnostic));
    actor.x24=demo;
    for(unsigned i=0;i<2;++i) {
        argument.as.i64=i?65536:-1;
        CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_AnimationClipLength,
            NULL,0,&argument,1,&result,&diagnostic));
        CHECK(result.type==sentinel.type&&result.as.u64==sentinel.as.u64);
    }
    actor.x24=NULL;argument.as.i64=0;
    CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_AnimationClipLength,
        NULL,0,&argument,1,&result,&diagnostic));
}

int main(void)
{
    physics_pairs();motion_pairs();animation_cases();kinetic_properties();vm_case();vm_retirement();owned_clip_lengths();
    printf("{\"checks\":%u,\"native_body_pairs\":%u,\"double_sensitive_cases\":%u,\"status\":\"PASS fighter kinetic binding/native body comparisons\"}\n",checks,pairs,double_differences);
    return 0;
}
