#include "native_shell_workshop_editor_internal.h"
#include <melee/ft/ftaction_codec.h>
extern "C" {
#include <melee/lb/lbaudio_ax.h>
}

#include <algorithm>
#include <cctype>
#include <cstring>

namespace WorkshopEditor {
namespace {
constexpr unsigned page_size = 8;

void stop_audition()
{
    if (session->audition_handle >= 0)
        lbAudioAx_800236B8(session->audition_handle);
    session->audition_handle = -1;
}

void draw_picker()
{
    const auto filter = field("workshop-bank-search")->GetValue();
    std::string query = filter;
    std::transform(query.begin(), query.end(), query.begin(),
                   [](unsigned char c) { return char(std::tolower(c)); });
    const bool package = field("workshop-sound-package")->HasAttribute("checked");
    const unsigned start = session->sound_page * page_size;
    std::string markup;
    unsigned matches = 0;
    const auto offer = [&](const char* bank, unsigned id) {
        const std::string key = number(id);
        const std::string label = std::string(bank) + " · " + key;
        if (label.find(query) == std::string::npos) return;
        const unsigned row = matches++;
        if (row < start || row >= start + page_size) return;
        markup += "<div class='workshop-inspector-row'><button id='workshop-bank-use-" +
            key + "'>" + Rml::StringUtilities::EncodeRml(label) +
            "</button><button id='workshop-bank-play-" + key + "'>Play</button></div>";
    };
    if (package) {
        for (unsigned clip = 0; clip < FT_ACTION_CLIP_COUNT &&
             nativeHome_WorkshopSound(clip, true, 6) >= 0; ++clip)
            offer("fighter clip", clip);
    } else {
        for (unsigned bank = 0; bank < 55; ++bank) {
            int first, last;
            if (const char* name = nativeAudio_LoadedSoundBank(bank, &first, &last)) {
                for (int sound = first; sound <= last; ++sound)
                    offer(name, (unsigned) sound);
            }
        }
    }
    document->GetElementById("workshop-bank-results")->SetInnerRML(
        markup.empty() ? "<div class='workshop-muted'>No loaded sounds match.</div>" : markup);
    auto* previous = document->GetElementById("workshop-bank-prev");
    auto* next = document->GetElementById("workshop-bank-next");
    if (start) previous->RemoveAttribute("disabled");
    else previous->SetAttribute("disabled", "");
    if (matches > start + page_size) next->RemoveAttribute("disabled");
    else next->SetAttribute("disabled", "");
}

void audition(unsigned id)
{
    uint16_t channel, volume, pan;
    if (!parse_u16("workshop-sound-channel", 6, &channel) ||
        !parse_u16("workshop-sound-volume", 255, &volume) ||
        !parse_u16("workshop-sound-pan", 255, &pan)) return;
    stop_audition();
    const int sound = nativeHome_WorkshopSound(
        id, field("workshop-sound-package")->HasAttribute("checked"), channel);
    if (sound >= 0) session->audition_handle = lbAudioAx_800237A8(sound, volume, pan);
    if (session->audition_handle < 0)
        set_text("workshop-editor-status", "This sound is unavailable in the current preview.");
}
} // namespace

void close_sound_picker()
{
    stop_audition();
    document->GetElementById("workshop-bank-picker")->SetProperty("display", "none");
}

bool sound_picker_click(const char* id)
{
    if (!std::strcmp(id, "workshop-browse-sounds")) {
        auto* picker = document->GetElementById("workshop-bank-picker");
        if (picker->IsVisible()) close_sound_picker();
        else {
            picker->SetProperty("display", "block");
            nativeShellWorkshopEditor_FilterSounds();
            field("workshop-bank-search")->Focus();
        }
        return true;
    }
    if (!std::strcmp(id, "workshop-play-sound")) {
        uint32_t sound;
        if (parse_u32("workshop-sound-id", &sound)) audition(sound);
        return true;
    }
    if (!std::strcmp(id, "workshop-bank-prev") || !std::strcmp(id, "workshop-bank-next")) {
        if (std::strcmp(id, "workshop-bank-prev")) ++session->sound_page;
        else if (session->sound_page) --session->sound_page;
        draw_picker();
        return true;
    }
    unsigned sound;
    if (parse_row(id, "workshop-bank-play-", &sound)) {
        audition(sound);
        return true;
    }
    if (parse_row(id, "workshop-bank-use-", &sound)) {
        field("workshop-sound-id")->SetValue(number(sound));
        apply_sound();
        show_event();
        return true;
    }
    return false;
}
} // namespace WorkshopEditor

void nativeShellWorkshopEditor_FilterSounds()
{
    using namespace WorkshopEditor;
    session->sound_page = 0;
    draw_picker();
}
