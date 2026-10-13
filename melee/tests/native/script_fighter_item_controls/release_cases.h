// Real shared release body, controlled transform/owner/collision leaves and
// actual source Dropped hook. This is not a full item physics/render fixture.
static void release_raw_drop(HSD_GObj* object)
{++raw_pickups;lifecycle_mutation(object);}

static void release_cases(void)
{
    HSD_GObj* caller=fighter(true);Fighter* f=caller->user_data;
    NativeScriptValue args[]={{.type=NativeScriptType_F32,.as.f32=-4},
        {.type=NativeScriptType_F32,.as.f32=5},{.type=NativeScriptType_F32,.as.f32=-6},
        {.type=NativeScriptType_F32,.as.f32=1.75f}};
    NativeScriptValue output=sentinel(),before=output;
    HSD_GObj* malformed=raw_item(caller);Item* malformed_data=malformed->user_data;
    f->item_gobj=malformed;
    for(unsigned n=0;n<5;++n) {
        Item peer=*malformed_data;void* root=malformed->hsd_obj;ItemModelDesc* descriptor=article.x10_modelDesc;
        if(n==0)malformed_data->xC4_article_data=NULL;
        if(n==1)article.x10_modelDesc=NULL;
        if(n==2)malformed_data->xCC_item_attr=NULL;
        if(n==3)malformed->hsd_obj=NULL;
        if(n==4)malformed_data->xB8_itemLogicTable=NULL;
        unsigned transforms=release_transforms;output=before;
        CHECK(!invoke(caller,NativeScriptBinding_ItemsReleaseHeld,args,4,&output)&&
            !memcmp(&output,&before,sizeof(output))&&release_transforms==transforms);
        *malformed_data=peer;malformed->hsd_obj=root;article.x10_modelDesc=descriptor;++cases;
    }
    HSD_GObjFree(malformed);f->item_gobj=NULL;
    const float factors[]={0,-0.0f,-2,1,1.75f};
    for(unsigned raw=0;raw<2;++raw) {
        HSD_GObj* target=raw?raw_item(caller):item(caller);Item* data=target->user_data;
        if(raw) {raw_logic.dropped=release_raw_drop;data->kind=It_Kind_Taru;}
        f->item_gobj=target;
        for(unsigned n=0;n<sizeof(factors)/sizeof(factors[0]);++n) {
            Vec3 velocity={-4,5,-6};Item peer=*data;
            unsigned owners=release_owners,boxes=release_hitboxes,heavy=release_heavy_tails;
            Item_8026ABD8(target,&velocity,factors[n]);Item expected=*data;
            CHECK(release_owners==owners+1&&release_hitboxes==boxes+1&&release_heavy_tails==heavy+raw);
            *data=peer;args[3].as.f32=factors[n];
            CHECK(invoke(caller,NativeScriptBinding_ItemsReleaseHeld,args,4,&output)&&output.type==NativeScriptType_Void);
            CHECK(!memcmp(data,&expected,sizeof(expected))&&release_owners==owners+2&&
                release_hitboxes==boxes+2&&release_heavy_tails==heavy+2*raw);
            ++comparisons;++cases;
        }
        unsigned transforms=release_transforms;run(caller,"Release");
        CHECK(data->xC44==1.75f&&data->x40_vel.x==-4&&data->x40_vel.y==5&&data->x40_vel.z==-6&&release_transforms==transforms+1);
        if(!raw)CHECK(field(target,"pickups",NULL).as.i64==11);
        HSD_GObjFree(target);f->item_gobj=NULL;++cases;
    }
    unsigned transforms=release_transforms;
    run(caller,"Release");CHECK(release_transforms==transforms);++cases;
    for(unsigned n=0;n<4;++n) {
        NativeScriptValue saved_arg=args[n];args[n].as.f32=__builtin_nanf("");output=before;
        CHECK(!invoke(caller,NativeScriptBinding_ItemsReleaseHeld,args,4,&output)&&
            !memcmp(&output,&before,sizeof(output))&&release_transforms==transforms);
        args[n]=saved_arg;args[n].as.f32=__builtin_inff();output=before;
        CHECK(!invoke(caller,NativeScriptBinding_ItemsReleaseHeld,args,4,&output)&&!memcmp(&output,&before,sizeof(output)));
        args[n]=saved_arg;args[n].type=NativeScriptType_I32;output=before;
        CHECK(!invoke(caller,NativeScriptBinding_ItemsReleaseHeld,args,4,&output)&&!memcmp(&output,&before,sizeof(output)));
        args[n]=saved_arg;++cases;
    }
    output=before;CHECK(!invoke(caller,NativeScriptBinding_ItemsReleaseHeld,args,3,&output)&&!memcmp(&output,&before,sizeof(output)));++cases;
    HSD_GObj* stock=fighter(false);output=before;
    CHECK(!invoke(stock,NativeScriptBinding_ItemsReleaseHeld,args,4,&output)&&!memcmp(&output,&before,sizeof(output)));
    HSD_GObjFree(stock);HSD_GObjFree(caller);++cases;

    const unsigned modes[]={2,3,4,5,6,7,9,12,13};
    for(unsigned raw=0;raw<2;++raw) for(unsigned n=0;n<sizeof(modes)/sizeof(modes[0]);++n) {
        unsigned mode=modes[n];
        if(raw&&(mode==4||mode==5||mode==9))continue; // Source-storage faults apply only to authored actors.
        caller=fighter(true);HSD_GObj* target=raw?raw_item(caller):item(caller);
        if(raw)raw_logic.dropped=release_raw_drop;
        f=caller->user_data;f->item_gobj=target;lifecycle_creator=caller;
        lifecycle_reuse=reuse;replacement=NULL;lifecycle_mode=mode;
        unsigned owners=release_owners,boxes=release_hitboxes;output=before;
        bool success=invoke(caller,NativeScriptBinding_ItemsReleaseHeld,args,4,&output);
        // Destruction is a normal Dropped outcome; the new same-slot actor is
        // not the released actor, so native continuation must stop before tails.
        CHECK(success==(mode==2||mode==7));
        if(success)CHECK(output.type==NativeScriptType_Void);
        else CHECK(!memcmp(&output,&before,sizeof(output)));
        CHECK(release_owners==owners+(mode==3)&&release_hitboxes==boxes+(mode==3));
        if(mode!=2&&mode!=7)HSD_GObjFree(target);
        if(replacement) {CHECK(field(replacement,"pickups",NULL).as.i64==0);HSD_GObjFree(replacement);}
        if(mode!=3)HSD_GObjFree(caller);
        lifecycle_mode=0;lifecycle_reuse=NULL;++cases;
    }
    raw_logic.dropped=NULL;
}
