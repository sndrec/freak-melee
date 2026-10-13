#include "native_shell.h"
#include "native_shell_chat.h"
#include "native_shell_collection.h"
#include "native_shell_css_original.h"
#include "native_shell_costumes.h"
#include "native_shell_profiles.h"
#include "native_shell_rules.h"
#include "native_shell_workshop.h"
#include "native_shell_workshop_editor.h"
#include "native_shell_workshop_code.h"

#ifdef MELEE_NATIVE

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include <aurora/aurora.h>
#include <aurora/rmlui.hpp>
#include <melee/gm/native_online_session.h>
#include <melee/gm/native_home.h>
#include <melee/gm/native_profile_input.h>
#include <melee/mn/native_online_menu.h>
#include <RmlUi/Core.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <SDL3/SDL_clipboard.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>

namespace {

    Rml::ElementDocument* document;
    NativeLobbySessionState displayedState =
        NativeLobbySessionState_Disconnected;
    uint64_t displayedRevision = UINT64_MAX;
    char displayedRoomCode[16];
    char controllerNames[4][256];
    char profileNames[4][NATIVE_PROFILE_NAME_BYTES + 1];
    int controllerIndices[4] = { -2, -2, -2, -2 };
    uint64_t nextControllerUpdate;
    uint64_t copyFeedbackUntil;
    Rml::Vector2i displayedDimensions;
    float displayedDensity;
    bool displayedCssEditing;
    bool displayedWorkshop;
    bool displayedWorkshopEditor;
    bool displayedWorkshopScript;
    bool displayedWorkshopAnimation;
    bool displayedWorkshopModel;
    bool displayedWorkshopGraphFocused;
    float displayedWorkshopTimelineFraction;
    bool displayedContentVisible;
    NativeNetplayContentDirection displayedContentDirection;
    uint8_t displayedContentIndex;
    uint8_t displayedContentCount;
    unsigned displayedContentPercent;
    bool displayedContentSizeKnown;
    uint8_t localPlayers = 1;
    int displayedControls = -1;

    const char* const roomControls[] = {
        "create-room",     "join-code", "join-room",  "local-name",
        "local-1",         "local-2",   "local-3",    "local-4",
        "start-match", "copy-code", "leave-room", "rotation-policy",
    };

    void set_text(const char* id, const std::string& value);
    void room_action(NativeOnlineMenuAction action);

    class ShellEvents final : public Rml::EventListener {
        void ProcessEvent(Rml::Event& event) override
        {
            if (event.GetCurrentElement()->HasAttribute("disabled")) {
                return;
            }
            const Rml::String& id = event.GetCurrentElement()->GetId();
            if (event.GetType() == "keydown") {
                const int key = event.GetParameter<int>("key_identifier", 0);
                if (key == Rml::Input::KI_ESCAPE) {
                    nativeShell_BlurRoom();
                    event.StopPropagation();
                } else if (key == Rml::Input::KI_RETURN ||
                           (key == Rml::Input::KI_SPACE &&
                            event.GetCurrentElement()->GetTagName() ==
                                "button"))
                {
                    if (id == "join-code") {
                        room_action(NativeOnlineMenuAction_Join);
                    } else if (id == "local-name") {
                        document->GetElementById("create-room")->Focus(true);
                    } else {
                        event.GetCurrentElement()->Click();
                    }
                    event.StopPropagation();
                }
                return;
            }
            if (id == "copy-code") {
                const char* code = nativeOnlineSession_RoomCode();
                if (code != nullptr && SDL_SetClipboardText(code)) {
                    set_text("copy-code", "Copied!");
                    copyFeedbackUntil = SDL_GetTicks() + 2000;
                }
            } else if (id == "create-room") {
                room_action(NativeOnlineMenuAction_Create);
            } else if (id == "join-room") {
                room_action(NativeOnlineMenuAction_Join);
            } else if (id == "leave-room") {
                room_action(NativeOnlineMenuAction_Leave);
            } else if (id == "start-match") {
                room_action(NativeOnlineMenuAction_StartMatch);
            } else if (id == "rotation-policy") {
                const NativeSignalingPresence* presence =
                    nativeOnlineSession_Presence();
                if (presence == nullptr || !nativeOnlineSession_SetRotation(
                    static_cast<NativeRoomRotationPolicy>(
                        (presence->rotation_policy + 1) %
                        (NativeRoomRotation_WinnerOut + 1)))) {
                    nativeShell_ShowRoomError("Rotation is unavailable right now.");
                }
            } else if (id == "room-players") {
                const Rml::Element* target = event.GetTargetElement();
                while (target != nullptr && target->GetId().empty()) {
                    target = target->GetParentNode();
                }
                unsigned peerIndex, seat;
                char trailing;
                if (target != nullptr &&
                    std::sscanf(target->GetId().c_str(), "seat-%u-%u%c",
                                &peerIndex, &seat, &trailing) == 2) {
                    const NativeSignalingPresence* presence =
                        nativeOnlineSession_Presence();
                    if (presence != nullptr &&
                        peerIndex < presence->participant_count &&
                        seat < presence->participants[peerIndex].local_players) {
                        const NativeSignalingParticipant& participant =
                            presence->participants[peerIndex];
                        const uint8_t bit = uint8_t(1U << seat);
                        const NativeRoomSeatState next =
                            (participant.active_mask & bit) != 0
                                ? NativeRoomSeatState_Waiting
                                : (participant.waiting_mask & bit) != 0
                                      ? NativeRoomSeatState_Spectating
                                      : NativeRoomSeatState_Fighting;
                        if (nativeOnlineSession_ChooseSeat(
                                participant.peer_id, uint8_t(seat), next)) {
                            nativeHome_CancelStartMatch();
                        } else {
                            nativeShell_ShowRoomError(
                                "Seat unavailable. A room can have four fighters.");
                        }
                    }
                }
            } else if (id == "room-tab") {
                nativeShellCollection_Close();
                nativeShellProfiles_Close();
                nativeShellRules_Close();
                nativeOnlineMenu_Focus();
            } else if (id.size() == 7 && id.compare(0, 6, "local-") == 0) {
                localPlayers = uint8_t(id[6] - '0');
                for (int i = 1; i <= 4; ++i) {
                    document->GetElementById("local-" + std::to_string(i))
                        ->SetClass("selected", i == localPlayers);
                }
            }
        }
    } shellEvents;

