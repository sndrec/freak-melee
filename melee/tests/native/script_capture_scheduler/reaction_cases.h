// Stock damage predicates and hit-group allocation through an owned actor/VM.
static void reaction_cases(void)
{
    reset();Fighter_GObj* actor=create(true);Fighter* fp=actor->user_data;
    NativeScriptRuntimeContext context={.source=actor,.source_class=NativeCallbackSource_Fighter,
        .generation=fp->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};
    NativeScriptDiagnostic diagnostic;
    ftCommonData common={.x140=5,.x154=0.5f,.x158=10,.x15C=20,.x160=30};
    ftCommonData* previous=p_ftCommonData;p_ftCommonData=&common;
    const float knockback[]={-1,0,0.01f,4.99f,5,5.01f,19.99f,20,20.01f,
        39.99f,40,40.01f,59.99f,60,60.01f,200};
    const float scales[]={0,0.5f,1.54f,-0.5f};
    for(unsigned scale=0;scale<4;++scale)for(unsigned flags=0;flags<8;++flags)
        for(unsigned sample=0;sample<sizeof(knockback)/sizeof(knockback[0]);++sample){
            common.x154=scales[scale];fp->allow_sdi=flags&1;fp->x221A_b3=(flags>>1)&1;
            fp->dmg.x18A8=(flags&4)?15:0;fp->dmg.kb_applied=knockback[sample];
            Fighter before=*fp;bool suppressed=ftCo_8008E984(fp);
            int level=ftCo_Damage_CalcKnockbackLevel(ftCo_ScaleBy154(fp->dmg.kb_applied));
            NativeScriptValue out;
            CHECK(nativeScriptBindingCombat_Read(&context,NativeScriptBinding_DamageReactionSuppressed,&out,&diagnostic));
            CHECK(out.type==NativeScriptType_Bool&&out.as.u64==suppressed);
            CHECK(nativeScriptBindingCombat_Read(&context,NativeScriptBinding_DamageKnockbackLevel,&out,&diagnostic));
            CHECK(out.type==NativeScriptType_I32&&out.as.i64==level&&!memcmp(fp,&before,sizeof(*fp)));
            CHECK(helper(actor,"DamageObservations"));
            CHECK(presentation_state(actor,"catches",NULL).as.i64==suppressed);
            CHECK(presentation_state(actor,"victims",NULL).as.i64==level&&!memcmp(fp,&before,sizeof(*fp)));++cases;
        }
    const uint32_t counters[]={0,1,2,UINT32_C(0x7FFFFFFF),UINT32_C(0x80000000),UINT32_MAX-1,UINT32_MAX};
    for(unsigned sample=0;sample<sizeof(counters)/sizeof(counters[0]);++sample){
        it_804D6D14=counters[sample];uint32_t expected=Item_8026AE60();uint32_t next=it_804D6D14;
        int64_t word=expected<=INT32_MAX?(int64_t)expected:-1-(int64_t)(UINT32_MAX-expected);
        it_804D6D14=counters[sample];Fighter before=*fp;NativeScriptValue out;
        CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_ArticlesNewCollisionGroup,NULL,0,NULL,0,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_I32&&out.as.i64==word&&it_804D6D14==next&&!memcmp(fp,&before,sizeof(*fp)));
        it_804D6D14=counters[sample];CHECK(helper(actor,"NewHitGroup"));
        CHECK(presentation_state(actor,"items",NULL).as.i64==word&&it_804D6D14==next&&!memcmp(fp,&before,sizeof(*fp)));++cases;
    }
    Fighter_GObj* second=create(true);it_804D6D14=777;
    CHECK(helper(actor,"NewHitGroup")&&presentation_state(actor,"items",NULL).as.i64==777);
    CHECK(helper(second,"NewHitGroup")&&presentation_state(second,"items",NULL).as.i64==778);
    CHECK(helper(actor,"NewHitGroup")&&presentation_state(actor,"items",NULL).as.i64==779&&it_804D6D14==780);
    HSD_GObjFree(second);++cases;
    for(unsigned bad=0;bad<6;++bad){
        NativeScriptRuntimeContext invalid=context;
        if(bad==0)invalid.generation++;if(bad==1)invalid.scene_epoch++;
        if(bad==2)invalid.source_class=NativeCallbackSource_Item;
        if(bad==3)invalid.source=NULL;if(bad==4)invalid.scene_epoch=0;
        if(bad==5)p_ftCommonData=NULL;
        Fighter before=*fp;NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
        CHECK(!nativeScriptBindingCombat_Read(&invalid,NativeScriptBinding_DamageReactionSuppressed,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_U64&&out.as.u64==UINT64_MAX);
        CHECK(!nativeScriptBindingCombat_Read(&invalid,NativeScriptBinding_DamageKnockbackLevel,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_U64&&out.as.u64==UINT64_MAX&&!memcmp(fp,&before,sizeof(*fp)));
        if(bad<5){
            it_804D6D14=71;
            CHECK(!nativeScriptBindingCombat_Invoke(&invalid,NativeScriptBinding_ArticlesNewCollisionGroup,NULL,0,&out,&diagnostic));
            CHECK(out.type==NativeScriptType_U64&&out.as.u64==UINT64_MAX&&it_804D6D14==71);
        }
        p_ftCommonData=&common;++cases;
    }
    NativeScriptValue extra={.type=NativeScriptType_I32,.as.i64=1},out={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
    it_804D6D14=71;
    CHECK(!nativeScriptBindingCombat_Invoke(&context,NativeScriptBinding_ArticlesNewCollisionGroup,&extra,1,&out,&diagnostic));
    CHECK(out.type==NativeScriptType_U64&&out.as.u64==UINT64_MAX&&it_804D6D14==71);++cases;
    HSD_GObjFree(actor);actor=create(false);fp=actor->user_data;
    context.source=actor;context.generation=fp->x8_spawnNum;
    CHECK(!nativeScriptBindingCombat_Read(&context,NativeScriptBinding_DamageReactionSuppressed,&out,&diagnostic));
    CHECK(!nativeScriptBindingCombat_Read(&context,NativeScriptBinding_DamageKnockbackLevel,&out,&diagnostic));
    CHECK(!nativeScriptBindingCombat_Invoke(&context,NativeScriptBinding_ArticlesNewCollisionGroup,NULL,0,&out,&diagnostic));
    CHECK(out.type==NativeScriptType_U64&&out.as.u64==UINT64_MAX&&it_804D6D14==71);++cases;
    p_ftCommonData=previous;HSD_GObjFree(actor);
}
