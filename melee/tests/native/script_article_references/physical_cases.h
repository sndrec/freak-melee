static void physical_cases(NativeScriptReceiverScope* scope,NativeScriptRuntimeContext* caller,
    HSD_GObj* object,uint32_t owner,const NativeScriptValue* captured)
{
    Item* data=object->user_data;
    Vec3 saved_position=data->pos,saved_normal=data->xDD4_itemVar.native_article.surface_normal;
    const uint32_t bits[6]={0x80000000u,0x3f800000u,0x7fc12345u,0x3e800000u,0xbf000000u,0x00000001u};
    memcpy(&data->pos,bits,3*sizeof(float));
    memcpy(&data->xDD4_itemVar.native_article.surface_normal,bits+3,3*sizeof(float));
    NativeScriptDiagnostic diagnostic;NativeScriptValue result;
    for(unsigned i=0;i<6;++i) {
        uint32_t semantic=NATIVE_SCRIPT_RECEIVER_ARTICLE_PHYSICAL|(owner<<3)|i;
        CHECK(nativeScriptBindingArticleReference_Read(scope,caller,semantic,captured,1,&result,&diagnostic));
        uint32_t actual;memcpy(&actual,&result.as.f32,sizeof(actual));CHECK(result.type==NativeScriptType_F32&&actual==bits[i]);
        NativeScriptValue absent;CHECK(nativeScriptReceiver_CaptureArticle(scope,caller,UINT32_MAX,owner,&absent,&diagnostic));
        CHECK(nativeScriptBindingArticleReference_Read(scope,caller,semantic,&absent,1,&result,&diagnostic)&&result.type==NativeScriptType_F32&&result.as.f32==0);
        result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
        CHECK(!nativeScriptBindingReceiver_ValidateTarget(scope,caller,semantic,captured,1,&diagnostic));
        CHECK(!nativeScriptBindingReceiver_Write(scope,caller,semantic,captured,1,&result,&diagnostic));
        CHECK(result.type==NativeScriptType_U64&&result.as.u64==99);
    }
    for(unsigned i=6;i<8;++i) {
        result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
        CHECK(!nativeScriptBindingArticleReference_Read(scope,caller,NATIVE_SCRIPT_RECEIVER_ARTICLE_PHYSICAL|(owner<<3)|i,captured,1,&result,&diagnostic));
        CHECK(result.type==NativeScriptType_U64&&result.as.u64==99);
    }
    data->pos=saved_position;data->xDD4_itemVar.native_article.surface_normal=saved_normal;
}
