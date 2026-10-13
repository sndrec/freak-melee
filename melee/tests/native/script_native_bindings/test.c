#include <melee/ft/native_script_bindings.h>
#include <melee/ft/native_script_binding_spawn.h>
#include <melee/ft/native_script_expression.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/types.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/inlines.h>
#include <melee/ft/kinds/ftPeach/types.h>
#include <melee/it/types.h>
#include <melee/ef/forward.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/controller.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/class.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HSD_GObj** HSD_GObjPLinkHead;
HSD_JObjInfo hsdJObj;
static HSD_GObj* heads[32];
static HSD_GObj fighter_object,article_object;
static Fighter fighter;
static Item article;
static NativeScriptRuntimeContext fighter_context,article_context;
static NativeScriptDiagnostic diagnostic;
static unsigned checks,properties,parses;
#include "common_scale_reference.inc"
#define CHECK(value) do { ++checks; if (!(value)) { fprintf(stderr,"Native binding check %d: %s (%s)\n",__LINE__,#value,diagnostic.message); return false; } } while (0)

bool nativeModelEffect_Snapshot(HSD_GObj* object,uint32_t* generation)
{ (void)object;(void)generation;return false; }
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* object,uint32_t generation)
{ (void)object;(void)generation;return NULL; }

struct Fighter_ShakeTable_t* Fighter_SmashChargeShakeTable;

static bool setup(void)
{
    memset(&fighter,0,sizeof(fighter));memset(&article,0,sizeof(article));
    memset(&fighter_object,0,sizeof(fighter_object));memset(&article_object,0,sizeof(article_object));
    memset(heads,0,sizeof(heads));memset(&diagnostic,0,sizeof(diagnostic));
    HSD_GObjPLinkHead=heads;heads[HSD_GOBJ_PLINK_FIGHTER]=&fighter_object;heads[HSD_GOBJ_PLINK_ITEM]=&article_object;
    fighter_object.user_data=&fighter;article_object.user_data=&article;
    fighter_object.classifier=HSD_GOBJ_CLASS_FIGHTER;
    fighter.gobj=&fighter_object;fighter.x8_spawnNum=(int32_t)UINT32_C(0x80000001);
    article.entity=&article_object;article.x1C=(int32_t)UINT32_C(16777217);
    CHECK(nativeScriptBindings_Context(&fighter_object,NativeCallbackSource_Fighter,(uint32_t)fighter.x8_spawnNum,&fighter_context,&diagnostic));
    CHECK(nativeScriptBindings_Context(&article_object,NativeCallbackSource_Item,(uint32_t)article.x1C,&article_context,&diagnostic));
    CHECK(fighter_context.generation==UINT32_C(0x80000001)&&article_context.generation==UINT32_C(16777217));
    return true;
}

static bool read_float(NativeScriptRuntimeContext* context,uint32_t id,float expected)
{
    NativeScriptValue value={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
    CHECK(nativeScriptBindings_Read(context,id,NULL,0,&value,&diagnostic));
    CHECK(value.type==NativeScriptType_F32&&!memcmp(&value.as.f32,&expected,sizeof(expected)));++properties;return true;
}

static bool read_bool(uint32_t id,bool expected)
{
    NativeScriptValue value={0};CHECK(nativeScriptBindings_Read(&fighter_context,id,NULL,0,&value,&diagnostic));
    CHECK(value.type==NativeScriptType_Bool&&value.as.u64==(uint64_t)expected);++properties;return true;
}

static bool property_suite(void)
{
    CHECK(setup());
    const uint32_t field_ids[]={NativeScriptBinding_PositionX,NativeScriptBinding_PositionY,NativeScriptBinding_PositionZ,
        NativeScriptBinding_PositionDeltaX,NativeScriptBinding_PositionDeltaY,NativeScriptBinding_PositionDeltaZ,
        NativeScriptBinding_VelocityX,NativeScriptBinding_VelocityY,NativeScriptBinding_VelocityZ,
        NativeScriptBinding_AccelerationX,NativeScriptBinding_AccelerationY,NativeScriptBinding_AccelerationZ};
    for(unsigned sample=0;sample<257;++sample) {
        float base=((int)sample-128)*0.0625f;
        fighter.cur_pos=(Vec3){base+1,base+2,base+3};fighter.pos_delta=(Vec3){base+4,base+5,base+6};
        fighter.self_vel=(Vec3){base+7,base+8,base+9};fighter.x74_self_accel=(Vec3){base+10,base+11,base+12};
        for(unsigned i=0;i<12;++i)CHECK(read_float(&fighter_context,field_ids[i],base+(float)i+1));
        article.pos=(Vec3){base+21,base+22,base+23};article.x40_vel=(Vec3){base+24,base+25,base+26};
        CHECK(read_float(&article_context,NativeScriptBinding_PositionX,base+21));
        CHECK(read_float(&article_context,NativeScriptBinding_PositionY,base+22));
        CHECK(read_float(&article_context,NativeScriptBinding_PositionZ,base+23));
        CHECK(read_float(&article_context,NativeScriptBinding_VelocityX,base+24));
        CHECK(read_float(&article_context,NativeScriptBinding_VelocityY,base+25));
        CHECK(read_float(&article_context,NativeScriptBinding_VelocityZ,base+26));
        NativeScriptValue value={.type=NativeScriptType_F64,.as.f64=base+0.1};
        for(unsigned i=0;i<12;++i) {
            CHECK(nativeScriptBindings_Write(&fighter_context,field_ids[i],NULL,0,&value,&diagnostic));
            CHECK(read_float(&fighter_context,field_ids[i],(float)value.as.f64));
        }
        CHECK(fighter.cur_pos.x==(float)value.as.f64&&fighter.pos_delta.y==(float)value.as.f64&&fighter.self_vel.z==(float)value.as.f64&&fighter.x74_self_accel.x==(float)value.as.f64);
        value=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=-7};
        CHECK(nativeScriptBindings_Write(&article_context,NativeScriptBinding_PositionZ,NULL,0,&value,&diagnostic));CHECK(article.pos.z==-7.0f&&article.pos.x==base+21);
    }
    fighter.input.lstick[0]=(Vec2){0.375f,-0.625f};fighter.cur_anim_frame=-2.875f;fighter.facing_dir=-1;article.facing_dir=1;
    CHECK(read_float(&fighter_context,NativeScriptBinding_InputX,0.375f));CHECK(read_float(&fighter_context,NativeScriptBinding_InputY,-0.625f));
    CHECK(read_float(&fighter_context,NativeScriptBinding_Frame,-2.875f));CHECK(read_float(&fighter_context,NativeScriptBinding_FacingDirection,-1));CHECK(read_float(&article_context,NativeScriptBinding_ArticleFacing,1));
    NativeScriptValue value={0};CHECK(nativeScriptBindings_Read(&fighter_context,NativeScriptBinding_FrameInteger,NULL,0,&value,&diagnostic));CHECK(value.type==NativeScriptType_I32&&value.as.i64==-2);
    const uint32_t booleans[]={NativeScriptBinding_InputSpecialHeld,NativeScriptBinding_InputSpecialPressed,NativeScriptBinding_Grounded,
        NativeScriptBinding_Airborne,NativeScriptBinding_EffectsActive,NativeScriptBinding_UsedTether,NativeScriptBinding_Fastfall};
    for(unsigned on=0;on<2;++on) {
        fighter.input.held_buttons[0]=on?HSD_PAD_B:HSD_PAD_A;fighter.input.pressed_buttons=on?HSD_PAD_B:HSD_PAD_A;
        fighter.ground_or_air=on?GA_Air:GA_Ground;fighter.x2219_b0=on;fighter.used_tether=on;fighter.fall_fast=on;
        for(unsigned i=0;i<7;++i)CHECK(read_bool(booleans[i],i==2?!on:on));
        value=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=on};
        fighter.x2219_b1=1;fighter.x2228_b7=1;
        CHECK(nativeScriptBindings_Write(&fighter_context,NativeScriptBinding_Airborne,NULL,0,&value,&diagnostic));
        CHECK(nativeScriptBindings_Write(&fighter_context,NativeScriptBinding_EffectsActive,NULL,0,&value,&diagnostic));
        CHECK(nativeScriptBindings_Write(&fighter_context,NativeScriptBinding_UsedTether,NULL,0,&value,&diagnostic));
        CHECK(nativeScriptBindings_Write(&fighter_context,NativeScriptBinding_Fastfall,NULL,0,&value,&diagnostic));
        CHECK(fighter.x2219_b1&&fighter.x2228_b7&&fighter.ground_or_air==(on?GA_Air:GA_Ground));
    }
    value=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=1};
    CHECK(!nativeScriptBindings_Write(&fighter_context,NativeScriptBinding_EffectsActive,NULL,0,&value,&diagnostic));
    CHECK(!nativeScriptBindings_Write(&fighter_context,NativeScriptBinding_Grounded,NULL,0,&value,&diagnostic));
    CHECK(!nativeScriptBindings_Write(&fighter_context,NativeScriptBinding_InputX,NULL,0,&value,&diagnostic));
    value=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0x123456789)};NativeScriptValue before=value;
    CHECK(!nativeScriptBindings_Read(&article_context,NativeScriptBinding_PositionDeltaX,NULL,0,&value,&diagnostic)&&!memcmp(&value,&before,sizeof(value)));
    CHECK(!nativeScriptBindings_Read(&fighter_context,NativeScriptBinding_ArticleFacing,NULL,0,&value,&diagnostic));
    CHECK(!nativeScriptBindings_Read(&fighter_context,NativeScriptBinding_PositionX,&before,1,&value,&diagnostic));
    CHECK(!nativeScriptBindings_Read(&fighter_context,UINT32_C(0x10000000),NULL,0,&value,&diagnostic));
    ++fighter.x8_spawnNum;CHECK(!nativeScriptBindings_SourceAlive(&fighter_context));
    CHECK(!nativeScriptBindings_Read(&fighter_context,NativeScriptBinding_PositionX,NULL,0,&value,&diagnostic));
    float x=fighter.cur_pos.x;CHECK(!nativeScriptBindings_Write(&fighter_context,NativeScriptBinding_PositionX,NULL,0,&before,&diagnostic)&&fighter.cur_pos.x==x);
    // Removed source has poisoned data. Registry membership must reject it
    // before binding code can dereference the retained object address/data.
    heads[HSD_GOBJ_PLINK_ITEM]=NULL;article_object.user_data=(void*)1;
    CHECK(!nativeScriptBindings_Read(&article_context,NativeScriptBinding_PositionX,NULL,0,&value,&diagnostic));
    CHECK(!nativeScriptBindings_Context(&article_object,NativeCallbackSource_Item,UINT32_C(16777217),&article_context,&diagnostic));
    return true;
}

