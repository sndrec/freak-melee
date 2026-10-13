#include <melee/ft/kinds/ftCommon/ftCo_ItemThrow.h>

static unsigned heavy_comparisons,heavy_cases;
extern float ground_heavy_multiplier;
bool HeavyReference_ftCo_80094EA4(Fighter_GObj*);

static void heavy_input(Fighter* fighter,unsigned sample,unsigned mode,bool held)
{
    input(fighter,sample);
    fighter->motion_id=ftCo_MS_LiftWait;
    fighter->cmd_vars[0]=42;fighter->cmd_vars[1]=43;fighter->throw_flags=UINT32_MAX;
    fighter->x673=mode&1?20:0;fighter->x674=mode&2?20:0;
    fighter->co_attrs.jump_startup_time=3;
    fighter->x2222_b0=(sample&8)!=0;
    fighter->item_gobj=held?(Item_GObj*)(uintptr_t)16:NULL;
    if(mode==3)fighter->input.pressed_buttons=HSD_PAD_B;
    ground_heavy_multiplier=sample%3==0?.5f:sample%3==1?1:2;
    p_ftCommonData->x400=1.5f;p_ftCommonData->x44=0;
}

static void heavy_pairs(void)
{
    // Both sides execute common selection and entry, with no authored hook.
    // Exercise both stick sources, facing signs, ordinary/smash thresholds,
    // input timers, held/no-item cases, and native airborne callback selection.
    unsigned selected=0;
    for(unsigned sample=0;sample<200;++sample)for(unsigned mode=0;mode<4;++mode)
    for(unsigned held=0;held<2;++held) {
        HSD_GObj* object=create();Fighter* fighter=object->user_data;
        attack_reset();heavy_input(fighter,sample,mode,held);Fighter initial=*fighter;
        bool expected_result=HeavyReference_ftCo_80094EA4(object);
        Fighter expected=*fighter;unsigned count=attack_count,trace[64];
        int motion=attack_last_motion;memcpy(trace,attack_trace,sizeof(trace));
        if(expected_result) {
            const int motions[]={ftCo_MS_HeavyThrowF,ftCo_MS_HeavyThrowB,
                ftCo_MS_HeavyThrowHi,ftCo_MS_HeavyThrowLw,ftCo_MS_HeavyThrowF4,
                ftCo_MS_HeavyThrowB4,ftCo_MS_HeavyThrowHi4,ftCo_MS_HeavyThrowLw4};
            for(unsigned i=0;i<ARRAY_SIZE(motions);++i)if(motion==motions[i])selected|=1u<<i;
        }
        *fighter=initial;attack_reset();CHECK(invoke(object,"HeavyThrow"));
        CHECK(state(object,"result",NULL).as.u64==expected_result);
        CHECK(state(object,"tail",NULL).as.i64==1);
        CHECK(attack_last_motion==motion && attack_count==count);
        CHECK(!memcmp(attack_trace,trace,count*sizeof(*trace)));
        CHECK(!memcmp(fighter,&expected,sizeof(*fighter)) && !attack_stock);
        ++heavy_comparisons;++heavy_cases;++cases;HSD_GObjFree(object);
    }
    CHECK(selected==255);
}

static void heavy_hooks(void)
{
    prepare(true);
    HSD_GObj* object=create();Fighter* fighter=object->user_data;
    attack_reset();heavy_input(fighter,2,0,true);
    CHECK(invoke(object,"HeavyThrow"));CHECK(state(object,"result",NULL).as.u64);
    CHECK(state(object,"entered",NULL).as.i64==6);
    CHECK(state(object,"hook_tail",NULL).as.i64==1 && state(object,"tail",NULL).as.i64==1);
    CHECK(attack_count==4 && attack_trace[0]==Attack_Motion &&
        attack_trace[1]==Attack_Animation && attack_trace[2]==222 && attack_trace[3]==221);
    CHECK(fighter->accessory4_cb==ftCo_80095EFC && fighter->take_dmg_cb==ftCo_800974C4);
    CHECK(!attack_stock);++heavy_cases;++cases;HSD_GObjFree(object);

    const unsigned points[]={Attack_Motion,Attack_Animation,222,221};
    for(unsigned point=0;point<ARRAY_SIZE(points);++point)
    for(unsigned failure=1;failure<=5;++failure)for(unsigned vm=0;vm<2;++vm) {
        object=create();fighter=object->user_data;attack_reset();heavy_input(fighter,2,0,true);
        NativeMatchPoolReference reference;CHECK(nativeMatchPool_Reference(&nativeGObjPool,object,&reference));
        NativeScriptRuntimeContext context={.source=object,.source_class=NativeCallbackSource_Fighter,
            .generation=fighter->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};
        NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX},before=result;
        NativeScriptDiagnostic diagnostic={0};attack_stop=points[point];attack_failure=failure;
        NativeScriptFighterProgram* program=(NativeScriptFighterProgram*)nativeScriptFighter_Program(fighter->kind);
        uint64_t program_identity=program->identity;
        if(vm)CHECK(!invoke(object,"HeavyThrow"));
        else CHECK(!nativeScriptBindingGroundControls_Invoke(&context,NativeScriptBinding_ControlsTryHeavyItemThrow,
            NULL,0,&result,&diagnostic) && !memcmp(&result,&before,sizeof(result)));
        CHECK(attack_count==point+1 && attack_trace[point]==points[point]);
        program->identity=program_identity;
        if(failure==4)object->user_data=fighter;
        if(nativeMatchPool_Resolve(reference))HSD_GObjFree(object);else CHECK(failure==1);
        ++heavy_cases;++cases;
    }
    object=create();fighter=object->user_data;attack_reset();heavy_input(fighter,2,0,true);
    NativeScriptRuntimeContext context={.source=object,.source_class=NativeCallbackSource_Fighter,
        .generation=fighter->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};
    NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(object,fighter->x8_spawnNum,&view));
    NativeScriptStorageInstance saved=*view.storage;ftCommonData* common=p_ftCommonData;
    for(unsigned mode=0;mode<7;++mode) {
        NativeScriptRuntimeContext bad=context;Fighter before_fighter=*fighter;
        NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=99},before=result;
        NativeScriptDiagnostic diagnostic={0};
        switch(mode) {
        case 0:++bad.scene_epoch;break;case 1:++bad.generation;break;
        case 2:bad.source_class=NativeCallbackSource_Item;break;
        case 3:view.storage->live=false;break;case 4:package.ready=false;break;
        case 5:p_ftCommonData=NULL;break;default:break;
        }
        CHECK(!nativeScriptBindingGroundControls_Invoke(&bad,NativeScriptBinding_ControlsTryHeavyItemThrow,
            mode==6?&result:NULL,mode==6?1:0,&result,&diagnostic));
        CHECK(!memcmp(&result,&before,sizeof(result)) && !memcmp(fighter,&before_fighter,sizeof(*fighter)) && !attack_count);
        *view.storage=saved;package.ready=true;p_ftCommonData=common;++heavy_cases;++cases;
    }
    HSD_GObjFree(object);
}
