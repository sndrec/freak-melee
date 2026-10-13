// Entry/tick/movement bodies are pinned; geometry, animation and release leaves
// remain controlled. This is common-engine parity, not a rendered match test.
static void throw_pose_cases(void)
{
    NativeScriptDiagnostic diagnostic;
    const uint32_t semantics[]={NativeScriptBinding_CaptureSetVictimHoldMotion,
        NativeScriptBinding_CaptureSetVictimThrowMotion,NativeScriptBinding_CaptureSetVictimAttachedThrow};
    const char* helpers[]={"HoldPose","ThrowPose","AttachedThrow"};
    for(unsigned owned=0;owned<2;++owned)for(unsigned op=0;op<3;++op)for(unsigned same=0;same<2;++same){
        reset();throw_case=true;Fighter_GObj* h=create(true);Fighter_GObj* v=create(owned!=0);reciprocal_pair(h,v);
        Fighter* holder=h->user_data;Fighter* victim=v->user_data;
        FtMotionId motion=op?ftCo_MS_ThrownF:ftCo_MS_ShoulderedWait;
        victim->motion_id=same?motion:ftCo_MS_Wait;victim->grab_timer=17;victim->facing_dir=-1;
        holder->facing_dir=1;victim->self_vel=(Vec3){1,2,3};victim->cur_anim_frame=17.25f;
        Fighter hs=*holder,vs=*victim;
        if(!op)reference_hold_motion(v,motion);
        else if(op==1)reference_throw_pose(v,motion,0.5f);
        else reference_attached_throw(v,motion,0x0c4c5080,3.5f,true);
        Fighter he=*holder,ve=*victim;unsigned expected[32],n=trace_count;
        memcpy(expected,trace,sizeof(trace));float start=capture_motion_frame,rate=capture_motion_rate;u32 flags=capture_motion_flags;
        HSD_GObj* donor=animation_donor;
        *holder=hs;*victim=vs;trace_count=0;
        NativeScriptValue args[]={{.type=NativeScriptType_I32,.as.i64=motion},
            {.type=op==1?NativeScriptType_F32:NativeScriptType_U32},{.type=NativeScriptType_F32,.as.f32=3.5f},
            {.type=NativeScriptType_Bool,.as.u64=1}},out;
        if(op==1)args[1].as.f32=0.5f;else args[1].as.u64=0x0c4c5080;
        NativeScriptRuntimeContext context=victim_policy_context(h);
        CHECK(nativeScriptBindings_Invoke(&context,semantics[op],NULL,0,args,op==2?4:op==1?2:1,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_Void&&!memcmp(holder,&he,sizeof(he))&&!memcmp(victim,&ve,sizeof(ve)));
        CHECK(trace_count==n&&!memcmp(trace,expected,n*sizeof(*trace)));
        CHECK(!n || (capture_motion_flags==flags&&capture_motion_frame==start&&capture_motion_rate==rate&&animation_donor==donor));
        *holder=hs;*victim=vs;trace_count=0;presentation_state(h,"items",args);
        CHECK(helper(h,helpers[op])&&!memcmp(holder,&he,sizeof(he))&&!memcmp(victim,&ve,sizeof(ve)));
        CHECK(trace_count==n);HSD_GObjFree(v);HSD_GObjFree(h);++cases;
    }
    // The caller chooses rate, starting frame, flags and initial-update policy.
    for(unsigned initialize=0;initialize<2;++initialize)for(unsigned rate_index=0;rate_index<3;++rate_index){
        reset();throw_case=true;Fighter_GObj* h=create(true);Fighter_GObj* v=create(false);reciprocal_pair(h,v);
        Fighter* victim=v->user_data;victim->grab_timer=17;Fighter before=*victim;
        float rate=rate_index==0?-1:rate_index==1?0:2;
        reference_throw_pose(v,ftCo_MS_ThrownF,rate);Fighter expected=*victim;*victim=before;trace_count=0;
        NativeScriptRuntimeContext context=victim_policy_context(h);
        NativeScriptValue args[]={{.type=NativeScriptType_I32,.as.i64=ftCo_MS_ThrownF},{.type=NativeScriptType_F32,.as.f32=rate}},out;
        CHECK(nativeScriptBindings_Invoke(&context,semantics[1],NULL,0,args,2,&out,&diagnostic));
        CHECK(!memcmp(victim,&expected,sizeof(expected))&&capture_motion_rate==rate);
        *victim=before;trace_count=0;reference_attached_throw(v,ftCo_MS_ThrownF,0,rate,initialize!=0);
        expected=*victim;unsigned n=trace_count;*victim=before;trace_count=0;
        NativeScriptValue attached[]={{.type=NativeScriptType_I32,.as.i64=ftCo_MS_ThrownF},
            {.type=NativeScriptType_U32,.as.u64=0},{.type=NativeScriptType_F32,.as.f32=rate},
            {.type=NativeScriptType_Bool,.as.u64=initialize}};
        CHECK(nativeScriptBindings_Invoke(&context,semantics[2],NULL,0,attached,4,&out,&diagnostic));
        CHECK(!memcmp(victim,&expected,sizeof(expected))&&trace_count==n&&capture_motion_frame==rate&&capture_motion_flags==0);
        HSD_GObjFree(v);HSD_GObjFree(h);++cases;
    }
}

static void throw_tick_cases(void)
{
    NativeScriptDiagnostic diagnostic;
    for(unsigned owned=0;owned<2;++owned)for(unsigned has=0;has<2;++has)
        for(unsigned reverse=0;reverse<2;++reverse)for(unsigned release=0;release<2;++release)
        for(unsigned pause=0;pause<2;++pause)for(unsigned paused=0;paused<2;++paused)for(unsigned down=0;down<2;++down){
            reset();throw_case=true;Fighter_GObj* h=create(true);Fighter_GObj* v=has?create(owned!=0):NULL;
            if(v)reciprocal_pair(h,v);Fighter* holder=h->user_data;Fighter* victim=v?v->user_data:NULL;
            holder->throw_flags=0;holder->throw_flags_b4=reverse;holder->throw_flags_b3=release;
            holder->cmd_vars[0]=pause;holder->mv.co.throw.x4=paused;holder->facing_dir=-1;
            holder->motion_id=down?ftCo_MS_ThrowLw:ftCo_MS_ThrowF;holder->cur_anim_frame=12.5f;
            holder->x6A4_transNOffset=(Vec3){1,2,3};holder->frame_speed_mul=1;holder->x1A6A=511;
            Fighter hs=*holder,vs={0};if(v){victim->grab_timer=17;victim->cur_anim_frame=down?12.5f:10;vs=*victim;}
            reference_throw_tick(h);Fighter he=*holder,ve={0};if(v)ve=*victim;
            unsigned expected[32],n=trace_count;memcpy(expected,trace,sizeof(trace));
            HSD_GObj* credit=release_credit;bool upward=release_upward;
            *holder=hs;if(v)*victim=vs;trace_count=0;release_credit=NULL;release_upward=false;
            NativeScriptRuntimeContext context=victim_policy_context(h);NativeScriptValue out;
            CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_ThrowsUpdate,NULL,0,NULL,0,&out,&diagnostic));
            CHECK(out.type==NativeScriptType_Void&&!memcmp(holder,&he,sizeof(he))&&(!v||!memcmp(victim,&ve,sizeof(ve))));
            CHECK(trace_count==n&&!memcmp(trace,expected,n*sizeof(*trace))&&release_credit==credit&&release_upward==upward);
            *holder=hs;if(v)*victim=vs;trace_count=0;
            CHECK(helper(h,"ThrowTick")&&!memcmp(holder,&he,sizeof(he))&&(!v||!memcmp(victim,&ve,sizeof(ve))));
            if(v)HSD_GObjFree(v);HSD_GObjFree(h);++cases;
        }
    for(unsigned air=0;air<2;++air)for(unsigned paused=0;paused<2;++paused)for(unsigned left=0;left<2;++left){
        reset();throw_case=true;Fighter_GObj* h=create(true);Fighter* fp=h->user_data;
        fp->ground_or_air=air?GA_Air:GA_Ground;fp->mv.co.throw.x4=paused;
        fp->facing_dir=left?-1:1;fp->mv.co.throw.xC=(Vec3){1,2,3};fp->self_vel=(Vec3){4,5,6};
        Fighter before=*fp;reference_throw_motion(h);Fighter expected=*fp;unsigned n=trace_count;
        *fp=before;trace_count=0;NativeScriptRuntimeContext context=victim_policy_context(h);NativeScriptValue out;
        CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_PhysicsThrowMotion,NULL,0,NULL,0,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_Void&&!memcmp(fp,&expected,sizeof(expected))&&trace_count==n);
        *fp=before;trace_count=0;CHECK(helper(h,"ThrowMotion")&&!memcmp(fp,&expected,sizeof(expected)));
        HSD_GObjFree(h);++cases;
    }
}

