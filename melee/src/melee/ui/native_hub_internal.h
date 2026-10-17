#pragma once
#include "native_hub_paint.h"
#include "native_hub_catalog.h"
#include <melee/ui/native_chat.h>

enum class HubPage {
    QuickPlay,
    Lobbies,
    Chat,
    Friends,
    Inbox,
    Workshop,
    Settings,
    Profile,
    Clone,
    Send
};
enum class HubAction {
    None,
    QuickPlay,
    Lobbies,
    Chat,
    Friends,
    Inbox,
    Workshop,
    Settings,
    Profile,
    Back,
    Singles,
    Doubles,
    Create,
    Join,
    Leave,
    CopyCode,
    Practice,
    StartMatch,
    Edit,
    NewClone,
    StockTemplate,
    CreateClone,
    Fighter,
    Previous,
    Next,
    CopyFighter,
    RenameFighter,
    ApplyRename,
    OpenImport,
    EditLayout,
    SendFighter,
    Recipient,
    ConfirmSend,
    Cancel,
    AcceptGift,
    DeclineGift,
    Controls,
    SaveProfile,
    FieldName,
    FieldBio,
    FieldCode,
    FieldChat,
    FieldTitle,
    SendChat,
    Seat,
    ShowProfile,
    WriteLetter,
    AddFriend,
    Invite
};
struct HubRegion {
    float x, y, w, h;
    HubAction action;
    int value;
    char id[40], peer[65], peer_name[33];
    bool enabled;
};
struct HubState {
    HubPage page = HubPage::Workshop, previous = HubPage::Workshop;
    HubRegion regions[192];
    unsigned region_count;
    float pointer_x = 310, pointer_y = 240;
    char notice[256], name[33] = "Player", bio[257], code[13], chat[513],
                      title[64];
    char viewed_name[33], viewed_peer[65], recipient[65], recipient_name[33];
    NativeContentId gift_id;
    char gift_title[64];
    char* field;
    unsigned field_capacity, caret;
    int selected = -1, first_fighter = 0, template_kind = 0, local_players = 1,
        chat_scroll = 0;
    bool visible, initialized, fonts, selection_before, dismissed, pressed;
    bool returning_from_tool, rename, select_all, profile_self = true;
    unsigned pending_action;
    int pending_value;
    char pending_peer[65], pending_name[33];
    unsigned back_frames;
    uint64_t notice_until;
};
extern HubState hub;
bool nativeHub_Hover(float x, float y, float w, float h);
void nativeHub_Region(const char* id, float x, float y, float w, float h,
                      HubAction action, int value = 0, bool enabled = true,
                      const char* peer = nullptr, const char* name = nullptr);
void nativeHub_Button(const char* id, const char* label, float x, float y,
                      float w, float h, HubAction action, int value = 0,
                      bool gold = false, bool enabled = true,
                      const char* peer = nullptr, const char* name = nullptr);
void nativeHub_Field(const char* id, const char* value, float x, float y,
                     float w, float h, HubAction action);
void nativeHub_Panel(float x, float y, float w, float h);
void nativeHub_ChangePage(HubPage page);
void nativeHub_RenderPage(void);
void nativeHub_Activate(HubAction action, int value);
