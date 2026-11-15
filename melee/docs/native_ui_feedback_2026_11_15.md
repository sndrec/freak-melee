# Hub and character-select feedback

Feedback snapshot: 2026-11-15. There are 19 submissions containing 33 pinned
notes. All were downloaded with their screenshots and diagnostic captures to
`.local/feedback-review-2026-11-15/`. The private captures stay outside Git.

## Goal

Address every note in the submissions below. Build on the original Melee menus,
assets, typography, transitions and controller-hand navigation. Compare changes
against the relevant original screens in the native port. Keep each submission
open until all of its notes are implemented and checked. Then remove that
submission and its attachments from the hosted feedback service, and record the
fix, verification and removal here.

The snapshot below defines this batch. Newly submitted reports can be reviewed
separately without making this batch grow indefinitely. Continue committing and
pushing coherent changes. Client publication follows a separate release request.

## Work order

1. Review the screenshots, pins and captured scene state, then inspect the
   current implementation. The reports were captured on `2026-11-14-public-1`;
   some changes already exist in the current checkout.
2. Separate Quick Play presentation from player-created lobbies. Establish the
   persistent session controls and CSS-owned lobby controls together, so the
   navigation changes remain usable throughout.
3. Complete the CSS card layout, original Back control and portrait presentation.
   Update portrait capture using the actual card and selectable poses.
4. Move ordinary settings into native menus organized by broad categories.
   Complete the standalone News screen and the remaining hub layout fixes.
5. Verify the earlier five-point submission against the current build. Complete
   any remaining points and remove its hosted entry once verified.
6. Finish a combined visual and interaction review. Update every report below
   with its completion evidence and confirm that completed entries are gone.

Ask Twilight before resolving an ambiguous design choice. Twilight confirmed
that Players on this PC belongs in one persistent Controls setting, shared by
lobby and Quick Play setup. Continue independent work while a design question
is pending.

## Session navigation and lobby controls

### `a91be492-7eed-400a-b38b-45c3af0005e8` — persistent session indicator

- [x] Pin 1: Show a clear, persistent session indicator when returning to the
  hub while in a lobby or Quick Play queue. Provide the appropriate Leave lobby
  or Leave queue action and a return action. Use the area beneath the hub frame
  as the proposed starting point. Remove the permanent top-right Character
  select and username buttons, preserving needed navigation through the new
  session controls and appropriate menus.

### `2c36642f-fa8f-4a1a-9be6-3bceb5e7405e` — Quick Play presentation

- [x] Pin 1: Entering Quick Play, including searching from its CSS, must leave
  the Lobbies page showing its ordinary lobby browser. Quick Play should have
  its own visible identity even when shared networking infrastructure is used.

### `cc5d2f06-0fbe-45c2-b42f-998c0ff250b0` — lobby setup and ownership

- [x] Pin 1: Move the listed/unlisted control into a suitable lobby settings
  location, clear of the hub frame.
- [x] Pin 2: Creating a lobby goes directly to character select. Remove the
  redundant Choose fighter and Character select controls from the hub lobby
  page once the persistent return controls are available.
- [x] Pin 3: Put the necessary lobby actions on the lobby CSS or its rules menu,
  accessed through the original top-of-CSS match-rules area. Preserve the
  underlying actions while removing the redundant hub control cluster. The
  rules view belongs to CSS, outside the hub frame; its stock Back control
  returns to CSS. Manual rotation and Match rules live here. The hub keeps
  only its persistent Return to lobby action, with no extra Character Select.
- [x] Pin 4: Remove the redundant subtitle in this position throughout hub
  pages, including "Play together. Share your fighters."
- [x] Pin 5: Put lobby chat and Copy code on the CSS rather than this hub page.

### `d9f9bea8-673f-48af-af6b-845a73c18470` — four lobby cards

- [x] Pin 1: Display four player cards on the lobby CSS, with N/A for unfilled
  slots. Fit the necessary controls and lobby chat alongside them. Start from
  the suggested compact row of cards on the lower left. Twilight's subsequent
  direction places a taller chat panel above the match controls on the right.
  Workshop has no button on CSS. Compare proportions with vanilla and check
  readable window sizes.

### `3645f09b-8b8c-4349-945e-b4016e04c9f8` — browser layout

- [x] Pin 1: Move Players on this PC into one persistent Controls setting,
  shared by lobby and Quick Play setup. Remove the duplicate lobby selectors.
