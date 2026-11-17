#include "native_fighter_model_editor.h"

#include "native_fighter_authoring.h"
#include "native_fighter_rig.h"

#ifdef MELEE_NATIVE

#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/pobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/tobj.h>
#include <sysdolphin/baselib/displayfunc.h>

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
            for (;;) {
                if (mesh->polygon_count == NATIVE_MODEL_MAX_POLYGONS) return false;
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
    NativeFighterRig rig;
    HSD_Dat fighter;
    bool named_rig = project->fighter_dat &&
        HSD_DatOpen(&fighter, project->fighter_dat, project->fighter_size) == HSD_DAT_OK &&
        HSD_DatFindRoot(&fighter, NATIVE_RIG_ROOT, &root) &&
        nativeFighterRig_Read(&fighter, project->facts.joints + project->facts.omission_count,
                              &rig, NULL, 0);
    uint16_t physical[NATIVE_MODEL_MAX_JOINTS];
    for (unsigned i = 0, part = 0; i < editor->joint_count; ++i, ++part) {
        bool omission;
        do {
            omission = false;
            for (unsigned n = 0; n < project->facts.omission_count; ++n)
                if (project->facts.omissions[n].joint == part) { omission = true; ++part; break; }
        } while (omission);
        physical[i] = (uint16_t)part;
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
            } else if (category == 0 && named_rig && physical[i] < MAX_FT_PARTS && rig.names[physical[i]][0]) {
                snprintf(target, 64, "%s", rig.names[physical[i]]);
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

bool nativeFighterModel_MaterialInputs(const NativeFighterModelEditor* editor,
                                      unsigned mesh, unsigned material,
                                      u32 render_mode)
{
    if (!editor || mesh >= editor->mesh_count || material >= editor->material_count) return false;
    u32 required = 0, texture = editor->materials[material].texture_offset;
    for (unsigned layer = 0; texture && layer < 8; ++layer) {
        u32 flags, source;
        if (!HSD_DatReadU32(&editor->dat, texture + 64, &flags) ||
            !HSD_DatReadU32(&editor->dat, texture + 12, &source)) return false;
        if ((render_mode & (RENDER_TEX0 << layer)) &&
            (flags & TEX_COORD_MASK) == TEX_COORD_UV &&
            source >= GX_TG_TEX0 && source <= GX_TG_TEX7)
            required |= 1u << (GX_VA_TEX0 + source - GX_TG_TEX0);
        if (!reference(&editor->dat, texture + 4, &texture)) texture = 0;
    }
    if (!required) return true;
    u32 polygon;
    if (!reference(&editor->dat, editor->meshes[mesh].offset + 12, &polygon)) return false;
    for (unsigned p = 0; p < editor->meshes[mesh].polygon_count; ++p) {
        u32 descriptors, present = 0;
        if (!reference(&editor->dat, polygon + 8, &descriptors)) return false;
        for (unsigned a = 0; a < 27; ++a) {
            u32 attr, type;
            if (!HSD_DatReadU32(&editor->dat, descriptors + a * 24, &attr)) return false;
            if (attr == GX_VA_NULL) break;
            if (!HSD_DatReadU32(&editor->dat, descriptors + a * 24 + 4, &type)) return false;
            if (attr < 32 && type != GX_NONE) present |= 1u << attr;
        }
        if ((present & required) != required) return false;
        if (p + 1 < editor->meshes[mesh].polygon_count &&
            !reference(&editor->dat, polygon + 4, &polygon)) return false;
    }
    return true;
}

void nativeFighterModel_MaterialJointFlags(const NativeFighterModelEditor* editor,
    unsigned material, u32 render_mode, u32 flags[NATIVE_MODEL_MAX_JOINTS])
{
    bool affected[NATIVE_MODEL_MAX_JOINTS] = {0};
    u32 passes[NATIVE_MODEL_MAX_JOINTS] = {0};
    for (unsigned m = 0; m < editor->mesh_count; ++m) {
        const NativeModelMesh* mesh = &editor->meshes[m];
        if (mesh->material == material) affected[mesh->joint] = true;
        if (mesh->material >= editor->material_count) continue;
        const u32 mode = mesh->material == material ? render_mode : editor->materials[mesh->material].render_mode;
        passes[mesh->joint] |= mode & RENDER_XLU ? (mode & RENDER_NO_ZUPDATE ? JOBJ_TEXEDGE : JOBJ_XLU) : JOBJ_OPA;
    }
    for (unsigned j = 0; j < editor->joint_count; ++j) {
        flags[j] = editor->joints[j].flags;
        if (affected[j]) flags[j] = (flags[j] & ~(JOBJ_OPA | JOBJ_XLU | JOBJ_TEXEDGE)) | passes[j];
    }
    for (unsigned j = 0; j < editor->joint_count; ++j) if (affected[j])
        for (int parent = editor->joints[j].parent; parent >= 0; parent = editor->joints[parent].parent)
            flags[parent] |= passes[j] << 10;
}

bool nativeFighterModel_SetMeshMaterial(NativeFighterModelEditor* editor,
                                       unsigned mesh, unsigned material)
{
    if (editor == NULL || mesh >= editor->mesh_count ||
        material >= editor->material_count ||
        !nativeFighterModel_MaterialInputs(editor, mesh, material,
                                           editor->materials[material].render_mode)) return false;
    u32 target;
    if (!reference(&editor->dat, editor->meshes[mesh].offset + 8,
                   &target)) return false;
    write_u32(editor, editor->meshes[mesh].offset + 8,
              editor->materials[material].offset);
    editor->meshes[mesh].material = (uint16_t) material;
    u32 flags[NATIVE_MODEL_MAX_JOINTS];
    nativeFighterModel_MaterialJointFlags(editor, material,
                                         editor->materials[material].render_mode, flags);
    for (unsigned j = 0; j < editor->joint_count; ++j) if (flags[j] != editor->joints[j].flags) {
        write_u32(editor, editor->joints[j].offset + 4, flags[j]);
        editor->joints[j].flags = flags[j];
    }
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
    for (unsigned p = 0; p < editor->meshes[mesh].polygon_count; ++p) {
        u16 flags = 0;
        if (!HSD_DatReadU16(&editor->dat, polygon + 12, &flags)) break;
        if ((flags & 0x3000) == POBJ_ENVELOPE || (flags & 0x3000) == POBJ_SEPARATE_ENVELOPES) {
            u32 table;
            bool valid = reference(&editor->dat, polygon + 20, &table);
            if (valid && (flags & 0x3000) == POBJ_SEPARATE_ENVELOPES)
                valid = reference(&editor->dat, table, &table);
            if (valid)
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
