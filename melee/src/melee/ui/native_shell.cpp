#include "native_shell.h"
#include <melee/gm/native_feedback.h>
#include <melee/gm/native_account_transport.h>
#include "native_hub_catalog.h"

#include "native_hub.h"
#include "native_chat.h"
#include "native_play_menu.h"
#include "native_css_chat.h"
#include "native_shell_css_original.h"
#include "native_shell_workshop.h"
#include "native_shell_workshop_agent.h"
#include "native_shell_workshop_code.h"
#include "native_shell_workshop_editor.h"
#include "native_shell_workshop_model.h"

#ifdef MELEE_NATIVE

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include <aurora/aurora.h>
#include <aurora/rmlui.hpp>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <melee/gm/native_home.h>
#include <melee/gm/native_services.h>
#include <melee/gm/native_online_session.h>
#include <melee/gm/native_profile_input.h>
#include <melee/mn/native_css_cards.h>
#include <melee/mn/native_css_interaction.h>
#include <melee/mn/native_online_menu.h>
#include <RmlUi/Core.h>
#include <RmlUi/Core/Elements/ElementFormControlSelect.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <SDL3/SDL_clipboard.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>

namespace {

    Rml::ElementDocument* document;
    Rml::Vector2i displayedDimensions;
    float game_pointer_left, game_pointer_top, game_pointer_width,
        game_pointer_height;
    float displayedDensity;
    bool displayedCssEditing;
    bool displayedHub;
    bool displayedWorkshop;
    bool displayedWorkshopEditor;
    bool displayedWorkshopScript;
    bool displayedWorkshopAnimation;
    bool displayedWorkshopModel;
    bool displayedWorkshopGraphFocused;
    float displayedWorkshopTimelineFraction;

