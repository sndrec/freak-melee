#include "native_hub_profiles.h"
#include <SDL3/SDL_timer.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {
    constexpr uint32_t gold = 0xE5C361FF, muted = 0xB9ADBDFF;
    void message(const char* text) { std::snprintf(nativeProfileEditor.status,192,"%s",text); }
    bool apply()
    {
        auto& s = nativeProfileEditor;
        if (!s.applied) {
            const auto* profile = nativeProfileInput_Assigned(s.port);
            s.previous_assigned = profile != nullptr;
            if (profile) s.previous_profile = *profile;
        }
        NativeProfile profile = s.draft;
        if (!*profile.name) std::snprintf(profile.name,sizeof(profile.name),"Controller");
        if (!nativeProfileInput_Assign(s.port,&profile)) { message("Could not apply these controls for testing."); return false; }
        s.applied = true; return true;
    }
    void enter(NativeProfileCalibrationStep step)
    {
        auto& s = nativeProfileEditor;
        s.step = step; s.step_started = SDL_GetTicks(); s.testing = false; s.dragged = -1; s.status[0] = 0;
    }
    bool sticks_complete()
    {
        const auto& c = nativeProfileEditor.capture;
        for (unsigned i=0;i<4;++i) {
            int center = (int(c.rest_low[i])+c.rest_high[i])/2;
            if (int(c.high[i])-center < 8192 || center-int(c.low[i]) < 8192) return false;
        }
        return true;
    }
    void bar(unsigned trigger,float& x,float& y,float& width)
    {
        bool review = nativeProfileEditor.step == ProfileCalibration_Review;
        x = review ? 100+trigger*240.f : 120; y = review ? 333 : 219+trigger*84.f; width = review ? 200 : 420;
    }
    void range(unsigned trigger,int16_t& low,int16_t& high)
    {
        const auto& s = nativeProfileEditor;
        if (s.step == ProfileCalibration_Review) { low=s.draft.calibration.center[4+trigger]; high=s.draft.calibration.maximum[4+trigger]; }
        else nativeControllerCalibration_TriggerRange(&s.capture,trigger,&low,&high);
    }
    void move_handle()
    {
        auto& s = nativeProfileEditor;
        if (s.dragged < 0) return;
        unsigned trigger = unsigned(s.dragged)/2; float x,y,width; bar(trigger,x,y,width);
        int16_t low,high; range(trigger,low,high);
        int value = int(std::lround(std::clamp((hub.pointer_x-x)/width,0.f,1.f)*32767));
        if (s.dragged&1) high = int16_t(std::clamp(value,int(low)+8192,32767));
        else low = int16_t(std::clamp(value,0,int(high)-8192));
        if (s.step == ProfileCalibration_Review) {
            s.draft.calibration.center[4+trigger] = low; s.draft.calibration.maximum[4+trigger] = high; apply();
        } else nativeControllerCalibration_SetTriggerRange(&s.capture,trigger,low,high);
        s.status[0] = 0;
    }
    void stick(float x,float y,unsigned first)
    {
        const auto& s = nativeProfileEditor; const auto& c = s.capture;
        bool review = s.step == ProfileCalibration_Review;
        float vx = review ? nativeControllerCalibration_Normalize(&s.draft.calibration,first,s.raw[first]) :
            s.raw[first]-(c.samples?(int(c.rest_low[first])+c.rest_high[first])*.5f:0);
        float vy = review ? nativeControllerCalibration_Normalize(&s.draft.calibration,first+1,s.raw[first+1]) :
            s.raw[first+1]-(c.samples?(int(c.rest_low[first+1])+c.rest_high[first+1])*.5f:0);
        nativeMenuPaint_Round(x-1,y-1,104,104,52,0x9A90AE66);
        nativeMenuPaint_Round(x,y,102,102,51,0x080910D8);
        nativeMenuPaint_Rect(x+50,y+4,1,94,0x766E7FFF); nativeMenuPaint_Rect(x+4,y+50,94,1,0x766E7FFF);
        unsigned count = c.stick_gate_count[first/2];
        for (unsigned i=0;i<count;++i) {
            float points[4];
            for (unsigned end=0;end<2;++end) {
                const auto& point = c.stick_gate[first/2][(i+end)%count];
                float gx = point.x, gy = point.y;
                if (review) {
                    gx=nativeControllerCalibration_Normalize(&s.draft.calibration,first,point.x);
                    gy=nativeControllerCalibration_Normalize(&s.draft.calibration,first+1,point.y);
                } else {
                    gx-=(int(c.rest_low[first])+c.rest_high[first])*.5f;
                    gy-=(int(c.rest_low[first+1])+c.rest_high[first+1])*.5f;
                }
                points[end*2]=x+51+gx*44/32767; points[end*2+1]=y+51-gy*44/32767;
            }
            if (count>1) nativeMenuPaint_Line(points[0],points[1],points[2],points[3],1,gold);
        }
        nativeMenuPaint_Round(x+47+vx*44/32767,y+47-vy*44/32767,8,8,4,gold);
        if (review) {
            const float raw_x = x + 51 + s.raw[first] * (44.f / 32767);
            const float raw_y = y + 51 - s.raw[first + 1] * (44.f / 32767);
            constexpr uint32_t raw_color = 0xD8CCE4FF;
            nativeMenuPaint_Line(raw_x, raw_y - 7, raw_x + 7, raw_y, 1, raw_color);
            nativeMenuPaint_Line(raw_x + 7, raw_y, raw_x, raw_y + 7, 1, raw_color);
            nativeMenuPaint_Line(raw_x, raw_y + 7, raw_x - 7, raw_y, 1, raw_color);
            nativeMenuPaint_Line(raw_x - 7, raw_y, raw_x, raw_y - 7, 1, raw_color);
        }
        nativeMenuPaint_Text(first?"C-stick":"Main stick",x-14,y+111,130,12,muted,NativeMenuFont_Notice,true);
        char value[80];
        if (review) std::snprintf(value,80,"Calibrated %+.1f, %+.1f",vx*80/32767,vy*80/32767);
        else {
            unsigned directions = 0;
            for (unsigned i=first;i<first+2;++i) {
                int center = (int(c.rest_low[i])+c.rest_high[i])/2;
                directions += c.low[i] < center-8192; directions += c.high[i] > center+8192;
            }
            std::snprintf(value,80,"%u / 4 directions",directions);
        }
        nativeMenuPaint_Text(value,x-24,y+130,150,10,gold,NativeMenuFont_Notice,true);
        std::snprintf(value,80,"Raw %d, %d",s.raw[first],s.raw[first+1]);
        nativeMenuPaint_Text(value,x-24,y+145,150,9,muted,NativeMenuFont_Notice,true);
    }
    void triggers()
    {
        const auto& s = nativeProfileEditor; bool review = s.step == ProfileCalibration_Review;
        for (unsigned i=0;i<2;++i) {
            float x,y,width; bar(i,x,y,width); int16_t low,high; range(i,low,high);
            float start=x+width*low/32767,end=x+width*high/32767;
            float raw=x+width*std::clamp(int(s.raw[4+i]),0,32767)/32767;
            float percent=std::clamp((s.raw[4+i]-low)*100.f/std::max(1,int(high)-low),0.f,100.f);
            char label[80],id[40];
            nativeMenuPaint_Text(i?"R":"L",x-23,y,20,14,gold);
            nativeMenuPaint_Round(x-2,y-2,width+4,22,3,0x9A90AE99);
            nativeMenuPaint_Rect(x,y,width,18,0x080910FF);
            nativeMenuPaint_Rect(start,y,end-start,18,0x6E604DFF);
            if (raw>start) nativeMenuPaint_Rect(start,y,std::min(raw,end)-start,18,gold);
            nativeMenuPaint_Rect(raw-1,y-3,2,24,0xFFFFFFFF);
            std::snprintf(label,80,"%.1f%%   Raw %d",percent,s.raw[4+i]);
            nativeMenuPaint_Text(label,x,y-18,width,10,muted);
            std::snprintf(label,80,"Start %.1f%%",low*100.f/32767);
            nativeMenuPaint_Text(label,x,y+31,width*.5f,9,gold);
            std::snprintf(label,80,"End %.1f%%",high*100.f/32767);
            nativeMenuPaint_Text(label,x+width*.55f,y+31,width*.45f,9,gold);
            std::snprintf(id,40,"profile-trigger-%u-range",i);
            nativeHub_Region(id,x-8,y-5,width+16,42,HubAction::ProfileCalibrationRange,4+int(i));
            for (unsigned h=0;h<2;++h) {
                float p = h?end:start;
                nativeMenuPaint_Line(p,y+18,p-6,y+27,2,gold);
                nativeMenuPaint_Line(p-6,y+27,p+6,y+27,2,gold);
                nativeMenuPaint_Line(p+6,y+27,p,y+18,2,gold);
                std::snprintf(id,40,"profile-trigger-%u-%s",i,h?"end":"start");
                nativeHub_Region(id,p-9,y-5,18,42,HubAction::ProfileCalibrationRange,int(i*2+h));
            }
        }
        if (!review) nativeMenuPaint_Text("Drag Start past the resting value; End goes just before the click.",65,362,518,11,muted);
    }
}
void nativeHubProfileCalibration_Restore(bool discard)
{
    auto& s = nativeProfileEditor;
    if (s.applied) {
        if (s.previous_assigned) nativeProfileInput_Assign(s.port,&s.previous_profile);
        else nativeProfileInput_Clear(s.port);
        s.applied = false;
    }
    if (discard && hub.page == NativeHubPage_ControllerCalibration) s.draft.calibration = s.original_calibration;
    s.testing = false; s.dragged = -1;
}
void nativeHubProfileCalibration_Begin(bool review)
{
    auto& s = nativeProfileEditor;
    nativeHub_Focus(nullptr,0);
    if (PADGetIndexForPort(s.port)<0) { message("Connect a controller to this port first."); return; }
    s.original_calibration = s.draft.calibration; s.guided = !review; nativeControllerCalibration_Reset(&s.capture);
    enter(review?ProfileCalibration_Review:ProfileCalibration_Rest);
    if (review && !apply()) return;
    nativeHub_ChangePage(NativeHubPage_ControllerCalibration);
}
void nativeHubProfileCalibration_Tick()
{
    auto& s = nativeProfileEditor;
    nativeProfileInput_RawAxes(s.port,&s.draft,s.raw);
    for (unsigned i=0;i<2;++i) {
        const auto& a = s.draft.axes[4+i];
        s.clicks[i] = a.assigned && (a.native_axis!=int(4+i) || a.direction!=1) ? -1 :
            int8_t(PADGetMappedButtonState(s.port,i?PAD_TRIGGER_R:PAD_TRIGGER_L));
    }
    if (s.step == ProfileCalibration_Rest) {
        bool moving = false;
        for (unsigned i=0;s.capture.samples && i<6;++i)
            moving |= s.raw[i] < int(s.capture.rest_low[i])-1500 || s.raw[i] > int(s.capture.rest_high[i])+1500;
        if (moving) { nativeControllerCalibration_Reset(&s.capture); s.step_started=SDL_GetTicks(); }
        nativeControllerCalibration_Sample(&s.capture,s.raw,true,nullptr);
    } else if (s.step==ProfileCalibration_Sticks || s.step==ProfileCalibration_Triggers)
        nativeControllerCalibration_Sample(&s.capture,s.raw,false,s.step==ProfileCalibration_Triggers?s.clicks:nullptr);
    if (s.dragged >= 0) { if (hub.pressed) move_handle(); else s.dragged=-1; }
}
bool nativeHubProfileCalibration_Activate(HubAction action,int value)
{
    auto& s = nativeProfileEditor;
    if (action==HubAction::Back || action==HubAction::ProfileCalibrationCancel) {
        if (action==HubAction::ProfileCalibrationCancel || s.step==ProfileCalibration_Rest || !s.guided) {
            nativeHubProfileCalibration_Restore(true); nativeHubProfiles_FormatZones();
            nativeHub_ChangePage(NativeHubPage_PlayerProfiles,true);
        } else if (s.step==ProfileCalibration_Sticks) {
            nativeHubProfileCalibration_Restore(false); nativeControllerCalibration_Reset(&s.capture); enter(ProfileCalibration_Rest);
        } else enter(NativeProfileCalibrationStep(unsigned(s.step)-1));
        return true;
    }
    if (action==HubAction::ProfileTest) { s.testing=!s.testing; return true; }
    if (action==HubAction::ProfileCalibrationRange && value>=0 && value<6) {
        if (value>=4) {
            unsigned i=unsigned(value)-4; float x,y,width; bar(i,x,y,width); int16_t low,high; range(i,low,high);
            float p=(hub.pointer_x-x)*32767/width; value=int(i*2)+(std::fabs(p-high)<std::fabs(p-low));
        }
        s.dragged=value; move_handle(); return true;
    }
    if (action!=HubAction::ProfileCalibrationNext) return true;
    switch (s.step) {
    case ProfileCalibration_Rest:
        if (s.capture.samples>=60 && SDL_GetTicks()-s.step_started>=1000) enter(ProfileCalibration_Sticks);
        break;
    case ProfileCalibration_Sticks:
        if (sticks_complete()) {
            auto capture = s.capture;
            for (unsigned i=0;i<2;++i) nativeControllerCalibration_SetTriggerRange(&capture,i,0,32767);
            if (nativeControllerCalibration_Finish(&capture,&s.draft.calibration) && apply()) enter(ProfileCalibration_Triggers);
            else message("Go back and keep the controller still while capturing its resting position.");
        }
        break;
    case ProfileCalibration_Triggers:
        if (!nativeControllerCalibration_Finish(&s.capture,&s.draft.calibration)) {
            message("Squeeze both triggers, or set their Start and End markers."); break;
        }
        if (nativeControllerCalibration_Normalize(&s.draft.calibration,4,s.raw[4])>0 ||
            nativeControllerCalibration_Normalize(&s.draft.calibration,5,s.raw[5])>0 || s.clicks[0]>0 || s.clicks[1]>0) {
            message("Release the triggers. Move Start past their resting readings."); break;
        }
        if (apply()) enter(ProfileCalibration_Review);
        break;
    case ProfileCalibration_Review:
        nativeHubProfileCalibration_Restore(false); nativeHubProfiles_FormatZones();
        nativeHub_ChangePage(NativeHubPage_PlayerProfiles,true); message("Calibration updated. Save & use applies the profile.");
        break;
    }
    return true;
}
void nativeHubProfileCalibration_Render()
{
    const auto& s = nativeProfileEditor;
    const char* headings[] = {"Leave the controller at rest","Move both sticks around their edges",
        "Set the trigger ranges","Try your controller"};
    nativeMenuPaint_Text(headings[s.step],55,118,530,22,gold,NativeMenuFont_MenuOption,true);
    if (s.step==ProfileCalibration_Rest || s.step==ProfileCalibration_Sticks) {
        nativeMenuPaint_Text(s.step==ProfileCalibration_Rest?"Release both sticks and triggers.":"Visit up, down, left and right on each stick.",65,157,518,12,muted,NativeMenuFont_Notice,true);
        stick(160,190,0); stick(380,190,2);
    } else if (s.step==ProfileCalibration_Triggers) triggers();
    else { stick(160,151,0); stick(380,151,2); triggers(); }
    nativeHub_Button("profile-calibration-back","< Back",40,402,110,28,HubAction::Back);
    nativeHub_Button("profile-calibration-cancel","Cancel",160,402,100,28,HubAction::ProfileCalibrationCancel);
    if (s.step==ProfileCalibration_Review) nativeHub_Button("profile-test-toggle",s.testing?"Stop test":"Test",275,402,110,28,HubAction::ProfileTest,0,s.testing);
    bool ready = PADGetIndexForPort(s.port)>=0 && (s.step==ProfileCalibration_Rest ? s.capture.samples>=60 && SDL_GetTicks()-s.step_started>=1000 :
        s.step==ProfileCalibration_Sticks?sticks_complete():!s.testing);
    nativeHub_Button("profile-calibration-next",s.step==ProfileCalibration_Review?"Use calibration":"Continue",400,402,200,28,HubAction::ProfileCalibrationNext,0,true,ready);
    nativeMenuPaint_Text(s.testing?"Input test active. START stops testing.":s.status,40,459,560,10,muted);
}
