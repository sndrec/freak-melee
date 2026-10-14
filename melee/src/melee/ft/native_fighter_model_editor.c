#include "native_fighter_model_editor.h"

#include "native_fighter_authoring.h"

#ifdef MELEE_NATIVE

#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/pobj.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool reference(const HSD_Dat* dat, u32 slot, u32* target)
{
    const char* external;
    return HSD_DatResolve(dat, slot, target, &external) == HSD_DAT_REF_LOCAL &&
           *target < dat->data_size;
}

static void write_u32(NativeFighterModelEditor* editor, u32 offset,
                      u32 value)
{
    uint8_t* bytes = editor->project->costume_dat + 0x20 + offset;
    bytes[0] = (uint8_t) (value >> 24);
    bytes[1] = (uint8_t) (value >> 16);
    bytes[2] = (uint8_t) (value >> 8);
    bytes[3] = (uint8_t) value;
}

static void read_name(const HSD_Dat* dat, u32 slot, const char* fallback,
                      char output[64])
{
    u32 offset;
    if (reference(dat, slot, &offset)) {
        const char* name = (const char*) HSD_DatBytes(dat, offset, 1);
        if (name != NULL) {
            size_t capacity = dat->data_size - offset;
            const char* end = memchr(name, 0, capacity);
            if (end != NULL && end > name && end - name < 64) {
                bool printable = true;
                for (const char* c = name; c < end; ++c)
                    if ((unsigned char) *c < 32) printable = false;
                if (printable) {
                    memcpy(output, name, (size_t) (end - name));
                    output[end - name] = 0;
                    return;
                }
            }
        }
    }
    snprintf(output, 64, "%s", fallback);
}

static bool add_material(NativeFighterModelEditor* editor, u32 offset,
                         uint16_t* index)
{
    for (uint16_t i = 0; i < editor->material_count; ++i)
        if (editor->materials[i].offset == offset) {
            *index = i;
            return true;
        }
    if (editor->material_count == NATIVE_MODEL_MAX_MATERIALS ||
        HSD_DatBytes(&editor->dat, offset, 24) == NULL) return false;
    const uint16_t id = editor->material_count++;
    NativeModelMaterial* material = &editor->materials[id];
    memset(material, 0, sizeof(*material));
    material->offset = offset;
    snprintf(material->name, sizeof(material->name), "Material %u", id);
    HSD_DatReadU32(&editor->dat, offset + 4, &material->render_mode);
    reference(&editor->dat, offset + 8, &material->texture_offset);
    if (reference(&editor->dat, offset + 12, &material->color_offset) &&
        HSD_DatBytes(&editor->dat, material->color_offset, 20) != NULL) {
        const uint8_t* colors = HSD_DatBytes(&editor->dat,
                                              material->color_offset, 20);
        memcpy(material->ambient, colors, 4);
        memcpy(material->diffuse, colors + 4, 4);
        memcpy(material->specular, colors + 8, 4);
        HSD_DatReadF32(&editor->dat, material->color_offset + 12,
                       &material->alpha);
        HSD_DatReadF32(&editor->dat, material->color_offset + 16,
                       &material->shininess);
    }
    *index = id;
    return true;
}

static bool add_meshes(NativeFighterModelEditor* editor, u32 first,
                       uint16_t joint)
{
    u32 offset = first;
    while (offset != 0) {
        if (editor->mesh_count == NATIVE_MODEL_MAX_MESHES ||
            HSD_DatBytes(&editor->dat, offset, 16) == NULL) return false;
        for (uint16_t i = 0; i < editor->mesh_count; ++i)
            if (editor->meshes[i].offset == offset) return false;
        NativeModelMesh* mesh = &editor->meshes[editor->mesh_count];
        memset(mesh, 0, sizeof(*mesh));
        mesh->offset = offset;
        mesh->joint = joint;
        mesh->material = UINT16_MAX;
        char fallback[64];
        snprintf(fallback, sizeof(fallback), "Mesh %u", editor->mesh_count);
        read_name(&editor->dat, offset, fallback, mesh->name);
        u32 material;
        if (reference(&editor->dat, offset + 8, &material) &&
            !add_material(editor, material, &mesh->material)) return false;
        u32 polygon;
        if (reference(&editor->dat, offset + 12, &polygon)) {
            for (unsigned i = 0; i < 4096 && polygon != 0; ++i) {
                if (HSD_DatBytes(&editor->dat, polygon, 24) == NULL)
                    return false;
                ++mesh->polygon_count;
                u32 next;
                if (!reference(&editor->dat, polygon + 4, &next)) break;
                polygon = next;
            }
        }
        ++editor->mesh_count;
        u32 next;
        if (!reference(&editor->dat, offset + 4, &next)) break;
        offset = next;
    }
    return true;
}

