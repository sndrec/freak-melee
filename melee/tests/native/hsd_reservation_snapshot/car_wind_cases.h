#include <melee/gr/grmutecity.h>
// Actual Item and dynamics companions, controlled stage/GObj construction.
static unsigned car_wind_checks,car_wind_rejections;
static void test_car_wind_state(void)
{
    unsigned first=checks;camera_idle=true;
    CHECK(nativeMatchPool_Bind(&nativeGObjPool,camera_objects,sizeof(camera_objects),camera_slots,camera_free,8,sizeof(HSD_GObj),_Alignof(HSD_GObj),106));
    CHECK(nativeMatchScene_Begin());CHECK(nativeGroundPool_BeginScene(106));CHECK(nativeItemPool_BeginScene(106));
    HSD_GObj* objects[2];NativeItemPoolSlot* items[2];NativeMatchPoolReference roots[2],userdata[2];
    memset(&mute_city_state,0,sizeof(mute_city_state));
    static u8 params[4];mute_city_state.parameters=(void*)params;
    for(unsigned i=0;i<MUTE_CITY_CAR_COUNT;++i) {mute_city_state.order[i]=(s32)i;mute_city_state.cars[i].x0=(float)i/30;}
    // Ascending positions force the boundary case: each newly inserted car
    // travels to index zero. The real sorter must retain all thirty identities.
    grMuteCity_801F1328();
    for(unsigned i=0;i<MUTE_CITY_CAR_COUNT;++i)CHECK(mute_city_state.order[i]==(s32)(29-i));
    for(unsigned i=0;i<2;++i) {
        objects[i]=nativeMatchPool_Allocate(&nativeGObjPool,&roots[i]);
        items[i]=nativeMatchPool_Allocate(&nativeItemPool,&userdata[i]);CHECK(objects[i] && items[i]);
        memset(objects[i],0,sizeof(*objects[i]));memset(items[i],0,sizeof(*items[i]));
        objects[i]->classifier=HSD_GOBJ_CLASS_ITEM;objects[i]->user_data_kind=6;
        objects[i]->user_data=&items[i]->actor;objects[i]->user_data_remove_func=Item_OnUserDataRemove;
        items[i]->actor.entity=objects[i];mute_city_state.cars[i ? 29 : 0].material=objects[i];
    }
    stage_info.x6A4=NULL;stage_info.grkind=Gr_Kind_MuteCity;nativeStageHazards_BeginScene();
    size_t bytes,ob,ib;CHECK(nativeMatchSnapshotPool_Save(&nativeGObjPool,stage_objects_saved,sizeof(stage_objects_saved),&ob));
    CHECK(nativeItemPool_Save(grid_items_saved,sizeof(grid_items_saved),&ib));
    NativeStageHazardSnapshot saved,bad,after,end,repeat;
    CHECK(nativeStageHazards_Save(&saved,sizeof(saved),&bytes));
    for(unsigned pass=0;pass<2;++pass) {
        if(pass)CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));
        for(unsigned frame=0;frame<16;++frame) {
            mute_city_state.cars[frame].x10+=(float)frame;
            mute_city_state.cars[29].x8+=(float)frame/7;
            s32 tmp=mute_city_state.order[frame];mute_city_state.order[frame]=mute_city_state.order[29];mute_city_state.order[29]=tmp;
            mute_city_state.collision_disabled=(s32)(frame%2);
        }
        CHECK(nativeStageHazards_Save(pass ? &repeat : &end,sizeof(end),&bytes));
    }
    CHECK(!memcmp(&end,&repeat,sizeof(end)));CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));
#define REJECT(change) do { bad=saved;change;CHECK(!nativeStageHazards_Validate(&bad,sizeof(bad))); \
        CHECK(!nativeStageHazards_Load(&bad,sizeof(bad)));CHECK(nativeStageHazards_Save(&after,sizeof(after),&bytes)); \
        CHECK(!memcmp(&after,&saved,sizeof(saved)));++car_wind_rejections;}while(0)
    REJECT(bad.state.mute_city.order[29]=30);REJECT(bad.state.mute_city.order[29]=-1);
    REJECT(bad.state.mute_city.order[29]=bad.state.mute_city.order[0]);
    REJECT(bad.objects[29].generation=0);REJECT(bad.userdata[29].pool=&nativeGroundPool);
    REJECT(bad.state.mute_city.cars[29].material=(void*)1);REJECT(bad.wind.epoch=1);
