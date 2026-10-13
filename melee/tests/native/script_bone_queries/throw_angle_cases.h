static void throw_angle_cases(void)
{
    for (unsigned shape = 0; shape < 3; ++shape)
        for (unsigned facing = 0; facing < 2; ++facing)
            for (unsigned iteration = 0; iteration < 6; ++iteration) {
                HSD_GObj* object = presentation_create(source.target, true);
                rig(object, shape);
                joints[0].rotate.y += facing ? (float)M_PI : 0;
                joints[2].translate.x += (float)iteration * 0.3f;
                Vec3 muzzle_offset = {0, 1.2325000762939453f, 4.263599872589111f};
                Vec3 barrel_offset = {0, 1.2325000762939453f, 0.013600001111626625f};
                Vec3 muzzle, barrel;
                lb_8000B1CC(&joints[2], &muzzle_offset, &muzzle);
                lb_8000B1CC(&joints[2], &barrel_offset, &barrel);
                float expected = atan2f(muzzle.y - barrel.y, muzzle.x - barrel.x);
                for (unsigned i = 0; i < 4; ++i) joints[i].flags |= JOBJ_MTX_DIRTY;
                CHECK(helper(object, "ThrowAngle") == NativeScriptVM_Ok);
                NativeScriptValue got = presentation_state(object, "x", NULL);
                CHECK(got.type == NativeScriptType_F32);
                CHECK(!memcmp(&got.as.f32, &expected, sizeof(float)));
                ++comparisons; ++cases;
                HSD_GObjFree(object);
            }
}
