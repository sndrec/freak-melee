# Global chat

## Live player list, 2026-10-17

Chatroom shows everyone connected to global chat, including players who have
not posted. Saving a display name updates that name in the player list.
Selecting a row opens the player's profile. The list supports 128 connected
players, with compact rows, paging and mouse-wheel scrolling.

The service sends a bounded presence snapshot on joins, leaves and name
changes. Clients display the new list after receiving its completion marker.
Connection loss clears the live list; reconnecting announces the saved display
name again. WebSocket attachments retain names through service hibernation.
The native client uses fixed storage for incoming presence and its event queue.

Worker type checks and all 18 service tests pass. The native wire test covers
presence records, name announcements, UTF-8 and size limits. Two Windows client
probes saw each other without sending messages. A hidden, muted game connected
to the local service and displayed both itself and a quiet probe in Chatroom;
the PNG and logs are under `.local/hub-chat-review`.

Private service deployment `f76ecd50-8d96-4fb4-940e-704c0cf20468` contains the
presence update. Existing chat history and room signaling remain in their
current Durable Objects.
A native presence-only probe also connected to that deployment and received its
own directory entry without posting a chat message. Its log is local under
`.local/native-chat-presence-live.log`.

The native shell connects automatically to `/chat/connect` on the private
service. One `GlobalChat` Durable Object owns the friends' shared conversation;
it is independent of the existing per-room objects. Room membership and client
build do not partition this conversation. The editable Name field supplies the
sender name. Enter or Send submits a message; the header button collapses chat.

The service keeps the latest 64 messages in SQLite. The upgrade sends that
history followed by a ready marker and the connected-client count. Broadcasts
include the original sender, which serves as the delivery acknowledgement only
after the database write commits. A process-random peer ID and incrementing
nonce identify an attempted message. Retrying an unconfirmed message reuses its
nonce and is deduplicated while that message remains in the retained history.
Names are limited to 32 UTF-8 bytes and text to 512; malformed UTF-8 and control
characters are rejected. Rml markup is escaped when displaying names and text.

The client uses a dedicated I/O worker with asynchronous WinHTTP operations.
The worker owns network handles and fixed receive/send storage. Bounded SPSC
queues exchange copied commands and events with the presentation thread; the
callbacks never access RmlUi or game state. Chat and shell sources are excluded
from the rollback section. Hidden rollback frames do not update the shell, and
chat outages do not stop gameplay or room signaling.

Disconnects reconnect with a 1–30 second backoff. A liveness probe every 20
seconds detects an unresponsive connection after 60 seconds. A pending message
becomes unconfirmed after ten seconds or a disconnect. It is never inserted
into displayed history before the service echoes it. Reconnect history can
confirm it; otherwise Retry is explicit. Shutdown cancels async operations and
waits for final handle callbacks before releasing their storage.

## Verification, 2026-09-22

- Release client and native probe build successfully. All 354 native tests pass
  serially, with `-UNDEBUG` now applied to native test executables so Release
  builds actually execute their assertion checks.
- Worker type checking and all 12 service tests pass. Chat coverage includes
  cross-client delivery, persisted history after hibernation, idempotent retry,
  the 64-message limit, presence, ping/pong, oversized messages and invalid UTF-8.
- Two separate native probe processes exchanged messages through the local
  Worker; both recorded acknowledgement and the other process's message.
- A native probe observed Online → Reconnecting → Online across a local Worker
  restart, retaining history without duplicate display. Clean shutdown reported
  zero milliseconds in that run.
- Private deployment `9ef54a9a-89f6-4c4d-a848-f17005513890` exposes chat alongside
  the existing room and TURN endpoints. A native probe acknowledged a message
  through that deployment. The actual game received it, sent a message through
  the sidebar, and retained chat while creating/leaving a room and entering CSS.
  Sending during a four-player local match also succeeded, with the acknowledged
  message retained after restarting the game.
- Full-window screenshots reviewed: `build/native/chat-live.png`,
  `chat-room.png`, `chat-left-room.png`, `chat-collapsed.png`, and `chat-css.png`.
  Chat does not cover or resize the 4:3 game viewport.
- Overflowing history exposed an unstyled RmlUi scrollbar consuming the text
  width. Explicit scrollbar sizing fixes this; `chat-scrollbar-final.png` shows
  the overflowing history with readable text and a working-width scroll thumb.
- Object-file section inspection found no `.mstate` or `.maudio` section in
  either chat client or chat UI state.

## Diagnostics

`native_chat_probe ENDPOINT NAME MESSAGE EXPECTED_MESSAGE SECONDS` starts the
same native chat client without the game, sends once connected, prints received
messages and connection changes, and succeeds only with an acknowledgement and
the expected message. Use two processes to check independent senders. Its
endpoint argument supports the local Worker at `http://127.0.0.1:8799`.

`native_chat_probe --presence ENDPOINT NAME EXPECTED_NAME SECONDS` announces
a player without posting. It succeeds when the directory contains both its
own name and the expected other player.

The game defaults to the deployed service. `FREAK_MELEE_CHAT_ENDPOINT` overrides
the endpoint for local development; do not change it to join different game
rooms. Ordinary application launches need no environment overrides.

## Remaining limits

This is a small shared text conversation, not yet a persistent identity or
social system. Names are currently session-only. There is no account system,
message editing, notification sound, controller-only text entry, or unread
counter. Retrying a message older than the 64-message retention window can
post it again. Chat is hidden along with the native rails in narrow windows.
Portable spectator integration and chat activity during a live rollback
correction still need broader end-to-end acceptance.
