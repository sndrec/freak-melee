#pragma once
#include "native_hub.h"
#include "native_hub_catalog.h"
#include <melee/mn/native_menu_paint.h>
#include <melee/ui/native_chat.h>

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
    Close,
    Singles,
    Doubles,
    Create,
    LocalPlayers,
    Join,
    Leave,
    CopyCode,
    Practice,
    Selection,
    Rules,
    Costumes,
    Packages,
    Rotation,
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
    RecipientScroll,
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
    PeopleScroll,
    ChatScroll,
    RoomChat,
    FieldRoomChat,
    SendRoomChat,
    RetryRoomChat,
    LobbyScroll,
    Seat,
    ShowProfile,
    WriteLetter,
    StageSeat,
    PickStage,
    ConfirmStage,
    WithdrawStage,
    LeaveQuick,
    NewLobby, FieldLobbyTitle, ToggleListing, CreateNamedLobby,
    RefreshLobbies, BrowseLobbies, JoinListedLobby, ChangeListing,
    Account, AccountMode, AccountSubmit, FieldAccountHandle, FieldAccountRecovery,
    AccountExportKey, AccountImportKey, AccountAddKey, AccountRemoveKey, AccountRevokeSession,
    AccountRevokeAll, AccountRotateRecovery, AccountSaveRecovery, AccountLogout,
    AccountRefresh, AccountCancel, AccountConfirm,
    SocialLookup, FieldSocialHandle, FieldLetterTo, FieldLetterSubject,
    FieldLetterBody, ComposeLetter, SendLetter, OpenLetter, DeleteLetter, ReplyLetter,
    MailFolder, MailPage, FriendPage, SocialRefresh, SocialFriend, SocialBlock,
    OpenAccount, SendInvitation, JoinInvitation,
    Startup, StartupNext, StartupBack, StartupPort, StartupSkipController, StartupRetry,
    StartupTriggerMaximum, StartupFighterMove, StartupConfirmFighter,
    StartupPortraitReset, StartupPortraitSave, StartupTileName, StartupPlaceTile
};
struct HubRegion {
    float x, y, w, h;
    HubAction action;
    int value;
    char id[40], peer[65], peer_name[33], account[25];
    bool enabled;
};
struct HubState {
    NativeHubPage page = NativeHubPage_Workshop,
                  previous = NativeHubPage_Workshop,
                  motion_from = NativeHubPage_Workshop;
    HubRegion regions[192];
    unsigned region_count;
    float pointer_x = 310, pointer_y = 240;
    char notice[256], name[33] = "Player", public_name[33] = "Player",
                      bio[257], code[13], chat[513], room_chat[513], title[64];
    char viewed_name[33], viewed_peer[65], recipient[65], recipient_name[33];
    char lobby_title[65];
    char account_handle[25], account_recovery[44];
    char social_handle[25], letter_to[25], letter_subject[65], letter_body[2049];
    char lobby_invite_to[25];
    bool lobby_invite_queued;
    bool lobby_listed = true, pending_listing;
    NativeContentId gift_id;
    char gift_title[64];
    char* field;
    unsigned field_capacity, caret;
    int selected = -1, first_fighter = 0, template_kind = 0, local_players = 1,
        chat_scroll = 0, people_scroll = 0, recipient_scroll = 0,
        room_chat_scroll = 0, room_people_scroll = 0, lobby_scroll = 0;
    bool visible, initialized, fonts, selection_before, dismissed, pressed;
    bool returning_from_tool, rename, select_all, send_from_css,
        profile_self = true;
    bool opened_from_play;
    bool consumes_input;
    bool passive_page;
    unsigned pending_action;
    int pending_value;
    char pending_peer[65], pending_name[33], pending_account[25];
    unsigned back_frames;
    unsigned stage_seat;
    uint64_t notice_until;
};
extern HubState hub;
bool nativeHub_Hover(float x, float y, float w, float h);
void nativeHub_Region(const char* id, float x, float y, float w, float h,
                      HubAction action, int value = 0, bool enabled = true,
                      const char* peer = nullptr, const char* name = nullptr,
                      const char* account = nullptr);
void nativeHub_Button(const char* id, const char* label, float x, float y,
                      float w, float h, HubAction action, int value = 0,
                      bool gold = false, bool enabled = true,
                      const char* peer = nullptr, const char* name = nullptr,
                      const char* account = nullptr);
void nativeHub_Field(const char* id, const char* value, float x, float y,
                     float w, float h, HubAction action, bool secret = false,
                     const char* placeholder = nullptr);
unsigned nativeHub_TextBlock(const char* value, float x, float y, float w, float h,
    float size, uint32_t color, unsigned first_line = 0);
unsigned nativeHub_TextCaret(const char* value, unsigned caret, int direction, float width);
void nativeHub_Panel(float x, float y, float w, float h);
void nativeHub_ChangePage(NativeHubPage page, bool backwards = false);
void nativeHub_RenderPage(NativeHubPage page);
void nativeHub_Activate(HubAction action, int value);
bool nativeHub_SaveProfile();
