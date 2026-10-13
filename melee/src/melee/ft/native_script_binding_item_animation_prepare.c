#include "native_script_binding_item_animation.h"
#include "native_script_item_program.h"
#include <melee/it/types.h>
#include <stdio.h>

static bool error(NativeScriptDiagnostic* d, const char* message)
{
    if (d) snprintf(d->message, sizeof(d->message), "%s", message);
    return false;
}

bool nativeScriptBindingItemAnimation_PrepareDescriptors(
    const NativeScriptCompileBank* bank, NativeScriptItemProgram* program,
    NativeScriptItemAnimationDescriptor* destination, uint32_t capacity,
    NativeScriptDiagnostic* d)
{
    if (!bank || !bank->valid || !program || program->bank != bank ||
        !program->article)
        return error(d, "Animation descriptor preparation requires its owned Article");
    bool used = false;
    for (uint32_t f = 0; f < bank->code.function_count; ++f) {
        if (bank->declarations.functions[f].owner != program->owner) continue;
        const NativeScriptFunctionCode* fn = &bank->code.functions[f];
        for (uint32_t i = 0; i < fn->instruction_count; ++i) {
            uint32_t semantic = bank->code.instructions[fn->first_instruction + i].semantic;
            if (semantic == NativeScriptBinding_ItemSetAnimationDescriptor ||
                semantic == NativeScriptBinding_ItemAddAnimationDescriptor) used = true;
        }
    }
    if (!used) return true;
    const ItemStateArray* states = program->article->xC_itemStates;
    uint32_t count = states ? states->count : 0;
    if (count > capacity || count > NATIVE_SCRIPT_ENTITY_ANIMATION_DESCRIPTORS ||
        (count && (!destination || !states->x0_itemStateDesc)))
        return error(d, "Animation descriptor table or prepared capacity is invalid");
    const HSD_Joint* root = program->article->x10_modelDesc ?
        program->article->x10_modelDesc->x0_joint : NULL;
    for (uint32_t i = 0; i < count; ++i) {
        const ItemStateDesc* s = &states->x0_itemStateDesc[i];
        NativeScriptItemAnimationDescriptor record = { .descriptor = s,
            .anim = s->x0_anim_joint, .material = s->x4_matanim_joint,
            .shape = s->x8_parameters, .script = s->xC_script };
        if (root) {
            record.add_valid = nativeArticleAnimation_Census(root, record.anim,
                record.material, record.shape, &record.add) && !record.add.uncovered;
            record.set_valid = true;
            const HSD_AnimJoint* aj = record.anim;
            const HSD_MatAnimJoint* mj = record.material;
            const HSD_ShapeAnimJoint* sj = record.shape;
            uint32_t siblings = 0;
            for (const HSD_Joint* joint = root; joint; joint = joint->next) {
                NativeArticleAnimationDemand demand;
                if (++siblings > 512 || !nativeArticleAnimation_Census(joint, aj, mj, sj, &demand) ||
                    demand.uncovered || UINT32_MAX - record.set.controllers < demand.controllers ||
                    UINT32_MAX - record.set.tracks < demand.tracks) {
                    record.set_valid = false; break;
                }
                record.set.controllers += demand.controllers;
                record.set.tracks += demand.tracks;
                if (aj) aj = aj->next;
                if (mj) mj = mj->next;
                if (sj) sj = sj->next;
            }
        }
        destination[i] = record;
    }
    program->animation_states = states;
    program->animation_descriptors = count ? destination : NULL;
    program->animation_descriptor_count = count;
    program->animation_descriptors_prepared = true;
    return true;
}
