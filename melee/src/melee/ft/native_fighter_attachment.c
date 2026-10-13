#include "native_fighter_attachment.h"

#ifdef MELEE_NATIVE
#include "fighter.h"
#include "ftcostume_assets.h"
#include "ftcharacter_assets.h"
#include "ftdata.h"
#include "ftparts.h"
#include "native_costume_runtime.h"
#include "types.h"
#include <melee/lb/lb_00F9.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <string.h>

static FtCostumeAssets* costume_assets(Fighter* fighter)
{
    const UnkCostumeStruct* costume = nativeCostumeRuntime_ForFighter(fighter);
    if (costume == NULL) {
        const struct UnkCostumeList* list =
            &CostumeListsForeachCharacter[fighter->kind];
        if (list->costume_list == NULL ||
            fighter->x619_costume_id >= list->numCostumes) return NULL;
        costume = &list->costume_list[fighter->x619_costume_id];
    }
    return costume->native_assets;
}

bool nativeFighterAttachment_HasRoot(Fighter_GObj* gobj, const char* root)
{
    FtCostumeAssets* assets = costume_assets(gobj->user_data);
    u32 offset;
    return assets != NULL && HSD_DatFindRoot(&assets->dat, root, &offset);
}

bool nativeFighterAttachment_Load(Fighter_GObj* gobj, unsigned slot,
                                  const char* root, unsigned bone,
                                  int visibility_entry, unsigned costume)
{
    Fighter* fighter = gobj->user_data;
    FtCostumeAssets* assets = costume_assets(fighter);
    u32 offset;
    if (slot >= NATIVE_FIGHTER_ATTACHMENTS || assets == NULL ||
        bone >= ftPartsTable[fighter->kind]->parts_num ||
        fighter->native_attachments[slot].joint != NULL ||
        !HSD_DatFindRoot(&assets->dat, root, &offset)) return false;
    FtPartsDesc* visibility = NULL;
    if (visibility_entry >= 0) {
        if (visibility_entry >= FT_CHARACTER_ITEM_CAPACITY || fighter->ft_data->x48_items == NULL)
            return false;
        visibility = fighter->ft_data->x48_items[visibility_entry];
        if (visibility == NULL || costume >= visibility->costume_count)
            return false;
    }
    HSD_Joint* descriptor = HSD_DatImportGraph(&assets->import, offset, DAT_JOINT);
    if (descriptor == NULL) return false;
    NativeFighterAttachment* attachment = &fighter->native_attachments[slot];
    ftPartsPObjSetDefaultClass();
    attachment->joint = HSD_JObjLoadJoint(descriptor);
    ftPartsPObjClearDefaultClass();
    if (attachment->joint == NULL) return false;
    attachment->objects.data = attachment->object_data;
    attachment->bone = bone;
    attachment->visible = true;
    attachment->has_visibility = visibility != NULL;
    ftParts_80075650(gobj, attachment->joint, &attachment->objects);
    while (attachment->objects.count < NATIVE_ATTACHMENT_DOBJS &&
           attachment->object_data[attachment->objects.count] != NULL)
        ++attachment->objects.count;
    if (visibility != NULL)
        ftParts_8007487C(visibility, &attachment->visibility, costume,
                         &attachment->objects, &attachment->objects);
    fighter->x2225_b2 = true;
    return true;
}

static HSD_JObj* joint_at(HSD_JObj* joint, unsigned index)
{
    while (joint != NULL && index-- != 0) {
        if (joint->child != NULL) joint = joint->child;
        else {
            while (joint != NULL && joint->next == NULL) joint = joint->parent;
            if (joint != NULL) joint = joint->next;
        }
    }
    return joint;
}

