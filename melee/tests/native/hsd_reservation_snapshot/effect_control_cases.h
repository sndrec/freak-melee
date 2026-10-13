#include <melee/ef/efdata.h>
#include <melee/ef/native_effect_control_snapshot.h>
static unsigned effect_control_checks,effect_control_refusals;
static void test_effect_controls(void)
{
    unsigned first=checks;camera_idle=true;
    CHECK(nativeMatchPool_Bind(&nativeGObjPool,camera_objects,sizeof(camera_objects),camera_slots,camera_free,8,sizeof(HSD_GObj),_Alignof(HSD_GObj),112));
    NativeMatchPoolReference roots[2];HSD_GObj* objects[2];HSD_JObj* models[2];
    for(unsigned i=0;i<2;++i) {
        objects[i]=nativeMatchPool_Allocate(&nativeGObjPool,&roots[i]);CHECK(objects[i]);
        models[i]=hsdNew(&stage_joint_class);CHECK(models[i]);
        memset((u8*)models[i]+sizeof(HSD_Class),0,sizeof(HSD_JObj)-sizeof(HSD_Class));
    }
    memset(efLib_ParamTable,0,sizeof(efLib_ParamTable));memset(efLib_AnimQueue,0,sizeof(efLib_AnimQueue));
    efLib_AnimCount=2;efLib_AnimQueue[0]=models[0];efLib_AnimQueue[1]=models[1];
    efLib_AnimQueue[31]=(void*)1; // dormant drain tail is never followed
    efLib_LoadKind=0;efLib_EffectCount=2;efLib_AllocationSerial=991;
    efLib_SetParamAlpha(objects[0],120);efLib_SetParamGfxId(objects[0],1017);
    efLib_SetParamAlpha(objects[1],200);efLib_SetParamGfxId(objects[1],1019);
    nativeEffectControl_BeginScene();
    NativeEffectControlSnapshot saved,bad,after,end,repeat;size_t bytes,ob,cb;
    CHECK(nativeEffectControl_Save(&saved,sizeof(saved),&bytes));
    CHECK(nativeMatchSnapshotPool_Save(&nativeGObjPool,stage_objects_saved,sizeof(stage_objects_saved),&ob));
    CHECK(nativeClass_Save(stage_graph_saved,sizeof(stage_graph_saved),&cb));
    for(unsigned pass=0;pass<2;++pass) {
        CHECK(nativeEffectControl_Load(&saved,sizeof(saved)));
        for(unsigned frame=0;frame<16;++frame) {
            efLib_SetParamAlpha(objects[frame%2],(u8)(frame*7));
            efLib_SetParamGfxId(objects[frame%2],1000+frame);
            efLib_LoadKind=frame%2;++efLib_AllocationSerial;efLib_EffectCount+=frame;
        }
        CHECK(nativeEffectControl_Save(pass ? &repeat : &end,sizeof(end),&bytes));
    }
    CHECK(!memcmp(&end,&repeat,sizeof(end)));CHECK(nativeEffectControl_Load(&saved,sizeof(saved)));
#define REJECT(change) do {bad=saved;change;CHECK(!nativeEffectControl_Validate(&bad,sizeof(bad))); \
    CHECK(!nativeEffectControl_Load(&bad,sizeof(bad)));CHECK(nativeEffectControl_Save(&after,sizeof(after),&bytes)); \
    CHECK(!memcmp(&after,&saved,sizeof(saved)));++effect_control_refusals;}while(0)
    REJECT(++bad.epoch);REJECT(bad.load_kind=2);REJECT(bad.animation_count=-1);REJECT(bad.animation_count=33);
    REJECT(bad.parameters[1].gobj=(void*)1);REJECT(bad.objects[1].generation=0);
    REJECT(bad.animations[1]=(void*)1);REJECT(++bad.joints[1].offset);
    REJECT(bad.joint_types[1]=&test_class);REJECT(bad.joint_types[31]=&stage_joint_class);
#undef REJECT
    CHECK(nativeMatchPool_Free(&nativeGObjPool,roots[1]));efLib_EffectCount=777;
    CHECK(!nativeEffectControl_Load(&saved,sizeof(saved)));CHECK(efLib_EffectCount==777);
    CHECK(nativeMatchSnapshotPool_Load(&nativeGObjPool,stage_objects_saved,ob));CHECK(nativeEffectControl_Load(&saved,sizeof(saved)));
    HSD_CLASS_METHOD(models[1])=&test_class;CHECK(!nativeEffectControl_Load(&saved,sizeof(saved)));
    CHECK(nativeClass_Load(stage_graph_saved,cb));CHECK(nativeEffectControl_Load(&saved,sizeof(saved)));
    CHECK(!nativeEffectControl_Save(efLib_AnimQueue,sizeof(saved),&bytes));
    CHECK(!nativeEffectControl_Save(models[1],sizeof(saved),&bytes));
    CHECK(!nativeEffectControl_Save(&after,sizeof(after),(void*)&efLib_AllocationSerial));
    camera_idle=false;CHECK(!nativeEffectControl_Load(&saved,sizeof(saved)));camera_idle=true;
    nativeEffectControl_EndScene();CHECK(!nativeEffectControl_Load(&saved,sizeof(saved)));
    efLib_AnimCount=0;efLib_ParamTable[0].gobj=efLib_ParamTable[1].gobj=NULL;
    for(unsigned i=0;i<2;++i)hsdFreeMemPiece(models[i],sizeof(HSD_JObj));
    camera_idle=false;effect_control_checks=checks-first;
}
