#include "engine_fixture.h"
#include "../script_kinetics/engine_fixture.h"
#include <melee/ft/native_script_bindings.h>
#include <melee/ft/native_script_program.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallSpecial.h>
#include <melee/ft/kinds/ftCommon/ftCo_Landing.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/mp/forward.h>
#include <melee/ef/forward.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/controller.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HSD_GObj** HSD_GObjPLinkHead;
static HSD_GObj* heads[32];static HSD_GObj object;static Fighter actor;

static NativeScriptRuntimeContext context;static NativeScriptDiagnostic diagnostic;
static unsigned checks,transitions;
#define CHECK(v) do { ++checks; if(!(v)){fprintf(stderr,"State check %d: %s (%s)\n",__LINE__,#v,diagnostic.message);exit(1);} }while(0)
bool nativeModelEffect_Snapshot(HSD_GObj* g,uint32_t* n){(void)g;(void)n;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* g,uint32_t n){(void)g;(void)n;return NULL;}
static void setup(void)
{
    scriptKineticsFixture_Reset();scriptStateFixture_Reset();memset(&actor,0,sizeof(actor));memset(&object,0,sizeof(object));memset(heads,0,sizeof(heads));
    HSD_GObjPLinkHead=heads;heads[HSD_GOBJ_PLINK_FIGHTER]=&object;object.classifier=HSD_GOBJ_CLASS_FIGHTER;object.user_data=&actor;actor.gobj=&object;
    actor.kind=Ft_Kind_CustomFirst;actor.behavior_kind=Ft_Kind_CustomFirst;actor.x8_spawnNum=17;actor.ground_or_air=GA_Air;
    actor.motion_id=333;actor.cur_pos=(Vec3){7,9,2};actor.self_vel=(Vec3){5,-2,1};actor.co_attrs.air_drift_max=1.5f;
    actor.co_attrs.max_jumps=2;actor.x2EC=29.9f;
    scriptKineticsFixture_OwnSource(&actor,NULL,NULL);
    CHECK(nativeScriptBindings_Context(&object,NativeCallbackSource_Fighter,17,&context,&diagnostic));
    context.scene_epoch=nativeGObjPool.lifetime;
}
static NativeScriptValue value_float(float v){return(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=v};}
static NativeScriptValue i32(int v){return(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=v};}
static void invoke(uint32_t id,NativeScriptValue* args,unsigned count)
{NativeScriptValue result;CHECK(nativeScriptBindings_Invoke(&context,id,NULL,0,args,count,&result,&diagnostic));CHECK(result.type==nativeScriptBindings_Descriptor(id)->result_type);++transitions;}
static void state_cases(void)
{
    setup();invoke(NativeScriptBinding_StateFall,NULL,0);CHECK(actor.motion_id==29&&actor.self_vel.x==1.5f);
    setup();NativeScriptValue args[]={i32(1),i32(0),{.type=NativeScriptType_Bool,.as.u64=1},value_float(0.75f),value_float(23.5f)};
    invoke(NativeScriptBinding_StateFreefall,args,5);CHECK(actor.motion_id==35&&actor.mv.co.fallspecial.mobility==1.125f&&actor.mv.co.fallspecial.landing_lag==23.5f&&actor.mv.co.fallspecial.allow_interrupt);
    setup();NativeScriptValue landing[]={i32(42),{.type=NativeScriptType_Bool,.as.u64=1},{.type=NativeScriptType_U32,.as.u64=0},value_float(0),value_float(1)};
    actor.u.mr.x2234_tornadoCharge=true;actor.u.mr.x2238_isCapeBoost=true;
    invoke(NativeScriptBinding_StateLanding,landing,5);CHECK(actor.motion_id==42&&actor.mv.co.landing.allow_interrupt&&script_state_hooks==0);CHECK(actor.u.mr.x2234_tornadoCharge&&actor.u.mr.x2238_isCapeBoost);
    setup();NativeScriptValue special[]={{.type=NativeScriptType_Bool,.as.u64=0},value_float(15)};invoke(NativeScriptBinding_StateLandingSpecial,special,2);CHECK(actor.motion_id==43&&!actor.mv.co.landing.allow_interrupt&&actor.frame_speed_mul==2);
    setup();invoke(NativeScriptBinding_StateFallAerial,NULL,0);CHECK(actor.motion_id==ftCo_MS_FallAerial);
    setup();NativeScriptValue free_blend[]={i32(1),i32(0),i32(1),value_float(0.5f),value_float(19),value_float(0.25f)};
    invoke(NativeScriptBinding_StateFreefallBlend,free_blend,6);CHECK(actor.motion_id==35&&actor.mv.co.fallspecial.mobility==0.75f&&actor.mv.co.fallspecial.landing_lag==19);
    setup();invoke(NativeScriptBinding_StateBasicLanding,NULL,0);CHECK(actor.motion_id==42&&actor.mv.co.landing.allow_interrupt);
    setup();invoke(NativeScriptBinding_StateLandingFallSpecial,special,2);CHECK(actor.motion_id==43&&actor.frame_speed_mul==2);
    setup();NativeScriptValue aerial[]={i32(70),value_float(10)};invoke(NativeScriptBinding_StateLandingAerial,aerial,2);CHECK(actor.motion_id==70&&actor.ground_or_air==GA_Ground);
    const uint32_t guarded[]={NativeScriptBinding_StateFall,NativeScriptBinding_StateFallAerial,NativeScriptBinding_StateLanding,NativeScriptBinding_StateFreefall};
    for(unsigned i=0;i<4;++i){setup();scriptStateFixture_RetireOnChange(true);NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
        NativeScriptValue* values=i==2?landing:i==3?args:NULL;unsigned count=i==2?5:i==3?5:0;
        CHECK(!nativeScriptBindings_Invoke(&context,guarded[i],NULL,0,values,count,&result,&diagnostic));CHECK(script_state_changes==1&&result.type==NativeScriptType_U64&&result.as.u64==UINT64_MAX);}
    // Shared fall primitive itself must stop before dereferencing poisoned data.
    setup();scriptStateFixture_RetireOnChange(true);ftCo_Fall_Enter(&object);CHECK(script_state_changes==1&&actor.self_vel.x==5);
    for(unsigned variant=0;variant<20;++variant){
        setup();NativeSourcePackage* source=scriptKineticsFixture_Source();NativeScriptReceiverView view;
        CHECK(nativeScriptFighter_View(&object,17,&view));
        switch(variant){
        case 0:source->address=NULL;break;case 1:source->ready=false;break;
        case 2:source->assets.ready=false;break;case 3:source->target=Ft_Kind_Mario;break;
        case 4:actor.behavior_kind=Ft_Kind_None;break;case 5:actor.ft_data=NULL;break;
        case 6:gFtDataList[actor.kind]=NULL;break;case 7:ftData_CharacterStateTables[actor.kind]=NULL;break;
        case 8:((NativeScriptFighterProgram*)nativeScriptFighter_Program(actor.kind))->bank=NULL;break;case 9:++source->program.identity;break;
        case 10:source->compiler.bank.valid=false;break;case 11:source->compiler.bank.has_motion_layout=false;break;
        case 12:script_fixture_source_status=NativeScriptVM_Error;break;case 13:view.storage->live=false;break;
        case 14:++view.storage->generation;break;case 15:view.storage->owner=1;break;
        case 16:view.storage->layout=NULL;break;case 17:++source->assets.facts.moves.count;break;
        case 18:++source->assets.facts.moves.transition_flags[0];break;
        case 19:script_fixture_source_view_program=NULL;break;
        }
        Fighter before=actor;NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
        CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_StateWait,NULL,0,NULL,0,&result,&diagnostic));
        CHECK(!memcmp(&before,&actor,sizeof(actor))&&script_state_changes==0);
        CHECK(result.type==NativeScriptType_U64&&result.as.u64==UINT64_MAX);
    }
}
static void queries(void)
{
    setup();NativeScriptValue value;
    actor.input.held_buttons[0]=HSD_PAD_A|HSD_PAD_X|HSD_PAD_Z|HSD_PAD_DPADUP|HSD_PAD_L|HSD_PAD_LR;actor.input.pressed_buttons=HSD_PAD_A|HSD_PAD_R|HSD_PAD_LR;
    const uint32_t positive[]={NativeScriptBinding_InputAttackHeld,NativeScriptBinding_InputAttackPressed,NativeScriptBinding_InputJumpHeld,NativeScriptBinding_InputShieldHeld,NativeScriptBinding_InputShieldPressed,NativeScriptBinding_InputUpHeld,NativeScriptBinding_InputZHeld,NativeScriptBinding_InputAttackOrSpecialPressed};
    for(unsigned i=0;i<sizeof(positive)/sizeof(*positive);++i){CHECK(nativeScriptBindings_Read(&context,positive[i],NULL,0,&value,&diagnostic));CHECK(value.type==NativeScriptType_Bool&&value.as.u64==1);}
    CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_InputDownHeld,NULL,0,&value,&diagnostic)&&value.as.u64==0);
    actor.input.held_buttons[0]=HSD_PAD_L|HSD_PAD_R;actor.input.pressed_buttons=HSD_PAD_L|HSD_PAD_R;
    CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_InputShieldHeld,NULL,0,&value,&diagnostic)&&value.as.u64==0);
    actor.input.lstick[1]=(Vec2){0.25f,-0.375f};actor.x673=12;actor.allow_interrupt=true;
    CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_InputPreviousX,NULL,0,&value,&diagnostic)&&value.as.f32==0.25f);
    CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_InputPreviousY,NULL,0,&value,&diagnostic)&&value.as.f32==-0.375f);
    CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_InputHorizontalSmashAge,NULL,0,&value,&diagnostic)&&value.as.i64==12);
    CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_ControlsInterruptAllowed,NULL,0,&value,&diagnostic)&&value.as.u64==1);
    for(unsigned mask=0;mask<16;++mask){actor.coll_data.env_flags=(mask&1?Collide_FloorMask:0)|(mask&2?Collide_LeftWallMask:0)|(mask&4?Collide_RightWallMask:0)|(mask&8?Collide_CeilingMask:0);actor.facing_dir=mask&1?-1:1;
        CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_CollisionFloorContact,NULL,0,&value,&diagnostic)&&value.as.u64==(uint64_t)((mask&1)!=0));
        CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_CollisionWallAhead,NULL,0,&value,&diagnostic)&&value.as.u64==(uint64_t)((actor.facing_dir==1&&(mask&2))||(actor.facing_dir==-1&&(mask&4))));
        CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_CollisionLeftGround,NULL,0,&value,&diagnostic)&&value.as.u64==(uint64_t)((mask&1)==0));
        CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_CollisionLanded,NULL,0,&value,&diagnostic)&&value.as.u64==(uint64_t)((mask&1)!=0));}
    actor.coll_data.floor.index=123;actor.coll_data.floor.normal=(Vec3){0.25f,0.75f,-0.125f};
    CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_FloorLine,NULL,0,&value,&diagnostic)&&value.as.i64==123);
    CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_FloorNormalX,NULL,0,&value,&diagnostic)&&value.as.f32==0.25f);
    CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_FloorNormalY,NULL,0,&value,&diagnostic)&&value.as.f32==0.75f);
    CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_FloorNormalZ,NULL,0,&value,&diagnostic)&&value.as.f32==-0.125f);
    NativeScriptValue box[]={value_float(12),value_float(0),value_float(-6),value_float(6),value_float(6),value_float(6)};
    CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_CollisionGroundBox,NULL,0,box,6,&value,&diagnostic));CHECK(value.type==NativeScriptType_Bool);
    CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_CollisionAirBox,NULL,0,box,6,&value,&diagnostic)&&value.type==NativeScriptType_Bool);
}
static void frontend_vm(void)
{
    const char* text="state bool wait\nfunc OwnField():\n    state.wait = true\nfunc Land():\n    state.freefall(1, 0, true, 0.75, 23.5)\n    state.landing_special(false, 15.0)\n";
    NativeScriptSourceNode syntax[128];NativeScriptSourceTree tree={text,(uint32_t)strlen(text),syntax,0,128};CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
    NativeScriptIRNode nodes[512];uint32_t args[256];NativeScriptIRArena arena={text,tree.source_bytes,nodes,0,512,args,0,256};
    NativeScriptFunctionDeclaration funcs[8];NativeScriptParameterDeclaration params[8];NativeScriptValueDeclaration fields[8];
    NativeScriptProgramIndex index={.tree=&tree,.expressions=&arena,.functions=funcs,.function_capacity=8,.parameters=params,.parameter_capacity=8,.values=fields,.value_capacity=8};CHECK(nativeScriptProgram_Index(&index,&diagnostic));
    NativeScriptLocal locals[32];NativeScriptBindingResolverContext binding={NativeScriptBindings_Fighter};NativeScriptProgramCompiler compiler={.index=&index,.native_bindings=nativeScriptBindings_Resolve,.native_binding_context=&binding,.locals=locals,.local_capacity=32};
    NativeScriptStatementCompiler body;CHECK(nativeScriptProgram_CompileBody(&compiler,0,&body,&diagnostic));
    uint32_t assignment=tree.nodes[funcs[0].source_node].first_child;CHECK(nodes[tree.nodes[assignment].target_expression].semantic==NATIVE_SCRIPT_STATE_SEMANTIC);
    CHECK(nativeScriptProgram_CompileBody(&compiler,1,&body,&diagnostic));NativeScriptInstruction code[512];NativeScriptFunctionCode fn[2]={0};
    NativeScriptBytecodeBuilder builder={.instructions=code,.instruction_capacity=512,.functions=fn,.function_count=2,.describe_call=nativeScriptBindings_DescribeCall,.binding_context=&binding};
    for(uint32_t i=0;i<2;++i) {
        CHECK(nativeScriptProgram_CompileBody(&compiler,i,&body,&diagnostic));
        CHECK(nativeScriptBytecode_Compile(&builder,&body,funcs[i].source_node,i,0,&diagnostic));
    }
    NativeScriptBytecodeProgram program={text,tree.source_bytes,code,builder.instruction_count,fn,2,false}; { NativeScriptDiagnostic preparation={0}; if(!nativeScriptBytecode_Prepare(&program,&preparation)){fprintf(stderr,"bytecode preparation: %s\n",preparation.message);abort();} }NativeScriptValue registers[256];NativeScriptVMFrame frames[8];NativeScriptVM vm={.registers=registers,.register_capacity=256,.frames=frames,.frame_capacity=8};
    setup();NativeScriptValue result;CHECK(nativeScriptVM_Run(&vm,&program,&nativeScriptBindingHost,&context,1,NULL,0,&result)==NativeScriptVM_Ok&&actor.motion_id==43);
    setup();scriptStateFixture_RetireOnChange(true);result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_MAX};CHECK(nativeScriptVM_Run(&vm,&program,&nativeScriptBindingHost,&context,1,NULL,0,&result)==NativeScriptVM_SourceRetired);CHECK(script_state_changes==1&&result.type==NativeScriptType_U64&&vm.depth==0&&vm.register_top==0);
}
static void motion_transitions(void)
{
    setup();
    NativeScriptValue values[3]={{.type=NativeScriptType_I64,.as.i64=341},
        {.type=NativeScriptType_F64,.as.f64=7.123456789},{.type=NativeScriptType_F32,.as.f32=0.5f}};
    NativeScriptValue result={0};
    CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_StateEnterSpeed,NULL,0,values,3,&result,&diagnostic));
    CHECK(actor.motion_id==341 && actor.cur_anim_frame==(float)7.123456789 && actor.frame_speed_mul==0.5f);
    values[0]=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000155)};
    unsigned before=script_state_changes;
    CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_StateEnter,NULL,0,values,1,&result,&diagnostic));CHECK(script_state_changes==before);
    values[0]=(NativeScriptValue){.type=NativeScriptType_I64,.as.i64=-1};
    CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_StateEnter,NULL,0,values,1,&result,&diagnostic));CHECK(script_state_changes==before);
    values[0]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=349};
    CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_StateEnter,NULL,0,values,1,&result,&diagnostic));CHECK(script_state_changes==before);
    values[0]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=342};
    values[1]=(NativeScriptValue){.type=NativeScriptType_U32,.as.u64=UINT32_C(0x80000000)};
    actor.cur_anim_frame=17.25f;
    CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_StateSwitchToAir,NULL,0,values,2,&result,&diagnostic));
    CHECK(actor.motion_id==342 && actor.ground_or_air==GA_Air && actor.cur_anim_frame==17.25f);
    values[0].as.i64=341;
    CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_StateSwitchToGround,NULL,0,values,2,&result,&diagnostic));
    CHECK(actor.motion_id==341 && actor.ground_or_air==GA_Ground && actor.cur_anim_frame==17.25f);
    CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_StateBegin,NULL,0,values,1,&result,&diagnostic));
    CHECK(script_kinetic_stage_count==4 && actor.cur_anim_frame==1.25f);
    setup();scriptKineticsFixture_RetireAt(2,0);
    result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
    CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_StateBegin,NULL,0,values,1,&result,&diagnostic));
    CHECK(script_kinetic_stage_count==2 && result.type==NativeScriptType_U64 && result.as.u64==UINT64_MAX);
    setup();scriptStateFixture_RetireOnChange(true);
    CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_StateBegin,NULL,0,values,1,&result,&diagnostic));CHECK(script_kinetic_stage_count==0);
}

#include "box_cases.h"
int main(void){motion_transitions();state_cases();queries();frontend_vm();box_cases();box_frontend_vm();printf("{\"checks\":%u,\"transitions\":%u,\"box_comparisons\":%u,\"status\":\"PASS owned general state/collision/native body contracts\"}\n",checks,transitions,box_comparisons);return 0;}
