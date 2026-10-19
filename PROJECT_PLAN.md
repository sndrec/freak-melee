# Freak Melee: current project plan

Updated: 2026-10-19. This is the current product contract and work list.
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
- Current focus: integrate the social hub by adapting Melee's existing menu
  scenes, models, textures, materials and authored behavior. The mockup guides
  layout and flow; it is not a separate interface to reproduce from scratch.
  Build flexible hub components from edited stock menu assets,
  retaining the working roster, lobby, sharing and matchmaking logic. Follow the
  [social hub design](melee/docs/native_social_hub_design.md), including the
  mode → CSS → ready practice → countdown flow. Workshop
  dogfooding continues when the user is ready. This document does not change
  the app's goal control.
- Justify every asset change against a specific base-game reference. Compare
  before and after with that reference and make the result closer to Melee's
  existing artwork. Preserve intentional shapes, colors, gradients, borders
  and proportions, including the navigation ribbon's interior cutout. A detail
  looking unusual is not evidence that it is wrong.
- Keep work on the hub until its implementation is complete. Remaining Model
  workspace work is deferred; its current backend is saved in the
  [texture-basis checkpoint](melee/docs/native_texture_basis_checkpoint.md).
- The native Lobbies page now has named listed/unlisted lobbies, live seat
  counts, browsing and joining by row. The host can change listing visibility.
  The [browser checkpoint](melee/docs/native_lobby_browser_checkpoint.md) records
  local verification.
- Account sign-in uses an automatically generated Ed25519 account key. The user
  rejected Windows security-key prompts and approved local sign-in, portable
  key-file export/import and recovery codes. The native client saves the key
  before registration/recovery finishes, retains it on intentional sign-out,
  and can issue independently revocable keys for additional PCs. The service
  stores public keys and hashed session/recovery secrets. See the
  [account contract](melee/docs/native_account_auth.md) for details.
  The new service suite passes 68 tests; six real native/service checks cover
  key generation, interoperability, restart, import/export, revocation and recovery.
  The native build, six account-screen checks and hosted two-client invitation
  review pass. Earlier Windows passkey acceptance is superseded by this design.
  The user confirmed account creation on their PC. Standard file-picker
  interaction and presentation remain for user review.
- The private signaling service now includes account keys, social pages,
  lobby browsing and Quick Play: deployment
  `c59460c2-3719-49ab-9093-2db550ed5bf4` at the existing private endpoint.
  The account-key native path is verified against that hosted service.
- Persistent friends and offline letters now have account-owned mailboxes,
  shared friendship records, read/delete state, blocking and durable delivery
  retries. The native Friends, profile, composer, Inbox and Sent pages use the
  existing menu components. The [social account guide](melee/docs/native_social_accounts.md)
  records behavior and verification. Existing friend/letter checks and the
  current 68-test service suite cover persistent social behavior.
- Chat and lobby sockets now carry verified account identities. Player-list
  clicks open account profiles; friends can send offline lobby invitations,
  and joining checks live membership, friendship and expiry. Sign-out leaves
  the lobby and reconnects global chat as a guest. Four native contracts and
  two hidden/muted clients pass the account-key hosted path, including four
  invitation UI checks and seven PNGs:
  `.local/account-key-hosted-1/receipt.json`. Six account-screen checks cover
  in-game creation and recovery entry: `.local/account-key-ui-1/receipt.json`.
  Standard file-picker interaction and the user's presentation/input review
  remain open. Signing in after entering a lobby as a guest currently requires
  leaving and rejoining that lobby to attach the account.
- Friends now offer Create & invite when the sender has no lobby. The existing
  form keeps name/listing/local-player controls and shows the recipient. B
  returns to their profile without delivery; confirmation sends once the
  service confirms membership and the lobby name. Five invitation UI checks
  and the full local social review pass:
  `.local/create-invite-social-1/receipt.json`.
