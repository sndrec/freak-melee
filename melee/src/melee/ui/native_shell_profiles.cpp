#include <melee/ui/native_hub.h>
#define _CRT_RAND_S
#include "native_shell_profiles.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "native_shell.h"
#include "native_shell_rules.h"
#include "native_shell_collection.h"
#include <melee/gm/native_profile_input.h>
#include <melee/gm/native_home.h>
#include <RmlUi/Core.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <RmlUi/Core/Elements/ElementFormControlSelect.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_timer.h>

namespace {
    Rml::ElementDocument* document;
    NativeProfileEntry catalog[64];
    size_t catalogCount;
    NativeProfile draft;
    char draftId[NATIVE_PROFILE_ID_BYTES + 1];
    unsigned port;
    bool opened, updating;
    // Capture waits for neutral, then an intentional press, then release.
    int captureAction = -1, captureAxis = -1;
    bool captureAdd, captureReady, captureRelease;
    uint64_t captureDeadline;
    uint64_t hideAfterSlide;
    uint64_t lastLiveUpdate;
    int lastController = -2;
    std::string displayedZones[4];
    uint16_t displayedZoneValues[4];
    const char* const actionNames[] = { "Attack", "Special", "Jump",
                                        "Shield", "Grab",    "Short hop" };
    const char* const axisNames[] = { "Move right",         "Move up",
                                      "Attack stick right", "Attack stick up",
                                      "Left trigger",       "Right trigger" };
    const char* const zoneIds[] = { "profile-main-zone", "profile-c-zone",
                                    "profile-l-zone", "profile-r-zone" };

    Rml::ElementFormControlInput* input(const char* id)
    {
        return static_cast<Rml::ElementFormControlInput*>(
            document->GetElementById(id));
    }

    void text(const char* id, const std::string& value)
    {
        document->GetElementById(id)->SetInnerRML(
            Rml::StringUtilities::EncodeRml(value));
    }

    void status(const char* value)
    {
        text("profile-status", value);
    }

    void stop_capture()
    {
        captureAction = captureAxis = -1;
        captureReady = captureRelease = false;
        document->GetElementById("profile-cancel")
            ->SetProperty("display", "none");
    }

    std::string binding_name(const NativeProfileBinding& binding)
    {
        if (binding.type == NativeProfileBinding_Default) {
            return "Default";
        }
        const char* name;
        if (binding.type == NativeProfileBinding_Button) {
            name = PADGetNativeButtonName(binding.native_input);
        } else {
            name = PADGetNativeAxisName(
                { binding.native_input, AXIS_SIGN_POSITIVE });
        }
        std::string result = name ? name : "Unknown input";
        if (binding.type == NativeProfileBinding_AxisPositive) {
            result += " +";
        }
        if (binding.type == NativeProfileBinding_AxisNegative) {
            result += " -";
        }
        return result;
    }

    void render_bindings()
    {
        std::string rml;
        for (int action = 0; action < NativeProfileAction_Count; ++action) {
            const auto& bindings = draft.actions[action];
            std::string value;
            for (int i = 0; i < bindings.count; ++i) {
                if (i) {
                    value += ", ";
                }
                value += binding_name(bindings.bindings[i]);
            }
            if (value.empty()) {
                value = "Unbound";
            }
            const std::string n = std::to_string(action);
            rml += "<div class=\"profile-binding\"><div "
                   "class=\"profile-binding-label\">";
            rml += actionNames[action];
            rml += "</div><div class=\"profile-binding-value\">";
            rml += Rml::StringUtilities::EncodeRml(value);
            rml += "</div><div class=\"profile-binding-buttons\">";
            rml += "<button id=\"bind-" + n + "\">Bind</button>";
            rml += "<button id=\"add-" + n + "\">+</button>";
            rml += "<button id=\"clear-" + n + "\">Clear</button></div></div>";
        }
        document->GetElementById("profile-actions")->SetInnerRML(rml);
        rml.clear();
        for (int axis = 0; axis < NativeProfileAxis_Count; ++axis) {
            const auto& binding = draft.axes[axis];
            NativeProfileBinding display = {
                binding.native_axis,
                uint8_t(binding.direction < 0
                            ? NativeProfileBinding_AxisNegative
                            : NativeProfileBinding_AxisPositive)
            };
            const std::string n = std::to_string(axis);
            rml += "<div class=\"profile-binding\"><div "
                   "class=\"profile-binding-label\">";
            rml += axisNames[axis];
            rml += "</div><div class=\"profile-binding-value\">";
            rml += Rml::StringUtilities::EncodeRml(
                binding.assigned ? binding_name(display) : "Default");
            rml += "</div><div class=\"profile-binding-buttons\"><button "
                   "id=\"axis-" +
                   n + "\">Bind</button>";
            rml += "<button id=\"axisdefault-" + n +
                   "\">Default</button></div></div>";
        }
        document->GetElementById("profile-axes")->SetInnerRML(rml);
    }

