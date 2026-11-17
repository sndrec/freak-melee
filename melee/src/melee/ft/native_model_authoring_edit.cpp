#include "native_model_authoring_internal.h"
#include <cstdlib>

namespace NativeModelAuthoring {
namespace {
struct Candidate {
    NativeFighterProject p;
    explicit Candidate(const NativeFighterProject& source) : p(source)
    {
        p.fighter_dat = p.animation_dat = p.costume_dat = p.asset_record = nullptr; p.authoring = nullptr;
    }
    ~Candidate() {std::free(p.fighter_dat); std::free(p.animation_dat); std::free(p.costume_dat); std::free(p.asset_record); std::free(p.authoring);}
    bool copy(const NativeFighterProject& source, Reply& r)
    {
        const uint8_t* input[] = {source.fighter_dat, source.animation_dat, source.costume_dat, source.asset_record};
        uint8_t** output[] = {&p.fighter_dat, &p.animation_dat, &p.costume_dat, &p.asset_record};
        const size_t sizes[] = {source.fighter_size, source.animation_size, source.costume_size, source.asset_record_size};
        for (unsigned i = 0; i < 4; ++i) {
            *output[i] = (uint8_t*)std::malloc(sizes[i]);
            if (!*output[i]) {r.fail("allocation_failed", "Could not stage the complete model edit."); return false;}
            std::memcpy(*output[i], input[i], sizes[i]);
        }
        if (source.authoring) {p.authoring = (NativeFighterAuthoring*)std::malloc(sizeof(*p.authoring));
            if (!p.authoring) {r.fail("allocation_failed", "Could not stage the model metadata."); return false;} *p.authoring = *source.authoring;}
        return true;
    }
    void publish(NativeFighterProject& destination)
    {
        std::swap(p.fighter_dat, destination.fighter_dat); std::swap(p.fighter_size, destination.fighter_size);
        std::swap(p.animation_dat, destination.animation_dat); std::swap(p.animation_size, destination.animation_size);
        std::swap(p.costume_dat, destination.costume_dat); std::swap(p.costume_size, destination.costume_size);
        std::swap(p.asset_record, destination.asset_record); std::swap(p.asset_record_size, destination.asset_record_size);
        std::swap(p.authoring, destination.authoring);
        destination.facts = p.facts; destination.rig_edit_revision = p.rig_edit_revision; destination.rig_edit_serial = p.rig_edit_serial;
    }
};
bool required(yyjson_val* args, const char* key, Reply& r)
{
    if (has(args, key)) return true;
    r.fail("missing_model_argument", std::string("Supply ") + key + " for this model operation."); return false;
}
bool rest(NativeFighterProject& p, yyjson_val* args, Reply& r)
{
    if (!required(args, "changes", r)) return false;
    std::unique_ptr<NativeFighterModelEditor> model; if (!open(p, model, r)) return false;
    bool selected[NATIVE_MODEL_MAX_JOINTS]{};
    size_t i, n; yyjson_val* change;
    yyjson_arr_foreach(yyjson_obj_get(args, "changes"), i, n, change) {
        const unsigned bone = number(change, "bone");
        if (bone >= model->joint_count || selected[bone]) {r.fail("invalid_bone_selection", "Use each inspected bone index once."); return false;}
        selected[bone] = true;
        const std::vector<uint8_t> before(p.costume_dat, p.costume_dat + p.costume_size);
        const char* names[] = {"rotation", "scale", "position"}; bool changed = false, supplied = false;
        for (unsigned c = 0; c < 3; ++c) if (auto* values = yyjson_obj_get(change, names[c])) {
            supplied = true;
            for (unsigned axis = 0; axis < 3; ++axis) {
                const float next = (float)yyjson_get_num(yyjson_arr_get(values, axis));
                const auto& joint = model->joints[bone]; const float old = c == 0 ? joint.rotation[axis] : c == 1 ? joint.scale[axis] : joint.position[axis];
                if (next == old) continue;
                if (!nativeFighterModel_SetJoint(model.get(), bone, c, axis, next)) {r.fail("rest_edit_rejected", "The rest transform exceeds the native position, rotation or scale range."); return false;}
                changed = true;
            }
        }
        if (!supplied) {r.fail("missing_transform", "Supply position, rotation or scale for each edited bone."); return false;}
        if (changed && !nativeFighterModel_CompensateRest(model.get(), before.data(), before.size(), bone)) {r.fail("rest_compensation_failed", "The transform could not preserve rigid/skinned geometry. The complete edit was rejected."); return false;}
    }
    return true;
}
bool import(NativeFighterProject& p, yyjson_val* args, Reply& r, bool reimport)
{
    if (!*text(args, "path")) {r.fail("missing_source", "Choose a GLB/glTF source file."); return false;}
    NativeGltfImportRequest requests[NATIVE_GLTF_MAX_PRIMITIVES]; unsigned count = 0;
    char error[512]{};
    if (reimport) {
        if (!required(args, "mesh", r)) return false;
        NativeGltfReimportPlan plan;
        if (!nativeFighterGltf_PlanReimport(&p, number(args, "mesh"), text(args, "path"), boolean(args, "whole_source"), &plan, error, sizeof(error))) {r.fail("reimport_plan_failed", error); return false;}
        for (unsigned i = 0; i < plan.count; ++i) {const auto& match = plan.matches[i];
            requests[count++] = {match.primitive, match.joint, match.mesh, has(args, "binding") ? binding(args) : NATIVE_GLTF_BIND_KEEP, match.scale, boolean(args, "replace_material")};}
    } else {
        if (!required(args, "imports", r)) return false;
        size_t i, n; yyjson_val* request;
        yyjson_arr_foreach(yyjson_obj_get(args, "imports"), i, n, request) {
            requests[count++] = {number(request, "primitive"), number(request, "bone"), has(request, "replace_mesh") ? number(request, "replace_mesh") : UINT32_MAX,
                binding(request), (float)real(request, "scale", 1), boolean(request, "replace_material")};
        }
    }
    if (!count || !nativeFighterGltf_ApplyBatch(&p, text(args, "path"), requests, count, error, sizeof(error))) {r.fail("model_import_failed", *error ? error : "No source primitives were selected."); return false;}
    yyjson_mut_obj_add_uint(r.document, r.result, "imported_meshes", count); return true;
}
}
bool edit(NativeFighterProject& project, yyjson_val* args, Reply& r, History* history)
{
    const char* op = text(args, "operation");
    const char* fields = !std::strcmp(op, "import") ? "|path|imports|" :
        !std::strcmp(op, "reimport") ? "|path|mesh|whole_source|binding|replace_material|" :
        !std::strcmp(op, "rest") ? "|changes|" : !std::strcmp(op, "material") ? "|material|properties|" :
        !std::strcmp(op, "paint") ? "|mesh|bone|centers|radius|strength|subtract|" :
        !std::strcmp(op, "rig") ? "|rig_operation|bone_id|parent_id|name|" :
        !std::strcmp(op, "label") ? "|category|id|name|" :
        !std::strcmp(op, "texture_image") ? "|material|path|layer|image|sequence|" : "|mesh|material|";
    size_t field_index, field_count; yyjson_val *key, *value;
    yyjson_obj_foreach(args, field_index, field_count, key, value) {
        const std::string token = "|" + std::string(yyjson_get_str(key)) + "|";
        if (!std::strstr("|project|member|instance|revision|summary|operation|", token.c_str()) &&
            !std::strstr(fields, token.c_str())) {
            r.fail("unused_model_argument", std::string(yyjson_get_str(key)) + " does not apply to this model operation."); return false;
        }
    }
    Candidate candidate(project); if (!candidate.copy(project, r)) return false;
    auto& p = candidate.p;
    NativeFighterCoreAssetState before; std::vector<uint8_t> names_before, names_after;
    if (!nativeFighterCoreAssets_Capture(project, before) || !encode_names(project, names_before, r)) {if (r.ok) r.fail("snapshot_failed", "Could not capture the original model assets."); return false;}
    bool ok = false; char error[512]{};
    if (!std::strcmp(op, "rest")) ok = rest(p, args, r);
    else if (!std::strcmp(op, "import") || !std::strcmp(op, "reimport")) ok = import(p, args, r, !std::strcmp(op, "reimport"));
    else if (!std::strcmp(op, "material")) {
        if (!required(args, "material", r) || !required(args, "properties", r)) return false;
        ok = edit_material(p, number(args, "material"), yyjson_obj_get(args, "properties"), r);
    } else if (!std::strcmp(op, "paint")) {
        for (const char* key : {"mesh", "bone", "centers", "radius", "strength"}) if (!required(args, key, r)) return false;
        std::vector<NativeSkinBrushPoint> centers;
        size_t i, n; yyjson_val* center;
        yyjson_arr_foreach(yyjson_obj_get(args, "centers"), i, n, center) {
            NativeSkinBrushPoint point{}; for (unsigned a = 0; a < 3; ++a) point.position[a] = (float)yyjson_get_num(yyjson_arr_get(center, a)); centers.push_back(point);
        }
        unsigned affected = 0;
        ok = nativeFighterSkin_Paint(&p, number(args, "mesh"), number(args, "bone"), centers.data(), (unsigned)centers.size(), (float)real(args, "radius"), (float)real(args, "strength"), boolean(args, "subtract"), &affected, error, sizeof(error));
        yyjson_mut_obj_add_uint(r.document, r.result, "affected_vertices", affected);
    } else if (!std::strcmp(op, "rig")) {
        if (!required(args, "rig_operation", r)) return false;
        NativeFighterRigEdit request{}; const char* kind = text(args, "rig_operation");
        request.kind = !std::strcmp(kind, "add") ? NativeRigEdit_Add : !std::strcmp(kind, "reparent") ? NativeRigEdit_Reparent : NativeRigEdit_Remove;
        if (request.kind != NativeRigEdit_Remove && !required(args, "parent_id", r)) return false;
        if (request.kind != NativeRigEdit_Add && !required(args, "bone_id", r)) return false;
        if (request.kind == NativeRigEdit_Add && !required(args, "name", r)) return false;
        request.bone_id = (int32_t)number(args, "bone_id"); request.parent_id = (int32_t)number(args, "parent_id");
        std::snprintf(request.name, sizeof(request.name), "%s", text(args, "name"));
        int32_t result = -1; ok = nativeFighterRig_Edit(&p, &request, &result, error, sizeof(error));
        if (ok) yyjson_mut_obj_add_int(r.document, r.result, "result_bone_id", result);
    } else {
        std::unique_ptr<NativeFighterModelEditor> model; if (!open(p, model, r)) return false;
        if (!std::strcmp(op, "mesh_material")) {
            if (!required(args, "mesh", r) || !required(args, "material", r)) return false;
            ok = nativeFighterModel_SetMeshMaterial(model.get(), number(args, "mesh"), number(args, "material"));
        } else if (!std::strcmp(op, "label")) {
            if (!required(args, "category", r) || !required(args, "id", r) || !required(args, "name", r)) return false;
            const unsigned id = number(args, "id"); const char* category = text(args, "category"); unsigned key = UINT32_MAX, type = 0;
            if (!std::strcmp(category, "bone") && id < model->joint_count) {key = model->joints[id].offset; type = NativeAuthoring_Bone;}
            else if (!std::strcmp(category, "mesh") && id < model->mesh_count) {key = model->meshes[id].offset; type = NativeAuthoring_Mesh;}
            else if (!std::strcmp(category, "material") && id < model->material_count) {key = model->materials[id].offset; type = NativeAuthoring_Material;}
            ok = key != UINT32_MAX && nativeFighterAuthoring_SetName(&p, type, key, text(args, "name"));
        } else if (!std::strcmp(op, "texture_image")) {
            if (!required(args, "material", r) || !*text(args, "path")) {if (r.ok) r.fail("missing_image", "Choose a PNG/JPEG image file."); return false;}
            const unsigned material = number(args, "material"); if (material >= model->material_count) {r.fail("unknown_material", "Choose a material from this member's material list."); return false;}
            unsigned layer = number(args, "layer"), frame = number(args, "image"); uint32_t sequence = 0;
            if (has(args, "sequence")) {
                std::vector<NativeMaterialTextureSequence> images; if (!sequences(*model, material, images, r)) return false;
                const unsigned selected = number(args, "sequence"); if (selected >= images.size() || frame >= images[selected].frames) {r.fail("unknown_texture_image", "Choose an inspected sequence and image index."); return false;}
                sequence = images[selected].offset; layer = images[selected].layer;
            }
            ok = nativeFighterGltf_SetTextureImage(&p, material, layer, sequence, frame, text(args, "path"), error, sizeof(error));
        }
    }
    if (!ok) {if (r.ok) r.fail("model_edit_rejected", *error ? error : "The edit exceeds a model reference, type, binding or native format limit."); return false;}
    if (!nativeFighterProject_ValidateAssets(&p, error, sizeof(error))) {r.fail("invalid_model_result", error); return false;}
    if (!encode_names(p, names_after, r)) return false;
    const bool changed = !nativeFighterCoreAssets_Matches(p, before) || names_before != names_after;
    yyjson_mut_obj_add_bool(r.document, r.result, "changed", changed); yyjson_mut_obj_add_bool(r.document, r.result, "saved", false); yyjson_mut_obj_add_bool(r.document, r.result, "installed", false);
    add_string(r, r.result, "preview_status", "refresh_required");
    if (auto* requested = yyjson_obj_get(args, "properties")) yyjson_mut_obj_add_val(r.document, r.result, "requested_properties", yyjson_val_mut_copy(r.document, requested));
    if (!std::strcmp(op, "material")) {
        std::unique_ptr<NativeFighterModelEditor> model; if (!open(p, model, r)) return false;
        auto* properties = yyjson_mut_obj(r.document); yyjson_mut_obj_add_val(r.document, r.result, "applied_properties", properties); material_properties(*model, number(args, "material"), r, properties);
    }
    if (changed) {
        // Reserve both sides of live Undo before publishing any part of the
        // candidate. Offline callers use their existing disk transaction.
        if (history) {
            history->before = std::move(before);
            if (!nativeFighterCoreAssets_Capture(p, history->after)) {
                r.fail("snapshot_failed", "Could not reserve model Redo. The draft is unchanged."); return false;
            }
            const auto capture_labels = [](const NativeFighterProject& source, std::vector<uint8_t>& bytes) {
                if (!source.authoring) {bytes.clear(); return;}
                const auto* begin = (const uint8_t*)source.authoring;
                bytes.assign(begin, begin + sizeof(*source.authoring));
            };
            capture_labels(project, history->before_labels);
            capture_labels(p, history->after_labels);
            history->labels_saved = true;
        }
        candidate.publish(project);
    }
    return true;
}
}
