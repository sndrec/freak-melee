#include "native_source_rig_validation.h"
#include "native_fighter_model_graph.h"

#include "kinds/ftCommon/ftCo_09F7.h"
#include "native_hitbox_lifetime.h"
#include "native_script_item_program.h"
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/jobj.h>

#include <stdio.h>
#include <string.h>

typedef struct CostumeRig {
    unsigned count;
    bool omitted[MAX_FT_PARTS];
} CostumeRig;

static bool check_joints(const CostumeRig* rig, const int* ids,
                         unsigned count, const char* use,
                         NativeScriptDiagnostic* diagnostic)
{
    for (unsigned i = 0; i < count; ++i) {
        if ((unsigned) ids[i] < rig->count && !rig->omitted[ids[i]]) continue;
        if (diagnostic)
            snprintf(diagnostic->message, sizeof(diagnostic->message),
                     "%s needs joint %d on the fighter's costume skeleton.", use, ids[i]);
        return false;
    }
    return true;
}

static bool check_actions(NativeSourceImportedAssets* assets, const CostumeRig* costume,
                          NativeScriptDiagnostic* diagnostic)
{
    const FtCharacterAssets* fighter = &assets->character;
    const FighterPartsTable* rig = &assets->parts;
    const FtActionDataStats* table = &fighter->action_stats;
    memset(assets->external_hitbox_parts, 0, sizeof(assets->external_hitbox_parts));
    for (u32 word = 0; word < table->word_count;) {
        const union CmdUnion* cmd = &table->commands[word];
        unsigned opcode = cmd->Command_00.code;
        // The importer leaves gaps zeroed. An End command also takes one word.
        if (opcode == 0) {
            ++word;
            continue;
        }
        u32 offset = table->source_begin + word * 4;
        u32 raw;
        if (!HSD_DatReadU32(&fighter->dat, offset, &raw)) return false;
        u32 width = ftActionData_SourceCommandWidth(raw);
        if (width == 0 || width > table->word_count - word) return false;
        word += width;

        unsigned bone;
        bool common = false;
        if (nativeHitbox_IsCreate(opcode)) {
            bone = cmd->create_hitbox_0.bone;
            common = cmd->create_hitbox_0.use_common_bone_ids;
            if (!common && bone >= rig->parts_num && bone < MAX_FT_PARTS) {
                assets->external_hitbox_parts[bone] = true;
                continue;
            }
        } else if (opcode == 10) {
            bone = cmd->spawn_gfx_0.useUnkBone ? fighter->model_data.x12
                                               : cmd->spawn_gfx_0.boneId;
            // These selectors resolve to the model joint or one of its five
            // effect anchors before the common-bone flag is considered.
            if (bone == FT_EFFECT_ROTATING_JOINT || bone == FT_EFFECT_MODEL_JOINT)
                continue;
            common = cmd->spawn_gfx_0.useCommonBoneIDs;
        } else if (opcode == 58) {
            bone = cmd->wind_fx_0.bone;
        } else if (opcode == 40) {
            unsigned textures = fighter->model_data.x8.x8;
            if ((unsigned) cmd->set_tex_anim.idx < textures &&
                (!cmd->set_tex_anim.b || (unsigned) cmd->set_tex_anim.idx2 < textures))
                continue;
            if (diagnostic)
                snprintf(diagnostic->message, sizeof(diagnostic->message),
                         "Texture animation at 0x%X exceeds the fighter's %u texture slots.",
                         (unsigned) offset, textures);
            return false;
        } else if (opcode == 41) {
            unsigned group = cmd->part_anim.unk1, variant = cmd->part_anim.unk2;
            if (group < ARRAY_SIZE(fighter->part_animation_groups) &&
                fighter->part_animation_groups[group] &&
                variant < FT_CHARACTER_PART_ANIMATION_VARIANTS)
                continue;
            if (diagnostic)
                snprintf(diagnostic->message, sizeof(diagnostic->message),
                         "Action at 0x%X refers to missing part animation %u/%u.",
                         (unsigned) offset, group, variant);
            return false;
        } else {
            continue;
        }
        unsigned joint = common ? (bone < FT_COMMON_PART_COUNT ? rig->part_to_joint[bone] : MAX_FT_PARTS) : bone;
        if (joint < rig->parts_num && (!costume->omitted[joint] || nativeHitbox_IsCreate(opcode)))
            continue;
        if (diagnostic)
            snprintf(diagnostic->message, sizeof(diagnostic->message),
                     "Action command %u at 0x%X refers to missing %s %u.",
                     opcode, (unsigned) offset, common ? "bone role" : "joint", bone);
        return false;
    }
    return true;
}

