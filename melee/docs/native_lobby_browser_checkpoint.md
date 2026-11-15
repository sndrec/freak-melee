# Native lobby browser checkpoint

Updated: 2026-10-19. The current product scope is in the
[social hub design](native_social_hub_design.md).

## Behavior

Lobbies shows named, listed player-created lobbies for the running client
build. Each row reports fighting seats, waiting seats, spectators and whether
a game is in progress. Join opens that lobby without entering its code.
The browser refreshes while open and supports eight rows per page.

Create lobby opens a name field, a Listed/Unlisted choice and the number of
players on this PC. The host can change visibility afterward. Unlisted lobbies
accept their codes. The last participant leaving unlists the lobby. Quick Play
parties and assigned matches are excluded.

The native page uses the hub's existing stock menu scene, typefaces, sliced
panels and hand input. Retain the stock navigation ribbon texture, including
its stylized interior shape. Its resemblance to a letter is intentional
artwork, not an error to remove.

## Service and client

Room owns the title and listing choice. Only the connected host can change
them. LobbyDirectory keeps room codes and monotonic room revisions, then reads
the live room objects when building a page. Player counts come from actual
seat masks; clients cannot advertise invented occupancy. Hibernated rooms
retain their connected participants. Revision ordering prevents an old
advertisement from relisting a newer unlisted lobby.

The initial private directory examines at most 512 listed rooms, in batches of
16 RPC calls. The browser has its own signaling transport connection and fixed
page storage. Gameplay signaling and global chat use their existing separate
connections. Browser requests never wait for network I/O on the UI thread.

The added listing/directory messages require the matching client and service.
The service changes are local and have **not been deployed**. Persistent
accounts, friends, letters and invitations remain open. The preferred sign-in
direction uses native Windows passkeys; registration and recovery still need
implementation and verification.

## Verification

- Client and content tool builds after restoring the stock ribbon artwork:
  `.local/build-native-hub-directory-restored-ribbon.log`.
- TypeScript checks: `.local/hub-directory-typecheck-final.log`.
- Service suite: `.local/hub-directory-service-tests-final.log`, 48 passing
  tests. New checks cover host authority, live occupancy, build filtering,
  visibility, empty lobbies, pagination, hibernation, stale updates and exclusion
  of Quick Play parties. The final run has no teardown exceptions.
- Native wire checks: `.local/hub-directory-native-tests.log`. Added encoding,
  decoding, truncated-message and invalid-page checks passed.
- Native UI acceptance and reviewed screenshots are recorded in the receipt
  produced by `review_native_lobby_browser.py`. It runs two hidden, muted
  clients against an isolated local service and stops all owned processes.
  It covers listed joining, unlisting, joining by code and leaving.
  `.local/native-lobby-browser-live-2/receipt.json` records the stock-ribbon
  version. `.local/native-lobby-browser-live-3/receipt.json` also checks blank
  names, but used a temporary ribbon edit which the user rejected and which
  has been removed. That run establishes the lobby behavior, not the final
  artwork. The restoration leaves `native_menu_scene.c` identical to its
  pre-browser version.

Earlier failed build and harness logs remain under `.local`. They are not
acceptance results. Same-PC checks do not establish separate-network behavior
or the user's visual acceptance of the complete hub.
