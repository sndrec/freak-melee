#include "native_fighter_pose.h"
#include "native_fighter_runtime.h"
#include "native_fighter_rig.h"
#include "ftcharacter_assets.h"
#include "ftcostume_assets.h"
#include "fighter.h"
#include "ftparts.h"
#include "ftanimation_bank.h"
#include <melee/lb/lbanim.h>
#include <sysdolphin/baselib/jobj.h>
#include <math.h>
#include <stddef.h>
#include <string.h>

static bool span(const void* base, size_t size, const void* pointer, size_t bytes)
{
    uintptr_t first = (uintptr_t)base, at = (uintptr_t)pointer;
    return base != NULL && pointer != NULL && at >= first && at - first <= size && bytes <= size - (at - first);
}

static bool imported_joint(const HSD_DatImport* import, const HSD_Joint* joint)
{
    if (import == NULL || import->record_count > import->record_capacity ||
        import->record_count > FT_CHARACTER_IMPORT_RECORD_CAPACITY ||
        import->arena_used > import->arena_size ||
        !span(import->arena, import->arena_used, import->records,
              import->record_count * sizeof(*import->records)) ||
        !span(import->arena, import->arena_used, joint, sizeof(*joint)) || import->records == NULL) return false;
    for (unsigned i = 0; i < import->record_count; ++i)
        if (import->records[i].type == DAT_JOINT && import->records[i].object == joint) return true;
    return false;
}

// This consumes the selected descriptor and its following sibling forest, just
// like ftAnim_GetNextJointInTree. Shared nodes/cycles are rejected before calls.
static unsigned descriptors(const HSD_DatImport* import, HSD_Joint* root, HSD_Joint** output)
{
    HSD_Joint* pending[MAX_FT_PARTS]; unsigned depth = 0, count = 0;
    HSD_Joint* node = root;
    while (node != NULL) {
        if (count == MAX_FT_PARTS || !imported_joint(import, node)) return 0;
        for (unsigned i = 0; i < count; ++i) if (output[i] == node) return 0;
        output[count++] = node;
        if (node->child != NULL) {
            // Actual ftAnim_GetNextJointInTree asserts at depth 30, even when
            // an ancestor has no following sibling. Validate that same stack.
            pending[depth++] = node->next;
            if (depth >= 30) return 0;
            node = node->child;
        } else if (node->next != NULL) node = node->next;
        else {
            node = NULL;
            while (depth && node == NULL) node = pending[--depth];
        }
    }
    return count;
}

HSD_Joint* nativeFighterPose_SelectGraph(const FtCharacterAssets* assets, unsigned slot, const char* path)
{
    if (assets == NULL || slot >= FT_CHARACTER_ITEM_CAPACITY || !nativeFighterPose_PathValid(path)) return NULL;
    HSD_Joint* nodes[MAX_FT_PARTS]; HSD_Joint* graph = assets->joint_graphs[slot];
    if (graph == NULL || assets->fighter_items[slot] != graph || !descriptors(&assets->import, graph, nodes)) return NULL;
    while (*path) {
        unsigned ordinal = 0;
        while (*path >= '0' && *path <= '9') ordinal = ordinal * 10 + (unsigned)(*path++ - '0');
        graph = graph->child;
        while (graph != NULL && ordinal--) graph = graph->next;
        if (graph == NULL) return NULL;
        if (*path) ++path;
    }
    return graph;
}

bool nativeFighterPose_BindGraph(const FtCharacterAssets* assets,unsigned slot,
    const char* path,const NativeFighterRig* rig,const int32_t* ids,
    unsigned count,NativeFighterPoseBinding* output)
{
    if(!assets||!rig||!ids||!output||!count||count>MAX_FT_PARTS)return false;
    HSD_Joint* graph=nativeFighterPose_SelectGraph(assets,slot,path);
    NativeFighterPoseBinding binding={0};
    if(!graph||descriptors(&assets->import,graph,binding.descriptors)!=count)return false;
    for(unsigned n=0;n<count;++n){
        int part=nativeFighterRig_Resolve(rig,ids[n]);
        if(part<0||(unsigned)part>=rig->count)return false;
        for(unsigned j=0;j<n;++j)if(binding.parts[j]==part)return false;
        binding.parts[n]=(uint16_t)part;
    }
    binding.count=(uint16_t)count;*output=binding;return true;
}