#undef REJECT
    bad=saved;++bad.userdata[29].generation;mute_city_state.cars[0].x10=777;
    CHECK(nativeStageHazards_Validate(&bad,sizeof(bad)));CHECK(!nativeStageHazards_Load(&bad,sizeof(bad)));
    CHECK(mute_city_state.cars[0].x10==777);CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));
    CHECK(nativeMatchPool_Free(&nativeGObjPool,roots[1]));NativeMatchPoolReference replacement;
    CHECK(nativeMatchPool_Allocate(&nativeGObjPool,&replacement)==objects[1]);
    CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));CHECK(nativeMatchSnapshotPool_Load(&nativeGObjPool,stage_objects_saved,ob));
    Item_OnUserDataRemove(&items[1]->actor);CHECK(nativeMatchPool_Allocate(&nativeItemPool,&replacement)==items[1]);
    CHECK(nativeStageHazards_Validate(&saved,sizeof(saved)));CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));
    CHECK(nativeItemPool_Load(grid_items_saved,ib));CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));
    objects[1]->user_data_remove_func=NULL;CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));
    CHECK(nativeMatchSnapshotPool_Load(&nativeGObjPool,stage_objects_saved,ob));
    CHECK(!nativeStageHazards_Save(&items[1]->actor,sizeof(saved),&bytes));
    nativeStageHazards_EndScene();CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));

    // Original wind pool reset shape, constructed here without stage callbacks.
    memset(&dynamics_state,0,sizeof(dynamics_state));
    for(unsigned i=0;i<DYNAMICS_NODE_COUNT;++i)dynamics_state.nodes[i].next=i+1<DYNAMICS_NODE_COUNT ? &dynamics_state.nodes[i+1] : NULL;
    for(unsigned i=0;i<WIND_FIELD_COUNT;++i)dynamics_state.wind[i].next=i+1<WIND_FIELD_COUNT ? &dynamics_state.wind[i+1] : NULL;
    dynamics_state.free_nodes=&dynamics_state.nodes[0];
    dynamics_state.active_wind=&dynamics_state.wind[0];dynamics_state.wind[0].next=NULL;
    dynamics_state.free_wind=&dynamics_state.wind[1];nativeDynamics_BeginScene();
    CHECK(castle_state.wind_angle_step==0.017453292f && castle_state.strong_wind==1);
    castle_state.parameters=(void*)params;castle_state.wind=&dynamics_state.wind[0];
    stage_info.grkind=Gr_Kind_Castle;nativeStageHazards_BeginScene();
    NativeDynamicsSnapshot wind_saved;size_t wb;
    CHECK(nativeDynamics_Save(&wind_saved,sizeof(wind_saved),&wb));
    CHECK(nativeStageHazards_Save(&saved,sizeof(saved),&bytes));
    castle_state.wind_angle_step=-0.017453292f;castle_state.strong_wind=0;dynamics_state.wind[0].unk_scale=0.9f;
    CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));CHECK(castle_state.strong_wind==1);
    CHECK(dynamics_state.wind[0].unk_scale==0.9f); // no duplicated wind payload
    CHECK(nativeDynamics_Load(&wind_saved,wb));CHECK(dynamics_state.wind[0].unk_scale==0);
#define REJECT(change) do { bad=saved;change;CHECK(!nativeStageHazards_Validate(&bad,sizeof(bad))); \
        CHECK(!nativeStageHazards_Load(&bad,sizeof(bad)));CHECK(nativeStageHazards_Save(&after,sizeof(after),&bytes)); \
        CHECK(!memcmp(&after,&saved,sizeof(saved)));++car_wind_rejections;}while(0)
    REJECT(bad.state.castle.wind=(void*)1);REJECT(++bad.wind.epoch);REJECT(bad.wind.slot=WIND_FIELD_COUNT);
    REJECT(bad.wind.reserved=1);REJECT(bad.state.castle.strong_wind=2);REJECT(bad.state.castle.parameters=NULL);
#undef REJECT
    CHECK(!nativeStageHazards_Save(&dynamics_state,sizeof(saved),&bytes));
    camera_idle=false;CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));camera_idle=true;
    nativeDynamics_EndScene();CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));
    nativeStageHazards_EndScene();CHECK(nativeItemPool_RetireScene());CHECK(nativeGroundPool_RetireScene());nativeMatchScene_Retire();
    camera_idle=false;car_wind_checks=checks-first;
}
