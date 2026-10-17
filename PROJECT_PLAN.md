# Freak Melee: current project plan

Updated: 2026-10-17. This is the current product contract and work list.
The [documentation index](melee/docs/README.md) links implementation details,
authoring specifications and dated evidence. Historical checkpoints describe
their own builds; their old "remaining" lists are not the current backlog.

## Current direction

- **Glim** is ready for review in the Workshop and installed character catalog.
  The [fighter guide](characters/glim/README.md) covers its moves and rebuild
  recipe; [authoring observations](characters/glim/WORKSHOP_NOTES.md) record
  the UI checks, fixed bugs and remaining gaps. The follow-up authoring controls
  are ready for the user's next dogfooding pass.
  Save coherent progress in local Git commits. The active goal forbids upstream
  pushes. Keep disc-derived assets and generated packages local.
- Continue the native client and character-authoring platform.
- Optimization is deferred at the user's request. Preserve the current
  interpreter, confirmed-checkpoint rollback and GPU-free reconstruction;
  do not continue performance experiments without a new request.
- Current focus: integrate the approved social-hub mockup into the native game.
  Build the personal roster, cloning, lobby and sharing flows through the game renderer. Follow the
  [social hub design](melee/docs/native_social_hub_design.md), including the
  mode → CSS → ready practice → countdown flow. Workshop
  dogfooding continues when the user is ready. This document does not change
  the app's goal control.
- The user approved native integration and delegated routine decisions while
  away. Keep unresolved product and account-service decisions for their return.
  Track verified native behavior separately from the browser mockup. Keep the
  existing Workshop workspaces usable throughout the transition.
- Native hub checkpoints now cover first-run cloning and collection edits,
  the stock Melee hand, ISO-extracted starter portraits and a live global-chat
  player directory. Local hidden/muted authoring acceptance and two-client
  chat presence checks passed. The design document records remaining CSS,
  matchmaking and account/inbox work.
  The hand now has an independent practice-scene import, with native practice,
  scene-return and Quick Play doubles checks in the play-menu checkpoint.
- Native CSS now presents active lobby seats with local players first, using
  stock card artwork. Physical inputs and canonical match ports stay intact.
  Portrait dragging opens fighter-delivery confirmation. Same-lobby transfers
  use separate content connections and reach waiting players and spectators,
  including while they practice. Native CSS/practice controls now replace the
  room rails and preserve access to the detailed tools. Selected remote fighters
  now download to the cache before readying, refreshing portraits without adding
  roster entries. The Quick Play party allocator is now tested locally for solo
  and premade queue assembly, cancellation and assignment recovery. Native queue
  controls, open player cards and identity-preserving handoff now work against
  the local service, including ready practice and shared-PC pairs. The
  three-second countdown, B-to-unready and fixed-rule match launch now pass
  native singles and four-player doubles checks. Result agreement, per-seat
  loser stage votes and same-opponent rematches now pass service and native
  automation checks. The stage-confirmation UI and service deployment remain
  open. Late countdown cancellation now waits for a service decision, and native
  opponent-exit checks recover to the original party and search again.
  Signaling-loss checks now finish the P2P match and enter a second match after
  recovering both parties. Native rematches also pass with separate-client
  premade teammates and four independent solo clients. See the
  [rematch checkpoint](melee/docs/native_quick_rematch_checkpoint.md),
  [return checkpoint](melee/docs/native_quick_return_checkpoint.md) and the
  [CSS checkpoint](melee/docs/native_css_cards_checkpoint.md).
- The hub now uses the stock main menu's authored 20-frame enter/exit animations.
  Forward/back navigation, profile return, reduced motion and CSS/practice scene
  reconstruction pass native checks. The detailed Workshop-entry regression
  also passes with motion enabled. See the
  [play-menu checkpoint](melee/docs/native_play_menu_checkpoint.md).
- Ordinary scripts run in the native interpreter. Generated C is confined to
  reference comparisons, not ordinary authoring, sharing or execution.