    void update_layout()
    {
        const Rml::Context* context = document->GetContext();
        const Rml::Vector2i dimensions = context->GetDimensions();
        const float density = context->GetDensityIndependentPixelRatio();
        const bool editing = nativeShellCssOriginal_IsEditing();
        const bool hubVisible = nativeHub_IsVisible();
        const bool workshopOpen = nativeShellWorkshop_IsOpen();
        const bool workshop = workshopOpen && !nativeShellWorkshop_IsTesting();
        const bool workshopEditor =
            workshop && nativeShellWorkshopEditor_IsOpen();
        const bool workshopScript =
            workshop && nativeShellWorkshopEditor_IsScriptMode();
        const bool workshopAnimation =
            workshop && nativeShellWorkshopEditor_IsAnimationMode();
        const bool workshopModel =
            workshop && nativeShellWorkshopEditor_IsModelMode();
        const bool workshopGraphFocused =
            workshop && nativeShellWorkshopEditor_IsGraphFocused();
        const float workshopTimelineFraction =
            workshopEditor ? nativeShellWorkshopEditor_TimelineFraction()
                           : 0.0f;
        if (dimensions == displayedDimensions && density == displayedDensity &&
            editing == displayedCssEditing && hubVisible == displayedHub &&
            workshop == displayedWorkshop &&
            workshopEditor == displayedWorkshopEditor &&
            workshopScript == displayedWorkshopScript &&
            workshopAnimation == displayedWorkshopAnimation &&
            workshopModel == displayedWorkshopModel &&
            workshopGraphFocused == displayedWorkshopGraphFocused &&
            workshopTimelineFraction == displayedWorkshopTimelineFraction)
        {
            return;
        }
        displayedDimensions = dimensions;
        displayedDensity = density;
        displayedCssEditing = editing;
        displayedHub = hubVisible;
        displayedWorkshop = workshop;
        displayedWorkshopEditor = workshopEditor;
        displayedWorkshopScript = workshopScript;
        displayedWorkshopAnimation = workshopAnimation;
        displayedWorkshopModel = workshopModel;
        displayedWorkshopGraphFocused = workshopGraphFocused;
        displayedWorkshopTimelineFraction = workshopTimelineFraction;
        // The game stays 4:3; authoring tools reserve their own editing area.
        const int railWidth = (dimensions.x - dimensions.y * 4 / 3) / 2;
        const int workshopWidth = std::min(
            int(330 * density), std::max(0, (dimensions.x - 200) / 2));
        const int editorLeftWidth =
            std::clamp(int(dimensions.x * 0.135f), int(160 * density),
                       int(230 * density));
        const int editorRightWidth =
            workshopScript
                ? std::clamp(int(dimensions.x * 0.22f), int(290 * density),
                             int(360 * density))
                : std::clamp(int(dimensions.x * 0.16f), int(210 * density),
                             int(265 * density));
        const int leftWidth =
            workshopModel    ? int(232 * density)
            : workshopScript ? dimensions.x - editorRightWidth
            : workshopEditor ? editorLeftWidth
            : workshop       ? workshopWidth
            : editing ? std::min(dimensions.x - 1,
                                 std::max(railWidth * 2, int(320 * density)))
                      : railWidth;
        const int rightWidth = workshopModel ? int(267 * density)
                               : workshopEditor
                                   ? (workshopScript ? 0 : editorRightWidth)
                               : workshop ? workshopWidth
                                          : 0;
        const bool compactHeader = dimensions.x < int(1680 * density);
        const int editorTop = int((compactHeader ? 156 : 114) * density);
        document->GetElementById("workshop-panel")->SetClass("compact-header", compactHeader);
        for (const char* id : { "workshop-editor-browser", "workshop-editor-inspector",
                               "workshop-action-viewport", "workshop-animation-viewport",
                               "workshop-script-pane", "workshop-script-inspector",
                               "workshop-model-pane" }) {
            document->GetElementById(id)->SetProperty(Rml::PropertyId::Top,
                Rml::Property(float(editorTop), Rml::Unit::PX));
        }
        const int topMargin = workshopModel    ? editorTop + int(82 * density)
                              : workshopEditor ? editorTop
                                               : 0;
        const int workshopFooterHeight = int(30 * density);
        const int bottomMargin =
            workshopModel ? int(182 * density)
            : workshopEditor
                ? (workshopScript
                       ? std::max(0, dimensions.y - topMargin -
                                         int(174 * density))
                       : std::min(
                             std::max(0, dimensions.y - topMargin -
                                             int(120 * density)),
                             std::max(
                                 int(dimensions.y * workshopTimelineFraction),
                                 int((workshopAnimation ? 220 : 170) *
                                     density))))
                : 0;
        Rml::Element* leftRail = document->GetElementById("left-rail");
        leftRail->SetProperty(
            "display",
            editing && !workshopOpen && !hubVisible ? "flex" : "none");
        leftRail->SetProperty(Rml::PropertyId::Width,
                              Rml::Property(float(leftWidth), Rml::Unit::PX));
        leftRail->SetClass("css-editing", editing);
        for (const char* id : { "workshop-library", "workshop-inspector" }) {
            document->GetElementById(id)->SetProperty(
                Rml::PropertyId::Width,
                Rml::Property(float(std::strcmp(id, "workshop-library") == 0 &&
                                            !workshopEditor
                                        ? dimensions.x
                                        : workshopWidth),
                              Rml::Unit::PX));
        }
        for (const char* id : { "workshop-editor-browser" }) {
            document->GetElementById(id)->SetProperty(
                Rml::PropertyId::Width,
                Rml::Property(float(editorLeftWidth), Rml::Unit::PX));
        }
        for (const char* id :
             { "workshop-editor-browser", "workshop-editor-inspector" })
        {
            document->GetElementById(id)->SetProperty(
                Rml::PropertyId::Bottom,
                Rml::Property(float(bottomMargin), Rml::Unit::PX));
        }
        document->GetElementById("workshop-editor-inspector")
            ->SetProperty(
                Rml::PropertyId::Width,
                Rml::Property(float(editorRightWidth), Rml::Unit::PX));
        document->GetElementById("workshop-script-inspector")
            ->SetProperty(
                Rml::PropertyId::Width,
                Rml::Property(float(editorRightWidth), Rml::Unit::PX));
        for (const char* id :
             { "workshop-action-viewport", "workshop-animation-viewport" })
        {
            Rml::Element* viewport = document->GetElementById(id);
            viewport->SetProperty(
                Rml::PropertyId::Left,
                Rml::Property(float(editorLeftWidth), Rml::Unit::PX));
            viewport->SetProperty(
                Rml::PropertyId::Right,
                Rml::Property(float(editorRightWidth), Rml::Unit::PX));
            viewport->SetProperty(
                Rml::PropertyId::Bottom,
                Rml::Property(float(bottomMargin), Rml::Unit::PX));
        }
        for (const char* id :
             { "workshop-editor-timeline", "workshop-animation-pane" })
        {
            Rml::Element* timeline = document->GetElementById(id);
            timeline->SetProperty(
                Rml::PropertyId::Height,
                Rml::Property(
                    float(workshopGraphFocused &&
                                  std::strcmp(id, "workshop-animation-pane") ==
                                      0
                              ? dimensions.y - topMargin - workshopFooterHeight
                              : std::max(0,
                                         bottomMargin - workshopFooterHeight)),
                    Rml::Unit::PX));
            timeline->SetProperty(
                Rml::PropertyId::Bottom,
                Rml::Property(float(workshopFooterHeight), Rml::Unit::PX));
            if (std::strcmp(id, "workshop-animation-pane") == 0) {
                timeline->SetProperty("top",
                                      workshopGraphFocused
                                          ? std::to_string(topMargin) + "px"
                                          : "auto");
            }
        }
        document->GetElementById("workshop-script-pane")
            ->SetProperty(Rml::PropertyId::Left,
                          Rml::Property(0.0f, Rml::Unit::PX));
        document->GetElementById("workshop-script-pane")
            ->SetProperty(Rml::PropertyId::Width,
                          Rml::Property(float(leftWidth), Rml::Unit::PX));
        document->GetElementById("workshop-script-pane")
            ->SetProperty(
                Rml::PropertyId::Bottom,
                Rml::Property(float(workshopFooterHeight), Rml::Unit::PX));
        document->GetElementById("workshop-script-inspector")
            ->SetProperty(
                Rml::PropertyId::Bottom,
                Rml::Property(float(workshopFooterHeight), Rml::Unit::PX));
        aurora_set_present_margins(
            workshop || editing ? unsigned(leftWidth) : 0,
            unsigned(rightWidth), unsigned(topMargin), unsigned(bottomMargin));

        const float availableWidth =
            dimensions.x - (workshop || editing ? leftWidth : 0) - rightWidth;
        const float availableHeight = dimensions.y - topMargin - bottomMargin;
        const float gameWidth =
            std::min(availableHeight * 4.0f / 3.0f, availableWidth);
        const float gameHeight = gameWidth * 0.75f;
        const float gameLeft =
            workshop || editing
                ? leftWidth + (availableWidth - gameWidth) * 0.5f
                : (dimensions.x - gameWidth) * 0.5f;
        const float gameTop =
            topMargin + (availableHeight - gameHeight) * 0.5f;
        game_pointer_left = gameLeft;
        game_pointer_top = gameTop;
        game_pointer_width = gameWidth;
        game_pointer_height = gameHeight;
        nativeShellCssOriginal_Resize(int(gameLeft), int(gameTop),
                                      int(gameWidth), int(gameHeight));
    }

} // namespace

