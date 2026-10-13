// Capture owner executes the VM; the selected victim receives the pose.
// Cleanup, bone attachment and ground-to-air leaves are controlled, while
// both entry recipes and the victim-motion body are extracted production C.
static void attached_capture_cases(void)
{
    NativeScriptDiagnostic diagnostic;
    const float timers[]={0,3.5f,27};
    for(unsigned owned=0;owned<2;++owned)for(unsigned owner_air=0;owner_air<2;++owner_air)
        for(unsigned victim_air=0;victim_air<2;++victim_air)for(unsigned facing=0;facing<2;++facing)
        for(unsigned freeze=0;freeze<2;++freeze)for(unsigned pose=0;pose<2;++pose)for(unsigned timer=0;timer<3;++timer){
            reset();capture_motion_case=true;
            Fighter_GObj* owner=create(true);Fighter_GObj* victim=create(owned!=0);link_pair(owner,victim);
            Fighter* h=owner->user_data;Fighter* v=victim->user_data;
            h->ground_or_air=owner_air;h->facing_dir=facing?-1:1;
            h->capture_settings.timer_damage_multiplier=timers[timer];
            h->capture_settings.wiggle_scale=0.5f+timer;h->capture_settings.wiggle_limit=9+timer;
            v->ground_or_air=victim_air;v->facing_dir=-h->facing_dir;v->x2222_b6=freeze;
            v->x221B_b5=v->x221B_b7=true;v->self_vel=(Vec3){3,4,5};v->x1A50=8;v->x1A51=9;
            FtMotionId motion=pose?ftCo_MS_CaptureDamageKoopaAir:ftCo_MS_CaptureDamageKoopa;
            NativeScriptValue input={.type=NativeScriptType_I32,.as.i64=motion},out;
            presentation_state(owner,"items",&input);
            Fighter before=*v,holder_before=*h;
            reference_attached_capture(victim,owner,motion);Fighter expected=*v;
            unsigned n=trace_count,expected_trace[32];memcpy(expected_trace,trace,sizeof(trace));
            CHECK(animation_donor==owner);*v=before;trace_count=0;animation_donor=NULL;
            NativeScriptRuntimeContext context=victim_policy_context(owner);
            CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_CaptureBeginAttached,NULL,0,&input,1,&out,&diagnostic));
            CHECK(out.type==NativeScriptType_Void&&!memcmp(v,&expected,sizeof(expected))&&!memcmp(h,&holder_before,sizeof(holder_before)));
            CHECK(trace_count==n&&!memcmp(trace,expected_trace,n*sizeof(*trace))&&animation_donor==owner);
            *v=before;trace_count=0;animation_donor=NULL;
            CHECK(helper(owner,"BeginAttached")&&!memcmp(v,&expected,sizeof(expected))&&!memcmp(h,&holder_before,sizeof(holder_before)));
            CHECK(trace_count==n&&!memcmp(trace,expected_trace,n*sizeof(*trace))&&animation_donor==owner);
            CHECK(presentation_state(owner,"victims",NULL).as.i64==99);
            HSD_GObjFree(victim);HSD_GObjFree(owner);++cases;
        }
    // Each authored boundary must stop before the next initialization stage.
    const unsigned stages[]={1,2,3,4,5,7,6,8};
    for(unsigned stage=0;stage<8;++stage)for(unsigned mode=0;mode<3;++mode){
        reset();capture_motion_case=true;
        Fighter_GObj* owner=create(true);Fighter_GObj* victim=create(true);link_pair(owner,victim);
        NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(owner,((Fighter*)owner->user_data)->x8_spawnNum,&view));
        NativeScriptValue input={.type=NativeScriptType_I32,.as.i64=ftCo_MS_CaptureDamageKoopa};
        presentation_state(owner,"items",&input);
        NativeScriptFighterProgram* program=(NativeScriptFighterProgram*)nativeScriptFighter_Program(((Fighter*)owner->user_data)->kind);
        uint64_t identity=program->identity;
        retire_step=stages[stage];retire_target=mode==1;victim_motion_retire_mode=mode==2?2:0;
        CHECK(!helper(owner,"BeginAttached")&&finish_saved_value(&view,"victims").as.i64!=99);
        CHECK(trace_count==stage+1&&trace[trace_count-1]==stages[stage]);
        program->identity=identity;HSD_GObjFree(victim);HSD_GObjFree(owner);++cases;
    }
    for(unsigned bad=0;bad<16;++bad){
        reset();capture_motion_case=true;
        Fighter_GObj* owner=create(true);Fighter_GObj* victim=create(true);link_pair(owner,victim);
        Fighter* h=owner->user_data;Fighter* v=victim->user_data;
        NativeScriptRuntimeContext context=victim_policy_context(owner);
        NativeScriptValue input={.type=NativeScriptType_I32,.as.i64=ftCo_MS_CaptureDamageKoopa};uint32_t count=1;
        if(bad==0)input.as.i64=-1;if(bad==1)input.as.i64=32768;if(bad==2)input.as.i64=347;
        if(bad==3)input.type=NativeScriptType_F32;if(bad==4)input.type=NativeScriptType_U32;
        if(bad==5)count=0;if(bad==6)count=2;if(bad==7)++context.generation;if(bad==8)++context.scene_epoch;
        if(bad==9)h->victim_gobj=NULL;if(bad==10)++h->native_hitlag_link_generation;
        if(bad==11)v->x1C_actionStateList=NULL;if(bad==12)v->x20_actionStateList=NULL;if(bad==13)++v->x18;
        if(bad==14)h->x1A5C=NULL;if(bad==15)input.as.i64=INT64_MAX;
        Fighter before=*v,holder_before=*h;NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=77};
        CHECK(!nativeScriptBindingCapture_Invoke(&context,NativeScriptBinding_CaptureBeginAttached,&input,count,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_U64&&out.as.u64==77&&!trace_count&&!memcmp(v,&before,sizeof(before))&&!memcmp(h,&holder_before,sizeof(holder_before)));
        HSD_GObjFree(victim);HSD_GObjFree(owner);++cases;
    }
    // The holder's freeze flag must never be substituted for its victim's.
    for(unsigned owned=0;owned<2;++owned)for(unsigned holder_flag=0;holder_flag<2;++holder_flag)
        for(unsigned victim_flag=0;victim_flag<2;++victim_flag){
            reset();Fighter_GObj* owner=create(true);Fighter_GObj* victim=create(owned!=0);link_pair(owner,victim);
            Fighter* h=owner->user_data;Fighter* v=victim->user_data;
            h->x2222_b6=holder_flag;v->x2222_b6=victim_flag;
            Fighter before=*v,holder_before=*h;NativeScriptValue value;
            NativeScriptRuntimeContext context=victim_policy_context(owner);
            CHECK(nativeScriptBindingCapture_Read(&context,NativeScriptBinding_CaptureVictimFreezeState,&value,&diagnostic));
            CHECK(value.type==NativeScriptType_Bool&&value.as.u64==victim_flag&&!memcmp(v,&before,sizeof(before))&&!memcmp(h,&holder_before,sizeof(holder_before)));
            CHECK(helper(owner,"VictimFreezeState")&&presentation_state(owner,"catches",NULL).as.i64==victim_flag);
            CHECK(!memcmp(v,&before,sizeof(before))&&!memcmp(h,&holder_before,sizeof(holder_before)));
            HSD_GObjFree(victim);HSD_GObjFree(owner);++cases;
        }
    for(unsigned flag=0;flag<2;++flag){
        reset();Fighter_GObj* owner=create(true);((Fighter*)owner->user_data)->x2222_b6=flag;
        CHECK(helper(owner,"VictimFreezeState")&&presentation_state(owner,"catches",NULL).as.i64==0);
        HSD_GObjFree(owner);++cases;
    }
    reset();
}
