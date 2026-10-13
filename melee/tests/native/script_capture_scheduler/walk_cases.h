static void walk_entry_cases(void)
{
    NativeScriptDiagnostic diagnostic;const float velocity[]={-2,-0.5f,0,0.5f,2};
    const unsigned base[]={45,341,344};
    for(unsigned b=0;b<3;++b)for(unsigned v=0;v<5;++v){
        reset();throw_case=true;Fighter_GObj* h=create(true);Fighter* fp=h->user_data;
        fp->gr_vel=velocity[v];fp->co_attrs.walk_max_vel=2;Fighter before=*fp;
        reference_ftWalkCommon_800DFCA4(h,(FtMotionId)base[b],0x40,3.5f,7,11,13,0.5f,1.25f,2,1);
        Fighter expected=*fp;unsigned n=trace_count;*fp=before;trace_count=0;
        NativeScriptRuntimeContext context=victim_policy_context(h);NativeScriptValue out;
        NativeScriptValue a[10]={{.type=NativeScriptType_I32,.as.i64=base[b]},
            {.type=NativeScriptType_U32,.as.u64=0x40}};
        const float parameters[]={3.5f,7,11,13,0.5f,1.25f,2,1};
        for(unsigned i=0;i<8;++i)a[i+2]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=parameters[i]};
        CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_WalkEnter,NULL,0,a,10,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_Void&&!memcmp(fp,&expected,sizeof(expected))&&trace_count==n);
        CHECK(n==2&&trace[0]==5&&trace[1]==7&&capture_motion_flags==0x40&&capture_motion_frame==3.5f&&capture_motion_rate==1);
        *fp=before;trace_count=0;presentation_state(h,"items",a);
        CHECK(helper(h,"WalkEnter")&&!memcmp(fp,&expected,sizeof(expected))&&trace_count==n);
        HSD_GObjFree(h);++cases;
    }
}

static void walk_rate_cases(void)
{
    NativeScriptDiagnostic diagnostic;const float velocity[]={-2,-0.5f,0,2};
    for(unsigned lane=0;lane<3;++lane)for(unsigned facing=0;facing<2;++facing)
        for(unsigned v=0;v<4;++v)for(unsigned friction=0;friction<2;++friction)for(unsigned reverse=0;reverse<2;++reverse){
            reset();throw_case=true;walk_friction=friction?1:0.5f;
            Fighter_GObj* h=create(true);Fighter* fp=h->user_data;
            fp->motion_id=(FtMotionId)(341+lane);fp->mv.co.walk.msid=341;
            fp->mv.co.walk.x0=-velocity[v];fp->gr_vel=velocity[v];fp->facing_dir=facing?1:-1;
            fp->mv.co.walk.slow_anim_rate=reverse?-0.5f:0.5f;
            fp->mv.co.walk.middle_anim_rate=reverse?-1.25f:1.25f;fp->mv.co.walk.fast_anim_rate=reverse?-2:2;
            Fighter before=*fp;reference_ftWalkCommon_800DFDDC(h);Fighter expected=*fp;unsigned n=trace_count;
            *fp=before;trace_count=0;NativeScriptRuntimeContext context=victim_policy_context(h);NativeScriptValue out;
            CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_WalkUpdateAnimation,NULL,0,NULL,0,&out,&diagnostic));
            CHECK(!memcmp(fp,&expected,sizeof(expected))&&trace_count==n&&n==1&&trace[0]==18);
            *fp=before;trace_count=0;CHECK(helper(h,"WalkRate")&&!memcmp(fp,&expected,sizeof(expected))&&trace_count==n);
            HSD_GObjFree(h);++cases;
        }
}

static void walk_physics_cases(void)
{
    NativeScriptDiagnostic diagnostic;
    const float stick[]={-1,-0.5f,0,0.5f,1},velocity[]={-3,-2,-1,-0.5f,0,0.5f,1,2,3},scale[]={0,0.5f,1};
    for(unsigned s=0;s<5;++s)for(unsigned v=0;v<9;++v)for(unsigned a=0;a<3;++a)
        for(unsigned slope=0;slope<2;++slope)for(unsigned friction=0;friction<3;++friction){
            reset();walk_friction=friction==0?0.5f:friction==1?1:1.5f;
            Fighter_GObj* h=create(true);Fighter* fp=h->user_data;
            fp->gr_vel=velocity[v];fp->input.lstick[0].x=stick[s];fp->mv.co.walk.accel_mul=scale[a];
            fp->co_attrs.walk_accel_base=0.1f;fp->co_attrs.walk_accel_mul=0.25f;fp->co_attrs.walk_max_vel=2;
            fp->co_attrs.ground_friction=0.125f;fp->co_attrs.ground_max_horizontal_velocity=4;
            fp->coll_data.floor.normal=slope?(Vec3){0.6f,0.8f,0}:(Vec3){0,1,0};
            Fighter before=*fp;reference_ftWalkCommon_800E0060(h);Fighter expected=*fp;*fp=before;
            NativeScriptRuntimeContext context=victim_policy_context(h);NativeScriptValue out;
            CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_PhysicsWalk,NULL,0,NULL,0,&out,&diagnostic));
            CHECK(out.type==NativeScriptType_Void&&!memcmp(fp,&expected,sizeof(expected)));
            *fp=before;CHECK(helper(h,"WalkPhysics")&&!memcmp(fp,&expected,sizeof(expected)));
            HSD_GObjFree(h);++cases;
        }
    for(unsigned animated=0;animated<2;++animated)for(unsigned facing=0;facing<2;++facing)
        for(unsigned logical=0;logical<2;++logical)for(unsigned slope=0;slope<2;++slope)for(unsigned friction=0;friction<3;++friction){
            reset();walk_friction=friction==0?0.5f:friction==1?1:1.5f;
            Fighter_GObj* h=create(true);Fighter* fp=h->user_data;
            fp->x594_b0=animated!=0;fp->facing_dir1=facing?1:-1;fp->facing_dir=logical?1:-1;
            fp->x6A4_transNOffset=(Vec3){4,5,6};fp->gr_vel=-0.25f;fp->xE4_ground_accel_1=0.5f;
            fp->co_attrs.ground_friction=0.125f;fp->coll_data.floor.normal=slope?(Vec3){0.6f,0.8f,0}:(Vec3){0,1,0};
            Fighter before=*fp;reference_ft_800850B4(h);Fighter expected=*fp;*fp=before;
            NativeScriptRuntimeContext context=victim_policy_context(h);NativeScriptValue out;
            CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_PhysicsGroundAnimationFacingMotion,NULL,0,NULL,0,&out,&diagnostic));
            CHECK(out.type==NativeScriptType_Void&&!memcmp(fp,&expected,sizeof(expected)));
            *fp=before;CHECK(helper(h,"GroundFacingMotion")&&!memcmp(fp,&expected,sizeof(expected)));
            HSD_GObjFree(h);++cases;
        }
}