- Work that requires other physical PCs is deferred to the user's testing.
  Local feature completion is not blocked on recruiting testers.
- Kirby cloning/copy definitions and whole-move transfer remain deferred.

## Product contract

Build a private Windows x86-64 Melee client for friends by modifying the full
existing decompilation in place. Aurora supplies the platform and rendering.
Do not reconstruct the game by moving selected functions into another engine.
Versus is the initial focus; support up to four active fighters, including
shared PCs, CPUs, teams, free-for-all, items and stage hazards. Single-player
extras are deferred, not a reason to delete unrelated source.

Preserve player-visible gameplay and visuals under default controls. Native
pointers, compiler details, SDK services and data layouts may change. Do not
silently rebalance moves or timing. Document intentional fixes to undefined or
bugged stock behavior in scripts and comparison notes, including Luigi's
uninitialized Cyclone charge and Ness's lingering yo-yo hitbox. Behavioral
parity is acceptable where forcing original private state or RNG would require
awkward hacks; deterministic agreement between native clients remains required.

Keep the game projection 4:3. The planned in-game social hub replaces the
default native sidebars. Keep chat and room controls accessible during play;
their presentation is part of the upcoming mockup review. Window layout must
not alter gameplay visibility, blast zones or magnifying-glass damage.
Trust Aurora as the rendering foundation. Inspect changed UI and obvious
discrepancies; do not require exhaustive pixel comparison of unchanged scenes.

The supplied USA v1.02 ISO is a local asset source and is never shipped in
Git or releases. Existing DAT assets receive typed native imports. Embedded
PowerPC code in m-ex DATs is not a supported executable format. Port available
source; reverse engineer source-unavailable characters individually if needed.
HSDLib, m-ex, Slippi and other cloned tools are references, not runtime build
dependencies. Native GLB/glTF import uses permissively licensed cgltf.

## Current implementation and acceptance

| Area | Implemented baseline | Remaining acceptance or implementation |
| --- | --- | --- |
| Native versus | Full decomp builds and runs; original fighter/stage assets, gameplay, audio, menus, complete matches and results. All 29 selectable stages have recorded runtime coverage. | Rare fighter/item/stage combinations and failures reported during play. Foundation M0 and original-versus M1 are accepted. |
| Fighter scripting | Direct typed frontend, bytecode VM, generic engine bindings, owned parameters/state, articles/effects, callbacks and lifetimes. All 26 supported forms prepare, excluding Kirby. | Specific untested behavior/lifetime branches and interactive clone workflows; do not restart wholesale translation. |
| Rollback | Admitted mutable arenas and external roots, confirmed checkpoints, input history and GPU-free reconstruction; interpreted owner state and audio/effect reconciliation are retained. | Broader scene/fighter coverage and eventual multi-PC acceptance. Further optimization is deferred. |
| Workshop | Action, Scripting, Animation and Model workspaces, draft ownership, source validation, frozen preview, practice testing and packaging. | Model gaps and collaborative usability acceptance below. |
| Rooms/social | Native hub and authored page transitions, first-run personal roster and cloning, participant CSS cards, ready practice, host start, global/lobby chat, player directories, shared-PC inputs, spectator/rotation foundations, fighter delivery and early remote portraits. Fixed-rule Quick Play and rematches have local native evidence. | Quick Play stage-confirmation UI and deployment, lobby browsing, persistent accounts/friends, email-style inbox, invitations and collaborative presentation acceptance. Physical multi-PC testing remains deferred to the user. |
| Friend release | Setup/update ZIPs, isolated user data, update activation and rollback to the previous release. | Complete Workshop payload, fresh-clone/fresh-install checks, current content tests and presentation polish. |

### Fighter evidence

There are 22 single-fighter templates plus Zelda, Sheik, Popo and Nana.
All 26 forms pass normal source preparation; this is not proof of every move
branch. Recorded stock/source comparisons cover:

