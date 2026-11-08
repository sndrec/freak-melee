# Startup review: implementation and verification

Updated: 2026-11-08. This list records the user's second wizard review and follow-up fixes.
The playable revision is implemented and the native checks pass. Hardware
calibration still needs the user's hands-on pass. Commit locally; do not push.
The next section records the latest feedback; older sections describe their
own checkpoints. Intro shot planning follows this playable revision.

## Account-first flow, coherent capture and separate portraits

Welcome now opens Sign-Up, Login and Continue as Guest, with Recover in the
corner and an explanation of the hovered choice. A signed-in account supplies
its profile and proceeds to controller selection. Guests enter their profile
after choosing Guest. Account management stays in Settings.

The calibration wizard reads all six mapped axes from one locked SDL snapshot.
This prevents a stick pair from combining two different controller reports.
The measured stick mapping applies on the trigger-adjustment page. Test stops
controller-driven hand movement and tilt until START or Stop test is pressed.
The fast-swirl report remains a hardware acceptance item: a coherent read is
implemented, but the physical symptom has not been reproduced on the user's
adapter. Leaving the stick page writes the last 2048 raw samples to
`content/stick-capture.csv` for that attempt. MaxX Throttle's fixed-direction
projection capture was inspected as a reference; Melee retains an outline of
the measured boundary rather than copying MaxX's square gameplay mapping.

Tile naming and camera framing share one page, with a second preview at CSS
size. Zoom changes the orthographic projection around the framed point. Pan
has no artificial position clamp. Ambient lighting now carries the HSD diffuse
contribution flag required for the renderer to apply its ambient color.
The capture retains the original model materials and textures.

New stock clones receive the original player-card image extracted from the
stock CSS material-animation tables. That 136x188 image is separate from the
custom CSS tile. Workshop > Portrait & tile opens the owned fighter's shot
editor, also linked from the detailed editor's portrait controls. Its two
modes save independent images and camera framing. Player-card PNGs and camera
values are ordinary named resources in the fighter's resource bundle.

During placement, the player cards remain visible, the tile stays in the roster
area above them, and the hand draws after the tile.

Build: `.local/startup-feedback-3-build-5.log`. Three input/provider/release
checks pass in `.local/startup-feedback-3-contracts.log`. The combined native
walkthrough passes eight checks and records thirteen captures in
`.local/startup-feedback-3-ui-review/receipt.json`, including account-first
navigation, saved profile resumption, portrait controls, direct placement,
owned-project editing and separate card/tile saves. Three account-notice checks
pass in `.local/startup-feedback-3-notice-review/receipt.json`. These clients
were hidden, muted and isolated. They do not establish hardware calibration
feel or final user acceptance.
After adding the intro's named-animation/costume capture support, the combined
startup walkthrough was repeated on the final executable: eight checks and
thirteen captures pass in `.local/startup-feedback-3-final-review/receipt.json`.

## First batch: implemented and checked

The welcome screen has only the temporary Freak Melee text logo, startup
indicator and Continue. Profile fields use separate positions, the empty name
field has a faded Player example, account-management buttons stay out of the
completed setup account page, and the controller button says Select controller.

The calibration capture stores a 32-direction outline for each stick. The
calibration page draws those outlines; the review page displays scaled axis
values and raw samples. Measurement pages suppress hand motion and menu tilt.
Pre-click trigger capture and adjustment controls are implemented in the next
batch described below; the physical adapter pass remains open.

A render-order error caused stock portraits to reappear even when the saved
layout contained only a custom tile. The CSS now applies its visibility mask
at its draw boundary, after cursor/animation updates. The native capture after
cloning shows one custom Mario tile and no stock roster. Existing saved layouts
are preserved. Missing native-home layouts start empty.

Build: `.local/startup-review-batch-1-build-2.log`. Five focused input/profile
tests pass in `.local/startup-review-batch-1-contracts.log`. The four-check
setup walkthrough passes in `.local/startup-review-batch-1-ui/receipt.json`;
welcome, profile and returned CSS captures were inspected. The walkthrough
skips physical calibration and does not create an online account. Account
error popups and trigger click limits are covered by the next batch. The new
starter flow remains open.

## Account notices and trigger limits

Failed account creation, sign-in and recovery show the service's error message
in a reusable modal notice. The notice slides in from the right and uses the
unchanged stock main-menu panel border and caption lettering. Any new button
press dismisses it, while the underlying page keeps its state. This reuses
existing stock artwork; it does not alter a raster texture or claim to execute
the original unlock-notice scene.

Trigger capture reads independent mapped physical buttons directly from SDL.
Generated analog-threshold presses are excluded. A click-capable trigger stores
the last sample before its first valid click; holding it fully in and releasing
it cannot widen that limit. A trigger without an independent click uses its
full measured travel. The review page displays raw input, normalized percentage
and analog maximum, with minus/plus controls to adjust each maximum.

