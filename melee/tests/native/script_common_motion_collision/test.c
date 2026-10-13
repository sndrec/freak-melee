#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "fixture.h"
#include <melee/ft/native_script_binding_actions.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_source_bank.h>
#include <melee/ft/native_script_binding_common_movement.h>
#include <melee/ft/ft_081B.h>
#include <melee/mp/mpcoll.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/fighter.h>
#include <melee/ft/kinds/ftCommon/ftCo_HammerWait.h>
#include <melee/ft/kinds/ftCommon/ftCo_HammerLanding.h>
#include <melee/ft/kinds/ftCommon/ftCo_Landing.h>
#include <melee/lb/lbanim.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""

NativeSourcePackage target_package;
static MotionState common[341],motions[2];static Fighter fighters[256],replacement_data;

static FighterPartsTable* parts[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=parts;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
static ftCommonData common_parameters={.x310=2,.x6B4=3};ftCommonData* p_ftCommonData=&common_parameters;
static unsigned checks,cases,comparisons,created,order[3],order_count,success_at,trigger_at,change,callbacks;
static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"common motion/collision %u: %s (%u:%u %s)\n",__LINE__,#x,diagnostic.source.line,diagnostic.source.column,diagnostic.message);exit(1);}}while(0)
static uint32_t function(const char* name)
{
    const NativeScriptProgramIndex* p=&target_package.compiler.bank.declarations;
    for(unsigned i=0;i<p->function_count;++i)if(nativeScriptSource_SpanEquals(p->tree,p->functions[i].name,name))return i;
    CHECK(false);return 0;
}
static void prepare(void)
{
    CHECK(nativeSourceBank_Allocate(&target_package.compiler,&diagnostic));
    const char* source="fighter:\n    behavior independent\nstate int divisor\nstate int after\nstate int phase\nstate bool result\nfunc Step():\n    collision.ground_with_nudge()\n    state.after = state.after + 1\nfunc Air():\n    physics.ground_to_air()\nfunc Fault() -> int:\n    return 1 / state.divisor\nfunc Contact():\n    state.result = collision.landed\n    state.after = state.after + 1\nfunc CatchHit():\n    collision.air_catch_hit()\n    state.after = state.after + 1\nfunc StateChanged():\n    state.phase = 5\n    state.result = collision.skip_soft_floor()\naction Common:\n    on cpu_capture_decision:\n        return false\n    on landing:\n        state.phase = 2\n        state.result = collision.skip_soft_floor()\n    on grounded_motion_setup:\n        state.phase = 4\n        state.result = collision.skip_soft_floor()\n    on landing_action:\n        state.phase = 6\n        state.result = collision.skip_soft_floor()\n    on tick:\n        Step()\n";
    NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=2};
    NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout};
    const NativeScriptCompileBank* accepted=NULL;
    CHECK(nativeScriptCompile_Build(&target_package.compiler.bank,&request,&accepted,&diagnostic));
    CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,accepted,&layout,NULL,&target_package.program,&diagnostic));
    target_package.address=&target_package;target_package.target=Ft_Kind_CustomFirst;target_package.ready=target_package.assets.ready=true;
    target_package.assets.facts.moves=layout;target_package.assets.common_motions=common;target_package.assets.motions=motions;
    
    target_package.assets.parts.parts_num=4;target_package.assets.facts.joints=4;parts[Ft_Kind_CustomFirst]=&target_package.assets.parts;
    gFtDataList[Ft_Kind_CustomFirst]=&target_package.assets.character.data;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=motions;
    nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&target_package.program);
}
static void remove_fighter(void* value){nativeScriptFighter_Detach(((Fighter*)value)->gobj);}
static HSD_GObj* fighter(bool owned)
{
    CHECK(created<256);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(o);
    Fighter* f=&fighters[created++];*f=(Fighter){.gobj=o,.kind=owned?Ft_Kind_CustomFirst:Ft_Kind_Link,
        .behavior_kind=owned?Ft_Kind_CustomFirst:Ft_Kind_Link,.x8_spawnNum=created+10,.ft_data=&target_package.assets.character.data,.allow_interrupt=true,.x34_scale={1,1,1}};

    GObj_InitUserData(o,4,remove_fighter,f);if(owned)CHECK(nativeScriptFighter_Attach(o));return o;
}
static NativeScriptRuntimeContext context(HSD_GObj* o)
{uint32_t id=((Fighter*)o->user_data)->x8_spawnNum;return(NativeScriptRuntimeContext){.source=o,.generation=id,.actor_id=id,.source_class=NativeCallbackSource_Fighter,.scene_epoch=nativeGObjPool.lifetime};}
static int64_t named_state(HSD_GObj* o,const char* name)
{
    NativeScriptReceiverView v;CHECK(nativeScriptFighter_View(o,((Fighter*)o->user_data)->x8_spawnNum,&v));
    for(unsigned i=0;i<v.program->value_count;++i)if(nativeScriptSource_SpanEquals(v.program->tree,v.program->values[i].name,name)){
        NativeScriptValue value;CHECK(nativeScriptStorage_Read(v.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&value,&diagnostic));return value.as.i64;
    }CHECK(false);return 0;
}
static int64_t after(HSD_GObj* o){return named_state(o,"after");}
static HSD_GObj* active;
bool collision_decision(Fighter_GObj* o,unsigned index)
{
    CHECK(order_count<3);order[order_count++]=index;
    if(change && index==trigger_at) {
        unsigned mode=change;change=0;NativeScriptValue output={.type=NativeScriptType_U64,.as.u64=99};
        CHECK(nativeScriptFighter_Run(o,NativeScriptFighter_CpuCaptureDecision,NULL,0, NULL,&output)==NativeScriptVM_Ok);
        CHECK(output.type==NativeScriptType_Bool&&!output.as.u64);++callbacks;
        Fighter* f=o->user_data;
        if(mode==1)HSD_GObjFree(o);
        if(mode==2){CHECK(nativeScriptFighter_Helper(o,f->x8_spawnNum,function("Fault"),NULL,0,&output,&diagnostic)!=NativeScriptVM_Ok);CHECK(nativeScriptFighter_Status(o)==NativeScriptVM_Error);}
        if(mode==3)++f->x8_spawnNum;
        if(mode==4)++((NativeScriptFighterProgram*)nativeScriptFighter_Program(f->kind))->identity;
        if(mode==5){replacement_data=*f;o->user_data=&replacement_data;}
        return false;
    }
    return index==success_at;
}
void collision_fall(Fighter_GObj* o){(void)o;CHECK(order_count<3);order[order_count++]=2;}
bool ft_800827A0(Fighter_GObj* o){return collision_decision(o,0);}
bool ftCo_8009A3C8(Fighter_GObj* o){return collision_decision(o,1);}
void ftCo_Fall_Enter(Fighter_GObj* o){collision_fall(o);}
bool mpColl_8004B4B0(CollData* c){c->cur_pos.x+=1;return collision_decision(active,0);}
static void reset(unsigned success){order_count=0;success_at=success;trigger_at=UINT32_MAX;change=0;memset(order,0,sizeof(order));}
static void collision_cases(void)
{
    HSD_GObj* o=fighter(true);active=o;NativeScriptRuntimeContext c=context(o);NativeScriptValue output;
    const float nudge[]={0,1,-1,0.0f/0.0f};
    for(unsigned direction=0;direction<2;++direction)for(unsigned n=0;n<4;++n)for(unsigned success=0;success<3;++success){
        Fighter* f=o->user_data;f->facing_dir=direction?-1:1;f->xF8_playerNudgeVel.x=nudge[n];
        f->cur_pos=(Vec3){3,4,5};f->coll_data.cur_pos=(Vec3){1,2,3};f->input.lstick[0].x=0.25f;
        Fighter before=*f;reset(success);reference_nudge(o);Fighter expected=*f;
        unsigned count=order_count,expected_order[3];memcpy(expected_order,order,sizeof(order));
        *f=before;reset(success);CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_CollisionGroundWithNudge,NULL,0,NULL,0,&output,&diagnostic));
        CHECK(output.type==NativeScriptType_Void&&order_count==count&&!memcmp(order,expected_order,count*sizeof(*order)));
        CHECK(!memcmp(&f->cur_pos,&expected.cur_pos,sizeof(Vec3))&&!memcmp(&f->coll_data,&expected.coll_data,sizeof(CollData)));
        *f=before;reset(success);int64_t prior=after(o);CHECK(nativeScriptFighter_Run(o,NativeScriptFighter_Tick,NULL,0, NULL,&output)==NativeScriptVM_Ok);
        CHECK(order_count==count&&!memcmp(order,expected_order,count*sizeof(*order))&&after(o)==prior+1);++comparisons;++cases;
    }
    HSD_GObjFree(o);
    o=fighter(false);active=o;reset(2);ft_80084280(o);CHECK(order_count==3);HSD_GObjFree(o);++cases;
}
static void air_cases(void)
{
    HSD_GObj* o=fighter(true);NativeScriptRuntimeContext c=context(o);Fighter* f=o->user_data;
    for(unsigned used=0;used<4;++used){
        f->ground_or_air=GA_Ground;f->gr_vel=3;f->x98_atk_shield_kb=(Vec3){1,2,3};f->cur_pos=(Vec3){4,5,6};
        f->x74_self_accel=(Vec3){7,8,9};f->x1968_jumpsUsed=used;f->ecb_lock=0;f->coll_data.x130_flags=0;
        Fighter before=*f;reference_ground_to_air(f);Fighter expected=*f;*f=before;
        NativeScriptValue output;CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_PhysicsGroundToAir,NULL,0,NULL,0,&output,&diagnostic));
        CHECK(output.type==NativeScriptType_Void&&!memcmp(f,&expected,sizeof(*f)));*f=before;
        CHECK(nativeScriptFighter_Helper(o,c.generation,function("Air"),NULL,0,&output,&diagnostic)==NativeScriptVM_Ok);CHECK(!memcmp(f,&expected,sizeof(*f)));++comparisons;++cases;
    }HSD_GObjFree(o);
}
static void retirements(void)
{
    for(unsigned phase=0;phase<2;++phase)for(unsigned mode=1;mode<=5;++mode) {
        HSD_GObj* o=fighter(true);active=o;NativeScriptRuntimeContext c=context(o);reset(2);trigger_at=phase;change=mode;
        NativeScriptFighterProgram* selected=(NativeScriptFighterProgram*)nativeScriptFighter_Program(Ft_Kind_CustomFirst);
        uint64_t identity=selected->identity;Fighter* data=o->user_data;Vec3 initial=data->cur_pos;
        NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=77};
        CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_CollisionGroundWithNudge,NULL,0,NULL,0,&result,&diagnostic));
        CHECK(change==0&&order_count==phase+1&&result.type==NativeScriptType_U64&&result.as.u64==77);
        if(phase==0)CHECK(!memcmp(&data->cur_pos,&initial,sizeof(initial)));
        selected->identity=identity;
        if(mode!=1){if(mode==5)o->user_data=data;HSD_GObjFree(o);}++cases;
    }
    for(unsigned mode=1;mode<=2;++mode){HSD_GObj* o=fighter(true);active=o;reset(2);trigger_at=1;change=mode;
        NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=77};uint32_t id=((Fighter*)o->user_data)->x8_spawnNum;
        CHECK(nativeScriptFighter_Helper(o,id,function("Step"),NULL,0,&result,&diagnostic)!=NativeScriptVM_Ok);CHECK(order_count==2&&result.type==NativeScriptType_U64&&result.as.u64==77);
        if(mode!=1)HSD_GObjFree(o);++cases;
    }
}
static unsigned tether_order[32],tether_count,tether_trigger=UINT32_MAX,tether_change,tether_comparisons,tether_guards;
static bool tether_contact,tether_interrupted,has_hammer;
static void trace(unsigned phase){CHECK(tether_count<32);tether_order[tether_count++]=phase;}
static void tether_invalidate(Fighter_GObj* o)
{
    unsigned mode=tether_change;tether_change=0;Fighter* f=o->user_data;NativeScriptValue output;
    CHECK(nativeScriptFighter_Run(o,NativeScriptFighter_CpuCaptureDecision,NULL,0,NULL,&output)==NativeScriptVM_Ok);
    if(mode==1)HSD_GObjFree(o);
    else if(mode==2)CHECK(nativeScriptFighter_Helper(o,f->x8_spawnNum,function("Fault"),NULL,0,&output,&diagnostic)!=NativeScriptVM_Ok);
    else if(mode==3)++f->x8_spawnNum;
    else if(mode==4)++((NativeScriptFighterProgram*)nativeScriptFighter_Program(f->kind))->identity;
    else if(mode==5){replacement_data=*f;o->user_data=&replacement_data;}
}
bool tether_geometry(CollData* c){trace(0);c->cur_pos.x+=10;if(tether_change&&tether_trigger==0)tether_invalidate(active);return tether_contact;}
bool tether_environment(Fighter_GObj* o){trace(1);if(tether_change&&tether_trigger==1)tether_invalidate(o);return tether_interrupted;}
bool tether_hammer(Fighter_GObj* o){(void)o;return has_hammer;}
void tether_cleanup(Fighter* f){CHECK(f==active->user_data);trace(7);f->self_vel.x=9;}
bool tether_hook(Fighter_GObj* o)
{
    unsigned phase=(unsigned)named_state(o,"phase");trace(phase);
    if(tether_change && phase==tether_trigger) {
        unsigned mode=tether_change;tether_change=0;Fighter* f=o->user_data;
        if(mode==1)HSD_GObjFree(o);
        else if(mode==2){NativeScriptValue value;CHECK(nativeScriptFighter_Helper(o,f->x8_spawnNum,function("Fault"),NULL,0,&value,&diagnostic)!=NativeScriptVM_Ok);}
        else if(mode==3)++f->x8_spawnNum;
        else if(mode==4)++((NativeScriptFighterProgram*)nativeScriptFighter_Program(f->kind))->identity;
        else if(mode==5){replacement_data=*f;o->user_data=&replacement_data;}
    }
    return false;
}
void tether_motion(Fighter_GObj* o,FtMotionId m,MotionFlags flags,float start,float speed,float blend,Fighter_GObj* peer)
{
    CHECK(flags==0&&start==0&&speed==1&&blend==0&&peer==NULL);trace(3);
    Fighter* f=o->user_data;f->motion_id=m;
    // Controlled motion engine leaf dispatches actual VM hooks in native order.
    // The full renderer/motion setup body is outside this fixture.
    NativeFighterLifetime life;CHECK(nativeFighterLifetime_Capture(f,&life));
    if(life.source_owned) {
        if(nativeScriptFighter_Run(o,NativeScriptFighter_GroundedMotionSetup,NULL,0,NULL,NULL)!=NativeScriptVM_Ok || !nativeFighterLifetime_Current(&life))return;
        if(nativeScriptFighter_CallbackBound(o,NativeScriptFighterCallback_StateChange))
            (void)nativeScriptFighter_RunCallback(o,NativeScriptFighterCallback_StateChange);
    }
}
void tether_neutral(Fighter_GObj* o)
{
    trace(8);
    if(has_hammer){ftCo_800C4ED8(o);return;}
    // Neutral entry is a controlled selected leaf; the generic native neutral
    // algorithm already has its own retained source guards and separate proof.
    ((Fighter*)o->user_data)->motion_id=ftCo_MS_Wait;
}
static void tether_reset(bool contact,bool interrupted,bool hammer)
{tether_count=0;tether_contact=contact;tether_interrupted=interrupted;has_hammer=hammer;tether_trigger=UINT32_MAX;tether_change=0;}
static void tether_initial(HSD_GObj* o,float y)
{Fighter* f=o->user_data;f->ground_or_air=GA_Air;f->self_vel=(Vec3){0,y,0};f->cur_pos=(Vec3){3,4,5};f->coll_data.cur_pos=(Vec3){1,2,3};f->motion_id=360;}
static void tether_cases(void)
{
    HSD_GObj* o=fighter(true);active=o;NativeScriptRuntimeContext c=context(o);Fighter* f=o->user_data;NativeScriptValue value;
    for(unsigned contact=0;contact<2;++contact)for(unsigned interrupted=0;interrupted<2;++interrupted) {
        tether_initial(o,0);Fighter before=*f;tether_reset(contact,interrupted,false);
        GroundOrAir expected=reference_air_contact(o);Fighter expected_f=*f;*f=before;tether_reset(contact,interrupted,false);
        CHECK(nativeScriptBindings_Read(&c,NativeScriptBinding_CollisionLanded,NULL,0,&value,&diagnostic));
        CHECK(value.type==NativeScriptType_Bool&&value.as.u64==(expected!=GA_Ground));CHECK(!memcmp(&f->cur_pos,&expected_f.cur_pos,sizeof(Vec3)));
        *f=before;tether_reset(contact,interrupted,false);int64_t prior=after(o);
        CHECK(nativeScriptFighter_Helper(o,c.generation,function("Contact"),NULL,0,&value,&diagnostic)==NativeScriptVM_Ok&&after(o)==prior+1);++tether_comparisons;
    }
    const float speeds[]={-3,-2,-1};
    for(unsigned contact=0;contact<2;++contact)for(unsigned interrupted=0;interrupted<2;++interrupted)for(unsigned speed=0;speed<3;++speed)for(unsigned hammer=0;hammer<2;++hammer) {
        tether_initial(o,speeds[speed]);Fighter before=*f;tether_reset(contact,interrupted,hammer);
        CHECK(nativeScriptFighter_BindCallback(o,NativeScriptFighterCallback_StateChange,function("StateChanged")));
        reference_air_catch_hit(o);Fighter expected_f=*f;unsigned count=tether_count,expected_order[32];memcpy(expected_order,tether_order,sizeof(tether_order));
        *f=before;tether_reset(contact,interrupted,hammer);CHECK(nativeScriptFighter_BindCallback(o,NativeScriptFighterCallback_StateChange,function("StateChanged")));
        CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_CollisionAirCatchHit,NULL,0,NULL,0,&value,&diagnostic));
        CHECK(value.type==NativeScriptType_Void&&count==tether_count&&!memcmp(expected_order,tether_order,count*sizeof(unsigned)));
        CHECK(f->motion_id==expected_f.motion_id&&f->ground_or_air==expected_f.ground_or_air&&f->self_vel.x==expected_f.self_vel.x&&f->mv.co.landing.allow_interrupt==expected_f.mv.co.landing.allow_interrupt);
        *f=before;tether_reset(contact,interrupted,hammer);CHECK(nativeScriptFighter_BindCallback(o,NativeScriptFighterCallback_StateChange,function("StateChanged")));int64_t prior=after(o);
        CHECK(nativeScriptFighter_Helper(o,c.generation,function("CatchHit"),NULL,0,&value,&diagnostic)==NativeScriptVM_Ok&&after(o)==prior+1);
        CHECK(count==tether_count&&!memcmp(expected_order,tether_order,count*sizeof(unsigned)));++tether_comparisons;
    }
    HSD_GObjFree(o);
    // Instrumented geometry/environment boundaries exercise retained publication.
    // They are hazards in controlled leaves, not normal authored callbacks.
    for(unsigned operation=0;operation<2;++operation)for(unsigned phase=0;phase<2;++phase)for(unsigned mode=1;mode<=5;++mode) {
        o=fighter(true);active=o;c=context(o);f=o->user_data;tether_initial(o,-2);Vec3 before=f->cur_pos;
        tether_reset(true,false,false);tether_trigger=phase;tether_change=mode;
        NativeScriptFighterProgram* selected=(NativeScriptFighterProgram*)nativeScriptFighter_Program(f->kind);uint64_t identity=selected->identity;
        value=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=77};
        bool ok=operation?nativeScriptBindings_Invoke(&c,NativeScriptBinding_CollisionAirCatchHit,NULL,0,NULL,0,&value,&diagnostic):
            nativeScriptBindings_Read(&c,NativeScriptBinding_CollisionLanded,NULL,0,&value,&diagnostic);
        CHECK(!ok&&tether_change==0&&tether_count==phase+1&&value.type==NativeScriptType_U64&&value.as.u64==77);
        if(!phase)CHECK(!memcmp(&f->cur_pos,&before,sizeof(before)));
        selected->identity=identity;if(mode!=1){if(mode==5)o->user_data=f;HSD_GObjFree(o);}++tether_guards;
    }
    // Above the threshold, a held hammer selects native HammerWait. Its
    // cached cleanup follows the same actual motion-hook retirement boundary.
    for(unsigned phase=4;phase<=5;++phase)for(unsigned mode=1;mode<=5;++mode) {
        o=fighter(true);active=o;c=context(o);f=o->user_data;tether_initial(o,-1);tether_reset(true,false,true);
        CHECK(nativeScriptFighter_BindCallback(o,NativeScriptFighterCallback_StateChange,function("StateChanged")));
        NativeScriptFighterProgram* selected=(NativeScriptFighterProgram*)nativeScriptFighter_Program(f->kind);uint64_t identity=selected->identity;
        tether_trigger=phase;tether_change=mode;value=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=77};
        CHECK(nativeScriptFighter_Helper(o,c.generation,function("CatchHit"),NULL,0,&value,&diagnostic)!=NativeScriptVM_Ok);
        CHECK(tether_change==0&&tether_order[tether_count-1]==phase&&value.type==NativeScriptType_U64&&value.as.u64==77);
        selected->identity=identity;if(mode!=1){if(mode==5)o->user_data=f;HSD_GObjFree(o);}++tether_guards;
    }
    // Real authored hook -> actual common-movement binding -> controlled native
    // soft-floor leaf; retirement/fault occurs while the hook VM is running.
    const unsigned phases[]={2,4,5,6};
    for(unsigned phase=0;phase<4;++phase)for(unsigned hammer=0;hammer<2;++hammer)for(unsigned mode=1;mode<=5;++mode) {
        o=fighter(true);active=o;c=context(o);f=o->user_data;tether_initial(o,-2);tether_reset(true,false,hammer);
        CHECK(nativeScriptFighter_BindCallback(o,NativeScriptFighterCallback_StateChange,function("StateChanged")));
        f->mv.co.landing.allow_interrupt=false;NativeScriptFighterProgram* selected=(NativeScriptFighterProgram*)nativeScriptFighter_Program(f->kind);uint64_t identity=selected->identity;
        tether_trigger=phases[phase];tether_change=mode;value=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=77};
        CHECK(nativeScriptFighter_Helper(o,c.generation,function("CatchHit"),NULL,0,&value,&diagnostic)!=NativeScriptVM_Ok);
        CHECK(tether_change==0&&value.type==NativeScriptType_U64&&value.as.u64==77);
        CHECK(tether_order[tether_count-1]==phases[phase]);if(phases[phase]<6)CHECK(!f->mv.co.landing.allow_interrupt);
        selected->identity=identity;if(mode!=1){if(mode==5)o->user_data=f;HSD_GObjFree(o);}++tether_guards;
    }
}
static void type_cases(void)
{
    const char* invalid[]={"fighter:\n    behavior independent\nfunc Bad():\n    physics.ground_to_air(1)\n",
        "fighter:\n    behavior independent\nfunc Bad() -> bool:\n    return collision.ground_with_nudge()\n",
        "fighter:\n    behavior independent\nfunc Bad():\n    collision.air_catch_hit(1)\n",
        "fighter:\n    behavior independent\nfunc Bad() -> bool:\n    return collision.air_catch_hit()\n",
        "fighter:\n    behavior independent\nfunc Bad():\n    collision.air()\n"};
    for(unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);++i){NativeSourceBank scratch={0};CHECK(nativeSourceBank_Allocate(&scratch,&diagnostic));
        const NativeScriptCompileBank* accepted=&target_package.compiler.bank;
        NativeScriptCompileRequest request={.source=invalid[i],.source_bytes=(uint32_t)strlen(invalid[i]),.motion_layout=&accepted->motion_layout};
        CHECK(!nativeScriptCompile_Build(&scratch.bank,&request,&accepted,&diagnostic)&&accepted==&target_package.compiler.bank);nativeSourceBank_Release(&scratch);++cases;
    }
}
int main(void)
{
    HSD_GObjLibInitDataType setup={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&setup);prepare();
    collision_cases();air_cases();retirements();tether_cases();type_cases();CHECK(callbacks==12);CHECK(nativeScriptRuntime_Idle());
    nativeSourceBank_Release(&target_package.compiler);
    printf("{\"status\":\"PASS common motion/collision\",\"checks\":%u,\"cases\":%u,\"native_comparisons\":%u,\"nested_callbacks\":%u,\"tether_comparisons\":%u,\"tether_guards\":%u}\n",checks,cases,comparisons,callbacks,tether_comparisons,tether_guards);return 0;
}
