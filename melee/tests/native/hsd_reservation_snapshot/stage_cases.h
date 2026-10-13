#include <melee/gr/native_stage_snapshot.h>
#include <melee/gr/native_ground_pool.h>
#include <melee/gr/stage.h>
#include <melee/gr/stage_start_state.h>
#include <sysdolphin/baselib/jobj.h>

// Actual Ground owner with a controlled idle scheduler; generic GObj backing
// remains the same controlled eight-slot pool used by the camera fixture.
HSD_GObj* HSD_GObj_CurrentInvokedProcGObj;
HSD_GObjProc* HSD_GObj_CurrentInvokedProc;
HSD_GObj* HSD_GObj_804D7818;
struct _unk_gobj_struct HSD_GObj_DelayedProcInfo;
bool nativeScriptRuntime_Idle(void) { return camera_idle; }
s8 HSD_GObj_FogKind=6;
static StKind fixture_stage;
StKind Stage_80225194(void) { return fixture_stage; }
static HSD_ClassInfo stage_joint_class,stage_camera_class;
static unsigned stage_checks,stage_rejections;
static NativeStageSnapshot stage_saved,stage_bad;
static u8 stage_graph_saved[262144],stage_objects_saved[8192],stage_ground_saved[131072];
static void stage_joint_init(void)
{ hsdInitClassInfo(&stage_joint_class,&hsdClass,"fixture","stage-joint",sizeof(stage_joint_class),sizeof(HSD_JObj)); }
static void stage_camera_init(void)
{ hsdInitClassInfo(&stage_camera_class,&hsdClass,"fixture","stage-camera",sizeof(stage_camera_class),sizeof(HSD_CObj)); }

static void reject_stage(void)
{
    StageInfo before=stage_info;GroundParam parameter=*stage_info.param;
    u8 updates[64];VecMtx spawns;s16 counter=stage_joint_update_counter;
    memcpy(updates,stage_joint_updates,sizeof(updates));memcpy(spawns,stage_spawn_cache,sizeof(spawns));
    CHECK(!nativeStage_Validate(&stage_bad,sizeof(stage_bad)));
    CHECK(!nativeStage_Load(&stage_bad,sizeof(stage_bad)));
    CHECK(!memcmp(&before,&stage_info,sizeof(before)));
    CHECK(!memcmp(&parameter,stage_info.param,sizeof(parameter)));
    CHECK(!memcmp(updates,stage_joint_updates,sizeof(updates)));
    CHECK(!memcmp(spawns,stage_spawn_cache,sizeof(spawns)));
    CHECK(counter==stage_joint_update_counter);++stage_rejections;
}