- [x] Pin 2: Move the ticker's right fade boundary farther left to keep emerging
  text within the frame's usable interior.

## Character select and portraits

### `91780dec-6922-4cc1-bea4-58aedfe4c4c5` — original CSS presentation

- [x] Pin 1: Keep the player cards' presentation vanilla for this pass; remove
  the added player-name/status overlays from the cards.
- [x] Pin 2: Match the top CSS heading's size and placement to the original text
  in that region.
- [x] Pin 3: Remove the START to practice text and adjacent lobby code strip.
- [x] Pin 4: Place match controls below the taller chat panel beside the player cards,
  following Twilight's updated direction while this batch was in progress.
- [x] Pin 5: Reuse the original CSS Back graphic at the top right.

### `9feef587-6b44-4359-945a-f9f66d539859` — custom card portraits

- [x] Pin 1: Make custom portraits fill the player card at the same scale as
  stock portraits. Check the capture, saved image and runtime placement paths.

### `38278921-d4e2-4d4d-b6b6-21b41c8c3ae1` — portrait capture

- [x] Pin 1: Make Card size preview the portrait on the actual P1 player-card
  graphic at the appropriate size and proportions.
- [x] Pin 2: Allow selecting an animation and pose/frame for both CSS-tile and
  player-card captures. Apply the original player-card portrait drop shadow to
  the card presentation.

## Native settings and standalone News

### `4a8f3006-0291-465a-ae8e-dfdfdf919fd6` — settings categories

- [x] Pin 1: Organize top-level Settings by broad categories such as Video,
  Audio, Game, Online and Controls. Place the existing settings under the
  appropriate categories, keeping their functionality discoverable.

### `88ed16a2-d996-41e7-bc97-5e02e95ab8a7` — native settings screens

- [ ] Pin 1: Move ordinary settings, including player profiles, gameplay input
  options, bindings and calibration, into the native in-game interface. Use the
  original Options screens as direct design references and support controller
  navigation. Preserve the detailed Workshop's desktop editing interface.

### `b1849c74-ba9f-47ee-a47c-b8b9668ec15c` — News screen

- [x] Pin 1: Open News as its own menu outside the hub frame/navigation, similar
  in separation to First Startup. Preserve article history, reading controls
  and a clear return path.

## Remaining hub layout

### `32b76f1e-cc08-4f2e-8ed3-7dfa9a5f1c20` — Quick Play mode selection

- [x] Pin 1: Replace the two giant yellow mode buttons with large floating
  italic menu-caption text for 1v1 and 2v2, highlighted on hover/selection.

### `10ad546f-a635-4baa-bac2-13de5a4e81f3` — chatroom player names

- [x] Pin 1: Increase player-name text to roughly 1.8 times its current size.
  Present a simple list without the extra outline/background. Compress long
  names horizontally to fit rather than truncating them.

### `112f430f-d764-4fdd-af3b-8cbd73cc7c86` — chatroom status

- [x] Pin 1: Relocate or remove the overlapping Connected status text.

### `72281e7c-a55a-4fa3-9bf3-7a34f2071b3d` — inbox spacing

- [x] Pin 1: Raise the inbox's bottom buttons clear of the frame. Raise the
  letter/unread count into the space between the list and the frame.

### Redundant subtitles — three separate submissions

- [x] `0941b5e8-0448-4933-b1ac-852afc72ffa4`, pin 1: remove Lobbies subtitle.
- [x] `efff1a7e-6b06-40d7-9aaa-08e1aeb15662`, pin 1: remove Workshop subtitle.
- [x] `1e4ec68e-9753-41d8-955a-e4d46e1e4abd`, pin 1: remove Settings subtitle.

Check other hub pages for the same redundant subtitle pattern, as requested in
the five-pin lobby report. Keep useful operation results and error messages.

## Earlier submission to verify and clear

### `162b2c34-722c-48c0-9c95-2a99a2a7f1ae` — five-point hub/F2 report

Current implementation is recorded in `native_news_feedback_plan.md` and commits
`4a28ff8` and `30140f4`. Verify the current paths rather than reimplementing them.

- [x] Pin 1: Bottom description/controller-help panel is removed.
- [x] Pin 2: Ticker has no NEWS badge, scrolls right to left and fades at both
  edges, using the smaller gold italic menu-caption face.
- [x] Pin 3: Confirm that the header font referenced by the pin is the ticker's
  font reference. Preserve that face in the resulting ticker.
