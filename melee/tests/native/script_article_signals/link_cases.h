// Actual published actors/storage, real VM calls, and the extracted existing
// native relationship implementation. No replacement linking implementation.
static NativeScriptValue link_int(int64_t value)
{return (NativeScriptValue){.type=NativeScriptType_I32,.as.i64=value};}
static NativeScriptValue link_call(HSD_GObj* object,uint32_t op,NativeScriptValue* args,unsigned count)
{
    NativeScriptRuntimeContext context={.source=object,.source_class=object->classifier==HSD_GOBJ_CLASS_ITEM?
        NativeCallbackSource_Item:NativeCallbackSource_Fighter,.generation=object->classifier==HSD_GOBJ_CLASS_ITEM?
        ((Item*)object->user_data)->x1C:19,.scene_epoch=nativeGObjPool.lifetime};
    NativeScriptValue output;NativeScriptDiagnostic diagnostic;
    CHECK(nativeScriptBindings_Invoke(&context,op,NULL,0,args,count,&output,&diagnostic));return output;
}
static void link_signal(HSD_GObj* object,const char* name,float value)
{
    const NativeScriptItemProgram* p=nativeScriptItem_Spec(object);uint32_t code=codeid(p->owner,name),id=UINT32_MAX;
    for(unsigned i=0;i<p->signal_count;++i)if(p->signals[i].function==code)id=p->signals[i].signal_id;
    CHECK(id!=UINT32_MAX);NativeScriptValue output;
    CHECK(nativeScriptItem_RunSignal(object,id,value,&output)==NativeScriptVM_Ok);
}
static void link_owned_vm(HSD_GObj* fighter_object,bool expected)
{
    uint32_t function=UINT32_MAX;
    for(unsigned i=0;i<bank.declarations.function_count;++i)
        if(!functions[i].owner&&nativeScriptSource_SpanEquals(&bank.tree,functions[i].name,"LinkOwned"))function=i;
    CHECK(function!=UINT32_MAX);NativeScriptValue output;NativeScriptDiagnostic diagnostic;
    CHECK(nativeScriptFighter_Helper(fighter_object,19,function,NULL,0,&output,&diagnostic)==NativeScriptVM_Ok);
    CHECK(field(fighter_object,"delivered",NULL).as.u64==expected);
}
static void link_cases(HSD_GObj* fighter_object,HSD_GObj* foreign)
{
    HSD_GObj* a=item(0),*b=item(0),*c=item(1);Item* from=a->user_data,*to=b->user_data,*other=c->user_data;
    from->owner=to->owner=other->owner=fighter_object;
    CHECK(nativeScriptItem_SetAttachmentOwner(a,fighter_object));CHECK(nativeScriptItem_SetAttachmentOwner(b,fighter_object));
    CHECK(nativeScriptItem_SetAttachmentOwner(c,fighter_object));
    for(unsigned i=0;i<4;++i)from->native_article_links[i]=to->native_article_links[i]=other->native_article_links[i]=UINT32_MAX;
    NativeScriptValue args[3]={link_int(from->x1C),link_int(0),link_int(to->x1C)};
    // Native complete CollData copy: every slot, preserving unrelated Item bytes.
    CollData saved_from=from->x378_itemColl,saved_to=to->x378_itemColl;
    memset(&to->x378_itemColl,0x5a,sizeof(to->x378_itemColl));
    for(unsigned copy_slot=0;copy_slot<4;++copy_slot) {
        from->native_article_links[copy_slot]=to->x1C;
        Item before=*from;
        nativeArticle_CopyLinkCollision(a,copy_slot);Item expected=*from;*from=before;
        NativeScriptValue selected=link_int(copy_slot);
        CHECK(link_call(a,NativeScriptBinding_ItemCopyLinkCollision,&selected,1).as.u64);
        CHECK(!memcmp(from,&expected,sizeof(*from)));
        from->native_article_links[copy_slot]=UINT32_MAX;before=*from;
        CHECK(!link_call(a,NativeScriptBinding_ItemCopyLinkCollision,&selected,1).as.u64);
        CHECK(!memcmp(from,&before,sizeof(*from)));
    }
    from->x378_itemColl=saved_from;to->x378_itemColl=saved_to;
    sethandle(fighter_object,from->x1C);field(fighter_object,"other",&args[2]);
    link_owned_vm(fighter_object,true);CHECK(from->native_article_links[0]==to->x1C);
    link_owned_vm(fighter_object,true); // Reinstalling the same link is allowed.
    args[2]=link_int(other->x1C);CHECK(!link_call(fighter_object,NativeScriptBinding_ArticlesLinkOwned,args,3).as.u64);
    CHECK(from->native_article_links[0]==to->x1C);
    args[2]=link_int(from->x1C);CHECK(!link_call(fighter_object,NativeScriptBinding_ArticlesLinkOwned,args,3).as.u64);
    args[2]=link_int(to->x1C);to->owner=foreign;
    CHECK(!link_call(fighter_object,NativeScriptBinding_ArticlesLinkOwned,args,3).as.u64);to->owner=fighter_object;
    CHECK(nativeScriptItem_SetAttachmentOwner(b,foreign));CHECK(!link_call(fighter_object,NativeScriptBinding_ArticlesLinkOwned,args,3).as.u64);
    CHECK(nativeScriptItem_SetAttachmentOwner(b,fighter_object));
    NativeScriptValue slot_args[2]={link_int(0),link_int(0)};
    link_signal(a,"ReadLink",0);CHECK(field(a,"handle",NULL).as.i64==to->x1C&&!field(a,"delivered",NULL).as.u64);
    to->native_article_links[0]=from->x1C;link_signal(a,"ReadLink",0);CHECK(field(a,"delivered",NULL).as.u64);
    // Reflection changes credit, not package authorship or an existing link.
    to->owner=foreign;CHECK(link_call(a,NativeScriptBinding_ItemLinkId,slot_args,1).as.i64==to->x1C);
    link_signal(a,"Forward",7.5f);CHECK(field(a,"delivered",NULL).as.u64&&to->pos.x==7.5f);to->owner=fighter_object;
    // Compare all slots and all reciprocal slots with the existing native body.
    for(unsigned slot=0;slot<4;++slot)for(unsigned backlink=0;backlink<4;++backlink) {
        for(unsigned i=0;i<4;++i)from->native_article_links[i]=to->native_article_links[i]=UINT32_MAX;
        CHECK(nativeArticle_Link(a,slot,b));CHECK(nativeArticle_Link(b,backlink,a));
        slot_args[0]=link_int(slot);slot_args[1]=link_int(backlink);
        CHECK(link_call(a,NativeScriptBinding_ItemLinkId,slot_args,1).as.i64==nativeArticle_LinkId(a,slot));
        CHECK(link_call(a,NativeScriptBinding_ItemLinkReciprocal,slot_args,2).as.u64==nativeArticle_LinkReciprocal(a,slot,backlink));
        // Multiple backlinks to this parent must all be removed.
        to->native_article_links[(backlink+1)%4]=from->x1C;
        uint32_t saved_from[4],saved_to[4],expected_from[4],expected_to[4];
        memcpy(saved_from,from->native_article_links,sizeof(saved_from));memcpy(saved_to,to->native_article_links,sizeof(saved_to));
        nativeArticle_Unlink(a,slot);memcpy(expected_from,from->native_article_links,sizeof(expected_from));memcpy(expected_to,to->native_article_links,sizeof(expected_to));
        memcpy(from->native_article_links,saved_from,sizeof(saved_from));memcpy(to->native_article_links,saved_to,sizeof(saved_to));
        link_call(a,NativeScriptBinding_ItemUnlink,slot_args,1);
        CHECK(!memcmp(expected_from,from->native_article_links,sizeof(expected_from))&&!memcmp(expected_to,to->native_article_links,sizeof(expected_to)));
    }
    // Full-width IDs and recycled GObj addresses cannot change link identity.
    override_id=UINT32_C(0x80000005);HSD_GObj* wide=item(0);override_id=0;Item* wide_data=wide->user_data;
    wide_data->owner=fighter_object;CHECK(nativeScriptItem_SetAttachmentOwner(wide,fighter_object));
    args[1]=link_int(0);args[2]=link_int((int64_t)UINT32_C(0x80000005)-INT64_C(0x100000000));
    CHECK(link_call(fighter_object,NativeScriptBinding_ArticlesLinkOwned,args,3).as.u64);slot_args[0]=link_int(0);
    CHECK(link_call(a,NativeScriptBinding_ItemLinkId,slot_args,1).as.i64==args[2].as.i64);
    HSD_GObjFree(wide);HSD_GObj* reused=item(0);CHECK(reused==wide);
    CHECK(link_call(a,NativeScriptBinding_ItemLinkId,slot_args,1).as.i64==-1);HSD_GObjFree(reused);
    args[2]=link_int(to->x1C);CHECK(link_call(fighter_object,NativeScriptBinding_ArticlesLinkOwned,args,3).as.u64);
    link_signal(a,"Unlink",0);CHECK(from->native_article_links[0]==UINT32_MAX);
    // Rejected input/context leaves caller-provided output and links intact.
    NativeScriptRuntimeContext context={.source=a,.source_class=NativeCallbackSource_Item,.generation=from->x1C,.scene_epoch=nativeGObjPool.lifetime};
    NativeScriptDiagnostic diagnostic;NativeScriptValue output;
    for(unsigned failure=0;failure<5;++failure) {
        output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};slot_args[0]=link_int(failure==0?4:0);
        context.generation=from->x1C;context.scene_epoch=nativeGObjPool.lifetime;
        if(failure==1)context.generation++;if(failure==2)context.scene_epoch++;
        if(failure==3)slot_args[0]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=0};
        if(failure==4)slot_args[0]=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000000)};
        CHECK(!nativeScriptBindingItemLinks_Invoke(&context,NativeScriptBinding_ItemLinkId,slot_args,1,&output,&diagnostic));
        CHECK(output.type==NativeScriptType_U64&&output.as.u64==99&&from->native_article_links[0]==UINT32_MAX);
    }
    // Removal is marked before Destroyed runs; cleanup still owns live storage.
    from->native_article_links[0]=to->x1C;to->native_article_links[0]=from->x1C;
    from->native_removal_in_progress=true;
    NativeScriptValue destruction_result;
    CHECK(nativeScriptItem_RunHook(a,NativeScriptItemHook_Destroyed,NULL,0,&destruction_result)==NativeScriptVM_Ok);
    CHECK(from->native_article_links[0]==UINT32_MAX&&to->native_article_links[0]==UINT32_MAX);
    // An Article caller has the same retained signal delivery rules as a Fighter:
    // recipient deletion/reuse succeeds, caller deletion stops, faults propagate.
    for(unsigned mutation=1;mutation<=4;++mutation) {
        HSD_GObj* sender=item(0),*recipient=item(0);Item* sender_data=sender->user_data;
        sender_data->native_article_links[0]=((Item*)recipient->user_data)->x1C;
        const NativeScriptItemProgram* p=nativeScriptItem_Spec(sender);
        uint32_t function=codeid(p->owner,"Forward"),signal=UINT32_MAX;
        for(unsigned i=0;i<p->signal_count;++i)if(p->signals[i].function==function)signal=p->signals[i].signal_id;
        CHECK(signal!=UINT32_MAX);signal_caller=sender;signal_mode=mutation;signal_reuse=mutation==4?reuse:NULL;
        NativeScriptValue returned={.type=NativeScriptType_U64,.as.u64=99};
        NativeScriptVMStatus status=nativeScriptItem_RunSignal(sender,signal,4.5f,&returned);
        signal_caller=fighter_object;signal_mode=0;signal_reuse=NULL;
        if(mutation==1||mutation==4) {
            CHECK(status==NativeScriptVM_Ok&&field(sender,"delivered",NULL).as.u64);
            if(mutation==4){CHECK(replacement==recipient&&field(replacement,"completed",NULL).as.i64==0);HSD_GObjFree(replacement);}
            HSD_GObjFree(sender);
        } else {
            CHECK(status!=NativeScriptVM_Ok&&returned.type==NativeScriptType_U64&&returned.as.u64==99);
            if(mutation!=2)HSD_GObjFree(sender);HSD_GObjFree(recipient);
        }
    }
    HSD_GObjFree(a);HSD_GObjFree(b);HSD_GObjFree(c);
}