- All 22 single-fighter CPU cases and the paired-family CPU/transform cases.
- 176 single-fighter ground/air special entries, plus selected charge,
  direction, follow-through and interaction scenarios.
- Two-source counter/reflect/capture/throw interactions with independent state.
- 100 selected result outcomes across 25 bases / 26 forms.
- Samus's authored grapple/resource and selected interruption/respawn paths.

Some sets combine initial runs with corrected reruns rather than one final
sealed whole-roster run. Pixels/audio and exhaustive lifetime branches remain
outside those receipts. See the [roster status](melee/docs/SCRIPT_PORT_CHECKPOINT.md).
Rook has source-owned behavior and snacks with actual interpreted gameplay
evidence; preserve her project and leave deliberately unimplemented behavior
alone. Do not call every recipe, billboard or interactive edit accepted merely
because a package prepares.

### Rollback evidence and deferred performance

Production saves only the newest reconstructed contiguous fully confirmed
boundary, not each predicted or resimulated tick. Admission happens at scene
construction; saves/restores copy sealed mutable spans without page tracking.
The broad page mechanism and old history ring are diagnostic references.
Immutable assets/bytecode remain resident. Saved state includes authored
variables, callbacks, object identities, allocation order and live relationships.

Hidden reconstruction retains CPU work needed for faithful state: poses,
animation/material changes, callback/display-side writes, RNG and particle
spawn/age/removal. It omits GPU geometry/output. Wrongly predicted particles
must disappear after restoration; actual resimulated effects must remain.
Audio intents reconcile against the live playback timeline, and physical rumble
does not repeat for hidden ticks. Network/UI/device state stays outside rollback.

Latest committed full-path local cohorts measured about 11.82 ms median and
14.23-15.14 ms peak for fifteen reconstructed ticks, checkpoint work and one
normal rendered submission. The requested 10 ms median / <=12 ms measured peak
is not met and remains deferred. These are active wall times excluding explicit
pacing sleeps, two clients sharing one PC/GPU, not GPU-completion measurements.
They do not establish timing on friends' machines. See
[current rollback architecture](melee/docs/native_rollback_current.md).

## Required remaining work

### 1. Model workspace completion

Retain the approved [Model specification](melee/docs/native_model_workspace_design.md)
and [Workshop mockup](melee/docs/native_character_workshop_ui.md). This is mesh
import, materials, rigging and skinning, not mesh modeling.

- [ ] Import compatible source skin weights instead of requiring rigid or
  generated bindings for every imported mesh.
- [ ] Add/edit extra bones while updating owned rig facts, hierarchy, animation,
  skin/attachment references and gameplay bone roles coherently.
- [ ] Preserve rigid attachments as well as skinned meshes during rest-rig edits.
- [ ] Reimport by source identity, retaining project-owned materials and weights
  unless the author explicitly replaces them.
- [ ] Extend painting across the supported editable mesh set and remove storage
  limits that prevent useful authored models; do not introduce tick allocation.
- [ ] Exercise an asymmetric imported mesh through Edit/Pose/Animation modes,
  brush edits, save/reopen/undo and Test Character.

Keep a compatible Melee rig and semantic bone mapping; arbitrary imported
armatures are deferred. Engine-controlled helper/root bones retain their native
runtime roles. Edit Mode changes rest/bind data without baking the temporary
pose into it. Pose Mode uses temporary offsets; Animation Mode plays a clip.
Do not promise animation retargeting merely from inverse-bind compensation.

### 2. Collaborative Workshop acceptance

The first recording review is implemented for the next user pass: automatic
uninstalled draft preview and live Test Character, full-window draft navigation,
Collection draft rows, labelled resources and input/dropdown styling, the full
97-field common-attribute schema, typed collapsible parameter values with source
defaults/overrides, syntax coloring and bundled symbol reference. The new labels
and custom portraits inside original menus now use scene GX rendering and
context-specific glyph atlases. These changes still need the user's usability
and typography assessment; the broader acceptance tasks below remain open.