static bool coordinate_lifetime_rejections(void)
{
    // The field-only fast return still admits a live source before dereferencing
    // userdata. Poison an unlinked actor's pointer to catch accidental bypasses.
    const uint32_t ids[]={NativeScriptBinding_PositionX,NativeScriptBinding_PositionY,NativeScriptBinding_PositionZ,
        NativeScriptBinding_VelocityX,NativeScriptBinding_VelocityY,NativeScriptBinding_VelocityZ,
        NativeScriptBinding_PositionDeltaX,NativeScriptBinding_PositionDeltaY,NativeScriptBinding_PositionDeltaZ,
        NativeScriptBinding_AccelerationX,NativeScriptBinding_AccelerationY,NativeScriptBinding_AccelerationZ};
    for(unsigned actor=0;actor<2;++actor) {
        CHECK(setup());
        NativeScriptRuntimeContext* context=actor?&article_context:&fighter_context;
        HSD_GObj* object=actor?&article_object:&fighter_object;
        unsigned link=actor?HSD_GOBJ_PLINK_ITEM:HSD_GOBJ_PLINK_FIGHTER;
        void* data=object->user_data;
        for(unsigned field=0;field<(actor?6u:12u);++field) {
            NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=UINT64_MAX},value=sentinel;
            ++context->generation;
            CHECK(!nativeScriptBindings_Read(context,ids[field],NULL,0,&value,&diagnostic));
            CHECK(!memcmp(&value,&sentinel,sizeof(value)));
            --context->generation;
            heads[link]=NULL;object->user_data=(void*)1;
            CHECK(!nativeScriptBindings_Read(context,ids[field],NULL,0,&value,&diagnostic));
            CHECK(!memcmp(&value,&sentinel,sizeof(value)));
            object->user_data=data;heads[link]=object;
        }
    }
    return true;
}

static NativeScriptIRNode nodes[512];static uint32_t arguments[256];
static bool parse(const char* source,unsigned context,bool expected,NativeScriptIRType type,uint32_t semantic)
{
    NativeScriptIRArena arena={source,(uint32_t)strlen(source),nodes,0,512,arguments,0,256};
    NativeScriptBindingResolverContext binding={context};uint32_t root;memset(&diagnostic,0,sizeof(diagnostic));++parses;
    bool ok=nativeScriptExpression_Parse(&arena,(NativeScriptSourceSpan){0,arena.source_bytes,13,9},nativeScriptBindings_Resolve,&binding,&root,&diagnostic);
    CHECK(ok==expected);
    if(ok){CHECK(nodes[root].type==type);if(semantic)CHECK(nodes[root].semantic==semantic);}
    else CHECK(diagnostic.source.line==13&&diagnostic.message[0]);
    return true;
}