static void test_stage_state(void)
{
    unsigned first=checks;camera_idle=true;
    CHECK(nativeMatchPool_Bind(&nativeGObjPool,camera_objects,sizeof(camera_objects),
        camera_slots,camera_free,8,sizeof(HSD_GObj),_Alignof(HSD_GObj),101));
    CHECK(nativeMatchScene_Begin());CHECK(nativeGroundPool_BeginScene(101));
    stage_joint_class.head.info_init=stage_joint_init;
    stage_camera_class.head.info_init=stage_camera_init;
    HSD_JObj* joint=hsdNew(&stage_joint_class);
    HSD_JObj* second_joint=hsdNew(&stage_joint_class);
    HSD_CObj* camera=hsdNew(&stage_camera_class);
    CHECK(joint && second_joint && camera);
    memset((u8*)joint+sizeof(HSD_Class),0,sizeof(*joint)-sizeof(HSD_Class));
    memset((u8*)camera+sizeof(HSD_Class),0,sizeof(*camera)-sizeof(HSD_Class));
    NativeMatchPoolReference object_ref,ground_ref,fog_ref;
    HSD_GObj* object=nativeMatchPool_Allocate(&nativeGObjPool,&object_ref);
    Ground* ground=nativeMatchPool_Allocate(&nativeGroundPool,&ground_ref);
    HSD_GObj* fog=nativeMatchPool_Allocate(&nativeGObjPool,&fog_ref);
    CHECK(object && ground && fog);memset(object,0,sizeof(*object));memset(ground,0,sizeof(*ground));
    memset(fog,0,sizeof(*fog));fog->classifier=0xA;fog->obj_kind=HSD_GObj_FogKind;
    object->classifier=HSD_GOBJ_CLASS_STAGE;object->obj_kind=HSD_GObj_JObjKind;
    object->user_data_kind=3;object->user_data=ground;object->user_data_remove_func=nativeGroundPool_Release;
    ground->gobj=object;
    static GroundParam parameter;memset(&parameter,0,sizeof(parameter));parameter.y=1;
    memset(&stage_info,0,sizeof(stage_info));stage_info.grkind=0;stage_info.param=&parameter;
    stage_info.map_gobjs[63]=object;stage_info.x280[260]=joint;stage_info.x694[3]=camera;
    stage_info.x12C=fog;stage_info.x72C=object;
    memset(stage_joint_updates,0,sizeof(stage_joint_updates));stage_joint_update_counter=0;
    memset(stage_spawn_cache,0,sizeof(stage_spawn_cache));nativeStage_BeginScene();
    size_t written,graph_bytes,objects_bytes,ground_bytes;
    stage_start_state.started=false;
    CHECK(nativeStage_Save(&stage_saved,sizeof(stage_saved),&written));CHECK(written==sizeof(stage_saved));
    // Both lifecycle phases use one lease. Retiring/rebinding below still
    // invalidates the old capture, even when the fixture reuses its pool epoch.
    stage_start_state.started=true;nativeStage_BeginScene();
    CHECK(nativeStage_Validate(&stage_saved,sizeof(stage_saved)));
    CHECK(nativeStage_Load(&stage_saved,sizeof(stage_saved)));
    CHECK(!stage_start_state.started);
    CHECK(nativeClass_Save(stage_graph_saved,sizeof(stage_graph_saved),&graph_bytes));
    CHECK(nativeMatchSnapshotPool_Save(&nativeGObjPool,stage_objects_saved,sizeof(stage_objects_saved),&objects_bytes));
    CHECK(nativeGroundPool_Save(stage_ground_saved,sizeof(stage_ground_saved),&ground_bytes));
    NativeStageSnapshot end,repeat;
    for (unsigned pass=0;pass<2;++pass) {
        if (pass) CHECK(nativeStage_Load(&stage_saved,sizeof(stage_saved)));
        for (unsigned frame=0;frame<16;++frame) {
            stage_info.cam_info.cam_bounds.left-=(float)frame;
            stage_info.blast_zone.top+=(float)frame/3;
            stage_info.x6E4[frame%2]+=(int)frame;
            stage_info.x70C+=(float)frame/7;
            stage_joint_updates[frame]=1;++stage_joint_update_counter;
            stage_spawn_cache[frame%4].x+=(float)frame;
            parameter.y+=0.125f;
            parameter.xB8.r+=(u8)frame;parameter.xD8.a^=(u8)frame;
        }
        CHECK(nativeStage_Save(pass ? &repeat : &end,sizeof(end),&written));
    }
    CHECK(!memcmp(&end,&repeat,sizeof(end)));CHECK(nativeStage_Load(&stage_saved,sizeof(stage_saved)));
    stage_info.x280[260]=second_joint;
    CHECK(nativeStage_Save(&stage_bad,sizeof(stage_bad),&written));
    CHECK(nativeStage_Load(&stage_saved,sizeof(stage_saved)));CHECK(stage_info.x280[260]==joint);
    CHECK(nativeStage_Load(&stage_bad,sizeof(stage_bad)));CHECK(stage_info.x280[260]==second_joint);
    CHECK(nativeStage_Load(&stage_saved,sizeof(stage_saved)));
#define BAD(change) do { stage_bad=stage_saved;change;reject_stage(); } while(0)
    BAD(++stage_bad.epoch);BAD(++stage_bad.selected_stage);BAD(stage_bad.stage.param=NULL);
    BAD(stage_bad.stage.x6A4=joint);BAD(stage_bad.stage.grkind=-1);BAD(stage_bad.joint_updates[63]=2);
    BAD(stage_bad.map_objects[63].slot=8);BAD(stage_bad.map_userdata[63].generation=0);
    BAD(stage_bad.fog.lifetime=0);BAD(stage_bad.special_object.pool=NULL);
    BAD(stage_bad.joints[260].type=&camera_class);BAD(++stage_bad.joints[260].backing.offset);
    BAD(stage_bad.cameras[3].backing.bytes=1);BAD(stage_bad.stage.x280[260]=(void*)42);
#undef BAD
    CHECK(!nativeStage_Save(&stage_info,sizeof(stage_saved),&written));
    CHECK(!nativeStage_Save(&stage_bad,sizeof(stage_bad),(size_t*)&stage_joint_update_counter));
    CHECK(!nativeStage_Save(&stage_bad,sizeof(stage_bad)-1,&written));
    parameter.x4=7;CHECK(!nativeStage_Load(&stage_saved,sizeof(stage_saved)));parameter.x4=0;
    camera_idle=false;CHECK(!nativeStage_Load(&stage_saved,sizeof(stage_saved)));camera_idle=true;
    ++fixture_stage;CHECK(!nativeStage_Load(&stage_saved,sizeof(stage_saved)));--fixture_stage;
    // A null/new map root is legal; restoring the previous generation requires
    // companion payloads first. Validate never demands current occupancy.
    stage_info.map_gobjs[63]=NULL;stage_info.x280[260]=NULL;stage_info.x694[3]=NULL;
    CHECK(nativeStage_Load(&stage_saved,sizeof(stage_saved)));
    nativeGroundPool_Release(ground);CHECK(nativeMatchPool_Free(&nativeGObjPool,object_ref));
    NativeMatchPoolReference replacement;
    CHECK(nativeMatchPool_Allocate(&nativeGObjPool,&replacement)==object);
    CHECK(nativeStage_Validate(&stage_saved,sizeof(stage_saved)));
    CHECK(!nativeStage_Load(&stage_saved,sizeof(stage_saved)));
    CHECK(nativeMatchSnapshotPool_Load(&nativeGObjPool,stage_objects_saved,objects_bytes));
    CHECK(!nativeStage_Load(&stage_saved,sizeof(stage_saved)));
    CHECK(nativeGroundPool_Load(stage_ground_saved,ground_bytes));
    HSD_CLASS_METHOD(joint)=&test_class;CHECK(!nativeStage_Load(&stage_saved,sizeof(stage_saved)));
    CHECK(nativeClass_Load(stage_graph_saved,graph_bytes));CHECK(nativeStage_Load(&stage_saved,sizeof(stage_saved)));
    nativeStage_EndScene();CHECK(!nativeStage_Load(&stage_saved,sizeof(stage_saved)));
    nativeStage_BeginScene();CHECK(!nativeStage_Load(&stage_saved,sizeof(stage_saved)));
    CHECK(nativeStage_Save(&stage_bad,sizeof(stage_bad),&written));
    hsdForgetClassLibrary(NULL);CHECK(!nativeStage_Load(&stage_bad,sizeof(stage_bad)));
    nativeStage_EndScene();CHECK(nativeGroundPool_RetireScene());nativeMatchScene_Retire();camera_idle=false;
    stage_checks=checks-first;
}
