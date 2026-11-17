#include "native_agent_result_schema.h"

namespace NativeAgent::ResultSchema {
namespace {
Object model_view(Catalog& c)
{
    auto pose = c.object({{"bone", "integer"}})
        .add("position", c.array("number")).add("rotation", c.array("number")).add("scale", c.array("number"));
    return c.object({{"context", "string"}, {"mode", "string"}, {"visible", "boolean"}, {"mesh", "integer"},
                     {"material", "integer"}, {"active_bone", "integer"}, {"action_clip", "integer"},
                     {"frame", "integer"}, {"playing", "boolean"}, {"bones_visible", "boolean"},
                     {"bones_drawn", "boolean"}, {"pose_initialized", "boolean"}})
        .add("bones", c.array("integer")).add("hidden_meshes", c.array("integer"))
        .add("temporary_pose_bones", c.array("integer"))
        .add("temporary_pose", c.array(pose.schema), false);
}

Object preview_view(Catalog& c)
{
    auto camera = c.object({{"yaw", "number"}, {"pitch", "number"}, {"distance", "number"}})
        .add("target", c.array("number"));
    auto pose = c.object({{"bank", "string"}, {"index", "integer"}, {"frame", "number"}});
    return c.object({{"bones", "boolean"}, {"hitboxes", "boolean"}, {"hurtboxes", "boolean"},
                     {"playing", "boolean"}, {"viewport_visible", "boolean"}})
        .add("camera", c.choice({camera.schema, c.type("null")}))
        .add("pose", c.choice({pose.schema, c.type("null")}));
}

Object live_build(Catalog& c)
{
    auto member = script_validation(c).merge(source_status(c)).fields({{"key", "string"}, {"member", "string"}});
    auto failure = c.object({{"key", "string"}, {"code", "string"}, {"message", "string"}});
    return workshop_mutation(c)
        .fields({{"accepted", "boolean"}, {"diagnostic_count", "integer"}, {"draft_view", "string"}})
        .add("member_reports", c.array(c.choice({member.schema, failure.schema})))
        .fields({{"package_bytes", "integer"}, {"package_id", "string"}, {"saved", "boolean"},
                 {"installed", "boolean"}, {"cached", "boolean"}, {"status", "string"}}, false);
}

Object practice_state(Catalog& c)
{
    return c.object({{"test_id", "integer"}, {"test_revision", "integer"}, {"frame", "integer"},
                     {"target_frame", "integer"}, {"oldest_frame", "integer"}, {"seed", "integer"},
                     {"fighter_revision", "string"}, {"package_id", "string"}})
        .add("status", c.enumeration({"closed", "opening", "ready", "running", "checkpoint_pending", "failed"}))
        .add("checkpoint_frame", c.nullable("integer"))
        .fields({{"message", "string"}, {"capture_id", "string"}}, false);
}

void practice(Catalog& c)
{
    auto state = practice_state(c);
    c.add({"session.practice.state", "session.practice.run", "session.practice.checkpoint", "session.practice.reset"}, state);
    c.add({"session.practice.assert"}, practice_state(c).fields({{"passed", "boolean"}})
        .add("checks", c.array(c.object({{"predicate", "string"}, {"passed", "boolean"}, {"from", "integer"},
                         {"through", "integer"}, {"observed", "integer"}})
            .fields({{"first_failure_frame", "integer"}}, false).schema)));

    auto actor = c.object({{"port", "integer"}, {"actor", "integer"}, {"runtime_kind", "integer"},
                           {"action_id", "integer"}, {"airborne", "boolean"}})
        .add("hitboxes", c.array("integer")).fields({{"script_state", "string"}}, false);
    for (const char* field : {"animation_frame", "damage", "x", "y", "vx", "vy", "facing", "hitlag"})
        actor.add(field, c.nullable("number"));
    auto item = c.object({{"kind", "integer"}, {"state_id", "integer"}, {"owner_actor", "integer"}});
    for (const char* field : {"x", "y", "vx", "vy"}) item.add(field, c.nullable("number"));
    auto hit = c.object({{"item_kind", "integer"}, {"attacker_actor", "integer"}, {"victim_actor", "integer"}})
        .add("damage", c.nullable("number"));
    auto fault = c.object({{"source_kind", "string"}, {"message", "string"}, {"source_generation", "integer"},
                           {"line", "integer"}, {"column", "integer"}});
    auto frame = c.object({{"frame", "integer"}, {"rng", "integer"}, {"items", "integer"},
                           {"owned_items", "integer"}, {"item_hit_count", "integer"}, {"script_fault_count", "integer"}})
        .add("fighters", c.array(actor.schema)).add("live_items", c.array(item.schema))
        .add("item_hits", c.array(hit.schema)).add("script_faults", c.array(fault.schema));
    c.add({"session.practice.trace"}, practice_state(c).add("frames", c.array(frame.schema)));
    c.add({"session.practice.start"}, live_build(c).fields({{"test_id", "integer"}}, false));
}

void presentation(Catalog& c)
{
    auto inner = c.object({{"project", "string"}, {"revision", "string"}, {"status", "string"}})
        .fields({{"saved_disk_revision", "string"}, {"target", "string"}, {"tile_name", "string"}, {"subjects", "integer"},
                 {"member_index", "integer"}, {"message", "string"}, {"animation", "string"}, {"frame", "number"},
                 {"last_frame", "number"}, {"total_animations", "integer"}, {"animation_offset", "integer"}}, false)
        .add("animations", c.array("string"), false)
        .add("camera", c.object({{"yaw", "number"}, {"pitch", "number"}, {"zoom", "number"},
                                 {"pan_x", "number"}, {"pan_y", "number"}}).schema, false)
        .add("capture", c.object({{"id", "integer"}, {"status", "string"}, {"target", "string"}}).schema, false);
    c.add({"session.presentation.inspect", "session.presentation.configure", "session.presentation.capture"}, inner);
    c.add({"session.presentation.open"}, workshop_mutation(c).fields({{"status", "string"}}));
    c.add({"session.presentation.place"}, workshop_mutation(c).fields({{"placed", "boolean"}}));
    c.add({"session.presentation.close"}, c.object({{"status", "string"}}));
}

void feedback(Catalog& c)
{
    auto captured = c.object()
        .fields({{"revision", "string"}, {"member", "string"}, {"workspace", "string"}, {"selected_source", "string"},
                 {"selected_action", "integer"}, {"frame", "integer"}, {"source_revision", "string"},
                 {"build_identity", "string"}, {"build", "string"}, {"saved_disk_revision", "string"},
                 {"preview_ready", "boolean"}, {"testing", "boolean"}}, false);
    for (const char* field : {"fighter_revision", "loaded_preview_package_id", "preview_fighter_revision",
                              "built_package_id", "installed_package_id"})
        captured.add(field, c.nullable("string"), false);
    auto authored = c.object({{"author", "string"}, {"task_id", "string"}, {"issue_key", "string"},
                              {"category", "string"}, {"summary", "string"}, {"operation", "string"},
                              {"expected", "string"}, {"observed", "string"}, {"reproduction", "string"},
                              {"diagnostics", "string"}, {"capability", "string"}, {"alternatives", "string"}})
        .add("captured_workshop", captured.schema);
    auto report = c.object({{"id", "string"}, {"capture_at", "string"}, {"capture_id", "integer"},
                            {"displayed_frame", "integer"}, {"context_complete", "boolean"}, {"image_complete", "boolean"},
                            {"fighter_files_attached", "boolean"}, {"evidence_hash", "string"},
                            {"status", "string"}, {"screenshot", "string"}, {"context", "string"}, {"notes", "string"},
                            {"attachments_ready", "boolean"}, {"stored", "boolean"}})
        .fields({{"image_hash", "string"}, {"attachment_hash", "string"}, {"error", "string"},
                 {"sent_bytes", "integer"}, {"total_bytes", "integer"}, {"duplicate", "boolean"}}, false)
        .add("agent_report", authored.schema);
    c.add({"feedback.prepare", "feedback.inspect", "feedback.status", "feedback.submit"}, report);
    c.add({"feedback.list"}, c.object({{"capacity", "integer"}, {"storage_limit_bytes", "integer"}})
        .add("reports", c.array(report.schema)));
    c.add({"feedback.discard"}, c.object({{"report_id", "string"}, {"status", "string"}}));
}
}

Object workshop_state(Catalog& c)
{
    auto state = c.object({{"instance", "string"}, {"build_identity", "string"}, {"source_revision", "string"},
                           {"build", "string"}, {"workshop_open", "boolean"}, {"testing", "boolean"},
                           {"preview_loading", "boolean"}, {"editing_allowed", "boolean"}, {"portrait_open", "boolean"},
                           {"agent_access_enabled", "boolean"}, {"content_root", "string"},
                           {"revision", "string"}, {"dirty", "boolean"}})
        .add("preview_scene", c.enumeration({"loading", "practice", "character_select", "other"}))
        .add("project", c.nullable("string"))
        .fields({{"projects_root", "string"}, {"saved_disk_revision", "string"}, {"member", "string"},
                 {"workspace", "string"}, {"selected_source", "string"}, {"gesture_active", "boolean"},
                 {"can_undo", "boolean"}, {"can_redo", "boolean"}, {"selected_action", "integer"},
                 {"frame", "integer"}, {"accepted_behavior_matches_sources", "boolean"},
                 {"preview_matches_fighter", "boolean"}, {"preview_ready", "boolean"}}, false)
        .add("members", c.array(c.object({{"key", "string"}, {"title", "string"}}).schema), false)
        .add("model_view", model_view(c).schema, false)
        .add("preview_view", preview_view(c).schema, false);
    for (const char* field : {"fighter_revision", "built_package_id", "loaded_preview_package_id",
                              "requested_preview_package_id", "installed_package_id", "preview_fighter_revision"})
        state.add(field, c.nullable("string"), false);
    return state;
}

Object workshop_mutation(Catalog& c)
{
    return workshop_state(c).fields({{"committed", "boolean"}});
}

void add_sessions(Catalog& c)
{
    c.add({"session.inspect"}, workshop_state(c));
    c.add({"session.open", "session.undo", "session.redo", "session.workspace.select", "session.member.select",
           "session.actions.select", "session.animations.select", "session.preview.configure",
           "session.scripts.select"}, workshop_mutation(c));
    c.add({"session.disconnect", "session.practice.stop"}, workshop_mutation(c).fields({{"status", "string"}}));
    c.add({"session.save"}, workshop_mutation(c).fields({{"status", "string"}, {"saved", "boolean"}, {"installed", "boolean"}}));
    c.add({"session.build", "session.install", "session.preview.refresh"}, live_build(c));
    c.add({"session.scripts.list"}, workshop_state(c).fields({{"parameters", "string"}})
        .add("files", c.array(c.object({{"file", "string"}, {"name", "string"}, {"bytes", "integer"}, {"dirty", "boolean"}}).schema)));
    c.add({"session.scripts.read"}, workshop_state(c).fields({{"file", "string"}, {"text", "string"}}));
    c.add({"session.scripts.check"}, workshop_state(c).merge(script_validation(c))
        .fields({{"draft_view", "string"}, {"editable_source_sha256", "string"}, {"parameter_sha256", "string"},
                 {"combined_bytes", "integer"}, {"matches_validated_source", "boolean"}}, false));
    c.add({"session.scripts.update"}, workshop_mutation(c).fields({{"status", "string"}})
        .fields({{"preview_status", "string"}, {"saved", "boolean"}, {"installed", "boolean"}}, false));
    auto view = c.object({{"transient", "boolean"}, {"saved", "boolean"}, {"installed", "boolean"}})
        .add("model_view", model_view(c).schema);
    c.add({"session.models.view"}, workshop_state(c).merge(view));
    c.add({"session.models.select", "session.models.pose"}, workshop_mutation(c).merge(view));
    practice(c);
    presentation(c);
    feedback(c);
}
}
