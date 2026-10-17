#include "native_shell_collection.h"

#ifdef MELEE_NATIVE

#include "native_shell.h"
#include "native_shell_costumes.h"
#include "native_shell_workshop.h"
#include <melee/ft/native_fighter_family_project.h>
#include "native_shell_profiles.h"
#include "native_shell_rules.h"

#include <melee/gm/native_content_store.h>
#include <melee/gm/native_home.h>
#include <melee/gm/native_match_launch.h>
#include <melee/gm/native_online_session.h>

#include <RmlUi/Core.h>
#include <RmlUi/Core/Elements/ElementFormControlSelect.h>
#include <SDL3/SDL_timer.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {

enum { MAX_COLLECTION_ITEMS = 512 };

struct CollectionItem {
    NativeContentId id;
    char title[NATIVE_CONTENT_TITLE_BYTES];
    NativeNetplayGiftKind kind;
};

Rml::ElementDocument* document;
CollectionItem items[MAX_COLLECTION_ITEMS];
NativeFighterDraft local_drafts[128];
size_t local_draft_count;
size_t item_count;
uint64_t displayed_revision;
uint64_t hide_after_slide;
bool opened;
std::string displayed_incoming;
std::string displayed_outgoing;

const char* content_root(char path[512])
{
    const char* root = nativeMatchLaunch_ContentRoot();
    return root != nullptr ? root
                           : nativeContentStore_DefaultRoot(path, 512)
                                 ? path : nullptr;
}

void status(const char* message)
{
    document->GetElementById("collection-status")
        ->SetInnerRML(Rml::StringUtilities::EncodeRml(message));
}

void load_items()
{
    item_count = 0;
    char default_path[512];
    const char* root = content_root(default_path);
    NativeContentId ids[MAX_COLLECTION_ITEMS];
    size_t count = 0;
    if (root == nullptr || !nativeContentStore_ListInstalled(
            root, ids, MAX_COLLECTION_ITEMS, &count)) {
        status("Could not read the complete installed collection.");
        return;
    }
    std::string rows;
    for (size_t index = 0; index < count; ++index) {
        uint8_t* bytes = nullptr;
        size_t size = 0;
        if (!nativeContentStore_Read(root, &ids[index], &bytes, &size)) {
            continue;
        }
        CollectionItem item = {};
        item.id = ids[index];
        NativeCostumePackage costume;
        NativeFighterPackage fighter;
        NativeFighterPackageData fighter_data;
        const uint8_t* dat;
        size_t dat_size;
        if (nativeContentStore_ParseCostume(bytes, size, &costume, &dat,
                                            &dat_size)) {
            item.kind = NativeNetplayGift_Costume;
            std::memcpy(item.title, costume.title, sizeof(item.title));
        } else if (nativeContentStore_ParseFighter(bytes, size, &fighter,
                                                   &fighter_data)) {
            item.kind = NativeNetplayGift_Fighter;
            std::memcpy(item.title, fighter.title, sizeof(item.title));
        }
        std::free(bytes);
        if (item.kind == 0) {
            continue;
        }
        char hex[65];
        nativeContentStore_IdHex(&item.id, hex);
        const size_t row = item_count;
        items[item_count++] = item;
        rows += "<div class='collection-item'><div><span class='collection-kind'>";
        rows += item.kind == NativeNetplayGift_Fighter ? "FIGHTER" : "COSTUME";
        rows += "</span><strong>";
        rows += Rml::StringUtilities::EncodeRml(item.title);
        rows += "</strong><span class='collection-id'>";
        rows.append(hex, 12);
        rows += "</span></div><button id='collection-send-" +
                std::to_string(row) + "'>Send</button></div>";
    }
    std::string draft_rows;
    if (!nativeFighterFamilyProject_ListLocal(local_drafts, 128, &local_draft_count)) {
        local_draft_count = 0;
        draft_rows = "<p class='collection-empty'>Could not read local drafts.</p>";
    }
    for (size_t i = 0; i < local_draft_count; ++i) {
        draft_rows += "<div class='collection-item'><div><span class='collection-kind'>EDITABLE DRAFT</span><strong>" +
            Rml::StringUtilities::EncodeRml(local_drafts[i].title) +
            "</strong><span class='collection-id'>Local project · editable</span></div><button id='collection-draft-" +
            std::to_string(i) + "'>Edit</button></div>";
    }
    document->GetElementById("collection-drafts")->SetInnerRML(draft_rows.empty()
        ? "<p class='collection-empty'>No drafts yet. Create one in the Workshop.</p>" : draft_rows);
    document->GetElementById("collection-count")
        ->SetInnerRML(std::to_string(item_count) + " installed");
    document->GetElementById("collection-items")
        ->SetInnerRML(rows.empty() ?
            "<div class='collection-empty'>No installed packages yet.</div>"
            : rows);
}

