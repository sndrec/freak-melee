#include "native_shell_chat.h"

#include <cstdint>
#include <string>

#include "native_chat.h"
#include <melee/gm/native_services.h>
#include <RmlUi/Core.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <SDL3/SDL_stdinc.h>

namespace {
    Rml::ElementDocument* document;
    uint64_t displayedRevision = UINT64_MAX;
    uint64_t lastMessage;
    NativeChatDelivery previousDelivery;
    bool collapsed;
    bool scrollToLatest;

    Rml::ElementFormControlInput* input(const char* id)
    {
        return static_cast<Rml::ElementFormControlInput*>(
            document->GetElementById(id));
    }

    void send_message()
    {
        const auto name = input("local-name")->GetValue();
        const auto text = input("chat-message")->GetValue();
        uint8_t packet[NATIVE_CHAT_PACKET_BYTES];
        const char* sender = name.empty() ? "Player" : name.c_str();
        if (!nativeChatWire_Post(1, sender, text.c_str(), packet,
                                 sizeof(packet)))
        {
            document->GetElementById("chat-delivery")
                ->SetInnerRML(
                    text.empty() ? ""
                                 : "Check your name and shorten the message.");
            return;
        }
        if (!nativeChat_Send(sender, text.c_str())) {
            document->GetElementById("chat-delivery")
                ->SetInnerRML("Chat is reconnecting. Try again shortly.");
        } else {
            nativeShellChat_Update();
        }
    }

    class ChatEvents final : public Rml::EventListener {
        void ProcessEvent(Rml::Event& event) override
        {
            if (event.GetCurrentElement()->HasAttribute("disabled")) {
                return;
            }
            const auto& id = event.GetCurrentElement()->GetId();
            if (event.GetType() == "keydown") {
                const int key = event.GetParameter<int>("key_identifier", 0);
                if (key == Rml::Input::KI_ESCAPE) {
                    event.GetCurrentElement()->Blur();
                    event.StopPropagation();
                } else if (key == Rml::Input::KI_RETURN) {
                    if (id == "chat-message") {
                        send_message();
                    } else {
                        event.GetCurrentElement()->Click();
                    }
                    event.StopPropagation();
                }
                return;
            }
            if (id == "chat-send") {
                send_message();
            } else if (id == "chat-toggle") {
                collapsed = !collapsed;
                document->GetElementById("global-chat")
                    ->SetClass("collapsed", collapsed);
                document->GetElementById("chat-toggle")
                    ->SetInnerRML(collapsed ? "+" : "−");
                if (!collapsed) {
                    scrollToLatest = true;
                }
            }
        }
    } events;
}

void nativeShellChat_Init(Rml::ElementDocument* shell)
{
    document = shell;
    displayedRevision = UINT64_MAX;
    lastMessage = 0;
    previousDelivery = NativeChatDelivery_None;
    collapsed = false;
    scrollToLatest = false;
    for (const char* id : { "chat-message", "chat-send", "chat-toggle" }) {
        document->GetElementById(id)->AddEventListener("click", &events);
        document->GetElementById(id)->AddEventListener("keydown", &events);
    }
    const char* endpoint = SDL_GetEnvironmentVariable(
        SDL_GetEnvironment(), "FREAK_MELEE_CHAT_ENDPOINT");
    nativeChat_Start(endpoint ? endpoint : NATIVE_SERVICE_ENDPOINT);
    nativeShellChat_Update();
}

void nativeShellChat_Update()
{
    nativeChat_Update();
    const NativeChatView* state = nativeChat_View();
    auto* history = document->GetElementById("chat-history");
    if (scrollToLatest) {
        history->SetScrollTop(history->GetScrollHeight());
        scrollToLatest = false;
    }
    if (displayedRevision == state->revision) {
        return;
    }
    displayedRevision = state->revision;
    const char* status =
        state->state == NativeChat_Connecting     ? "Connecting..."
        : state->state == NativeChat_Reconnecting ? "Reconnecting..."
                                                  : "Offline";
    document->GetElementById("chat-state")
        ->SetInnerRML(state->state == NativeChat_Online
                          ? std::to_string(state->online) + " online"
                          : status);
    uint64_t newest = state->count ? state->messages[state->count - 1].id : 0;
    if (newest != lastMessage) {
        bool atEnd = history->GetScrollHeight() - history->GetClientHeight() -
                         history->GetScrollTop() <
                     20;
        std::string rml;
        for (uint32_t i = 0; i < state->count; ++i) {
            const NativeChatMessage& message = state->messages[i];
            rml += "<div class=\"chat-entry\"><div class=\"chat-name\">";
            rml += Rml::StringUtilities::EncodeRml(message.name);
            rml += "</div><div class=\"chat-text\">";
            rml += Rml::StringUtilities::EncodeRml(message.text);
            rml += "</div></div>";
        }
        history->SetInnerRML(rml);
        scrollToLatest = atEnd || lastMessage == 0 ||
                         previousDelivery != NativeChatDelivery_None;
        lastMessage = newest;
    }
    if (previousDelivery != NativeChatDelivery_None &&
        state->delivery == NativeChatDelivery_None &&
        input("chat-message")->GetValue() == state->pending.text)
    {
        input("chat-message")->SetValue("");
    }
    previousDelivery = state->delivery;
    bool sending = state->delivery == NativeChatDelivery_Sending;
    input("chat-message")->SetDisabled(sending);
    auto* send = document->GetElementById("chat-send");
    if (state->state != NativeChat_Online || sending) {
        send->SetAttribute("disabled", true);
    } else {
        send->RemoveAttribute("disabled");
    }
    send->SetInnerRML(
        state->delivery == NativeChatDelivery_Unconfirmed ? "Retry" : "Send");
    document->GetElementById("chat-delivery")
        ->SetInnerRML(sending ? "Sending..."
                      : state->delivery == NativeChatDelivery_Unconfirmed
                          ? "Delivery unconfirmed."
                          : "");
}

void nativeShellChat_Shutdown()
{
    nativeChat_Stop();
    document = nullptr;
}