bool nativeSourceAssets_ValidateExternalParts(const NativeSourceImportedAssets* assets,
    const NativeScriptItemPrograms* items, NativeScriptDiagnostic* diagnostic)
{
    for (unsigned part = assets->parts.parts_num; part < MAX_FT_PARTS; ++part) {
        if (!assets->external_hitbox_parts[part]) continue;
        unsigned i = 0;
        while (i < items->count && items->programs[i].resource_reserved_part != (int) part)
            ++i;
        if (i == items->count) {
            if (diagnostic)
                snprintf(diagnostic->message, sizeof(diagnostic->message),
                         "A hitbox uses joint %u outside the rig. An item must reserve that joint.", part);
            return false;
        }
    }
    return true;
}

static bool part_tree_fits(HSD_AnimJoint* node, unsigned joint, unsigned parts,
                           const bool omitted[MAX_FT_PARTS])
{
    HSD_AnimJoint* ancestors[30];
    unsigned depth = 0;
    while (node) {
        while (joint < parts && omitted[joint]) ++joint;
        if (joint >= parts) return false;
        ++joint;
        if (node->child) {
            if (depth + 1 >= ARRAY_SIZE(ancestors)) return false;
            ancestors[depth++] = node;
            node = node->child;
        } else {
            while (!node->next && depth) node = ancestors[--depth];
            node = node->next;
        }
    }
    return true;
}

static void bind_detachable_joints(NativeSourceImportedAssets* assets)
{
    memset(assets->detachable_joints, 0, sizeof(assets->detachable_joints));
    for (unsigned slot = 0; slot < FT_CHARACTER_ITEM_CAPACITY; ++slot) {
        HSD_Joint* graph = assets->character.joint_graphs[slot];
        if (!graph) continue;
        uint32_t checked[8] = {0};
        for (unsigned i = 0; i < assets->facts.omission_count; ++i) {
            unsigned ordinal = assets->omission_rows[i].x3;
            uint32_t bit = UINT32_C(1) << (ordinal % 32);
            if (checked[ordinal / 32] & bit) continue;
            checked[ordinal / 32] |= bit;
            const HSD_Joint* donor = nativeFighterModelGraph_JointAt(graph, ordinal);
            if (!donor || (donor->flags & (JOBJ_INSTANCE | JOBJ_PTCL | JOBJ_SPLINE))) continue;
            // The runtime registers a fresh copy of the descriptor. Its original
            // address and every other donor joint are outside this new model.
            HSD_Joint detached = *donor;
            detached.next = detached.child = NULL;
            if (ftCostumeAssets_ValidateModel(&detached, NULL, NULL, 124, NULL) == FT_MODEL_VALID)
                assets->detachable_joints[slot][ordinal / 32] |= bit;
        }
    }
}