Build: `.local/startup-review-notice-trigger-build-4.log`. Five focused
input/profile tests pass in `.local/startup-review-notice-trigger-contracts.log`,
including the real SDL provider's distinction between a physical click and an
emulated trigger press. The capture test checks held-click travel, release and
the full-travel fallback. The native notice walkthrough records its screenshot
and three checks in `.local/startup-review-notice-trigger-final-ui/receipt.json`:
an HTTP 409 error appears, holding B dismisses once, and a click over Continue
does not pass through the notice. It uses an isolated localhost HTTP fixture
and no existing account data. Physical pre-click capture and adjustment still
need the user's controller test.

## Starter carousel

Startup now shows a three-position carousel with the stock costumes and idle
animations. It offers 24 starter families, leaving Kirby out. Popo and Nana
appear together; Zelda's clone still includes Sheik. The display names include
Ganondorf, Dr. Mario, Ice Climbers and Mr. Game & Watch. The starter page has
no naming field, and the extra editable-copy sentence has been removed.

Menu previews own their imported costumes and idle tracks. They retain three
nearby choices, retire replaced models and release all previews on leaving the
page. The preview maps animation nodes around reserved bones, expands the
idle's subactions for mesh visibility and starts with the stock costume-zero
visibility values from each fighter's spawn setup. It executes no fighter
gameplay callbacks. The flat-colour presentation accepts a palette and depth;
Game & Watch's values match PlGw.dat's black body, white outline and 0.01 depth.
The disc models and textures remain unchanged.

The carousel selection feeds confirmation and portrait framing, described
below. The later sections record shorthand text, direct placement and the
default-roster shortcut. User acceptance of the presentation remains open.
The intro remains a separate storyboard discussion after the playable flow.

The 24-choice load/lifetime review is recorded in
`.local/startup-carousel-review-4/receipt.json`, with a captured overview.
All 24 choices load; Back releases the models and reopening rebuilds them.
The four-check setup/resume/clone walkthrough also passes in
`.local/startup-carousel-flow-review/receipt.json`. These checks establish
loading and navigation. The captures still show that large heads and the
paired Ice Climbers need better framing; do not treat the load checks as proof
that every preview's presentation is finished.
Build: `.local/startup-carousel-build-6.log`. The final flat-preview capture is
`.local/startup-carousel-final/receipt.json`. Its body and outline share the
source depth scale, and the outline leaves the depth buffer unchanged. Facing
and precise framing remain part of the camera/portrait work.

## Portrait capture foundation

Preview framing now measures the visible posed mesh, including skinning and
shape animation. It uses the same vertex reader as the Model workspace. The
flat style also supplies its facing before the depth scale, so the silhouette
keeps its profile. Paired previews have room for both members.

The capture backend renders a square camera view into a transparent target and
writes a 256-by-256 RGBA PNG. It supports orbit, tilt, zoom, pan and a chosen
animation frame. A capture can contain multiple subjects, including both Ice
Climbers. Captures wait for the shader variants they need before saving. The
PNG writer is shared with ordinary screenshots, which keep their RGB output.

This backend is available through the native inspection channel for isolated
checks. The user-facing portrait controls, shorthand label and direct CSS
placement still need connecting to the startup flow. The goal remains active.

Build: `.local/startup-portrait-foundation-build-7.log`. Six focused model and
input/profile tests pass in `.local/startup-portrait-contracts.log`. The isolated
review in `.local/startup-portrait-cold-review/receipt.json` starts with an empty
renderer cache and saves all 24 choices as nonempty transparent RGBA portraits,
including both Ice Climbers. The portrait overview and restored carousel screen
were inspected. This checks the export backend, not the unfinished wizard flow.

Portrait controls follow the Trophy Gallery: C-stick orbits, A + main stick
or the D-pad pans, and X/Y zoom. The source is `_Toy_80309404` in `ty/toy.c`.
The main stick continues moving the hand when A is released. Mouse controls are
middle-drag orbit, Shift + middle-drag pan and wheel zoom.

## Portrait page: implementation in progress

Choosing a starter now opens a confirmation page before portrait framing. The
portrait page displays a frozen model in the same square view used by PNG
capture. It accepts the Gallery controls above, with A + stick panning while the
hand is over the picture. Holding A there consumes hand movement and button
activation. C-stick input moves the portrait camera and leaves the page still.
Reset view restores the original framing. Use portrait saves the image and
copies it into the independent fighter draft before installation.

