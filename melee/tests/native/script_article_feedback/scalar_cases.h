static void scalar_cases(void)
{
    // Admit every audited position/scalar recipe with its actual context needs.
    for (int id = 0; id <= UINT16_MAX; ++id) {
        uint32_t requirements;
        if (!nativeScriptEffectDispatch_PositionScalarRequirements(id, &requirements)) continue;
        for (unsigned no_root = 0; no_root < 2; ++no_root) {
            HSD_GObj* object = create(0);
            if (no_root) { object->hsd_obj = NULL; object->obj_kind = HSD_GOBJ_OBJ_NONE; }
            value(object, "effect", id, true);
            unsigned before = spawns;
            NativeScriptValue output = {.type=NativeScriptType_U64, .as.u64=99};
            bool admitted = !(requirements & NativeScriptEffectPosition_FighterParent) &&
                (!no_root || !(requirements & NativeScriptEffectPosition_Root));
            NativeScriptVMStatus status = nativeScriptItem_RunHook(
                object, NativeScriptItemHook_Reflected, NULL, 0, &output);
            if (admitted) {
                CHECK(status == NativeScriptVM_Ok && output.type == NativeScriptType_Bool && !output.as.u64);
                CHECK(spawns == before + 1 && spawned_id == id && !spawned_root);
                CHECK(spawned_position.x == 12.5f && spawned_position.y == -4 && spawned_position.z == 7);
                CHECK(spawned_scalar == -0.75f);
                value(object, "completed", 1, false);
            } else CHECK(status != NativeScriptVM_Ok && output.type == NativeScriptType_U64 &&
                         output.as.u64 == 99 && spawns == before);
            cleanup(object);
        }
    }
    for (unsigned mutation = 1; mutation <= 7; ++mutation) {
        HSD_GObj* object = create(0);
        value(object, "effect", 1166, true);
        NativeMatchPoolReference ref;
        CHECK(nativeMatchPool_Reference(&nativeGObjPool, object, &ref));
        uint64_t identity = bank.storage_parameters.identity;
        mode = mutation;
        NativeScriptValue output = {.type=NativeScriptType_U64, .as.u64=99};
        NativeScriptVMStatus status = nativeScriptItem_RunHook(
            object, NativeScriptItemHook_Reflected, NULL, 0, &output);
        if (mutation == 7) {
            CHECK(status == NativeScriptVM_Ok && output.type == NativeScriptType_Bool && !output.as.u64);
            value(object, "completed", 1, false);
        } else CHECK(status != NativeScriptVM_Ok && output.type == NativeScriptType_U64 && output.as.u64 == 99);
        mode = 0; bank.storage_parameters.identity = identity;
        if (nativeMatchPool_Resolve(ref)) { if (mutation == 1) item.x1C = 17; cleanup(object); }
    }
    for (unsigned invalid = 0; invalid < 12; ++invalid) {
        HSD_GObj* object = create(0);
        NativeScriptRuntimeContext c = context(object);
        NativeScriptValue args[5] = {
            {.type=NativeScriptType_I32, .as.i64=1166},
            {.type=NativeScriptType_F32, .as.f32=1},
            {.type=NativeScriptType_F32, .as.f32=2},
            {.type=NativeScriptType_F32, .as.f32=3},
            {.type=NativeScriptType_F32, .as.f32=4},
        };
        uint32_t count = 5;
        switch (invalid) {
        case 0: args[0] = (NativeScriptValue){.type=NativeScriptType_U64, .as.u64=UINT64_C(0x10000048e)}; break;
        case 1: args[0].as.i64 = -1; break;
        case 2: args[0].as.i64 = 1272; break;
        case 3: args[0].as.i64 = 1040; break;
        case 4: args[0] = (NativeScriptValue){.type=NativeScriptType_F32, .as.f32=1166}; break;
        case 5: args[1].as.f32 = NAN; break;
        case 6: args[2].as.f32 = INFINITY; break;
        case 7: args[3].as.f32 = NAN; break;
        case 8: args[4].as.f32 = NAN; break;
        case 9: args[4].type = NativeScriptType_I32; break;
        case 10: count = 4; break;
        case 11: ++c.generation; break;
        }
        NativeScriptValue output = {.type=NativeScriptType_U64, .as.u64=99};
        NativeScriptDiagnostic diagnostic;
        unsigned before = spawns;
        CHECK(!nativeScriptBindingArticleFeedback_Invoke(&c,
            NativeScriptBinding_ItemSpawnWithScalarAtPosition, args, count, &output, &diagnostic));
        CHECK(spawns == before && output.type == NativeScriptType_U64 && output.as.u64 == 99);
        cleanup(object);
    }
}
