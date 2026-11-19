#include "native_hub_rules.h"
#include <melee/gm/native_home.h>
#include <melee/gm/native_online_session.h>
#include <melee/gm/native_settings.h>
#include <melee/mn/native_menu_rules.h>
#include <melee/mn/native_menu_scene.h>
extern "C" {
#include <melee/gm/gmmain_lib.h>
#include <melee/mn/mnitemsw.h>
}
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace {
    GameRules rules;
    GamePrefs prefs;
    NativeHubPage return_page;
    bool from_css;
    unsigned selected;
    char status[160];
    constexpr uint64_t item_mask = UINT64_C(0xEFFFFFFF);
    constexpr uint32_t gold = 0xE5C361FF, muted = 0xB9ADBDFF;
    const char* item_names[] = {"Bob-omb", "Home-Run Bat", "Mr. Saturn", "Fire Flower",
        "Flipper", "Food", "Freezie", "Green Shell", "Hammer", "Fan", "Heart Container",
        "Super Mushroom", "Party Ball", "Ray Gun", "Lip's Stick", "Poké Ball", "Metal Box",
        "Proximity Mine", "Maxim Tomato", "Cloaking Device", "Parasol", "Red Shell",
        "Bunny Hood", "Star Rod", "Super Scope", "Screw Attack", "Starman", "Beam Sword",
        "", "Barrel Cannon", "Warp Star", "Poison Mushroom"};
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
            *target=uint8_t(next<low?high:next>high?low:next);
            nativeMenuRules_AnimateAdjustment(direction);
            save();
        }
    }
    void row_region(unsigned row,const NativeMenuRuleBounds& b,bool enabled)
    {
        char id[40]; std::snprintf(id,sizeof(id),"rules-row-%u",row);
        if (hub.page==NativeHubPage_MatchRules && row==5) {
            nativeHub_Region(id,b.x,b.y,b.w,b.h,HubAction::RuleItems,0,enabled); return;
        }
        nativeHub_Region(id,b.x,b.y,b.w*.53f,b.h,HubAction::RuleAdjust,int(row*2+1),enabled);
        std::snprintf(id,sizeof(id),"rules-row-%u-left",row);
        nativeHub_Region(id,b.x+b.w*.53f,b.y,b.w*.22f,b.h,HubAction::RuleAdjust,int(row*2),enabled);
        std::snprintf(id,sizeof(id),"rules-row-%u-right",row);
        nativeHub_Region(id,b.x+b.w*.75f,b.y,b.w*.25f,b.h,HubAction::RuleAdjust,int(row*2+1),enabled);
    }
    void page_arrows()
    {
        for (unsigned side = 0; side < 2; ++side) {
            const float x = side ? 538 : 43;
            const bool hover = nativeHub_Hover(x, 193, 58, 91);
            nativeMenuPaint_Text(side ? ">" : "<", x, 204, 58, 58,
                                hover ? 0xFFE36BFF : gold, NativeMenuFont_MenuCaption, true, false);
            nativeHub_Region(side ? "rules-next-page" : "rules-previous-page", x, 193, 58, 91,
                             HubAction::RulePage, side ? 1 : -1);
        }
    }
    void lobby_settings()
    {
        const auto* listing = nativeOnlineSession_Listing();
        const auto* presence = nativeOnlineSession_Presence();
        const bool owner = can_edit();
        const unsigned limit = listing && listing->capacity >= 2 ? listing->capacity : 4;
        const char* name = listing && listing->title[0] ? listing->title : "Lobby";
        const char* labels[] = {"Lobby name", "Lobby listing", "Teams", "Rotation", "Fighter limit", "Password"};
        const char* values[][3] = {{name}, {"Unlisted", "Listed"}, {"OFF", "ON"},
            {"Manual", "Winner stays", "Winner out"}, {"2", "3", "4"},
            {listing && listing->locked ? "Set" : "None"}};
        const unsigned counts[] = {1, 2, 2, 3, 3, 1};
        const unsigned indices[] = {0, listing && listing->listed, nativeHome_IsTeams(),
            presence ? unsigned(presence->rotation_policy) % 3 : 0, limit - 2, 0};
        const char* ids[] = {"lobby-name", "change-listing", "lobby-teams", "rotation-policy",
                             "lobby-capacity", "lobby-edit-password"};
        const HubAction actions[] = {HubAction::RuleLobbyName, HubAction::RuleListing, HubAction::RuleLobbyTeams,
                                     HubAction::RuleRotation, HubAction::RuleLobbyCapacity,
                                     HubAction::RuleLobbyPassword};
        for (unsigned row = 0; row < 6; ++row) {
            NativeMenuRuleBounds b;
            if (nativeMenuRules_Bounds(row, &b) && nativeHub_Hover(b.x,b.y,b.w,b.h))
                selected = row;
        }
        nativeMenuPaint_End();
        nativeMenuRules_Draw(NativeMenuRules_Lobby, selected, 0);
        Mtx canvas;
        Mtx44 projection;
        nativeMenuScene_View(false, false, canvas, projection);
        nativeMenuPaint_Begin(canvas);
        for (unsigned row = 0; row < 6; ++row) {
            NativeMenuRuleBounds b;
            if (!nativeMenuRules_Bounds(row, &b)) continue;
            nativeMenuRules_Label(row, labels[row], selected == row);
            if (row == 0 && hub.field == hub.lobby_title) {
                nativeHub_Field(ids[row], hub.lobby_title, b.x+b.w*.47f, b.y+1,
                                b.w*.4f, b.h-2, HubAction::RuleLobbyName);
                nativeHub_Button("lobby-name-save", "OK", b.x+b.w*.88f, b.y+1,
                                 b.w*.11f, b.h-2, HubAction::RuleLobbyNameSave, 0, true,
                                 hub.lobby_title[0] != 0);
                continue;
            }
            if (row == 5 && hub.field == hub.lobby_password) {
                nativeHub_Field("lobby-new-password", hub.lobby_password, b.x+b.w*.39f, b.y+1,
                                b.w*.48f, b.h-2, HubAction::RuleLobbyPassword, true, "Empty to remove");
                nativeHub_Button("lobby-password-save", "OK", b.x+b.w*.88f, b.y+1,
                                 b.w*.11f, b.h-2, HubAction::RuleLobbyPasswordSave, 0, true, owner);
                continue;
            }
            nativeMenuRules_Value(row, values[row], counts[row], indices[row]);
            if (row == 0 || row == 5) {
                nativeHub_Region(ids[row], b.x, b.y, b.w, b.h, actions[row], 0, owner);
                continue;
            }
            char region[48];
            nativeHub_Region(ids[row], b.x, b.y, b.w*.53f, b.h, actions[row], 1, owner);
            std::snprintf(region, sizeof(region), "%s-left", ids[row]);
            nativeHub_Region(region, b.x+b.w*.53f, b.y, b.w*.22f, b.h, actions[row], -1, owner);
            std::snprintf(region, sizeof(region), "%s-right", ids[row]);
            nativeHub_Region(region, b.x+b.w*.75f, b.y, b.w*.25f, b.h, actions[row], 1, owner);
        }
    }
}
bool nativeHubRules_Page(NativeHubPage page)
{
    return page==NativeHubPage_MatchRules || page==NativeHubPage_ItemSwitch ||
        page==NativeHubPage_AdditionalRules || page==NativeHubPage_LobbyRules;
}
GameRules* nativeHubRules_Data() { return nativeHubRules_Page(hub.page)?&rules:nullptr; }
GamePrefs* nativeHubRules_Preferences() { return nativeHubRules_Page(hub.page)?&prefs:nullptr; }
void nativeHubRules_Open()
{
    if (!nativeHome_IsSelection()||nativeOnlineSession_Quick()) return;
    nativeHub_Focus(nullptr,0); nativeMenuRules_Unload();
    rules=*gmMainLib_GetGameRules(); prefs=*gmMainLib_GetGamePrefs();
    const auto* lobby=nativeOnlineSession_LobbyState();
    if (lobby && lobby->has_rules && !nativeOnlineSession_IsOwner()) nativeRoomRules_Apply(&lobby->rules,&rules,&prefs);
    status[0]=0; from_css=!hub.visible; return_page=hub.page; selected=0;
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
    if (action<HubAction::RuleAdjust || action>HubAction::RuleLobbyPasswordSave) return false;
    if (action==HubAction::RuleItems) { change_page(NativeHubPage_ItemSwitch); return true; }
    if (action==HubAction::RulePage) {
        const NativeHubPage pages[] = {NativeHubPage_MatchRules, NativeHubPage_AdditionalRules, NativeHubPage_LobbyRules};
        const int count = nativeOnlineSession_Presence() ? 3 : 2;
        const int page = hub.page==NativeHubPage_LobbyRules ? 2 : hub.page==NativeHubPage_AdditionalRules ? 1 : 0;
        change_page(pages[(page + count + (value < 0 ? -1 : 1)) % count], value < 0);
        return true;
    }
    if (!can_edit()) { nativeHub_Notice("Only the lobby host can change match rules."); return true; }
    switch(action) {
    case HubAction::RuleAdjust: adjust(unsigned(value)/2,value&1?1:-1); return true;
    case HubAction::RuleRate:
        if (value < 0 || value > 5) return true;
        prefs.item_freq = value ? uint8_t(value - 1) : UINT8_MAX;
        if (prefs.item_freq!=UINT8_MAX && !(prefs.item_mask&item_mask)) prefs.item_mask=item_mask;
        break;
    case HubAction::RuleItem:
        if (unsigned(value)<32 && (item_mask&(UINT64_C(1)<<value))) prefs.item_mask^=UINT64_C(1)<<value;
        if (!(prefs.item_mask&item_mask)) prefs.item_freq=UINT8_MAX;
        break;
    case HubAction::RuleLobbyTeams:
        nativeMenuRules_AnimateLobbyValue(2, value);
        nativeHome_SetTeams(!nativeHome_IsTeams());
        nativeHome_CancelStartMatch();
        return true;
    case HubAction::RuleLobbyName: {
        if (hub.field == hub.lobby_title) return true;
        const auto* listing = nativeOnlineSession_Listing();
        std::snprintf(hub.lobby_title, sizeof(hub.lobby_title), "%s",
                      listing && listing->title[0] ? listing->title : "Lobby");
        nativeHub_Focus(hub.lobby_title, sizeof(hub.lobby_title));
        return true;
    }
    case HubAction::RuleLobbyPassword:
        if (hub.field != hub.lobby_password) {
            std::memset(hub.lobby_password, 0, sizeof(hub.lobby_password));
            nativeHub_Focus(hub.lobby_password, sizeof(hub.lobby_password));
        }
        return true;
    case HubAction::RuleListing:
    case HubAction::RuleLobbyCapacity:
    case HubAction::RuleLobbyPasswordSave:
    case HubAction::RuleLobbyNameSave: {
        const auto* listing = nativeOnlineSession_Listing();
        const char* name = listing && listing->title[0] ? listing->title : "Lobby";
        const bool listed = listing && listing->listed;
        const unsigned limit = listing && listing->capacity >= 2 ? listing->capacity : 4;
        if (!nativeOnlineSession_SetListing(action == HubAction::RuleLobbyNameSave ? hub.lobby_title : name,
                action == HubAction::RuleListing ? !listed : listed,
                action == HubAction::RuleLobbyCapacity ? 2 + (limit - 2 + (value < 0 ? 2 : 1)) % 3 : limit,
                action == HubAction::RuleLobbyPasswordSave ? hub.lobby_password : nullptr)) {
            nativeHub_Notice(action == HubAction::RuleLobbyPasswordSave
                ? "Use letters, numbers or punctuation without spaces." : "Could not change the lobby settings.");
        } else {
            if (action == HubAction::RuleListing) nativeMenuRules_AnimateLobbyValue(1, value);
            if (action == HubAction::RuleLobbyCapacity) nativeMenuRules_AnimateLobbyValue(4, value);
            if (action == HubAction::RuleLobbyPasswordSave)
                std::memset(hub.lobby_password, 0, sizeof(hub.lobby_password));
            nativeHub_Focus(nullptr, 0);
            nativeHome_CancelStartMatch();
        }
        return true;
    }
    case HubAction::RuleRotation: {
        const auto* p=nativeOnlineSession_Presence();
        if (p && nativeOnlineSession_SetRotation(NativeRoomRotationPolicy((p->rotation_policy + (value < 0 ? 2 : 1)) % 3)))
            nativeMenuRules_AnimateLobbyValue(3, value);
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
    if (page==NativeHubPage_LobbyRules) {
        page_arrows(); lobby_settings();
        return;
    }
    if (page==NativeHubPage_ItemSwitch) {
        nativeMenuPaint_End();
        nativeMenuRules_Draw(NativeMenuRules_Items, selected, 0);
        Mtx canvas; Mtx44 projection;
        nativeMenuScene_View(false,false,canvas,projection);
        nativeMenuPaint_Begin(canvas);
        for (unsigned row = 0; row < 31; ++row) {
            NativeMenuRuleBounds bounds;
            if (!nativeMenuRules_Bounds(row, &bounds)) continue;
            if (nativeHub_Hover(bounds.x, bounds.y, bounds.w, bounds.h)) selected = row;
            const unsigned item = mnItemSw_ItemKind(row);
            char id[32];
            std::snprintf(id, sizeof(id), "rules-item-%u", item);
            nativeHub_Region(id, bounds.x, bounds.y, bounds.w, bounds.h,
                             HubAction::RuleItem, item, can_edit());
            nativeMenuPaint_Text(item_names[item], bounds.x + 62, bounds.y + 4, 134, 9,
                                 selected == row ? 0x181109FF : 0xF2ECEDFF);
        }
        NativeMenuRuleBounds rate;
        if (nativeMenuRules_Bounds(31, &rate)) {
            if (nativeHub_Hover(rate.x, rate.y, rate.w, rate.h)) selected = 31;
            for (unsigned column = 0; column < 6; ++column) {
                char id[32];
                std::snprintf(id, sizeof(id), "rules-item-rate-%u", 5 - column);
                nativeHub_Region(id, rate.x + column * rate.w / 6, rate.y, rate.w / 6, rate.h,
                                 HubAction::RuleRate, 5 - column, can_edit());
            }
        }
        return;
    }
    const unsigned rows=page==NativeHubPage_MatchRules ? 6 : 5;
    for (unsigned i=0;i<rows;++i) {
        NativeMenuRuleBounds b;
        if (nativeMenuRules_Bounds(i,&b) && nativeHub_Hover(b.x,b.y,b.w,b.h)) selected=i;
    }
    page_arrows();
    nativeMenuPaint_End(); nativeMenuRules_Draw(page==NativeHubPage_MatchRules?NativeMenuRules_Match:NativeMenuRules_Additional,selected,current(selected));
    Mtx canvas; Mtx44 projection; nativeMenuScene_View(false,false,canvas,projection); nativeMenuPaint_Begin(canvas);
    for (unsigned i=0;i<rows;++i) {
        NativeMenuRuleBounds b; if (!nativeMenuRules_Bounds(i,&b)) continue;
        row_region(i,b,can_edit()||i>=5);
    }
    if (!can_edit()) nativeMenuPaint_Text("The lobby host controls match rules.",180,432,400,12,muted);
}
void nativeHubRules_Inspect()
{
    const auto* listing=nativeOnlineSession_Listing();
    const auto* presence=nativeOnlineSession_Presence();
    std::fprintf(stderr,"[hub-inspect] lobby-rules listed=%d owner=%d rotation=%u capacity=%u participants=%u\n",listing&&listing->listed,nativeOnlineSession_IsOwner(),presence?unsigned(presence->rotation_policy):0,listing?unsigned(listing->capacity):0,presence?unsigned(presence->participant_count):0);
    for (unsigned i=0;presence&&i<presence->participant_count;++i)
        std::fprintf(stderr,"[hub-inspect] lobby-person row=%u active=%u waiting=%u local=%u\n",i,unsigned(presence->participants[i].active_mask),unsigned(presence->participants[i].waiting_mask),unsigned(presence->participants[i].local_players));

    std::fprintf(stderr,"[hub-inspect] match-rules page=%u editable=%d css=%d mode=%u time=%u stock=%u timer=%u damage=%u handicap=%u stage=%u friendly=%u pause=%u score=%u penalty=%u freq=%u mask=%llx status=%s\n",
        unsigned(hub.page),can_edit(),from_css,unsigned(rules.mode),unsigned(rules.time_limit),unsigned(rules.stock_count),unsigned(rules.stock_time_limit),unsigned(rules.damage_ratio)*10,
        unsigned(rules.handicap),unsigned(rules.stage_sel),unsigned(rules.friendly_fire),unsigned(rules.pause),unsigned(rules.score_display),unsigned(rules.unk_xc),unsigned(prefs.item_freq),(unsigned long long)prefs.item_mask,status);
}
