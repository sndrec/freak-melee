#include <melee/it/native_article_model.h>
#include <melee/it/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

// Actual HSD helpers and backend are linked by native_model_runtime_test.
// Dirty propagation is compared with stock, not replaced by a flags-only stub.
typedef struct VisibilityGraph {
    HSD_JObj root, selected, sibling, descendant;
} VisibilityGraph;

static void reset_graph(VisibilityGraph* graph, unsigned mask)
{
    memset(graph, 0, sizeof(*graph));
    graph->root.child = &graph->selected;
    graph->selected.parent = &graph->root;
    graph->selected.next = &graph->sibling;
    graph->selected.child = &graph->descendant;
    graph->sibling.parent = &graph->root;
    graph->descendant.parent = &graph->selected;
    graph->root.flags = JOBJ_HIDDEN | JOBJ_MTX_INDEP_SRT;
    graph->selected.flags = (mask & 1 ? JOBJ_HIDDEN : 0) |
        (mask & 2 ? JOBJ_CLASSICAL_SCALE : 0) |
        (mask & 4 ? JOBJ_INSTANCE : 0);
    graph->sibling.flags = JOBJ_HIDDEN | JOBJ_MTX_INDEP_SRT;
    graph->descendant.flags = JOBJ_HIDDEN | JOBJ_MTX_INDEP_SRT;
}

static void compare_graph(const VisibilityGraph* actual,
                          const VisibilityGraph* expected)
{
    assert(actual->root.flags == expected->root.flags);
    assert(actual->selected.flags == expected->selected.flags);
    assert(actual->sibling.flags == expected->sibling.flags);
    assert(actual->descendant.flags == expected->descendant.flags);
}

void test_native_article_joint_visibility(void)
{
    VisibilityGraph actual, expected;
    for (unsigned mask = 0; mask < 8; ++mask) {
        for (unsigned subtree = 0; subtree < 2; ++subtree) {
            for (unsigned visible = 0; visible < 2; ++visible) {
                reset_graph(&actual, mask);
                reset_graph(&expected, mask);
                HSD_GObj object = { .hsd_obj = &actual.root };
                unsigned channel = subtree ? 10 : 15;
                if (subtree) {
                    if (visible) HSD_JObjClearFlagsAll(&expected.selected, JOBJ_HIDDEN);
                    else HSD_JObjSetFlagsAll(&expected.selected, JOBJ_HIDDEN);
                } else {
                    if (visible) HSD_JObjClearFlags(&expected.selected, JOBJ_HIDDEN);
                    else HSD_JObjSetFlags(&expected.selected, JOBJ_HIDDEN);
                }
                nativeArticleModel_Write(&object, 0, "0", channel, (float)visible);
                compare_graph(&actual, &expected);
                assert(nativeArticleModel_Read(&object, 0, "0", channel) == visible);
                assert(nativeArticleModel_Read(&object, 0, "0", 10) == visible);
                assert(nativeArticleModel_Read(&object, 0, "0", 15) == visible);
                assert(actual.root.flags & JOBJ_HIDDEN); // Hidden parent stays hidden.
                assert(actual.sibling.flags & JOBJ_HIDDEN); // Selected sibling not visited.
                if (!subtree || mask & 4) assert(actual.descendant.flags & JOBJ_HIDDEN);
                else assert((actual.descendant.flags & JOBJ_HIDDEN) == (visible ? 0 : JOBJ_HIDDEN));
            }
        }
    }
    reset_graph(&actual, 1);
    Item item = { 0 };
    item.native_models[0] = &actual.root;
    DynamicBoneTable bones = { 0 };
    bones.bones[7] = &actual.selected;
    item.xBBC_dynamicBoneTable = &bones;
    HSD_GObj object = { .hsd_obj = &actual.root, .user_data = &item };
    nativeArticleModel_Write(&object, 1, "0", 15, 1);
    assert(nativeArticleModel_Read(&object, 1, "0", 15) == 1);
    nativeArticleModel_WriteDynamic(&object, 7, 15, 0);
    assert(nativeArticleModel_ReadDynamic(&object, 7, 15) == 0);
    nativeArticleModel_WriteDynamic(&object, 7, 15, 1);
    assert(nativeArticleModel_ReadDynamic(&object, 7, 15) == 1);
    u32 flags = actual.selected.flags;
    nativeArticleModel_Write(&object, 0, "0", 15, NAN);
    nativeArticleModel_Write(&object, 0, "0/7", 15, 1);
    nativeArticleModel_Write(&object, 2, "0", 15, 1);
    nativeArticleModel_Write(&object, 4, "0", 15, 1);
    nativeArticleModel_WriteDynamic(&object, 100, 15, 1);
    nativeArticleModel_Write(NULL, 0, "0", 15, 1);
    assert(actual.selected.flags == flags);
    assert(nativeArticleModel_Read(&object, 0, "0/7", 15) == 0);
    assert(nativeArticleModel_Read(&object, 2, "0", 15) == 0);
    assert(nativeArticleModel_ReadDynamic(&object, 100, 15) == 0);
    assert(nativeArticleModel_Read(NULL, 0, "0", 15) == 0);
    puts("Article selected-joint visibility matches actual HSD graph operations");
}
