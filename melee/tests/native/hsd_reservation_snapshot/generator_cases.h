#include <melee/gr/grzakogenerator.h>
#include <melee/gr/native_stage_generator_snapshot.h>
#include <sysdolphin/baselib/gobjuserdata.h>
static unsigned generator_checks,generator_rejections,generator_procs,generator_detaches;
static bool generator_fail_create;
HSD_GObj* GObj_Create(u16 classifier,u8 link,u8 priority)
{
    (void)priority;if(generator_fail_create)return NULL;
    NativeMatchPoolReference ref;HSD_GObj* object=nativeMatchPool_Allocate(&nativeGObjPool,&ref);
    if(object) {memset(object,0,sizeof(*object));object->classifier=classifier;object->p_link=link;object->user_data_kind=HSD_GOBJ_USER_DATA_NONE;}
    return object;
}
void fn_801CADBC(HSD_GObj* object) {(void)object;abort();}
HSD_GObjProc* HSD_GObj_SetupProc(HSD_GObj* object,HSD_GObjEvent callback,u8 priority)
{CHECK(object && callback==fn_801CADBC && priority==0);++generator_procs;return NULL;}
ItemKind itGetKind(Item_GObj* object) {return ((Item*)object->user_data)->kind;}
void it_8027CE18(Item_GObj* object) {CHECK(object);++generator_detaches;}
static void test_generator_state(void)
{
    unsigned first=checks;camera_idle=true;
    CHECK(nativeMatchPool_Bind(&nativeGObjPool,camera_objects,sizeof(camera_objects),camera_slots,camera_free,8,sizeof(HSD_GObj),_Alignof(HSD_GObj),109));
    CHECK(nativeMatchScene_Begin());CHECK(nativeItemPool_BeginScene(109));
    NativeStageGeneratorSnapshot pending,pending_repeat;size_t pending_bytes;
    stage_generator_state.driver=NULL;
    stage_generator_state.control.x4=NULL;
    // Dormant source table bits are preserved without following stale objects.
    stage_generator_state.table.entries[3].x4=(void*)1;
    nativeStageGenerator_BeginScene();
    CHECK(nativeStageGenerator_Save(&pending,sizeof(pending),&pending_bytes));
    HSD_GObj* driver=grZakoGenerator_801CAE04(NULL);CHECK(driver && generator_procs==1);
    CHECK(nativeStageGenerator_Load(&pending,sizeof(pending)));
    CHECK(!stage_generator_state.driver && !stage_generator_state.control.x4 && stage_generator_state.table.entries[3].x4==(void*)1);
    CHECK(nativeStageGenerator_Save(&pending_repeat,sizeof(pending_repeat),&pending_bytes));
    CHECK(!memcmp(&pending,&pending_repeat,sizeof(pending)));
    // Resume the constructed source state without creating another driver.
    stage_generator_state.driver=driver;
    stage_generator_state.control.x0=NULL;
    stage_generator_state.control.x4=&stage_generator_state.table;
    for(unsigned i=0;i<STAGE_GENERATOR_ENTRIES;++i) {
        stage_generator_state.table.entries[i].x0=i<80 ? (s16)(i+32) : -1;
        stage_generator_state.table.entries[i].x2=0;
        stage_generator_state.table.entries[i].x4=NULL;
    }
    nativeStageGenerator_BeginScene();
    CHECK(stage_generator_state.control.x4==&stage_generator_state.table);
    for(unsigned i=0;i<80;++i)CHECK(stage_generator_state.table.entries[i].x0==(s16)(i+32) && !stage_generator_state.table.entries[i].x2 && !stage_generator_state.table.entries[i].x4);
    CHECK(stage_generator_state.table.entries[80].x0==-1);
    grZakoGenerator_801CAEB0(7,220);CHECK(stage_generator_state.table.entries[80].x0==220 && stage_generator_state.table.entries[80].x8==7);
    grZakoGenerator_801CAEF0(false);CHECK(!stage_generator_state.control.xA_b0);grZakoGenerator_801CAEF0(true);
    NativeStageGeneratorSnapshot saved,bad,after,end,repeat;size_t bytes,ob,ib;
    HSD_GObj* objects[2];NativeItemPoolSlot* items[2];NativeMatchPoolReference roots[2],userdata[2];
    for(unsigned i=0;i<2;++i) {
        objects[i]=nativeMatchPool_Allocate(&nativeGObjPool,&roots[i]);items[i]=nativeMatchPool_Allocate(&nativeItemPool,&userdata[i]);CHECK(objects[i] && items[i]);
        memset(objects[i],0,sizeof(*objects[i]));memset(items[i],0,sizeof(*items[i]));objects[i]->classifier=HSD_GOBJ_CLASS_ITEM;
        objects[i]->user_data_kind=6;objects[i]->user_data=&items[i]->actor;objects[i]->user_data_remove_func=Item_OnUserDataRemove;items[i]->actor.entity=objects[i];
        items[i]->actor.kind=i ? It_Kind_Coin : It_Kind_Kuriboh;items[i]->actor.xDD4_itemVar.zako.idx=i ? 80 : 0;
        stage_generator_state.table.entries[i ? 80 : 0].x4=objects[i];
    }
    CHECK(nativeStageGenerator_Save(&saved,sizeof(saved),&bytes));
    CHECK(nativeMatchSnapshotPool_Save(&nativeGObjPool,stage_objects_saved,sizeof(stage_objects_saved),&ob));
    CHECK(nativeItemPool_Save(grid_items_saved,sizeof(grid_items_saved),&ib));
    grZakoGenerator_801CAC14(objects[0]);CHECK(!stage_generator_state.table.entries[0].x4 && stage_generator_state.table.entries[0].x2==2);
    grZakoGenerator_801CAC14(objects[1]);CHECK(!stage_generator_state.table.entries[80].x4);
    CHECK(nativeStageGenerator_Load(&saved,sizeof(saved)));
    for(unsigned pass=0;pass<2;++pass) {
        if(pass)CHECK(nativeStageGenerator_Load(&saved,sizeof(saved)));
        for(unsigned frame=0;frame<16;++frame) {stage_generator_state.table.entries[frame].x2+=(s16)frame;stage_generator_state.control.x8=(s16)frame;}
        CHECK(nativeStageGenerator_Save(pass ? &repeat : &end,sizeof(end),&bytes));
    }
    CHECK(!memcmp(&end,&repeat,sizeof(end)));CHECK(nativeStageGenerator_Load(&saved,sizeof(saved)));
#define REJECT(change) do {bad=saved;change;CHECK(!nativeStageGenerator_Validate(&bad,sizeof(bad))); \
        CHECK(!nativeStageGenerator_Load(&bad,sizeof(bad)));CHECK(nativeStageGenerator_Save(&after,sizeof(after),&bytes)); \
        CHECK(!memcmp(&saved,&after,sizeof(saved)));++generator_rejections;}while(0)
    REJECT(++bad.epoch);REJECT(bad.state.control.x8=-1);REJECT(bad.state.control.x8=81);
    REJECT(bad.state.control.x4=(void*)1);REJECT(bad.state.driver=(void*)1);REJECT(bad.driver.generation=0);
    REJECT(bad.state.table.entries[80].x4=(void*)1);REJECT(bad.items[80].pool=&nativeGObjPool);
    REJECT(bad.objects[1].generation=1);
