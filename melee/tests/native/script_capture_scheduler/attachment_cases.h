static Fighter_GObj* attachment_create(bool skin)
{
    reset();attachment_skin=skin;
    attachment_imports=attachment_loads=attachment_frees=attachment_allocs=attachment_chain_frees=attachment_visibility_calls=attachment_retire=0;
    memset(&attachment_costume,0,sizeof(attachment_costume));
    attachment_costume.dat=(HSD_Dat){.roots=attachment_roots,.root_count=1,.strings="Accessory_TopN_joint"};
    attachment_costume.import=(HSD_DatImport){.dat=&attachment_costume.dat,.arena=attachment_arena,
        .arena_size=sizeof(attachment_arena),.record_capacity=16};
    attachment_selected=(UnkCostumeStruct){.native_assets=&attachment_costume};
    CostumeListsForeachCharacter[Ft_Kind_CustomFirst]=(struct UnkCostumeList){
        .costume_list=&attachment_selected,.numCostumes=1};
    Fighter_GObj* object=presentation_create(Ft_Kind_CustomFirst,false);
    Fighter* fp=object->user_data;attachment_fighter=fp;
    fp->ft_data=&capture_package.assets.character.data;gFtDataList[fp->kind]=fp->ft_data;
    fp->x1C_actionStateList=owned_common;fp->x20_actionStateList=owned_special;
    ftData_CharacterStateTables[fp->kind]=capture_package.assets.motions;
    capture_package.assets.parts.parts_num=8;capture_package.assets.facts.joints=8;
    capture_package.assets.facts.omission_count=0;
    ftPartsTable=attachment_part_tables;
    ftPartsTable[fp->kind]=&capture_package.assets.parts;fp->parts=attachment_parts;
    memset(attachment_parts,0,sizeof(attachment_parts));
    for(unsigned i=0;i<8;++i)attachment_parts[i].joint=object->hsd_obj;
    fp->ft_data->x2C=&capture_package.assets.character.dynamics;
    capture_package.assets.character.dynamics=(ftDynamics){.descriptor_count=3,
        .ftDynamicBones=&capture_package.assets.character.dynamic_bones};
    for(unsigned i=0;i<3;++i)capture_package.assets.character.dynamic_bones.array[i]=(BoneDynamicsDesc){
        .bone_id=i,.dyn_desc={.count=2,.pos={i,2,3}}};
    fp->dynamics_num=1;fp->dynamic_bone_sets[0].bone_id=7;
    memset(&attachment_visibility,0,sizeof(attachment_visibility));
    attachment_visibility.descriptor=(FtPartsDesc){.model_num=1,.costume_count=5,
        .vis_table=attachment_visibility.table};
    attachment_visibility.rows[0]=(FtPartsVisLookup){.x0=1,.x4=attachment_visibility.variants};
    attachment_visibility.variants[0]=(TempS){.x0=2,.x4=attachment_visibility.indices};
    attachment_visibility.indices[1]=1;
    attachment_visibility.table[4][0]=attachment_visibility.rows;
    capture_package.assets.character.import=(HSD_DatImport){.arena=(uint8_t*)&attachment_visibility,
        .arena_size=sizeof(attachment_visibility),.arena_used=sizeof(attachment_visibility)};
    capture_package.assets.character.fighter_items[1]=&attachment_visibility.descriptor;
    fp->ft_data->x48_items=capture_package.assets.character.fighter_items;
    capture_package.assets.facts.layout.item_format=FtCharacterItems_ModelParts;
    return object;
}

static uint32_t attachment_function(const char* name)
{
    NativeScriptCompileBank* bank=&capture_package.compiler.bank;
    for(unsigned i=0;i<bank->declarations.function_count;++i)
        if(nativeScriptSource_SpanEquals(&bank->tree,declarations[i].name,name))return i;
    CHECK(false);return UINT32_MAX;
}