The [second recording feedback list](melee/docs/native_workshop_feedback_2026_10_13.md)
is implemented and ready for the user's next review. The pass addresses its
crash, editing interactions, contextual labels, material/visibility gaps and
typography. Pause after the focused checks and local commits. Model mode uses
editor eye visibility alone; Animation and Test Character use authored
visibility. IK remains future authoring work.

Follow-up action editing adds [duration-owned hitbox intervals](melee/docs/native_hitbox_intervals.md):
moving bars carries their endpoints, edges resize them, and adjacent intervals
offer Refresh Hit Eligibility. Static lifetimes are converted when a draft
opens; unresolved runtime-dependent sequences remain available in the event list.

Glim dogfooding additionally fixed imported bone-attached mesh draw passes,
timed hitbox flag ownership, parameter-default display and hitbox distance
fields. The example's four specials, two air jumps and authored aerials ran
through a finite live gameplay exercise; charged Flash and Skip hit Sandbag.
Projectile reflection and wall bounce still need broader play. The example
does not close the collaborative acceptance checklist below.

The follow-up [asset authoring pass](melee/docs/native_workshop_asset_authoring.md)
adds independent clip creation/duplication, transactional multi-mesh import,
persistent editor names and action hiding, and replacement of individual images
in animated material sequences. These controls stay within the existing
workspaces. Editing a new clip preserves the target action's events; hidden
actions retain their gameplay slots. User dogfooding remains the acceptance
step.

- [ ] Complete clone -> edit -> preview/test -> save/reopen -> install with the
  user, preserving the source fighter and keeping preview/cache/install distinct.
- [ ] Review the real Action Editor timeline, loop unrolling, simultaneous
  hitboxes, contextual inspector, scrubbing and viewport picking.
- [ ] Review per-move script files, editable fighter-state declarations and
  named parameter UI, source diagnostics and interrupted behavior.
- [ ] Review shared animation curves, hierarchy, pose copying, keys/tangents,
  playback, root motion and attachment edits through the native evaluator.
- [ ] Verify Blender animation interchange and preservation of untouched tracks.
- [ ] Finish paired-member preview presentation; show the chosen member while
  editing and run the complete family during Test Character.
- [ ] Exercise undo/recovery and coherent multi-file saves; claim only the
  failure/recovery boundaries actually tested.

Do not invent a different layout without first discussing the reason with the
user. Test Character runs normal controls in practice with Sandbag; frozen
editing uses a character-centered void preview without gameplay HUD. Frozen
camera controls: middle-drag orbit, Shift+middle-drag pan, wheel zoom.
Bone transforms use G/R/S, axis/plane constraints, commit/cancel and a stable
starting pose. Auto Key starts off; explicit insertion remains available.

### 3. Current content and room integration

- [ ] Recheck source-only single/paired fighters and edited costumes/resources
  through isolated stores, room auto-cache and explicit gifting.
- [ ] Confirm a recipient can use/edit installed content without the author's
  source tree, SDK, original draft directory or behavior DLL.
- [ ] Recheck current match setup, results/return, seat choices/host override,
  1v1 rotation and spectator catch-up with the new runtime.
- [ ] Confirm downloads/spectator/chat activity does not stall active matches.
- [ ] Verify current scripted object creation/destruction and custom edits
  survive corrected inputs; use targeted batches rather than re-proving the
  entire roster after every small change.

Existing room/content features are not assumed absent. Their older compiled-C
receipts are historical; the purpose is closing integrated current-backend gaps.

### 4. Local release readiness and presentation

- [ ] Include `native_content_tool.exe` in release payloads: current Workshop
  validation invokes it beside the client, but preparation currently omits it.
- [ ] Verify source selection/configuration from a fresh source checkout with
  explicit ISO and DSP-coefficient inputs; no hidden reference-folder dependency.
