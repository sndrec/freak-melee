#include "native_agent_result_schema.h"
#include <melee/ft/native_model_authoring.h>
#include <cstring>

namespace NativeAgent::ResultSchema {
namespace {
void saved_and_live(Catalog& c, const char* saved, const char* live, const Object& payload, bool mutation = false)
{
    c.add({saved}, saved_draft(c).merge(payload));
    auto result = mutation ? workshop_mutation(c) : workshop_state(c);
    c.add({live}, result.merge(payload));
}

Object page(Catalog& c)
{
    return c.object({{"total", "integer"}}).add("next_offset", c.nullable("integer"));
}

Object edit_receipt(Catalog& c)
{
    return c.object({{"changed", "boolean"}, {"saved", "boolean"}, {"installed", "boolean"}});
}

Object transform(Catalog& c)
{
    return c.object().add("position", c.array("number"))
        .add("rotation", c.array("number")).add("scale", c.array("number"));
}

Object clip(Catalog& c)
{
    return c.object({{"index", "integer"}}).add("bank", c.enumeration({"action", "demo"}));
}

yyjson_mut_val* action_values(Catalog& c)
{
    // The set of fields belongs to the opcode. Their numeric or boolean
    // descriptors and choices travel alongside the values in the same reply.
    return c.map(c.choice({c.type("number"), c.type("boolean")}));
}

yyjson_mut_val* action_fields(Catalog& c)
{
    return c.map(c.object({{"type", "string"}, {"readOnly", "boolean"}})
        .fields({{"description", "string"}, {"minimum", "number"}, {"maximum", "number"},
                 {"quantum", "number"}, {"expert", "boolean"}}, false)
        .add("choices", c.array(c.object({{"value", "integer"}, {"label", "string"}}).schema), false).schema);
}

Object event(Catalog& c)
{
    return c.object({{"event_id", "string"}, {"opcode", "integer"}, {"kind", "string"},
                     {"label", "string"}, {"description", "string"}, {"timer", "boolean"},
                     {"editable", "boolean"}, {"occurrences", "integer"}})
        .add("first_frame", c.nullable("integer"));
}

void actions(Catalog& c)
{
    const auto action_row = c.object({{"id", "integer"}, {"name", "string"}, {"has_events", "boolean"},
                                      {"hidden", "boolean"}}).fields({{"label", "string"}}, false);
    saved_and_live(c, "actions.list", "session.actions.list", page(c).add("actions", c.array(action_row.schema)));
    auto read = c.object()
        .fields({{"action_id", "integer"}, {"name", "string"}, {"label", "string"}, {"hidden", "boolean"},
                 {"animation_flags", "integer"}, {"loop", "boolean"}, {"root_motion", "boolean"},
                 {"has_animation", "boolean"}, {"command_count", "integer"}, {"occurrence_count", "integer"},
                 {"timeline_complete", "boolean"}, {"maximum_commands", "integer"}, {"maximum_occurrences", "integer"},
                 {"maximum_edit_frame", "integer"}, {"total", "integer"}, {"event_id", "string"},
                 {"opcode", "integer"}, {"kind", "string"}, {"description", "string"},
                 {"timer", "boolean"}, {"editable", "boolean"}}, false)
        .add("first_frame", c.nullable("integer"), false)
        .add("next_offset", c.nullable("integer"), false)
        .add("frames", c.array("integer"), false)
        .add("events", c.array(event(c).schema), false)
        .add("fields", action_fields(c), false)
        .add("properties", action_values(c), false)
        .add("types", c.array(c.object({{"kind", "string"}, {"label", "string"}, {"description", "string"}})
            .add("properties", action_values(c)).add("fields", action_fields(c)).schema), false)
        .add("occurrences", c.choice({c.type("integer"), c.array(
            c.object({{"event_id", "string"}, {"frame", "integer"}, {"opcode", "integer"}})
                .fields({{"duration_end_frame", "number"}}, false).schema)}), false);
    saved_and_live(c, "actions.read", "session.actions.read", read);
    auto edited = edit_receipt(c).fields({{"status", "string"}, {"preview_status", "string"}})
        .fields({{"converted_hitboxes", "integer"}, {"event_id", "string"}, {"hidden", "boolean"}, {"label", "string"}}, false)
        .add("updated_events", c.array(event(c).fields({{"previous_event_id", "string"}})
            .add("fields", action_fields(c)).add("properties", action_values(c))
            .add("requested", action_values(c)).schema), false);
    saved_and_live(c, "actions.edit", "session.actions.edit", edited, true);
}

void animations(Catalog& c)
{
    auto bone = transform(c).fields({{"id", "integer"}, {"name", "string"}, {"depth", "integer"}})
        .fields({{"role", "string"}}, false)
        .add("animation_joint", c.nullable("integer")).add("parent", c.nullable("integer"));
    auto read = c.object()
        .fields({{"animation_joints", "integer"}, {"binding_flags", "integer"}, {"frames", "number"},
                 {"maximum_keys_per_track", "integer"}, {"maximum_pose_copy_joints", "integer"},
                 {"maximum_selected_keys", "integer"}, {"maximum_tracks", "integer"}, {"name", "string"},
                 {"total", "integer"}, {"track_count", "integer"}, {"rig_bound", "boolean"},
                 {"frame", "number"}, {"space", "string"}}, false)
        .add("clip", clip(c).schema, false)
        .add("next_offset", c.nullable("integer"), false)
        .add("bones", c.array(bone.schema), false)
        .add("pose", c.array(yyjson_mut_val_mut_copy(c.document, bone.schema)), false)
        .add("clips", c.array(c.object({{"available", "boolean"}, {"name", "string"}})
            .fields({{"label", "string"}, {"frames", "number"}, {"tracks", "integer"}}, false)
            .add("clip", clip(c).schema).schema), false)
        .add("keys", c.array(c.object({{"frame", "integer"}, {"id", "integer"}, {"interpolation", "string"},
                        {"tangent", "number"}, {"value", "number"}, {"value_editable", "boolean"},
                        {"tangent_editable", "boolean"}}).schema), false)
        .add("samples", c.array(c.object({{"frame", "number"}})
            .add("value", c.nullable("number")).schema), false)
        .add("tracks", c.array(c.object({{"channel", "string"}, {"id", "integer"}, {"joint", "integer"},
                        {"keys", "integer"}, {"start_frame", "integer"}, {"unit", "string"}})
            .add("bone", c.nullable("integer")).fields({{"bone_name", "string"}}, false).schema), false);
    saved_and_live(c, "animations.read", "session.animations.read", read);
    const auto edited = edit_receipt(c)
        .fields({{"frames", "number"}, {"selected_keys", "integer"}, {"changed_keys", "integer"},
                 {"applied_keys_truncated", "boolean"}, {"preview_status", "string"}})
        .fields({{"inserted_keys", "integer"}, {"deleted_keys", "integer"}, {"written_channels", "integer"},
                 {"skipped_channels", "integer"}}, false)
        .add("clip", clip(c).schema)
        .add("applied_keys", c.array(c.object({{"track", "integer"}, {"key", "integer"}, {"frame", "integer"},
                                              {"value", "number"}, {"tangent", "number"}}).schema))
        .add("requested_keys", c.array(c.object({{"track", "integer"}, {"key", "integer"}})
            .fields({{"frame", "integer"}, {"value", "number"}, {"tangent", "number"}}, false).schema), false)
        .add("requested_transform", c.object()
            .fields({{"frame_offset", "number"}, {"pivot_frame", "number"}, {"pivot_value", "number"},
                     {"value_offset", "number"}, {"time_scale", "number"}, {"value_scale", "number"},
                     {"rotation", "number"}}, false).schema, false);
    saved_and_live(c, "animations.edit", "session.animations.edit", edited, true);
    c.add({"session.animations.preview"}, workshop_state(c).fields({{"frame", "number"}})
        .add("clip", clip(c).schema)
        .add("bones", c.array(transform(c).fields({{"id", "integer"}})
            .add("world_matrix", c.array("number")).schema)));
}

void resources(Catalog& c)
{
    const auto resource = c.object({{"name", "string"}, {"kind", "string"}, {"format", "string"},
                         {"sha256", "string"}, {"bytes", "integer"}, {"width", "integer"}, {"height", "integer"}});
    const auto read = page(c).fields({{"bundle_bytes", "integer"}, {"bundle_entries", "integer"}, {"effect_bank", "integer"}})
        .add("resources", c.array(resource.schema))
        .add("articles", c.array(c.object({{"declaration", "string"}, {"kind", "integer"}, {"slot", "integer"}}).schema))
        .add("sound_banks", c.array("integer"))
        .add("common_items", c.array(c.object({{"stock_kind", "integer"}, {"name", "string"}}).schema))
        .add("asset_formats", c.array(c.object({{"category", "string"}, {"authoring", "string"}}).schema))
        .add("limits", c.object({{"entries", "integer"}, {"name_bytes", "integer"}, {"bundle_bytes", "integer"},
                                 {"blob_bytes", "integer"}, {"png_bytes", "integer"}, {"png_dimension", "integer"}}).schema);
    saved_and_live(c, "resources.read", "session.resources.read", read);
    saved_and_live(c, "resources.edit", "session.resources.edit", edit_receipt(c)
        .fields({{"bundle_bytes", "integer"}, {"bundle_entries", "integer"}})
        .fields({{"preview_status", "string"}}, false), true);
    saved_and_live(c, "resources.export", "session.resources.export",
        c.object({{"name", "string"}, {"output", "string"}, {"bytes", "integer"}, {"sha256", "string"}}), true);
    c.add({"resources.blueprint"}, c.object({{"output", "string"}, {"bytes", "integer"}, {"sha256", "string"},
                                            {"article_offset", "integer"}, {"dat_bytes", "integer"},
                                            {"graph_validated", "boolean"}}));
}

void attributes(Catalog& c)
{
    saved_and_live(c, "attributes.list", "session.attributes.list", page(c).add("attributes", c.array(c.object({
        {"name", "string"}, {"label", "string"}, {"description", "string"}, {"group", "string"},
        {"type", "string"}, {"value", "number"}, {"minimum", "number"}, {"maximum", "number"},
        {"exclusive_minimum", "boolean"}}).schema)));
    saved_and_live(c, "attributes.edit", "session.attributes.edit", edit_receipt(c)
        .add("changes", c.array(c.object({{"name", "string"}, {"requested", "number"}, {"applied", "number"}}).schema))
        .fields({{"preview_status", "string"}}, false), true);
}

// A saved model can contain stock GX values outside the ranges accepted by
// the editor. Readback shares the edit field names and types, but reports
// the values in the asset.
yyjson_mut_val* model_values(Catalog& c, yyjson_val* descriptor)
{
    const char* type = yyjson_get_str(yyjson_obj_get(descriptor, "type"));
    if (auto* fields = yyjson_obj_get(descriptor, "properties")) {
        auto result = c.object();
        size_t index, count;
        yyjson_val *key, *value;
        yyjson_obj_foreach(fields, index, count, key, value)
            result.add(yyjson_mut_get_str(yyjson_val_mut_copy(c.document, key)), model_values(c, value));
        return result.schema;
    }
    if (auto* items = yyjson_obj_get(descriptor, "items")) {
        auto* schema = c.array(model_values(c, items));
        for (const char* key : {"minItems", "maxItems"})
            if (auto* length = yyjson_obj_get(descriptor, key))
                yyjson_mut_obj_add_val(c.document, schema, key, yyjson_val_mut_copy(c.document, length));
        return schema;
    }
    return c.type(type);
}

void models(Catalog& c)
{
    c.add({"models.scan"}, c.object({{"maximum_primitives", "integer"}})
        .add("primitives", c.array(c.object({{"id", "integer"}, {"name", "string"}, {"identity", "string"},
            {"vertices", "integer"}, {"triangles", "integer"}, {"has_normals", "boolean"},
            {"has_uv", "boolean"}, {"has_skin", "boolean"}, {"has_texture", "boolean"}}).schema)));
    const char* schema = NativeModelAuthoring::edit_schema(false);
    auto* input = yyjson_read(schema, std::strlen(schema), 0);
    auto* fields = yyjson_obj_get(yyjson_doc_get_root(input), "properties");
    auto* material = yyjson_obj_get(fields, "properties");
    auto* texture = yyjson_obj_get(fields, "texture");
    auto* material_values = model_values(c, material);
    auto* texture_values = model_values(c, texture);
    const auto copy = [&](yyjson_mut_val* value) { return yyjson_mut_val_mut_copy(c.document, value); };
    auto mesh = c.object({{"id", "integer"}, {"name", "string"}, {"bone", "integer"}, {"material", "integer"},
                         {"polygon_groups", "integer"}, {"imported", "boolean"}})
        .fields({{"source_path", "string"}, {"source_identity", "string"}, {"binding", "string"}, {"scale", "number"}}, false);
    auto info = c.object({{"bones", "integer"}, {"materials", "integer"}, {"meshes", "integer"},
                          {"maximum_bones", "integer"}, {"maximum_meshes", "integer"},
                          {"maximum_materials", "integer"}, {"maximum_vertex_influences", "integer"},
                          {"position_space", "string"}, {"rotation_unit", "string"}});
    const auto field_descriptor = [&] {
        return c.object({{"type", "string"}, {"additionalProperties", "boolean"}})
            .add("properties", c.map(Object(c.document, false).fields({{"type", "string"}}).schema));
    };
    info.add("material_fields", field_descriptor().schema).add("texture_fields", field_descriptor().schema);
    auto bones = page(c).add("bones", c.array(transform(c).fields({{"id", "integer"}, {"bone_id", "integer"},
        {"parent", "integer"}, {"name", "string"}, {"extra", "boolean"}, {"flags", "integer"}, {"depth", "integer"}})
        .fields({{"role", "string"}}, false).schema));
    auto materials = page(c).add("materials", c.array(c.object({{"id", "integer"}, {"name", "string"},
        {"surface_editable", "boolean"}}).add("properties", copy(material_values)).schema));
    auto textures = page(c).add("textures", c.array(c.object({{"id", "integer"}, {"animation_id_editable", "boolean"}})
        .add("properties", copy(texture_values)).schema));
    auto sequences = page(c).add("texture_sequences", c.array(c.object({{"id", "integer"},
        {"animation_id", "integer"}, {"images", "integer"}}).add("layer", c.nullable("integer")).schema));
    auto vertices = page(c).fields({{"corner_count", "integer"}})
        .add("vertices", c.array(c.object({{"id", "integer"}}).add("position", c.array("number"))
            .add("influences", c.array(c.object({{"bone", "integer"}, {"weight", "number"}}).schema)).schema));
    auto reimport = c.object().add("matches", c.array(c.object({{"primitive", "integer"}, {"mesh", "integer"},
        {"bone", "integer"}, {"scale", "number"}}).schema));
    const auto reads = {info, bones, materials, textures, sequences, vertices, reimport,
                        page(c).add("meshes", c.array(mesh.schema)), page(c).add("sources", c.array(copy(mesh.schema)))};
    auto saved = saved_draft(c);
    auto live = workshop_state(c);
    auto* saved_views = yyjson_mut_arr(c.document);
    auto* live_views = yyjson_mut_arr(c.document);
    for (const auto& view : reads) {
        yyjson_mut_arr_append(saved_views, c.object().merge(saved).merge(view).schema);
        yyjson_mut_arr_append(live_views, c.object().merge(live).merge(view)
            .fields({{"request_id", "string"}}, false).schema);
    }
    auto* saved_read = c.type("object");
    auto* live_read = c.type("object");
    yyjson_mut_obj_add_val(c.document, saved_read, "anyOf", saved_views);
    yyjson_mut_obj_add_val(c.document, live_read, "anyOf", live_views);
    c.add("models.read", saved_read);
    c.add("session.models.read", live_read);
    auto edited = edit_receipt(c).fields({{"preview_status", "string"}})
        .fields({{"imported_meshes", "integer"}, {"affected_vertices", "integer"}, {"result_bone_id", "integer"}}, false)
        .add("requested_properties", yyjson_val_mut_copy(c.document, material), false)
        .add("requested_texture", yyjson_val_mut_copy(c.document, texture), false)
        .add("applied_properties", material_values, false)
        .add("applied_texture", texture_values, false);
    yyjson_doc_free(input);
    saved_and_live(c, "models.edit", "session.models.edit", edited, true);
}
}

void add_editing(Catalog& c)
{
    actions(c);
    animations(c);
    resources(c);
    attributes(c);
    models(c);
}
}
