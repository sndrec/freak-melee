#include "native_shell_workshop_bones.h"

#ifdef MELEE_NATIVE

#include <melee/ft/native_fighter_animation_editor.h>
#include <melee/gm/native_home.h>
#include <RmlUi/Core.h>

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace {
constexpr unsigned MAX_BONE_MARKERS = 140;
Rml::ElementDocument* document;
const NativeFighterProject* shown_project;
NativeFighterAnimationClip shown_clip = { NativeFighterAnimation_Action, UINT_MAX };
unsigned joints[MAX_BONE_MARKERS];
unsigned joint_count;
struct MarkerPosition { int x; int y; bool visible; };
MarkerPosition action_positions[MAX_BONE_MARKERS];
MarkerPosition animation_positions[MAX_BONE_MARKERS];
struct LinkPosition { int x; int y; int dx; int dy; bool visible; bool selected; };
LinkPosition action_link_positions[MAX_BONE_MARKERS];
LinkPosition animation_link_positions[MAX_BONE_MARKERS];
Rml::Element* action_markers[MAX_BONE_MARKERS];
Rml::Element* animation_markers[MAX_BONE_MARKERS];
Rml::Element* action_links[MAX_BONE_MARKERS];
Rml::Element* animation_links[MAX_BONE_MARKERS];
unsigned parent_joints[MAX_BONE_MARKERS];
bool hierarchy_ready;

void rebuild(const NativeFighterProject* project, NativeFighterAnimationClip clip)
{
    shown_project = project;
    shown_clip = clip;
    joint_count = 0;
    hierarchy_ready = false;
    std::fill(action_positions, action_positions + MAX_BONE_MARKERS,
              MarkerPosition { -1, -1, false });
    std::fill(animation_positions, animation_positions + MAX_BONE_MARKERS,
              MarkerPosition { -1, -1, false });
    std::fill(action_link_positions, action_link_positions + MAX_BONE_MARKERS,
              LinkPosition { -1, -1, 0, 0, false, false });
    std::fill(animation_link_positions, animation_link_positions + MAX_BONE_MARKERS,
              LinkPosition { -1, -1, 0, 0, false, false });
    NativeFighterAnimationTrack tracks[1024];
    size_t count;
    if (project != nullptr && clip.index != UINT_MAX &&
        nativeFighterAnimation_Tracks(project, clip,
                                      tracks, 1024, &count)) {
        bool seen[MAX_BONE_MARKERS] = {};
        for (size_t i = 0; i < count; ++i) {
            const unsigned joint = tracks[i].joint;
            if (joint >= MAX_BONE_MARKERS || seen[joint]) continue;
            seen[joint] = true;
            joints[joint_count++] = joint;
        }
    }
    for (const char* prefix : { "workshop-action-bones",
                                "workshop-animation-bones" }) {
        std::string markup;
        const bool action_overlay = std::strcmp(prefix,
            "workshop-action-bones") == 0;
        for (unsigned i = 0; i < joint_count; ++i) {
            const std::string link = action_overlay ? "workshop-action-link-" :
                                                      "workshop-animation-link-";
            markup += "<div id='" + link + std::to_string(joints[i]) +
                      "' class='workshop-bone-link'></div>";
        }
        for (unsigned i = 0; i < joint_count; ++i) {
            const std::string id = action_overlay ? "workshop-action-bone-" :
                                                    "workshop-animation-bone-";
            markup += "<button id='" + id + std::to_string(joints[i]) +
                      "' class='workshop-bone-marker' title='Joint " +
                      std::to_string(joints[i]) + "'></button>";
        }
        document->GetElementById(prefix)->SetInnerRML(markup);
    }
    for (unsigned i = 0; i < joint_count; ++i) {
        action_markers[i] = document->GetElementById(
            ("workshop-action-bone-" + std::to_string(joints[i])).c_str());
        animation_markers[i] = document->GetElementById(
            ("workshop-animation-bone-" + std::to_string(joints[i])).c_str());
        action_links[i] = document->GetElementById(
            ("workshop-action-link-" + std::to_string(joints[i])).c_str());
        animation_links[i] = document->GetElementById(
            ("workshop-animation-link-" + std::to_string(joints[i])).c_str());
    }
}

void find_hierarchy()
{
    if (hierarchy_ready || joint_count == 0) return;
    bool available = false;
    for (unsigned i = 0; i < joint_count; ++i) {
        parent_joints[i] = UINT_MAX;
        unsigned parent;
        if (nativeHome_WorkshopBoneParent(joints[i], &parent)) {
            parent_joints[i] = parent;
            available = true;
        }
    }
    hierarchy_ready = available;
}

void position(const char* viewport_id, Rml::Element** markers,
              Rml::Element** links, MarkerPosition* positions,
              LinkPosition* link_positions,
              int selected_joint)
{
    Rml::Element* viewport = document->GetElementById(viewport_id);
    const int width = viewport->GetClientWidth();
    const int height = viewport->GetClientHeight();
    if (width <= 0 || height <= 0) return;
    const float game_height = std::min(float(height), width * 0.75f);
    const float game_width = game_height * 4.0f / 3.0f;
    const float left = (width - game_width) * 0.5f;
    const float top = (height - game_height) * 0.5f;
    for (unsigned i = 0; i < joint_count; ++i) {
        Rml::Element* marker = markers[i];
        if (marker == nullptr) continue;
        NativeHomeWorkshopBoneScreen screen;
        const bool visible = nativeHome_WorkshopBoneScreen(joints[i], &screen) &&
            screen.x >= 0.0f && screen.x <= 1.0f &&
            screen.y >= 0.0f && screen.y <= 1.0f;
        if (visible != positions[i].visible) {
            marker->SetProperty("display", visible ? "block" : "none");
            positions[i].visible = visible;
        }
        if (!visible) continue;
        const int x = (int) std::lround(left + screen.x * game_width);
        const int y = (int) std::lround(top + screen.y * game_height);
        if (x != positions[i].x) {
            marker->SetProperty("left", std::to_string(x) + "px");
            positions[i].x = x;
        }
        if (y != positions[i].y) {
            marker->SetProperty("top", std::to_string(y) + "px");
            positions[i].y = y;
        }
        marker->SetClass("selected", (int) joints[i] == selected_joint);
    }
    for (unsigned i = 0; i < joint_count; ++i) {
        Rml::Element* link = links[i];
        if (link == nullptr) continue;
        unsigned parent_index = joint_count;
        for (unsigned j = 0; j < joint_count; ++j)
            if (joints[j] == parent_joints[i]) {
                parent_index = j;
                break;
            }
        const bool visible = hierarchy_ready && parent_index < joint_count &&
            positions[i].visible && positions[parent_index].visible;
        LinkPosition& last = link_positions[i];
        if (last.visible != visible) {
            link->SetProperty("display", visible ? "block" : "none");
            last.visible = visible;
        }
        if (!visible) continue;
        const int dx = positions[i].x - positions[parent_index].x;
        const int dy = positions[i].y - positions[parent_index].y;
        const int x = positions[parent_index].x;
        const int y = positions[parent_index].y;
        if (last.x != x || last.y != y || last.dx != dx || last.dy != dy) {
            const float length = std::sqrt(float(dx * dx + dy * dy));
            const float angle = std::atan2(float(dy), float(dx)) *
                                (180.0f / 3.14159265f);
            link->SetProperty("left", std::to_string(x) + "px");
            link->SetProperty("top", std::to_string(y) + "px");
            link->SetProperty("width", std::to_string(length) + "px");
            link->SetProperty("transform", "rotate(" +
                              std::to_string(angle) + "deg)");
            last.x = x;
            last.y = y;
            last.dx = dx;
            last.dy = dy;
        }
        const bool selected = (int) joints[i] == selected_joint;
        if (last.selected != selected) {
            link->SetClass("selected", selected);
            last.selected = selected;
        }
    }
}
} // namespace