static unsigned joints(HSD_JObj* root, HSD_JObj** output)
{
    HSD_JObj* pending[MAX_FT_PARTS]; unsigned depth = 0, count = 0;
    HSD_JObj* node = root;
    while (node != NULL) {
        if (count == MAX_FT_PARTS) return 0;
        for (unsigned i = 0; i < count; ++i) if (output[i] == node) return 0;
        output[count++] = node;
        if (node->next != NULL && node->next->parent != node->parent) return 0;
        if (node->child != NULL) {
            if (node->child->parent != node) return 0;
            if (node->next != NULL) pending[depth++] = node->next;
            node = node->child;
        } else if (node->next != NULL) {
            if (node->next->parent != node->parent) return 0;
            node = node->next;
        } else node = depth ? pending[--depth] : NULL;
    }
    return count;
}

static bool contains(HSD_JObj* const* nodes, unsigned count, const HSD_JObj* joint)
{
    for (unsigned i = 0; i < count; ++i) if (nodes[i] == joint) return true;
    return false;
}

static bool clip_valid(const NativeFighterPoseAssets* assets, const FigaTree* clip, unsigned* nodes)
{
    if (!span(assets->animation_arena, assets->animation_arena_size, clip, sizeof(*clip)) ||
        !isfinite(clip->frames) || clip->frames < 0) return false;
    bool owned = false;
    for (unsigned i = 0; i < assets->action_count; ++i)
        if (assets->character->data.xC[i].x14 == clip) { owned = true; break; }
    if (!owned) return false;
    unsigned count = 0, tracks = 0;
    for (;;) {
        if (!span(assets->animation_bytes, assets->animation_size, (const void*)((uintptr_t)clip->nodes + count), 1)) return false;
        int frames = clip->nodes[count];
        if (frames == -1) break;
        if (frames < 0 || count == MAX_FT_PARTS || tracks > FT_ANIMATION_SCRATCH_TRACK_CAPACITY - (unsigned)frames) return false;
        tracks += (unsigned)frames; ++count;
    }
    if (tracks && !span(assets->animation_arena, assets->animation_arena_size, clip->tracks, tracks * sizeof(*clip->tracks))) return false;
    for (unsigned i = 0; i < tracks; ++i)
        if (!span(assets->animation_bytes, assets->animation_size, clip->tracks[i].ad_head, clip->tracks[i].length)) return false;
    *nodes = count; return true;
}