static bool add_joint(NativeFighterModelEditor* editor, u32 offset,
                      int16_t parent, uint8_t depth)
{
    if (editor->joint_count == NATIVE_MODEL_MAX_JOINTS ||
        HSD_DatBytes(&editor->dat, offset, 64) == NULL) return false;
    for (uint16_t i = 0; i < editor->joint_count; ++i)
        if (editor->joints[i].offset == offset) return false;
    const uint16_t index = editor->joint_count++;
    NativeModelJoint* joint = &editor->joints[index];
    memset(joint, 0, sizeof(*joint));
    joint->offset = offset;
    joint->parent = parent;
    joint->depth = depth;
    HSD_DatReadU32(&editor->dat, offset + 4, &joint->flags);
    char fallback[64];
    snprintf(fallback, sizeof(fallback), "Joint %u", index);
    read_name(&editor->dat, offset, fallback, joint->name);
    for (unsigned axis = 0; axis < 3; ++axis) {
        HSD_DatReadF32(&editor->dat, offset + 20 + axis * 4,
                       &joint->rotation[axis]);
        HSD_DatReadF32(&editor->dat, offset + 32 + axis * 4,
                       &joint->scale[axis]);
        HSD_DatReadF32(&editor->dat, offset + 44 + axis * 4,
                       &joint->position[axis]);
    }
    u32 mesh;
    if (!(joint->flags & (JOBJ_PTCL | JOBJ_SPLINE)) &&
        reference(&editor->dat, offset + 16, &mesh) &&
        !add_meshes(editor, mesh, index)) return false;
    u32 child;
    if (reference(&editor->dat, offset + 8, &child) &&
        !add_joint(editor, child, (int16_t) index, depth + 1)) return false;
    u32 next;
    if (reference(&editor->dat, offset + 12, &next) &&
        !add_joint(editor, next, parent, depth)) return false;
    return true;
}

bool nativeFighterModel_Open(NativeFighterModelEditor* editor,
                             NativeFighterProject* project)
{
    if (editor == NULL || project == NULL || project->costume_dat == NULL)
        return false;
    memset(editor, 0, sizeof(*editor));
    editor->project = project;
    if (HSD_DatOpen(&editor->dat, project->costume_dat,
                    project->costume_size) != HSD_DAT_OK) return false;
    u32 root = 0;
    if (!HSD_DatFindRoot(&editor->dat, project->facts.costume_joint, &root))
        return false;
    snprintf(editor->root_name, sizeof(editor->root_name), "%s",
             project->facts.costume_joint);
    if (!add_joint(editor, root, -1, 0)) {
        return false;
    }
    for (unsigned category = 0; category < 3; ++category) {
        unsigned count = category == 0   ? editor->joint_count
                         : category == 1 ? editor->mesh_count
                                         : editor->material_count;
        for (unsigned i = 0; i < count; ++i) {
            char* target = category == 0   ? editor->joints[i].name
                           : category == 1 ? editor->meshes[i].name
                                           : editor->materials[i].name;
            const char* custom = nativeFighterAuthoring_Name(
                project, category,
                category == 0   ? editor->joints[i].offset
                : category == 1 ? editor->meshes[i].offset
                                : editor->materials[i].offset);
            if (custom && *custom) {
                snprintf(target, 64, "%s", custom);
            } else if (category == 0 && !strncmp(target, "Joint ", 6)) {
                const char* role = nativeFighterAuthoring_BoneRole(project, i);
                if (role) {
                    snprintf(target, 64, "%s · joint %u", role, i);
                }
            }
        }
    }
    return true;
}

