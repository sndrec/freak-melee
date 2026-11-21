#include "native_fighter_model_graph.h"
#include "native_fighter_runtime.h"
#include "native_source_package.h"
#include "fighter.h"
#include "ftparts.h"
#include "ftcharacter_assets.h"
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>

// Same preorder as ftAnim_GetNextJointInTree, with a local bounded ancestor
// stack instead of the animation iterator's shared scratch storage.
HSD_Joint* nativeFighterModelGraph_JointAt(HSD_Joint* joint, unsigned ordinal)
{
    if (ordinal == 255) return joint;
    HSD_Joint* ancestors[MAX_FT_PARTS];
    unsigned depth = 0;
    for (unsigned i = 0; i < ordinal && joint != NULL; ++i) {
        if (joint->child != NULL) {
            if (depth == MAX_FT_PARTS) return NULL;
            ancestors[depth++] = joint;
            joint = joint->child;
        } else if (joint->next != NULL) {
            joint = joint->next;
        } else {
            joint = NULL;
            while (depth != 0 && joint == NULL) joint = ancestors[--depth]->next;
        }
    }
    return joint;
}

bool nativeFighterModelGraph_Install(Fighter_GObj* gobj, int graph_slot, int omission)
{
    Fighter* fighter = gobj != NULL ? GET_FIGHTER(gobj) : NULL;
    if (fighter == NULL || (unsigned)fighter->kind >= Ft_Kind_RuntimeCapacity ||
        (unsigned)graph_slot >= FT_CHARACTER_ITEM_CAPACITY || ftPartsTable == NULL ||
        Fighter_804D6540 == NULL || fighter->ft_data == NULL ||
        fighter->ft_data->x48_items == NULL || fighter->parts == NULL) return false;
    const NativeSourcePackage* package=nativeFighterRuntime_SourcePackage(fighter->kind);
    const NativeFighterDefinition* definition = package?NULL:nativeFighterRuntime_Definition(fighter->kind);
    FighterPartsTable* table = ftPartsTable[fighter->kind];
    struct Fighter_804D6540_t* omissions = Fighter_804D6540[fighter->kind];
    uint32_t mask=package?package->assets.facts.layout.joint_graph_mask:definition?definition->asset_layout.joint_graph_mask:0;
    if ((package&&(!package->assets.ready || fighter->ft_data!=&package->assets.character.data ||
            table!=&package->assets.parts || omissions!=&package->assets.omissions ||
            table->parts_num!=package->assets.facts.joints+package->assets.facts.omission_count)) ||
        !(mask & (1u << graph_slot)) ||
        table == NULL || table->parts_num > MAX_FT_PARTS || omissions == NULL ||
        omissions->x4 < 0 || omissions->x4 > 32 || omissions->x0 == NULL ||
        (unsigned)omission >= (unsigned)omissions->x4) return false;
    const struct Fighter_804D6540_x0_t* placement = &omissions->x0[omission];
    if (placement->x0 >= table->parts_num || placement->x1 >= table->parts_num ||
        placement->x0 == placement->x1 || placement->x2 > 3) return false;
    FighterBone* target = &fighter->parts[placement->x0];
    FighterBone* parent = &fighter->parts[placement->x1];
    if (target->joint != NULL || target->x4_jobj2 != NULL || target->flags_b1 || target->flags_b2 ||
        parent->joint == NULL || parent->x4_jobj2 == NULL ||
        (placement->x2 >= 2 && (parent->joint->parent == NULL || parent->x4_jobj2->parent == NULL))) return false;
    unsigned ordinal = placement->x3;
    if (package && !(package->assets.detachable_joints[graph_slot][ordinal / 32] &
                     (UINT32_C(1) << (ordinal % 32)))) return false;
    HSD_Joint* descriptor = nativeFighterModelGraph_JointAt(fighter->ft_data->x48_items[graph_slot], ordinal);
    if (descriptor == NULL || (descriptor->flags & (JOBJ_PTCL | JOBJ_SPLINE))) return false;
    // The reserved part owns one descriptor, not its child or sibling trees.
    HSD_Joint source = *descriptor;
    source.child = source.next = NULL;
    unsigned mesh_count = 0;
    for (HSD_DObjDesc* mesh = source.u.dobjdesc; mesh != NULL; mesh = mesh->next)
        if (++mesh_count > 124) return false;
    if ((unsigned)fighter->dobj_list.count > 124 ||
        mesh_count > 124 - (unsigned)fighter->dobj_list.count ||
        (mesh_count != 0 && fighter->dobj_list.data == NULL)) return false;
    HSD_JObj* current = ftParts_8007482C(&source);
    if (current == NULL) return false;
    HSD_JObj* alternate = ftParts_8007482C(&source);
    if (alternate == NULL) { HSD_JObjRemove(current); return false; }
    ftParts_80075304(placement->x2, parent->joint, current);
    ftParts_80075304(placement->x2, parent->x4_jobj2, alternate);
    u32 depth = parent->xC + (placement->x2 <= 1);
    int mesh_index = fighter->dobj_list.count;
    ftParts_80074194(fighter, target, current, &mesh_index, depth);
    fighter->dobj_list.count = mesh_index;
    target->x4_jobj2 = alternate;
    target->flags_b2 = true;
    return true;
}
