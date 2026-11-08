#include "native_hub_startup_triggers.h"
#include "native_hub_startup.h"
#include <melee/gm/native_profile_input.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {
    int dragged = -1;
    bool active()
    {
        return nativeHubStartup.active && (nativeHubStartup.step == NativeStartup_Triggers ||
                                           nativeHubStartup.step == NativeStartup_Test);
    }
    void bar(unsigned trigger, float& x, float& y, float& width)
    {
        bool review = nativeHubStartup.step == NativeStartup_Test;
        x = review ? 124+trigger*206.f : 145;
        y = review ? 349 : 216+trigger*87.f;
        width = review ? 180 : 420;
    }
    void range(unsigned trigger, int16_t& start, int16_t& end)
    {
        const auto& s = nativeHubStartup;
        if (s.step == NativeStartup_Test) {
            start = s.profile.calibration.center[4+trigger];
            end = s.profile.calibration.maximum[4+trigger];
        } else nativeControllerCalibration_TriggerRange(&s.capture, trigger, &start, &end);
    }
    void move()
    {
        auto& s = nativeHubStartup;
        unsigned trigger = unsigned(dragged)/2;
        float x,y,width; bar(trigger,x,y,width);
        int16_t start,end; range(trigger,start,end);
        int value = int(std::lround(std::clamp((hub.pointer_x-x)/width,0.f,1.f)*32767));
        if (dragged&1) end = int16_t(std::clamp(value,int(start)+8192,32767));
        else start = int16_t(std::clamp(value,0,int(end)-8192));
        if (!nativeControllerCalibration_SetTriggerRange(&s.capture,trigger,start,end)) return;
        s.error[0] = 0;
        if (s.step == NativeStartup_Test) {
            s.profile.calibration.center[4+trigger] = start;
            s.profile.calibration.maximum[4+trigger] = end;
            nativeProfileInput_Assign(s.port,&s.profile);
        }
    }
}

void nativeHubStartupTriggers_Begin(int handle)
{
    if (!active() || handle < 0 || handle >= 6) return;
    if (handle >= 4) {
        unsigned trigger = unsigned(handle)-4;
        float x,y,width; bar(trigger,x,y,width);
        int16_t start,end; range(trigger,start,end);
        float value = (hub.pointer_x-x)*32767/width;
        handle = int(trigger)*2 + (std::fabs(value-end) < std::fabs(value-start));
    }
    dragged = handle;
    move();
}

void nativeHubStartupTriggers_Update()
{
    if (!active() || !hub.pressed) { dragged = -1; return; }
    if (dragged >= 0) move();
}

void nativeHubStartupTriggers_Render()
{
    const auto& s = nativeHubStartup;
    bool review = s.step == NativeStartup_Test;
    for (unsigned i = 0; i < 2; ++i) {
        float x,y,width; bar(i,x,y,width);
        int16_t start,end; range(i,start,end);
        float low = x+width*start/32767, high = x+width*end/32767;
        float raw = x+width*std::clamp(int(s.raw[4+i]),0,32767)/32767;
        float percent = std::clamp((s.raw[4+i]-start)*100.f/(end-start),0.f,100.f);
        char label[96],id[48];
        nativeMenuPaint_Text(i ? "R" : "L",x-27,y-1,20,16,0xE5C361FF);
        nativeMenuPaint_Round(x-2,y-2,width+4,22,3,0x9A90AE99);
        nativeMenuPaint_Rect(x,y,width,18,0x080910FF);
        nativeMenuPaint_Rect(low,y,high-low,18,0x6E604DFF);
        if (raw > low) nativeMenuPaint_Rect(low,y,std::min(raw,high)-low,18,0xE5C361FF);
        nativeMenuPaint_Rect(raw-1,y-3,2,24,0xFFFFFFFF);
        std::snprintf(label,sizeof(label),"%.1f%%   Raw %d",percent,s.raw[4+i]);
        nativeMenuPaint_Text(label,x,y-19,width,10,0xEEE9EDFF);
        std::snprintf(label,sizeof(label),"Start %.1f%%",start*100.f/32767);
        nativeMenuPaint_Text(label,x,y+(review?32:42),width*.5f,review?8:10,0xE5C361FF);
        std::snprintf(label,sizeof(label),"End %.1f%%",end*100.f/32767);
        nativeMenuPaint_Text(label,x+width*.55f,y+(review?32:42),width*.45f,review?8:10,0xE5C361FF);
        std::snprintf(id,sizeof(id),"setup-trigger-%u-range",i);
        nativeHub_Region(id,x-8,y-5,width+16,42,HubAction::StartupTriggerRange,4+int(i));
        for (unsigned handle=0; handle<2; ++handle) {
            float point = handle ? high : low;
            bool selected = dragged == int(i*2+handle);
            uint32_t color = selected ? 0xFFFFFFFF : 0xE5C361FF;
            nativeMenuPaint_Line(point,y+18,point-6,y+27,2,color);
            nativeMenuPaint_Line(point-6,y+27,point+6,y+27,2,color);
            nativeMenuPaint_Line(point+6,y+27,point,y+18,2,color);
            std::snprintf(id,sizeof(id),"setup-trigger-%u-%s",i,handle?"end":"start");
            nativeHub_Region(id,point-9,y-5,18,42,HubAction::StartupTriggerRange,int(i*2+handle));
        }
    }
}