- [x] Pin 4: Long F2 notes remain visible and scrollable while editing.
- [x] Pin 5: Button outlines use a separate pass and labels are centered without
  the old atlas-padding offset.

## Verification and report removal

Review related changes in batches. Use targeted checks for behavior and a native
visual review at ordinary and narrow sizes for layout, typography and clipping.
Exercise mouse and controller-hand activation, back/return navigation and held
button consumption. Keep automated client runs hidden and muted. Ask or wait if
a user-owned running client locks the build.

For each submission, record the source commit and the relevant verification.
Keep entries containing unresolved design questions or unverified behavior open.
After all points in a submission are checked, delete its hosted entry using the
existing authenticated administration operation and remove both stored
attachments. Verify that the entry is absent from the service. The existing
deletion operation also supports scheduled cleanup of interrupted object removal.
Keep the local downloaded evidence and this concise completion record while the
batch is in progress.

Complete this goal when all 33 notes have been addressed, the client builds,
the relevant checks and visual review pass, and all 19 completed submissions
have been removed from the hosted feedback list. Summarize changes and any
remaining hardware or remote-player acceptance explicitly.

## Implementation record

### Session navigation and shared hub layout

The first batch adds a persistent hub session strip, removes the top-right
profile/character-select controls, keeps the lobby browser available during
Quick Play, and enters CSS directly on lobby creation/join. Players on this PC
is saved once under Controls and used by lobby and Quick Play setup; existing
sessions keep their current participant count. Profile/binding editing still
uses its existing desktop screen until the native settings migration is done.

Quick Play mode choices use floating italic text. Chatroom player rows use
larger plain names, compress long names to fit, and omit the redundant Connected
label. Redundant page subtitles are removed. The inbox footer is raised and the
ticker's right boundary is inset; those two adjustments still need their own
populated visual review before their submissions can be cleared.

Build: `melee/build/native/melee_native.exe`, successful 2026-11-15.
Evidence: `.local/ui-feedback-review-697ac3/review.json` and its screenshots.
Two hidden, muted clients used an isolated local signaling/chat service. The
review verified persistence across clients, controller A activation of Quick
Play, a 40-frame held A causing exactly one listing toggle, browser presentation
before/during searching, session return/leave and direct CSS entry after lobby
creation. Native screenshots were checked at 1440x900 and 960x540, including a
31-character player name. Hosted removals are recorded below after verification.

The feedback tool now supports explicit single-report deletion with a local
retry receipt. An isolated test verified retry after an interrupted attachment
delete and rejection of attachment keys belonging to another report.

Implementation commit: `744daa6`. The following submissions were fully verified
and removed on 2026-11-15. Each has a successful `removal.json` receipt in its
local capture directory, including the original attachment keys. Both stored
attachments were deleted. A fresh hosted query confirmed all eight IDs absent;
11 submissions remain. See `.local/ui-feedback-removal.log` and
`.local/feedback-review-2026-11-15/remaining.json`.

| Removed submission | Verified result |
| --- | --- |
| `a91be492-7eed-400a-b38b-45c3af0005e8` | Persistent session strip in hub pages, correct return/leave actions and removed top-right controls. |
| `2c36642f-fa8f-4a1a-9be6-3bceb5e7405e` | Ordinary lobby browser while Quick Play is idle and searching. |
| `32b76f1e-cc08-4f2e-8ed3-7dfa9a5f1c20` | Floating italic mode choices, hover highlight and controller A activation. |
| `10ad546f-a635-4baa-bac2-13de5a4e81f3` | Larger plain player names, including a compressed 31-character name at narrow size. |
| `112f430f-d764-4fdd-af3b-8cbd73cc7c86` | Connected chat has no overlapping Connected label; delivery and error statuses remain. |
| `0941b5e8-0448-4933-b1ac-852afc72ffa4` | Lobbies subtitle removed. |
| `efff1a7e-6b06-40d7-9aaa-08e1aeb15662` | Workshop subtitle removed. |
| `1e4ec68e-9753-41d8-955a-e4d46e1e4abd` | Settings subtitle removed. |

### CSS-owned lobby controls and four-card layout

The lobby browser now stays a browser. Lobby controls live on CSS and its rules
view, opened through the original match-rules heading. Stock Back closes the
rules view to CSS; from CSS it returns to the hub. Manual rotation, Match rules,
Listed/Unlisted and participant roles live in that view. The redundant hub
Character Select button is removed. Chat is taller and sits above Ready and
Start match; Workshop is absent from CSS.

