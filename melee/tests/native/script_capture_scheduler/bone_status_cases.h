static void bone_status_cases(void)
{
    reset();Fighter_GObj* actor=create(true);Fighter* fp=actor->user_data;
    static NativeSourceImportedAssets saved_assets;
    static FighterBone parts[4];static FighterPartsTable* tables[Ft_Kind_CustomFirst+1];
    saved_assets=capture_package.assets;FighterPartsTable** saved_tables=ftPartsTable;
    NativeSourceImportedAssets* assets=&capture_package.assets;
    assets->facts.joints=4;assets->facts.omission_count=0;assets->parts.parts_num=4;
    assets->parts.part_to_joint=assets->part_to_joint;
    assets->parts.joint_to_part=assets->joint_to_part;
    ftPartsTable=tables;tables[fp->kind]=&assets->parts;fp->parts=parts;
    NativeScriptRuntimeContext context={.source=actor,.source_class=NativeCallbackSource_Fighter,
        .generation=fp->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};
    NativeScriptDiagnostic diagnostic;
    for(unsigned count=1;count<=3;++count)for(unsigned status=0;status<3;++status)
      for(unsigned sticky=0;sticky<2;++sticky){
        fp->hurt_capsules_len=count;fp->x221A_b5=sticky;
        for(unsigned i=0;i<count;++i){fp->hurt_capsules[i].capsule.bone_idx=1;
            fp->hurt_capsules[i].capsule.state=HurtCapsule_Disabled;
            fp->hurt_capsules[i].capsule.skip_update_pos=true;}
        Fighter before=*fp;ftColl_8007B128(actor,1,(HurtCapsuleState)status);Fighter expected=*fp;*fp=before;
        NativeScriptValue args[2]={{.type=NativeScriptType_I32,.as.i64=1},{.type=NativeScriptType_I32,.as.i64=status}},out;
        CHECK(nativeScriptBindingCombat_Invoke(&context,NativeScriptBinding_HurtboxesSetBone,args,2,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_Void&&!memcmp(fp,&expected,sizeof(*fp)));
        CHECK(fp->hurt_capsules[0].capsule.skip_update_pos);
        if(count>1)CHECK(fp->hurt_capsules[1].capsule.state==HurtCapsule_Disabled);++cases;
      }
    const int32_t absent[]={INT32_MIN,-1,0,3,4,INT32_MAX};
    for(unsigned i=0;i<sizeof(absent)/sizeof(*absent);++i){
        Fighter before=*fp;NativeScriptValue args[2]={{.type=NativeScriptType_I32,.as.i64=absent[i]},
            {.type=NativeScriptType_I32,.as.i64=2}},out;
        CHECK(nativeScriptBindingCombat_Invoke(&context,NativeScriptBinding_HurtboxesSetBone,args,2,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_Void&&!memcmp(fp,&before,sizeof(*fp)));++cases;
    }
    fp->hurt_capsules_len=0;Fighter before=*fp;
    NativeScriptValue args[2]={{.type=NativeScriptType_I32,.as.i64=1},{.type=NativeScriptType_I32,.as.i64=2}},out;
    CHECK(nativeScriptBindingCombat_Invoke(&context,NativeScriptBinding_HurtboxesSetBone,args,2,&out,&diagnostic));
    CHECK(!memcmp(fp,&before,sizeof(*fp)));++cases;
    fp->hurt_capsules_len=3;fp->x221A_b5=false;
    CHECK(presentation_state(actor,"items",&args[0]).as.i64==1);
    CHECK(helper(actor,"MaskBone")&&fp->hurt_capsules[0].capsule.state==HurtCapsule_Intangible&&fp->x221A_b5);
    CHECK(helper(actor,"ClearBone")&&fp->hurt_capsules[0].capsule.state==HurtCapsule_Enabled&&fp->x221A_b5);++cases;
    for(unsigned bad=0;bad<12;++bad){
        NativeScriptRuntimeContext invalid=context;NativeScriptValue inputs[2]={args[0],args[1]};
        NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=99};out=sentinel;
        before=*fp;uint64_t identity=capture_package.program.identity;
        if(bad==0)inputs[0].type=NativeScriptType_U32;
        if(bad==1)inputs[0].as.i64=INT64_C(1)<<32;
        if(bad==2)inputs[1].type=NativeScriptType_Bool;
        if(bad==3)inputs[1].as.i64=-1;
        if(bad==4)inputs[1].as.i64=3;
        if(bad==5)++invalid.generation;
        if(bad==6)++invalid.scene_epoch;
        if(bad==7)invalid.source_class=NativeCallbackSource_Item;
        if(bad==8)++capture_package.program.identity;
        if(bad==9)fp->hurt_capsules_len=sizeof(fp->hurt_capsules)/sizeof(*fp->hurt_capsules)+1;
        if(bad==10)assets->facts.omission_count=1;
        if(bad==11)tables[fp->kind]=NULL;
        Fighter invalid_before=*fp;
        CHECK(!nativeScriptBindingCombat_Invoke(&invalid,NativeScriptBinding_HurtboxesSetBone,inputs,2,&out,&diagnostic));
        CHECK(!memcmp(&out,&sentinel,sizeof(out))&&!memcmp(fp,&invalid_before,sizeof(*fp)));
        *fp=before;capture_package.program.identity=identity;
        assets->facts.omission_count=0;tables[fp->kind]=&assets->parts;++cases;
    }
    HSD_GObjFree(actor);capture_package.assets=saved_assets;ftPartsTable=saved_tables;
}
