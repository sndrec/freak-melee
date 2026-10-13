#include <melee/ft/native_script_bindings.h>

static void clone_cases(Item_GObj* object,OSHeapHandle heap)
{
    NativeScriptRuntimeContext context={.source=object,.source_class=NativeCallbackSource_Item,
        .generation=((Item*)object->user_data)->x1C,.scene_epoch=nativeGObjPool.lifetime};
    NativeScriptValue arg={.type=NativeScriptType_I32,.as.i64=1},out={0};
    CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_ItemCloneModel,NULL,0,&arg,1,&out,&diagnostic));
    CHECK(out.type==NativeScriptType_Bool&&!out.as.u64);++cases;
    Item* item_data=object->user_data;
    const HSD_Joint* descriptor=item_data->xC4_article_data->x10_modelDesc->x0_joint;
    HSD_JObj* primary=HSD_JObjLoadJoint((HSD_Joint*)descriptor);CHECK(primary);
    HSD_GObjObject_80390A70(object,HSD_GObj_JObjKind,primary);
    Quaternion rotation={0.25f,-0.5f,0.75f,0};Vec3 scale={8,9,10},translation={5,6,7};
    HSD_JObjSetRotation(primary,&rotation);HSD_JObjSetScale(primary,&scale);HSD_JObjSetTranslate(primary,&translation);
    item_data->scl=2.5f;
    ssize_t heap_before=OSCheckHeap(heap);
    HSD_JObj* instances[3]={0};NativeArticleModelSceneSlot retained[3];
    for(unsigned slot=1;slot<=3;++slot) {
        arg.as.i64=slot;
        CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_ItemCloneModel,NULL,0,&arg,1,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_Bool&&out.as.u64);++graphs;
        instances[slot-1]=item_data->native_models[slot-1];
        HSD_JObj* instance=instances[slot-1];CHECK(instance&&instance!=primary);
        for(unsigned earlier=0;earlier<slot-1;++earlier)CHECK(instance!=instances[earlier]);
        CHECK(!memcmp(&instance->rotate,&rotation,sizeof(rotation)));
        CHECK(instance->scale.x==2.5f&&instance->scale.y==2.5f&&instance->scale.z==2.5f);
        CHECK(!memcmp(&instance->translate,&descriptor->position,sizeof(Vec3)));
        if(primary->child)CHECK(instance->child&&instance->child!=primary->child);
        CHECK(nativeArticleModelScene_CaptureSlot(object,slot,&retained[slot-1]));
        CHECK(retained[slot-1].root==instance&&retained[slot-1].descriptor==descriptor&&retained[slot-1].root_index==NATIVE_ARTICLE_MODEL_PRIMARY_ROOT);
        CHECK(nativeArticleModelScene_SlotCurrent(&retained[slot-1])&&OSCheckHeap(heap)==heap_before);
        CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_ItemCloneModel,NULL,0,&arg,1,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_Bool&&!out.as.u64&&item_data->native_models[slot-1]==instance);
        CHECK(OSCheckHeap(heap)==heap_before);
        CHECK(nativeArticleModelScene_BeginMatrices(object,slot));matrix(instance);CHECK(nativeArticleModelScene_EndMatrices());
        ++cases;
    }
    for(unsigned slot=1;slot<=3;++slot) {
        nativeArticleModel_Destroy(object,slot);CHECK(!nativeArticleModelScene_SlotCurrent(&retained[slot-1]));
        CHECK(!item_data->native_models[slot-1]&&OSCheckHeap(heap)==heap_before);
    }
    uint32_t signal=2166136261u;const char* name="CloneModel";
    for(unsigned i=0;name[i];++i){signal^=(uint8_t)name[i];signal*=16777619u;}
    CHECK(nativeScriptItem_RunSignal(object,signal,0,&out)==NativeScriptVM_Ok);
    CHECK(item_data->native_models[2]&&item_data->native_models[2]!=primary);++graphs;++cases;
    CHECK(OSCheckHeap(heap)==heap_before);
    CHECK(!memcmp(&primary->rotate,&rotation,sizeof(rotation)));
    CHECK(!memcmp(&primary->scale,&scale,sizeof(scale))&&!memcmp(&primary->translate,&translation,sizeof(translation)));
    HSD_GObjFree(object);
}

static void capacity_cases(OSHeapHandle heap)
{
    enum { OWNERS=(NATIVE_ARTICLE_MODEL_SCENE_CAPACITY+2)/3 };
    Item_GObj* actors[OWNERS];unsigned admitted=0;
    ssize_t before=OSCheckHeap(heap);
    for(unsigned i=0;i<OWNERS;++i) {
        actors[i]=item(i+2);
        for(unsigned slot=1;slot<=3;++slot) {
            bool loaded=nativeArticleModelScene_Load(actors[i],slot,0);
            CHECK(loaded==(admitted<NATIVE_ARTICLE_MODEL_SCENE_CAPACITY));
            if(loaded){++admitted;++graphs;}
            else CHECK(!((Item*)actors[i]->user_data)->native_models[slot-1]);
        }
    }
    CHECK(admitted==NATIVE_ARTICLE_MODEL_SCENE_CAPACITY&&OSCheckHeap(heap)==before);
    HSD_JObj* old=((Item*)actors[0]->user_data)->native_models[0];
    CHECK(nativeArticleModelScene_Load(actors[0],1,0));++graphs;
    CHECK(((Item*)actors[0]->user_data)->native_models[0]!=old&&OSCheckHeap(heap)==before);
    for(unsigned i=0;i<OWNERS;++i)HSD_GObjFree(actors[i]);
    CHECK(OSCheckHeap(heap)==before);++cases;
}
