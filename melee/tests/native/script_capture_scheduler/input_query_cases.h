static void input_query_compare(Fighter_GObj* actor,uint32_t semantic,const char* name,int expected)
{
    Fighter* fp=actor->user_data;Fighter before=*fp;
    NativeScriptRuntimeContext context=victim_policy_context(actor);
    NativeScriptValue out;NativeScriptDiagnostic diagnostic;
    CHECK(nativeScriptBindings_Invoke(&context,semantic,NULL,0,NULL,0,&out,&diagnostic));
    bool integer=semantic==NativeScriptBinding_ControlsJumpInput;
    CHECK(out.type==(integer?NativeScriptType_I32:NativeScriptType_Bool));
    CHECK((integer?out.as.i64:(int64_t)out.as.u64)==expected&&!memcmp(fp,&before,sizeof(before)));
    CHECK(helper(actor,name)&&presentation_state(actor,"items",NULL).as.i64==expected);
    CHECK(!memcmp(fp,&before,sizeof(before)));++cases;
}

static void directional_input_cases(void)
{
    const float stick[]={-1,-0.4f,-0.3f,0,0.3f,0.4f,1};
    for(unsigned facing=0;facing<2;++facing)for(unsigned s=0;s<7;++s)
        for(unsigned threshold=0;threshold<3;++threshold){
            reset();Fighter_GObj* actor=create(true);Fighter* fp=actor->user_data;
            fp->facing_dir=facing?1:-1;fp->input.lstick[0].x=stick[s];
            p_ftCommonData->walk_stick_threshold=threshold==0?0:threshold==1?0.3f:0.4f;
            p_ftCommonData->x34=-p_ftCommonData->walk_stick_threshold;
            input_query_compare(actor,NativeScriptBinding_ControlsWalkRequested,"WalkRequested",reference_ftWalkCommon_800DFC70(actor));
            input_query_compare(actor,NativeScriptBinding_ControlsWalkStopRequested,"WalkStopRequested",reference_ft_8008A1FC(actor));
            input_query_compare(actor,NativeScriptBinding_ControlsTurnRequested,"TurnRequested",reference_ftCo_800C97A8(actor));
            HSD_GObjFree(actor);
        }
}

static void platform_and_jump_input_cases(void)
{
    const float stick[]={-1,-0.7f,-0.69f,0,0.69f,0.7f,1};
    const u8 timer[]={0,2,3,0xfe};
    const u32 flags[]={0,LINE_FLAG_PLATFORM,LINE_FLAG_PLATFORM|2};
    const HSD_Pad buttons[]={0,HSD_PAD_X,HSD_PAD_Y,HSD_PAD_PROFILE_SHORT_HOP,HSD_PAD_B};
    p_ftCommonData->x464=0.7f;p_ftCommonData->x468=3;
    p_ftCommonData->tap_jump_threshold=0.7f;p_ftCommonData->tap_jump_window=3;
    for(unsigned s=0;s<7;++s)for(unsigned t=0;t<4;++t)for(unsigned f=0;f<3;++f){
        reset();Fighter_GObj* actor=create(true);Fighter* fp=actor->user_data;
        fp->input.lstick[0].y=stick[s];fp->x671_timer_lstick_tilt_y=timer[t];
        fp->coll_data.floor.index=input_floor_line=(int)(s+10);input_floor_flags=flags[f];
        input_query_compare(actor,NativeScriptBinding_ControlsPlatformDropRequested,"PlatformDropRequested",reference_ftCo_80099F1C(actor));
        HSD_GObjFree(actor);
    }
    for(unsigned s=0;s<7;++s)for(unsigned t=0;t<4;++t)for(unsigned tap=0;tap<2;++tap)for(unsigned b=0;b<5;++b){
        reset();Fighter_GObj* actor=create(true);Fighter* fp=actor->user_data;
        fp->input.lstick[0].y=stick[s];fp->x671_timer_lstick_tilt_y=timer[t];
        fp->input.held_buttons[0]=tap?0:HSD_PAD_PROFILE_TAP_JUMP_DISABLED;
        fp->input.pressed_buttons=buttons[b];
        input_query_compare(actor,NativeScriptBinding_ControlsJumpInput,"JumpInput",reference_ftCo_Jump_GetInput(actor));
        HSD_GObjFree(actor);
    }
}

static void throw_threshold_cases(void)
{
    const uint32_t bits[]={0,0x80000000,0x3f200000,0xbf400000,0x3f800000,0x7f800000,0x7fc00000};
    const uint32_t semantic[]={NativeScriptBinding_ControlsThrowHorizontalThreshold,NativeScriptBinding_ControlsThrowUpThreshold,NativeScriptBinding_ControlsThrowDownThreshold};
    const char* names[]={"ThrowHorizontalThreshold","ThrowUpThreshold","ThrowDownThreshold"};
    float* field[]={&p_ftCommonData->x98,&p_ftCommonData->attackhi3_stick_threshold_y,&p_ftCommonData->xB0};
    for(unsigned p=0;p<3;++p)for(unsigned b=0;b<7;++b){
        reset();Fighter_GObj* actor=create(true);Fighter* fp=actor->user_data;Fighter before=*fp;
        memcpy(field[p],&bits[b],sizeof(float));NativeScriptValue out;NativeScriptDiagnostic diagnostic;
        NativeScriptRuntimeContext context=victim_policy_context(actor);
        CHECK(nativeScriptBindingFighterObservations_Read(&context,semantic[p],&out,&diagnostic));
        CHECK(out.type==NativeScriptType_F32&&!memcmp(&out.as.f32,&bits[b],sizeof(float)));
        CHECK(helper(actor,names[p]));out=presentation_state(actor,"victim_frame",NULL);
        CHECK(!memcmp(&out.as.f32,&bits[b],sizeof(float))&&!memcmp(fp,&before,sizeof(before)));
        HSD_GObjFree(actor);++cases;
    }
    for(unsigned p=0;p<3;++p){
        reset();Fighter_GObj* actor=create(true);NativeScriptRuntimeContext context=victim_policy_context(actor);
        NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};NativeScriptDiagnostic diagnostic;
        ftCommonData* previous=p_ftCommonData;p_ftCommonData=NULL;
        CHECK(!nativeScriptBindingFighterObservations_Read(&context,semantic[p],&out,&diagnostic));
        CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);p_ftCommonData=previous;
        HSD_GObjFree(actor);++cases;
    }
}

