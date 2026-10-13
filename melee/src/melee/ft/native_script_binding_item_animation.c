#include "native_script_binding_item_animation.h"
#include "native_script_binding_item_model_resolve.h"
#include "native_script_item_program.h"
#include <melee/it/native_article_animation_live.h>
#include <melee/it/native_article_model_scene.h>
#include <melee/it/item.h>
#include <limits.h>
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/native_animation_pool.h>
#include <math.h>
#include <stdio.h>

typedef struct AnimationSource {
    NativeItemLifetime item;
    NativeArticleModelSceneSlot secondary;
    const NativeScriptItemModelSelector* selector;
    const NativeScriptItemModelAnimationTarget* target;
    const HSD_Joint* graph_descriptor;
    HSD_JObj* graph;
    HSD_JObj* joint;
} AnimationSource;

static bool error(NativeScriptDiagnostic* d, const char* message)
{
    if (d) snprintf(d->message, sizeof(d->message), "%s", message);
    return false;
}

static bool descriptor_owner(const NativeItemLifetime* source)
{
    return nativeItemLifetime_Current(source) &&
        nativeScriptItem_Status(source->object) == NativeScriptVM_Ok &&
        source->program->owner == source->data->native_script_owner &&
        source->program->author_kind == source->data->native_article_author_kind &&
        source->program->kind == source->data->kind &&
        source->program->article == source->data->xC4_article_data &&
        nativeArticleModelScene_ResolveOwner(source, NULL);
}

static bool descriptor_call(NativeScriptRuntimeContext* context, uint32_t semantic,
    NativeScriptValue* args, uint32_t count, NativeScriptValue* output,
    NativeScriptDiagnostic* d)
{
    NativeItemLifetime source;
    if (!output || !args || count != 1 || args[0].type != NativeScriptType_I32 ||
        args[0].as.i64 < INT32_MIN || args[0].as.i64 > INT32_MAX ||
        !context || context->source_class != NativeCallbackSource_Item ||
        !nativeItemLifetime_Capture(context->source, &source) || !source.source_owned ||
        source.generation != context->generation || source.reference.lifetime != context->scene_epoch ||
        !descriptor_owner(&source))
        return error(d, "Animation descriptor requires its live owned Item and exact Int index");
    const NativeScriptItemProgram* program = source.program;
    const ItemStateArray* states = program->article->xC_itemStates;
    if (!program->animation_descriptors_prepared || states != program->animation_states ||
        program->animation_descriptor_count > NATIVE_SCRIPT_ENTITY_ANIMATION_DESCRIPTORS ||
        (states && states->count != program->animation_descriptor_count) ||
        (program->animation_descriptor_count && !program->animation_descriptors))
        return error(d, "Animation descriptor table differs from its prepared owner");
    uint32_t index = (uint32_t)(int32_t)args[0].as.i64;
    bool set = semantic == NativeScriptBinding_ItemSetAnimationDescriptor;
    HSD_JObj* root = (HSD_JObj*)source.model;
    if (!states || index >= program->animation_descriptor_count || (!set && !root)) {
        *output = (NativeScriptValue){ .type = NativeScriptType_Void }; return true;
    }
    const NativeScriptItemAnimationDescriptor* prepared = &program->animation_descriptors[index];
    if (!states->x0_itemStateDesc || prepared->descriptor != &states->x0_itemStateDesc[index])
        return error(d, "Animation descriptor address differs from its prepared state table");
    ItemStateDesc* selected = &states->x0_itemStateDesc[index];
    if (selected->x0_anim_joint != prepared->anim || selected->x4_matanim_joint != prepared->material ||
        selected->x8_parameters != prepared->shape || selected->xC_script != prepared->script)
        return error(d, "Animation descriptor triplet differs from its pinned preparation");
    const HSD_Joint* graph = program->article->x10_modelDesc ?
        program->article->x10_modelDesc->x0_joint : NULL;
    if (root && (source.object_kind != HSD_GObj_JObjKind || !graph))
        return error(d, "Animation descriptor requires its primary joint graph");
    if (root && !(set ? prepared->set_valid : prepared->add_valid)) {
        const NativeArticleAnimationDemand* demand = set ? &prepared->set : &prepared->add;
        if (d) snprintf(d->message, sizeof(d->message),
            "Animation descriptor %u %s is not admitted (uncovered=0x%x)",
            index, set ? "set" : "add", demand->uncovered);
        return false;
    }
    if (root && !nativeArticleAnimation_Live(graph, root, program,
        program->animation_origins, program->animation_origin_count,
            nativeScriptItem_AttachmentOwner(source.object)))
        return error(d, "Animation descriptor live graph/controller provenance is not admitted");
    // Set's native shadow-copy and dynamic-bone loop read these exact fields.
    // Skeleton pointers are compared only, never dereferenced by this kernel.
    if (set && ((source.data->xC8_joint && source.data->xC8_joint != graph) ||
        source.data->x374_dynamicBonesNum > (int32_t)(sizeof(source.data->xD4_dynamicBones) /
                                                   sizeof(source.data->xD4_dynamicBones[0]))))
        return error(d, "Animation descriptor shadow or dynamic-bone extent is invalid");
    NativeArticleAnimationDemand demand = set ? prepared->set : prepared->add;
    if (root && !nativeArticleAnimation_CanAllocate(demand.controllers, demand.tracks))
        return error(d, "Animation descriptor controller pools lack admitted capacity");
    source.data->xD0_itemStateDesc = selected;
    if (set) Item_80268D34(source.object, selected);
    else HSD_JObjAddAnimAll(root, selected->x0_anim_joint, selected->x4_matanim_joint,
                          selected->x8_parameters);
    if (!descriptor_owner(&source) || source.data->xD0_itemStateDesc != selected ||
        program->article->xC_itemStates != states ||
        (root && !nativeArticleAnimation_Live(graph, root, program,
            program->animation_origins, program->animation_origin_count,
            nativeScriptItem_AttachmentOwner(source.object))))
        return error(d, "Animation descriptor owner or controller provenance changed before completion");
    *output = (NativeScriptValue){ .type = NativeScriptType_Void }; return true;
}