static bool semantic_suite(void)
{
    CHECK(parse("position.x + velocity.y",NativeScriptBindings_Fighter,true,NativeScriptType_F32,0));
    CHECK(parse("position.delta.z",NativeScriptBindings_Fighter,true,NativeScriptType_F32,NativeScriptBinding_PositionDeltaZ));
    CHECK(parse("position.x + velocity.y",NativeScriptBindings_Article,true,NativeScriptType_F32,0));
    CHECK(parse("facing",NativeScriptBindings_Article,true,NativeScriptType_F32,NativeScriptBinding_ArticleFacing));
    CHECK(parse("facing.direction",NativeScriptBindings_Fighter,true,NativeScriptType_F32,NativeScriptBinding_FacingDirection));
    CHECK(parse("input.special_held and not grounded",NativeScriptBindings_Fighter,true,NativeScriptType_Bool,0));
    CHECK(parse("frame.integer",NativeScriptBindings_Fighter,true,NativeScriptType_I32,NativeScriptBinding_FrameInteger));
    CHECK(parse("atan2(position.y,position.x)",NativeScriptBindings_Fighter,true,NativeScriptType_F64,NativeScriptBinding_Atan2));
    CHECK(parse("atan2(position.y,position.x)",NativeScriptBindings_Article,true,NativeScriptType_F32,NativeScriptBinding_ArticleAtan2));
    CHECK(parse("int(0x80000001)",NativeScriptBindings_Fighter,true,NativeScriptType_I32,NativeScriptBinding_Int));
    CHECK(parse("sqrt(abs(-4.0))",NativeScriptBindings_Article,true,NativeScriptType_F32,NativeScriptBinding_Sqrt));
    CHECK(parse("sin(0.25)",NativeScriptBindings_Effect,true,NativeScriptType_F32,NativeScriptBinding_Sin));
    CHECK(parse("atan2(1,2)",NativeScriptBindings_Effect,true,NativeScriptType_F64,NativeScriptBinding_Atan2));
    CHECK(parse("pi_half + pi",NativeScriptBindings_Article,true,NativeScriptType_F64,0));
    CHECK(parse("radians(position.x)",NativeScriptBindings_Fighter,true,NativeScriptType_F32,NativeScriptBinding_RadiansFloat));
    CHECK(parse("radians(90.0 + position.x)",NativeScriptBindings_Fighter,true,NativeScriptType_F64,NativeScriptBinding_RadiansDouble));
    const char* unsupported[]={"position.delta.x","acceleration.x","facing.direction","input.x","grounded","movement.fastfall"};
    for(unsigned i=0;i<sizeof(unsupported)/sizeof(*unsupported);++i)CHECK(parse(unsupported[i],NativeScriptBindings_Article,false,0,0));
    CHECK(parse("position.x",NativeScriptBindings_Effect,false,0,0));
    CHECK(parse("position.unknown",NativeScriptBindings_Fighter,false,0,0));
    CHECK(parse("position[0]",NativeScriptBindings_Fighter,false,0,0));
    CHECK(parse("sqrt()",NativeScriptBindings_Fighter,false,0,0));
    CHECK(parse("sqrt(\"x\")",NativeScriptBindings_Fighter,false,0,0));
    CHECK(parse("position.x(1)",NativeScriptBindings_Fighter,false,0,0));
    NativeScriptBindingResolverContext binding={NativeScriptBindings_Fighter};NativeScriptCallLayout layout;
    CHECK(nativeScriptBindings_DescribeCall(&binding,NativeScriptBinding_Atan2,&layout,&diagnostic));CHECK(layout.argument_count==2&&layout.argument_types[0]==NativeScriptType_F64&&layout.output_mask==0);
    CHECK(nativeScriptBindings_DescribeCall(&binding,NativeScriptBinding_Int,&layout,&diagnostic));CHECK(layout.argument_types[0]==NativeScriptType_Invalid);
    CHECK(!nativeScriptBindings_DescribeCall(&binding,NativeScriptBinding_PositionX,&layout,&diagnostic));
    return true;
}

static bool invoke(uint32_t id,NativeScriptValue* args,unsigned count,NativeScriptValue* result)
{ return nativeScriptBindings_Invoke(&fighter_context,id,NULL,0,args,count,result,&diagnostic); }

static bool math_suite(void)
{
    CHECK(setup());NativeScriptValue args[2],value;
    args[0]=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0xffffffff80000001)};
    CHECK(invoke(NativeScriptBinding_UInt,args,1,&value));CHECK(value.type==NativeScriptType_U32&&value.as.u64==UINT32_C(0x80000001));
    CHECK(invoke(NativeScriptBinding_Int,args,1,&value));CHECK(value.type==NativeScriptType_I32&&value.as.i64==INT32_MIN+1);
    args[0]=(NativeScriptValue){.type=NativeScriptType_F64,.as.f64=-3.875};
    CHECK(invoke(NativeScriptBinding_Int,args,1,&value)&&value.as.i64==-3);
    CHECK(invoke(NativeScriptBinding_Float,args,1,&value)&&value.type==NativeScriptType_F32&&value.as.f32==-3.875f);
    CHECK(invoke(NativeScriptBinding_Abs,args,1,&value)&&value.as.f32==3.875f);
    args[0]=(NativeScriptValue){.type=NativeScriptType_F64,.as.f64=90.0123456789};
    CHECK(invoke(NativeScriptBinding_RadiansDouble,args,1,&value));double radian_reference=args[0].as.f64*(double)0.01745329252f;CHECK(!memcmp(&value.as.f64,&radian_reference,8));
    CHECK(invoke(NativeScriptBinding_RadiansFloat,args,1,&value));float radian_float=(float)args[0].as.f64*0.01745329252f;CHECK(!memcmp(&value.as.f32,&radian_float,4));
    CHECK(nativeScriptBindings_Read(&fighter_context,NativeScriptBinding_PiHalf,NULL,0,&value,&diagnostic)&&value.as.f64==3.14159265358979323846/2.0);
    args[0]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=-0.0f};CHECK(invoke(NativeScriptBinding_Signbit,args,1,&value)&&value.as.i64==1);
    args[0]=(NativeScriptValue){.type=NativeScriptType_F64,.as.f64=0.123456789123456};args[1]=(NativeScriptValue){.type=NativeScriptType_F64,.as.f64=-0.98765432198765};
    CHECK(invoke(NativeScriptBinding_Atan2,args,2,&value));double reference=atan2(args[0].as.f64,args[1].as.f64);CHECK(value.type==NativeScriptType_F64&&!memcmp(&value.as.f64,&reference,8));
    CHECK(nativeScriptBindings_Invoke(&article_context,NativeScriptBinding_ArticleAtan2,NULL,0,args,2,&value,&diagnostic));float reference_f=atan2f((float)args[0].as.f64,(float)args[1].as.f64);CHECK(value.type==NativeScriptType_F32&&!memcmp(&value.as.f32,&reference_f,4));
    args[0]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=NAN};args[1]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=2};
    CHECK(invoke(NativeScriptBinding_Min,args,2,&value)&&value.as.f32==2);CHECK(invoke(NativeScriptBinding_Max,args,2,&value)&&value.as.f32==2);
    CHECK(!invoke(NativeScriptBinding_Int,args,1,&value));
    args[0]=(NativeScriptValue){.type=NativeScriptType_Text,.as.text={1,3}};CHECK(!invoke(NativeScriptBinding_Float,args,1,&value));
    CHECK(!invoke(NativeScriptBinding_PositionX,args,1,&value));
    CHECK(!nativeScriptBindings_Invoke(&article_context,NativeScriptBinding_Atan2,NULL,0,args,2,&value,&diagnostic));
    args[0]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=4};++fighter.x8_spawnNum;
    NativeScriptValue before={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};value=before;
    CHECK(!invoke(NativeScriptBinding_Sqrt,args,1,&value)&&!memcmp(&before,&value,sizeof(value)));
    return true;
}

