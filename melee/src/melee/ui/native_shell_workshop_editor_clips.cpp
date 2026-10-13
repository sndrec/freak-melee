#include "native_shell_workshop_editor_internal.h"

#ifdef MELEE_NATIVE
namespace WorkshopEditor {

void read_demo_clips()
{
    session->demo_count = std::min<size_t>(session->project->demo_count, 128);
    for (size_t i = 0; i < session->demo_count; ++i) {
        NativeFighterAnimationInfo info = {};
        const NativeFighterAnimationClip clip = { NativeFighterAnimation_Demo, (unsigned) i };
        session->demo_has_clip[i] = nativeFighterAnimation_Info(session->project, clip, &info);
        session->demo_clip_frames[i] = session->demo_has_clip[i] ? info.frames : 0.0f;
        session->demo_clip_names[i].clear();
        if (!session->demo_has_clip[i]) continue;
        std::string name = info.name;
        const size_t action = name.find("_ACTION_");
        if (action != std::string::npos) name.erase(0, action + 8);
        const size_t suffix = name.rfind("_figatree");
        if (suffix != std::string::npos && suffix + 9 == name.size()) name.resize(suffix);
        session->demo_clip_names[i] = name;
    }
}

void select_demo_clip(unsigned index)
{
    if (session->project == nullptr || index >= session->demo_count ||
        !session->demo_has_clip[index]) return;
    cancel_session_gestures();
    const bool changed = session->animation_clip.bank != NativeFighterAnimation_Demo ||
                         session->animation_clip.index != index;
    session->animation_clip = { NativeFighterAnimation_Demo, index };
    session->animation_name = session->demo_clip_names[index];
    if (changed) session->animation_frame = 0;
    session->displayed_animation_clip.index = UINT_MAX;
    set_text("workshop-action-title", session->animation_name);
    set_text("workshop-context-action", "Non-gameplay animation");
    set_text("workshop-context-clip", session->animation_name);
    set_text("workshop-action-detail", "Owned demo clip " + number(index));
    set_text("workshop-editor-status", "");
    nativeShellWorkshopEditor_FilterClips();
    show_animation();
}

} // namespace WorkshopEditor

using namespace WorkshopEditor;

void nativeShellWorkshopEditor_FilterClips()
{
    if (document == nullptr || session->project == nullptr) return;
    read_demo_clips();
    std::string search = field("workshop-animation-search")->GetValue();
    for (char& ch : search) ch = (char) std::tolower((unsigned char) ch);
    std::string groups[2];
    for (unsigned bank = 0; bank < 2; ++bank) {
        const bool demo = bank == 1;
        const size_t count = demo ? session->demo_count : session->action_count;
        for (size_t i = 0; i < count; ++i) {
            if (!(demo ? session->demo_has_clip[i] : session->action_has_clip[i])) continue;
            const std::string name = demo ? session->demo_clip_names[i] : session->actions[i].name;
            const std::string label = WorkshopLabels::action(name.c_str());
            std::string searchable = name + " " + label + " " + number((unsigned) i);
            for (char& ch : searchable) ch = (char) std::tolower((unsigned char) ch);
            if (!search.empty() && searchable.find(search) == std::string::npos) continue;
            const bool selected = session->animation_clip.bank ==
                (demo ? NativeFighterAnimation_Demo : NativeFighterAnimation_Action) &&
                session->animation_clip.index == i;
            const float frames = demo ? session->demo_clip_frames[i] : session->action_clip_frames[i];
            groups[bank] += "<button id='" + std::string(demo ? "workshop-demo-" : "workshop-clip-") +
                number((unsigned) i) + "' class='workshop-action-row workshop-clip-row" +
                (selected ? " selected" : "") + "'><span>" +
                Rml::StringUtilities::EncodeRml(label) +
                "</span><small class='workshop-clip-length'>" +
                number((unsigned) std::ceil(frames)) + "f</small></button>";
        }
    }
    std::string markup;
    if (!groups[0].empty()) markup += "<div class='workshop-action-group'>GAMEPLAY</div>" + groups[0];
    if (!groups[1].empty()) markup += "<div class='workshop-action-group'>RESULTS &amp; OTHER CLIPS</div>" + groups[1];
    document->GetElementById("workshop-animation-clips")->SetInnerRML(
        markup.empty() ? "<span class='workshop-muted'>No matching clips.</span>" : markup);
    session->clip_scroll_delay = session->animation_clip.index != UINT_MAX ? 2 : 0;
}
#endif