static void throw_boundary_cases(void)
{
    NativeScriptDiagnostic diagnostic;
    for(unsigned owned=0;owned<2;++owned){
        reset();Fighter_GObj* h=create(true);Fighter_GObj* v=create(owned!=0);reciprocal_pair(h,v);
        Fighter* holder=h->user_data;Fighter* victim=v->user_data;holder->facing_dir=-1;victim->facing_dir=1;
        victim->cur_anim_frame=7.25f;Fighter before=*victim;
        CHECK(helper(h,"SyncFacing"));before.facing_dir=-1;CHECK(!memcmp(victim,&before,sizeof(before)));
        CHECK(helper(h,"VictimFrame")&&presentation_state(h,"victim_frame",NULL).as.f32==7.25f);
        HSD_GObjFree(v);holder->victim_gobj=holder->x1A5C=NULL;holder->native_hitlag_link_generation=0;
        const uint32_t no_victim[]={NativeScriptBinding_CaptureSetVictimHoldMotion,NativeScriptBinding_CaptureSetVictimThrowMotion,
            NativeScriptBinding_CaptureSetVictimAttachedThrow,NativeScriptBinding_CaptureSyncVictimFacing};
        NativeScriptRuntimeContext context=victim_policy_context(h);
        NativeScriptValue args[]={{.type=NativeScriptType_I32,.as.i64=ftCo_MS_ThrownF},
            {.type=NativeScriptType_F32,.as.f32=1},{.type=NativeScriptType_F32,.as.f32=0},{.type=NativeScriptType_Bool,.as.u64=1}},out;
        for(unsigned op=0;op<4;++op){if(op==2)args[1]=(NativeScriptValue){.type=NativeScriptType_U32,.as.u64=0};
            CHECK(nativeScriptBindings_Invoke(&context,no_victim[op],NULL,0,args,op==0?1:op==1?2:op==2?4:0,&out,&diagnostic));CHECK(out.type==NativeScriptType_Void);}
        CHECK(helper(h,"VictimFrame")&&presentation_state(h,"victim_frame",NULL).as.f32==0);
        HSD_GObjFree(h);++cases;
    }
    for(unsigned op=0;op<3;++op)for(unsigned bad=0;bad<9;++bad){
        reset();throw_case=true;Fighter_GObj* h=create(true);Fighter_GObj* v=create(true);reciprocal_pair(h,v);
        Fighter* holder=h->user_data;Fighter* victim=v->user_data;
        uint32_t semantic=op==0?NativeScriptBinding_CaptureSetVictimHoldMotion:op==1?NativeScriptBinding_CaptureSetVictimThrowMotion:NativeScriptBinding_CaptureSetVictimAttachedThrow;
        NativeScriptValue args[]={{.type=NativeScriptType_I32,.as.i64=ftCo_MS_ThrownF},
            {.type=op==1?NativeScriptType_F32:NativeScriptType_U32},{.type=NativeScriptType_F32,.as.f32=0},{.type=NativeScriptType_Bool,.as.u64=1}};
        if(op==1)args[1].as.f32=1;uint32_t n=op==0?1:op==1?2:4;
        NativeScriptRuntimeContext context=victim_policy_context(h);
        if(bad==0)args[0].as.i64=-1;if(bad==1)args[0].as.i64=347;if(bad==2)args[0].type=NativeScriptType_U32;
        if(bad==3)n=0;if(bad==4)++n;if(bad==5)victim->victim_gobj=NULL;if(bad==6)++context.generation;
        if(bad==7){if(!op)++context.scene_epoch;else if(op==1)args[1].as.f32=INFINITY;else args[2].as.f32=NAN;}
        if(bad==8){if(!op)victim->x20_actionStateList=NULL;else if(op==1)args[1].type=NativeScriptType_I32;else args[1].as.u64=UINT64_MAX;}
        Fighter hs=*holder,vs=*victim;NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
        CHECK(!nativeScriptBindingCapture_Invoke(&context,semantic,args,n,&out,&diagnostic));
        CHECK(!trace_count&&!memcmp(holder,&hs,sizeof(hs))&&!memcmp(victim,&vs,sizeof(vs))&&out.type==NativeScriptType_U64&&out.as.u64==99);
        HSD_GObjFree(v);HSD_GObjFree(h);++cases;
    }
    for(unsigned op=0;op<2;++op)for(unsigned target=0;target<4;++target){
        reset();throw_case=true;Fighter_GObj* h=create(true);Fighter_GObj* v=create(true);reciprocal_pair(h,v);
        NativeScriptRuntimeContext context=victim_policy_context(h);retire_step=5;retire_target=target==1;victim_motion_retire_mode=target>=2?target-1:0;
        NativeScriptFighterProgram* program=(NativeScriptFighterProgram*)nativeScriptFighter_Program(((Fighter*)h->user_data)->kind);uint64_t identity=program->identity;
        NativeScriptValue args[]={{.type=NativeScriptType_I32,.as.i64=ftCo_MS_ThrownF},
            {.type=op?NativeScriptType_U32:NativeScriptType_F32},{.type=NativeScriptType_F32,.as.f32=0},{.type=NativeScriptType_Bool,.as.u64=1}},out={.type=NativeScriptType_U64,.as.u64=99};
        if(!op)args[1].as.f32=1;
        CHECK(!nativeScriptBindingCapture_Invoke(&context,op?NativeScriptBinding_CaptureSetVictimAttachedThrow:NativeScriptBinding_CaptureSetVictimThrowMotion,args,op?4:2,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&trace[trace_count-1]==5);
        CHECK(((Fighter*)v->user_data)->accessory1_cb==NULL);
        program->identity=identity;HSD_GObjFree(v);HSD_GObjFree(h);++cases;
    }
    const unsigned boundaries[]={16,9,10,18};
    for(unsigned boundary=0;boundary<4;++boundary)for(unsigned target=0;target<2;++target){
        reset();throw_case=true;Fighter_GObj* h=create(true);Fighter_GObj* v=create(true);reciprocal_pair(h,v);
        Fighter* fp=h->user_data;((Fighter*)v->user_data)->grab_timer=17;
        fp->throw_flags_b3=boundary<3;fp->cmd_vars[0]=1;
        NativeScriptRuntimeContext context=victim_policy_context(h);retire_step=boundaries[boundary];retire_target=target;
        NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
        CHECK(!nativeScriptBindingCapture_Invoke(&context,NativeScriptBinding_ThrowsUpdate,NULL,0,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&trace[trace_count-1]==retire_step);
        CHECK(!fp->mv.co.throw.x4 || boundary==3);
        HSD_GObjFree(v);HSD_GObjFree(h);++cases;
    }
}

static void throw_release_lifetime_cases(void)
{
    ftCommonData common={.x154=0.4f,.x158=10,.x15C=20,.x160=30,.x100=0.03f,
        .xFC=10,.x144_radians=0.7853982f,.x148=44,.x14C=10,.x150=20,
        .x190=0.75f,.x23C=1000,.x208=2000,.x20C=2000,.x5E8=2000,
        .x12C=10,.x568=0.5f,.x56C=1,.x570=2,.x574=3,.x578=4,
        .x57C=3,.x580=4,.x584=5,.x588=6,.ledge_cooldown=30};
    ftCommonData* previous=p_ftCommonData;p_ftCommonData=&common;
    for(unsigned mode=0;mode<3;++mode){
        reset();damage_motion_case=true;Fighter_GObj* actor=create(true);Fighter* fp=actor->user_data;
        fp->ground_or_air=GA_Air;fp->dmg.kb_applied=10;fp->dmg.x1848_kb_angle=45;
        fp->dmg.facing_dir_1=1;fp->dmg.x184c_damaged_hurtbox=0;fp->dmg.x1860_element=HitElement_Normal;
        NativeScriptFighterProgram* program=(NativeScriptFighterProgram*)nativeScriptFighter_Program(fp->kind);uint64_t identity=program->identity;
        Fighter expected={0};
        if(!mode){Fighter start=*fp;release_di_calls=0;reference_release_finish(actor,NULL,false);expected=*fp;
            CHECK(release_di_calls==1);*fp=start;trace_count=0;}
        else{retirement_holder=actor;retire_step=5;damage_retire_mode=mode==2;}
        release_di_calls=0;actual_release_finish(actor,NULL,false);
        CHECK(release_di_calls==(mode?0u:1u));
        CHECK(mode || !memcmp(fp,&expected,sizeof(expected)));
        program->identity=identity;damage_retire_mode=0;HSD_GObjFree(actor);++cases;
    }
    p_ftCommonData=previous;
    damage_motion_case=false;damage_retire_mode=0;
}

static void throw_cases(void)
{
    throw_pose_cases();throw_tick_cases();throw_boundary_cases();throw_release_lifetime_cases();reset();
}
