#include "native_shell_workshop_hitboxes.h"
#include <melee/lb/native_math_functions.h>

#ifdef MELEE_NATIVE

#include <melee/ft/native_fighter_action_editor.h>
#include <melee/gm/native_home.h>
#include <RmlUi/Core.h>

#include <algorithm>
#include <climits>
#include <cmath>
#include <string>

namespace {

Rml::ElementDocument* document;
Rml::Element* markers[4];
Rml::Element* arrows[4];
Rml::Element* damage_labels[4];
Rml::Element* special_labels[4];
struct MarkerPosition {
    int x;
    int y;
    int arrow_degrees;
    int arrow_length;
    unsigned damage;
    unsigned source_angle;
    bool visible;
    bool arrow_visible;
    bool special_visible;
    bool selected;
};
MarkerPosition positions[4];

} // namespace

void nativeShellWorkshopHitboxes_Init(Rml::ElementDocument* shell)
{
    document = shell;
    for (unsigned id = 0; id < 4; ++id) {
        markers[id] = document->GetElementById(
            ("workshop-viewport-hitbox-" + std::to_string(id)).c_str());
        arrows[id] = document->GetElementById(
            ("workshop-hitbox-arrow-" + std::to_string(id)).c_str());
        damage_labels[id] = document->GetElementById(
            ("workshop-hitbox-damage-" + std::to_string(id)).c_str());
        special_labels[id] = document->GetElementById(
            ("workshop-hitbox-special-" + std::to_string(id)).c_str());
        positions[id] = { -1, -1, INT_MIN, -1, UINT_MAX, UINT_MAX,
                          false, false, false, false };
    }
}

void nativeShellWorkshopHitboxes_Update(bool visible, int selected_hitbox,
                                       const NativeFighterHitbox* hitboxes,
                                       unsigned active_mask)
{
    if (document == nullptr) return;
    document->GetElementById("workshop-action-hitboxes")
        ->SetProperty("display", visible ? "block" : "none");
    if (!visible) return;
    Rml::Element* viewport = document->GetElementById(
        "workshop-action-viewport");
    const int width = viewport->GetClientWidth();
    const int height = viewport->GetClientHeight();
    if (width <= 0 || height <= 0) return;
    const float game_height = std::min(float(height), width * 0.75f);
    const float game_width = game_height * 4.0f / 3.0f;
    const float left = (width - game_width) * 0.5f;
    const float top = (height - game_height) * 0.5f;
    for (unsigned id = 0; id < 4; ++id) {
        NativeHomeWorkshopHitboxScreen screen = {};
        const bool shown = hitboxes != nullptr &&
            (active_mask & (1u << id)) != 0 &&
            nativeHome_WorkshopHitboxScreen(id, hitboxes[id].angle, &screen) &&
            screen.x >= 0.0f && screen.x <= 1.0f &&
            screen.y >= 0.0f && screen.y <= 1.0f;
        if (shown != positions[id].visible) {
            markers[id]->SetProperty("display", shown ? "block" : "none");
            positions[id].visible = shown;
        }
        if (!shown) {
            if (positions[id].arrow_visible) {
                arrows[id]->SetProperty("display", "none");
                positions[id].arrow_visible = false;
            }
            continue;
        }
        const int px = (int) std::lround(left + screen.x * game_width);
        const int py = (int) std::lround(top + screen.y * game_height);
        if (px != positions[id].x) {
            markers[id]->SetProperty("left", std::to_string(px) + "px");
            arrows[id]->SetProperty("left", std::to_string(px) + "px");
            positions[id].x = px;
        }
        if (py != positions[id].y) {
            markers[id]->SetProperty("top", std::to_string(py) + "px");
            arrows[id]->SetProperty("top", std::to_string(py) + "px");
            positions[id].y = py;
        }
        const unsigned damage = hitboxes[id].damage;
        const unsigned angle = hitboxes[id].angle;
        if (damage != positions[id].damage || angle != positions[id].source_angle) {
            damage_labels[id]->SetInnerRML(std::to_string(damage) + "%");
            const std::string description = angle == 361
                ? "Sakurai angle (context-dependent)"
                : angle > 360
                    ? "Special angle " + std::to_string(angle)
                    : std::to_string(angle) +
                      " degree launch angle, relative to facing";
            markers[id]->SetAttribute("title", "Hitbox #" +
                std::to_string(id) + " · " + std::to_string(damage) +
                "% base damage · " + description);
            if (angle > 360)
                special_labels[id]->SetInnerRML(angle == 361
                    ? "Sakurai" : "Angle " + std::to_string(angle));
            positions[id].damage = damage;
            positions[id].source_angle = angle;
        }
        const bool special = angle > 360;
        if (special != positions[id].special_visible) {
            special_labels[id]->SetProperty("display", special ? "block" : "none");
            positions[id].special_visible = special;
        }
        const float dx = screen.direction_x * game_width;
        const float dy = screen.direction_y * game_height;
        const float projected_length = cr_hypotf(dx, dy);
        const bool arrow_visible = !special && screen.direction_valid &&
            std::isfinite(projected_length) && projected_length >= 2.0f;
        if (arrow_visible != positions[id].arrow_visible) {
            arrows[id]->SetProperty("display", arrow_visible ? "block" : "none");
            positions[id].arrow_visible = arrow_visible;
        }
        if (arrow_visible) {
            const int length = (int) std::lround(std::clamp(
                projected_length, 24.0f, 42.0f));
            const int degrees = (int) std::lround(cr_atan2f(dy, dx) *
                (180.0f / 3.14159265358979323846f));
            if (length != positions[id].arrow_length) {
                arrows[id]->SetProperty("width", std::to_string(length) + "px");
                positions[id].arrow_length = length;
            }
            if (degrees != positions[id].arrow_degrees) {
                arrows[id]->SetProperty("transform", "rotate(" +
                    std::to_string(degrees) + "deg)");
                positions[id].arrow_degrees = degrees;
            }
        }
        const bool selected = (int) id == selected_hitbox;
        if (selected != positions[id].selected) {
            markers[id]->SetClass("selected", selected);
            positions[id].selected = selected;
        }
    }
}

#endif
