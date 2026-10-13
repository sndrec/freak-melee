static void caught_policy_cases(void)
{
    // Complete caught-capsule recipe against its pinned native implementation.
    for(unsigned owned=0;owned<2;++owned)for(unsigned scaled=0;scaled<3;++scaled){
        Fighter_GObj* holder=enclosure_pair(owned,false,false);Fighter_GObj* victim=candidate;
        Fighter* v=victim->user_data;v->x34_scale.y=scaled?scaled*0.75f:1;
        v->co_attrs.model_scaling=scaled?2:1;v->hurt_capsules_len=2;
        Fighter before=*v;
        CHECK(helper(holder,"EncloseCaught"));Fighter after=*v;
        CHECK(presentation_state(holder,"items",NULL).as.i64==99);
        CHECK(trace_count==1&&trace[0]==15);
        *v=before;trace_count=0;original_enclose_caught(holder,7);
        CHECK(!memcmp(v,&after,sizeof(after))&&trace_count==1&&trace[0]==15);
        CHECK(v->hurt_capsules[0].capsule.scale==7/ftCommon_GetModelScale(v));
        HSD_GObjFree(victim);HSD_GObjFree(holder);++cases;
    }
    // The native graph-constraint leaf remains controlled, including its flag.
    for(unsigned owned=0;owned<2;++owned){
        Fighter_GObj* holder=enclosure_pair(owned,false,false);Fighter_GObj* victim=candidate;
        Fighter* h=holder->user_data;Fighter* v=victim->user_data;
        CHECK(helper(holder,"ConstrainCaught")&&v->x2226_b2);
        CHECK(trace_count==1&&trace[0]==4&&presentation_state(holder,"items",NULL).as.i64==99);
        CHECK(h->victim_gobj==victim);HSD_GObjFree(victim);HSD_GObjFree(holder);++cases;
    }
    // Setup runs on the holder and masks the victim only after successful return.
    for(unsigned owned=0;owned<2;++owned)for(unsigned retirement=0;retirement<8;++retirement){
        Fighter_GObj* holder=enclosure_pair(owned,false,false);Fighter_GObj* victim=candidate;
        Fighter* h=holder->user_data;Fighter* v=victim->user_data;
        v->x34_scale.y=v->co_attrs.model_scaling=1;v->x1A6A=0;
        v->victim_gobj=holder;
        if(retirement){presentation_retire_mode=retirement;presentation_retire_at=1;}
        ftCo_CaptureWait_Setup(victim);
        CHECK(v->invisible&&v->accessory1_cb==ftCo_800DB464);
        CHECK(v->x1A6A==(retirement?0:511));
        if(!retirement)CHECK(h->cur_pos.x==1);
        presentation_restore();HSD_GObjFree(victim);HSD_GObjFree(holder);++cases;
    }
    // Missing setup hook keeps the generic mask; no stock fighter policy runs.
    CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));
    uint32_t original=capture_package.program.hooks[NativeScriptFighter_CaptureWaitSetup];
    capture_package.program.hooks[NativeScriptFighter_CaptureWaitSetup]=UINT32_MAX;
    nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&capture_package.program);
    Fighter_GObj* holder=enclosure_pair(true,false,false);Fighter_GObj* victim=candidate;
    Fighter* v=victim->user_data;v->invisible=false;v->x1A6A=0;
    v->victim_gobj=holder;
    ftCo_CaptureWait_Setup(victim);CHECK(!v->invisible&&v->x1A6A==511&&!trace_count);
    HSD_GObjFree(victim);HSD_GObjFree(holder);++cases;
    capture_package.program.hooks[NativeScriptFighter_CaptureWaitSetup]=original;
    CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));
    nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&capture_package.program);
    // Invalid geometry is rejected before any native shape change.
    for(unsigned bad=0;bad<8;++bad){
        holder=enclosure_pair(true,false,false);victim=candidate;v=victim->user_data;
        v->x34_scale.y=v->co_attrs.model_scaling=1;
        if(bad==0)v->parts=NULL;if(bad==1)victim->hsd_obj=NULL;
        if(bad==2)enclosure_bones[FtPart_XRotN].joint=NULL;
        if(bad==3)capture_package.assets.part_to_joint[FtPart_XRotN]=FTPART_INVALID;
        if(bad==4)v->x34_scale.y=0;if(bad==5)v->co_attrs.model_scaling=-1;
        if(bad==6)v->co_attrs.model_scaling=NAN;if(bad==7)v->co_attrs.model_scaling=INFINITY;
        Fighter before=*v;CHECK(!helper(holder,"EncloseCaught"));
        CHECK(!memcmp(v,&before,sizeof(before))&&!trace_count);
        HSD_GObjFree(victim);HSD_GObjFree(holder);++cases;
    }
    // A retired victim or source at the graph leaf stops script continuation.
    for(unsigned bad=0;bad<3;++bad){
        holder=enclosure_pair(true,false,false);victim=candidate;
        if(bad==0)enclosure_bones[FtPart_TransN2].joint=NULL;
        if(bad==1)enclosure_bones[FtPart_XRotN].joint=NULL;
        if(bad==2)((Fighter*)holder->user_data)->native_hitlag_link_generation++;
        Fighter before=*(Fighter*)victim->user_data;
        CHECK(!helper(holder,"ConstrainCaught")&&!trace_count);
        CHECK(!memcmp(&before,victim->user_data,sizeof(before)));
        HSD_GObjFree(victim);HSD_GObjFree(holder);++cases;
    }
    for(unsigned target=0;target<2;++target){
        holder=enclosure_pair(true,false,false);victim=candidate;
        NativeScriptReceiverView view;uint8_t before[NATIVE_SCRIPT_FIGHTER_STATE_BYTES];
        CHECK(nativeScriptFighter_View(holder,((Fighter*)holder->user_data)->x8_spawnNum,&view));
        CHECK(view.storage->capacity<=sizeof(before));
        memcpy(before,view.storage->state,view.storage->capacity);
        retire_step=4;retire_target=target;
        CHECK(!helper(holder,"ConstrainCaught")&&trace_count==1);
        CHECK(!memcmp(before,view.storage->state,view.storage->capacity));
        HSD_GObjFree(victim);HSD_GObjFree(holder);++cases;
    }
    reset();holder=create(true);
    CHECK(helper(holder,"EncloseCaught")&&helper(holder,"ConstrainCaught")&&!trace_count);
    NativeScriptRuntimeContext c=victim_policy_context(holder);NativeScriptDiagnostic d;
    NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
    const float radii[]={-1,NAN,INFINITY};
    for(unsigned n=0;n<3;++n){NativeScriptValue arg={.type=NativeScriptType_F32,.as.f32=radii[n]};
        CHECK(!nativeScriptBindingCapture_Invoke(&c,NativeScriptBinding_CaptureEncloseCaught,&arg,1,&out,&d));++cases;}
    CHECK(!nativeScriptBindingCapture_Invoke(&c,NativeScriptBinding_CaptureConstrainCaught,&out,1,&out,&d));
    CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&!trace_count);
    HSD_GObjFree(holder);++cases;
}