- [ ] Verify a clean extracted install, launch, Workshop check/build/install,
  update, rollback and preservation of profiles/collections/CSS layout.
- [ ] Replace placeholder original-style UI text with reusable Melee-style
  dynamic lettering; derive glyphs/metrics from available textures if needed.
- [ ] Finish labels/icons, clipping, usable layouts and actionable errors with
  the user. Preserve 4:3 scaling during resizing/fullscreen.
- [ ] Recheck profile binding/calibration and default-control equivalence,
  optional tap-jump/short-hop/attack-stick behavior and controller feedback.

### 5. Later external acceptance

Actual friend PCs, shared-PC physical controllers, separate networks, direct
and relayed routes, signaling loss, jitter/loss bursts and install usability
remain release gates. They are deferred from the local goal phase. Do not call
same-PC processes proof of these conditions. Optimization has its separate
deferred target above; it does not prevent working on other features.

## Persistent design decisions

Profiles own bindings and calibration. Assignment is explicit, not based on
unique controller IDs. Sample normalized inputs once per simulation frame;
match-relevant settings are pinned in setup. Preserve analog shields and proper
Z/Grab behavior. Default controls reproduce Melee; optional conveniences are
explicit changes, not stick modifications that accidentally alter DI.

Collections and the editable CSS layout belong to the installation. Cached
dependencies render/simulate but are not selectable. Explicit gifting/acceptance
installs them permanently, including dependencies. Each player chooses only
installed content and everyone sees that chosen costume. Exchange stable package
identities, not grid positions. The WYSIWYG CSS edits real portrait joints in
place; wider left tools temporarily hide the right rail and move the 4:3 canvas.

The planned first launch offers a starter clone for an empty personal roster.
The social hub leads to Lobbies, Quick Play and character selection. START in CSS
enters local grid practice for up to four players, with a separate Sandbag. The normal
pause/exit chord returns to character select; ordinary B remains Special.
Local match and online room flows use the locked practice characters. Once
assigned fighters are ready, the host's Start match control opens stage select;
no automatic start or second CSS. Preserve the current home flow during design.

Players choose fighting/waiting/spectating and the host can override; players
can change their choice again. Teams/FFA assignments are manual. 1v1 supports
friendly winner/loser rotation policies. Up to four active fighters, extra room
members spectate. Active-player disconnect aborts the match to the lobby.
Global chat spans everyone with the client open, independently of room presence.
Room chat has its own conversation. Private messages are email-style letters
in the inbox, available to any recipient. Fighter sharing has its own confirmation
and requires both players to be in the same lobby. Place the CSS selection cookie,
then carry a fighter tile to another player's card, or use Roster's Send fighter
command. Accepted fighters join the roster; match downloads remain cached.
Friends can send lobby invitations. The contextual player list is visible only
with global or room chat; Friends has its own menu.
The planned hub replaces the default sidebars. Chat and room controls remain
accessible during play; Workshop, Options and management retain dedicated space.
All simple menus use the Melee hand for controller and optional mouse navigation,
with keyboard input for typing. The detailed Workshop editors keep their editing
controls. Workshop and Settings are full-size main-menu destinations. Workshop
prominently offers fighter editing, cloning and importing. Player lists are
compact, and player names open full profile pages with editable bios and rotating
menu transitions.

Quick Play has 1v1 and 2v2 queues. Doubles accepts solos and premade pairs on
one PC or separate PCs. The landing page shows only the two mode choices;
choosing one goes straight to CSS. Show occupied/open participant cards and a
separate Enter queue command. START can ready a chosen fighter into practice
before or after queueing. Once every required player is connected and ready,
run a three-second countdown; B unreadies and cancels it. Both modes use four
stocks, six minutes, no items, and
all six stages: Battlefield, Final Destination, Fountain of Dreams, Dream Land,
Pokémon Stadium and Yoshi's Story. The first stage is random; the losing side
picks subsequent stages. Opponents stay together until someone leaves. These
queues and fixed rules now have native local checks. Result agreement and
rematches also have native checks using stage-vote automation. The player-facing
stage-confirmation controls and their full controller/mouse acceptance remain
open. Late cancellation and opponent-exit party recovery have local native
checks. The interactive mockup uses local sample opponents.

