#include <melee/gr/native_stage_hazard_snapshot.h>

static unsigned hazard_checks,hazard_rejections;
static void test_hazard_state(void)
{
    unsigned first=checks;camera_idle=true;
    CHECK(nativeMatchPool_Bind(&nativeGObjPool,camera_objects,sizeof(camera_objects),
        camera_slots,camera_free,8,sizeof(HSD_GObj),_Alignof(HSD_GObj),102));
    CHECK(nativeMatchScene_Begin());CHECK(nativeGroundPool_BeginScene(102));
    NativeMatchPoolReference obj_ref,data_ref;
    HSD_GObj* object=nativeMatchPool_Allocate(&nativeGObjPool,&obj_ref);
    Ground* actor=nativeMatchPool_Allocate(&nativeGroundPool,&data_ref);
    CHECK(object && actor);memset(object,0,sizeof(*object));memset(actor,0,sizeof(*actor));
    object->classifier=HSD_GOBJ_CLASS_STAGE;object->user_data_kind=3;
    object->user_data=actor;object->user_data_remove_func=nativeGroundPool_Release;actor->gobj=object;
    static u8 parameter[4];
    GrKind kinds[]={Gr_Kind_Corneria,Gr_Kind_Venom,Gr_Kind_Greens,Gr_Kind_Kraid,Gr_Kind_Last};
    NativeStageHazardSnapshot saved,bad,after,endpoint,repeat;
    for (unsigned k=0;k<5;++k) {
        memset(&corneria_state,0,sizeof(corneria_state));memset(&venom_state,0,sizeof(venom_state));
        memset(&greens_state,0,sizeof(greens_state));memset(&kraid_state,0,sizeof(kraid_state));
        corneria_state.parameters=(void*)parameter;venom_state.parameters=(void*)parameter;
        greens_state.parameters=(void*)parameter;kraid_state.parameters=(void*)parameter;
        corneria_state.arwings[2]=venom_state.arwings[2]=object;
        corneria_state.current_slot=venom_state.current_slot=2;
        stage_info.grkind=kinds[k];stage_info.x6A4=NULL;arwing_conversation_selection=-1;
        nativeStageHazards_BeginScene();size_t bytes;
        CHECK(nativeStageHazards_Save(&saved,sizeof(saved),&bytes));CHECK(bytes==sizeof(saved));
        for (unsigned pass=0;pass<2;++pass) {
            if (pass) CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));
            for (unsigned frame=0;frame<16;++frame) {
                if(k==0) { corneria_state.spawn_timer+=(int)frame;corneria_state.types[2]++;
                    corneria_state.cannon_positions[frame%5].x+=(float)frame; }
                if(k==1) { venom_state.spawn_timer-=(int)frame;venom_state.groups[2]++; }
                if(k==2) { greens_state.block_states[frame]=(u8)frame;greens_state.block_update_flag++; }
                if(k==3) kraid_state.debug_animation+=(int)frame;
                arwing_conversation_selection++;
            }
            CHECK(nativeStageHazards_Save(pass ? &repeat : &endpoint,sizeof(endpoint),&bytes));
        }
        CHECK(!memcmp(&endpoint,&repeat,sizeof(endpoint)));CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));
#define REJECT(change) do { bad=saved;change;CHECK(!nativeStageHazards_Validate(&bad,sizeof(bad))); \
            CHECK(!nativeStageHazards_Load(&bad,sizeof(bad))); \
            CHECK(nativeStageHazards_Save(&after,sizeof(after),&bytes)); \
            CHECK(!memcmp(&saved,&after,sizeof(saved)));++hazard_rejections; } while(0)
        REJECT(++bad.epoch);REJECT(bad.kind=Gr_Kind_Count);
        REJECT(bad.objects[0].generation=1);
        if(k==0) { REJECT(bad.state.corneria.current_slot=3);
            REJECT(((u8*)&bad.state.corneria)[offsetof(CorneriaState,updates_disabled)]=2);
            REJECT(bad.state.corneria.parameters=NULL);REJECT(bad.userdata[2].generation=0); }
        if(k==1) { REJECT(bad.state.venom.current_slot=3);REJECT(bad.state.venom.parameters=NULL); }
        if(k>0) REJECT(((u8*)&bad.state)[sizeof(bad.state)-1]=1);
#undef REJECT
        camera_idle=false;CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));camera_idle=true;
        CHECK(!nativeStageHazards_Save(&corneria_state,sizeof(saved),&bytes));
        CHECK(!nativeStageHazards_Save(&bad,sizeof(bad),(size_t*)&arwing_conversation_selection));
        if(k==0) {
            size_t ob,gb;CHECK(nativeMatchSnapshotPool_Save(&nativeGObjPool,stage_objects_saved,sizeof(stage_objects_saved),&ob));
            CHECK(nativeGroundPool_Save(stage_ground_saved,sizeof(stage_ground_saved),&gb));
            nativeGroundPool_Release(actor);CHECK(nativeMatchPool_Free(&nativeGObjPool,obj_ref));
            NativeMatchPoolReference replacement;CHECK(nativeMatchPool_Allocate(&nativeGObjPool,&replacement)==object);
            CHECK(nativeStageHazards_Validate(&saved,sizeof(saved)));CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));
            CHECK(nativeMatchSnapshotPool_Load(&nativeGObjPool,stage_objects_saved,ob));
            CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));CHECK(nativeGroundPool_Load(stage_ground_saved,gb));
            CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));
            object->user_data_kind=0;CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));object->user_data_kind=3;
            corneria_state.parameters=NULL;CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));corneria_state.parameters=(void*)parameter;
        }
        // Loading an inactive-stage snapshot must leave its dormant records alone.
        if(k==4) { corneria_state.spawn_timer=777;CHECK(nativeStageHazards_Load(&saved,sizeof(saved)));
            CHECK(corneria_state.spawn_timer==777); }
        nativeStageHazards_EndScene();CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));
        nativeStageHazards_BeginScene();CHECK(!nativeStageHazards_Load(&saved,sizeof(saved)));
        nativeStageHazards_EndScene();
    }
    CHECK(nativeGroundPool_RetireScene());nativeMatchScene_Retire();camera_idle=false;hazard_checks=checks-first;
}