    const char* state_label(NativeLobbySessionState state)
    {
        switch (state) {
        case NativeLobbySessionState_Connecting:
            return "Connecting...";
        case NativeLobbySessionState_Waiting:
            return "Room open";
        case NativeLobbySessionState_Starting:
            return "Starting match";
        case NativeLobbySessionState_Ready:
            return "In a match";
        case NativeLobbySessionState_Returning:
            return "Returning to room";
        case NativeLobbySessionState_Failed:
            return "Connection failed";
        default:
            return "Offline";
        }
    }

    std::string escape_rml(const char* text)
    {
        std::string result;
        if (text == nullptr) {
            return result;
        }
        for (; *text != '\0'; ++text) {
            switch (*text) {
            case '&':
                result += "&amp;";
                break;
            case '<':
                result += "&lt;";
                break;
            case '>':
                result += "&gt;";
                break;
            case '"':
                result += "&quot;";
                break;
            default:
                result += *text;
                break;
            }
        }
        return result;
    }

    void set_text(const char* id, const std::string& value)
    {
        if (Rml::Element* element = document->GetElementById(id)) {
            element->SetInnerRML(value);
        }
    }

    void room_action(NativeOnlineMenuAction action)
    {
        auto* nameInput = static_cast<Rml::ElementFormControlInput*>(
            document->GetElementById("local-name"));
        auto* codeInput = static_cast<Rml::ElementFormControlInput*>(
            document->GetElementById("join-code"));
        const Rml::String name = nameInput->GetValue();
        Rml::String code = codeInput->GetValue();
        if (name.size() > NATIVE_SIGNALING_MAX_NAME_BYTES) {
            nativeShell_ShowRoomError("That name is too long.");
            return;
        }
        if (action == NativeOnlineMenuAction_Join) {
            Rml::String normalized;
            for (char c : code) {
                if (c >= 'a' && c <= 'z') {
                    c -= 'a' - 'A';
                }
                if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                    continue;
                }
                if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))) {
                    nativeShell_ShowRoomError(
                        "Room codes use letters and numbers.");
                    return;
                }
                normalized += c;
            }
            if (normalized.size() < 6 || normalized.size() > 12) {
                nativeShell_ShowRoomError("Enter a 6-12 character room code.");
                return;
            }
            code = normalized;
            codeInput->SetValue(code);
        }
        nativeShell_ShowRoomError("");
        if (!nativeOnlineMenu_Request(action,
                                      name.empty() ? "Player" : name.c_str(),
                                      code.c_str(), localPlayers))
        {
            nativeShell_ShowRoomError("Return home to manage your room.");
        } else {
            nativeShell_BlurRoom();
        }
    }

    void update_controls()
    {
        const bool available = nativeOnlineMenu_IsAvailable();
        const bool canStart = nativeHome_CanStartMatch();
        const bool offline = nativeOnlineSession_State() ==
                             NativeLobbySessionState_Disconnected;
        const bool homeSelection = nativeHome_IsSelection();
        const bool localRosterMatches = offline ||
                                        nativeHome_LocalRosterMatchesRoom();
        const bool needsOpposingTeams = nativeHome_TeamsNeedOpponents();
        const int controlState =
            int(available) | (int(canStart) << 1) | (int(offline) << 2) |
            (int(nativeHome_IsPractice()) << 3) |
            (int(nativeOnlineSession_IsOwner()) << 4) |
            (int(localRosterMatches) << 5) | (int(homeSelection) << 6) |
            (int(needsOpposingTeams) << 7);
        if (controlState == displayedControls) {
            return;
        }
        displayedControls = controlState;
        for (const char* id :
             { "create-room", "join-room", "leave-room", "start-match" })
        {
            const bool enabled =
                available &&
                (std::strcmp(id, "start-match") != 0 || canStart);
            Rml::Element* button = document->GetElementById(id);
            if (enabled) {
                button->RemoveAttribute("disabled");
            } else {
                button->SetAttribute("disabled", true);
            }
        }
        document->GetElementById("room-unavailable")
            ->SetProperty("display", available ? "none" : "block");
        document->GetElementById("start-match")
            ->SetProperty("display", available ? "block" : "none");
        document->GetElementById("start-hint")
            ->SetProperty("display", available && canStart ? "none" : "block");
        if (!available) {
            set_text("start-hint", "Match in progress");
        } else if (nativeHome_IsSelection()) {
            set_text("start-hint", "START to practice and ready up");
        } else if (!localRosterMatches) {
            const int expected = nativeOnlineSession_LocalPlayers();
            set_text("start-hint", "Need " + std::to_string(expected) +
                                       " fighters. Exit practice to change.");
        } else if (needsOpposingTeams) {
            set_text("start-hint", "Assign fighters to different teams.");
        } else if (!offline && !nativeOnlineSession_IsOwner()) {
            set_text("start-hint", "Waiting for the host");
        } else if (offline) {
            set_text("start-hint", "Select at least 2 fighters");
        } else {
            set_text("start-hint", "Waiting for ready players");
        }
    }

    void update_layout()
    {
        const Rml::Context* context = document->GetContext();
        const Rml::Vector2i dimensions = context->GetDimensions();
        const float density = context->GetDensityIndependentPixelRatio();
        const bool editing = nativeShellCssOriginal_IsEditing();
        const bool workshopOpen = nativeShellWorkshop_IsOpen();
        const bool workshop = workshopOpen &&
            !nativeShellWorkshop_IsTesting();
        const bool workshopEditor = workshop &&
            nativeShellWorkshopEditor_IsOpen();
        const bool workshopScript = workshop &&
            nativeShellWorkshopEditor_IsScriptMode();
        const bool workshopAnimation = workshop &&
            nativeShellWorkshopEditor_IsAnimationMode();
        const bool workshopModel = workshop &&
            nativeShellWorkshopEditor_IsModelMode();
        const bool workshopGraphFocused = workshop &&
            nativeShellWorkshopEditor_IsGraphFocused();
        const float workshopTimelineFraction = workshopEditor
            ? nativeShellWorkshopEditor_TimelineFraction() : 0.0f;
        if (dimensions == displayedDimensions && density == displayedDensity &&
            editing == displayedCssEditing &&
            workshop == displayedWorkshop &&
            workshopEditor == displayedWorkshopEditor &&
            workshopScript == displayedWorkshopScript &&
            workshopAnimation == displayedWorkshopAnimation &&
            workshopModel == displayedWorkshopModel &&
            workshopGraphFocused == displayedWorkshopGraphFocused &&
            workshopTimelineFraction == displayedWorkshopTimelineFraction) {
            return;
        }
        displayedDimensions = dimensions;
        displayedDensity = density;
        displayedCssEditing = editing;
        displayedWorkshop = workshop;
        displayedWorkshopEditor = workshopEditor;
        displayedWorkshopScript = workshopScript;
        displayedWorkshopAnimation = workshopAnimation;
        displayedWorkshopModel = workshopModel;
        displayedWorkshopGraphFocused = workshopGraphFocused;
        displayedWorkshopTimelineFraction = workshopTimelineFraction;
        // Melee keeps its original aspect ratio. Never let a narrow window put
        // native panels over the game: hide them if the letterbox is too
        // small.
        const int railWidth = (dimensions.x - dimensions.y * 4 / 3) / 2;
        const bool visible = railWidth >= 140 * density;
        const int workshopWidth = std::min(int(330 * density),
                                           std::max(0, (dimensions.x - 200) / 2));
        const int editorLeftWidth = std::clamp(int(dimensions.x * 0.135f),
                                               int(160 * density),
                                               int(230 * density));
        const int editorRightWidth = workshopScript
            ? std::clamp(int(dimensions.x * 0.22f),int(290*density),int(360*density))
            : std::clamp(int(dimensions.x * 0.16f),int(210*density),int(265*density));
        const int leftWidth = workshopModel ? int(232 * density)
            : workshopScript
            ? dimensions.x - editorRightWidth
            : workshopEditor ? editorLeftWidth
            : workshop ? workshopWidth
            : editing ? std::min(dimensions.x - 1,
                                 std::max(railWidth * 2, int(320 * density)))
                      : railWidth;
        const int rightWidth = workshopModel ? int(267 * density)
            : workshopEditor
            ? (workshopScript ? 0 : editorRightWidth)
            : workshop ? workshopWidth : 0;
        const int topMargin = workshopModel ? int(196 * density)
            : workshopEditor ? int(114 * density) : 0;
        const int workshopFooterHeight = int(30 * density);
        const int bottomMargin = workshopModel ? int(182 * density)
            : workshopEditor
            ? (workshopScript
                ? std::max(0, dimensions.y - topMargin -
                                   int(174 * density))
                : std::min(std::max(0, dimensions.y - topMargin -
                                      int(120 * density)),
                    std::max(int(dimensions.y * workshopTimelineFraction),
                             int((workshopAnimation ? 220 : 170) * density))))
            : 0;
        Rml::Element* leftRail = document->GetElementById("left-rail");
        leftRail->SetProperty("display", visible && !workshopOpen ? "flex" : "none");
        leftRail->SetProperty(Rml::PropertyId::Width,
                              Rml::Property(float(leftWidth), Rml::Unit::PX));
        leftRail->SetClass("css-editing", editing);
        Rml::Element* rightRail = document->GetElementById("right-rail");
        rightRail->SetProperty("display", visible && !editing && !workshopOpen
                                             ? "flex" : "none");
        rightRail->SetProperty(Rml::PropertyId::Width,
                               Rml::Property(float(railWidth), Rml::Unit::PX));
        for (const char* id : { "workshop-library", "workshop-inspector" }) {
            document->GetElementById(id)->SetProperty(
                Rml::PropertyId::Width,
                Rml::Property(float(std::strcmp(id, "workshop-library") == 0 && !workshopEditor ? dimensions.x : workshopWidth), Rml::Unit::PX));
        }
        for (const char* id : { "workshop-editor-browser" }) {
            document->GetElementById(id)->SetProperty(
                Rml::PropertyId::Width,
                Rml::Property(float(editorLeftWidth), Rml::Unit::PX));
        }
        for (const char* id : { "workshop-editor-browser",
                                "workshop-editor-inspector" }) {
            document->GetElementById(id)->SetProperty(
                Rml::PropertyId::Bottom,
                Rml::Property(float(bottomMargin), Rml::Unit::PX));
        }
        document->GetElementById("workshop-editor-inspector")->SetProperty(
            Rml::PropertyId::Width,
            Rml::Property(float(editorRightWidth), Rml::Unit::PX));
        document->GetElementById("workshop-script-inspector")->SetProperty(
            Rml::PropertyId::Width,
            Rml::Property(float(editorRightWidth), Rml::Unit::PX));
        for (const char* id : { "workshop-action-viewport",
                                "workshop-animation-viewport" }) {
            Rml::Element* viewport = document->GetElementById(id);
            viewport->SetProperty(Rml::PropertyId::Left,
                Rml::Property(float(editorLeftWidth), Rml::Unit::PX));
            viewport->SetProperty(Rml::PropertyId::Right,
                Rml::Property(float(editorRightWidth), Rml::Unit::PX));
            viewport->SetProperty(Rml::PropertyId::Bottom,
                Rml::Property(float(bottomMargin), Rml::Unit::PX));
        }
        for (const char* id : { "workshop-editor-timeline",
                                "workshop-animation-pane" }) {
            Rml::Element* timeline = document->GetElementById(id);
            timeline->SetProperty(
                Rml::PropertyId::Height,
                Rml::Property(float(workshopGraphFocused &&
                    std::strcmp(id, "workshop-animation-pane") == 0
                    ? dimensions.y - topMargin - workshopFooterHeight
                    : std::max(0, bottomMargin - workshopFooterHeight)),
                    Rml::Unit::PX));
            timeline->SetProperty(Rml::PropertyId::Bottom,
                Rml::Property(float(workshopFooterHeight), Rml::Unit::PX));
            if (std::strcmp(id, "workshop-animation-pane") == 0)
                timeline->SetProperty("top", workshopGraphFocused
                    ? std::to_string(topMargin) + "px" : "auto");
        }
        document->GetElementById("workshop-script-pane")->SetProperty(
            Rml::PropertyId::Left, Rml::Property(0.0f, Rml::Unit::PX));
        document->GetElementById("workshop-script-pane")->SetProperty(
            Rml::PropertyId::Width,
            Rml::Property(float(leftWidth), Rml::Unit::PX));
        document->GetElementById("workshop-script-pane")->SetProperty(
            Rml::PropertyId::Bottom,
            Rml::Property(float(workshopFooterHeight), Rml::Unit::PX));
        document->GetElementById("workshop-script-inspector")->SetProperty(
            Rml::PropertyId::Bottom,
            Rml::Property(float(workshopFooterHeight), Rml::Unit::PX));
        aurora_set_present_margins(
            workshop || editing ? unsigned(leftWidth) : 0,
            unsigned(rightWidth), unsigned(topMargin),
            unsigned(bottomMargin));

        const float availableWidth = dimensions.x -
                                     (workshop || editing ? leftWidth : 0) -
                                     rightWidth;
        const float availableHeight = dimensions.y - topMargin - bottomMargin;
        const float gameWidth = std::min(availableHeight * 4.0f / 3.0f,
                                         availableWidth);
        const float gameHeight = gameWidth * 0.75f;
        const float gameLeft = workshop || editing
            ? leftWidth + (availableWidth - gameWidth) * 0.5f
            : (dimensions.x - gameWidth) * 0.5f;
        const float gameTop = topMargin +
                              (availableHeight - gameHeight) * 0.5f;
        nativeShellCssOriginal_Resize(int(gameLeft), int(gameTop),
                                      int(gameWidth), int(gameHeight));
    }

    void update_controllers()
    {
        const uint64_t now = SDL_GetTicks();
        if (now < nextControllerUpdate) {
            return;
        }
        nextControllerUpdate = now + 500;
        bool changed = false;
        int connected = 0;
        for (uint32_t port = 0; port < 4; ++port) {
            const int index = PADGetIndexForPort(port);
            const char* hardware = PADGetName(port);
            const NativeProfile* profile = nativeProfileInput_Assigned(port);
            const char* name =
                profile != nullptr ? profile->name : "Default controls";
            hardware = hardware != nullptr ? hardware : "";
            connected += index >= 0;
            if (controllerIndices[port] != index ||
                std::strcmp(controllerNames[port], hardware) != 0 ||
                std::strcmp(profileNames[port], name) != 0)
            {
                controllerIndices[port] = index;
                std::snprintf(controllerNames[port],
                              sizeof(controllerNames[port]), "%s", hardware);
                std::snprintf(profileNames[port], sizeof(profileNames[port]),
                              "%s", name);
                changed = true;
            }
        }
        if (!changed) {
            return;
        }
        std::string rows;
        for (int port = 0; port < 4; ++port) {
            const bool active = controllerIndices[port] >= 0;
            rows += active ? "<div class=\"controller-row connected\">"
                           : "<div class=\"controller-row empty\">";
            rows +=
                "<span class=\"port\">" + std::to_string(port + 1) + "</span>";
            rows += "<div class=\"controller-copy\"><div "
                    "class=\"controller-name\">";
            rows +=
                active ? escape_rml(controllerNames[port]) : "No controller";
            rows += "</div>";
            if (active) {
                rows += "<div class=\"controller-detail\">";
                rows += escape_rml(profileNames[port]);
                rows += "</div>";
            }
            rows += "</div></div>";
        }
        set_text("local-controllers", rows);
        set_text("controller-count", std::to_string(connected) + " / 4");
    }

    void update_content_progress()
    {
        NativeNetplayContentProgress progress;
        const bool visible =
            nativeOnlineSession_ContentProgress(&progress);
        Rml::Element* label = document->GetElementById("content-progress");
        if (!visible) {
            if (displayedContentVisible) {
                label->SetProperty("display", "none");
                displayedContentVisible = false;
            }
            return;
        }
        const bool sizeKnown = progress.bytes_total != 0;
        const unsigned percent = sizeKnown
            ? unsigned(uint64_t(progress.bytes_done) * 100 /
                       progress.bytes_total)
            : 0;
        if (displayedContentVisible &&
            displayedContentDirection == progress.direction &&
            displayedContentIndex == progress.package_index &&
            displayedContentCount == progress.package_count &&
            displayedContentPercent == percent &&
            displayedContentSizeKnown == sizeKnown) {
            return;
        }
        displayedContentVisible = true;
        displayedContentDirection = progress.direction;
        displayedContentIndex = progress.package_index;
        displayedContentCount = progress.package_count;
        displayedContentPercent = percent;
        displayedContentSizeKnown = sizeKnown;
        std::string message;
        if (progress.direction == NativeNetplayContent_Receiving) {
            message = sizeKnown ? "Receiving package " :
                                  "Waiting for package ";
            message += std::to_string(progress.package_index) + "/" +
                       std::to_string(progress.package_count);
        } else {
            message = "Sending match package";
        }
        if (sizeKnown) {
            message += ": " + std::to_string(percent) + "%";
        }
        set_text("content-progress", message);
        label->SetProperty("display", "block");
    }

    void update_room(void)
    {
        const NativeLobbySessionState state = nativeOnlineSession_State();
        const NativeSignalingPresence* presence =
            nativeOnlineSession_Presence();
        const uint64_t revision = presence != nullptr ? presence->revision : 0;
        const char* code = nativeOnlineSession_RoomCode();
        code = code != nullptr ? code : "";
        if (state == displayedState && revision == displayedRevision &&
            std::strcmp(code, displayedRoomCode) == 0)
        {
            return;
        }
        displayedState = state;
        displayedRevision = revision;
        std::snprintf(displayedRoomCode, sizeof(displayedRoomCode), "%s",
                      code);

        set_text("room-state", state_label(state));
        const bool offline = state == NativeLobbySessionState_Disconnected;
        document->GetElementById("offline-room")
            ->SetProperty("display", offline ? "block" : "none");
        document->GetElementById("connected-room")
            ->SetProperty("display", offline ? "none" : "block");
        document->GetElementById("room-dot")
            ->SetClass("online",
                       !offline && state != NativeLobbySessionState_Failed);
        document->GetElementById("room-dot")
            ->SetClass("failed", state == NativeLobbySessionState_Failed);
        set_text("room-code", escape_rml(code));
        set_text("copy-code", "Copy room code");
        const char* failure = nativeOnlineSession_Failure();
        set_text("room-failure",
                 failure != nullptr ? escape_rml(failure) : "");
        document->GetElementById("room-failure")
            ->SetProperty("display", failure != nullptr ? "block" : "none");
        set_text("room-count", std::to_string(presence != nullptr
                                                  ? presence->participant_count
                                                  : 0));
        const bool canSetRotation = presence != nullptr &&
            state == NativeLobbySessionState_Waiting &&
            presence->match_size == 0 && nativeOnlineSession_IsOwner();
        const char* rotation = presence == nullptr ||
                                   presence->rotation_policy == NativeRoomRotation_Manual
                                   ? "Manual"
                               : presence->rotation_policy ==
                                         NativeRoomRotation_WinnerStays
                                   ? "Winner stays" : "Winner out";
        set_text("rotation-policy", std::string(rotation) +
                  (canSetRotation ? "  ›" : ""));
        Rml::Element* rotationButton = document->GetElementById("rotation-policy");
        if (canSetRotation) rotationButton->RemoveAttribute("disabled");
        else rotationButton->SetAttribute("disabled", "");

        std::string players;
        if (presence != nullptr) {
            for (uint32_t i = 0; i < presence->participant_count; ++i) {
                const NativeSignalingParticipant& participant =
                    presence->participants[i];
                const bool owner = std::strcmp(participant.peer_id,
                                               presence->owner_peer_id) == 0;
                players +=
                    "<div class=\"player-row\"><div class=\"player-name\">";
                players += escape_rml(
                    participant.name[0] != '\0' ? participant.name : "Player");
                players += "</div><div class=\"player-role\">";
                if (owner) {
                    players += "<span class=\"owner-tag\">Host</span> / ";
                }
                unsigned activeCount = 0;
                unsigned waitingCount = 0;
                for (unsigned seat = 0; seat < participant.local_players; ++seat) {
                    activeCount += (participant.active_mask >> seat) & 1U;
                    waitingCount += (participant.waiting_mask >> seat) & 1U;
                }
                if (activeCount != 0) {
                    players += std::to_string(activeCount);
                    players += activeCount == 1 ? " fighter" : " fighters";
                }
                if (waitingCount != 0) {
                    if (activeCount != 0) players += " / ";
                    players += std::to_string(waitingCount) + " waiting";
                }
                if (activeCount == 0 && waitingCount == 0) players += "Spectating";
                NativeLobbySelection selection{};
                if (participant.active_mask != 0 &&
                    nativeLobbySelection_Decode(&selection, participant.selection,
                                                participant.selection_size)) {
                    players += selection.confirmed ? " / Ready" : " / Selecting";
                }
                players += "</div><div class=\"seat-list\">";
                const bool canChoose =
                    state == NativeLobbySessionState_Waiting &&
                    presence->match_size == 0 &&
                    (nativeOnlineSession_IsOwner() ||
                     std::strcmp(participant.peer_id,
                                 nativeOnlineSession_LocalPeerId()) == 0);
                for (unsigned seat = 0; seat < participant.local_players; ++seat) {
                    const unsigned bit = 1U << seat;
                    const char* status =
                        (participant.active_mask & bit) != 0 ? "Fighting"
                        : (participant.waiting_mask & bit) != 0 ? "Waiting"
                                                                : "Spectating";
                    std::string label =
                        "P" + std::to_string(seat + 1) + "  " + status;
                    if ((participant.waiting_mask & bit) != 0 &&
                        participant.waiting_ranks[seat] != 0) {
                        label += " #" +
                                 std::to_string(participant.waiting_ranks[seat]);
                    }
                    if (canChoose) {
                        players += "<button class=\"seat-action\" id=\"seat-" +
                                   std::to_string(i) + "-" +
                                   std::to_string(seat) + "\">" + label +
                                   " &#8250;</button>";
                    } else {
                        players += "<span class=\"seat-status\">" + label +
                                   "</span>";
                    }
                }
                players += "</div></div>";
            }
        }
        if (players.empty()) {
            players = "<div class=\"muted\">Waiting for the room...</div>";
        }
        set_text("room-players", players);
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
    nativeShellChat_Init(document);
    nativeShellCollection_Init(document);
    nativeShellWorkshop_Init(document);
    nativeShellCssOriginal_Init(document);
    nativeShellCostumes_Init(document);
    nativeShellProfiles_Init(document);
    nativeShellRules_Init(document);
    for (const char* id : roomControls) {
        document->GetElementById(id)->AddEventListener("click", &shellEvents);
        document->GetElementById(id)->AddEventListener("keydown",
                                                       &shellEvents);
    }
    document->GetElementById("room-players")
        ->AddEventListener("click", &shellEvents);
    document->GetElementById("room-tab")
        ->AddEventListener("click", &shellEvents);
    document->GetElementById("room-tab")
        ->AddEventListener("keydown", &shellEvents);
    displayedRevision = UINT64_MAX;
    update_layout();
    update_room();
    return true;
}

