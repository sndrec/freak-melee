// Controlled transition inputs; production runtime/heap/scene kernels unchanged.
#pragma clang section data = ".data"
#include <melee/ft/native_fighter_runtime.c>
#pragma clang section data = ".mstate$m"
#include <melee/lb/lbheap.c>
#include <melee/it/native_article_model_scene.c>
#pragma clang section data = ".data"
#include <melee/ft/native_fighter_project.h>
#include <melee/ft/native_source_motion_policy.h>
#include <melee/ft/native_script_binding_item_animation.h>
#include <melee/ft/native_script_binding_item_model_resolve.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/it/native_article_animation_live.h>
#include <melee/gm/native_rollback_regions.h>
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/native_animation_pool.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/id.h>
#include <sysdolphin/baselib/list.h>
#include <melee/ft/native_script_binding_common_properties.h>
#include <melee/lb/lbvector.h>
#include <melee/gr/stage.h>
#include <melee/lb/lbaudio_ax.h>
#include <melee/mp/forward.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <sysdolphin/baselib/controller.h>
#include <math.h>
#include <stdio.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

enum { HEAP_BYTES=64*1024*1024, OBS_NODES=64, OBS_TRACKS=64 };
static unsigned checks,cases,comparisons,frames,restores;
static unsigned char* heap_bytes;
static NativeSourcePackage candidate;
static NativeFighterProject project;
static NativeScriptDiagnostic diagnostic;
static const char* declaration;
static bool primary_only;
static Item* data;
static Item_GObj* objects[3];
static NativeRollbackRegionSet regions;
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"animation VM %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)


static void preserve_fighter(void* unused) { (void)unused; }
static bool smash_cstick,smash_item_throw,smash_extra_throw;
static unsigned smash_defaults,smash_throws,smash_inputs;
static Fighter* smash_fighter;
static Fighter_GObj* smash_object;
static HSD_GObj smash_held_item;
static bool fixtureCStick(Fighter* fighter){CHECK(fighter==smash_fighter);return smash_cstick;}
static bool fixtureItemThrowable(Fighter* fighter){CHECK(fighter==smash_fighter);return smash_item_throw;}
static bool fixtureExtraThrow(Fighter* fighter){CHECK(fighter==smash_fighter);return smash_extra_throw;}
static void fixtureUpDefault(Fighter_GObj* object){CHECK(object==smash_object);++smash_defaults;}
static void fixtureDownDefault(Fighter_GObj* object){CHECK(object==smash_object);++smash_defaults;}
static void fixtureNessUp(Fighter_GObj* object){(void)object;CHECK(false);}
static void fixtureNessDown(Fighter_GObj* object){(void)object;CHECK(false);}
static void fixtureItemThrow(Fighter_GObj* object,FtMotionId motion)
{CHECK(object==smash_object);CHECK(motion==ftCo_MS_LightThrowHi4||motion==ftCo_MS_LightThrowLw4);++smash_throws;}
#include "smash_inputs.h"
static void smash_dispatch_cases(Fighter_GObj* object,Fighter* fighter,bool owned)
{
    static struct ftCommonData common;struct ftCommonData* saved=p_ftCommonData;
    common=(struct ftCommonData){.xCC=.5f,.xD0=4,.xD4=-.5f,.xD8=4};p_ftCommonData=&common;
    smash_fighter=fighter;smash_object=object;Fighter before=*fighter;
    for(unsigned route=0;route<3;++route)for(unsigned mode=0;mode<6;++mode)for(unsigned held=0;held<3;++held){
        *fighter=before;fighter->item_gobj=held?&smash_held_item:NULL;
        fighter->input.pressed_buttons=(mode==1||mode==2)?HSD_PAD_A:mode==5?HSD_PAD_B:0;
        fighter->input.lstick[0].y=route==2?-.75f:.75f;
        fighter->input.held_buttons[0]=mode==4?HSD_PAD_PROFILE_CSTICK_TILT:0;
        fighter->x671_timer_lstick_tilt_y=mode==2?8:0;
        smash_cstick=mode==3||mode==4;smash_item_throw=held==1;smash_extra_throw=held==2;
        smash_defaults=smash_throws=0;
        bool selected=mode==1||(mode==2&&route==1)||mode==3;
        bool scripted=selected&&!held&&owned&&!primary_only;
        Fighter expected=*fighter;
        if(scripted){expected.self_vel.x=12.5f;expected.self_vel.y=route==2?-8.5f:8.5f;}
        bool result=route==0?fixtureUpftCo_AttackHi4_CheckInput(object):route==1?fixtureUpftCo_AttackHi4_CheckInputNoD0(object):fixtureDownftCo_AttackLw4_CheckInput(object);
        CHECK(result==selected);CHECK(smash_throws==(selected&&held?1u:0u));
        CHECK(smash_defaults==(selected&&!held&&!scripted?1u:0u));
        CHECK(!memcmp(fighter,&expected,sizeof(*fighter)));++smash_inputs;
        if(owned)CHECK(nativeScriptFighter_Status(object)==NativeScriptVM_Ok);
    }
    *fighter=before;p_ftCommonData=saved;++cases;
    printf("{\"observation\":\"smash_dispatch\",\"inputs\":54,\"owned\":%s,\"has_hooks\":%s}\n",owned?"true":"false",owned&&!primary_only?"true":"false");
}
static void vector_collision_cases(NativeScriptRuntimeContext* context,CollData* collision)
{
    CollData saved=*collision;
    static const uint32_t normals[3][3]={
        {NativeScriptBinding_CeilingNormalX,NativeScriptBinding_CeilingNormalY,NativeScriptBinding_CeilingNormalZ},
        {NativeScriptBinding_WallLeftNormalX,NativeScriptBinding_WallLeftNormalY,NativeScriptBinding_WallLeftNormalZ},
        {NativeScriptBinding_WallRightNormalX,NativeScriptBinding_WallRightNormalY,NativeScriptBinding_WallRightNormalZ}
    };
    Vec3* vectors[3]={&collision->ceiling.normal,&collision->left_facing_wall.normal,&collision->right_facing_wall.normal};
    static const uint32_t bits[]={0,0x80000000u,0x3f400000u,0xc0600000u,0x7fc01234u,0x7f800000u};
    for(unsigned trial=0;trial<6;++trial)for(unsigned surface=0;surface<3;++surface){
        float* components[3]={&vectors[surface]->x,&vectors[surface]->y,&vectors[surface]->z};
        for(unsigned axis=0;axis<3;++axis){
            uint32_t expected=bits[(trial+surface+axis)%6];memcpy(components[axis],&expected,sizeof(expected));
            NativeScriptValue observed={0};uint32_t actual=0;
            CHECK(nativeScriptBindings_Read(context,normals[surface][axis],NULL,0,&observed,&diagnostic));
            memcpy(&actual,&observed.as.f32,sizeof(actual));
            CHECK(observed.type==NativeScriptType_F32&&actual==expected);
        }
    }
    *collision=saved;
    for(unsigned trial=0;trial<24;++trial)for(unsigned axis=1;axis<=4;axis*=2){
        Vec3 expected={(float)trial-12,3.25f,trial%2?-2.0f:0.0f};
        float angle=((float)trial-12)*0.1875f;
        NativeScriptValue args[8]={
            {.type=NativeScriptType_F32,.as.f32=expected.x}, {.type=NativeScriptType_F32,.as.f32=expected.y},
            {.type=NativeScriptType_F32,.as.f32=expected.z}, {.type=NativeScriptType_I32,.as.i64=axis},
            {.type=NativeScriptType_F32,.as.f32=angle}, {.type=NativeScriptType_F32,.as.f32=123},
            {.type=NativeScriptType_F32,.as.f32=456}, {.type=NativeScriptType_F32,.as.f32=789}
        },observed;
        lbVector_Rotate(&expected,(int)axis,angle);
        CHECK(nativeScriptBindings_Invoke(context,NativeScriptBinding_VectorsRotate,NULL,0,args,8,&observed,&diagnostic));
        Vec3 actual={args[5].as.f32,args[6].as.f32,args[7].as.f32};
        CHECK(observed.type==NativeScriptType_Void&&!memcmp(&actual,&expected,sizeof(expected)));
    }
    for(unsigned invalid=0;invalid<3;++invalid){
        NativeScriptValue args[8],original[8],out={.type=NativeScriptType_I32,.as.i64=913},original_out=out;
        for(unsigned i=0;i<8;++i)args[i]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=(float)i};
        args[3]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=invalid==0?0:invalid==1?3:8};
        memcpy(original,args,sizeof(args));
        CHECK(!nativeScriptBindings_Invoke(context,NativeScriptBinding_VectorsRotate,NULL,0,args,8,&out,&diagnostic));
        CHECK(!memcmp(args,original,sizeof(args))&&!memcmp(&out,&original_out,sizeof(out)));
    }
    printf("{\"observation\":\"vector_collision\",\"source_class\":%u,\"normal_reads\":54,\"native_rotation_comparisons\":72,\"invalid_axes\":3}\n",context->source_class);++cases;
}