static bool bytecode_case(const char* source,unsigned family,
    NativeScriptRuntimeContext* context,float expected)
{
    NativeScriptSourceNode syntax[128];NativeScriptLocal locals[32];
    NativeScriptInstruction instructions[1024];NativeScriptFunctionCode function[1];
    NativeScriptValue registers[256];NativeScriptVMFrame frames[8];
    NativeScriptSourceTree tree={source,(uint32_t)strlen(source),syntax,0,128};
    CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
    uint32_t body=NATIVE_SCRIPT_NODE_NONE;
    for(uint32_t i=0;i<tree.count;++i)if(tree.nodes[i].kind==NativeScriptSource_Function)body=i;
    CHECK(body!=NATIVE_SCRIPT_NODE_NONE);
    NativeScriptIRArena arena={source,(uint32_t)strlen(source),nodes,0,512,arguments,0,256};
    NativeScriptBindingResolverContext binding={family};
    NativeScriptStatementCompiler input={.tree=&tree,.expressions=&arena,
        .bindings=nativeScriptBindings_Resolve,.binding_context=&binding,
        .locals=locals,.local_capacity=32,.return_type=NativeScriptType_F32};
    CHECK(nativeScriptStatement_Compile(&input,body,&diagnostic));
    NativeScriptBytecodeBuilder builder={.instructions=instructions,.instruction_capacity=1024,
        .functions=function,.function_count=1,.describe_call=nativeScriptBindings_DescribeCall,.binding_context=&binding};
    CHECK(nativeScriptBytecode_Compile(&builder,&input,body,0,0,&diagnostic));
    NativeScriptBytecodeProgram program={source,(uint32_t)strlen(source),instructions,builder.instruction_count,function,1,false}; { NativeScriptDiagnostic preparation={0}; if(!nativeScriptBytecode_Prepare(&program,&preparation)){fprintf(stderr,"bytecode preparation: %s\n",preparation.message);abort();} }
    NativeScriptVM vm={.registers=registers,.register_capacity=256,.frames=frames,.frame_capacity=8};
    NativeScriptValue result={0};
    CHECK(nativeScriptVM_Run(&vm,&program,&nativeScriptBindingHost,context,0,NULL,0,&result)==NativeScriptVM_Ok);
    CHECK(result.type==NativeScriptType_F32&&!memcmp(&result.as.f32,&expected,4));
    CHECK(vm.depth==0&&vm.register_top==0);
    // Retired actors must stop before any property write or result publication.
    ++context->generation;result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
    CHECK(nativeScriptVM_Run(&vm,&program,&nativeScriptBindingHost,context,0,NULL,0,&result)==NativeScriptVM_SourceRetired);
    CHECK(result.type==NativeScriptType_U64&&result.as.u64==UINT64_MAX);
    --context->generation;
    return true;
}

static bool bytecode_suite(void)
{
    CHECK(setup());fighter.cur_pos=(Vec3){17.125f,-6.25f,1};fighter.self_vel=(Vec3){2,-3.875f,99};
    fighter.facing_dir=-1;fighter.ground_or_air=GA_Ground;fighter.input.held_buttons[0]=HSD_PAD_B;
    float expected=(float)(0.95*17.125f+0.05*-3.875f);
    const char* fighter_source="func NativeSlice() -> float:\n"
        "    let float interpolation = 0.95 * position.x + 0.05 * velocity.y\n"
        "    position.x = interpolation\n"
        "    velocity.z = sqrt(abs(position.y))\n"
        "    movement.used_tether = input.special_held and not airborne\n"
        "    facing.direction = -facing.direction\n"
        "    return position.x\n";
    CHECK(bytecode_case(fighter_source,NativeScriptBindings_Fighter,&fighter_context,expected));
    CHECK(fighter.cur_pos.x==expected&&fighter.self_vel.z==2.5f&&fighter.used_tether&&fighter.facing_dir==1);
    article.pos=(Vec3){21.5f,-9,4};article.x40_vel=(Vec3){-1,2.875f,99};article.facing_dir=1;
    const char* article_source="func ArticleSlice() -> float:\n"
        "    position.x = position.x + velocity.y * 0.5\n"
        "    velocity.z = sqrt(abs(position.y))\n"
        "    facing = -facing\n"
        "    return position.x\n";
    CHECK(bytecode_case(article_source,NativeScriptBindings_Article,&article_context,22.9375f));
    CHECK(article.pos.x==22.9375f&&article.x40_vel.z==3&&article.facing_dir==-1);
    return true;
}