extern "C" void nativeShell_Update(void)
{
    if (document != nullptr) {
        update_layout();
        update_controllers();
        update_room();
        update_content_progress();
        update_controls();
        nativeShellChat_Update();
        nativeShellCollection_Update();
        nativeShellWorkshop_Update();
        nativeShellCssOriginal_Update();
        nativeShellCostumes_Update();
        nativeShellProfiles_Update();
        nativeShellRules_Update();
        if (copyFeedbackUntil != 0 && SDL_GetTicks() >= copyFeedbackUntil) {
            set_text("copy-code", "Copy room code");
            copyFeedbackUntil = 0;
        }
    }
}

extern "C" void nativeShell_FocusRoom(void)
{
    if (document == nullptr) {
        return;
    }
    const char* id =
        nativeOnlineSession_State() == NativeLobbySessionState_Disconnected
            ? "create-room"
            : "copy-code";
    document->GetElementById(id)->Focus(true);
}

extern "C" void nativeShell_BlurRoom(void)
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

extern "C" void nativeShell_NavigateRoom(int direction, bool activate)
{
    if (document == nullptr) {
        return;
    }
    Rml::Element* focused = document->GetContext()->GetFocusElement();
    if (activate) {
        if (focused != nullptr && focused->GetTagName() == "button" &&
            focused->IsVisible(true) && !focused->HasAttribute("disabled"))
        {
            focused->Click();
        } else {
            nativeShell_FocusRoom();
        }
        return;
    }
    Rml::Element* controls[sizeof(roomControls) / sizeof(roomControls[0]) +
                           NATIVE_SIGNALING_MAX_PARTICIPANTS * 4];
    int count = 0;
    for (const char* id : roomControls) {
        controls[count++] = document->GetElementById(id);
        if (std::strcmp(id, "copy-code") != 0) continue;
        const NativeSignalingPresence* presence =
            nativeOnlineSession_Presence();
        if (presence == nullptr) continue;
        for (uint32_t peer = 0; peer < presence->participant_count; ++peer) {
            for (uint32_t seat = 0;
                 seat < presence->participants[peer].local_players; ++seat) {
                char seatId[24];
                std::snprintf(seatId, sizeof(seatId), "seat-%u-%u", peer,
                              seat);
                if (Rml::Element* button = document->GetElementById(seatId)) {
                    controls[count++] = button;
                }
            }
        }
    }
    int index = direction > 0 ? -1 : 0;
    for (int i = 0; i < count; ++i) {
        if (focused == controls[i]) {
            index = i;
        }
    }
    for (int i = 0; i < count; ++i) {
        index = (index + direction + count) % count;
        Rml::Element* element = controls[index];
        if (element->GetTagName() == "button" && element->IsVisible(true) &&
            !element->HasAttribute("disabled"))
        {
            element->Focus(true);
            break;
        }
    }
}

