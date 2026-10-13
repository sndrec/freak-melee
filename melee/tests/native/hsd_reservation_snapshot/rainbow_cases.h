#include <melee/gr/grrcruise.h>
#include <melee/gr/grrcruise_data.h>
#include <melee/gr/stage.h>
#include <melee/mp/mplib.h>
#include <melee/mp/types.h>
static unsigned rainbow_checks,rainbow_refusals,rainbow_collision_calls,rainbow_random_calls;
static HSD_JObj* rainbow_models[RAINBOW_CRUISE_ENTRIES+RAINBOW_CRUISE_VANISH];
extern s16 grRc_803E4FF0[];
// Source-private callback exported only by the extracted fixture translation unit.
void grRCruise_802010A4(void*,int,CollData*,int,mpLib_GroundEnum,float);
HSD_JObj* Ground_801C3FA4(HSD_GObj* object,s32 index)
{
    CHECK(object && object->user_data);
    for(unsigned i=0;i<RAINBOW_CRUISE_ENTRIES;++i)
        if(grRc_803E4FF0[i]==index)return rainbow_models[i];
    abort();
}
s32 Ground_801C32D4(s32 map,s32 index) {CHECK(map==1);return index;}
float Stage_GetCamBoundsBottomOffset(void) {return -4.0f;}
s32 HSD_Randi(s32 range) {CHECK(range>0);++rainbow_random_calls;return 0;}
void HSD_JObjSetMtxDirtySub(HSD_JObj* model) {model->flags|=JOBJ_MTX_DIRTY;}
void (HSD_JObjSetMtxDirty)(HSD_JObj* model) {HSD_JObjSetMtxDirtySub(model);}
void grRCruise_80201B60(HSD_JObj* model,bool visible)
{CHECK(model);if(visible)model->flags&=~JOBJ_HIDDEN;else model->flags|=JOBJ_HIDDEN;}
void mpLib_80055E9C(int index) {CHECK(index>=0);++rainbow_collision_calls;}
void mpLib_80057424(int index) {CHECK(index>=0);++rainbow_collision_calls;}
void mpLib_80057BC0(int index) {CHECK(index>=0);++rainbow_collision_calls;}
void mpJointListAdd(int index) {CHECK(index>=0);++rainbow_collision_calls;}
static void test_rainbow_state(void)
{
    unsigned first=checks;camera_idle=true;
    CHECK(nativeMatchPool_Bind(&nativeGObjPool,camera_objects,sizeof(camera_objects),camera_slots,camera_free,8,sizeof(HSD_GObj),_Alignof(HSD_GObj),110));
    CHECK(nativeMatchScene_Begin());CHECK(nativeGroundPool_BeginScene(110));
    for(unsigned i=0;i<RAINBOW_CRUISE_ENTRIES+RAINBOW_CRUISE_VANISH;++i) {
        rainbow_models[i]=hsdNew(&stage_joint_class);CHECK(rainbow_models[i]);
        memset((u8*)rainbow_models[i]+sizeof(HSD_Class),0,sizeof(HSD_JObj)-sizeof(HSD_Class));
        rainbow_models[i]->translate.y=2.0f;
    }
    NativeMatchPoolReference object_ref,ground_ref;
    HSD_GObj* object=nativeMatchPool_Allocate(&nativeGObjPool,&object_ref);
    Ground* ground=nativeMatchPool_Allocate(&nativeGroundPool,&ground_ref);CHECK(object && ground);
    memset(object,0,sizeof(*object));memset(ground,0,sizeof(*ground));
    object->classifier=HSD_GOBJ_CLASS_STAGE;object->user_data_kind=3;object->user_data=ground;
    object->user_data_remove_func=nativeGroundPool_Release;ground->gobj=object;ground->map_id=1;
    ground->u.rcruise.entries=rainbow_cruise_state.entries;ground->u.rcruise.vanish=rainbow_cruise_state.vanish;
    memset(&dynamics_state,0,sizeof(dynamics_state));
    for(unsigned i=0;i<DYNAMICS_NODE_COUNT;++i)dynamics_state.nodes[i].next=i+1<DYNAMICS_NODE_COUNT ? &dynamics_state.nodes[i+1] : NULL;
    for(unsigned i=1;i<WIND_FIELD_COUNT;++i)dynamics_state.wind[i].next=i+1<WIND_FIELD_COUNT ? &dynamics_state.wind[i+1] : NULL;
    dynamics_state.free_nodes=&dynamics_state.nodes[0];dynamics_state.active_wind=&dynamics_state.wind[0];
    dynamics_state.free_wind=&dynamics_state.wind[1];nativeDynamics_BeginScene();
    ground->u.rcruise.x4=&dynamics_state.wind[0];
    static struct grRCruise_YakumonoParam params;
    params.x20=2;params.x24=3;params.x28=1;params.x2C=0;params.x30=1;
    params.x34=1;params.x38=2;params.x3C=2;params.x40=2;params.x44=1;
    rainbow_cruise_state.parameters=&params;rainbow_cruise_state.owner=object;
    grRCruise_80200B48(object); // actual source initializer
    for(unsigned i=0;i<RAINBOW_CRUISE_ENTRIES;++i) {
        CHECK(rainbow_cruise_state.entries[i].x14==rainbow_models[i]);
        CHECK(rainbow_cruise_state.entries[i].x2==grRc_803E4FF0[i]);
        rainbow_cruise_state.entries[i].x0=1;rainbow_cruise_state.entries[i].x8=1;
    }
    for(unsigned i=0;i<RAINBOW_CRUISE_VANISH;++i) {
        rainbow_cruise_state.vanish[i].jobj=rainbow_models[RAINBOW_CRUISE_ENTRIES+i];
        rainbow_cruise_state.vanish[i].x0=i%4;
    }
    memset(stage_info.x280,0,sizeof(stage_info.x280));stage_info.x280[0]=rainbow_models[0];
    stage_info.grkind=Gr_Kind_RCruise;stage_info.x6A4=NULL;nativeStageHazards_BeginScene();
    NativeStageHazardSnapshot saved,bad,after,end,repeat;NativeDynamicsSnapshot wind_saved;
    size_t bytes,cb,ob,gb,wb;
    CHECK(nativeStageHazards_Save(&saved,sizeof(saved),&bytes));
    CHECK(nativeClass_Save(stage_graph_saved,sizeof(stage_graph_saved),&cb));
    CHECK(nativeMatchSnapshotPool_Save(&nativeGObjPool,stage_objects_saved,sizeof(stage_objects_saved),&ob));
    CHECK(nativeGroundPool_Save(stage_ground_saved,sizeof(stage_ground_saved),&gb));
    CHECK(nativeDynamics_Save(&wind_saved,sizeof(wind_saved),&wb));
    unsigned collision_count=0,random_count=0;
    CollData contact={0};contact.x34_flags.b1234=1;
    for(unsigned pass=0;pass<2;++pass) {
        CHECK(nativeClass_Load(stage_graph_saved,cb));CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));
        rainbow_collision_calls=rainbow_random_calls=0;
        for(unsigned frame=0;frame<16;++frame) {
            for(unsigned i=0;i<RAINBOW_CRUISE_ENTRIES;++i)
                grRCruise_802010A4(object,rainbow_cruise_state.entries[i].x2,&contact,0,0,0);
            grRCruise_80200C04(object); // actual source platform state machine
            rainbow_cruise_state.vanish[frame%RAINBOW_CRUISE_VANISH].x0=frame%4;
        }
        CHECK(rainbow_collision_calls && rainbow_random_calls);
        if(!pass) {collision_count=rainbow_collision_calls;random_count=rainbow_random_calls;}
        else CHECK(collision_count==rainbow_collision_calls && random_count==rainbow_random_calls);
        CHECK(nativeStageHazards_Save(pass ? &repeat : &end,sizeof(end),&bytes));
    }
    CHECK(!memcmp(&end,&repeat,sizeof(end)));
    CHECK(nativeClass_Load(stage_graph_saved,cb));CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));
