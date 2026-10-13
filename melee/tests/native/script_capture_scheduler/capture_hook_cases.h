static Fighter_GObj* capture_hook_pair(bool owned_victim)
{
    Fighter_GObj* holder=enclosure_pair(owned_victim,false,false);
    Fighter* h=holder->user_data;
    static __typeof__(*h->ft_data->x8) grab_parts;
    grab_parts.x11=FtPart_TransN2;h->ft_data->x8=&grab_parts;
    // The actual CapturePulled entry establishes both reciprocal links before
    // common throw entry; the old fixture supplied only victim_gobj.
    Fighter* v=candidate->user_data;
    v->victim_gobj=v->x1A5C=holder;
    v->native_hitlag_link_generation=h->x8_spawnNum;
    capture_motion_case=true;
    return holder;
}

static void capture_hook_cases(void)
{
    // A returned float chooses the shared next-motion start, not a new motion.
    const int remaps[]={0,1,29,100};
    for(unsigned owned=0;owned<2;++owned)for(unsigned n=0;n<4;++n){
        Fighter_GObj* holder=capture_hook_pair(owned);Fighter_GObj* victim=candidate;
        Fighter* h=holder->user_data;h->motion_id=212;h->cur_anim_frame=3;h->gr_vel=8;
        NativeScriptValue input={.type=NativeScriptType_I32,.as.i64=remaps[n]};
        presentation_state(holder,"items",&input);fn_800D9CE8(holder);
        CHECK(h->motion_id==213&&h->gr_vel==0&&h->cur_pos.x==1);
        CHECK(capture_motion_frame==remaps[n]/4.0f&&capture_motion_rate==1&&capture_motion_flags==0x4000);
        CHECK(h->mv.co.capturedamage.x18==enclosure_bones[FtPart_TransN2].joint);
        CHECK(h->accessory1_cb==fn_800DA190&&trace_count==1&&trace[0]==5);
        HSD_GObjFree(victim);HSD_GObjFree(holder);++cases;
    }
    // Dash-catch keeps its native frame and never invokes the standing hook.
    Fighter_GObj* holder=capture_hook_pair(false);Fighter_GObj* victim=candidate;
    Fighter* h=holder->user_data;h->motion_id=214;h->cur_anim_frame=3;
    fn_800D9CE8(holder);CHECK(h->motion_id==215&&capture_motion_frame==3&&h->cur_pos.x==0);
    HSD_GObjFree(victim);HSD_GObjFree(holder);++cases;

    // The throw hook runs on the holder, for stock and interpreted victims.
    for(unsigned owned=0;owned<2;++owned){
        holder=capture_hook_pair(owned);victim=candidate;h=holder->user_data;
        Fighter* v=victim->user_data;v->grab_timer=17;v->x1988=0;v->x1A6A=0;
        ftCo_800DE3FC(victim,ftCo_MS_ThrownHi,0.75f);
        CHECK(v->motion_id==ftCo_MS_ThrownHi&&animation_donor==holder);
        CHECK(capture_motion_frame==0&&capture_motion_rate==0.75f&&capture_motion_flags==0);
        CHECK(v->x1988==2&&h->cur_pos.x==1&&v->grab_timer==0&&v->x1A6A==511);
        CHECK(v->accessory1_cb==ftCo_800DE508&&presentation_state(holder,"catches",NULL).as.i64==1);
        CHECK(trace_count==6&&trace[0]==4&&trace[1]==5&&trace[2]==11&&trace[3]==16&&trace[4]==17&&trace[5]==7);
        HSD_GObjFree(victim);HSD_GObjFree(holder);++cases;
    }
    // Absent hooks retain native defaults instead of the typed neutral result.
    const uint32_t hooks[]={NativeScriptFighter_CatchPullFrame,NativeScriptFighter_ThrowVictimSetup};
    for(unsigned n=0;n<2;++n){
        uint32_t saved=capture_package.program.hooks[hooks[n]];
        capture_package.program.hooks[hooks[n]]=UINT32_MAX;
        CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));
        nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&capture_package.program);
        holder=capture_hook_pair(true);victim=candidate;h=holder->user_data;Fighter* v=victim->user_data;
        if(!n){h->motion_id=212;h->cur_anim_frame=3;fn_800D9CE8(holder);CHECK(capture_motion_frame==3&&h->cur_pos.x==0);}
        else{v->grab_timer=17;v->x1988=0;ftCo_800DE3FC(victim,ftCo_MS_ThrownF,1);CHECK(v->x1988==0&&h->cur_pos.x==0&&v->grab_timer==0);}
        HSD_GObjFree(victim);HSD_GObjFree(holder);++cases;
        capture_package.program.hooks[hooks[n]]=saved;
        CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));
        nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&capture_package.program);
    }
    // Reject native continuation when a callback faults or its actor/schema retires.
    for(unsigned thrown=0;thrown<2;++thrown)for(unsigned bad=1;bad<=7;++bad){
        holder=capture_hook_pair(true);victim=candidate;h=holder->user_data;Fighter* v=victim->user_data;
        h->motion_id=212;h->cur_anim_frame=3;v->grab_timer=17;v->x1A6A=0;
        presentation_retire_mode=bad;presentation_retire_at=1;
        if(thrown){ftCo_800DE3FC(victim,ftCo_MS_ThrownF,1);CHECK(v->grab_timer==17&&v->x1A6A==0&&v->accessory1_cb!=ftCo_800DE508);}
        else{fn_800D9CE8(holder);CHECK(h->motion_id==212&&!trace_count);}
        presentation_restore();HSD_GObjFree(victim);HSD_GObjFree(holder);++cases;
    }
    // Successful VM return still must retain both sides of the capture relation.
    for(unsigned thrown=0;thrown<2;++thrown)for(unsigned bad=1;bad<=3;++bad){
        holder=capture_hook_pair(true);victim=candidate;h=holder->user_data;Fighter* v=victim->user_data;
        h->motion_id=212;h->cur_anim_frame=3;v->grab_timer=17;v->x1A6A=0;capture_hook_peer_change=bad;
        if(thrown){ftCo_800DE3FC(victim,ftCo_MS_ThrownF,1);CHECK(v->grab_timer==17&&v->x1A6A==0);}
        else{fn_800D9CE8(holder);CHECK(h->motion_id==212&&!trace_count);}
        HSD_GObjFree(victim);HSD_GObjFree(holder);++cases;
    }
    // Victim retirement during the shared motion change prevents hook dispatch.
    for(unsigned thrown=0;thrown<2;++thrown){
        holder=capture_hook_pair(true);victim=candidate;h=holder->user_data;Fighter* v=victim->user_data;
        h->motion_id=212;v->grab_timer=17;v->x1988=0;v->x1A6A=0;retire_step=5;retire_target=1;
        if(thrown){ftCo_800DE3FC(victim,ftCo_MS_ThrownF,1);CHECK(v->x1988==0&&v->grab_timer==17&&v->x1A6A==0);}
        else{fn_800D9CE8(holder);CHECK(h->accessory1_cb!=fn_800DA190);}
        HSD_GObjFree(victim);HSD_GObjFree(holder);++cases;
    }
}