bool nativeFighterModel_SetJoint(NativeFighterModelEditor* editor,
                                unsigned joint, unsigned channel,
                                unsigned axis, float value)
{
    if (editor == NULL || joint >= editor->joint_count || axis >= 3 ||
        channel >= 3 || !isfinite(value) ||
        (channel == 0 && fabsf(value) > 100.0f) ||
        (channel == 1 && (value < 0.001f || value > 100.0f)) ||
        (channel == 2 && fabsf(value) > 10000.0f)) return false;
    const u32 offset = editor->joints[joint].offset +
        (channel == 0 ? 20 : channel == 1 ? 32 : 44) + axis * 4;
    if (HSD_DatBytes(&editor->dat, offset, 4) == NULL) return false;
    u32 bits;
    memcpy(&bits, &value, sizeof(bits));
    write_u32(editor, offset, bits);
    float* field = channel == 0 ? editor->joints[joint].rotation :
                   channel == 1 ? editor->joints[joint].scale :
                                  editor->joints[joint].position;
    field[axis] = value;
    return true;
}

static bool rest_world_matrices(const NativeFighterModelEditor* model,
                                Mtx* world)
{
    Vec3 accumulated_scale[NATIVE_MODEL_MAX_JOINTS];
    bool has_scale[NATIVE_MODEL_MAX_JOINTS] = { false };
    for (unsigned i = 0; i < model->joint_count; ++i) {
        const NativeModelJoint* bone = &model->joints[i];
        const int parent = bone->parent;
        if (bone->flags & 0x20000) return false;
        Vec3 scale = { bone->scale[0], bone->scale[1], bone->scale[2] };
        Vec3 rotation = { bone->rotation[0], bone->rotation[1],
                          bone->rotation[2] };
        Vec3 position = { bone->position[0], bone->position[1],
                          bone->position[2] };
        Vec3* parent_scale = parent >= 0 && has_scale[parent]
            ? &accumulated_scale[parent] : NULL;
        if (parent_scale != NULL &&
            (fabsf(parent_scale->x) < 0.000001f ||
             fabsf(parent_scale->y) < 0.000001f ||
             fabsf(parent_scale->z) < 0.000001f)) return false;
        if (bone->flags & 8) {
            if (parent_scale != NULL) {
                accumulated_scale[i] = *parent_scale;
                has_scale[i] = true;
            }
        } else {
            has_scale[i] = true;
            accumulated_scale[i].x = scale.x *
                (parent_scale ? parent_scale->x : 1.0f);
            accumulated_scale[i].y = scale.y *
                (parent_scale ? parent_scale->y : 1.0f);
            accumulated_scale[i].z = scale.z *
                (parent_scale ? parent_scale->z : 1.0f);
        }
        Mtx local;
        HSD_MtxSRT(local, &scale, &rotation, &position, parent_scale);
        if (parent >= 0) C_MTXConcat(world[parent], local, world[i]);
        else memcpy(world[i], local, sizeof(local));
        for (unsigned row = 0; row < 3; ++row)
            for (unsigned column = 0; column < 4; ++column)
                if (!isfinite(world[i][row][column])) return false;
    }
    return true;
}