void nativeShellWorkshopBones_Init(Rml::ElementDocument* shell)
{
    document = shell;
    nativeShellWorkshopBones_Reset();
}

void nativeShellWorkshopBones_Reset()
{
    shown_project = nullptr;
    shown_clip.index = UINT_MAX;
    joint_count = 0;
    if (document != nullptr) {
        document->GetElementById("workshop-action-bones")->SetInnerRML("");
        document->GetElementById("workshop-animation-bones")->SetInnerRML("");
    }
}

void nativeShellWorkshopBones_Update(const NativeFighterProject* project,
    NativeFighterAnimationClip clip, int selected_joint, bool action_visible, bool animation_visible)
{
    if (document == nullptr) return;
    if (project != shown_project || clip.bank != shown_clip.bank || clip.index != shown_clip.index)
        rebuild(project, clip);
    document->GetElementById("workshop-action-bones")
        ->SetProperty("display", action_visible ? "block" : "none");
    document->GetElementById("workshop-animation-bones")
        ->SetProperty("display", animation_visible ? "block" : "none");
    if (action_visible || animation_visible) find_hierarchy();
    if (action_visible)
        position("workshop-action-viewport", action_markers, action_links,
                 action_positions, action_link_positions, selected_joint);
    if (animation_visible)
        position("workshop-animation-viewport", animation_markers,
                 animation_links,
                 animation_positions, animation_link_positions,
                 selected_joint);
}

bool nativeShellWorkshopBones_Click(const char* id, unsigned* joint)
{
    if (id == nullptr || joint == nullptr) return false;
    const char* number = nullptr;
    if (std::strncmp(id, "workshop-action-bone-", 21) == 0)
        number = id + 21;
    else if (std::strncmp(id, "workshop-animation-bone-", 24) == 0)
        number = id + 24;
    if (number == nullptr) return false;
    unsigned parsed;
    char trailing;
    if (std::sscanf(number, "%u%c", &parsed, &trailing) != 1)
        return false;
    *joint = parsed;
    return true;
}

#endif