static bool current(const AnimationSource* source)
{
    return nativeItemLifetime_Current(&source->item) &&
        nativeScriptItem_Status(source->item.object) == NativeScriptVM_Ok &&
        source->item.program->owner == source->item.data->native_script_owner &&
        source->item.program->author_kind == source->item.data->native_article_author_kind &&
        source->item.program->kind == source->item.data->kind &&
        source->item.program->article == source->item.data->xC4_article_data &&
        nativeArticleModelScene_ResolveOwner(&source->item, NULL) &&
        (!source->selector->model ||
         nativeArticleModelScene_SlotCurrent(&source->secondary));
}

static bool capture(NativeScriptRuntimeContext* context, uint32_t semantic,
    const NativeScriptValue* args, uint32_t count, bool path,
    AnimationSource* source, NativeScriptDiagnostic* d)
{
    if (!context || context->source_class != NativeCallbackSource_Item || !args ||
        !count || !nativeItemLifetime_Capture(context->source, &source->item) ||
        !source->item.source_owned || source->item.generation != context->generation ||
        source->item.reference.lifetime != context->scene_epoch ||
        source->item.object_kind != HSD_GObj_JObjKind || !source->item.bank->valid)
        return error(d, "Model animation requires its live owned Item source");
    const NativeScriptItemProgram* program = source->item.program;
    if (program->model_selector_count > NATIVE_SCRIPT_ITEM_MODEL_SELECTORS ||
        (program->model_selector_count && !program->model_selectors))
        return error(d, "Model animation selector table is invalid");
    uint32_t low = 0, high = program->model_selector_count;
    while (low < high) {
        uint32_t middle = low + (high - low) / 2;
        if (program->model_selectors[middle].semantic < semantic) low = middle + 1;
        else high = middle;
    }
    if (low == program->model_selector_count ||
        program->model_selectors[low].semantic != semantic)
        return error(d, "Model animation selector is not prepared for this owner");
    source->selector = &program->model_selectors[low];
    const NativeScriptItemModelSelector* selector = source->selector;
    if (selector->argument_count != count || selector->model > 3 ||
        selector->depth > sizeof(selector->path) || args[0].type != NativeScriptType_I32 ||
        args[0].as.i64 != selector->model ||
        (path && (count < 2 || args[1].type != NativeScriptType_Text ||
         args[1].as.text.offset != selector->path_offset ||
         args[1].as.text.length != selector->path_length)) ||
        (!path && (selector->depth || selector->path_length)))
        return error(d, "Model animation arguments differ from the immutable selector");
    uint32_t target = 0;
    if (selector->model) {
        if (!nativeArticleModelScene_CaptureSlot(context->source, selector->model,
                                                &source->secondary))
            return error(d, "Model animation secondary slot is not owned by this scene");
        source->graph = source->secondary.root;
        source->graph_descriptor = source->secondary.descriptor;
        if (!source->graph) {
            source->target = NULL;
            source->joint = NULL;
            return current(source) || error(d, "Vacant animation slot retired");
        }
        if (source->secondary.root_index >= program->model_root_count ||
            source->secondary.root_index >= 8)
            return error(d, "Model animation root ordinal is invalid");
        target = source->secondary.root_index + 1;
    } else {
        if (!program->article || !program->article->x10_modelDesc)
            return error(d, "Primary animation model descriptor is absent");
        source->graph = (HSD_JObj*)source->item.model;
        source->graph_descriptor = program->article->x10_modelDesc->x0_joint;
    }
    source->target = &selector->animation_targets[target];
    if (!source->graph || !source->graph_descriptor || !source->target->valid ||
        source->target->uncovered ||
        !nativeArticleAnimation_Live(source->graph_descriptor, source->graph, program,
            program->animation_origins, program->animation_origin_count,
            nativeScriptItem_AttachmentOwner(source->item.object)))
        return error(d, "Model animation graph or retained controllers are not admitted");
    source->joint = source->graph;
    const HSD_Joint* descriptor = source->graph_descriptor;
    for (uint32_t step = 0; step < selector->depth; ++step) {
        if (!descriptor) { source->joint = NULL; break; }
        descriptor = descriptor->child;
        source->joint = source->joint ? source->joint->child : NULL;
        for (uint32_t sibling = 0; sibling < selector->path[step]; ++sibling) {
            descriptor = descriptor ? descriptor->next : NULL;
            source->joint = source->joint ? source->joint->next : NULL;
        }
    }
    if (descriptor != source->target->descriptor ||
        (!descriptor && source->joint) || (descriptor && !source->joint) || !current(source))
        return error(d, "Model animation target differs from its prepared topology");
    return true;
}

