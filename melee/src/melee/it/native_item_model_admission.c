#include "native_item_model_admission.h"
#include "types.h"
#include <sysdolphin/baselib/dat_import_internal.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
enum { ITEM_BONE_CAPACITY=sizeof(((DynamicBoneTable*)0)->bones)/sizeof(((DynamicBoneTable*)0)->bones[0]) };
static bool fail(HSD_DatImport* import,HSD_DatResult result,u32 offset)
{import->status=result;import->error_offset=offset;return false;}
typedef struct ModelNode {
    const HSD_Joint* descriptor;
    u32 parent, child, next, completed;
} ModelNode;

typedef struct ModelFrame { u32 node, phase; } ModelFrame;

static u32 joint_offset(const HSD_DatImport* import, const HSD_Joint* joint,
                        u32 fallback)
{
    for (u32 i = 0; i < import->record_count; ++i)
        if (import->records[i].type == DAT_JOINT &&
            import->records[i].object == joint)
            return import->records[i].offset;
    return fallback;
}

// Simulates JObjLoad's child/next/parent links and postorder ID insertion,
// followed by JObjResolveRefsAll's instance links. No HSD objects are created.
bool nativeItemModel_Admit(HSD_DatImport* import, const ItemModelDesc* model,
                             u32 offset, u32* output)
{
    if (model == NULL) return fail(import, HSD_DAT_DESCRIPTOR, offset);
    if (model->x0_joint == NULL) { *output = 1; return true; } // native anchor
    ModelNode* nodes = calloc(NATIVE_ITEM_MODEL_NODE_LIMIT, sizeof(*nodes));
    ModelFrame* frames = malloc(NATIVE_ITEM_MODEL_NODE_LIMIT * sizeof(*frames));
    if (nodes == NULL || frames == NULL) {
        free(nodes); free(frames);
        return fail(import, HSD_DAT_CAPACITY, offset);
    }
    bool okay = false;
    u32 count = 1, depth = 1, completed = 0;
    nodes[0] = (ModelNode) { model->x0_joint, UINT32_MAX, UINT32_MAX,
                           UINT32_MAX, 0 };
    frames[0] = (ModelFrame) { 0, 0 };
    while (depth != 0) {
        ModelFrame* frame = &frames[depth - 1];
        ModelNode* node = &nodes[frame->node];
        const HSD_Joint* descriptor = node->descriptor;
        u32 here = joint_offset(import, descriptor, offset);
        if (frame->phase == 0) {
            // Only the default registered load method is modeled. Unknown
            // named classes must not silently inherit this topology proof.
            if (descriptor->class_name != NULL &&
                strcmp(descriptor->class_name, "hsd_jobj") != 0) {
                fail(import, HSD_DAT_SYMBOL, here);
                goto done;
            }
            for (u32 i = 0; i + 1 < depth; ++i) {
                if (nodes[frames[i].node].descriptor == descriptor) {
                    fail(import, HSD_DAT_DESCRIPTOR, here);
                    goto done;
                }
            }
        }
        const HSD_Joint* link = NULL;
        u32 parent = UINT32_MAX;
        bool child = false;
        if (frame->phase == 0) {
            frame->phase = 1;
            if (!(descriptor->flags & JOBJ_INSTANCE)) {
                link = descriptor->child;
                parent = frame->node;
                child = true;
            }
        } else if (frame->phase == 1) {
            frame->phase = 2;
            link = descriptor->next;
            parent = node->parent;
        } else {
            node->completed = ++completed;
            --depth;
            continue;
        }
        if (link == NULL) continue;
        if (count == NATIVE_ITEM_MODEL_NODE_LIMIT || depth == NATIVE_ITEM_MODEL_NODE_LIMIT) {
            fail(import, HSD_DAT_CAPACITY, here);
            goto done;
        }
        u32 added = count++;
        nodes[added] = (ModelNode) { link, parent, UINT32_MAX, UINT32_MAX, 0 };
        if (child) node->child = added;
        else node->next = added;
        frames[depth++] = (ModelFrame) { added, 0 };
    }
    for (u32 i = 0; i < count; ++i) {
        if (!(nodes[i].descriptor->flags & JOBJ_INSTANCE)) continue;
        u32 best = UINT32_MAX, last = 0;
        for (u32 j = 0; j < count; ++j) {
            if (nodes[j].descriptor == nodes[i].descriptor->child &&
                nodes[j].completed > last) {
                best = j;
                last = nodes[j].completed;
            }
        }
        if (best == UINT32_MAX) {
            fail(import, HSD_DAT_DESCRIPTOR,
                 joint_offset(import, nodes[i].descriptor, offset));
            goto done;
        }
        nodes[i].child = best;
    }
    u32 at = 0, visited = 0;
    while (at != UINT32_MAX) {
        if (++visited > ITEM_BONE_CAPACITY) {
            fail(import, HSD_DAT_CAPACITY,
                 joint_offset(import, nodes[at].descriptor, offset));
            goto done;
        }
        if (nodes[at].child != UINT32_MAX) at = nodes[at].child;
        else if (nodes[at].next != UINT32_MAX) at = nodes[at].next;
        else {
            while (true) {
                u32 parent = nodes[at].parent;
                if (parent == UINT32_MAX) { at = UINT32_MAX; break; }
                if (nodes[parent].next != UINT32_MAX) {
                    at = nodes[parent].next;
                    break;
                }
                at = parent;
            }
        }
    }
    *output = visited;
    okay = true;
done:
    free(frames); free(nodes);
    return okay;
}