void update_recipients()
{
    const NativeSignalingPresence* presence = nativeOnlineSession_Presence();
    const uint64_t revision = presence != nullptr ? presence->revision : 0;
    if (revision == displayed_revision) {
        return;
    }
    displayed_revision = revision;
    auto* select = static_cast<Rml::ElementFormControlSelect*>(
        document->GetElementById("collection-recipient"));
    const std::string previous = select->GetValue();
    select->RemoveAll();
    select->Add("Choose someone in the room", "");
    const char* local = nativeOnlineSession_LocalPeerId();
    bool previous_found = false;
    if (presence != nullptr) {
        for (unsigned i = 0; i < presence->participant_count; ++i) {
            const NativeSignalingParticipant* peer =
                &presence->participants[i];
            if (local != nullptr && std::strcmp(peer->peer_id, local) == 0) {
                continue;
            }
            select->Add(Rml::StringUtilities::EncodeRml(
                            peer->name[0] != '\0' ? peer->name : "Player"),
                        peer->peer_id);
            previous_found |= previous == peer->peer_id;
        }
    }
    if (previous_found) {
        select->SetValue(previous);
    }
}

std::string peer_name(const char* peer_id)
{
    const NativeSignalingPresence* presence = nativeOnlineSession_Presence();
    if (presence != nullptr) {
        for (unsigned i = 0; i < presence->participant_count; ++i) {
            const NativeSignalingParticipant* peer =
                &presence->participants[i];
            if (std::strcmp(peer->peer_id, peer_id) == 0) {
                return peer->name[0] != '\0' ? peer->name : "Player";
            }
        }
    }
    return "Player";
}

void update_gift_status()
{
    NativeNetplayGiftStatus gift;
    std::string incoming;
    const bool has_incoming = nativeOnlineSession_GiftStatus(true, &gift);
    if (has_incoming) {
        const std::string title =
            Rml::StringUtilities::EncodeRml(gift.title);
        const std::string sender = Rml::StringUtilities::EncodeRml(
            peer_name(gift.peer_id));
        if (gift.state == NativeNetplayGift_Offered) {
            incoming = "<strong>" + sender + " offered " + title +
                "</strong><div class='gift-buttons'><button id='gift-accept'>"
                "Accept &amp; install</button><button id='gift-decline'>"
                "No thanks</button></div>";
        } else if (gift.state == NativeNetplayGift_Receiving) {
            const unsigned percent = gift.bytes_total == 0 ? 0 :
                unsigned(uint64_t(gift.bytes_done) * 100 / gift.bytes_total);
            incoming = "Installing " + title + " · " +
                       std::to_string(percent) + "%";
        } else if (gift.state == NativeNetplayGift_Installed) {
            incoming = gift.kind == NativeNetplayGift_Fighter
                ? title + " installed. Add its portrait through Edit CSS to select it."
                : title + " installed. Choose its fighter, then select it in COSTUMES.";
        } else if (gift.state == NativeNetplayGift_Failed) {
            incoming = "Could not install " + title + ".";
        }
    }
    Rml::Element* card = document->GetElementById("gift-incoming");
    if (incoming != displayed_incoming) {
        displayed_incoming = incoming;
        card->SetInnerRML(incoming);
        card->SetProperty("display", incoming.empty() ? "none" : "block");
        if (opened && has_incoming &&
            gift.state == NativeNetplayGift_Installed) {
            load_items();
        }
        if (has_incoming && gift.state == NativeNetplayGift_Installed &&
            gift.kind == NativeNetplayGift_Costume) {
            nativeShellCostumes_RefreshInstalled();
        }
    }
    std::string outgoing;
    if (nativeOnlineSession_GiftStatus(false, &gift)) {
        const std::string title =
            Rml::StringUtilities::EncodeRml(gift.title);
        const std::string recipient = Rml::StringUtilities::EncodeRml(
            peer_name(gift.peer_id));
        if (gift.state == NativeNetplayGift_Offered) {
            outgoing = "Waiting for " + recipient + " to accept " + title;
        } else if (gift.state == NativeNetplayGift_Sending) {
            const unsigned percent = gift.bytes_total == 0 ? 0 :
                unsigned(uint64_t(gift.bytes_done) * 100 / gift.bytes_total);
            outgoing = "Sending " + title + " · " +
                       std::to_string(percent) + "%";
        } else if (gift.state == NativeNetplayGift_Sent) {
            outgoing = title + " sent to " + recipient;
        } else if (gift.state == NativeNetplayGift_Declined) {
            outgoing = recipient + " declined " + title;
        } else if (gift.state == NativeNetplayGift_Failed) {
            outgoing = "Could not send " + title;
        }
    }
    if (outgoing != displayed_outgoing) {
        displayed_outgoing = outgoing;
        document->GetElementById("collection-transfer")
            ->SetInnerRML(outgoing);
    }
}

