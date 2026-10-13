static unsigned bubble_checks,bubble_rejections;
static NativeStageHazardSnapshot bubble_saved,bubble_bad;
static void reject_bubble(void)
{
    BrinstarState before=brinstar_state;int selection=arwing_conversation_selection;
    CHECK(!nativeStageHazards_Validate(&bubble_bad,sizeof(bubble_bad)));
    CHECK(!nativeStageHazards_Load(&bubble_bad,sizeof(bubble_bad)));
    CHECK(!memcmp(&before,&brinstar_state,sizeof(before)));
    CHECK(selection==arwing_conversation_selection);++bubble_rejections;
}

static void test_bubble_state(void)
{
    unsigned first=checks;camera_idle=true;
    CHECK(nativeMatchPool_Bind(&nativeGObjPool,camera_objects,sizeof(camera_objects),
        camera_slots,camera_free,8,sizeof(HSD_GObj),_Alignof(HSD_GObj),103));
    CHECK(nativeMatchScene_Begin());CHECK(nativeGroundPool_BeginScene(103));
    stage_joint_class.head.info_init=stage_joint_init;
    HSD_JObj* models[3];
    for (unsigned i=0;i<3;++i) {
        models[i]=hsdNew(&stage_joint_class);CHECK(models[i]);
        memset((u8*)models[i]+sizeof(HSD_Class),0,sizeof(*models[i])-sizeof(HSD_Class));
    }
    NativeMatchPoolReference handles[2],userdata[2];HSD_GObj* objects[2];Ground* actors[2];
    for (unsigned i=0;i<2;++i) {
        objects[i]=nativeMatchPool_Allocate(&nativeGObjPool,&handles[i]);
        actors[i]=nativeMatchPool_Allocate(&nativeGroundPool,&userdata[i]);CHECK(objects[i] && actors[i]);
        memset(objects[i],0,sizeof(*objects[i]));memset(actors[i],0,sizeof(*actors[i]));
        objects[i]->classifier=HSD_GOBJ_CLASS_STAGE;objects[i]->user_data_kind=3;
        objects[i]->user_data=actors[i];objects[i]->user_data_remove_func=nativeGroundPool_Release;
        actors[i]->gobj=objects[i];
    }
    static u8 parameters[4];memset(&brinstar_state,0,sizeof(brinstar_state));
    brinstar_state.parameters=(void*)parameters;
    memset(stage_info.x280,0,sizeof(stage_info.x280));stage_info.x280[260]=models[0];
    stage_info.grkind=Gr_Kind_Zebes;stage_info.x6A4=NULL;
    for (unsigned i=0;i<2;++i) {
        grZe_BubbleEntry* entry=&brinstar_state.bubbles.bubbles[i ? 19 : 0];
        entry->x00_active=1;entry->x04=models[i];entry->x20_gobj=objects[i];
    }
    // Dormant aliases are not dereferenced, even when they aren't mapped memory.
    brinstar_state.bubbles.bubbles[18].x04=(void*)(uintptr_t)0x12345678;
    nativeStageHazards_BeginScene();size_t written,gb,ob,cb;
    CHECK(nativeStageHazards_Save(&bubble_saved,sizeof(bubble_saved),&written));
    CHECK(nativeGroundPool_Save(stage_ground_saved,sizeof(stage_ground_saved),&gb));
    CHECK(nativeMatchSnapshotPool_Save(&nativeGObjPool,stage_objects_saved,sizeof(stage_objects_saved),&ob));
    CHECK(nativeClass_Save(stage_graph_saved,sizeof(stage_graph_saved),&cb));
    NativeStageHazardSnapshot endpoint,repeat;
    for (unsigned pass=0;pass<2;++pass) {
        if (pass) CHECK(nativeStageHazards_Load(&bubble_saved,sizeof(bubble_saved)));
        for (unsigned frame=0;frame<16;++frame) {
            brinstar_state.bubble_cooldown-=(int)frame;brinstar_state.debris_count++;
            brinstar_state.bubbles.positions[frame%4].y+=(float)frame/3;
            brinstar_state.bubbles.bubbles[19].x02_timer++;
            brinstar_state.bubbles.bubbles[0].x18_size+=(float)frame/7;
        }
        CHECK(nativeStageHazards_Save(pass ? &repeat : &endpoint,sizeof(endpoint),&written));
    }
    CHECK(!memcmp(&endpoint,&repeat,sizeof(endpoint)));CHECK(nativeStageHazards_Load(&bubble_saved,sizeof(bubble_saved)));
#define BAD(change) do { bubble_bad=bubble_saved;change;reject_bubble(); } while(0)
    BAD(++bubble_bad.epoch);BAD(bubble_bad.kind=Gr_Kind_Venom);
    BAD(bubble_bad.state.brinstar.parameters=NULL);BAD(bubble_bad.objects[19].slot=8);
    BAD(bubble_bad.userdata[19].generation=0);BAD(bubble_bad.joints[19].type=&camera_class);
    BAD(++bubble_bad.joints[19].backing.offset);BAD(bubble_bad.joints[19].backing.bytes=1);
    BAD(bubble_bad.state.brinstar.bubbles.bubbles[19].x04=(void*)42);
    BAD(bubble_bad.joints[18]=bubble_bad.joints[19]);
#undef BAD
    CHECK(!nativeStageHazards_Save(models[0],sizeof(bubble_saved),&written));
    CHECK(!nativeStageHazards_Save(&bubble_bad,sizeof(bubble_bad),(size_t*)models[0]));
    brinstar_state.bubbles.bubbles[19].x04=models[2];
    CHECK(nativeStageHazards_Save(&bubble_bad,sizeof(bubble_bad),&written));
    CHECK(nativeStageHazards_Load(&bubble_saved,sizeof(bubble_saved)));
    // Preserve a merged donor's stale address after freeing its former graph.
    brinstar_state.bubbles.bubbles[19].x00_active=0;
    brinstar_state.bubbles.bubbles[19].x20_gobj=NULL;
    nativeGroundPool_Release(actors[1]);CHECK(nativeMatchPool_Free(&nativeGObjPool,handles[1]));
    hsdFreeMemPiece(models[1],sizeof(HSD_JObj));
    CHECK(nativeStageHazards_Save(&bubble_bad,sizeof(bubble_bad),&written));
    CHECK(bubble_bad.state.brinstar.bubbles.bubbles[19].x04==models[1]);
    CHECK(!bubble_bad.joints[19].backing.epoch);
    CHECK(nativeStageHazards_Validate(&bubble_saved,sizeof(bubble_saved)));
    CHECK(!nativeStageHazards_Load(&bubble_saved,sizeof(bubble_saved)));
    CHECK(nativeMatchSnapshotPool_Load(&nativeGObjPool,stage_objects_saved,ob));
    CHECK(nativeGroundPool_Load(stage_ground_saved,gb));
    CHECK(!nativeStageHazards_Load(&bubble_saved,sizeof(bubble_saved)));
    CHECK(nativeClass_Load(stage_graph_saved,cb));
    CHECK(nativeStageHazards_Load(&bubble_saved,sizeof(bubble_saved)));
    CHECK(HSD_CLASS_METHOD(models[1])==&stage_joint_class);
    // End/epoch/activity checks are independent of graph ownership.
    camera_idle=false;CHECK(!nativeStageHazards_Load(&bubble_saved,sizeof(bubble_saved)));camera_idle=true;
    nativeStageHazards_EndScene();CHECK(!nativeStageHazards_Load(&bubble_saved,sizeof(bubble_saved)));
    nativeStageHazards_BeginScene();CHECK(!nativeStageHazards_Load(&bubble_saved,sizeof(bubble_saved)));
    nativeStageHazards_EndScene();CHECK(nativeGroundPool_RetireScene());nativeMatchScene_Retire();camera_idle=false;
    bubble_checks=checks-first;
}