extern "C" void nativeShell_ShowRoomError(const char* message)
{
    if (document == nullptr) {
        return;
    }
    set_text("room-failure", escape_rml(message));
    document->GetElementById("room-failure")
        ->SetProperty("display", message != nullptr && message[0] != '\0'
                                     ? "block"
                                     : "none");
}

extern "C" void nativeShell_Shutdown(void)
{
    if (document != nullptr) {
        nativeShellProfiles_Close();
        nativeShellRules_Close();
        nativeShellWorkshop_Close();
        nativeShellCollection_Close();
        nativeShellCssOriginal_Close();
        nativeShellChat_Shutdown();
        document->Close();
        document = nullptr;
    }
}

extern "C" void nativeShell_InspectElement(const char* id)
{
    if (!document) return;
    auto* element = document->GetElementById(id);
    if (!element) { std::fprintf(stderr, "[ui-inspect] %s: absent\n", id); return; }
    const auto origin = element->GetAbsoluteOffset(Rml::BoxArea::Content);
    const auto size = element->GetBox().GetSize(Rml::BoxArea::Content);
    const auto colour = element->GetComputedValues().color();
    std::fprintf(stderr, "[ui-inspect] %s: tag=%s visible=%d origin=%.1f,%.1f size=%.1f,%.1f scroll=%.1f,%.1f colour=%u,%u,%u,%u\n",
             id, element->GetTagName().c_str(), element->IsVisible(true),
             origin.x, origin.y, size.x, size.y, element->GetScrollLeft(), element->GetScrollTop(),
             colour.red, colour.green, colour.blue, colour.alpha);
    auto* control = dynamic_cast<Rml::ElementFormControl*>(element);
    std::fprintf(stderr,"[ui-inspect-value] %s: disabled=%d checked=%d value=%s text=%s\n", id,
                 element->HasAttribute("disabled"),element->HasAttribute("checked"),
                 control ? control->GetValue().c_str() : "", element->GetInnerRML().substr(0,240).c_str());
}

