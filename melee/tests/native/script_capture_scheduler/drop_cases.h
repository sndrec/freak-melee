static void drop_cases(void)
{
    ftCommonData common={0};common.x3BC=1;ftCommonData* previous=p_ftCommonData;p_ftCommonData=&common;
    FighterPartsTable* tables[Ft_Kind_RuntimeCapacity]={0};u8 map[MAX_FT_PARTS];
    FighterPartsTable stock={.parts_num=MAX_FT_PARTS,.part_to_joint=map};
    NativeSourceImportedAssets* assets=&capture_package.assets;FighterPartsTable owned_before=assets->parts;
    FighterPartsTable** table_before=ftPartsTable;
    for(unsigned i=0;i<MAX_FT_PARTS;++i)map[i]=(u8)i;
    assets->parts=stock;tables[Ft_Kind_CustomFirst]=&assets->parts;tables[Ft_Kind_Mario]=&stock;ftPartsTable=tables;
    NativeScriptDiagnostic diagnostic;
    for(unsigned owned=0;owned<2;++owned)for(unsigned has=0;has<2;++has)
        for(unsigned swapped=0;swapped<2;++swapped)for(unsigned attached=0;attached<2;++attached)
        for(unsigned mode=0;mode<6;++mode)for(unsigned air=0;air<2;++air){
            if(!has&&(swapped||attached||mode||air))continue;
            if(!attached&&mode)continue;
            reset();Fighter_GObj* h=create(true);Fighter_GObj* v=has?create(owned!=0):NULL;
            if(v)reciprocal_pair(h,v);Fighter* holder=h->user_data;Fighter* victim=v?v->user_data:NULL;
            HSD_JObj joints[2]={{.flags=JOBJ_MTX_INDEP_SRT,.translate={2,3,4}},
                              {.flags=JOBJ_MTX_INDEP_SRT,.translate={5,6,7}}};
            FighterBone bones[2][MAX_FT_PARTS]={0};
            for(unsigned i=0;i<MAX_FT_PARTS;++i){bones[0][i].joint=&joints[0];bones[1][i].joint=&joints[1];}
            holder->parts=bones[0];h->hsd_obj=&joints[0];holder->cur_pos=(Vec3){11,12,13};
            holder->x221B_b5=true;holder->x221B_b7=swapped!=0;holder->x2219_b7=has!=0;
            holder->coll_data.floor.index=7;holder->ground_or_air=air?GA_Air:GA_Ground;
            holder->coll_data.ecb.top.y=8;holder->coll_data.ecb.bottom.y=-2;
            holder->x34_scale.y=1.25f;holder->x1A70=(Vec3){0,1,2};holder->x2174=(Vec3){7,8,9};holder->facing_dir=-1;
            if(v){victim->parts=bones[1];v->hsd_obj=&joints[1];victim->cur_pos=(Vec3){21,22,23};
                victim->coll_data.floor.index=7;victim->ground_or_air=air?GA_Air:GA_Ground;
                victim->coll_data.ecb.top.y=8;victim->coll_data.ecb.bottom.y=-2;
                victim->x34_scale.y=1.25f;victim->x1A70=(Vec3){0,1,2};victim->x2174=(Vec3){7,8,9};victim->facing_dir=1;
                victim->x2219_b7=true;victim->x2226_b2=attached&&!swapped;holder->x2226_b2=attached&&swapped;}
            drop_attached=swapped?holder:victim;drop_floor_mode=mode;
            Fighter hs=*holder,vs={0};if(v)vs=*victim;HSD_JObj js[2];memcpy(js,joints,sizeof(js));
            if(v){reference_unlink_capture(h,v);ftCo_Fall_Enter(v);}
            Fighter he=*holder,ve={0};if(v)ve=*victim;HSD_JObj je[2];memcpy(je,joints,sizeof(je));
            unsigned expected[32],n=trace_count;memcpy(expected,trace,sizeof(trace));
            *holder=hs;if(v)*victim=vs;memcpy(joints,js,sizeof(js));trace_count=0;
            NativeScriptRuntimeContext context=victim_policy_context(h);NativeScriptValue out;
            CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_CaptureDropVictim,NULL,0,NULL,0,&out,&diagnostic));
            CHECK(out.type==NativeScriptType_Void&&!memcmp(holder,&he,sizeof(he))&&(!v||!memcmp(victim,&ve,sizeof(ve))));
            CHECK(!memcmp(joints,je,sizeof(je))&&trace_count==n&&!memcmp(trace,expected,n*sizeof(*trace)));
            *holder=hs;if(v)*victim=vs;memcpy(joints,js,sizeof(js));trace_count=0;
            CHECK(helper(h,"DropVictim")&&presentation_state(h,"victims",NULL).as.i64==99);
            CHECK(!memcmp(holder,&he,sizeof(he))&&(!v||!memcmp(victim,&ve,sizeof(ve)))&&!memcmp(joints,je,sizeof(je))&&trace_count==n);
            if(v)HSD_GObjFree(v);HSD_GObjFree(h);++cases;
        }
    for(unsigned target=0;target<3;++target){
        reset();Fighter_GObj* h=create(true);Fighter_GObj* v=create(true);reciprocal_pair(h,v);
        ((Fighter*)h->user_data)->x2219_b7=((Fighter*)v->user_data)->x2219_b7=true;
        retirement_holder=h;drop_attached=v->user_data;retire_step=24;
        retire_target=target==1;victim_motion_retire_mode=target==2?2:0;
        NativeScriptFighterProgram* program=(NativeScriptFighterProgram*)nativeScriptFighter_Program(((Fighter*)h->user_data)->kind);uint64_t identity=program->identity;
        NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(h,((Fighter*)h->user_data)->x8_spawnNum,&view));
        CHECK(!helper(h,"DropVictim")&&finish_saved_value(&view,"victims").as.i64!=99);
        CHECK(trace[trace_count-1]==retire_step&&trace_count==1);
        program->identity=identity;HSD_GObjFree(v);HSD_GObjFree(h);++cases;
    }
    // Animation-facing is the stored animation orientation, independent of a
    // temporary reverse in logical facing. Preserve the native float bits.
    const uint32_t samples[]={0x3f800000,0xbf800000,0,0x80000000,0x3f000000,0x7f800000,0x7fc00001};
    for(unsigned i=0;i<sizeof(samples)/sizeof(*samples);++i){
        reset();Fighter_GObj* h=create(true);Fighter* fp=h->user_data;fp->facing_dir=-1;
        memcpy(&fp->facing_dir1,&samples[i],sizeof(float));Fighter before=*fp;
        NativeScriptRuntimeContext context=victim_policy_context(h);NativeScriptValue out;
        CHECK(nativeScriptBindingFighterObservations_Read(&context,NativeScriptBinding_FacingAnimationDirection,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_F32&&!memcmp(&out.as.f32,&samples[i],sizeof(float))&&!memcmp(fp,&before,sizeof(before)));
        if(i<5){CHECK(helper(h,"AnimationFacing"));out=presentation_state(h,"victim_frame",NULL);CHECK(!memcmp(&out.as.f32,&samples[i],sizeof(float)));}
        HSD_GObjFree(h);++cases;
    }
    for(unsigned invalid=0;invalid<4;++invalid){
        reset();Fighter_GObj* h=create(true);Fighter_GObj* v=create(false);reciprocal_pair(h,v);
        NativeScriptRuntimeContext context=victim_policy_context(h);NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
        if(invalid==0)((Fighter*)v->user_data)->victim_gobj=NULL;
        if(invalid==1)++context.generation;
        if(invalid==2)context.source_class=NativeCallbackSource_Item;
        Fighter hb=*(Fighter*)h->user_data,vb=*(Fighter*)v->user_data;
        CHECK(!nativeScriptBindingCapture_Invoke(&context,NativeScriptBinding_CaptureDropVictim,NULL,invalid==3?1:0,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&!memcmp(h->user_data,&hb,sizeof(hb))&&!memcmp(v->user_data,&vb,sizeof(vb))&&trace_count==0);
        HSD_GObjFree(v);HSD_GObjFree(h);++cases;
    }
    assets->parts=owned_before;ftPartsTable=table_before;p_ftCommonData=previous;reset();
}
