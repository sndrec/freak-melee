#include "native_model_authoring_internal.h"

namespace NativeModelAuthoring {
void vector(Reply& r, yyjson_mut_val* row, const char* key, const float* value, unsigned count)
{
    auto* array = yyjson_mut_arr(r.document); yyjson_mut_obj_add_val(r.document, row, key, array);
    for (unsigned i = 0; i < count; ++i) yyjson_mut_arr_add_real(r.document, array, value[i]);
}
bool matches(const char* name, const char* query)
{
    std::string a = name, b = query;
    const auto lower = [](char c) {return c >= 'A' && c <= 'Z' ? c + 32 : c;};
    std::transform(a.begin(), a.end(), a.begin(), lower); std::transform(b.begin(), b.end(), b.begin(), lower);
    return a.find(b) != std::string::npos;
}
void page(Reply& r, size_t total, unsigned offset, unsigned limit)
{
    yyjson_mut_obj_add_uint(r.document, r.result, "total", total);
    if (offset + limit < total) yyjson_mut_obj_add_uint(r.document, r.result, "next_offset", offset + limit);
    else yyjson_mut_obj_add_null(r.document, r.result, "next_offset");
}
bool open(const NativeFighterProject& p, std::unique_ptr<NativeFighterModelEditor>& model, Reply& r)
{
    model = std::make_unique<NativeFighterModelEditor>();
    if (!nativeFighterModel_Open(model.get(), const_cast<NativeFighterProject*>(&p))) {r.fail("model_unavailable", "Could not read this member's owned model."); return false;}
    return true;
}
bool bone_ids(const NativeFighterProject& p, unsigned count, std::vector<int32_t>& ids, std::vector<uint8_t>& extra, Reply& r)
{
    HSD_Dat dat; NativeFighterRig rig; char error[256];
    if (HSD_DatOpen(&dat, p.fighter_dat, p.fighter_size) != HSD_DAT_OK ||
        !nativeFighterRig_Read(&dat, p.facts.joints + p.facts.omission_count, &rig, error, sizeof(error))) {
        r.fail("rig_unavailable", "Could not read the fighter's stable bone identities."); return false;
    }
    ids.clear(); extra.clear();
    for (unsigned part = 0; part < p.facts.joints + p.facts.omission_count; ++part) {
        bool omitted = false;
        for (unsigned i = 0; i < p.facts.omission_count; ++i) omitted |= p.facts.omissions[i].joint == part;
        if (omitted) continue;
        ids.push_back(nativeFighterRig_Id(&rig, part)); extra.push_back((rig.flags[part] & NATIVE_RIG_EXTRA) != 0);
    }
    if (ids.size() != count) {r.fail("rig_mismatch", "The model and fighter bone counts disagree."); return false;}
    return true;
}
const char* binding_name(unsigned value)
{
    const char* names[] = {"rigid", "automatic", "transfer", "source", "keep"};
    return value < std::size(names) ? names[value] : "unknown";
}
NativeGltfBindingMode binding(yyjson_val* args)
{
    const char* value = text(args, "binding");
    for (unsigned i = 0; i < 5; ++i) if (!std::strcmp(value, binding_name(i))) return (NativeGltfBindingMode)i;
    return NATIVE_GLTF_BIND_AUTOMATIC;
}
bool encode_names(const NativeFighterProject& p, std::vector<uint8_t>& bytes, Reply& r)
{
    uint8_t* data = nullptr; size_t size = 0;
    if (!nativeFighterAuthoring_Encode(&p, &data, &size)) {r.fail("metadata_unavailable", "Could not encode model labels and source metadata."); return false;}
    bytes.clear(); if (size) bytes.assign(data, data + size); std::free(data); return true;
}
bool sequences(const NativeFighterModelEditor& model, unsigned material, std::vector<NativeMaterialTextureSequence>& output, Reply& r)
{
    for (unsigned capacity = 64; capacity <= 32768; capacity *= 2) {
        output.resize(capacity);
        const unsigned count = nativeFighterMaterial_Sequences(&model, material, output.data(), capacity);
        if (count < capacity) {output.resize(count); return true;}
    }
    r.fail("sequence_limit", "The texture sequence list exceeds the 32768-entry inspection capacity."); return false;
}
void scan(yyjson_val* args, Reply& r)
{
    NativeGltfScan result; char error[512]{};
    if (!nativeFighterGltf_Scan(text(args, "path"), &result, error, sizeof(error))) {r.fail("model_scan_failed", error); return;}
    auto* rows = yyjson_mut_arr(r.document); yyjson_mut_obj_add_val(r.document, r.result, "primitives", rows);
    for (unsigned i = 0; i < result.count; ++i) {
        const auto& primitive = result.primitives[i]; auto* row = yyjson_mut_obj(r.document); yyjson_mut_arr_append(rows, row);
        yyjson_mut_obj_add_uint(r.document, row, "id", i); add_string(r, row, "name", primitive.name); add_string(r, row, "identity", primitive.identity);
        yyjson_mut_obj_add_uint(r.document, row, "vertices", primitive.vertices); yyjson_mut_obj_add_uint(r.document, row, "triangles", primitive.triangles);
        yyjson_mut_obj_add_bool(r.document, row, "has_normals", primitive.has_normals); yyjson_mut_obj_add_bool(r.document, row, "has_uv", primitive.has_uv);
        yyjson_mut_obj_add_bool(r.document, row, "has_skin", primitive.has_skin); yyjson_mut_obj_add_bool(r.document, row, "has_texture", primitive.has_texture);
    }
    yyjson_mut_obj_add_uint(r.document, r.result, "maximum_primitives", NATIVE_GLTF_MAX_PRIMITIVES);
}
void read(const NativeFighterProject& p, yyjson_val* args, Reply& r)
{
    std::unique_ptr<NativeFighterModelEditor> model;
    if (!open(p, model, r)) return;
    const char* view = text(args, "view");
    if (!std::strcmp(view, "info")) {
        yyjson_mut_obj_add_uint(r.document, r.result, "bones", model->joint_count); yyjson_mut_obj_add_uint(r.document, r.result, "meshes", model->mesh_count);
        yyjson_mut_obj_add_uint(r.document, r.result, "materials", model->material_count);
        yyjson_mut_obj_add_uint(r.document, r.result, "maximum_bones", NATIVE_MODEL_MAX_JOINTS);
        yyjson_mut_obj_add_uint(r.document, r.result, "maximum_meshes", NATIVE_MODEL_MAX_MESHES);
        yyjson_mut_obj_add_uint(r.document, r.result, "maximum_materials", NATIVE_MODEL_MAX_MATERIALS);
        yyjson_mut_obj_add_uint(r.document, r.result, "maximum_vertex_influences", NATIVE_SKIN_MAX_INFLUENCES);
        add_string(r, r.result, "position_space", "bind_space_game_units"); add_string(r, r.result, "rotation_unit", "radians");
        auto* schema = yyjson_read(material_schema(), std::strlen(material_schema()), 0);
        yyjson_mut_obj_add_val(r.document, r.result, "material_fields", yyjson_val_mut_copy(r.document, yyjson_doc_get_root(schema))); yyjson_doc_free(schema);
        return;
    }
    const unsigned offset = number(args, "offset"), limit = has(args, "limit") ? number(args, "limit") : 50;
    const unsigned mesh = number(args, "mesh"), material = number(args, "material");
    if ((!std::strcmp(view, "vertices") || !std::strcmp(view, "reimport_plan")) && (!has(args, "mesh") || mesh >= model->mesh_count)) {r.fail("unknown_mesh", "Choose a mesh from this member's mesh list."); return;}
    if (!std::strcmp(view, "reimport_plan")) {
        NativeGltfReimportPlan plan; char error[512]{};
        if (!*text(args, "path") || !nativeFighterGltf_PlanReimport(&p, mesh, text(args, "path"), boolean(args, "whole_source"), &plan, error, sizeof(error))) {r.fail("reimport_plan_failed", *error ? error : "Choose a source model file."); return;}
        auto* rows = yyjson_mut_arr(r.document); yyjson_mut_obj_add_val(r.document, r.result, "matches", rows);
        for (unsigned i = 0; i < plan.count; ++i) {auto* row = yyjson_mut_obj(r.document); yyjson_mut_arr_append(rows, row);
            yyjson_mut_obj_add_uint(r.document, row, "primitive", plan.matches[i].primitive); yyjson_mut_obj_add_uint(r.document, row, "mesh", plan.matches[i].mesh);
            yyjson_mut_obj_add_uint(r.document, row, "bone", plan.matches[i].joint); yyjson_mut_obj_add_real(r.document, row, "scale", plan.matches[i].scale);}
        return;
    }
    auto* rows = yyjson_mut_arr(r.document); yyjson_mut_obj_add_val(r.document, r.result, view, rows);
    size_t total = 0;
    if (!std::strcmp(view, "vertices")) {
        unsigned corners; const unsigned count = nativeFighterSkin_Vertices(&p, mesh, nullptr, 0, nullptr, 0, &corners);
        if (!count || count > NATIVE_MESH_SOURCE_MAX_VERTICES) {r.fail("vertices_unavailable", "Could not decode this mesh's vertices within the supported capacity."); return;}
        std::vector<NativeSkinVertex> vertices(count);
        if (nativeFighterSkin_Vertices(&p, mesh, vertices.data(), count, nullptr, 0, &corners) != count) {r.fail("vertices_unavailable", "The mesh's vertex read failed."); return;}
        for (size_t i = offset; i < count && yyjson_mut_arr_size(rows) < limit; ++i) {
            const auto& vertex = vertices[i]; auto* row = yyjson_mut_obj(r.document); yyjson_mut_arr_append(rows, row);
            yyjson_mut_obj_add_uint(r.document, row, "id", i); vector(r, row, "position", vertex.position);
            auto* influences = yyjson_mut_arr(r.document); yyjson_mut_obj_add_val(r.document, row, "influences", influences);
            for (unsigned j = 0; j < vertex.skin.count; ++j) {auto* weight = yyjson_mut_obj(r.document); yyjson_mut_arr_append(influences, weight); yyjson_mut_obj_add_uint(r.document, weight, "bone", vertex.skin.joints[j]); yyjson_mut_obj_add_real(r.document, weight, "weight", vertex.skin.weights[j]);}
        }
        yyjson_mut_obj_add_uint(r.document, r.result, "corner_count", corners); total = count;
    } else if (!std::strcmp(view, "texture_sequences")) {
        if (!has(args, "material") || material >= model->material_count) {r.fail("unknown_material", "Choose a material from this member's material list."); return;}
        std::vector<NativeMaterialTextureSequence> images;
        if (!sequences(*model, material, images, r)) return;
        for (size_t i = offset; i < images.size() && yyjson_mut_arr_size(rows) < limit; ++i) {auto* row = yyjson_mut_obj(r.document); yyjson_mut_arr_append(rows, row); yyjson_mut_obj_add_uint(r.document, row, "id", i); yyjson_mut_obj_add_uint(r.document, row, "layer", images[i].layer); yyjson_mut_obj_add_uint(r.document, row, "images", images[i].frames);}
        total = images.size();
    } else if (!std::strcmp(view, "bones")) {
        std::vector<int32_t> ids; std::vector<uint8_t> extra;
        if (!bone_ids(p, model->joint_count, ids, extra, r)) return;
        for (unsigned i = 0; i < model->joint_count; ++i) {
            const auto& bone = model->joints[i]; if (!matches(bone.name, text(args, "query"))) continue;
            if (total++ < offset || yyjson_mut_arr_size(rows) >= limit) continue;
            auto* row = yyjson_mut_obj(r.document); yyjson_mut_arr_append(rows, row); yyjson_mut_obj_add_uint(r.document, row, "id", i);
            yyjson_mut_obj_add_int(r.document, row, "bone_id", ids[i]); yyjson_mut_obj_add_int(r.document, row, "parent", bone.parent);
            add_string(r, row, "name", bone.name); yyjson_mut_obj_add_bool(r.document, row, "extra", extra[i]);
            yyjson_mut_obj_add_uint(r.document, row, "flags", bone.flags); yyjson_mut_obj_add_uint(r.document, row, "depth", bone.depth);
            if (const char* role = nativeFighterAuthoring_BoneRole(&p, i)) add_string(r, row, "role", role);
            vector(r, row, "position", bone.position); vector(r, row, "rotation", bone.rotation); vector(r, row, "scale", bone.scale);
        }
    } else if (!std::strcmp(view, "meshes") || !std::strcmp(view, "sources")) {
        for (unsigned i = 0; i < model->mesh_count; ++i) {
            const auto& mesh = model->meshes[i]; const auto* source = nativeFighterMeshSource_Get(&p, mesh.offset);
            const bool sources = !std::strcmp(view, "sources"); if (sources && !source) continue;
            if (!matches(mesh.name, text(args, "query"))) continue;
            if (total++ < offset || yyjson_mut_arr_size(rows) >= limit) continue;
            auto* row = yyjson_mut_obj(r.document); yyjson_mut_arr_append(rows, row); yyjson_mut_obj_add_uint(r.document, row, "id", i); add_string(r, row, "name", mesh.name);
            yyjson_mut_obj_add_uint(r.document, row, "bone", mesh.joint); yyjson_mut_obj_add_uint(r.document, row, "material", mesh.material);
            yyjson_mut_obj_add_uint(r.document, row, "polygon_groups", mesh.polygon_count); yyjson_mut_obj_add_bool(r.document, row, "imported", source != nullptr);
            if (source) {add_string(r, row, "source_path", source->path); add_string(r, row, "source_identity", source->identity); add_string(r, row, "binding", binding_name(source->binding)); yyjson_mut_obj_add_real(r.document, row, "scale", source->scale);}
        }
    } else if (!std::strcmp(view, "materials")) {
        for (unsigned i = 0; i < model->material_count; ++i) {
            const auto& mat = model->materials[i]; if (!matches(mat.name, text(args, "query"))) continue;
            if (total++ < offset || yyjson_mut_arr_size(rows) >= limit) continue;
            auto* row = yyjson_mut_obj(r.document); yyjson_mut_arr_append(rows, row); yyjson_mut_obj_add_uint(r.document, row, "id", i); add_string(r, row, "name", mat.name);
            yyjson_mut_obj_add_bool(r.document, row, "surface_editable", mat.color_offset != 0);
            auto* properties = yyjson_mut_obj(r.document); yyjson_mut_obj_add_val(r.document, row, "properties", properties); material_properties(*model, i, r, properties);
        }
    }
    page(r, total, offset, limit);
}
}
