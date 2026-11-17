#include "native_workshop_agent_catalog.h"
#include "native_action_authoring.h"
#include "native_animation_authoring.h"
#include "native_attribute_authoring.h"
#include "native_model_authoring.h"
#include "native_resource_authoring.h"
#include <cstring>
namespace NativeWorkshopAgent {
#define INSTANCE R"("instance":{"type":"string","minLength":34,"maxLength":43})"
#define REVISION R"("revision":{"type":"string","minLength":64,"maxLength":64})"
static constexpr char capture_output[] = R"json({
    "type":"object",
    "required":["capture_id","scope","status","view"],
    "properties":{
        "capture_id":{"type":"string","pattern":"^[0-9a-f]{32}$"},
        "request_id":{"type":"string"},
        "scope":{"enum":["viewport","window"]},
        "status":{"enum":["pending","ready","failed"]},
        "view":{
            "type":"object",
            "required":["instance","member","revision","workspace","loaded_preview_package_id","preview_view"],
            "properties":{
                "instance":{"type":"string"},"revision":{"type":"string"},"member":{"type":"string"},
                "workspace":{"type":"string"},"build_identity":{"type":"string"},
                "fighter_revision":{"type":["string","null"]},
                "preview_fighter_revision":{"type":["string","null"]},
                "loaded_preview_package_id":{"type":"string"},
                "model_view":{"type":"object"},
                "preview_view":{
                    "type":"object",
                    "required":["pose","camera","playing","hitboxes","hurtboxes","bones","viewport_visible"],
                    "properties":{
                        "pose":{"type":"object","required":["bank","index","frame"],"properties":{
                            "bank":{"enum":["action","demo"]},"index":{"type":"integer"},"frame":{"type":"number"}}},
                        "camera":{"type":["object","null"],"required":["target","yaw","pitch","distance"],"properties":{
                            "target":{"type":"array","minItems":3,"maxItems":3,"items":{"type":"number"}},
                            "yaw":{"type":"number"},"pitch":{"type":"number"},"distance":{"type":"number"}}},
                        "playing":{"type":"boolean"},"hitboxes":{"type":"boolean"},"hurtboxes":{"type":"boolean"},
                        "bones":{"type":"boolean"},"viewport_visible":{"type":"boolean"}
                    }
                }
            }
        },
        "output":{"type":"string"},"png_sha256":{"type":"string","pattern":"^[0-9a-f]{64}$"},
        "bytes":{"type":"integer","minimum":1},"width":{"type":"integer","minimum":1},
        "height":{"type":"integer","minimum":1},"displayed_frame":{"type":"integer","minimum":1},
        "presented_at_ns":{"type":"integer"},"message":{"type":"string"},"image_error":{"type":"string"}
    },
    "allOf":[
        {"if":{"properties":{"status":{"const":"ready"}}},"then":{
          "required":["output","png_sha256","bytes","width","height","displayed_frame","presented_at_ns"]}},
        {"if":{"properties":{"status":{"const":"failed"}}},"then":{"required":["message"]}}
    ],
    "additionalProperties":false
})json";
static const Command commands[] = {
    {"session.resources.read", "Inspect the live member's unsaved named PNG/blob resources, hashes, image dimensions, limits and article/sound/effect inventory.", NativeResourceAuthoring::read_schema(true), true},
    {"session.resources.edit", "Import/remove resources as one atomic batch and current-workspace Undo step. Leaves saved files and installed packages unchanged; refresh preview explicitly after checking source/resource references.", NativeResourceAuthoring::edit_schema(true), false},
    {"session.resources.export", "Export a live unsaved resource's exact bytes to a new file. Existing output files and draft history are preserved.", NativeResourceAuthoring::export_schema(true), false},
    {"session.models.view", "Read the Model workspace's transient mesh/material/bone selection, hidden meshes, preview mode, playhead and temporary local pose. Does not read saved-file defaults.", NativeModelAuthoring::view_schema(), true},
    {"session.models.select", "Select Model context/mode, mesh/material/bones, editor-only hidden meshes and an action clip/frame or playback state. Skeleton mode always draws bones. Changes view state, not fighter assets; no save or installation.", NativeModelAuthoring::select_schema(), false},
    {"session.models.pose", "Apply an atomic temporary local pose batch or reset to the rest pose in Model/Pose Mode. Uses inspected bone ordinals, game units, radians and scale multipliers. Does not key animations, edit the rest rig or save assets.", NativeModelAuthoring::pose_schema(), false},
    {"session.models.read", "Inspect the live member's unsaved meshes, skeleton, bind-space vertices and weights, materials, texture sequences and source-aware reimport plan.", NativeModelAuthoring::read_schema(true), true},
    {"session.models.edit", "Atomically edit the live model: GLB/glTF import/reimport, compensated rest transforms, weight painting, materials, texture images, labels, mesh assignments or extra bones. Shares one Model Undo step and reloads frozen preview; leaves disk and installed fighters unchanged.", NativeModelAuthoring::edit_schema(true), false},
    {"session.attributes.list", "Read the live member's unsaved common attributes, names, types, descriptions and native limits.", NativeAttributeAuthoring::list_schema(true), true},
    {"session.attributes.edit", "Edit common attributes by name as one atomic Workshop undo step. Returns requested and applied values; leaves disk and installed fighters unchanged.", NativeAttributeAuthoring::edit_schema(true), false},
    {"session.animations.read", "Read unsaved clips, bones, tracks, keys, local poses and native evaluated samples.", NativeAnimationAuthoring::read_schema(true), true},
    {"session.animations.preview", "Read the actual frozen preview actor's applied clip/frame, local bone transforms and world matrices. Returns preview_pending until the requested draft pose has reached the actor. Includes current draft/loaded-package identity.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE R"(,"bones":{"type":"array","maxItems":512,"items":{"type":"integer","minimum":0,"maximum":511}}},"required":["instance"],"additionalProperties":false})", true},
    {"session.animations.edit", "Atomically edit animation assets, including bulk keys/transforms/deletion, pose copying and independent clip creation. Shares Workshop undo and selects the edited clip for frozen preview.", NativeAnimationAuthoring::edit_schema(true), false},
    {"session.animations.select", "Select an action/demo clip in Animation and seek the frozen preview to a frame.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "," REVISION R"(,"clip":{"type":"object","properties":{"bank":{"type":"string","enum":["action","demo"]},"index":{"type":"integer","minimum":0,"maximum":1023}},"required":["bank","index"],"additionalProperties":false},"frame":{"type":"integer","minimum":0,"maximum":65535}},"required":["instance","revision","clip"],"additionalProperties":false})", false},
    {"session.actions.list", "List the live member's real action IDs, names and labels, including unsaved archive edits.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE R"(,"query":{"type":"string","maxLength":256},"offset":{"type":"integer","minimum":0,"maximum":1000000},"limit":{"type":"integer","minimum":1,"maximum":200}},"required":["instance"],"additionalProperties":false})", true},
    {"session.actions.read", "Inspect live playback, events, typed properties, creation defaults or the expanded event timeline. Uses the same native action codecs and timer/loop tracing as the Workshop. Event handles belong to the returned revision.", NativeActionAuthoring::read_schema(true), true},
    {"session.actions.edit", "Atomically edit label/visibility metadata, playback, convert proven hitbox lifetimes with own_hitboxes, create an event, update one event or a batch of up to 256 distinct events, retime repetitions, or delete. Grouped updates return requested/applied properties and share one Workshop undo step. Requires the inspected live revision.", NativeActionAuthoring::edit_schema(true), false},
    {"session.actions.select", "Show this action in the Action Editor and seek its frozen preview to an optional frame.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "," REVISION R"(,"action_id":{"type":"integer","minimum":0,"maximum":1023},"frame":{"type":"integer","minimum":0,"maximum":1000}},"required":["instance","revision","action_id"],"additionalProperties":false})", false},
    {"feedback.prepare", "Capture an agent bug or feature request now, using the F2 screenshot and gameplay diagnostics. Inspect before submitting. Reusing task_id and issue_key returns the original report. Works without a draft or preview. No fighter source/assets are attached.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "," REVISION R"(,"task_id":{"type":"string","minLength":1,"maxLength":120},"issue_key":{"type":"string","minLength":1,"maxLength":120},"category":{"type":"string","enum":["bug","feature_request"]},"summary":{"type":"string","minLength":1,"maxLength":300},"operation":{"type":"string","maxLength":300},"expected":{"type":"string","minLength":1,"maxLength":4000},"observed":{"type":"string","minLength":1,"maxLength":4000},"reproduction":{"type":"string","maxLength":8000},"diagnostics":{"type":"string","maxLength":12000},"capability":{"type":"string","maxLength":4000},"alternatives":{"type":"string","maxLength":4000},"pins":{"type":"array","maxItems":16,"items":{"type":"object","properties":{"x":{"type":"number","minimum":0,"maximum":1},"y":{"type":"number","minimum":0,"maximum":1},"text":{"type":"string","minLength":1,"maxLength":1900}},"required":["x","y","text"],"additionalProperties":false}}},"required":["instance","task_id","issue_key","category","summary","expected","observed"],"additionalProperties":false})", false},
    {"feedback.list", "List this installation's prepared and submitted agent reports, their categories, local attachments and upload status. The same reports appear in the Workshop Agent tools panel.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "},\"required\":[\"instance\"],\"additionalProperties\":false}", true},
    {"feedback.inspect", "Inspect the original report, capture-time Workshop identity and local screenshot/context paths before submission. Never collects fresh state for the report.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE R"(,"report_id":{"type":"string","minLength":36,"maxLength":36}},"required":["instance","report_id"],"additionalProperties":false})", true},
    {"feedback.status", "Read upload progress or its durable receipt. stored:true confirms the existing F2 feedback service accepted this report.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE R"(,"report_id":{"type":"string","minLength":36,"maxLength":36}},"required":["instance","report_id"],"additionalProperties":false})", true},
    {"feedback.submit", "Send the inspected agent report through the existing F2 backend and administration queue. Uploads its screenshot, notes and capture-time gameplay diagnostics. Retries reuse the original sealed bytes and ID.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE R"(,"report_id":{"type":"string","minLength":36,"maxLength":36}},"required":["instance","report_id"],"additionalProperties":false})", false},
    {"feedback.discard", "Delete an agent report's local capture and receipt after inspection. A report already stored on the server stays in the administration queue. Cannot discard an upload in progress.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE R"(,"report_id":{"type":"string","minLength":36,"maxLength":36}},"required":["instance","report_id"],"additionalProperties":false})", false},
    {"session.inspect", "Inspect the running Workshop, selected member, live revision, unsaved state and undo availability.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "},\"required\":[\"instance\"],\"additionalProperties\":false}", true},
    {"session.open", "Open a saved draft in this instance's Workshop. Requires the current live revision and refuses to discard unsaved edits or interrupt an online match.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "," REVISION R"(,"project":{"type":"string","minLength":1,"maxLength":500}},"required":["instance","revision","project"],"additionalProperties":false})", false},
    {"session.member.select", "Select the family member to inspect or edit. Returns the new live view and revision.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "," REVISION R"(,"member":{"type":"string","minLength":1,"maxLength":31}},"required":["instance","revision","member"],"additionalProperties":false})", false},
    {"session.workspace.select", "Select the Action, Scripting, Animation or Model workspace in the current editor.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "," REVISION R"(,"workspace":{"type":"string","enum":["action","scripting","animation","model"]}},"required":["instance","revision","workspace"],"additionalProperties":false})", false},
    {"session.scripts.list", "List the live member's editable source files and parameters, including unsaved Workshop changes.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "},\"required\":[\"instance\"],\"additionalProperties\":false}", true},
    {"session.scripts.read", "Read one live editable move or fighter-state file by its relative path from session.scripts.list.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE R"(,"file":{"type":"string","minLength":1,"maxLength":400}},"required":["instance","file"],"additionalProperties":false})", true},
    {"session.scripts.select", "Show one editable source file in the Workshop's Scripting pane. Resolve the file through session.scripts.list first.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "," REVISION R"(,"file":{"type":"string","minLength":1,"maxLength":400}},"required":["instance","revision","file"],"additionalProperties":false})", false},
    {"session.scripts.check", "Check the selected live member's unsaved sources, parameters and owned assets in an isolated native validator. Returns independent errors by source file and line. Leaves saved files, accepted behavior and preview unchanged; bounded to 15 seconds.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "," REVISION "},\"required\":[\"instance\",\"revision\"],\"additionalProperties\":false}", true},
    {"session.scripts.update", "Change live source files and optional parameter text as one Workshop undo step. Does not save, validate, install or restart preview.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "," REVISION
        R"(,"summary":{"type":"string","minLength":1,"maxLength":200},"files":{"type":"array","maxItems":128,"items":{"type":"object","properties":{"file":{"type":"string","minLength":1,"maxLength":400},"text":{"type":"string","maxLength":524288}},"required":["file","text"],"additionalProperties":false}},"parameters":{"type":"string","maxLength":65536}},"required":["instance","revision","summary","files"],"additionalProperties":false})", false},
    {"session.save", "Save the whole live fighter family, editable sources and parameters as one recoverable disk transaction. Refuses external disk changes. Does not build behavior, refresh preview or install the fighter.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "," REVISION "},\"required\":[\"instance\",\"revision\"],\"additionalProperties\":false}", false},
    {"session.build", "Validate and build all editable family members, then accept and save their behavior and owned assets in one recoverable transaction. Returns package identity and member diagnostics; preserves preview and undo history.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "," REVISION "},\"required\":[\"instance\",\"revision\"],\"additionalProperties\":false}", false},
    {"session.install", "Build, accept and install the complete live fighter family, updating its existing CSS tiles. Returns the installed package identity. Preserves the currently running preview.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "," REVISION "},\"required\":[\"instance\",\"revision\"],\"additionalProperties\":false}", false},
    {"session.preview.refresh", "Build a cached snapshot of unsaved edits and load it in the frozen preview. Inspection confirms the actual loaded package and fighter revision. Preserves draft files and installed membership.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "," REVISION "},\"required\":[\"instance\",\"revision\"],\"additionalProperties\":false}", false},
    {"session.preview.configure", "Set the frozen camera and the active Action or Animation preview controls. Angles use radians, target and distance use game units. Read preview_view from session.inspect. Model visibility and playback use session.models.select.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "," REVISION
        R"(,"camera":{"type":"object","properties":{"target":{"type":"array","minItems":3,"maxItems":3,"items":{"type":"number","minimum":-10000,"maximum":10000}},"yaw":{"type":"number","minimum":-100,"maximum":100},"pitch":{"type":"number","minimum":-1.45,"maximum":1.45},"distance":{"type":"number","minimum":8,"maximum":220}},"additionalProperties":false},"reset_camera":{"type":"string","enum":["default","front"]},"hitboxes":{"type":"boolean"},"hurtboxes":{"type":"boolean"},"bones":{"type":"boolean"},"playing":{"type":"boolean"}},"required":["instance","revision"],"additionalProperties":false})", false},
    {"session.preview.capture", "Queue a PNG of this rendered Workshop view at the inspected revision. Select viewport or window and a new absolute .png output path. Poll session.preview.capture_status using the capture ID.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "," REVISION
        R"(,"scope":{"type":"string","enum":["viewport","window"]},"output":{"type":"string","minLength":1,"maxLength":500}},"required":["instance","revision","scope","output"],"additionalProperties":false})", false, capture_output},
    {"session.preview.capture_status", "Read the latest capture's frozen metadata and pending, ready or failed status. A ready capture includes its PNG path, hash and dimensions. MCP also returns the PNG as image content.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE
        R"(,"capture_id":{"type":"string","minLength":32,"maxLength":32}},"required":["instance","capture_id"],"additionalProperties":false})", true, capture_output},
    {"session.practice.start", "Build current edits and enter ordinary local practice with the whole fighter family. Inspection confirms when the package loads. Preserves saved drafts and installed membership.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "," REVISION "},\"required\":[\"instance\",\"revision\"],\"additionalProperties\":false}", false},
    {"session.practice.stop", "Return this Workshop's live practice test to its frozen preview. Available during the owned test; leaves ordinary matches untouched.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "," REVISION "},\"required\":[\"instance\",\"revision\"],\"additionalProperties\":false}", false},
    {"session.undo", "Undo one edit in the current Workshop member and workspace. Requires the inspected live revision.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "," REVISION "},\"required\":[\"instance\",\"revision\"],\"additionalProperties\":false}", false},
    {"session.redo", "Redo one edit in the current Workshop member and workspace. Requires the inspected live revision.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "," REVISION "},\"required\":[\"instance\",\"revision\"],\"additionalProperties\":false}", false},
    {"session.disconnect", "Disconnect agent access to this running instance. Leaves its Workshop draft open and preserves edits. Access can be enabled again in Agent tools.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "},\"required\":[\"instance\"],\"additionalProperties\":false}", false},
};
#undef INSTANCE
#undef REVISION
const Command* catalog(size_t& count) { count = sizeof(commands) / sizeof(*commands); return commands; }
const Command* find(const char* name)
{
    for (const auto& command : commands) if (!std::strcmp(name, command.name)) return &command;
    return nullptr;
}
bool validate(const Command& command, yyjson_val* arguments, NativeAuthoring::Reply& reply)
{
    auto* schema = yyjson_read(command.schema, std::strlen(command.schema), 0);
    std::string error;
    bool ok = schema && NativeAuthoring::validate_arguments(arguments, yyjson_doc_get_root(schema), error);
    yyjson_doc_free(schema);
    if (!ok) reply.fail("invalid_arguments", error.empty() ? "Could not read the operation schema." : error);
    return ok;
}
}
