#include <melee/ft/native_script_effect_dispatch.h>
#include <melee/ef/eflib.h>
#include <melee/ef/types.h>
#include <sysdolphin/baselib/generator.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/psstructs.h>
#include <sysdolphin/baselib/psappsrt.h>
#include <sysdolphin/baselib/debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

static unsigned checks,cases,copies;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"position ABI %u %s\n",__LINE__,#x);exit(1);}}while(0)
#include "expected.inc"
#include "requirements.inc"
#include "scalar_requirements.inc"
#include "two_scalar_requirements.inc"
static HSD_Generator generator;
static HSD_psAppSRT appsrt;
static HSD_GObj object;
static HSD_JObj joint;
static EF_Effect effect;
static bool fail;
static int captured_link,captured_bank,captured_id;
static HSD_GObj* captured_parent;
HSD_Generator* hsd_8039F05C(s32 link,s32 bank,s32 id)
{captured_link=link;captured_bank=bank;captured_id=id;return fail?NULL:&generator;}
HSD_psAppSRT* psAddGeneratorAppSRT_begin(HSD_Generator* g,s32 kind)
{CHECK(g==&generator&&kind==1);g->appsrt=&appsrt;return &appsrt;}
void hsd_8039D4DC(HSD_Generator* g){CHECK(g==&generator);}
EF_Effect* efLib_Create(int id,HSD_GObj* parent)
{captured_id=id;captured_parent=parent;return fail?NULL:&effect;}
void HSD_JObjSetMtxDirtySub(HSD_JObj* j){CHECK(j==&joint);j->flags|=JOBJ_MTX_DIRTY;}
void(HSD_JObjSetMtxDirty)(HSD_JObj* j){CHECK(j==&joint);j->flags|=JOBJ_MTX_DIRTY;}
ATTRIBUTE_NORETURN void __assert(char* file,u32 line,char* text)
{fprintf(stderr,"assert %s:%lu %s\n",file,line,text);abort();}
static void efLib_Cb_AccumOffset_FromParams(EF_Effect* e){(void)e;}
static void eflib_create_generator_add_appsrt(HSD_Generator** g,int id,HSD_JObj* j)
{CHECK(j==&joint);captured_id=id;*g=&generator;}
#include "copy_bodies.inc"

