static void jump_prepare_cases(void)
{
    for(unsigned input=0;input<4;++input)for(unsigned short_hop=0;short_hop<2;++short_hop){
        reset();Fighter_GObj* actor=create(true);Fighter* fp=actor->user_data;Fighter before=*fp,expected=before;
        // The original authored primitive assigns these two fields directly.
        expected.mv.co.kneebend.jump_input=(ftCo_JumpInput)input;expected.mv.co.kneebend.is_short_hop=short_hop;
        NativeScriptValue args[2]={{.type=NativeScriptType_I32,.as.i64=input},{.type=NativeScriptType_Bool,.as.u64=short_hop}},out;
        NativeScriptRuntimeContext context=victim_policy_context(actor);NativeScriptDiagnostic diagnostic;
        CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_JumpPrepare,NULL,0,args,2,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_Void&&!memcmp(fp,&expected,sizeof(expected)));
        *fp=before;presentation_state(actor,"items",args);
        CHECK(helper(actor,short_hop?"PrepareShortHop":"PrepareJump")&&!memcmp(fp,&expected,sizeof(expected)));
        HSD_GObjFree(actor);++cases;
    }
}

static void short_hop_cases(void)
{
    const HSD_Pad held[]={0,HSD_PAD_X,HSD_PAD_PROFILE_SHORT_HOP};
    const float y[]={-1,0,0.69f,0.7f,1},cy[]={-1,0.7f,1};
    p_ftCommonData->tap_jump_release_threshold=0.7f;
    for(unsigned input=0;input<4;++input)for(unsigned initial=0;initial<2;++initial)
        for(unsigned h=0;h<3;++h)for(unsigned s=0;s<5;++s)for(unsigned c=0;c<3;++c){
            reset();Fighter_GObj* actor=create(true);Fighter* fp=actor->user_data;
            fp->mv.co.kneebend.jump_input=(ftCo_JumpInput)input;fp->mv.co.kneebend.is_short_hop=initial;
            fp->input.held_buttons[0]=held[h];fp->input.lstick[0].y=y[s];fp->input.cstick[0].y=cy[c];
            Fighter before=*fp;reference_ftCo_KneeBend_Check_ShortHop(actor);Fighter expected=*fp;*fp=before;
            NativeScriptRuntimeContext context=victim_policy_context(actor);NativeScriptValue out;NativeScriptDiagnostic diagnostic;
            CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_JumpCheckShortHop,NULL,0,NULL,0,&out,&diagnostic));
            CHECK(out.type==NativeScriptType_Void&&!memcmp(fp,&expected,sizeof(expected)));
            *fp=before;CHECK(helper(actor,"CheckShortHop")&&!memcmp(fp,&expected,sizeof(expected)));
            HSD_GObjFree(actor);++cases;
        }
}

static void jump_launch_cases(void)
{
    const float multiplier[]={-1,0,0.5f,1,2},stick[]={-1,-0.25f,0,0.25f,1},vx[]={-3,-0.5f,0,0.5f,3};
    FtSFX sound={.x10=0x456};FtSFX* previous=capture_package.assets.character.data.x4C_sfx;
    capture_package.assets.character.data.x4C_sfx=&sound;HSD_GObj worn={0};
    p_ftCommonData->x438=0.75f;
    for(unsigned m=0;m<5;++m)for(unsigned s=0;s<5;++s)for(unsigned v=0;v<5;++v)
        for(unsigned hop=0;hop<2;++hop)for(unsigned audible=0;audible<2;++audible)for(unsigned item=0;item<2;++item){
            reset();jump_case=true;Fighter_GObj* actor=create(true);Fighter* fp=actor->user_data;
            fp->self_vel=(Vec3){vx[v],9,4};fp->input.lstick[0].x=stick[s];fp->x197C=item?&worn:NULL;
            fp->mv.co.jump.x0=hop;fp->mv.co.jump.x4=true;
            fp->co_attrs.ground_to_air_jump_momentum_multiplier=0.8f;fp->co_attrs.jump_h_initial_velocity=0.5f;
            fp->co_attrs.jump_h_max_velocity=0.75f;fp->co_attrs.hop_v_initial_velocity=3;fp->co_attrs.jump_v_initial_velocity=6;
            Fighter before=*fp;reference_ftCo_800CB110(actor,audible!=0,multiplier[m]);Fighter expected=*fp;
            unsigned n=trace_count;*fp=before;trace_count=0;
            NativeScriptValue args[2]={{.type=NativeScriptType_Bool,.as.u64=audible},{.type=NativeScriptType_F32,.as.f32=multiplier[m]}},out;
            NativeScriptRuntimeContext context=victim_policy_context(actor);NativeScriptDiagnostic diagnostic;
            CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_JumpLaunch,NULL,0,args,2,&out,&diagnostic));
            CHECK(out.type==NativeScriptType_Void&&!memcmp(fp,&expected,sizeof(expected))&&trace_count==n);
            CHECK(n==(audible?1+item:0)&&(!n||trace[0]==31)&&(n<2||trace[1]==32));
            *fp=before;trace_count=0;presentation_state(actor,"victim_frame",&args[1]);
            CHECK(helper(actor,audible?"LaunchJump":"LaunchSilentJump")&&!memcmp(fp,&expected,sizeof(expected))&&trace_count==n);
            HSD_GObjFree(actor);++cases;
        }
    capture_package.assets.character.data.x4C_sfx=previous;
}

