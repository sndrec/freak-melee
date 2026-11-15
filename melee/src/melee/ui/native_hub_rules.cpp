#include "native_hub_rules.h"
#include <melee/gm/native_home.h>
#include <melee/gm/native_online_session.h>
#include <melee/gm/native_settings.h>
#include <melee/mn/native_menu_rules.h>
#include <melee/mn/native_menu_scene.h>
extern "C" {
#include <melee/gm/gmmain_lib.h>
}
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace {
    GameRules rules;
    GamePrefs prefs;
    NativeHubPage return_page;
    bool from_css;
    unsigned selected,people_offset;
    char status[160];
    constexpr uint64_t item_mask = UINT64_C(0xEFFFFFFF);
    constexpr uint32_t gold = 0xE5C361FF, muted = 0xB9ADBDFF;
    const char* rates[] = {"Off","Very low","Low","Medium","High","Very high"};
    const char* items[] = {"Bob-omb","Home-Run Bat","Mr. Saturn","Fire Flower","Flipper","Food","Freezie","Green Shell",
        "Hammer","Fan","Heart Container","Super Mushroom","Party Ball","Ray Gun","Lip's Stick","Poke Ball",
        "Metal Box","Proximity Mine","Maxim Tomato","Cloaking Device","Parasol","Red Shell","Bunny Hood","Star Rod",
        "Super Scope","Screw Attack","Starman","Beam Sword","","Barrel Cannon","Warp Star","Poison Mushroom"};
    bool can_edit()
    {
        return nativeHome_IsSelection() && !nativeOnlineSession_Quick() &&
            (nativeOnlineSession_State() == NativeLobbySessionState_Disconnected || nativeOnlineSession_IsOwner());
    }
    void save()
    {
        if (!can_edit()) return;
        GameRules next=*gmMainLib_GetGameRules();
        GamePrefs next_prefs=*gmMainLib_GetGamePrefs();
        next.mode=rules.mode; next.time_limit=rules.time_limit; next.stock_count=rules.stock_count;
        next.handicap=rules.handicap; next.damage_ratio=rules.damage_ratio; next.stage_sel=rules.stage_sel;
        next.stock_time_limit=rules.stock_time_limit; next.friendly_fire=rules.friendly_fire;
        next.pause=rules.pause; next.score_display=rules.score_display; next.unk_xc=rules.unk_xc;
        next_prefs.item_freq=prefs.item_freq; next_prefs.item_mask=prefs.item_mask;
        if (!nativeSettings_Save(&next,&next_prefs)) {
            std::snprintf(status,sizeof(status),"Could not save match rules."); return;
        }
        *gmMainLib_GetGameRules()=next; *gmMainLib_GetGamePrefs()=next_prefs;
        nativeHome_CancelStartMatch(); status[0]=0;
    }
    void change_page(NativeHubPage page,bool backwards=false)
    {
        selected=0; nativeHub_ChangePage(page,backwards);
    }
    unsigned current(unsigned row)
    {
        if (hub.page==NativeHubPage_AdditionalRules) {
            const unsigned values[]={rules.stock_time_limit,rules.friendly_fire,rules.pause,rules.score_display,rules.unk_xc,0};
            return values[row<6?row:0];
        }
        const unsigned values[]={rules.mode,rules.mode==1?rules.stock_count:rules.time_limit,
            rules.handicap,rules.damage_ratio,rules.stage_sel,0,0};
        return values[row<7?row:0];
    }
    void adjust(unsigned row,int direction)
    {
        unsigned char* target=nullptr; int low=0,high=1;
        if (hub.page==NativeHubPage_MatchRules) {
            switch(row) {
            case 0: target=&rules.mode; high=2; break;
            case 1: target=rules.mode==1?&rules.stock_count:&rules.time_limit; low=1; high=99; break;
            case 2: target=&rules.handicap; high=2; break;
            case 3: target=&rules.damage_ratio; low=5; high=20; break;
            case 4: target=&rules.stage_sel; high=4; break;
            }
        } else if (hub.page==NativeHubPage_AdditionalRules) {
            switch(row) {
            case 0: target=&rules.stock_time_limit; high=99; break;
            case 1: target=&rules.friendly_fire; break;
            case 2: target=&rules.pause; break;
            case 3: target=&rules.score_display; break;
            case 4: target=&rules.unk_xc; high=2; break;
            }
        }
        if (target) {
            int next=int(*target)+direction;
            *target=uint8_t(next<low?high:next>high?low:next); save();
        }
    }
    void row_region(unsigned row,const NativeMenuRuleBounds& b,bool enabled)
    {
        char id[40]; std::snprintf(id,sizeof(id),"rules-row-%u",row);
        if (hub.page==NativeHubPage_MatchRules && row>=5) {
            nativeHub_Region(id,b.x,b.y,b.w,b.h,row==5?HubAction::RuleItems:HubAction::RuleAdditional,0,enabled); return;
        }
        if (hub.page==NativeHubPage_AdditionalRules && row==5) {
            nativeHub_Region("rules-lobby",b.x,b.y,b.w,b.h,HubAction::RuleLobby,0,nativeOnlineSession_Presence()!=nullptr); return;
        }
        nativeHub_Region(id,b.x,b.y,b.w*.53f,b.h,HubAction::RuleAdjust,int(row*2+1),enabled);
        std::snprintf(id,sizeof(id),"rules-row-%u-left",row);
        nativeHub_Region(id,b.x+b.w*.53f,b.y,b.w*.22f,b.h,HubAction::RuleAdjust,int(row*2),enabled);
        std::snprintf(id,sizeof(id),"rules-row-%u-right",row);
        nativeHub_Region(id,b.x+b.w*.75f,b.y,b.w*.25f,b.h,HubAction::RuleAdjust,int(row*2+1),enabled);
    }
    void lobby()
    {
        const auto* presence=nativeOnlineSession_Presence();
        const auto* listing=nativeOnlineSession_Listing();
        const bool owner=nativeOnlineSession_IsOwner();
        const char* local=nativeOnlineSession_LocalPeerId();
        const char* policies[]={"Manual","Winner stays","Winner out"};
        struct Seat { const NativeSignalingParticipant* person; unsigned seat; } seats[128];
        unsigned count=0;
        for (unsigned i=0;presence&&i<presence->participant_count;++i)
            for (unsigned seat=0;seat<presence->participants[i].local_players&&count<128;++seat)
                seats[count++]={&presence->participants[i],seat};
        people_offset=std::min(people_offset,count?(count-1)/4*4:0);
        for (unsigned i=0;i<7;++i) {
            bool enabled=i<2?owner:i==6?count>4:false;
            if (i>=2&&i<6&&people_offset+i-2<count) {
                const auto& seat=seats[people_offset+i-2];
                enabled=owner||(local&&!std::strcmp(local,seat.person->peer_id));
            }
            NativeMenuRuleBounds b;
            if (enabled&&nativeMenuRules_Bounds(i,&b)&&nativeHub_Hover(b.x,b.y,b.w,b.h)) selected=i;
        }
        nativeMenuPaint_End(); nativeMenuRules_Draw(NativeMenuRules_Lobby,selected,0);
        Mtx canvas; Mtx44 projection; nativeMenuScene_View(false,false,canvas,projection); nativeMenuPaint_Begin(canvas);
        for (unsigned row=0;row<7;++row) {
            NativeMenuRuleBounds b; if (!nativeMenuRules_Bounds(row,&b)) continue;
            const char* label=""; const char* value=""; char name[64],range[40],id[40];
            HubAction action=HubAction::RulePeoplePage; int parameter=0; bool enabled=false; const char* peer=nullptr;
            if (row==0) { label="Lobby listing"; value=listing&&listing->listed?"Listed":"Unlisted"; action=HubAction::RuleListing; enabled=owner; std::snprintf(id,sizeof(id),"change-listing"); }
            else if (row==1) { label="Rotation"; value=presence?policies[presence->rotation_policy%3]:"Manual"; action=HubAction::RuleRotation; enabled=owner; std::snprintf(id,sizeof(id),"rotation-policy"); }
            else if (row==6) {
                label="More players"; std::snprintf(range,sizeof(range),"%u-%u / %u",count?people_offset+1:0,std::min(people_offset+4,count),count); value=range;
                parameter=people_offset+4<count?4:-int(people_offset); enabled=count>4; std::snprintf(id,sizeof(id),"lobby-next");
            } else if (people_offset+row-2<count) {
                const auto& seat=seats[people_offset+row-2]; const auto& person=*seat.person;
                unsigned state=person.active_mask&(1U<<seat.seat)?0:person.waiting_mask&(1U<<seat.seat)?1:2;
                const char* labels[]={"Fighting","Waiting","Spectating"};
                std::snprintf(name,sizeof(name),person.local_players>1?"%s (P%u)":"%s",person.name,seat.seat+1); label=name; value=labels[state];
                action=HubAction::RuleSeat; parameter=int(((state+1)%3)*4+seat.seat); peer=person.peer_id;
                enabled=owner||(local&&!std::strcmp(local,person.peer_id));
                std::snprintf(id,sizeof(id),"lobby-seat-%u-%u",unsigned(seat.person-presence->participants),seat.seat);
            } else { label="Open slot"; std::snprintf(id,sizeof(id),"lobby-empty-%u",row); }
            nativeMenuRules_Label(row,label,selected==row&&enabled); nativeMenuRules_Value(row,value);
            nativeHub_Region(id,b.x,b.y,b.w,b.h,action,parameter,enabled,peer);
        }
    }
}
bool nativeHubRules_Page(NativeHubPage page)
{
    return page==NativeHubPage_MatchRules || page==NativeHubPage_ItemSwitch ||
        page==NativeHubPage_AdditionalRules || page==NativeHubPage_LobbyRules;
}
GameRules* nativeHubRules_Data() { return nativeHubRules_Page(hub.page)?&rules:nullptr; }
void nativeHubRules_Open()
{
    if (!nativeHome_IsSelection()||nativeOnlineSession_Quick()) return;
    nativeHub_Focus(nullptr,0); nativeMenuRules_Unload();
    rules=*gmMainLib_GetGameRules(); prefs=*gmMainLib_GetGamePrefs();
    const auto* lobby=nativeOnlineSession_LobbyState();
    if (lobby && lobby->has_rules && !nativeOnlineSession_IsOwner()) nativeRoomRules_Apply(&lobby->rules,&rules,&prefs);
    status[0]=0; from_css=!hub.visible; return_page=hub.page; selected=people_offset=0;
    nativeHub_Open(NativeHubPage_MatchRules);
}
bool nativeHubRules_Activate(HubAction action,int value)
{
    if (action==HubAction::Rules) { nativeHubRules_Open(); return true; }
    if (!nativeHubRules_Page(hub.page)) return false;
    if (action==HubAction::Back) {
        if (hub.page==NativeHubPage_LobbyRules) change_page(NativeHubPage_AdditionalRules,true);
        else if (hub.page!=NativeHubPage_MatchRules) change_page(NativeHubPage_MatchRules,true);
        else if (from_css) { nativeHub_ChangePage(return_page,true); nativeHub_Activate(HubAction::Selection,0); }
        else nativeHub_ChangePage(return_page,true);
        return true;
    }
    if (action<HubAction::RuleAdjust || action>HubAction::RulePeoplePage) return false;
    if (action==HubAction::RuleItems) { change_page(NativeHubPage_ItemSwitch); return true; }
    if (action==HubAction::RuleAdditional) { change_page(NativeHubPage_AdditionalRules); return true; }
    if (action==HubAction::RuleLobby) { change_page(NativeHubPage_LobbyRules); return true; }
    if (action==HubAction::RulePeoplePage) { people_offset=unsigned(std::max(0,int(people_offset)+value)); return true; }
    if (action==HubAction::RuleSeat) {
        if (!nativeOnlineSession_ChooseSeat(hub.pending_peer,uint8_t(value&3),NativeRoomSeatState(value>>2)))
            nativeHub_Notice("Could not change this player's seat.");
        else nativeHome_CancelStartMatch();
        return true;
    }
    if (!can_edit()) { nativeHub_Notice("Only the lobby host can change match rules."); return true; }
    switch(action) {
    case HubAction::RuleAdjust: adjust(unsigned(value)/2,value&1?1:-1); return true;
    case HubAction::RuleRate:
        prefs.item_freq=prefs.item_freq==UINT8_MAX?0:prefs.item_freq>=4?UINT8_MAX:prefs.item_freq+1;
        if (prefs.item_freq!=UINT8_MAX && !(prefs.item_mask&item_mask)) prefs.item_mask=item_mask;
        break;
    case HubAction::RuleItem:
        if (unsigned(value)<32 && (item_mask&(UINT64_C(1)<<value))) prefs.item_mask^=UINT64_C(1)<<value;
        if (!(prefs.item_mask&item_mask)) prefs.item_freq=UINT8_MAX;
        break;
    case HubAction::RuleAll: prefs.item_mask=value?item_mask:0; if (!value) prefs.item_freq=UINT8_MAX; break;
    case HubAction::RuleListing: {
        const auto* listing=nativeOnlineSession_Listing(); char title[65];
        std::snprintf(title,sizeof(title),"%.32s's lobby",nativeHub_LocalName());
        if (!nativeOnlineSession_SetListing(listing&&listing->title[0]?listing->title:title,!listing||!listing->listed))
            nativeHub_Notice("Could not change the lobby listing.");
        return true;
    }
    case HubAction::RuleRotation: {
        const auto* p=nativeOnlineSession_Presence();
        if (p) nativeOnlineSession_SetRotation(NativeRoomRotationPolicy((p->rotation_policy+1)%3));
        return true;
    }
    default: return false;
    }
    save(); return true;
}
void nativeHubRules_Render(NativeHubPage page)
{
    const auto* shared=nativeOnlineSession_LobbyState();
    if (shared && shared->has_rules && !nativeOnlineSession_IsOwner()) nativeRoomRules_Apply(&shared->rules,&rules,&prefs);
    nativeHub_Button("rules-close","< Back",40,420,110,28,HubAction::Back);
    nativeMenuPaint_Text(status,40,461,555,11,muted);
    if (page==NativeHubPage_LobbyRules) { lobby(); return; }
    if (page==NativeHubPage_ItemSwitch) {
        unsigned count = 0;
        for (unsigned i=0;i<32;++i) if (item_mask & (UINT64_C(1)<<i)) count += bool(prefs.item_mask & (UINT64_C(1)<<i));
        char label[48]; std::snprintf(label,sizeof(label),"Items: %u selected",count);
        nativeMenuPaint_Text(label,46,112,270,14,gold);
        unsigned rate = prefs.item_freq == UINT8_MAX ? 0 : unsigned(prefs.item_freq)+1;
        nativeHub_Button("rules-items",rates[rate<6?rate:0],362,99,221,27,HubAction::RuleRate);
        unsigned row = 0;
        for (unsigned i=0;i<32;++i) {
            if (!(item_mask & (UINT64_C(1)<<i))) continue;
            char id[40], title[64]; std::snprintf(id,sizeof(id),"rules-item-%u",i);
            bool selected = prefs.item_mask & (UINT64_C(1)<<i);
            std::snprintf(title,sizeof(title),"%s  %s",selected ? "[x]" : "[ ]",items[i]);
            nativeHub_Button(id,title,42+(row%3)*185,139+(row/3)*21,180,19,HubAction::RuleItem,int(i),selected);
            ++row;
        }
        nativeHub_Button("rules-item-all","All",223,377,80,21,HubAction::RuleAll,1);
        nativeHub_Button("rules-item-none","None",316,377,80,21,HubAction::RuleAll,0);
        return;
    }
    unsigned rows=page==NativeHubPage_MatchRules?7:6;
    for (unsigned i=0;i<rows;++i) {
        NativeMenuRuleBounds b;
        if (nativeMenuRules_Bounds(i,&b) && nativeHub_Hover(b.x,b.y,b.w,b.h)) selected=i;
    }
    nativeMenuPaint_End(); nativeMenuRules_Draw(page==NativeHubPage_MatchRules?NativeMenuRules_Match:NativeMenuRules_Additional,selected,current(selected));
    Mtx canvas; Mtx44 projection; nativeMenuScene_View(false,false,canvas,projection); nativeMenuPaint_Begin(canvas);
    for (unsigned i=0;i<rows;++i) {
        NativeMenuRuleBounds b; if (!nativeMenuRules_Bounds(i,&b)) continue;
        row_region(i,b,can_edit()||i>=5);
        if (page==NativeHubPage_AdditionalRules && i==5) nativeMenuRules_Label(i,"Lobby settings",selected==i);
    }
    if (!can_edit()) nativeMenuPaint_Text("The lobby host controls match rules.",180,432,400,12,muted);
}
void nativeHubRules_Inspect()
{
    const auto* listing=nativeOnlineSession_Listing();
    const auto* presence=nativeOnlineSession_Presence();
    std::fprintf(stderr,"[hub-inspect] lobby-rules listed=%d owner=%d rotation=%u participants=%u\n",listing&&listing->listed,nativeOnlineSession_IsOwner(),presence?unsigned(presence->rotation_policy):0,presence?unsigned(presence->participant_count):0);
    for (unsigned i=0;presence&&i<presence->participant_count;++i)
        std::fprintf(stderr,"[hub-inspect] lobby-person row=%u active=%u waiting=%u local=%u\n",i,unsigned(presence->participants[i].active_mask),unsigned(presence->participants[i].waiting_mask),unsigned(presence->participants[i].local_players));

    std::fprintf(stderr,"[hub-inspect] match-rules page=%u editable=%d css=%d mode=%u time=%u stock=%u timer=%u damage=%u handicap=%u stage=%u friendly=%u pause=%u score=%u penalty=%u freq=%u mask=%llx status=%s\n",
        unsigned(hub.page),can_edit(),from_css,unsigned(rules.mode),unsigned(rules.time_limit),unsigned(rules.stock_count),unsigned(rules.stock_time_limit),unsigned(rules.damage_ratio)*10,
        unsigned(rules.handicap),unsigned(rules.stage_sel),unsigned(rules.friendly_fire),unsigned(rules.pause),unsigned(rules.score_display),unsigned(rules.unk_xc),unsigned(prefs.item_freq),(unsigned long long)prefs.item_mask,status);
}
