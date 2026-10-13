/* Connected native shield/camera and scalar observations; actual kernels are
 * extracted unchanged from the committed reference, never successful leaves. */
static bool geometry_host(HSD_GObj* caller,bool shield,NativeScriptValue* a,NativeScriptValue* out)
{
    NativeScriptRuntimeContext c=context(caller);
    return nativeScriptBindingTargets_Invoke(&c,shield?NativeScriptBinding_TargetsShield:
        NativeScriptBinding_TargetsReadCameraTargetPosition,a,shield?5:4,out,&diagnostic);
}
static void geometry_arguments(NativeScriptValue* a,uint32_t id)
{
    a[0]=integer((int32_t)id);
    for(unsigned i=1;i<5;++i)a[i]=floating(90+(float)i);
}
static void geometry_run(HSD_GObj* o,const char* name)
{
    NativeScriptRuntimeContext c=context(o);NativeScriptValue out;
    if(c.source_class==NativeCallbackSource_Item)
        CHECK(nativeScriptItem_RunSignal(o,hash(name),0,&out)==NativeScriptVM_Ok);
    else {
        const NativeScriptProgramIndex* p=&nativeScriptFighter_Program(((Fighter*)o->user_data)->kind)->bank->declarations;
        uint32_t id=0;for(;id<p->function_count;++id) {
            const NativeScriptCompileBank* b=nativeScriptFighter_Program(((Fighter*)o->user_data)->kind)->bank;
            if(nativeScriptSource_SpanEquals(&b->tree,p->functions[id].name,name))break;
        }
        CHECK(id<p->function_count);
        CHECK(nativeScriptFighter_Helper(o,c.generation,id,NULL,0,&out,&diagnostic)==NativeScriptVM_Ok);
    }
    ++vm_calls;
}
static void geometry_cases(HSD_GObj* fighter_caller,HSD_GObj* article_caller,HSD_GObj* owned,HSD_GObj* stock)
{
    HSD_GObj* callers[]={fighter_caller,article_caller};HSD_GObj* targets[]={owned,stock};
    common_data.x260_startShieldHealth=60;common_data.x264=.3f;common_data.x2D4=.6f;common_data.x2D8=1;
    for(unsigned t=0;t<2;++t)for(unsigned c=0;c<2;++c)for(unsigned root=0;root<2;++root)
    for(unsigned transform=0;transform<3;++transform)for(unsigned offset=0;offset<2;++offset) {
        Fighter* f=targets[t]->user_data;struct ftCo_DatAttrs* attributes=f->ft_data->x0;
        attributes->camera_zoom_target_bone=root?0:2;f->ft_data->x8->x11=root?0:2;
        f->co_attrs.x170=offset?(Vec3){2.5f,-1.75f,3.25f}:(Vec3){0};
        f->co_attrs.initial_shield_size=2.375f;f->shield_health=transform==2?-17.25f:37;
        f->lightshield_amount=.375f;f->x34_scale.y=-1.25f;f->co_attrs.model_scaling=1.75f;
        for(unsigned j=0;j<4;++j) {
            HSD_JObj* joint=f->parts[j].joint;joint->flags|=JOBJ_MTX_DIRTY;
            joint->rotate=transform?(Quaternion){.x=.2f,.y=-.35f,.z=.6f}:(Quaternion){0};
            joint->scale=transform==2?(Vec3){1.5f,.75f,-2}:(Vec3){1,1,1};
        }
        for(unsigned shield=0;shield<2;++shield) {
            Vec3 native;float radius=0;
            if(shield)radius=nativeFighterTarget_Shield(f->x8_spawnNum,&native);
            else CHECK(nativeFighterTarget_CameraPosition(f->x8_spawnNum,&native));
            for(unsigned j=0;j<4;++j)f->parts[j].joint->flags|=JOBJ_MTX_DIRTY;
            NativeScriptValue a[5],out;geometry_arguments(a,f->x8_spawnNum);
            CHECK(geometry_host(callers[c],shield,a,&out)&&out.type==NativeScriptType_Bool&&out.as.u64);
            CHECK(bits(a[1].as.f32)==bits(native.x)&&bits(a[2].as.f32)==bits(native.y)&&bits(a[3].as.f32)==bits(native.z));
            if(shield)CHECK(bits(a[4].as.f32)==bits(radius));else CHECK(a[4].as.f32==94);
            ++comparisons;++cases;
            NativeScriptValue v=integer((int32_t)f->x8_spawnNum);field(callers[c],"handle",&v);
            const char* names[]={"x","y","z","radius"};
            for(unsigned i=0;i<4;++i){v=floating(91+(float)i);field(callers[c],names[i],&v);}
            geometry_run(callers[c],shield?"QueryShield":"QueryCamera");
            CHECK(field(callers[c],"found",NULL).as.u64);
            CHECK(bits(field(callers[c],"x",NULL).as.f32)==bits(native.x));
            CHECK(bits(field(callers[c],"y",NULL).as.f32)==bits(native.y));
            CHECK(bits(field(callers[c],"z",NULL).as.f32)==bits(native.z));
            CHECK(shield?bits(field(callers[c],"radius",NULL).as.f32)==bits(radius):field(callers[c],"radius",NULL).as.f32==94);
        }
        for(unsigned m=0;m<5;++m) {
            const int motions[]={ftCo_MS_GuardOn,ftCo_MS_Guard,ftCo_MS_GuardSetOff,ftCo_MS_GuardOff,ftCo_MS_Wait};
            f->motion_id=motions[m];NativeScriptRuntimeContext ctx=context(callers[c]);NativeScriptValue a=integer((int32_t)f->x8_spawnNum),out;
            CHECK(nativeScriptBindingTargets_Invoke(&ctx,NativeScriptBinding_TargetsGuardActive,&a,1,&out,&diagnostic));
            CHECK(out.type==NativeScriptType_Bool&&out.as.u64==(uint64_t)nativeFighterTarget_GuardActive(f->x8_spawnNum));
            CHECK(nativeScriptBindingTargets_Invoke(&ctx,NativeScriptBinding_TargetsModelScale,&a,1,&out,&diagnostic));
            CHECK(out.type==NativeScriptType_F32&&bits(out.as.f32)==bits(nativeFighterTarget_ModelScale(f->x8_spawnNum)));
            geometry_run(callers[c],"QueryScalar");CHECK(field(callers[c],"guard",NULL).as.u64==(uint64_t)(m<3));
            CHECK(bits(field(callers[c],"scale",NULL).as.f32)==bits(out.as.f32));comparisons+=2;++cases;
        }
    }
    // Missing target and missing raw joint preserve all native destinations and
    // all VM state outputs, independently of central scratch initialization.
    Fighter* f=stock->user_data;f->ft_data->x0->camera_zoom_target_bone=2;f->ft_data->x8->x11=2;
    for(unsigned c=0;c<2;++c)for(unsigned shield=0;shield<2;++shield)for(unsigned mode=0;mode<5;++mode) {
        uint32_t id=mode==0?0:mode==1?UINT32_MAX:22;HSD_JObj* joint=f->parts[2].joint;
        int index=f->ft_data->x0->camera_zoom_target_bone;u8 shield_index=f->ft_data->x8->x11;
        if(mode==2)f->parts[2].joint=NULL;
        if(mode==3){f->ft_data->x0->camera_zoom_target_bone=-1;f->ft_data->x8->x11=255;}
        if(mode==4){f->ft_data->x0->camera_zoom_target_bone=4;f->ft_data->x8->x11=4;}
        NativeScriptValue a[5],old[5],out;geometry_arguments(a,id);memcpy(old,a,sizeof(a));
        CHECK(geometry_host(callers[c],shield,a,&out)&&!out.as.u64&&!memcmp(a,old,sizeof(a)));
        NativeScriptValue v=integer((int32_t)id);field(callers[c],"handle",&v);const char* names[]={"x","y","z","radius"};
        for(unsigned i=0;i<4;++i){v=floating(91+(float)i);field(callers[c],names[i],&v);}
        geometry_run(callers[c],shield?"QueryShield":"QueryCamera");CHECK(!field(callers[c],"found",NULL).as.u64);
        for(unsigned i=0;i<4;++i)CHECK(field(callers[c],names[i],NULL).as.f32==91+(float)i);
        f->parts[2].joint=joint;f->ft_data->x0->camera_zoom_target_bone=index;f->ft_data->x8->x11=shield_index;++cases;
    }
    for(unsigned c=0;c<2;++c) {
        NativeScriptRuntimeContext ctx=context(callers[c]);NativeScriptValue arg=integer(0),out;
        CHECK(nativeScriptBindingTargets_Invoke(&ctx,NativeScriptBinding_TargetsModelScale,&arg,1,&out,&diagnostic)&&out.as.f32==0);
        CHECK(nativeScriptBindingTargets_Invoke(&ctx,NativeScriptBinding_TargetsGuardActive,&arg,1,&out,&diagnostic)&&!out.as.u64);
    }
    const uint32_t scale_words[]={0,0x80000000,0x7f800000,0xff800000,0x7fc12345,1};
    for(unsigned c=0;c<2;++c)for(unsigned s=0;s<sizeof(scale_words)/sizeof(*scale_words);++s) {
        memcpy(&f->x34_scale.y,&scale_words[s],4);f->co_attrs.model_scaling=1.75f;
        NativeScriptRuntimeContext ctx=context(callers[c]);NativeScriptValue arg=integer(22),out;
        CHECK(nativeScriptBindingTargets_Invoke(&ctx,NativeScriptBinding_TargetsModelScale,&arg,1,&out,&diagnostic));
        CHECK(bits(out.as.f32)==bits(nativeFighterTarget_ModelScale(22)));++comparisons;++cases;
        field(callers[c],"handle",&arg);geometry_run(callers[c],"QueryScalar");
        CHECK(bits(field(callers[c],"scale",NULL).as.f32)==bits(out.as.f32));
    }
    // Nonfinite arithmetic is native data, not a binding admission failure.
    common_data.x260_startShieldHealth=0;f->shield_health=1;NativeScriptValue a[5],out;
    geometry_arguments(a,22);CHECK(geometry_host(fighter_caller,true,a,&out)&&out.as.u64&&isinf(a[4].as.f32));
    common_data.x260_startShieldHealth=60;ftCommonData* saved=p_ftCommonData;p_ftCommonData=NULL;
    geometry_arguments(a,22);NativeScriptValue old[5];memcpy(old,a,sizeof(a));out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
    CHECK(!geometry_host(fighter_caller,true,a,&out)&&out.as.u64==99&&!memcmp(old,a,sizeof(a)));p_ftCommonData=saved;
    FighterKind kind=f->kind;f->kind=Ft_Kind_Yoshi;tables[Ft_Kind_Yoshi]=&stock_parts;p_ftCommonData=NULL;
    geometry_arguments(a,22);CHECK(geometry_host(fighter_caller,true,a,&out)&&out.as.u64&&bits(a[4].as.f32)==bits(f->co_attrs.initial_shield_size));
    f->kind=kind;p_ftCommonData=saved;
    for(unsigned shield=0;shield<2;++shield)for(unsigned mode=0;mode<5;++mode) {
        geometry_arguments(a,22);if(mode==0)a[0]=floating(22);if(mode==1)a[0]=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000016)};
        if(mode==2)a[1]=integer(1);memcpy(old,a,sizeof(a));out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
        NativeScriptRuntimeContext ctx=context(fighter_caller);
        CHECK(!nativeScriptBindingTargets_Invoke(&ctx,shield?NativeScriptBinding_TargetsShield:NativeScriptBinding_TargetsReadCameraTargetPosition,
            a,mode==3?0:mode==4?6:shield?5:4,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&!memcmp(old,a,sizeof(a)));++negative_count;
    }
    // Retained graph/caller mutation barriers, with explicit fixed-storage
    // observer leaves rather than arbitrary destructive callback containment.
    for(unsigned shield=0;shield<2;++shield)for(unsigned m=1;m<=11;++m) {
        query_caller=m==7?item():fighter(0,100+fighter_count);query_target=fighter(1,200+fighter_count);replacement=NULL;mutation=m;
        Fighter* target=query_target->user_data;target->co_attrs.x170=(Vec3){1,2,3};target->co_attrs.initial_shield_size=2;
        target->shield_health=60;target->ft_data->x0->camera_zoom_target_bone=2;target->ft_data->x8->x11=2;
        bone_install_dummy(&observer);target->parts[2].joint->robj=&observer;
        geometry_arguments(a,target->x8_spawnNum);memcpy(old,a,sizeof(a));out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
        CHECK(!geometry_host(query_caller,shield,a,&out));CHECK(!mutation&&out.as.u64==99&&!memcmp(old,a,sizeof(a)));++cases;
        if(m==5)CHECK(replacement==query_target);if(m==6)CHECK(replacement==query_caller);
        if(m!=5)HSD_GObjFree(query_target);if(m!=6)HSD_GObjFree(query_caller);if(replacement)HSD_GObjFree(replacement);
    }
}
