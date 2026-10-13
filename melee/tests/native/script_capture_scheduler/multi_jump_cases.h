static void multi_tuning_cases(void)
{
    reset();Fighter_GObj* actor=create(true);Fighter* fp=actor->user_data;
    struct Fighter_x2D0_t table={0};
    fp->can_multijump=true;fp->dat_attrs=fp->dat_attrs_backup=fp->x2D0=&table;
    fp->x1968_jumpsUsed=3;
    NativeScriptRuntimeContext context=victim_policy_context(actor);
    NativeScriptValue args[13]={{.type=NativeScriptType_I32,.as.i64=3}};
    for(unsigned i=1;i<10;++i)args[i]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=0.25f*i};
    args[10]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=5};
    args[11]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=341};
    args[12]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=-1};
    for(int count=1;count<=5;++count)for(int helmet=0;helmet<2;++helmet) {
        args[10].as.i64=count;args[12].as.i64=helmet?342:-1;
        NativeFighterMultiJumpStats stats={.turn_frames=3,.reverse_threshold=0.25f,.horizontal_impulse=0.5f,
            .acceleration_multiplier=0.75f,.speed_multiplier=1,.count=count,
            .first_motion=341,.helmet_first_motion=helmet?342:-1};
        for(unsigned i=0;i<5;++i)stats.vertical_impulses[i]=args[5+i].as.f32;
        Fighter before=*fp;struct Fighter_x2D0_t original=table;
        bool accepted=nativeFighterJump_ConfigureMulti(fp,&stats);
        Fighter expected=*fp;struct Fighter_x2D0_t expected_table=table;
        *fp=before;table=original;NativeScriptValue out;NativeScriptDiagnostic d;
        CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_JumpsConfigureMulti,NULL,0,args,13,&out,&d));
        CHECK(out.type==NativeScriptType_Bool&&out.as.u64==accepted);
        CHECK(!memcmp(fp,&expected,sizeof(expected))&&!memcmp(&table,&expected_table,sizeof(table))&&fp->x1968_jumpsUsed==3);
        ++cases;
    }
    CHECK(helper(actor,"ConfigureMultiJump")&&presentation_state(actor,"items",NULL).as.i64==1);
    CHECK(table.x28==5&&fp->co_attrs.max_jumps==6&&table.x14[0]==4&&table.x14[4]==2);
    for(int index=-1;index<=6;++index) {
        NativeScriptValue input={.type=NativeScriptType_I32,.as.i64=index};
        presentation_state(actor,"items",&input);
        CHECK(helper(actor,"MultiSpeed"));
        NativeScriptValue value=presentation_state(actor,"victim_frame",NULL);
        CHECK(value.type==NativeScriptType_F32&&value.as.f32==nativeFighterJump_MultiVerticalSpeed(fp,index));
        ++cases;
    }
    for(unsigned bad=0;bad<6;++bad) {
        NativeScriptValue copy[13];memcpy(copy,args,sizeof(copy));
        if(bad==0)copy[0].as.i64=0;
        if(bad==1)copy[10].as.i64=0;
        if(bad==2)copy[10].as.i64=6;
        if(bad==3)copy[11].as.i64=340;
        if(bad==4)copy[12].as.i64=350;
        if(bad==5)copy[5].as.f32=NAN;
        Fighter before=*fp;struct Fighter_x2D0_t original=table;
        NativeScriptValue out={.type=NativeScriptType_I32,.as.i64=991},unchanged=out;NativeScriptDiagnostic d;
        bool ok=nativeScriptBindings_Invoke(&context,NativeScriptBinding_JumpsConfigureMulti,NULL,0,copy,13,&out,&d);
        if(bad==5)CHECK(!ok&&!memcmp(&out,&unchanged,sizeof(out)));
        else CHECK(ok&&out.type==NativeScriptType_Bool&&!out.as.u64);
        CHECK(!memcmp(fp,&before,sizeof(before))&&!memcmp(&table,&original,sizeof(table)));++cases;
    }
    for(unsigned bad=0;bad<3;++bad) {
        Fighter before=*fp;
        if(bad==0)fp->can_multijump=false;
        if(bad==1)fp->dat_attrs_backup=NULL;
        if(bad==2)fp->x2D0=NULL;
        NativeScriptValue out={.type=NativeScriptType_I32,.as.i64=991},unchanged=out;NativeScriptDiagnostic d;
        bool ok=nativeScriptBindings_Invoke(&context,NativeScriptBinding_JumpsMultiVerticalSpeed,NULL,0,args,1,&out,&d);
        if(bad==1)CHECK(!ok&&!memcmp(&out,&unchanged,sizeof(out)));
        else CHECK(ok&&out.type==NativeScriptType_F32&&out.as.f32==0);
        *fp=before;++cases;
    }
    fp->dat_attrs=fp->dat_attrs_backup=NULL;fp->x2D0=NULL;HSD_GObjFree(actor);
}

