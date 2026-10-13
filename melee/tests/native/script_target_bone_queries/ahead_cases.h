bool ahead_teams;
static void target_query_compile_cases(void)
{
    const char* calls[]={"targets.read_opponent_ahead(item.state.x, item.state.y, item.state.z)",
        "targets.read_lockable_item_ahead(item.state.x, item.state.x, item.state.z)",
        "targets.read_opponent_ahead(item.state.x, item.state.y, item.state.y)",
        "targets.read_opponent_ahead(item.state.x, item.state.y)",
        "targets.read_lockable_item_ahead(1.0, item.state.y, item.state.z)",
        "targets.overlaps_hitbox(1.0, 0)", "targets.overlaps_hitbox(1, 0.0)",
        "targets.overlaps_hitbox(1)", "targets.overlaps_hitbox(1, 0, 2)",
        "targets.nearest_available_fighter()", "targets.nearest_available_fighter(0)"};
    NativeSourceBank scratch={0};CHECK(nativeSourceBank_Allocate(&scratch,&diagnostic));
    NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&article};
    NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};
    for(unsigned i=0;i<sizeof(calls)/sizeof(*calls);++i) {
        char source[1024];snprintf(source,sizeof(source),"fighter:\n    behavior independent\narticle Probe states 1:\n    state float x\n    state float y\n    state float z\n    on signal Test:\n        %s\n    state 0 anim -1:\n",calls[i]);
        NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),
            .motion_layout=&packages[0].assets.facts.moves,.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};
        const NativeScriptCompileBank* accepted=NULL;
        CHECK(nativeScriptCompile_Build(&scratch.bank,&request,&accepted,&diagnostic)==(i==0));
        if(i==1||i==2)CHECK(strstr(diagnostic.message,"distinct")!=NULL);
        if(i)++negative_count;
    }
    nativeSourceBank_Release(&scratch);
}
static bool ahead_host(HSD_GObj* source,bool items,NativeScriptValue* outputs,NativeScriptValue* result)
{
    NativeScriptRuntimeContext c=context(source);
    return nativeScriptBindingTargets_Invoke(&c,items?NativeScriptBinding_TargetsReadLockableItemAhead:
        NativeScriptBinding_TargetsReadOpponentAhead,outputs,3,result,&diagnostic);
}
static void ahead_compare(HSD_GObj* source,bool item_query)
{
    Item* ip=source->user_data;Vec3 expected={0};HSD_GObj* selected;
    if(item_query) {selected=it_8026C258(&ip->pos,ip->facing_dir);if(selected)it_8026BB88(selected,&expected);}
    else selected=it_8026B634(&ip->pos,&expected,ip->owner,ip->facing_dir);
    NativeScriptValue outputs[3]={floating(90),floating(91),floating(92)},result;
    CHECK(ahead_host(source,item_query,outputs,&result)&&result.type==NativeScriptType_Bool&&result.as.u64==(selected!=NULL));
    for(unsigned i=0;i<3;++i) {
        float value=selected?(i==0?expected.x:i==1?expected.y:expected.z):90+(float)i;
        CHECK(bits(outputs[i].as.f32)==bits(value));
        NativeScriptValue initial=floating(90+(float)i);field(source,i==0?"x":i==1?"y":"z",&initial);
    }
    geometry_run(source,item_query?"QueryItemAhead":"QueryOpponentAhead");
    CHECK(field(source,"found",NULL).as.u64==(selected!=NULL));
    for(unsigned i=0;i<3;++i) {
        float value=selected?(i==0?expected.x:i==1?expected.y:expected.z):90+(float)i;
        CHECK(bits(field(source,i==0?"x":i==1?"y":"z",NULL).as.f32)==bits(value));
    }
    ++comparisons;++cases;
}
static void ahead_cases(HSD_GObj* owner,HSD_GObj* source,HSD_GObj* owned,HSD_GObj* stock)
{
    target_query_compile_cases();
    HSD_GObj* targets[]={owner,owned,stock};Fighter saved[3];HSD_JObj joints_saved[3][4];
    Item* ip=source->user_data;Item item_saved=*ip;
    for(unsigned t=0;t<3;++t) {
        Fighter* f=targets[t]->user_data;saved[t]=*f;f->player_id=t;f->co_attrs.x170=(Vec3){0};
        f->ft_data->x0->camera_zoom_target_bone=2;
        for(unsigned j=0;j<4;++j) {
            HSD_JObj* bone=f->parts[j].joint;joints_saved[t][j]=*bone;
            bone->translate=(Vec3){0};bone->rotate=(Quaternion){0};bone->scale=(Vec3){1,1,1};bone->flags|=JOBJ_MTX_DIRTY;
        }
    }
    for(unsigned n=0;n<128;++n) {
        ahead_teams=(n&1)!=0;ip->owner=(n&6)?targets[((n>>1)%3)]:NULL;
        ip->facing_dir=(n%4==0)?-1:(n%4==1)?1:(n%4==2)?0:.5f;ip->pos=(Vec3){0,0,17};
        for(unsigned t=0;t<3;++t) {
            Fighter* f=targets[t]->user_data;f->team=(n>>(t+3))&1;f->x221F_b3=(n>>(t+2))&1;
            f->player_id=t==2&&(n&32)?((Fighter*)owned->user_data)->player_id:t;
            f->parts[0].joint->translate=(Vec3){t==0?-12:t==1?-5:5,(n&64)?3:0,(float)t};
            for(unsigned j=0;j<4;++j)f->parts[j].joint->flags|=JOBJ_MTX_DIRTY;
        }
        ahead_compare(source,false);
    }
    // All four native lock-on categories, blocked held/owner-bound entries,
    // exact-facing half planes, nearest XY ties and ECB-center output offsets.
    HSD_GObj* candidates[5];const int categories[]={ITEM_UNK_MATO,ITEM_UNK_LOCKON,ITEM_UNK_ENEMY,ITEM_UNK_7,0};
    for(unsigned i=0;i<5;++i)candidates[i]=item();
    for(unsigned n=0;n<128;++n) {
        ip->facing_dir=(n%4==0)?-1:(n%4==1)?1:(n%4==2)?0:.5f;
        for(unsigned i=0;i<5;++i) {
            Item* candidate=candidates[i]->user_data;candidate->hold_kind=categories[i];
            candidate->pos=(Vec3){(i&1)?-5:5,(float)(n&1),10+(float)i};
            candidate->grab_victim=(n&(1u<<i))?owner:NULL;
            candidate->owner=(n&32)?owner:NULL;candidate->xDC8_word.flags.x13=(n&64)!=0;
            candidate->x378_itemColl.ecb.top.y=7+(float)i;candidate->x378_itemColl.ecb.bottom.y=-3;
        }
        ahead_compare(source,true);
    }
    for(unsigned i=0;i<5;++i)HSD_GObjFree(candidates[i]);
    // Admission failures leave both the caller's destinations and result intact.
    for(unsigned n=0;n<6;++n) {
        NativeScriptRuntimeContext c=context(source);if(n==0)c=context(owner);if(n==1)++c.generation;
        if(n==2)++c.scene_epoch;if(n==3)c.source=NULL;
        NativeScriptValue outputs[3]={floating(90),floating(91),floating(92)},original[3];memcpy(original,outputs,sizeof(outputs));
        NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=99};
        CHECK(!nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsReadOpponentAhead,outputs,n==4?2:3,
            n==5?NULL:&result,&diagnostic));
        CHECK(!memcmp(outputs,original,sizeof(outputs))&&result.type==NativeScriptType_U64&&result.as.u64==99);++negative_count;
    }
    ahead_teams=false;
    // An Item cannot stand in for a damage-owner Fighter. A malformed camera
    // bone must reject before native search evaluates the borrowed rig.
    ip->owner=source;
    NativeScriptValue outputs[3]={floating(90),floating(91),floating(92)},result={.type=NativeScriptType_U64,.as.u64=99};
    CHECK(!ahead_host(source,false,outputs,&result));
    CHECK(result.type==NativeScriptType_U64&&result.as.u64==99&&outputs[0].as.f32==90);++negative_count;
    ip->owner=owner;
    Fighter* invalid=owned->user_data;invalid->x221F_b3=0;invalid->player_id=1;
    int camera=invalid->ft_data->x0->camera_zoom_target_bone;invalid->ft_data->x0->camera_zoom_target_bone=400;
    CHECK(!ahead_host(source,false,outputs,&result));
    CHECK(result.type==NativeScriptType_U64&&result.as.u64==99&&outputs[0].as.f32==90);++negative_count;
    invalid->ft_data->x0->camera_zoom_target_bone=camera;
    *ip=item_saved;
    for(unsigned t=0;t<3;++t) {
        Fighter* f=targets[t]->user_data;*f=saved[t];
        for(unsigned j=0;j<4;++j)*f->parts[j].joint=joints_saved[t][j];
    }
}
