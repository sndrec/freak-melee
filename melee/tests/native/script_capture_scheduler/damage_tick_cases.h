// Shared damage operations use the owned caller, not a donor fighter handler.
static void damage_timer_cases(void)
{
    ftCommonData common={0};ftCommonData* previous=p_ftCommonData;p_ftCommonData=&common;
    const float timers[]={-2,0,0.5f,1,2,20};const int recovery[]={0,15,123};
    Item item={0};Item_GObj held={.user_data=&item};
    for(unsigned timer=0;timer<6;++timer)for(unsigned stunned=0;stunned<2;++stunned)
        for(unsigned holding=0;holding<3;++holding)for(unsigned policy=0;policy<3;++policy){
            reset();damage_tick_case=true;Fighter_GObj* actor=create(true);Fighter* fp=actor->user_data;
            common.x4CC=recovery[policy];common.x6B8=17;fp->mv.co.damage.x0=timers[timer];fp->x221C_b6=stunned;
            fp->x2098=31;fp->x2338.y=99;fp->item_gobj=holding?&held:NULL;
            item.kind=holding==1?It_Kind_Hammer:It_Kind_Capsule;
            Fighter before=*fp;damage_tick_reference_ftCo_8008F744(actor);Fighter expected=*fp;
            unsigned exits=damage_exit_calls,n=trace_count,expected_trace[32];memcpy(expected_trace,trace,sizeof(trace));
            *fp=before;damage_exit_calls=trace_count=0;
            NativeScriptRuntimeContext context=victim_policy_context(actor);NativeScriptDiagnostic diagnostic;NativeScriptValue out;
            CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_DamageUpdateTimer,NULL,0,NULL,0,&out,&diagnostic));
            CHECK(out.type==NativeScriptType_Void&&!memcmp(fp,&expected,sizeof(expected))&&damage_exit_calls==exits);
            CHECK(trace_count==n&&!memcmp(trace,expected_trace,n*sizeof(*trace)));
            *fp=before;damage_exit_calls=trace_count=0;
            CHECK(helper(actor,"DamageTimer")&&!memcmp(fp,&expected,sizeof(expected))&&damage_exit_calls==exits);
            CHECK(trace_count==n&&!memcmp(trace,expected_trace,n*sizeof(*trace))&&presentation_state(actor,"items",NULL).as.i64==99);
            HSD_GObjFree(actor);++cases;
        }
    // Program retirement inside knockback-exit must stop hammer recovery and
    // the next VM write even when the physical actor's generation is unchanged.
    for(unsigned mode=0;mode<2;++mode){
        reset();damage_tick_case=true;Fighter_GObj* actor=create(true);Fighter* fp=actor->user_data;
        retirement_holder=actor;retire_step=33;victim_motion_retire_mode=mode?2:0;
        fp->mv.co.damage.x0=1;fp->x221C_b6=true;fp->item_gobj=&held;item.kind=It_Kind_Hammer;fp->x2338.y=99;
        NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(actor,fp->x8_spawnNum,&view));
        NativeScriptFighterProgram* program=(NativeScriptFighterProgram*)nativeScriptFighter_Program(fp->kind);uint64_t identity=program->identity;
        CHECK(!helper(actor,"DamageTimer")&&finish_saved_value(&view,"items").as.i64!=99);
        CHECK(trace_count==1&&trace[0]==33&&fp->x2338.y==99&&damage_exit_calls==1);
        program->identity=identity;HSD_GObjFree(actor);++cases;
    }
    p_ftCommonData=previous;reset();
}