bool nativeSourceAssets_ValidateRig(NativeSourceImportedAssets* assets,
                                    NativeScriptDiagnostic* diagnostic)
{
    const FtCharacterAssets* fighter = &assets->character;
    CostumeRig costume = { .count = assets->parts.parts_num };
    const CostumeRig* rig = &costume;
    for (unsigned i = 0; i < assets->facts.omission_count; ++i)
        costume.omitted[assets->facts.omissions[i].joint] = true;
    const int root = 0;
    if (!check_joints(rig, &root, 1, "Fighter root", diagnostic)) return false;
    for (unsigned i = 0; i < assets->facts.bone_count; ++i) {
        const int joint = assets->facts.bones[i].joint;
        if (!check_joints(rig, &joint, 1, "Common bone role", diagnostic)) return false;
    }
    const struct ftData_x8* model = &fighter->model_data;
    const int anchors[] = { model->x10, model->x11, model->x12 };
    const int camera = fighter->common_attributes.camera_zoom_target_bone;
    const int thrown = fighter->scale.x0;
    const ftData_x44_t* ecb = &fighter->collision;
    const int collision[] = { ecb->unk0, ecb->unk2, ecb->unk4,
                              ecb->unk6, ecb->unk8, ecb->unkA };
    if (!check_joints(rig, anchors, ARRAY_SIZE(anchors), "Model attachment", diagnostic) ||
        !check_joints(rig, &camera, 1, "Camera", diagnostic) ||
        !check_joints(rig, &thrown, 1, "Thrown hitbox", diagnostic) ||
        !check_joints(rig, collision, ARRAY_SIZE(collision), "Stage collision", diagnostic) ||
        !check_joints(rig, fighter->x54, ARRAY_SIZE(fighter->x54), "Rotating effect", diagnostic))
        return false;
    const int feet[] = { model->x13, model->x14 };
    for (unsigned i = 0; i < ARRAY_SIZE(feet); ++i) {
        if (feet[i] != FT_EFFECT_ROTATING_JOINT && feet[i] != FT_EFFECT_MODEL_JOINT &&
            !check_joints(rig, &feet[i], 1, "Footstep effect", diagnostic))
            return false;
    }
    for (int i = 0; i < fighter->hurtboxes.count; ++i) {
        int bone = fighter->hurtboxes.inits[i].bone_idx;
        if (!check_joints(rig, &bone, 1, "Hurtbox", diagnostic)) return false;
    }
    for (unsigned i = 0; i < ARRAY_SIZE(fighter->collision_bubbles); ++i) {
        if (!check_joints(rig, &fighter->collision_bubbles[i].x0, 1,
                          "Collision bubble", diagnostic)) return false;
    }
    for (int i = 0; i < fighter->dynamics.x4; ++i) {
        if (!check_joints(rig, &fighter->dynamics.x8[i].x0, 1,
                          "Dynamics collider", diagnostic)) return false;
    }
    for (int i = 0; i < fighter->dynamics.dynamicsNum; ++i) {
        const BoneDynamicsDesc* chain = &fighter->dynamic_bones.array[i];
        int start = chain->bone_id;
        if (!check_joints(rig, &start, 1, "Dynamics chain", diagnostic)) return false;
        if (chain->dyn_desc.count > rig->count - (unsigned) start) {
            if (diagnostic)
                snprintf(diagnostic->message, sizeof(diagnostic->message),
                         "Dynamics chain %d extends past the fighter's last joint.", i);
            return false;
        }
        for (unsigned j = 1; j < chain->dyn_desc.count; ++j) {
            const int bone = start + (int) j;
            if (!check_joints(rig, &bone, 1, "Dynamics chain", diagnostic)) return false;
        }
    }
    for (unsigned i = 0; i < ARRAY_SIZE(fighter->part_animation_groups); ++i) {
        const struct ftData_x1C* group = fighter->part_animation_groups[i];
        if (!group) continue;
        int root = group->x0;
        if (!check_joints(rig, &root, 1, "Part animation", diagnostic)) return false;
        for (unsigned j = 0; j < group->x2; ++j) {
            int bone = group->x4[j];
            if (!check_joints(rig, &bone, 1, "Part animation", diagnostic)) return false;
        }
        for (unsigned variant = 0; variant < FT_CHARACTER_PART_ANIMATION_VARIANTS; ++variant) {
            if (part_tree_fits(group->x8[variant], group->x0, rig->count, rig->omitted))
                continue;
            if (diagnostic)
                snprintf(diagnostic->message, sizeof(diagnostic->message),
                         "Part animation %u/%u exceeds the rig or animation tree depth.", i, variant);
            return false;
        }
    }
    if (fighter->data.x58) {
        const struct ftData_x58_t* ik = fighter->data.x58;
        int bones[] = { ik->x0, ik->x1, ik->x8, ik->x9, ik->x10, ik->x11 };
        if (!check_joints(rig, bones, ARRAY_SIZE(bones), "Inverse kinematics", diagnostic))
            return false;
    }
    if (!check_actions(assets, rig, diagnostic)) return false;
    bind_detachable_joints(assets);
    return true;
}