static void walk_boundary_cases(void)
{
    NativeScriptDiagnostic diagnostic;
    const float velocity[]={-2,-1.4f,-0.6f,0,0.6f,1.4f,2};
    for(unsigned v=0;v<7;++v)for(unsigned scaled=0;scaled<2;++scaled){
        reset();Fighter_GObj* h=create(true);Fighter* fp=h->user_data;
        fp->gr_vel=velocity[v];fp->mv.co.walk.accel_mul=scaled?0.5f:1;fp->co_attrs.walk_max_vel=2;
        Fighter before=*fp;int expected=reference_ftWalkCommon_GetWalkType(h);NativeScriptValue out;
        NativeScriptRuntimeContext context=victim_policy_context(h);
        CHECK(nativeScriptBindingFighterObservations_Read(&context,NativeScriptBinding_WalkType,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_I32&&out.as.i64==expected&&!memcmp(fp,&before,sizeof(before)));
        CHECK(helper(h,"WalkType")&&presentation_state(h,"items",NULL).as.i64==expected);HSD_GObjFree(h);++cases;
    }
    for(unsigned boundary=0;boundary<2;++boundary)for(unsigned program_change=0;program_change<2;++program_change){
        reset();throw_case=true;Fighter_GObj* h=create(true);Fighter* fp=h->user_data;
        fp->co_attrs.walk_max_vel=2;retirement_holder=h;retire_step=boundary?7:5;
        victim_motion_retire_mode=program_change?2:0;
        NativeScriptFighterProgram* program=(NativeScriptFighterProgram*)nativeScriptFighter_Program(fp->kind);uint64_t identity=program->identity;
        NativeScriptValue value={.type=NativeScriptType_I32,.as.i64=341};presentation_state(h,"items",&value);
        CHECK(!helper(h,"WalkEnter")&&trace_count==boundary+1&&trace[trace_count-1]==retire_step);
        CHECK(fp->mv.co.walk.msid==0);program->identity=identity;HSD_GObjFree(h);++cases;
    }
    for(unsigned invalid=0;invalid<9;++invalid){
        reset();Fighter_GObj* h=create(true);Fighter* fp=h->user_data;fp->co_attrs.walk_max_vel=2;
        NativeScriptRuntimeContext context=victim_policy_context(h);NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
        NativeScriptValue a[10]={{.type=NativeScriptType_I32,.as.i64=341},{.type=NativeScriptType_U32}};
        for(unsigned i=2;i<10;++i)a[i]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=1};
        if(invalid==0)a[0].as.i64=345;if(invalid==1)a[0].type=NativeScriptType_F32;
        if(invalid==2)a[1].type=NativeScriptType_I32;if(invalid==3)a[7].as.f32=0;
        if(invalid==4)a[3].as.f32=NAN;if(invalid==5)a[9].as.f32=INFINITY;
        if(invalid==6)fp->x18=0;if(invalid==7)fp->x20_actionStateList=NULL;
        Fighter before=*fp;
        CHECK(!nativeScriptBindingCommonMovement_Invoke(&context,NativeScriptBinding_WalkEnter,a,invalid==8?9:10,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&!memcmp(fp,&before,sizeof(before))&&trace_count==0);
        HSD_GObjFree(h);++cases;
    }
    for(unsigned invalid=0;invalid<3;++invalid){
        reset();Fighter_GObj* h=create(true);Fighter* fp=h->user_data;fp->motion_id=341;fp->mv.co.walk.msid=341;fp->mv.co.walk.slow_anim_rate=1;
        if(invalid==0)fp->motion_id=344;if(invalid==1)fp->mv.co.walk.slow_anim_rate=0;if(invalid==2)fp->gr_vel=NAN;
        Fighter before=*fp;NativeScriptRuntimeContext context=victim_policy_context(h);NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
        CHECK(!nativeScriptBindingCommonMovement_Invoke(&context,NativeScriptBinding_WalkUpdateAnimation,NULL,0,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&!memcmp(fp,&before,sizeof(before))&&trace_count==0);HSD_GObjFree(h);++cases;
    }
}

static void walk_cases(void)
{
    ftCommonData common={0};common.walk_fast_stick_threshold=0.7f;common.walk_middle_animation_stick_threshold=0.3f;
    common.walk_accel_taper_gain=0.8f;common.x440=0.9f;ftCommonData* previous=p_ftCommonData;p_ftCommonData=&common;
    walk_entry_cases();walk_rate_cases();walk_physics_cases();walk_boundary_cases();
    p_ftCommonData=previous;walk_friction=1;reset();
}