- Friends and account profiles now show Online/Offline/Checking from verified
  player snapshots and current lobby membership. Four native contracts and
  seven local invitation/presence UI checks pass, including sign-out to a guest
  with the same displayed name: `.local/social-presence-review-1/receipt.json`.
- Background social refresh preserves account-name typing. Removing the last
  contact or letter on a later page returns to the last populated page. The
  current build passes four contracts, five social UI checks, seven invitation/
  presence checks and four editing/pagination checks. Evidence and fixture
  limitations are in the [social account guide](melee/docs/native_social_accounts.md).
- Access to chat during a real match remains deferred to a collaborative design
  pass at the user's request. Preserve the current CSS/practice chat flow.
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
  automation checks. The native six-stage page now supports losing-player picks
  and teammate confirmation, including shared-PC and separate-client doubles.
  The user's visual review remains open. Late countdown
  cancellation now waits for a service decision, and native
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
| Rooms/social | Native hub and authored page transitions, first-run personal roster and cloning, participant CSS cards, ready practice, host start, global/lobby chat, player directories, shared-PC inputs, spectator/rotation foundations, fighter delivery and early remote portraits. Listed/unlisted lobby browsing, account-key sign-in, verified player profiles, friends, offline letters and lobby invitations have local native evidence. Fixed-rule Quick Play and rematches have local native evidence. | File-picker and presentation/input acceptance, contextual typography and collaborative presentation acceptance. Physical multi-PC testing remains deferred to the user. |
| Friend release | Complete Workshop payload, fresh-source build, clean extracted-install authoring, setup/update ZIPs, isolated user data, update activation and rollback with data preserved. | Current content tests, friend-PC acceptance and presentation polish. |

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

### Menu asset integration (current priority)

- [x] Audit the procedural hub against the stock menu construction code and
  record the user's asset-reuse requirement in the hub specification.
- [x] Load the actual menu background, frame, navigation, camera and lights,
  retaining their materials and animations. Use separate hub and retail archive
  ownership. Preserve ribbon ends while resizing the middle; extract reusable
  panel borders and apply them to lobby controls. Native scene-return checks
  pass. See the [menu asset checkpoint](melee/docs/native_menu_assets_checkpoint.md).
- [x] Share the extracted border, source colors and button gradients across hub,
  CSS and practice controls, fighter-delivery notices and Quick Play countdowns.
  Keep the shared artwork independent of main-menu scene ownership.
- [ ] Obtain the user's visual review of the adapted hub/CSS controls.
- [x] Recover separate menu-caption and ribbon alphabets from stock textures,
  retain the notice/results/tile faces, and place page captions through their
  original joint and material. Native navigation and full first-launch authoring
  checks pass with the new captions.
- [ ] Review reconstructed glyphs and adapt the remaining CSS control labels to
  their surrounding components.
- [x] Move the hub's content through scene anchors/cameras and keep hand hit
  testing aligned with the edited components. Original C-stick orbit, projected
  clicks and stationary mouse positions pass across seven camera poses and
  three window sizes; transitions block clicks until the controls settle.
- [x] Recheck CSS/practice/editor scene lifetimes, two-client caching/gifts and
  shared-PC Quick Play doubles with the adapted controls. Tests pass against
  the local service. Flow tests do not substitute for asset/style acceptance.

### 1. Model workspace completion

Retain the approved [Model specification](melee/docs/native_model_workspace_design.md)
and [Workshop mockup](melee/docs/native_character_workshop_ui.md). This is mesh
import, materials, rigging and skinning, not mesh modeling.

- [x] Import compatible source skin weights instead of requiring rigid or
  generated bindings for every imported mesh. Named bones, mapped hierarchy and
  relative bind poses are checked before import; authored floating-point weights
  survive save/reopen and render in the native client. See the
  [source-skin checkpoint](melee/docs/native_source_skin_checkpoint.md).