    void render_options()
    {
        text("profile-tap", draft.tap_jump ? "On" : "Off");
        text("profile-buffer", draft.action_buffer_frames
                                   ? std::to_string(draft.action_buffer_frames)
                                   : "Off");
        text("profile-cstick",
             draft.c_stick == NativeProfileCStick_Smash ? "Smash" : "Tilt");
        text("profile-deadzones",
             draft.calibration.use_dead_zones ? "On" : "Off");
        text("profile-digital", draft.calibration.emulate_digital_triggers
                                    ? "Emulated"
                                    : "Physical only");
    }

    void render_live_inputs(bool connected)
    {
        static constexpr int defaultAxes[] = {
            SDL_GAMEPAD_AXIS_LEFTX, SDL_GAMEPAD_AXIS_LEFTY,
            SDL_GAMEPAD_AXIS_RIGHTX, SDL_GAMEPAD_AXIS_RIGHTY,
            SDL_GAMEPAD_AXIS_LEFT_TRIGGER, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,
        };
        static constexpr int defaultDirections[] = { 1, -1, 1, -1, 1, 1 };
        for (int i = 0; i < NativeProfileAxis_Count; ++i) {
            char meterId[32], valueId[40], position[16], value[16];
            std::snprintf(meterId, sizeof(meterId), "profile-live-%d", i);
            std::snprintf(valueId, sizeof(valueId), "profile-live-value-%d", i);
            auto* meter = document->GetElementById(meterId);
            if (!connected) {
                meter->SetProperty(i < 4 ? "left" : "width",
                                   i < 4 ? "50%" : "0%");
                text(valueId, "—");
                continue;
            }
            const auto& mapping = draft.axes[i];
            const int axis = mapping.assigned ? mapping.native_axis
                                              : defaultAxes[i];
            const int direction = mapping.assigned ? mapping.direction
                                                   : defaultDirections[i];
            const int raw = PADGetNativeAxisState(port, axis) * direction;
            if (i < 4) {
                const int percent = std::clamp(
                    int(std::lround(raw * 100.0 / 32767.0)), -100, 100);
                const int center = std::clamp(
                    int(std::lround((raw + 32768.0) * 100.0 / 65535.0)),
                    0, 100);
                std::snprintf(position, sizeof(position), "%d%%", center);
                std::snprintf(value, sizeof(value), "%+d%%", percent);
                meter->SetProperty("left", position);
            } else {
                const int percent = std::clamp(
                    int(std::lround(raw * 100.0 / 32767.0)),
                    0, 100);
                std::snprintf(position, sizeof(position), "%d%%", percent);
                std::snprintf(value, sizeof(value), "%d%%", percent);
                meter->SetProperty("width", position);
            }
            text(valueId, value);
        }
    }

    void render_draft()
    {
        input("profile-name")->SetValue(draft.name);
        const uint16_t values[] = {
            draft.calibration.main_stick_dead_zone,
            draft.calibration.c_stick_dead_zone,
            draft.calibration.left_trigger_activation,
            draft.calibration.right_trigger_activation
        };
        for (int i = 0; i < 4; ++i) {
            char value[16];
            const float percent =
                values[i] * 100.0f / 32767.0f;
            std::snprintf(value, sizeof(value), "%.1f", percent);
            input(zoneIds[i])->SetValue(value);
            displayedZones[i] = value;
            displayedZoneValues[i] = values[i];
        }
        render_options();
        render_bindings();
    }

    void hide_delete_confirmation()
    {
        document->GetElementById("profile-delete-confirm")
            ->SetProperty("display", "none");
        document->GetElementById("profile-delete")
            ->SetProperty("display", "block");
    }

    bool selected_profile_exists()
    {
        for (size_t i = 0; i < catalogCount; ++i) {
            if (std::strcmp(catalog[i].id, draftId) == 0) {
                return true;
            }
        }
        return false;
    }

