#include <melee/ft/native_fighter_skin_edit.h>
#include "native_shell_workshop_model.h"
#include "native_shell_workshop_model_internal.h"

#include "native_shell_workshop_materials.h"
#include "native_shell_workshop_transform.h"
#include "native_shell_workshop_skinning.h"
#include <melee/ft/native_fighter_authoring.h>
#include <melee/ft/native_fighter_material_animation.h>
#include <melee/ft/native_fighter_core_asset_state.h>

#ifdef MELEE_NATIVE

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <windows.h>

// The common dialog declarations require the Windows types above.
#include <commdlg.h>

#include "native_shell_workshop.h"
#include <melee/ft/native_fighter_gltf_import.h>
#include <melee/ft/native_fighter_model_editor.h>
#include <melee/gm/native_home.h>
#include <RmlUi/Core.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <RmlUi/Core/Elements/ElementFormControlSelect.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/native_model_view.h>

namespace WorkshopModelInternal {

Rml::ElementDocument* document;

ModelSession sessions[2];
ModelSession* active = &sessions[0];
// The one set of widgets and in-progress gestures belongs to the visible view.
// Committed edits, pose and undo belong to the selected member's session.
bool visible;
int hovered_mesh = -1;
bool playing;
unsigned reveal_material;
std::chrono::steady_clock::time_point play_started;
int play_start_frame;
void build_bones();

void update_mesh_view()
{
    if (!visible) {
        nativeModelView_Clear();
        return;
    }
    NativeHomeWorkshopMesh meshes[NATIVE_MODEL_MAX_MESHES];
    unsigned ordinals[NATIVE_MODEL_MAX_JOINTS] = {};
    for (unsigned i = 0; i < active->model.mesh_count; ++i) {
        unsigned joint = active->model.meshes[i].joint;
        meshes[i] = { joint, ordinals[joint]++, active->hidden_meshes[i] != 0 };
    }
    nativeHome_WorkshopMeshView(meshes, active->model.mesh_count, hovered_mesh,
                                active->context == Context::Skinning ? active->selected_mesh
                                                             : -1,
                                (unsigned) active->selected_joint, active->rig_mode != RigMode::Animation);
}

Rml::Element* element(const char* id)
{
    return document->GetElementById(id);
}

Rml::ElementFormControlInput* input(const char* id)
{
    return static_cast<Rml::ElementFormControlInput*>(element(id));
}

void text(const char* id, const std::string& value)
{
    element(id)->SetInnerRML(Rml::StringUtilities::EncodeRml(value));
}

std::string escape(const std::string& value)
{
    return Rml::StringUtilities::EncodeRml(value);
}

void status(const std::string& value)
{
    text("workshop-editor-status", value);
}

std::string decimal(float value)
{
    char output[32];
    std::snprintf(output, sizeof(output), "%.9g", value);
    return output;
}

bool parse_float(const char* id, float* value)
{
    const std::string source = input(id)->GetValue();
    char* end;
    errno = 0;
    *value = std::strtof(source.c_str(), &end);
    return !source.empty() && *end == 0 && errno == 0 &&
           std::isfinite(*value);
}

bool index(const char* id, const char* prefix, unsigned* result)
{
    const size_t length = std::strlen(prefix);
    if (std::strncmp(id, prefix, length) != 0) return false;
    char* end;
    const unsigned long parsed = std::strtoul(id + length, &end, 10);
    if (end == id + length || *end != 0 || parsed > UINT_MAX) return false;
    *result = (unsigned) parsed;
    return true;
}


std::string joint_label(unsigned joint)
{
    if (joint >= active->model.joint_count) return "Joint";
    return active->model.joints[joint].name;
}

void render_tree()
{
    if (active->project == nullptr) return;
    std::string tree;
    if (active->context == Context::Meshes || active->context == Context::Materials) {
        tree += "<div class='workshop-model-tree-heading'>MESHES</div>";
        for (unsigned i = 0; i < active->model.mesh_count; ++i) {
            const NativeModelMesh& mesh = active->model.meshes[i];
            tree += "<div class='workshop-model-mesh-row'><button "
                    "id='workshop-model-mesh-" +
                    std::to_string(i) + "' class='workshop-model-tree-row" +
                    (active->selected_mesh == (int) i ? " selected" : "") +
                    "'><span>" + escape(mesh.name) + "</span><small>" +
                    std::to_string(mesh.polygon_count) +
                    "</small></button>"
                    "<button id='workshop-model-eye-" +
                    std::to_string(i) + "' class='workshop-model-eye" +
                    (active->hidden_meshes[i] ? " hidden" : "") + "' title='" +
                    (active->hidden_meshes[i] ? "Show mesh" : "Hide mesh") +
                    " (editor only)'><span><i></i></span></button></div>";
        }
    }
    if (active->context == Context::Materials) {
        tree += "<div class='workshop-model-tree-heading'>MATERIALS</div>";
        for (unsigned i = 0; i < active->model.material_count; ++i)
            tree += "<button id='workshop-model-material-" +
                    std::to_string(i) + "' class='workshop-model-tree-row" +
                    (active->selected_material == (int) i ? " selected" : "") +
                    "'><span>" + escape(active->model.materials[i].name) + "</span></button>";
    }
    if (active->context == Context::Skeleton || active->context == Context::Skinning) {
        tree += "<div class='workshop-model-tree-heading'>BONE OUTLINER</div>";
        for (unsigned i = 0; i < active->model.joint_count; ++i) {
            const NativeModelJoint& bone = active->model.joints[i];
            tree += "<button id='workshop-model-joint-" + std::to_string(i) +
                    "' class='workshop-model-tree-row" +
                    (active->selected_joints[i] ? " selected" : "") +
                    "' style='padding-left:" +
                    std::to_string(8 + std::min(11, (int) bone.depth) * 11) +
                    "px'><span>" + escape(bone.name) + "</span></button>";
        }
    }
    Rml::Element* outliner = element("workshop-model-tree");
    const float scroll = outliner->GetScrollTop();
    outliner->SetInnerRML(tree);
    outliner->SetScrollTop(scroll);
    if (active->context == Context::Skinning)
        element("workshop-model-tree-actions")->SetInnerRML(
            "<div class='eyebrow'>SKINNED MESH</div><select "
            "id='workshop-model-skin-mesh' class='room-input'></select>"
            "<p class='workshop-muted'>The existing compatible rig is retained.</p>");
    else if (active->context == Context::Skeleton)
        element("workshop-model-tree-actions")->SetInnerRML(rig_tree_controls());
    else element("workshop-model-tree-actions")->SetInnerRML("");
    if (active->context == Context::Skinning) {
        std::string options;
        for (unsigned i = 0; i < active->model.mesh_count; ++i)
            options += "<option value='" + std::to_string(i) + "'" +
                       (active->selected_mesh == (int) i ? " selected" : "") +
                       ">" + escape(active->model.meshes[i].name) + "</option>";
        element("workshop-model-skin-mesh")->SetInnerRML(options);
    }
}

std::string transform_fields(const float* position, const float* rotation,
                             const float* scale)
{
    std::string result = "<div class='workshop-model-axis-labels'>"
                         "<span>X</span><span>Y</span><span>Z</span></div>";
    const char* names[] = { "Position", "Rotation °", "Scale" };
    const char* ids[] = { "pos", "rot", "scale" };
    const float* values[] = { position, rotation, scale };
    for (unsigned channel = 0; channel < 3; ++channel) {
        result += "<label>" + std::string(names[channel]) + "</label>"
                  "<div class='workshop-model-axis'>";
        for (unsigned axis = 0; axis < 3; ++axis) {
            const float value = channel == 1 ?
                values[channel][axis] * 57.2957795f : values[channel][axis];
            result += std::string("<input class='room-input' type='text' id='workshop-model-") +
                      ids[channel] + "-" + std::to_string(axis) +
                      "' value='" + decimal(value) + "' />";
        }
        result += "</div>";
    }
    return result;
}

bool rebuilding_model_inspector;

void render_inspector()
{
    if (active->project == nullptr) return;
    std::string markup;
    if (active->context == Context::Meshes && active->selected_mesh >= 0 &&
        active->selected_mesh < active->model.mesh_count) {
        const NativeModelMesh& mesh = active->model.meshes[active->selected_mesh];
        markup = "<div class='workshop-model-inspector-section'><div class='eyebrow'>"
                 "SELECTED MESH</div><h3>" + escape(mesh.name) +
                 "</h3><p>Joint " + std::to_string(mesh.joint) + " · " +
                 std::to_string(mesh.polygon_count) +
                 " geometry parts</p></div><div class='workshop-model-inspector-section'>"
                 "<div class='eyebrow'>MATERIAL</div><label>Assigned</label>"
                 "<select id='workshop-model-assigned-material' class='room-input'>";
        for (unsigned i = 0; i < active->model.material_count; ++i)
            markup += "<option value='" + std::to_string(i) + "'" +
                      (mesh.material == i ? " selected" : "") + ">" +
                      escape(active->model.materials[i].name) + "</option>";
        markup += "</select>"
                  "<button id='workshop-model-edit-material' class='room-button'>"
                  "Edit material ↗</button></div><div class='workshop-model-inspector-section'>"
                  "<div class='eyebrow'>BINDING</div><p>Uses the compatible fighter "
                  "joint hierarchy. Edit bone weights in Skinning.</p></div>";
        if(const auto* source=nativeFighterMeshSource_Get(active->project,(uint32_t)mesh.offset)) {
            std::string filename=source->path;
            const auto slash=filename.find_last_of("/\\");
            if(slash!=std::string::npos)filename.erase(0,slash+1);
            markup += "<div class='workshop-model-inspector-section'><div class='eyebrow'>SOURCE</div><p title='" +
                escape(source->path) + "'>"+escape(filename)+"</p><button id='workshop-model-reimport' class='room-button'>"
                "Reimport source…</button></div>";
        }
    } else if (active->context == Context::Materials && active->selected_material >= 0 &&
               active->selected_material < active->model.material_count) {
        const NativeModelMaterial& mat = active->model.materials[active->selected_material];
        char color[16];
        std::snprintf(color, sizeof(color), "#%02X%02X%02X",
                      mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
        markup = "<div class='workshop-model-inspector-section'><div class='eyebrow'>"
                 "SELECTED MATERIAL</div><h3>" + escape(mat.name) + "</h3><p>";
        unsigned uses = 0;
        for (unsigned i = 0; i < active->model.mesh_count; ++i)
            if (active->model.meshes[i].material == active->selected_material) ++uses;
        markup +=
            std::to_string(uses) + " mesh" + (uses == 1 ? "" : "es") +
            " use this material.</p></div><div "
            "class='workshop-model-inspector-section'>"
            "<div class='eyebrow'>SURFACE</div><label>Diffuse tint "
            "(hex)</label>"
            "<input id='workshop-model-diffuse' class='room-input' "
            "type='text' "
            "value='" +
            color +
            "' /><label>Opacity</label>"
            "<input id='workshop-model-alpha' class='room-input' type='text' "
            "value='" +
            decimal(mat.alpha) +
            "' /><label>Shininess</label>"
            "<input id='workshop-model-shininess' class='room-input' "
            "type='text' "
            "value='" +
            decimal(mat.shininess) +
            "' />"
            "</div><div "
            "class='workshop-model-inspector-section'>"
            "<div class='eyebrow'>TEXTURE</div><p>" +
            std::string(mat.texture_offset ? "Texture attached"
                                           : "No texture attached");
        markup += "</p><label>Replace texture (PNG or JPEG)</label>"
                  "<input id='workshop-model-texture-path' class='room-input' "
                  "type='text' placeholder='Choose an image file' />"
                  "<button id='workshop-model-texture-browse' "
                  "class='room-button'>Browse...</button>"
                  "<button id='workshop-model-texture-apply' "
                  "class='room-button primary'>Apply texture</button>"
                  "<p>The image is embedded in this costume. Meshes sharing "
                  "the material change together.</p></div>";
        {
            NativeMaterialTextureSequence sequences[128];
            unsigned count = nativeFighterMaterial_Sequences(
                &active->model, (unsigned) active->selected_material,
                sequences, 128);
            if (count) {
                markup +=
                    "<div class='workshop-model-inspector-section'><div "
                    "class='eyebrow'>ANIMATED TEXTURE IMAGES</div><p>Replace "
                    "an image used by expressions or texture animation. "
                    "Animation keys keep their image indices.</p><select "
                    "id='workshop-model-texture-image' class='room-input'>";
                for (unsigned i = 0; i < count; ++i) {
                    for (unsigned f = 0; f < sequences[i].frames; ++f) {
                        markup += "<option value='" +
                                  std::to_string(sequences[i].offset) + ":" +
                                  std::to_string(f) + "'>Sequence " +
                                  std::to_string(i + 1) + " · layer " +
                                  std::to_string(sequences[i].layer) +
                                  " · image " + std::to_string(f) +
                                  "</option>";
                    }
                }
                markup +=
                    "</select><button id='workshop-model-texture-image-apply' "
                    "class='room-button'>Replace selected "
                    "image</button><small>Uses the image path in the Texture "
                    "section above.</small></div>";
            }
        }
        markup += nativeWorkshopMaterials_Controls(
            active->model, (unsigned) active->selected_material);
    } else if (active->context == Context::Skinning) {
        markup =
            "<div class='workshop-model-inspector-section'><div "
            "class='eyebrow'>"
            "BONE WEIGHTS</div><h3>" +
            escape(joint_label(active->selected_joint)) +
            "</h3><p>The surface shows this bone's influence: blue 0%, "
            "yellow, "
            "orange, red 100%.</p></div><div "
            "class='workshop-model-inspector-section'>"
            "<div class='eyebrow'>WEIGHT BRUSH</div><p>Drag to paint the "
            "selected mesh in its current pose. Painting pauses clip playback. "
            "The brush radius is measured on the rest mesh.</p>"
            "<label>Radius (Melee units)</label><input "
            "id='workshop-model-brush-radius' "
            "class='room-input' type='text' value='" +
            decimal(active->brush_radius) +
            "' /><label>Strength</label><input "
            "id='workshop-model-brush-strength' "
            "class='room-input' type='text' value='" +
            decimal(active->brush_strength) +
            "' /><label>Brush</label><select id='workshop-model-brush-mode' "
            "class='room-input'>"
            "<option value='add'" +
            std::string(active->brush_subtract ? "" : " selected") +
            ">Add weight</option><option value='subtract'" +
            std::string(active->brush_subtract ? " selected" : "") +
            ">Subtract weight</option></select><p>Weights are normalized. "
            "Each stroke is undoable.</p></div>";
    } else if (active->context == Context::Skeleton &&
               active->selected_joint >= 0 && active->selected_joint < active->model.joint_count) {
        const NativeModelJoint& bone = active->model.joints[active->selected_joint];
        markup = "<div class='workshop-model-inspector-section'><div class='eyebrow'>"
                 "SELECTED BONE</div><h3>" + escape(bone.name) +
                 "</h3><p>Joint " + std::to_string(active->selected_joint) +
                 " · parent " + (bone.parent < 0 ? std::string("none") :
                                     std::to_string(bone.parent)) +
                 "</p></div>";
        if (active->context == Context::Skeleton && active->rig_mode != RigMode::Animation) {
            markup += "<div class='workshop-model-inspector-section'>"
                      "<div class='eyebrow'>" +
                      std::string(active->rig_mode == RigMode::Edit ? "REST TRANSFORM" :
                                                        "TEMPORARY POSE") +
                      "</div>";
            const float* p = active->rig_mode == RigMode::Edit ? bone.position :
                                                        active->pose[active->selected_joint].position;
            const float* r = active->rig_mode == RigMode::Edit ? bone.rotation :
                                                        active->pose[active->selected_joint].rotation;
            const float* s = active->rig_mode == RigMode::Edit ? bone.scale :
                                                        active->pose[active->selected_joint].scale;
            markup += transform_fields(p, r, s);
            markup += "<p>" +
                      std::string(active->rig_mode == RigMode::Edit
                          ? "Keeps skinned meshes and rigid attachments in place. Review inherited clips after changing the rest rig."
                          : "Temporary preview only; no animation keys or rest data change.") +
                      "</p></div>";
        } else {
            markup += "<div class='workshop-model-inspector-section'>"
                      "<div class='eyebrow'>ANIMATION PLAYBACK</div>"
                      "<p>The selected clip drives this rig. Use Edit animation "
                      "to change its keys.</p></div>";
        }
    }
    unsigned category =
        active->context == Context::Meshes      ? NativeAuthoring_Mesh
        : active->context == Context::Materials ? NativeAuthoring_Material
                                                : NativeAuthoring_Bone;
    int selected = category == NativeAuthoring_Mesh ? active->selected_mesh
                   : category == NativeAuthoring_Material
                       ? active->selected_material
                       : active->selected_joint;
    if (selected >= 0) {
        const char* label = category == NativeAuthoring_Mesh
                                ? active->model.meshes[selected].name
                            : category == NativeAuthoring_Material
                                ? active->model.materials[selected].name
                                : active->model.joints[selected].name;
        markup = "<div "
                 "class='workshop-model-inspector-section'><label>Name</"
                 "label><input id='workshop-model-name' class='room-input' "
                 "type='text' maxlength='63' value='" +
                 escape(label) +
                 "'/></div>" +
                 markup;
    }
    if (active->context == Context::Skeleton) markup += rig_inspector();
    rebuilding_model_inspector = true;
    element("workshop-model-inspector")->SetInnerRML(markup);
    rebuilding_model_inspector = false;
}

void render()
{
    if (active->project == nullptr) return;
    const char* contexts[] = { "meshes", "materials", "skeleton", "skinning" };
    for (unsigned i = 0; i < 4; ++i)
        element((std::string("workshop-model-context-") + contexts[i]).c_str())
            ->SetClass("selected", (unsigned) active->context == i);
    const char* modes[] = { "edit", "pose", "animation" };
    for (unsigned i = 0; i < 3; ++i)
        element((std::string("workshop-model-rig-") + modes[i]).c_str())
            ->SetClass("selected", (unsigned) active->rig_mode == i);
    text("workshop-model-context-hint",
        active->context == Context::Meshes ? "Select a mesh in the outliner" :
        active->context == Context::Materials ? "Edit a material and preview it" :
        active->context == Context::Skeleton ? "Compatible fighter rig" :
                                       "Select a bone, then paint mesh vertices");
    text("workshop-model-view-footer",
        active->context == Context::Skinning && active->rig_mode == RigMode::Edit
            ? "LMB drag paint · MMB orbit · wheel zoom"
            : "MMB orbit · Shift MMB pan · wheel zoom");
    text("workshop-model-view-title",
        active->context == Context::Materials && active->selected_material >= 0 ?
            active->model.materials[active->selected_material].name :
        (active->context == Context::Meshes && active->selected_mesh >= 0) ?
            active->model.meshes[active->selected_mesh].name : joint_label(
                (unsigned) std::max(0, active->selected_joint)));
    text("workshop-model-view-subtitle",
        active->rig_mode == RigMode::Edit ? "Edit Mode · rest rig" :
        active->rig_mode == RigMode::Pose ? "Pose Mode · temporary pose" :
                                   "Animation Mode · clip playback");
    text("workshop-model-mode-note",
        active->rig_mode == RigMode::Edit ? "Edit the costume rest rig" :
        active->rig_mode == RigMode::Pose ? "Pose without changing the rest rig" :
                                   "Play and scrub an existing clip");
    element("workshop-model-reset-pose")->SetProperty("display",
        active->rig_mode == RigMode::Pose ? "block" : "none");
    element("workshop-model-clip-controls")->SetProperty("display",
        active->rig_mode == RigMode::Animation ? "block" : "none");
    element("workshop-model-bones")
        ->SetClass("selected", active->bones_visible || active->context == Context::Skeleton);
    text("workshop-model-rig-summary", std::to_string(active->model.joint_count) +
         " joints · compatible Melee core preserved");
    text("workshop-model-mesh-summary", std::to_string(active->model.mesh_count) +
         " meshes · " + std::to_string(active->model.material_count) + " materials");
    render_tree();
    render_inspector();
}

void set_frame(int next)
{
    const int max_frame = active->selected_clip >= 0 ?
        (int) std::max(1.0f, std::ceil(active->clip_frames[active->selected_clip])) : 1;
    active->frame = std::clamp(next, 0, max_frame);
    input("workshop-model-frame")->SetValue(std::to_string(active->frame));
    text("workshop-model-frame-label", std::to_string(active->frame) + " / " +
         std::to_string(max_frame));
    nativeHome_WorkshopPoseInvalidate();
}

void build_clips()
{
    std::string options;
    for (size_t i = 0; i < active->action_count; ++i) {
        if (!active->has_clip[i]) continue;
        options += "<option value='" + std::to_string(i) + "'" +
                   (active->selected_clip == (int) i ? " selected" : "") + ">" +
                   escape(active->actions[i].name) + "</option>";
    }
    element("workshop-model-clip")->SetInnerRML(options);
    input("workshop-model-frame")->SetAttribute("max",
        std::to_string(active->selected_clip >= 0 ?
            (int) std::max(1.0f, std::ceil(active->clip_frames[active->selected_clip])) : 1));
    set_frame(0);
}

void reopen_model(const RigView* restored)
{
    RigView previous;
    rig_capture_view(previous);
    const uint32_t mesh_offset = active->selected_mesh >= 0 && active->selected_mesh < active->model.mesh_count
        ? active->model.meshes[active->selected_mesh].offset : UINT32_MAX;
    const uint32_t material_offset = active->selected_material >= 0 && active->selected_material < active->model.material_count
        ? active->model.materials[active->selected_material].offset : UINT32_MAX;
    uint32_t hidden_offsets[NATIVE_MODEL_MAX_MESHES];
    unsigned hidden_count = 0;
    for (unsigned i = 0; i < active->model.mesh_count; ++i) {
        if (active->hidden_meshes[i]) {
            hidden_offsets[hidden_count++] = active->model.meshes[i].offset;
        }
    }
    nativeFighterModel_Open(&active->model, active->project);
    std::memset(active->hidden_meshes, 0, sizeof(active->hidden_meshes));
    for (unsigned i = 0; i < active->model.mesh_count; ++i) {
        for (unsigned j = 0; j < hidden_count; ++j) {
            if (active->model.meshes[i].offset == hidden_offsets[j]) {
                active->hidden_meshes[i] = 1;
            }
        }
    }
    active->roles_applied = false;
    active->selected_mesh = active->selected_material = -1;
    for (unsigned i=0; i<active->model.mesh_count; ++i)
        if (active->model.meshes[i].offset == mesh_offset) active->selected_mesh=(int)i;
    for (unsigned i=0; i<active->model.material_count; ++i)
        if (active->model.materials[i].offset == material_offset) active->selected_material=(int)i;
    rig_restore_view(restored ? *restored : previous);
    build_bones();
    render();
}

bool apply_joint_transform()
{
    if (active->selected_joint < 0 || active->selected_joint >= active->model.joint_count ||
        active->rig_mode == RigMode::Animation) return false;
    float values[3][3];
    const char* channels[] = { "pos", "rot", "scale" };
    for (unsigned c = 0; c < 3; ++c)
        for (unsigned a = 0; a < 3; ++a) {
            const std::string id = "workshop-model-" +
                                   std::string(channels[c]) + "-" +
                                   std::to_string(a);
            if (!parse_float(id.c_str(), &values[c][a])) return false;
            if (c == 1) values[c][a] *= 0.01745329252f;
        }
    for (unsigned a = 0; a < 3; ++a)
        if (std::fabs(values[0][a]) > 10000.0f ||
            std::fabs(values[1][a]) > 100.0f ||
            values[2][a] < 0.001f || values[2][a] > 100.0f)
            return false;
    if (active->rig_mode == RigMode::Pose) {
        NativeHomeWorkshopBoneLocal& target = active->pose[active->selected_joint];
        for (unsigned a = 0; a < 3; ++a) {
            target.position[a] = values[0][a];
            target.rotation[a] = values[1][a];
            target.scale[a] = values[2][a];
        }
        active->pose_changed[active->selected_joint] = true;
        nativeHome_WorkshopPoseInvalidate();
        status("Temporary pose applied. Reset pose to clear it.");
        return true;
    }
    NativeModelJoint& target = active->model.joints[active->selected_joint];
    const auto before_labels=label_snapshot();
    const std::vector<uint8_t> before(active->project->costume_dat,
        active->project->costume_dat + active->project->costume_size);
    for (unsigned a = 0; a < 3; ++a) {
        const float next[] = { values[1][a], values[2][a], values[0][a] };
        const float old[] = { target.rotation[a], target.scale[a],
                              target.position[a] };
        for (unsigned c = 0; c < 3; ++c) {
            if (next[c] == old[c]) continue;
            if (!nativeFighterModel_SetJoint(&active->model, active->selected_joint,
                                              c, a, next[c])) {
                std::memcpy(active->project->costume_dat, before.data(), before.size());
                reopen_model();
                return false;
            }
        }
    }
    if (!nativeFighterModel_CompensateRest(&active->model, before.data(),
                                           before.size(),
                                           (unsigned) active->selected_joint)) {
        std::memcpy(active->project->costume_dat, before.data(), before.size());
        reopen_model();
        status("This rest edit could not preserve the model. Check for invalid or singular transforms.");
        return false;
    }
    const bool asset_changed=record_asset(before);
    record_label_change(before_labels,asset_changed);
    nativeHome_WorkshopPoseInvalidate();
    status(nativeShellWorkshop_RefreshPreview()
        ? "Rest rig updated; meshes kept in place. Review inherited clips."
        : "Rest rig updated. Test Character to reload the preview.");
    return true;
}

void apply_surface()
{
    if (active->selected_material < 0 || active->selected_material >= active->model.material_count)
        return;
    unsigned rgb;
    const std::string source = input("workshop-model-diffuse")->GetValue();
    float alpha, shininess;
    if (source.size() != 7 || source[0] != '#' ||
        !std::all_of(source.begin() + 1, source.end(),
                     [](unsigned char c) { return std::isxdigit(c) != 0; }) ||
        std::sscanf(source.c_str() + 1, "%06x", &rgb) != 1 ||
        !parse_float("workshop-model-alpha", &alpha) ||
        !parse_float("workshop-model-shininess", &shininess) ||
        alpha < 0.0f || alpha > 1.0f ||
        shininess < 0.0f || shininess > 1000.0f) {
        status("Enter a #RRGGBB color and valid opacity and shininess.");
        return;
    }
    const NativeModelMaterial& mat = active->model.materials[active->selected_material];
    if (mat.color_offset == 0) {
        status("This material has no editable color record.");
        return;
    }
    const std::vector<uint8_t> before(active->project->costume_dat,
        active->project->costume_dat + active->project->costume_size);
    for (unsigned c = 0; c < 3; ++c) {
        const uint8_t color = (uint8_t) (rgb >> (16 - c * 8));
        if (!nativeFighterModel_SetMaterialColor(&active->model, active->selected_material,
                                                  1, c, color)) {
            std::memcpy(active->project->costume_dat, before.data(), before.size());
            reopen_model();
            status("Could not update this material.");
            return;
        }
    }
    const float fields[] = { alpha, shininess };
    for (unsigned i = 0; i < 2; ++i) {
        if (!nativeFighterModel_SetMaterialFloat(&active->model, active->selected_material,
                                                  i != 0, fields[i])) {
            std::memcpy(active->project->costume_dat, before.data(), before.size());
            reopen_model();
            status("Could not update this material.");
            return;
        }
    }
    std::string error;
    if (!nativeWorkshopMaterials_Apply(document, active->model,
                                       (unsigned) active->selected_material, &error))
    {
        auto* restored =
            (uint8_t*) std::realloc(active->project->costume_dat, before.size());
        if (restored) {
            active->project->costume_dat = restored;
            active->project->costume_size = before.size();
            std::memcpy(restored, before.data(), before.size());
            reopen_model();
        }
        status(error.empty() ? "Could not update material rendering." : error);
        return;
    }
    record_asset(before);
    status(nativeShellWorkshop_RefreshPreview()
        ? "Material updated; refreshing the live preview."
        : "Material updated. Test Character to reload the preview.");
    render_inspector();
}

void apply_texture()
{
    if (active->selected_material < 0 || active->selected_material >= active->model.material_count)
        return;
    const std::string path = input("workshop-model-texture-path")->GetValue();
    if (path.empty()) {
        status("Choose a PNG or JPEG image first.");
        return;
    }
    char error[192] = {};
    const std::vector<uint8_t> before(active->project->costume_dat,
                                      active->project->costume_dat + active->project->costume_size);
    const std::vector<uint8_t> fighter(active->project->fighter_dat,active->project->fighter_dat+active->project->fighter_size);
    if (!nativeFighterGltf_SetTextureImage(
            active->project, (unsigned) active->selected_material, 0, 0, 0,
            path.c_str(), error, sizeof(error)))
    {
        status(error[0] ? error : "Could not embed this texture.");
        return;
    }
    record_asset(before,fighter);
    reopen_model();
    status(nativeShellWorkshop_RefreshPreview()
        ? "Material texture updated; refreshing the live preview."
        : "Texture updated. Test Character to reload the preview.");
}

NativeWorkshopSkinStroke stroke;

void stroke_append(int mouse_x, int mouse_y)
{
    if (!stroke.active) return;
    Rml::Element* viewport = element("workshop-model-viewport");
    const float width = (float) viewport->GetClientWidth();
    const float height = (float) viewport->GetClientHeight();
    const float game_height = std::min(height, width * 0.75f);
    const float game_width = game_height * 4.0f / 3.0f;
    const auto origin = viewport->GetAbsoluteOffset();
    const float local_x = mouse_x - origin.x - (width - game_width) * 0.5f;
    const float local_y = mouse_y - origin.y - (height - game_height) * 0.5f;
    nativeWorkshopSkinning_Append(&stroke, local_x, local_y, game_width, game_height);
}

void stroke_begin(int mouse_x, int mouse_y)
{
    if (active->context != Context::Skinning ||
        active->selected_mesh < 0 || active->selected_joint < 0) return;
    float radius, strength;
    if (!parse_float("workshop-model-brush-radius", &radius) ||
        !parse_float("workshop-model-brush-strength", &strength) ||
        radius <= 0.0f || radius > 1000.0f ||
        strength <= 0.0f || strength > 1.0f) {
        status("Enter a positive brush radius and strength up to 1.");
        return;
    }
    active->brush_radius = radius;
    active->brush_strength = strength;
    active->brush_subtract = static_cast<Rml::ElementFormControlSelect*>(
        element("workshop-model-brush-mode"))->GetValue() == "subtract";
    if (!nativeWorkshopSkinning_Begin(&stroke, active->project, (unsigned) active->selected_mesh)) {
        status("This mesh has no editable surface in the current preview.");
        return;
    }
    playing = false;
    text("workshop-model-play", "Play");
    stroke_append(mouse_x, mouse_y);
}

void stroke_end()
{
    if (!stroke.active) return;
    stroke.active = false;
    if (stroke.centers.empty()) {
        nativeWorkshopSkinning_Clear(&stroke);
        status("Click closer to a mesh vertex to paint it.");
        return;
    }
    const std::vector<uint8_t> before(active->project->costume_dat,
                                      active->project->costume_dat + active->project->costume_size);
    const auto before_labels=label_snapshot();
    unsigned affected = 0;
    char error[192] = {};
    if (!nativeFighterSkin_Paint(active->project, (unsigned) active->selected_mesh,
                                        (unsigned) active->selected_joint,
                                        stroke.centers.data(),
                                        (unsigned) stroke.centers.size(),
                                        active->brush_radius, active->brush_strength,
                                        active->brush_subtract, &affected,
                                        error, sizeof(error))) {
        nativeWorkshopSkinning_Clear(&stroke);
        status(error[0] ? error : "Could not paint this mesh.");
        return;
    }
    nativeWorkshopSkinning_Clear(&stroke);
    if (!affected) {
        status("This brush stroke did not change any vertex weights.");
        return;
    }
    const bool asset_changed=record_asset(before);
    record_label_change(before_labels,asset_changed);
    reopen_model();
    const bool preview = nativeShellWorkshop_RefreshPreview();
    status(std::to_string(affected) + " vertices painted. " +
           (preview ? "Refreshing the live preview."
                    : "Test Character to reload the model."));
}

int pick_mesh(int x, int y)
{
    Rml::Element* viewport = element("workshop-model-viewport");
    const auto base = viewport->GetAbsoluteOffset();
    const float width = (float) viewport->GetClientWidth();
    const float height = (float) viewport->GetClientHeight();
    const float h = std::min(height, width * 0.75f), w = h * 4.0f / 3.0f;
    if (w <= 0 || h <= 0) {
        return -1;
    }
    const float u = (x - base.x - (width - w) * 0.5f) / w;
    const float v = (y - base.y - (height - h) * 0.5f) / h;
    float origin[3], direction[3];
    if (u < 0 || u > 1 || v < 0 || v > 1 ||
        !nativeHome_WorkshopRay(u, v, origin, direction))
    {
        return -1;
    }
    return nativeModelView_Pick(origin, direction);
}

class CameraEvents final : public Rml::EventListener {
public:
    void ProcessEvent(Rml::Event& event) override {
        if (!visible || active->project == nullptr) return;
        const auto& name = event.GetType();
        if (name == "keydown" && active->context == Context::Meshes &&
            event.GetTargetElement()->GetTagName() != "input" &&
            event.GetParameter<int>("key_identifier", 0) == Rml::Input::KI_H)
        {
            if (event.GetParameter<bool>("alt_key", false)) {
                std::memset(active->hidden_meshes, 0, sizeof(active->hidden_meshes));
            } else if (active->selected_mesh >= 0) {
                active->hidden_meshes[active->selected_mesh] = 1;
            }
            hovered_mesh = -1;
            render_tree();
            update_mesh_view();
            event.StopPropagation();
            return;
        }
        if (name == "mousemove") {
            hovered_mesh = -1;
            for (Rml::Element* target = event.GetTargetElement(); target;
                 target = target->GetParentNode())
            {
                unsigned mesh;
                if (index(target->GetId().c_str(), "workshop-model-mesh-",
                          &mesh) &&
                    mesh < active->model.mesh_count)
                {
                    hovered_mesh = (int) mesh;
                    break;
                }
            }
            if (active->context == Context::Meshes &&
                event.GetTargetElement() == element("workshop-model-viewport"))
            {
                hovered_mesh =
                    pick_mesh(event.GetParameter<int>("mouse_x", 0),
                              event.GetParameter<int>("mouse_y", 0));
            }
            update_mesh_view();
        }
        if (name == "mousedown" && active->context == Context::Meshes &&
            event.GetParameter<int>("button", -1) == 0 &&
            event.GetTargetElement() == element("workshop-model-viewport"))
        {
            const int picked =
                pick_mesh(event.GetParameter<int>("mouse_x", 0),
                          event.GetParameter<int>("mouse_y", 0));
            if (picked >= 0) {
                active->selected_mesh = picked;
                active->selected_material = active->model.meshes[picked].material;
                render();
            }
        }

        if (name == "mousescroll" &&
            event.GetTargetElement() == element("workshop-model-viewport")) {
            nativeHome_WorkshopCameraZoom(
                event.GetParameter<float>("wheel_delta_y", 0.0f));
            event.StopPropagation();
        }
        if (name == "mousedown" &&
            event.GetTargetElement() == element("workshop-model-viewport") &&
            event.GetParameter<int>("button", -1) == 2) {
            dragging = true;
            pan = event.GetParameter<bool>("shift_key", false);
            last_x = event.GetParameter<int>("mouse_x", 0);
            last_y = event.GetParameter<int>("mouse_y", 0);
        }
        if (name == "mousedown" &&
            event.GetTargetElement() == element("workshop-model-viewport") &&
            event.GetParameter<int>("button", -1) == 0)
            stroke_begin(event.GetParameter<int>("mouse_x", 0),
                         event.GetParameter<int>("mouse_y", 0));
        if (name == "mouseup") {
            dragging = false;
            if (event.GetParameter<int>("button", -1) == 0) stroke_end();
        }
        if (name == "mousemove" && stroke.active)
            stroke_append(event.GetParameter<int>("mouse_x", 0),
                          event.GetParameter<int>("mouse_y", 0));
        if (name == "mousemove" && dragging) {
            const int x = event.GetParameter<int>("mouse_x", last_x);
            const int y = event.GetParameter<int>("mouse_y", last_y);
            nativeHome_WorkshopCameraDrag((float) (x - last_x),
                                          (float) (y - last_y), pan);
            last_x = x;
            last_y = y;
        }
    }
    void Reset() { dragging = false; nativeWorkshopSkinning_Clear(&stroke); }
private:
    bool dragging = false;
    bool pan = false;
    int last_x = 0;
    int last_y = 0;
};
CameraEvents camera_events;

void build_bones()
{
    std::string markup;
    const unsigned count = std::min<unsigned>(active->model.joint_count, 180);
    for (unsigned i = 0; i < count; ++i)
        markup += "<button id='workshop-model-bone-" +
                  std::to_string(i) + "' class='workshop-bone-marker' "
                  "title='" + escape(active->model.joints[i].name) + "'></button>";
    element("workshop-model-bone-overlay")->SetInnerRML(markup);
}

void position_bones()
{
    Rml::Element* viewport = element("workshop-model-viewport");
    Rml::Element* overlay = element("workshop-model-bone-overlay");
    overlay->SetProperty(
        "display",
        (active->bones_visible || active->context == Context::Skeleton) ? "block" : "none");
    if ((!active->bones_visible && active->context != Context::Skeleton) ||
        viewport->GetClientWidth() < 20)
    {
        return;
    }
    const float width = (float) viewport->GetClientWidth();
    const float height = (float) viewport->GetClientHeight();
    const float game_height = std::min(height, width * 0.75f);
    const float game_width = game_height * 4.0f / 3.0f;
    const float left = (width - game_width) * 0.5f;
    const float top = (height - game_height) * 0.5f;
    for (unsigned i = 0; i < std::min<unsigned>(active->model.joint_count, 180); ++i) {
        Rml::Element* marker = element(("workshop-model-bone-" +
                                       std::to_string(i)).c_str());
        if (marker == nullptr) continue;
        NativeHomeWorkshopBoneScreen screen;
        const bool shown = nativeHome_WorkshopBoneScreen(i, &screen) &&
            screen.x >= 0 && screen.x <= 1 && screen.y >= 0 && screen.y <= 1;
        marker->SetProperty("display", shown ? "block" : "none");
        if (!shown) continue;
        marker->SetProperty("left", std::to_string((int) std::lround(
            left + screen.x * game_width)) + "px");
        marker->SetProperty("top", std::to_string((int) std::lround(
            top + screen.y * game_height)) + "px");
        marker->SetClass("selected", active->selected_joints[i]);
    }
}

} // namespace WorkshopModelInternal

using namespace WorkshopModelInternal;

void nativeShellWorkshopModel_Init(Rml::ElementDocument* shell)
{
    document = shell;
    Rml::Element* viewport = element("workshop-model-viewport");
    viewport->AddEventListener("mousedown", &camera_events);
    viewport->AddEventListener("mousescroll", &camera_events);
    shell->AddEventListener("mousemove", &camera_events);
    shell->AddEventListener("mouseup", &camera_events);
    shell->AddEventListener("keydown", &camera_events, true);
}

bool nativeShellWorkshopModel_Open(unsigned member,
                                   NativeFighterProject* fighter,
                                   const NativeFighterActionInfo* moves,
                                   const bool* available, const float* lengths,
                                   size_t count, const char* path)
{
    if (member >= 2 || fighter == nullptr) return false;
    ModelSession& session = sessions[member];
    bool loaded = true;
    if (session.project != fighter) {
        // The caller closes the old draft before replacing its member projects.
        // Reopening the same live member must not erase its undo or dirty baseline.
        session = ModelSession{};
        session.project = fighter;
        session.selected_joints[0] = true;
        if(!nativeFighterCoreAssets_Capture(*fighter,session.saved_assets))return false;
        uint8_t* names=nullptr;size_t names_size=0;
        if(!nativeFighterAuthoring_Encode(fighter,&names,&names_size))return false;
        session.saved_authoring.assign(names,names+names_size);std::free(names);
        loaded = nativeFighterModel_Open(&session.model, fighter);
        if (loaded) {
            session.selected_mesh = session.model.mesh_count ? 0 : -1;
            session.selected_material = session.model.material_count ? 0 : -1;
            for (unsigned i = 0; i < session.model.joint_count; ++i) {
                for (unsigned axis = 0; axis < 3; ++axis) {
                    session.pose[i].rotation[axis] = session.model.joints[i].rotation[axis];
                    session.pose[i].scale[axis] = session.model.joints[i].scale[axis];
                    session.pose[i].position[axis] = session.model.joints[i].position[axis];
                }
            }
            for (size_t i = 0; i < count; ++i)
                if (available[i] && (!std::strcmp(moves[i].name, "Wait") ||
                                     !std::strcmp(moves[i].name, "Wait1"))) {
                    session.selected_clip = (int) i;
                    break;
                }
            if (session.selected_clip < 0)
                for (size_t i = 0; i < count; ++i)
                    if (available[i]) { session.selected_clip = (int) i; break; }
        }
    }
    std::snprintf(session.path, sizeof(session.path), "%s", path);
    session.actions = moves;
    session.has_clip = available;
    session.clip_frames = lengths;
    session.action_count = count;
    return nativeShellWorkshopModel_Select(member);
}

bool nativeShellWorkshopModel_Select(unsigned member)
{
    if (member >= 2 || sessions[member].project == nullptr) return false;
    ModelSession& next = sessions[member];
    // HSD_Dat borrows the costume buffer: rebuild its view after any structural
    // edit/undo reallocations without resetting session pose, undo or baseline.
    nativeWorkshopTransform_Clear();
    nativeModelView_Clear();
    camera_events.Reset();
    playing = false;
    hovered_mesh = -1;
    const bool loaded = nativeFighterModel_Open(&next.model, next.project);
    active = &next;
    active->model_available = loaded;
    if (!loaded) {
        // Selection still owns this member's script/action draft. A failed model
        // parse must not display or mutate the previously selected member.
        nativeShellWorkshopModel_Show(false);
        element("workshop-model-import-overlay")->SetProperty("display", "none");
        element("workshop-model-bone-overlay")->SetInnerRML("");
        element("workshop-model-tree")->SetInnerRML("");
        element("workshop-model-inspector")->SetInnerRML("");
        status("Could not read this fighter's costume model.");
        return false;
    }
    active->roles_applied = false;
    active->selected_joint = std::clamp(active->selected_joint, 0,
        std::max(0, (int) active->model.joint_count - 1));
    active->selected_mesh = active->model.mesh_count ?
        std::clamp(active->selected_mesh, 0, (int) active->model.mesh_count - 1) : -1;
    active->selected_material = active->model.material_count ?
        std::clamp(active->selected_material, 0, (int) active->model.material_count - 1) : -1;
    if (active->selected_clip < 0 ||
        (size_t) active->selected_clip >= active->action_count ||
        !active->has_clip[active->selected_clip]) active->selected_clip = -1;
    element("workshop-model-import-overlay")->SetProperty("display",
        active->import_visible ? "block" : "none");
    if (active->import_visible) render_import();
    text("workshop-model-play", "Play");
    build_clips();
    set_frame(active->frame);
    build_bones();
    render();
    buttons();
    nativeHome_WorkshopPoseInvalidate();
    return true;
}

void nativeShellWorkshopModel_Close()
{
    nativeWorkshopTransform_Clear();
    nativeModelView_Clear();
    camera_events.Reset();
    visible = false;
    playing = false;
    hovered_mesh = -1;
    for (ModelSession& session : sessions) session = ModelSession{};
    active = &sessions[0];
    element("workshop-model-import-overlay")->SetProperty("display", "none");
    element("workshop-model-bone-overlay")->SetInnerRML("");
}

void nativeShellWorkshopModel_Show(bool show)
{
    if (!show) {
        nativeModelView_Clear();
    }
    visible = show && active->project != nullptr && active->model_available;
    element("workshop-model-pane")->SetProperty("display",
        visible ? "block" : "none");
    if (visible) {
        render();
        buttons();
        nativeHome_WorkshopPoseInvalidate();
    } else {
        playing = false;
        camera_events.Reset();
        nativeHome_WorkshopPoseInvalidate();
    }
}

bool nativeShellWorkshopModel_IsProperty(const char* id)
{
    if (rebuilding_model_inspector) return false;
    const std::string_view key(id);
    return key == "workshop-model-name" || key == "workshop-model-assigned-material" ||
        key == "workshop-model-diffuse" || key == "workshop-model-alpha" ||
        key == "workshop-model-shininess" || key.starts_with("workshop-model-pos-") ||
        key.starts_with("workshop-model-rot-") || key.starts_with("workshop-model-scale-") ||
        key.starts_with("workshop-material-") || key.starts_with("workshop-texture-");
}

bool nativeShellWorkshopModel_CommitProperty(const char* id)
{
    if (!visible || !active->project || !nativeShellWorkshopModel_IsProperty(id)) return false;
    const std::string_view key(id);
    if (!std::strcmp(id, "workshop-model-name")) {
        unsigned category =
            active->context == Context::Meshes      ? NativeAuthoring_Mesh
            : active->context == Context::Materials ? NativeAuthoring_Material
                                                    : NativeAuthoring_Bone;
        int selected = category == NativeAuthoring_Mesh ? active->selected_mesh
                       : category == NativeAuthoring_Material
                           ? active->selected_material
                           : active->selected_joint;
        if (selected < 0) {
            return true;
        }
        auto before_labels = label_snapshot();
        bool ok = nativeFighterAuthoring_SetName(
                      active->project, category,
                      category == NativeAuthoring_Mesh
                          ? active->model.meshes[selected].offset
                      : category == NativeAuthoring_Material
                          ? active->model.materials[selected].offset
                          : active->model.joints[selected].offset,
                      input("workshop-model-name")->GetValue().c_str());
        if (ok) {
            record_label_change(before_labels, false);
        }
        reopen_model();
        render();
        status(ok ? "Name applied. Save to keep it."
                  : "Could not save this name. Use up to 63 bytes without "
                    "tabs or newlines.");
        return true;
    }
    if (key.starts_with("workshop-model-pos-") ||
        key.starts_with("workshop-model-rot-") || key.starts_with("workshop-model-scale-")) {
        if (!apply_joint_transform()) status("Invalid bone transform.");
        render_inspector();
        return true;
    }
    if (std::strcmp(id, "workshop-model-assigned-material") == 0) {
        if (active->selected_mesh < 0) return true;
        const unsigned material = (unsigned) std::strtoul(
            static_cast<Rml::ElementFormControlSelect*>(
                element("workshop-model-assigned-material"))->GetValue().c_str(),
            nullptr, 10);
        if (material >= active->model.material_count) return true;
        const uint32_t offset = 0x20 + active->model.meshes[active->selected_mesh].offset + 8;
        uint8_t before[4];
        std::memcpy(before, active->project->costume_dat + offset, 4);
        if (nativeFighterModel_SetMeshMaterial(&active->model, active->selected_mesh,
                                                material)) {
            record(offset, 4, before);
            active->selected_material = (int) material;
            render();
            status(nativeShellWorkshop_RefreshPreview()
                ? "Mesh material changed; refreshing the live preview."
                : "Mesh material changed. Test Character to reload it.");
        }
        return true;
    }
    apply_surface();
    return true;
}

bool nativeShellWorkshopModel_Click(const char* id, bool shift)
{
    if (rig_click(id)) return true;
    if (import_click(id)) return true;
    if (!std::strcmp(id, "workshop-model-texture-image-apply")) {
        std::string value = static_cast<Rml::ElementFormControlSelect*>(
                                element("workshop-model-texture-image"))
                                ->GetValue();
        unsigned sequence = 0, frame = 0;
        if (std::sscanf(value.c_str(), "%u:%u", &sequence, &frame) != 2) {
            return true;
        }
        const std::vector<uint8_t> before(active->project->costume_dat,
                                          active->project->costume_dat +
                                              active->project->costume_size);
        char error[192] = {};
        if (nativeFighterGltf_SetTextureImage(
                active->project, (unsigned) active->selected_material, 0,
                sequence, frame,
                input("workshop-model-texture-path")->GetValue().c_str(),
                error, sizeof(error)))
        {
            record_asset(before);
            reopen_model();
            nativeShellWorkshop_RefreshPreview();
            status("Texture-sequence image replaced. Its timing is unchanged; "
                   "Save to keep the edit.");
        } else {
            status(error);
        }
        return true;
    }

    if (!visible || active->project == nullptr) return false;
    const char* contexts[] = { "meshes", "materials", "skeleton", "skinning" };
    for (unsigned i = 0; i < 4; ++i)
        if (std::strcmp(id, (std::string("workshop-model-context-") +
                            contexts[i]).c_str()) == 0) {
            active->context = (Context) i;
            reveal_material = active->context == Context::Materials ? 2 : 0;
            if (active->context == Context::Skinning) {
                if (active->selected_joint < 0 ||
                    !(active->model.joints[active->selected_joint].flags & JOBJ_SKELETON))
                    for (unsigned bone = 0; bone < active->model.joint_count; ++bone)
                        if (active->model.joints[bone].flags & JOBJ_SKELETON) {
                            active->selected_joint = (int) bone;
                            std::memset(active->selected_joints, 0,
                                        sizeof(active->selected_joints));
                            active->selected_joints[bone] = true;
                            break;
                        }
            }
            render();
            return true;
        }
    const char* modes[] = { "edit", "pose", "animation" };
    for (unsigned i = 0; i < 3; ++i)
        if (std::strcmp(id, (std::string("workshop-model-rig-") +
                            modes[i]).c_str()) == 0) {
            nativeWorkshopTransform_Clear();
            if ((RigMode) i == RigMode::Pose && !active->pose_initialized) {
                for (unsigned bone = 0; bone < active->model.joint_count; ++bone) {
                    active->pose_changed[bone] =
                        nativeHome_WorkshopBoneLocal(bone, &active->pose[bone]);
                }
                active->pose_initialized = true;
            }
            active->rig_mode = (RigMode) i;
            playing = false;
            nativeHome_WorkshopPoseInvalidate();
            render();
            return true;
        }
    unsigned chosen;
    if (index(id, "workshop-model-mesh-", &chosen)) {
        if (chosen < active->model.mesh_count) {
            active->selected_mesh = (int) chosen;
            active->selected_material = active->model.meshes[chosen].material;
            render();
        }
        return true;
    }
    if (index(id, "workshop-model-material-", &chosen)) {
        if (chosen < active->model.material_count) {
            active->selected_material = (int) chosen;
            render();
        }
        return true;
    }
    if (index(id, "workshop-model-joint-", &chosen) ||
        index(id, "workshop-model-bone-", &chosen)) {
        if (chosen < active->model.joint_count) {
            if (!shift) {
                std::memset(active->selected_joints, 0, sizeof(active->selected_joints));
            }
            active->selected_joints[chosen] = !shift || !active->selected_joints[chosen];
            active->selected_joint = (int) chosen;
            if (!active->selected_joints[chosen]) {
                for (unsigned i = 0; i < active->model.joint_count; ++i) {
                    if (active->selected_joints[i]) {
                        active->selected_joint = (int) i;
                        break;
                    }
                }
            }
            render();
        }
        return true;
    }
    if (std::strcmp(id, "workshop-model-bones") == 0) {
        if (active->context != Context::Skeleton) {
            active->bones_visible = !active->bones_visible;
        }
        render();
        return true;
    }
    if (index(id, "workshop-model-eye-", &chosen)) {
        if (chosen < active->model.mesh_count) {
            active->hidden_meshes[chosen] = !active->hidden_meshes[chosen];
            hovered_mesh = -1;
            render_tree();
            update_mesh_view();
        }
        return true;
    }
    if (std::strcmp(id, "workshop-model-wireframe") == 0) {
        status("Wireframe rendering is not available in the native renderer yet.");
        return true;
    }
    if (std::strcmp(id, "workshop-model-reset-pose") == 0) {
        nativeWorkshopTransform_Clear();
        active->pose_initialized = true;
        for (unsigned i = 0; i < active->model.joint_count; ++i) {
            active->pose_changed[i] = true;
            for (unsigned axis = 0; axis < 3; ++axis) {
                active->pose[i].rotation[axis] = active->model.joints[i].rotation[axis];
                active->pose[i].scale[axis] = active->model.joints[i].scale[axis];
                active->pose[i].position[axis] = active->model.joints[i].position[axis];
            }
        }
        nativeHome_WorkshopPoseInvalidate();
        render_inspector();
        status("Temporary pose reset.");
        return true;
    }
    if (std::strcmp(id, "workshop-model-texture-browse") == 0) {
        char path[MAX_PATH] = {};
        OPENFILENAMEA picker = {};
        picker.lStructSize = sizeof(picker);
        picker.hwndOwner = GetActiveWindow();
        picker.lpstrFilter = "Images (*.png;*.jpg;*.jpeg)\0*.png;*.jpg;*.jpeg\0"
                             "All files (*.*)\0*.*\0\0";
        picker.lpstrFile = path;
        picker.nMaxFile = sizeof(path);
        picker.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
        if (GetOpenFileNameA(&picker))
            input("workshop-model-texture-path")->SetValue(path);
        return true;
    }
    if (std::strcmp(id, "workshop-model-texture-apply") == 0) {
        apply_texture(); return true;
    }
    if (std::strcmp(id, "workshop-model-edit-material") == 0) {
        active->context = Context::Materials;
        render();
        reveal_material = 2;
        return true;
    }
    if (std::strcmp(id, "workshop-model-play") == 0) {
        if (active->rig_mode != RigMode::Animation) return true;
        if (!playing) {
            playing = true;
            play_start_frame = active->frame;
            play_started = std::chrono::steady_clock::now();
            text("workshop-model-play", "Pause");
        } else {
            playing = false;
            text("workshop-model-play", "Play");
        }
        return true;
    }
    if (std::strcmp(id, "workshop-model-front") == 0 ||
        std::strcmp(id, "workshop-model-frame-all") == 0) {
        nativeHome_WorkshopCameraReset(
            std::strcmp(id, "workshop-model-front") == 0);
        return true;
    }
    return false;
}

static bool commit_model_pose(const NativeWorkshopTransformChange* edits,
                              unsigned count)
{
    for (unsigned i = 0; i < count; ++i) {
        active->pose[edits[i].joint] = edits[i].after;
        active->pose_changed[edits[i].joint] = true;
    }
    render_inspector();
    status("Preview pose changed. The rest rig and saved animations are "
           "unchanged.");
    return true;
}

void nativeShellWorkshopModel_Update()
{
    if (reveal_material && --reveal_material == 0 && active->selected_material >= 0) {
        if (auto* row = document->GetElementById("workshop-model-material-" + std::to_string(active->selected_material)))
            row->ScrollIntoView(true);
    }
    if (!visible || active->project == nullptr || active->selected_clip < 0) return;
    if (!active->roles_applied && nativeHome_WorkshopBoneCount() != 0) {
        for (unsigned i = 0; i < active->model.joint_count; ++i) {
            const char* role = nativeHome_WorkshopBoneRole(i);
            const char* custom_name=nativeFighterAuthoring_Name(active->project,NativeAuthoring_Bone,active->model.joints[i].offset);
            if (role && (!custom_name || !*custom_name) && std::strncmp(active->model.joints[i].name, "Joint ", 6) == 0)
                std::snprintf(active->model.joints[i].name,
                              sizeof(active->model.joints[i].name),
                              "%s · joint %u", role, i);
        }
        active->roles_applied = true;
        build_bones();
        render();
    }
    if (active->context == Context::Skinning) {
        auto* mesh = static_cast<Rml::ElementFormControlSelect*>(
            element("workshop-model-skin-mesh"));
        const int wanted = std::atoi(mesh->GetValue().c_str());
        if (wanted >= 0 && wanted < active->model.mesh_count &&
            wanted != active->selected_mesh) {
            active->selected_mesh = wanted;
            render_inspector();
        }
    }
    if (active->rig_mode == RigMode::Animation) {
        const auto* clip = static_cast<Rml::ElementFormControlSelect*>(
            element("workshop-model-clip"));
        const int next_clip = std::atoi(clip->GetValue().c_str());
        if (next_clip >= 0 && next_clip < (int) active->action_count &&
            active->has_clip[next_clip] && active->selected_clip != next_clip) {
            active->selected_clip = next_clip;
            input("workshop-model-frame")->SetAttribute("max",
                std::to_string((int) std::max(1.0f,
                    std::ceil(active->clip_frames[active->selected_clip]))));
            set_frame(0);
            playing = false;
            text("workshop-model-play", "Play");
        }
        if (playing) {
            const int elapsed = (int) std::floor(
                std::chrono::duration<float>(
                    std::chrono::steady_clock::now() - play_started).count() *
                60.0f);
            const int count = (int) std::max(1.0f,
                std::ceil(active->clip_frames[active->selected_clip]));
            const int next = (play_start_frame + elapsed) % (count + 1);
            if (active->frame != next) set_frame(next);
        } else {
            const int next = std::atoi(input("workshop-model-frame")
                                           ->GetValue().c_str());
            if (active->frame != next) set_frame(next);
        }
    }
    const float pose_frame = active->rig_mode == RigMode::Animation ? (float) active->frame : 0.0f;
    if (nativeHome_WorkshopPose({NativeFighterAnimation_Action,
                               (unsigned) active->selected_clip}, pose_frame, nullptr) &&
        active->rig_mode != RigMode::Animation) {
        for (unsigned i = 0; i < active->model.joint_count; ++i) {
            if (active->rig_mode == RigMode::Pose && !active->pose_changed[i]) continue;
            NativeHomeWorkshopBoneLocal local = {};
            for (unsigned a = 0; a < 3; ++a) {
                local.rotation[a] = active->rig_mode == RigMode::Edit ?
                    active->model.joints[i].rotation[a] : active->pose[i].rotation[a];
                local.scale[a] = active->rig_mode == RigMode::Edit ?
                    active->model.joints[i].scale[a] : active->pose[i].scale[a];
                local.position[a] = active->rig_mode == RigMode::Edit ?
                    active->model.joints[i].position[a] : active->pose[i].position[a];
            }
            nativeHome_WorkshopSetBoneLocal(i, &local);
        }
    }
    if (active->rig_mode == RigMode::Pose && active->context == Context::Skeleton) {
        unsigned joints[NATIVE_MODEL_MAX_JOINTS], count = 0;
        for (unsigned i = 0; i < active->model.joint_count; ++i) {
            if (active->selected_joints[i]) {
                joints[count++] = i;
            }
        }
        nativeWorkshopTransform_Update("workshop-model-viewport", joints,
                                       count, (unsigned) active->selected_joint,
                                       (uint64_t) active->project, commit_model_pose);
    } else {
        nativeWorkshopTransform_Clear();
    }
    position_bones();
    update_mesh_view();
}

bool nativeShellWorkshopModel_HasUnsavedEdits(unsigned member)
{
    if (member >= 2) return false;
    const ModelSession& session = sessions[member];
    if(!session.project)return false;
    if(!nativeFighterCoreAssets_Matches(*session.project,session.saved_assets))return true;
    uint8_t* names=nullptr;size_t size=0;
    if(!nativeFighterAuthoring_Encode(session.project,&names,&size))return true;
    const bool changed=size!=session.saved_authoring.size() ||
        (size && std::memcmp(names,session.saved_authoring.data(),size));
    std::free(names);return changed;
}

void nativeShellWorkshopModel_Inspect()
{
    std::fprintf(stderr, "[workshop-model] visible=%u context=%u mode=%u mesh=%d joint=%d clip=%d frame=%d playing=%u joints=%u revision=%llu\n",
                 visible, (unsigned) active->context, (unsigned) active->rig_mode,
                 active->selected_mesh, active->selected_joint, active->selected_clip, active->frame, playing,
                 active->model.joint_count, (unsigned long long)(active->project?active->project->rig_edit_revision:0));
    if (visible && active->project && active->selected_mesh >= 0)
        nativeWorkshopSkinning_Inspect(active->project, (unsigned) active->selected_mesh);
    if (visible && active->pose_initialized)
        for (unsigned i = 0; i < active->model.joint_count; ++i) {
            const auto& pose = active->pose[i];
            std::fprintf(stderr, "[workshop-model-pose] index=%u offset=%u selected=%u apply=%u local=", i,
                         (unsigned)active->model.joints[i].offset, active->selected_joints[i], active->pose_changed[i]);
            for (unsigned a = 0; a < 3; ++a)
                std::fprintf(stderr, "%.9f,%.9f,%.9f,", pose.position[a], pose.rotation[a], pose.scale[a]);
            std::fprintf(stderr, "\n");
        }
}

void nativeShellWorkshopModel_MarkSaved(unsigned member)
{
    if (member >= 2) return;
    ModelSession& session = sessions[member];
    if (session.project == nullptr) return;
    nativeFighterCoreAssets_Capture(*session.project,session.saved_assets);
    uint8_t* names=nullptr;size_t size=0;
    if(nativeFighterAuthoring_Encode(session.project,&names,&size)) {
        session.saved_authoring.assign(names,names+size);std::free(names);
    }
}


int nativeShellWorkshopModel_SelectedClip()
{
    return active->selected_clip;
}

#endif