- [x] Add/edit extra bones while updating owned rig facts, hierarchy, animation,
  skin/attachment references and gameplay bone roles coherently. Skeleton/Edit
  now exposes Add child, Change parent and Remove for authored extra bones.
  Stable bone IDs retain script references, and the coordinated transaction
  updates animation banks, physical references, metal visibility and alternate
  poses. Backend checks cover all 26 supported stock fighter definitions plus
  Glim. Native UI checks cover complete-asset, byte-exact Undo/Redo, retained
  temporary poses through reordering, painting onto an added bone, creating its
  animation channels, Save/reopen without the source GLB, and Test Character.
  Later data changes guard structural Undo; Action/Animation history guards its
  rig revision. See the [extra-bone checkpoint](melee/docs/native_extra_bone_checkpoint.md).
- [x] Finish rest-edit acceptance for rigid attachments and skinned polygons.
  Compensation now handles HSD single-bone envelopes and shared vertex data;
  648 geometry cases pass. Native Save/Undo/Redo and practice checks pass, but
  the current viewport comparison now has zero changed pixels after fixing
  an unintended physics tick during refresh. See the
  [rest-edit checkpoint](melee/docs/native_model_rest_checkpoint.md).
- [x] Add rest-edit compensation for morph targets. Complete-shape blends and
  additive offsets retain their surfaces through rest translation, rotation and
  scale. The combined 1,008-case geometry suite and both native Model workflows
  pass; both viewport comparisons have zero changed pixels. See the
  [rest-edit checkpoint](melee/docs/native_model_rest_checkpoint.md).
- [x] Reimport by source identity, retaining project-owned materials and weights
  unless the author explicitly replaces them.
  [Source tracking and matching](melee/docs/native_gltf_reimport_checkpoint.md)
  now retain document/mesh/material-slot identities through export reordering
  and editor renaming. Native transaction and metadata checks pass, as does
  source-only import Undo/Redo and reopen in the native client. Weight-preserving
  apply, independent material replacement and per-mesh scales pass 409 native
  checks and saved-project cases. Source links and assets now save together,
  including failure recovery and metadata-only history. The existing import
  dialog now handles selected/whole-source reimport with per-mesh scales and
  separate material/binding choices. Native checks pass painted/shared data,
  topology rejection/transfer, byte-exact Undo/Redo, reopen without the source,
  and Test Character. [Rest edits and reimport](melee/docs/native_gltf_rest_checkpoint.md)
  now retain attachment coordinate corrections and publish them with Model
  history. See the checkpoints for receipts and limits.