bool nativeFighterModel_CompensateRest(NativeFighterModelEditor* editor,
                                      const uint8_t* previous,
                                      size_t previous_size,
                                      unsigned changed_joint)
{
    if (editor == NULL || previous == NULL ||
        changed_joint >= editor->joint_count ||
        previous_size != editor->project->costume_size) return false;
    NativeFighterProject old_project = *editor->project;
    old_project.costume_dat = (uint8_t*) previous;
    NativeFighterModelEditor* old = calloc(1, sizeof(*old));
    if (old == NULL) return false;
    bool result = nativeFighterModel_Open(old, &old_project) &&
                  old->joint_count == editor->joint_count;
    Mtx* old_world = result ? calloc(editor->joint_count, sizeof(Mtx)) : NULL;
    Mtx* new_world = result ? calloc(editor->joint_count, sizeof(Mtx)) : NULL;
    if (old_world == NULL || new_world == NULL ||
        !rest_world_matrices(old, old_world) ||
        !rest_world_matrices(editor, new_world)) result = false;
    if (result) for (unsigned i = 0; i < editor->joint_count; ++i) {
        int cursor = (int) i;
        while (cursor >= 0 && cursor != (int) changed_joint)
            cursor = editor->joints[cursor].parent;
        if (cursor < 0) continue;
        u32 old_bind_offset, new_bind_offset;
        if (!reference(&old->dat, old->joints[i].offset + 56,
                       &old_bind_offset)) continue;
        if (!reference(&editor->dat, editor->joints[i].offset + 56,
                       &new_bind_offset) ||
            HSD_DatBytes(&old->dat, old_bind_offset, sizeof(Mtx)) == NULL ||
            HSD_DatBytes(&editor->dat, new_bind_offset,
                         sizeof(Mtx)) == NULL) {
            result = false;
            break;
        }
        Mtx inverse_new, old_bind, adjustment, new_bind;
        if (!C_MTXInverse(new_world[i], inverse_new)) {
            result = false;
            break;
        }
        for (unsigned row = 0; row < 3; ++row)
            for (unsigned column = 0; column < 4; ++column) {
                const u32 slot = old_bind_offset + (row * 4 + column) * 4;
                if (!HSD_DatReadF32(&old->dat, slot,
                                    &old_bind[row][column]))
                    result = false;
            }
        if (!result) break;
        C_MTXConcat(inverse_new, old_world[i], adjustment);
        C_MTXConcat(adjustment, old_bind, new_bind);
        Mtx old_rest, new_rest;
        C_MTXConcat(old_world[i], old_bind, old_rest);
        C_MTXConcat(new_world[i], new_bind, new_rest);
        for (unsigned row = 0; row < 3; ++row)
            for (unsigned column = 0; column < 4; ++column)
                if (!isfinite(new_rest[row][column]) ||
                    fabsf(new_rest[row][column] -
                          old_rest[row][column]) > 0.002f)
                    result = false;
        if (!result) break;
        for (unsigned row = 0; row < 3; ++row)
            for (unsigned column = 0; column < 4; ++column) {
                const float value = new_bind[row][column];
                if (!isfinite(value)) result = false;
                u32 bits;
                memcpy(&bits, &value, sizeof(bits));
                write_u32(editor, new_bind_offset +
                          (row * 4 + column) * 4, bits);
            }
        if (!result) break;
    }
    free(old_world);
    free(new_world);
    free(old);
    return result;
}

bool nativeFighterModel_SetMaterialColor(NativeFighterModelEditor* editor,
                                        unsigned material, unsigned color,
                                        unsigned channel, uint8_t value)
{
    if (editor == NULL || material >= editor->material_count ||
        color >= 3 || channel >= 4) return false;
    NativeModelMaterial* entry = &editor->materials[material];
    if (entry->color_offset == 0 ||
        HSD_DatBytes(&editor->dat, entry->color_offset + color * 4 +
                     channel, 1) == NULL) return false;
    editor->project->costume_dat[0x20 + entry->color_offset +
                                color * 4 + channel] = value;
    uint8_t* field = color == 0 ? entry->ambient :
                     color == 1 ? entry->diffuse : entry->specular;
    field[channel] = value;
    return true;
}

bool nativeFighterModel_SetMaterialFloat(NativeFighterModelEditor* editor,
                                        unsigned material, bool shininess,
                                        float value)
{
    if (editor == NULL || material >= editor->material_count ||
        !isfinite(value) || value < 0.0f ||
        value > (shininess ? 1000.0f : 1.0f)) return false;
    NativeModelMaterial* entry = &editor->materials[material];
    const u32 offset = entry->color_offset + (shininess ? 16 : 12);
    if (entry->color_offset == 0 ||
        HSD_DatBytes(&editor->dat, offset, 4) == NULL) return false;
    u32 bits;
    memcpy(&bits, &value, sizeof(bits));
    write_u32(editor, offset, bits);
    if (shininess) entry->shininess = value;
    else entry->alpha = value;
    return true;
}

