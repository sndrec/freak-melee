static void setup_shield_break(Fighter_GObj* object)
{
    Fighter* fp=object->user_data;
    shield_break_model.x11=0;shield_break_part.joint=&shield_break_joint;
    fp->ft_data->x8=&shield_break_model;fp->parts=&shield_break_part;
    fp->co_attrs.shield_break_initial_velocity=4.5f;
    fp->self_vel=(Vec3){2,3,4};fp->x2222_b3=false;
}

static void shield_break_cases(void)
{
    struct ftData_x8* saved_model=capture_package.assets.character.data.x8;
    for(unsigned ko=0;ko<2;++ko)for(unsigned hook=0;hook<2;++hook) {
        reset();shield_break_case=true;
        Fighter_GObj* stock=create(false);Fighter_GObj* source=create(true);
        Fighter* a=stock->user_data;Fighter* b=source->user_data;
        a->kind=ko?Ft_Kind_Purin:Ft_Kind_Mario;
        setup_shield_break(stock);setup_shield_break(source);
        NativeScriptValue choice={.type=NativeScriptType_I32,.as.i64=ko};
        presentation_state(source,"items",&choice);
        uint32_t saved=capture_package.program.hooks[NativeScriptFighter_ShieldBreak];
        if(!hook) {
            // Program changes are transition-only. Reattach against a newly
            // published absent-hook program, with the physical source intact.
            HSD_GObjFree(source);capture_package.program.hooks[NativeScriptFighter_ShieldBreak]=UINT32_MAX;
            CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));
            nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&capture_package.program);
            source=create(true);b=source->user_data;setup_shield_break(source);
            presentation_state(source,"items",&choice);
        }
        trace_count=0;reference_shield_break(stock);
        unsigned expected[32],expected_count=trace_count;memcpy(expected,trace,sizeof(trace));
        trace_count=0;ftCo_80098B20(source);
        CHECK(nativeScriptFighter_Status(source)==NativeScriptVM_Ok);
        CHECK(trace_count==expected_count&&!memcmp(trace,expected,expected_count*sizeof(*trace)));
        CHECK(b->motion_id==a->motion_id&&b->ground_or_air==a->ground_or_air&&
            !memcmp(&b->self_vel,&a->self_vel,sizeof(b->self_vel)));
        CHECK(b->x2222_b3==(hook?ko:false));
        CHECK(presentation_state(source,"victims",NULL).as.i64==hook);
        HSD_GObjFree(source);HSD_GObjFree(stock);
        capture_package.program.hooks[NativeScriptFighter_ShieldBreak]=saved;
        CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));
        nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&capture_package.program);++cases;
    }
    const unsigned boundaries[]={6,5,7,40,41,28};
    for(unsigned i=0;i<6;++i)for(unsigned program=0;program<2;++program) {
        reset();shield_break_case=true;Fighter_GObj* source=create(true);Fighter* fp=source->user_data;
        setup_shield_break(source);NativeScriptValue choice={.type=NativeScriptType_I32,.as.i64=1};
        presentation_state(source,"items",&choice);
        retirement_holder=source;retire_step=boundaries[i];damage_motion_case=true;damage_retire_mode=program;
        NativeFighterLifetime lifetime;CHECK(nativeFighterLifetime_Capture(fp,&lifetime));
        ftCo_80098B20(source);
        CHECK(trace_count==i+1&&trace[trace_count-1]==boundaries[i]);
        CHECK(!nativeFighterLifetime_Current(&lifetime));
        if(!program)CHECK(nativeScriptFighter_Status(source)==NativeScriptVM_SourceRetired);
        if(program)--((NativeScriptFighterProgram*)nativeScriptFighter_Program(fp->kind))->identity;
        else --fp->x8_spawnNum;
        damage_motion_case=false;damage_retire_mode=0;retire_step=0;
        HSD_GObjFree(source);++cases;
    }
    reset();shield_break_case=true;Fighter_GObj* source=create(true);
    setup_shield_break(source);NativeScriptValue choice={.type=NativeScriptType_I32,.as.i64=2};
    presentation_state(source,"items",&choice);
    presentation_retire_mode=1;presentation_retire_at=1;
    ftCo_80098B20(source);
    CHECK(trace_count==3&&nativeScriptFighter_Status(source)==NativeScriptVM_SourceRetired);
    CHECK(((Fighter*)source->user_data)->x2222_b3);
    presentation_restore();HSD_GObjFree(source);++cases;
    shield_break_case=false;capture_package.assets.character.data.x8=saved_model;
}