static void common_fighter_cases(bool owned)
{
    static Fighter fighter,expected;
    Fighter_GObj* object=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(object);
    fighter=(Fighter){.gobj=object,.kind=owned?Ft_Kind_CustomFirst:Ft_Kind_Mario,.behavior_kind=owned?Ft_Kind_CustomFirst:Ft_Kind_Mario,
        .x8_spawnNum=73,.ft_data=owned?&candidate.assets.character.data:NULL};
    GObj_InitUserData(object,4,preserve_fighter,&fighter);
    if(owned)CHECK(nativeScriptFighter_Attach(object));
    NativeScriptRuntimeContext context={.source=object,.source_class=NativeCallbackSource_Fighter,
        .generation=73,.scene_epoch=nativeGObjPool.lifetime};
    static const struct {uint32_t semantic;size_t offset;NativeScriptIRType type;} fields[]={
        {NativeScriptBinding_SmashChargeMode,offsetof(SmashAttr,state),NativeScriptType_I32},
        {NativeScriptBinding_SmashChargeFrames,offsetof(SmashAttr,x2118_frames),NativeScriptType_F32},
        {NativeScriptBinding_SmashChargeLimit,offsetof(SmashAttr,x211C_holdFrame),NativeScriptType_F32},
        {NativeScriptBinding_SmashSavedAnimationRate,offsetof(SmashAttr,x2124_frameSpeedMul),NativeScriptType_F32},
        {NativeScriptBinding_SmashChargeColor,offsetof(SmashAttr,x2128),NativeScriptType_I32},
        {NativeScriptBinding_SmashShakeIndex,offsetof(SmashAttr,x212C),NativeScriptType_I32},
        {NativeScriptBinding_SmashShakePeriod,offsetof(SmashAttr,x212D),NativeScriptType_I32}
    };
    for(unsigned iteration=0;iteration<16;++iteration)for(unsigned i=0;i<sizeof(fields)/sizeof(*fields);++i){
        NativeScriptValue value={.type=fields[i].type},observed;
        if(value.type==NativeScriptType_F32)value.as.f32=(float)iteration*1.25f+0.125f;
        else if(i==0)value.as.i64=(int64_t)iteration%5;
        else if(i>=5)value.as.i64=(int64_t)iteration*17;
        else value.as.i64=(int64_t)iteration-8;
        expected=fighter;
        void* target=(uint8_t*)&expected.smash_attrs+fields[i].offset;
        if(value.type==NativeScriptType_F32)memcpy(target,&value.as.f32,sizeof(float));
        else if(i>=5)*(uint8_t*)target=(uint8_t)value.as.i64;
        else {int32_t integer=(int32_t)value.as.i64;memcpy(target,&integer,sizeof(integer));}
        CHECK(nativeScriptBindings_Write(&context,fields[i].semantic,NULL,0,&value,&diagnostic));
        CHECK(!memcmp(&fighter,&expected,sizeof(fighter)));
        CHECK(nativeScriptBindings_Read(&context,fields[i].semantic,NULL,0,&observed,&diagnostic));
        CHECK(observed.type==value.type);
        if(value.type==NativeScriptType_F32?observed.as.f32!=value.as.f32:observed.as.i64!=value.as.i64)
            fprintf(stderr,"Smash read mismatch: owned=%u iteration=%u field=%u expected bits=%llu observed bits=%llu\n",owned,iteration,i,(unsigned long long)value.as.u64,(unsigned long long)observed.as.u64);
        CHECK(value.type==NativeScriptType_F32?observed.as.f32==value.as.f32:observed.as.i64==value.as.i64);
    }
    for(unsigned i=0;i<64;++i){
        uint32_t flags=(i&1?LINE_FLAG_PLATFORM:0)|(i&2?0x80000000u:0)|(i<<4);
        fighter.coll_data.floor.flags=flags;expected=fighter;NativeScriptValue observed;
        CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_FloorIsPlatform,NULL,0,&observed,&diagnostic));
        CHECK(observed.type==NativeScriptType_Bool&&observed.as.u64==((flags&LINE_FLAG_PLATFORM)!=0));
        CHECK(!memcmp(&fighter,&expected,sizeof(fighter)));
    }
    for(unsigned i=0;i<32;++i){
        fighter.self_vel=(Vec3){(float)i-16,2.5f,0.75f};expected=fighter;
        Vec3 normal=i%3==0?(Vec3){0,0,0}:i%3==1?(Vec3){0.6f,0.8f,0}:(Vec3){2,-1,3};
        lbVector_Mirror(&expected.self_vel,&normal);
        NativeScriptValue inputs[3]={{.type=NativeScriptType_F32,.as.f32=normal.x},
            {.type=NativeScriptType_F32,.as.f32=normal.y},{.type=NativeScriptType_F32,.as.f32=normal.z}},observed;
        CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_PhysicsMirrorVelocity,NULL,0,inputs,3,&observed,&diagnostic));
        CHECK(!memcmp(&fighter,&expected,sizeof(fighter)));
    }
    vector_collision_cases(&context,&fighter.coll_data);
    {NativeScriptValue stage;expected=fighter;
        CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_StageInternalKind,NULL,0,&stage,&diagnostic));
        CHECK(stage.type==NativeScriptType_I32&&stage.as.i64==(int32_t)Stage_80225194());
        CHECK(!memcmp(&fighter,&expected,sizeof(fighter)));
    }
    for(unsigned trial=0;trial<32;++trial){
        fighter.self_vel=(Vec3){(float)trial-16,2.5f,0.75f};expected=fighter;
        Vec3 axis=trial%4==0?(Vec3){1,0,0}:trial%4==1?(Vec3){0,1,0}:trial%4==2?(Vec3){0,0,1}:(Vec3){0.6f,0.8f,0};
        float angle=((float)trial-16)*0.125f;
        lbVector_RotateAboutUnitAxis(&expected.self_vel,&axis,angle);
        NativeScriptValue args[4]={{.type=NativeScriptType_F32,.as.f32=axis.x},
            {.type=NativeScriptType_F32,.as.f32=axis.y},{.type=NativeScriptType_F32,.as.f32=axis.z},
            {.type=NativeScriptType_F32,.as.f32=angle}},observed;
        CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_PhysicsRotateVelocityUnitAxis,NULL,0,args,4,&observed,&diagnostic));
        CHECK(!memcmp(&fighter,&expected,sizeof(fighter)));
    }
    NativeScriptValue bad={.type=NativeScriptType_I32,.as.i64=256},observed;
    expected=fighter;
    CHECK(!nativeScriptBindings_Write(&context,NativeScriptBinding_SmashShakeIndex,NULL,0,&bad,&diagnostic));
    CHECK(!memcmp(&fighter,&expected,sizeof(fighter)));
    bad.as.i64=-1;CHECK(!nativeScriptBindings_Write(&context,NativeScriptBinding_SmashChargeMode,NULL,0,&bad,&diagnostic));
    CHECK(!memcmp(&fighter,&expected,sizeof(fighter)));
    bad.as.i64=5;CHECK(!nativeScriptBindings_Write(&context,NativeScriptBinding_SmashChargeMode,NULL,0,&bad,&diagnostic));
    CHECK(!memcmp(&fighter,&expected,sizeof(fighter)));
    struct Fighter_ShakeTable_t table={.x4=17};struct Fighter_ShakeTable_t* saved=Fighter_SmashChargeShakeTable;
    Fighter_SmashChargeShakeTable=&table;
    CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_SmashShakeTableCount,NULL,0,&observed,&diagnostic));
    CHECK(observed.type==NativeScriptType_I32&&observed.as.i64==17);
    Fighter_SmashChargeShakeTable=NULL;
    CHECK(!nativeScriptBindings_Read(&context,NativeScriptBinding_SmashShakeTableCount,NULL,0,&observed,&diagnostic));
    Fighter_SmashChargeShakeTable=saved;
    if(owned){
        unsigned function=0;
        const NativeScriptProgramIndex* index=&candidate.compiler.bank.declarations;
        for(;function<index->function_count;++function)if(nativeScriptSource_SpanEquals(index->tree,index->functions[function].name,"configure_charge"))break;
        CHECK(function<index->function_count);
        expected=fighter;
        expected.smash_attrs.state=(SmashState)4;expected.smash_attrs.x2118_frames=1.125f;
        expected.smash_attrs.x211C_holdFrame=42.0f;expected.smash_attrs.x2124_frameSpeedMul=0.5f;
        expected.smash_attrs.x2128=17;expected.smash_attrs.x212C=3;expected.smash_attrs.x212D=8;
        Vec3 normal={0.6f,0.8f,0};lbVector_Mirror(&expected.self_vel,&normal);
        Vec3 axis={0,0,1};lbVector_RotateAboutUnitAxis(&expected.self_vel,&axis,0.375f);
        lbVector_Rotate(&expected.self_vel,1,-0.625f);
        expected.self_vel.x+=fighter.coll_data.ceiling.normal.z;
        expected.self_vel.x+=1.75f;
        expected.self_vel.y+=fighter.coll_data.left_facing_wall.normal.z;
        expected.self_vel.z+=fighter.coll_data.right_facing_wall.normal.z;
        CHECK(nativeScriptFighter_Helper(object,73,function,NULL,0,&observed,&diagnostic)==NativeScriptVM_Ok);
        CHECK(observed.type==NativeScriptType_Void&&!memcmp(&fighter,&expected,sizeof(fighter)));
    }
    smash_dispatch_cases(object,&fighter,owned);
    if(owned)nativeScriptFighter_Detach(object);
    HSD_GObjFree(object);
    CHECK(!nativeScriptBindingCommonProperties_Read(&context,NativeScriptBinding_SmashChargeFrames,&observed,&diagnostic));
    CHECK(!nativeScriptBindingCommonProperties_Write(&context,NativeScriptBinding_SmashChargeFrames,&bad,&diagnostic));
    printf("{\"observation\":\"common_fighter_fields\",\"field_writes\":112,\"platform_queries\":64,\"native_mirror_comparisons\":32,\"owned\":%s}\n",owned?"true":"false");++cases;
}

