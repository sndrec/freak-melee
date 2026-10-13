#include <melee/cm/camera.h>
#include <melee/cm/camera_state_internal.h>
#include <melee/cm/native_camera_snapshot.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <melee/gr/forward.h>

NativeMatchPool nativeGObjPool;
u8 HSD_GObj_CameraKind=4, HSD_GObj_JObjKind=5;
static bool camera_idle;
bool nativeGmLoop_SnapshotBoundary(void) { return camera_idle; }
// Resident callback bodies are endpoints, not invoked by serialization. The
// production comparison/union-tag logic still handles these exact code symbols.
s32 Camera_SetBounds(Vec4* x) { (void)x;abort(); }
s32 fn_8002F908(HSD_RectF32* x) { (void)x;abort(); }
s32 fn_8002FBA0(HSD_RectF32* x) { (void)x;abort(); }
bool fn_8015B4EC(Vec3* x) { (void)x;abort(); }
bool ftMh_MS_343_80151428(Vec3* x) { (void)x;abort(); }

static unsigned camera_checks,camera_rejections;
static HSD_GObj camera_objects[8] ATTRIBUTE_ALIGN(16);
static NativeMatchPoolSlot camera_slots[8];
static uint32_t camera_free[8];
static HSD_ClassInfo camera_class;
static void camera_init_info(void)
{
    hsdInitClassInfo(&camera_class,&hsdClass,"fixture","camera",
        sizeof(camera_class),sizeof(HSD_CObj));
}
static NativeCameraSnapshot camera_saved,camera_bad;
static u8 camera_graph_saved[262144], camera_pool_saved[8192];
static size_t camera_graph_bytes,camera_pool_bytes;

static void reject_camera(void)
{
    Camera before=game_camera;
    CameraDebugMode debug=cm_80453004;
    CameraUnkGlobals coefficients=cm_803BCCA0;
    CHECK(!nativeCamera_Validate(&camera_bad,sizeof(camera_bad)));
    CHECK(!nativeCamera_Load(&camera_bad,sizeof(camera_bad)));
    CHECK(!memcmp(&before,&game_camera,sizeof(before)));
    CHECK(!memcmp(&debug,&cm_80453004,sizeof(debug)));
    CHECK(!memcmp(&coefficients,&cm_803BCCA0,sizeof(coefficients)));
    ++camera_rejections;
}

