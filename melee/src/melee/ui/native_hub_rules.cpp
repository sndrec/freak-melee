#include "native_hub_rules.h"
#include <melee/gm/native_home.h>
#include <melee/gm/native_online_session.h>
#include <melee/gm/native_settings.h>
extern "C" {
#include <melee/gm/gmmain_lib.h>
}
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
    GameRules rules;
    GamePrefs prefs;
    NativeHubPage return_page;
    bool from_css;
    char numbers[4][8], status[160];
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
    unsigned number(unsigned i)
    {
        const unsigned values[] = {rules.time_limit,rules.stock_count,rules.stock_time_limit,unsigned(rules.damage_ratio)*10};
        return values[i];
    }
    void format(unsigned i) { std::snprintf(numbers[i],8,"%u",number(i)); }
    void message(const char* text) { std::snprintf(status,sizeof(status),"%s",text); }
    void save()
    {
        nativeHub_Focus(nullptr,0);
        if (!can_edit()) { message("Only the lobby host can change match rules."); return; }
        // Preserve unrelated preferences owned by other menus.
        GameRules next = *gmMainLib_GetGameRules();
        GamePrefs next_prefs = *gmMainLib_GetGamePrefs();
        next.mode = rules.mode; next.time_limit = rules.time_limit;
        next.stock_count = rules.stock_count; next.stock_time_limit = rules.stock_time_limit;
        next.damage_ratio = rules.damage_ratio; next.friendly_fire = rules.friendly_fire;
        next.pause = rules.pause; next.score_display = rules.score_display;
        next_prefs.item_freq = prefs.item_freq; next_prefs.item_mask = prefs.item_mask;
        if (!nativeSettings_Save(&next,&next_prefs)) { message("Could not save match rules."); return; }
        *gmMainLib_GetGameRules() = next; *gmMainLib_GetGamePrefs() = next_prefs;
        message("Saved. These rules apply to the next match.");
    }
    void numeric(unsigned i,const char* id,const char* label,const char* range,float x,float y)
    {
        nativeMenuPaint_Text(label,x,y,158,14,gold);
        nativeHub_Field(id,numbers[i],x+165,y-6,76,28,HubAction::RuleField,false,nullptr,int(i));
        nativeMenuPaint_Text(range,x,y+27,245,10,muted);
    }
}
bool nativeHubRules_Page(NativeHubPage page)
{
    return page == NativeHubPage_MatchRules || page == NativeHubPage_ItemSwitch;
}
void nativeHubRules_Open()
{
    if (!can_edit()) { nativeHub_Notice("Match rules can be changed by the lobby host on character select."); return; }
    nativeHub_Focus(nullptr,0); rules = *gmMainLib_GetGameRules(); prefs = *gmMainLib_GetGamePrefs();
    for (unsigned i=0;i<4;++i) format(i);
    status[0] = 0; from_css = !hub.visible; return_page = hub.page;
    nativeHub_Open(NativeHubPage_MatchRules);
}
bool nativeHubRules_OwnsField(const char* field)
{
    for (unsigned i=0;field && i<4;++i) if (field == numbers[i]) return true;
    return false;
}
void nativeHubRules_CommitField()
{
    for (unsigned i=0;hub.field && i<4;++i) {
        if (hub.field != numbers[i]) continue;
        char* end; long value = std::strtol(numbers[i],&end,10);
        unsigned minimum = i == 0 ? (rules.mode == 1 ? 0 : 1) : i == 1 ? 1 : i == 2 ? 0 : 50;
        unsigned maximum = i == 3 ? 200 : 99;
        if (end == numbers[i] || *end || value < minimum || value > maximum || (i == 3 && value%10)) {
            message(i == 3 ? "Damage ratio: enter 50 to 200 in steps of 10." : "Enter a value within the displayed range.");
        } else if (i == 0) rules.time_limit = uint8_t(value);
        else if (i == 1) rules.stock_count = uint8_t(value);
        else if (i == 2) rules.stock_time_limit = uint8_t(value);
        else rules.damage_ratio = uint8_t(value/10);
        format(i);
    }
}
bool nativeHubRules_Activate(HubAction action,int value)
{
    if (action == HubAction::Rules) { nativeHubRules_Open(); return true; }
    if (!nativeHubRules_Page(hub.page)) return false;
    if (action == HubAction::Back) {
        if (hub.page == NativeHubPage_ItemSwitch) nativeHub_ChangePage(NativeHubPage_MatchRules,true);
        else if (from_css) { nativeHub_ChangePage(return_page,true); nativeHub_Activate(HubAction::Selection,0); }
        else nativeHub_ChangePage(return_page,true);
        return true;
    }
    if (action < HubAction::RuleField || action > HubAction::RuleSave) return false;
    if (!can_edit()) { nativeHub_Notice("Return to character select as the lobby host to change rules."); return true; }
    switch (action) {
    case HubAction::RuleField:
        if (unsigned(value)<4) nativeHub_Focus(numbers[value],8);
        return true;
    case HubAction::RuleMode:
        nativeHub_Focus(nullptr,0);
        if (unsigned(value)<3) rules.mode = uint8_t(value);
        if (rules.mode != 1 && !rules.time_limit) { rules.time_limit = 1; format(0); }
        return true;
    case HubAction::RuleToggle:
        if (value == 0) rules.friendly_fire = !rules.friendly_fire;
        else if (value == 1) rules.pause = !rules.pause;
        else if (value == 2) rules.score_display = !rules.score_display;
        return true;
    case HubAction::RuleItems: nativeHub_Focus(nullptr,0); nativeHub_ChangePage(NativeHubPage_ItemSwitch); return true;
    case HubAction::RuleRate:
        prefs.item_freq = prefs.item_freq == UINT8_MAX ? 0 : prefs.item_freq >= 4 ? UINT8_MAX : prefs.item_freq+1;
        if (prefs.item_freq != UINT8_MAX && !(prefs.item_mask & item_mask)) prefs.item_mask = item_mask;
        return true;
    case HubAction::RuleItem:
        if (unsigned(value)<32 && (item_mask & (UINT64_C(1)<<value))) prefs.item_mask ^= UINT64_C(1)<<value;
        if (!(prefs.item_mask & item_mask)) prefs.item_freq = UINT8_MAX;
        return true;
    case HubAction::RuleAll: prefs.item_mask = value ? item_mask : 0; if (!value) prefs.item_freq = UINT8_MAX; return true;
    case HubAction::RuleSave: save(); return true;
    default: return false;
    }
}
void nativeHubRules_Render(NativeHubPage page)
{
    nativeHub_Button("rules-close","< Back",40,402,110,28,HubAction::Back);
    nativeHub_Button("rules-save","Save rules",417,402,166,28,HubAction::RuleSave,0,true,can_edit());
    nativeMenuPaint_Text(status,40,450,555,11,muted);
    if (page == NativeHubPage_ItemSwitch) {
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
    const char* labels[] = {"Time","Stock","Coin"}; const char* ids[] = {"rules-mode-time","rules-mode-stock","rules-mode-coin"};
    for (unsigned i=0;i<3;++i) nativeHub_Button(ids[i],labels[i],45+i*184,105,172,32,HubAction::RuleMode,int(i),rules.mode==i);
    numeric(0,"rules-time","Time limit","1-99 minutes (Stock: 0-99)",50,170);
    numeric(1,"rules-stocks","Stocks","1-99 lives",339,170);
    numeric(2,"rules-stock-time","Stock timer","0-99 minutes; 0: no timer",50,230);
    numeric(3,"rules-damage","Damage ratio","50-200%, in steps of 10",339,230);
    nativeHub_Button("rules-friendly",rules.friendly_fire?"Friendly fire: On":"Friendly fire: Off",45,286,267,29,HubAction::RuleToggle,0);
    nativeHub_Button("rules-pause",rules.pause?"Pause: On":"Pause: Off",327,286,267,29,HubAction::RuleToggle,1);
    nativeHub_Button("rules-score",rules.score_display?"Score display: On":"Score display: Off",45,333,267,29,HubAction::RuleToggle,2);
    nativeHub_Button("rules-item-switch","Item switch >",327,333,267,29,HubAction::RuleItems);
}
void nativeHubRules_Inspect()
{
    std::fprintf(stderr,"[hub-inspect] match-rules page=%u editable=%d css=%d mode=%u time=%u stock=%u timer=%u damage=%u friendly=%u pause=%u score=%u freq=%u mask=%llx status=%s\n",
        unsigned(hub.page),can_edit(),from_css,unsigned(rules.mode),unsigned(rules.time_limit),unsigned(rules.stock_count),unsigned(rules.stock_time_limit),unsigned(rules.damage_ratio)*10,
        unsigned(rules.friendly_fire),unsigned(rules.pause),unsigned(rules.score_display),unsigned(prefs.item_freq),(unsigned long long)prefs.item_mask,status);
}