- [ ] Extend painting across the supported editable mesh set and remove storage
  limits that prevent useful authored models; do not introduce tick allocation.
  Imported meshes now retain up to 16 floating-point influences through painting
  and replacement, with proportional normalization and atomic capacity rejection.
  [Saved DAT and live viewport checks](melee/docs/native_skin_weights_checkpoint.md)
  pass. [Stock-layout painting](melee/docs/native_stock_skin_checkpoint.md) now
  passes the 130-costume batch (8,149 meshes), 227 brush cases and a live Yoshi
  mouse/Save/Undo/Redo check. [Rigid attachment painting](melee/docs/native_rigid_skin_checkpoint.md)
  now passes 52 matrix cases, the retail batch and a native mouse/Save/Undo/Redo/
  reimport workflow. Two-slot rigid meshes retain root/deform-bone domains.
  [Independent texture attachments](melee/docs/native_texture_skin_checkpoint.md)
  now retain their weights through ordinary/morph painting and compaction,
  including when position weights temporarily match them. The current native
  suites and retail batch pass. A reflected-texture fixture is pixel-identical
  after rewriting at two native poses; an incorrect attachment changes 12,944
  pixels. [Separate position and texture tables](melee/docs/native_separate_envelopes_checkpoint.md)
  now remove the shared ten-slot restriction, including painting a triangle with
  nine fixed texture attachments and several position bindings. Each table retains
  its own ten-slot draw limit. Differing texture normal bases and unusual helper
  model-node domains remain open.
  [Morph surface evaluation](melee/docs/native_morph_view_checkpoint.md) now
  shares one runtime evaluator between drawing, picking, highlighting and
  posed-corner queries. Complete/additive native hover reviews pass. The
  [Weighted morph runtime and rest edits](melee/docs/native_morph_skin_checkpoint.md)
  now preserve one controller across private envelope batches, compaction and
  save/load. Posed native drawing, picking and highlighting agree. The
  [Morph painting](melee/docs/native_morph_paint_checkpoint.md) now preserves
  targets, normal seams and controller indices through saved strokes and
  native Undo/Redo. [GLB morph import and named reimport](melee/docs/native_gltf_morph_checkpoint.md)
  now retain position/normal deltas, signed defaults and target slots across
  export reordering. Native painting, history, rest and snapshot checks pass.
  [Larger morph meshes](melee/docs/native_morph_capacity_checkpoint.md) now use
  the full 65,536-entry logical index range, including NBT triples, without
  enlarging rollback state. [32-bit target indices](melee/docs/native_morph_wide_indices_checkpoint.md)
  now let full-sized expressions exceed the former shared physical-index range.
  Per-controller logical limits and additional tangent/UV/color channels remain open in the
  [authoring design](melee/docs/native_morph_skin_design.md).
  [Custom costume import storage](melee/docs/native_costume_capacity_checkpoint.md)
  now grows during asset construction instead of rejecting every graph above
  2,048 records. Compaction follows the same capacity policy. Match-time
  fighter/item capacities are unchanged. HSD scene storage can commit more
  pages during construction for larger imported models.
  [Painting across large draw sections](melee/docs/native_large_skin_checkpoint.md)
  now uses local position arrays and saved vertex identities. Large native
  meshes retain shared vertices through painting, rest compensation and
  compaction. [Large GLB/glTF import](melee/docs/native_large_gltf_checkpoint.md)
  now admits up to one million source vertices/corners, partitions geometry
  into ordinary draw sections and retains source correspondence through Save,
  painting and reimport. Dense skin construction also uses iterative polygon
  loading and bounded, stable HSD storage growth.
  [Costume compaction](melee/docs/native_costume_compaction_checkpoint.md) now
  reclaims dead geometry/texture storage while preserving editor identities;
  128 consecutive brush edits stay bounded, all retail costumes pass, and both
  native morph rest-edit comparisons retain zero changed pixels. Unknown/opaque
  extensions retain their data until their layout is described.
  The [stock geometry inventory](melee/docs/native_stock_skin_inventory.md)
  now covers all 130 retail costume archives and identifies shared position
  indices, attribute seams and root-envelope transforms the brush must preserve.
- [x] Exercise an asymmetric imported mesh through Edit/Pose/Animation modes,
  brush edits, save/reopen/undo and Test Character.
  Native posed/animated painting, saved weights, Undo/Redo, rest edits, relaunch
  without the GLB and practice pass. The strict preview comparison also passes
  after correcting quaternion/Euler handling in the Workshop's live pose API.
  See the [workflow checkpoint](melee/docs/native_model_workflow_checkpoint.md).

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
- [x] Show the chosen member while editing paired fighters, holding both
  companion actors, and run the complete family during Test Character.
  [Native family checks](melee/docs/native_family_preview_checkpoint.md) cover
  member switching, frozen frames/positions, live processing and editor return.
- [ ] Exercise undo/recovery and coherent multi-file saves; claim only the
  failure/recovery boundaries actually tested.

Do not invent a different layout without first discussing the reason with the
user. Test Character runs normal controls in practice with Sandbag; frozen
editing uses a character-centered void preview without gameplay HUD. Frozen
camera controls: middle-drag orbit, Shift+middle-drag pan, wheel zoom.
Bone transforms use G/R/S, axis/plane constraints, commit/cancel and a stable
starting pose. Auto Key starts off; explicit insertion remains available.

### 3. Current content and room integration

