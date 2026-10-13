#include "native_article_animation_live.h"
#include "native_article_animation_demand.h"
#include "native_article_animation_texture.h"
#include "native_script_item_actor.h"
#include "types.h"
#include <melee/ft/native_script_item_program.h>
#include <melee/ft/native_script_binding_item_model_resolve.h>
#include <sysdolphin/baselib/dobj.h>
#include <string.h>

enum { GRAPH_LIMIT = 512, STATE_LIMIT = 2048 };

typedef struct OriginWriter {
    NativeArticleAnimationOrigin* data;
    const HSD_Joint* graph;
    uint32_t capacity, count;
} OriginWriter;

static int order(const NativeArticleAnimationOrigin* a,
                 const NativeArticleAnimationOrigin* b)
{
    uintptr_t x = (uintptr_t)a->target, y = (uintptr_t)b->target;
    if (x != y) return x < y ? -1 : 1;
    if (a->kind != b->kind) return a->kind < b->kind ? -1 : 1;
    x = (uintptr_t)a->controller; y = (uintptr_t)b->controller;
    if (x != y) return x < y ? -1 : 1;
    x = (uintptr_t)a->texture; y = (uintptr_t)b->texture;
    return x == y ? 0 : x < y ? -1 : 1;
}

static bool append(OriginWriter* writer, const void* target,
                   const HSD_AObjDesc* controller, uint32_t kind, const HSD_TexAnim* texture)
{
    if (!controller && !texture) return true;
    NativeArticleAnimationOrigin value = { writer->graph, target, controller, kind, texture };
    uint32_t low = 0, high = writer->count;
    while (low < high) {
        uint32_t mid = low + (high - low) / 2;
        if (order(&writer->data[mid], &value) < 0) low = mid + 1;
        else high = mid;
    }
    if (low < writer->count && !order(&writer->data[low], &value)) return true;
    if (writer->count == writer->capacity) return false;
    memmove(writer->data + low + 1, writer->data + low,
            (writer->count - low) * sizeof(value));
    writer->data[low] = value; ++writer->count;
    return true;
}

typedef struct AnimationZip {
    const HSD_Joint* joint;
    const HSD_AnimJoint* animation;
    const HSD_MatAnimJoint* material;
} AnimationZip;

static bool apply_origins(OriginWriter* writer, const HSD_Joint* root,
    const HSD_AnimJoint* animation, const HSD_MatAnimJoint* material)
{
    AnimationZip stack[GRAPH_LIMIT] = { { root, animation, material } };
    uint32_t count = 1, visits = 0;
    while (count) {
        AnimationZip zip = stack[--count];
        if (++visits > GRAPH_LIMIT ||
            !append(writer, zip.joint, zip.animation ? zip.animation->aobjdesc : NULL,
                    NativeArticleAnimationTarget_Joint, NULL)) return false;
        const HSD_MatAnim* ma = zip.material ? zip.material->matanim : NULL;
        uint32_t drawables = 0;
        for (const HSD_DObjDesc* d = zip.joint->u.dobjdesc; d; d = d->next) {
            if (++drawables > GRAPH_LIMIT) return false;
            if (d->mobjdesc && ma) {
                if (!append(writer, d->mobjdesc, ma->aobjdesc,
                            NativeArticleAnimationTarget_Material, NULL)) return false;
                NativeArticleAnimationTextureID textures[NATIVE_ARTICLE_ANIMATION_TEXTURE_LIMIT];
                uint32_t texture_count;
                if (!nativeArticleAnimation_TextureIDs(d->mobjdesc, textures,
                    NATIVE_ARTICLE_ANIMATION_TEXTURE_LIMIT, &texture_count)) return false;
                for (uint32_t i = 0; i < texture_count; ++i) {
                    const HSD_TObjDesc* t = textures[i].descriptor;
                    const HSD_TexAnim* ta = ma->texanim;
                    uint32_t candidates = 0;
                    while (ta && ta->id != textures[i].id) {
                        if (++candidates > GRAPH_LIMIT) return false;
                        ta = ta->next;
                    }
                    if (ta && !append(writer, t, ta->aobjdesc,
                                      NativeArticleAnimationTarget_Texture, ta)) return false;
                }
            }
            if (ma) ma = ma->next;
        }
        const HSD_AnimJoint* aj = zip.animation ? zip.animation->child : NULL;
        const HSD_MatAnimJoint* mj = zip.material ? zip.material->child : NULL;
        for (const HSD_Joint* j = zip.joint->child; j; j = j->next) {
            if (count == GRAPH_LIMIT) return false;
            stack[count++] = (AnimationZip){ j, aj, mj };
            if (aj) aj = aj->next;
            if (mj) mj = mj->next;
        }
    }
    return true;
}

static bool target_in_graph(const HSD_Joint* graph, const HSD_Joint* target)
{
    const HSD_Joint* stack[GRAPH_LIMIT] = { graph };
    uint32_t count = 1, visits = 0;
    while (count) {
        const HSD_Joint* joint = stack[--count];
        if (++visits > GRAPH_LIMIT) return false;
        if (joint == target) return true;
        if (joint->next) { if (count == GRAPH_LIMIT) return false; stack[count++] = joint->next; }
        if (joint->child) { if (count == GRAPH_LIMIT) return false; stack[count++] = joint->child; }
    }
    return false;
}

static bool application(OriginWriter* writer, const NativeArticleAnimationApplication* app,
                        NativeArticleAnimationDemand* output)
{
    NativeArticleAnimationDemand demand;
    if (!app->target || !target_in_graph(writer->graph, app->target) ||
        !nativeArticleAnimation_Census(app->target, app->anim, app->matanim,
            app->shapeanim, &demand) || demand.uncovered ||
        !apply_origins(writer, app->target, app->anim, app->matanim)) return false;
    if (output) *output = demand;
    return true;
}