static void attachment_cases(void)
{
    FighterPartsTable** saved_tables=ftPartsTable;
    NativeSourceAssetFacts saved_facts=capture_package.assets.facts;
    FighterPartsTable saved_parts=capture_package.assets.parts;
    FtCharacterAssets saved_character=capture_package.assets.character;
    uint32_t original=capture_package.program.hooks[NativeScriptFighter_Load];
    for(unsigned skin=0;skin<2;++skin) {
        Fighter_GObj* object=attachment_create(skin!=0);Fighter* fp=object->user_data;
        capture_package.program.hooks[NativeScriptFighter_Load]=attachment_function("MakeHat");
        CHECK(nativeScriptFighter_CanPublish(fp->kind));nativeScriptFighter_PublishProgram(fp->kind,&capture_package.program);
        CHECK(nativeScriptFighter_Attach(object));
        CHECK(!nativeScriptFighter_IsLoading(object));
        CHECK(attachment_imports==1&&attachment_loads==1&&attachment_allocs==2);
        CHECK(fp->native_attachments[0].joint==attachment_joints&&fp->native_attachments[0].bone==6);
        CHECK(fp->native_attachments[0].objects.count==2&&fp->dynamics_num==3);
        CHECK(fp->native_attachment_dynamic_mask==6&&fp->native_attachments[0].dynamic_mask==6);
        CHECK(presentation_state(object,"items",NULL).as.i64==1);
        NativeScriptRuntimeContext c=victim_policy_context(object);
        const char* source=capture_package.compiler.bank.code.source;
        NativeScriptValue root={.type=NativeScriptType_Text,.as.text={
            .offset=(uint32_t)(strstr(source,"Accessory_TopN_joint")-source),.length=sizeof("Accessory_TopN_joint")-1}};
        NativeScriptValue output={.type=NativeScriptType_U64,.as.u64=123};NativeScriptDiagnostic d={0};
        CHECK(nativeScriptBindingAttachments_Invoke(&c,NativeScriptBinding_AttachmentsHasRoot,&root,1,&output,&d)&&output.as.u64);
        NativeScriptValue args[5]={{.type=NativeScriptType_I32,.as.i64=1},root,
            {.type=NativeScriptType_I32,.as.i64=6},{.type=NativeScriptType_I32,.as.i64=-1},{.type=NativeScriptType_I32}};
        output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=123};
        CHECK(!nativeScriptBindingAttachments_Invoke(&c,NativeScriptBinding_AttachmentsLoad,args,5,&output,&d));
        CHECK(output.type==NativeScriptType_U64&&output.as.u64==123&&attachment_loads==1);
        CHECK(helper(object,"ClearHat"));CHECK(fp->dynamics_num==1&&fp->dynamic_bone_sets[0].bone_id==7);
        CHECK(attachment_frees==1&&attachment_chain_frees==2&&!fp->native_attachment_dynamic_mask&&!fp->native_attachments[0].joint);
        CHECK(helper(object,"ClearHat")&&attachment_frees==1&&attachment_chain_frees==2);
        for(unsigned invalid=0;invalid<6;++invalid) {
            NativeScriptValue bad=root;NativeScriptRuntimeContext context=c;
            if(invalid==0)bad.type=NativeScriptType_I32;
            if(invalid==1)bad.as.text.length=0;
            if(invalid==2)bad.as.text.offset=UINT32_MAX;
            if(invalid==3)bad.as.text.length=256;
            if(invalid==4)++context.generation;
            if(invalid==5)++context.scene_epoch;
            output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=123};
            CHECK(!nativeScriptBindingAttachments_Invoke(&context,NativeScriptBinding_AttachmentsHasRoot,&bad,1,&output,&d));
            CHECK(output.type==NativeScriptType_U64&&output.as.u64==123);++cases;
        }
        HSD_GObjFree(object);++cases;
        // A valid costume visibility descriptor is not an article or raw lookup.
        // Every malformed nested span must fail before graph allocation.
        for(unsigned malformed=0;malformed<10;++malformed) {
            object=attachment_create(skin!=0);Fighter* fp=object->user_data;
            capture_package.program.hooks[NativeScriptFighter_Load]=attachment_function("MakeVisibleHat");
            if(malformed==1)capture_package.assets.facts.layout.item_format=FtCharacterItems_Plain;
            if(malformed==2)capture_package.assets.character.fighter_items[1]=attachment_arena;
            if(malformed==3)attachment_visibility.descriptor.model_num=12;
            if(malformed==4)attachment_visibility.descriptor.costume_count=9;
            if(malformed==5)attachment_visibility.descriptor.vis_table=(void*)attachment_arena;
            if(malformed==6)attachment_visibility.table[4][0]=attachment_arena;
            if(malformed==7)attachment_visibility.rows[0].x4=(void*)attachment_arena;
            if(malformed==8)attachment_visibility.variants[0].x4=attachment_arena;
            if(malformed==9)attachment_visibility.indices[1]=32;
            CHECK(nativeScriptFighter_CanPublish(fp->kind));nativeScriptFighter_PublishProgram(fp->kind,&capture_package.program);
            CHECK(nativeScriptFighter_Attach(object)==(malformed==0));
            CHECK(attachment_loads==(malformed==0)&&attachment_visibility_calls==(malformed==0));
            CHECK(!nativeScriptFighter_IsLoading(object));
            if(malformed==0) {
                CHECK(fp->native_attachments[0].has_visibility&&fp->native_attachments[0].visibility.model_num==1);
                CHECK(helper(object,"ClearHat"));
            }
            HSD_GObjFree(object);++cases;
        }
    }
    // Retirement during the allocating engine leaf cannot publish success or
    // continue to the next cloth allocation in the scripted load callback.
    Fighter_GObj* object=attachment_create(true);attachment_retire=1;
    capture_package.program.hooks[NativeScriptFighter_Load]=attachment_function("MakeHat");
    CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));
    nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&capture_package.program);
    CHECK(!nativeScriptFighter_Attach(object));CHECK(!nativeScriptFighter_IsLoading(object));
    CHECK(attachment_loads==1&&!attachment_allocs);
    --((Fighter*)object->user_data)->x8_spawnNum;attachment_retire=0;
    nativeFighterAttachment_Clear(object);HSD_GObjFree(object);++cases;
    capture_package.program.hooks[NativeScriptFighter_Load]=original;
    CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));
    nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&capture_package.program);
    attachment_skin=false;
    ftPartsTable=saved_tables;capture_package.assets.facts=saved_facts;
    capture_package.assets.parts=saved_parts;capture_package.assets.character=saved_character;
}