bool nativeScriptBindingItemAnimation_Invoke(NativeScriptRuntimeContext* context,
    uint32_t semantic, NativeScriptValue* args, uint32_t count,
    NativeScriptValue* output, NativeScriptDiagnostic* d)
{
    if (semantic == NativeScriptBinding_ItemSetAnimationDescriptor ||
        semantic == NativeScriptBinding_ItemAddAnimationDescriptor)
        return descriptor_call(context, semantic, args, count, output, d);
    uint32_t domain = semantic & NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK;
    bool add = domain == NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_ADD;
    bool request = domain == NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_REQUEST;
    bool advance = domain == NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_ADVANCE;
    bool remove = domain == NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_REMOVE;
    bool rate = domain == NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_RATE;
    bool value = add || request || rate;
    if (!output || (!value && !advance && !remove) || !args ||
        (value ? count != 2 && count != 3 : count != 1 && count != 2))
        return error(d, "Model animation operation arity is invalid");
    bool path = count == (value ? 3u : 2u);
    uint32_t last = count - 1;
    if (add) {
        if (args[last].type != NativeScriptType_I32 || args[last].as.i64 < 0)
            return error(d, "Model animation requires a prepared bundle ordinal");
    } else if (value && (args[last].type != NativeScriptType_F32 ||
                        !isfinite(args[last].as.f32))) {
        return error(d, "Model animation frame/rate must be finite");
    }
    AnimationSource source = { 0 };
    if (!capture(context, semantic, args, count, path, &source, d)) return false;
    const NativeScriptItemProgram* program = source.item.program;
    const NativeScriptItemModelAnimationAsset* bundle = NULL;
    if (add) {
        uint32_t ordinal = source.selector->animation;
        if (!program->model_animations || program->model_animation_count > 8 ||
            ordinal >= program->model_animation_count || args[last].as.i64 != ordinal)
            return error(d, "Model animation bundle is not the exact same-owner declaration");
        bundle = &program->model_animations[ordinal];
    }
    bool attached = false;
    if (source.joint) {
        if (advance && nativeAnimationGlobals.end_callbacks)
            return error(d, "Model animation end callbacks require explicit lifetime support");
        if (add) {
            if (nativeArticleAnimation_CanAllocate(source.target->controllers,
                                                   source.target->tracks)) {
                HSD_JObjAddAnimAll(source.joint, (HSD_AnimJoint*)bundle->anim,
                    (HSD_MatAnimJoint*)bundle->matanim,
                    (HSD_ShapeAnimJoint*)bundle->shapeanim);
                attached = true;
            }
        } else if (request) HSD_JObjReqAnimAll(source.joint, args[last].as.f32);
        else if (advance) HSD_JObjAnimAll(source.joint);
        else if (remove) HSD_JObjRemoveAnimAll(source.joint);
        else lb_8000BA0C(source.joint, args[last].as.f32);
    }
    if (!current(&source) || (source.graph &&
        !nativeArticleAnimation_Live(source.graph_descriptor, source.graph, program,
            program->animation_origins, program->animation_origin_count,
            nativeScriptItem_AttachmentOwner(source.item.object))))
        return error(d, "Model animation owner or controller provenance changed before completion");
    *output = add ? (NativeScriptValue){ .type = NativeScriptType_Bool, .as.u64 = attached } :
                   (NativeScriptValue){ .type = NativeScriptType_Void };
    return true;
}