CSS tile labels use the supplied angular glyph set, fill the name bar vertically
and compress horizontally. Ganondorf remains on one line. Truly long names can
use taller multiline bars. Roster icons stay compact.

Correct the reported native GameCube-adapter stick response after a hardware
capture. The source audit found an extra 8000-unit deadzone and SDL-to-PAD scaling
to 127 before Melee's 80-unit clamp. Follow the input audit in the social hub
design; the browser mockup's hand response is a separate path. The selection
cookie should be placeable anywhere in CSS.

Connectivity is direct-first ICE with signaling and a relay when NAT requires
it. Avoid requiring port forwarding. Losing signaling must not terminate an
established viable peer connection. Updates use prepared complete releases,
exact build matching and previous-version recovery; never replace user data.

Independent fighter packages own data, rigs, clips, states, scripts, tuning,
articles and resources. Donor identity is editor provenance, not a live behavior
dependency. State variables and named typed parameters belong to the project.
Script extensions must be reusable engine primitives, never a fighter/move-
specific shortcut or a call to a stock special handler. Special decisions remain
authored even when their general physics/collision primitives are native C.
Keep documentation linked and author-facing in the style of the scripting guide.
Follow [documentation guidelines](melee/docs/documentation_guidelines.md): explain
what a feature does, its inputs and results, and when to use it in plain language.
Keep implementation history and test evidence in developer notes.

## Working rules and upstream

Private upstream: [sndrec/freak-melee](https://github.com/sndrec/freak-melee).
The initial source commit preserves pre-cleanup docs and selected build inputs.
Original nested `melee`/`aurora` histories and their donor remotes remain local;
the root repository tracks source files, not Git submodule pointers. Make
explicit scoped commits. The user's 2026-10-13 request authorizes this source
backup and documentation publication; future unrelated changes remain local
unless a later instruction authorizes pushing them. Never push to donor repos.

Preserve unrelated dirty work. Avoid compatibility layers, dead implementations
and forwarding wrappers. Organize code by responsibility; prefer fixed pools,
cache-friendly data and allocation at state transitions. Read the required
[Multi-Core By Default](https://www.dgtlgrove.com/p/multi-core-by-default)
article before introducing threaded code. New script
validation belongs at construction where possible; preserve runtime lifetime
barriers across callback/reentry boundaries.

Run meaningful checks for changed behavior; no lengthy repetitive game launches
for documentation changes or reversible UI tweaks. Routine test launches remain
hidden and muted and use the native input channel instead of desktop keypresses.
When done with a task-owned Dolphin process, force-kill it: normal close can
leave a CPU-spinning process. Commit coherent work regularly.

Screenshots include the complete UI and use PNG. Keep bulky test archives out
of source. C: currently has more space than A:; inspect free space before choosing
an output drive. Preserve recordings that reproduce unresolved bugs.

## Reading order

1. [Build and launch](melee/docs/native_build.md).
2. [Documentation index](melee/docs/README.md) and
   [current Workshop capabilities](melee/docs/native_character_workshop_progress.md).
3. [Approved Workshop UI](melee/docs/native_character_workshop_ui.md) and
   [Model design](melee/docs/native_model_workspace_design.md).
4. [Scripting guide](melee/docs/scripting/index.md),
   [roster evidence](melee/docs/SCRIPT_PORT_CHECKPOINT.md) and
   [interpreter contract](melee/docs/native_interpreter_transition.md).
5. [Rollback architecture](melee/docs/native_rollback_current.md) and
   [checkpoint evidence index](melee/docs/checkpoint_index.md).
