#include "native_hub_profiles.h"
#include <algorithm>
#include <cstdio>

namespace {
    constexpr unsigned capacity = 64, page_size = 5;
    constexpr uint32_t gold = 0xE5C361FF, muted = 0xB9ADBDFF;
    struct Controller { uint32_t instance; char name[160]; int port; };
    Controller controllers[capacity];
    unsigned count, first;

    int controller_index(uint32_t instance)
    {
        for (unsigned i=0;i<PADCount();++i)
            if (PADGetInstanceForControllerIndex(i)==instance) return int(i);
        return -1;
    }
    int controller_port(uint32_t instance)
    {
        for (unsigned port=0;port<4;++port)
            if (instance && PADGetInstanceForPort(port)==instance) return int(port);
        return -1;
    }
    void refresh()
    {
        count=std::min(capacity,unsigned(PADCount()));
        for (unsigned i=0;i<count;++i) {
            auto& row=controllers[i];
            row.instance=PADGetInstanceForControllerIndex(i);
            const char* name=PADGetNameForControllerIndex(i);
            std::snprintf(row.name,sizeof(row.name),"%s",name?name:"Controller");
            row.port=controller_port(row.instance);
        }
        std::sort(controllers,controllers+count,[](const auto& a,const auto& b) { return a.instance<b.instance; });
        first=count?std::min(first,(count-1)/page_size*page_size):0;
    }
    void apply_slot_controls(unsigned port)
    {
        if (const auto* profile=nativeProfileInput_Assigned(port)) nativeProfileInput_Assign(port,profile);
        else PADSetPortDeadZones(port,nullptr);
    }
    void select(uint32_t instance)
    {
        auto& s=nativeProfileEditor;
        const int index=controller_index(instance);
        if (index<0) {
            std::snprintf(s.status,sizeof(s.status),"That controller was disconnected. Choose another."); return;
        }
        nativeHubProfileCalibration_Restore(false);
        const int source=controller_port(instance), destination=PADGetIndexForPort(s.port);
        // Keep both players connected when moving a controller between occupied
        // slots. Profiles belong to player slots and stay in place.
        if (source!=int(s.port)) {
            PADSetPortForIndex(unsigned(index),s.port);
            if (source>=0 && destination>=0) PADSetPortForIndex(unsigned(destination),unsigned(source));
            apply_slot_controls(s.port);
            if (source>=0) apply_slot_controls(unsigned(source));
        }
        s.controller=PADGetInstanceForPort(s.port);
        std::snprintf(s.status,sizeof(s.status),"Controller assigned to Player %u.",s.port+1);
        nativeHub_ChangePage(NativeHubPage_PlayerProfiles,true);
    }
}

bool nativeHubProfileDevices_Activate(HubAction action,int value)
{
    auto& s=nativeProfileEditor;
    switch (action) {
    case HubAction::ProfileChooseController:
        first=0; *s.status=0; nativeHub_ChangePage(NativeHubPage_ControllerPicker); return true;
    case HubAction::Back:
        nativeHub_ChangePage(NativeHubPage_PlayerProfiles,true); return true;
    case HubAction::ProfileControllerSelect: select(uint32_t(value)); return true;
    case HubAction::ProfileControllerNone:
        nativeHubProfileCalibration_Restore(false);
        PADClearPort(s.port); s.controller=0;
        std::snprintf(s.status,sizeof(s.status),"Player %u has no controller assigned.",s.port+1);
        nativeHub_ChangePage(NativeHubPage_PlayerProfiles,true); return true;
    case HubAction::ProfileControllerPage:
        refresh(); first=unsigned(std::clamp(int(first)+value,0,count?int((count-1)/page_size*page_size):0)); return true;
    default: return false;
    }
}

void nativeHubProfileDevices_Render()
{
    refresh();
    const auto& s=nativeProfileEditor;
    nativeHub_Button("controller-picker-back","< Profiles",40,106,140,27,HubAction::Back);
    char title[96]; std::snprintf(title,sizeof(title),"Choose a controller for Player %u",s.port+1);
    nativeMenuPaint_Text(title,50,153,540,20,gold,NativeMenuFont_MenuOption);
    nativeHub_Panel(40,184,560,193);
    if (!count) nativeHub_TextBlock("Connect a controller to see it here.",58,221,520,45,14,muted);
    for (unsigned i=first;i<std::min(count,first+page_size);++i) {
        const auto& row=controllers[i];
        char id[40],name[192],status[32];
        std::snprintf(id,sizeof(id),"controller-device-%u",row.instance);
        std::snprintf(name,sizeof(name),"%u  %s",i+1,row.name);
        const float y=195+(i-first)*34;
        nativeHub_Button(id,name,52,y,390,29,HubAction::ProfileControllerSelect,int(row.instance),row.port==int(s.port));
        if (row.port<0) std::snprintf(status,sizeof(status),"Unassigned");
        else std::snprintf(status,sizeof(status),row.port==int(s.port)?"Selected · P%u":"Player %u",row.port+1);
        nativeMenuPaint_Text(status,453,y+8,128,11,row.port==int(s.port)?gold:muted);
    }
    nativeHub_Button("controller-picker-prev","<",40,388,35,26,HubAction::ProfileControllerPage,-int(page_size),false,first!=0);
    nativeHub_Button("controller-picker-next",">",85,388,35,26,HubAction::ProfileControllerPage,int(page_size),false,first+page_size<count);
    nativeHub_Button("controller-picker-none","No controller",420,388,180,28,HubAction::ProfileControllerNone);
    nativeHub_TextBlock("Choosing another player's controller swaps the two slots. Each slot keeps its controls.",40,431,560,32,11,muted);
    if (*s.status) nativeMenuPaint_Text(s.status,52,365,536,9,muted);
}

void nativeHubProfileDevices_Inspect()
{
    refresh();
    const auto& s=nativeProfileEditor;
    std::fprintf(stderr,"[hub-inspect] controller-picker port=%u count=%u first=%u selected=%u\n",s.port,count,first,unsigned(PADGetInstanceForPort(s.port)));
    for (unsigned i=0;i<count;++i)
        std::fprintf(stderr,"[hub-inspect] controller-device index=%u instance=%u port=%d name=%s\n",i,controllers[i].instance,controllers[i].port,controllers[i].name);
}