extern "C" bool nativeShell_Init(void)
{
    Rml::Context* context = aurora::rmlui::get_context();
    if (context == nullptr) {
        std::fprintf(stderr, "Native shell UI is unavailable.\n");
        return false;
    }
    aurora::rmlui::set_surface_overlay(true);

    const char* windowsDirectory =
        SDL_GetEnvironmentVariable(SDL_GetEnvironment(), "WINDIR");
    const std::string fontDirectory =
        std::string(windowsDirectory != nullptr ? windowsDirectory
                                                : "C:/Windows") +
        "/Fonts/";
    const bool regular =
        Rml::LoadFontFace(fontDirectory + "segoeui.ttf", false);
    const bool bold = Rml::LoadFontFace(fontDirectory + "segoeuib.ttf", false);
    Rml::LoadFontFace(fontDirectory + "segoeuii.ttf", false);
    Rml::LoadFontFace(fontDirectory + "segoeuiz.ttf", false);
    Rml::LoadFontFace(fontDirectory + "arial.ttf", false);
    Rml::LoadFontFace(fontDirectory + "arialbd.ttf", false);
    const bool display =
        Rml::LoadFontFace(fontDirectory + "arialbi.ttf", false);
    const bool codeFont =
        Rml::LoadFontFace(fontDirectory + "consola.ttf", false);
    if (!regular || !bold || !display) {
        std::fprintf(stderr, "Could not load the native shell fonts.\n");
        return false;
    }

    nativeWorkshopCode_Register();
    nativeShellWorkshopEditor_RegisterGraph();
    document = context->LoadDocument("ui/freak_shell.rml");
    if (document == nullptr) {
        std::fprintf(stderr, "Could not load the native shell UI.\n");
        return false;
    }
    if (!codeFont) {
        document->GetElementById("workshop-script-source")
            ->SetProperty("font-family", "Segoe UI");
    }
    document->Show();
    char guest_root_buffer[512];
    const char* guest_root = nativeHubCatalog_Root(guest_root_buffer);
    const char* service_endpoint = SDL_GetEnvironmentVariable(SDL_GetEnvironment(), "FREAK_MELEE_SERVICE_ENDPOINT");
    if (guest_root) nativeAccountTransport_InitGuest(guest_root, service_endpoint ? service_endpoint : NATIVE_SERVICE_ENDPOINT);
    const char* chat_endpoint = SDL_GetEnvironmentVariable(
        SDL_GetEnvironment(), "FREAK_MELEE_CHAT_ENDPOINT");
    nativeChat_Start(chat_endpoint ? chat_endpoint : NATIVE_SERVICE_ENDPOINT);
    nativeShellWorkshop_Init(document);
    nativeShellCssOriginal_Init(document);
    update_layout();
    return true;
}

