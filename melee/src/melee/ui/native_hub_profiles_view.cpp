#include "native_hub_profiles.h"
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace {
    constexpr uint32_t gold = 0xE5C361FF, muted = 0xB9ADBDFF;
    void binding_name(const NativeProfileBinding& binding,char* output,size_t capacity)
    {
        const char* name = binding.type == NativeProfileBinding_Default ? "Default" :
            binding.type == NativeProfileBinding_Button ? PADGetNativeButtonName(binding.native_input) :
            PADGetNativeAxisName({binding.native_input,AXIS_SIGN_POSITIVE});
        std::snprintf(output,capacity,"%s%s",name ? name : "Unknown input",
            binding.type == NativeProfileBinding_AxisPositive ? " +" : binding.type == NativeProfileBinding_AxisNegative ? " -" : "");
    }
    void label(const char* text,float y) { nativeMenuPaint_Text(text,240,y,220,13,gold,NativeMenuFont_MenuOption); }
    void gameplay()
    {
        const auto& p = nativeProfileEditor.draft;
        label("Tap jump",196);
        nativeHub_Button("profile-tap",p.tap_jump?"On":"Off",475,187,112,29,HubAction::ProfileOption,0);
        nativeMenuPaint_Text("Jump by pushing the main stick up.",240,224,345,10,muted);
        label("Action buffer",256);
        char value[24]; std::snprintf(value,24,p.action_buffer_frames?"%u frames":"Off",p.action_buffer_frames);
        nativeHub_Button("profile-buffer",value,475,247,112,29,HubAction::ProfileOption,1);
        nativeMenuPaint_Text("Remember early actions for up to 7 frames.",240,284,345,10,muted);
        label("Attack stick",316);
        nativeHub_Button("profile-cstick",p.c_stick?"Tilt":"Smash",475,307,112,29,HubAction::ProfileOption,2);
        nativeHub_TextBlock("Choose smash attacks or tilts on the ground. Bind Short hop on the Buttons page.",240,348,345,32,10,muted);
    }
    void bindings(bool axes)
    {
        const auto& s = nativeProfileEditor;
        const char* actions[] = {"Attack","Special","Jump","Shield","Grab","Short hop"};
        const char* names[] = {"Move right","Move up","Attack right","Attack up","L trigger","R trigger"};
        for (unsigned i=0;i<6;++i) {
            float y = 187+i*27;
            nativeMenuPaint_Text(axes?names[i]:actions[i],240,y+7,83,11,gold);
            char value[256]{}, id[40];
            if (axes) {
                const auto& a = s.draft.axes[i];
                if (a.assigned) binding_name({a.native_axis,uint8_t(a.direction>0?NativeProfileBinding_AxisPositive:NativeProfileBinding_AxisNegative)},value,256);
                else std::snprintf(value,256,"Default");
            } else {
                const auto& bindings = s.draft.actions[i];
                if (!bindings.count) std::snprintf(value,256,"Unbound");
                for (unsigned j=0;j<bindings.count;++j) {
                    char name[80]; binding_name(bindings.bindings[j],name,80);
                    size_t used = std::strlen(value);
                    std::snprintf(value+used,256-used,"%s%s",j?", ":"",name);
                }
            }
            nativeMenuPaint_Text(value,326,y+8,axes?136:127,9,muted);
            std::snprintf(id,40,axes?"axis-%u":"bind-%u",i);
            nativeHub_Button(id,"Bind",axes?469:457,y,48,23,axes?HubAction::ProfileAxis:HubAction::ProfileBind,int(i));
            if (axes) {
                std::snprintf(id,40,"axisdefault-%u",i);
                nativeHub_Button(id,"Default",525,y,62,23,HubAction::ProfileAxisDefault,int(i));
            } else {
                std::snprintf(id,40,"add-%u",i);
                nativeHub_Button(id,"+",513,y,28,23,HubAction::ProfileAdd,int(i),false,s.draft.actions[i].count<NATIVE_PROFILE_MAX_BINDINGS);
                std::snprintf(id,40,"clear-%u",i);
                nativeHub_Button(id,"Clear",549,y,38,23,HubAction::ProfileClear,int(i));
            }
        }
        nativeHub_TextBlock(axes?"Bind the direction named on each row. Recalibrate after changing an axis.":
            "Bind replaces the inputs. + adds another. Release the controls before and after each binding.",240,357,347,28,9,muted);
    }
    void calibration()
    {
        const auto& s = nativeProfileEditor;
        nativeHub_Button("profile-deadzones",s.draft.calibration.use_dead_zones?"Dead zones: On":"Dead zones: Off",240,187,167,28,HubAction::ProfileOption,3);
        nativeHub_Button("profile-digital",s.draft.calibration.emulate_digital_triggers?"Click: Emulated":"Click: Physical",420,187,167,28,HubAction::ProfileOption,4);
        const char* names[] = {"Main dead zone %","C-stick dead zone %","L click threshold %","R click threshold %"};
        const char* ids[] = {"profile-main-zone","profile-c-zone","profile-l-zone","profile-r-zone"};
        for (unsigned i=0;i<4;++i) {
            float y = 227+i*27;
            nativeMenuPaint_Text(names[i],240,y+7,237,11,gold);
            nativeHub_Field(ids[i],s.zones[i],494,y,93,23,HubAction::ProfileField,false,nullptr,int(i+1));
        }
        nativeHub_Button("profile-calibrate","Calibrate",240,347,167,29,HubAction::ProfileCalibrate);
        nativeHub_Button("profile-test","Test controller",420,347,167,29,HubAction::ProfileTest);
    }
}
void nativeHubProfiles_Render()
{
    const auto& s = nativeProfileEditor;
    if (hub.page == NativeHubPage_ControllerCalibration) { nativeHubProfileCalibration_Render(); return; }
    if (hub.page == NativeHubPage_ControllerPicker) { nativeHubProfileDevices_Render(); return; }
    nativeHub_Button("profile-close","< Controls",40,106,120,27,HubAction::Back);
    nativeMenuPaint_Text("Controller port",316,114,112,11,gold);
    for (unsigned i=0;i<4;++i) {
        char id[32],name[4]; std::snprintf(id,32,"profile-port-%u",i); std::snprintf(name,4,"%u",i+1);
        nativeHub_Button(id,name,438+i*40,106,34,27,HubAction::ProfilePort,int(i),s.port==i);
    }
    nativeHub_Panel(40,146,176,245); nativeHub_Panel(227,146,373,245);
    nativeMenuPaint_Text("Saved profiles",50,157,156,13,gold,NativeMenuFont_MenuOption);
    for (unsigned i=s.first;i<std::min(unsigned(s.count),s.first+5);++i) {
        char id[40]; std::snprintf(id,40,"profile-entry-%u",i);
        nativeHub_Button(id,s.catalog[i].name,50,181+(i-s.first)*25,156,22,HubAction::ProfileSelect,int(i),!std::strcmp(s.id,s.catalog[i].id));
    }
    nativeHub_Button("profile-list-prev","<",50,312,35,24,HubAction::ProfileListPage,-5,false,s.first!=0);
    nativeHub_Button("profile-list-next",">",92,312,35,24,HubAction::ProfileListPage,5,false,s.first+5<s.count);
    nativeHub_Button("profile-new","New",140,312,66,24,HubAction::ProfileNew);
    nativeMenuPaint_Text("Name",50,347,156,10,gold);
    nativeHub_Field("profile-name",s.draft.name,50,361,156,24,HubAction::ProfileField,false,"Player");
    const char* tabs[] = {"Gameplay","Buttons","Axes","Calibration"};
    for (unsigned i=0;i<4;++i) {
        char id[32]; std::snprintf(id,32,"profile-tab-%u",i);
        nativeHub_Button(id,tabs[i],237+i*88,151,83,26,HubAction::ProfileTab,int(i),s.tab==i);
    }
    if (s.tab==0) gameplay(); else if (s.tab==1 || s.tab==2) bindings(s.tab==2); else calibration();
    bool capturing = s.capture_action >= 0 || s.capture_axis >= 0;
    nativeHub_Button("profile-save","Save & use",40,402,154,28,HubAction::ProfileSave,0,true,!capturing);
    nativeHub_Button("profile-reset","Reset",204,402,86,28,HubAction::ProfileReset,0,false,!capturing);
    nativeHub_Button("profile-delete","Delete",300,402,86,28,HubAction::ProfileDelete,0,false,!capturing && *s.id);
    if (capturing) nativeHub_Button("profile-cancel","Cancel binding",396,402,204,28,HubAction::ProfileCancelBinding);
    else {
        nativeHub_Button("profile-controller","Controller",396,402,108,28,HubAction::ProfileChooseController);
        nativeHub_Button("profile-unassign","Use defaults",514,402,86,28,HubAction::ProfileDefault);
    }
    const char* controller = PADGetIndexForPort(s.port)<0 ? "No controller" : PADGetName(s.port);
    nativeMenuPaint_Text(controller?controller:"Controller",40,439,560,9,muted);
    nativeMenuPaint_Text(s.status,40,459,560,10,muted);
    if (s.deleting) {
        if (!hub.passive_page) hub.region_count = 0;
        nativeMenuPaint_Rect(20,90,600,390,0x00000088);
        nativeHub_Panel(130,194,380,131);
        nativeMenuPaint_Text("Delete this saved profile?",150,213,340,17,gold,NativeMenuFont_MenuOption,true);
        nativeMenuPaint_Text(s.draft.name,150,246,340,13,muted,NativeMenuFont_Notice,true);
        nativeHub_Button("profile-delete-cancel","Cancel",150,282,160,28,HubAction::Cancel);
        nativeHub_Button("profile-delete-yes","Delete",330,282,160,28,HubAction::ProfileDeleteConfirm);
    }
}
