# Chat

Updated: 2026-10-19.

## Using chat

**Chatroom** opens the global conversation and lists everyone connected to it.
**Lobbies > Chat** opens the conversation for your current lobby. Fighting,
waiting and spectating members can all participate. Enter sends the text in the
message field; the Send button does the same. Global and lobby chat keep separate
drafts and scroll positions. The mouse wheel scrolls messages by text line, so
long messages remain readable. Older and Newer buttons also scroll history with
the controller hand. The compact player lists have paging controls and
mouse-wheel scrolling. Selecting a player opens their profile.

Lobby chat is available from character select and ready practice. Pause practice
and press Z to open the hub, or use F1 or its Lobby button. B returns from lobby
chat to the lobby controls. **Return to practice** resumes the practice view.

Both conversations retain their latest 64 messages. Lobby history lasts while
someone remains connected; it clears when the last member leaves. Global history
persists between sessions. Each message supports up to 512 UTF-8 bytes. Names
support up to 32 bytes. Control characters and malformed UTF-8 are rejected.

A message appears after the service confirms delivery. **Sending...** indicates
that confirmation is pending. An unconfirmed lobby message has an explicit Retry
button. Retry sends the same message identifier, preventing a second copy while
the original remains in history. If that entry has expired, the service reports
that it is no longer retained. You can write a fresh message after that.

Signed-in players carry their verified account name in global and lobby
presence. Selecting them opens their account profile. Guest connections use
their local display name. [Friends and letters](native_social_accounts.md)
provides persistent contacts, correspondence and lobby invitations. Fighter
sharing uses the connected lobby's delivery flow.

## Native implementation

`native_chat.c` owns the global connection, reconnects and fixed message storage.
`native_shell.cpp` starts, updates and stops it. `native_hub_pages.cpp` draws the
native Chatroom and lobby-chat pages. The original sidebar presenter has been
removed. These native pages remain inside the game's 4:3 canvas.

`native_room_chat.c` owns the lobby history, staged history replacement, pending
message and retry state. The existing room-signaling worker carries lobby chat.
Its receive queue holds 128 chat records separately from the 16-event signaling
queue. If chat overflows, the client requests a complete history replacement;
gameplay signaling continues. History becomes visible only after its completion
marker agrees with the received records. This storage belongs to the online
session and is excluded from rollback snapshots. No additional network thread
or gameplay ICE route is created for chat.

The per-room Durable Object stores lobby messages in SQLite before broadcasting
them. The sender's name and peer identifier come from its connection attachment.
Attachments also retain subscription, rate-limit and message-number state across
hibernation. New posts are limited to one every 500 ms per connection; history
requests are limited to one per second. Duplicate retries are checked first.
Chat rejections have their own packet and leave room signaling connected.

The server sends history only after a client subscribes. Messages use additional
room-protocol packet types, with the existing presence and gameplay signaling
records unchanged. History and live entries include a sequence number, nonce,
sender identifier, name and text. The client's fixed chat queue keeps incoming
history bursts out of the gameplay-signaling queue. Closing a room socket sends
a WebSocket close response and updates membership.

## Verification

The service's type generation, application/test type checks and all 21 tests
pass. The three lobby-chat cases cover cross-client and spectator delivery,
room isolation, authoritative sender names, idempotent retry, invalid text,
rate limits, history retention, server hibernation and empty-room cleanup.
The final private deployment is recorded in
`.local/native-room-chat-service-final-deploy.log`.

The native signaling-wire test covers chat encoding, UTF-8 rejection, truncation,
history limits and the new packet types. The room-chat state test covers staged
history publication, acknowledgments, duplicate delivery, disconnect state,
history-based confirmation and chat overflow recovery. Global chat wire,
participant ordering and the existing connection wire checks also pass.

`review_native_lobby_chat.py` starts two hidden, muted native game clients in
isolated profiles and a fresh private lobby. Additional spectator connections
exercise the compact player list and the final page of a 15-member lobby.
Messages are sent only to that private lobby. Global chat is disabled in these
test clients. The run checks both native send controls, server echoes, switching
between global and room views, spectator delivery without gameplay links,
profile navigation and return to character select. PNG screenshots and logs from
the completed run are under `.local/native-room-chat-acceptance-2`.

The content-channel and gameplay-connection probes also pass after chat was
added. Four gameplay players retained their routes while a waiting/spectating
recipient received content; the shared-PC input probe still delivered rollback
input after its signaling sockets closed. Logs are under
`.local/native-room-chat-content-regression.log` and
`.local/native-room-chat-gameplay-regression.log`.

The extended run in `.local/native-room-chat-acceptance-3` also selects a fighter,
enters ready practice, opens lobby chat with pause/Z and receives a fourth
message there. Reviewed screenshots show that practice retains its room and chat.
The history-only run in `.local/native-room-chat-history-3` sends four long
messages, uses controller A on Older, and returns to the newest text with Newer.
Its reviewed screenshots show wrapped text staying inside the message panel.
Physical multi-PC acceptance remains deferred to user testing.

## Diagnostics

`native_chat_probe --presence ENDPOINT NAME EXPECTED_NAME SECONDS` checks the
quiet global player directory. It posts no message.

`native_chat_probe ENDPOINT NAME MESSAGE EXPECTED_MESSAGE SECONDS` exchanges
messages through the global endpoint. Use a local service for automated message
tests. `FREAK_MELEE_CHAT_ENDPOINT` selects that global endpoint for development.
Room chat uses the selected room-signaling service.

`ui-inspect lobby-chat-state` reports the active room-chat connection, history
and delivery state, followed by its retained entries. It is an explicit local
diagnostic; normal UI updates do not print message contents.