extern "C" void nativeShell_Update(void)
{
    if (document != nullptr) {
        nativeChat_Update();
        update_layout();
        nativeShellWorkshop_Update();
        nativeShellCssOriginal_Update();
    }
}

extern "C" void nativeShell_Blur(void)
{
    if (document == nullptr) {
        return;
    }
    if (Rml::Element* focused = document->GetContext()->GetFocusElement()) {
        focused->Blur();
    }
}

extern "C" bool nativeShell_IsEditingText(void)
{
    if (document == nullptr) {
        return false;
    }
    const Rml::Element* focused = document->GetContext()->GetFocusElement();
    return focused != nullptr && focused->GetTagName() == "input";
}

extern "C" void nativeShell_Shutdown(void)
{
    nativeWorkshopAgent_Shutdown();
    nativeFeedback_Shutdown();
    nativeHub_Shutdown();
    nativeChat_Stop();
    if (document != nullptr) {
        nativeShellWorkshop_Close();
        nativeShellCssOriginal_Close();
        document->Close();
        document = nullptr;
    }
}

extern "C" void nativeShell_InspectElement(const char* id)
{
    Rml::ElementDocument* inspected = nativeFeedback_IsOpen()
        ? aurora::rmlui::get_context()->GetDocument("feedback-document") : document;
    if (!std::strcmp(id, "workshop-model-surface")) {
        nativeShellWorkshopModel_Inspect();
        return;
    }
    if (!std::strcmp(id, "workshop-preview")) {
        nativeHome_InspectWorkshop();
        return;
    }
    if (!std::strcmp(id,"quick-play") && nativePlayMenu_Inspect(id)) return;
    if (!std::strcmp(id, "css-selection")) {
        nativeCssInteraction_Inspect();
        return;
    }
    if (!std::strcmp(id, "css-cards")) {
        nativeCssCards_Inspect();
        return;
    }
    // Lobby chat remains active while gameplay hides the hub.
    if (!nativeFeedback_IsOpen() && (nativeHub_IsVisible() || !std::strcmp(id, "lobby-chat-state"))) {
        nativeHub_Inspect(id);
        return;
    }
    if (!nativeFeedback_IsOpen() && nativePlayMenu_Inspect(id)) {
        return;
    }
    if (!inspected) {
        return;
    }
    auto* element = inspected->GetElementById(id);
    if (!element) {
        std::fprintf(stderr, "[ui-inspect] %s: absent\n", id);
        return;
    }
    const auto origin = element->GetAbsoluteOffset(Rml::BoxArea::Content);
    const auto size = element->GetBox().GetSize(Rml::BoxArea::Content);
    const auto colour = element->GetComputedValues().color();
    std::fprintf(stderr,
                 "[ui-inspect] %s: tag=%s visible=%d origin=%.1f,%.1f "
                 "size=%.1f,%.1f scroll=%.1f,%.1f colour=%u,%u,%u,%u\n",
                 id, element->GetTagName().c_str(), element->IsVisible(true),
                 origin.x, origin.y, size.x, size.y, element->GetScrollLeft(),
                 element->GetScrollTop(), colour.red, colour.green,
                 colour.blue, colour.alpha);
    auto* control = dynamic_cast<Rml::ElementFormControl*>(element);
    if (auto* select = dynamic_cast<Rml::ElementFormControlSelect*>(element)) {
        for (int i = 0; i < select->GetNumOptions(); ++i) {
            const auto* option = select->GetOption(i);
            std::fprintf(stderr, "[ui-inspect-option] %s index=%d value=%s text=%s\n",
                id, i, option->GetAttribute<Rml::String>("value", "").c_str(),
                option->GetInnerRML().substr(0, 240).c_str());
        }
    }
    std::fprintf(
        stderr,
        "[ui-inspect-value] %s: disabled=%d checked=%d value=%s text=%s\n", id,
        element->HasAttribute("disabled"), element->HasAttribute("checked"),
        control ? control->GetValue().c_str() : "",
        element->GetInnerRML().substr(0, 240).c_str());
}