static void victim_animation_family_cases(void)
{
    const int tags[]={Ft_Kind_Peach,Ft_Kind_Zelda,Ft_Kind_Mario,Ft_Kind_None};
    uint32_t saved_tag=capture_package.assets.facts.animation_tag;
    for(unsigned v=0;v<6;++v){
        reset();Fighter_GObj* actor=create(true);Fighter_GObj* victim=v==5?NULL:create(v!=4);
        capture_package.assets.facts.animation_tag=tags[v<4?v:0];
        if(victim)link_pair(actor,victim);
        Fighter before=*(Fighter*)actor->user_data,peer={0};if(victim)peer=*(Fighter*)victim->user_data;
        int expected=victim?reference_nativeFighterRuntime_AnimationFamily(victim->user_data):Ft_Kind_None;
        CHECK(expected==(v==5?Ft_Kind_None:v==4?Ft_Kind_Mario:tags[v]));
        NativeScriptRuntimeContext context=victim_policy_context(actor);NativeScriptValue out;NativeScriptDiagnostic diagnostic;
        CHECK(nativeScriptBindingCapture_Read(&context,NativeScriptBinding_CaptureVictimAnimationFamily,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_I32&&out.as.i64==expected);
        CHECK(helper(actor,"VictimFamily")&&presentation_state(actor,"items",NULL).as.i64==expected);
        CHECK(!memcmp(actor->user_data,&before,sizeof(before))&&(!victim||!memcmp(victim->user_data,&peer,sizeof(peer))));
        HSD_GObjFree(actor);if(victim)HSD_GObjFree(victim);++cases;
    }
    for(unsigned invalid=0;invalid<4;++invalid){
        reset();Fighter_GObj* actor=create(true);Fighter_GObj* victim=create(true);link_pair(actor,victim);
        Fighter* fp=actor->user_data;Fighter* vp=victim->user_data;
        if(invalid==0)++vp->x8_spawnNum;if(invalid==1)fp->x1A5C=NULL;
        if(invalid==2)victim->classifier=HSD_GOBJ_CLASS_ITEM;if(invalid==3)++fp->native_hitlag_link_generation;
        Fighter before=*fp,peer=*vp;NativeScriptRuntimeContext context=victim_policy_context(actor);
        NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};NativeScriptDiagnostic diagnostic;
        CHECK(!nativeScriptBindingCapture_Read(&context,NativeScriptBinding_CaptureVictimAnimationFamily,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&!memcmp(fp,&before,sizeof(before))&&!memcmp(vp,&peer,sizeof(peer)));
        victim->classifier=HSD_GOBJ_CLASS_FIGHTER;HSD_GObjFree(actor);HSD_GObjFree(victim);++cases;
    }
    capture_package.assets.facts.animation_tag=saved_tag;
}

static void input_query_reject_cases(void)
{
    const uint32_t semantic[]={NativeScriptBinding_ControlsWalkRequested,NativeScriptBinding_ControlsWalkStopRequested,
        NativeScriptBinding_ControlsTurnRequested,NativeScriptBinding_ControlsPlatformDropRequested,NativeScriptBinding_ControlsJumpInput};
    for(unsigned s=0;s<5;++s)for(unsigned invalid=0;invalid<4;++invalid){
        reset();Fighter_GObj* actor=create(true);Fighter before=*(Fighter*)actor->user_data;
        NativeScriptRuntimeContext context=victim_policy_context(actor);NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
        NativeScriptDiagnostic diagnostic;ftCommonData* previous=p_ftCommonData;
        if(invalid==1)p_ftCommonData=NULL;if(invalid==2)context.scene_epoch=0;
        if(invalid==3)context.source_class=NativeCallbackSource_Item;
        CHECK(!nativeScriptBindingCommonMovement_Invoke(&context,semantic[s],NULL,invalid==0?1:0,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&!memcmp(actor->user_data,&before,sizeof(before)));
        p_ftCommonData=previous;HSD_GObjFree(actor);++cases;
    }
}

static void input_query_cases(void)
{
    ftCommonData common={0};ftCommonData* previous=p_ftCommonData;p_ftCommonData=&common;
    directional_input_cases();platform_and_jump_input_cases();throw_threshold_cases();
    victim_animation_family_cases();input_query_reject_cases();
    p_ftCommonData=previous;input_floor_flags=0;input_floor_line=0;reset();
}
