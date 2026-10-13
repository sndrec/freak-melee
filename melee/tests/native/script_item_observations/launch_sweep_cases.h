static bool stage_sweep(NativeScriptRuntimeContext* context,
                        NativeScriptValue* arguments, uint32_t count,
                        NativeScriptValue* output)
{
    return nativeScriptBindings_Invoke(context, NativeScriptBinding_ItemSweepStage,
                                      NULL, 0, arguments, count, output, &diagnostic);
}

static void launch_sweep_cases(void)
{
    static const uint32_t ids[] = {
        NativeScriptBinding_ItemLaunchAngle, NativeScriptBinding_ItemLaunchSpeed,
        NativeScriptBinding_ItemLaunchOriginX, NativeScriptBinding_ItemLaunchOriginY,
        NativeScriptBinding_ItemLaunchOriginZ, NativeScriptBinding_ItemReflectionFacing,
    };
    static const char* names[] = {
        "LaunchAngle", "LaunchSpeed", "LaunchX", "LaunchY", "LaunchZ", "ReflectionFacing",
    };
    static const uint32_t patterns[] = {
        0, 0x80000000, 0x3f800000, 0xbf800000, 1, 0x7f800000, 0xff800000, 0x7fc01234,
    };
    for (unsigned field = 0; field < 6; ++field) {
        for (unsigned pattern = 0; pattern < sizeof(patterns)/sizeof(patterns[0]); ++pattern) {
            seed();
            float* fields[] = {
                &item.xDD4_itemVar.native_article.launch_angle,
                &item.xDD4_itemVar.native_article.launch_speed,
                &item.xDD4_itemVar.native_article.launch_origin.x,
                &item.xDD4_itemVar.native_article.launch_origin.y,
                &item.xDD4_itemVar.native_article.launch_origin.z, &item.xC68,
            };
            memcpy(fields[field], &patterns[pattern], sizeof(float));
            Item before = item;
            NativeScriptRuntimeContext c = context();
            NativeScriptValue output = sentinel();
            CHECK(nativeScriptBindings_Read(&c, ids[field], NULL, 0, &output, &diagnostic));
            CHECK(output.type == NativeScriptType_F32);
            CHECK(!memcmp(&output.as.f32, &patterns[pattern], sizeof(float)));
            CHECK(run(names[field], &output) == NativeScriptVM_Ok);
            CHECK(output.type == NativeScriptType_F32);
            CHECK(!memcmp(&output.as.f32, &patterns[pattern], sizeof(float)));
            CHECK(!memcmp(&item, &before, sizeof(item)));
            ++cases;
        }
        for (unsigned invalid = 0; invalid < 5; ++invalid) {
            seed();
            NativeScriptRuntimeContext c = context();
            switch (invalid) {
            case 0: ++c.generation; break;
            case 1: ++c.scene_epoch; break;
            case 2: c.source_class = NativeCallbackSource_Fighter; break;
            case 3: storage.live = false; break;
            case 4: item.native_removal_in_progress = true; break;
            }
            NativeScriptValue output = sentinel();
            CHECK(!nativeScriptBindings_Read(&c, ids[field], NULL, 0, &output, &diagnostic));
            CHECK(output.type == NativeScriptType_U64 && output.as.u64 == 99);
            ++cases;
        }
    }
    for (unsigned pending = 0; pending < 4; ++pending) {
        seed(); item.xCA8 = (float)pending;
        Item before = item;
        CHECK(!itFoxIllusion_Logic14_DmgDealt(actor));
        Item expected = item;
        item = before;
        NativeScriptValue output = sentinel();
        CHECK(run("CancelHitlag", &output) == NativeScriptVM_Ok);
        CHECK(output.type == NativeScriptType_Void);
        CHECK(!memcmp(&item, &expected, sizeof(item)));
        ++pairs; ++cases;
    }
    for (unsigned hit = 0; hit < 2; ++hit) {
        for (unsigned graph = 0; graph < 2; ++graph) {
            seed(); if (!graph) actor->hsd_obj = NULL;
            collision_result = hit;
            sweep_previous = (Vec3){-7, 8, 9}; sweep_current = item.pos;
            Item before = item;
            bool expected_hit = it_8026E9A4(actor, &sweep_previous, &item.pos, NULL);
            Item expected = item;
            item = before;
            NativeScriptValue output = sentinel();
            unsigned calls = stage_calls;
            CHECK(run("StageSweep", &output) == NativeScriptVM_Ok);
            CHECK(output.type == NativeScriptType_Bool && output.as.u64 == expected_hit);
            CHECK(!memcmp(&item, &expected, sizeof(item)));
            CHECK(stage_calls == calls + 1);
            ++pairs; ++cases;
        }
    }
    for (unsigned m = 1; m <= 7; ++m) {
        seed(); observed = true; mutation = m; collision_result = 1;
        sweep_previous = (Vec3){-7, 8, 9}; sweep_current = item.pos;
        NativeScriptValue output = sentinel();
        CHECK(run("StageSweep", &output) != NativeScriptVM_Ok);
        CHECK(output.type == NativeScriptType_U64 && output.as.u64 == 99);
        ++cases;
    }
    for (unsigned invalid = 0; invalid < 10; ++invalid) {
        seed();
        NativeScriptRuntimeContext c = context();
        NativeScriptValue args[3] = {
            {.type=NativeScriptType_F32, .as.f32=-7},
            {.type=NativeScriptType_F32, .as.f32=8},
            {.type=NativeScriptType_F32, .as.f32=9},
        };
        uint32_t count = 3;
        switch (invalid) {
        case 0: ++c.generation; break;
        case 1: ++c.scene_epoch; break;
        case 2: c.source_class = NativeCallbackSource_Fighter; break;
        case 3: storage.live = false; break;
        case 4: count = 2; break;
        case 5: args[0].type = NativeScriptType_Text; break;
        case 6: args[0].as.f32 = NAN; break;
        case 7: args[2].as.f32 = INFINITY; break;
        case 8: item.pos.x = NAN; break;
        case 9: item.pos.z = INFINITY; break;
        }
        Item before = item;
        unsigned calls = stage_calls;
        NativeScriptValue output = sentinel();
        CHECK(!stage_sweep(&c, args, count, &output));
        CHECK(output.type == NativeScriptType_U64 && output.as.u64 == 99);
        CHECK(!memcmp(&item, &before, sizeof(item)) && stage_calls == calls);
        ++cases;
    }
}
