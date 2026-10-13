static uint32_t parameter_call(HSD_GObj* source,const char* name,unsigned operation,unsigned count,NativeScriptValue* args)
{
    uint32_t semantic=call_arguments(source,name,operation,args);
    return (semantic&~NATIVE_SCRIPT_TARGET_EXPORT_ARGUMENT_MASK)|(count<<NATIVE_SCRIPT_TARGET_EXPORT_ARGUMENT_SHIFT);
}
static void argument_cases(HSD_GObj* fighter_source,HSD_GObj* article_source,HSD_GObj* target)
{
    HSD_GObj* sources[]={fighter_source,article_source};
    for(unsigned s=0;s<2;++s) {
        HSD_GObj* source=sources[s];set(source,"handle",(int32_t)UINT32_C(0x80000005));
        NativeScriptValue out;
        CHECK(invoke(source,s?"Mixed":"InvokeMixed",&out)==NativeScriptVM_Ok);
        CHECK(field(source,"scalar",NULL).as.f32==6.25f);++cases;
        CHECK(invoke(source,s?"Identity":"InvokeIdentity",&out)==NativeScriptVM_Ok);
        CHECK(field(source,"result",NULL).as.i64==-1);++cases;
        CHECK(invoke(source,s?"Flag":"InvokeFlag",&out)==NativeScriptVM_Ok);
        CHECK(field(source,"boolean",NULL).as.u64);++cases;
        CHECK(invoke(source,s?"SetValue":"InvokeSetValue",&out)==NativeScriptVM_Ok);
        CHECK(field(source,"boolean",NULL).as.u64&&field(target,"scalar",NULL).as.f32==17);++cases;
        CHECK(invoke(source,s?"Many":"InvokeMany",&out)==NativeScriptVM_Ok);
        CHECK(field(source,"result",NULL).as.i64==29);++cases;
        NativeScriptRuntimeContext context_value=context(source);
        for(unsigned n=0;n<128;++n) {
            NativeScriptValue args[6],before[6];uint32_t semantic=parameter_call(source,"Mixed",4,6,args);
            args[2]=(NativeScriptValue){.type=n&1?NativeScriptType_I32:NativeScriptType_I64,.as.i64=(int)n-64};
            args[3]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=.25f*(1+(n&3))};
            args[4]=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=n&1};
            args[5]=(NativeScriptValue){.type=n&2?NativeScriptType_U32:NativeScriptType_I32,.as.u64=n&31};
            memcpy(before,args,sizeof(args));
            NativeScriptCallLayout layout;NativeScriptBindingResolverContext resolver={.context_mask=s?NativeScriptBindings_Article:NativeScriptBindings_Fighter};
            CHECK(nativeScriptBindings_DescribeCall(&resolver,semantic,&layout,&diagnostic));
            CHECK(layout.argument_count==6&&!layout.output_mask);
            for(unsigned a=2;a<6;++a)CHECK(layout.argument_types[a]==NativeScriptType_Invalid);
            CHECK(nativeScriptBindings_Invoke(&context_value,semantic,NULL,0,args,6,&out,&diagnostic));
            float expected=(n&1)?((float)((int)n-64)*args[3].as.f32+(float)(n&31)):-args[3].as.f32;
            CHECK(out.type==NativeScriptType_F32&&out.as.f32==expected&&!memcmp(args,before,sizeof(args)));++cases;
        }
        for(unsigned mode=0;mode<8;++mode) {
            NativeScriptValue args[6],before[6];uint32_t semantic=parameter_call(source,"Mixed",4,6,args);
            args[2]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=-7};
            args[3]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=1.25f};
            args[4]=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=1};
            args[5]=(NativeScriptValue){.type=NativeScriptType_U32,.as.u64=15};
            unsigned count=6;
            if(mode==0){count=5;semantic=parameter_call(source,"Mixed",4,count,args);}
            if(mode==1)args[2].type=NativeScriptType_F32;
            if(mode==2)args[4].type=NativeScriptType_I32;
            if(mode==3)args[3].type=NativeScriptType_Text;
            if(mode==4)semantic&=~NATIVE_SCRIPT_TARGET_EXPORT_ARGUMENT_MASK;
            if(mode==5)semantic=(semantic&~NATIVE_SCRIPT_TARGET_EXPORT_ARGUMENT_MASK)|(33u<<NATIVE_SCRIPT_TARGET_EXPORT_ARGUMENT_SHIFT);
            if(mode==6)++count;
            if(mode==7)args[5].type=NativeScriptType_Null;
            memcpy(before,args,sizeof(args));out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
            int64_t visits=field(target,"visits",NULL).as.i64;
            CHECK(!nativeScriptBindings_Invoke(&context_value,semantic,NULL,0,args,count,&out,&diagnostic));
            CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&!memcmp(args,before,sizeof(args)));
            CHECK(field(target,"visits",NULL).as.i64==visits&&nativeScriptRuntime_Idle());++cases;
        }
        NativeScriptProgramIndex* declarations=&target_packages[((Fighter*)target->user_data)->kind-Ft_Kind_CustomFirst].compiler.bank.declarations;
        NativeScriptFunctionDeclaration* helper=&declarations->functions[function(1,"Mixed")];
        for(unsigned mode=0;mode<3;++mode) {
            NativeScriptFunctionDeclaration saved=*helper;uint32_t first=helper->parameter_first;
            NativeScriptParameterDeclaration parameter=declarations->parameters[first];
            if(mode==0)helper->parameter_first=declarations->parameter_count+1;
            if(mode==1)helper->parameter_count=31;
            if(mode==2)declarations->parameters[first].type=NativeScriptType_Object;
            NativeScriptValue args[6],out={.type=NativeScriptType_U64,.as.u64=99};uint32_t semantic=parameter_call(source,"Mixed",4,6,args);
            args[2]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=-7};args[3]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=1.25f};
            args[4]=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=1};args[5]=(NativeScriptValue){.type=NativeScriptType_U32,.as.u64=15};
            CHECK(!nativeScriptBindingTargets_Invoke(&context_value,semantic,args,6,&out,&diagnostic));
            CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);
            *helper=saved;declarations->parameters[first]=parameter;CHECK(nativeScriptRuntime_Idle());++cases;
        }
    }
}
