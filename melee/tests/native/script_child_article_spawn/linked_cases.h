static void linked_cases(void)
{
    NativeScriptRuntimeContext c;parent(&c);Vec3 point={1.25f,-0.0f,3.75f};
    for(unsigned slot=0;slot<4;++slot)for(unsigned back=0;back<4;++back) {
        reset_records();child_call=true;linked_slot=slot;backlink_index=back;Item_GObj* child=(void*)1;
        CHECK(nativeScriptItemSpawn_Child(&c,programs[1].owner,&point,-1,777,0,7.125f,back,slot,&child,&diagnostic)==NativeScriptVM_Ok&&child);
        CHECK(spawned_checks==1&&created_checks==1);
        CHECK(((Item*)parent_item->user_data)->native_article_links[slot]==((Item*)child->user_data)->x1C);
        CHECK(((Item*)child->user_data)->native_article_links[back]==parent_id);
        reset_records();Item_GObj* rejected=(void*)1;
        CHECK(nativeScriptItemSpawn_Child(&c,programs[1].owner,&point,-1,777,0,0,back,slot,&rejected,&diagnostic)==NativeScriptVM_Ok&&!rejected);
        CHECK(!spawn_order_count);remove_item(child);++cases;
    }
    // Real source -> compiler -> binding -> constructor -> created callback.
    reset_records();child_call=true;linked_slot=0;backlink_index=1;expected_creation=0;
    NativeScriptValue result;CHECK(nativeScriptItem_RunSignal(parent_item,public_signal("public_link"),0,&result)==NativeScriptVM_Ok);
    Item_GObj* child=find_item(programs[1].owner);CHECK(child&&created_checks==1);
    CHECK(((Item*)parent_item->user_data)->native_article_links[0]==((Item*)child->user_data)->x1C);
    CHECK(((Item*)child->user_data)->native_article_links[1]==parent_id);
    CHECK(((Item*)child->user_data)->owner==spawn_caller&&creation_value(child)==0);
    CHECK(((Item*)child->user_data)->xDD4_itemVar.native_article.launch_origin.x==point.x);
    remove_item(child);++cases;
    // Reflection transfers damage credit, retaining original author and control.
    HSD_GObj* reflected_owner=foreign(999);((Item*)parent_item->user_data)->owner=reflected_owner;
    reset_records();child_call=true;linked_slot=0;backlink_index=1;expected_creation=0;damage_owner=reflected_owner;
    CHECK(nativeScriptItem_RunSignal(parent_item,public_signal("public_link"),0,&result)==NativeScriptVM_Ok);
    child=find_item(programs[1].owner);CHECK(child&&((Item*)child->user_data)->owner==reflected_owner);
    CHECK(nativeScriptItem_Creator(child)==original&&nativeScriptItem_AttachmentOwner(child)==attachment);
    CHECK(((Item*)child->user_data)->native_article_author_kind==programs[1].author_kind);
    remove_item(child);((Item*)parent_item->user_data)->owner=spawn_caller;HSD_GObjFree(reflected_owner);damage_owner=spawn_caller;++cases;
    // Created can remove the child. The factory must not publish a dead handle.
    reset_records();child_call=true;linked_slot=0;backlink_index=0;mode=RemoveChild;trigger=StageCreated;Item_GObj* removed=(void*)1;
    CHECK(nativeScriptItemSpawn_Child(&c,programs[1].owner,&point,-1,777,0,7.125f,0,0,&removed,&diagnostic)!=NativeScriptVM_Ok);
    CHECK(removed==(void*)1&&!find_item(programs[1].owner));++cases;
    reset_records();Item_GObj* output=(void*)1;
    CHECK(nativeScriptItemSpawn_Child(&c,programs[1].owner,&point,-1,777,0,0,0,4,&output,&diagnostic)==NativeScriptVM_Error);
    CHECK(output==(void*)1&&!spawn_order_count);++cases;
    // Allocation failure must not invent either direction of the relationship.
    reset_records();positional_oom=true;uint32_t previous=((Item*)parent_item->user_data)->native_article_links[0];output=(void*)1;
    CHECK(nativeScriptItemSpawn_Child(&c,programs[1].owner,&point,-1,777,0,0,0,0,&output,&diagnostic)==NativeScriptVM_Ok&&!output);
    CHECK(((Item*)parent_item->user_data)->native_article_links[0]==previous);++cases;
    remove_item(parent_item);parent_item=NULL;
}