The combined native walkthrough in
`.local/startup-portrait-ui-review/receipt.json` passes five checks: profile
setup, interruption/resume, Gallery camera controls with hand consumption,
portrait installation, and completed-setup restart. Its controller inputs use
the native injection channel. Six focused model/input/profile tests pass in
`.local/startup-portrait-ui-contracts.log`. The confirmation, portrait and final
one-tile CSS screenshots were inspected. The saved draft portrait matches the
captured RGBA PNG. Mouse-camera handling still needs a hands-on check.
The final build after removing the old clone-finish entry point is recorded in
`.local/startup-portrait-ui-build-4.log`; it completes successfully.

## Tile label and placement

After portrait capture, the player writes a short tile name beside a large stamp
preview. This name is saved separately from the fighter's full name, retained
when the fighter is renamed, and included in shared packages. An empty package
tile name uses the full title. The Workshop's tile list and original CSS use the
short name; fighter headings keep the full name.

Continuing installs the completed fighter without adding a tile. The next page
shows the actual CSS and a tile following the Melee hand. The player drops it
with A or a click. Back cancels the preview without saving a layout change.
Dropping commits the chosen position and finishes startup directly on CSS.
The placement controls and hand use the CSS's camera and transforms. The menu
keeps its regular hand/token state while startup owns input.

Build: `.local/startup-stamp-build-5.log`. The hidden, muted native walkthrough
passes six checks in `.local/startup-stamp-ui-review-3/receipt.json`, including
mouse/stick placement, cancellation, the exact saved coordinates, a separate
short name, one installed fighter and completed-setup restart. The stamp-entry,
placement and returned CSS screenshots were inspected. The packet and layout
contracts pass in `.local/startup-stamp-packet-contracts.log` and
`.local/startup-stamp-layout-contracts.log`.

The real asset-family review passes 118 checks across three family modes and
both stock pairs in
`A:/freak-melee-tests/startup-stamp-family-20261107-2/receipt.json`. It verifies
the short name through build, package import, project reopen, identical rebuild
and a full-title rename. It also runs per-member source validation. This checks
authoring and transport; paired gameplay was not exercised by this review.

## Native tile presentation and default roster

The original Mario CSS tile supplies the reusable outer frame. Its 684 border
pixels are unchanged; the portrait and name interior are cleared for new
content. The extracted name strip's dark red is used behind the user's angular
glyphs. Two-line stock portraits are cropped above their baked names, including
Game & Watch. Transparent portrait backgrounds remain transparent.
The pixel comparison is recorded in
`.local/native-css-art-reference/receipt.json`.

Portrait framing, the short-name page, the Workshop and CSS share that tile
presentation. The camera window matches its portrait area. Saved startup
portraits are 464 by 304 pixels for a single-line name, or 464 by 248 for two
lines. This replaces the earlier square presentation while preserving the
general capture tool's square default. Ready/START, Workshop and Back are in
the CSS footer, and the hub has a visible Character select return button.

Workshop > Add Melee roster shows a confirmation with empty and full CSS
previews rendered from the original menu models. Confirming builds 24 editable
fighter families, excludes Kirby, and replaces the CSS with the original tile
positions. Existing fighters stay installed and editable. Stock abbreviations
such as D K and C. Falcon are separate tile names; full fighter names remain
unchanged.

The shortcut records its original package IDs. Repeating it reuses those
fighters. If a default was edited and published, restoring defaults imports the
original package as a separate editable fighter and keeps the edited one. A
failed build leaves the previous CSS layout intact and retains any completed
fighters in the Workshop.

The combined wizard review passes six checks and records eleven screenshots in
`.local/startup-final-ui-review-2/receipt.json`. It covers setup/resume, Gallery
camera controls and input consumption, portrait export, short names, actual CSS
placement/cancellation, completion and reopening from Settings. Portrait and
final CSS captures were inspected. It skips physical controller measurement.

The default-roster review passes four combined checks covering cancellation, all 24 source fighters, retaining
the initial fighter, restoring an edited default and repeating without extra
drafts or installed packages. These are menu/content checks; they do not play
matches with each generated fighter. The receipt is
`.local/default-roster-final-review/receipt.json`; the final build completes in
`.local/startup-defaults-build-10.log`. The confirmation and final full-roster
captures were inspected. The final screenshot also verifies that CSS status
text clears the portrait row and the two stock abbreviations fit one line.

A fresh-data launcher is available at
`.local/startup-revision-playtest/Launch Startup Review.cmd`. It uses the current
native build and creates separate user data under `runs` on every launch,
preserving earlier runs. The tracked launcher source is
`melee/tools/launch_startup_review.cmd`. User-launched reviews play audio;
automated native checks stay hidden and muted.

Physical pre-click calibration, mouse-camera feel and visual/usability
acceptance remain for the user's hands-on pass. The intro remains deferred to
the joint storyboard discussion.

## Follow-up: stick boundaries, manual trigger ranges and music