- [x] Recheck source-only single/paired fighters with embedded costume textures,
  copied animations and owned resources through isolated stores, room auto-cache
  and explicit gifting. Zelda/Sheik, Popo/Nana and Fox pass the
  [family content batch](melee/docs/native_family_content_checkpoint.md).
- [x] Recheck separately packaged costumes and their parent-fighter dependencies
  through caching, explicit acceptance and current match setup.
  One outfit package coordinates every member of a fighter family. X/Y cycles
  installed outfits on CSS, and the player card shows the outfit name. Import
  and management stay in Costumes. Explicit outfit acceptance installs its exact
  cached fighter parent; see the
  [dependency checkpoint](melee/docs/native_costume_dependency_checkpoint.md).
  Local coordinated imports, both-member runtime loading, X/Y selection and
  CSS-to-practice checks pass for Zelda/Sheik, Popo/Nana and Fox in the
  [outfit checkpoint](melee/docs/native_family_outfit_checkpoint.md).
  [Missing-parent outfit gifts](melee/docs/native_outfit_gift_checkpoint.md) now
  fetch and validate both packages, install them on acceptance and add the
  parent to the roster. [Current room matches](melee/docs/native_outfit_match_checkpoint.md)
  pass remote caching, exact per-slot models, Zelda's form change, rollback,
  Results and return to CSS, including four Ice Climbers slots on two shared PCs.
- [x] Confirm a recipient can edit received content without the author's draft
  directory, SDK or behavior DLL. The earlier
  [single-fighter release check](melee/docs/native_received_fighter_authoring_checkpoint.md)
  covers two Mario packages. The current family batch covers all five members
  above, preserving package IDs on unchanged rebuilds and returning from live
  practice to the editor with the original author offline.
- [x] Recheck current match setup, results/return, seat choices/host override
  and 1v1 rotation with the new runtime. The
  [shared-PC promotion check](melee/docs/native_room_rotation_checkpoint.md)
  covers both rotation policies, player self-reassignment after host override,
  waiting-controller isolation and the promoted seat's gameplay/Results input
  in a second live match.
- [ ] Implement and verify spectator match viewing and live catch-up. On a PC
  with no active fighters, keep local practice running until the player chooses
  Watch match. The [confirmed input history](melee/docs/native_match_stream_checkpoint.md)
  records the seed, exact match packages and corrected inputs. The
  [three-client transport check](melee/docs/native_spectator_transport_checkpoint.md)
  verifies explicit requests, cancellation/rejoin and final history delivery
  during Results while the spectator keeps practicing. The
  [cold-cache check](melee/docs/native_spectator_content_checkpoint.md) verifies
  late downloads of exact fighter families and coordinated outfits without
  installing them or rereading source packages during gameplay. The
  [playback check](melee/docs/native_spectator_playback_checkpoint.md) covers the
  Watch match control, viewer asset activation, verified catch-up, cancellation
  and Results with source-only paired fighters, including a cold-cache run
  through 5,982 verified frames and return to the local roster. A separate
  one-minute tie verifies both sudden-death phases through Results. The
  [spectating-host backend](melee/docs/native_spectator_owner_checkpoint.md)
  now publishes active-player proposals without configuring the host as a
  fighter; two successive sessions and input transport pass. The
  [lobby stage-selection flow](melee/docs/native_lobby_stage_checkpoint.md)
  also passes a native match with a spectating owner and shared-PC stage input.
  The previous loser receives the next pick, with lowest-active-index fallback.
  Filling the existing
  transfer window reduces the cold-cache setup from 89.297 to 28.781 seconds;
  the sampled active clients maintain about 60 ticks/s without added input waits.
  Concurrent content, chat and two viewers also pass the local load check below.
- [x] Confirm downloads/spectator/chat activity does not stall active matches.
  The [concurrent-load check](melee/docs/native_auxiliary_load_checkpoint.md)
  covers two cold-cache viewers and private chat during both a paired-family
  match and four Ice Climbers slots on two shared PCs. Sampled active tick rates
  remain about 60 ticks/s with no added input waits; both viewers verify the
  full match and return home. Physical multi-PC testing remains deferred.
