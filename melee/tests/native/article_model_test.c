#include <melee/it/native_article_model.h>
#include <melee/it/types.h>
#include <melee/lb/native_model_joint.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/mtx.h>
#include <melee/lb/lb_00B0.h>
#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

void test_native_article_model_roots_import(void);
void test_native_article_joint_visibility(void);

// Direct shared operations work on HSD joints, with no entity/userdata context.
static void test_native_model_joint(void)
{
    HSD_JObj chain[14] = { 0 };
    for (unsigned i = 0; i < 13; ++i) chain[i].child = &chain[i + 1];
    chain[6].flags = JOBJ_INSTANCE; // Path resolution is distinct from visibility traversal.
    assert(nativeModelJoint_Resolve(chain, "") == chain);
    assert(nativeModelJoint_Resolve(chain, "0/0/0/0/0/0/0/0/0/0/0/0") == &chain[12]);
    assert(nativeModelJoint_Resolve(chain, "0/0/0/0/0/0/0/0/0/0/0/0/0") == NULL);
    assert(nativeModelJoint_Resolve(chain, NULL) == NULL);
    assert(nativeModelJoint_Resolve(NULL, "") == NULL);

    HSD_JObj siblings[256] = { 0 };
    HSD_JObj wide = { .child = siblings };
    for (unsigned i = 0; i < 255; ++i) siblings[i].next = &siblings[i + 1];
    assert(nativeModelJoint_Resolve(&wide, "255") == &siblings[255]);
    assert(nativeModelJoint_Resolve(&wide, "000255") == &siblings[255]);
    const char* invalid[] = { "256", "9999999999999999999", "-1", "+1",
                             " 1", "1 ", "1/", "0//1", "0/x" };
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
        assert(nativeModelJoint_Resolve(&wide, invalid[i]) == NULL);

    for (unsigned mask = 0; mask < 8; ++mask) {
        HSD_JObj actual[4] = { 0 }, expected[4] = { 0 };
        for (unsigned which = 0; which < 2; ++which) {
            HSD_JObj* graph = which ? expected : actual;
            graph[0].child = &graph[1];
            graph[1].parent = graph[2].parent = &graph[0];
            graph[1].child = &graph[3];
            graph[1].next = &graph[2];
            graph[3].parent = &graph[1];
            graph[0].flags = JOBJ_HIDDEN;
            graph[1].flags = (mask & 1 ? JOBJ_MTX_INDEP_SRT : 0) |
                (mask & 2 ? JOBJ_USE_QUATERNION : 0) |
                (mask & 4 ? JOBJ_INSTANCE : 0);
            graph[2].flags = graph[3].flags = JOBJ_HIDDEN;
        }
        HSD_JObjSetTranslateY(&expected[1], 7.25f);
        nativeModelJoint_Write(nativeModelJoint_Resolve(actual, "0"), 1, 7.25f);
        Quaternion quaternion = { .x = .25f, .y = -.5f, .z = .75f, .w = 1.25f };
        HSD_JObjSetRotation(&expected[1], &quaternion);
        const float components[] = { .25f, -.5f, .75f, 1.25f };
        for (unsigned component = 0; component < 4; ++component)
            nativeModelJoint_Write(&actual[1], 11 + component, components[component]);
        HSD_JObjClearFlagsAll(&expected[1], JOBJ_HIDDEN);
        nativeModelJoint_Write(&actual[1], 10, 1);
        HSD_JObjSetFlags(&expected[1], JOBJ_HIDDEN);
        nativeModelJoint_Write(&actual[1], 15, 0);
        for (unsigned i = 0; i < 4; ++i) {
            assert(actual[i].flags == expected[i].flags);
            assert(memcmp(&actual[i].translate, &expected[i].translate, sizeof(Vec3)) == 0);
            assert(memcmp(&actual[i].rotate, &expected[i].rotate, sizeof(Quaternion)) == 0);
        }
        assert(nativeModelJoint_Read(&actual[1], 1) == 7.25f);
        assert(nativeModelJoint_Read(&actual[1], 3) == (mask & 2 ? 0 : .25f));
        assert(nativeModelJoint_Read(&actual[1], 10) == 0);
        assert(nativeModelJoint_Read(&actual[1], 15) == 0);
        assert(actual[0].flags & JOBJ_HIDDEN);
        assert(actual[2].flags & JOBJ_HIDDEN); // Selected joint's sibling is untouched.
        assert((actual[3].flags & JOBJ_HIDDEN) == (mask & 4 ? JOBJ_HIDDEN : 0));
        HSD_JObj retained[4];
        memcpy(retained, actual, sizeof(retained));
        for (unsigned channel = 0; channel < 16; ++channel) {
            nativeModelJoint_Write(&actual[1], channel, NAN);
            nativeModelJoint_Write(&actual[1], channel, INFINITY);
            nativeModelJoint_Write(&actual[1], channel, -INFINITY);
        }
        nativeModelJoint_Write(&actual[1], 9, 123); // Existence remains read-only.
        nativeModelJoint_Write(&actual[1], 16, 123);
        assert(memcmp(actual, retained, sizeof(actual)) == 0);
    }
    for (unsigned channel = 0; channel < 16; ++channel) {
        float missing = ((channel >= 6 && channel <= 8) || channel == 14) ? 1 : 0;
        assert(nativeModelJoint_Read(NULL, channel) == missing);
        nativeModelJoint_Write(NULL, channel, 9);
    }
}

