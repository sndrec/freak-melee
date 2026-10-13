// Exact extracted production routing bodies. HSD traversal is an observed sink;
// native_model_runtime separately executes real HSD animation on the subtree.
#include <melee/it/native_article_model.h>
#include <melee/it/types.h>
#include <melee/lb/native_model_joint.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#undef NDEBUG
#include <assert.h>

static unsigned checks, calls;
static HSD_JObj* selected;
static unsigned operation;
static float requested;
static HSD_AnimJoint* observed_anim;
static HSD_MatAnimJoint* observed_material;
static HSD_ShapeAnimJoint* observed_shape;
#define CHECK(x) do { ++checks; assert(x); } while (0)

void HSD_JObjAddAnimAll(HSD_JObj* joint, HSD_AnimJoint* anim,
                       HSD_MatAnimJoint* material, HSD_ShapeAnimJoint* shape)
{
    ++calls; selected = joint; operation = 1;
    observed_anim = anim; observed_material = material; observed_shape = shape;
}
void HSD_JObjReqAnimAll(HSD_JObj* joint, float frame)
{
    ++calls; selected = joint; operation = 2; requested = frame;
}
void HSD_JObjAnimAll(HSD_JObj* joint)
{
    ++calls; selected = joint; operation = 3;
}
void HSD_JObjRemoveAnimAll(HSD_JObj* joint)
{
    ++calls; selected = joint; operation = 4;
}
#include "actual.h"

int main(void)
{
    Item item = { 0 }; Article article = { 0 };
    HSD_JObj roots[4] = { 0 }, first[4] = { 0 }, second[4] = { 0 }, leaf[4] = { 0 };
    HSD_AnimJoint anim = { 0 }; HSD_MatAnimJoint material = { 0 };
    HSD_ShapeAnimJoint shape = { 0 };
    HSD_GObj object = { .user_data = &item, .hsd_obj = &roots[0] };
    item.xC4_article_data = &article;
    article.native_model_animation_count = 2;
    article.native_model_animations[0].name_hash = 123;
    article.native_model_animations[0].anim = &anim;
    article.native_model_animations[0].matanim = &material;
    article.native_model_animations[0].shapeanim = &shape;
    article.native_model_animations[1].name_hash = 456; // Explicit empty bundle.
    for (unsigned model = 0; model < 4; ++model) {
        roots[model].child = &first[model]; first[model].next = &second[model];
        second[model].child = &leaf[model];
        if (model) item.native_models[model - 1] = &roots[model];
        const char* paths[] = { "", "0", "1", "1/0" };
        HSD_JObj* expected[] = { &roots[model], &first[model], &second[model], &leaf[model] };
        for (unsigned p = 0; p < 4; ++p) {
            unsigned before = calls;
            CHECK(nativeArticleModel_AddAnimation(&object, model, paths[p], 123));
            CHECK(calls == before + 1 && selected == expected[p] && operation == 1);
            CHECK(observed_anim == &anim && observed_material == &material && observed_shape == &shape);
            nativeArticleModel_RequestAnimation(&object, model, paths[p], 7.25f);
            CHECK(calls == before + 2 && selected == expected[p] && operation == 2 && requested == 7.25f);
            nativeArticleModel_AdvanceAnimation(&object, model, paths[p]);
            CHECK(calls == before + 3 && selected == expected[p] && operation == 3);
            nativeArticleModel_RemoveAnimation(&object, model, paths[p]);
            CHECK(calls == before + 4 && selected == expected[p] && operation == 4);
        }
        CHECK(nativeArticleModel_AddAnimation(&object, model, "0", 456));
        CHECK(!observed_anim && !observed_material && !observed_shape);
        unsigned before = calls;
        const char* bad[] = { NULL, "2", "0/0", "0/", "/0", "-1", "256", "0x1" };
        for (unsigned p = 0; p < sizeof(bad)/sizeof(*bad); ++p) {
            CHECK(!nativeArticleModel_AddAnimation(&object, model, bad[p], 123));
            nativeArticleModel_RequestAnimation(&object, model, bad[p], 0);
            nativeArticleModel_AdvanceAnimation(&object, model, bad[p]);
            nativeArticleModel_RemoveAnimation(&object, model, bad[p]);
            CHECK(calls == before);
        }
        CHECK(!nativeArticleModel_AddAnimation(&object, model, "", 0));
        CHECK(!nativeArticleModel_AddAnimation(&object, model, "", 999));
        nativeArticleModel_RequestAnimation(&object, model, "", NAN);
        nativeArticleModel_RequestAnimation(&object, model, "", INFINITY);
        CHECK(calls == before);
    }
    unsigned before = calls;
    for (unsigned model = 0; model <= 4; ++model) {
        CHECK(!nativeArticleModel_AddAnimation(NULL, model, "", 123));
        nativeArticleModel_RequestAnimation(NULL, model, "", 0);
        nativeArticleModel_AdvanceAnimation(NULL, model, "");
        nativeArticleModel_RemoveAnimation(NULL, model, "");
        CHECK(calls == before);
    }
    CHECK(!nativeArticleModel_AddAnimation(&object, 4, "", 123));
    nativeArticleModel_RequestAnimation(&object, 4, "", 0);
    nativeArticleModel_AdvanceAnimation(&object, 4, "");
    nativeArticleModel_RemoveAnimation(&object, 4, ""); CHECK(calls == before);
    item.native_models[0] = NULL;
    CHECK(!nativeArticleModel_AddAnimation(&object, 1, "", 123));
    nativeArticleModel_AdvanceAnimation(&object, 1, ""); CHECK(calls == before);
    item.xC4_article_data = NULL;
    CHECK(!nativeArticleModel_AddAnimation(&object, 0, "0", 123)); CHECK(calls == before);
    printf("{\"checks\":%u,\"routed_calls\":%u,\"model_path_pairs\":16}\n", checks, calls);
}