Online cards reuse the original card models, with four compact slots at the
lower left and original N/A presentation for empty slots. Player-name/status
labels are removed. The original CSS rules heading and Back artwork are visible
again. Custom player-card portraits now use the stock portrait rectangle;
portrait capture and fresh-image validation remain open under their own reports.
Joining from an already-open CSS also applies the configured local player count.

Verification: `.local/css-feedback-review-7/review.json`, two hidden, muted
clients using the local signaling service. The batch exercised both window
sizes, four slots and local-first order, chat delivery/UTF-8/wrapping, original
header/Back hit regions, modal START and B consumption, host role override and
subsequent player choice, held A causing exactly one listing change, hub return,
and dragging a fighter onto a compact remote card. Native screenshots were
reviewed. CSS layout and participant unit tests passed; the Release client built.

Native settings migration, standalone News, populated inbox/ticker review
and the earlier F2 review remain open.


Implementation commit: `93038d0`. These submissions and both attachments per
submission were removed after verification on 2026-11-15; their local
`removal.json` receipts report success. A fresh hosted query confirmed all three
absent. Eight submissions remain in the fixed batch.

| Removed submission | Verified result |
| --- | --- |
| `cc5d2f06-0fbe-45c2-b42f-998c0ff250b0` | Direct CSS entry, lobby controls inside CSS/rules, removed hub duplicates, CSS chat and Copy code. |
| `d9f9bea8-673f-48af-af6b-845a73c18470` | Four compact authored cards, original N/A slots, chat above match buttons, no Workshop button on CSS. |
| `91780dec-6922-4cc1-bea4-58aedfe4c4c5` | Original heading and Back artwork, removed player/status overlays and extra instruction/code strip, revised button placement. |


### Portrait capture and original card framing

Portrait capture now offers the fighter's animations and an exact frame field
in both modes. Each mode remembers its own animation, frame and camera. A frozen
preview clamps at the last frame. The small card preview uses the original P1
card model, with the same portrait rectangle as CSS. The source archives and
one import arena stay loaded while choosing poses.

Stock portraits were compared directly: their shadow is black, offset down-left,
with a main opacity of 3/7 (109/255). The new card capture uses that presentation.
A depth prepass keeps overlapping meshes from darkening their own shadow, and
both members of a pair draw before the foreground portraits. The shadow changes
RGB only, preserving the texture-alpha calculation. RGBA export stores coverage
alpha and writes straight-alpha PNGs. The export policy is changed only at the
existing drained offscreen-pass boundaries.

The final batch passed in `.local/portrait-feedback-review-6/review.json`.
It verified independent poses, exact frame entry and save-on-blur, held-A frame
stepping, reloading camera records, changed PNG contents and native CSS placement.
The RGBA check confirms a black 109/255 shadow, rejecting colored shadow pixels.
A paired Ice Climbers capture passed as well. Native screenshots were reviewed at
1440x900 and 960x540; `comparison.png` shows the captured image beside extracted
stock portraits. Eight graphics recording/presented-capture checks and both CSS
layout/participant checks passed. The Release client built successfully.

CSS now measures each original costume quad during menu construction and uses
those bounds for custom portraits and the P1 preview. The checked P1 quad is
14.199951 by 18.999756 game units at (-30.192432, -4.400098). The image remains
136x188; the small preview preserves the original quad's slight stretch while
using the capture's camera aspect ratio. No original artwork is modified.


Implementation commit: `e5c8246`. These submissions and both attachments per
submission were removed after final verification on 2026-11-15. Their local
`removal.json` receipts report success; a fresh hosted query confirmed both IDs
absent. Six submissions remain in the fixed batch, covering ten pinned notes.

| Removed submission | Verified result |
| --- | --- |
| `9feef587-6b44-4359-945a-f9f66d539859` | Custom portrait capture, saved 136x188 image, original-quad sizing in preview and CSS. |
| `38278921-d4e2-4d4d-b6b6-21b41c8c3ae1` | Actual P1 preview, separate animation/frame/camera choices, black stock-style shadow and paired capture. |


### Categorized Options, standalone News and remaining hub review