extern "C" void nativeShell_ClickAt(int x, int y, int button)
{
    if (document == nullptr) {
        return;
    }
    Rml::Context* context = document->GetContext();
    context->ProcessMouseMove(x, y, 0);
    context->ProcessMouseButtonDown(button, 0);
    context->ProcessMouseButtonUp(button, 0);
}

extern "C" void nativeShell_DragAt(int start_x, int start_y,
                                     int end_x, int end_y)
{
    if (document == nullptr) {
        return;
    }
    Rml::Context* context = document->GetContext();
    context->ProcessMouseMove(start_x, start_y, 0);
    context->ProcessMouseButtonDown(0, 0);
    for (int step = 1; step <= 8; ++step) {
        context->ProcessMouseMove(start_x + (end_x - start_x) * step / 8,
                                  start_y + (end_y - start_y) * step / 8, 0);
    }
    context->ProcessMouseButtonUp(0, 0);
}

extern "C" void nativeShell_EnterText(const char* text)
{
    if (document == nullptr || text == nullptr) {
        return;
    }
    document->GetContext()->ProcessTextInput(Rml::String(text));
}

extern "C" void nativeShell_MovePointer(int x, int y)
{
    if (document) {
        document->GetContext()->ProcessMouseMove(x, y, 0);
    }
}

extern "C" void nativeShell_PressKey(const char* name, unsigned modifiers)
{
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
    if (std::strcmp(name, "escape") == 0) {
        key = Rml::Input::KI_ESCAPE;
    }
    if (std::strcmp(name, "enter") == 0) {
        key = Rml::Input::KI_RETURN;
    }
    if (std::strcmp(name, "delete") == 0) {
        key = Rml::Input::KI_DELETE;
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