static void consumption(void)
{
    const Vec3 values[]={{0,-0.0f,1},{-7.25f,3.125f,FLT_MAX},{NAN,INFINITY,-INFINITY}};
    const int models[]={9,0x1F46,0x32CA,0x3A98},generators[]={0x121,0xFF,0x7918,0xFC,0xF7,0x6A,27,30000};
    object.hsd_obj=&joint;effect.gobj=&object;
    { Vec3 local={0,0,5},original=local;fail=false;
      CHECK(efLib_CreateGenerator_AppSRT_SetPos(0x92,&object,&joint,&local)==&effect);
      CHECK(effect.attach_jobj==&joint&&effect.update==efLib_Cb_AccumOffset_FromParams);
      CHECK(captured_id==0x92&&!memcmp(&effect.params,&original,sizeof(original)));
      local=(Vec3){99,99,99};CHECK(!memcmp(&effect.params,&original,sizeof(original)));
      fail=true;CHECK(!efLib_CreateGenerator_AppSRT_SetPos(0x92,&object,NULL,NULL));
      ++cases;++copies;
    }
    for(unsigned value=0;value<sizeof(values)/sizeof(*values);++value){
        const float directions[]={-3,-0.0f,0,2,NAN,INFINITY};
        for(unsigned d=0;d<sizeof(directions)/sizeof(*directions);++d){
            Vec3 local=values[value],original=local;fail=false;memset(&generator,0,sizeof(generator));memset(&appsrt,0,sizeof(appsrt));
            CHECK(efLib_CreateGenerator_Translate_FacingDir(0x14B,&local,directions[d])==&generator);
            CHECK(!memcmp(&generator.appsrt->translate,&original,sizeof(original)));
            CHECK(generator.appsrt->rot.y==(directions[d]<0?-M_PI_2_F:M_PI_2_F));
            CHECK(!!(generator.kind&0x40000)==(directions[d]<0));
            local=(Vec3){37,38,39};CHECK(!memcmp(&generator.appsrt->translate,&original,sizeof(original)));
            fail=true;CHECK(!efLib_CreateGenerator_Translate_FacingDir(0x14B,NULL,0));++cases;++copies;
        }
        for(unsigned i=0;i<sizeof(generators)/sizeof(*generators);++i){
            Vec3 local=values[value],original=local;fail=false;memset(&generator,0,sizeof(generator));
            CHECK(efLib_CreateGenerator(generators[i],&local)==&generator);
            CHECK(captured_id==generators[i]&&captured_bank==generators[i]/1000);
            CHECK(captured_link==(generators[i]==0x121?2:(generators[i]==0xFF||generators[i]==0x7918||generators[i]==0xFC||generators[i]==0xF7)?1:0));
            CHECK(!memcmp(&generator.pos,&original,sizeof(original)));local=(Vec3){37,38,39};CHECK(!memcmp(&generator.pos,&original,sizeof(original)));
            fail=true;CHECK(!efLib_CreateGenerator(generators[i],NULL));++cases;++copies;
        }
        for(unsigned i=0;i<sizeof(models)/sizeof(*models);++i){
            Vec3 local=values[value],original=local;fail=false;memset(&joint,0,sizeof(joint));
            CHECK(efLib_Create_Attach_Pos(models[i],&object,&local)==&effect);
            CHECK(captured_id==models[i]&&captured_parent==&object);CHECK(!memcmp(&joint.translate,&original,sizeof(original)));
            local=(Vec3){37,38,39};CHECK(!memcmp(&joint.translate,&original,sizeof(original)));
            fail=true;CHECK(!efLib_Create_Attach_Pos(models[i],&object,NULL));++cases;++copies;
        }
    }
}
int main(void)
{
    unsigned totals[8]={0},roots=0,fighters=0,models=0,scalar_roots=0,scalar_fighters=0,scalar_models=0;
    for(unsigned id=0;id<65536;++id){NativeScriptEffectArguments actual=nativeScriptEffectDispatch_Arguments((int32_t)id);
        if(actual!=expected[id])fprintf(stderr,"effect 0x%04X: arguments %u, expected %u\n",id,(unsigned)actual,expected[id]);
        CHECK(actual==expected[id]);CHECK((unsigned)actual<8);++totals[actual];
        uint32_t two_requirements=99;bool two_accepted=nativeScriptEffectDispatch_PositionTwoScalarRequirements((int32_t)id,&two_requirements);
        CHECK(two_accepted==(expected[id]==NativeScriptEffectArguments_PositionTwoScalar));
        CHECK(two_requirements==(two_accepted?expected_two_scalar_requirements[id]:99));
        if(two_accepted)CHECK(two_requirements==NativeScriptEffectPosition_CreatesModel);
        uint32_t scalar_requirements=99;bool scalar_accepted=nativeScriptEffectDispatch_PositionScalarRequirements((int32_t)id,&scalar_requirements);
        CHECK(scalar_accepted==(expected[id]==NativeScriptEffectArguments_PositionScalar));
        CHECK(scalar_requirements==(scalar_accepted?expected_scalar_requirements[id]:99));
        if(scalar_accepted){scalar_roots+=(scalar_requirements&NativeScriptEffectPosition_Root)!=0;scalar_fighters+=(scalar_requirements&NativeScriptEffectPosition_FighterParent)!=0;scalar_models+=(scalar_requirements&NativeScriptEffectPosition_CreatesModel)!=0;}
        uint32_t requirements=99;bool accepted=nativeScriptEffectDispatch_PositionRequirements((int32_t)id,&requirements);
        CHECK(accepted==(expected[id]==NativeScriptEffectArguments_Position));
        CHECK(requirements==(accepted?expected_requirements[id]:99));
        if(accepted){roots+=(requirements&NativeScriptEffectPosition_Root)!=0;fighters+=(requirements&NativeScriptEffectPosition_FighterParent)!=0;models+=(requirements&NativeScriptEffectPosition_CreatesModel)!=0;}
    }
    CHECK(totals[NativeScriptEffectArguments_JointOffset]==1);
    CHECK(roots==5&&fighters==1&&models==13);
    CHECK(scalar_roots==1&&scalar_fighters==1&&scalar_models==14);
    CHECK(!nativeScriptEffectDispatch_PositionScalarRequirements(0x3F1,NULL));
    CHECK(!nativeScriptEffectDispatch_PositionTwoScalarRequirements(0x3FF,NULL));
    CHECK(!nativeScriptEffectDispatch_PositionRequirements(0x410,NULL));
    uint32_t untouched=99;
    CHECK(!nativeScriptEffectDispatch_PositionRequirements(-1,&untouched)&&untouched==99);
    CHECK(!nativeScriptEffectDispatch_PositionRequirements(INT32_MIN,&untouched)&&untouched==99);
    CHECK(!nativeScriptEffectDispatch_PositionRequirements(65536,&untouched)&&untouched==99);
    CHECK(!nativeScriptEffectDispatch_PositionRequirements(INT32_MAX,&untouched)&&untouched==99);
    CHECK(!nativeScriptEffectDispatch_PositionScalarRequirements(-1,&untouched)&&untouched==99);
    CHECK(!nativeScriptEffectDispatch_PositionScalarRequirements(INT32_MIN,&untouched)&&untouched==99);
    CHECK(!nativeScriptEffectDispatch_PositionScalarRequirements(65536,&untouched)&&untouched==99);
    CHECK(!nativeScriptEffectDispatch_PositionScalarRequirements(INT32_MAX,&untouched)&&untouched==99);
    CHECK(!nativeScriptEffectDispatch_PositionTwoScalarRequirements(-1,&untouched)&&untouched==99);
    CHECK(!nativeScriptEffectDispatch_PositionTwoScalarRequirements(INT32_MIN,&untouched)&&untouched==99);
    CHECK(!nativeScriptEffectDispatch_PositionTwoScalarRequirements(65536,&untouched)&&untouched==99);
    CHECK(!nativeScriptEffectDispatch_PositionTwoScalarRequirements(INT32_MAX,&untouched)&&untouched==99);
    CHECK(totals[NativeScriptEffectArguments_Joint]==100);CHECK(totals[NativeScriptEffectArguments_JointScalar]==38);CHECK(totals[NativeScriptEffectArguments_Position]==63+592+1000);
    CHECK(totals[NativeScriptEffectArguments_TwoJoint]==4);
    CHECK(nativeScriptEffectDispatch_Arguments(-1)==NativeScriptEffectArguments_Unsupported);
    CHECK(nativeScriptEffectDispatch_Arguments(INT32_MIN)==NativeScriptEffectArguments_Unsupported);
    CHECK(nativeScriptEffectDispatch_Arguments(65536)==NativeScriptEffectArguments_Unsupported);
    CHECK(nativeScriptEffectDispatch_Arguments(INT32_MAX)==NativeScriptEffectArguments_Unsupported);
    CHECK(nativeScriptEffectDispatch_Arguments(0x503)==NativeScriptEffectArguments_Position);
    CHECK(nativeScriptEffectDispatch_Arguments(0x504)==NativeScriptEffectArguments_Position);
    CHECK(nativeScriptEffectDispatch_Arguments(0x505)==NativeScriptEffectArguments_Position);
    CHECK(nativeScriptEffectDispatch_Arguments(0x4F8)==NativeScriptEffectArguments_Joint);
    CHECK(nativeScriptEffectDispatch_Arguments(0x4F9)==NativeScriptEffectArguments_Joint);
    CHECK(nativeScriptEffectDispatch_Arguments(0x4FA)==NativeScriptEffectArguments_Joint);
    CHECK(totals[NativeScriptEffectArguments_PositionScalar]==34);
    CHECK(totals[NativeScriptEffectArguments_PositionTwoScalar]==1);
    CHECK(nativeScriptEffectDispatch_Arguments(0x4CF)==NativeScriptEffectArguments_PositionScalar);
    CHECK(nativeScriptEffectDispatch_Arguments(0x448)==NativeScriptEffectArguments_JointOffset); // typed callee proves JObj + copied Vec3
    CHECK(nativeScriptEffectDispatch_Arguments(0x4C1)==NativeScriptEffectArguments_Unsupported); // other forwarder
    consumption();printf("{\"status\":\"PASS\",\"checks\":%u,\"cases\":%u,\"ids_compared\":65536,\"joint\":%u,\"joint_scalar\":%u,\"position\":%u,\"helper_copy_cases\":%u,\"position_roots\":%u,\"position_fighter_parents\":%u,\"position_model_results\":%u}\n",checks,cases,totals[1],totals[2],totals[3],copies,roots,fighters,models);return 0;
}
