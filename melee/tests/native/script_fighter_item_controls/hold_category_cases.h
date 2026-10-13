// The hold category is distinct from Item kind and use category. Run the
// ordinary source compiler/actor/VM path, comparing with the actual getter.
static void hold_category_cases(void)
{
    HSD_GObj* caller=fighter(true);
    HSD_GObj* target=item(caller);
    Fighter* holder=caller->user_data;
    Item* held=target->user_data;
    holder->item_gobj=target;
    unsigned saved_hold=attributes.x0_hold_kind;
    unsigned saved_use=attributes.use_kind;
    for(unsigned category=0;category<8;++category) {
        attributes.x0_hold_kind=category;
        attributes.use_kind=(category+1)%4;
        run(caller,"HoldCategory");
        NativeScriptValue value=field(caller,"hold_category",NULL);
        CHECK(value.type==NativeScriptType_I32);
        CHECK(value.as.i64==itGetHoldKind(target));
        CHECK(value.as.i64==category);
        ++comparisons;
    }
    // An ordinary engine Item has the same query contract as an authored Item.
    HSD_GObj* ordinary=raw_item(caller);
    holder->item_gobj=ordinary;
    attributes.x0_hold_kind=4;
    run(caller,"HoldCategory");
    CHECK(field(caller,"hold_category",NULL).as.i64==itGetHoldKind(ordinary));
    ++comparisons;
    holder->item_gobj=NULL;
    run(caller,"HoldCategory");
    CHECK(field(caller,"hold_category",NULL).as.i64==0);
    ++cases;

    NativeScriptRuntimeContext c=context(caller);
    holder->item_gobj=target;
    const uint32_t queries[]={NativeScriptBinding_ItemsHoldKind,
                             NativeScriptBinding_ItemsHeldUseCategory,
                             NativeScriptBinding_ItemsHeavy};
    for(unsigned query=0;query<3;++query) {
        const ItemAttr* original=held->xCC_item_attr;
        held->xCC_item_attr=NULL;
        NativeScriptValue value=sentinel(),before=value;
        CHECK(!nativeScriptBindingFighterItems_Read(&c,queries[query],&value,&diagnostic));
        CHECK(!memcmp(&value,&before,sizeof(value)));
        held->xCC_item_attr=(ItemAttr*)original;
        ++cases;
    }
    for(unsigned bad=0;bad<7;++bad) {
        NativeScriptRuntimeContext before_context=c;
        Fighter before_holder=*holder;
        Item before_held=*held;
        u16 classifier=target->classifier;
        if(bad==0)++c.generation;
        if(bad==1)++c.scene_epoch;
        if(bad==2)c.source_class=NativeCallbackSource_Item;
        if(bad==3)target->classifier=HSD_GOBJ_CLASS_FIGHTER;
        if(bad==4)held->entity=ordinary;
        if(bad==5)held->native_removal_in_progress=true;
        if(bad==6)holder->kind=Ft_Kind_Samus;
        NativeScriptValue value=sentinel(),before=value;
        CHECK(!nativeScriptBindingFighterItems_Read(&c,NativeScriptBinding_ItemsHoldKind,&value,&diagnostic));
        CHECK(!memcmp(&value,&before,sizeof(value)));
        c=before_context;*holder=before_holder;*held=before_held;
        target->classifier=classifier;
        ++cases;
    }
    // Physical absence remains nullable even if a stale pointer is in the slot.
    holder->item_gobj=ordinary;
    HSD_GObjFree(ordinary);
    NativeScriptValue value=sentinel();
    CHECK(nativeScriptBindingFighterItems_Read(&c,NativeScriptBinding_ItemsHoldKind,&value,&diagnostic));
    CHECK(value.type==NativeScriptType_I32&&value.as.i64==0);
    ++cases;
    holder->item_gobj=NULL;
    attributes.x0_hold_kind=saved_hold;
    attributes.use_kind=saved_use;
    HSD_GObjFree(target);
    HSD_GObjFree(caller);
}
