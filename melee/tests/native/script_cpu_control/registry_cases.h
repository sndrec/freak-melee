// Exercise actual planner admission with one controlled no-op kernel. No game
// geometry, CPU choice or successful substitute for another kernel is provided.
static void registry_cases(void)
{
    for(unsigned mode=0;mode<7;++mode) {
        setup();static Item item;memset(&item,0,sizeof(item));
        NativeMatchPoolReference reference;
        HSD_GObj* object=nativeMatchPool_Allocate(&nativeGObjPool,&reference);
        CHECK(object);memset(object,0,sizeof(*object));
        object->classifier=HSD_GOBJ_CLASS_ITEM;object->user_data=&item;
        item.entity=object;item.x1C=100;heads[HSD_GOBJ_PLINK_ITEM]=object;
        switch(mode) {
        case 1: object->user_data=NULL;break;
        case 2: object->classifier=HSD_GOBJ_CLASS_FIGHTER;break;
        case 3: item.entity=NULL;break;
        case 4: CHECK(nativeMatchPool_Free(&nativeGObjPool,reference));break;
        case 5: object->next=object;break;
        }
        registry_probe=true;registry_kernel_calls=0;
        registry_probe_item=&item;registry_retire_payload=mode==6;
        NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=99},out=sentinel;
        bool accepted=nativeScriptBindingCPUControl_Invoke(&context,
            NativeScriptBinding_CPUPrepareCombatPlan,NULL,0,&out,&diagnostic);
        registry_probe=false;
        CHECK(accepted==(mode==0));
        CHECK(registry_kernel_calls==(mode==0 || mode==6?1u:0u));
        if(mode) CHECK(!memcmp(&out,&sentinel,sizeof(out)));
        else CHECK(out.type==NativeScriptType_Void);
    }
}