    bool refresh_catalog()
    {
        updating = true;
        const bool ok = nativeProfileStore_List(catalog, std::size(catalog),
                                                &catalogCount);
        std::sort(catalog, catalog + catalogCount,
                  [](const auto& a, const auto& b) {
                      const int names = std::strcmp(a.name, b.name);
                      return names ? names < 0 : std::strcmp(a.id, b.id) < 0;
                  });
        auto* list = static_cast<Rml::ElementFormControlSelect*>(
            document->GetElementById("profile-list"));
        list->RemoveAll();
        list->Add("New profile", "");
        for (size_t i = 0; i < catalogCount; ++i) {
            list->Add(Rml::StringUtilities::EncodeRml(catalog[i].name),
                      catalog[i].id);
        }
        list->SetValue(draftId);
        auto* remove = document->GetElementById("profile-delete");
        if (selected_profile_exists()) {
            remove->RemoveAttribute("disabled");
        } else {
            remove->SetAttribute("disabled", "");
        }
        updating = false;
        if (!ok) {
            status("Could not list every profile (limit 64).");
        }
        return ok;
    }

    bool read_draft()
    {
        const auto name = input("profile-name")->GetValue();
        if (name.empty() || name.size() > NATIVE_PROFILE_NAME_BYTES ||
            name.find_first_not_of(" \t\r\n") == std::string::npos)
        {
            status("Enter a shorter, nonempty name.");
            return false;
        }
        NativeProfile parsed = draft;
        uint16_t* values[] = { &parsed.calibration.main_stick_dead_zone,
                               &parsed.calibration.c_stick_dead_zone,
                               &parsed.calibration.left_trigger_activation,
                               &parsed.calibration.right_trigger_activation };
        for (int i = 0; i < 4; ++i) {
            const auto value = input(zoneIds[i])->GetValue();
            if (value == displayedZones[i]) {
                *values[i] = displayedZoneValues[i];
                continue;
            }
            char* end;
            const double percent = std::strtod(value.c_str(), &end);
            if (end == value.c_str() || *end || !std::isfinite(percent) ||
                percent < 0 || percent > 100)
            {
                status(i < 2 ? "Stick dead zones: 0–100%."
                             : "Trigger click: 0–100%.");
                return false;
            }
            *values[i] = uint16_t(std::max(
                0L, std::lround(percent * 32767 / 100)));
        }
        std::memcpy(parsed.name, name.c_str(), name.size() + 1);
        if (!nativeProfile_Validate(&parsed)) {
            return false;
        }
        draft = parsed;
        return true;
    }

    void save_profile()
    {
        if (!nativeHome_IsSelection() || !read_draft()) {
            return;
        }
        stop_capture();
        hide_delete_confirmation();
        if (!draftId[0]) {
            unsigned int random[2];
            if (rand_s(&random[0]) || rand_s(&random[1])) {
                status("Could not create a profile ID.");
                return;
            }
            std::snprintf(draftId, sizeof(draftId), "player-%08x%08x",
                          random[0], random[1]);
        }
        if (!nativeProfileStore_Save(&draft, draftId)) {
            status("Could not save. Your draft is still here.");
            return;
        }
        if (!nativeProfileInput_Assign(port, &draft)) {
            status("Saved, but controller assignment failed.");
            return;
        }
        if (refresh_catalog()) {
            status("Saved and assigned to this port.");
        }
    }

    void begin_capture(int action, int axis, bool add)
    {
        stop_capture();
        if (PADGetIndexForPort(port) < 0) {
            status("Connect a controller to this port first.");
            return;
        }
        if (add && draft.actions[action].count == NATIVE_PROFILE_MAX_BINDINGS)
        {
            status("Four bindings maximum. Clear or replace one.");
            return;
        }
        captureAction = action;
        captureAxis = axis;
        captureAdd = add;
        captureDeadline = SDL_GetTicks() + 15000;
        status("Release controls, then press the input to bind.");
        document->GetElementById("profile-cancel")
            ->SetProperty("display", "block");
    }

