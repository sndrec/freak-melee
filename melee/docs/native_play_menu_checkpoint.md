# Native character-select and practice controls

Updated: 2026-10-17.

Character select and ready practice now use controls drawn by the game. The old
room sidebars and sidebar chat presenter have been removed. The game keeps its
4:3 viewport. The Workshop, bindings, rules, costume selector and package tools
remain available from the hub.

## Controls

On character select, place each active player's cookie, then press START or
choose **Ready / START** to enter practice. The button uses the same ready check
and scene transition as START. Carrying a cookie or a fighter portrait prevents
readiness. **Workshop** opens the fighter library; **Back** opens Lobbies. F1 or
holding B also opens the hub.

Practice has a compact strip with **Lobby**, **Start match** and **Character
select**. Start match enables when the existing local/room readiness rules allow
it. In a lobby, the host starts the match. Character select exits through the
game's normal match-abort path and clears readiness. The ordinary pause-and-exit
combination remains available.

Pause practice, then press Z to open the hub with a controller. F1 or the Lobby
button also opens it. B or **Return to practice** closes it. Menu input stays out
of the fighter controls, including the frame used to close the menu. Workshop's
live Test Character mode keeps its gameplay input.

Incoming fighters have native Accept fighter and No thanks buttons on both CSS
and practice. Delivery progress and results appear in the same area. A received
fighter is added to the saved roster. The native hub's Workshop page also keeps
its acceptance controls.

Lobbies includes seat choices, host start, rotation policy, match rules and lobby
chat. Its player list pages through larger rooms. [Chat](native_chat.md) describes
its private-room delivery and history.
Settings opens bindings, rules, costumes and installed packages. Editing fighters
and rules requires returning to character select. The costume selector is now a
separate tool page, with the existing package-selection and DAT-import functions.

## Implementation

- `native_play_menu.cpp` owns the fixed button list, input, delivery display and
  ready request. CSS clicks use the stock hand's position. Mouse input follows
  that same path. Practice has its own pointer handling for the compact strip.
- `native_menu_paint.c` supplies the shared menu drawing functions. Hub and
  practice menus use its orthographic canvas. CSS supplies a matrix that maps the
  canvas onto the original menu plane, retaining its camera and tilt. The stock
  ready-banner process still owns START handling; the native button queues a
  request for that process.
- `nativeCssOriginal_PointToCanvas` projects native control bounds through the
  CSS camera for diagnostic clicks. It is the reverse of the existing mouse ray
  mapping.
- `native_hub.cpp` opens pages from either CSS or practice and retains a return
  destination. Its input-consumption state covers opening and closing frames.
- `native_shell.cpp` hosts the detailed RmlUI tools and their viewport margins.
  It now owns the chat client's start, presentation update and shutdown directly.
  The deleted sidebar presenter no longer owns network lifetime.
- The native menu and tool state remains host-owned in the build's snapshot
  classification. The new button list uses fixed storage.

## Verification

`review_native_css.py` passed in `.local/native-play-menu-acceptance-2`. It checks
cookie selection and free placement, carried-cookie readiness rejection, CPU
selection, native Ready-button practice entry, native practice return, opening
the hub with Z while paused, and round trips through costumes, rules and installed
packages. It also verifies that the old right rail is absent. Reviewed PNGs show
the native CSS controls and practice strip.

The first run reached practice successfully but tried pausing during the retail
Ready/Go countdown. Its screenshot showed 2.70 seconds remaining. The check now
allows that intro to finish before exercising pause controls.

`review_native_css_room.py` passed in `.local/native-play-menu-room-1`. Two hidden,
muted clients verify the CSS card identities, portrait drag/drop and cancellation,
an uncached fighter delivery to a spectating recipient in practice, native
acceptance, the saved roster tile, role changes and duplicate-install prevention.
Reviewed PNGs show the native delivery notice and the new tile after returning.

`review_native_hub.py` passed in `.local/native-play-menu-authoring-1`: empty first
launch, a playable clone, rename replacement, an independent copy, opening and
closing the frozen Workshop preview with CRLF source preserved, profile saving
and controller return from CSS.

`review_native_hub_presence.py` passed in `.local/native-play-menu-presence-1`.
The running native Chatroom receives the service's player directory and shows
the isolated test client's chosen name. This verifies the chat lifetime after
removing its sidebar presenter. The test posts no messages or invitations.

The participant, lobby-selection, chat-wire and connection-wire CTest checks
pass. The native client builds successfully.

The focused run in `.local/native-play-menu-costume-escape` verifies the empty
costume selector's instruction text and Escape returning to native Settings.

These local checks do not establish hardware GameCube-adapter feel, physical
multi-PC delivery or the final authored transitions. Native Quick Play queue
checks are recorded in [its checkpoint](native_quick_play_checkpoint.md).

## Remaining integration

Quick Play's countdown and fixed-rule first match now pass native singles and
shared-PC doubles checks. Result agreement and rematches now have local native
automation checks; stage-confirmation controls and service deployment remain
open. Remote portrait preparation
and lobby chat have their own completed local checks. Authored menu transitions
remain open. Account sign-in and offline inbox decisions remain deferred for
the user.