#undef REJECT
    bad=saved;++bad.items[80].generation;stage_generator_state.table.entries[0].x2=777;
    CHECK(nativeStageGenerator_Validate(&bad,sizeof(bad)));CHECK(!nativeStageGenerator_Load(&bad,sizeof(bad)));
    CHECK(stage_generator_state.table.entries[0].x2==777);CHECK(nativeStageGenerator_Load(&saved,sizeof(saved)));
    Item_OnUserDataRemove(&items[1]->actor);NativeMatchPoolReference replacement;
    CHECK(nativeMatchPool_Allocate(&nativeItemPool,&replacement)==items[1]);CHECK(!nativeStageGenerator_Load(&saved,sizeof(saved)));
    CHECK(nativeItemPool_Load(grid_items_saved,ib));CHECK(nativeStageGenerator_Load(&saved,sizeof(saved)));
    CHECK(nativeMatchPool_Reference(&nativeGObjPool,driver,&replacement));CHECK(nativeMatchPool_Free(&nativeGObjPool,replacement));
    CHECK(nativeMatchPool_Allocate(&nativeGObjPool,&replacement)==driver);CHECK(!nativeStageGenerator_Load(&saved,sizeof(saved)));
    CHECK(nativeMatchSnapshotPool_Load(&nativeGObjPool,stage_objects_saved,ob));CHECK(nativeStageGenerator_Load(&saved,sizeof(saved)));
    CHECK(!nativeStageGenerator_Save(&stage_generator_state,sizeof(saved),&bytes));
    CHECK(!nativeStageGenerator_Save(&after,sizeof(after),(void*)&items[1]->actor));
    camera_idle=false;CHECK(!nativeStageGenerator_Load(&saved,sizeof(saved)));camera_idle=true;
    // Failed driver allocation preserves the old table, matching source failure
    // behavior. Only the source's enable/cursor reset happens before failure.
    stage_generator_state.table.entries[0].x8=991;generator_fail_create=true;
    CHECK(!grZakoGenerator_801CAE04(NULL));CHECK(stage_generator_state.table.entries[0].x8==991 && stage_generator_state.driver==driver);
    generator_fail_create=false;CHECK(nativeStageGenerator_Load(&saved,sizeof(saved)));
    grZakoGenerator_801CACB8(objects[1]);CHECK(stage_generator_state.table.entries[80].x0==-1 && !stage_generator_state.table.entries[80].x4 && generator_detaches==1);
    CHECK(nativeStageGenerator_Load(&saved,sizeof(saved)));
    nativeStageGenerator_EndScene();CHECK(!nativeStageGenerator_Load(&saved,sizeof(saved)));CHECK(nativeItemPool_RetireScene());nativeMatchScene_Retire();camera_idle=false;
    generator_checks=checks-first;
}
