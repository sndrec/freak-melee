# Native character-select and practice controls

Updated: 2026-11-15.

Character select and ready practice now use controls drawn by the game. The old
room sidebars and sidebar chat presenter have been removed. The game keeps its
4:3 viewport. The Workshop, bindings, rules, costume selector and package tools
remain available from the hub.

## Controls

On character select, place each active player's cookie, then press START or
choose **Ready / START** to enter practice. The button uses the same ready check
and scene transition as START. Carrying a cookie or a fighter portrait prevents
readiness. **Back** opens Lobbies. F1 or holding B also opens the hub. In 1v1
Quick Play, the latest placed local token owns the single local seat. Enter the
queue before readying into practice.

Online practice has a status-only strip and incoming chat that fades away.
START pauses and unreadies; START resumes and readies. The normal paused
L+R+A+START exit returns to CSS. When all active lobby fighters are ready, a
three-second countdown leads to stage selection. Waiting players and spectators
are excluded. [Stage selection](native_lobby_stage_checkpoint.md) explains each
policy. Offline practice keeps its Lobby and Character select buttons.

Pause practice, then press Z to open the hub with a controller. F1 or the Lobby
button also opens it. B or **Return to practice** closes it. Menu input stays out
of the fighter controls, including the frame used to close the menu. Workshop's
live Test Character mode keeps its gameplay input.

Incoming fighters have native Accept fighter and No thanks buttons on CSS.
Delivery progress and results appear in the same area. A received
fighter is added to the saved roster. The native hub's Workshop page also keeps
its acceptance controls.

CSS includes lobby chat, while Rules > Additional Rules > Lobby settings holds
seat choices, rotation and listing controls. [Chat](native_chat.md) describes
delivery and history.
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

The Melee hand is available in both CSS and practice. CSS lends its already
decoded model to `native_hub_hand.c`. Practice imports only the hand's model and
material animation from the disc's CSS table into its own scene buffers. It
uses the same hand renderer, pressed pose, fingertip anchor and orthographic
camera. The compact practice strip loads it before the full hub opens.

`mnCharSelAssets_DecodeModel` is the shared decoder for a single model record;
the full CSS table calls it directly too. The practice import keeps its disc
bytes and bounded conversion arena alive for texture and geometry references.
CSS exit, practice exit and application shutdown release the hand before its
source buffers are freed. The practice model uses no references to the retired
CSS scene.

## Verification

### Authored hub page changes

The hub now plays Melee's main-menu transition tracks from `MnMaAll`.
`native_menu_motion.c` reads the transform animation on joint 3 of
`MenMainConTop_Top`, retaining its tilted parent orientation and the stock
camera's perspective. Forward navigation plays frames 340–359 on the outgoing
page and 320–339 on the incoming page. Back uses 380–399 and 360–379. Both pages
draw through GX while the frame, navigation and pointer keep their layout.

Each change lasts 20 frames at 60 Hz. Moving controls ignore clicks until the
transition ends, including clicks sent using their old positions. The motion
uses two small joint trees and a bounded import arena. Their animation streams
reference the scene's disc buffer. CSS and practice release these resources
explicitly before retiring their scene heaps; opening the next hub loads them
for that scene. No allocations are made while sampling a transition.

The default follows Windows' client-area animation preference.
`FREAK_MELEE_MENU_MOTION=1` enables motion for a process, and `0` selects instant
page changes. The test harness uses this override for its own isolated clients,
leaving the Windows setting unchanged.

`review_native_hub_motion.py` passes with animated mode in
`.local/native-hub-motion-2` and with `--reduced-motion` in
`.local/native-hub-reduced-motion-1`. Both cover forward/back navigation, profile
editing and return, stale clicks, opening the paused-practice hub, and rebuilding
the hub after returning to CSS. The animated run's first attempt exposed stale
joint pointers across scene resets; explicit cleanup at CSS/practice exit fixed
it. Mid-transition and settled PNGs were reviewed.

The existing authoring check also passes with motion enabled in
`.local/native-hub-motion-authoring-regression-1`: first-run clone creation,
rename replacement, independent copying, frozen Workshop entry/return with CRLF
source preserved, profile save and controller return. The native build passes.

The hand follow-up passes in `.local/native-practice-hand-motion-1`, including
the `hand=1` diagnostic on the compact practice controls before opening the hub,
paused-practice navigation and return to CSS. Reviewed PNGs show the actual hand
in the practice hub. A two-client shared-PC doubles check also passes in
`.local/native-practice-hand-quick-doubles-1`: practice, B-to-unready, countdown
restart, and live session `b380ac738f8a9fc0` in room `8CC800DAE9DF`. It reports
no gameplay failure or checksum mismatch after the scene-owned hand cleanup.

### Earlier control checks

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
multi-PC delivery or the user's final visual acceptance. Native Quick Play queue
checks are recorded in [its checkpoint](native_quick_play_checkpoint.md).

## Remaining integration

Quick Play's countdown and fixed-rule first match now pass native singles and
shared-PC doubles checks. Result agreement and rematches now have local native
automation checks; stage-confirmation controls and service deployment remain
open. Remote portrait preparation
and lobby chat have their own completed local checks. Authored menu transitions
now have native runtime evidence above. Account sign-in and offline inbox
decisions remain deferred for the user.