static bool input_history_suite(void)
{
    CHECK(setup());
    fighter.x670_timer_lstick_tilt_x=91;fighter.x674=73;fighter.x675=52;
    const NativeScriptIRType types[]={NativeScriptType_I32,NativeScriptType_I64,NativeScriptType_U32,NativeScriptType_U64};
    for(unsigned type=0;type<4;++type)for(unsigned age=0;age<=0xFE;++age){
        NativeScriptValue input={.type=types[type]};
        if(type<2)input.as.i64=age;else input.as.u64=age;
        Fighter expected=fighter;expected.x673=(u8)age;
        CHECK(nativeScriptBindings_Write(&fighter_context,NativeScriptBinding_InputHorizontalSmashAge,NULL,0,&input,&diagnostic));
        CHECK(!memcmp(&expected,&fighter,sizeof(fighter)));
        NativeScriptValue observed={0};CHECK(nativeScriptBindings_Read(&fighter_context,NativeScriptBinding_InputHorizontalSmashAge,NULL,0,&observed,&diagnostic));
        CHECK(observed.type==NativeScriptType_I32&&observed.as.i64==age);
    }
    const NativeScriptValue invalid[]={
        {.type=NativeScriptType_I32,.as.i64=-1},{.type=NativeScriptType_I32,.as.i64=255},
        {.type=NativeScriptType_I64,.as.i64=INT64_MIN},{.type=NativeScriptType_I64,.as.i64=INT64_MAX},
        {.type=NativeScriptType_I64,.as.i64=INT64_C(0x100000000)},
        {.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000000)},
        {.type=NativeScriptType_U64,.as.u64=UINT64_MAX},{.type=NativeScriptType_U32,.as.u64=UINT32_MAX},
        {.type=NativeScriptType_Bool,.as.u64=2},{.type=NativeScriptType_F32,.as.f32=0},
    };
    for(unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);++i){Fighter before=fighter;
        CHECK(!nativeScriptBindings_Write(&fighter_context,NativeScriptBinding_InputHorizontalSmashAge,NULL,0,&invalid[i],&diagnostic));
        CHECK(!memcmp(&before,&fighter,sizeof(fighter)));
    }
    NativeScriptValue zero={.type=NativeScriptType_I32,.as.i64=0};Fighter before=fighter;
    CHECK(!nativeScriptBindings_Write(&article_context,NativeScriptBinding_InputHorizontalSmashAge,NULL,0,&zero,&diagnostic));
    CHECK(!memcmp(&before,&fighter,sizeof(fighter)));
    CHECK(!nativeScriptBindings_Write(&fighter_context,NativeScriptBinding_InputHorizontalSmashAge,&zero,1,&zero,&diagnostic));
    CHECK(!memcmp(&before,&fighter,sizeof(fighter)));
    NativeScriptRuntimeContext retired=fighter_context;++retired.generation;
    CHECK(!nativeScriptBindings_Write(&retired,NativeScriptBinding_InputHorizontalSmashAge,NULL,0,&zero,&diagnostic));
    CHECK(!memcmp(&before,&fighter,sizeof(fighter)));
    CHECK(bytecode_case("func ConsumeSmash() -> float:\n    input.horizontal_smash_age = 0\n    return float(input.horizontal_smash_age)\n",NativeScriptBindings_Fighter,&fighter_context,0));
    CHECK(fighter.x673==0&&fighter.x670_timer_lstick_tilt_x==91&&fighter.x674==73&&fighter.x675==52);
    return true;
}