class CollectionEvents final : public Rml::EventListener {
    void ProcessEvent(Rml::Event& event) override
    {
        Rml::Element* target = event.GetTargetElement();
        while (target != nullptr && target->GetId().empty()) {
            target = target->GetParentNode();
        }
        if (target == nullptr) {
            return;
        }
        const Rml::String id = target->GetId();
        if (event.GetType() == "keydown") {
            const int key = event.GetParameter<int>("key_identifier", 0);
            if (key == Rml::Input::KI_ESCAPE) {
                nativeShellCollection_Close();
                event.StopPropagation();
            } else if ((key == Rml::Input::KI_RETURN ||
                        key == Rml::Input::KI_SPACE) &&
                       target->GetTagName() == "button") {
                target->Click();
                event.StopPropagation();
            }
            return;
        }
        if (id == "collection-tab") {
            nativeShellCollection_Open();
        } else if (id == "collection-close") {
            nativeShellCollection_Close();
        } else if (id == "gift-accept") {
            if (!nativeOnlineSession_AcceptGift()) {
                status("Could not install the offered package.");
            }
        } else if (id == "gift-decline") {
            nativeOnlineSession_DeclineGift();
        } else if (id.compare(0, 17, "collection-draft-") == 0) {
            char* end = nullptr;
            const long index = std::strtol(id.c_str() + 17, &end, 10);
            if (*end == '\0' && index >= 0 && size_t(index) < local_draft_count)
                nativeShellWorkshop_Open(local_drafts[index].path, NativeWorkshopOrigin::Collection);
        } else if (id.compare(0, 16, "collection-send-") == 0) {
            char* end = nullptr;
            const long index = std::strtol(id.c_str() + 16, &end, 10);
            if (*end != '\0' || index < 0 || size_t(index) >= item_count) {
                return;
            }
            const std::string recipient =
                static_cast<Rml::ElementFormControlSelect*>(
                    document->GetElementById("collection-recipient"))
                    ->GetValue();
            if (recipient.empty()) {
                status("Choose someone in the room first.");
            } else if (!nativeOnlineSession_OfferGift(
                           recipient.c_str(), &items[index].id)) {
                status("Could not offer this package. Wait for the peer connection, then retry.");
            } else {
                status("");
            }
        }
    }
} events;

} // namespace

void nativeShellCollection_Open()
{
    if (!nativeHome_IsSelection() && !nativeHome_IsPractice()) {
        nativeShell_ShowRoomError("Return home to manage your collection.");
        return;
    }
    nativeShellProfiles_Close();
    nativeShellRules_Close();
    status("");
    load_items();
    displayed_revision = UINT64_MAX;
    update_recipients();
    opened = true;
    hide_after_slide = 0;
    Rml::Element* panel = document->GetElementById("collection-panel");
    panel->SetProperty("visibility", "visible");
    panel->SetClass("open", true);
    document->GetElementById("collection-close")->Focus(true);
}

void nativeShellCollection_Init(Rml::ElementDocument* shell)
{
    document = shell;
    opened = false;
    displayed_revision = UINT64_MAX;
    displayed_incoming.clear();
    displayed_outgoing.clear();
    for (const char* id : { "collection-tab", "collection-panel",
                            "gift-incoming" }) {
        Rml::Element* element = document->GetElementById(id);
        element->AddEventListener("click", &events);
        element->AddEventListener("keydown", &events);
    }
}

void nativeShellCollection_Update()
{
    if (hide_after_slide != 0 && SDL_GetTicks() >= hide_after_slide) {
        document->GetElementById("collection-panel")
            ->SetProperty("visibility", "hidden");
        hide_after_slide = 0;
    }
    if (opened) {
        if (!nativeHome_IsSelection() && !nativeHome_IsPractice()) {
            nativeShellCollection_Close();
        } else {
            update_recipients();
        }
    }
    update_gift_status();
}

void nativeShellCollection_Close()
{
    if (!opened) {
        return;
    }
    opened = false;
    hide_after_slide = SDL_GetTicks() + 250;
    document->GetElementById("collection-panel")->SetClass("open", false);
    nativeShell_BlurRoom();
}

#endif