bool nativeArticleAnimation_PrepareApplications(const HSD_Joint* descriptor,
    const NativeArticleAnimationApplication* applications, uint32_t count,
    NativeArticleAnimationOrigin* buffer, uint32_t capacity, uint32_t* written)
{
    NativeArticleAnimationDemand graph;
    if (!descriptor || !written || count > GRAPH_LIMIT || (count && !applications) ||
        capacity > NATIVE_SCRIPT_ENTITY_ANIMATION_ORIGINS || (capacity && !buffer) ||
        !nativeArticleAnimation_Census(descriptor, NULL, NULL, NULL, &graph) ||
        graph.uncovered) return false;
    OriginWriter writer = { buffer, descriptor, capacity, 0 };
    for (uint32_t i = 0; i < count; ++i)
        if (!application(&writer, &applications[i], NULL)) return false;
    *written = writer.count;
    return true;
}

static const HSD_Joint* path(const HSD_Joint* root,
                            const NativeScriptItemModelSelector* selector)
{
    if (selector->depth > 12) return NULL;
    for (unsigned depth = 0; root && depth < selector->depth; ++depth) {
        root = root->child;
        for (unsigned i = 0; root && i < selector->path[depth]; ++i) root = root->next;
    }
    return root;
}

bool nativeArticleAnimation_PrepareOrigins(const HSD_Joint* descriptor,
    const NativeScriptItemProgram* program, NativeArticleAnimationOrigin* buffer,
    uint32_t capacity, uint32_t* written)
{
    if (!descriptor || !program || !written || capacity > NATIVE_SCRIPT_ENTITY_ANIMATION_ORIGINS ||
        (capacity && !buffer) || !program->article || !program->article->x10_modelDesc ||
        program->model_root_count > NATIVE_FIGHTER_ARTICLE_MODEL_ROOT_LIMIT ||
        (program->model_root_count && !program->model_roots) ||
        program->model_animation_count > NATIVE_FIGHTER_ARTICLE_MODEL_ANIMATION_LIMIT ||
        (program->model_animation_count && !program->model_animations) ||
        program->model_selector_count > NATIVE_SCRIPT_ITEM_MODEL_SELECTORS ||
        (program->model_selector_count && !program->model_selectors)) return false;
    bool primary = descriptor == program->article->x10_modelDesc->x0_joint;
    bool owned = primary;
    for (uint32_t i = 0; i < program->model_root_count; ++i)
        if (program->model_roots[i].root == descriptor) owned = true;
    NativeArticleAnimationDemand graph;
    if (!owned || !nativeArticleAnimation_Census(descriptor, NULL, NULL, NULL, &graph) ||
        graph.uncovered) return false;
    OriginWriter writer = { buffer, descriptor, capacity, 0 };
    for (uint32_t i = 0; i < program->model_selector_count; ++i) {
        const NativeScriptItemModelSelector* s = &program->model_selectors[i];
        if ((s->semantic & NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK) !=
            NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_ADD) continue;
        if (s->animation >= program->model_animation_count || s->model > 3 || s->depth > 12)
            return false;
        const NativeScriptItemModelAnimationAsset* bundle = &program->model_animations[s->animation];
        for (uint32_t target = 0; target <= program->model_root_count; ++target) {
            const HSD_Joint* whole = target ? program->model_roots[target - 1].root :
                program->article->x10_modelDesc->x0_joint;
            if (whole != descriptor || (target == 0) != (s->model == 0)) continue;
            const NativeScriptItemModelAnimationTarget* prepared = &s->animation_targets[target];
            if (!prepared->valid) continue;
            const HSD_Joint* selected = path(descriptor, s);
            if (selected != prepared->descriptor) return false;
            if (!selected) continue;
            NativeArticleAnimationDemand demand;
            NativeArticleAnimationApplication app = { selected, bundle->anim, bundle->matanim, bundle->shapeanim };
            if (prepared->uncovered || !application(&writer, &app, &demand) ||
                demand.controllers != prepared->controllers || demand.tracks != prepared->tracks ||
                demand.uncovered) return false;
        }
    }
    // Native Item state installation acts on the primary graph only. Secondary
    // graphs must not be rejected for unrelated unsupported primary animations.
    if (primary && program->article->xC_itemStates) {
        const ItemStateArray* states = program->article->xC_itemStates;
        if (states->count > STATE_LIMIT || (states->count && !states->x0_itemStateDesc)) return false;
        for (uint32_t i = 0; i < states->count; ++i) {
            const ItemStateDesc* s = &states->x0_itemStateDesc[i];
            const HSD_AnimJoint* aj = s->x0_anim_joint;
            const HSD_MatAnimJoint* mj = s->x4_matanim_joint;
            const HSD_ShapeAnimJoint* sj = s->x8_parameters;
            uint32_t siblings = 0;
            // Item_80268BE0 traverses the primary root's next siblings too;
            // public HSD_JObjAddAnimAll only traverses the selected subtree.
            for (const HSD_Joint* j = descriptor; j; j = j->next) {
                if (++siblings > GRAPH_LIMIT) return false;
                NativeArticleAnimationDemand demand;
                if (nativeArticleAnimation_Census(j, aj, mj, sj, &demand) && !demand.uncovered) {
                    NativeArticleAnimationApplication app = { j, aj, mj, sj };
                    if (!application(&writer, &app, NULL)) return false;
                }
                if (aj) aj = aj->next;
                if (mj) mj = mj->next;
                if (sj) sj = sj->next;
            }
        }
    }
    *written = writer.count;
    return true;
}