bool nativeFighterAttachment_AddDynamics(Fighter_GObj* gobj, unsigned slot,
                                         unsigned first, unsigned count)
{
    Fighter* fighter = gobj->user_data;
    ftDynamics* dynamics = fighter->ft_data->x2C;
    if (slot >= NATIVE_FIGHTER_ATTACHMENTS || dynamics == NULL || count == 0 ||
        dynamics->ftDynamicBones == NULL ||
        dynamics->descriptor_count > Ft_Dynamics_NumMax ||
        first >= dynamics->descriptor_count ||
        count > dynamics->descriptor_count - first ||
        fighter->dynamics_num + count >= Ft_Dynamics_NumMax) return false;
    NativeFighterAttachment* attachment = &fighter->native_attachments[slot];
    if (attachment->joint == NULL || attachment->dynamic_mask != 0) return false;
    HSD_JObj* joints[Ft_Dynamics_NumMax];
    for (unsigned i = 0; i < count; ++i) {
        BoneDynamicsDesc* descriptor =
            &dynamics->ftDynamicBones->array[first + i];
        joints[i] = joint_at(attachment->joint, descriptor->bone_id);
        if (joints[i] == NULL || descriptor->dyn_desc.count == 0) return false;
    }
    for (unsigned i = 0; i < count; ++i) {
        unsigned destination = fighter->dynamics_num++;
        BoneDynamicsDesc* descriptor =
            &dynamics->ftDynamicBones->array[first + i];
        BoneDynamicsDesc* live = &fighter->dynamic_bone_sets[destination];
        lb_8000FD48(joints[i], &live->dyn_desc, descriptor->dyn_desc.count);
        live->bone_id = FtPart_TopN;
        lb_80011710(&descriptor->dyn_desc, &live->dyn_desc);
        attachment->dynamic_mask |= 1u << destination;
        fighter->native_attachment_dynamic_mask |= 1u << destination;
    }
    return true;
}

void nativeFighterAttachment_Clear(Fighter_GObj* gobj)
{
    Fighter* fighter = gobj->user_data;
    unsigned remaining = 0;
    for (unsigned i = 0; i < fighter->dynamics_num; ++i) {
        if (fighter->native_attachment_dynamic_mask & (1u << i))
            lb_8000FD18(&fighter->dynamic_bone_sets[i].dyn_desc);
        else fighter->dynamic_bone_sets[remaining++] = fighter->dynamic_bone_sets[i];
    }
    fighter->dynamics_num = remaining;
    fighter->native_attachment_dynamic_mask = 0;
    for (unsigned slot = 0; slot < NATIVE_FIGHTER_ATTACHMENTS; ++slot) {
        NativeFighterAttachment* attachment = &fighter->native_attachments[slot];
        if (attachment->joint != NULL) HSD_JObjRemoveAll(attachment->joint);
        memset(attachment, 0, sizeof(*attachment));
    }
}

void nativeFighterAttachment_Draw(Fighter_GObj* gobj, int pass, Mtx view)
{
    Fighter* fighter = gobj->user_data;
    if (!fighter->x2225_b2) return;
    for (unsigned slot = 0; slot < NATIVE_FIGHTER_ATTACHMENTS; ++slot) {
        NativeFighterAttachment* attachment = &fighter->native_attachments[slot];
        if (attachment->joint == NULL || !attachment->visible) continue;
        HSD_JObjCopyMtx(attachment->joint,
                       HSD_JObjGetMtxPtr(fighter->parts[attachment->bone].joint));
        attachment->joint->flags |=
            JOBJ_USER_DEF_MTX | JOBJ_MTX_INDEP_PARENT | JOBJ_MTX_INDEP_SRT;
        HSD_JObjSetMtxDirty(attachment->joint);
        HSD_JObjDispAll(attachment->joint, view, HSD_GObj_80390EB8(pass), 0);
    }
}

void nativeFighterAttachment_Visibility(Fighter* fighter, int group, bool enabled)
{
    for (unsigned slot = 0; slot < NATIVE_FIGHTER_ATTACHMENTS; ++slot) {
        NativeFighterAttachment* attachment = &fighter->native_attachments[slot];
        if (attachment->joint == NULL || !attachment->has_visibility) continue;
        if (enabled)
            ftParts_80074CA0(&attachment->visibility, group, &attachment->objects);
        else ftParts_80074D7C(&attachment->visibility, group, &attachment->objects);
    }
}

#endif