static void damage_physics_cases(void)
{
    ftCommonData common={0};ftCommonData* previous=p_ftCommonData;p_ftCommonData=&common;
    common.friction_when_above_walk_speed=1.25f;
    const float vx[]={-3,-0.5f,0,0.5f,3},vy[]={-10,-1,2,12},friction[]={0,0.125f,0.6f},slope[]={-0.6f,0,0.6f};
    for(unsigned air=0;air<2;++air)for(unsigned stunned=0;stunned<2;++stunned)
        for(unsigned x=0;x<5;++x)for(unsigned y=0;y<4;++y)for(unsigned f=0;f<3;++f)for(unsigned s=0;s<3;++s){
            reset();damage_tick_case=true;Fighter_GObj* actor=create(true);Fighter* fp=actor->user_data;
            fp->ground_or_air=air;fp->x221C_b6=stunned;fp->self_vel=(Vec3){vx[x],vy[y],2};fp->gr_vel=vx[x];
            fp->co_attrs.gravity=0.25f;fp->co_attrs.terminal_velocity=2;fp->co_attrs.aerial_friction=friction[f];
            fp->co_attrs.ground_friction=friction[f];fp->co_attrs.walk_max_vel=1.5f;
            fp->coll_data.floor.normal=(Vec3){slope[s],slope[s]?0.8f:1,0};walk_friction=1;
            Fighter before=*fp;damage_tick_reference_ftCo_Damage_Phys(actor);Fighter expected=*fp;
            unsigned n=trace_count,expected_trace[32];memcpy(expected_trace,trace,sizeof(trace));*fp=before;trace_count=0;
            NativeScriptRuntimeContext context=victim_policy_context(actor);NativeScriptDiagnostic diagnostic;NativeScriptValue out;
            CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_PhysicsDamageMotion,NULL,0,NULL,0,&out,&diagnostic));
            CHECK(out.type==NativeScriptType_Void&&!memcmp(fp,&expected,sizeof(expected))&&trace_count==n&&!memcmp(trace,expected_trace,n*sizeof(*trace)));
            *fp=before;trace_count=0;
            CHECK(helper(actor,"DamagePhysics")&&!memcmp(fp,&expected,sizeof(expected))&&trace_count==n&&!memcmp(trace,expected_trace,n*sizeof(*trace)));
            HSD_GObjFree(actor);++cases;
        }
    // A retired normal-air leaf cannot resume the next authored statement.
    for(unsigned mode=0;mode<2;++mode){
        reset();damage_tick_case=true;Fighter_GObj* actor=create(true);Fighter* fp=actor->user_data;
        fp->ground_or_air=GA_Air;retirement_holder=actor;retire_step=30;victim_motion_retire_mode=mode?2:0;
        NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(actor,fp->x8_spawnNum,&view));
        NativeScriptFighterProgram* program=(NativeScriptFighterProgram*)nativeScriptFighter_Program(fp->kind);uint64_t identity=program->identity;
        CHECK(!helper(actor,"DamagePhysics")&&finish_saved_value(&view,"items").as.i64!=99&&trace_count==1&&trace[0]==30);
        program->identity=identity;HSD_GObjFree(actor);++cases;
    }
    p_ftCommonData=previous;reset();
}

static void damage_tick_cases(void)
{
    damage_timer_cases();damage_physics_cases();
    for(unsigned operation=0;operation<2;++operation)for(unsigned bad=0;bad<8;++bad){
        reset();damage_tick_case=true;Fighter_GObj* actor=create(true);Fighter* fp=actor->user_data;
        NativeScriptRuntimeContext context=victim_policy_context(actor);NativeScriptDiagnostic diagnostic;
        uint32_t semantic=operation?NativeScriptBinding_PhysicsDamageMotion:NativeScriptBinding_DamageUpdateTimer;
        uint32_t count=0;NativeScriptValue value={.type=NativeScriptType_I32,.as.i64=1};
        if(bad==0)count=1;if(bad==1)++context.generation;if(bad==2)++context.scene_epoch;
        if(bad==3)context.source_class=NativeCallbackSource_Item;if(bad==4)fp->x1C_actionStateList=NULL;
        if(bad==5)fp->x20_actionStateList=NULL;if(bad==6)++fp->x18;
        ftCommonData* previous=p_ftCommonData;if(bad==7)p_ftCommonData=NULL;
        Fighter before=*fp;NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=77};
        CHECK(!nativeScriptBindingCombat_Invoke(&context,semantic,&value,count,&out,&diagnostic));
        CHECK(!memcmp(fp,&before,sizeof(before))&&!trace_count&&!damage_exit_calls&&out.type==NativeScriptType_U64&&out.as.u64==77);
        p_ftCommonData=previous;HSD_GObjFree(actor);++cases;
    }
    reset();
}
