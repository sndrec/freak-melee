static void normal_sweep_cases(void)
{
    for(unsigned hit=0;hit<2;++hit) {
        seed();collision_result=hit;sweep_previous=(Vec3){-7,8,9};sweep_current=item.pos;
        item.xDD4_itemVar.native_article.surface_normal=(Vec3){1,2,3};Item before=item;
        bool expected_hit=it_8026E9A4(actor,&sweep_previous,&item.pos,&item.xDD4_itemVar.native_article.surface_normal);
        Item expected=item;item=before;NativeScriptValue output=sentinel();
        CHECK(run("NormalSweep",&output)==NativeScriptVM_Ok);
        CHECK(output.type==NativeScriptType_Bool&&output.as.u64==expected_hit);
        CHECK(!memcmp(&item,&expected,sizeof(item)));++pairs;++cases;
        seed();collision_result=hit;before=item;
        bool grounded=it_8026DA70(actor);expected=item;item=before;output=sentinel();
        CHECK(run("CheckGround",&output)==NativeScriptVM_Ok);
        CHECK(output.type==NativeScriptType_Bool&&output.as.u64==grounded);
        CHECK(!memcmp(&item,&expected,sizeof(item)));++pairs;++cases;
    }
    const char* names[]={"ReadNormalX","ReadNormalY","ReadNormalZ"};
    uint32_t semantics[]={NativeScriptBinding_ItemSurfaceNormalX,NativeScriptBinding_ItemSurfaceNormalY,NativeScriptBinding_ItemSurfaceNormalZ};
    uint32_t patterns[]={0,UINT32_C(0x80000000),UINT32_C(0x3f800000),UINT32_C(0xbf800000),1,UINT32_C(0x7f800000),UINT32_C(0x7fc01234)};
    for(unsigned axis=0;axis<3;++axis)for(unsigned n=0;n<7;++n) {
        seed();NativeScriptRuntimeContext c=context();NativeScriptValue value={.type=NativeScriptType_F32},output=sentinel();
        memcpy(&value.as.f32,&patterns[n],sizeof(float));
        CHECK(nativeScriptBindings_Write(&c,semantics[axis],NULL,0,&value,&diagnostic));
        CHECK(run(names[axis],&output)==NativeScriptVM_Ok&&output.type==NativeScriptType_F32);
        CHECK(!memcmp(&output.as.f32,&value.as.f32,sizeof(float)));++cases;
    }
    seed();NativeScriptValue output=sentinel();CHECK(run("WriteNormal",&output)==NativeScriptVM_Ok);
    Vec3 normal=item.xDD4_itemVar.native_article.surface_normal;
    CHECK(normal.x==1.25f&&normal.y==0&&signbit(normal.y)&&normal.z==-3.75f);++cases;
    for(unsigned mutation_case=1;mutation_case<=7;++mutation_case) {
        seed();observed=true;mutation=mutation_case;collision_result=1;
        sweep_previous=(Vec3){-7,8,9};sweep_current=item.pos;
        output=sentinel();CHECK(run("NormalSweep",&output)!=NativeScriptVM_Ok);
        CHECK(output.type==NativeScriptType_U64&&output.as.u64==99);++cases;
    }
    // Writes must reject a different scene/generation before touching storage.
    for(unsigned invalid=0;invalid<3;++invalid) {
        seed();NativeScriptRuntimeContext c=context();Item before=item;
        NativeScriptValue value={.type=NativeScriptType_F32,.as.f32=2};
        if(invalid==0)c.generation++;else if(invalid==1)c.scene_epoch++;else value.type=NativeScriptType_I32;
        CHECK(!nativeScriptBindingItemObservations_Write(&c,NativeScriptBinding_ItemSurfaceNormalX,&value,&diagnostic));
        CHECK(!memcmp(&item,&before,sizeof(item)));++cases;
    }
}
