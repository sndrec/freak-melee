#include "native_feedback_view.h"
#include <aurora/rmlui.hpp>
#include <RmlUi/Core.h>
#include <RmlUi/Core/Elements/ElementFormControl.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_events.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace {
Rml::ElementDocument* document;
NativeFeedbackDraft* draft;
Rml::ObserverPtr<Rml::Element> hidden[32];
unsigned hidden_count;
Rml::ObserverPtr<Rml::Element> old_focus;
bool old_overlay, locked, modal, discard_confirmation, retry_enabled, finished, controls_visible;
NativeFeedbackAction action;
int selected = -1;
uint64_t bubble_until;
float image_left, image_top, image_width, image_height;

Rml::Element* element(const char* id) { return document->GetElementById(id); }
void visible(const char* id, bool show) { element(id)->SetProperty("display", show ? "block" : "none"); }
void text(const char* id, const char* value) { element(id)->SetInnerRML(Rml::StringUtilities::EncodeRml(value)); }
void position(Rml::Element* item, const char* property, float value)
{ item->SetProperty(property, std::to_string(value) + "px"); }
bool contains(Rml::Element* item, Rml::Vector2f pointer)
{
    if (!item->IsVisible()) return false;
    const auto origin = item->GetAbsoluteOffset(Rml::BoxArea::Border);
    const auto size = item->GetBox().GetSize(Rml::BoxArea::Border);
    const auto point = pointer - origin;
    return point.x >= 0 && point.x < size.x && point.y >= 0 && point.y < size.y;
}
void layout()
{
    const auto size = document->GetContext()->GetDimensions();
    const float scale = std::min(float(size.x) / draft->image.width, float(size.y) / draft->image.height);
    image_width = draft->image.width * scale; image_height = draft->image.height * scale;
    image_left = (size.x - image_width) * .5f; image_top = (size.y - image_height) * .5f;
    auto* image = element("feedback-image");
    position(image, "left", image_left); position(image, "top", image_top);
    position(image, "width", image_width); position(image, "height", image_height);
    if (selected >= 0 && selected < int(draft->count)) {
        const auto& pin = draft->pins[selected];
        auto* bubble = element("feedback-bubble");
        const float x = image_left + pin.x * image_width;
        const float y = image_top + pin.y * image_height;
        const float left = std::clamp(x - 175, 8.f, std::max(8.f, float(size.x) - 360));
        const float measured = bubble->GetBox().GetSize(Rml::BoxArea::Border).y;
        const float height = measured > 40 ? measured : 210;
        const bool above = y > height + 115;
        const float top = above ? y - 46 - height : std::min(y + 22, std::max(74.f, float(size.y) - height - 10));
        position(bubble, "left", left); position(bubble, "top", top);
        auto* tail = bubble->GetChild(bubble->GetNumChildren() - 1);
        position(tail, "left", std::clamp(x - left - 8, 14.f, 322.f));
        tail->SetProperty("bottom", above ? "-10px" : "auto");
        tail->SetProperty("top", above ? "auto" : "-9px");
        bubble->SetProperty("transform-origin", std::to_string(x - left) + "px " + (above ? "100%" : "0%"));
    }
}
void pins(int arriving = -1)
{
    Rml::String html;
    for (unsigned i = 0; i < draft->count; ++i) {
        const auto& pin = draft->pins[i];
        html += "<button class='feedback-pin" + Rml::String(int(i) == arriving ? " arriving" : "") +
            "' id='feedback-pin-" + std::to_string(i) + "' style='left:" + std::to_string(pin.x * 100) +
            "%;top:" + std::to_string(pin.y * 100) + "%;'>" + std::to_string(pin.id) + "<span class='needle'/></button>";
    }
    element("feedback-pins")->SetInnerRML(html);
}
void remove_note()
{
    if (selected < 0) return;
    for (unsigned i = unsigned(selected); i + 1 < draft->count; ++i) draft->pins[i] = draft->pins[i+1];
    --draft->count; selected = -1; bubble_until = 0;
    visible("feedback-bubble", false); pins();
}
bool commit()
{
    if (selected < 0 || bubble_until) return true;
    auto* input = static_cast<Rml::ElementFormControl*>(element("feedback-note"));
    const auto value = input->GetValue();
    unsigned units = 0;
    for (unsigned char c : value) if ((c & 0xC0) != 0x80) units += c >= 0xF0 ? 2 : 1;
    if (value.size() >= sizeof(NativeFeedbackPin::text) || units > 2048) {
        text("feedback-status", "Please shorten this note to 2,048 characters."); input->Focus(); return false;
    }
    if (value.find_first_not_of(" \r\n\t") == Rml::String::npos) { remove_note(); return true; }
    std::snprintf(draft->pins[selected].text, sizeof(NativeFeedbackPin::text), "%s", value.c_str());
    input->Blur(); element("feedback-bubble")->SetClass("committing", true);
    bubble_until = SDL_GetTicks() + 160; return true;
}
void edit(int index)
{
    if (selected != index && !commit()) return;
    selected = index; bubble_until = 0;
    element("feedback-bubble")->SetClass("committing", false);
    auto* input = static_cast<Rml::ElementFormControl*>(element("feedback-note"));
    input->SetValue(draft->pins[index].text);
    text("feedback-note-number", std::to_string(draft->pins[index].id).c_str());
    visible("feedback-hover", false); visible("feedback-bubble", true); layout(); input->Focus();
}
int pin_index(Rml::Element* item)
{
    while (item && item != document) {
        unsigned index; char extra;
        if (std::sscanf(item->GetId().c_str(), "feedback-pin-%u%c", &index, &extra) == 1 && index < draft->count)
            return int(index);
        item = item->GetParentNode();
    }
    return -1;
}
class Events final : public Rml::EventListener {
    void ProcessEvent(Rml::Event& event) override
    {
        if (!draft) return;
        const auto& type = event.GetType();
        auto* target = event.GetTargetElement();
        const int pin = pin_index(target);
        if (type == "mousemove") {
            Rml::Vector2f pointer(float(event.GetParameter<int>("mouse_x", -1)),
                                  float(event.GetParameter<int>("mouse_y", -1)));
            auto* header = element("feedback-header");
            auto* status = element("feedback-status");
            header->SetClass("faded", !modal && contains(header, pointer) &&
                             !contains(element("feedback-exit"), pointer));
            status->SetClass("faded", !modal && contains(status, pointer));
            return;
        }
        if (type == "mouseover" && !locked && !modal && selected < 0 && pin >= 0) {
            text("feedback-hover", draft->pins[pin].text);
            const auto size = document->GetContext()->GetDimensions();
            position(element("feedback-hover"), "left", std::clamp(image_left + draft->pins[pin].x * image_width + 18,
                8.f, std::max(8.f, float(size.x) - 330)));
            position(element("feedback-hover"), "top", std::clamp(image_top + draft->pins[pin].y * image_height + 15,
                74.f, std::max(74.f, float(size.y) - 150)));
            visible("feedback-hover", *draft->pins[pin].text != 0); return;
        }
        if (type == "mouseout") { visible("feedback-hover", false); return; }
        if (type != "click") return;
        const auto& id = target->GetId();
        if (id == "feedback-close") { action = NativeFeedbackAction::Close; return; }
        if (id == "feedback-retry") { action = NativeFeedbackAction::Retry; return; }
        if (locked) return;
        if (id == "feedback-exit") { action = NativeFeedbackAction::ConfirmDiscard; return; }
        if (id == "feedback-keep") { modal = false; visible("feedback-modal", false); return; }
        if (id == "feedback-send") { action = discard_confirmation ? NativeFeedbackAction::Discard : NativeFeedbackAction::Send; return; }
        if (modal) return;
        if (id == "feedback-done") { commit(); return; }
        if (id == "feedback-remove") { remove_note(); return; }
        if (pin >= 0) { edit(pin); return; }
        if (id == "feedback-image" || id == "feedback-pins" || target == document) {
            if (!commit()) return;
            if (draft->count == 64) { text("feedback-status", "This report has 64 notes. Edit or remove a pin to add another."); return; }
            const float x = (event.GetParameter<int>("mouse_x", 0) - image_left) / image_width;
            const float y = (event.GetParameter<int>("mouse_y", 0) - image_top) / image_height;
            if (x < 0 || x > 1 || y < 0 || y > 1) return;
            const unsigned index = draft->count++;
            draft->pins[index] = {++draft->next_pin, x, y, {}};
            pins(int(index)); edit(int(index));
        }
    }
} events;
}