static void bounded_drift_cases(void)
{
    const float sticks[]={-1,-0.3f,-0.29f,0,0.29f,0.3f,1};
    const float velocities[]={-3,-0.2f,0,0.2f,3};
    p_ftCommonData->x88=0.7f;p_ftCommonData->x8C=4;p_ftCommonData->x258=0.3f;
    for(unsigned s=0;s<7;++s)for(unsigned v=0;v<5;++v)for(unsigned down=0;down<2;++down) {
        reset();multi_jump_case=true;Fighter_GObj* actor=create(true);Fighter* fp=actor->user_data;
        fp->input.lstick[0].x=sticks[s];fp->input.lstick[0].y=down?-1:0;
        fp->self_vel=(Vec3){velocities[v],-0.5f,0};fp->x671_timer_lstick_tilt_y=0;
        fp->co_attrs.gravity=0.1f;fp->co_attrs.terminal_velocity=2;fp->co_attrs.fast_fall_velocity=3;
        fp->co_attrs.aerial_friction=0.15f;fp->co_attrs.air_max_horizontal_velocity=2;
        Fighter before=*fp;ft_80084E1C(actor,0.3f,0.5f,1.0f);Fighter expected=*fp;unsigned sounds=trace_count;
        *fp=before;trace_count=0;CHECK(helper(actor,"BoundedDrift"));
        CHECK(!memcmp(fp,&expected,sizeof(expected))&&trace_count==sounds&&sounds==down);
        CHECK(helper(actor,"DriftThreshold")&&presentation_state(actor,"victim_frame",NULL).as.f32==0.3f);
        HSD_GObjFree(actor);++cases;
    }
    multi_jump_case=false;
}

static void ledge_box_cases(void)
{
    for(unsigned facing=0;facing<2;++facing)for(int direction=-1;direction<=1;++direction)
        for(unsigned blocked=0;blocked<3;++blocked)for(unsigned contact=0;contact<2;++contact)
            for(unsigned interrupted=0;interrupted<2;++interrupted) {
        reset();Fighter_GObj* actor=create(true);Fighter* fp=actor->user_data;
        fp->facing_dir=facing?-1:1;fp->cur_pos=(Vec3){1,2,3};
        fp->x2064_ledgeCooldown=blocked==1;fp->x2224_b2=blocked==2;
        box_contact=contact;box_interrupted=interrupted;box_direction=99;
        box_plain_calls=box_ledge_calls=box_environment_calls=0;
        ftCollisionBox box={8,0,{-4,4},{3,5}};
        Fighter before=*fp;bool expected_contact=ft_8008239C(actor,direction,&box);Fighter expected=*fp;
        ftCollisionBox expected_box=box_observed;int expected_direction=box_direction;
        *fp=before;box_direction=99;box_plain_calls=box_ledge_calls=box_environment_calls=0;
        NativeScriptValue value={.type=NativeScriptType_I32,.as.i64=direction};presentation_state(actor,"items",&value);
        CHECK(helper(actor,"LedgeBox")&&!memcmp(fp,&expected,sizeof(expected)));
        CHECK(presentation_state(actor,"catches",NULL).as.i64==expected_contact);
        CHECK(!memcmp(&box_observed,&expected_box,sizeof(expected_box))&&box_direction==expected_direction);
        CHECK(box_plain_calls==(blocked!=0)&&box_ledge_calls==(blocked==0)&&box_environment_calls==1);
        HSD_GObjFree(actor);++cases;
    }
}

static void policy_reject_cases(void)
{
    reset();Fighter_GObj* actor=create(true);Fighter* fp=actor->user_data;
    NativeScriptRuntimeContext context=victim_policy_context(actor);
    NativeScriptValue args[7]={{.type=NativeScriptType_I32,.as.i64=1}};
    for(unsigned i=1;i<7;++i)args[i]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=i};
    for(unsigned bad=0;bad<6;++bad) {
        NativeScriptValue copy[7];memcpy(copy,args,sizeof(copy));
        NativeScriptRuntimeContext c=context;unsigned count=7;
        if(bad==0)count=6;
        if(bad==1)copy[0].type=NativeScriptType_F32;
        if(bad==2)copy[0].as.i64=INT64_MAX;
        if(bad==3)copy[6].as.f32=NAN;
        if(bad==4)++c.generation;
        if(bad==5)++c.scene_epoch;
        NativeScriptValue out={.type=NativeScriptType_I32,.as.i64=991},unchanged=out;NativeScriptDiagnostic d;
        Fighter before=*fp;
        CHECK(!nativeScriptBindingState_Invoke(&c,NativeScriptBinding_CollisionAirBoxLedge,copy,count,&out,&d));
        CHECK(!memcmp(&out,&unchanged,sizeof(out))&&!memcmp(fp,&before,sizeof(before)));++cases;
    }
    for(unsigned bad=0;bad<6;++bad) {
        NativeScriptValue copy[3]={{.type=NativeScriptType_F32,.as.f32=0.3f},
            {.type=NativeScriptType_F32,.as.f32=0.5f},{.type=NativeScriptType_F32,.as.f32=1}};
        NativeScriptRuntimeContext c=context;ftCommonData* common=p_ftCommonData;unsigned count=3;
        if(bad==0)count=2;
        if(bad==1)copy[1].type=NativeScriptType_I32;
        if(bad==2)copy[2].as.f32=INFINITY;
        if(bad==3)p_ftCommonData=NULL;
        if(bad==4)++c.generation;
        if(bad==5)++c.scene_epoch;
        NativeScriptValue out={.type=NativeScriptType_I32,.as.i64=991},unchanged=out;NativeScriptDiagnostic d;
        Fighter before=*fp;
        CHECK(!nativeScriptBindingAirPhysics_Invoke(&c,NativeScriptBinding_PhysicsAirDriftWithLimits,copy,count,&out,&d));
        CHECK(!memcmp(&out,&unchanged,sizeof(out))&&!memcmp(fp,&before,sizeof(before)));++cases;
        p_ftCommonData=common;
    }
    HSD_GObjFree(actor);
}

static void multi_jump_cases(void)
{
    ftCommonData common={0};ftCommonData* previous=p_ftCommonData;p_ftCommonData=&common;
    multi_tuning_cases();bounded_drift_cases();ledge_box_cases();policy_reject_cases();
    p_ftCommonData=previous;reset();
}