void test_native_article_model(void)
{
    test_native_model_joint();
    test_native_article_model_roots_import();
    test_native_article_joint_visibility();
    HSD_JObj target = { .scale = { 1, 1, 1 } };
    HSD_JObj first = { .next = &target, .scale = { 1, 1, 1 } };
    HSD_JObj parent = { .child = &first, .scale = { 1, 1, 1 } };
    HSD_JObj root = { .child = &parent, .scale = { 1, 1, 1 } };
    HSD_GObj object = { .hsd_obj = &root };
    for (unsigned channel = 0; channel < 9; ++channel) {
        float value = 0.25f + (float) channel;
        target.flags &= ~JOBJ_MTX_DIRTY;
        nativeArticleModel_Write(&object, 0, "0/1", channel, value);
        assert(nativeArticleModel_Read(&object, 0, "0/1", channel) == value);
        assert((target.flags & JOBJ_MTX_DIRTY) != 0);
    }
    assert(first.translate.y == 0 && root.scale.z == 1);
    nativeArticleModel_Write(&object, 0, "", 8, 2.5f);
    assert(root.scale.z == 2.5f && target.scale.z == 8.25f);
    const char* invalid[] = { "2", "0/2", "0/", "-1", "256", "0//1",
                             "0/0/0/0/0/0/0/0/0/0/0/0/0" };
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        nativeArticleModel_Write(&object, 0, invalid[i], 1, 99);
        assert(nativeArticleModel_Read(&object, 0, invalid[i], 1) == 0);
        assert(nativeArticleModel_Read(&object, 0, invalid[i], 8) == 1);
    }
    assert(target.translate.y == 1.25f);
    nativeArticleModel_Write(&object, 0, "0/1", 1, NAN);
    assert(target.translate.y == 1.25f);
    nativeArticleModel_Write(NULL, 0, "", 1, 99);
    assert(nativeArticleModel_Read(NULL, 0, "", 8) == 1);
    target.flags |= JOBJ_USE_QUATERNION;
    nativeArticleModel_Write(&object, 0, "0/1", 3, 99);
    assert(target.rotate.x == 3.25f);
    assert(nativeArticleModel_Read(&object, 0, "0/1", 3) == 0);
    for (unsigned component = 0; component < 4; ++component) {
        nativeArticleModel_Write(&object, 0, "0/1", 11 + component,
                                  0.125f + component);
        assert(nativeArticleModel_Read(&object, 0, "0/1", 11 + component) ==
               0.125f + component);
    }
    assert(target.rotate.w == 3.125f && target.flags & JOBJ_USE_QUATERNION);
    Item item = { 0 };
    DynamicBoneTable dynamic = { 0 };
    dynamic.bones[3] = &first;
    item.xBBC_dynamicBoneTable = &dynamic;
    HSD_JObj extra = { .scale = { 1, 1, 1 } };
    item.native_models[0] = &extra;
    object.user_data = &item;
    // Read the selected graph's actual world point once, through the same shared
    // HSD calculation used by stock article behavior. Slot zero is not a dynamic
    // bone-table index; slot one is an independently loaded model root.
    HSD_JObj* selected[] = { &root, &parent, &target, &extra };
    const char* world_paths[] = { "", "0", "0/1", "" };
    for (unsigned i = 0; i < 4; ++i) {
        Vec3 expected, actual;
        lb_8000B1CC(selected[i], NULL, &expected);
        assert(nativeArticleModel_ReadWorldPosition(&object, i == 3 ? 1 : 0,
                                                    world_paths[i], &actual));
        assert(actual.x == expected.x && actual.y == expected.y && actual.z == expected.z);
    }
    Vec3 absent = { 99, 99, 99 };
    assert(!nativeArticleModel_ReadWorldPosition(&object, 4, "", &absent));
    assert(absent.x == 0 && absent.y == 0 && absent.z == 0);
    assert(!nativeArticleModel_ReadWorldPosition(&object, 0, "0/7", &absent));
    assert(!nativeArticleModel_ReadWorldPosition(NULL, 0, "", &absent));
    assert(!nativeArticleModel_ReadWorldPosition(&object, 0, "", NULL));
    // Dynamic article indices are not root-relative child paths. State changes
    // preserve fuse transforms through the table, including matrix invalidation.
    for (unsigned channel = 0; channel < 9; ++channel) {
        first.flags &= ~JOBJ_MTX_DIRTY;
        float value = 10.5f + channel;
        nativeArticleModel_WriteDynamic(&object, 3, channel, value);
        assert(nativeArticleModel_ReadDynamic(&object, 3, channel) == value);
        assert(first.flags & JOBJ_MTX_DIRTY);
    }
    assert(target.translate.y == 1.25f && root.translate.y == 0);
    nativeArticleModel_WriteDynamic(&object, 3, 1, NAN);
    nativeArticleModel_WriteDynamic(&object, 100, 1, 99);
    assert(nativeArticleModel_ReadDynamic(&object, 3, 1) == 11.5f);
    assert(nativeArticleModel_ReadDynamic(&object, 100, 1) == 0);
    assert(nativeArticleModel_ReadDynamic(&object, 4, 8) == 1);
    assert(nativeArticleModel_ReadDynamic(NULL, 3, 1) == 0);
    nativeArticleModel_Write(&object, 1, "", 1, 12);
    assert(nativeArticleModel_Read(&object, 1, "", 1) == 12);
    assert(nativeArticleModel_Read(&object, 1, "", 9) == 1);
    assert(nativeArticleModel_Read(&object, 2, "", 9) == 0);
    assert(nativeArticleModel_Read(&object, 4, "", 9) == 0);
    assert(root.translate.y == 0);
    nativeArticleModel_Write(&object, 1, "", 10, 0);
    assert(nativeArticleModel_Read(&object, 1, "", 10) == 0);
    nativeArticleModel_Write(&object, 1, "", 10, 1);
    assert(nativeArticleModel_Read(&object, 1, "", 10) == 1);

    Vec3 translation = { 10, -20, 30 }, scaling = { 2, 3, 4 };
    Quaternion rotation = { .x = .25f, .y = -.5f, .z = .75f, .w = 1.25f };
    for (unsigned flags = 0; flags < 4; ++flags) {
        for (unsigned force = 0; force < 2; ++force) {
            HSD_JObj expected = { 0 };
            extra = (HSD_JObj){ 0 };
            extra.flags = expected.flags =
                (flags & 1 ? JOBJ_USE_QUATERNION : 0) |
                (flags & 2 ? JOBJ_MTX_INDEP_SRT : 0);
            if (force) {
                HSD_JObjSetTranslateWithMtxDirty(&expected, &translation);
                HSD_JObjSetRotationWithMtxDirty(&expected, &rotation);
                HSD_JObjSetScaleWithMtxDirty(&expected, &scaling);
            } else {
                HSD_JObjSetTranslate(&expected, &translation);
                HSD_JObjSetRotation(&expected, &rotation);
                HSD_JObjSetScale(&expected, &scaling);
            }
            nativeArticleModel_SetTransform(&object, 1, "", &translation,
                                             &rotation, &scaling, force != 0);
            assert(memcmp(&extra.translate, &expected.translate, sizeof(Vec3)) == 0);
            assert(memcmp(&extra.rotate, &expected.rotate, sizeof(Quaternion)) == 0);
            assert(memcmp(&extra.scale, &expected.scale, sizeof(Vec3)) == 0);
            assert(extra.flags == expected.flags);
        }
    }
    HSD_JObj before_invalid = extra;
    Quaternion invalid_rotation = rotation; invalid_rotation.w = NAN;
    nativeArticleModel_SetTransform(&object, 1, "", &translation,
                                     &invalid_rotation, &scaling, true);
    nativeArticleModel_SetTransform(&object, 1, "0/", &translation,
                                     &rotation, &scaling, true);
    assert(memcmp(&extra, &before_invalid, sizeof(extra)) == 0);

    HSD_Joint immutable_child = { .scale = { 1, 1, 1 }, .position = { 3, 4, 5 } };
    HSD_Joint immutable_root = { .child = &immutable_child,
        .scale = { 2, 2, 2 }, .position = { 6, 7, 8 }, .flags = JOBJ_HIDDEN };
    Article article = { .native_model_root_count = 2,
        .native_model_roots = { { 12345, &immutable_root }, { 67890, NULL } } };
    item.xC4_article_data = &article;
    // The stack-owned transform probe is no longer occupying an instance slot.
    item.native_models[0] = NULL;
    assert(nativeArticleModel_Load(&object, 1, 12345));
    assert(nativeArticleModel_Load(&object, 2, 12345));
    assert(item.native_models[0] != item.native_models[1]);
    assert(item.native_models[0]->child != item.native_models[1]->child);
    assert(item.native_models[0]->translate.x == 6 &&
           item.native_models[0]->child->translate.x == 3);
    assert(item.native_models[0]->flags & JOBJ_HIDDEN);
    HSD_JObj* retained = item.native_models[0];
    assert(!nativeArticleModel_Load(&object, 1, 67890));
    assert(!nativeArticleModel_Load(&object, 1, 99999));
    assert(!nativeArticleModel_Load(&object, 0, 12345));
    assert(!nativeArticleModel_Load(NULL, 1, 12345));
    assert(item.native_models[0] == retained);
    nativeArticleModel_Write(&object, 1, "0", 0, 99);
    assert(immutable_child.position.x == 3 &&
           item.native_models[1]->child->translate.x == 3);
    assert(nativeArticleModel_Load(&object, 1, 12345));
    assert(item.native_models[0]->child->translate.x == 3);

    HSD_AObjDesc animation_desc = { .end_frame = 12 };
    HSD_AnimJoint child_anim = { .aobjdesc = &animation_desc };
    HSD_AnimJoint animation = { .aobjdesc = &animation_desc, .child = &child_anim };
    HSD_MatAnimJoint material = { 0 };
    HSD_ShapeAnimJoint shape = { 0 };
    article.native_model_animation_count = 2;
    article.native_model_animations[0].name_hash = 111;
    article.native_model_animations[0].anim = &animation;
    article.native_model_animations[0].matanim = &material;
    article.native_model_animations[0].shapeanim = &shape;
    article.native_model_animations[1].name_hash = 222; // Explicit null bundle.
    HSD_JObj* expected_model = item.native_models[1];
    HSD_JObjAddAnimAll(expected_model, &animation, &material, &shape);
    assert(nativeArticleModel_AddAnimation(&object, 1, "", 111));
    HSD_JObj* actual_model = item.native_models[0];
    assert(actual_model->aobj && actual_model->child->aobj && expected_model->aobj);
    for (unsigned sample = 0; sample < 4; ++sample) {
        float frame = .25f + sample, rate = .5f + sample;
        HSD_JObjReqAnimAll(expected_model, frame);
        nativeArticleModel_RequestAnimation(&object, 1, "", frame);
        lb_8000BA0C(expected_model->child, rate);
        nativeArticleModel_SetAnimationRate(&object, 1, "0", rate);
        assert(actual_model->child->aobj->framerate == expected_model->child->aobj->framerate);
        HSD_JObjAnimAll(expected_model);
        nativeArticleModel_AdvanceAnimation(&object, 1, "");
        assert(actual_model->aobj->curr_frame == expected_model->aobj->curr_frame);
        assert(actual_model->child->aobj->curr_frame == expected_model->child->aobj->curr_frame);
        assert(actual_model->child->aobj->flags == expected_model->child->aobj->flags);
    }
    float rate_before = actual_model->child->aobj->framerate;
    nativeArticleModel_SetAnimationRate(&object, 1, "0", NAN);
    assert(actual_model->child->aobj->framerate == rate_before);
    actual_model->child->flags |= JOBJ_MTX_DIRTY;
    Vec3 expected_rotation, actual_rotation;
    HSD_MtxGetRotation(actual_model->child->mtx, &expected_rotation);
    u32 matrix_flags = actual_model->child->flags;
    assert(nativeArticleModel_ReadMatrixRotation(&object, 1, "0", &actual_rotation));
    assert(memcmp(&expected_rotation, &actual_rotation, sizeof(Vec3)) == 0);
    assert(actual_model->child->flags == matrix_flags); // No implicit SetupMatrix.
    assert(!nativeArticleModel_ReadMatrixRotation(&object, 1, "0/7", &actual_rotation));
    assert(actual_rotation.x == 0 && actual_rotation.y == 0 && actual_rotation.z == 0);
    assert(!nativeArticleModel_AddAnimation(&object, 1, "", 333));
    assert(!nativeArticleModel_AddAnimation(NULL, 1, "", 111));
    HSD_JObjRemoveAnimAll(expected_model);
    nativeArticleModel_RemoveAnimation(&object, 1, "");
    assert(!actual_model->aobj && !actual_model->child->aobj && !expected_model->aobj);
    assert(nativeArticleModel_AddAnimation(&object, 1, "", 222));
    nativeArticleModel_RemoveAnimation(&object, 1, "");
    // Starting at the first child leaves the root's AObj untouched, matching
    // article constructors which install their bundle on main_root->child.
    article.native_model_animations[0].anim = &child_anim;
    article.native_model_animations[0].matanim = NULL;
    article.native_model_animations[0].shapeanim = NULL;
    assert(nativeArticleModel_AddAnimation(&object, 1, "0", 111));
    assert(!actual_model->aobj && actual_model->child->aobj);
    HSD_JObjAddAnimAll(expected_model->child, &child_anim, NULL, NULL);
    nativeArticleModel_RequestAnimation(&object, 1, "0", 4);
    HSD_JObjReqAnimAll(expected_model->child, 4);
    nativeArticleModel_AdvanceAnimation(&object, 1, "0");
    HSD_JObjAnimAll(expected_model->child);
    assert(actual_model->child->aobj->curr_frame == expected_model->child->aobj->curr_frame);
    assert(!actual_model->aobj);
    nativeArticleModel_RemoveAnimation(&object, 1, "0");
    HSD_JObjRemoveAnimAll(expected_model->child);
    assert(!actual_model->aobj && !actual_model->child->aobj);
    assert(!nativeArticleModel_AddAnimation(&object, 1, "0/7", 111));
    nativeArticleModel_Destroy(&object, 1);
    nativeArticleModel_Destroy(&object, 2);
    assert(item.native_models[0] == NULL && item.native_models[1] == NULL);
    puts("Scripted article joint paths, transforms and matrix invalidation passed");
}