    class ProfileEvents final : public Rml::EventListener {
        void ProcessEvent(Rml::Event& event) override
        {
            if (updating) {
                return;
            }
            Rml::Element* target = event.GetTargetElement();
            while (target && target->GetId().empty()) {
                target = target->GetParentNode();
            }
            if (!target || target->HasAttribute("disabled")) {
                return;
            }
            const std::string id = target->GetId();
            if (event.GetType() == "keydown") {
                const int key = event.GetParameter<int>("key_identifier", 0);
                if (key == Rml::Input::KI_ESCAPE) {
                    if (captureAction >= 0 || captureAxis >= 0) {
                        stop_capture();
                        status("Binding cancelled.");
                    } else {
                        nativeShellProfiles_Close();
                    }
                    event.StopPropagation();
                } else if ((key == Rml::Input::KI_RETURN ||
                            key == Rml::Input::KI_SPACE) &&
                           target->GetTagName() == "button")
                {
                    target->Click();
                    event.StopPropagation();
                }
                return;
            }
            if (event.GetType() == "change") {
                if (id != "profile-list") {
                    return;
                }
                const auto value =
                    static_cast<Rml::ElementFormControlSelect*>(target)
                        ->GetValue();
                if (value == draftId) {
                    return;
                }
                NativeProfile loaded;
                if (value.empty()) {
                    nativeProfile_Default(&loaded, "");
                } else if (!nativeProfileStore_Load(&loaded, value.c_str())) {
                    status("Could not load this profile.");
                    return;
                }
                stop_capture();
                hide_delete_confirmation();
                draft = loaded;
                std::snprintf(draftId, sizeof(draftId), "%s", value.c_str());
                auto* remove = document->GetElementById("profile-delete");
                if (draftId[0]) {
                    remove->RemoveAttribute("disabled");
                } else {
                    remove->SetAttribute("disabled", "");
                }
                render_draft();
                status("");
                return;
            }
            if (!opened || !nativeHome_IsSelection()) {
                return;
            }
            if (id == "profile-close") {
                nativeShellProfiles_Close();
            } else if (id == "profile-new") {
                stop_capture();
                hide_delete_confirmation();
                draftId[0] = 0;
                nativeProfile_Default(&draft, "");
                render_draft();
                refresh_catalog();
                status("New profile. Enter a name to save.");
            } else if (id == "profile-save") {
                save_profile();
            } else if (id == "profile-unassign") {
                nativeProfileInput_Clear(port);
                status("This port now uses default controls.");
            } else if (id == "profile-delete") {
                if (selected_profile_exists()) {
                    document->GetElementById("profile-delete")
                        ->SetProperty("display", "none");
                    document->GetElementById("profile-delete-confirm")
                        ->SetProperty("display", "flex");
                    status("");
                }
            } else if (id == "profile-delete-cancel") {
                hide_delete_confirmation();
            } else if (id == "profile-delete-yes") {
                hide_delete_confirmation();
                if (!selected_profile_exists() ||
                    !nativeProfileStore_Delete(draftId)) {
                    status("Could not delete this profile.");
                    return;
                }
                stop_capture();
                draftId[0] = 0;
                nativeProfile_Default(&draft, "");
                render_draft();
                if (refresh_catalog()) {
                    status("Profile deleted. Assigned controls remain until changed.");
                }
            } else if (id == "profile-reset") {
                stop_capture();
                const auto name = input("profile-name")->GetValue();
                nativeProfile_Default(&draft, "");
                render_draft();
                input("profile-name")->SetValue(name);
                status("Draft reset. Save to apply.");
            } else if (id == "profile-tap") {
                draft.tap_jump = !draft.tap_jump;
                render_options();
            } else if (id == "profile-buffer") {
                draft.action_buffer_frames = (draft.action_buffer_frames + 1) % 8;
                render_options();
            } else if (id == "profile-cstick") {
                draft.c_stick ^= 1;
                render_options();
            } else if (id == "profile-deadzones") {
                draft.calibration.use_dead_zones ^= true;
                render_options();
            } else if (id == "profile-digital") {
                draft.calibration.emulate_digital_triggers ^= true;
                render_options();
            } else if (id == "profile-cancel") {
                stop_capture();
                status("Binding cancelled.");
            } else if (id.starts_with("profile-port-")) {
                stop_capture();
                port = unsigned(id.back() - '0');
                lastController = -2;
                lastLiveUpdate = 0;
                for (unsigned i = 0; i < 4; ++i) {
                    document
                        ->GetElementById("profile-port-" + std::to_string(i))
                        ->SetClass("selected", i == port);
                }
            } else if (id == "profile-play" || id == "profile-bind" ||
                       id == "profile-tune")
            {
                stop_capture();
                for (const char* page :
                     { "profile-play", "profile-bind", "profile-tune" })
                {
                    document->GetElementById(page)->SetClass("selected",
                                                             id == page);
                    document->GetElementById(std::string(page) + "-panel")
                        ->SetProperty("display",
                                      id == page ? "block" : "none");
                }
            } else if (id.starts_with("bind-")) {
                begin_capture(id.back() - '0', -1, false);
            } else if (id.starts_with("add-")) {
                begin_capture(id.back() - '0', -1, true);
            } else if (id.starts_with("clear-")) {
                stop_capture();
                draft.actions[id.back() - '0'] = {};
                render_bindings();
            } else if (id.starts_with("axisdefault-")) {
                stop_capture();
                draft.axes[id.back() - '0'] = {};
                draft.calibration.measured = false;
                render_bindings();
            } else if (id.starts_with("axis-")) {
                begin_capture(-1, id.back() - '0', false);
            }
        }
    } events;
}