bool nativeFighterPose_Validate(Fighter_GObj* object, NativeFighterPoseOperation operation,
                                int32_t destination, int32_t start, HSD_Joint* graph, FigaTree* clip)
{
    NativeFighterPoseAssets assets;
    if ((unsigned)operation > NativeFighterPose_RestorePartAnimation || (unsigned)destination > NativeFighterPose_Blend ||
        !nativeFighterRuntime_PoseAssets(object, &assets)) return false;
    Fighter* fighter = object->user_data;
    unsigned count = ftPartsTable[fighter->kind]->parts_num;
    if (!count || count > MAX_FT_PARTS || fighter->parts == NULL ||
        (uint32_t)start >= count || Fighter_804D6540 == NULL) return false;
    const struct Fighter_804D6540_t* omissions = Fighter_804D6540[fighter->kind];
    if (omissions != NULL && (omissions->x4 < 0 || omissions->x4 > 32 ||
        (omissions->x4 && omissions->x0 == NULL))) return false;
    if (omissions != NULL) for (unsigned i = 0; i < (unsigned)omissions->x4; ++i)
        if (omissions->x0[i].x0 >= count) return false;
    HSD_JObj* live[MAX_FT_PARTS]; HSD_JObj* alternate[MAX_FT_PARTS];
    unsigned live_count = joints(object->hsd_obj, live), alternate_count = joints(fighter->x8AC_animSkeleton, alternate);
    if (!live_count || !alternate_count || fighter->parts[0].joint != object->hsd_obj ||
        fighter->parts[0].x4_jobj2 != fighter->x8AC_animSkeleton) return false;
    HSD_JObj** target = destination == NativeFighterPose_Blend ? alternate : live;
    unsigned target_count = destination == NativeFighterPose_Blend ? alternate_count : live_count;
    // Every runtime part that is used must belong to the selected current rig.
    for (unsigned i = 0; i < count; ++i) {
        if (fighter->parts[i].joint != NULL && !contains(live, live_count, fighter->parts[i].joint)) return false;
        if (fighter->parts[i].x4_jobj2 != NULL && !contains(alternate, alternate_count, fighter->parts[i].x4_jobj2)) return false;
        for (unsigned j = 0; j < i; ++j) {
            if (fighter->parts[i].joint != NULL && fighter->parts[i].joint == fighter->parts[j].joint) return false;
            if (fighter->parts[i].x4_jobj2 != NULL && fighter->parts[i].x4_jobj2 == fighter->parts[j].x4_jobj2) return false;
        }
    }
    if (operation == NativeFighterPose_RestorePartAnimation) {
        if (graph || clip != fighter->x590 || !isfinite(fighter->cur_anim_frame) ||
            !isfinite(fighter->frame_speed_mul) || !isfinite(fighter->x8A4_animBlendFrames) ||
            destination != (clip && fighter->x8A4_animBlendFrames != 0 ? NativeFighterPose_Blend : NativeFighterPose_Live)) return false;
        HSD_Joint* source[MAX_FT_PARTS];
        unsigned source_count = descriptors(&assets.costume->import, assets.costume->joint, source);
        unsigned part = 0, selected = MAX_FT_PARTS;
        for (unsigned node = 0; node < source_count; ++node, ++part) {
            while (part < count && ftParts_8007506C(fighter->kind, part)) ++part;
            if (part == count) return false;
            if (part == (unsigned)start) selected = node;
        }
        if (!source_count || selected == MAX_FT_PARTS || !fighter->parts[start].joint ||
            !fighter->ft_data->x0 || !fighter->ft_data->x8 || fighter->ft_data->x8->x10 >= count ||
            !isfinite(fighter->ft_data->x0->model_scaling) || fighter->ft_data->x0->model_scaling == 0) return false;
        source_count = descriptors(&assets.costume->import, source[selected], source);
        // Hiding clears flags_b5 before restoring. Admit every potentially
        // restored joint, including ones currently overridden by the part.
        part = start;
        for (unsigned node = 0; node < source_count; ++node, ++part) {
            while (part < count && ftParts_8007506C(fighter->kind, part)) ++part;
            if (part == count || !contains(target, target_count,
                    destination ? fighter->parts[part].x4_jobj2 : fighter->parts[part].joint)) return false;
        }
        if (!source_count) return false;
        if (!clip) return true;
        unsigned node_count;
        if (!clip_valid(&assets, clip, &node_count) || (unsigned)fighter->anim_id >= assets.action_count ||
            fighter->x597_bits != nativeFighterRuntime_AnimationFamily(fighter)) return false;
        unsigned family = fighter->x597_bits;
        if (family >= Ft_Kind_RuntimeCapacity || !ftPartsTable[family]) return false;
        unsigned family_count = ftPartsTable[family]->parts_num;
        const struct Fighter_804D6540_t* family_omissions = Fighter_804D6540[family];
        if (!family_count || family_count > MAX_FT_PARTS || !ftPartsTable[family]->part_to_joint ||
            !ftPartsTable[family]->joint_to_part || (family_omissions &&
            (family_omissions->x4 < 0 || family_omissions->x4 > 32 ||
             (family_omissions->x4 && !family_omissions->x0)))) return false;
        if (family_omissions) for (unsigned i = 0; i < (unsigned)family_omissions->x4; ++i)
            if (family_omissions->x0[i].x0 >= family_count) return false;
        // Match the native partial Figa traversal, including its prefix skip
        // and subtree depth limit; checking a whole clip alone is insufficient.
        unsigned node = 0;
        for (part = 0; part < (unsigned)start; ++part) {
            if (part >= family_count) return false;
            uint32_t mask = ftParts_8007506C(family, part);
            if (!mask || (mask & fighter->x594_bits)) {
                if (node == node_count) return false;
                ++node;
            }
        }
        while (node < node_count) {
            if (family == (unsigned)fighter->kind) {
                while (part < count && !fighter->parts[part].flags_b1) ++part;
                while (part < count && fighter->parts[part].flags_b2) {
                    uint32_t mask = ftParts_8007506C(family, part);
                    if (!mask || (mask & fighter->x594_bits)) break;
                    ++part;
                }
            } else {
                while (part < family_count) {
                    uint32_t mask = ftParts_8007506C(family, part);
                    if (!mask || (mask & fighter->x594_bits)) break;
                    ++part;
                }
            }
            if (part >= count || part >= family_count) return false;
            if (part != (unsigned)start && fighter->parts[part].xC <= fighter->parts[start].xC) break;
            if (family != (unsigned)fighter->kind && ftPartsTable[family]->joint_to_part[part] != FTPART_INVALID &&
                ftPartsTable[family]->joint_to_part[part] >= count) return false;
            unsigned mapped = family == (unsigned)fighter->kind ? part : ftPartsRemap(fighter->kind, family, part);
            if (mapped != 0xff && (mapped >= count ||
                (fighter->parts[mapped].flags_b1 && !fighter->parts[mapped].flags_b0 &&
                 !contains(target, target_count, destination ? fighter->parts[mapped].x4_jobj2 : fighter->parts[mapped].joint)))) return false;
            ++node; ++part;
        }
        return true;
    }
    if (operation == NativeFighterPose_RequestFrame || operation == NativeFighterPose_Evaluate)
        return graph == NULL && clip == NULL && start == 0;
    if (operation == NativeFighterPose_LoadAnimation) {
        unsigned node_count;
        if (graph != NULL || start != 0 || !clip_valid(&assets, clip, &node_count)) return false;
        unsigned i = 0;
        for (unsigned node = 0; node < node_count; ++node, ++i) {
            while (i < count && !fighter->parts[i].flags_b1) ++i;
            while (i < count && fighter->parts[i].flags_b2) {
                u32 mask = ftParts_8007506C(fighter->kind, i);
                if (mask == 0 || (mask & fighter->x594_bits)) break;
                ++i;
            }
            if (i == count) return false;
            if (!fighter->parts[i].flags_b0 && !fighter->parts[i].flags_b5 &&
                !contains(target, target_count, destination ? fighter->parts[i].x4_jobj2 : fighter->parts[i].joint)) return false;
        }
        return true;
    }
    if (clip != NULL) return false;
    if (operation == NativeFighterPose_BlendToLive || operation == NativeFighterPose_CopyToLive) {
        if (graph != NULL || destination != NativeFighterPose_Live) return false;
        for (unsigned i = start; i < count; ++i)
            if (fighter->parts[i].flags_b1 && !fighter->parts[i].flags_b0 && !fighter->parts[i].flags_b5 &&
                (!contains(live, live_count, fighter->parts[i].joint) ||
                 !contains(alternate, alternate_count, fighter->parts[i].x4_jobj2))) return false;
        return true;
    }
    HSD_Joint* source[MAX_FT_PARTS]; unsigned source_count = 0;
    if (operation == NativeFighterPose_CopyCostume) {
        if (graph != NULL) return false;
        source_count = descriptors(&assets.costume->import, assets.costume->joint, source);
        unsigned part = 0, selected = MAX_FT_PARTS;
        for (unsigned node = 0; node < source_count; ++node, ++part) {
            while (part < count && ftParts_8007506C(fighter->kind, part)) ++part;
            if (part == count) return false;
            if (part == (unsigned)start) selected = node;
        }
        if (!source_count || selected == MAX_FT_PARTS ||
            !contains(target, target_count, destination ? fighter->parts[start].x4_jobj2 : fighter->parts[start].joint)) return false;
        // Starting a new shared iterator at this descriptor cannot reach
        // siblings belonging to ancestors from the whole-costume traversal.
        HSD_Joint* selected_graph = source[selected];
        source_count = descriptors(&assets.costume->import, selected_graph, source);
        if (!source_count) return false;
    } else {
        bool declared = false;
        for (unsigned i = 0; i < assets.pose_graph_count; ++i)
            if (nativeFighterRuntime_PoseGraph(object, (int32_t)i) == graph) { declared = true; break; }
        if (graph == NULL || !declared) return false;
        source_count = descriptors(&assets.character->import, graph, source);
        if (!source_count) return false;
    }
    bool copying = operation != NativeFighterPose_BlendGraph;
    if (copying && (fighter->ft_data->x8 == NULL || fighter->ft_data->x0 == NULL ||
        fighter->ft_data->x8->x10 >= count || !isfinite(fighter->ft_data->x0->model_scaling) ||
        fighter->ft_data->x0->model_scaling == 0)) return false;
    unsigned part = start;
    for (unsigned node = 0; node < source_count; ++node, ++part) {
        while (part < count && ftParts_8007506C(fighter->kind, part)) ++part;
        if (part == count) return false;
        FighterBone* bone = &fighter->parts[part];
        bool needed = copying ? bone->flags_b0 || !bone->flags_b5 || part == fighter->ft_data->x8->x10 :
                      !bone->flags_b0 && !bone->flags_b5;
        if (needed && !contains(target, target_count, destination ? bone->x4_jobj2 : bone->joint)) return false;
    }
    return true;
}