bool nativeFeedbackView_Open(NativeFeedbackDraft& value)
{
    auto* context = aurora::rmlui::get_context();
    if (!context || context->GetNumDocuments() > 32 || (!document && context->GetNumDocuments() == 32)) return false;
    if (!document) {
        document = context->LoadDocument("ui/freak_feedback.rml");
        if (!document) return false;
        for (const char* name : {"click", "mousemove", "mouseover", "mouseout"}) document->AddEventListener(name, &events);
    }
    draft = &value; action = NativeFeedbackAction::None; selected = -1;
    bubble_until = 0; locked = modal = discard_confirmation = retry_enabled = finished = false;
    controls_visible = true; visible("feedback-header", true); visible("feedback-status", true);
    hidden_count = 0;
    // Finish pointer capture outside the old document's hover targets before
    // hiding it. Drag handlers receive their release without clicking a menu.
    SDL_Event release{}; release.type = SDL_EVENT_MOUSE_BUTTON_UP;
    release.button.x = release.button.y = -100;
    for (unsigned button = 1; button <= 5; ++button) {
        release.button.button = Uint8(button); aurora::rmlui::handle_event(release);
    }
    old_focus = {};
    if (auto* focus = context->GetFocusElement()) old_focus = focus->GetObserverPtr();
    for (int i = 0; i < context->GetNumDocuments(); ++i) {
        auto* item = context->GetDocument(i);
        if (item != document && item->IsVisible()) { hidden[hidden_count++] = item->GetObserverPtr(); item->Hide(); }
    }
    old_overlay = aurora::rmlui::get_surface_overlay(); aurora::rmlui::set_surface_overlay(true);
    visible("feedback-modal", false); visible("feedback-bubble", false); visible("feedback-hover", false);
    element("feedback-bubble")->SetClass("committing", false);
    element("feedback-header")->SetClass("faded", false);
    element("feedback-status")->SetClass("faded", false);
    text("feedback-status", "Screenshot and gameplay context captured. Tab hides controls. Escape cancels.");
    pins(); layout(); document->Show(); document->PullToFront(); return true;
}
void nativeFeedbackView_Close()
{
    if (!draft) return;
    document->Hide();
    for (unsigned i = 0; i < hidden_count; ++i)
        if (auto* item = hidden[i].get()) static_cast<Rml::ElementDocument*>(item)->Show();
    if (auto* focus = old_focus.get(); focus && focus->IsVisible()) focus->Focus();
    aurora::rmlui::set_surface_overlay(old_overlay);
    hidden_count = 0; old_focus = {}; draft = nullptr;
}
void nativeFeedbackView_Update()
{
    if (!draft) return;
    if (bubble_until && SDL_GetTicks() >= bubble_until) {
        bubble_until = 0; selected = -1; visible("feedback-bubble", false);
        element("feedback-bubble")->SetClass("committing", false);
    }
    layout();
}
bool nativeFeedbackView_Key(int key, bool shift)
{
    if (!draft) return false;
    if (key == SDL_SCANCODE_TAB && !modal && (selected < 0 || bubble_until)) {
        controls_visible = !controls_visible;
        visible("feedback-header", controls_visible); visible("feedback-status", controls_visible); return true;
    }
    if (key == SDL_SCANCODE_ESCAPE) {
        if (locked) { if (retry_enabled || finished) action = NativeFeedbackAction::Close; return true; }
        if (modal) { modal = false; visible("feedback-modal", false); }
        else if (selected >= 0 && !bubble_until) {
            if (!*draft->pins[selected].text) remove_note();
            else { selected = -1; visible("feedback-bubble", false); }
        }
        else action = NativeFeedbackAction::ConfirmDiscard;
        return true;
    }
    if ((key == SDL_SCANCODE_RETURN || key == SDL_SCANCODE_KP_ENTER) && !shift) {
        if (locked) {
            if (retry_enabled) action = NativeFeedbackAction::Retry;
            else if (finished) action = NativeFeedbackAction::Close;
            return true;
        }
        if (modal) action = discard_confirmation ? NativeFeedbackAction::Discard : NativeFeedbackAction::Send;
        else if (selected >= 0 && !bubble_until) commit();
        else { if (commit()) action = NativeFeedbackAction::ConfirmSend; }
        return true;
    }
    return false;
}
NativeFeedbackAction nativeFeedbackView_Action()
{ const auto result = action; action = NativeFeedbackAction::None; return result; }
void nativeFeedbackView_Confirm(bool discard)
{
    if (!commit()) return;
    selected = -1; bubble_until = 0; visible("feedback-bubble", false); visible("feedback-hover", false);
    modal = true; discard_confirmation = discard;
    text("feedback-dialog-title", discard ? "Discard feedback?" : "Send feedback?");
    text("feedback-dialog-text", discard ? "Discard this screenshot and your notes?" : "Send this screenshot, your notes, and gameplay diagnostics?");
    text("feedback-send", discard ? "Discard feedback" : "Send feedback");
    visible("feedback-modal", true); visible("feedback-keep", true); visible("feedback-send", true);
    visible("feedback-retry", false); visible("feedback-close", false);
    element("feedback-keep")->Focus();
}
void nativeFeedbackView_Status(const char* message, bool lock, bool retry, bool complete)
{
    text("feedback-status", message); locked = lock; retry_enabled = retry; finished = complete;
    if (!lock) {
        modal = false; controls_visible = true;
        visible("feedback-modal", false); visible("feedback-header", true); visible("feedback-status", true); return;
    }
    selected = -1; bubble_until = 0; visible("feedback-bubble", false); visible("feedback-hover", false);
    modal = true; visible("feedback-modal", true);
    text("feedback-dialog-title", complete ? "Feedback sent" : retry ? "Could not send feedback" : "Sending feedback");
    text("feedback-dialog-text", message);
    visible("feedback-keep", false); visible("feedback-send", false);
    visible("feedback-retry", retry); visible("feedback-close", retry || complete);
    if (retry) element("feedback-retry")->Focus();
    else if (complete) element("feedback-close")->Focus();
}