static bool common_property_suite(void)
{
    CHECK(setup());
    CHECK(p_ftCommonData);
    for(unsigned sample=0;sample<129;++sample) {
        float base=((int)sample-64)*.0625f;
        fighter.co_attrs.terminal_velocity=base+1;
        p_ftCommonData->x44C=base+2;
        fighter.x34_scale.y=base+3;fighter.co_attrs.model_scaling=base+4;
        fighter.co_attrs.xBC.size=base+13;
        fighter.coll_data.ceiling.normal=(Vec3){base+5,base+6,91};
        fighter.coll_data.left_facing_wall.normal=(Vec3){base+7,base+8,92};
        fighter.coll_data.right_facing_wall.normal=(Vec3){base+9,base+10,93};
        fighter.coll_data.ecb.top.y=base+11;fighter.coll_data.ecb.bottom.y=base+12;
        const uint32_t ids[]={NativeScriptBinding_AirTerminalVelocity,NativeScriptBinding_ControlsStickSmoothing,
            NativeScriptBinding_ModelInstanceScaleY,NativeScriptBinding_CeilingNormalX,NativeScriptBinding_CeilingNormalY,
            NativeScriptBinding_WallLeftNormalX,NativeScriptBinding_WallLeftNormalY,
            NativeScriptBinding_WallRightNormalX,NativeScriptBinding_WallRightNormalY,
            NativeScriptBinding_ECBTopY,NativeScriptBinding_ECBBottomY};
        const float offsets[]={1,2,3,5,6,7,8,9,10,11,12};
        for(unsigned i=0;i<sizeof(ids)/sizeof(*ids);++i)CHECK(read_float(&fighter_context,ids[i],base+offsets[i]));
        CHECK(read_float(&fighter_context,NativeScriptBinding_ModelScale,reference_model_scale(&fighter)));
        CHECK(read_float(&fighter_context,NativeScriptBinding_ModelEffectScale,fighter.co_attrs.xBC.size));
        NativeScriptValue id={0};CHECK(nativeScriptBindings_Read(&fighter_context,NativeScriptBinding_FighterCreationID,NULL,0,&id,&diagnostic));
        CHECK(id.type==NativeScriptType_U32&&id.as.u64==UINT32_C(0x80000001));
        NativeScriptValue flag={.type=NativeScriptType_Bool,.as.u64=sample&1};
        CHECK(nativeScriptBindings_Write(&fighter_context,NativeScriptBinding_ItemsBlockGroundPickup,NULL,0,&flag,&diagnostic));
        CHECK(fighter.x2223_b4==(sample&1));CHECK(read_bool(NativeScriptBinding_ItemsBlockGroundPickup,sample&1));
        CHECK(nativeScriptBindings_Write(&fighter_context,NativeScriptBinding_ControlsInterruptAllowed,NULL,0,&flag,&diagnostic));
        CHECK(fighter.allow_interrupt==(sample&1));CHECK(read_bool(NativeScriptBinding_ControlsInterruptAllowed,sample&1));
    }
    NativeScriptValue unchanged={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
    ftCommonData* common=p_ftCommonData;p_ftCommonData=NULL;
    CHECK(!nativeScriptBindings_Read(&fighter_context,NativeScriptBinding_ControlsStickSmoothing,NULL,0,&unchanged,&diagnostic));
    CHECK(unchanged.type==NativeScriptType_U64&&unchanged.as.u64==UINT64_MAX);p_ftCommonData=common;
    CHECK(!nativeScriptBindings_Read(&article_context,NativeScriptBinding_ModelScale,NULL,0,&unchanged,&diagnostic));
    CHECK(unchanged.type==NativeScriptType_U64&&unchanged.as.u64==UINT64_MAX);
    CHECK(!nativeScriptBindings_Read(&article_context,NativeScriptBinding_ModelEffectScale,NULL,0,&unchanged,&diagnostic));
    CHECK(unchanged.type==NativeScriptType_U64&&unchanged.as.u64==UINT64_MAX);
    NativeScriptValue invalid={.type=NativeScriptType_F32,.as.f32=1};
    CHECK(!nativeScriptBindings_Write(&fighter_context,NativeScriptBinding_ModelScale,NULL,0,&invalid,&diagnostic));
    CHECK(!nativeScriptBindings_Write(&fighter_context,NativeScriptBinding_ModelEffectScale,NULL,0,&invalid,&diagnostic));
    fighter.x34_scale.y=2;fighter.co_attrs.model_scaling=3;fighter.x2223_b4=false;
    fighter.coll_data.ceiling.normal.x=4;fighter.co_attrs.terminal_velocity=5;p_ftCommonData->x44C=6;
    CHECK(bytecode_case("func CommonSlice() -> float:\n    items.block_ground_pickup = true\n    return model.scale + ceiling.normal.x + air.terminal_velocity + controls.stick_smoothing\n",NativeScriptBindings_Fighter,&fighter_context,21));
    CHECK(fighter.x2223_b4);
    fighter.co_attrs.xBC.size=.375f;
    CHECK(bytecode_case("func EffectSize() -> float:\n    return model.effect_scale\n",NativeScriptBindings_Fighter,&fighter_context,.375f));
    CHECK(bytecode_case("func IasaSlice() -> float:\n    controls.interrupt_allowed = false\n    if controls.interrupt_allowed:\n        return 0.0\n    controls.interrupt_allowed = true\n    if controls.interrupt_allowed:\n        return 1.0\n    return 0.0\n",NativeScriptBindings_Fighter,&fighter_context,1));
    CHECK(fighter.allow_interrupt);
    NativeScriptValue policy={.type=NativeScriptType_Bool,.as.u64=0};
    CHECK(!nativeScriptBindings_Write(&article_context,NativeScriptBinding_ControlsInterruptAllowed,NULL,0,&policy,&diagnostic));
    CHECK(fighter.allow_interrupt);
    CHECK(!nativeScriptBindings_Write(&fighter_context,NativeScriptBinding_ControlsInterruptAllowed,&policy,1,&policy,&diagnostic));
    CHECK(fighter.allow_interrupt);
    ++fighter.x8_spawnNum;
    CHECK(!nativeScriptBindings_Write(&fighter_context,NativeScriptBinding_ControlsInterruptAllowed,NULL,0,&policy,&diagnostic));
    CHECK(fighter.allow_interrupt);
    --fighter.x8_spawnNum;
    const float facings[]={-1,-.25f,-0.0f,0,.25f,1,NAN};
    for(unsigned i=0;i<sizeof(facings)/sizeof(*facings);++i) {
        fighter.facing_dir=facings[i];NativeScriptValue sign={0};
        CHECK(nativeScriptBindings_Read(&fighter_context,NativeScriptBinding_FacingSign,NULL,0,&sign,&diagnostic));
        CHECK(sign.type==NativeScriptType_I32&&sign.as.i64==reference_facing_sign(&fighter));
    }
    return true;
}

static HSD_JObj rendered_root;
static HSD_ClassInfo root_subclass,unrelated_class;
static unsigned root_admission_mutation,root_comparisons,root_rejections;
static void root_class_initialize(void)
{
    HSD_CLASS_INFO(&hsdJObj)->head.flags=1;
    switch(root_admission_mutation){
    case 1:++fighter.x8_spawnNum;break;
    case 2:fighter_object.hsd_obj=NULL;break;
    case 3:fighter_object.user_data=NULL;break;
    case 4:fighter_object.obj_kind=0;break;
    case 5:HSD_CLASS_METHOD(&rendered_root)=&unrelated_class;break;
    }
}
static void initialize_root(void)
{
    memset(&rendered_root,0,sizeof(rendered_root));
    memset(&hsdJObj,0,sizeof(hsdJObj));
    HSD_CLASS_INFO(&hsdJObj)->head=(HSD_ClassInfoHead){.flags=1,.obj_size=sizeof(HSD_JObj),.info_init=root_class_initialize};
    root_subclass=(HSD_ClassInfo){.head={.flags=1,.obj_size=sizeof(HSD_JObj)+16,.parent=HSD_CLASS_INFO(&hsdJObj)}};
    unrelated_class=(HSD_ClassInfo){.head={.flags=1,.obj_size=sizeof(HSD_JObj)}};
    HSD_CLASS_METHOD(&rendered_root)=HSD_CLASS_INFO(&hsdJObj);
    fighter_object.hsd_obj=&rendered_root;fighter_object.obj_kind=HSD_GObj_JObjKind;
    root_admission_mutation=0;
}
static bool reject_root_read(void)
{
    NativeScriptValue value={.type=NativeScriptType_U64,.as.u64=UINT64_MAX},before=value;
    CHECK(!nativeScriptBindings_Read(&fighter_context,NativeScriptBinding_ModelRootScaleY,NULL,0,&value,&diagnostic));
    CHECK(!memcmp(&value,&before,sizeof(value)));++root_rejections;return true;
}
static bool root_scale_suite(void)
{
    CHECK(setup());initialize_root();
    const NativeScriptBindingDescriptor* descriptor=nativeScriptBindings_Descriptor(NativeScriptBinding_ModelRootScaleY);
    CHECK(descriptor&&descriptor->result_type==NativeScriptType_F32);
    CHECK((descriptor->flags&(NativeScriptFlag_Readable|NativeScriptFlag_Pure))==(NativeScriptFlag_Readable|NativeScriptFlag_Pure));
    CHECK(!(descriptor->flags&(NativeScriptFlag_Writable|NativeScriptFlag_SourceOnly)));
    CHECK(parse("model.root_scale_y",NativeScriptBindings_Fighter,true,NativeScriptType_F32,NativeScriptBinding_ModelRootScaleY));
    CHECK(parse("model.root_scale_y",NativeScriptBindings_Article,false,0,0));
    CHECK(parse("model.root_scale_y",NativeScriptBindings_Effect,false,0,0));
    const uint32_t bits[]={0,UINT32_C(0x80000000),UINT32_C(0x3f000000),UINT32_C(0xc0400000),
        UINT32_C(0x7f800000),UINT32_C(0xff800000),UINT32_C(0x7fc01234)};
    const char* body="func RenderedScale() -> float:\n    return model.root_scale_y\n";
    for(unsigned subclass=0;subclass<2;++subclass){
        HSD_CLASS_METHOD(&rendered_root)=subclass?&root_subclass:HSD_CLASS_INFO(&hsdJObj);
        for(unsigned i=0;i<sizeof(bits)/sizeof(*bits);++i){
            memcpy(&rendered_root.scale.y,&bits[i],4);
            fighter.x34_scale.y=19;fighter.co_attrs.model_scaling=23;
            float expected=reference_root_scale_y(&fighter_object);
            CHECK(read_float(&fighter_context,NativeScriptBinding_ModelRootScaleY,expected));++root_comparisons;
            CHECK(bytecode_case(body,NativeScriptBindings_Fighter,&fighter_context,expected));
        }
    }
    // This common native property admits a registered Fighter without a pinned
    // caller program or equal fighter kind, as required by retained receivers.
    fighter.kind=Ft_Kind_Captain;
    rendered_root.scale.y=2.25f;
    CHECK(read_float(&fighter_context,NativeScriptBinding_ModelRootScaleY,2.25f));
    fighter_object.hsd_obj=NULL;
    CHECK(read_float(&fighter_context,NativeScriptBinding_ModelRootScaleY,reference_root_scale_y(&fighter_object)));++root_comparisons;
    CHECK(bytecode_case(body,NativeScriptBindings_Fighter,&fighter_context,1));
    NativeScriptValue write={.type=NativeScriptType_F32,.as.f32=3};
    CHECK(!nativeScriptBindings_Write(&fighter_context,NativeScriptBinding_ModelRootScaleY,NULL,0,&write,&diagnostic));
    NativeScriptValue value={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
    CHECK(!nativeScriptBindings_Read(&article_context,NativeScriptBinding_ModelRootScaleY,NULL,0,&value,&diagnostic));
    CHECK(value.type==NativeScriptType_U64&&value.as.u64==UINT64_MAX);
    initialize_root();HSD_CLASS_METHOD(&rendered_root)=&unrelated_class;CHECK(reject_root_read());
    initialize_root();HSD_CLASS_METHOD(&rendered_root)=NULL;CHECK(reject_root_read());
    initialize_root();HSD_CLASS_INFO(&hsdJObj)->head.obj_size=sizeof(HSD_JObj)-1;CHECK(reject_root_read());
    initialize_root();fighter_object.obj_kind=0;CHECK(reject_root_read());
    initialize_root();fighter_object.classifier=0;CHECK(reject_root_read());fighter_object.classifier=HSD_GOBJ_CLASS_FIGHTER;
    initialize_root();fighter.gobj=&article_object;CHECK(reject_root_read());fighter.gobj=&fighter_object;
    initialize_root();++fighter.x8_spawnNum;CHECK(reject_root_read());--fighter.x8_spawnNum;
    for(unsigned mutation=1;mutation<=5;++mutation){
        CHECK(setup());initialize_root();root_admission_mutation=mutation;
        HSD_CLASS_INFO(&hsdJObj)->head.flags=0;
        CHECK(reject_root_read());
    }
    CHECK(setup());initialize_root();heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;fighter_object.user_data=(void*)1;
    CHECK(reject_root_read());return true;
}

static bool jump_observation_suite(void)
{
    CHECK(setup());CHECK(p_ftCommonData);
    const uint32_t ids[]={NativeScriptBinding_ControlsAerialJumpReverseThreshold,
        NativeScriptBinding_AirJumpHorizontalMultiplier,NativeScriptBinding_ControlsTapJumpThreshold,
        NativeScriptBinding_ControlsAerialActionDownThreshold,NativeScriptBinding_ControlsHorizontalStickDeadzone};
    const char* names[]={"controls.aerial_jump_reverse_threshold","air.jump_horizontal_multiplier",
        "controls.tap_jump_threshold","controls.aerial_action_down_threshold","controls.horizontal_stick_deadzone"};
    float* fields[]={&p_ftCommonData->x78,&fighter.co_attrs.air_jump_h_multiplier,
        &p_ftCommonData->tap_jump_threshold,&p_ftCommonData->x88,&p_ftCommonData->horizontal_stick_deadzone};
    const uint32_t bits[]={0,UINT32_C(0x80000000),UINT32_C(0x3f000000),UINT32_C(0xbf400000),
        UINT32_C(0x7f800000),UINT32_C(0xff800000),UINT32_C(0x7fc01234)};
    NativeScriptValue unchanged={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
    NativeScriptValue write={.type=NativeScriptType_F32,.as.f32=1};
    for(unsigned i=0;i<sizeof(ids)/sizeof(*ids);++i){
        const NativeScriptBindingDescriptor* descriptor=nativeScriptBindings_Descriptor(ids[i]);
        CHECK(descriptor&&descriptor->result_type==NativeScriptType_F32&&
            (descriptor->flags&(NativeScriptFlag_Readable|NativeScriptFlag_Pure))==(NativeScriptFlag_Readable|NativeScriptFlag_Pure)&&
            !(descriptor->flags&(NativeScriptFlag_Writable|NativeScriptFlag_Callable)));
        CHECK(parse(names[i],NativeScriptBindings_Fighter,true,NativeScriptType_F32,ids[i]));
        CHECK(parse(names[i],NativeScriptBindings_Article,false,0,0));
        CHECK(parse(names[i],NativeScriptBindings_Effect,false,0,0));
        CHECK(!nativeScriptBindings_Write(&fighter_context,ids[i],NULL,0,&write,&diagnostic));
        CHECK(!nativeScriptBindings_Read(&article_context,ids[i],NULL,0,&unchanged,&diagnostic));
        CHECK(unchanged.type==NativeScriptType_U64&&unchanged.as.u64==UINT64_MAX);
        for(unsigned n=0;n<sizeof(bits)/sizeof(*bits);++n){
            memcpy(fields[i],&bits[n],4);CHECK(read_float(&fighter_context,ids[i],*fields[i]));
            char source[192];snprintf(source,sizeof(source),"func Observe() -> float:\n    return %s\n",names[i]);
            CHECK(bytecode_case(source,NativeScriptBindings_Fighter,&fighter_context,*fields[i]));
        }
        char source[192];snprintf(source,sizeof(source),"func Bad():\n    %s = 0\n",names[i]);
        NativeScriptSourceNode syntax[16];NativeScriptLocal locals[4];
        NativeScriptSourceTree tree={source,(uint32_t)strlen(source),syntax,0,16};
        CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
        NativeScriptIRArena arena={source,(uint32_t)strlen(source),nodes,0,512,arguments,0,256};
        NativeScriptBindingResolverContext binding={NativeScriptBindings_Fighter};
        NativeScriptStatementCompiler compiler={.tree=&tree,.expressions=&arena,.bindings=nativeScriptBindings_Resolve,
            .binding_context=&binding,.locals=locals,.local_capacity=4,.return_type=NativeScriptType_Void};
        CHECK(!nativeScriptStatement_Compile(&compiler,1,&diagnostic));CHECK(strstr(diagnostic.message,"not writable"));
    }
    ftCommonData* common=p_ftCommonData;p_ftCommonData=NULL;
    for(unsigned i=0;i<sizeof(ids)/sizeof(*ids);++i)if(i!=1){CHECK(!nativeScriptBindings_Read(&fighter_context,ids[i],NULL,0,&unchanged,&diagnostic));
        CHECK(unchanged.type==NativeScriptType_U64&&unchanged.as.u64==UINT64_MAX);}
    CHECK(read_float(&fighter_context,ids[1],fighter.co_attrs.air_jump_h_multiplier));p_ftCommonData=common;
    p_ftCommonData->x78=.5f;
    const float inputs[]={-1,-.5f,-.25f,-0.0f,0,.25f,.5f,1,NAN};
    const char* direction="func Choose() -> float:\n    if input.x * facing.direction > -controls.aerial_jump_reverse_threshold:\n        return 1\n    return 0\n";
    for(unsigned facing=0;facing<2;++facing)for(unsigned i=0;i<sizeof(inputs)/sizeof(*inputs);++i){
        fighter.facing_dir=facing?-1:1;fighter.input.lstick[0].x=inputs[i];
        float expected=reference_float_direction(&fighter_object)==ftPe_MS_FloatFallF?1:0;
        CHECK(bytecode_case(direction,NativeScriptBindings_Fighter,&fighter_context,expected));
    }
    ++fighter.x8_spawnNum;
    for(unsigned i=0;i<sizeof(ids)/sizeof(*ids);++i){CHECK(!nativeScriptBindings_Read(&fighter_context,ids[i],NULL,0,&unchanged,&diagnostic));
        CHECK(unchanged.type==NativeScriptType_U64&&unchanged.as.u64==UINT64_MAX);}
    return true;
}

extern unsigned script_unavailable_spawn_calls;
extern uint32_t script_unavailable_spawn_semantic,script_unavailable_spawn_count;
extern NativeScriptRuntimeContext* script_unavailable_spawn_context;
extern NativeScriptValue script_unavailable_spawn_arguments[16];
static bool spawn_dispatch_suite(void)
{
    CHECK(setup());
    const uint32_t base[]={NativeScriptBinding_SpawnAuthoredProjectile,NativeScriptBinding_SpawnBoneAttached,NativeScriptBinding_SpawnAttachedPresentation};
    for(unsigned operation=1;operation<=3;++operation){
        const NativeScriptBindingDescriptor* descriptor=nativeScriptBindings_Descriptor(base[operation-1]);
        NativeScriptValue args[16]={0},before[16];
        for(unsigned i=0;i<descriptor->argument_count;++i){
            args[i].type=descriptor->arguments[i].type;
            if(args[i].type==NativeScriptType_F32)args[i].as.f32=3.25f;
            else if(args[i].type==NativeScriptType_I32)args[i].as.i64=(int64_t)(7+i);
            else args[i].as.u64=UINT32_C(0x80000000)+i;
        }
        memcpy(before,args,sizeof(args));script_unavailable_spawn_calls=0;
        uint32_t semantic=NATIVE_SCRIPT_SPAWN_DOMAIN|(operation<<20)|17;
        NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
        // The actual top-level dispatcher must reach the explicitly unavailable
        // factory leaf, even though encoded IDs exceed every normal enum range.
        CHECK(!nativeScriptBindings_Invoke(&fighter_context,semantic,NULL,0,args,descriptor->argument_count,&result,&diagnostic));
        CHECK(script_unavailable_spawn_calls==1&&script_unavailable_spawn_context==&fighter_context);
        CHECK(script_unavailable_spawn_semantic==semantic&&script_unavailable_spawn_count==descriptor->argument_count);
        CHECK(!memcmp(args,before,sizeof(args)));
        CHECK(!memcmp(args,script_unavailable_spawn_arguments,descriptor->argument_count*sizeof(*args)));
        CHECK(result.type==NativeScriptType_U64&&result.as.u64==UINT64_MAX);
    }
    return true;
}
static bool ecb_suite(void)
{
    CHECK(setup());
    const uint32_t ids[]={NativeScriptBinding_ECBTopX,NativeScriptBinding_ECBTopY,
        NativeScriptBinding_ECBBottomX,NativeScriptBinding_ECBBottomY,
        NativeScriptBinding_ECBLeftX,NativeScriptBinding_ECBLeftY,
        NativeScriptBinding_ECBRightX,NativeScriptBinding_ECBRightY};
    const char* names[]={"ecb.top.x","ecb.top.y","ecb.bottom.x","ecb.bottom.y",
        "ecb.left.x","ecb.left.y","ecb.right.x","ecb.right.y"};
    float* fields[]={&fighter.coll_data.ecb.top.x,&fighter.coll_data.ecb.top.y,
        &fighter.coll_data.ecb.bottom.x,&fighter.coll_data.ecb.bottom.y,
        &fighter.coll_data.ecb.left.x,&fighter.coll_data.ecb.left.y,
        &fighter.coll_data.ecb.right.x,&fighter.coll_data.ecb.right.y};
    const uint32_t bits[]={0,0x80000000,0x3f800000,0xc0200000,1,0x80000001,
        0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345};
    for(unsigned field=0;field<8;++field){
        const NativeScriptBindingDescriptor* descriptor=nativeScriptBindings_Descriptor(ids[field]);
        CHECK(descriptor&&descriptor->result_type==NativeScriptType_F32&&
            (descriptor->flags&NativeScriptFlag_Readable)&&!(descriptor->flags&NativeScriptFlag_Writable));
        CHECK(parse(names[field],NativeScriptBindings_Fighter,true,NativeScriptType_F32,ids[field]));
        CHECK(parse(names[field],NativeScriptBindings_Article,false,0,0));
        CHECK(parse(names[field],NativeScriptBindings_Effect,false,0,0));
        for(unsigned sample=0;sample<sizeof(bits)/sizeof(*bits);++sample){
            for(unsigned other=0;other<8;++other)memcpy(fields[other],&bits[(sample+other)%11],4);
            static Fighter before;before=fighter;NativeScriptValue value;
            CHECK(nativeScriptBindings_Read(&fighter_context,ids[field],NULL,0,&value,&diagnostic));
            CHECK(value.type==NativeScriptType_F32&&!memcmp(&value.as.f32,fields[field],4));
            CHECK(!memcmp(&fighter,&before,sizeof(fighter)));++properties;
            CHECK(!nativeScriptBindings_Write(&fighter_context,ids[field],NULL,0,&value,&diagnostic));
            CHECK(!memcmp(&fighter,&before,sizeof(fighter)));
        }
        NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=UINT64_MAX},value=sentinel,selector={.type=NativeScriptType_I32};
        CHECK(!nativeScriptBindings_Read(&fighter_context,ids[field],&selector,1,&value,&diagnostic));CHECK(!memcmp(&value,&sentinel,sizeof(value)));
        CHECK(!nativeScriptBindings_Read(&article_context,ids[field],NULL,0,&value,&diagnostic));CHECK(!memcmp(&value,&sentinel,sizeof(value)));
        ++fighter_context.generation;
        CHECK(!nativeScriptBindings_Read(&fighter_context,ids[field],NULL,0,&value,&diagnostic));CHECK(!memcmp(&value,&sentinel,sizeof(value)));
        --fighter_context.generation;
    }
    return true;
}
int main(void)
{
    if(!property_suite()||!coordinate_lifetime_rejections()||!semantic_suite()||!math_suite()||!bytecode_suite()||!input_history_suite()||!common_property_suite()||!root_scale_suite()||!jump_observation_suite()||!spawn_dispatch_suite()||!ecb_suite())return 1;
    printf("{\"checks\":%u,\"property_observations\":%u,\"semantic_expressions\":%u,\"root_scale_native_comparisons\":%u,\"root_scale_rejections\":%u,\"status\":\"PASS actual native property and typed binding contracts\"}\n",checks,properties,parses,root_comparisons,root_rejections);
    return 0;
}
