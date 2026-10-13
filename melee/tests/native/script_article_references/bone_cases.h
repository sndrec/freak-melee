static DynamicBoneTable bones,other_bones;
static HSD_JObj bone_joint;
static HSD_GObj* bone_target;
static unsigned bone_mutation;
static void bone_reentry(HSD_JObj* joint)
{
    joint->flags&=~JOBJ_MTX_DIRTY;
    if(bone_mutation==1)((Item*)bone_target->user_data)->xBBC_dynamicBoneTable=&other_bones;
    if(bone_mutation==2)joint->parent=NULL;
    if(bone_mutation==3)HSD_CLASS_METHOD(joint)=NULL;
}
static void bone_cases(NativeScriptReceiverScope* scope,NativeScriptRuntimeContext* caller,
    HSD_GObj* object,uint32_t owner)
{
    Item* data=object->user_data;uint32_t saved_count=model.x4_bone_count;
    DynamicBoneTable* saved_table=data->xBBC_dynamicBoneTable;
    model.x4_bone_count=7;data->xBBC_dynamicBoneTable=&bones;bone_target=object;
    bones=(DynamicBoneTable){0};bone_joint=(HSD_JObj){.parent=&model_root};
    HSD_CLASS_METHOD(&bone_joint)=&class_info;bones.bones[6]=&bone_joint;
    bone_joint.mtx[0][3]=17;bone_joint.mtx[1][3]=19;bone_joint.mtx[2][3]=23;
    NativeScriptValue args[]={{.type=NativeScriptType_U32,.as.u64=data->x1C},
        {.type=NativeScriptType_I32},{.type=NativeScriptType_I32,.as.i64=6},
        {.type=NativeScriptType_F32,.as.f32=1},{.type=NativeScriptType_F32,.as.f32=2},
        {.type=NativeScriptType_F32,.as.f32=3}},result;
    NativeScriptDiagnostic diagnostic;
    uint32_t semantic=NATIVE_SCRIPT_RECEIVER_ARTICLE_BONE_POSITION|owner;
    Vec3 expected;lb_8000B1CC(&bone_joint,NULL,&expected);
    CHECK(nativeScriptBindingArticleReference_Invoke(scope,caller,semantic,NULL,0,args,6,&result,&diagnostic)&&result.as.u64);
    CHECK(args[3].as.f32==expected.x&&args[4].as.f32==expected.y&&args[5].as.f32==expected.z);
    NativeScriptRuntimeContext item_caller={.source=object,.source_class=NativeCallbackSource_Item,
        .generation=data->x1C,.actor_id=data->x1C,.scene_epoch=caller->scene_epoch};
    NativeScriptReceiverScope item_scope={.pool=scope->pool};
    CHECK(nativeScriptReceiver_Push(item_scope.pool,&item_caller,caller->scene_epoch,&bank.declarations,
        Ft_Kind_CustomFirst,nativeScriptActors_Lookup,nativeScriptActors_Helper,NULL,&item_scope.frame,&diagnostic));
    CHECK(nativeScriptBindingArticleReference_Invoke(&item_scope,&item_caller,semantic,NULL,0,args,6,&result,&diagnostic)&&result.as.u64);
    CHECK(args[3].as.f32==17&&args[4].as.f32==19&&args[5].as.f32==23);
    CHECK(nativeScriptReceiver_Pop(item_scope.pool,item_scope.frame,&diagnostic));
    uint32_t function=UINT32_MAX;for(unsigned i=0;i<bank.declarations.function_count;++i)
        if(!functions[i].owner&&nativeScriptSource_SpanEquals(&bank.tree,functions[i].name,"ReadBone"))function=i;
    CHECK(function!=UINT32_MAX);
    CHECK(nativeScriptFighter_Helper(caller->source,caller->generation,function,NULL,0,&result,&diagnostic)==NativeScriptVM_Ok&&result.as.u64);
    CHECK(field(caller->source,"bx",NULL).as.f32==17&&field(caller->source,"by",NULL).as.f32==19&&field(caller->source,"bz",NULL).as.f32==23);
    const int indices[]={-1,7,100,INT32_MAX};
    for(unsigned i=0;i<4;++i) {
        args[2].as.i64=indices[i];args[3].as.f32=1;args[4].as.f32=2;args[5].as.f32=3;
        CHECK(nativeScriptBindingArticleReference_Invoke(scope,caller,semantic,NULL,0,args,6,&result,&diagnostic)&&!result.as.u64);
        CHECK(args[3].as.f32==1&&args[4].as.f32==2&&args[5].as.f32==3);
    }
    args[2].as.i64=6;
    queried_root=object;replace_root=true;
    result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
    CHECK(!nativeScriptBindingArticleReference_Invoke(scope,caller,semantic,NULL,0,args,6,&result,&diagnostic));
    CHECK(result.type==NativeScriptType_U64&&result.as.u64==99&&args[3].as.f32==1&&args[4].as.f32==2&&args[5].as.f32==3);
    object->hsd_obj=&model_root;
    for(unsigned i=1;i<=3;++i) {
        data->xBBC_dynamicBoneTable=&bones;bone_joint.parent=&model_root;
        HSD_CLASS_METHOD(&bone_joint)=&class_info;bone_joint.flags|=JOBJ_MTX_DIRTY;bone_mutation=i;
        result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
        CHECK(!nativeScriptBindingArticleReference_Invoke(scope,caller,semantic,NULL,0,args,6,&result,&diagnostic));
        CHECK(result.type==NativeScriptType_U64&&result.as.u64==99&&args[3].as.f32==1&&args[4].as.f32==2&&args[5].as.f32==3);
    }
    bone_mutation=0;bone_joint.parent=&bone_joint;HSD_CLASS_METHOD(&bone_joint)=&class_info;data->xBBC_dynamicBoneTable=&bones;
    CHECK(!nativeScriptBindingArticleReference_Invoke(scope,caller,semantic,NULL,0,args,6,&result,&diagnostic));
    model.x4_bone_count=saved_count;data->xBBC_dynamicBoneTable=saved_table;
}
