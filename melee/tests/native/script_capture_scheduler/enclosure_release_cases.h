static HSD_Joint enclosure_model;
static void* enclosure_models[FT_CHARACTER_ITEM_CAPACITY];
static FighterPartsTable* enclosure_tables[Ft_Kind_RuntimeCapacity];
static FighterPartsTable enclosure_stock_parts;
static u8 enclosure_stock_map[MAX_FT_PARTS];
static FighterBone enclosure_bones[MAX_FT_PARTS];
static HSD_JObj enclosure_joint;

static Fighter_GObj* enclosure_pair(bool owned,bool air,bool left)
{
    reset();Fighter_GObj* holder=create(true);Fighter_GObj* victim=create(owned);link_pair(holder,victim);
    Fighter* h=holder->user_data;Fighter* v=victim->user_data;
    memset(enclosure_models,0,sizeof(enclosure_models));enclosure_models[3]=&enclosure_model;
    h->ft_data->x48_items=enclosure_models;h->facing_dir=left?-1:1;
    h->capture_enclosure_settings=(NativeCaptureEnclosureSettings){
        .launch_velocity={-h->facing_dir*2,3,0},.escape_velocity={7,-4,0},
        .damage_behavior=3,.shrink_fraction=0.5f,.shrink_frames=17,
        .lifetime=80,.damage_time_scale=1.25f,.model_slot=3,.escape_invincibility_frames=7};
    h->capture_settings.timer_decay=0.75f;h->capture_settings.mash_reduction=2;
    h->capture_settings.mash_pause_frames=1.5f;h->capture_settings.mash_animation_rate=0.25f;
    NativeSourceImportedAssets* assets=&capture_package.assets;
    assets->facts.joints=MAX_FT_PARTS;assets->facts.omission_count=0;
    assets->parts.parts_num=MAX_FT_PARTS;assets->parts.part_to_joint=assets->part_to_joint;
    for(unsigned i=0;i<MAX_FT_PARTS;++i){assets->part_to_joint[i]=(u8)i;enclosure_stock_map[i]=(u8)i;}
    enclosure_stock_parts=(FighterPartsTable){.parts_num=MAX_FT_PARTS,.part_to_joint=enclosure_stock_map};
    ftPartsTable=enclosure_tables;enclosure_tables[Ft_Kind_CustomFirst]=&assets->parts;
    enclosure_tables[Ft_Kind_Mario]=&enclosure_stock_parts;
    memset(enclosure_bones,0,sizeof(enclosure_bones));
    enclosure_joint=(HSD_JObj){.flags=JOBJ_MTX_INDEP_SRT,.scale={1.25f,2,0.75f}};
    enclosure_bones[FtPart_TransN].joint=&enclosure_joint;
    enclosure_bones[FtPart_TransN2].joint=&enclosure_joint;
    enclosure_bones[FtPart_XRotN].joint=&enclosure_joint;
    v->parts=h->parts=enclosure_bones;holder->hsd_obj=&enclosure_joint;
    victim->hsd_obj=&enclosure_joint;v->ground_or_air=air?GA_Air:GA_Ground;
    v->co_attrs.xBC.size=2.5f;v->co_attrs.xBC.x4=(Vec3){1,2,3};
    v->co_attrs.xBC.x10=(Vec3){4,5,6};v->co_attrs.xBC.x1C=7;
    v->x20A0_accessory=NULL;
    return holder;
}

