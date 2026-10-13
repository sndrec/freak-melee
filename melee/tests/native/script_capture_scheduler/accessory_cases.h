static unsigned accessory_cases_count;

static void accessory_reset_model(unsigned looping)
{
    memset(accessory_model, 0, sizeof(accessory_model));
    memset(accessory_clocks, 0, sizeof(accessory_clocks));
    memset(&accessory_draw, 0, sizeof(accessory_draw));
    memset(&accessory_material, 0, sizeof(accessory_material));
    memset(&accessory_texture, 0, sizeof(accessory_texture));
    accessory_order_count = accessory_frees = 0;
    accessory_frame = -1;
    for (unsigned i = 0; i < 4; ++i) {
        accessory_clocks[i].framerate = 0.25f + i;
        accessory_clocks[i].flags = ((looping >> i) & 1) ? AOBJ_LOOP : 0;
    }
    accessory_model[0].object.parent.class_info = HSD_CLASS_INFO(&hsdJObj);
    accessory_model[1].object.parent.class_info = HSD_CLASS_INFO(&hsdJObj);
    accessory_model[0].flags = JOBJ_MTX_INDEP_SRT;
    accessory_model[0].child = &accessory_model[1];
    accessory_model[1].parent = &accessory_model[0];
    accessory_model[0].aobj = &accessory_clocks[0];
    accessory_model[1].aobj = &accessory_clocks[1];
    accessory_model[0].u.dobj = &accessory_draw;
    accessory_draw.mobj = &accessory_material;
    accessory_material.aobj = &accessory_clocks[2];
    accessory_material.tobj = &accessory_texture;
    accessory_texture.aobj = &accessory_clocks[3];
}

static Fighter_GObj* accessory_source(void)
{
    Fighter_GObj* object = attachment_create(false);
    Fighter* fp = object->user_data;
    accessory_case = true; accessory_fighter = fp; accessory_retire = 0;
    HSD_CLASS_INFO(&hsdJObj)->head.flags = 1;
    HSD_CLASS_INFO(&hsdJObj)->head.obj_size = sizeof(HSD_JObj);
    ((HSD_JObj*)object->hsd_obj)->object.parent.class_info = HSD_CLASS_INFO(&hsdJObj);
    capture_package.assets.parts.parts_num = capture_package.assets.facts.joints = 52;
    capture_package.assets.parts.part_to_joint = capture_package.assets.part_to_joint;
    capture_package.assets.parts.joint_to_part = capture_package.assets.joint_to_part;
    fp->parts = accessory_parts;
    memset(accessory_parts, 0, sizeof(accessory_parts));
    for (unsigned i = 0; i < 52; ++i) accessory_parts[i].joint = object->hsd_obj;
    memset(&accessory_assets, 0, sizeof(accessory_assets));
    accessory_assets.recipe.root = &accessory_assets.root;
    accessory_assets.recipe.base = (FtAccessoryAnimation){ &accessory_assets.base, &accessory_assets.material, NULL };
    accessory_assets.recipe.variant_count = 4;
    accessory_assets.records[0] = (HSD_DatImportRecord){ .type = DAT_JOINT, .object = &accessory_assets.root };
    accessory_assets.records[1] = (HSD_DatImportRecord){ .type = DAT_ANIMJOINT, .object = &accessory_assets.base };
    accessory_assets.records[2] = (HSD_DatImportRecord){ .type = DAT_MATANIM_JOINT, .object = &accessory_assets.material };
    for (unsigned i = 0; i < 4; ++i) {
        accessory_assets.recipe.variants[i].joint = &accessory_assets.variants[i];
        accessory_assets.records[3+i] = (HSD_DatImportRecord){ .type = DAT_ANIMJOINT, .object = &accessory_assets.variants[i] };
    }
    FtCharacterAssets* assets = &capture_package.assets.character;
    assets->accessory_assets[4] = &accessory_assets.recipe;
    assets->fighter_items[4] = &accessory_assets.recipe;
    assets->import = (HSD_DatImport){ .dat = &accessory_dat, .arena = (u8*)&accessory_assets,
        .arena_size = sizeof(accessory_assets), .arena_used = sizeof(accessory_assets),
        .records = accessory_assets.records, .record_count = 7, .record_capacity = 7 };
    CHECK(nativeScriptFighter_Attach(object));
    return object;
}

static bool accessory_helper(Fighter_GObj* object, unsigned variant, float rate)
{
    const NativeScriptCompileBank* bank = &capture_package.compiler.bank;
    for (uint32_t i = 0; i < bank->declarations.function_count; ++i)
        if (nativeScriptSource_SpanEquals(&bank->tree, declarations[i].name, "Tether_CreateThrowAccessory")) {
            NativeScriptValue args[] = { {.type=NativeScriptType_I32,.as.i64=ftCo_MS_ThrowF+variant},
                {.type=NativeScriptType_F32,.as.f32=rate} }, result;
            NativeScriptDiagnostic diagnostic;
            return nativeScriptFighter_Helper(object, ((Fighter*)object->user_data)->x8_spawnNum,
                i, args, 2, &result, &diagnostic) == NativeScriptVM_Ok;
        }
    CHECK(false);return false;
}

