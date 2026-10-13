#include "native_shell_rules.h"

#ifdef MELEE_NATIVE

#include "native_shell.h"
#include "native_shell_profiles.h"
#include "native_shell_collection.h"

extern "C" {
#include <melee/gm/gmmain_lib.h>
}
#include <melee/gm/native_home.h>
#include <melee/gm/native_online_session.h>
#include <melee/gm/native_settings.h>

#include <RmlUi/Core.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <SDL3/SDL_timer.h>

#include <cstdint>
#include <cstdlib>
#include <string>

namespace {

Rml::ElementDocument* document;
GameRules draft_rules;
struct GamePrefs draft_prefs;
bool opened;
uint64_t hide_after_slide;

constexpr const char* item_rates[] = {
    "Off", "Very low", "Low", "Medium", "High", "Very high"
};
// gm_1601's 32-entry item switch has one sentinel (bit 28), not a spawnable
// item. The remaining bits are the original menu's saved item preferences.
constexpr uint64_t selectable_items_mask = UINT64_C(0xEFFFFFFF);

Rml::ElementFormControlInput* input(const char* id)
{
    return static_cast<Rml::ElementFormControlInput*>(
        document->GetElementById(id));
}

void set_status(const char* message)
{
    document->GetElementById("rules-status")->SetInnerRML(message);
}

bool can_edit()
{
    return nativeHome_IsSelection() &&
           (nativeOnlineSession_State() ==
                NativeLobbySessionState_Disconnected ||
            nativeOnlineSession_IsOwner());
}

void render_choices()
{
    for (unsigned mode = 0; mode < 3; ++mode) {
        const char* id = mode == 0 ? "rules-mode-time"
                         : mode == 1 ? "rules-mode-stock"
                                     : "rules-mode-coin";
        document->GetElementById(id)->SetClass("selected",
                                                 draft_rules.mode == mode);
    }
    const unsigned rate = draft_prefs.item_freq == UINT8_MAX
                              ? 0
                              : unsigned(draft_prefs.item_freq) + 1;
    document->GetElementById("rules-items")
        ->SetInnerRML(item_rates[rate < 6 ? rate : 0]);
    unsigned selected_items = 0;
    for (unsigned bit = 0; bit < 32; ++bit) {
        if (!(selectable_items_mask & (UINT64_C(1) << bit))) {
            continue;
        }
        const bool selected =
            (draft_prefs.item_mask & (UINT64_C(1) << bit)) != 0;
        document->GetElementById("rules-item-" + std::to_string(bit))
            ->SetClass("selected", selected);
        selected_items += selected;
    }
    document->GetElementById("rules-item-count")
        ->SetInnerRML(std::to_string(selected_items) + " selected");
    document->GetElementById("rules-friendly")
        ->SetInnerRML(draft_rules.friendly_fire ? "On" : "Off");
    document->GetElementById("rules-pause")
        ->SetInnerRML(draft_rules.pause ? "On" : "Off");
    document->GetElementById("rules-score")
        ->SetInnerRML(draft_rules.score_display ? "On" : "Off");
}

void render_inputs()
{
    input("rules-time")->SetValue(std::to_string(draft_rules.time_limit));
    input("rules-stocks")->SetValue(std::to_string(draft_rules.stock_count));
    input("rules-stock-time")
        ->SetValue(std::to_string(draft_rules.stock_time_limit));
    input("rules-damage")
        ->SetValue(std::to_string(unsigned(draft_rules.damage_ratio) * 10));
}

bool read_number(const char* id, unsigned minimum, unsigned maximum,
                 uint8_t* value)
{
    const std::string entered = input(id)->GetValue();
    if (entered.empty()) {
        return false;
    }
    char* end = nullptr;
    const long parsed = std::strtol(entered.c_str(), &end, 10);
    if (end == entered.c_str() || *end != '\0' || parsed < minimum ||
        parsed > maximum)
    {
        return false;
    }
    *value = uint8_t(parsed);
    return true;
}

void save_rules()
{
    if (!can_edit()) {
        set_status("Only the room host can change match rules.");
        return;
    }
    GameRules next_rules = *gmMainLib_GetGameRules();
    struct GamePrefs next_prefs = *gmMainLib_GetGamePrefs();
    next_rules.mode = draft_rules.mode;
    next_rules.friendly_fire = draft_rules.friendly_fire;
    next_rules.pause = draft_rules.pause;
    next_rules.score_display = draft_rules.score_display;
    next_prefs.item_freq = draft_prefs.item_freq;
    next_prefs.item_mask = draft_prefs.item_mask;
    const unsigned min_time = next_rules.mode == 1 ? 0 : 1;
    uint8_t damage_percent;
    if (next_rules.mode > 2 ||
        !read_number("rules-time", min_time, 99, &next_rules.time_limit) ||
        !read_number("rules-stocks", 1, 99, &next_rules.stock_count) ||
        !read_number("rules-stock-time", 0, 99,
                     &next_rules.stock_time_limit) ||
        !read_number("rules-damage", 50, 200, &damage_percent) ||
        damage_percent % 10 != 0)
    {
        set_status("Check the highlighted rule ranges and try again.");
        return;
    }
    next_rules.damage_ratio = damage_percent / 10;
    if (next_prefs.item_freq != UINT8_MAX && next_prefs.item_mask == 0) {
        next_prefs.item_mask = UINT64_MAX;
    }
    if (!nativeSettings_Save(&next_rules, &next_prefs)) {
        set_status("Could not save match rules.");
        return;
    }
    *gmMainLib_GetGameRules() = next_rules;
    *gmMainLib_GetGamePrefs() = next_prefs;
    draft_rules = next_rules;
    draft_prefs = next_prefs;
    set_status("Rules saved. They apply to the next match.");
}

void open_rules()
{
    if (!nativeHome_IsSelection()) {
        nativeShell_ShowRoomError("Return to character select to edit rules.");
        return;
    }
    if (!can_edit()) {
        nativeShell_ShowRoomError("Only the room host can edit match rules.");
        return;
    }
    nativeShellProfiles_Close();
    nativeShellCollection_Close();
    draft_rules = *gmMainLib_GetGameRules();
    draft_prefs = *gmMainLib_GetGamePrefs();
    render_choices();
    render_inputs();
    set_status("");
    hide_after_slide = 0;
    opened = true;
    document->GetElementById("rules-panel")
        ->SetProperty("visibility", "visible");
    document->GetElementById("rules-panel")->SetClass("open", true);
    document->GetElementById("rules-close")->Focus(true);
}

class RuleEvents final : public Rml::EventListener {
    void ProcessEvent(Rml::Event& event) override
    {
        Rml::Element* target = event.GetTargetElement();
        while (target != nullptr && target->GetId().empty()) {
            target = target->GetParentNode();
        }
        if (target == nullptr || target->HasAttribute("disabled")) {
            return;
        }
        const Rml::String& id = target->GetId();
        if (event.GetType() == "keydown") {
            const int key = event.GetParameter<int>("key_identifier", 0);
            if (key == Rml::Input::KI_ESCAPE) {
                nativeShellRules_Close();
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
        if (id == "rules-tab") {
            open_rules();
            return;
        }
        if (!opened || !can_edit()) {
            return;
        }
        if (id == "rules-close") {
            nativeShellRules_Close();
        } else if (id == "rules-save") {
            save_rules();
        } else if (id == "rules-mode-time") {
            draft_rules.mode = 0;
            render_choices();
        } else if (id == "rules-mode-stock") {
            draft_rules.mode = 1;
            render_choices();
        } else if (id == "rules-mode-coin") {
            draft_rules.mode = 2;
            render_choices();
        } else if (id == "rules-items") {
            draft_prefs.item_freq = draft_prefs.item_freq == UINT8_MAX
                                        ? 0
                                        : draft_prefs.item_freq == 4
                                              ? UINT8_MAX
                                              : draft_prefs.item_freq + 1;
            if (draft_prefs.item_freq != UINT8_MAX &&
                !(draft_prefs.item_mask & selectable_items_mask)) {
                draft_prefs.item_mask = selectable_items_mask;
            }
            render_choices();
        } else if (id == "rules-item-all") {
            draft_prefs.item_mask = selectable_items_mask;
            render_choices();
        } else if (id == "rules-item-none") {
            draft_prefs.item_mask = 0;
            draft_prefs.item_freq = UINT8_MAX;
            render_choices();
        } else if (id == "rules-friendly") {
            draft_rules.friendly_fire ^= 1;
            render_choices();
        } else if (id == "rules-pause") {
            draft_rules.pause ^= 1;
            render_choices();
        } else if (id == "rules-score") {
            draft_rules.score_display ^= 1;
            render_choices();
        } else if (id.compare(0, 11, "rules-item-") == 0) {
            char* end = nullptr;
            const long bit = std::strtol(id.c_str() + 11, &end, 10);
            if (*end == '\0' && bit >= 0 && bit < 32 &&
                (selectable_items_mask & (UINT64_C(1) << bit))) {
                draft_prefs.item_mask ^= UINT64_C(1) << bit;
                if (!(draft_prefs.item_mask & selectable_items_mask)) {
                    draft_prefs.item_mask = 0;
                    draft_prefs.item_freq = UINT8_MAX;
                }
                render_choices();
            }
        }
    }
} events;

} // namespace

void nativeShellRules_Init(Rml::ElementDocument* shell)
{
    document = shell;
    opened = false;
    hide_after_slide = 0;
    for (const char* id : { "rules-tab", "rules-panel" }) {
        Rml::Element* element = document->GetElementById(id);
        element->AddEventListener("click", &events);
        element->AddEventListener("keydown", &events);
    }
}

void nativeShellRules_Close()
{
    if (!opened) {
        return;
    }
    opened = false;
    hide_after_slide = SDL_GetTicks() + 250;
    document->GetElementById("rules-panel")->SetClass("open", false);
    nativeShell_BlurRoom();
}

void nativeShellRules_Update()
{
    if (hide_after_slide && SDL_GetTicks() >= hide_after_slide) {
        document->GetElementById("rules-panel")
            ->SetProperty("visibility", "hidden");
        hide_after_slide = 0;
    }
    if (opened && !can_edit()) {
        nativeShellRules_Close();
    }
}

extern "C" bool nativeShellRules_IsOpen(void)
{
    return opened;
}

#endif