static void destroy_item(void* opaque)
{
    Item* item=opaque;
    for(unsigned slot=1;slot<=3;++slot)nativeArticleModel_Destroy(item->entity,slot);
    nativeScriptItem_Detach(item->entity);
}
static void stage_candidate(const char* path)
{
    CHECK(nativeFighterProject_Open(path,&project));
    candidate.address=&candidate;candidate.target=Ft_Kind_CustomFirst;
    candidate.owned_input.assets=(NativeSourceAssetInput){project.fighter_dat,project.fighter_size,
        project.animation_dat,project.animation_size,project.costume_dat,project.costume_size};
    CHECK(nativeSourceAssets_Import(&project.facts,&candidate.owned_input.assets,Ft_Kind_CustomFirst,&candidate.assets,&diagnostic));
    CHECK(nativeSourceBank_Allocate(&candidate.compiler,&diagnostic));
    static char source[16384];
    snprintf(source,sizeof(source),
        "fighter:\n    behavior independent\n"
        "action SmashHooks:\n    on up_smash_enter:\n        velocity.x = 12.5\n        velocity.y = 8.5\n"
        "    on down_smash_enter:\n        velocity.x = 12.5\n        velocity.y = -8.5\n"
        "func configure_charge():\n"
        "    smash.charge_mode = Smash.manual_charge\n    smash.charge_frames = 1.125\n"
        "    smash.charge_limit = 42.0\n    smash.saved_animation_rate = 0.5\n"
        "    smash.charge_color = 17\n    smash.shake_index = 3\n    smash.shake_period = 8\n"
        "    physics.mirror_velocity(0.6, 0.8, 0.0)\n"
        "    physics.rotate_velocity_unit_axis(0.0, 0.0, 1.0, 0.375)\n"
        "    let float rx = 0\n    let float ry = 0\n    let float rz = 0\n"
        "    vectors.rotate(velocity.x, velocity.y, velocity.z, 1, -0.625, rx, ry, rz)\n"
        "    velocity.x = rx + ceiling.normal.z + program.entity_params.%s.probe\n    velocity.y = ry + wall.left.normal.z\n"
        "    velocity.z = rz + wall.right.normal.z\n"
        "article %s states 4:\n"
        "    parameter float probe = 1.75\n"
        "    model_root TrailFirst 68\n    model_root TrailSecond 72\n"
        "    model_animation FlightFirst joint 76 material None shape None\n    model_animation FlightSecond 88\n"
        "    state int visits\n    state int op\n    state bool added\n"
        "    state 0 anim -1:\n        on physics:\n"
        "            item.state.visits = item.state.visits + 1\n"
        "            if item.state.op == 0:\n"
        "                item.state.added = item.load_model(1, TrailFirst)\n"
        "                item.state.added = item.load_model(2, TrailSecond)\n"
        "            if item.state.op == 1:\n"
        "                item.state.added = item.add_model_animation(0, \"0\", FlightFirst)\n"
        "                item.state.added = item.add_model_animation(1, \"\", FlightFirst)\n"
        "                item.state.added = item.add_model_animation(2, FlightSecond)\n"
        "            if item.state.op == 2:\n"
        "                item.request_model_animation(0, \"0\", 0.0)\n"
        "                item.request_model_animation(1, \"\", 0.0)\n"
        "                item.request_model_animation(2, 0.0)\n"
        "            if item.state.op == 3:\n"
        "                item.advance_model_animation(0, \"0\")\n"
        "                item.advance_model_animation(1, \"\")\n"
        "                item.advance_model_animation(2)\n"
        "            if item.state.op == 4:\n"
        "                item.set_model_animation_rate(0, \"\", 2.5)\n"
        "                item.set_model_animation_rate(1, \"0\", 3.0)\n"
        "                item.set_model_animation_rate(2, 4.0)\n"
        "            if item.state.op == 5:\n"
        "                item.remove_model_animation(0, \"0\")\n"
        "                item.remove_model_animation(1, \"\")\n"
        "                item.remove_model_animation(2)\n"
        "            if item.state.op == 6:\n"
        "                item.state.added = item.add_model_animation(3, FlightFirst)\n"
        "                item.request_model_animation(3, 1.0)\n"
        "                item.advance_model_animation(3)\n"
        "                item.remove_model_animation(3)\n"
        "                item.set_model_animation_rate(3, 1.0)\n"
        "                item.state.added = item.add_model_animation(1, \"0/255\", FlightFirst)\n"
        "                item.request_model_animation(1, \"0/255\", 1.0)\n"
        "                item.advance_model_animation(1, \"0/255\")\n"
        "                item.remove_model_animation(1, \"0/255\")\n"
        "                item.set_model_animation_rate(1, \"0/255\", 1.0)\n"
        "            if item.state.op == 7:\n"
        "                item.destroy_model(1)\n"
        "                item.state.added = item.load_model(1, TrailSecond)\n"
        "                item.state.added = item.add_model_animation(1, \"\", FlightSecond)\n"
        "                item.request_model_animation(1, \"\", 2.0)\n"
        "            if item.state.op == 8:\n"
        "                item.remove_model_animation(0)\n"
        "                item.destroy_model(1)\n                item.destroy_model(2)\n"
        "            if item.state.op == 9:\n"
        "                item.state.added = item.add_model_animation(1, \"\", FlightFirst)\n"
        "            if item.state.op == 10:\n"
        "                item.state.added = item.add_model_animation(0, \"\", FlightFirst)\n"
        "    state 1 anim -1:\n        on physics:\n            pass\n"
        "    state 2 anim -1:\n        on physics:\n            pass\n"
        "    state 3 anim -1:\n        on physics:\n            pass\n",
        declaration,declaration);
    if(primary_only)snprintf(source,sizeof(source),
        "fighter:\n    behavior independent\n"
        "func configure_charge():\n"
        "    smash.charge_mode = Smash.manual_charge\n    smash.charge_frames = 1.125\n"
        "    smash.charge_limit = 42.0\n    smash.saved_animation_rate = 0.5\n"
        "    smash.charge_color = 17\n    smash.shake_index = 3\n    smash.shake_period = 8\n"
        "    physics.mirror_velocity(0.6, 0.8, 0.0)\n"
        "    physics.rotate_velocity_unit_axis(0.0, 0.0, 1.0, 0.375)\n"
        "    let float rx = 0\n    let float ry = 0\n    let float rz = 0\n"
        "    vectors.rotate(velocity.x, velocity.y, velocity.z, 1, -0.625, rx, ry, rz)\n"
        "    velocity.x = rx + ceiling.normal.z + program.entity_params.%s.probe\n    velocity.y = ry + wall.left.normal.z\n"
        "    velocity.z = rz + wall.right.normal.z\n"
        "article %s states 4:\n"
        "    parameter float probe = 1.75\n"
        "    state int visits\n    state int op\n"
        "    state 0 anim -1:\n        on physics:\n"
        "            item.state.visits = item.state.visits + 1\n"
        "            if item.state.op == 2:\n                item.request_model_animation(0, \"0\", 0.0)\n"
        "            if item.state.op == 3:\n                item.advance_model_animation(0, \"0\")\n"
        "            if item.state.op == 4:\n                item.set_model_animation_rate(0, 2.5)\n"
        "            if item.state.op == 5:\n                item.remove_model_animation(0)\n"
        "    state 1 anim -1:\n        on physics:\n            pass\n"
        "    state 2 anim -1:\n        on physics:\n            pass\n"
        "    state 3 anim -1:\n        on physics:\n            pass\n",declaration,declaration);
    NativeScriptSourceTree* tree=&candidate.compiler.bank.tree;
    tree->source=source;tree->source_bytes=(uint32_t)strlen(source);
    CHECK(nativeScriptSource_Parse(tree,&diagnostic));
    CHECK(nativeSourceMotionPolicy_Prepare(tree,ftData_MotionStateList,&candidate.assets,&diagnostic));
    CHECK(nativeSourceEntityAssets_Prepare(&candidate.entities,&candidate.assets,&candidate.resources,tree,&diagnostic));
    candidate.metadata=(NativeScriptItemMetadataContext){.motions=&candidate.assets.facts.moves,
        .assets=candidate.entities.rows,.asset_count=candidate.entities.count,.source_facts=&candidate.assets.facts,.source_assets=&candidate.assets};
    NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),
        .metadata=nativeScriptItemMetadata_Validate,.metadata_context=&candidate.metadata,.motion_layout=&candidate.assets.facts.moves};
    const NativeScriptCompileBank* accepted=NULL;
    CHECK(nativeScriptCompile_Build(&candidate.compiler.bank,&request,&accepted,&diagnostic));
    CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,accepted,&candidate.assets.facts.moves,&candidate.metadata,&candidate.program,&diagnostic));
    CHECK(nativeSourceItemVisuals_Prepare(&candidate.visuals,&candidate.program.items,&diagnostic));
    candidate.parameters.items=&candidate.metadata;candidate.parameters.fighter.bytes=project.fighter_dat;
    candidate.parameters.fighter.fighter_root=candidate.assets.facts.fighter_root;
    candidate.input_memory=malloc(1);CHECK(candidate.input_memory);candidate.ready=true;
    CHECK(nativeSourcePackage_Pin(&candidate));CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));
    nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&candidate.program);
    slots[0]=(NativeFighterSlot){.source=&candidate,.fighter_kind=Ft_Kind_CustomFirst,.active=true};
}
static Item_GObj* create_item(unsigned at)
{
    const NativeScriptItemProgram* program=nativeScriptItem_Program(Ft_Kind_CustomFirst,candidate.program.items.programs[0].owner);CHECK(program);
    Item_GObj* object=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(object);
    data[at]=(Item){.entity=object,.kind=program->kind,.x1C=(s32)(at+1),.msid=-1,
        .native_article_author_kind=program->author_kind,.native_script_owner=program->owner,
        .xC4_article_data=(Article*)program->article,.xCC_item_attr=program->article->x0_common_attr};
    GObj_InitUserData(object,6,destroy_item,&data[at]);CHECK(nativeScriptItem_Attach(object,program));
    CHECK(nativeScriptItem_SelectState(object,0));
    HSD_JObj* primary=HSD_JObjLoadJoint(program->article->x10_modelDesc->x0_joint);CHECK(primary&&primary->child);
    HSD_GObjObject_80390A70(object,HSD_GObj_JObjKind,primary);return object;
}
static NativeScriptValue state(Item_GObj* object,const char* name,const NativeScriptValue* write)
{
    Item* item=object->user_data;NativeScriptReceiverView view;CHECK(nativeScriptItem_View(object,(uint32_t)item->x1C,&view));
    for(uint32_t n=0;n<view.program->value_count;++n)if(view.program->values[n].owner==view.storage->owner&&
        nativeScriptSource_SpanEquals(view.program->tree,view.program->values[n].name,name)){
        uint32_t semantic=NATIVE_SCRIPT_STATE_SEMANTIC|n;
        if(write)CHECK(nativeScriptStorage_Write(view.storage,semantic,NULL,0,write,&diagnostic));
        NativeScriptValue output;CHECK(nativeScriptStorage_Read(view.storage,semantic,NULL,0,&output,&diagnostic));return output;
    }
    CHECK(false);return (NativeScriptValue){0};
}
static void run(Item_GObj* object,int operation)
{
    NativeScriptValue value={.type=NativeScriptType_I32,.as.i64=operation};state(object,"op",&value);
    nativeScriptItem_BeginTick(object);value=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=73};
    memset(&diagnostic,0,sizeof(diagnostic));
    NativeScriptVMStatus status=nativeScriptItem_RunPhase(object,NativeScriptItemPhase_Physics,&value);
    if(status!=NativeScriptVM_Ok)fprintf(stderr,"operation %d status %d\n",operation,status);
    CHECK(status==NativeScriptVM_Ok);
    CHECK(value.type==NativeScriptType_Void);++cases;
}
typedef struct NodeObservation {
    uint32_t flags;Vec3 translation,scale;Quaternion rotation;
    HSD_AObj controller;HSD_FObj tracks[OBS_TRACKS];uint32_t track_count;bool animated;
} NodeObservation;
typedef struct GraphObservation { NodeObservation nodes[OBS_NODES];uint32_t count; } GraphObservation;
static void graph_observe(HSD_JObj* joint,GraphObservation* result)
{
    for(;joint;joint=joint->next){
        CHECK(result->count<OBS_NODES);NodeObservation* node=&result->nodes[result->count++];
        node->flags=joint->flags;node->translation=joint->translate;node->scale=joint->scale;node->rotation=joint->rotate;
        if(joint->aobj){
            node->animated=true;node->controller=*joint->aobj;node->controller.fobj=NULL;node->controller.hsd_obj=NULL;
            for(HSD_FObj* track=joint->aobj->fobj;track;track=track->next){
                CHECK(node->track_count<OBS_TRACKS);node->tracks[node->track_count]=*track;
                node->tracks[node->track_count++].next=NULL;
            }
        }
        if(joint->child&&!(joint->flags&JOBJ_INSTANCE))graph_observe(joint->child,result);
    }
}
static void same_graph(HSD_JObj* actual,HSD_JObj* expected)
{
    static GraphObservation left,right;memset(&left,0,sizeof(left));memset(&right,0,sizeof(right));
    graph_observe(actual,&left);graph_observe(expected,&right);CHECK(!memcmp(&left,&right,sizeof(left)));++comparisons;
}
static const NativeScriptItemProgram* program(void)
{return nativeScriptItem_Program(Ft_Kind_CustomFirst,candidate.program.items.programs[0].owner);}
static void add_native(HSD_JObj* graph,unsigned ordinal)
{
    const NativeScriptItemModelAnimationAsset* bundle=&program()->model_animations[ordinal];
    HSD_JObjAddAnimAll(graph,(HSD_AnimJoint*)bundle->anim,(HSD_MatAnimJoint*)bundle->matanim,(HSD_ShapeAnimJoint*)bundle->shapeanim);
}
static void reference(unsigned operation)
{
    HSD_JObj* primary=objects[1]->hsd_obj;
    if(primary_only){
        switch(operation){
        case 2:HSD_JObjReqAnimAll(primary->child,0);break;
        case 3:HSD_JObjAnimAll(primary->child);break;
        case 4:lb_8000BA0C(primary,2.5f);break;
        case 5:HSD_JObjRemoveAnimAll(primary);break;
        default:CHECK(false);
        }
        return;
    }
    switch(operation){
    case 0:CHECK(nativeArticleModelScene_Load(objects[1],1,0));CHECK(nativeArticleModelScene_Load(objects[1],2,1));break;
    case 1:add_native(primary->child,0);add_native(data[1].native_models[0],0);add_native(data[1].native_models[1],1);break;
    case 2:HSD_JObjReqAnimAll(primary->child,0);HSD_JObjReqAnimAll(data[1].native_models[0],0);HSD_JObjReqAnimAll(data[1].native_models[1],0);break;
    case 3:HSD_JObjAnimAll(primary->child);HSD_JObjAnimAll(data[1].native_models[0]);HSD_JObjAnimAll(data[1].native_models[1]);break;
    case 4:lb_8000BA0C(primary,2.5f);lb_8000BA0C(data[1].native_models[0]->child,3);lb_8000BA0C(data[1].native_models[1],4);break;
    case 5:HSD_JObjRemoveAnimAll(primary->child);HSD_JObjRemoveAnimAll(data[1].native_models[0]);HSD_JObjRemoveAnimAll(data[1].native_models[1]);break;
    case 6:break;
    case 7:nativeArticleModel_Destroy(objects[1],1);CHECK(nativeArticleModelScene_Load(objects[1],1,1));add_native(data[1].native_models[0],1);HSD_JObjReqAnimAll(data[1].native_models[0],2);break;
    case 8:HSD_JObjRemoveAnimAll(primary);nativeArticleModel_Destroy(objects[1],1);nativeArticleModel_Destroy(objects[1],2);break;
    case 10:add_native(primary,0);break;
    default:CHECK(false);
    }
}
static void paired(unsigned operation)
{
    ssize_t free_before=OSCheckHeap(HSD_GetHeap());run(objects[0],(int)operation);reference(operation);
    same_graph(objects[0]->hsd_obj,objects[1]->hsd_obj);
    for(unsigned slot=0;slot<2;++slot)same_graph(data[0].native_models[slot],data[1].native_models[slot]);
    CHECK(OSCheckHeap(HSD_GetHeap())==free_before);
}
static const NativeScriptItemModelSelector* selector(uint32_t domain)
{
    const NativeScriptItemProgram* owner=program();
    for(uint32_t i=0;i<owner->model_selector_count;++i){const NativeScriptItemModelSelector* s=&owner->model_selectors[i];
        if((s->semantic&NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK)==domain&&s->model==1&&(!s->depth||domain==NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_RATE)&&s->argument_count==3)return s;
        if((s->semantic&NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK)==domain&&s->model==1&&!s->depth&&s->argument_count==2&&domain==NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_ADVANCE)return s;
    }
    CHECK(false);return NULL;
}
static bool invoke(uint32_t domain,float scalar,NativeScriptValue* output)
{
    const NativeScriptItemModelSelector* s=selector(domain);
    NativeScriptRuntimeContext context={.source=objects[0],.source_class=NativeCallbackSource_Item,
        .generation=(uint32_t)data[0].x1C,.scene_epoch=nativeGObjPool.lifetime};
    NativeScriptValue args[3]={{.type=NativeScriptType_I32,.as.i64=1},
        {.type=NativeScriptType_Text,.as.text={s->path_offset,s->path_length}},
        {.type=NativeScriptType_F32,.as.f32=scalar}};
    if(domain==NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_ADD)args[2]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=s->animation};
    return nativeScriptBindingItemAnimation_Invoke(&context,s->semantic,args,s->argument_count,output,&diagnostic);
}
static void reject_preserves(uint32_t domain,float scalar)
{
    static GraphObservation before,after;memset(&before,0,sizeof(before));memset(&after,0,sizeof(after));
    graph_observe(data[0].native_models[0],&before);
    NativeScriptValue value={.type=NativeScriptType_I32,.as.i64=73};
    CHECK(!invoke(domain,scalar,&value));CHECK(value.type==NativeScriptType_I32&&value.as.i64==73);
    graph_observe(data[0].native_models[0],&after);CHECK(!memcmp(&before,&after,sizeof(before)));++cases;
}
static void unsafe_cases(void)
{
    HSD_JObj* root=data[0].native_models[0];HSD_JObj* animated=root?root->child:NULL;
    CHECK(animated&&animated->aobj&&animated->aobj->fobj);
    const HSD_AObjDesc* old=animated->aobj->native_origin;HSD_AObjDesc foreign=*old;
    animated->aobj->native_origin=&foreign;reject_preserves(NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_ADVANCE,0);animated->aobj->native_origin=old;
    const HSD_FObjDesc* old_track=animated->aobj->fobj->native_origin;
    animated->aobj->fobj->native_origin=NULL;reject_preserves(NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_ADVANCE,0);animated->aobj->fobj->native_origin=old_track;
    HSD_SList callback={0};CHECK(!nativeAnimationGlobals.end_callbacks);
    nativeAnimationGlobals.end_callbacks=&callback;reject_preserves(NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_ADVANCE,0);nativeAnimationGlobals.end_callbacks=NULL;
    nativeAnimationGlobals.callback_depth=1;reject_preserves(NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_ADVANCE,0);nativeAnimationGlobals.callback_depth=0;
    reject_preserves(NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_REQUEST,INFINITY);
    reject_preserves(NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_RATE,NAN);
    uint32_t slot=nativeAObjPool.free_order[nativeAObjPool.free_count-1];uint32_t generation=nativeAObjPool.slots[slot].generation;
    nativeAObjPool.slots[slot].generation=UINT32_MAX;NativeScriptValue value={0};
    CHECK(invoke(NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_ADD,0,&value)&&value.type==NativeScriptType_Bool&&!value.as.u64);
    nativeAObjPool.slots[slot].generation=generation;++cases;
    static HSD_AObj* controllers[NATIVE_AOBJ_CAPACITY];unsigned count=0;
    while(nativeAObjPool.free_count)controllers[count++]=HSD_AObjAlloc();
    CHECK(invoke(NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_ADD,0,&value)&&value.type==NativeScriptType_Bool&&!value.as.u64);
    while(count)HSD_AObjFree(controllers[--count]);same_graph(root,data[1].native_models[0]);++cases;
    static HSD_FObj* tracks[NATIVE_FOBJ_CAPACITY];
    while(nativeFObjPool.free_count)tracks[count++]=HSD_FObjAlloc();
    CHECK(invoke(NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_ADD,0,&value)&&value.type==NativeScriptType_Bool&&!value.as.u64);
    while(count)HSD_FObjFree(tracks[--count]);same_graph(root,data[1].native_models[0]);++cases;
}
static void borrowed_vm_case(void)
{
    NativeArticleModelSceneSlot original;CHECK(nativeArticleModelScene_CaptureSlot(objects[0],1,&original));
    objects[2]=create_item(2);data[2].native_models[0]=data[0].native_models[0];
    NativeScriptValue operation={.type=NativeScriptType_I32,.as.i64=3};state(objects[2],"op",&operation);
    NativeScriptValue output={.type=NativeScriptType_I32,.as.i64=73};
    CHECK(nativeScriptItem_RunPhase(objects[2],NativeScriptItemPhase_Physics,&output)==NativeScriptVM_Error);
    CHECK(output.type==NativeScriptType_I32&&output.as.i64==73);
    CHECK(nativeScriptItem_Status(objects[2])==NativeScriptVM_Error);
    CHECK(nativeArticleModelScene_SlotCurrent(&original));
    data[2].native_models[0]=NULL;HSD_GObjFree(objects[2]);objects[2]=NULL;++cases;
}
static void compiler_negatives(void)
{
    static const char* const bodies[]={
        "            item.add_model_animation(4, FlightFirst)\n",
        "            item.add_model_animation(1, MissingBundle)\n",
        "            item.add_model_animation(1, TrailFirst)\n",
        "            item.add_model_animation(1, 0)\n",
        "            item.add_model_animation(1, \"FlightFirst\")\n",
        "            item.add_model_animation(item.state.slot, FlightFirst)\n",
        "            item.add_model_animation(1, \"bad\", FlightFirst)\n",
        "            item.request_model_animation(1, 0.0, 0.0)\n",
        "            item.advance_model_animation(1, \"\", 0.0)\n",
        "            item.remove_model_animation(4)\n",
        "            item.set_model_animation_rate(1, \"\", \"rate\")\n"
    };
    NativeSourceBank scratch={0};CHECK(nativeSourceBank_Allocate(&scratch,&diagnostic));
    uint64_t identity=candidate.program.identity;
    for(unsigned i=0;i<sizeof(bodies)/sizeof(*bodies);++i){
        char source[2048];snprintf(source,sizeof(source),
            "fighter:\n    behavior independent\n"
        "func configure_charge():\n"
        "    smash.charge_mode = Smash.manual_charge\n    smash.charge_frames = 1.125\n"
        "    smash.charge_limit = 42.0\n    smash.saved_animation_rate = 0.5\n"
        "    smash.charge_color = 17\n    smash.shake_index = 3\n    smash.shake_period = 8\n"
        "    physics.mirror_velocity(0.6, 0.8, 0.0)\n"
        "    physics.rotate_velocity_unit_axis(0.0, 0.0, 1.0, 0.375)\n"
        "    let float rx = 0\n    let float ry = 0\n    let float rz = 0\n"
        "    vectors.rotate(velocity.x, velocity.y, velocity.z, 1, -0.625, rx, ry, rz)\n"
        "    velocity.x = rx + ceiling.normal.z + program.entity_params.%s.probe\n    velocity.y = ry + wall.left.normal.z\n"
        "    velocity.z = rz + wall.right.normal.z\n"
        "article %s states 4:\n"
        "    parameter float probe = 1.75\n"
            "    model_root TrailFirst 68\n    model_root TrailSecond 72\n"
            "    model_animation FlightFirst joint 76 material None shape None\n    model_animation FlightSecond 88\n"
            "    state int slot\n    state 0 anim -1:\n        on physics:\n%s",declaration,declaration,bodies[i]);
        NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),
            .metadata=nativeScriptItemMetadata_Validate,.metadata_context=&candidate.metadata,.motion_layout=&candidate.assets.facts.moves};
        const NativeScriptCompileBank* accepted=&candidate.compiler.bank;memset(&diagnostic,0,sizeof(diagnostic));
        CHECK(!nativeScriptCompile_Build(&scratch.bank,&request,&accepted,&diagnostic));
        CHECK(diagnostic.message[0]&&accepted==&candidate.compiler.bank);
        CHECK(candidate.compiler.bank.valid&&candidate.program.identity==identity);
    }
    nativeSourceBank_Release(&scratch);++cases;
}
static bool captured(const void* pointer,size_t bytes)
{
    for(uint32_t i=0;i<regions.count;++i){uintptr_t p=(uintptr_t)pointer,b=(uintptr_t)regions.regions[i].memory;
        if(p>=b&&p-b<=regions.regions[i].size&&bytes<=regions.regions[i].size-(p-b))return true;}
    return false;
}
static void history_step(unsigned frame)
{
    CHECK(nativeRollbackPages_BeginFrame(frame));
    if(frame==0){paired(0);paired(1);paired(2);paired(3);}
    if(frame==1){paired(10);paired(4);paired(3);}
    if(frame==2){paired(5);paired(7);paired(3);}
    CHECK(nativeRollbackPages_EndFrame());++frames;
}
static void history(void)
{
    paired(8);CHECK(nativeRollbackRegions_Build(&regions));
    CHECK(captured(&scene,sizeof(scene))&&captured(data,sizeof(*data)*3));
    CHECK(captured(nativeScriptItemActors,sizeof(nativeScriptItemActors)));
    CHECK(captured(&nativeAObjPool,sizeof(nativeAObjPool))&&captured(nativeAObjPool.payload,nativeAObjPool.stride*nativeAObjPool.capacity));
    CHECK(captured(&nativeFObjPool,sizeof(nativeFObjPool))&&captured(nativeFObjPool.payload,nativeFObjPool.stride*nativeFObjPool.capacity));
    CHECK(!captured(&candidate,sizeof(candidate))&&!captured(&checks,sizeof(checks)));
    uint64_t expected[3],baseline=nativeRollbackRegions_Hash(&regions);
    CHECK(nativeRollbackPages_Init(regions.regions,regions.count,regions.exact_regions,regions.exact_count,4,512));
    for(unsigned frame=0;frame<3;++frame){history_step(frame);expected[frame]=nativeRollbackRegions_Hash(&regions);}
    CHECK(nativeRollbackPages_ToFrame(1,3));++restores;CHECK(nativeRollbackRegions_Hash(&regions)==expected[0]);
    for(unsigned frame=1;frame<3;++frame){history_step(frame);CHECK(nativeRollbackRegions_Hash(&regions)==expected[frame]);}
    CHECK(nativeRollbackPages_ToFrame(0,3));++restores;CHECK(nativeRollbackRegions_Hash(&regions)==baseline);
    for(unsigned frame=0;frame<3;++frame){history_step(frame);CHECK(nativeRollbackRegions_Hash(&regions)==expected[frame]);}
    CHECK(!nativeArticleModelScene_Retire());nativeRollbackPages_Shutdown();++cases;
}
int main(int argc,char** argv)
{
    CHECK(argc==3||argc==4);primary_only=argc==4;declaration=argv[2];heap_bytes=VirtualAlloc(NULL,HEAP_BYTES,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);CHECK(heap_bytes);
    void* start=OSInitAlloc(heap_bytes,heap_bytes+HEAP_BYTES,1);OSHeapHandle heap=OSCreateHeap(start,heap_bytes+HEAP_BYTES);CHECK(heap>=0);
    HSD_SetHeap(heap);OSSetCurrentHeap(heap);HSD_ObjSetHeap(HEAP_BYTES,NULL);HSD_ObjInit();HSD_IDSetup();
    lbHeap_80431FA0.heap_array[0]=(struct Heap){.id=heap,.type=LbHeapType_Hsd,.status=LbHeapStatus_Create,.start=(uintptr_t)heap_bytes,.size=HEAP_BYTES};
    data=lbHeap_80015BD0(0,sizeof(*data)*3);CHECK(data);memset(data,0,sizeof(*data)*3);
    HSD_GObjLibInitDataType config={.p_link_max=63,.gx_link_max=63,.gproc_pri_max=24};HSD_GObj_80391304(&config);
    common_fighter_cases(false);
    stage_candidate(argv[1]);common_fighter_cases(true);if(!primary_only)compiler_negatives();CHECK(nativeArticleModelScene_Prepare());objects[0]=create_item(0);objects[1]=create_item(1);
    {NativeScriptRuntimeContext context={.source=objects[0],.source_class=NativeCallbackSource_Item,
        .generation=(uint32_t)data[0].x1C,.scene_epoch=nativeGObjPool.lifetime};
        vector_collision_cases(&context,&data[0].x378_itemColl);
        NativeScriptValue stage,out;
        CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_StageInternalKind,NULL,0,&stage,&diagnostic));
        CHECK(stage.type==NativeScriptType_I32&&stage.as.i64==(int32_t)Stage_80225194());
        data[0].sfx_unk2=SFX_NONE;
        CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_ItemStopLoopSound,NULL,0,NULL,0,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_Void&&data[0].sfx_unk2==SFX_NONE);
        CHECK(!data[0].x520_cameraBox);
        for(unsigned mode=0;mode<4;++mode){
            data[0].xDCD_flag.b01=mode;
            CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_ItemReleaseCamera,NULL,0,NULL,0,&out,&diagnostic));
            CHECK(out.type==NativeScriptType_Void&&!data[0].x520_cameraBox&&data[0].xDCD_flag.b01==mode);
        }


    }
    if(primary_only){
        CHECK(!program()->model_root_count&&!program()->model_animation_count);
        CHECK(program()->model_selector_count==4);
        paired(2);paired(3);paired(4);paired(5);
        printf("{\"observation\":\"primary_only\",\"origins\":%u,\"roots\":0,\"bundles\":0,\"selectors\":4}\n",program()->animation_origin_count);
    }else{
    paired(6);CHECK(!state(objects[0],"added",NULL).as.u64);
    paired(0);paired(1);CHECK(state(objects[0],"added",NULL).as.u64);
    paired(2);paired(3);
    HSD_JObj* primary=objects[0]->hsd_obj;CHECK(primary->child&&primary->child->child&&primary->child->child->aobj);
    HSD_AObj* retained=primary->child->child->aobj;NativeMatchPoolReference retained_reference;
    CHECK(nativeMatchPool_Reference(&nativeAObjPool,retained,&retained_reference));paired(10);
    CHECK(!primary->aobj&&primary->child->aobj&&primary->child->child->aobj==retained);
    CHECK(nativeMatchPool_Resolve(retained_reference)==retained);
    CHECK(!program()->model_animations[0].anim->aobjdesc&&program()->model_animations[0].anim->child->aobjdesc);
    printf("{\"observation\":\"controller_topology\",\"primary_depths\":[1,2],\"secondary_depth\":1,\"retained_grandchild\":true}\n");
    paired(4);CHECK(primary->child->aobj->framerate==2.5f&&primary->child->child->aobj->framerate==2.5f);
    CHECK(data[0].native_models[0]->child->aobj->framerate==1.0f);
    CHECK(data[0].native_models[1]->child->aobj->framerate==4.0f);
    paired(3);unsafe_cases();borrowed_vm_case();history();paired(8);
    }
    CHECK(nativeArticleModelScene_Retire());HSD_GObjFree(objects[0]);HSD_GObjFree(objects[1]);
    CHECK(!nativeAObjPool.used&&!nativeFObjPool.used);CHECK(nativeSourcePackage_Unpin(&candidate));slots[0].active=false;
    CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));NativeScriptFighterProgram* empty=calloc(1,sizeof(*empty));CHECK(empty);
    nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,empty);free(empty);CHECK(nativeSourcePackage_Release(&candidate));
    nativeFighterProject_Close(&project);CHECK(nativeGObjPool_RetireScene());lbHeap_80015CA8(0,data);OSDestroyHeap(heap);CHECK(VirtualFree(heap_bytes,0,MEM_RELEASE));
    printf("{\"status\":\"PASS\",\"checks\":%u,\"cases\":%u,\"native_graph_comparisons\":%u,\"frames\":%u,\"restores\":%u}\n",checks,cases,comparisons,frames,restores);return 0;
}