- [x] Verify current scripted object creation/destruction and custom edits
  survive corrected inputs; use targeted batches rather than re-proving the
  entire roster after every small change. The
  [edited-item check](melee/docs/native_script_item_rollback_checkpoint.md)
  covers two pull/throw/expiry cycles with received source-only Fox and Zelda
  packages, new object identities after removal, live corrections and matching
  peer checksums through Results.
- [x] Prepare Pokemon Stadium transformation assets before retained rollback
  history. A debugger trace identifies the mid-match fragment import as the
  retained-history OS heap allocation abort. Preload all four forms, activate
  resident assets during transitions, and retire their storage at scene exit.
  Active archive registrations are copied gameplay state. Local native checks
  cover rock -> normal -> fire -> normal, reconstruction with matching peer
  checksums, and another Stadium match after Results and scene reset.
  See the [allocation investigation](melee/docs/native_stadium_allocation_checkpoint.md).

Existing room/content features are not assumed absent. Their older compiled-C
receipts are historical; the purpose is closing integrated current-backend gaps.

### 4. Local release readiness and presentation

- [x] Include `native_content_tool.exe` and its runtime dependencies in release
  payloads. CMake supplies the exact DLL/UI lists; test fixtures and stale build
  assets are excluded. See [local release acceptance](melee/docs/native_release.md).
- [x] Verify source selection/configuration and a full native build from a
  tracked-source-only export of `eec1d55`, with explicit ISO/DSP inputs and its
  own dependency downloads. Its extracted release also passes Workshop
  check/build/install. No local reference repositories or old build are used.
- [x] Verify a clean extracted install, launch, Workshop check/build/install,
  update, rollback and preservation of profiles/collections/CSS layout.
  The 2026-10-17 local acceptance used bundled tools, a minimal PATH, isolated
  user data and local release manifests. This is not a friend-PC acceptance.
- [ ] Replace placeholder original-style UI text with reusable Melee-style
  dynamic lettering; derive glyphs/metrics from available textures if needed.
  Reconstructed glyphs and the remaining CSS labels need contextual visual review.
- [x] Expand the hub's ASCII coverage and decode UTF-8 for accented names and
  messages. All seven native faces have accented Latin text and common
  punctuation; precomposed/combining forms share glyphs and measurements.
  Native editing/save/return and atlas checks pass, preserving ASCII artwork
  and spacing. International text shaping remains future work.
- [ ] Finish labels/icons, clipping, usable layouts and actionable errors with
  the user. Preserve 4:3 scaling during resizing/fullscreen.
- [ ] Recheck profile binding/calibration and default-control equivalence,
  optional tap-jump/short-hop/attack-stick behavior and controller feedback.
  The [local controls recheck](melee/docs/native_profile_controls_checkpoint.md)
  covers saved profile UI, all button masks on four canonical ports, optional
  jump/attack-stick behavior in gameplay and rumble suppression. Custom Shield
  bindings now retain full strength alongside mapped analog triggers. Raw-device
  rebinding, physical adapter response and motor feedback still need hands-on review.
  [Port calibration](melee/docs/native_profile_hotplug_checkpoint.md) now survives
  connection, replacement and physical port changes through the actual SDL/Aurora
  provider, preserving each device's saved settings.

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
In player-created lobbies, the previous loser chooses the stage, or the
lowest-index player on the losing team. With no previous loser, or when that
player leaves or spectates, the lowest-index active player chooses. These are
canonical match indices, independent of the CSS's local-first card order.

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
The hub replaces the default sidebars. Chat and room controls are available
from CSS and practice; match-time chat awaits collaborative design. Workshop,
Options and management retain dedicated space.
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
stage-confirmation controls have local controller/mouse checks; the user's
presentation review remains open. Late cancellation and opponent-exit party
recovery have local native checks. The interactive mockup uses local sample
opponents.

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
