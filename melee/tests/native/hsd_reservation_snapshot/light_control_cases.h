#include <sysdolphin/baselib/native_light_control_snapshot.h>
static HSD_ClassInfo light_fixture_class;
static void light_fixture_init(void)
{hsdInitClassInfo(&light_fixture_class,&hsdClass,"fixture","light",sizeof(light_fixture_class),sizeof(HSD_LObj));}
static void test_light_controls(void)
{
    unsigned first=checks;camera_idle=true;light_fixture_class.head.info_init=light_fixture_init;
    HSD_LObj* light=hsdNew(&light_fixture_class);CHECK(light);
    HSD_ClassInfo* type=HSD_CLASS_METHOD(light);memset(light,0,sizeof(*light));HSD_CLASS_METHOD(light)=type;
    HSD_OBJ(light)->ref_count=1;HSD_ListInitAllocData();
    light_current_list=NULL;memset(light_active,0,sizeof(light_active));light_active_count=0;
    HSD_LObjAddCurrent(light);CHECK(light_current_list && HSD_OBJ(light)->ref_count==2);
    light_active[0]=light;light_active_count=1;light_diffuse_mask=3;light_specular_mask=4;light_alpha_mask=5;
    NativeLightControlSnapshot saved,after,bad;size_t bytes,ordinary,classes;
    CHECK(nativeLightControl_Save(&saved,sizeof(saved),&bytes));
    CHECK(nativeObjAlloc_Save(particle_payloads_saved,sizeof(particle_payloads_saved),&ordinary));
    CHECK(nativeClass_Save(stage_graph_saved,sizeof(stage_graph_saved),&classes));
    HSD_LObjDeleteCurrent(light);CHECK(!light_current_list && HSD_OBJ(light)->ref_count==1);
    light_active[0]=NULL;light_active_count=0;light_diffuse_mask=99;
    CHECK(nativeObjAlloc_Load(particle_payloads_saved,ordinary));CHECK(nativeClass_Load(stage_graph_saved,classes));
    CHECK(nativeLightControl_Load(&saved,sizeof(saved)));
    CHECK(light_current_list->data==light && HSD_OBJ(light)->ref_count==2 && light_active[0]==light && light_diffuse_mask==3);
    CHECK(nativeLightControl_Save(&after,sizeof(after),&bytes));CHECK(!memcmp(&saved,&after,sizeof(saved)));
#define REJECT(change) do {bad=saved;change;CHECK(!nativeLightControl_Validate(&bad,sizeof(bad)));CHECK(!nativeLightControl_Load(&bad,sizeof(bad)));}while(0)
    REJECT(bad.state.current=(void*)1);REJECT(++bad.list.slot);REJECT(bad.state.count=MAX_GXLIGHT);
    REJECT(bad.state.definition=(void*)1);REJECT(bad.state.active[0]=(void*)1);REJECT(++bad.lights[0].offset);
    REJECT(bad.types[0]=&stage_joint_class);REJECT(bad.lights[0].bytes=1);
#undef REJECT
    CHECK(!nativeLightControl_Save(light_active,sizeof(saved),&bytes));
    CHECK(!nativeLightControl_Save(light,sizeof(saved),&bytes));
    CHECK(!nativeLightControl_Save(&after,sizeof(after),(void*)&light_active_count));
    camera_idle=false;CHECK(!nativeLightControl_Load(&saved,sizeof(saved)));camera_idle=true;
    HSD_LObjDeleteCurrent(light);CHECK(!light_current_list && HSD_OBJ(light)->ref_count==1);
    memset(light_active,0,sizeof(light_active));light_active_count=0;hsdFreeMemPiece(light,sizeof(*light));
    fprintf(stderr,"light-controls bytes=%zu checks=%u source-list/refcount restoration passed\n",sizeof(saved),checks-first);
    camera_idle=false;
}
