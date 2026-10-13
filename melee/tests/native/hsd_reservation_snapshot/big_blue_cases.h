#include <melee/gr/grbigblue.static.h>
static unsigned big_blue_checks,big_blue_rejections;
static void test_big_blue_state(void)
{
    unsigned first=checks;camera_idle=true;
    CHECK(nativeMatchPool_Bind(&nativeGObjPool,camera_objects,sizeof(camera_objects),camera_slots,camera_free,8,sizeof(HSD_GObj),_Alignof(HSD_GObj),108));
    CHECK(nativeMatchScene_Begin());CHECK(nativeGroundPool_BeginScene(108));
    CHECK(big_blue_state.tracks[0].jobj_index==4 && big_blue_state.tracks[11].jobj_index==13);
    CHECK(big_blue_state.tracks[11].start_index==18 && big_blue_state.tracks[11].end_index==17);
    static grBb_YakumonoParam params;memset(&params,0,sizeof(params));params.x18=1;params.x140_scale=2.0f;
    big_blue_state.parameters=&params;stage_info.x6A4=NULL;stage_info.grkind=Gr_Kind_BigBlue;nativeStageHazards_BeginScene();
    size_t bytes;NativeStageHazardSnapshot saved,bad,after,end,repeat;
    CHECK(nativeStageHazards_Save(&saved,sizeof(saved),&bytes));
    for(unsigned pass=0;pass<2;++pass) {
        if(pass)CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));
        for(unsigned frame=0;frame<16;++frame) {
            big_blue_state.tracks[frame%12].delta.x+=(float)frame/7;
            big_blue_state.tracks[11].delta.z-=(float)frame;
            params.x18=(s32)(frame%2);
        }
        CHECK(nativeStageHazards_Save(pass ? &repeat : &end,sizeof(end),&bytes));
    }
    CHECK(!memcmp(&end,&repeat,sizeof(end)));CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));
    CHECK(params.x18==1 && params.x140_scale==2.0f && big_blue_state.tracks[11].delta.z==0);
#define REJECT(change) do {bad=saved;change;CHECK(!nativeStageHazards_Validate(&bad,sizeof(bad))); \
        CHECK(!nativeStageHazards_Load(&bad,sizeof(bad)));CHECK(nativeStageHazards_Save(&after,sizeof(after),&bytes)); \
        CHECK(!memcmp(&saved,&after,sizeof(saved)));++big_blue_rejections;}while(0)
    REJECT(bad.reserved=1);REJECT(bad.state.big_blue.parameters=(void*)1);
    REJECT(++bad.state.big_blue.tracks[0].jobj_index);REJECT(++bad.state.big_blue.tracks[11].start_index);
    REJECT(++bad.state.big_blue.tracks[11].end_index);REJECT(bad.state.big_blue.tracks[11].pad=1);
#undef REJECT
    CHECK(!nativeStageHazards_Save(&params,sizeof(saved),&bytes));
    CHECK(!nativeStageHazards_Save(&after,sizeof(after),(void*)&params));
    params.x140_scale=3;params.x18=0;CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));
    CHECK(params.x18==1 && params.x140_scale==3); // unrelated parameter bytes remain separate
    camera_idle=false;CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));camera_idle=true;
    nativeStageHazards_EndScene();CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));
    CHECK(nativeGroundPool_RetireScene());nativeMatchScene_Retire();camera_idle=false;big_blue_checks=checks-first;
}
