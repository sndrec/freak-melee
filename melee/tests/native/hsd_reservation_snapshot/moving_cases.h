static unsigned moving_checks,moving_rejections;
static void test_moving_state(void)
{
    unsigned first=checks;camera_idle=true;
    CHECK(nativeMatchPool_Bind(&nativeGObjPool,camera_objects,sizeof(camera_objects),
        camera_slots,camera_free,8,sizeof(HSD_GObj),_Alignof(HSD_GObj),104));
    CHECK(nativeMatchScene_Begin());CHECK(nativeGroundPool_BeginScene(104));
    stage_joint_class.head.info_init=stage_joint_init;
    HSD_JObj* models[18];
    for (unsigned i=0;i<18;++i) {
        models[i]=hsdNew(&stage_joint_class);CHECK(models[i]);
        memset((u8*)models[i]+sizeof(HSD_Class),0,sizeof(*models[i])-sizeof(HSD_Class));
    }
    HSD_GObj* objects[3];Ground* actors[3];NativeMatchPoolReference handles[3],userdata[3];
    for (unsigned i=0;i<3;++i) {
        objects[i]=nativeMatchPool_Allocate(&nativeGObjPool,&handles[i]);
        actors[i]=nativeMatchPool_Allocate(&nativeGroundPool,&userdata[i]);CHECK(objects[i] && actors[i]);
        memset(objects[i],0,sizeof(*objects[i]));memset(actors[i],0,sizeof(*actors[i]));
        objects[i]->classifier=HSD_GOBJ_CLASS_STAGE;objects[i]->user_data_kind=3;
        objects[i]->user_data=actors[i];objects[i]->user_data_remove_func=nativeGroundPool_Release;actors[i]->gobj=objects[i];
    }
    static u8 parameters[4];size_t bytes,ob,gb,cb;
    CHECK(kongo_state.links[0].unk0==8 && kongo_state.links[0].unk8==0.06981317f);
    CHECK(kongo_state.links[14].unk0==0x2D && kongo_state.links[14].unk8==0.10471976f);
    CHECK(ice_mountain_state.rows[0].gobj_id==1 && ice_mountain_state.rows[0].x4==180.0f);
    kongo_state.parameters=(void*)parameters;
    for (unsigned i=0;i<15;++i) kongo_state.links[i].unk4=models[i];
    kongo_state.banks.unk0=models[15];kongo_state.banks.unk4=models[16];
    ice_mountain_state.parameters=(void*)parameters;
    memcpy(ice_mountain_state.extra_rows,objects,sizeof(objects));
    memset(stage_info.x280,0,sizeof(stage_info.x280));stage_info.x280[260]=models[0];stage_info.x6A4=NULL;
    CHECK(nativeClass_Save(stage_graph_saved,sizeof(stage_graph_saved),&cb));
    CHECK(nativeMatchSnapshotPool_Save(&nativeGObjPool,stage_objects_saved,sizeof(stage_objects_saved),&ob));
    CHECK(nativeGroundPool_Save(stage_ground_saved,sizeof(stage_ground_saved),&gb));
    NativeStageHazardSnapshot saved,bad,after,end,repeat;
    for (unsigned kind=0;kind<2;++kind) {
        stage_info.grkind=kind ? Gr_Kind_Icemt : Gr_Kind_Kongo;nativeStageHazards_BeginScene();
        CHECK(nativeStageHazards_Save(&saved,sizeof(saved),&bytes));
        for (unsigned pass=0;pass<2;++pass) {
            if(pass)CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));
            for (unsigned frame=0;frame<16;++frame) {
                if(!kind) {
                    kongo_state.links[frame%15].unkC+=(float)frame/7;
                    kongo_state.links[frame%15].unk10-=(float)frame/3;
                    kongo_state.links[14].unk14+=(float)frame;
                } else {
                    ice_mountain_state.rows[frame%6].x4+=(float)frame;
                    ice_mountain_state.rows[frame%6].x8-=(float)frame/2;
                }
                arwing_conversation_selection++;
            }
            CHECK(nativeStageHazards_Save(pass ? &repeat : &end,sizeof(end),&bytes));
        }
        CHECK(!memcmp(&end,&repeat,sizeof(end)));CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));
#define REJECT(change) do { bad=saved;change;CHECK(!nativeStageHazards_Validate(&bad,sizeof(bad))); \
            CHECK(!nativeStageHazards_Load(&bad,sizeof(bad))); \
            CHECK(nativeStageHazards_Save(&after,sizeof(after),&bytes)); \
            CHECK(!memcmp(&saved,&after,sizeof(saved)));++moving_rejections; } while(0)
        REJECT(++bad.epoch);REJECT(bad.kind=Gr_Kind_Last);
        if(!kind) {
            REJECT(bad.state.kongo.parameters=NULL);REJECT(++bad.joints[16].backing.offset);
            REJECT(bad.joints[16].type=&camera_class);REJECT(bad.state.kongo.banks.unk4=(void*)42);
            REJECT(bad.objects[0].generation=1);
            CHECK(!nativeStageHazards_Save(models[16],sizeof(saved),&bytes));
            kongo_state.links[14].unk4=models[17];CHECK(nativeStageHazards_Save(&bad,sizeof(bad),&bytes));
            CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));
            HSD_CLASS_METHOD(models[16])=&test_class;CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));
            CHECK(nativeClass_Load(stage_graph_saved,cb));CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));
        } else {
            REJECT(bad.state.ice_mountain.parameters=NULL);REJECT(bad.objects[2].slot=8);
            REJECT(bad.userdata[2].generation=0);REJECT(bad.joints[0].type=&stage_joint_class);
            nativeGroundPool_Release(actors[2]);CHECK(nativeMatchPool_Free(&nativeGObjPool,handles[2]));
            CHECK(nativeStageHazards_Validate(&saved,sizeof(saved)));CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));
            CHECK(nativeMatchSnapshotPool_Load(&nativeGObjPool,stage_objects_saved,ob));
            CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));CHECK(nativeGroundPool_Load(stage_ground_saved,gb));
            CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));
        }
#undef REJECT
        camera_idle=false;CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));camera_idle=true;
        nativeStageHazards_EndScene();CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));
        nativeStageHazards_BeginScene();CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));nativeStageHazards_EndScene();
    }
    CHECK(nativeGroundPool_RetireScene());nativeMatchScene_Retire();camera_idle=false;moving_checks=checks-first;
}
