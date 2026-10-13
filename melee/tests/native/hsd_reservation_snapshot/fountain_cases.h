#include <sysdolphin/baselib/tobj.h>
static unsigned fountain_checks,fountain_refusals;
static void test_fountain_state(void)
{
    unsigned first=checks;camera_idle=true;
    CHECK(nativeMatchPool_Bind(&nativeGObjPool,camera_objects,sizeof(camera_objects),camera_slots,camera_free,8,sizeof(HSD_GObj),_Alignof(HSD_GObj),111));
    CHECK(nativeMatchScene_Begin());CHECK(nativeGroundPool_BeginScene(111));
    NativeMatchPoolReference root_ref;HSD_GObj* object=nativeMatchPool_Allocate(&nativeGObjPool,&root_ref);CHECK(object);
    HSD_CObj* camera=hsdNew(&camera_class);CHECK(camera);
    memset((u8*)camera+sizeof(HSD_Class),0,sizeof(*camera)-sizeof(HSD_Class));
    memset(object,0,sizeof(*object));object->classifier=0x11;object->p_link=0x12;
    object->obj_kind=HSD_GObj_CameraKind;object->hsd_obj=camera;object->user_data_kind=3;
    object->user_data=&fountain_state.reflection;object->user_data_remove_func=nativeFountain_ReleaseReflection;
    static HSD_ImageDesc image;static u8 marker[64],params[4];
    memset(&image,0,sizeof(image));image.image_ptr=marker;image.width=80;image.height=60;image.format=4;
    fountain_state.parameters=(void*)params;fountain_state.owner=object;fountain_state.reflection.image=&image;
    memset(fountain_state.reflection.texture_matrix,0,sizeof(Mtx));
    stage_info.grkind=Gr_Kind_Izumi;stage_info.x6A4=NULL;nativeStageHazards_BeginScene();
    NativeStageHazardSnapshot saved,bad,after,end,repeat;size_t bytes,ob,cb;
    CHECK(nativeStageHazards_Save(&saved,sizeof(saved),&bytes));
    CHECK(nativeMatchSnapshotPool_Save(&nativeGObjPool,stage_objects_saved,sizeof(stage_objects_saved),&ob));
    CHECK(nativeClass_Save(stage_graph_saved,sizeof(stage_graph_saved),&cb));
    for(unsigned pass=0;pass<2;++pass) {
        CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));
        for(unsigned frame=0;frame<16;++frame)
            fountain_state.reflection.texture_matrix[frame%3][frame%4]+=(float)frame/7;
        CHECK(nativeStageHazards_Save(pass ? &repeat : &end,sizeof(end),&bytes));
    }
    CHECK(!memcmp(&end,&repeat,sizeof(end)));CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));
#define REJECT(change) do {bad=saved;change;CHECK(!nativeStageHazards_Validate(&bad,sizeof(bad))); \
    CHECK(!nativeStageHazards_Load(&bad,sizeof(bad)));CHECK(nativeStageHazards_Save(&after,sizeof(after),&bytes)); \
    CHECK(!memcmp(&after,&saved,sizeof(saved)));++fountain_refusals;}while(0)
    REJECT(bad.state.fountain.parameters=NULL);REJECT(bad.state.fountain.owner=NULL);
    REJECT(bad.state.fountain.reflection.image=(void*)1);REJECT(bad.userdata[0].generation=1);
    REJECT(++bad.reflection_camera.backing.offset);REJECT(bad.reflection_camera.type=&stage_joint_class);
#undef REJECT
    fountain_state.reflection.texture_matrix[2][3]=777;object->user_data_remove_func=NULL;
    CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));CHECK(fountain_state.reflection.texture_matrix[2][3]==777);
    CHECK(nativeMatchSnapshotPool_Load(&nativeGObjPool,stage_objects_saved,ob));CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));
    nativeFountain_ReleaseReflection(&fountain_state.reflection);CHECK(!fountain_state.owner);
    CHECK(nativeMatchPool_Free(&nativeGObjPool,root_ref));CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));
    CHECK(nativeMatchSnapshotPool_Load(&nativeGObjPool,stage_objects_saved,ob));CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));
    HSD_CLASS_METHOD(camera)=&test_class;CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));
    CHECK(nativeClass_Load(stage_graph_saved,cb));CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));
    ++image.width;CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));--image.width;
    CHECK(!nativeStageHazards_Save(&image,sizeof(saved),&bytes));
    CHECK(!nativeStageHazards_Save(camera,sizeof(saved),&bytes));
    CHECK(!nativeStageHazards_Save(&after,sizeof(after),(void*)&fountain_state));
    marker[0]=231;CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));CHECK(marker[0]==231);
    camera_idle=false;CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));camera_idle=true;
    nativeStageHazards_EndScene();CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));
    hsdFreeMemPiece(camera,sizeof(*camera));CHECK(nativeGroundPool_RetireScene());nativeMatchScene_Retire();
    camera_idle=false;fountain_checks=checks-first;
}
