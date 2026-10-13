static void cue_seed(Fighter* fp,unsigned mask)
{
    fp->throw_flags=UINT32_C(0xA5A55AA5);
    #define SEED_CUE(bit) fp->throw_flags_b##bit=(mask>>bit)&1
    SEED_CUE(0);SEED_CUE(1);SEED_CUE(2);SEED_CUE(3);
    SEED_CUE(4);SEED_CUE(5);SEED_CUE(6);SEED_CUE(7);
    #undef SEED_CUE
}
static bool reference_consume_cue(Fighter* fp,unsigned bit)
{
    bool value=false;
    #define STOCK_CUE(bit) case bit:value=fp->throw_flags_b##bit;fp->throw_flags_b##bit=0;break
    switch(bit){STOCK_CUE(0);STOCK_CUE(1);STOCK_CUE(2);STOCK_CUE(3);
        STOCK_CUE(4);STOCK_CUE(5);STOCK_CUE(6);STOCK_CUE(7);}
    #undef STOCK_CUE
    return value;
}
static void cue_cases(void)
{
    reset();Fighter_GObj* actor=create(true);Fighter* fp=actor->user_data;
    NativeScriptRuntimeContext context={.source=actor,.source_class=NativeCallbackSource_Fighter,
        .generation=fp->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};
    NativeScriptDiagnostic diagnostic;
    const int locks[]={0,1,-1,60};
    for(unsigned flag=0;flag<256;++flag)for(unsigned lock=0;lock<4;++lock){
        fp->ecb_lock=locks[lock];fp->coll_data.x130_flags=UINT32_C(0xA5A50000)|flag;
        Fighter before=*fp;ftCommon_UnlockECB(fp);Fighter expected=*fp;*fp=before;
        NativeScriptValue out;
        CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_CollisionUnlockECB,NULL,0,NULL,0,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_Void&&!memcmp(fp,&expected,sizeof(*fp)));++cases;
    }
    fp->ecb_lock=60;fp->coll_data.x130_flags=UINT32_MAX;
    CHECK(helper(actor,"UnlockECB")&&fp->ecb_lock==0&&fp->coll_data.x130_flags==(UINT32_MAX&~CollData_X130_Locked));++cases;
    for(unsigned mask=0;mask<256;++mask)for(unsigned bit=0;bit<8;++bit){
        cue_seed(fp,mask);Fighter before=*fp;
        bool raised=reference_consume_cue(fp,bit);Fighter expected=*fp;*fp=before;
        NativeScriptValue argument={.type=NativeScriptType_I32,.as.i64=bit},out;
        CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_CueConsume,NULL,0,&argument,1,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_Bool&&out.as.u64==raised&&!memcmp(fp,&expected,sizeof(*fp)));
        CHECK(nativeScriptBindingSetup_Invoke(&context,NativeScriptBinding_CueConsume,&argument,1,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_Bool&&!out.as.u64&&!memcmp(fp,&expected,sizeof(*fp)));++cases;
    }
    for(unsigned bit=0;bit<8;++bit){
        cue_seed(fp,255);Fighter before=*fp;reference_consume_cue(fp,bit);uint32_t expected=fp->throw_flags;*fp=before;
        NativeScriptValue argument={.type=NativeScriptType_I32,.as.i64=bit};
        CHECK(presentation_state(actor,"items",&argument).type==NativeScriptType_I32);
        CHECK(helper(actor,"ConsumeCue")&&fp->throw_flags==expected);
        CHECK(presentation_state(actor,"catches",NULL).as.i64==1);
        CHECK(helper(actor,"ConsumeCue")&&fp->throw_flags==expected);
        CHECK(presentation_state(actor,"catches",NULL).as.i64==0);++cases;
    }
    for(unsigned bad=0;bad<10;++bad){
        cue_seed(fp,255);Fighter before=*fp;NativeScriptRuntimeContext invalid=context;
        NativeScriptValue argument={.type=NativeScriptType_I32,.as.i64=4};
        if(bad==0)argument.as.i64=-1;if(bad==1)argument.as.i64=8;
        if(bad==2)argument.as.i64=INT64_MAX;if(bad==3)argument.type=NativeScriptType_F32;
        if(bad==4)argument.type=NativeScriptType_U32;
        if(bad==5)invalid.generation++;if(bad==6)invalid.scene_epoch++;
        if(bad==7)invalid.source_class=NativeCallbackSource_Item;
        if(bad==8)invalid.source=NULL;if(bad==9)invalid.scene_epoch=0;
        NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
        CHECK(!nativeScriptBindingSetup_Invoke(&invalid,NativeScriptBinding_CueConsume,&argument,1,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_U64&&out.as.u64==UINT64_MAX&&!memcmp(fp,&before,sizeof(*fp)));++cases;
        if(bad>=5){
            CHECK(!nativeScriptBindingSetup_Invoke(&invalid,NativeScriptBinding_CollisionUnlockECB,NULL,0,&out,&diagnostic));
            CHECK(out.type==NativeScriptType_U64&&!memcmp(fp,&before,sizeof(*fp)));++cases;
        }
    }
    HSD_GObjFree(actor);
}