static void accessory_cases(void)
{
    NativeSourceAssetFacts facts = capture_package.assets.facts;
    FighterPartsTable parts = capture_package.assets.parts;
    FtCharacterAssets character = capture_package.assets.character;
    const float scales[] = { 0.65f, 1.0f, 1.8f }, rates[] = { 0.75f, 1.0f, 1.8f };
    for (unsigned variant = 0; variant < 4; ++variant)
        for (unsigned s = 0; s < 3; ++s) for (unsigned r = 0; r < 3; ++r)
            for (unsigned looping = 0; looping < 4; ++looping) {
                Fighter_GObj* object = accessory_source();
                Fighter* fp = object->user_data; fp->x34_scale.y = scales[s];
                accessory_variant = variant; accessory_reset_model(looping);
                reference_accessory_constructor(object, ftCo_MS_ThrowF+variant, rates[r]);
                HSD_JObj expected[2]; HSD_AObj clocks[4]; unsigned order[16], n=accessory_order_count;
                memcpy(expected, accessory_model, sizeof(expected)); memcpy(clocks, accessory_clocks, sizeof(clocks));
                memcpy(order, accessory_order, sizeof(order)); float frame=accessory_frame;
                fp->x20A0_accessory = NULL; accessory_reset_model(looping);
                NativeScriptValue size = {.type=NativeScriptType_F32,.as.f32=scales[s]};
                (void)presentation_state(object, "victim_frame", &size);
                CHECK(accessory_helper(object,variant,rates[r]));
                CHECK(fp->x20A0_accessory==accessory_model && !memcmp(expected,accessory_model,sizeof(expected)));
                CHECK(!memcmp(clocks,accessory_clocks,sizeof(clocks)) && accessory_frame==frame);
                CHECK(n==accessory_order_count && !memcmp(order,accessory_order,n*sizeof(*order)));
                CHECK(accessory_clocks[2].framerate==2.25f && accessory_clocks[3].framerate==3.25f);
                NativeScriptRuntimeContext context=victim_policy_context(object);
                NativeScriptValue result; NativeScriptDiagnostic diagnostic;
                CHECK(nativeScriptBindingPresentation_AccessoryInvoke(&context,NativeScriptBinding_AccessoryClear,
                    NULL,0,&result,&diagnostic) && !fp->x20A0_accessory && accessory_frees==1);
                HSD_GObjFree(object);accessory_case=false;++cases;++accessory_cases_count;
            }
    for (unsigned step=1;step<=6;++step) {
        Fighter_GObj* object=accessory_source();Fighter* fp=object->user_data;
        accessory_variant=0;accessory_reset_model(0);accessory_retire=step;
        CHECK(!accessory_helper(object,0,1));
        CHECK(accessory_order_count==step && accessory_order[step-1]==step);
        if(step==1)CHECK(!fp->x20A0_accessory && accessory_frees==1);
        HSD_GObjFree(object);accessory_case=false;++cases;++accessory_cases_count;
    }
    for (unsigned policy=0;policy<2;++policy)for(unsigned looping=0;looping<4;++looping) {
        Fighter_GObj* object=accessory_source();Fighter* fp=object->user_data;
        accessory_reset_model(looping);fp->x20A0_accessory=accessory_model;
        NativeScriptRuntimeContext context=victim_policy_context(object);
        NativeScriptValue args[]={{.type=NativeScriptType_F32,.as.f32=2.5f},
            {.type=NativeScriptType_Bool,.as.u64=policy}},result;NativeScriptDiagnostic diagnostic;
        CHECK(nativeScriptBindingPresentation_AccessoryInvoke(&context,NativeScriptBinding_AccessorySetGeometryRate,
            args,2,&result,&diagnostic));
        for(unsigned i=0;i<2;++i)CHECK(accessory_clocks[i].framerate==
            ((policy || !((looping>>i)&1))?2.5f:0.25f+i));
        CHECK(accessory_clocks[2].framerate==2.25f && accessory_clocks[3].framerate==3.25f);
        HSD_GObjFree(object);accessory_case=false;++cases;++accessory_cases_count;
    }
    for (unsigned bad=0;bad<12;++bad) {
        Fighter_GObj* object=accessory_source();Fighter* fp=object->user_data;
        accessory_reset_model(0);fp->x20A0_accessory=accessory_model;
        NativeScriptRuntimeContext context=victim_policy_context(object);
        NativeScriptValue args[3]={{.type=NativeScriptType_I32,.as.i64=4},
            {.type=NativeScriptType_I32,.as.i64=0}},result={.type=NativeScriptType_U64,.as.u64=123};
        NativeScriptDiagnostic diagnostic;
        uint32_t semantic=NativeScriptBinding_AccessoryAddAnimation,count=2;
        if(bad==0)args[0].as.i64=-1;
        if(bad==1)args[0].as.i64=FT_CHARACTER_ITEM_CAPACITY;
        if(bad==2)args[1].as.i64=-2;
        if(bad==3)args[1].as.i64=4;
        if(bad==4)capture_package.assets.character.accessory_assets[4]=NULL;
        if(bad==5)capture_package.assets.character.fighter_items[4]=NULL;
        if(bad==6)accessory_assets.records[0].type=DAT_ANIMJOINT;
        if(bad==7)count=1;
        if(bad==8){semantic=NativeScriptBinding_AccessoryRequestFrame;count=1;args[0]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=NAN};}
        if(bad==9){semantic=NativeScriptBinding_AccessorySetGeometryRate;args[0]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=1};args[1]=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=2};}
        if(bad==10){semantic=NativeScriptBinding_AccessoryFollowBone;count=1;args[0].as.i64=52;}
        if(bad==11)fp->x20A0_accessory=NULL;
        CHECK(!nativeScriptBindingPresentation_AccessoryInvoke(&context,semantic,args,count,&result,&diagnostic));
        CHECK(result.type==NativeScriptType_U64 && result.as.u64==123 && !accessory_order_count && !accessory_frees);
        HSD_GObjFree(object);accessory_case=false;++cases;++accessory_cases_count;
    }
    capture_package.assets.facts=facts;capture_package.assets.parts=parts;
    capture_package.assets.character=character;accessory_retire=0;
}