static void test_camera_state(void)
{
    unsigned first=checks;
    CHECK(nativeMatchPool_Bind(&nativeGObjPool,camera_objects,sizeof(camera_objects),
        camera_slots,camera_free,8,sizeof(HSD_GObj),_Alignof(HSD_GObj),99));
    camera_class.head.info_init=camera_init_info;
    HSD_CObj* primary=hsdNew(&camera_class);
    HSD_CObj* auxiliary=hsdNew(&camera_class);
    CHECK(primary && auxiliary);
    memset((u8*)primary+sizeof(HSD_Class),0,sizeof(*primary)-sizeof(HSD_Class));
    memset((u8*)auxiliary+sizeof(HSD_Class),0,sizeof(*auxiliary)-sizeof(HSD_Class));
    NativeMatchPoolReference root,quake_root;
    HSD_GObj* object=nativeMatchPool_Allocate(&nativeGObjPool,&root);
    HSD_GObj* quake=nativeMatchPool_Allocate(&nativeGObjPool,&quake_root);
    CHECK(object && quake);
    memset(object,0,sizeof(*object));memset(quake,0,sizeof(*quake));
    object->classifier=0x10; object->obj_kind=HSD_GObj_CameraKind;object->hsd_obj=primary;
    quake->classifier=HSD_GOBJ_CLASS_STAGE; quake->obj_kind=HSD_GObj_JObjKind;
    memset(&game_camera,0,sizeof(game_camera));
    game_camera.gobj=object; game_camera.quake_gobj=quake;cm_804D6464=auxiliary;
    camera_idle=true;nativeCamera_BeginScene();
    size_t written;
    CHECK(nativeCamera_Save(&camera_saved,sizeof(camera_saved),&written) && written==sizeof(camera_saved));
    CHECK(nativeClass_Save(camera_graph_saved,sizeof(camera_graph_saved),&camera_graph_bytes));
    CHECK(nativeMatchSnapshotPool_Save(&nativeGObjPool,camera_pool_saved,sizeof(camera_pool_saved),&camera_pool_bytes));

    camera_bad=camera_saved;++camera_bad.epoch;reject_camera();
    camera_bad=camera_saved;camera_bad.auxiliary=(HSD_CObj*)1;reject_camera();
    camera_bad=camera_saved;++camera_bad.auxiliary_backing.offset;reject_camera();
    camera_bad=camera_saved;++camera_bad.primary_backing.epoch;reject_camera();
    camera_bad=camera_saved;camera_bad.camera.gobj=(HSD_GObj*)1;reject_camera();
    camera_bad=camera_saved;++camera_bad.primary_object.generation;reject_camera();
    camera_bad=camera_saved;camera_bad.camera.mode=(CameraType)99;reject_camera();
    camera_bad=camera_saved;camera_bad.debug.last_mode=(CameraType)-1;reject_camera();
    camera_bad=camera_saved;camera_bad.camera.x2D0.callback=(void(*)(Camera_x2D0*))1;reject_camera();
    camera_bad=camera_saved;camera_bad.camera.x341_b1_b2=3;camera_bad.camera.x344.cb=(bool(*)(Vec3*))1;reject_camera();
    camera_bad=camera_saved;camera_bad.camera.x341_b3_b4=3;camera_bad.camera.x35C.cb=(s32(*)(Vec3*))1;reject_camera();
    camera_bad=camera_saved;camera_bad.quake_object.slot=8;reject_camera();
    camera_bad=camera_saved;camera_bad.camera.quake_gobj=(HSD_GObj*)((u8*)quake+1);reject_camera();
    camera_bad=camera_saved;camera_bad.camera.quake_gobj=NULL;reject_camera();

    camera_idle=false;written=313;
    CHECK(!nativeCamera_Save(&camera_bad,sizeof(camera_bad),&written) && written==313);
    CHECK(!nativeCamera_Validate(&camera_saved,sizeof(camera_saved)));
    CHECK(!nativeCamera_Load(&camera_saved,sizeof(camera_saved)));
    camera_idle=true;
    CHECK(!nativeCamera_Save(&game_camera,sizeof(camera_saved),&written));
    CHECK(!nativeCamera_Save(primary,sizeof(camera_saved),&written));
    CHECK(!nativeCamera_Save(&camera_bad,sizeof(camera_bad),(size_t*)&camera_bad));
    CHECK(!nativeCamera_Save(&camera_bad,sizeof(camera_bad),(size_t*)&cm_804D6464));
    CHECK(!nativeCamera_Save(&camera_bad,sizeof(camera_bad)-1,&written));
    CHECK(!nativeCamera_Load(&camera_saved,sizeof(camera_saved)-1));
    u8 unaligned[sizeof(camera_saved)+1];memcpy(unaligned+1,&camera_saved,sizeof(camera_saved));
    CHECK(nativeCamera_Load(unaligned+1,sizeof(camera_saved)));

    // Valid callback tags and stale vector union bits both roundtrip. The code
    // addresses are not invoked and arbitrary function pointers still refuse.
    camera_bad=camera_saved;camera_bad.camera.mode=CAMERA_BOSS_INTRO;
    camera_bad.camera.x341_b1_b2=3;camera_bad.camera.x344.cb=fn_8015B4EC;
    camera_bad.camera.x2D0.callback=(void(*)(Camera_x2D0*))(Event)Camera_SetBounds;
    CHECK(nativeCamera_Load(&camera_bad,sizeof(camera_bad)));
    camera_bad.camera.x344.cb=ftMh_MS_343_80151428;
    camera_bad.camera.x2D0.callback=(void(*)(Camera_x2D0*))(Event)fn_8002F908;
    CHECK(nativeCamera_Load(&camera_bad,sizeof(camera_bad)));
    camera_bad.camera.x341_b1_b2=2;camera_bad.camera.x344.vec=(Vec3){3,5,8};
    camera_bad.camera.x341_b3_b4=1;camera_bad.camera.x35C.vec=(Vec3){11,13,17};
    camera_bad.camera.x2D0.callback=(void(*)(Camera_x2D0*))(Event)fn_8002FBA0;
    CHECK(nativeCamera_Load(&camera_bad,sizeof(camera_bad)));

    // The later quake generation must not cause a partial scalar restore.
    CHECK(nativeMatchPool_Free(&nativeGObjPool,quake_root));
    NativeMatchPoolReference replacement;
    CHECK(nativeMatchPool_Allocate(&nativeGObjPool,&replacement)==quake);
    CHECK(replacement.generation!=quake_root.generation);
    Camera before=game_camera;
    CHECK(nativeCamera_Validate(&camera_saved,sizeof(camera_saved)));
    CHECK(!nativeCamera_Load(&camera_saved,sizeof(camera_saved)));
    CHECK(!memcmp(&before,&game_camera,sizeof(before)));
    CHECK(nativeMatchSnapshotPool_Load(&nativeGObjPool,camera_pool_saved,camera_pool_bytes));
    // Class bytes restore separately, including camera matrices and flags.
    primary->flags=913;auxiliary->view_mtx[2][3]=57;
    HSD_CLASS_METHOD(auxiliary)=&test_class;
    CHECK(!nativeCamera_Load(&camera_saved,sizeof(camera_saved)));
    CHECK(nativeClass_Load(camera_graph_saved,camera_graph_bytes));
    CHECK(primary->flags==0 && auxiliary->view_mtx[2][3]==0);
    CHECK(nativeCamera_Load(&camera_saved,sizeof(camera_saved)));
    CHECK(nativeMatchPool_Free(&nativeGObjPool,root));
    CHECK(nativeMatchPool_Allocate(&nativeGObjPool,&replacement)==object);
    CHECK(!nativeCamera_Load(&camera_saved,sizeof(camera_saved)));
    CHECK(nativeMatchSnapshotPool_Load(&nativeGObjPool,camera_pool_saved,camera_pool_bytes));
    CHECK(nativeCamera_Load(&camera_saved,sizeof(camera_saved)));

    // A controlled scalar evolution exercises every byte, not camera gameplay.
    NativeCameraSnapshot endpoint,repeat;
    for (unsigned pass=0;pass<2;++pass) {
        if (pass) CHECK(nativeCamera_Load(&camera_saved,sizeof(camera_saved)));
        for (unsigned frame=0;frame<16;++frame) {
            game_camera.transform.position.x+=(float)frame/3;
            game_camera.transform_copy.target_interest.y+=(float)frame/7;
            game_camera.pause_eye_offset.z+=0.5f;
            game_camera.quakes[frame%2][frame].epicenter.y=(float)frame;
            Camera_SetQuakeScale((float)frame/8);
            Camera_80030730(45+(float)frame);
            cm_80453004.follow_eye_pos.z+=(float)frame;
            memset(game_camera.x380,frame,sizeof(game_camera.x380));
        }
        CHECK(nativeCamera_Save(pass ? &repeat : &endpoint,sizeof(endpoint),&written));
    }
    CHECK(!memcmp(&endpoint,&repeat,sizeof(endpoint)));
    CHECK(nativeCamera_Load(&camera_saved,sizeof(camera_saved)));
    nativeCamera_EndScene();CHECK(!nativeCamera_Load(&camera_saved,sizeof(camera_saved)));
    nativeCamera_BeginScene();CHECK(!nativeCamera_Load(&camera_saved,sizeof(camera_saved)));
    CHECK(nativeCamera_Save(&camera_bad,sizeof(camera_bad),&written));
    hsdForgetClassLibrary(NULL);
    CHECK(!nativeCamera_Load(&camera_bad,sizeof(camera_bad)));
    nativeCamera_EndScene(); camera_idle=false;
    camera_checks=checks-first;
}