bool nativeFighterModel_SetMeshMaterial(NativeFighterModelEditor* editor,
                                       unsigned mesh, unsigned material)
{
    if (editor == NULL || mesh >= editor->mesh_count ||
        material >= editor->material_count) return false;
    u32 target;
    if (!reference(&editor->dat, editor->meshes[mesh].offset + 8,
                   &target)) return false;
    write_u32(editor, editor->meshes[mesh].offset + 8,
              editor->materials[material].offset);
    editor->meshes[mesh].material = (uint16_t) material;
    return true;
}

unsigned nativeFighterModel_EnvelopeGroups(const NativeFighterModelEditor* editor,
                                           unsigned mesh,
                                           NativeModelEnvelopeGroup* output,
                                           unsigned capacity)
{
    if (editor == NULL || mesh >= editor->mesh_count) return 0;
    u32 polygon;
    if (!reference(&editor->dat, editor->meshes[mesh].offset + 12,
                   &polygon)) return 0;
    unsigned count = 0;
    for (unsigned p = 0; p < 4096 && polygon != 0; ++p) {
        u16 flags = 0;
        if (!HSD_DatReadU16(&editor->dat, polygon + 12, &flags)) break;
        if ((flags & 0x3000) == POBJ_ENVELOPE) {
            u32 table;
            if (reference(&editor->dat, polygon + 20, &table))
                for (unsigned palette = 0; palette < 10; ++palette) {
                    u32 entries;
                    if (!reference(&editor->dat, table + palette * 4,
                                   &entries)) break;
                    NativeModelEnvelopeGroup group = { 0 };
                    group.polygon = (uint16_t) p;
                    group.palette = (uint8_t) palette;
                    for (unsigned i = 0;
                         i < NATIVE_MODEL_MAX_ENVELOPE_INFLUENCES; ++i) {
                        u32 joint;
                        if (!reference(&editor->dat, entries + i * 8,
                                       &joint)) break;
                        unsigned mapped = editor->joint_count;
                        for (unsigned j = 0; j < editor->joint_count; ++j)
                            if (editor->joints[j].offset == joint) {
                                mapped = j;
                                break;
                            }
                        if (mapped == editor->joint_count) break;
                        const unsigned index = group.influence_count++;
                        group.influences[index].joint = (uint16_t) mapped;
                        group.influences[index].weight_offset =
                            entries + i * 8 + 4;
                        HSD_DatReadF32(&editor->dat,
                                       group.influences[index].weight_offset,
                                       &group.influences[index].weight);
                    }
                    u32 extra_joint;
                    if (group.influence_count ==
                            NATIVE_MODEL_MAX_ENVELOPE_INFLUENCES &&
                        reference(&editor->dat,
                                  entries +
                                      NATIVE_MODEL_MAX_ENVELOPE_INFLUENCES * 8,
                                  &extra_joint))
                        group.truncated = 1;
                    if (group.influence_count == 0) break;
                    if (output != NULL && count < capacity)
                        output[count] = group;
                    ++count;
                }
        }
        u32 next;
        if (!reference(&editor->dat, polygon + 4, &next)) break;
        polygon = next;
    }
    return count;
}

bool nativeFighterModel_SetEnvelopeWeight(NativeFighterModelEditor* editor,
                                          u32 weight_offset, float weight)
{
    if (editor == NULL || !isfinite(weight) || weight < 0.0f ||
        weight > 1.0f || HSD_DatBytes(&editor->dat, weight_offset, 4) == NULL)
        return false;
    u32 bits;
    memcpy(&bits, &weight, sizeof(bits));
    write_u32(editor, weight_offset, bits);
    return true;
}

#endif