#define REJECT(change) do {bad=saved;change;CHECK(!nativeStageHazards_Validate(&bad,sizeof(bad))); \
    CHECK(!nativeStageHazards_Load(&bad,sizeof(bad)));CHECK(nativeStageHazards_Save(&after,sizeof(after),&bytes)); \
    CHECK(!memcmp(&after,&saved,sizeof(saved)));++rainbow_refusals;}while(0)
    REJECT(bad.state.rainbow_cruise.parameters=NULL);REJECT(bad.state.rainbow_cruise.owner=NULL);
    REJECT(bad.state.rainbow_cruise.owner=(void*)1);REJECT(++bad.state.rainbow_cruise.entries[16].x2);
    REJECT(bad.state.rainbow_cruise.entries[16].x0=6);REJECT(bad.state.rainbow_cruise.vanish[19].x0=4);
    REJECT(bad.state.rainbow_cruise.vanish[19].jobj=NULL);REJECT(++bad.joints[36].backing.offset);
    REJECT(bad.joints[36].type=&camera_class);REJECT(bad.userdata[0].generation=0);
    REJECT(++bad.wind.epoch);REJECT(bad.wind.slot=WIND_FIELD_COUNT);
#undef REJECT
    // Restored companions must agree before either array is overwritten.
    rainbow_cruise_state.entries[0].x4=777;ground->u.rcruise.vanish=(void*)1;
    CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));CHECK(rainbow_cruise_state.entries[0].x4==777);
    CHECK(nativeGroundPool_Load(stage_ground_saved,gb));CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));
    nativeGroundPool_Release(ground);CHECK(nativeMatchPool_Free(&nativeGObjPool,object_ref));
    CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));
    CHECK(nativeMatchSnapshotPool_Load(&nativeGObjPool,stage_objects_saved,ob));CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));
    CHECK(nativeGroundPool_Load(stage_ground_saved,gb));CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));
    HSD_CLASS_METHOD(rainbow_models[36])=&test_class;CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));
    CHECK(nativeClass_Load(stage_graph_saved,cb));CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));
    CHECK(!nativeStageHazards_Save(&rainbow_cruise_state,sizeof(saved),&bytes));
    CHECK(!nativeStageHazards_Save(rainbow_models[36],sizeof(saved),&bytes));
    dynamics_state.wind[0].unk_scale=0.75f;
    CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));CHECK(dynamics_state.wind[0].unk_scale==0.75f);
    CHECK(nativeDynamics_Load(&wind_saved,wb));CHECK(dynamics_state.wind[0].unk_scale==0);
    camera_idle=false;CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));camera_idle=true;
    nativeDynamics_EndScene();CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));
    nativeStageHazards_EndScene();CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));
    for(unsigned i=0;i<RAINBOW_CRUISE_ENTRIES+RAINBOW_CRUISE_VANISH;++i)
        hsdFreeMemPiece(rainbow_models[i],sizeof(HSD_JObj));
    CHECK(nativeGroundPool_RetireScene());nativeMatchScene_Retire();camera_idle=false;rainbow_checks=checks-first;
}