static void jump_physics_cases(void)
{
    const float vy[]={-3,-1,0,1,3,6};
    for(unsigned advanced=0;advanced<2;++advanced)for(unsigned v=0;v<6;++v){
        reset();jump_case=true;Fighter_GObj* actor=create(true);Fighter* fp=actor->user_data;
        fp->mv.co.jump.x4=advanced!=0;fp->self_vel.y=vy[v];
        Fighter before=*fp;reference_ftCo_Jump_Phys_Inner(actor);Fighter expected=*fp;unsigned n=trace_count;
        *fp=before;trace_count=0;NativeScriptRuntimeContext context=victim_policy_context(actor);NativeScriptValue out;NativeScriptDiagnostic diagnostic;
        CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_PhysicsJumpMotion,NULL,0,NULL,0,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_Void&&!memcmp(fp,&expected,sizeof(expected))&&trace_count==n&&n==advanced);
        *fp=before;trace_count=0;CHECK(helper(actor,"JumpPhysics")&&!memcmp(fp,&expected,sizeof(expected))&&trace_count==n);
        HSD_GObjFree(actor);++cases;
    }
}

static void platform_drop_cases(void)
{
    const int motions[]={45,341,346},floors[]={-1,0,12};const float frame[]={0,3.5f,-2};
    p_ftCommonData->x46C=-0.5f;
    for(unsigned m=0;m<3;++m)for(unsigned flags=0;flags<2;++flags)for(unsigned f=0;f<3;++f)
        for(unsigned air=0;air<2;++air)for(unsigned floor=0;floor<3;++floor){
            reset();throw_case=true;Fighter_GObj* actor=create(true);Fighter* fp=actor->user_data;
            fp->ground_or_air=air?GA_Air:GA_Ground;fp->self_vel=(Vec3){-2,4,7};
            fp->coll_data.floor.index=floors[floor];fp->coll_data.floor_skip=99;fp->x671_timer_lstick_tilt_y=7;
            Fighter before=*fp;reference_ftCo_8009A184(actor,(FtMotionId)motions[m],flags?0x40:0,frame[f]);Fighter expected=*fp;
            unsigned n=trace_count;*fp=before;trace_count=0;
            NativeScriptValue args[3]={{.type=NativeScriptType_I32,.as.i64=motions[m]},
                {.type=NativeScriptType_U32,.as.u64=flags?0x40:0},{.type=NativeScriptType_F32,.as.f32=frame[f]}},out;
            NativeScriptRuntimeContext context=victim_policy_context(actor);NativeScriptDiagnostic diagnostic;
            CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_StatePlatformDrop,NULL,0,args,3,&out,&diagnostic));
            CHECK(out.type==NativeScriptType_Void&&!memcmp(fp,&expected,sizeof(expected))&&trace_count==n);
            CHECK(n==3&&trace[0]==6&&trace[1]==27&&trace[2]==5&&fp->coll_data.floor_skip==floors[floor]&&fp->x671_timer_lstick_tilt_y==0xfe);
            *fp=before;trace_count=0;presentation_state(actor,"items",args);
            NativeScriptValue flag={.type=NativeScriptType_I32,.as.i64=args[1].as.u64};presentation_state(actor,"catches",&flag);
            presentation_state(actor,"victim_frame",&args[2]);
            CHECK(helper(actor,"PlatformDrop")&&!memcmp(fp,&expected,sizeof(expected))&&trace_count==n);
            HSD_GObjFree(actor);++cases;
        }
    for(unsigned program_change=0;program_change<2;++program_change){
        reset();throw_case=true;Fighter_GObj* actor=create(true);Fighter* fp=actor->user_data;
        fp->coll_data.floor.index=12;fp->coll_data.floor_skip=99;fp->x671_timer_lstick_tilt_y=7;
        retirement_holder=actor;retire_step=5;victim_motion_retire_mode=program_change?2:0;
        NativeScriptFighterProgram* program=(NativeScriptFighterProgram*)nativeScriptFighter_Program(fp->kind);uint64_t identity=program->identity;
        NativeScriptValue value={.type=NativeScriptType_I32,.as.i64=341};presentation_state(actor,"items",&value);
        NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(actor,fp->x8_spawnNum,&view));
        CHECK(!helper(actor,"PlatformDrop")&&trace_count==3&&trace[2]==5);
        CHECK(fp->coll_data.floor_skip==99&&fp->x671_timer_lstick_tilt_y==7&&finish_saved_value(&view,"victims").as.i64==0);
        program->identity=identity;HSD_GObjFree(actor);++cases;
    }
}

