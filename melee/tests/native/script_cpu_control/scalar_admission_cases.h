static void scalar_admission_cases(void)
{
    for (unsigned bad = 0; bad < 27; ++bad) {
        setup();
        NativeScriptStorageParameters foreign_parameters =
            package.compiler.bank.storage_parameters;
        NativeScriptRuntimeContext* caller = &context;
        switch (bad) {
        case 0: caller = NULL; break;
        case 1: context.scene_epoch = 0; break;
        case 2: ++context.scene_epoch; break;
        case 3: context.source_class = NativeCallbackSource_Item; break;
        case 4: objects[0]->classifier = HSD_GOBJ_CLASS_ITEM; break;
        case 5: heads[HSD_GOBJ_PLINK_FIGHTER] = NULL; break;
        case 6: objects[0]->user_data = NULL; break;
        case 7: fighters[0].gobj = NULL; break;
        case 8: ++fighters[0].x8_spawnNum; break;
        case 9: fighters[0].kind = Ft_Kind_Mario; break;
        case 10: fighters[0].behavior_kind = Ft_Kind_Mario; break;
        case 11: package.ready = false; break;
        case 12: package.assets.ready = false; break;
        case 13: fighters[0].ft_data = NULL; break;
        case 14: gFtDataList[Ft_Kind_CustomFirst] = NULL; break;
        case 15: ftData_CharacterStateTables[Ft_Kind_CustomFirst] = NULL; break;
        case 16: package.program.bank = NULL; break;
        case 17: package.compiler.bank.valid = false; break;
        case 18: view_valid = false; break;
        case 19: storage.live = false; break;
        case 20: storage.owner = 1; break;
        case 21: ++storage.generation; break;
        case 22: storage.layout = NULL; break;
        case 23: storage.parameters = NULL; break;
        case 24: storage.parameters = &foreign_parameters; break;
        case 25: CHECK(nativeMatchPool_Free(&nativeGObjPool, refs[0])); break;
        case 26: view_faulted=true; break;
        }
        Fighter before = fighters[0];
        NativeScriptValue sentinel = {.type=NativeScriptType_Text,
                                     .as.u64=UINT64_C(0x5A1234567890ABCD)};
        NativeScriptValue output = sentinel;
        CHECK(!nativeScriptBindings_Read(caller, NativeScriptBinding_CPUBehavior,
            NULL, 0, &output, &diagnostic));
        CHECK(!memcmp(&output, &sentinel, sizeof(output)));
        NativeScriptValue input = {.type=NativeScriptType_I32, .as.i64=123};
        CHECK(!nativeScriptBindings_Write(caller, NativeScriptBinding_CPUMode,
            NULL, 0, &input, &diagnostic));
        output = sentinel;
        NativeScriptValue command = {.type=NativeScriptType_I32, .as.i64=127};
        CHECK(!nativeScriptBindings_Invoke(caller, NativeScriptBinding_CPUQueue,
            NULL, 0, &command, 1, &output, &diagnostic));
        CHECK(!memcmp(&output, &sentinel, sizeof(output)));
        CHECK(!memcmp(&fighters[0], &before, sizeof(before)));
    }
}

static void cpu_continuation_cases(void)
{
    for (unsigned mode=1; mode<=13; ++mode) {
        // Mode 2 changes a target identity before the native selector reads it;
        // it does not retire the caller. Existing selector tests cover that path.
        if (mode==2) continue;
        setup(); retire=mode;
        NativeScriptValue args[3] = {{.type=NativeScriptType_F32},
            {.type=NativeScriptType_F32}, {.type=NativeScriptType_F32}};
        NativeScriptValue sentinel = {.type=NativeScriptType_Text,
            .as.u64=UINT64_C(0x5A1234567890ABCD)}, output=sentinel;
        bool accepted=nativeScriptBindings_Invoke(&context,NativeScriptBinding_CPUNearestEnemyAt,
            NULL,0,args,3,&output,&diagnostic);
        if(accepted)fprintf(stderr,"Unexpected publication after caller retirement mode %u\n",mode);
        CHECK(!accepted);
        CHECK(!memcmp(&output,&sentinel,sizeof(output)));
        CHECK(!retire);
    }
}