extern "C" void nativeShell_ClickAt(int x, int y, int button)
{
    if (!nativeFeedback_IsOpen() && nativeHub_IsVisible()) {
        nativeHub_ClickAt(x, y, button);
        return;
    }
    if (nativeShell_CssPointerEvent(x, y,
                                    button == 0 ? NativeCssPointer_Press
                                                : NativeCssPointer_Back))
    {
        if (button == 0) {
            nativeShell_CssPointerEvent(x, y, NativeCssPointer_Release);
        }
        return;
    }
    if (document == nullptr) {
        return;
    }
    Rml::Context* context = document->GetContext();
    context->ProcessMouseMove(x, y, 0);
    context->ProcessMouseButtonDown(button, 0);
    context->ProcessMouseButtonUp(button, 0);
}

extern "C" void nativeShell_PointerButtonAt(int x, int y, int button, bool down)
{
    if (!nativeFeedback_IsOpen() && nativeHub_IsVisible()) {
        SDL_Event event{};
        event.type = down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
        event.button.x = float(x); event.button.y = float(y);
        event.button.button = button == 0 ? SDL_BUTTON_LEFT : button == 1 ? SDL_BUTTON_RIGHT : SDL_BUTTON_MIDDLE;
        nativeHub_Event(&event);
        return;
    }
    const auto event = down ? (button == 0 ? NativeCssPointer_Press : NativeCssPointer_Back)
                            : NativeCssPointer_Release;
    if (nativeShell_CssPointerEvent(x, y, event)) return;
    if (!document) return;
    Rml::Context* context = document->GetContext();
    context->ProcessMouseMove(x, y, 0);
    if (down) context->ProcessMouseButtonDown(button, 0);
    else context->ProcessMouseButtonUp(button, 0);
}

extern "C" void nativeShell_DragAt(int start_x, int start_y, int end_x,
                                   int end_y, int button, unsigned modifiers)
{
    if (!nativeFeedback_IsOpen() && nativeHub_IsVisible()) {
        if (button == 0) {
            nativeShell_PointerButtonAt(start_x, start_y, button, true);
            nativeHub_MovePointer(end_x, end_y);
            nativeShell_PointerButtonAt(end_x, end_y, button, false);
        }
        return;
    }
    if (button == 0 && nativeShell_CssPointerEvent(start_x, start_y, NativeCssPointer_Press))
    {
        nativeShell_CssPointerEvent(end_x, end_y, NativeCssPointer_Move);
        nativeShell_CssPointerEvent(end_x, end_y, NativeCssPointer_Release);
        return;
    }
    if (document == nullptr) {
        return;
    }
    Rml::Context* context = document->GetContext();
    int flags = (modifiers & 1 ? Rml::Input::KM_SHIFT : 0) |
                (modifiers & 2 ? Rml::Input::KM_CTRL : 0) |
                (modifiers & 4 ? Rml::Input::KM_ALT : 0);
    context->ProcessMouseMove(start_x, start_y, flags);
    context->ProcessMouseButtonDown(button, flags);
    for (int step = 1; step <= 8; ++step) {
        context->ProcessMouseMove(start_x + (end_x - start_x) * step / 8,
                                  start_y + (end_y - start_y) * step / 8, flags);
    }
    context->ProcessMouseButtonUp(button, flags);
}

extern "C" void nativeShell_WheelAt(int x, int y, int steps)
{
    if (!document || (!nativeFeedback_IsOpen() && nativeHub_IsVisible())) return;
    Rml::Context* context = document->GetContext();
    context->ProcessMouseMove(x, y, 0);
    context->ProcessMouseWheel(Rml::Vector2f{0, float(-steps)}, 0);
}