static void enclosure_release_cases(void)
{
    NativeScriptDiagnostic diagnostic;
    for(unsigned owned=0;owned<2;++owned)for(unsigned air=0;air<2;++air)
      for(unsigned left=0;left<2;++left){
        Fighter_GObj* holder=enclosure_pair(owned,air,left);Fighter_GObj* victim=candidate;
        Fighter* h=holder->user_data;Fighter* v=victim->user_data;
        NativeCaptureEnclosureSettings expected=h->capture_enclosure_settings;
        expected.timer_decay=h->capture_settings.timer_decay;expected.mash_reduction=h->capture_settings.mash_reduction;
        expected.mash_pause_frames=h->capture_settings.mash_pause_frames;
        expected.mash_animation_rate=h->capture_settings.mash_animation_rate;
        CHECK(helper(holder,"ReleaseEnclosure"));CHECK(presentation_state(holder,"items",NULL).as.i64==99);
        CHECK(!memcmp(&v->capture_enclosure.settings,&expected,sizeof(expected)));
        CHECK(v->capture_enclosure.holder==holder&&v->capture_enclosure.restore_model_on_escape&&!v->capture_enclosure.subtract_damage_form);
        CHECK(v->motion_id==ftCo_MS_YoshiEgg&&v->ground_or_air==GA_Air&&animation_donor==holder);
        CHECK(v->invisible&&v->facing_dir==h->facing_dir&&v->dmg.x182c_behavior==3);
        CHECK(!memcmp(&v->self_vel,&expected.launch_velocity,sizeof(Vec3))&&v->grab_timer==80);
        CHECK(v->capture_enclosure.shrink_remaining==17&&v->capture_enclosure.mash_pause_remaining==0);
        CHECK(v->x20A0_accessory==&enclosure_accessory&&enclosure_accessory.scale.x==2.5f&&enclosure_accessory.scale.y==2.5f);
        CHECK(v->capture_enclosure.root_scale.x==1.25f&&v->capture_enclosure.root_scale.y==2&&v->capture_enclosure.root_scale.z==0.75f);
        CHECK(v->hurt_capsules[0].capsule.bone==&enclosure_joint&&v->hurt_capsules[0].capsule.scale==7);
        CHECK(!memcmp(&v->hurt_capsules[0].capsule.a_offset,&v->co_attrs.xBC.x4,sizeof(Vec3)));
        CHECK(!memcmp(&v->hurt_capsules[0].capsule.b_offset,&v->co_attrs.xBC.x10,sizeof(Vec3)));
        CHECK(v->take_dmg_cb==ftCo_800BC438&&v->take_dmg_2_cb==ftCo_CaptureEnclosure_Damage&&v->accessory4_cb);
        CHECK(!h->victim_gobj&&!v->victim_gobj&&!h->x1A5C&&!v->x1A5C);
        const unsigned ground_trace[]={12,6,5,13,14,15},air_trace[]={12,5,13,14,15};
        CHECK(trace_count==(air?5:6)&&!memcmp(trace,air?air_trace:ground_trace,trace_count*sizeof(unsigned)));
        h->capture_enclosure_settings.lifetime=-99;h->capture_settings.timer_decay=1234;
        CHECK(!memcmp(&v->capture_enclosure.settings,&expected,sizeof(expected)));++cases;
        HSD_GObjFree(victim);HSD_GObjFree(holder);
    }
    // Invalid accessory settings must reject before applying throw damage.
    for(unsigned bad=0;bad<4;++bad){
        Fighter_GObj* holder=enclosure_pair(false,false,false);Fighter_GObj* victim=candidate;
        Fighter* h=holder->user_data;
        if(bad==0)h->capture_enclosure_settings.model_slot=-1;
        if(bad==1)h->capture_enclosure_settings.model_slot=FT_CHARACTER_ITEM_CAPACITY;
        if(bad==2)h->ft_data->x48_items=NULL;if(bad==3)enclosure_models[3]=NULL;
        NativeScriptRuntimeContext context=victim_policy_context(holder);
        NativeScriptValue output={.type=NativeScriptType_U64,.as.u64=99};
        CHECK(!nativeScriptBindingCapture_Invoke(&context,NativeScriptBinding_CaptureReleaseIntoEnclosure,NULL,0,&output,&diagnostic));
        CHECK(output.type==NativeScriptType_U64&&output.as.u64==99&&trace_count==0&&h->victim_gobj==victim);++cases;
        HSD_GObjFree(victim);HSD_GObjFree(holder);
    }
    const unsigned stops[]={12,6,5,13};
    for(unsigned owned=0;owned<2;++owned)for(unsigned stop=0;stop<4;++stop)for(unsigned target=0;target<2;++target){
        Fighter_GObj* holder=enclosure_pair(owned!=0,false,false);Fighter_GObj* victim=candidate;
        retire_step=stops[stop];retire_target=target;
        NativeScriptReceiverView view;
        CHECK(nativeScriptFighter_View(holder,((Fighter*)holder->user_data)->x8_spawnNum,&view));
        uint8_t before_bytes[256],after_bytes[256];
        NativeScriptStorageSnapshot before={.bytes=before_bytes,.capacity=sizeof(before_bytes)};
        CHECK(nativeScriptStorage_Save(view.storage,&before,&diagnostic));
        CHECK(!helper(holder,"ReleaseEnclosure"));
        NativeScriptStorageSnapshot after={.bytes=after_bytes,.capacity=sizeof(after_bytes)};
        CHECK(nativeScriptStorage_Save(view.storage,&after,&diagnostic));
        CHECK(before.state_bytes==after.state_bytes&&!memcmp(before.bytes,after.bytes,before.state_bytes));
        CHECK(trace_count&&trace[trace_count-1]==stops[stop]);++cases;
        HSD_GObjFree(victim);HSD_GObjFree(holder);
    }
    // Native throw damage itself samples these joints before enclosure entry.
    // Malformed rigs must therefore reject before damage, not just afterwards.
    for(unsigned owned=0;owned<2;++owned)for(unsigned bad=0;bad<6;++bad){
        Fighter_GObj* holder=enclosure_pair(owned!=0,false,false);Fighter_GObj* victim=candidate;
        Fighter* h=holder->user_data;Fighter* v=victim->user_data;
        if(bad==0)h->parts=NULL;if(bad==1)v->parts=NULL;
        if(bad==2)enclosure_bones[FtPart_TransN2].joint=NULL;
        if(bad==3)enclosure_bones[FtPart_TransN].joint=NULL;
        if(bad==4){v->x2226_b2=true;enclosure_bones[FtPart_XRotN].joint=NULL;}
        if(bad==5){h->x221B_b7=true;h->x2226_b2=true;enclosure_bones[FtPart_XRotN].joint=NULL;}
        NativeScriptRuntimeContext context=victim_policy_context(holder);
        NativeScriptValue output={.type=NativeScriptType_U64,.as.u64=99};
        CHECK(!nativeScriptBindingCapture_Invoke(&context,NativeScriptBinding_CaptureReleaseIntoEnclosure,NULL,0,&output,&diagnostic));
        CHECK(output.type==NativeScriptType_U64&&output.as.u64==99&&trace_count==0&&h->victim_gobj==victim);++cases;
        HSD_GObjFree(victim);HSD_GObjFree(holder);
    }
    for(unsigned bad=0;bad<8;++bad){
        Fighter_GObj* holder=enclosure_pair(true,false,false);Fighter_GObj* victim=candidate;
        Fighter* v=victim->user_data;
        if(bad==0)v->parts=NULL;if(bad==1)victim->hsd_obj=NULL;
        if(bad==2)capture_package.assets.parts.parts_num=0;
        if(bad==3)capture_package.assets.parts.parts_num=MAX_FT_PARTS+1;
        if(bad==4)enclosure_bones[FtPart_TransN].joint=NULL;
        if(bad==5)capture_package.assets.part_to_joint[FtPart_TransN]=FTPART_INVALID;
        if(bad==6)++capture_package.assets.facts.omission_count;
        if(bad==7)v->x18=1;
        NativeCaptureEnclosureSettings settings=((Fighter*)holder->user_data)->capture_enclosure_settings;
        CHECK(!ftCo_CaptureEnclosure_Enter(victim,holder,ftCo_MS_YoshiEgg,&settings,&enclosure_model,1));
        CHECK(trace_count==0);++cases;HSD_GObjFree(victim);HSD_GObjFree(holder);
    }
    reset();Fighter_GObj* holder=create(true);
    CHECK(helper(holder,"ReleaseEnclosure")&&presentation_state(holder,"items",NULL).as.i64==99);
    CHECK(trace_count==0);++cases;HSD_GObjFree(holder);
}
