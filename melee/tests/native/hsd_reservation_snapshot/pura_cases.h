static unsigned pura_checks,pura_rejections;
static void test_pura_state(void)
{
    unsigned first=checks;camera_idle=true;
    CHECK(nativeMatchPool_Bind(&nativeGObjPool,camera_objects,sizeof(camera_objects),camera_slots,camera_free,8,sizeof(HSD_GObj),_Alignof(HSD_GObj),107));
    CHECK(nativeMatchScene_Begin());CHECK(nativeGroundPool_BeginScene(107));
    CHECK(pura_state.maps[0].joint.x==20 && pura_state.maps[40].joint.x==495);
    CHECK(pura_state.maps[41].joint.x==-1 && pura_state.maps[42].joint.x==0 && pura_state.maps[43].joint.x==0);
    HSD_JObj* models[3];
    for(unsigned i=0;i<3;++i) {models[i]=hsdNew(&stage_joint_class);CHECK(models[i]);memset((u8*)models[i]+sizeof(HSD_Class),0,sizeof(*models[i])-sizeof(HSD_Class));}
    static u8 parameters[4];pura_state.parameters=parameters;
    for(unsigned i=0;i<41;++i)pura_state.maps[i].jobj=models[i%3];
    // Source only updates the first 42 entries. The unused tail's pointer bits
    // are copied but are not followed or promoted to live graph references.
    pura_state.maps[43].jobj=(void*)42;
    memset(stage_info.x280,0,sizeof(stage_info.x280));stage_info.x280[0]=models[0];
    stage_info.x6A4=NULL;stage_info.grkind=Gr_Kind_Pura;nativeStageHazards_BeginScene();
    size_t bytes,cb;NativeStageHazardSnapshot saved,bad,after,end,repeat;
    CHECK(nativeClass_Save(stage_graph_saved,sizeof(stage_graph_saved),&cb));
    CHECK(nativeStageHazards_Save(&saved,sizeof(saved),&bytes));
    for(unsigned pass=0;pass<2;++pass) {
        if(pass)CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));
        for(unsigned frame=0;frame<16;++frame)pura_state.maps[frame].jobj=models[(frame+1)%3];
        CHECK(nativeStageHazards_Save(pass ? &repeat : &end,sizeof(end),&bytes));
    }
    CHECK(!memcmp(&end,&repeat,sizeof(end)));CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));
    CHECK(pura_state.maps[43].jobj==(void*)42);
#define REJECT(change) do {bad=saved;change;CHECK(!nativeStageHazards_Validate(&bad,sizeof(bad))); \
        CHECK(!nativeStageHazards_Load(&bad,sizeof(bad)));CHECK(nativeStageHazards_Save(&after,sizeof(after),&bytes)); \
        CHECK(!memcmp(&saved,&after,sizeof(saved)));++pura_rejections;}while(0)
    REJECT(bad.state.pura.parameters=NULL);REJECT(++bad.state.pura.maps[0].joint.x);
    REJECT(++bad.state.pura.maps[40].joint.y);REJECT(++bad.state.pura.maps[40].joint.z);
    REJECT(bad.state.pura.maps[43].x06=1);REJECT(bad.state.pura.maps[40].jobj=(void*)1);
    REJECT(bad.joints[40].backing.reserved=1);REJECT(++bad.joints[40].backing.offset);
    REJECT(bad.joints[40].type=(void*)1);REJECT(bad.joints[43]=bad.joints[40]);
#undef REJECT
    // A late restored-type mismatch refuses before an earlier mapping writes.
    pura_state.maps[0].jobj=models[1];HSD_CLASS_METHOD(models[1])=(void*)1;
    CHECK(nativeStageHazards_Validate(&saved,sizeof(saved)));CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));
    CHECK(pura_state.maps[0].jobj==models[1]);CHECK(nativeClass_Load(stage_graph_saved,cb));
    CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));CHECK(pura_state.maps[0].jobj==models[0]);
    CHECK(!nativeStageHazards_Save(models[1],sizeof(saved),&bytes));
    CHECK(!nativeStageHazards_Save(&after,sizeof(after),(void*)&pura_state));
    camera_idle=false;CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));camera_idle=true;
    nativeStageHazards_EndScene();CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));
    // Identity fields are frozen by the next initialization lease, not an
    // embedded old format or a hard-coded pointer table in the serializer.
    pura_state.maps[0].joint.x=21;nativeStageHazards_BeginScene();
    CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));CHECK(nativeStageHazards_Save(&after,sizeof(after),&bytes));
    nativeStageHazards_EndScene();
    battlefield_state.parameters=(void*)parameters;battlefield_state.is_demo=0;
    stage_info.grkind=Gr_Kind_Battle;nativeStageHazards_BeginScene();
    CHECK(nativeStageHazards_Save(&saved,sizeof(saved),&bytes));
    battlefield_state.is_demo=1;CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));CHECK(!battlefield_state.is_demo);
    bad=saved;bad.state.battlefield.is_demo=2;CHECK(!nativeStageHazards_Validate(&bad,sizeof(bad)));++pura_rejections;
    nativeStageHazards_EndScene();CHECK(nativeGroundPool_RetireScene());nativeMatchScene_Retire();camera_idle=false;pura_checks=checks-first;
}