Settings now opens a separate Options page with Video, Audio, Game, Online and
Controls. The original Options idle track selects the gold frame, its point-light
color is taken from mnmain's Options palette, and the title uses the original
Options caption joint. Category buttons use the original width-adjustable
ribbons. Video exposes display mode and VSync; Audio exposes stereo/mono and
music/effects balance, saved per installation. Existing settings are grouped
under their corresponding categories. Players on this PC stays in Controls.

News has its own full-width reader over the original animated menu backdrop,
without the hub frame, navigation ribbons, ticker or session strip. It keeps
article scrolling, links, history, pagination and return to the originating page.
The persisted offline cache covers the headline feed and dismissed notices;
article bodies still require the service after a client restart.

The native profile/input/binding/calibration replacement remains open under
`88ed16a2-d996-41e7-bc97-5e02e95ab8a7`. Profile editing, match-rule editing,
costume management and package management still open their existing desktop
screens from their categories. These are remaining work, not a completed native
settings migration.

Verification, all with isolated hidden and muted clients:

- `.local/settings-feedback-review-2/review.json`: five categories, held A
  toggling once, held B returning one level, audio balance, local-player count,
  return to hub, restart persistence and screenshots at 1440x900 and 960x540.
  The screenshots were compared with the original Options/Rumble/Sound captures
  in `.local/retail-options-reference-1/`. Fullscreen switching was not exercised
  automatically, to avoid disturbing the user's desktop.
- `.local/news-feedback-review-3/review.json`: standalone reader at both sizes,
  article scroll, history pagination, return to Workshop, live feed update,
  notice delivery/dismissal, seen persistence and offline headline cache.
- `.local/footer-feedback-review-2/review.json`: eight populated received-letter
  rows, unread count, next/previous pages, both window sizes and a long ticker
  sampled at multiple offsets. The footer/count sit between the list and frame;
  the ticker fades inside the frame and uses the header's smaller gold face.
  Its local HTTP fixture supplies isolated test account/mail data; this check
  makes no authentication-service claim.
- `.local/five-point-feedback-review-1/review.json` and `note-bubble.png`:
  long note scrolling follows the caret, Home/End navigation, editing after
  resize, pin removal, immutable F2 attachments and upload retry. The final
  line remains visible. Native screenshots confirm removal of the old hub
  description/help bar and centered, readable button lettering. The font
  renderer draws all black coverage before all glyph fills, so neighboring
  outlines cannot cover earlier fills. Earlier implementation commits are
  `30140f4` and `4a28ff8`; this batch verifies their current paths.

The Release client built successfully; the two CSS checks and eight graphics
recording/presented-capture checks passed. The public updater is unchanged.


Implementation commit: `c00eba7`, pushed to main. These five submissions and
both attachments per submission were removed after the checks above on
2026-11-15. Their local `removal.json` receipts report success. A fresh hosted
query confirmed all five IDs absent. Eighteen submissions / 32 pinned notes
are complete; the one native-settings submission remains in the fixed batch.

| Removed submission | Verified result |
| --- | --- |
| `4a8f3006-0291-465a-ae8e-dfdfdf919fd6` | Five top-level Options categories, grouped existing controls, original Options presentation. |
| `b1849c74-ba9f-47ee-a47c-b8b9668ec15c` | Standalone News reader, history/paging/scrolling and return path. |
| `72281e7c-a55a-4fa3-9bf3-7a34f2071b3d` | Populated Inbox footer/count spacing at both reviewed sizes. |
| `3645f09b-8b8c-4349-945e-b4016e04c9f8` | Persistent local-player setting in Controls and ticker fades inset within the frame. |
| `162b2c34-722c-48c0-9c95-2a99a2a7f1ae` | All five earlier hub/font/ticker/F2 points checked on the current client. |


### Native controller profiles and calibration

The Controls page now opens native Player profiles, using the original Options
frame, title and lighting. Saved profiles, naming and port selection sit beside
Gameplay, Buttons, Axes and Calibration tabs. The existing profile store and
input application code are used directly. Save & use, defaults, reset, deletion
confirmation and paginated saved-profile selection remain available. The old
Rml profile implementation, markup and style sheet were removed, along with
its old open/update/close call sites.

Gameplay exposes tap jump, the 0�7-frame action buffer and smash/tilt C-stick.
Bindings support replacement, additional inputs, duplicate rejection, signed
axes, clearing and default axes. Capture waits for neutral, then an input, then
release. The edit is committed on release. Escape, Cancel binding or holding
START cancels without changing the previous binding; a short START press stays
bindable. Percentage fields confirm on Enter or blur and restore the previous
value after invalid input.