void nativeShellProfiles_Init(Rml::ElementDocument* shell)
{
    document = shell;
    nativeProfile_Default(&draft, "");
    draftId[0] = 0;
    opened = false;
    hideAfterSlide = 0;
    lastLiveUpdate = 0;
    port = 0;
    lastController = -2;
    stop_capture();
    for (const char* id : { "profiles-panel" }) {
        auto* element = document->GetElementById(id);
        element->AddEventListener("click", &events);
        element->AddEventListener("keydown", &events);
        element->AddEventListener("change", &events);
    }
    render_draft();
}

void nativeShellProfiles_Close()
{
    if (!opened) {
        return;
    }
    stop_capture();
    hide_delete_confirmation();
    opened = false;
    hideAfterSlide = SDL_GetTicks() + 250;
    document->GetElementById("profiles-panel")->SetClass("open", false);
    nativeShell_Blur();
}

extern "C" bool nativeShellProfiles_IsOpen(void)
{
    return opened;
}

void nativeShellProfiles_Update()
{
    if (hideAfterSlide && SDL_GetTicks() >= hideAfterSlide) {
        document->GetElementById("profiles-panel")
            ->SetProperty("visibility", "hidden");
        hideAfterSlide = 0;
    }
    if (!opened) {
        return;
    }
    if (!nativeHome_IsSelection()) {
        nativeShellProfiles_Close();
        return;
    }
    const int controller = PADGetIndexForPort(port);
    if (controller != lastController) {
        lastController = controller;
        lastLiveUpdate = 0;
        const char* name = PADGetName(port);
        text("profile-controller", controller < 0 ? "No controller"
                                   : name         ? name
                                                  : "Controller");
        stop_capture();
    }
    const uint64_t now = SDL_GetTicks();
    if (document->GetElementById("profile-tune")->IsClassSet("selected") &&
        (lastLiveUpdate == 0 || now - lastLiveUpdate >= 100)) {
        render_live_inputs(controller >= 0);
        lastLiveUpdate = now;
    }
    if (captureAction < 0 && captureAxis < 0) {
        return;
    }
    if (SDL_GetTicks() > captureDeadline) {
        stop_capture();
        status("Binding timed out. Try again.");
        return;
    }
    const int button = PADGetNativeButtonPressed(port);
    const auto axis = PADGetNativeAxisPulled(port);
    const bool neutral = button < 0 && axis.nativeAxis < 0;
    if (captureRelease) {
        if (neutral) {
            stop_capture();
            status("Binding set. Save to apply.");
        }
        return;
    }
    if (!captureReady) {
        captureReady = neutral;
        return;
    }
    if (captureAxis >= 0) {
        if (axis.nativeAxis < 0) {
            return;
        }
        draft.axes[captureAxis] = { int16_t(axis.nativeAxis),
                                    int8_t(axis.sign), true };
        draft.calibration.measured = false;
    } else {
        if (button < 0 && axis.nativeAxis < 0) {
            return;
        }
        auto& bindings = draft.actions[captureAction];
        const NativeProfileBinding binding = {
            int16_t(button >= 0 ? button : axis.nativeAxis),
            uint8_t(button >= 0     ? NativeProfileBinding_Button
                    : axis.sign > 0 ? NativeProfileBinding_AxisPositive
                                    : NativeProfileBinding_AxisNegative)
        };
        if (!captureAdd) {
            bindings = {};
        }
        bool duplicate = false;
        for (unsigned i = 0; i < bindings.count; ++i) {
            duplicate |=
                bindings.bindings[i].native_input == binding.native_input &&
                bindings.bindings[i].type == binding.type;
        }
        if (!duplicate) {
            bindings.bindings[bindings.count++] = binding;
        }
    }
    captureRelease = true;
    render_bindings();
    status("Release the control to finish.");
}

void nativeShellProfiles_Open()
{
    if (!nativeHome_IsSelection()) {
        nativeHub_Notice("Return to character select to edit profiles.");
        return;
    }
    nativeShellRules_Close();
    nativeShellCollection_Close();
    opened = true;
    hideAfterSlide = 0;
    lastLiveUpdate = 0;
    hide_delete_confirmation();
    document->GetElementById("profiles-panel")
        ->SetProperty("visibility", "visible");
    document->GetElementById("profiles-panel")->SetClass("open", true);
    refresh_catalog();
    document->GetElementById("profile-close")->Focus(true);
}
