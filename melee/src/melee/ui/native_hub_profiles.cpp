#define _CRT_RAND_S
#include "native_hub_profiles.h"
#include <melee/gm/native_home.h>
#include <SDL3/SDL_timer.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

NativeProfileEditor nativeProfileEditor;
namespace {
    void message(const char* text) { std::snprintf(nativeProfileEditor.status,192,"%s",text); }
    void stop_capture()
    {
        auto& s = nativeProfileEditor;
        s.capture_action = s.capture_axis = -1;
        s.capture_ready = s.capture_release = false;
        s.capture_cancel_at = 0;
    }
    bool capturing() { return nativeProfileEditor.capture_action >= 0 || nativeProfileEditor.capture_axis >= 0; }
    void refresh()
    {
        auto& s = nativeProfileEditor;
        if (!nativeProfileStore_List(s.catalog,64,&s.count)) message("Could not list every saved profile.");
        std::sort(s.catalog,s.catalog+s.count,[](const auto& a,const auto& b) {
            int names = std::strcmp(a.name,b.name); return names ? names < 0 : std::strcmp(a.id,b.id) < 0;
        });
        s.first = s.count ? std::min(s.first,unsigned((s.count-1)/5*5)) : 0;
    }
    void new_profile()
    {
        auto& s = nativeProfileEditor;
        nativeHub_Focus(nullptr,0); stop_capture(); s.deleting = false; s.id[0] = 0;
        nativeProfile_Default(&s.draft,""); nativeHubProfiles_FormatZones();
        message("Enter a name, adjust your controls, then Save & use.");
    }
    void begin_capture(int action,int axis,bool add)
    {
        auto& s = nativeProfileEditor;
        stop_capture(); nativeHub_Focus(nullptr,0);
        if (PADGetIndexForPort(s.port) < 0) { message("Connect a controller to this port first."); return; }
        if (add && s.draft.actions[action].count >= NATIVE_PROFILE_MAX_BINDINGS) {
            message("Four bindings maximum. Clear or replace a binding first."); return;
        }
        s.capture_action = action; s.capture_axis = axis; s.capture_add = add;
        s.capture_deadline = SDL_GetTicks()+15000;
        message("Release controls, then press an input. Hold START to cancel.");
    }
    void save()
    {
        auto& s = nativeProfileEditor;
        nativeHub_Focus(nullptr,0); stop_capture();
        if (!*s.draft.name || std::strspn(s.draft.name," \t\r\n") == std::strlen(s.draft.name) ||
            !nativeProfile_Validate(&s.draft)) { message("Enter a nonempty profile name and check the values."); return; }
        if (!*s.id) {
            unsigned random[2];
            if (rand_s(random) || rand_s(random+1)) { message("Could not create a profile ID."); return; }
            std::snprintf(s.id,sizeof(s.id),"player-%08x%08x",random[0],random[1]);
        }
        if (!nativeProfileStore_Save(&s.draft,s.id)) { message("Could not save this profile. Your edits are still here."); return; }
        if (!nativeProfileStore_WriteSelection(s.port,s.id) || !nativeProfileInput_Assign(s.port,&s.draft)) {
            message("Profile saved. Could not set it as this port's active profile."); return;
        }
        refresh(); message("Saved and assigned to this controller port.");
    }
}
bool nativeHubProfiles_Page(NativeHubPage page)
{
    return page == NativeHubPage_PlayerProfiles || page == NativeHubPage_ControllerCalibration ||
        page == NativeHubPage_ControllerPicker;
}
void nativeHubProfiles_FormatZones()
{
    auto& s = nativeProfileEditor;
    const auto& c = s.draft.calibration;
    const uint16_t values[] = {c.main_stick_dead_zone,c.c_stick_dead_zone,c.left_trigger_activation,c.right_trigger_activation};
    for (unsigned i=0;i<4;++i) {
        s.zone_values[i] = values[i]; std::snprintf(s.zones[i],8,"%.1f",values[i]*100.f/32767);
    }
}
bool nativeHubProfiles_OwnsField(const char* field)
{
    const auto& s = nativeProfileEditor;
    if (!field || !nativeHubProfiles_Page(hub.page)) return false;
    if (field == s.draft.name) return true;
    for (unsigned i=0;i<4;++i) if (field == s.zones[i]) return true;
    return false;
}
void nativeHubProfiles_CommitField()
{
    auto& s = nativeProfileEditor;
    uint16_t* values[] = {&s.draft.calibration.main_stick_dead_zone,&s.draft.calibration.c_stick_dead_zone,
        &s.draft.calibration.left_trigger_activation,&s.draft.calibration.right_trigger_activation};
    for (unsigned i=0;hub.field && i<4;++i) {
        if (hub.field != s.zones[i]) continue;
        char previous[8]; std::snprintf(previous,8,"%.1f",s.zone_values[i]*100.f/32767);
        if (!std::strcmp(previous,s.zones[i])) return;
        char* end; double percent = std::strtod(s.zones[i],&end);
        if (end == s.zones[i] || *end || !std::isfinite(percent) || percent < 0 || percent > 100) {
            std::snprintf(s.zones[i],8,"%s",previous); message("Enter a percentage from 0 to 100."); return;
        }
        *values[i] = s.zone_values[i] = uint16_t(std::lround(percent*32767/100));
        std::snprintf(s.zones[i],8,"%.1f",percent);
    }
}
void nativeHubProfiles_Open()
{
    auto& s = nativeProfileEditor;
    if (!nativeHome_IsSelection()) { nativeHub_Notice("Return to character select to edit controller profiles."); return; }
    if (!s.initialized) {
        if (!nativeProfileStore_ReadSelection(s.port,s.id) || !*s.id || !nativeProfileStore_Load(&s.draft,s.id)) {
            s.id[0] = 0; nativeProfile_Default(&s.draft,"");
        }
        s.initialized = true; nativeHubProfiles_FormatZones();
    }
    refresh(); s.controller = PADGetInstanceForPort(s.port);
    nativeHub_ChangePage(NativeHubPage_PlayerProfiles);
}
void nativeHubProfiles_Leave(NativeHubPage destination)
{
    if (!nativeHubProfiles_Page(hub.page) || nativeHubProfiles_Page(destination)) return;
    stop_capture(); nativeProfileEditor.deleting = false;
    nativeHubProfileCalibration_Restore(true);
}
bool nativeHubProfiles_Key(SDL_Keycode key)
{
    if (!nativeHubProfiles_Page(hub.page)) return false;
    auto& s = nativeProfileEditor;
    if (key == SDLK_ESCAPE && (capturing() || s.testing)) {
        stop_capture(); s.testing = false; message("Input test or binding cancelled."); return true;
    }
    return capturing() || s.testing;
}
bool nativeHubProfiles_LockCamera()
{
    return nativeHubProfiles_Page(hub.page) && (capturing() || nativeProfileEditor.testing ||
        (hub.page == NativeHubPage_ControllerCalibration && nativeProfileEditor.step != ProfileCalibration_Review));
}
bool nativeHubProfiles_Controller(unsigned port,const HSD_PadStatus& pad)
{
    if (!nativeHubProfiles_Page(hub.page)) return false;
    auto& s = nativeProfileEditor;
    if (capturing()) {
        if (port == s.port) {
            if (pad.button & PAD_BUTTON_START) {
                if (!s.capture_cancel_at) s.capture_cancel_at = SDL_GetTicks();
                if (SDL_GetTicks()-s.capture_cancel_at >= 700) { stop_capture(); message("Binding cancelled."); }
            } else s.capture_cancel_at = 0;
        }
        return true;
    }
    if (hub.page != NativeHubPage_ControllerCalibration) return false;
    if (port != s.port) return true;
    if (s.testing) {
        if (pad.trigger & PAD_BUTTON_START) s.testing = false;
        return true;
    }
    if (s.step == ProfileCalibration_Rest || s.step == ProfileCalibration_Sticks) {
        if (pad.trigger & PAD_BUTTON_START) nativeHubProfileCalibration_Activate(HubAction::ProfileCalibrationNext,0);
        if (pad.trigger & PAD_BUTTON_B) nativeHubProfileCalibration_Activate(HubAction::Back,0);
        return true;
    }
    return false;
}
void nativeHubProfiles_Tick()
{
    auto& s = nativeProfileEditor;
    if (!nativeHubProfiles_Page(hub.page)) return;
    if (!hub.visible) {
        stop_capture(); nativeHubProfileCalibration_Restore(true);
        if (hub.page == NativeHubPage_ControllerCalibration) nativeHub_ChangePage(NativeHubPage_PlayerProfiles,true);
        return;
    }
    uint32_t controller = PADGetInstanceForPort(s.port);
    if (controller != s.controller) {
        bool interrupted = capturing() || hub.page == NativeHubPage_ControllerCalibration;
        stop_capture(); s.controller = controller;
        if (hub.page == NativeHubPage_ControllerCalibration) {
            nativeHubProfileCalibration_Restore(true); nativeHub_ChangePage(NativeHubPage_PlayerProfiles,true);
        }
        if (interrupted) message("Controller changed. Select its port and try again.");
    }
    if (hub.page == NativeHubPage_ControllerCalibration) { nativeHubProfileCalibration_Tick(); return; }
    if (!capturing()) return;
    if (SDL_GetTicks() >= s.capture_deadline) { stop_capture(); message("Binding timed out. Try again."); return; }
    int button = PADGetNativeButtonPressed(s.port); auto axis = PADGetNativeAxisPulled(s.port);
    bool neutral = button < 0 && axis.nativeAxis < 0;
    if (s.capture_release) {
        if (neutral) {
            if (s.capture_axis >= 0) {
                s.draft.axes[s.capture_axis] = s.captured_axis;
                s.draft.calibration.measured = false;
            } else {
                auto& bindings = s.draft.actions[s.capture_action];
                if (!s.capture_add) bindings = {};
                bool duplicate = false;
                for (unsigned i=0;i<bindings.count;++i) duplicate |= bindings.bindings[i].native_input == s.captured_binding.native_input &&
                    bindings.bindings[i].type == s.captured_binding.type;
                if (!duplicate && bindings.count < NATIVE_PROFILE_MAX_BINDINGS) bindings.bindings[bindings.count++] = s.captured_binding;
            }
            stop_capture(); message("Binding set. Save & use applies your profile.");
        }
        return;
    }
    if (!s.capture_ready) { s.capture_ready = neutral; return; }
    if (s.capture_axis >= 0) {
        if (axis.nativeAxis < 0) return;
        s.captured_axis = {int16_t(axis.nativeAxis),int8_t(axis.sign),true};
    } else {
        if (button < 0 && axis.nativeAxis < 0) return;
        s.captured_binding = {int16_t(button >= 0 ? button : axis.nativeAxis),
            uint8_t(button >= 0 ? NativeProfileBinding_Button : axis.sign > 0 ? NativeProfileBinding_AxisPositive : NativeProfileBinding_AxisNegative)};
    }
    s.capture_release = true; message("Release to finish. Hold START to cancel.");
}
bool nativeHubProfiles_Activate(HubAction action,int value)
{
    auto& s = nativeProfileEditor;
    if (action == HubAction::EditControls) { nativeHubProfiles_Open(); return true; }
    if (!nativeHubProfiles_Page(hub.page)) return false;
    if (hub.page == NativeHubPage_ControllerPicker) return nativeHubProfileDevices_Activate(action,value);
    if (hub.page == NativeHubPage_ControllerCalibration) return nativeHubProfileCalibration_Activate(action,value);
    if (s.deleting) {
        if (action == HubAction::ProfileDeleteConfirm) {
            if (nativeProfileStore_Delete(s.id)) { new_profile(); refresh(); message("Profile deleted. Assigned controls stay active until you change them."); }
            else message("Could not delete this profile.");
        }
        if (action == HubAction::Back || action == HubAction::Cancel || action == HubAction::ProfileDeleteConfirm) s.deleting = false;
        return true;
    }
    if (capturing() && action != HubAction::ProfileCancelBinding && action != HubAction::Back) return true;
    switch (action) {
    case HubAction::Back: stop_capture(); nativeHub_ChangePage(NativeHubPage_Controls,true); return true;
    case HubAction::ProfileNew: new_profile(); return true;
    case HubAction::ProfileSelect:
        if (unsigned(value) < s.count) {
            NativeProfile loaded;
            if (!nativeProfileStore_Load(&loaded,s.catalog[value].id)) message("Could not load this profile.");
            else { nativeHub_Focus(nullptr,0); s.draft = loaded; std::snprintf(s.id,65,"%s",s.catalog[value].id); nativeHubProfiles_FormatZones(); message(""); }
        }
        return true;
    case HubAction::ProfileListPage: s.first = unsigned(std::clamp(int(s.first)+value,0,s.count?int((s.count-1)/5*5):0)); return true;
    case HubAction::ProfilePort:
        if (unsigned(value) < 4) { stop_capture(); s.port = unsigned(value); s.controller = PADGetInstanceForPort(s.port); }
        return true;
    case HubAction::ProfileTab: nativeHub_Focus(nullptr,0); if (unsigned(value)<4) s.tab = unsigned(value); return true;
    case HubAction::ProfileSave: save(); return true;
    case HubAction::ProfileDefault:
        if (nativeProfileStore_WriteSelection(s.port,"")) {
            nativeProfileInput_Clear(s.port); message("This port now uses default controls.");
        } else message("Could not save this port's control selection.");
        return true;
    case HubAction::ProfileChooseController:
        return nativeHubProfileDevices_Activate(action,value);
    case HubAction::ProfileDelete: s.deleting = *s.id != 0; return true;
    case HubAction::ProfileReset: {
        nativeHub_Focus(nullptr,0); char name[33]; std::memcpy(name,s.draft.name,33);
        nativeProfile_Default(&s.draft,name); nativeHubProfiles_FormatZones(); message("Controls reset. Save & use applies the changes."); return true;
    }
    case HubAction::ProfileOption:
        if (value == 0) s.draft.tap_jump = !s.draft.tap_jump;
        if (value == 1) s.draft.action_buffer_frames = (s.draft.action_buffer_frames+1)%8;
        if (value == 2) s.draft.c_stick ^= 1;
        if (value == 3) s.draft.calibration.use_dead_zones ^= true;
        if (value == 4) s.draft.calibration.emulate_digital_triggers ^= true;
        return true;
    case HubAction::ProfileField:
        if (value == 0) nativeHub_Focus(s.draft.name,sizeof(s.draft.name));
        else if (unsigned(value) <= 4) nativeHub_Focus(s.zones[value-1],8);
        return true;
    case HubAction::ProfileBind: if (unsigned(value)<NativeProfileAction_Count) begin_capture(value,-1,false); return true;
    case HubAction::ProfileAdd: if (unsigned(value)<NativeProfileAction_Count) begin_capture(value,-1,true); return true;
    case HubAction::ProfileClear: if (unsigned(value)<NativeProfileAction_Count) s.draft.actions[value] = {}; return true;
    case HubAction::ProfileAxis: if (unsigned(value)<NativeProfileAxis_Count) begin_capture(-1,value,false); return true;
    case HubAction::ProfileAxisDefault:
        if (unsigned(value)<NativeProfileAxis_Count) { s.draft.axes[value] = {}; s.draft.calibration.measured = false; } return true;
    case HubAction::ProfileCancelBinding: stop_capture(); message("Binding cancelled."); return true;
    case HubAction::ProfileCalibrate: nativeHubProfileCalibration_Begin(false); return true;
    case HubAction::ProfileTest: nativeHubProfileCalibration_Begin(true); return true;
    default: return false;
    }
}
void nativeHubProfiles_Inspect()
{
    const auto& s = nativeProfileEditor;
    std::fprintf(stderr,"[hub-inspect] controller-profile page=%u port=%u profiles=%zu first=%u tab=%u id=%s name=%s tap=%u buffer=%u cstick=%u measured=%d capture=%d,%d ready=%d release=%d step=%u testing=%d applied=%d\n",
        unsigned(hub.page),s.port,s.count,s.first,s.tab,s.id,s.draft.name,s.draft.tap_jump,s.draft.action_buffer_frames,s.draft.c_stick,
        s.draft.calibration.measured,s.capture_action,s.capture_axis,s.capture_ready,s.capture_release,unsigned(s.step),s.testing,s.applied);
    const auto* assigned = nativeProfileInput_Assigned(s.port);
    std::fprintf(stderr,"[hub-inspect] controller-profile-status device=%d assigned=%s status=%s\n",int(PADGetIndexForPort(s.port)),assigned?assigned->name:"none",s.status);
    for (unsigned i=0;i<s.count;++i) std::fprintf(stderr,"[hub-inspect] controller-profile-entry index=%u id=%s name=%s\n",i,s.catalog[i].id,s.catalog[i].name);
    for (unsigned i=0;i<6;++i) {
        const auto& a = s.draft.actions[i]; const auto& axis = s.draft.axes[i];
        std::fprintf(stderr,"[hub-inspect] controller-binding action=%u count=%u axis=%d direction=%d assigned=%d raw=%d center=%d min=%d max=%d\n",i,a.count,axis.native_axis,axis.direction,axis.assigned,s.raw[i],s.draft.calibration.center[i],s.draft.calibration.minimum[i],s.draft.calibration.maximum[i]);
        for (unsigned j=0;j<a.count;++j) std::fprintf(stderr,"[hub-inspect] controller-input action=%u binding=%u type=%u input=%d\n",i,j,a.bindings[j].type,a.bindings[j].native_input);
    }
    const auto& c = s.draft.calibration;
    std::fprintf(stderr,"[hub-inspect] controller-zones main=%u c=%u l=%u r=%u use=%d digital=%d samples=%u gates=%u,%u\n",
        c.main_stick_dead_zone,c.c_stick_dead_zone,c.left_trigger_activation,c.right_trigger_activation,c.use_dead_zones,c.emulate_digital_triggers,
        s.capture.samples,s.capture.stick_gate_count[0],s.capture.stick_gate_count[1]);
}