The user found inaccurate stick outlines and released triggers reading above
zero. The outline now keeps a bounded convex boundary of actual input samples,
with the same measured center used for its live dot. It no longer projects
samples onto the centers of angular buckets. Returning to the center leaves
the outer boundary intact.

Trigger capture has large bars with draggable Start and End markers. The
measured release and pre-click values provide suggestions. Start defines zero;
End defines full analog travel. Both endpoints remain adjustable during review
and are saved in the controller profile. The hand and mouse both drag them.
The release check uses those chosen thresholds. Trigger measurement updates
only trigger ranges, so moving the hand preserves completed stick measurements.

The supplied mode-select FLAC is converted to looping stereo HPS. The regular
music stream plays it during setup and returns to menu music on completion.
Native disc assets use Aurora's normal mounted-file interface; conversion
tools are used only when authoring the file. See [native disc assets](native_disc_assets.md).

Hardware feel and released-trigger readings still require the user's next pass.
The build completes in `.local/startup-calibration-music-build-3.log`. Three
focused input/provider/release tests pass in
`.local/startup-calibration-contracts.log`, including actual octagonal corner
coordinates, returning to center, manual trigger thresholds and separation of
stick and trigger capture. The six-check native startup/resume/finish review
passes in `.local/startup-calibration-music-ui-review/receipt.json`. It confirms
the mounted music asset and completes the wizard with hidden, muted clients;
it skips physical calibration and does not establish audible playback quality
or hands-on marker dragging.

## Screens and messages

- Welcome contains only Freak Melee, FIRST STARTUP with four progress dots,
  and Continue. Use text for the logo until the real logo is designed.
- Give the pages distinct compositions. Use larger captions and buttons;
  display name and bio may occupy opposite sides. Keep the progress indicator.
- A fresh display-name field is empty. Show a faded Player example while it is
  unfocused. Clicking it starts with empty text.
- Remove the account page's Optional paragraph. After signing in and saving
  recovery information, show the identity and Continue. Account management stays
  in Settings.
- Show failed account requests, including an occupied name, in a reusable
  dismissible notification using the original unlock-notice presentation.
  It slides in from the side and consumes the dismissing button press.

## Controller calibration

- Rename the controller confirmation button to Select controller.
- Suppress stick-driven cursor movement and menu tilt during measurements.
- Show the measured stick outlines and numerical readings during review.
- Capture a trigger's analog maximum from the last sample before its physical
  digital click, with a slow squeeze and later numerical adjustment. Controllers
  without a digital click use full analog travel. Distinguish physical clicks
  from generated button thresholds.

## First fighter and CSS

- Replace the starter's button grid with a rotating carousel inspired by early
  DDR selection screens and Melee's trophy gallery. Show standing idle models,
  proper fighter names and short descriptions. Keep paired fighters together.
- Confirm the base fighter before cloning. Do not ask for a new fighter name in
  this selection step.
- Then frame the CSS portrait with a camera, write the short tile text, and
  place the tile directly on the actual CSS.
- A fresh CSS begins empty and contains only the tiles the player places.
  Investigate the stock portraits shown after the previous wizard run, including
  render-time visibility and saved-layout behavior. Preserve existing layouts
  unless the user explicitly replaces them.
- Offer a secondary Fill CSS with Melee Defaults action. Confirm with the exact
  question: Populate the Character Select Screen with the default SSBM roster?
  Show empty and populated CSS previews with an arrow between them.
- That shortcut creates editable clones and reproduces the original tiles as
  closely as possible. Leave Kirby out while his scripted clone is deferred.
- On confirmation, use the default CSS arrangement. Keep the player's existing
  fighters in the Workshop; the shortcut replaces their layout, not their files.
- Revisit the CSS's Ready/START, Workshop and Back placement as part of the
  placement/finish flow.

## Intro: first storyboard after the playable flow

The [intro storyboard](native_intro_storyboard.md) and its tracked shot recipe
now arrange these beats into a 62-second caption-led study. Eleven native
pose/costume captures produce nine boards and a timed animatic in
`.local/intro-storyboard-5/`. Frames decoded directly from `MvHowto.mth` inform
the large white caption treatment. The native staging channel loads named
animations and original costume variants, without altering their source assets.
Actual edited gameplay, custom outfits and the appearance-filter shot remain
to produce. The next review can address timing and composition:

1. A fighter starts as a faithful copy. Show editing hitboxes, animation and
   scripts, with gameplay from cinematic camera angles.
2. Show online sharing and the unrestricted possibilities: an overpowered
   low-percent KO and a censored appearance example.
3. Show disabling custom appearances: a censored fighter becomes a neutral
   wireframe based on hurtboxes, then falls asleep. Implement the appearance
   filter before presenting it as an available setting.
4. Show several Falco outfits dropping in and taunting. Appearance changes are
   shared even when moves stay unchanged.
5. End with Be prepared for anything in Freak Melee.
