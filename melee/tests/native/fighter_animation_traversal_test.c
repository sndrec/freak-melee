#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <melee/ft/ftanim.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/mobj.h>

static unsigned checks;
#define CHECK(condition) do { ++checks; assert(condition); } while (0)

void OSReport(char* format, ...) { (void) format; }
void __assert(char* file, u32 line, char* text)
{
    fprintf(stderr, "%s:%lu: %s\n", file, line, text);
    abort();
}

// Execute each real traversal implementation with dirty private scratch,
// interleaved walks and the deepest supported ancestry. No alternate walker.
#define CHECK_TRAVERSAL(label, Node, Depth, step)                             \
static void label(void)                                                     \
{                                                                          \
    Node first[4] = {0}, second[3] = {0};                                    \
    Node* first_stack[30];                                                  \
    Node* second_stack[30];                                                 \
    memset(first_stack, 0xA5, sizeof(first_stack));                           \
    memset(second_stack, 0x5A, sizeof(second_stack));                         \
    first[0].child = &first[1]; first[0].next = &first[3];                     \
    first[1].child = &first[2];                                              \
    second[0].child = &second[1]; second[0].next = &second[2];                 \
    Node* a = first;                                                        \
    Node* b = second;                                                       \
    Depth a_depth = 0, b_depth = 0;                                          \
    step(&a, &a_depth, first_stack);                                         \
    CHECK(a == &first[1] && a_depth == 1);                                    \
    step(&b, &b_depth, second_stack);                                        \
    CHECK(b == &second[1] && b_depth == 1);                                   \
    step(&a, &a_depth, first_stack);                                         \
    CHECK(a == &first[2] && a_depth == 2);                                    \
    step(&b, &b_depth, second_stack);                                        \
    CHECK(b == &second[2] && b_depth == 0);                                   \
    step(&a, &a_depth, first_stack);                                         \
    CHECK(a == &first[3] && a_depth == 0);                                    \
    step(&b, &b_depth, second_stack);                                        \
    CHECK(b == NULL && b_depth == 0);                                        \
    step(&a, &a_depth, first_stack);                                         \
    CHECK(a == NULL && a_depth == 0);                                        \
    a = first;                                                             \
    for (unsigned i = 0; i < 4; ++i) {                                       \
        CHECK(a == &first[i]);                                              \
        step(&a, &a_depth, first_stack);                                     \
    }                                                                      \
    CHECK(a == NULL && a_depth == 0);                                        \
    Node deep[30] = {0};                                                    \
    for (unsigned i = 0; i < 29; ++i) deep[i].child = &deep[i+1];              \
    a = deep;                                                              \
    for (unsigned i = 0; i < 30; ++i) {                                      \
        CHECK(a == &deep[i] && a_depth == (s32) i);                           \
        step(&a, &a_depth, first_stack);                                     \
    }                                                                      \
    CHECK(a == NULL && a_depth == 0);                                        \
}

CHECK_TRAVERSAL(joints, HSD_Joint, s32, ftAnim_GetNextJointInTree)
CHECK_TRAVERSAL(animations, HSD_AnimJoint, int, ftAnim_GetNextAnimJointInTree)
CHECK_TRAVERSAL(materials, HSD_MatAnimJoint, int, ftAnim_GetNextMatAnimJointInTree)

int main(void)
{
    joints(); animations(); materials();
    printf("PASS %u actual animation-tree checks\n", checks);
    return 0;
}