The native calibration pages use the existing raw-input calibration backend:
stable rest, edge capture, manual trigger Start/End markers, numerical review
and input test mode. Stick calibration is applied before adjusting the trigger
ranges. Temporary assignments are restored on cancel, exit or controller change;
Save & use remains the explicit persistence step. Controller identity uses the
SDL instance rather than the device's changing list index. This detects a
replacement even if it reuses the old index. Detailed controller settings stay
outside rollback state.

The diagnostic control channel can create up to four owned SDL virtual gamepads,
set their raw axes/buttons and ports, and detach them. They use the ordinary SDL
and Aurora input path; they do not replace calibration samples or its decisions.
The diagnostic pointer down/up path now delivers held mouse state to the hub,
which allows real trigger-handle dragging in automated reviews too.

Verification: `.local/native-profiles-review-3/receipt.json` passed on the Release
client. It covers all tabs, held-A edge behavior, raw SDL button and signed-axis
capture, additional/duplicate bindings, cancellation, Enter-confirmed values,
invalid input, raw rest/edge capture, both Start and End handles on both triggers,
review/test input suppression, START stopping testing, disconnect recovery,
save/reopen and deletion cancellation/confirmation. Screenshots were reviewed at
1440x900 and 960x540. The review uses isolated data and hidden, muted clients.

The same cohort completed three 2,200-sample Ganondorf practice recordings for
default controls, disabled tap jump and tilt C-stick. Full/short jumps and all
three C-stick directions retain their expected behavior. Six profile/provider,
rumble and CSS checks passed in `.local/native-profiles-contracts-final.log`;
the provider test includes stable device identity and replacement. The final
client/control-tool build is `.local/native-profiles-build-7.log`.

Physical GameCube controller feel and physical rumble remain hands-on checks.
Submission `88ed16a2-d996-41e7-bc97-5e02e95ab8a7` stays open: match-rule editing,
costume management and package management still need their native replacements.
Eighteen submissions remain completed and removed; no release was published.

Implementation commit: `72a53c7`, pushed to main. The native-settings report
remains open until the three remaining desktop screens are replaced and checked.


### Native Match rules, Costumes and installed packages

The remaining ordinary desktop panels have been replaced with native Options
pages. Match rules keeps its stock CSS entry point and returns to CSS when
opened there; opening it from Game returns to Game. Numeric fields confirm on
Enter or blur, reject invalid ranges, and retain the prior value. The mode,
timers, stock count, damage ratio, friendly fire, pause, score display, item
frequency and all 31 item switches remain available. Only the lobby host can
change the next match's rules; Quick Play keeps its fixed rules.

Costumes shows the active local players and each selected fighter's compatible
outfits. The default outfit, paged installed choices and DAT import remain
available. Family imports still request every required member model. Selection
uses the same outfit catalog and CSS assignment path as X/Y cycling.

Installed packages has separate installed/draft tabs, paging, refresh, a lobby
recipient page, transfer status, and Workshop entry/return. Sending an outfit
includes its missing fighter dependency through the existing gift flow. The old
Rml panels, styles, handlers and input-ownership checks are removed. The native
pages use the original gold Options frame, caption, buttons and controller hand.

Verification: `.local/native-options-review-5/review.json` passed with isolated,
hidden, muted clients. It covers Enter validation, held-A and held-B behavior,
item switches, rule save/reopen/restart, CSS return, outfit selection/reset,
package/draft paging, disabled sending without recipients, and screenshots at
1440x900 and 960x540. The rule, item, outfit and package screenshots were reviewed.
Old local source-package fixtures were replaced with freshly generated clones
for the outfit checks; no compatibility path was added to the client.

`.local/native-options-gifts-7/receipt.json` passed against an isolated local
signaling service. A guest could not change host rules; Workshop returned to
Packages; accepting an outfit installed the exact outfit and its missing fighter
parent. Repeating the gift kept installed files and CSS layout unchanged. The
shared CSS review helper now counts occupied cards among the four displayed
slots, and the gift review uses the current native recipient page.

All 12 selected settings, profile/provider, rumble, CSS, content-transfer,
costume-package and stage-start checks passed in
`.local/native-ui-feedback-final-contracts.log`. Final build and hosted removal
are recorded below after the completion audit.