static void jump_reject_cases(void)
{
    NativeScriptDiagnostic diagnostic;
    for(unsigned invalid=0;invalid<6;++invalid){
        reset();Fighter_GObj* actor=create(true);Fighter before=*(Fighter*)actor->user_data;
        NativeScriptRuntimeContext context=victim_policy_context(actor);NativeScriptValue args[2]={{.type=NativeScriptType_I32,.as.i64=1},{.type=NativeScriptType_Bool}},out={.type=NativeScriptType_U64,.as.u64=99};
        if(invalid==1)args[0].type=NativeScriptType_F32;if(invalid==2)args[0].as.i64=-1;
        if(invalid==3)args[0].as.i64=4;if(invalid==4)args[1].type=NativeScriptType_I32;if(invalid==5)context.scene_epoch=0;
        CHECK(!nativeScriptBindingJump_Invoke(&context,NativeScriptBinding_JumpPrepare,args,invalid==0?1:2,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&!memcmp(actor->user_data,&before,sizeof(before)));
        HSD_GObjFree(actor);++cases;
    }
    for(unsigned invalid=0;invalid<6;++invalid){
        reset();Fighter_GObj* actor=create(true);Fighter before=*(Fighter*)actor->user_data;
        NativeScriptRuntimeContext context=victim_policy_context(actor);NativeScriptValue args[2]={{.type=NativeScriptType_Bool,.as.u64=1},{.type=NativeScriptType_F32,.as.f32=1}},out={.type=NativeScriptType_U64,.as.u64=99};
        if(invalid==1)args[0].type=NativeScriptType_I32;if(invalid==2)args[1].as.f32=NAN;
        if(invalid==3)args[1].as.f32=INFINITY;if(invalid==5)context.source_class=NativeCallbackSource_Item;
        FtSFX* sound=((Fighter*)actor->user_data)->ft_data->x4C_sfx;
        if(invalid==4)((Fighter*)actor->user_data)->ft_data->x4C_sfx=NULL;
        CHECK(!nativeScriptBindingJump_Invoke(&context,NativeScriptBinding_JumpLaunch,args,invalid==0?1:2,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&!memcmp(actor->user_data,&before,sizeof(before)));
        ((Fighter*)actor->user_data)->ft_data->x4C_sfx=sound;
        HSD_GObjFree(actor);++cases;
    }
    const uint32_t steps[]={NativeScriptBinding_JumpCheckShortHop,NativeScriptBinding_PhysicsJumpMotion};
    for(unsigned s=0;s<2;++s)for(unsigned invalid=0;invalid<3;++invalid){
        reset();Fighter_GObj* actor=create(true);Fighter before=*(Fighter*)actor->user_data;
        NativeScriptRuntimeContext context=victim_policy_context(actor);NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};ftCommonData* previous=p_ftCommonData;
        if(invalid==1)p_ftCommonData=NULL;if(invalid==2)context.scene_epoch=0;
        CHECK(!nativeScriptBindingJump_Invoke(&context,steps[s],NULL,invalid==0?1:0,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&!memcmp(actor->user_data,&before,sizeof(before)));
        p_ftCommonData=previous;HSD_GObjFree(actor);++cases;
    }
    for(unsigned invalid=0;invalid<8;++invalid){
        reset();Fighter_GObj* actor=create(true);Fighter* fp=actor->user_data;
        NativeScriptRuntimeContext context=victim_policy_context(actor);NativeScriptValue args[3]={{.type=NativeScriptType_I32,.as.i64=341},{.type=NativeScriptType_U32},{.type=NativeScriptType_F32}},out={.type=NativeScriptType_U64,.as.u64=99};
        ftCommonData* previous=p_ftCommonData;
        if(invalid==1)args[0].type=NativeScriptType_F32;if(invalid==2)args[0].as.i64=347;
        if(invalid==3)args[1].type=NativeScriptType_I32;if(invalid==4)args[1].as.u64=UINT64_MAX;
        if(invalid==5)args[2].as.f32=NAN;if(invalid==6)fp->x20_actionStateList=NULL;if(invalid==7)p_ftCommonData=NULL;
        Fighter before=*fp;
        CHECK(!nativeScriptBindingCommonMovement_Invoke(&context,NativeScriptBinding_StatePlatformDrop,args,invalid==0?2:3,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&!memcmp(fp,&before,sizeof(before))&&trace_count==0);
        p_ftCommonData=previous;HSD_GObjFree(actor);++cases;
    }
}

static void jump_cases(void)
{
    ftCommonData common={0};ftCommonData* previous=p_ftCommonData;p_ftCommonData=&common;
    jump_prepare_cases();short_hop_cases();jump_launch_cases();jump_physics_cases();platform_drop_cases();jump_reject_cases();
    p_ftCommonData=previous;reset();
}