extern "C" void nativeShell_EnterText(const char* text)
{
    if (!nativeFeedback_IsOpen() && (nativeHub_IsVisible() || nativeCssChat_Focused())) {
        nativeHub_EnterText(text);
        return;
    }
    if (document == nullptr || text == nullptr) {
        return;
    }
    document->GetContext()->ProcessTextInput(Rml::String(text));
}

extern "C" void nativeShell_MovePointer(int x, int y)
{
    if (!nativeFeedback_IsOpen() && nativeHub_IsVisible()) {
        nativeHub_MovePointer(x, y);
        return;
    }
    if (nativeShell_CssPointerEvent(x, y, NativeCssPointer_Move)) {
        return;
    }
    if (document) {
        document->GetContext()->ProcessMouseMove(x, y, 0);
    }
}
extern "C" bool nativeShell_CssPointerEvent(int x, int y,
                                            NativeCssPointerEvent event)
{
    if (nativeFeedback_IsOpen()) return false;
    if (game_pointer_width <= 0 || game_pointer_height <= 0) {
        return false;
    }
    float u = (x - game_pointer_left) / game_pointer_width,
          v = (y - game_pointer_top) / game_pointer_height;
    if (nativePlayMenu_Pointer(u, v, event)) {
        return true;
    }
    return nativeCssInteraction_Pointer(u, v, event);
}

extern "C" void nativeShell_PressKey(const char* name, unsigned modifiers)
{
    if (name && (!std::strcmp(name, "f2") || (nativeFeedback_IsOpen() &&
        (!std::strcmp(name, "enter") || !std::strcmp(name, "escape"))))) {
        const char* mapped = !std::strcmp(name, "enter") ? "Return" :
            !std::strcmp(name, "escape") ? "Escape" : name;
        SDL_Event event{}; event.type = SDL_EVENT_KEY_DOWN;
        event.key.scancode = SDL_GetScancodeFromName(mapped);
        event.key.key = SDL_GetKeyFromScancode(event.key.scancode, SDL_KMOD_NONE, false);
        event.key.mod = (modifiers & 1 ? SDL_KMOD_SHIFT : 0) |
            (modifiers & 2 ? SDL_KMOD_CTRL : 0) | (modifiers & 4 ? SDL_KMOD_ALT : 0);
        nativeFeedback_Event(&event);
        event.type = SDL_EVENT_KEY_UP; nativeFeedback_Event(&event);
        return;
    }
    if (!nativeFeedback_IsOpen() && (nativeHub_IsVisible() || nativeCssChat_Focused())) {
        nativeHub_PressKey(name, modifiers);
        return;
    }
    if (!document || !name) {
        return;
    }
    Rml::Input::KeyIdentifier key = Rml::Input::KI_UNKNOWN;
    if (name[0] && !name[1]) {
        char c = name[0];
        if (c >= 'a' && c <= 'z') {
            c -= 'a' - 'A';
        }
        if (c >= 'A' && c <= 'Z') {
            key = (Rml::Input::KeyIdentifier) (Rml::Input::KI_A + c - 'A');
        }
    }
    static const struct {
        const char* name;
        Rml::Input::KeyIdentifier key;
    } named[] = {
        { "escape", Rml::Input::KI_ESCAPE }, { "enter", Rml::Input::KI_RETURN },
        { "delete", Rml::Input::KI_DELETE }, { "backspace", Rml::Input::KI_BACK },
        { "left", Rml::Input::KI_LEFT }, { "right", Rml::Input::KI_RIGHT },
        { "up", Rml::Input::KI_UP }, { "down", Rml::Input::KI_DOWN },
        { "home", Rml::Input::KI_HOME }, { "end", Rml::Input::KI_END },
        { "pageup", Rml::Input::KI_PRIOR }, { "pagedown", Rml::Input::KI_NEXT },
        { "tab", Rml::Input::KI_TAB }, { "space", Rml::Input::KI_SPACE },
    };
    for (const auto& entry : named) {
        if (!std::strcmp(name, entry.name)) {
            key = entry.key;
            break;
        }
    }
    if (key == Rml::Input::KI_UNKNOWN) {
        return;
    }
    int flags = (modifiers & 1 ? Rml::Input::KM_SHIFT : 0) |
                (modifiers & 2 ? Rml::Input::KM_CTRL : 0) |
                (modifiers & 4 ? Rml::Input::KM_ALT : 0);
    document->GetContext()->ProcessKeyDown(key, flags);
    document->GetContext()->ProcessKeyUp(key, flags);
}

#endif
