# Freak Melee â€” project contract and implementation plan

Updated: 2026-10-10

## Approved execution direction (2026-10-07)

Ordinary authored fighter, article and effect behavior will run in the native
interpreter. Replace generated C and ordinary behavior DLLs with direct `.fms`
validation, typed bytecode and generic engine bindings. Preserve the existing
language, templates, engine scheduling and approved Workshop UI. Handwritten
native extensions remain an explicit advanced facility, not an ordinary-script
fallback. Migrate call sites properly and remove the superseded production path.

Replace broad heap/page capture with explicit audited mutable match snapshots;
retain the broad mechanism as a development correctness reference during the
transition. Immutable assets and interpreter code are shared, while object
lifetime, linked references, scheduling and authored variables restore together.
Persistent page rearming is only a possible interim optimization.

The real eight-fighter interpreter evaluation reproduced all 64 semantic endpoints
and observed authored function counts with zero VM errors. The observed difference
was about 1.3 ms of simulation across 15 replay ticks. Both complete runtimes still
miss the 16.667 ms target; page tracking dominates, and repeated drawing remains
measured work. Sub-100-microsecond save/load is a desired target, not established
evidence. Do not treat backend selection as a performance acceptance claim.

Implementation scope, numerical/lifetime contracts, source-only sharing and
remaining gates: `melee/docs/native_interpreter_transition.md`. Ordinary source
package loading now uses the direct frontend, typed bytecode and interpreter;
the evaluation's C-AST frontend and C callback ingress remain temporary reference
test tooling. Historical generated-C evidence below remains historical evidence,
not the operative backend direction.

Current integration status (2026-10-09): ten full retained sources prepare
successfully, and simultaneous Popo/Nana packages now publish independently
owned members through the existing family transaction. A real hidden, muted,
rendered match reached 360 input samples with eight live source actors, no faults
and each Nana's history target set to its own Popo. This verifies startup/common
ticking, not complete linked-special gameplay. Source-owned Workshop member and
special-action preview fixes build and pass an actual Popo/Nana dropdown/pose
smoke; the updated focused fixture passes 2,106 checks. Constructor/teardown
ownership checkpoints and current companion/Workshop receipts are in tracked
`melee/docs/native_source_*_checkpoint.md` documents.

The next connected comparison fixes Gumstrings' owner-vs-held-item relation and
matches both Popo and Nana against stock in all four grounded special scenarios.
Eight hidden/muted rendered games reach 360 samples each with no script faults;
both actors' action timelines and all traced Fighter fields match, as do the
ice shot, Blizzard and rope traces. This does not close interrupted/airborne
specials, injury, separation, partner death or match-end acceptance. Evidence:
`melee/docs/native_source_companion_specials_checkpoint.md`.

Compact whole-match save/load remains unfinished. Regions/Pages is still the
broad correctness oracle, and the rollback performance target has not passed.
Remaining stock-source migration and whole-fighter behavioral acceptance stay in
scope. The dated records below describe their historical boundaries; newer
checkpoints supersede older pending/rejected statements. The full goal is active.

Fighter-owner/transition checkpoint, 2026-10-07: actual normal/demo allocation now
uses stable occupied-slot snapshots for Fighter and its fixed satellites; genuine
shared extra-list consumers remain separate. Source-only owned move transitions,
generic sound/joint operations and constant parameter defaults are implemented.
Client/tool builds and 27 focused checks pass. Eight-actor gameplay matched prior
diagnostics and all 64 broad-oracle rollback endpoints reproduced; 173.91 ms still
misses the deadline. Animation/effect/script/global owners and complete snapshot
backend replacement remain. Ordinary source-only package/Workshop loading and the
remaining bindings/template setup have not yet migrated. See tracked transition,
presentation and Fighter ownership evidence; the full goal remains active.

Motion/primitive checkpoint, 2026-10-07: validated code IDs now run at actual
motion/special entry sites; shared generic combat/status/effect/RNG operations and
978 typed constants are implemented. Native client/tool builds and 25 focused
checks pass. The live eight-fighter probe delivered per-state scripts and all 64
rollback endpoints reproduced; mean 168.31 ms still misses the deadline. Source
proof removed unused archive buffers, reducing partial live allocation by exactly
524,288 bytes; the real interpolation graph remains. Fighter ownership/satellite
snapshot migration and source-only Workshop/package loading remain unfinished.
See tracked motion/combat/ownership references; no full-template acceptance is
claimed. The broad goal remains active.

Callback/import checkpoint, 2026-10-07: source-owned instance callback IDs now
run at actual damage, death, hitlag, dealt-hit and state-change sites. The live
four-Popo/eight-fighter probe completed 2,000 ticks per script owner, and all 64
15-tick rollback endpoints reproduced with callback registrations/variables
included. The 170.36 ms mean still misses the deadline and uses the broad oracle.
Typed import providers copy initial DAT values into named owned parameters at
load; full Workshop/package metadata loading remains open. The real-source GM
scene audit passed 13 controlled transitions with actual object pools; live
assets/UI/async transition acceptance remains separate. See tracked live-loading,
setup-binding and scene-lifetime documents for exact boundaries and receipts.

Direct-runtime checkpoint, 2026-10-07: source declaration indexing, typed IR,
direct bytecode lowering, fixed-pool interpreted execution and the first actual
native property/math binding family now build in the client. All 33 example
sources pass declaration indexing; body/native-family and gameplay acceptance
remain separate. The native client/tool build and 11 focused checks pass without
opening a game. Workshop/package loading and live scheduling have not migrated
yet. Explicit snapshot utilities restore occupied payload, slot generations and
free ordering; actual HSD consumers and whole-game snapshot replacement remain
pending. See `melee/docs/native_script_frontend.md` and
`melee/docs/native_match_pool_first_consumer.md` for evidence and ownership scope.

Owned-state/event checkpoint, 2026-10-07: declaration-owned state and immutable
parameter tables now execute in the direct VM fixture and reproduce a 64-tick
endpoint after restore. Actual event-signature/default-return metadata covers
fighter, article, item and effect contexts with audited read-only and return
policies. Storage/event fixtures pass 713/301 checks; native client/tool builds
and 13 focused CTests pass. Live scheduler, payload families and Workshop/package
activation remain pending. Mutable trail outputs are explicitly unsupported.

Native entry checkpoint, 2026-10-07: immutable code/ABI, real actor liveness,
typed instance storage and scoped payloads now execute together in the native
entry fixture. General animation/motion/physics bindings preserve source-backed
numeric and lifecycle behavior. The native client/tool and 17 focused CTests
pass. An untimed live eight-fighter census completed 2,000 Battlefield samples
with no topology errors; its partial 1,388,376-byte payload estimate is not full
mutable state or a performance result. Live scheduler/Workshop/package migration
and explicit whole-game snapshots remain pending. Evidence and next steps:
`melee/docs/native_script_frontend.md` and `melee/docs/native_match_object_census.md`.

Live lifecycle checkpoint, 2026-10-07: transactional compiler banks and actual
load/tick/removal sites now support source-only interpreted programs, with correct
respawn identity rebinding. Four Popo actors each completed 2,000 VM ticks with
expected owned state, and all 64 actual eight-fighter rollback windows reproduced
complete endpoints. Mean175.53ms still misses the deadline; the page oracle
remains. Native GObj/proc storage now uses process-reserved scene-epoch pools,
with actual-source lifecycle/exhaustion tests and a matching live census. Native
builds and 20 focused checks pass. Complete callbacks/entity/metadata closure,
Workshop/package activation and remaining explicit mutable owners are open;
see `melee/docs/native_script_live_loading.md` and pool consumer proposal.

Current priority, revised 2026-10-07: integrate direct interpreted fighter
behavior and explicit match snapshots, preserving the vanilla fighter authoring
work. Move every fighter-specific versus decision into readable scripts,
including article/item and lifecycle behavior; keep reusable engine operations
in C. Tuning and persistent variables belong to the authored fighter. Compare
translated paths with stock and document evidence and limits. New API extensions
must remain generic. Kirby cloning/custom copy abilities are deferred; stock
Kirby remains playable. DK's selected move, CPU and costume comparisons now
have evidence in `melee/docs/donkey_script_translation.md`; continue the wider
roster rather than requiring an exhaustive DK matrix before the next fighter.
Bowser's checkpoints are recorded in `melee/docs/bowser_script_translation.md`;
all four specials, Flame, capture/throw and identified fighter-wide setup,
item/knockback and CPU decisions are now authored. Six CPU and four common
throw/held-item stock/script comparisons match, alongside the prior special
comparisons. Generic APIs cover CPU candidates/threats, floor probes and
pre-throw victim-pose selection. The build and 34 focused checks pass; Rook
remains verified with unchanged source and one replacement installation.
Broader Bowser interaction acceptance remains open. Continue the remaining
roster; Kirby cloning stays deferred. Use hidden windows for runtime tests.

Pikachu/Pichu checkpoint, 2026-09-29: both have independent asset bases and a
Workshop script template. Skull Bash's charge, launch, hit response, recovery
and transitions are authored using existing generic operations and owned
parameters. Fourteen 700-sample stock/script comparisons match, including
both facings, aerial starts, full charge, slopes, real contact and Pichu's
self-damage. The build and 35 focused checks pass. A metadata audit also
corrected Bowser's clone audio bank to stock bank 15. See
`melee/docs/pikachu_pichu_script_translation.md` for evidence and limitations.
Next: Thunder Jolt, Quick Attack/Agility, Thunder and their articles, then
remaining Pikachu/Pichu lifecycle/common/CPU behavior. These are partial
bases, not accepted whole-fighter clones. Tests remain hidden with drawing on.

Bowser checkpoint, 2026-09-29: Whirling Fortress and Bowser Bomb now use owned
parameters and readable scripts, including shell hurtbox/visibility changes,
root-motion startup, dive and landing effects. A generic dynamic-bone floor
policy fixes the clone's missed stock-kind exception. Sixteen 550-sample
stock/script comparisons match, including both facings, ground/air starts,
steering, slopes, real Zelda contact, landing and one offstage KO/respawn path.
All 34 focused native fighter/script checks and the build pass. Tests use hidden
windows with drawing enabled. Rook was rebuilt and verified against the extended
SDK with source unchanged and one installed package. These are selected-path
results; Bowser is still a partial clone base, not whole-fighter acceptance.

The collaborative workshop UI planning pass is complete and its approved
mockups remain the implementation baseline. Follow `melee/docs/native_character_workshop_ui.md`
and `melee/docs/native_character_editor_handoff.md`; consult existing backend
evidence in `melee/docs/native_character_workshop_progress.md` before choosing
remaining implementation tasks. Do not repeat the historical backend bring-up.
The workshop remains required M3 scope: Collection opens a fighter project;
Action Editor, Scripting and Animation each get a full work mode. Include the
shared multi-channel animation graph, individually selectable simultaneous
hitboxes and first-pass pose copying between clip frames/endpoints. Validate
the functional interface and script experience with actual Mario clone data.
Whole-move copying remains deferred beyond the initial clone/edit workflow.
Goal execution and the return to Sol are controlled separately in the app.
General rollback and online-play testing will resume during the user's later
multi-PC pass; failures found there remain in scope. Existing port, netplay,
social and release work retains the single-PC acceptance boundary below;
this UI approval does not itself establish implementation or test completion.

Status: native port implementation is underway in the full `melee/` source tree. The client presents the Versus, character-select and stage-select menus through Aurora. All twenty-nine selectable stages now reach playable local matches: Battlefield, Final Destination, Dream Land, Green Greens, Jungle Japes, Yoshi's Story, Yoshi's Island, Flat Zone, Yoshi's Island (64), Kongo Jungle (64), Fountain of Dreams, Princess Peach's Castle, Kongo Jungle, Brinstar, Corneria, Mute City, Rainbow Cruise, Great Bay, Hyrule Temple, Onett, Pokemon Stadium, Brinstar Depths, Fourside, Mushroom Kingdom, Mushroom Kingdom II, Venom, Poke Floats, Big Blue, and Icicle Mountain; Final Destination's secondary background animations were visually confirmed in-game. Brinstar's acid movement is visually confirmed, and debugger traces reached its original tendril-animation trigger plus the breakable-platform hit and disable path. Yoshi's Island (64)'s three load-sensitive clouds now use the archive's complete indexed animation arrays; repeated vanish, hidden, reappearance, and restoration cycles plus background guest events completed in a deterministic 11,585-sample run. Kongo Jungle (64)'s barrel now imports all three directly indexed animation joints; barrel rotation and movement, repeated bird flights, and a natural fighter launch completed in a deterministic 28,288-sample run. Venom's scheduler exercised all eleven Arwing flight types, both attack groups, every near attack animation, and laser firing in a deterministic 103,310-sample run. Corneria exercised all thirteen Arwing flight types, both attack groups, every near attack animation, laser firing, and repeated normal cannon cycles in a deterministic 165,103-sample run. One-stock Battlefield, Dream Land, Jungle Japes, Onett, Pokemon Stadium, Brinstar Depths, Fourside, Poke Floats, Big Blue, Icicle Mountain, Mushroom Kingdom II, and Yoshi's Island matches reached the results screen with visible fighters. The Dream Land four-player result portraits now match Ganondorf, Zelda, Ice Climbers, and Game & Watch after the shared portrait/stock frame lookup was corrected; an in-game results screenshot confirms the names and portraits. The Dream Land match completed after the replay recorder was corrected to stop reading freed fighter bones during scene transitions; its 6,080 recorded controller samples replayed in 20.8 seconds with matching tracked fighter state. This establishes several local versus paths, not broad gameplay fidelity or reliable match completion across all characters and stages. Stable screenshots do not by themselves validate moving platforms, hazards, destructibles, or scripted stage transitions; those mechanics require controller-level evidence. All 63 native subsystem/preflight tests pass. Native diagnostic replays capture the boot RNG seed and four-port input samples, compare a fighter-state digest during playback, and can run with virtual input timing and VSync disabled. The same accelerated mode now works for non-replay direct matches. An initial eight-frame live rollback probe now restores game heaps, mutable globals, allocator metadata, and renderer identity counters while running ordinary game draw callbacks with hidden Aurora GX submission; a sound-containing run reproduced all 134,602,788 tracked bytes exactly and reused its live AX sound instance without duplicating it. Its fixed four-port input history now records multiple local players, predicts missing remote inputs, accepts queued confirmations, and identifies the earliest mismatching predicted sample. A production rollback session now owns frame capture, input resolution, earliest-correction restoration, hidden catch-up, audio reconciliation, and rumble suppression; the late-input probe uses that session and reproduces the corrected full-draw timeline twice. Two native processes now exchange redundant input batches over direct nonblocking UDP with arbitrary shared-PC port masks; a delayed run exercised real correction and both peers ended on the same fighter digest and RNG state. Rumble list state is included in snapshots while physical motor commands are suppressed during hidden frames and applied at the corrected endpoint. This is narrow deterministic evidence rather than full M2 acceptance. The fighter action, item, CPU, and results archive graphs have typed native imports and independent ISO preflights. Profile persistence, live action/axis application, and active-match networking foundations now exist; the create/join room UI, synchronized per-PC character confirmation, owner-authoritative stage choice, canonical match agreement, ICE, and rollback now launch a deployed two-client match through normal menus. Profile UI, synchronized results and return-to-room rotation, content sharing, and updates remain unfinished. Milestone checkboxes represent acceptance, not merely code written. Detailed subsystem evidence and limitations live in `melee/docs/`.

Controller-level evidence now covers the moving platforms, hazards, destructibles, and scripted transitions of all 29 selectable stages; `melee/docs/native_stage_sweep.md` records the limits of each check. The immediate porting task is broader fighter, CPU, item, match-end, and result-screen coverage while retaining replays for any failure. A second result roster with Ganondorf, Mario, Fox, and Peach completed on Battlefield, rendered the correct portraits, placements, statistics, and Peach winner model, and replayed all 22,982 samples without RNG or tracked fighter-state divergence. A 2v2 Final Destination team battle with very-high items rendered the correct team-colored roster and blue-team result layout, then replayed all 30,048 samples without divergence. A separate two-Ganondorf free-for-all rendered two explicit non-default costumes through match end. It remains necessary to capture any recurring erroneous hitbox or pose in a replay and to cover more characters, ties, result modes, and item interactions. Controlled replay trials found that skipping scene drawing or just fighter drawing first changes Zelda's star-KO fighter state at sample 4037; the fighter render callback writes camera-projected `cur_pos`, which her star-KO animation later reads. The tracked active-fighter digest matched through all 6,080 samples, but the shared RNG seed split at sample 4126. Two full-draw controls completed the replay. Keep the current rendered fast-match path; defer drawing-disabled simulation and the RNG investigation until rollback work. `CmdUnion` is a four-byte native word; the common/color, fighter action, and item script importers convert PowerPC fields and branches. The fighter importer validates and converts 64,174 distinct action instruction starts across all 33 fighter archives, including one intentional null goto. An independent native test now verifies every field of all 6,655 reachable fighter hitbox commands against the big-endian retail words. Focused live tests created the complete hitbox sets for Falco's up tilt, Luigi's down smash, and Sheik's up smash, and their traced world-space capsules remained attached to plausible animated bones. This closes the three previously reported action-decoding examples without treating the broader intermittent symptom as proven absent. The item importer converts 1,046 distinct command starts across `ItCo.usd` and fighter archives, including byte payloads used by item handlers. A preflight covers all 71 retail stage archives: 53 item articles and 144 script entries reach 269 distinct commands. All 23 common-fighter roots now have typed native imports. The CPU root converts 61 byte scripts and 224 attack arrays; the native test now compares every imported byte, command boundary, attack field, terminator, distance threshold, and weapon reach value with the retail archive. A level-nine Samus, Ness, and Mewtwo match executed ground, air, projectile, and held-weapon CPU selection families. That run exposed Samus's grapple attributes as the final raw reference-bearing fighter article block; its four model joints and fifteen animation pointer slots now have typed imports, and the exact 1,592-sample crash recording replays cleanly. A later 64,322-sample run crossed a scene reload but diverges during replay at the known shared-RNG boundary, so it is stability evidence only. Kirby's copy-ability family now uses a typed native importer for all 25 retail `PlKbCp*.dat` roots. It normalizes their variable layouts, converts hat and merged models, visibility, textures, dynamics, joints, animations, Game & Watch color data, 28 item articles, 49 item scripts, and 113 decoded instructions, then retains the imported storage through the original shifted copy-data table. Link arrow and Game & Watch Chef references and Yoshi's capture-egg joint have explicit native types, and the old 32-bit pointer-table clear is now pointer-width safe. Both the independent layout inventory and complete import corpus test pass. A live Battlefield run also selected Kirby, acquired Mario's copy hat through inhale, and fired the copied fireball without a panic. This verifies one copy runtime path; the other copy abilities and costumes still require live coverage before broader fighter completion checks.

The native Match rules page now exposes the original 31 selectable item switches alongside spawn frequency. A live release-client click, save, and clean restart preserved a disabled Bob-omb bit and showed 30 selected; All and None are also available, with None setting frequency Off. The focused settings and room-match packet tests pass. A multi-client match with a custom item mask remains to be played. The visible item-model fix was separately verified in local and online high-item matches; see `melee/docs/native_shell.md` and the recent item-rendering evidence in `melee/docs/`.

The first per-installation costume package path now exists: content-addressed cache entries remain non-selectable until an explicit install marker is accepted, original-format DATs receive a typed model/material preflight, and a developer tool can import/export/cache/accept packages. Local direct matches can load an installed costume per fighter slot without changing the stock gameplay costume index. Room and match packets carry exact character and costume package IDs. During room setup, the selected costume's owning peer sends any missing package over the room connection; recipients verify the full SHA-256 identity and cache it before match readiness without installing it. A two-client test began with an empty guest collection, reached Fourside gameplay, and left the guest with an exact matching cached package and zero installed entries. The editable CSS and installed-costume picker are implemented. The picker offers in-client DAT import for the selected fighter and color. A live Windows file-picker test rejected Luigi's DAT for Mario without installing it, then installed Mario's DAT, selected it in CSS, and loaded it in practice beside Sandbag. The room sidebar now displays transfer progress; a second two-client test showed a 4 MiB Mario test costume receiving during stage select, reached gameplay, and left the guest with an exact cache digest and zero installed entries. Native custom fighters, their compiled behavior modules, room auto-cache, and explicit gifting now work in the local paths documented below. A rendered same-PC gift check accepted a valid compiled Mario clone and refused an otherwise identical bundle with a damaged module, leaving it cache-only. Extra costumes for custom fighters and their dependency gifting remain open. See `melee/docs/native_content_packages.md`.

## 1. Product and non-negotiable boundaries

Build a private, extensible Windows PC client for playing Melee with friends. Start with the **full existing `melee/` decompilation and modify that codebase into the client**. Integrate Aurora into its native build. Do not reconstruct the game by copying selected decompiled functions into a separate engine or project.

Preserve gameplay that looks and feels identical to the player, including the original mechanics under default controls. Internal behavior may change freely where needed for nativization: original addresses, pointer layouts, PowerPC execution details, compiler conventions, and console services are not preservation targets. Explicit player-facing exceptions are configurable input conveniences, additional custom content, and a wider native shell around the original game canvas. Do not silently fix gameplay quirks, rebalance fighters, or change gameplay timing.

Initial platform: Windows x86-64. Initial mode focus: versus, up to four active fighters, including mixed local/remote players, CPUs, teams, items, and stage hazards. Original single-player modes and extras such as trophies are deferred. Their source remains available; unrelated removal is not part of porting.

Initial UI retains Melee's aesthetics and reusable assets. Layouts may change substantially to accommodate profiles, larger rosters, online rooms, and sharing. A full visual redesign is deferred.

The trusted-friends environment does not require anti-cheat, moderation, DRM, or arbitrary-code sandboxing. Content identity, validation, and exact build matching are still necessary for correct execution and determinism.

## 2. Source ownership and build approach

- `melee/` is the game source of truth and future client project. Add the Windows build there and modify existing call sites directly.
- `aurora/` supplies platform facilities. Link required components and audit API coverage; do not assume every SDK or audio function is implemented merely because a header exists.
- `m-ex/` is reference material and asset/authoring context. Its GameCube patches and binary runtime are not the native extension mechanism.
- The supplied ISO is the baseline asset source. Import required assets locally and retain original input files. Updates must not overwrite user collections or profiles.
- Preserve nested repository histories. Do not reorganize or relocate the supplied repositories as an incidental build step.
- Keep a reproducible original reference for behavioral comparison. Original matching build configuration is useful as a reference, not a compatibility requirement for the native executable.
- Avoid shims, legacy API facades, and dead alternative implementations. Use organized, direct implementations; new files should normally remain below 2,000 lines.
- Prefer existing C gameplay code and C-style native interfaces. Use preallocated match pools and snapshot storage, with allocation/loading at explicit transitions. Expandable content must not imply unbounded allocation during simulation.
- Any future threaded implementation must first follow the user's required Multi-Core By Default article: https://www.dgtlgrove.com/p/multi-core-by-default . No threading design is approved or implemented by this document.

## 3. Fidelity and native porting

Replace or modify machine-dependent implementation details as needed for Windows. This includes PowerPC/compiler-specific code, pointers, structure layouts, SDK services, scheduling, graphics, and audio plumbing. Preserve the resulting player-visible gameplay, not the original internal execution. Keeping the full decomp as the source base does not require preserving obsolete internals.

Load existing asset formats into appropriate native runtime structures. Original on-disk compatibility does not require retaining GameCube in-memory layouts. A blanket byte swap or pointer widening is not an adequate archive conversion strategy.

Preserve the gameplay consequences of simulation cadence, input ordering, randomness, animation timing, physics, collisions, hitlag, hitstun, and camera decisions. Correct undefined behavior and use appropriate native representations. Investigate numerical differences where they can affect gameplay; reproducing every original intermediate value is not a blanket requirement. Deterministic behavior between native clients remains a strict rollback requirement. Keep compiler floating-point settings conservative until their gameplay and determinism effects are understood.

Keep the original Melee output as a centered 4:3 game canvas. A wider native window adds dedicated Freak Melee interface space to the left and right without widening the game's projection or changing its viewport, cameras, blast-zone visibility, magnifying-glass behavior, or other gameplay presentation. Menus may use those side rails for lobby presence, chat, room rules, profiles, downloads, collections, and custom-character catalog editing. During matches the rails should become compact or unobtrusive. Higher-refresh interpolation is deferred.

Trust Aurora as the rendering foundation rather than creating an exhaustive visual parity campaign. Occasionally compare the native game with Dolphin and investigate obvious discrepancies. Concentrate screenshots and visual review on presentation we actually change, particularly new or modified UI and the native side rails. Do not require pixel-perfect comparisons or broad repeated captures of unchanged rendering features. Builds and headless checks still do not establish that the game plays correctly.

## 4. Profiles and input

Profiles own bindings **and calibration**, including analog stick and trigger settings. Controller assignment is explicit when a player joins; do not depend on unique hardware IDs. Multiple bindings for one action are supported. Optional named controller presets within a profile may support changing devices.

Expose actions rather than GameCube button labels: Jump, Attack, Special, Shield, Grab, movement, attack stick, and appropriate menu/pause actions. Preserve analog shield strength and digital trigger activation. Grab must retain original Z behavior, including its analog shield value and contextual effects, rather than approximate it as full shield plus attack.

Requested options: tap-jump toggle, dedicated short-hop binding, and smash/tilt attack-stick mode. Define ground/air behavior, simultaneous-button precedence, hold/release semantics, and character-specific interactions before implementation. Do not erase upward movement input to disable tap jump or alter stick values used for DI merely to request a tilt.

Default settings must reproduce original controls. Optional settings intentionally alter input interpretation while retaining the underlying move mechanics. Audit combined X/Y behavior before collapsing physical-button transitions. Short-hop behavior must account for jump squat and aerial jump/float interactions.

Sample and normalize input once per simulation frame into a reproducible input record. Relevant settings are fixed during a match and included in its setup information. Rollback uses recorded inputs, not live controller polling. Menu navigation and normal pause/exit actions must remain accessible with arbitrary bindings.

## 5. Characters and packages

Collections belong to the **installation**, not a player profile. Every player sees the selected costumes of other players; local overrides of another player's appearance are not supported.

Distinguish two states:

1. **Cached dependency:** available to simulate/render a match or replay, but not selectable by its recipient.
2. **Installed collection entry:** selectable by local players and eligible to send to friends.

Automatic downloads do not grant collection membership. An explicit send/accept flow installs a gift permanently. If already cached, acceptance can register the existing package without another transfer. Include required dependencies. This is a social UX distinction, not DRM.

Each player selects only their own installed characters/costumes. Different machines may therefore show different selectable rosters. Exchange stable package identities and selected entries, never local grid indices. At match setup, resolve the selected content and dependencies to identical runtime IDs on all participants. Costume-only gifts for absent custom characters must offer their required character dependency.

Character select must have an in-client WYSIWYG layout editor on the original Melee CSS. The first pass edits fighter portraits only: move, resize, remove from the visible layout, and add back without uninstalling. Installed custom fighters join that editor with native portrait visuals aligned to the original CSS canvas; stock fighters continue to use their GX portrait joints. Costumes belong in a picker under their fighter. One layout belongs to the installation and is shared by its local players. Persist the layout by stable content identity and geometry, independent of other players' arrangements or network slot numbering. Editing temporarily doubles the left rail, hides its ordinary controls and the right room/chat rail, and shifts the unchanged 4:3 game image right within the same window.

The first native CSS layout model stores one installation-wide arrangement of
stable fighter identities and normalized tile geometry. It seeds the original
25-icon roster and saves atomically outside replaceable builds. Versus uses the
original GX CSS for selection; the editor manipulates its real portrait joints
and hit bounds in place. Live checks covered move, resize, remove, re-add,
restart persistence, and selecting a moved Mario icon with the original cursor.
The home CSS now offers installed costume packages compatible with each local
fighter's stock character and base color. Selection enters practice and is
published to the room; the owner's stage-select match proposal carries each
selected package ID. Missing selected costumes transfer before match readiness.
An isolated two-client UI run took installed Mario on the host and stock Luigi
on an initially empty guest through room readiness, Fourside stage select, and
live gameplay; the guest cached the exact package and still listed zero installed
packages. In-client costume import is built and its validation/install path is
tested. Installed custom fighters can now be selected on the original CSS and
taken into local practice; online custom-fighter agreement remains open.
See `melee/docs/native_css_layout.md`.

Ordinary authored move behavior uses the typed move language and native interpreter; authors and recipients need no C compiler, generated C or behavior DLL. Handwritten native extensions require an explicit advanced build contract. Existing PowerPC code embedded in m-ex DATs is not executed or automatically translated. Characters with source require a source port; source-unavailable characters may be individually reverse engineered later. No universal m-ex binary compatibility commitment.

**Scripting API rule â€” fighter-agnostic extensions only.** Do not add a script property, operation, or compiler branch named for one fighter, move, or legacy fighter-union field merely to port stock code or make a parity trace pass. Put per-fighter persistent values such as charges and cooldowns in declared rollback-tracked `state` variables; put tunable move constants in named attributes. Add an engine primitive only when its behavior is reusable across fighters, then keep the fighter's decisions in its script. Compare stock and clone behavior at observable boundaries; do not force identical private storage layouts. Document intentional corrections to undefined or buggy stock behavior in the script and parity notes. Full Mario and Dr. Mario scripts use ordinary charge variables. Compiler charge shortcuts and hard-coded numeric Mario attribute aliases have been removed; author scripts against declared state and project-owned parameters.

**Project-owned parameters.** New script-authored `attrs.Name` values belong to the fighter project's `fighter_parameters.fmp`, with typed names and values editable in the Workshop. Package loading binds the values once to the interpreted program; changing only a value needs repackaging, while changing the name or type needs a rebuild. The imported DAT attribute block remains for original engine routines that read it. Migrate stock-translation scripts off byte-offset declarations as their source values are verified; do not create fighter-specific parameter APIs or write parameter edits back into the DAT.

The first Rook asset preflight now imports `PlRk.dat`, `PlRkAJ.dat`, and `PlRkNr.dat` through the native typed importers without running the game or PowerPC code. Its animation table has stale clip offsets and lengths; the native bank importer can resolve invalid entries by their named embedded DAT symbol. A stock Mario test exercises the same recovery against a deliberately shifted real clip, and the full stock animation preflight remains green. A new typed fighter bundle validates and caches all three real Rook DATs under a content identity and stable `rook` code key; acceptance is blocked until matching native behavior is registered. This is asset conversion and packaging evidence only. Native fighter registration, callbacks, CSS presentation, match package resolution, and Rook's missing behavior are still open; see `melee/docs/native_custom_fighters.md`.

The native fighter and character-mapping tables now reserve IDs 34-127 for custom registration while preserving the original `Ft_Kind_None` and `ChKind_None` sentinels at 33. Stock-only validation, selectors, and preflight loops still stop at the stock count. The full 359-test suite passed and a fresh native client boot visibly reached the original CSS after each table split. Compiled custom definitions, match selection/loading, and gameplay remain unimplemented.

The native registration routine now imports a cached three-DAT fighter bundle into the existing fighter, costume, animation, character-mapping, and callback tables after fighter-system reset. The developer bundle command probed the real Rook package at slot 34, exercised ordinary preload without custom ISO lookups, rejected a wrong compiled code key and a duplicate slot, and cleared the registration afterward. A fresh stock CSS boot, direct stock match initialization, and all 359 native tests passed. This is table-binding evidence, not a playable Rook: the probe supplied no real motion callbacks, and selecting custom fighters through CSS, resolving their packages in online matches, and native Rook behavior remain open.

The registration boundary now requires an explicit character-specific motion-state count and rejects animation references outside the imported action table. A source audit found Rook's 12-row m-ex move table occupies states 341-352; two comments copied Jigglypuff's 365/366 labels and callbacks, whose internal transitions target Jigglypuff's state IDs. Those callbacks cannot simply be rebound to Rook. The native Rook definition must replace the copied states with named callbacks and complete the empty neutral-special callbacks before being offered for play.

The first compiled Rook definition now supplies the 12 named motion states and native special-move callbacks. Its source-derived side, up, and down special logic keeps mutable move variables in the rollback-tracked fighter. The developer bundle command binds this real definition and verifies the callback table after importing the Rook DATs. Neutral special still uses a temporary animation-end/physics/collision contract because the MexTK source leaves its callbacks empty; the intended effect, live move behavior, CSS presentation, match package resolution, and online agreement remain unverified. Do not offer Rook for selection yet.

The native fighter registration now also supports a landing callback on the air-to-ground transition. Rook uses it to replenish its down-special charges, as its MexTK `onlanding` hook intended. This compiles and passes registration checks but still needs live move testing.

The content tool now installs a fighter bundle only when its compiled definition exists and the full runtime import succeeds. An isolated Rook store validated `bundle-fighter`, `accept`, and `list` with the real three-DAT package. The original CSS now displays and selects installed Rook; a live isolated run entered practice, moved him, and returned to CSS with the selection intact. Home selection resolves installed package IDs to registered native kinds before preload. Restoring the two visibility groups set by Rook's original `OnDeath` callback made his body visible in a direct practice run. Online custom-fighter reconciliation, full Rook visual and move fidelity, and gifting remain open.

Import supported original costume, model, texture, and animation files automatically, validating skeleton/bone references and required symbols. Import supported move-command/attribute data explicitly. Do not assume every DAT is cosmetic or that every animation change is gameplay-neutral. Preserve HSDRaw-oriented authoring where practical.

Character definitions reuse standard movement/states and declare named moves, animations, events, attributes, and explicit native callbacks. Resolve definitions at load/build time into direct runtime tables. Authors must not copy giant common-state tables or supply original function addresses.

The character editor is part of the native client. An author can clone a supported stock or installed custom fighter into an editable installation-local project, open the grey-grid practice room with a smaller live game viewport, and use the rest of the window to inspect and edit that clone. Separate draft saving, preview apply, and building/installing a playable package. Provide undo/redo and draft recovery. Edits apply transactionally at a safe practice reset, never into an active online match. The clone keeps a distinct identity and preserves its source fighter. Stock callback kind checks, attributes and articles must be audited before a base is offered as faithfully cloneable; copying callback tables is insufficient.

Make ordinary special-move behavior accessible without C knowledge. The user chose readable scripts as the primary interface, supported by completion, examples, visual hitbox inspectors and a source-linked timeline. Introduce a small typed Melee move language for behavior otherwise written in C, covering states, animation/input conditions, velocity and position, articles/effects, bone transforms, persistent variables, and interruption/cleanup. Existing native subactions continue to own their command behavior. Validate scripts directly through a typed representation into interpreted bytecode, with code references at the existing engine phases and explicit rollback state. Map diagnostics to script source; ordinary authoring and sharing require no C toolchain. Scripts own logic, definitions own typed data, and visual controls edit those same documents. Computed events can be inspected in a test trace without pretending they are editable fixed timestamps. Retain native stock behavior explicitly where it has not been expressed in the language. Test an edited move in the real engine, including hitlag and interruption, before claiming accessibility or fidelity.

C/C++ callbacks remain an advanced escape hatch for unusual mechanics. Ship a complete fighter template and documented examples. For the first fully cloneable fighter, Mario, reproduce all four specials in editable scripts: each script owns entry, animation, interrupt, physics, collision, article cues and cleanup rather than delegating to Mario's original special handlers. Scripts may call general Melee engine primitives. Keep original move code as a parity reference until scripted behavior is validated; an inherited stock callback can remain available for explicitly untranslated moves, but it is not a completed Mario template. A fresh Mario clone's Scripting mode opens with an uncompiled draft of those faithful scripts. Validate the small language against real moves before fixing its syntax; a general-purpose language or mandatory visual logic graph is unnecessary. The current compiled-in Rook definition is a checkpoint, not the final distribution model for new native code. Ordinary authored scripts use direct validation and interpreted program loading; handwritten native extensions use a separate explicit advanced contract. Program loading, safe replacement, dependency-pinned packaging and rollback state ownership allow characters to be applied and shared independently of whole-client rebuilds. See `melee/docs/native_character_editor_design.md` for the source audit, runtime contracts and delivery order.

Animation authoring includes in-game bone posing/keyframes and Blender import/export for larger work. Build on the supplied HSDLib and native figatree decoding. Share explicit bone, transform, curve, root-motion and clip metadata between tools; preserve untouched tracks and validate edited tracks in the actual evaluator, including fractional frames. Do not promise lossless conversion through generic interchange formats. Keep gameplay replay scrubbing distinct from animation-only pose scrubbing.

HSDLib/HSDRawViewer is primary groundwork for the asset and action/subaction editor: its existing source includes DAT graph writing, command schemas, subaction compile/decompile, action/subroutine navigation, editable command properties, hitbox picking/manipulation, interrupt/frame markers, curve editing, and animation-bank rebuilding. Reuse the format knowledge, metadata and suitable offline library operations. The native client supplies the in-game UI and real simulation preview. Distinguish editing original subaction commands from authoring native special-state behavior; the small language fills that latter gap. A hitbox/timing edit must not require translating an entire stock move into the new language. See the HSDLib reuse audit in `melee/docs/native_character_editor_design.md`.

Character state is explicitly registered, preallocated, and captured by rollback. Hidden mutable DLL globals, independent randomness, and asynchronous gameplay mutation are not permitted. Packages and code remain fixed during matches.

Deliver a complete playable template and one build/package/test operation. Cover costume/animations, normals, specials, sound/effects, grabs/throws, custom state, and documented CPU interactions. Kirby cloning and custom copy abilities are deferred by user decision (2026-09-28); stock Kirby remains playable, but custom copy integration is not a current clone acceptance gate. Build the editor in coherent slices: verified clone/project lifecycle; one script-authored move with timeline, hitboxes and live testing; broader special behavior; in-game animation and Blender round-trip; package sharing and rollback. Do not build a broad inspector before proving that complete edit/test/save/install loop.

After the base fighter clone backend and collaborative interface design, add complete supported move copying: states, subactions, animations, behavior and their assets move together with explicit dependency resolution. This transfer is not a gate for starting the editor interface or shipping its initial clone/edit workflow. Seed animation mappings from Melee's existing shared-part remapping and validate extra bones, attachments and behavior-specific data. Start full clone coverage with Mario and expand to Captain Falcon, Fox, Jigglypuff and others; readable stock behavior templates grow incrementally, without depending on automatic translation of arbitrary C. Kirby is excluded from the current stock-clone translation scope by user decision (2026-09-28). `melee/docs/native_character_editor_handoff.md` records the authoring model, source ownership, module/state proof, actual integration points and acceptance gates for implementation.

Rook at `A:/melee modding/Rook` is the proposed first substantial authoring acceptance case. It is an external reference; do not modify it incidentally. Preserve behavior while replacing address-based declarations, numeric bookkeeping, and copied common tables.

## 6. Rollback, rooms, and spectators

Separate network peers, local profiles, and fighter slots. Support four players on four PCs, two on each of two PCs, three plus one, fully local matches, and CPU slots. The host selects room rules; it is not an authoritative server for combat outcomes.

Build deterministic frame stepping and snapshot restoration before online rollback. Snapshots include all mutable state that influences future simulation: fighters, items, stage objects, RNG, object/pool state, pending gameplay events, and relevant camera state. Audit hidden global/static state and presentation feedback into simulation.

Local snapshots and portable spectator snapshots have different requirements. Raw process pointers must not be used as cross-machine state identities or compared directly in deterministic hashes. Restore-and-replay must not duplicate audio, rumble, file writes, or other external effects.

Exchange inputs between active peers, predict missing inputs within a bounded rollback window, and resimulate when necessary. Select transport and rollback implementation after evaluating their actual integration requirements. A networking library alone does not provide deterministic simulation or usable snapshots.

Rooms support active players, a waiting queue, and deliberate spectators. Automated 1v1 rotations support winner-stays/loser-out and winner-out, with manual overrides. **Teams and free-for-all assignments are manual.** Queue membership belongs to individual players, including players sharing a PC.

Spectators consume confirmed inputs and render locally with a small delay. A slow spectator cannot stall active players. Joining mid-match requires packages, a portable snapshot, and catch-up inputs; spectator catch-up is separate from the active rollback window. Bound transfer bandwidth and prioritize active gameplay traffic.

**Active-player disconnect:** when declared disconnected by the session's failure policy, abort the match and return reachable remaining participants to the lobby. Do not continue with neutral input, grant a win, or trigger automatic result-based rotation. Connection loss for a PC removes all its local active slots. A spectator disconnect has no match effect. Timeouts must distinguish ordinary packet loss from a disconnect.

If the room owner leaves, transfer room control to a remaining peer when feasible. Abort an active match only if that departing peer owns an active fighter slot; a spectator-only owner leaving must not interrupt combat. If room recovery fails, show an explicit disconnected-room screen rather than claiming the lobby is usable. Seamless in-match host migration/rejoin is deferred.

## 7. Connectivity and release services

Prefer direct peer connections established through signaling and NAT traversal. Use a relay only when needed. Established direct matches must not depend on a signaling server heartbeat. A relayed connection necessarily depends on its relay while in use; no promise of universal direct connectivity.

Proposed hosting: Cloudflare Workers/Durable Objects for room/signaling services, Cloudflare STUN/TURN with a compatible native transport, and R2 for prepared release artifacts. Exact APIs, pricing, transport compatibility, and provisioning remain implementation-time verification. Self-hosting is possible but is not required for the proposed first service deployment.

Room notifications must not gate simulation frames. A signaling outage may prevent new joins while an established match continues. Custom packages transfer between peers as part of normal sharing; central hosting of everyone's private collection is not required. Complete active match downloads before starting. Late spectators catch up independently.

Use a small launcher/updater. Download complete prepared releases into a separate directory, verify completeness, then activate after the game exits. Retain the previous working build. Store profiles, saves, packages, and caches separately from replaceable builds. Do not force updates during matches.

Initially require matching client builds and matching selected gameplay content. Rebuild character modules for changed interfaces; do not add backward-compatibility layers. Keep unrelated installed packages out of match compatibility checks unless they actually influence the match.

Replays identify the build, rules, packages, initial simulation information, and input stream. They may require retaining their original build/packages. Do not promise playback by arbitrary future builds. Cached replay content remains non-selectable unless separately installed.

## 8. UI direction

2026-09-22 shell checkpoint: a live RmlUi sidebar surrounds the original 4:3
game view. Create/join, names and shared-PC player counts, room presence, copy
code, character-select entry, and leave now use actual sidebar controls. The
temporary HSD room text/editor has been removed. Two clients joined one room
through the sidebar and entered CSS; the user confirmed ordinary mouse clicks.
The layout was screenshot-reviewed at 1440x810, 1280x720, and 4:3. Global chat
now has a separate application-lifetime connection, 64 recent messages,
acknowledged sending, online count, reconnect, and collapse. Its private service
is deployed, and live sidebar sends during local gameplay plus retained history
across room changes/CSS/restarts are verified. Overflowing chat history has an
explicitly sized scrollbar. Options now opens a full-window profile editor with
saved-profile selection, explicit port assignment, gameplay options, action/axis
capture, and calibration. UI save/reload and invalid-entry rejection are verified;
physical capture, keyboard/menu bindings, controller-only navigation, and broader
layout acceptance remain open. Calibration now displays live input for the
selected controller. This exposed a profile conversion bug: SDL gamepad trigger
axes start at zero, so the custom binding path incorrectly produced half shield
at rest. The corrected path maps zero to zero and full travel to 255, and the
editor now presents trigger activation across 0â€“100%. Source-backed tests and
running-client screenshots cover the correction; hands-on trigger motion and
physical rebind capture remain open. See `melee/docs/native_profiles_ui.md`.
Saved-profile deletion now has an in-app confirmation; an isolated-store client
check verified cancel, confirm, and refreshed picker behavior while existing
session control assignments remain stable.
Collection navigation remains explicitly disabled. All 354
discovered native tests pass serially with assertions enabled, and all 12 service
tests pass. See `melee/docs/native_shell.md` and `melee/docs/native_chat.md` for
evidence and limits. Spectator/rollback activity and broader UI acceptance
remain open; this does not close the full online milestone.

The native shell includes one shared global chat for everyone who has Freak
Melee open, independent of room membership. Keep it available in the right
sidebar across menus, local play, online matches, and spectating; support
collapsing it during play. Room joins/leaves do not clear or switch the chat.
Use a separate application-lifetime chat/presence connection, outside rollback
and the match transport, so chat outages never stop an established match.
Show real connection state and a bounded recent-message history; never present
locally queued messages as delivered until acknowledged. This supersedes any
assumption that the initial chat is exclusive to a lobby.

Prioritize the permanent shell and its real interaction model before further
hardening the temporary room/CSS menu flow. Repair blockers to joining and
playing, but defer speculative transition work until the final UI owns it.

Sidebars are for information and quick actions useful during gameplay.
Configuration must not be squeezed into those rails: Options opens a separate
full-window screen sliding in from the left, with a large Back button at its
upper-left corner. Profiles, bindings, and calibration use that space. Other
management screens should likewise rearrange the interface to provide enough
room for their tasks rather than inherit the narrow gameplay layout.

Home-flow revision, 2026-09-22: boot directly to a simplified character-select
home screen, supporting up to four local players. START enters a simple grey
grid practice room with Sandbag. The usual START, then L+R+A+START exit returns
to this home CSS; ordinary B retains Special during practice. The home CSS must
not navigate back into the original Melee menu hierarchy. This replaces the
earlier normal-boot-to-VS-menu direction; explicit diagnostic launch paths stay
available. Sidebar horizontal padding is now 8dp instead of 16dp.

Preserve both fully local versus and online play. Recommended distinct paths:
home CSS â†’ START â†’ practice â†’ normal match-exit chord â†’ home CSS; home CSS â†’
Local match â†’ stage choice â†’ local versus â†’ results â†’ home CSS; online room â†’
practice locks local character choices and marks those players ready â†’ all
assigned active players ready â†’ host sidebar Start match button â†’ owner stage
choice â†’ identical setup/content loaded and acknowledged â†’ match â†’ results and
rotation/home. There is no second character selection; returning to home CSS
clears readiness. Practice remains local simulation and does not consume a
room's network match state. Spectators do not gate readiness. A start request is
separate from final simulation-start agreement. The native grey-grid stage now runs in local diagnostic matches;
Sandbag now occupies a separate fifth slot, player KOs respawn indefinitely,
and escaped Sandbags reset to the center at 0%. Four Ice Climbers pairs plus
Sandbag (nine fighter entities) run together. Normal boot now enters home CSS;
local practice/start/stage/match/results flow and two-client stage cancellation
are exercised. The host explicitly starts with the sidebar button. A four-player
Chef action desync was traced to a Game & Watch item union where a native
eight-byte pointer overwrote the adjacent variant. The corrected layout and
semantic state test pass; a fresh delayed two-client run with Chef, Sheik, and
Ice Climbers actions reached results with 1423/2984 confirmed state checks and
no mismatch. Persistent post-match rooms, spectators/rotations, and simplified
CSS presentation remain unfinished. See
`melee/docs/native_home_flow.md` for the source audit and required acceptance.
Home rooms now accept nonconsecutive selected physical controller ports. A
two-client test used the host's ports 1 and 3 plus a guest player; port 3 moved
its assigned fighter and acknowledged its results panel. The full two-minute
Battlefield round returned both clients to the same room after 7436/7437
matching confirmed checksums. This used virtual port-3 input; physical
multi-controller testing remains open.
Solo practice also works when Team Battle is selected: the home CSS accepts
one locked-in fighter without requiring opposing teams, while normal versus
start requires at least two opposing teams. A live two-player check confirmed
that same-team fighters can practice but cannot start a match; changing one
fighter to blue enabled the sidebar button and opened stage select. The online
host also validates the combined room roster before proposing a match.
The home CSS now uses a native "CHOOSE A FIGHTER" caption in the original
header, hides the obsolete Back graphic, and keeps the Team Battle toggle and
four fighter doors. The caption is absent in practice and returns on exit;
further CSS simplification remains open.
Online motor output now follows the same logical-to-physical controller map as
input, with remote fighter slots muted. The mapping remains through results
and resets at home CSS; focused tests pass, while physical multi-controller
rumble still needs a live check.

Reuse original menu assets, animation, sounds, and aesthetics where appropriate. Existing HSD menus use animated model/joint hierarchies; do not assume they can be treated as static images. New layouts may combine retained presentation with explicitly implemented room/profile/collection controls.

The centered game canvas remains the authoritative Melee presentation. The native shell owns the additional width and its UI independently, so enlarging the application window does not require gameplay-camera or archive-asset widescreen patches. At a 16:9 1280x720 presentation, a 960x720 game canvas leaves a 160-pixel rail on each side; other window sizes preserve the same separation and scale deliberately.

Sync menu intentions such as selections, ready state, and assignments through session messages. Do not run networking, downloads, or the entire UI through combat rollback. Selection/ready changes invalidate affected setup agreements. Start only after every active peer has loaded the same chosen content and agreed on the match setup.

Gifting offers a chosen package to a connected recipient with an explicit accept/install action. Cached entries do not appear as selectable gifts owned by the recipient until installed. No account system or public marketplace is required.

For a possible public release, consider a source-only sharing lane: transfer
validated fighter scripts and assets, then build validated interpreted
bytecode locally with the client. Keep handwritten C/C++ extensions in a
separate explicitly trusted lane. A public interpreter requires validation of
code, bindings, assets and operation bounds; the current raw-memory evaluation
prototype is not a security sandbox.
This is deferred design work, not a requirement for the current private build.

## 9. Initial source audit

Inspected revisions: `melee` 039c4bf4c, `aurora` 749d6ee, `m-ex` c9f25da. These are local observations, not pinned release dependencies or verified complete builds.

| Area | Existing source | Finding / next investigation |
| --- | --- | --- |
| Original build | `melee/configure.py` | PowerPC alignment and Metrowerks build configuration; introduce a native build for this same full source tree. |
| Serialized assets | `melee/src/sysdolphin/baselib/archive.c` | Relocates 32-bit embedded pointers and assumes source byte order. Inventory archive root types and typed native loading. |
| Object lifecycle | `melee/src/sysdolphin/baselib/gobj*.c`, `memory.c` | Map ownership, scheduling, pools, and snapshot boundaries. |
| Audio | `melee/src/sysdolphin/baselib/axdriver.c`, `synth.c`; `melee/src/melee/lb/lbaudio_ax.c` | Audit original audio service requirements against actual Aurora coverage. |
| Inputs | `melee/src/melee/ft/fighter.c` | Existing input histories and trigger/Z normalization must survive action binding changes. |
| Jump interpretation | `melee/src/melee/ft/kinds/ftCommon/ftCo_Jump.c`, `ftCo_KneeBend.c` | Separate stick/button jump paths and release-based short-hop decision. Audit all consumers before adding options. |
| Character tables | `melee/src/melee/ft/ftdata.c` | Fixed character-indexed callback/data tables; replace through direct call-site changes. |
| Menus | `melee/src/melee/mn/mnmain.c`, `mncharsel.c` | Animated HSD assets; selection layout and identity assumptions need detailed audit. |
| Camera | `melee/src/melee/cm/camera.c` | Keep original camera bounds, tracking, projection, and 4:3 game canvas; native side rails live outside it. |
| Magnifying glass | `melee/src/melee/if/ifmagnify.c`, `melee/src/melee/ft/fighter.c` | Preserve the original 4:3 visibility and damage behavior by leaving the game viewport unchanged. |
| Rook | External `MexTK/rookmain.c`, `rookheader.h`, `speciallw.c` | Address-based callbacks, copied common tables, custom charge/random state; useful authoring and rollback acceptance case. |

This is a map of observed integration risks, not an exhaustive portability or determinism audit.

## 10. Milestones and acceptance

2026-09-10 implementation progress: local `melee` branch `native-windows` now has
a CMake object target covering all 984 existing game/HSD C files. Explicit
Windows/MSVC Clang passed toolchain compile/link checks. The initial pass compiled
878 files; native header/layout fixes raised that to 915, and verified ISO font
extraction enabled two more. Remaining errors and pointer warnings are being
ported directly. This is partial M0 work, not a linked or playable native game.
See `melee/docs/native_build.md` for reproducible commands and evidence.

Follow-up: callback contracts, allocation-hook signatures, and native reporting
now allow all 985 game/HSD files (984 original plus reporting) to compile. The
native reporting executable test passes. M0 remains incomplete: the game itself
is not linked and pointer-width hazards remain. Aurora is now included in the
native CMake build, and its core, GX, OS, matrix, video, controller, disc, card,
and THP library target builds successfully. The reporting test still passes.
The pre-platform-link symbol audit finds 333 external names including CRT
imports; `melee/docs/native_platform_audit.md` records the remaining startup,
asset, audio, and ABI work. Local commit `5c3b9ed58` records this checkpoint.

Native memory follow-up: HSD fixed-pool addresses and OS arena-rounding macros
retain 64-bit pointers; even small payload slots fit an aligned native free-list
pointer. The game/HSD object build and two native tests pass. The new pool test
uses storage above 4 GiB and covers reuse, alignment, bounds, exhaustion, and
overflow rejection. The OS-heap path and general DAT conversion remain unverified.
HSD object IDs and their joint/animation/skinning callers now retain native
pointer width as well. The rebuilt source tree and ID-table tests pass, including
equal low-32-bit keys, hash collisions, replacement, removal, and scene reset.

The OS-heap path now has a passing integration test linking HSD memory and pools
to Aurora's actual allocator: high-address allocation, free/coalescing, heap
selection, and heap recreation pass. Native HSD panics use file/line/message
callbacks and terminate through the CRT; a child-process assertion test verifies
flushed output and failure exit. All four native tests and the full object build
pass. Complete scene startup, DAT conversion, and the game executable remain open.
The real class allocator now also passes integration coverage. A new test
reproduced a resize crash on nonzero 64-bit heap memory; replacing a hardcoded
four-byte pointer-clear size fixed it. Class construction, table growth, reuse,
failed-constructor cleanup, instance counts, and library reset pass.
An immutable big-endian DAT reader now compiles with the source tree. Its native
test and eleven original archive samples pass, including stages with actual
external-reference chains. It exposes source offsets and bytes for typed
conversion; it does not yet supply native descriptors to the game archive loader.
Typed image/palette/LOD conversion is now implemented. Native tests pass and
original-data probes convert 60 Mario costume image descriptors plus 71 named
palette descriptors across the sample archives. Pixel bytes remain unchanged;
graph assembly, archive-loader integration, and rendered acceptance remain open.
Material/texture graph assembly now uses a fixed-capacity caller-owned arena and
identity table. Shared records, repeated-root identity, and capacity failures
pass tests; all 59 Mario costume material graphs import. External linking,
optional renderdesc decoding, joint/geometry/animation graphs, loader integration,
and rendering remain open.
Material-animation graph import now covers AObj/FObj descriptors and counted
texture tables, including Mario's original 150-record costume animation graph.
Converted synthetic tracks execute through the original FObj interpreter with
correct linear float/signed-integer frame values and restart, without playback
heap allocation. Other interpolation modes, skeletal graphs, loader integration,
and rendered behavior still need acceptance.
The original Mario costume joint root now imports as 720 native records,
including its materials, rigid/envelope geometry descriptors, and skinning
references. Synthetic model tests verify shared joints, transforms, matrices,
weights, terminators, and unchanged payloads. Morph targets, expression records,
full indexed-payload validation, runtime object loading, and rendering remain open.

- [x] **M0 â€” reproducible foundation:** inventory toolchains/dependencies and full source coverage; establish native build within `melee/`; preserve reference inputs and report unresolved symbols/platform assumptions explicitly. Do not replace missing behavior with silent stubs.
- [x] **M1 â€” native original versus:** original boot/menu-to-match path, assets, graphics, sound, controllers, complete local matches, results and return. Validate representative fighter/stage/item interactions against original behavior; record remaining coverage rather than extrapolating from one matchup.
- [ ] **M2 â€” deterministic frame execution:** fixed stepping, preallocated snapshots, restore/replay equality, canonical state diagnostics, and no repeated external effects. Exercise multiple fighters, items, RNG, hazards, deaths and match transitions across supported machines.
- [ ] **M3 â€” extensible content and controls:** native character tables, asset import, complete template, Rook port, profiles/calibration/actions, and the approved in-client character workshop. The initial workshop includes original subaction editing, readable special-behavior scripts executed by the native interpreter, a smaller live practice viewport, in-game pose/key editing and Blender round-trip. Complete supported move copying follows the base clone backend and interface design; it is not a prerequisite for the initial editor. Validate default-control equivalence and optional controls independently; Rook's custom state survives rollback. Complete the workshop acceptance checklist below using `melee/docs/native_character_editor_handoff.md`; do not treat a syntax demo or one edited move as acceptance of the entire milestone.

Character clones must be independent installed fighters. The source fighter is
recorded as editor provenance only: the built package owns its identity, data,
states, callback bindings and compiled behavior, with no live runtime dependency
on the source fighter's editable definitions. Reusing immutable engine helpers
is fine. The current `mario_clone` behavior key and Mario callback inheritance
are temporary bring-up scaffolding, not the finished clone contract.
The first in-client workshop slice can create a Mario draft, edit common
attributes, preview the unsaved edits in a restarted home practice scene, and
build/install a separate package with a chosen portrait. Preview caches and
validates a candidate without saving the draft or installing it; leaving
preview restores the previous fighter selection. This is a partial
implementation: a separate headless action backend can inspect all 303 Mario
actions and edit source timers and complete five-word hitbox records in a copied
DAT; 242 reachable Mario hitbox occurrences passed a byte-identical no-op
round trip, and an isolated edited Attack11 bundle loaded and executed in a
local match. Fighter DAT saves now retain a one-step disk undo/redo. The client still
has no in-game action or animation editing interface or multi-step recovery;
the core backend has a limited authored neutral-special path while other Mario
callbacks remain inherited. Per the user's 2026-09-23
priority, implement and prove the core clone, module, and animation contracts
before designing the final workshop interface together. Per the later user
clarification, defer whole-move transfer until after that design work.
One original-subaction result is now proven in live practice: two independent
Mario clones with Attack11's three hitbox damage fields at 3 versus 20 dealt
3 versus 20 damage to Sandbag on that same motion. A local input-comparison
playback fed the untouched clone's exact seed and 1,849 controller samples to
the edited package: both entered Attack11 at sample 145 and struck Sandbag at
sample 146. Traces and recordings are under `melee/build/native/jab-live`.
Other original-command fields and complete Mario move fidelity remain open.
A separately compiled native fighter DLL now crosses a tested Windows loader
boundary and declares an isolated per-fighter state block. Fighter drafts can
package their own module bytes under a distinct content ID after definition
validation. The packaged DLL now passes native asset import and fighter-table
registration while the module remains loaded. A module-backed Mario clone has
completed a direct one-stock match. An isolated callback's per-fighter bytes
restore under the rollback page mechanism; callback activation and corrected
resimulation in live gameplay remain outstanding. A first strict readable
script subset can now generate and compile ground/air neutral-special speed
callbacks. Packages now include the exact editable script bytes alongside the
DLL; the broader move language and transactional multi-file draft save remain
open. The first operation now writes the fighter's grounded velocity for a
ground special and horizontal self velocity for an air special. A compiled
script package reached the practice room, but concurrent user input made its
live movement comparison inconclusive; the generated callback test verifies
the corresponding fields directly. Generated modules now carry a fingerprint
of their exact script bytes, and draft packaging rejects mismatched saved
source and DLL; a crash-atomic two-file save is still open.
`script-build` now backs up the working module and restores it when saving
the new script fails after DLL replacement. A Windows file-sharing failure
test preserved both previous hashes and the original package identity; the
374-test native suite passed. A process-crash/power-loss atomic pair commit
and multi-step draft recovery are still open.
An installed Mario-based fighter package can now be copied into a new editable
draft, including its exact available script source and module. An isolated
clone kept its parent's files, then changed only its own weight and registered
as a distinct package. This is independent data ownership for the tested base,
not proof of full move behavior or arbitrary fighter cloning.
The script subset now has an explicit inherited-stock entry operation. A
Mario-based script package entered the original neutral-special state in live
practice, spawned a visible fireball, and replayed 5,119 recorded samples
through two uses without RNG or tracked-fighter-state divergence. Fully
authored state/cue/article/cleanup code remains open.
The module contract now passes an exact-client host table for motion-state
changes, animation start, collision checks and ground/air transitions. A script
can enter the Mario base's neutral state without calling stock entry and owns
the two neutral-state collision callbacks in a copied motion table. A live
6,092-sample practice recording crossed from aerial motion 344 to grounded
motion 343 and replayed without RNG or tracked-fighter divergence. Authored
cues, articles, interruption and cleanup remain open.
The source-linked action backend also reads original animation metadata and
edits the source flag word while protecting unimported bits. One isolated
Mario action changed a single DAT byte, packaged and registered successfully,
then disk undo restored the original. Named interrupt/flag controls and the
remaining original-command fields are still open.
Authored Mario neutral-special scripts can now apply a movement impulse at
the original action cue in ground or air. The callback consumes that cue once
and survives ground/air transitions; a live grounded-and-aerial practice run
replayed 3,009 inputs without divergence. Projectile ownership, broader script
operations and interruption cleanup remain open.
The same authored cue can explicitly call Mario's original article operation:
a live grounded and aerial practice run visibly spawned fireballs and hit
Sandbag, and all 1,712 recorded inputs replayed without divergence. This is
inherited stock article reuse. Package-owned article assets and logic remain
part of the workshop acceptance work.
The cue can now use `state.once()` with a per-fighter rollback-tracked byte.
Melee's own motion-change callback retains it across neutral ground/air
handoff and clears it on move exit; the fixture covers interruption and two
independent fighters. A live run fired on two separate specials after returning
to Wait, and all 672 inputs replayed without divergence. Broader corrected
rollback across projectile birth and destruction remains open.
The correction probe can now target recorded frames 240-591. On the authored
fighter, late corrections at frames 493 and 501 around its first neutral
special reproduced the complete tracked state across two full-draw passes;
the precise article birth/destruction frame is not yet independently labeled.
Original action editing also exposes post-creation hitbox scale/flag commands
and source-linked interrupt/cue markers. One isolated Mario Attack13 scale
change modified a single DAT byte, registered as a package, and undid cleanly;
its live hit result and the later hitbox-flag setter remain unproven.
The animation backend can now locate a Mario draft's original action clip,
enumerate its joint tracks and encoded keys, change a representable key value
in place, and sample the result through Melee's own FOBJ evaluator at integer
and fractional frames. An isolated SpecialN edit changed two animation-bank
bytes; at frame 11 the evaluated value changed from 0.118713379 to 0.25, and
an edited package registered. Separate animation undo restored the original
bank SHA-256. The same path can edit an exactly representable spline tangent:
changing a SpecialN tangent changed two bank bytes and the evaluator's frame-1
sample, then the package registered and undo restored the source hash. This is
source-preserving key-value/tangent editing, not yet key insertion/removal,
interpolation editing, in-game gizmos, Blender round-trip, or full
pose/attachment comparison. One bounded timing operation now moves an existing
timed key between neighboring timed keys by changing two same-width wait
values; an isolated SpecialN frame-11-to-12 move changed two bytes, altered the
native evaluator's interpolation, registered as a package, and undid cleanly.
Timing edits across packed-width boundaries and full clip rebuilding remain
open.
The neutral-special authoring subset now accepts state-entry-only handlers and
an article-only cue without a dummy zero-speed instruction. A generated module
preserved movement while invoking Mario's inherited article once per move in a
callback fixture; the isolated package built and registered. This improves the
first template but does not add a new article type or broaden state coverage.
The original subaction editor also exposes SetCommandVar by source offset,
keeping its variable index fixed while changing its 24-bit value. An isolated
Mario SpecialS edit changed one fighter-DAT byte, registered as a package,
rejected an out-of-range value, and undid cleanly. Its live move behavior has
not yet been checked. All 303 Mario action listings and six targeted
action/script/animation tests pass.
The readable script compiler also supports a single stock side, up, or down
special with ground/air facing-speed changes after the original entry callback.
Mario's clone definition now supplies those six callback entry points explicitly.
Generated-module tests cover all three families and isolated draft packages for
each registered against the ISO; all 366 native tests pass. Authored states/cues
for those specials and live match behavior remain open.
Original action editing now includes the game's body collision, all-bone hurt,
and targeted bone hurt state commands, preserving target bone IDs and limiting
state values to those handled by the decomp. Isolated Mario edits changed one
DAT byte each, registered as packages for body and targeted bone states, and
undid cleanly; live collision results still need checking.
A first source-independent normal-action transfer between two Mario drafts now
copies the reachable original command graph, complete clip and action flags
only when archive geometry, pointers, and exclusive ownership match. An edited
Attack11's hitbox and animation arrived together in a second draft, built as a
registered package, remained independent of a later source edit, and undid
back to the original file hashes. The integration test rejects mismatched
command topology and preserves unrelated destination attributes. The backend
also copies the three stock actions of Mario's jab, forward tilt or forward
smash as one in-memory group; an edited jab group built and registered as an
independent package. All 367 native tests pass. This does not satisfy general
move copying yet: non-stock state groups, changed archive layouts,
cross-skeleton retargeting, authored behavior, articles, effects, and atomic
multi-file draft saves are still open.
The readable script compiler now composes one neutral, side, up and down special
block in a single fighter module. A generated-module test combines an authored
neutral fireball cue with three stock-entry specials, checks their callbacks and
whole-source hash, and rejects duplicate families and missing inherited entry
points. An isolated Mario draft built the combined DLL and its package
registered and released against the ISO. This is backend composition, not
live-match proof or general authored behavior for all specials.
The original action backend now also identifies opcode-17 sound events and
edits their ID, volume and pan without changing command shape. An Attack11
three-byte edit passed source validation, exact byte comparison, undo and
package registration. Random/stage sound families, bank validation and audible
playback still need work.
Five-word original GFX spawn commands now also expose effect ID, bone,
lifetime, offset and range edits while retaining unknown fields. An isolated
Mario forward-smash offset edit changed one source DAT byte, undid exactly and
registered as a package. Importing new effect assets and checking its changed
live visual result remain open.
The scripted Mario bundle probe now validates actual registered module
callbacks and per-slot motion tables in two custom fighter slots instead of
requiring stock callbacks. An integration test compiled and bundled the
four-special script from a project path with spaces, rejected stale saved
script/module pairs in both packaging paths, rebuilt a distinct valid package,
and confirmed a data-only bundle still passes. It does not prove simultaneous
live matches with both scripted fighters.
Normal move copying now transfers each of Mario's five aerial attacks with its
separate landing action. An isolated down-air source with edited attack and
landing hitboxes plus a landing animation key copied byte-for-byte into an
independent draft; the package registered and released against the ISO. The
integration test covers all five aerial groups and undo. Cross-layout or
cross-skeleton transfers, code and external asset dependencies, crash-atomic
multi-file saves, and live attack/landing parity remain open.
The original throw-hitbox opcode is now listed by source offset and supports
damage edits while preserving its ID and other words. A Mario throw edit
changed one DAT byte, rejected wrong-action and out-of-range changes, and
undid exactly. Other throw fields and live behavior remain open.
A scripted Mario package now passes an isolated installation and recloning
test after the author's draft directory is removed. A second store accepts
and reclones it with byte-identical fighter, animation, costume, portrait,
module and script content; the recipient independently changes weight and
builds a different valid package using its included source. This proves one
local distribution path, not handwritten-C source packaging or live behavior.
A direct one-stock Battlefield run with an installed scripted Mario against CPU
stock Mario reached results and returned to CSS; it did not exercise a special
move. The Mario clone audit then found its inherited OnLoad callback registering
package-owned fireball/cape articles into stock Mario's global item slots. Native
custom Mario now retains those articles on its own imported fighter data and
selects them by spawning fighter at item creation. A focused stock-plus-two-
clones test preserves stock globals and resolves distinct articles for all
three; live projectile parity remains open.
The clone entered neutral special in a live match without stock Mario, and the
user observed its projectile in a separate rendered match. The runtime now
rejects a Mario package missing its fireball or cape import and refuses to use
stock Mario's article as a fallback for a custom-owned item. This establishes
the observed spawn path, not exact projectile parity or simultaneous stock and
clone item identity.
A four-player Battlefield smoke run with two instances of one installed Mario
clone, stock CPU Mario and Zelda reached results. The CPU clone used cape and
stock Mario entered neutral special; all 11,297 recorded controller samples
replayed without tracked state/RNG divergence. Item article pointer identity
for separate clone packages remains unverified.
The installed Mario was recloned and given a different weight, yielding a
second package ID. Both distinct clone packages then played in one four-player
match alongside stock Mario and Zelda through results; the 14,021-sample
recording replayed without tracked state/RNG mismatch. Exact per-package item
article identity was not measured in that run; the later targeted probe below
checks Mario fireballs. Visible confirmation of the weight edit remains open.
The semantic digest now covers a custom fighter's inherited stock behavior
state and authored module-state bytes. Focused tests detect Mario's tornado
charge, cape item relation, and changed authored state. A fresh four-player
match with both distinct Mario packages, stock Mario, and Zelda reached results;
all 18,849 samples replayed without tracked state/RNG mismatch. Live rollback
correction now also passes in a four-fighter match containing both clone
packages. A two-clone controlled-input recording enters the original clone's
scripted neutral special at sample 233; late-input corrections at frames 245
and 250 reproduced the complete tracked endpoint state in both runs. The
specific authored marker and projectile creation/destruction within the
corrected interval are now observed directly by the probe: the two-clone
recording gains P1's authored marker and P1-owned Mario fireball at frame 244,
resets the marker at 274, and removes the fireball at 319. Forced corrections
from frames 240 and 315 respectively repeat creation and removal with matching
per-frame observations and complete tracked endpoint state. This uses Mario's
inherited article constructor; independent package-defined articles, safe
module replacement, and networked correction acceptance remain open.
A second two-clone fixture fires both neutral specials one sample apart. The
resulting P1 and P2 fireballs appear at frames 244 and 245; each item's article
pointer matches its spawning clone's imported fighter article table, with no
article-pointer mismatch observed. They disappear at frames 319 and
320. Corrections from frames 240 and 315 repeat both creations and removals
with matching tracked endpoints and per-frame ownership observations. This
proves the simultaneous Mario-fireball article path for two distinct installed
packages, while reflection and other article families remain unverified.
Candidate fighter packages are now validated in detached storage during match
preflight, leaving any active practice fighter registration intact. The full
374-test suite and a two-clone scripted-special rollback correction replay pass
after this change; the live UI practice-to-match transition still needs testing.
Preview replacement now remembers the prior working cached package through the
practice-to-CSS-to-practice reset. If the new package fails CSS-side validation,
the transition retries the old preview once before falling back to the original
fighter selection. The client builds and all 374 tests pass; live UI recovery
has now also been exercised. A live control-channel session applied a valid
1.4-to-1.8-scale preview
replacement, rejected an invalid scale while retaining the 1.8 preview, and
used a temporary fault at CSS-side package validation to recover the prior
1.4 preview after scene exit. The test hook was removed, the normal client
rebuilt, and all 374 native tests passed. Home practice and lobby-match setup
now import selected packages into detached slots before leaving the current
scene, including the stock skeleton-part check; later scene startup only binds
those slots. A fresh CSS test with an isolated content store selected an
installed Mario clone and entered practice with a stock Ness P2, and a live
preview replacement also remained playable. The 374-test suite passes
serially. Initial direct command-line matches also stage after native archives
open; a live installed-clone practice launch succeeded, while an injected
pre-scene failure exited with code 1 before registration. A later direct-mode
rematch still has the older import path, and the failed-recovery-retry path
remains untested.
`melee/docs/native_character_workshop_progress.md` records the tested path and
limits; the M3 workshop checkbox remains open.
- [ ] **M4 â€” active online matches:** four fighter slots across mixed peer layouts; test prediction/resimulation under latency, jitter and loss; abort to lobby on active disconnect; verify direct and relayed routes plus signaling-outage behavior.
  A 2026-09-24 same-machine raw-UDP disconnect check reached gameplay with four
  fighter slots, then force-terminated the guest. The host timed out after about
  five seconds and returned to offline character select through No Contest;
  it did not award a result. Persistent-room return, spectator-only departure,
  transient loss, and cross-network routes remain open. See
  `melee/docs/native_netplay.md`.
  A separate same-machine 1-minute round sent both clients through a seeded
  loopback proxy that dropped 411 of 7,734 datagrams and delayed delivery by
  12â€“30 ms. Both clients reached the same results and compared thousands of
  confirmed checksums without rejection or logged divergence. Input stayed
  neutral, so this run did not exercise rollback correction under packet loss.
  A second one-minute run added changing P1 virtual-pad inputs and three frames
  of outbound delay under the same fault profile. The guest corrected five
  timelines over 14 hidden frames, both clients reached matching results, and
  neither logged a checksum mismatch. Loss bursts and cross-network acceptance
  remain open.
- [ ] **M5 â€” social flow:** spectators and mid-match catch-up, 1v1 rotations, manual teams/FFA, per-install collections, auto-cache versus explicit gifting, and differing selectable rosters. Prove spectators/content traffic do not stall matches.
- [ ] **M6 â€” friend release:** updater, recovery to previous build, clean installation and launch, persistent profiles/collections, centered 4:3 canvas plus native side-rail presentation, replay metadata, and actionable diagnostics. Replace placeholder text in original-style game overlays (including custom fighter names on results) with reusable dynamic text matching Melee's lettering. Audit which original labels are glyphs versus whole textures; if necessary, derive a reusable glyph atlas with matching metrics, outline, color and spacing, then check short and long names in every native overlay. Verify that resizing and fullscreen scaling never alter the game viewport's aspect or gameplay visibility. Visually inspect changed UI/presentation and occasionally compare gameplay rendering with Dolphin. Validate on actual friend PCs before declaring release readiness.

Architecture for later milestones informs earlier work. The order does not authorize broad rewrites before their relevant source is understood. Complete and verify coherent slices; update this document with evidence and unresolved limitations as implementation progresses.

For the current local development phase, acceptance work uses one PC: isolated client installs, multiple native processes, virtual or physical local controllers, injected latency/loss, and local service instances or the private deployment. Do not hold local feature progress on recruiting or coordinating other physical PCs. Cross-network routes, physical multi-PC controller layouts, relay behavior under separate networks, and actual friend-install usability remain release gates to revisit when other PCs are available; local simulations must not be reported as proof of those conditions.

### M3 character workshop acceptance

These are required work, approved 2026-09-23. The handoff supplies the detailed
contracts and implementation order; checkboxes require recorded evidence.

- [ ] **Projects and faithful cloning:** independent Mario-based projects;
  draft save/reopen, undo/recovery, preview apply and package install; original
  fighter unchanged. Audit the full Mario clone matrix before calling him fully
  cloneable. Expand supported bases and readable stock templates incrementally.
- [ ] **Original action editing:** edit hitboxes, timing, flags and supported
  control flow through existing subactions; source-linked timeline and viewport
  picking; edit/save/reopen/play without rewriting the move as behavior script.
  Reuse HSDLib/HSDRawViewer's format knowledge and proven workflow where suitable.
- [ ] **Interpreted authored behavior:** direct source validation, typed bytecode,
  source-level diagnostics, owned package loading and safe replacement;
  persistent and state-local variables, article ownership and cleanup all
  restore correctly. Prove projectile, directional movement, charge/reset and
  interruption examples with the real engine, including hitlag and ground/air
  transitions. Retain handwritten C/C++ as the advanced authoring path.
- [ ] **Later whole-move copying:** after the base clone workflow and interface
  design, transfer states, subactions, animation, behavior
  and dependencies between projects with independent editable ownership;
  resolve references and bone mappings, seeded by Melee's shared-part mapping.
  Test a second supported skeleton and report unsupported native dependencies.
- [ ] **Animation authoring:** in-client posing, keys, curves and playback
  editing plus Blender import/export; preserve untouched tracks and validate
  edited poses, root motion and attachments at integer/fractional frames and
  loop boundaries using the native evaluator.
- [ ] **Usable workshop:** authored UI assets, resizable practice preview,
  contextual tools, connected Animation/Subactions/Behavior views, controller
  focus, play/pause/step/reset, reproducible input fixtures and clear errors.
  Validate the complete workflow in the rendered client with the user.
- [ ] **Packaging and local multiplayer acceptance:** source and runtime output
  travel together; isolated-install play and cloning without the author's
  project directory; exact compiled package agreement, two-instance state
  isolation, four-fighter execution and forced rollback across owned-object
  creation/destruction. Keep cache versus explicit installation/gifting intact.

An automatic converter for arbitrary stock C and a full DCC replacement are
not required. Syntax sketches are provisional; native module/state ownership,
source-of-truth boundaries and complete move dependencies are architectural
requirements rather than optional polish.

Room-seat checkpoint, 2026-09-23: players can cycle each of their local seats between fighting, waiting, and spectating from the room sidebar. The host can assign any connected seat, and its owner can change it again; the latest accepted choice wins. The service enforces the four-fighter cap and freezes active assignments during a published match. The native 3+3 signaling probe passed against the private deployment with an owner override and subsequent self-reassignment; the live client UI was checked on one installation with two local seats. A 2026-09-24 service and native-wire pass added individual waiting ranks. A three-PC service test shows that a seat retains its place while waiting, enters at the tail after leaving, and keeps its order across room hibernation. The matching service was deployed as `c9eab09f-238f-476e-b971-11ac7bee2cfe`; a fresh native 3+3 remote probe verified rank-preserving owner and self reassignment. All 374 native and 15 service tests pass. Result-driven rotation, live queue presentation, and broader match-controller acceptance remain open. See `melee/docs/native_room_players.md`.

Result-rotation backend checkpoint, 2026-09-24: signaling wire v5 stores a room rotation policy (manual by default, winner-stays, or winner-out). An active native client reports the final match outcome before releasing the borrowed room. The service atomically clears the proposal and, only for an eligible two-fighter result with a unique winner and a waiting seat, swaps the outgoing seat with the queue head. Canceled/failed matches, ties, teams, free-for-all, and stale duplicate reports do not rotate. The native client and service were built/deployed together as service version `2c539446-69e1-44b8-a6a5-11d050e6d032`; the remote two-client native signaling probe passed with an owner policy update and a synthetic result that rotated the host's active seat out and a waiting guest seat in. A later rendered match on one PC placed two local seats on the host and one on the guest; after host P1 won under winner-out, host P1 became Waiting and host P2 became Fighting on return to CSS. All 374 native and 17 service tests pass, including same-PC seat rotation. The host-facing policy/queue UI, promoted-seat physical-controller match, and multi-PC acceptance remain open; M5 is not accepted. See `melee/docs/native_room_players.md`.

Rendered room-match checkpoint, 2026-09-24: two native clients on the same Windows PC joined the deployed room through the normal UI, readied from the practice grid, and completed a four-stock Final Destination match. Both displayed the same winner and returned to the same open room with Ready cleared after advancing past results. Their logs recorded 2,415 and 2,403 received input packets, zero rejected packets, and 2,146 and 2,402 confirmed peer checksum comparisons. This validates the ordinary two-client match-completion and return path with no waiting seat; a live rotation and cross-PC acceptance remain open. An earlier room disconnected during repeated guest relaunches, with cause unverified. See `melee/docs/native_room_players.md`.

Four-fighter room checkpoint, 2026-09-24: two native processes on one Windows PC each selected two local fighters, readied from practice, and completed a six-minute, four-stock Final Destination match through the normal room flow. The host owned P1/P2 and the guest P3/P4; distinct virtual inputs moved their assigned fighters, and both rendered the same four-player results and returned to the open room with Ready cleared. Each received 5,499 input packets without rejection. The host corrected four timelines over eight hidden frames and compared 5,491 confirmed peer checksums; the guest corrected one timeline over four hidden frames and compared 5,488. This is live shared-PC rollback evidence, but physical multi-controller routing, waiting-seat rotation, cross-PC routes, and broader latency/loss acceptance remain open. See `melee/docs/native_room_players.md`.

M0 was accepted on 2026-09-20. The full decompilation builds as the linked
Windows executable in `melee/`, real retail archives are imported through typed
native paths, platform assumptions and remaining fidelity gaps are tracked in
`melee/docs/`, and all 46 repository-owned native tests pass. The playable
client and runtime evidence exceed the foundation milestone.

M1 was accepted on 2026-09-21. The Windows client enters the intended Versus
menu flow, supports controller-driven character and stage selection, and
completes stock, time, coin, and team matches through their original results
layouts and return path. All 29 selectable stages and every playable fighter
have runtime evidence; item-heavy matches, typed archive preflights, sustained
event audio, explicit costumes, and deterministic replays cover representative
interactions. Remaining fighter mechanics, ties, and rare item combinations are
tracked coverage gaps rather than blockers for beginning M2.
The common-item model draw gate was subsequently corrected: the original raw
GameCube byte `1` had enabled a different native bitfield flag, so items could
simulate without rendering. A live very-high-items Final Destination match
showed a PokÃ© Ball and barrel together after the named visibility flag was set.
The same build completed a two-client, very-high-items Battlefield time match:
both clients visibly rendered the items and showed Ice Climbers as the winner,
with 2961/3725 confirmed checksums and no mismatch or rejected input packet.
Aurora's shared Dawn and pipeline SQLite caches now wait for a competing writer
instead of disabling caching on a transient lock; compression occurs before the
Dawn write lock is taken. Two simultaneous client starts and a three-second
forced-lock startup completed without cache errors. Cold shader compilation
latency remains a separate performance task.

M2 portable-checksum checkpoint: replay and netplay now share an explicit
semantic digest for match timing/respawns, player stocks/staleness, fighter
input/physics/collision/combat/pose state, common item and ground state, stage
and item joint animation, camera transforms/quakes/mode state and camera
subjects, and RNG. Common fighter tail state includes the bury stage and timers
plus the integer pairs used by hammer state. Every named scalar and vector in
the persistent per-character fighter variables is included, while their tracked
game-object references use the stable object map. It omits native pointers and
padding; a new
test proves pointer relocation invariance and sensitivity to representative
fighter, item, ground, animation, staleness, camera-target, and subject-state
changes. Live fighter, item, and ground links use stable category-and-list-order IDs, covering held-item,
ownership, grab, reflector, damage-source, and hitbox-victim relationships
without native addresses. The release client and all 63 native tests pass.
The common walk, turn, dash, run, jump, fall, squat, landing, jab, rapid-jab,
dash-attack, down-tilt, and standard damage motion families also have
motion-ID-selected schemas, as does guard state including powershield and analog
shield timers, knockdown, tech, air-dodge, rebound, platform-drop, and ledge
state. Item pickup, all light/heavy throw directions and smash variants,
parasol-open history, and heavy-item lift state are covered as well.
Standard grab counters, fighter throws, grabbed wait/damage state, standard
thrown state, and the thrown fighter's motion-owned victim relation are covered
through the same motion-selected and stable-object-ID rules.
Carried victims and cargo throws, Yoshi eggs, Koopa/Mewtwo/Kirby captures,
bury, Warp Star, hammer and mushroom transitions, entry platforms, freeze,
Like Like capture, and barrel state now follow that contract as well.
A fresh two-process Battlefield run exercised the expanded checksum with a
three-frame outbound delay. It exposed and fixed a protocol-ordering error that
could publish or compare a confirmed frame while its local state was still the
speculative pre-correction timeline. After checksum exchange was deferred until
pending correction completes, peer 1 corrected four timelines across 22 hidden
frames; the two clients compared 2,005 and 1,226 confirmed checksums without a
mismatch and ended at sample 2,031 with identical fighter digest and RNG state.
Remaining action-specific fighter motion state, item/stage-specific unions,
allocator pool topology, broader cross-machine coverage, and match-transition
coverage keep M2 from acceptance.

Room setup now consumes the accepted canonical match proposal as its sole source
for session identity, numeric rollback peers, controller-port ownership, and
remote routes. Duplicate transport configuration for those values was removed.
The native client and 63 native tests pass, and the local Worker-to-signaling-to-
ICE probe still closes signaling and exchanges validated four-port shared-PC
input. Room startup now obtains bounded STUN and optional short-lived UDP TURN
configuration from the same Worker origin before creating ICE agents. The
long-lived Cloudflare key remains in Worker secrets. Workers-runtime tests cover
the credential exchange and malformed responses, and the live local probe
proved the STUN-only development response is consumed by both native clients.
A deployed Worker with configured TURN secrets and forced-relay cross-network
validation remains open. The menu lobby still needs to construct the proposal
from visible room state and selections. Room lifetime now separates initial signaling/ICE
connection from canonical match configuration, so a menu can stay connected
while presence changes and commit the selected match later. The existing
diagnostic path and live two-client probe use those same transitions. A native
lobby session now owns the presentation-independent policy: it exposes current
presence, accepts participant updates, restricts match publication to the room
owner, derives active peer/port ownership from live presence, and makes guests
adopt the canonical owner proposal automatically. The live probe now uses this
owner/guest flow through ready ICE and validated rollback input.

The first in-game online screen now starts and pumps that process owner from the
native Versus menu. The temporary Tournament Melee row opens create-room and
join-room choices; join includes an eight-character controller-driven code
editor, while the connected room view shows the code, connection state, live
participants, roles, room ownership, and failures. Leaving the screen or closing
the client shuts down the room cleanly. A live create-room run against the
deployed Worker displayed its assigned code and local owner, then returned to
the stock menu with its text removed. This functional bring-up occupies the old
right preview pane. Replacing the Tournament label and description, moving the
flow into the wider native side rails, profile names, local-player count, roles,
rules, chat, and post-match return/rotation remain open. A connected room can
now preserve its signaling session while entering stock character select. Each
PC controls only its advertised local slots, publishes stable stock
character/costume identities, and confirms with Start. Selection changes clear
that confirmation, and the scene advances only after every active PC has
confirmed a valid roster of at least two total fighters. A deployed one-client
run reached this screen, selected Ganondorf, published confirmation, and
correctly remained there because no second active PC had joined. The room screen
now prevents that confusing one-person transition and enables character select
only after active participants advertise two to four fighters.

The private signaling Worker is deployed at
`https://freak-melee-signaling.sndrec32exe.workers.dev` (version
`089ff6bf-bb71-42fe-9ff5-802bd38aaafb`). Its health and bounded credential
routes respond successfully. A live two-client native probe joined a deployed
Durable Object room, agreed on the owner proposal, closed signaling, and carried
validated shared-PC rollback input over ICE. The deployment currently returns
STUN-only configuration; provisioning Cloudflare TURN credentials and proving a
forced relay between separate networks remain open.

The process now has a presentation-independent online-session owner whose lobby
storage remains at one stable address across scene transitions. Once the room is
ready it adopts the canonical match configuration, and the active match borrows
that exact room instead of copying or reconnecting it; the diagnostic CLI keeps
its separately owned path. A deployed two-client stock match exercised the
shared adoption code, produced the same RNG seed on both clients, released both
signaling connections, entered Battlefield, and remained connected without a
checksum or transport failure during the observation window. The menu now starts
and pumps this owner and displays its room code, state, presence, and failures.
It also carries the connected room into character select and publishes each
PC's confirmed local roster. The room owner now selects the stock stage and
constructs the canonical proposal from that stage, combined roster, visible
rules, and item settings while guests wait on stage select. Both clients verify
and adopt the same descriptor before ICE and scene entry. A deployed two-client
run launched Captain Falcon versus Ganondorf on Battlefield as session
`337e943cc1098f95`, agreed on seed `ed9b0688`, released signaling, completed the
two-minute match, rejected no input packets, and compared more than 7,300 peer
checksums per side without a mismatch. The guest corrected two timelines across
four hidden frames. Home rooms now retain signaling through active play, clear
the completed proposal, and reuse the room/ICE connection for another round.
Results input follows the local-to-logical port mapping so a guest can dismiss
its own panel. A local two-client run completed an aborted first round and a
full two-minute second round, then returned both clients to practice in the
same room; round two compared 6360/6656 confirmed checksums. Separate repeat
runs diverged at frame 684 and 2059, so repeat-round determinism is still an
open M4 acceptance item. Spectator flow and rotations have not been validated.
A fixed 64-frame semantic-state breakdown
now prints category hashes on a checksum failure without logging every frame.
The next two-round run completed with 7433/7378 matching checksums. A forced
guest exit ended the host match after the five-second input timeout and returned
the host to CSS in the same room; a replacement guest joined and launched a new
match, which diverged at frame 768 before automatic capture was enabled.
Subsequent repeat-room traces isolated frame-zero false mismatches to a stage
object's uninitialized x0 field and its unused material alpha value; the
semantic checksum now excludes x0 and includes alpha only when the original
material flag enables its use. A later round advanced 645 synchronized frames
before differing in item hitboxes, and another advanced 2576 frames before
pinpointing a disabled hitbox slot on a Food item. The checksum now ignores
disabled item-hitbox payload and continues to check active hitboxes. Original
stage and collision behavior was not changed. Another round diverged at frame
1134 in a Bob-omb hurtbox; the checksum now excludes its uninitialized,
fighter-only bone index and reads cached positions only when collision has
updated them. A further full first round completed with 7435 confirmed
guest-side checksums, but its second round diverged at frame 1424 in a Capsule
item's damage/timer/counter group after its earlier fields and hurtbox matched.
The next trace isolated another second-round difference at frame 555 to a
Crate's stale damage angle while knockback was zero. Collision writes both
fields together; the semantic checksum now includes the angle only while
knockback is active. A subsequent round exposed unused, unwritten Super Scope
script scratch words as another false checksum difference; the native digest
now excludes those words for Super Scope while retaining its active flags and
the same word for items that use it. A fresh two-client run then completed two
consecutive two-minute Battlefield rounds in one room, passed through results,
and returned to CSS without a checksum mismatch. The second round had short
timeline corrections and three rejected guest input packets. Broader character,
item, and stage coverage remains open.

A four-human, two-client direct-UDP Brinstar run then exposed a stage-data
layout failure: the rising-acid callback reached a missing collision descriptor
and both clients crashed at the same instruction. The `GrZe.dat` parameter has
a relocated environment-hit pointer at GameCube offset `0x2C`; native import now
resolves it into the expanded pointer layout and the stage reads a named field.
The archive test verifies its 14-damage, 90-degree payload. A fresh one-minute
Brinstar run with high items reached time-out on both clients with
3186/3616 matching confirmed checksums, zero rejected packets, and short
rollback corrections. All four fighters tied; the subsequent sudden-death
scene was not yet netplay-enabled and displayed No Contest. This broadened
direct-UDP stage/fighter evidence. A second four-human, two-client Brinstar
run with one stock and high items reached a Ganondorf win on both clients;
their identical winner screens and 1997/2129 matching confirmed checksums
establish the winner path for this direct-UDP configuration. Repeated Brinstar
room flow and cross-network play remain open. Timed ties now keep the existing
connection through Melee's separate sudden-death scene and start a fresh input
timeline without reseeding gameplay. A one-minute four-human Battlefield tie
then reached the same Ganondorf winner screen on two direct-UDP clients, with
zero checksum mismatches and zero rejected packets in either scene. Both
ignored one late old-scene packet. A second timed tie used local signaling and
ICE room transport, kept its established connection through sudden death,
and reached the same Ganondorf winner on both clients with zero checksum
mismatches or rejected packets. A normal sidebar create/join run also selected
fighters, readied in practice, launched from the host's Start match button,
played a two-minute tie and sudden death, and showed the same Dr. Mario winner
on both clients. It compared 5976/5977 confirmed checksums before the tie and
1643/1643 in sudden death, with zero mismatches, corrections, or rejected input
packets. Both returned to CSS in the same two-person room and reset readiness.
Cross-network sudden death and physical shared-PC controller acceptance remain
untested. An audit of
37 selectable-stage parameter blocks found no other missing generic pointer
declarations; Icicle Mountain's three relocated fields use its own typed
importer. Generic stage import now rejects any undeclared relocation inside a
parameter block, and the Brinstar archive test verifies that failure.

Lobby topology is now derived deterministically from room presence rather than
typed peer masks: active PCs are sorted by join order, receive compact rollback
peer IDs, and own the human ports implied by their local-player counts. Waiting
players and spectators remain outside the match. CPU and empty ports receive one
deterministic sender, while incompatible builds, duplicate identities/order, too
many active PCs, and human-count mismatches reject the proposal transactionally.
The live connection probe now exercises this policy with active players plus a
spectator.

Native model-runtime checkpoint: the full 993-source game/HSD archive builds and
all six native tests pass. Mario's original costume now creates and releases real
HSD objects with 61 joints, 68 polygons, and 379 resolved skinning weights; joint
matrices are finite. This follows the 720-record descriptor import. Rendering,
full numerical fidelity, and game startup remain unverified. See
`melee/docs/native_build.md` and `melee/build/native/mario-model-runtime-results.log`.

Native GX checkpoint: polygon imports now derive and validate indexed vertex
buffer extents, and native submission uses Aurora's array-size/byte-order ABI and
FIFO functions. Real command-recording checks and all six native tests pass;
Mario's model still loads and releases. The full archive now contains 994 source
units. No rendered-frame or game-startup acceptance is claimed.

Native texture checkpoint: corrected texture/palette object storage to match
Aurora, verified overwrite canaries and native pointer preservation, and recorded
original HSD texture setup for Mario's 58 textured materials. All six native tests
pass. Persistent texture ownership is next; there is still no rendered acceptance.

Native texture-lifetime checkpoint: HSD now owns/reuses native texture and palette
resources, updates animated image pointers, replaces changed layouts, and emits
resource destruction on release. Synthetic identity/replacement checks pass, and
Mario releases exactly 60 textures and two palettes after repeated setup. All six
native tests pass. Renderer execution and unload synchronization remain unverified.

Native rendering checkpoint: `native_model_view` uses the full HSD runtime and
Aurora to render textured, lit Mario in bind pose on local D3D12. The window was
visually inspected; a final 120-frame startup/shutdown run exits successfully and
all six native tests pass. Evidence: `melee/build/native/native-model-view.png`
and `native-view-smoke.log`. This is diagnostic model rendering, not original
startup, animated-model parity, or versus gameplay acceptance.

Native animation checkpoint: original Mario idle FigaTree data (61 nodes, 111
tracks, 50 frames) imports and plays through the existing Melee/HSD animation
routines in the viewer. A 1,800-frame run exits successfully and a rendered pose
was inspected. The synthetic playback/restart/bounds checks and all six native
tests pass; archive coverage is 995 sources. Fighter part masks, blending,
animation events and state-machine integration remain unfinished.

Native scheduler checkpoint: viewer animation/rendering and model/camera ownership
now run through original GObjs. Callback-order, high pause-mask, self-removal and
cleanup checks pass, alongside all six native tests. The animated GObj-driven
viewer completed 1,800 frames and was visually inspected. Fighter creation and
stage/gameplay scene initialization remain unfinished.

Stage collision descriptor conversion now produces writable native copies of
the original vertex, line, and joint tables. Five original stage archives decode,
including dynamic line categories, and all seven native tests pass. The complete
archive covers 996 game/HSD sources. Collision-system initialization and actual
fighter/stage interaction remain the next integration work; descriptor conversion
alone is not gameplay acceptance.

Original collision initialization and floor-query integration now pass on the
native heap. Five decoded stage archives initialize; synthetic floor sweeps and
endpoint queries pass with no query allocations. Fixed native record indexing
and island allocation sizes, and separated original debug drawing/state from
startup dependencies. All seven native tests pass; full archive coverage is 999
sources. Fighter ECB collision and dynamic stage behavior remain unfinished.

A synthetic moving-platform check now exercises original HSD joint binding,
translation/nonuniform scaling, previous collision coordinates, transformed
floor queries, and visibility changes without update allocations. All seven
tests and the original Mario model lifecycle pass. Complete stage animation
scripts and fighter response to moving platforms remain unfinished.

The original fixed-ECB landing routine now runs against native collision data:
normal/fast falls, original contact offset and flags, and platform misses pass
without simulation allocations. Shared utility definitions have been separated
from unrelated startup dependencies. All seven tests pass and the full archive
covers 1,003 sources. A complete fighter instance and its state callbacks are
still not running; this is collision-response integration only.

Joint-derived ECB integration also passes: a real HSD marker skeleton updates
its collision box with a changed pose and lands on a transformed platform using
the original collision routines, without simulation allocations. All seven
native tests pass. Actual fighter marker mapping, asset initialization, and
state callbacks remain unfinished.

The original fighter collision initializer now runs on a minimal Fighter data
fixture using decoded collision parameters and real HSD joints. Mario's original
collision/ledge record decodes successfully; its indices remain explicitly tied
to the fighter part table. All seven tests pass; archive coverage is 1,005
sources. Full Fighter_Create and character asset initialization remain unfinished.

All 33 common fighter part mappings and optional omitted-slot records now decode
into bounded native storage. Missing-slot markers and omission metadata are
preserved, with regression coverage. Original part setup still needs to consume
the decoded tables; the native client remains short of a complete fighter.

Original semantic-bone, cross-table remapping, and omitted-slot lookups now run
against decoded native tables, including the highest omission mask bit. Full
part setup additionally installs the original fighter material class; that
integration remains outstanding. No replacement part-setup path was added.

Fighter material portability corrections remove reads beyond the class object
that relied on adjacent globals, retain integer shader-register selection, and
use the correct callback signature. Build and existing tests pass; actual
fighter-material rendering is still unverified.

Original material register utilities now pass nonboolean-index and exhaustion
checks. The color-overlay selector also preserves full native pointer width.
The complete fighter material callback has not yet been exercised.

The original opaque fighter light-overlay routine now executes and emits GX
commands with the correct register/template, without allocation. Existing tests
pass, including the Mario model lifecycle. Complete material callback rendering,
refraction, and fighter part setup remain outstanding.

Refraction texture callbacks now use consistent types and explicit GameCube
byte order, with runtime checks for tiled RGB565, IA8, and RGBA8 output. Another
adjacent-global layout assumption is removed. Full refraction initialization
and rendered-effect acceptance remain outstanding.

The original fighter material class now installs on native model materials and
its complete setup callback executes for ordinary, reduced-alpha, custom
pixel-state, and color-overlay cases without draw-time allocation. Mario's 58
textured materials pass and release their resources. Refraction and replacement
material branches, rendered appearance, and full fighter part setup remain open.

Original fighter part setup now consumes Mario's decoded common part table and
binds 61 model joints and 59 draw objects, including material class installation.
Joint depth, draw ranges, render flags, material execution, and resource release
pass native checks. Full fighter creation/combat and other fighter models remain
unverified.

Mario's original model, decoded part mapping, and collision parameters now run
together through original part binding and fighter ECB initialization. Marker
references, ledge snap values, local collision extents under world translation,
and absence of evaluation allocations pass. This covers the starting model
pose, not an animated combat state or complete fighter creation.

Original fighter joint and polygon classes now execute using Aurora's native
matrix operations. Mario's polygon matrix setup passes ordinary and Z-scale
branches without allocation, alongside part, material, collision, and resource
checks. Rendered scaling/refraction effects and animated combat remain open.

Skinned display lists now occupy writable model-arena storage so the original
fighter/refraction loader cannot modify source DAT bytes. Import ownership and
mutation checks pass, and primitive rewrites reject truncated lengths. Complete
refraction effects and additional vertex layouts remain unverified.

Polygon matrix cache bookkeeping now retains valid slots and refreshes shared
matrices when either joint identity or mode changes. Both HSD and fighter paths
pass cache-hit/miss runtime checks alongside original Mario integration.

The native diagnostic scene now renders the installed fighter joint/material/
polygon classes with Mario's idle clip. A captured frame and 900-frame completed
run provide visual/runtime integration evidence. This remains a diagnostic
camera/lighting scene, without fighter controls or combat.

Original fighter animation initialization now creates and binds Mario's separate
61-joint interpolation skeleton without geometry. Independent poses and release
pass native checks. Blending, full animation callbacks, and the general
multiple-instance descriptor lookup contract remain unfinished.

Original fighter pose blend/copy functions now operate between native skeletons,
with checks for weighted translation, quaternion rotation, part exclusion,
direct-copy flags, matrix invalidation, and no allocation. Action-driven
animation transitions remain unfinished.

Mario's fighter preview now binds its decoded idle clip through the original
fighter animation dispatcher and part-selection path. A 300-frame run exited
successfully. Full action timing and cross-character remapping remain open.

The fighter preview now advances through original pose traversal and frame
requests. Rate, first-tick behavior, part exclusion, removal, and no-allocation
checks pass. Root motion, action processing, and blend timing remain open.

The native preview now uses the original fighter animation update. Current-frame
tracking, disabled state, no-animation queries, and controlled scaled root-motion
extraction pass. Physics consumption of root motion, real clip transitions, and
action-script processing remain open.

The original blend-timing branch now completes a ten-frame transition toward a
real idle clip on the interpolation skeleton in the native preview. The run
presented 300 frames and exited successfully. Full state-transition setup and
action-script triggers remain open.

The preview now uses original transition initialization for track removal,
pose reset, looping, rate, and blending. A real-idle transition completed in a
successful 300-frame run. Full character metadata and action-driven creation
remain open; preview model-scaling metadata is still a minimal fixture.

Common fighter attributes now decode into their original native type. Mario's
preview uses decoded model scale 1.1 and scale part 42 instead of placeholders;
its blend run still completes successfully. Remaining costume/visibility data,
full fighter creation, physics, and combat remain open.

Original movement helpers now execute with decoded Mario attributes. Gravity,
fall speeds, drift limits, friction, and stick-acceleration checks pass without
allocation. Full physics-tick and state-specific movement integration remain
unfinished.

Original grounded movement now uses surface friction and floor-normal
projection, with Ice Climbers exemptions and missing-floor behavior verified.
Complete physics-tick integration with stage collision remains unfinished.

## 11. Tooling and working authorization

The user authorizes routine local tooling work needed for this project: cloning repositories, downloading tools, launching programs, taking screenshots, and preparing MCP integrations. Proceed without repeatedly requesting those permissions. Keep changes purposeful and scoped; this does not authorize unrelated deletion, external messages, or publishing private content.

Commit coherent native-port progress frequently to local Git. Never push upstream. When finished with a task-owned Dolphin process, force-kill its verified PID: normal stop/close can leave it spinning on one CPU core on this PC. Do not terminate unrelated Dolphin sessions.

See `TOOLING.md` for the verified local inventory, Dolphin probe, and HSDRaw build command. Use the existing Dolphin MCP and command-line tools before adding custom integrations. Store local emulator settings, captures, and tool outputs in `.local/`, separate from source and distributable game files.

## 12. Reference material

- Aurora: https://github.com/encounter/aurora and local `aurora/docs/building.md`; verify build options against actual CMake definitions where documentation differs.
- Melee decompilation: https://github.com/doldecomp/melee
- m-ex: https://github.com/akaneia/m-ex
- Rollback requirements: https://github.com/pond3r/ggpo/blob/master/doc/DeveloperGuide.md
- Slippi reference implementations: https://github.com/project-slippi/slippi-ssbm-asm and https://github.com/project-slippi/Ishiiruka
- Cloudflare room connections: https://developers.cloudflare.com/durable-objects/best-practices/websockets/
- Cloudflare relay service: https://developers.cloudflare.com/realtime/turn/
- Cloudflare release downloads: https://developers.cloudflare.com/r2/objects/download-objects/

References inform design; their capabilities are not evidence that this client has those capabilities implemented.

### Native physics position integration

Extracted the original velocity decay and position integration body from
`Fighter_procUpdate` into `ftCommon_IntegrateMovement`; the original tick calls
it at the same point between the state physics callback and wind evaluation.
Native vector addition uses Aurora C_VECAdd. Arithmetic and branch order are
preserved, including the invisible-ceiling shield-knockback quirk. Original
animation movement-gate and joint rotation accessors now reside with their
respective update/rotation code, avoiding unrelated archive dependencies.

Native tests cover acceleration consumption, airborne knockback decay, nudge,
damage velocity interpolation, throw-triggered accumulated movement release,
the original shield-knockback quirk, and no heap allocation. All seven native
tests pass; viewer and model-runtime diagnostics relink successfully. Build
log: `build/native/integrate-build.log`. A source comparison confirms the moved
integration body differs only in native vector calls and explicit clearing.

This does not yet execute the complete Fighter_procUpdate: damage/KO, collision
notifications, sound, wind and callbacks still need full runtime integration.
The unchanged-animation accumulation branch and in-game parity remain unverified.

### Native wind and platform carry

The original ftColl_GetWindOffsetVec now resides with the stage device registry
in ftdevice.c; the original environment eligibility/cooldown functions reside
in ftCo_Environment.c. Fighter_procUpdate still calls the same functions in the
same order. Wind registration now accepts its actual callback type. Castle and
Garden callbacks use Ground_GObj pointers for their unused first parameter,
removing the mismatched void-pointer/integer declarations without casts.

All seven native tests pass. The collision harness exercises original registry
reset/registration, empty output, active/inactive callbacks, wind immunity,
type-2 cooldown and capture/barrel exclusions, and cooldown expiry. It also
executes original mpGetSpeed for static and translating floors, independently
moving endpoints, endpoint clamping, and missing floor. These checks allocate
no heap memory. Both model diagnostics relink. Evidence: build/native/wind-build.log
and build/native/platform-carry-build.log.

The wind callback itself is a controlled test fixture; actual stage wind/water
callbacks are compiled but not run here. Platform endpoints are supplied by the
fixture, so this is evidence for carry calculation, not an animated-stage or
full-fighter-tick acceptance test. Full damage/KO and fighter tick integration
remain outstanding.

### Combined native fighter position phase

Fighter_procUpdate now directly calls Fighter_UpdatePosition for its original
position phase, retaining the disabled-fighter guard before it and the full KO,
crowd and capsule processing after it. The phase contains original cooldown
updates, state physics callback, velocity integration, wind evaluation, ground
push, hitlag callback, platform carry and final wind application in original
order. Native vector calls and explicit vector clearing are the only changes
to the extracted body (verified by source comparison). Collision push and
hurt-capsule invalidation functions reside in ftcollision_update.c.

Native tests execute the combined phase with controlled callbacks, checking
ordering and final position. Hitlag pauses self movement, cooldowns and wind,
but retains ground push, hitlag callback and platform carry. Tests exercise
self movement plus translating-platform carry and hurt-capsule invalidation.
All seven tests pass without per-tick heap allocation; both model diagnostics
relink. Evidence: build/native/physics-phase-build.log.

This is not full Fighter_procUpdate or a playable fighter: KO processing,
crowd/sound effects and full state-machine/collision scheduling still need
integration. Test callbacks and platform endpoints are controlled fixtures.

### Native grounded fighter collision

Original grounded collision routines now reside in ftcollision_ground.c, and
original ECB rescaling resides beside initialization in ftcollision_init.c.
The native harness runs Fighter_UpdatePosition followed by ft_80082708 on its
floor fixture: a one-unit step stays supported, and a twenty-unit step leaves
the floor during subdivision at x=11 (rather than retaining requested x=21).
This exercises the original position synchronization and collision engine.

ft_80082708 previously used GroundOrAir for a support boolean: nonzero means
supported and zero triggers falling in callers. Its return type is now bool;
existing comparisons to GA_Ground (zero) are direct negations. A source check
verifies every modified character call site retains its original branch.
No numeric return behavior is changed.

All seven native tests pass and model diagnostics relink. Evidence:
build/native/ground-phase-build.log. Full Fighter_procMap, wall/ceiling cases,
state transitions after loss of support and in-game parity are not yet proven.

### Original startup is now the integration priority

Added the explicit melee_native build target using the original gmmain.c entry
point and full runtime archive. The target currently fails to link; it is not
a playable client. See docs/native_startup.md for the traced startup path,
remaining 97 unresolved symbols, and the ordered work toward original scene
initialization. The viewer remains a diagnostic, not an alternate client.
Original game exp/pow routines now have distinct symbols, and remaining linked
PPC matrix/vector calls use direct Aurora C implementations. Seven native tests
pass; diagnostics relink. Client link is the next acceptance gate, followed by
actual startup and scene execution, not further isolated preview expansion.

### Original entry now initializes Aurora

The original gmmain.c uses Aurora's main entry and initializes Aurora before
its existing OS/HSD/game sequence. Removed the console USB server startup wait,
per-frame development-server polling, unused PPC/EABI setup and 48 MB console
arena carveout. Replaced two compiler-runtime/data references with native C
operations. All game sources compile and seven native tests pass; the client
still fails to link on 95 unresolved symbols. Native event/presentation/shutdown
integration and actual startup remain pending. See docs/native_startup.md.

### Original scene loop uses Aurora events and frame submission

Adapted gm_801A4D34 directly: it polls window events while waiting for controller
input, retains original simulation/GObj/draw ordering, and submits screen frames
through Aurora with HSD clear/filter/gamma settings. Window-close state unwinds
the scene/mode loops to main and Aurora shutdown. This compiles; the client still
fails to link on 95 unresolved symbols. Seven existing tests pass, but they do
not cover this loop. Alarm-driven input pacing and VI startup remain outstanding;
actual boot, presentation and shutdown are unverified. See docs/native_startup.md.

### Native HSD video startup

Original HSD startup now initializes native video settings without allocating
console XFB/FIFO storage or installing retrace callbacks. Removed obsolete
allocation functions/options and the startup retrace wait; main calls
GXInit(NULL, 0). The native model-runtime test executes HSD_VIInit and checks
settings plus empty console queues/callbacks. Seven tests pass; diagnostics
relink. The client still has 93 unresolved symbols. Legacy movie,
error-screen and screenshot XFB consumers, input pacing and actual boot remain
pending. See docs/native_startup.md; evidence: build/native/video-init-build.log.

### Native startup diagnostics and initial input snapshot

Converted remaining executable console panic calls to HSD_Panic. Replaced
console linker-stack scanning with Windows current-thread stack bounds and
removed console scheduler validation from the frame loop. Initial controller
selection now pumps Aurora events and reads its synchronous pad snapshot,
without retrace/card-hardware waits. Early window close returns through main.
All seven tests pass, including the real native panic path; full client launch
and stack reporting are not runtime-verified. There are 89 unresolved symbols.
Evidence and remaining work: docs/native_startup.md.

### Audio backend boundary established

Audited original HSD synth/driver usage against Aurora: this checkout does not
supply AX voice mixing. docs/native_audio.md identifies original callback,
stream, sample-address and direct voice-state dependencies; native audio needs
real decoding/mixing/output, not empty AX substitutes. HSD's existing sound-mode
state now defaults to stereo and retains setter-driven active-node mix updates,
without console OS sound-mode storage. Persistence and audible playback remain
pending. Seven existing tests pass (not audio coverage); client link has 87
unresolved symbols. Evidence: build/native/audio-audit-build.log.


### Startup destination clarification

Native startup should open the Versus Mode menu before character selection,
with Melee highlighted. Skip the opening movie, title and console cable prompt.
Preserve required initialization. The original boot-to-menu routing is now
changed in source; runtime acceptance and native save/profile work remain.

### Native Kirby copy runtime checkpoint (2026-09-20)

Direct match launch now seeds the preload cache from its requested stage and active slots before scheduling loads. Player character lookup no longer reinterprets adjacent string globals as the GameCube linker layout; all affected paths read the typed fighter mapping table. Kirby merged copy-model insertion now indexes native pointer arrays by element instead of assuming four-byte pointers.

A Game & Watch swallow recording exposed a final archive-layout dependency: one serialized visibility lookup was also used by Kirby's two-model body, and the original code effectively treated the following high-address GameCube pointer as a negative, empty second count. The typed importer now materializes that empty native entry explicitly and tests its seven draw indices and three color words. The exact 6,479-sample crash recording reaches its endpoint without RNG or fighter-state divergence. A fresh live run visibly acquired Game & Watch's black copy form and remained stable under copied neutral-B input. All 44 native tests pass. Commit evidence is in the nested `melee` repository; full copy-roster runtime coverage remains open.

### Native item attribute bitfields (2026-09-20)

An item-heavy roster sweep found Pichu trying to swing a Proximity Mine and entering the original illegal-item panic. `ItemAttr`'s first two serialized bytes are PowerPC bitfields; direct byte copying into MSVC reversed their field meanings. The item importer now decodes all nine fields explicitly from the disc masks. The real-ISO test covers all 97 non-null imported item attribute records and known throwable, swingable and shootable action kinds. The corrected four-character high-item-rate run remained live for 40,106 recorded controller samples, and a deterministic 2,000-sample slice replays through the former sample-927 crash window. All 44 native tests pass. Broader runtime item coverage remains open.

### Native Yoshi shield layout and numeric conversion semantics (2026-09-20)

Yoshi's shield initialization no longer reinterprets adjacent GameCube-sized visibility records as a packed anonymous structure. It directly reads the second `TempS` variant through the typed `FtPartsVisLookup`, and the real-ISO fighter test validates both visibility groups for every Yoshi costume. The repaired Sheik/Pikachu/Yoshi/Jigglypuff lineup runs and deterministically replays past its former creation crash.

The ensuing item-heavy run exposed Wobbuffet's damage accumulator crossing below zero. The retail compiler routes unsigned floating conversions through a saturating helper, but the native compiler previously treated the out-of-range cast as undefined and produced `UINT32_MAX`, yielding a `2^32` floating hit. The decompiled game target now requests Clang's defined saturating float-cast behavior across the source set, matching the 93 corresponding helper calls in the retail executable without changing Wobbuffet logic. A boundary test covers float and double inputs. The exact 6,812-sample crash replay now completes, and a fresh 20,129-sample recording including reset replays without divergence or panic. All 45 native tests pass; commit evidence is in the nested `melee` repository.

### Additional fighter sweep and PokÃ©mon Stadium heap repair (2026-09-20)

Mario, Fox, Ness and Peach completed a 20,960-sample very-high-item Fountain of Dreams recording whose full playback matches through reset. The next Samus/Mewtwo/Luigi/Marth run exposed PokÃ©mon Stadium allocating a native `ImageDescWrapper` with its original 28-byte GameCube size; widened image pointers caused the initialization to overwrite the following heap cell. That constructor now allocates its typed native size, matching its sibling constructor. A corrected item-heavy Stadium run remained live for 30,162 samples, and its first 10,000 replay without divergence or panic. This expands roster/runtime coverage but does not prove every character move or item interaction.

A final Ganondorf/Young Link/Dr. Mario/Game & Watch Dream Land recording then replayed all 13,283 very-high-item samples without divergence or panic. Every playable character has now appeared in at least one sustained native roster run. This is broad initialization and ordinary CPU-play coverage; character-specific mechanics still require targeted acceptance.

### Stadium transformation and native HUD animation repair (2026-09-20)

PokÃ©mon Stadium's corrected 10,000-sample replay completes deterministically. Debugger checkpoints on that replay reached the asynchronous transformation callback, native fragment import and incoming-ground construction, while an in-client framebuffer capture at full scale shows the fire form, smoke, fighters and HUD rendered. The first capture exposed P2-P4 `HP` suffixes even though every gameplay stamina flag and animation clock was zero. The HUD reached its material texture animations through nested 32-bit layout casts; widened pointers made the second cast select `HSD_MatAnim::aobjdesc` instead of `HSD_MatAnim::texanim`. The damage HUD now owns typed animation-array fields and traverses the material graph directly for digits and suffixes. The exact frame now renders `48% / 57% / 7% / 71%`, the full replay remains deterministic, and all 45 native tests pass.

### Native memory-card startup removal (2026-09-20)

The supported native startup, input clock and scene paths no longer initialize, probe, poll, drain or display the GameCube memory-card and snapshot-photo services, and boot no longer installs the card-error mode override. The first runtime attempt exposed a second card probe in the controller-sample callback; removing that storage-owned poll and the associated photo poll allowed the exact 10,000-sample PokÃ©mon Stadium replay to complete and exit successfully. All 45 native tests pass. Melee's canonical in-memory settings and unlock data remain intact. Item switch, random-stage switch, language, sound, deflicker, vibration and name-entry transitions no longer submit the disabled card queue after changing live state; `melee/docs/native_persistence.md` records the native ownership boundary and the future installation/profile storage contract.

The first native player-profile representation and store are now implemented.
It owns a player name, tap-jump and C-stick policy, up to four physical sources
per action, optional short-hop bindings, six analog-axis assignments, and
stick/trigger calibration. New profiles retain canonical controller behavior
until an override is assigned. Profile IDs are confined to
`%LOCALAPPDATA%\Freak Melee\profiles`; complete files are written to temporary
storage, flushed, and atomically replaced. Fixed little-endian decoding rejects
bad magic, invalid values, truncation, trailing bytes, and checksum corruption
without changing the live profile. The focused test covers round trips,
replacement, corruption, path traversal, multiple bindings, axes, and
calibration. Aurora now exposes its standardized physical button and axis
state, and assigned profiles resolve multiple sources per semantic action after
SDL sampling but before replay/rollback capture. Default bindings preserve the
canonical pad sample; explicit bindings replace only their action. The focused
input test also covers axis inversion, dead zones, trigger thresholds, and the
short-hop extension bit. Tap-jump disable and dedicated short hop now enter the
common grounded and aerial jump decisions from that transported policy, so
remote simulation does not depend on possessing another player's profile.
C-stick tilt mode now redirects the common grounded C-stick smash edges to the
existing forward-, up-, and down-tilt decisions while preserving aerial
attacks, SDI, throws, and other C-stick consumers. The native Options screen
now selects and edits profiles. The startup path also accepts one stored
profile ID per controller port,
loads it after controller initialization, and applies it to either normal menu
startup or a direct match. Failed profile loads stop startup instead of silently
falling back, and clearing an assignment restores the controller's preceding
calibration.

Native versus rules and item/stage selections now have a small installation-wide
store under `%LOCALAPPDATA%\Freak Melee`. It loads after the original defaults
on normal home boot and saves when CSS exits toward practice or stage select.
It deliberately does not save at shutdown, because a room guest's live rules
may have been overwritten with the host's match configuration. An isolated
real-client boot loaded an 8-minute fixture. Room setup captures local rule
fields before adopting host rules and restores them on the next CSS entry. A
focused test passes for round-trips, items off, host-rule restoration, atomic
replacement, and malformed-file rejection.

The home sidebar now opens a full-window Match rules editor. Time, Stock, and
Coin mode, timers, stocks, damage ratio, item frequency, friendly fire, pause,
and score display are editable; room guests cannot change the host's choices.
Live UI clicks saved Stock and High items, and a restarted client loaded both
from an isolated settings root. The release client and all default targets
build, and all 355 tests pass serially. Per-item selection and random-stage
policy still need controls; cross-network rule propagation remains untested.

The private-room signaling protocol now has a host-owned opaque match-proposal
boundary. The Durable Object persists its epoch and bounded bytes through
hibernation, invalidates every ready state whenever the proposal or active
topology changes, rejects ready without a proposal, and clears the old proposal
when ownership transfers. Same-peer reconnect replacement retains it. Native
wire/client code publishes and receives the same representation, and both
Workers-runtime tests and live local native signaling/ICE probes pass. The
canonical native descriptor now covers the session, exact rules and item set,
four stable fighter/costume selections, CPU/team/handicap state, stable stage
identity, complete peer/port ownership, and a 32-byte selected-content digest.
Its fixed big-endian codec round-trips byte-identically and rejects incomplete
or ambiguous proposals. The direct-start room path now publishes or verifies
those exact bytes and their epoch, checks the active topology/build/per-PC human
count, and blocks ICE topology lock until every listed peer has readied the same
proposal. A live local two-client probe crossed that gate, closed signaling, and
carried validated rollback input. Guest-side application before scene preload,
per-player selection ownership, and lobby UI remain open.


### Initial live rollback restoration checkpoint (2026-09-21)

The Windows match loop now has a preallocated page-delta history over the active game heaps and dedicated mutable game-global section, plus exact per-frame copies for each tracked heap's Aurora allocator descriptor, the current-heap selector, and texture/palette identity counters that cannot safely be page-aligned with neighboring host state. Untracked audio-heap descriptors stay on the live host timeline. A diagnostic replay probe captures before controller sampling and ends after normal draw submission, restores eight frames, rewinds input to the exact saved sample boundary, and resimulates the ordinary gameplay and HSD/GObj draw callbacks. Aurora submits the full hidden GX work without acquiring or presenting a swapchain image. Its fixed sound-intent history follows Slippi's separation of live audio from restored game state: stable starts reuse their existing AX instances, corrected new starts play, and discarded starts are stopped. A Ganondorf/Zelda/Ice Climbers/Game & Watch Battlefield run restored frame 235 from frame 243, reproduced all 134,602,788 tracked bytes, reused one sound start, and exited successfully. The raw full-state hash remains process-local because it contains native pointers. Renderer-resource destruction across rollback, corrected sound timelines, rumble/file-write reconciliation, broader match coverage, and online prediction remain open before M2 acceptance.

The controller sampling boundary now feeds a fixed 16-sample, four-port input
history derived from Slippi's ownership and stale-input prediction model. A bit
mask supports multiple local players on one PC; remote slots repeat their last
confirmed sample, confirmations can arrive before use, and a mismatch records
the earliest correction point while updating later predictions. Trigger values
through 42 compare as the same neutral value, matching Slippi. The live probe
records and restores through this path, and the new native test covers mixed
ownership, prediction, confirmation, correction propagation, expiry, and
invalid submissions. A separate late-input probe now treats P1 as remote,
withholds sample 241, confirms a different input seven frames later, restores
frame 240, and reproduces the corrected full-draw endpoint twice across all
134,602,788 tracked bytes. It reconciled 11 stable and 7 newly introduced sound
starts. A 2v2 Final Destination replay with level-nine CPUs and very-high items
also reproduced its corrected endpoint while reconciling 3 stable and 1 new
sound start. The first run also found the controller queue and fixed rumble pool outside
the game-state section because they were defined by the executable entry-point
object; both now live in `.mstate`. Physical motor calls are suppressed while
hidden frames run, then the final corrected status is applied once.

The tested correction machinery now lives in `native_rollback_session.c`
rather than inside the diagnostic probe. The fixed session owns game-state page
history, input prediction and confirmation, earliest-frame restoration, audio
intent reconciliation, the hidden catch-up endpoint, and rumble-output
suppression. Replay rewinding and endpoint comparison remain probe concerns;
network transport can submit confirmations to the same session without taking
ownership of game state. Its focused native test exercises a normal frame, a
differing remote confirmation, restoration, corrected input reuse, catch-up,
and deliberate repeated resimulation. Both live probes still pass through this
session: the sound-window probe restored frame 235 from 243, and the late-input
probe restored frame 240 from 248 and reproduced its corrected endpoint twice.
Match-loop transport is now active through both fixed nonblocking direct UDP
and room-signaled ICE. An explicit session ID, numeric endpoints, peer IDs, and
non-overlapping controller-port masks support arbitrary shared-PC layouts. A
compact input packet repeats up to eight frames, serializes every pad field
without native padding, validates the source assignment, and atomically applies
non-expired confirmations. Frame zero is a five-second readiness barrier.
Later prediction is bounded to six frames behind the slowest remote-owned port;
at that boundary the client resends while waiting up to five seconds, so loss
cannot silently expire required rollback history. Two
local native processes completed the 2,031-sample Battlefield replay with no
rejected packets. With peer 0 holding outbound input for three frames, peer 1
corrected two timelines across 12 hidden full-draw frames. Both ended with
fighter digest `7f75ed3fbf3113d6` and RNG `748ae5d7`. This is one-machine
functional evidence, not distributed performance or cross-machine determinism
evidence. A maximum-delay run held peer 0 by six frames; both clients completed,
peer 1 corrected four timelines across 18 hidden frames, and both again ended on
the same digest and RNG state. The peers now also exchange a portable checksum
for fully confirmed frames; a three-frame-delay run compared 2,021 checksums on
one client and 1,265 on the other without a mismatch while still correcting two
timelines. That checksum currently covers fighter action/pose state and RNG, so
items, stage controllers, pools, and match state remain open. Setup, timeout,
checksum, and renderer failures now abort the match through Melee's original
No Contest cleanup, skip results/rotation, release rollback and transport, and
leave online play instead of terminating the executable. Until the native room
screen exists, the client safely falls back to the main menu and clears the
stale launch-time netplay proposal; reconnectable lobby recovery remains open.
Detailed commands and boundaries are in `melee/docs/native_netplay.md`.

The initial Cloudflare room-signaling service is implemented under
`melee/services/signaling` without deployment. One Durable Object owns each
private room; hibernatable WebSockets carry participant presence, exact build
identity, active/waiting/spectator roles, ready state, and directed opaque
connection-candidate messages. SQLite preserves monotonic room revision and
join order across hibernation. The service and native WinHTTP client now share a
bounded, versioned binary protocol. A two-lane native connection keeps blocking
receive and queued send work independent through fixed SPSC rings; only the
receive lane closes WinHTTP handles after both lanes join. Workers-runtime tests
cover route rejection, two-client join/ownership, presence updates, directed
signaling, and live socket restoration after forced Durable Object hibernation.
A live local probe connected two independent native clients to `wrangler dev`,
received ordered two-peer presence on both, forwarded opaque candidate bytes,
received the sender acknowledgement, and shut down cleanly. A pinned static
`libjuice` 1.7.2 ICE boundary now owns candidate gathering and match datagrams.
Its callbacks append to fixed SPSC queues while the game thread remains the only
rollback-state owner. Two local agents gathered host candidates, selected a
pair, and exchanged opaque datagrams in both directions without queue drops.
The native connection coordinator now creates one distinct agent per compatible
active room peer and forwards its complete ICE description through signaling.
An end-to-end local probe joined two clients, selected the ICE route, bound room
identities to numeric peers and shared-PC port masks, locked both topologies,
closed both signaling WebSockets, and resolved the same four input samples on
both rollback histories. The actual match loop now uses that same room
transport. It derives an agreed Melee RNG seed from the shared session ID and
semantic match signature before VS scene entry, so stage and fighter setup do
not begin from process-local clock seeds. Two live native clients then ran a
one-stock four-character Final Destination match for more than 40 seconds after
releasing signaling without a checksum mismatch. A three-frame-delay run with
changing remote human input corrected one timeline across four hidden frames
and compared 3,114 confirmed checksums on the predicting client and 2,465 on the
other without divergence. The initial pre-agreement run had aborted at frame
zero on different checksums, proving the detector catches mismatched setup.
All 59 native tests pass and the release client links with WinHTTP and
libjuice. This is same-machine host-candidate evidence, not cross-network
performance proof. External STUN/TURN routes, Cloudflare credential
provisioning, game-menu setup, deployment, and a reconnectable lobby screen
remain open. A room guest now canonically adopts the owner's descriptor before
scene preload, including the stock roster/costumes/stage, exact rules and item
mask, per-slot handicap, shared session, and semantic RNG signature. The live
connection probe begins the guest with a deliberately different session, stage,
and stock count, then proves both peers lock the owner's match and exchange
validated shared-PC input. Diagnostic CLI peer topology and local-human counts
must still match until the room UI owns those assignments.

The rollback probe now also emits address-normalized state diagnostics. Known
pointers into tracked regions, the executable image, and registered immutable
assets become stable identities plus offsets; the persistent `PlCo.dat` source
and import arena use content-derived identities. Two clean Battlefield
processes each reproduced their corrected full-draw endpoint exactly and all
four exact metadata hashes matched across processes. Their normalized whole-
arena manifests still differed on 203 pages. The first isolated mismatch was
inside `HSD_TObj.mtx`, demonstrating that padding, allocator slack, and
presentation-owned cache state make a pointer-normalized byte scan unsuitable
as a cross-peer gameplay checksum. Full hidden draw remains the conservative
rollback contract: Slippi likewise runs specific camera and offscreen work that
normally lives after simulation, and skipping renderer state here would lose
parity before we have proved a smaller boundary. Portable desync detection now
requires a semantic state manifest rather than exclusions based only on where a
byte was written.

### Installed custom fighter local-match checkpoint (2026-09-23)

The compiled Rook definition now enters a local direct match by installed
fighter-bundle ID. Match admission validates the package and native code key,
assigns stable custom slots, then registers assets and callbacks after the
fighter-system reset. Rook's 50-joint costume binds an explicit fighter-part
map and character-specific attributes. Human and CPU Rook have appeared in
Battlefield gameplay and completed a one-stock match. The original CPU attack
table now bounds custom kinds with empty lists; authored custom CPU attacks
are still needed. The original results scene no longer crashes on the missing
custom emblem or result-motion actor. The native shell now reads the installed
fighter title and covers the result archive's fixed stock title and player
name; a full-window capture shows "Rook" in both positions. The fighter
portrait and victory pose are still missing, and Rook's in-match scale and
move fidelity remain open. A native pointer-count error in the trophy-display
cleanup also corrupted the match-end header; correcting it restored the
Stock Battle result label. The native suite passes 359/359 tests. Netplay
custom-fighter selection remains gated until matching packages and definitions
can be reconciled between peers, and the home CSS still needs native portrait
and selection support.

The fighter bundle now carries a CSS portrait alongside its three DATs. Rook's
existing 64Ã—56 portrait was packaged under ID
`4fc6fb3ec1bfda9ebd2d939142d3a7e26389794b883ae6ec2d0beabf31257cc3`;
the prior DAT-only ID is obsolete. The new bundle was accepted into an isolated
installation, launched in a native Battlefield match, and reached the one-stock
results screen. The full 359-test suite passes. Custom portrait placement now works on the original CSS, while real fighter selection there, online content
agreement, and the remaining Rook gameplay work are still open.


An isolated installation now shows the Rook package portrait directly on the live original CSS canvas. The Add fighter drawer placed it in the open third-row cell; a screenshot confirmed the 64Ã—56 art, and a live editor drag moved that same visible portrait. Restore default removed it and returned to stock CSS. The native client links Windows Imaging for menu-time PNG decode, and all 359 native tests pass. Custom portraits remain visual-only until the original cursor and home roster carry installed package identity through practice, room agreement, and match preload.

The original CSS now selects installed Rook, locks the package into practice,
and publishes its identity in the room roster. Room match setup includes
fighter bundles in the same content transfer as costumes, then resolves each
bundle against behavior compiled into the matching client. A two-process run
selected host Rook and guest Falco, joined a room, chose Battlefield, and
rendered the same match on both clients through the timed round, sudden death,
results, and return to CSS under the same room code. The guest received Rook's
1,801,640-byte bundle in its cache without installing it; Rook remained absent
from that guest's CSS. The peers agreed on RNG seed `1bfc65b5`, compared
7,436/7,437 confirmed checksums in the timed round and 1,613 each in sudden
death, and reported no mismatch. A first attempt exposed stale Rook
registration from practice; match adoption now clears that previous scene's
fighter slots before resolving the room roster. Custom fighter round-trip is
therefore working for this same-machine case. A second two-process run began
with an empty guest content root; the guest downloaded Rook's bundle to cache,
verified its SHA-256 against the package ID, and both clients entered the same
Battlefield match. Multiple custom fighters, forced rollback correction, relay/cross-network
play and Rook's full move fidelity still need acceptance.

### Explicit room gifting checkpoint (2026-09-23)

The Collection panel lists installed fighter and costume packages, selects a
connected room recipient, and offers one package at a time. Recipients can
decline or explicitly accept/install from their room sidebar. A package already
in the verified cache installs without download; a missing package transfers
over the room ICE connection, is checked against its SHA-256 package ID, and
then gains an installed marker. An offer alone creates neither cache nor
collection membership. Separate offer serials allow decline followed by a new
offer of the same package. Match-required downloads still remain cache-only.

Local two-process checks exercised Rook's 1,801,640-byte bundle with a fresh
guest root, a cached-only guest root, a decline that left the root absent, and
a decline/re-offer/accept sequence. The received file hash matched its package
ID. The release build and all 359 native tests pass. Costume gifts, multiple
simultaneous offers and relay or cross-network transfer remain untested. A
later boot from the gifted guest store showed Rook in Add fighter, placed the
portrait on the original CSS, selected it with the controller cursor, and
entered practice with Rook and Sandbag. A separate live CSS run installed the
bundle while CSS was already open; opening Edit CSS refreshed Add fighter and
showed Rook without leaving the screen. A separate two-process run transferred
a 473,598-byte Mario costume to a fresh guest root and installed it with a
matching package hash. A subsequent guest boot selected that skin in COSTUMES
and entered practice; it uses original Mario art, so visible skin modification
is not established. The costume picker now refreshes when a gift installs
while CSS is open. M5 is still open for spectators, rotations, multiple
simultaneous offers, relay/cross-network transfer, and content-traffic isolation.

### Waiting participant room boundary (2026-09-23)

The signaling room preserves the current match when a spectator-only owner
leaves and another participant inherits room control. A waiting participant's
selection changes no longer invalidate the fighters' match. Its integration
test covers two active peers and one waiting peer during an owner transfer;
the service tests and type checks pass. The native client now leaves a
non-playing participant in the waiting room when a published match excludes
them, rather than failing match setup. The release build and all 359 native
tests pass. Spectator playback, mid-match catch-up, rotation policy, and live
cross-machine acceptance remain open.

The current signaling role belongs to a peer (one PC), and active peers assign
all their local players to consecutive match ports. This does not yet support
one player on a shared PC fighting while another waits or spectates. Per-player
queue work needs stable local-player identities and an explicit mapping from
those identities to match ports across signaling, CSS ownership, input routing,
and result rotation; changing only the room role would give incorrect behavior.

The match proposal now encodes a local seat index for each human fighter.
Gameplay and results input resolve match ports through that index before
looking up the physical controller. A shared-PC topology test covers two
seats on one peer, the proposal round-trip preserves a nonzero seat index,
and all 359 native tests pass. This is groundwork, not per-player queue
support: the signaling role and local-player count remain per PC. The required
room contract and live acceptance scenario are in
`melee/docs/native_room_players.md`.

The local room service and native client now exchange per-seat active and
waiting masks. Room joins fill up to four active fighter spots and queue any
remaining local seats; a seat in neither mask spectates. Match assembly copies
only active seats and preserves each player's local seat index for gameplay
and results input. The service rejects over-capacity assignments and freezes
active assignments after a match proposal, while waiting/spectating changes
can preserve that match. Service tests, all 359 native tests, and a local
Worker-to-native probe of two three-seat PCs passed: the second PC received
one active seat and two waiting seats. This supersedes the PC-wide role
limitation described above. A live match with a waiting seat on the same PC,
owner-controlled assignment UI, stable per-seat queue ordering, result-driven
1v1 rotation, and spectator playback remain open. Native practice selection
still requires all local seats to lock a character before confirmation.
The matching private signaling service was deployed as version
`4c963c0c-4561-4557-af61-9a0ea087e1c5`. Its health route, remote 3+3
native signaling probe, and two-peer ICE/rollback probe passed. The room
closes stale pre-v3 WebSockets rather than interpreting their old attachments
as seat assignments.

### Prepared release/update checkpoint (2026-09-23)

`melee/tools/prepare_native_release.py` now packages the native executable,
runtime DLLs, and shell UI as a SHA-256 manifest plus ZIP, and produces an
initial friend installation without the ISO. The installed launcher checks an
optional HTTPS channel before starting the game, verifies the archive and
every extracted file, activates a complete version folder, and retains the
previous build. Recovery switches back and skips the rejected release until a
newer one is published. A local two-release smoke test exercised activation,
rollback, rejected-version hold, and rejection of a corrupted ZIP hash. The
relocated executable's `--help` path loaded successfully. A separate real
startup from the copied install reached the character-select screen and wrote
a screenshot, confirming its runtime DLL and shell UI layout. The Cloudflare
release Worker and private R2 bucket are now deployed behind an unlisted
invite path. A local Worker run served the update ZIP and a one-time friend
setup ZIP with matching downloaded hashes; the extracted setup retained its
channel URL, native executable, and no ISO. The deployed empty channel returns
404 without changing an installed build. The publishing tool checks both
prepared archives, uploads them, verifies their remote hashes, and publishes
the manifest last. Private preview `2026-09-24-preview-1` is now published on
the invite channel. Its HTTPS setup ZIP matched the prepared hash; a fresh
extraction launched to character select with a local ISO. An older isolated
install updated through the HTTPS channel, rolled back, and skipped that build
on its next check. Friend-PC and cross-PC acceptance remain open;
`melee/docs/native_release.md` records the handoff contract.

### Custom fighter scene-transition checkpoint (2026-09-23)

Home practice, lobby matches, and initial direct launches now import selected
fighter bundles before leaving their current scene, then bind the prepared
slots after the next scene clears its fighter registry. This removes package
import from the registration assertion path and preserves an active practice
fighter if a replacement fails validation. An isolated fresh-CSS client loaded
an installed Mario clone into practice, a workshop edit replaced a live clone
preview, and a direct command-line launch reached practice with the clone.

A same-machine two-client room test selected that clone on the host and stock
Falco on a guest without the package installed. The guest cached the verified
bundle for the match. Both clients completed a synchronized Battlefield round,
which exposed an additional fighter restaging requirement at the tied match's
sudden-death scene transition. After that transition was repaired, a repeated
one-minute, no-items room match entered sudden death on both clients,
reached matching results with the clone winning, and returned both clients to
the same room's CSS after START on results. The timed round compared
3,833/3,832 confirmed peer checksums, and sudden death compared 1,665/1,664;
both clients reported zero rejected input packets and zero timeline
corrections. The release build and all 374 native tests pass. A correction
crossing authored custom-fighter article state, multiple custom fighters in one match, and
cross-network acceptance remain open. Detailed evidence is in
`melee/docs/native_character_workshop_progress.md`.

A diagnostic room run with three frames of outbound input delay produced two
real rollback corrections over eight hidden frames on the guest while the
Mario clone fought Falco. The peers compared 3,825/3,752 confirmed checksums
without a reported mismatch or rejected packet. The delay was removed and
the normal build restored. The corrected frame range was not logged, so this
does not yet prove resimulation of the clone's authored fireball callback or
its owned article state. The previously described forced local correction
probes cover that callback separately.

The workshop's readable special script now supports a bounded vertical
impulse for aerial entries and authored neutral cues. Generated module tests
verify that it changes airborne self-velocity Y, leaves ground velocity
alone, and rejects the operation in a ground entry with a source-line error.
The complete native build and all 374 tests pass. This is one operation toward
directional authored movement, not completion of the script language.

The diagnostic `--netplay-delay` option now also works on normal home boot and
follows each local client into a sidebar room match. A host with three delayed
frames and a zero-delay guest completed a custom Mario versus Falco match and
entered sudden death; the guest corrected two timelines across six hidden
frames in that scene without rejected input packets or a reported checksum
mismatch. This makes repeatable room rollback tests possible without a
temporary source change; callback-specific article-state evidence is still
required.

### Direct custom-fighter netplay checkpoint (2026-09-24)

Direct raw-UDP matches now accept an installed custom fighter when both
clients provide the same package ID and match settings. A two-client local
Mario-clone versus Falco run reached matching results after a one-minute
Battlefield round and sudden death. That run exposed a monitoring gap: one
client could receive a newer peer checksum before it had reached the frame
needed to compare the previous one. The exchange now retains pending
checksums in a fixed rollback-history window and fails on overflow. In a
repeat run with three frames of host outbound delay, both clients compared
over 3,800 confirmed state checksums in the timed round and over 1,500 in
sudden death, with zero rejected packets and matching result screens. The
native executable builds and all 374 tests pass. Raw-UDP direct play still
requires both clients to install the bundle separately; the sidebar room
path remains the one that transfers verified match content automatically.
Cross-network acceptance and callback-specific authored-article rollback
evidence remain open.

A four-instance test of the same Mario clone found a separate CPU issue:
custom fighter IDs inherited safe empty CPU attack lists, leaving cloned CPUs
idle. Each Mario-based runtime slot now copies Mario's seven imported attack
families and distance threshold into its own storage; the package verifier
binds the same imported common CPU descriptor as the game. In a local match,
three clone CPUs moved, fought, dealt damage and completed the result path.
A subsequent two-client four-clone match reached matching results through a
timed round and sudden death. With three outbound-delay frames on the host
and one B input, the guest corrected two timelines over eight hidden frames;
the clients compared over 3,800 confirmed checksums in the round and over
1,100 in sudden death, with zero rejected packets. The full native build and
all 374 tests pass. Four separate installed package IDs derived from the same
Mario project then filled all four custom fighter slots in a two-client
match. Both clients showed the same winner, the guest corrected two timelines
across eight hidden frames after a host B input, and neither rejected packets.
The host waited 14.9 seconds across many short input stalls, which needs
profiling. This validates four distinct package/module identities with
inherited Mario behavior; editable package-owned CPU policy, four unrelated
fighter implementations, and callback-specific article rollback evidence
remain open.

### Custom-fighter costume dependency checkpoint (2026-09-24)

Stock costume packages retain their encoding. Extra costumes for a custom
fighter now carry the exact parent fighter package ID, and validation uses
that fighter's own costume symbols and joint count. A separate Mario-clone
costume DAT was imported, selected by package ID, and rendered in local
practice. Launching a different clone with that package failed before boot.
An isolated recipient cached the costume before its parent but could not
install it until the parent fighter arrived and validated. Direct match and
room match assembly can load the parent-bound costume once both packages are
present. All 374 native tests pass. The original CSS picker still needs
custom-fighter costume controls in the collaborative UI pass; automated
two-package transfer in a live room remains to be exercised. See
`melee/docs/native_content_packages.md`.

### Private release follow-up (2026-09-24)

The parent-bound costume build was published as private preview 2, then the
friend-facing batch launcher exposed a Windows PowerShell module-discovery
failure during an actual update. The launcher now computes SHA-256 with .NET
directly, and each new release contains the launcher script so activation also
updates it. Private preview 3 includes both fixes. After placing the corrected
launcher in an isolated preview-2 install, a batch-driven update to preview 3,
rollback, and rejected-release hold passed. The
deployed manifest points to preview 3, the HTTPS setup download matches the
prepared SHA-256, and a fresh extraction passed `-CheckOnly` plus the relocated
executable's `--help` launch. Prior preview setup ZIPs should be replaced with
preview 3; the old launcher does not contain the self-update behavior.
Separate-PC installation and gameplay remain unverified.

### Room controls follow-up (2026-09-24)

The right sidebar now exposes the owner's 1v1 rotation policy and shows each
waiting seat's server-assigned rank. A live two-process room had one active
seat on the first PC and three active plus one waiting seat on the second.
Both views showed the waiting seat as rank 1; cycling the owner control from
Manual through Winner stays to Winner out updated the guest's read-only
display. The service's rotation and hibernation tests still cover queue order.
Live spectator playback, mid-match catch-up, promoted-seat controller input
in the following round, and disconnect recovery remain open room work.
This room UI build was published as private preview 4. A clean preview-3
installation updated to preview 4 through its batch launcher without replacing
that launcher manually; the previous version remained available for rollback.

### Readable special-move authoring follow-up (2026-09-25)

The small special-move language now supports setting facing-relative velocity
and airborne vertical velocity in addition to adding to them. Generated-module
fixtures verify replacement after stock entry and in an authored neutral cue
with nonzero initial speeds. Ground vertical edits and two edits to the same
axis in one handler remain rejected with source-line diagnostics. The Workshop
API panel names the new operations. This advances M3 behavior authoring but
does not establish the full state-variable, charge or article-ownership contract.

The Mario special-script follow-up binds named ground and air states for all
four specials and rejects a character DLL whose module layout does not match
the current client before calling into it. The compiler now accepts a complete
neutral-special phase set and generates entry, animation, interrupt, physics,
collision and accessory callbacks. `melee/tests/native/mario_fireball.fms`
expresses Fireball without calling Mario's original special handlers: it uses
standard movement/collision actions and directly creates the fireball item and
effect from the chosen bone when the original throw cue fires. Generated-module
tests cover both animation-end paths, input gating, physics callbacks,
ground/air transitions and article/effect cue dispatch. The later Fireball and
Super Jump Punch replay comparisons below add recorded-path parity evidence;
Cape and Tornado are still untranslated. Do not install this partial source as
the complete fresh-clone Mario template.

A live Final Destination comparison ran the scripted Mario clone through
3,670 recorded samples and verified replay without RNG or fighter-state
divergence. The initial comparison with an untouched inherited clone matched
all 351 action, damage and fireball events, but stock Mario's fireball origin
revealed a clone pose error. The imported animation flags still tagged Mario
as the clip owner; the cloned fighter therefore took the cross-fighter
retargeting path, changing translations despite identical source clips and
skeleton. The runtime now rebases only self-owned animation flags to the
clone's fighter kind and leaves genuinely borrowed clips tagged to their
source. After the fix, the first three Fireball joint poses and projectile
spawns match stock Mario exactly at the traced precision. Under the complete
3,670-sample input stream, all 276 stock and scripted action, damage and
fireball events match after normalizing their different fighter-kind IDs.
This establishes local evidence for the scripted Fireball path, while the
other three Mario specials still need script-owned implementations.

The script compiler now generates authored entry, animation, interrupt,
physics and collision callbacks for any of the four special slots using one
shared code-generation path. A compiled mixed neutral-plus-side fixture checks
that the two authored moves bind separate state pairs and that side-special
ground-to-air transition uses its own state. Cape's reflector/article lifecycle
and Tornado's charge and tap physics still need translation; Super Jump Punch
has a scripted implementation and recorded-path parity evidence below.

Super Jump Punch authoring started with reusable air-entry and freefall
operations. The script now also owns the move's input steering, reverse cue,
physics, collision and completion decisions. Its generated callbacks use
general engine primitives, not Mario's original special handlers. This is a
recorded-path parity result, not yet complete acceptance of every move state.

The physics-phase compiler now accepts ordered, conditional operations for
ground/air motion, attribute-driven gravity, quick air deceleration, and
three-axis velocity scaling. The Super Jump Punch fixture expresses the
original grounded/airborne and animation-command branches with those general
engine primitives; generated-DLL tests exercise both branches and attribute
values. Steering and recorded-path parity checks are described below.

Authored entry handlers can now put velocity edits before their state change,
matching Mario's air Up-special order: clear vertical speed, scale horizontal
speed, then enter the move. The compiled fixture checks the values at the
state-change call. Stock entry remains first when a script explicitly reuses
an original handler.

The Super Jump Punch fixture now owns its collision decisions too. Both
ground and air phases select the original three paths from command state,
vertical speed and ground/air state, while calling the shared airborne,
falling-special landing and grounded collision routines. A generated landing
callback uses the clone's editable lag attribute. Compiled-DLL tests exercise
all three paths, including a landing that changes ground/air state mid-call;
the generated conditions are exclusive. Interrupt script blocks now express
ordered local calculations and nested conditions for launch-angle steering,
reverse-cue consumption, facing updates and root-bone rotation.

An isolated Fireball-plus-partial-Up Mario clone then completed the existing
3,670-sample Final Destination input replay. Its 276 traced action, damage and
Fireball events matched stock Mario after normalizing the clone fighter ID;
the recording includes an Up-special entry at sample 1336. That event trace
did not compare each frame's velocity, angle or position, so it did not
establish full Up-special parity by itself.

A new opt-in fighter-state trace logs exact float words for motion frame,
input, launch angle, facing, velocity, position and animation movement at each
replay sample. On the original 3,670-sample recording, stock Mario and the
partial scripted clone matched all 3,669 sampled P2 state lines exactly. A
separate input-only replay made P2 human, pressed Up+B, and held right during
the jump. It exposed the first state difference at sample 204: stock Mario
set `lstick_angle` to `bea0d97c`, while the script left it at zero. At sample
207 the clone also entered a different motion. The interrupt language now
compiles the corresponding steering and reverse logic from script source. A
new Fireball-plus-Up clone was packaged and tested against that same targeted
recording. Stock Mario and the scripted clone match all 3,669 sampled P2
fighter-state lines exactly, including the formerly divergent steering frame.
This validates the recorded input path, not every possible trajectory, reverse
cue timing, interruption or stage interaction. Cape and Tornado still require
translation before Mario's four specials are fully script-owned.

Cape's item lifecycle exposed a separate dependency: the item's destruction
and animation paths called Mario's special reset/removal checks directly.
Native custom fighters can now supply a paired reset/removal callback through
their exact-client module definition, and the item dispatches to those
callbacks for the article's owner. The module fixture exercises both callbacks
and confirms fighter-local state survives a rollback restore. The readable
compiler does not yet emit Cape callbacks or author the move's physics and
reflector decisions; this is an article-to-module boundary, not Cape parity.

The readable script compiler now accepts `state bool` and `state float`
declarations before the move blocks. Generated animation, interrupt and physics callbacks can read and
write those named fighter-local fields. Their storage starts after any base
module state, reserves one byte for the neutral-special once cue, and accesses
float fields through `memcpy` even when the base size is unaligned. The
generated module reports its required state size and rejects overflow of the
256-byte fighter limit; that storage is part of
each fighter allocation already tracked by rollback. A compiled mixed-special
fixture validates bool/float reads, writes and rejection of duplicate or
unknown names. Animation and physics phases now share the bounded local-expression and
nested-condition compiler with interrupt phases instead of a fixed list of
single-line operations. The mixed-special fixture checks animation-end state
updates followed by freefall. The newly generated Fireball-plus-Up trial package
again matched stock Mario on all 3,669 sampled P2 state lines in the targeted
steering replay. Entry and collision handlers do not yet accept
these named variables;
Cape and Tornado still need their full decisions authored in the language.
As the next Tornado building block, code-block callbacks can now read and
assign animation commands 0-3 and test a fresh B press. The mixed-special
fixture exercises command consumption in an animation callback and rejects
out-of-range command indices. Tornado still needs its entry, tap physics,
collision box and effect/callback lifetime expressed through general engine
operations before it can replace Mario's handler.
General common-fighter movement operations are now exposed to the script:
ground and air stick acceleration, grounded-to-self velocity conversion,
horizontal and fall-speed clamps, basic fall and tap ascend. The compiled
multi-special fixture verifies their dispatch order and attribute arguments.
This supplies Tornado's movement building blocks but does not yet establish
Tornado gameplay parity or replace its stock handler.
An input-only stock Mario replay pressing grounded Down+B reached aerial
Tornado motion 350 at sample 201 while `ground_or_air` remained grounded;
landing later selected motion 349. This matches the decomp's unusual entry
choice. The script grammar now supports `state.enter(AirDownSpecial)` in a
ground handler and the compiled down-entry fixture verifies it enters the
aerial motion without changing the ground/air flag. That fixture is a state
selection probe, not a completed Tornado script.
The entry compiler now uses the same bounded ordered code blocks as animation,
interrupt and physics. Authored entry code can assign commands and persistent
state, branch on conditions, invoke general movement operations and change
velocity before or after an explicit `state.enter(...)` call. The old fixed
one-speed-per-axis emitter was removed. Inherited `stock.enter()` remains an
explicit first operation with optional motion edits after it. Compiled fixtures
exercise a grounded entry that selects aerial Down-special motion, then
performs conditional setup; the rebuilt Fireball-plus-Up trial again matched
all 3,669 stock Mario P2 fighter-state samples. The full Tornado entry and
collision decisions are still not authored.
The remaining Tornado collision implementation is grounded in
`ftmariospeciallw.c`: both motion states use the same six-value collision box
`{12, 0, {-6, 6}, {6, 6}}`; the grounded collision checks either grounded or
airborne contact, while the aerial collision checks landing. Transitions keep
the current animation frame and rearm the move's effect/hitlag callbacks.
The collision result also controls root X rotation against the floor normal.
Collision callbacks now use the same ordered, nested code blocks as the other
state phases. Scripts can branch with `if`/`elif`/`else`, probe standard
ground and air contact, transition between authored ground and air states,
and invoke standard or special-landing collision operations. The fixed
four-step collision grammar was removed. All 22 fighter tests pass, and the
rebuilt Fireball-plus-Up trial still matches all 3,669 stock Mario P2
fighter-state samples after collision generation changed. The general fighter
host and script expression language now expose six-component ground and air
collision-box checks. A compiled fixture runs both against the exact
`{12, 0, {-6, 6}, {6, 6}}` box used by Mario's Tornado, verifies the passed
dimensions and exercises state transitions. Scripts can also rotate the root
around X using the floor normal and facing direction, matching the decision
Tornado uses after collision. The generated module calls the engine's angle
function through its host interface because direct math imports failed in
the independently compiled fighter DLL. Scripts can now spawn an attached root
effect, pause/resume it over hitlag, and register and clear a root-X reset
callback for damage and death. The Down-special fixture exercises these
operations with Tornado's effect ID. All 22 fighter tests pass, including
script build and package round trips. An authored Mario Tornado now lives in
`melee/examples/mario_tornado.fms`, and the default native build compiles it
as a DLL. It uses Mario's original charge field so the existing landing reset
still applies, while keeping its move-local velocity and collision state in
tracked module state. Grounded-start and aerial-start input replays, including
repeated mid-move B taps, each matched all 3,669 stock Mario P2 fighter-state
trace lines. An off-stage aerial run through the move's animation end and a
second aerial Tornado also matched all 3,669 lines apiece. These comparisons
cover recorded motion, velocity and position;
they do not yet prove every effect visual, collision branch or rare
interaction. The other Mario specials are not all script-owned yet, so this
is a focused example rather than a finished fully cloneable Mario template.

Mario Cape now also has a script-owned example at
`melee/examples/mario_cape.fms`, built as a separate DLL in the native build.
The script makes the Side-special decisions itself: cape article creation and
cleanup, reflector activation, air boost and deceleration, ground/air
transitions, and the hip-attached wind effect. It calls general engine
primitives for animation, collision, item spawning and effects; it does not
call the original Mario Side-special handler. Grounded and aerial Side-special
replays each matched all 3,669 stock Mario P2 fighter-state trace lines. A
direct module test covers cape spawn, damage removal, hitlag pause/resume,
reflector activation, wind arguments and the landing transition. All 23
fighter-focused native tests pass. These checks do not yet prove that a live
reflected projectile or every cape article interaction matches stock play.
The Cape item's destruction and animation paths now receive generated module
callbacks for reset and motion-based removal, so a custom Mario clone does
not fall back to the original Mario Side-special checks. The direct test also
covers that article-to-module lifecycle path.

A unified, four-special Mario script now builds from
`melee/examples/mario_full.fms`. A fresh Mario workshop draft opens this as
an uncompiled Scripting template; building behavior is still an explicit
operation. The combined module was installed in a local clone and compared
against stock Mario using four 3,670-input Final Destination recordings:
grounded Cape, aerial Cape, Up-special steering and grounded Tornado. Every
recording reached its end, and all 3,669 sampled P2 fighter-state lines in
each matched the stock run exactly. The script contains no `stock.enter()` or
Mario-special handler calls. This is strong evidence for those four recorded
paths. A Fireball thrown into scripted Cape reflected back onto its stock
Mario owner for 9% damage on sample 238; the identical recording with stock
Mario in the Cape slot matched all 7,338 logged fighter-state lines, including
the damage frame. A Fireball timed to hit one frame after Cape entry instead
damaged the Cape user for 6% on sample 202; stock and scripted Mario matched
all 798 fighter-state lines through that interrupted 400-sample replay. The
same stock-versus-script comparison covered airborne Fireball, Tornado and
Super Jump Punch in separate 400-sample recordings. Fireball spawned its
projectile, Tornado landed, and Up-special reached freefall/landing; each
recording matched all 798 logged player-state lines. The full Mario clone
matrix still requires article lifetime after interruptions later in the move
and in-client package/preview usability before declaring Mario fully
cloneable. Two independently installed builds of the combined script also
completed a 3,670-input same-PC match in separate fighter slots with both
entering Cape on sample 201. A separate custom-Mario CPU recording passed the
eight-frame complete tracked-state rollback probe at frame 240, after the CPU
had used scripted Fireball. These are local checks, not evidence for every
special's rollback edge cases.
An edited second clone changed only Cape's aerial boost expression to
`attrs.specials.vel.y * 1.5`, rebuilt and installed as its own package. In the
same 3,670-input aerial-Cape replay its first fighter-state difference was
exactly the boost at sample 201; it still reached the replay end. A two-clone
match then ran the unchanged and edited builds together. At sample 201 both
entered aerial Cape, with the unchanged clone at vertical velocity
`3f2ccccd` and the edited clone at `3f833333`; this verifies independent
script behavior within one process for that move.
The fresh-clone Workshop Scripting template now opens from the executable's
`ui` directory rather than the process working directory, matching the build's
asset-copy destination and the normal root-folder launcher. The Release client
build passes; in-game Workshop rendering and interaction remain for user review.
The installed four-special Mario bundle was cloned back into a new editable
project with its script source intact, rebuilt, packaged under a new content
ID, installed, and replayed through airborne Up-special. Its 798 tracked
fighter-state lines matched stock Mario, covering the package-to-editor round
trip as well as the original disc-to-project clone path.
The rollback probes now accept cross-package input-comparison replays while
keeping recorded state and RNG digest checks disabled for that mode. A 720-input
fixture entered scripted Cape at sample 226; restoring frames 232-240 matched
all 134,840,356 tracked bytes, and a late P1 correction at frame 240 reproduced
the corrected endpoint on two full-draw passes. This is local active-Cape
rollback evidence, not coverage of every article lifecycle edge.
The correction probe now observes Cape item count, custom ownership, article
binding and the fighter's held handle. A shifted input fixture created Cape
inside the corrected frame-240 window (0 to 1 item); a frame-270 correction
crossed its normal action-end removal (1 to 0). Both corrected endpoints
matched on two full-draw passes with no binding mismatches. Damage/death
cleanup later in Cape remains open.
A Fireball timed to strike one frame after scripted Cape entry also removed the
attached article inside a frame-240 correction window (Cape count 1 to 0,
6% damage, no binding mismatch); two full-draw corrected passes matched. The
same 720-sample fixture without correction matched stock Mario on all 1,438
logged fighter-state lines. Later damage and death cleanup still need coverage.
A normal root-folder game launch opened a saved Mario draft in the Workshop,
displayed the full uncompiled template, and passed `Check script` with C
generation. The syntax-highlight layer initially collapsed source line breaks
and indentation; those now render correctly in an in-client capture. This
check did not change the saved draft or exercise the user's editing workflow.
A separate Mario clone was created and built through the in-client `Build
behavior` button. The resulting draft contains the generated C and DLL and
reopens as compiled source without script errors. Package installation and
practice preview through the Workshop UI remain for the user's hands-on review.

The first hands-on Workshop review found missing Action timeline events,
misaligned hitbox lanes, imprecise seeking, a displaced scripting caret, and
no immediate frozen fighter preview. The native editor now traces bounded
Action command flow, exposes scrolling and frame-level timeline interaction,
loads the chosen draft fighter into practice on entry, and offers a live Test
action / Return to editor switch. Editable scripting sources are now one
ordered file per special move plus `fighter_state.fms`, with a Variables panel
backed by that same state file; Check and Build compose these sources into the
package's generated script. The native build passes. These revised UI and
practice flows are awaiting the user's in-game usability test, so the Workshop
acceptance milestone remains open.

The native Model workspace has Meshes, Materials, Skeleton and Skinning
contexts; Edit, Pose and Animation modes; a live fighter preview; and a
three-step GLB/glTF import flow using native C++ cgltf parsing. Import creates
HSD geometry/materials in the costume DAT, with rigid, generated two-influence,
and replacement weight-transfer binding. The first brush edits imported meshes,
and rest edits compensate skinned inverse binds.

The usability pass replaces palette-number controls with selected-bone surface
heatmaps (blue/yellow/orange/red). Mesh hover highlights the actual surface;
viewport selection, eye controls and H/Alt+H visibility are editor-only.
Skeleton always shows bones. Pose Mode and Animation share G/R/S gizmos with
axis/plane constraints, immutable starting snapshots, commit/cancel and
selection-center/active/individual pivots. Auto Key defaults off; temporary
animation poses can be explicitly keyed, including new channels. Materials
have named render flags and blending/depth/alpha controls. The release build,
animation round-trip checks and an integrated native UI smoke pass succeeded.
The goal stays paused for the user's next usability review.

Model acceptance remains open: source skin import, extra bones, rigid attachment
rest compensation, source-aware reimport, wider weight-painting support and
scalable model storage are still outstanding. Engine-controlled helper/root
bones are not ordinary animation bones and retain gameplay transform overrides.
See `melee/docs/native_model_workspace_design.md` for implementation limits and
the target behavior.

## Vanilla fighter cloneability expansion (2026-09-27)

Make every selectable vanilla fighter an independent Workshop clone base. For
each fighter, import its own DATs, rig, animation and article data; translate
its unique behavior into the fighter script language; and compare the result
against the stock fighter in real gameplay. Extend the language with reusable
operations when needed. A clone base that still delegates unique decisions to
stock handlers is an intermediate step, not an accepted fully cloneable fighter.

Captain Falcon and Ganondorf now have separate imported DAT clone bases and
compileable fighter-wide callback drafts. The script compiler permits lifecycle
actions without an unrelated special move, and exposes part animations for
fighter accessories. Both drafts passed isolated package registration, with
283 and 272 own-rig animation clips respectively. Their shared special-state
implementation and side-special cleanup still use the original C callbacks;
these are intermediate asset/lifecycle ports, not full scripted parity.
Named, bounded float attribute declarations are now available to scripts so
new fighter behavior can read its own imported attribute values without
hard-coding Mario's C structure. A generated callback reads an attribute in
an isolated runtime fixture. The Captain/Ganondorf special translations still
need to use this capability and pass stock-versus-clone gameplay comparison.

The first expansion batch adds Dr. Mario's independent asset path and a
scripted Megavitamin selection/spawn path. It also adds integer fighter state,
bounded random selection excluding prior choices, and a general projectile
spawn operation. A script build, isolated package verification, relevant tests,
and the native client build pass. The Dr. Mario taunt still creates pills using
the stock callback and stock pill-history fields, so its shared history must
be moved under one script-owned path before accepting Dr. Mario. This batch
does not establish stock gameplay parity for him or any other remaining fighter.
The script compiler now also accepts named action blocks that replace selected
motion-state callbacks on the clone's copied state table. An isolated module
test checks callback dispatch, source-table preservation, and state-range
rejection. The next Dr. Mario boundary is general attached-article ownership
so the taunt and neutral special can share one pill-history state.
Fighter scripts now also support installable `on damage:` and `on death2:`
callbacks in both authored-script paths, with callback-dispatch tests. Full
cloneability includes taunts, article/item callbacks, and cleanup; shared
physics, collision, and rendering may remain engine primitives, but no
fighter-specific decisions may delegate to the original stock handler.
The item engine can now select a fighter-module-owned logic table for each
spawned article kind while retaining the fighter's DAT model/attributes.
The Workshop script compiler must still author complete article tables and
Dr. Mario's pill/taunt must use one before that fighter is accepted.
The compiler now emits module-owned article logic tables from declared item
states and interaction callbacks, with an isolated callback-dispatch test.
The item statement language does not yet cover Dr. Mario's pill behavior;
his seven item states and taunt remain stock until translated and compared.
The first generic article operations now cover declared state changes,
lifetime and reflection. Fighter modules may also override donor-kind
pickup/drop, visibility, knockback, absorb, teardown and special-attribute
loading callbacks. An absent
override still inherits donor C, so cloneability acceptance requires an audit
of all copied callback tables, not only the four special entries.
Shared item attachment/visibility and knockback operations are now available
to fighter scripts. Dr. Mario's draft authors his pickup/drop, visibility,
knockback, load and special-attribute callbacks, with a compiled module test.
The taunt, pill article logic and death reset still need translation and parity
checks before accepting his clone.
Dr. Mario's draft now initializes scripted vitamin history to zero, matching
the stock fighter's initial state, and uses a distinct attribute-reload
primitive. The taunt and neutral special still need one shared history before
combined selection sequences can reliably match stock.
Separate script hooks now cover the donor mid-update and color-animation
callbacks at their original dispatch sites. This exposes more of the fighter
decision surface for Kirby, Bowser and others; the actual behavior translations
and stock comparisons remain outstanding.

The Workshop clone picker now exposes each registered stock clone base rather
than hard-coding Mario. Captain Falcon and Ganondorf have provisional scripted
neutral specials that compile and package against their own imported attributes
and rigs. The language gained generic effect attachment, wind, throw-cue,
gravity/friction, drift-clamp, trigonometry and two-axis velocity operations to
express these moves. These began as source translations; the recorded-path
comparison below supplies initial gameplay evidence. Their other specials,
remaining fighter-specific cleanup and any
remaining callback behavior must be scripted before either clone is accepted.
Both provisional drafts now also own their side-special start, target-detection,
hit, collision, and damage/death cleanup decisions in script. Generic host
operations expose target type/item kind, attached effects, animation velocity, landing,
and the existing engine collision/physics routines. Package registration and a
focused callback-dispatch test pass. Grounded and airborne recorded-input
comparisons now show matching traced fighter state for selected Captain Falcon
and Ganondorf neutral/side paths, including grounded side-special contact,
hitlag and damage against Zelda. The repeatable comparator is
`melee/tools/compare_fighter_parity.py`; evidence and branch limits are in
`melee/docs/native_character_workshop_progress.md`. Acceptance still requires
targeted cases for other stick angles, targets, aerial contact, effects and
fighter-specific callback behavior. At that checkpoint, up/down specials
remained inherited C.
The parity trace now also compares full fighter poses, hitbox and hurtbox
digests, and live item state, ownership relationships and pose. Replayed
grounded Captain Falcon and Ganondorf contact paths still match with those
fields. A newly built Dr. Mario clone matched stock on one grounded
Megavitamin/Cape path, including the pill's item state. A taunt-then-Cape
replay exposed a clone-routing mismatch: the common taunt dispatcher checked
the new fighter identity rather than the stock behavior kind. After that
native routing fix, this single combined path matched stock, including the
attached taunt pill and Cape. Dr. Mario's taunt entry is now scripted, while
his taunt animation and pill callbacks still inherit stock C; vitamin
histories remain separate. Every new translation
requires source review and same-input stock comparison across its applicable
hit, miss, ground, air, landing and interruption paths; a match on one path
does not establish 1:1 behavior for an entire fighter.
The comparator now includes the RNG seed and live item material/texture
selection on every traced sample. A first-neutral-Megavitamin replay caught a
visible pill-color difference despite matching fighter state, item physics,
pose and RNG. An untouched clone matched stock, so the script's initial
vitamin history was the cause: stock starts at zero, while the script used
nine. After correcting it, the first-neutral replay matches through 520
samples, including 110 item states. A taunt-then-neutral-Megavitamin replay
still differs at the second pill on sample 254 because inherited stock taunt
and scripted neutral special keep separate vitamin histories. Dr. Mario needs
one script-owned history and a translated taunt/attached-pill path before
acceptance.

The parity comparator now reports the earliest differing sample across fighter,
action, damage, item and RNG traces, requires current item-visual data, and can
require an expected item kind so a missing article cannot silently pass.
Rebuilt Captain Falcon and Ganondorf drafts match stock on recorded grounded
side-special contact and airborne side-special miss paths with this stronger
trace. These are branch-level passes; other moves and article/callback paths
still require their own comparisons.

Captain Falcon's down special now has a provisional script translation. The
stock source was compared branch by branch, and the script uses the same
wall-hug collision flag and command-dependent ground collision routine. Same
input stock/custom recordings match through grounded contact, grounded miss,
leaving the platform, airborne landing, and an Onett stage-item run, including
fighter state, pose, hitboxes, hurtboxes, damage, and RNG where applicable.
Airborne contact now also matches stock, with both runs landing the same 9%
hit on the same sample. Replay inputs and a stock/custom comparison runner
are checked into `melee/tests/native/parity_inputs/` and `melee/tools/`.
An Onett wall-bounce recording now covers the grounded kick's bounce state and
common landing path; other wall contacts remain open. Falcon Dive now has a
script-owned entry, physics, collision, catch, and throw path; its native host
operations cover general capture bookkeeping rather than calling the stock
Falcon special handlers. Same-input stock/clone recordings match exactly on a
grounded whiff, grounded catch and throw (the same 5% and 12% hits), airborne
whiff, grounded Falcon catching airborne Zelda, and airborne Falcon catching
grounded Zelda, including both players' fighter-state, action, damage, and RNG
traces. A separate 650-sample Final Destination recording now matches stock
when airborne Dive reaches the left ledge; interrupted throw and other ledge
approaches remain unverified. The workshop progress document records the coverage and
remaining limits. Do not treat these paths as 1:1 acceptance of the move or
fighter.
Another recorded input path has Zelda dash-attack grounded Dive before capture:
both stock Captain Falcon and his packaged clone enter Dive at sample 231,
take 13% and enter damage state at sample 240, matching across all 380
samples. Damage during the later catch/throw remains unverified.
Ganondorf's Dark Dive uses Captain Falcon's stock handlers and motion layout;
its decisions now live in Ganondorf's own script, with the same generic
capture primitives and Ganondorf-specific attributes. A packaged clone matches
stock on the five recorded Dive input paths, including grounded and airborne
catch variants. Ganondorf's Wizard's Foot now also uses script-owned decisions,
with his distinct 0x50c flame effect. A packaged clone matches stock on the
five earlier kick paths and the Onett grounded wall bounce, including its
basic landing. A separate 650-sample airborne Dark Dive recording now matches
stock through left-ledge catch. Interrupted throw, other ledge approaches,
other wall contacts, and other branches remain unverified; these passes do not
establish fully cloneable Ganondorf.
The same pre-capture interruption recording also matches stock Ganondorf and
his packaged clone across 380 samples, including the damage transition.
Another recorded input path turns P1 left before grounded Falcon Punch or
Warlock Punch. Both packaged clones match their stock fighters across all 380
samples through left-facing motion 347; neither punch hits Zelda, so the hit
branch remains open.

Dr. Mario's attached taunt pill still needs a script-owned item lifecycle and
one shared vitamin history with neutral special. The reusable item-number
ownership contract, needed script/item operations, and parity cases are
specified in `melee/docs/native_attached_article_design.md`; no stock-field
sync shim should be used as an acceptance substitute.
Fighter scripts now have generic live-item-number query/removal operations,
with ownership checked at the time of use. Their generated-module dispatch
test and all 380 native tests pass. Attached spawn and authored pill logic
remain open, so Dr. Mario is still not accepted as fully cloneable.
Authored article script state now has separate storage from the original
attachment owner/facing metadata. A generated-module test writes article
state while preserving those fields; all 380 native tests pass. The host still
needs to create and follow an attached article before Dr. Mario's stock taunt
and pill callbacks can be replaced.
The generic `articles.spawn_attached(...)` expression and host operation now
create an owned authored article, return its creation number, and follow the
fighter origin while that fighter retains ownership. Its generated-module
dispatch test passes. It has not yet been used in a live match, and the Dr.
Mario pill's seven authored item states and taunt script are still open.
The item script now exposes attachment ownership, owner motion/commands, and
item commands/collision/world-interaction fields without routing through Dr.
Mario-specific C. Generated-module tests cover the owner guard and field
writes. General stage-collision, bounce-test, item-sound, owner-scale and
child-visibility operations also have generated-module coverage. The pill
still needs texture selection, any hit effects, its authored lifecycle, and
live stock comparison.
Article scripts can now name imported DAT float parameters by aligned byte
offset, and generated-module tests read them while rejecting duplicate or
unknown declarations. The generic `articles.spawn_authored_projectile(...)`
path now reads declared speed, angle and lifetime parameters and applies
launch setup after the item constructor. Generated-module tests cover its
metadata and dispatch. The Dr. Mario draft still uses the stock constructor;
DAT attribute extent validation, authored pill callbacks and live stock
comparison remain open.
Article scripts now expose the common item interaction callback and the
original attachment owner's root Y scale, with generated-module tests and
missing-host rejection. The native build and all 90 native CTest entries pass
at this checkpoint. A source-to-script callback ledger in
`melee/docs/native_attached_article_design.md` identifies the remaining pill
state work, particularly script-owned taunt tracking/cleanup and the distinct
held-item owner-scale rule. These are not yet proven by live parity tests.
Article scripts can now read a fighter holder's motion and root Y scale
independently of original attachment ownership; the generated-module fixture
covers attached, detached-but-held, and ownerless cases, and all 90 native
CTest entries pass. The Dr. Mario held-state callbacks are still unwritten.
The fighter and article compilers now share the same fighter-state field
layout. Article scripts can read/write a valid original owner's named
rollback-tracked state fields, compare their item creation ID with a stored
handle, and clear the owner's damage/death callbacks. The generated-module
fixture checks all three field types, detached-owner rejection, and cleanup;
unknown field names are rejected. Dr. Mario has not yet been switched to this
article script or compared with stock gameplay.
The item language now exposes a guarded `holder.incapacitated` predicate backed
by the game's common dead/sleep check. This is the condition used by the pill
held-state animation path, and it is tested for attached, detached-but-held,
and ownerless cases. The native build and all 90 native CTest entries pass.
The item expression language also has a general `sqrt` operation for
projectile speed tests; its host requirement and dispatch are tested. The
stock pill uses a specific reciprocal-square-root approximation, so the
eventual authored bounce threshold still needs live parity validation.
Dr. Mario's full script now authors the taunt animation/damage/death path and
all seven vitamin article states, with shared taunt/neutral vitamin history and
the generic authored projectile constructor. An isolated installed clone
matched stock in 520-input taunt-only and taunt-then-neutral recordings; the
latter matched all 1,038 fighter states, 154 item states and 519 RNG states
across eight initial seeds with authored article logic. Seeds 1 and 8 matched
again after switching neutral special to the authored constructor. Explicit
pickup/reflection/other contacts and the exact bounce threshold remain
unverified. This is a focused Dr. Mario parity milestone,
not proof that every fighter is fully cloneable. A separate recorded path
interrupts his taunt with Zelda's jab. The authored clone matches stock over
520 inputs, 1,038 fighter states, 29 actions, 24 item states, four damage
events and 519 RNG states. Its input fixture is checked into
`melee/tests/native/parity_inputs/`. Other interruption timings remain open.
The normal taunt also reaches command 2 and removes the attached pill while
the taunt action continues, matching stock. A separate recorded close-range
Megavitamin hits Zelda for 8% at sample 284; the authored clone matches stock
over 520 inputs, 1,038 fighter states, 26 actions, 79 item states, one damage
event and 519 RNG states. That replay is checked into the same parity-input
directory. Pickup, reflection, clank and other item interactions are still
separate acceptance work.
The close-range pill replay now has a shield variant. Zelda enters shield-hit
motion 181 at sample 284 with no damage, while the authored clone matches
stock over 520 inputs, 1,038 fighter states, 30 actions, 79 item states and
519 RNG states. This verifies one shield contact; pickup, reflection, clank,
and other shield-bounce cases remain open.
Both Dr. Mario taunt states now have scripted interrupt, physics and collision
callbacks in addition to animation. They call common engine operations and
no longer depend on the Dr. Mario taunt wrappers. Rebuilt stock-versus-clone
taunt-only, damage-interruption, pill-hit and pill-shield replays all match;
all 90 native tests pass after rebuilding the host and module fixtures.
Other article interactions remain open.
The Dr. Mario clone now scripts the stock death reset: model group 0, Tornado
charge, Cape boost/reference and taunt-pill tracking. A checked-in two-stock
replay matches stock over 520 inputs, 1,038 fighter states, 26 actions,
36 item states and 519 RNG states, including an offstage death, respawn and
post-respawn neutral special. Active-Cape and charged-Tornado cleanup at death
are not yet individually proven by replay.
Dr. Mario's scripted pill also matches a recorded reflection by Mario's Cape:
the pill transfers from P1 to P2 and reverses horizontal velocity at sample
292. Stock and clone match all 520 controller samples, 1,038 fighter states,
152 item states, two damage events and 519 RNG states. The fixture and a
generic owner-transfer assertion are checked in. A second checked-in replay
fires opposing Dr. Mario pills, recording both clank callbacks at sample 289
and matching all 520 inputs, 89 item states and 519 RNG states without fighter
damage. Pickup, other contacts and the bounce-speed threshold remain open.

Additional Dr. Mario stock-versus-scripted-clone coverage, 2026-09-28: each of
the four specials now has a checked-in grounded and airborne entry recording.
Close-range recordings also match stock through grounded Cape (one 12% hit),
Super Jump Punch (five hits, 14% total), and Tornado (five hits, 14% total),
plus all four airborne special contacts, including an 8% Megavitamin hit.
A separate grounded Tornado recording checks its button-tap lift and a second
use. All comparisons cover 520 controller samples and their traced fighter,
item, damage, action and RNG state. These are branch checks, not complete
fighter acceptance; remaining interactions and untested move branches remain
open. See `melee/tests/native/parity_inputs/README.md` for exact inputs.

Dr. Mario's Cape/Sheet article now runs through script-owned item states and
callbacks, with generic bone-attached spawning, live-article hitlag pause and
resume, dynamic-bone effects, and attachment activation in the native host.
The compiler emits legacy Cape hooks only for scripts that explicitly use the
legacy Cape spawn operation. Seven stock-versus-clone recordings match across
grounded and airborne Cape, both contact paths, interrupted taunt, pill
contact, and death/respawn (520 samples each). The airborne comparison showed
that stock starts the attached Cape in article state 0; the script follows that
observed behavior. This is a focused parity checkpoint, not full Dr. Mario
acceptance; reflection, hitlag and less common article cleanup paths remain
to be compared.

Two additional checked-in Cape recordings now match stock. Zelda interrupts
Cape with a jab at sample 196, removing the attached article on that damage
sample. In the other recording, P1 Dr. Mario's Cape reflects P2 Dr. Mario's
pill at sample 292; the pill transfers from P2 to P1 and reverses direction.
The parity runner now supports a generic item-removal-on-damage assertion so
the first recording cannot pass merely by entering Cape before the hit.

Captain Falcon stock-versus-scripted-clone comparison, 2026-09-28: the
repackaged current `captain_wip.fms` matches 14 existing recordings covering
grounded and airborne Kick, Onett wall bounce, grounded and airborne Dive
miss/catch, an airborne victim, interruption, ledge grab, and left-facing
Punch startup. Two new recordings reach Raptor Boost's fighter-detection hit
transition on the ground (349 to 350) and in the air (351 to 352), with one
damage event to Zelda in each; both match stock over 520 inputs. These 16
recorded paths establish a broader Captain Falcon checkpoint, but do not
prove every callback branch or make all stock fighters cloneable.

Ganondorf stock-versus-scripted-clone comparison, 2026-09-28: the current
`ganon_wip.fms` builds and packages independently, then matches stock across
the same 14 existing Kick, Dive, ledge, interruption and left-facing Punch
recordings. Two checked-in Gerudo Dragon recordings now also match its
grounded fighter-contact transition (349 to 350) and airborne transition
(351 to 352), each with damage to Zelda. Both new cases compare all 520
controller samples, 1,038 fighter states and 519 RNG states; their traces
also include action and damage events. This is a 16-path parity checkpoint,
not acceptance of every Ganondorf callback or every move branch.

Luigi clone-base checkpoint, 2026-09-28: an independent Luigi DAT/animation/
costume package now bundles, installs, and matches stock on six recorded
Fireball, Cyclone, and Green Missile paths. Its on-load path keeps the
Fireball article attached to the clone's own DAT without replacing the stock
global registration. These are inherited stock callbacks, so Luigi is not
yet accepted as script-owned or fully cloneable. The recorded inputs are
regression targets for translating his fighter callbacks and Fireball item
logic; see `melee/docs/native_character_workshop_progress.md`.

Luigi Fireball translation checkpoint, 2026-09-28: `luigi_wip.fms` now owns
neutral-special decisions and the Fireball article. The authored projectile
setup accepts generic attribute or literal launch values and an explicit
automatic-lifetime choice; article scripts gained shared shield bounce and an
optional effect channel. Grounded and airborne Fireball contacts, left-facing
startup, and a shield contact match stock on recorded input paths. The shield
contact removes the item in its contact frame, so parity evidence comes from
fighter/action/damage/RNG traces rather than an end-of-frame item sample.
Green Missile, Super Jump Punch, Cyclone, lifecycle callbacks, and additional
Fireball branches remain to translate and compare. The recorded shield
contact uses the hit-shield removal path; the newly exposed shield-bounce
operation still needs a separate exercised case.

The scripting language now has a linked, Godot-style reference starting at
`melee/docs/scripting/index.md`, with separate language, article, and Item API
pages. As new generic operations are added while translating the remaining
fighters, update their signatures, behavior, limitations, and examples in the
reference in the same work batch. Keep buildable API documentation distinct
from the older behavior-language design sketches.

Luigi follow-up, 2026-09-28: load/attribute reload, item and visibility
events, and knockback enter/exit now use scripted fighter-wide callbacks.
One stock-versus-clone recording reaches P1 damage/knockback three times and
matches over 520 samples; pickup/drop/visibility still need direct evidence.
The `melee/docs/luigi_green_missile_translation.md` ledger maps all paired
Green Missile phases and its one-shot RNG timing. Four new checked-in input
recordings cover held release, maximum-charge launch, a seeded misfire, and
an airborne charge that lands before release. They match the current clone's
Green Missile behavior and are acceptance targets for the ongoing script
translation. Green Missile Start, Hold, normal Launch, and Misfire Launch
ground/air collision callbacks are now scripted through generic
`state.switch_to_air/ground(motion, flags)` operations, documented in the
linked scripting reference. Six stock-versus-clone recordings still match
with those eight callbacks installed. The six ground/air physics callbacks
for Hold, normal Launch, and Misfire Launch are also scripted with the
matching common friction and gravity operations, and the same recordings
still match. Start and End ground/air physics now use named DAT attributes
and generic deceleration/fall operations, also matching the six recordings.
Flight physics, flight/end collision, charge setup, animation, and effects
were the next stock-owned targets at this checkpoint; later progress is
recorded below. The move is not yet fully cloneable.

The Green Missile charge/misfire callback is invoked during the motion
change into Start, not at Start-to-Hold; the translation ledger now reflects
this source-verified timing. Luigi's stock death handler has also been
replaced by an authored model-group reset. A stock-versus-clone death/respawn
recording matches through a post-respawn Fireball. The handler's extra write
to `u.lg.x2234` has no reader in the native source and was omitted rather
than exposed as a fighter-specific scripting method.

The shared horizontal stick-smash age and thrown-item statistic flag are now
available to the fighter scripting language for Green Missile's eventual
entry setup. Their generated operations are covered by the native
action-handler fixture. They do not yet replace Luigi's stock-owned entry;
the full authored entry and charge/launch phase decisions still need parity.

Luigi's Green Missile Flight physics and air-to-End transition, End
animation/collision, and ten stock no-op interrupts now run through script.
The airborne Hold replay reaches Flight 351, Air End 352, Ground End 346,
and Wait with a full stock-versus-clone trace match. A generic
`collision.wall_contact` expression supports the source's either-wall test;
wall-hit behavior itself still needs a dedicated recording. Entry, charge
setup, Start/Hold/Launch animation and Hold interrupts, hitbox adjustment,
and effects remain to translate.

Luigi Green Missile script-owned checkpoint, 2026-09-28: entry, the one-shot
charge/misfire roll, Start/Hold/Launch decisions, active hitbox damage scaling,
effect cues, Flight setup, and the on-damage End transition now use the
fighter script. The language gained generic explicit-frame motion entry,
hitbox update, effect-bone spawn and cue, and facing-velocity callback APIs;
each has a linked reference entry in `melee/docs/scripting/fighter.md`. The
four Green Missile recordings, quick-release path, and Fireball contact
regression match stock across their 520-sample traces after replacement.
Wall, ground-to-air, shield, and uncommon interruption paths still need
targeted recordings. Super Jump Punch and Cyclone remain stock-owned, so
Luigi is not yet accepted as fully cloneable.

Luigi Super Jump Punch checkpoint, 2026-09-28: its grounded and airborne
entry, animation, interrupt, physics, and collision callbacks now run from
`melee/examples/luigi_wip.fms` using Luigi's own DAT attributes. Reused
grounded and airborne contact recordings each match stock over 520 samples,
and the six Green Missile/Fireball regression paths still match. The linked
`melee/docs/scripting/movement.md` reference documents the movement and
landing primitives used by this translation. Steering, reversal, ledge,
interruption, and landing alternatives still need direct replay coverage.
Cyclone remains stock-owned, so Luigi is not yet fully cloneable.

Luigi Cyclone checkpoint, 2026-09-28: all four Luigi specials and visible
fighter-wide callbacks are now script-owned. Grounded and airborne Cyclone
contact plus repeated-tap recordings match stock over 520 samples each;
nine Green Missile, Super Jump Punch, Fireball, and contact regressions also
match. The clone now uses Cyclone's exact `0x0C4C508A` ground/air transition
flags, which fixed a two-frame grounded-animation stall. The scripting
reference adds `melee/docs/scripting/cyclone.md` and movement/charge entries.
The clone intentionally clears Cyclone charge on load/death because stock
source lacks an explicit load initializer. Broader ledge, slope, interrupt,
shield, and charge-across-lives coverage remains before declaring Luigi
fully parity-accepted.

Generic Cyclone-state correction, 2026-09-28: removed the Luigi-only
`charge.cyclone` compiler property. The clone now declares
`state bool cyclone_charge` in rollback-tracked fighter state and initializes
it on load/death; the script explains why stock's missing load initializer is
not reproduced. The same three 520-sample Cyclone recordings still match
observable stock traces after the change, and three focused native tests pass.
The parity comparator never required the clone to mirror Luigi's private
union field. The scripting API rule in section 5 forbids adding such fields
for future fighter translations; older Mario charge shortcuts remain migration
work.

Project-owned fighter parameter checkpoint, 2026-09-28: the custom project,
package, module loader, script compiler, and Workshop now carry typed
`fighter_parameters.fmp` values. Generic `attrs.Name` reads work in fighter,
item, and article code. Value-only changes repackage without recompiling;
schema changes require a build and mismatches reject before gameplay. The
stock importer copies binary32 defaults exactly and removes offset declarations
from the working script without changing the DAT. Full Mario and Dr. Mario
now use ordinary rollback-tracked charge variables and a generic
`on landing_action` callback preserving the stock reset boundary. Seventeen
focused scripting/content tests and the real DVD-transfer test pass, as do
ten rendered 520-sample stock-versus-clone comparisons across Mario,
Dr. Mario, and Luigi with owned parameters. The complete configured Release
build passes. Rook's local compiled package was refreshed for the module layout,
leaving one installed Rook. Older compiler shortcuts/examples, structured DAT
readers, and broader fighter branch coverage remain open. Details and limits
are in `melee/docs/fighter_owned_parameters.md`.

Compiler cleanup checkpoint, 2026-09-28: removed the remaining `charge.tornado`
and `charge.cape_boost` reads/writes and hard-coded Mario numeric attribute
aliases. The full Mario template supersedes the old isolated Cape/Tornado
examples; Cape article lifecycle coverage is retained in a focused fixture.
The multi-special fixture now uses generic declared charge state and named
attribute imports. Executed checks cover repeated charge use, landing-action
and death resets, and independence from the stock union bits. Retired names
are rejected rather than supported through compatibility aliases. The complete
Release build and 17 focused scripting/content checks pass.

Mario article-ownership checkpoint, 2026-09-28: the full Mario template now
scripts Fireball construction and all item callbacks, Cape attachment/effects/
destruction, damage/death/hitlag cleanup via an ordinary script-owned item
handle, and visible versus fighter-wide events. It no longer uses stock
Fireball spawning or Mario's private Cape GObj field. Existing generic article
primitives cover the translation without new fighter-specific APIs. The shared
effect attachment helper now handles models without indexed bone tables;
otherwise Mario's bounce effect was omitted and RNG first diverged at sample
126. Effect attachment modes are correctly documented and validated as attach
or position. All 13 rendered 520-sample Mario stock-versus-clone comparisons
match, with target-path assertions, plus Dr. Mario air-Cape and Luigi ground-
Fireball helper regressions. Eighteen focused scripting/content checks pass.
The native client is now included in the default CMake build; an older excluded
client must not be mistaken for the latest shared game library. New Mario
interaction inputs and `melee/docs/mario_script_translation.md` record the
source audit, reproducible checks, and remaining whole-fighter acceptance.

Generic attached-article cleanup and owned-parameter family checkpoint,
2026-09-28: removed the obsolete Mario-specific Cape compiler operations,
host entries and module callbacks. Generic owned item handles and authored
article callbacks now provide all Cape fixture coverage; retired operations
are rejected instead of retained as aliases. Stock Cape C remains the original
comparison reference. Captain Falcon and Ganondorf source audits confirm that
their scripts cover four specials and fighter-specific versus callbacks,
including family differences; both import 24 named defaults into project-owned
parameters. Fresh packaged modules match stock across seven recordings each:
left-facing neutral special, ground/air side contact, Dive catch/interruption,
Onett wall bounce and air Kick contact. Four Mario/Dr. Mario Cape regressions
also match after cleanup. The Release build and 19 focused checks pass. Rook's
module was rebuilt/replaced for the layout change, with 285 own-rig animations
and one installed package. No Rook move or asset changes. Evidence and
remaining acceptance boundaries are in
`melee/docs/captain_ganon_script_translation.md`; local trace summaries are
under `melee/.local/generic-articles-owned-family-parity-20260928/`. This remains
a recorded-path checkpoint, not full stock-roster acceptance.

Jigglypuff scripted-special checkpoint, 2026-09-28: independent `purin_clone`
asset extraction is available in the CLI and Workshop. Pound, Sing and Rest
own their versus entry, animation, interrupt, physics and collision decisions
in scripts, including both facing variants. Five Pound defaults are imported
into the owned parameter file; the unused stock Pound counter becomes ordinary
rollback-tracked state. Generic `on accessory`, `callbacks.track_accessory`
and explicit numbered entry/animation-start support replace the old side-only
cue declaration without a Jigglypuff-specific API. Fresh drafts load matching
templates for all six stock bases; existing drafts are preserved. Fourteen
520-sample input recordings match stock fighter/action/RNG traces, including
corrected leftward Pound and up/down aerial steering. Four Mario/Dr. Mario
Cape regressions also match. Full Release and 19 focused checks pass. Rollout,
costume accessory ownership and fighter-wide callbacks remain inherited;
contact/sleep and broader whole-fighter acceptance remain open. Details:
`melee/docs/jigglypuff_script_translation.md`. Local verified trace summaries:
`melee/.local/purin-authored-specials-verified-20260928/`,
`melee/.local/purin-final-steering-parity-20260928/` and
`melee/.local/accessory-generic-cape-regressions-20260928/`.

Reusable script-function checkpoint, 2026-09-28: typed `func` declarations
compile to ordinary C functions, with fighter-local state, owned parameters,
forward/nested calls and nonrecursive call graphs. `while` loops support shared
move routines without adding fighter-specific APIs. Helper callback dependencies
and generated engine requirements are validated before gameplay. The compiler
is split into callback parsing, expression/statement compilation, and function
parsing/emission; its source files stay below 2,000 lines. Sing now uses shared
script helpers and matches stock over four 520-sample recordings. Executed
integer/bool/float helper tests exercise nested calls, loops, random bounds,
callback registration and independence between two fighters. Documentation:
`melee/docs/scripting/functions.md`. Rollout's original state paths and shared
routines have been audited, but its script translation is not complete.
The complete configured Release build and 19 focused scripting/content checks
pass. Fresh helper-backed Sing packages match stock after dependency-check
cleanup; evidence is in `melee/.local/purin-helper-functions-parity-20260928/`.


Rollout scripting checkpoint, 2026-09-28: Jigglypuff's Rollout now owns all
Start/Loop/Full/Release/Turn/End/Hit callbacks and their ground/air and facing
variants in the script, including cleanup, damage-dealt rebound, reflection
and attack-stat callbacks. State uses ordinary named fighter-local fields;
38 Rollout values and five Pound values import into owned project parameters.
No stock Rollout handler delegation or fighter-specific scripting alias was
added. Generic combat/presentation operations cover scale, live hit groups,
color, sound, explicit motion rates, custom collision boxes, rumble and quake.
Reference: `melee/docs/scripting/combat_visuals.md`; source/coverage ledger:
`melee/docs/jigglypuff_script_translation.md`.

Eight 1,200-sample Rollout recordings match stock fighter pose, hitboxes,
hurtboxes, action and RNG traces. Short/full ground charges, air charges and
turns are covered in both facing directions. Rightward ground cases require
P2 damage and the post-hit rebound state; air/turn cases require their target
states so missed branches do not qualify as parity. Four freshly compiled
Mario/Dr. Mario Cape regressions also match, including article cleanup and
pill reflection. Full configured Release and all 19 focused scripting/content
checks pass. Evidence: `melee/.local/purin-rollout-parity-20260928/` and
`melee/.local/rollout-host-cape-regressions-20260928/`. Rook's real installed
package was rebuilt/validated for the new host layout without changing her
source or assets; replacement uses the draft install path, retaining one
installed Rook with 285 own-rig animations.

Jigglypuff costume accessories and fighter-wide ownership remain unfinished.
Wall rebounds, fighter reflection and uncommon interruption/ledge/slope
alternatives are translated from source but need additional recorded coverage.
Continue roster cloning and generic API/documentation work; the overall goal
remains active and no usage-based pause is requested.


Jigglypuff lifecycle/costume checkpoint, 2026-09-28: scripts now own load,
attribute reload, death, destruction, item pickup/drop/show/hide and knockback
events as well as all four specials. Generic costume attachment slots, binding
bones, visibility rows, cloth descriptor ranges and held-item part-animation
layers replace the inherited Purin costume decisions. The DAT importer retains
dormant cloth descriptors and all accessory visibility rows. Fixed per-fighter
attachment storage and tracked HSD allocations are included in rollback state
and semantic diagnostics; no allocation is added to the draw loop. API reference:
`melee/docs/scripting/model_attachments.md`; source and coverage ledger:
`melee/docs/jigglypuff_script_translation.md`.

All five stock-versus-script costume recordings match over 1,200 inputs each,
including accessory joint/material hashes and required Rollout hit/rebound states.
Blue/green stock native costumes exposed a 32-bit union alias that no longer
had the same pointer offset on Windows; reading the actual Purin hat field fixes
that native mismatch. Full configured Release, nineteen focused checks and six
additional shared asset/model/semantic checks pass. The compiled callback fixture
covers held/no-item, heavy/light, hold-category and accessory lifecycle branches;
these are not claimed as in-game pickup recordings. Final runtime evidence:
`melee/.local/purin-lifecycle-recovered-20260928/`. Rook was rebuilt for the host
layout and replaced through the project install path, without changing her source
or creating another installed collection entry. Whole-fighter acceptance still
needs broader item/material/interruption coverage. Continue roster translation
with generic primitives and linked documentation; the goal remains active.


Fox/Falco Reflector checkpoint, 2026-09-28: independent `fox_clone` and
`falco_clone` asset bases are available in Workshop and `clone-stock`. Their
shared template authors all ten Reflector phases, projectile-contact response,
and basic item/knockback/load events using generic engine primitives. Fourteen
source defaults become owned parameters, including a byte ownership flag
imported as bool. Timers and effect phase remain ordinary fighter variables.
The generic reflector descriptor replaces the former Mario-specific shortcut;
all Cape callers use it directly. No fighter-specific script property was added.

Fourteen stock/clone recordings match (seven paths for each fighter), including
turns, jump cancellation, air/ground transitions, projectile ownership transfer
and platform drop. Required-motion checks rejected an initial Fox recording
that never reached a platform; the corrected side-platform recording matches.
Four Cape regressions and nineteen focused checks pass, along with the complete
configured Release build. The tests check all nine descriptor values and
bit-exact source parameter import. Reference: `melee/docs/scripting/reflector.md`;
ledger: `melee/docs/fox_falco_script_translation.md`; reproducible manual batch:
`melee/tests/native/fox_falco_reflector_parity.py`.

Rook was rebuilt against the current host layout with source unchanged,
verified as 285 own-rig clips, and replaced through `install-project` without
adding another installed entry. Other Fox/Falco specials, articles, throw/radio
behavior and remaining lifecycle decisions are pending; neither fighter is
marked fully script-owned. Continue roster work with generic documented APIs.
The overall goal remains active, with no usage-based pause requested.


Generic article-state/model checkpoint, 2026-09-28: the Fox/Falco neutral-special,
gun and laser audit identified article-owned counters, flags and procedural
model transforms as prerequisites. Article scripts now declare named bool/int/
float variables in fixed 256-byte per-instance storage, zeroed at spawn and
included in the item state already captured by rollback. The former anonymous
float slots are removed from article callsites; existing Mario, Dr. Mario and
Luigi projectile callbacks use named variables directly. Separate holdable
items retain their existing scratch API. Article ground/air queries read the
engine condition, and holdable-only age/contact union aliases are rejected.
No fighter-specific script API or stock union alias was introduced.

General article joint properties expose local translation, Euler rotation and
per-axis scale through root-relative child paths and normal HSD setters. The
real model checks cover nested/sibling selection, every component, matrix
invalidation and missing/quaternion-joint handling. Native module layout checks
now include the item/state layout. The complete configured native build and
21 focused checks pass, plus five stock/script projectile recordings using
fresh owned-parameter packages. Luigi's original validation expectations used
the wrong item kind and expected a sampled item after same-frame removal; the
corrected requirements pass without a gameplay change. Evidence:
`melee/.local/article-state-regressions-20260928/`, with initial/final validation
records, and `melee/.local/article-state-final-tests.log`.

Reference: `melee/docs/scripting/article.md#local-variables` and
`melee/docs/scripting/item.md#article-model-transforms`; roster ledger:
`melee/docs/fox_falco_script_translation.md`. Rook was rebuilt with her source
unchanged, verified as 285 own-rig clips, and replaced through the project
installation path. Gun/laser callbacks, neutral-special decisions and their
shared throw ownership remain pending and need translation together; this
checkpoint does not claim their parity or whole-fighter acceptance. Continue
that roster batch. The broad goal remains active with no usage pause threshold.

### 2026-09-28 checkpoint: scripted laser callbacks and explicit launches

Fox/Falco laser behavior is translated in
`melee/examples/fox_falco_laser.fms`, with generic explicit world-angle/speed/state
launches from bone-local muzzle offsets. Articles receive fixed per-instance
launch metadata through `on launched`, use the shared stage sweep and vector
mirror routines, and own trajectory/model/expiry decisions in script. No stock
laser handler is delegated to, and no fighter-specific API was added.

The compiled example matches actual original C callbacks over 35,520 bit-exact
samples across 480 trajectories. Another 960 synthetic sweeps validate collision
origin, contact-position restoration and one-frame expiry. These are callback
and launch-argument checks, not acceptance of full neutral-special or real-stage
collision parity. Native build and 22 focused checks pass; evidence lives in
`melee/.local/laser-build.log`, `laser-final-tests.log` and
`laser-callback-parity.log`. Rook's source stayed unchanged; her 285 own-rig clips
were verified and the rebuilt package replaced its previous installation.

API references and acceptance details:
`melee/docs/scripting/article.md#launch-initialization`,
`melee/docs/scripting/fighter.md#articleslaunch_projectile`,
`melee/docs/scripting/item.md#trajectory-operations`, and
`melee/docs/fox_falco_script_translation.md#laser-callback-checkpoint-2026-09-28`.
The laser override stays out of the shared fighter template until gun callbacks,
neutral-special decisions, throw firing and ownership cleanup are translated
together. Continue that batch, then the remaining stock roster. Goal remains
active; there is no current usage pause threshold.

### 2026-09-28 checkpoint: gun model scripts and generic article signals

Fox/Falco gun opening, closing, recoil and visibility are translated in
`melee/examples/fox_falco_blaster_model.fms`. Added general owned-article
signals, read-only float tables, direct shared callbacks between states, and
model joint existence/recursive visibility. These use ordinary named variables;
no blaster-specific host API or stock private-state alias was introduced.

Actual original C comparisons match 68,040 samples of model transforms,
matrix flags and counters. Additional checks cover visibility, intercepted
sound IDs/trigger counts, missing joints, table bounds, signal argument/value
handling and direct callback sharing. This is callback evidence; integrated
ownership/firing/real-stage acceptance remains pending. The native build and
23 focused tests pass. Rook's unchanged source was rebuilt, all 285 own-rig
clips verified, and its previous installation replaced without a new duplicate.

Evidence and API references are linked from
`melee/docs/fox_falco_script_translation.md#gun-model-checkpoint--2026-09-28`.
Logs: `melee/.local/blaster-model-build.log`, `blaster-model-tests.log`,
`blaster-model-parity.log`, and `blaster-rook-refresh.log`.
Continue attached spawning, gun ownership/accessory effects, neutral-special
and throw firing/cleanup as one integration before enabling gun/laser overrides
in the playable Fox/Falco templates, then continue the remaining roster.
The broad goal remains active with no current usage pause threshold.


## 2026-09-28: Scripted blaster ownership/accessory checkpoint

Fox/Falco gun scripts now include pickup selection, owner-state following,
cleanup, incapacity scaling, interaction and muzzle effects. New operations
are generic: attachment origin/plane/collision/held settings, dynamic kind and
state expressions, accessory tracking, directional effects, and terminal item
removal. Native build and 23 focused tests pass; 68,040 exact stock model
samples and 70,576 mocked owner/state cases pass. Rook source unchanged,
285 own-rig clips verified, installed package replaced without duplication.
See melee/docs/fox_falco_script_translation.md for evidence and limitations.

The gun/laser overrides are still deliberately separate from playable WIP
clones until neutral phases, firing, throw shots and fighter cleanup switch
together and receive recorded gameplay comparisons. Full launch/effect/stage
parity is not claimed. Continue the roster conversion; the broad goal is active.


### 2026-09-28: Integrated scripted Fox/Falco blaster checkpoint

The shared template now owns neutral-B, gun/laser articles, back/up/down throw
shots and related cleanup. Five blaster values are imported into owned FMP
parameters. Added reusable throw-animation events, article-cue consumption,
pickup notification, bone-segment directions and instance/base scale reads.
No stock Fox/Falco blaster handlers are called by these authored paths.
Native build and 23 focused checks pass; 14 recordings compare stock and
scripted fighter/item/damage/RNG behavior. Initial contact-removal omission
was caught and fixed; same-tick throw shots are verified through victim/damage
traces. Rook source unchanged, 285 own-rig clips verified, prior install
replaced. See melee/docs/fox_falco_script_translation.md for evidence and limits.

Fox/Falco remain partial: side/up specials, illusion/phantasm, radio and
remaining lifecycle behavior still need translation. Additional branches and
visual/audio acceptance remain open. Continue roster work; broad goal active.


### 2026-09-28 â€” Fox/Falco side dash and authored trail checkpoint

The shared template now scripts all six side-B phases and both trail articles.
Fourteen deterministic stock/script comparisons match movement, actions,
fighter/item fields, main and secondary model poses/visuals, contact damage and
RNG. The fourteen existing integrated blaster/throw recordings still match.
The native build and 23 focused checks pass. An early compiler accessory
assignment was diagnosed from RNG mismatch and replaced with explicit script
entry/callback scheduling. The reusable model-instance API uses fixed item
slots and tracked HSD pools, without stock fighter/item private union access.
Twelve additional fighter tuning values import into owned parameters (31 total).
Rook was rebuilt with unchanged source, 285 own-rig clips verified, and the
previous installed package replaced without duplication.

Evidence and reproducible runner are in melee/docs/fox_falco_script_translation.md
and melee/tests/native/fox_falco_side_parity.py. Next: article parameter-default
migration into FMP, up-special/radio and remaining Fox/Falco lifecycle decisions,
then further roster translations. All scripting extensions must remain generic;
mutable state stays in named fighter/article variables, and runtime tuning must
be owned/editable package data rather than hidden stock DAT aliases. This
checkpoint does not establish every branch, rendered effect/audio or costume;
neither fighter is fully script-owned yet. The broad goal remains active.


### 2026-09-28 â€” article parameter ownership

Article tuning now shares the typed, immutable FMP parameter store with fighter
values, using article-qualified names and generic `item.params.Name` reads.
Stock import copies scalar defaults bit-exactly, removes tuning offsets, and
retains only the source fighter's mapped article blocks in shared templates.
Projectile initialization reads owned values; changing tuning needs no behavior
recompile. Workshop conversion covers all move files and preserves existing
owned values. Stock asset-slot bindings are shared by import/runtime lookup.
All 14 side-dash and 14 blaster/throw recordings match stock. Build and all 23
focused checks pass; default import covers Mario/Dr. Mario/Luigi/Fox/Falco and
rejects out-of-block offsets. Loaded-module callbacks and launch defaults run
with null DAT data and respond to typed parameter edits without a rebuild.
Rook's unchanged source was rebuilt, its 285 own-rig clips verified, and the
previous installation replaced. Full evidence/limitations live in the Fox/Falco
translation ledger. No interactive test of multi-file Workshop conversion was
performed. Common item asset attributes remain in owned DATs; the migration
covers declared behavioral tuning. Next: up specials, radio/lifecycle, then
continue the roster. Generic primitives and named user-owned state remain the
requirement. The broad goal remains active.


### 2026-09-28 - Fox/Falco up-special translation

The shared fighter script owns all seven Firefox/Firebird phases and imports
sixteen tuning values into editable package parameters. Added engine primitives
are generic: named-bone rotation, XY vector angles, surface contacts/normals,
soft-platform skipping, vertical animation velocity and ground-pickup exclusion.
Named fighter state owns move timers/angles; no stock move-union aliases were
introduced. All 22 rendered stock/script recordings match (ground/air steering,
default direction, floor rebound, platform skip), including pose/hitbox/hurtbox
and RNG state. Build and all 23 focused checks pass; compiler coverage checks
64 helpers and high-index recursion. Rook rebuilt with unchanged source and
285 own-rig clips, replacing the previous installation. Evidence and remaining
wall/ceiling/slope/interruption/costume limitations are in the Fox/Falco ledger.
Next: radio taunts and remaining lifecycle decisions, then continue the roster.
Neither fighter is fully script-owned; the full goal remains active.

### 2026-09-28 - Fox/Falco radio and lifecycle

All six radio motions, down-release entry, phase/facing state, action-status
query and cancellation callback are authored in scripts. Added APIs are generic
stage events, input history, action counters, blended idle entry and callback
registration; no stock special/taunt delegation is introduced. Normal/extended
item events, death resets, knockback layers and gun cleanup are audited.
Native data fixes cover cached stage SIS imports, Venom's indexed overlay,
initialized Arwing laser spawn positions and bounded high action-counter reads.
Custom sound dependencies load from prepared next-scene definitions and use
explicit SSM indices, independently of behavior inheritance. The initial Falco
bank mistake was diagnosed and corrected; failed preliminary traces remain
recorded separately. All eight final radio paths match stock on Corneria/Venom
in both facing directions. Build and 24 focused checks pass; Rook rebuilt from
unchanged source, retained 285 own-rig clips and replaced its old installation.
Evidence and limitations are in melee/docs/fox_falco_script_translation.md.
Next: shared fighter-kind exceptions through generic authored contracts and
CPU audit, then further roster translation. Neither whole-fighter independence
nor all audio/visual branches are accepted yet. The broad goal remains active.

### 2026-09-28 - Generic idle/statistics decisions

Scripts now own the idle-variant policy and motion classification query through
generic predicates, removing two further Fox/Falco donor-ID decisions. The
motion query receives an explicit integer event ID. Compiled callback checks
match stock classification over 0..400 and exercise held/unheld idle decisions;
they do not claim a new gameplay recording or end-screen statistics check.
Both script formats share a 512-handler compiler capacity with checked
accept/reject boundaries; no interpreter or simulation allocation is added.
Build and all 24 focused checks pass; Rook's unchanged source rebuilt and
replaced its installation for the current host. Remaining CPU decisions and
branch acceptance are documented in the Fox/Falco ledger. Continue the stock
roster with generic documented APIs; the full goal remains active.

### 2026-09-28 - Generic weapon ribbons for upcoming sword fighters

Custom fighters can author a read-only trail description independently of
stock fighter IDs: attachment joint/axis, dimensions, fade, alpha and RGB.
The shared original ribbon renderer consumes it without simulation allocation
or new persistent state. The compiler rejects mutation and RNG in this
rendering-data callback. Packed source bytes import into owned integer
parameters through the generic importer. Stock settings remain in use for
stock fighters and held Beam Swords; unsupported descriptions no longer read
uninitialized attributes. Native build and all 25 focused checks pass,
including compiled descriptors in both script forms, source-byte import,
value edits and invalid inputs. Rook rebuilt unchanged and replaced its
installation. See melee/docs/scripting/trail.md for the contract and proof
boundary. Marth/Roy clone registration, special translations and rendered
sword-trail comparisons remain next; no whole-fighter acceptance is claimed.

### 2026-09-28 - Marth/Roy neutral and up specials

Marth and Roy now have independent Workshop clone asset bases and partial
templates. Their eight neutral and two up-special states, weapon-ribbon
descriptions, and item/knockback events are scripted. Move decisions call shared
engine primitives, never their stock special handlers. Charge state is a normal
fighter variable; special tuning and ribbon bytes are imported into owned named
parameters. New API operations expose animation root-motion deltas, ordinary
gravity and friction-aware air acceleration, without fighter-specific policy.

The native build and all 25 focused checks pass. Selected evidence covers 46
complete stock/script recordings: 24 neutral, 18 up-special and four contact
paths, with both facings, ground/air entry, charge levels, landing during charge,
steering, real damage and Roy's full-charge self-damage. Coverage failures from
preliminary inputs are excluded; the runner now has corrected inputs and gates.
See melee/docs/marth_roy_script_translation.md and the local acceptance report
for exact traces and limits. The copied client templates are current, and Rook
rebuilt from unchanged source with 285 own-rig clips, replacing her installation
for the new host layout without another Collection entry.

These are partial translations. Side-special, counter, death/reset,
shared-engine exceptions, CPU decisions, broader branches/costumes and rendered
ribbon comparison still need work. Neither fighter is accepted as fully
script-owned. Continue that roster work; the broad goal remains active.

### 2026-09-28 - Marth/Roy side-special, counter and hidden tests

Both templates now script all four specials, weapon ribbons, item/knockback
callbacks, death reset and normal landing reset. Side-special air-lift state is
an ordinary fighter variable. Owned parameters include its physics and Counter
volume, contact hitlag and damage tuning. A generic import-only literal default
handles tuning that has no fighter DAT offset. Counter scripts use a shared
engine guard-volume primitive with an authored contact callback; the additions
also expose semantic bone indices, attack-input edges and hitlag callback clearing.
No new fighter-specific scripting operation or stock-special delegation was added.

Native build and 26 focused checks pass. Thirty selected complete stock/script
recordings match: 20 side-special paths (all 18 phase motions, directional
branches, A/B continuation, early-input lockout and air-to-ground transitions)
and 10 counter paths (ground/air misses and laser reactions, plus damaging
melee responses for each fighter). Preliminary recordings that missed their
intended motions or targets are excluded. Exact traces, source comparisons and
limits are in melee/docs/marth_roy_script_translation.md and
melee/.local/sword-side-counter-acceptance.json. The previous 46 neutral/up
recordings remain an earlier checkpoint, not a fresh run of the expanded script.
Rook rebuilt unchanged for the new host layout and replaced her old installation.

At the user's request, automated gameplay tests now run with hidden windows
by default. Aurora keeps rendering active for this configuration; Windows
reported the test window hidden while match ticks and draw work continued.
Visual test windows require an explicit request. No drawing-disabled path was
introduced. The hidden-window changes were committed locally in both repositories.

Marth/Roy still have common engine kind exceptions under audit (jab/shield and
cloth rules), plus CPU and Kirby-copy integration, repeated lifecycle paths,
edge/interruption cases, scale/costume coverage and rendered ribbon acceptance.
Neither whole fighter is accepted as fully script-owned yet. Continue generic
roster translation and maintain its linked documentation; the broad goal is active.

### 2026-09-28 - Script-owned jab/shield/cloth policies

Generic callbacks now let clones own common jab-restart decisions, ordinary
and powershield entry presentation, and the dynamic-bone global-wind policy.
Marth's requested third jab restarts the ordinary first jab; Roy retains his
single-jab behavior. Marth's shield script changes the current visibility
selector immediately and plays the original sound after shared setup. A new
current-selector operation calls the engine primitive directly, separately
from the saved-selector operation. Both scripts opt cloth into global wind.
No fighter-specific scripting API, runtime allocation or stock special handler
was added. The linked scripting reference documents timing and semantics.

Native build and 27 focused checks pass. Eight additional full stock/script
recordings match (jab, ordinary shield, powershield and charging under Corneria
wind, for both fighters). Coverage gates verify Marth's restart and actual
global-wind activation; Roy's unreachable second-jab requirement was corrected.
Traces additionally compare visibility selectors and wind mode. Local evidence
is melee/.local/sword-common-policy-acceptance.json; prior special matrices
remain earlier checkpoints rather than newly rerun evidence. Rook rebuilt
from unchanged source with 285 own-rig clips and replaced her installed package
for the new host layout. All gameplay comparisons remained hidden.

The shared/cross-fighter audit still needs CPU/Kirby integration and Peach's
stock-kind counter recognition. Contact shields, wind restoration, repeated
lifecycle paths, broader costumes/scales and rendered/audio acceptance also
remain. Whole-fighter ownership is still unaccepted. Continue the roster work;
the broad goal remains active.

### 2026-09-28 - Counter interactions and CPU comparison checkpoint

A generic body-rebound query now lets an intercepting fighter script decide
whether an opposing inert body attack can enter its ordinary rebound. Marth
and Roy permit this in their counter-window states, replacing Peach's
stock-kind recognition for these scripted opponents. Both compiler forms
are covered; no fighter-specific script operation or stock-special handler
was added. Contact traces record the transient event where it is processed,
before the engine clears its flags, and compare stock versus clone events.

Build and 27 focused checks pass. Six selected complete recordings match:
level-nine CPU Marth and Roy attacking stationary human Zelda on Final
Destination and Battlefield (2,400 samples each), plus Peach's actual body
attack interception/rebound against each scripted counter (400 samples each).
Coverage gates require real damage or guard interception plus rebound.
Failed preliminary input/coverage cases are excluded. Evidence and limitations
are in melee/.local/sword-cross-cpu-acceptance.json and the linked translation
ledger. The earlier special matrices were not rerun for this expansion.

The CPU audit found no Marth/Roy-specific shared-planner branches. Seven attack
table families and the distance threshold are copied into fixed per-match
clone storage, but their defaults still come from the stock profile at
activation. Package-owned editable CPU profiles remain open, alongside Kirby
copy integration, broader lifecycle/costume/scale coverage, and rendered/audio
acceptance. Neither fighter is accepted as fully script-owned yet.

Rook rebuilt from unchanged source with 285 own-rig clips and replaced her
installation for the updated host layout. All automated gameplay tests stayed
hidden while drawing continued; Windows verified the running window hidden.
The broad goal remains active; continue generic roster translation and docs.

### 2026-09-28 - Package-owned CPU attack data

Stock cloning imports seven CPU candidate tables and a distance threshold
once into the editable cpu_profile.fcp project file. Packages carry that file
and installed cloning restores it. Runtime activation now binds package-owned
tables in fixed storage; the stock-donor lookup and its old copy helper were
removed. Missing/empty authored profiles yield empty tables, with no fallback.
An explicit import-cpu command imports defaults into existing drafts. There is
no CPU-table Workshop panel yet; the linked reference documents the actual
file format, shared planner contract and ownership boundary.

Build and 28 focused checks pass. All 32 original profiles agree with the typed
importer and round-trip through text without binary32 changes. Installed-clone
acceptance preserves the CPU file after author-draft deletion and checks
independent editing. Four fresh complete stock/script CPU recordings match
for Marth/Roy versus stationary human Zelda on Final Destination/Battlefield,
with real damage gates, 2,400 samples each, drawing enabled and hidden windows.
Evidence is melee/.local/sword-owned-cpu-acceptance.json. Earlier special and
contact recordings remain previous checkpoints, not newly rerun coverage.

Rook's package was rebuilt from unchanged behavior source, preserving her
empty candidate lists and 285 own-rig clips. The installed package replaced
its predecessor; the Collection still lists one installed Rook. Kirby copy,
broader lifecycle/CPU coverage, other roster translations and rendered/audio
acceptance remain open. Neither entire swordsman is accepted as fully owned
yet. The broad goal remains active.

### Donkey Kong authored-move checkpoint (2026-09-28)

Kirby cloning and custom copy abilities are deferred by user decision; stock Kirby remains playable and custom copy integration is removed from current clone acceptance gates. Donkey Kong now has an independent asset base and an uncompiled Workshop template for authored Headbutt and Spinning Kong. All entry, animation, physics, collision and presentation decisions in those two moves use existing generic scripting primitives and owned project parameters. Fourteen full stock/script comparisons match, covering both facings, ground/air entry, steering/release, run-off-stage transition, freefall/landing, and actual grounded contact with Zelda. Fourteen focused backend checks and the native build pass. Tests run with hidden windows and drawing enabled. `melee/docs/donkey_script_translation.md` records evidence and limits. Giant Punch, Hand Slap, mixed charge/cleanup, cargo/heavy-item behavior and fighter-wide ownership remain open; Donkey Kong is not yet a completed clone template.

### Donkey Kong four-special checkpoint (2026-09-28)

The template now authors Giant Punch and Hand Slap too. Punch charge, cancel/effect/momentum phases and base-damage snapshots are ordinary fighter variables; all tuning is owned by the project. Generic additions expose connected-floor queries, live world hitbox placement, arbitrary one-shot cue bits, secondary damage callbacks, grounded escape checks and ground-only/air-only specials. Hand Slap remains ground-only. Death, charge color reset, item and knockback callbacks are authored; asset load/reload and cargo/heavy-item behavior remain inherited and open.

Thirty-seven complete stock/script recordings match, with both fighters' traces and RNG compared. They cover the earlier two-special matrix again, charging/release/full reentry, both facings and aerial starts, shield cancellation/escape, full-charge â†’ spin â†’ punch, damage during charging and after cancellation, single/repeated slaps, Yoshi's Story terrain and actual damage from all four specials. Acceptance rechecks move/contact coverage and at least two slap restarts. The preliminary slap-contact input was too far away and is excluded; its corrected recording damages Zelda in both builds. Evidence: `melee/.local/donkey-all-special-acceptance.json`; native build and 32 fighter backend checks pass. Automated windows stayed hidden with drawing enabled.

Rook rebuilt from unchanged source for the updated host layout, preserving 285 own-rig clips and zero borrowed clips. Her installed package was replaced, not duplicated. No new Rook gameplay or pixel/audio acceptance is claimed. DK remains a partial template until cargo/heavy-item actions, animation-derived setup and shared-kind exceptions are independently owned. Broader lifecycle/costume/scale and roster acceptance remain open; Kirby is deferred. The broad goal remains active.

### Donkey Kong cargo throw checkpoint (2026-09-28)

All eight cargo throw motions now have script-owned animation, interrupt, physics and collision callbacks. Shared engine primitives expose throw cue/release bookkeeping, ordinary throw movement and the animation's saved facing. The script chooses finish and paired ground/air transition policies, preserving back-throw reversal. Cargo acquisition and throw entry, held-fighter locomotion, heavy items, load/reload setup and CPU/shared-kind exceptions remain inherited and open. No DK-specific scripting API was added.

Eighteen complete 650-sample stock/script recordings match: four directions on ground/in air, both facings, and two later up throws requiring landing into the paired ground motion. All recordings require entry/facing, real victim damage and return to idle; aerial runs must land in the paired motion or finish into normal fall. Both fighters' full traces and RNG match. Early aerial up throws correctly finish before landing; the initial universal landing expectation was invalid and those complete traces were rechecked with the proper finish policy. Preliminary late inputs that were already grounded are excluded. Grounded throws leaving a ledge are not newly covered. Evidence: melee/.local/donkey-cargo-throw-acceptance.json, including source/trace hashes and all 32 generated cargo phase bindings. Build and 32 fighter backend checks pass. The earlier four-special matrix was not rerun. Automated windows remained hidden with normal drawing; visibility was checked during the batch.

Rook rebuilt from unchanged behavior for the host layout, with 285 own-rig clips and zero borrowed clips. The previous installation was replaced; Collection lists one Rook package. No new Rook gameplay or pixel/audio comparison is claimed. Documentation and the reproducible cargo runner are maintained alongside the translation. DK remains partial, Kirby cloning/copy abilities remain deferred, and the broad goal remains active.

### Donkey Kong cargo acquisition and locomotion checkpoint (2026-09-28)

Cargo acquisition, walking variants, turnaround, jump startup, falling, platform drops, landing and throw selection now belong to the script. A generic common-throw finish predicate lets a fighter own its end transition; shared walking/jump/landing/capture primitives expose common engine operations without DK handlers. Turn/jump/landing timers and animation-derived walk lengths are ordinary fighter variables. All 72 phase bindings for motions 351â€“368 are authored. Holding-damage callbacks are included, but their shared entry decision and runtime coverage remain open. Heavy-item states, load/reload and CPU/shared-kind paths still require translation; DK remains partial.

Twenty-six complete stock/script recordings match: the 18 throw recordings rerun with owned cargo entry, plus eight carrying sequences covering all walking variants, left turnaround, button/tap full and short jumps, ledge fall and an elevated Battlefield platform drop. Each requires intended motions and real victim damage. Both fighter traces and RNG match. Full jumps peak above short jumps. Initial walking and Battlefield sequences missed their intended states in both runs and are excluded; corrected inputs cover those paths. Evidence is `melee/.local/donkey-cargo-locomotion-acceptance.json`, including hashes, identical owned sources, generated bindings and coverage checks. Build and 32 backend checks pass. Tests stay hidden with drawing enabled; window visibility was checked. Earlier special-move recordings were not rerun, and no pixel/audio equivalence is claimed.

The scripting reference and DK ledger document the new generic operations and remaining ownership. Rook rebuilt from unchanged behavior for the SDK layout, retaining 285 own-rig clips and zero borrowed clips and replacing her previous package. No new Rook gameplay acceptance is claimed. Kirby cloning/copy abilities remain deferred, and the broad goal remains active.

### Donkey Kong heavy-item carrying checkpoint (2026-09-28)

Heavy pickup finish and all 40 callbacks for motions 341â€“350 are now scripted. Ordinary variables own animation-derived walk lengths and turn/jump/landing timers. The script preserves heavy turnaround's later throw/jump interrupt window and uses shared engine walking/jump/landing/throw primitives. Generic additions are `on heavy_pickup_finish` and `controls.try_heavy_item_throw()`; neither invokes DK-specific handlers. Holding-damage entry, load/reload, shared heavy-throw movement policy, CPU exceptions and non-default model scale remain open. DK stays a partial template.

Thirteen complete 700-sample stock/script recordings match, including ordinary item state, both fighters' traces and RNG. They cover four directional smash throws, all walk variants, left turn, early blocked/late allowed turn interrupts, button/tap full and short jumps, landing, and off-edge carrying/air throw followed by death/rebirth. Forward throw requires real victim damage, and full jumps peak above short jumps. The ledge test originally expected idle; stock dies instead. Its unchanged matching traces were rechecked against that actual path, without changing gameplay. Evidence: `melee/.local/donkey-heavy-acceptance.json`, with input/trace/source hashes and 40 heavy/72 cargo binding checks. Build, 33 fighter/replay backend checks and the offline-fixture contract check pass. Test windows stayed hidden with drawing enabled; visibility was checked. Prior special/cargo matrices were not rerun; exact pixel/audio parity is not claimed.

A generic fixed-capacity offline `--fixture-item kind,sample,x,y` setup now supplies identical common items to stock and clone input comparisons. It is rejected with netplay or rollback probes and is inactive in ordinary gameplay/verified replays. Rook rebuilt from unchanged behavior source for the SDK layout, with 285 own-rig clips and zero borrowed clips, replacing the previous installed package. No new Rook gameplay acceptance is claimed. Scripting documentation and the DK translation ledger were updated; Kirby cloning/copy remains deferred, and the broad goal remains active.

### Donkey Kong owned setup and heavy-item policies - 2026-09-28

The script owns load/reload, animation-derived carry lengths, scale-dependent named movement tuning, heavy-throw physics/collision and heavy-item keep/drop damage decisions. Generated behavior no longer sets DK's stock heavy-item flag or attribute pointer. New generic primitives permit binding zero-argument script helpers to current motion callbacks, querying common damage suppression/knockback level, entering a common damage reaction and releasing a held item. The reference documents their callback timing, state ownership and examples.

Eighty complete stock/script recordings match after removing inherited setup: 37 specials, 26 cargo and 17 heavy-item paths. The fresh heavy batch covers actual holding damage and same-frame barrel drop, 1.8x and 0.5x carrying, and return to 1.0x at mushroom expiry. Preliminary mushroom fixtures that applied to another fighter or failed to change scale are excluded. Testing uncovered and repaired a native 64-bit mushroom-state union alias defect shared by stock and clone paths; callbacks, scale values and saved physics now occupy one named state, included in semantic replay hashing. This changes native storage, preserving original gameplay rules. Special/cargo comparisons precede this isolated correction; the heavy batch includes it, with growth/shrink/expiry rerun after final state initialization. Build and 34 backend checks pass, with compiler/common-policy/replay rechecked after the final change. Evidence: `melee/.local/donkey-owned-setup-acceptance.json` and `melee/docs/donkey_script_translation.md`. Tests remain hidden with drawing enabled; no new pixel/audio acceptance is claimed.

Rook rebuilt from unchanged behavior for the SDK, preserving 285 own-rig clips and zero borrowed clips and replacing her previous package. Shared DK CPU classification, charge cancellation and cargo tactics remain open; cargo holding-damage motion 360 has authored callbacks but no confirmed runtime entry. DK remains partial. Kirby cloning and custom copy abilities remain deferred by user decision; stock Kirby remains playable. The broad goal remains active.

### Donkey Kong owned CPU policies - 2026-09-28

The DK template now owns all fighter-specific branches found in the shared CPU source: spacing defaults/updates, attack and grabbing classification, charge entry/cancellation and cargo edge tactics. Its charge remains ordinary fighter state and spacing defaults are named owned parameters. Generic callbacks, bounded CPU input commands and a connected-floor island query provide reusable engine operations; no DK handler or live fighter-union tuning is exposed to scripts. The scripting reference now includes a linked CPU policy page describing callback timing, predicate continuation, command encoding, properties and examples.

Six fresh 3,000-controller-sample stock/script recordings match: level-nine CPUs against stationary/shielding Zelda on Final Destination and Battlefield, plus levels one/four against stationary Zelda on Final Destination. Every case requires actual damage. Coverage includes charge/loop/cancel/release, cargo acquisition/walking/forward throw, and both left/right edge input. One shielding match ends before the input stream finishes; the full remaining streams still match. The initial shielding run exposed a negative float-to-unsigned command-byte conversion mismatch at sample 866; the compiler now converts through an integer, with negative-local contract checks in both callback forms. All six cases reran after that fix. An invalid all-CPU direct-match setup was rejected before launch and contributes no runtime evidence. Native build and all 34 focused backend checks pass. Evidence: `melee/.local/donkey-cpu-policy-acceptance.json` and `melee/docs/donkey_script_translation.md`. Windows remained hidden with drawing enabled. The earlier 80 manual-input comparisons were not rerun for CPU-only hooks, and every planner branch, costume and lifecycle path remains outside this selected acceptance.

Rook rebuilt from unchanged source for the SDK, retaining 285 own-rig clips and zero borrowed clips and replacing her previous installed package; Collection lists one Rook. No new Rook gameplay or pixel/audio acceptance is claimed. DK remains partial pending broader acceptance and cargo holding-damage runtime entry evidence. Kirby cloning/custom copy abilities remain deferred, stock Kirby stays playable, and the broad goal remains active.

### Donkey Kong five-costume checkpoint - 2026-09-28

Ten fresh stock/script recordings match across all five retail DK costumes: normal, black, red, blue and green. Each has one charged Giant Punch contact recording and one left-facing aerial cargo back throw. Gates require charge/release or correct throw entry/facing, actual victim damage and the intended throw finish/idle. Both fighter traces and RNG match. The imported assets preserve every retail byte except the two public root names normalized for the custom slot, with unchanged file size/offsets; the default DAT is unchanged. Evidence: `melee/.local/donkey-costume-acceptance.json` and the maintained DK translation ledger. The reproducible runner builds independent content and keeps hidden clients drawing normally. A preliminary harness coverage lookup used the wrong log basename and is excluded; all ten cases reran after correcting it.

This is selected costume/model/move acceptance, not exact pixels/audio or every opposing-fighter, interruption and lifecycle path. No engine/API or Rook package change was needed, so the preceding native build and 34 backend checks remain the code checkpoint without repeating them for a harness/docs addition. Cargo holding-damage motion 360 still has no confirmed stock runtime entry; no new gameplay entry was invented. Continue the remaining vanilla roster with generic scripting extensions. Kirby cloning/custom copy stays deferred; the broad goal remains active.


### Bowser Flame Breath checkpoint (2026-09-29)

Neutral-B and its Flame article are now authored with owned parameters and
ordinary fighter/article variables. Scripts own depletion/recharge, minimum
loop duration, conditional appearance RNG, hit grouping, surface steering,
hitbox/model scaling, expiry, reflection, shield bounce and absorption cleanup.
Generic APIs cover world joint positions/launches, hit groups, armor, article
hitboxes/contact normals/collision boxes, root effects and vector math; the
linked scripting reference documents signatures, behavior and examples.

A confirmed native source mismatch was corrected before comparison: Bowser's
placeholder pointer slots had widened on PC, shifting shared 32-bit counters;
the flame hit-group ID was incorrectly typed as a boolean. Explicit word types
and offset checks restore the intended storage. Thirteen hidden, normally drawn
stock/script comparisons match, including both facings, ground/air entry,
sustained depletion, recharge, slopes, actual contact, Fox reflection and Ness
absorption. See `melee/docs/bowser_script_translation.md` for exact evidence,
rejected coverage attempts and remaining limits. Bowser remains a partial clone
until side capture and the remaining fighter-wide decisions are authored.
Kirby cloning/custom copy abilities remain deferred; stock Kirby remains playable.

Four selected Fortress/Bomb regressions also match, including contact and
KO/respawn. The final build and all 34 focused fighter/script checks pass.
Rook was rebuilt from unchanged script for the SDK, retaining 285 own-rig clips
and zero borrowed clips; installation replaced the prior package. Tests remain
hidden with drawing enabled. This checkpoint is local only; nothing was pushed.


### Bowser side capture and throw checkpoint - 2026-09-29

Koopa Klaw's holder decisions are now scripted: catch, wait, repeat bites,
threshold-crossing throw choice, cue-based reversal/release and ground/air
transitions. The common attached-victim engine receives editable capture
settings and explicit poses; it no longer calls Bowser attribute getters.
The x64 captured-pose union's misplaced pointer was corrected to its scalar
timer with offset assertions. Stock Bowser/Giga Bowser retain their defaults.

All fourteen 700-sample stock/script pairs pass: both-facings ground/air
misses; ground/air throws in both directions, waiting, repeated bites and
mash-outs. Both ports' traced pose/combat state, actions, damage and RNG remain
compared. Four selected breath/Fortress/Bomb regressions and all 34 focused
native checks pass; the final build succeeds. Tests remain hidden with drawing
enabled, verified against actual Windows window visibility. Full evidence and
limitations are recorded in melee/docs/bowser_script_translation.md and
melee/.local/bowser-capture-acceptance.json. No whole-fighter acceptance claim.

Rook was rebuilt, bundled, verified and installed for the SDK from unchanged
script, retaining 285 own-rig clips and zero borrowed clips, with one installed
package. Bowser's remaining fighter-wide setup, cleanup, item/knockback and CPU
ownership remains next; broader capture interruption/offstage/costume and
editor/rollback acceptance remain open. Kirby stays deferred. Local work only.


### Pikachu/Pichu Skull Bash and Thunder Jolt checkpoint - 2026-09-29

Both independent asset bases are available as partial Workshop templates.
Skull Bash and Thunder Jolt are fully authored for both fighters, including
Jolt's moving controller and linked surface model. Named owned parameters and
ordinary fighter/article variables replace live stock tuning and private
unions. No stock special/item handler delegation occurs in these translations.
Quick Attack/Agility, Thunder and fighter-wide ownership remain open; neither
base is yet a completed clone. Kirby cloning/copy remains deferred, with stock
Kirby playable.

Fourteen selected Skull Bash pairs and sixteen 700-sample Jolt pairs match.
Jolt covers ground/air in both facings, slopes, repeated shots, real victim
contact, Fox ownership transfer and Ness absorption. Both fighters' complete
traced states, item core/pose/visual/relations and RNG remain compared. Initial
interaction gates used incorrect event labels; corrected retained comparisons
and fresh interaction recordings establish the intended contacts. Two final
creation-guard regressions also match. Evidence and open branches are recorded
in melee/docs/pikachu_pichu_script_translation.md and
melee/.local/pikachu-pichu-jolt-acceptance.json. Tests stay hidden, fast paced,
with normal drawing. Exact pixels/audio, Jolt's pre-contact controller reflection
and shield bounce are not established by these recordings.

Reusable additions include explicit article construction, four creation-ID
link slots, reciprocal queries/cleanup, linked signals, collision snapshot
copy, indexed joint reads, stored sweep normals and expression-based item
sound arguments. Author identity remains separate from reflected damage owner.
The linked scripting reference documents signatures, callback ordering and
examples. Build and all 36 focused backend checks pass, including stale-ID,
removed-target, author isolation and parser bounds. Rook was rebuilt from
unchanged script for the SDK, with 285 own-rig clips and zero borrowed clips;
installation replaces the prior package. Local commits only, no push. The
broad goal remains active; continue up/down specials and remaining ownership.


### Pikachu/Pichu Quick Attack and Agility checkpoint - 2026-09-29

Both up specials are now authored across ground/air startup, frozen travel,
second-dash decisions, surface/platform collision, bone rotation/scale, trail
effects and recovery. Named owned parameters and ordinary variables replace
stock tuning/private move state. The script retains both second-dash motion
changes and original RNG/callback ordering; it delegates to no stock special
handler. Thunder, its articles and remaining fighter-wide/common/CPU ownership
remain open. Neither base is yet a fully cloneable fighter. Kirby stays deferred.

Twenty-eight full 700-sample stock/script pairs match: both facings, ground and
air directions, neutral/small input defaults, second dash, sloped and platform
stages, and actual Mario Cape turns during recovery. Both ports' traced state,
pose/combat/visibility, items and RNG stay compared. The initial sloped-floor
travel expectation was corrected against the actual stock path. Cape attempts
that missed or pressed B during braking are excluded; final recordings require
an observed reversal. Four Jolt/Skull Bash regressions also match with their
original contact/motion gates. Build and all 36 focused backend checks pass.
Evidence and limitations: melee/docs/pikachu_pichu_script_translation.md and
melee/.local/pikachu-pichu-up-acceptance.json. All game windows remain hidden;
drawing stays enabled and playback is accelerated.

Generic additions provide kinetic clearing, normal air deceleration,
threshold-based facing, named-bone scaling, animation-motion sample clearing,
single-precision square root and zero-argument forced-turn callback binding.
Ground/ledge checking now accepts a direction expression. Signatures, effects,
callback limits and examples are linked through the scripting reference.
Rook was rebuilt from unchanged source for the SDK, with 285 own-rig clips,
zero borrowed clips and one replaced installed package. The deployed Workshop
template matches the source. Local commits only; no push. The goal stays active.

### Pikachu/Pichu Thunder checkpoint - 2026-09-29

Thunder's ground/air Start, Loop, Hit and End phases and all three article
states are authored. Named owned parameters and ordinary state replace live
DAT tuning and private move unions. Segments share a collision group, carry
an initialization payload and form a creation-ID chain. Scripts own delayed
visibility, falling travel, tip contact, platform impact, stop propagation,
quantized shrinking, effects, feedback and damage interruption. No stock
Thunder fighter/item handler is emitted. All four specials are now authored;
fighter-wide lifecycle/common/CPU ownership remains open, so the bases stay
partial. Kirby's clone and copy-ability work is deferred by user decision.

Twenty-eight full 700-sample Thunder pairs match: ground/air and airborne
self-contact in both facings, repeated casts, slopes, platform interception,
offstage casts, actual opponent damage, shield contact, damage interruption
and Ness absorption. Six final-template Quick Attack/Agility, Jolt and Skull
Bash regressions also match with their original coverage gates. Both ports'
traced fighter state/pose/combat/visibility, items and RNG stay compared.
Pichu self-damage cannot satisfy the interruption gate. Missed contacts and
grounded Magnet attempts are excluded; airborne Magnet reaches real absorption.

The native stock path had a 64-bit union alias mismatch: a status integer wrote
into the upper half of Thunder's pointer. Accesses now use Thunder's own
pointer/status member, with NULL pointer clearing and an integer for 0/1/3.
The platform comparison also caught a translation error: ground impact disables
queued contact hitlag while preserving hitboxes. Fresh recordings pass after
that correction. No original gameplay decision was replaced to hide either bug.

Generic additions cover grouped article creation, creation payloads, existing
item links, nested article branches, authored damage callbacks, full model
visibility, ground collision queries and item contact/damage-hitlag controls.
The linked scripting reference documents signatures, ordering and limits.
Build and all 36 focused backend checks pass. Rook's unchanged source was rebuilt
for the SDK, retaining 285 own-rig clips, zero borrowed clips and one replaced
installed package. Evidence and untested branches are in
melee/docs/pikachu_pichu_script_translation.md and
melee/.local/pikachu-pichu-thunder-acceptance.json. Tests remain hidden,
accelerated and drawn. Local commits only, no push. The broad goal stays active.

### Pikachu/Pichu fighter-wide policies checkpoint - 2026-09-30

The combined template now authors lifecycle/costume selectors, item visibility,
auxiliary attachment presentation, knockback model response, first-jab entry
bookkeeping/restarts, forward-smash effect hitlag callbacks and CPU decisions.
Generic additions expose integer jab transition flags, a one-shot state-change
helper, common smash/auxiliary events, saved visibility expressions, recovery
queues and planner destination coordinates. All move decisions stay in scripts.
The up-special collision box applies instance scale to immutable owned values.

Fourteen full stock/script pairs match: repeated first jabs (33 animation
restarts), actual forward-smash victim contact, held-bat pickup/up-special/drop,
and four 2200-sample CPU matches per fighter covering level 9, shielding,
level 4 and Battlefield. Four final-template Thunder/up-special regressions
also match with original contact/motion gates. Equal noncontact smash attempts,
the wrong motion gate and the original jab action-log gate are excluded; fresh
coverage checks require the intended behavior. Both ports' state, pose/combat/
visibility, item state and RNG remain compared. Build and all 36 backend checks
pass. Rook's unchanged scripts were rebuilt for the SDK, with 285 own-rig clips,
zero borrowed clips and one replaced installed package. Documentation is linked
through the scripting reference. Hidden, accelerated tests keep drawing enabled.

The CPU matches do not reach up-B recovery; its far/near queue branches are
translated and source-reviewed but need runtime acceptance. Nondefault costumes,
auxiliary attachments, scaled collision boxes and broader integration remain
open. The bases stay partial. Evidence: melee/.local/pikachu-pichu-policy-acceptance.json
and melee/docs/pikachu_pichu_script_translation.md. Kirby stays deferred.
Local commits only, no push. The full goal stays active.


### Pikachu/Pichu costume variants and CPU recovery checkpoint - 2026-09-30

Sixteen 700-sample stock/script pairs match: all four costumes for each fighter
through Thunder and both up-special dashes. Custom skins now own a generic
costume.variant separate from the engine asset slot. This fixes Pichu headwear
selection; retaining all model-table rows and selecting texture bindings by
variant also fixes blue/green custom skin startup crashes. Tables remain owned
by the independent fighter DAT. An edited-script roundtrip exposed an
unreferenced gap after texture indices; the shared variant count comes from
visibility rows while the texture span is a checked upper bound.

Eight 1000-sample CPU recovery pairs match for both fighters at near/far
positions on each side of Final Destination. A generic offline airborne
fixture uses shared fall/kinetics/ECB transitions with air jumps consumed;
the normal planner still chooses its destination and queue. Accepted recordings
require actual recovery decisions in the intended distance branch, identical
planner destinations, airborne startup/travel/end and both initial dashes.
The first coverage gate incorrectly required grounded startup 353; existing
recordings were compared again with correct airborne 356/357/358 gates.
Both ports' pose/combat/visibility, item state and RNG match throughout.

The final build and all 38 focused backend checks pass, including all stock
asset imports and edited-action package roundtrips. Rook's unchanged scripts
were rebuilt for the SDK, retaining 285 own-rig clips and zero borrowed clips
and replacing the prior installed package. Tests stay hidden and accelerated,
with drawing enabled. Evidence: melee/.local/pikachu-pichu-variant-recovery-acceptance.json
and melee/docs/pikachu_pichu_script_translation.md. Broader interactions,
scaled collision, auxiliary attachments, pixel/audio review and Workshop/rollback
integration remain open; the bases stay partial. Kirby cloning/copy abilities
remain deferred. Local commits only, no push; the full goal stays active.


### Samus copied base and Screw Attack checkpoint - 2026-09-30

Samus is the fifteenth copied stock base: 313 actions, 14 demos, 60 joints
and four own-package article asset slots. A partial template now scripts
Screw Attack entry, turning, physics, completion, collision and effects,
plus held-item policies. One generic addition calculates normal horizontal
air acceleration separately from gravity; movement decisions remain scripted.

Eight 700-sample stock/script pairs match: ground/air entry, facing each way,
turnaround and real victim contact. Contact gates require damage and attacker
hitlag. Every pair reaches freefall, seven reach special landing and grounded
turnaround catches/waits on a ledge. An initial airborne miss is excluded;
the accepted air-contact case uses the shared offline placement fixture.
The build and all 38 backend checks pass. Rook was rebuilt with unchanged
source for the extended API, retaining 285 own-rig clips and zero borrowed
clips while replacing her existing installed package. All runs stay hidden,
accelerated and normally drawn.

Samus remains partial: Charge Shot, missiles, bombs, grapple, article callbacks
and broader policies remain native. Stock load/death retain their handle
initialization and stock-loss cleanup. Authored secondary damage cleanup does
not yet clear unported charge/grapple handles; cross-move interruption is an
explicit remaining translation/acceptance boundary. No blanket Samus, direct
pixel/audio, Workshop or rollback acceptance is claimed. Evidence:
melee/.local/samus-spin-acceptance.json and melee/docs/samus_script_translation.md.
Kirby cloning/copy abilities remain deferred. Local commits only, no push;
the full goal stays active.

### Samus Charge Shot checkpoint - 2026-09-30

The partial Samus template now owns Charge Shot's fighter callbacks and article
behavior, alongside Screw Attack. Charge, timing, air-start state and live
handles are ordinary script variables; tuning values are project-owned
parameters. Generic additions cover bone-local world positions, live article
attachment/release, preserving ordinary held items, throw-speed damage policy,
article-joint effects, landing-capable stage sweeps and position/facing effects.

Nine 1,100-sample stock/script pairs match: partial/full charge in both facing
directions, cancel/resume, airborne fire both ways, incoming charging damage
and stored charge through Screw Attack followed by release. Victim damage
gates check actual contacts; interruption requires same-frame article removal.
Full-left traces a state-8 flying projectile and its recurring accessory path;
stored-spin traces a later state-6 projectile. The runner's initial wrong item
ID gate was corrected and existing recordings re-compared; an earlier
stored-spin case without later firing is excluded. Hidden, accelerated runs
keep drawing enabled. Build and all 38 focused backend checks pass, including
new attachment bounds, held-slot preservation, effect/collision dispatch and
missing-operation rejection. Rook was refreshed from unchanged source for the
SDK, retaining 285 own-rig clips and no borrowed clips, replacing her package.

Samus remains partial: missiles, bombs, grapple, CPU and remaining lifecycle
policies still need translation. Secondary cleanup now owns Charge Shot but
not the unported grapple. Reflection, absorption, stock loss, arbitrary tuning,
additional opponents, direct visual/audio and rollback acceptance remain open.
Evidence: melee/.local/samus-charge-acceptance.json and
melee/docs/samus_script_translation.md. Kirby remains deferred. Local only.

## 2026-09-30: Samus missile authoring checkpoint

Side-B's normal/smash selection, four ground/air motion phases, accessory
creation and kind-95 normal/Super Missile article decisions now live in
readable scripts. Generic additions cover fighter/item target queries, XY
normalization/angles/rotation, stage-response callbacks, shared expiry,
reflection, direct article-local signals and unsigned serial arithmetic.
New API documentation is linked from the scripting index.

Twelve 900-sample stock/script pairs match both ports' traced gameplay,
article pose/visual digests and RNG. Coverage checks require correct flight
variant/direction, explosion and removal; homing targets require vertical
steering, ground-right cases require victim damage and repeat cases require
three launches. Two follow-up pairs also match after unsigned-wrap changes.
The original repeated-shot names said limit; these do not keep three flying
missiles alive and therefore do not establish the active-shot limit branch.
Build and all 38 focused backend checks pass. Rook's source was unchanged;
her SDK refresh retained 285 own-rig clips and replaced her installed package.

Samus remains partial: bombs, grapple, CPU and remaining lifecycle/policies.
The owned missile serial still needs the primary death reset alongside the
remaining lifecycle translation. Reflection, shield bounce, item-target
fallback, active-shot expiry and stock-loss interactions need comparisons.
Evidence: melee/.local/samus-missile-acceptance.json and
melee/docs/samus_script_translation.md. Tests run hidden and accelerated with
drawing enabled. Kirby remains deferred. Commit locally only; no push.

## 2026-09-30: Samus bomb/morph authoring checkpoint

Down-B entry/accessory, motions 355/356 and bomb-jump morph motions 341/342,
and kind-93 article decisions now live in scripts. Reusable additions expose
article-to-owner function calls, geometric overlap, editable hurtbox capsules,
shared stage follow/bounce/slide callbacks, kinetic conversion and ordered
individual input checks. Named owned parameters hold tuning data; linked
scripting references document each addition.

Twelve 900-sample stock/script pairs match gameplay state, pose, attacks,
article core/pose/visual digests and RNG: ground, air, crouch entry, drift,
repeated bombs and contact in both facings. Contact requires opponent damage
and exercises bomb jumps. Release build and 38 focused checks pass. Rook's
unchanged source was rebuilt and reinstalled with 285 own-rig clips.

Samus remains partial: grapple, CPU, primary death reset and remaining
lifecycle decisions. Ground-slide state 2, reflection/shield/clank,
cross-move interruptions, stock loss and tuning coverage remain open.
Evidence: melee/.local/samus-bomb-acceptance.json and
melee/docs/samus_script_translation.md. Tests remain hidden and accelerated
with drawing enabled. Kirby stays deferred. Commit locally only; no push.

### Parallel native-to-script work (2026-09-30)

User authorized additional GPT-6.1 Sol High agents for separate fighter
translation batches. Root owns shared API/registry/build integration and
acceptance; agents own dedicated fighter scripts, definitions, ledgers and
stock-comparison fixtures. Use generic primitives and owned editable tuning,
with hidden, accelerated tests and normal rendering. Kirby remains deferred.
Pause the goal and interrupt all agents when account usage reaches 30 percent
remaining. Do not push; commit coherent checkpoints locally.

Samus CPU decisions and morph-roll behavior are authored. Four 1,800-frame
pairs match (FD levels4/1, BF level9, Screw Attack threat), alongside a 350-frame
charge/release/damage case and 320-frame recovery pairs in both directions.
Stock comparisons found and fixed a native common-roll union layout defect:
packed bool flags overlapped the item-throw timer. Full-match acceptance is
still blocked by unported grapple/throw attachments and lifecycle work;
longer recovery tests now pass rolls and diverge on grapple creation.

Parallel first batches: Link/Young Link Spin Attack; Yoshi Egg Throw/Ground
Pound with Egg/Star articles; Mewtwo Shadow Ball/Disable with articles. Their
remaining specials/lifecycle paths stay explicitly partial. Integration and
runtime acceptance are in progress; see individual translation ledgers.

The first integrated parallel checkpoint accepts ten Yoshi Egg Throw/Ground
Pound cases, fifteen Mewtwo Shadow Ball/Disable cases and ten Teleport cases,
plus sixteen Link/Young Link ground/air/landing/edge Spin Attack cases.
Yoshi's Egg Roll batch passes eight 600-sample runtime comparisons and
5,620 exact direct-C checks. All 39 focused backend checks pass.
Costume/figatree counts now
remain separate from runtime parts inserted from omission descriptors, and
raw script joint operations use the active runtime count. Link's workshop
physical-to-runtime joint-index mapping remains a separately recorded defect.
Rook's installed module was rebuilt for the current SDK without source edits.

The next shared API batch adds generic held-article creation, dynamic article
joint access, animation-descriptor selection, independent item damage policies,
bounds callbacks, and capture/reflector policies needed for Bomb and Confusion.
All 39 focused checks pass; those complete fighter translations still require
their own stock comparisons before acceptance. Article asset-kind metadata now
uses an explicit native attribute offset instead of assuming every dynamic kind
has Mario's cape layout. Yoshi's enclosed-victim shrink duration also uses its
actual duration field rather than a console-only cross-union offset alias.
Egg Lay, custom shield dispatch, Link's remaining weapons, Samus's grapple and
other fighter-specific lifecycle paths remain explicit work items.

The next comparison batch covers fully authored Confusion, Link/Young Link
Bomb pulling and all seven Bomb article states, and reusable enclosed-victim
capture policies for Egg Lay. Ten Confusion cases passed initially, but the
same-revision Shadow Ball regression exposed inconsistent fighter-variable
packing between fighter and article compilation. The compiler now shares the
selected reserved-byte count; mixed bool/int/float owner-variable tests cover
both script modes. Full repaired runtime regression remains pending.
Bomb currently has 254 original-C comparisons and fully owned package builds;
its hidden drawn runtime matrix is pending. Generic capture settings are
snapshotted into fixed victim state before release clears links; item capture
is a common lifecycle shared by stock Yoshi/Kirby and authored fighters.
These pending batches do not yet close either fighter's lifecycle acceptance.

The repaired Mewtwo revision now passes all 35 700-sample comparisons, including
its previous 25 special/article cases and ten Confusion cases. All 47 imported
fields match the raw DAT. This accepts the four-special batch, not the remaining
forward-throw, CPU, OnLoad/root-animation, dynamics or aerial-jump policies.
Shared enclosure/item capture passes 19,324 direct comparisons against corrected
original Yoshi/Kirby C; Arrow vibration passes 288 direct cases after the native
callback-table layout correction. All 42 focused backend checks and Release
build pass. Bomb's first drawn comparison revealed model-only animation advance
was being confused with item frame/command advancement; separate generic
operations now exist and all sixteen corrected 750-sample runtime comparisons
pass for both fighters, facings, pulls, throws, drops and explosions. The direct
Bomb harness compares 256 source-C cases, including the distinction between
model advancement and item command/frame advancement. Rook was
rebuilt/reinstalled for the SDK with its script source unchanged.

Egg Lay's full fighter/article candidate compiles with the current SDK and is
now undergoing coordinated hidden gameplay acceptance. It remains unregistered
until that comparison passes. Mewtwo forward-throw/CPU/lifecycle and Link's
Bow/Arrow are the next parallel batches; neither base is yet fully cloneable.

The shared lifecycle API now exposes direct bone animation-copy policy,
hip-root compensation, per-dynamic-set interaction start indices and authored
aerial-jump entry with explicit animation-driven physics and item-sound policy.
Stock jump call sites use the same refactored common operation directly.
Both compiled script forms pass context/host/rig/jump checks; all 42 focused
checks pass. Mewtwo's complete lifecycle/throw/CPU scratch candidate passes
22,504 mechanically extracted original-C comparisons, including threshold-edge
jump inputs, bookkeeping and sound call order. Gameplay acceptance is pending.

An explicit offline capturable-item fixture enables Yoshi's item-capture branch
on a common-item proxy. Ordinary items keep their normal eligibility; this
fixture does not establish adventure-enemy assets or callbacks. The fixture
check passes. Rook was rebuilt for the current SDK without source edits.


### Parallel clone checkpoint: muted tests and owned article models

All future Melee runtime tests must use `--mute` and remain hidden unless the
user explicitly requests a visible inspection. Output gain is zero before the
SDL device resumes; audio mixing, synth callbacks and queue timing still run.
The shared parity launcher always supplies the mute flag. Local commit
fbde961c5 records this requirement and implementation.

Yoshi Egg Lay now passes 21 hidden, muted, accelerated comparisons with drawing,
12,870 controller samples in total, including escape, mash, enclosure damage,
interruption and older-special regressions. Capturable common-item proxies cover
release and expiry, but do not establish adventure-enemy asset/callback parity.
Mewtwo's expanded authored candidate passes 42 drawn comparisons plus 22,510
original-C checks using the actual stock bone map. That map caught a semantic
TransN versus literal joint-index mismatch; the authored root flag now targets
owned joint index1. The common worn-item/Screw jump-sound branch is still open.
Neither fighter is claimed complete while remaining common/lifecycle paths exist.

The generic article SDK now supports declared immutable model roots imported at
activation, three independently transformed extra instances, full raw quaternion
storage, draw eligibility separate from joint visibility, retained fighter IDs,
shield queries, stage-line/sweep operations and explicit animation/effect APIs.
Forty-five focused native checks passed; the expression form of moving-line speed
then passed its strengthened parser check. Bow/Arrow bindings are import metadata
only: Link's pending authored Bow/Arrow remains gated on cleanup acceptance.
Rook was rebuilt against the current SDK without changing behavior source:
3048bc2c9dbe276837644a8d03f2cb28ddcd9a341d111890e5171ebd5a9152ac,
285 own-rig clips and zero borrowed clips. Do not commit `.local/` artifacts.

### Typed state and common-policy checkpoint

Typed fixed-size fighter/article arrays now expose named fields and validated
immutable layouts. Fighter script storage is 256 bytes; authored item storage
is 512 bytes. Word fields align to four bytes, and invalid dynamic indices read
zero or ignore writes. Article creation accepts explicit initial velocity and
supports child creation with the current damage owner. Release and all 54
focused backend checks pass (77960b92e).

Yoshi's expanded common/lifecycle template passes 2,730 original-C comparisons
and 43 hidden, muted, accelerated, drawn gameplay cases, with 22,920 input
samples. The actual capture byte array is preserved at native DAT import rather
than incorrectly word-swapped. CPU donor policy remains partial; laser guard
contact does not establish projectile ownership reflection.

The partial Ness base and PK Fire template are available in the workshop. Its
two fighter states and two authored articles pass 6,446 source-C comparisons
and ten drawn gameplay cases (7,000 input samples). Other Ness behavior remains
explicitly untranslated; this is not a fully cloneable Ness claim. Both accepted
templates are recorded in the local integration commit.

Rook was rebuilt and reinstalled with behavior source unchanged for the current
SDK: 7b5e82b0b7f59c52227e999d3f989686af1239ae341e25af11144cd57b82b6e0.
Her animation binding remains 285 own-rig clips and zero borrowed clips.
Next generic APIs cover article model animation bundles/retained creator state
and absorber geometry/healing; fighter-specific decisions remain in scripts.
All future runtime tests remain hidden and muted unless the user requests a
visible inspection. Snapshot and runtime evidence stays local and uncommitted.

### Generic creator, model animation and absorption checkpoint

Local commit 0bd48e5cc records retained article creators validated by live
spawn ID and immutable definition identity, independent current-owner and
reflector handles, fixed model-animation bundles, absorber geometry/healing,
and authored CPU travel/recovery policy hooks. Fighter-specific decisions
remain in scripts; these engine operations are reusable by custom fighters.
Release and all 56 focused backend checks pass, followed by four strengthened
model/state compiler checks. Frozen baseline4 includes matching runtime/SDK
and a SHA-256 manifest; its sources remain unchanged during game comparisons.

Current-ABI source regressions also pass: Mewtwo 23,495, Ness PK Fire 6,446,
Link Arrow 17,114 and Bow 8,350 comparisons. Bow's 768 cleanup cases and
Boomerang/Hookshot lifecycle remain open, and Bow/Arrow remains unregistered.
Ness PSI Magnet passes 21,489 original-C checks; its drawn runtime matrix and
PK Fire regression are still running before template promotion.

Yoshi's additional CPU policies pass 9,297 source comparisons and six normal
runtime cases, plus all 43 accepted regressions: 49 cases and 34,520 controller
samples. These exercise ordinary travel, Roll and Egg Throw decisions and
normal offstage recovery (126 recorded calls per side). Giant/tiny behavior
and the internal travel counter remain source/common-caller checks only, not
observed gameplay claims. Source-kind removal still requires an explicit
owned bone/article registration audit; accepted scripts do not yet prove that
all stock-table dependencies can be removed.

Rook was rebuilt/reinstalled with her behavior source unchanged for baseline4:
f60585825212463ab323c8292ddc7a69bc83d302a340feb49aa697027b6176d1,
285 own-rig clips and zero borrowed clips. This supersedes the earlier package
identifier. All future runtime tests must remain muted, including explicitly
requested visible inspections; normal user gameplay retains its audio.


### Owned asset metadata and reusable article construction checkpoint

- Current native Release build and 56 focused checks pass. Evidence:
  `melee/.local/native-script-baseline5-final-build5.log` and
  `melee/.local/native-script-baseline5-final-checks.log` (16.41 seconds).
- Stock and copied DAT decoding now takes an explicit asset layout rather than
  selecting schemas through a fighter kind. All 33 supplied ISO fighter archives
  import with preserved article/action totals. Definitions own article bindings
  and effect-bank IDs; Mewtwo and Yoshi own their exact 44/39 semantic bone pairs.
  Independent omission records preserve all four source fields. Partial fighters
  still retain stock gameplay inheritance; this checkpoint does not remove it.
- Generic article support now distinguishes root, collision-center and bone
  spawn origins, with an optional separate previous bone position; adds animation
  descriptors without implicit cleanup; exposes validated attachment rotation,
  creator hitlag, direct velocity reflection, named death callbacks and common
  input thresholds/checks. No new fighter-specific scripting command is added.
- Runtime ownership regression uses three actually registered copied packages,
  checks per-instance assets, dynamic article-kind attributes and unload cleanup.
  Custom skeleton roles may intentionally alias a joint (Rook ThrowN/TransN2).
- Rook rebuilt against the current ABI and installed as
  `5761a4456d99e171e9ea345e7d66fac1d275aee99e90f384db27800e684a1157`:
  behavior source unchanged; 285 own-rig clips, no borrowed clips.
- Runtime, SDK and compiler frozen at `melee/.local/baseline5`, with 3,146 file
  digests in `freeze.json`. All eventual game tests remain hidden, muted and
  accelerated with drawing enabled. Normal user launches retain sound.
- Earlier accepted translations are recorded in local commits `1a0f7e6ac`
  (Yoshi CPU policies: 49 total drawn runtime cases) and `539405245` (Ness Magnet:
  21 drawn runtime cases including PK Fire regressions). Their ledgers retain
  source-only coverage limits.
- Link Bow/Arrow and Boomerang remain unregistered drafts. Boomerang return/homing
  source mismatches are being diagnosed before runtime acceptance. Ness PK Thunder
  has a source-backed generic API proposal and scratch translation, not accepted
  gameplay. Kirby remains deferred.


### 2026-09-30 â€” fully authored fighter ownership checkpoint (baseline 6)

- Local commit `e904cea53`: generic `fighter: / behavior independent` declaration
  clears inherited behavior and checks every nonempty extended motion callback.
  The raw catalog base retains fallback for uncompiled drafts. All twenty clone
  bases own result/intro/ending/wait demo root names; runtime binds package-owned
  name records after clearing stock callbacks.
- Build and 58 focused checks pass, including 96 compiled callback-mask cases.
  Runtime/SDK/compiler frozen at `melee/.local/baseline6` (3,148 file hashes).
- Mewtwo independent package
  `9994476a51d55c99109ba013afd5fe7044439bd21005368a5769da6183a1a5dd`
  passed all 45 stock gameplay comparisons (31,500 input samples) and 23,495
  direct original-C checks. Only declared clone kind/behavior identities are
  normalized; gameplay, pose, hitbox, item, visual and RNG checks remain intact.
  Ledger: `melee/.local/mewtwo-independence6-acceptance.json`.
- Yoshi's independent package
  `5e2709f4f1f1127471413fcbfbb0b156952f5da2bf5d6d876924b6dacc091a04`
  passes all 49 gameplay comparisons (34,520 input samples), with 95 owned
  tuning parameters and stock gameplay inheritance removed. Source-only coverage
  limits remain documented in `melee/docs/yoshi_script_translation.md`.
  Ledger: `melee/.local/yoshi-independent-runtime6/independence6-acceptance.json`.
  Its extra self-replay exposed playback consuming sample zero during the boot
  button read. Replay setup now follows that read; the existing recording's first
  4,000 samples, including match completion, replay without state/RNG divergence.
  This verifies that slice, not all 26,750 samples in the recording. Evidence:
  `melee/.local/replay-bootstrap-check/result.json` and `verify.log`.
- Rook rebuilt without changing source or animation bindings: installed ID
  `0d8c02bf963a08e3a0fe3850eb539208a3ab910cb6311a2900b7523f6c1e20f9`,
  285 own-rig clips, zero borrowed.
- Link source-only candidates now pass Boomerang 15,452, Arrow 17,114 and Bow
  8,350 comparisons on frozen baseline 5. They remain unregistered; 768 Bow
  transferred-owner/absent-creator cleanup cases and further lifecycle behavior
  are pending. Ness Thunder remains a source/API proposal, Kirby remains skipped.

### Script translation acceptance policy (2026-09-30)

Compare authored behavior against stock source and gameplay in useful batches.
Exact deterministic agreement is preferred when it follows cleanly from the
engine contract, but is not a release gate for faithful player-visible behavior
when stock undefined behavior or RNG differences would require awkward hacks.
Initialize undefined authored state deliberately and document the stock issue,
chosen behavior and comparison limits in script comments and translation ledgers.
Do not hide failures by filtering meaningful gameplay digests or claiming an
unverified path passed. Behavioral fidelity and exact replay remain distinct
claims. Generic scripting extensions remain mandatory; named editable fighter
state and parameters replace fighter-specific host commands.


### 2026-09-30 â€” generic typed handles and article references (baseline 7)

- Full native Release build and all 66 focused checks pass. Evidence:
  `melee/.local/native-script-generic7-acceptance.json`, build4/checks logs.
- Scripts support explicit bool/int/float locals and preserve complete 32-bit
  handles through typed expressions and helper arguments. Integer division,
  operator types and narrowing rules are checked during compilation.
- Read-only authored article references validate live identity, immutable package
  author, kind, logic, field type and array bounds. Their catalog supports sixteen
  articles; per-article state stays bounded to 512 bytes. No runtime allocation,
  field-name lookup or float identity conversion is introduced.
- Generic child creation installs explicit damage owner/collision group, backlink
  and creation value before on-created callbacks. Creation and link-ID results
  retain full integer bits. Constructor tests cover callbacks removing both
  parent and child without a later constructor dereference. Generated callback
  continuations after transitive source removal remain a separate lifetime batch.
- Selected-joint `self_visible` and validated creator command/model-scale reads
  support the unregistered Young Link Milk fixture. It passes 69,180 original-C
  comparisons and real HSD graph checks; appeal/full-fighter cleanup and active
  registration remain pending. Link and Ness drafts are not newly activated.
- Rook rebuilt with unchanged behavior source, 285 own-rig clips and zero borrowed
  clips. Current installed package:
  `c039446b5d040a78de7c0b022239c09e6f142bf702db833cab747fccd73a8be9`.
- Runtime/SDK/compiler frozen in `melee/.local/baseline7` with 3,154 file hashes.
  No additional gameplay comparison is claimed for this compiler/backend batch.
  Eventual tests stay hidden, muted and accelerated with drawing enabled.


### 2026-09-30 â€” recipient-owned events and synchronous lifetime (baseline 8)

- Full native Release build and all 76 focused checks pass. Evidence:
  `melee/.local/native-script-generic8-acceptance.json`, final build/check logs.
- `export func` exposes up to 64 direct zero-argument callbacks with sorted,
  nonzero name hashes. `targets.call` resolves the recipient's active package
  and uses its own variable layout and immutable parameters, including foreign
  packages. Missing recipients/exports return false without stock fallback.
- Generated callbacks preserve evaluation order and check full source identity
  after synchronous engine operations, transitive helpers and article signals.
  Clang call-site lowering preserves result types and short circuit behavior.
  Named dispatch has balanced bounded depth; no gameplay allocation is added.
- Actual engine-body fixtures cover removed/recycled sources, changed capture
  victims, inner/outer hitlag continuations and recursive item destruction.
  Constructors stop before pickup/registration if child or owner is removed.
  The audit ledger records explicit limits for advanced native fighter deletion;
  this is not a blanket audit of all external stock helper callers.
- Two actual compiled packages prove recipient-owned offsets/17 vs 29 values,
  active DLL base and parameter immutability, and rebind after scene release.
  Existing source comparisons retain their assertions and use registered,
  exact-generation fixture sources for the newly required Host contract.
- Rook rebuilt with unchanged behavior source; installed package:
  `41beb84eb4f9688d06afd0130a86afa2e8c6ae38bc7966b169e715a52b2f49c1`. No game window or audio was opened for this batch.
- Runtime/SDK/compiler frozen in `melee/.local/baseline8`: 3162 file hashes.
  Link Bow/Arrow/Boomerang and Ness Thunder remain unregistered pending their
  remaining translation and gameplay checks. No new fighter completion claimed.


### 2026-09-30 â€” complete-fighter independence audit after baseline 8

The live runtime registry contains 20 stock clone bases. Only Yoshi and Mewtwo
currently declare independent behavior. The other 18 registered bases have
scripted move families but retain stock fallback: Mario/Dr. Mario, Falcon/Ganon,
Luigi, Jigglypuff, Fox/Falco, Marth/Roy, DK, Bowser, Pikachu/Pichu, Samus,
Link/Young Link and Ness. Do not count four scripted specials as complete fighter
independence; lifecycle, CPU, common-action policies, articles and owned asset
metadata must be included. Peach, Zelda, Sheik, Popo/Nana and Game & Watch do not
yet have registered stock clone templates. Kirby remains explicitly excluded.

Link Bow cleanup is the next bounded source-comparison batch using baseline 8's
recipient-owned exports; supported recipient contexts restore the previously
excluded terminal cases. Missing optional exports must not retain doomed items
indefinitely or fall back to stock fighter handlers. Keep terminal item behavior
and document the skipped unavailable recipient mutation.

Falcon/Ganon are a tractable subsequent independence closure: their specials and
most lifecycle callbacks are already scripted, and neither has article slots.
Remaining confirmed decisions include CPU grab/ranged candidates, Ganon taunt
visuals, Falcon held-item fourth-swing routing, owned tuning/bone/demo metadata,
and independent source/runtime acceptance. Existing generic CPU operations cover
the planner decisions; any added presentation/action policy must stay reusable.

Ness Thunder still needs a coordinated generic ball/trail batch. Typed handles,
read-only article references, child construction and source lifetime are already
available in baseline 8; refresh the proposal against current code rather than
rebuilding those features. Further tests stay hidden, muted and accelerated with
drawing. Behavioral stock fidelity permits documented UB/RNG differences;
determinism between native clients remains required for rollback and replay.


### 2026-09-30 â€” generic editable policy values

`effects.wind` now accepts ordinary expressions and owned tuning for its
lifetime, offset, strength, decay and frequency. Its engine operation is
unchanged; arguments evaluate once in source order. `capture.victim_damage`
exposes the linked victim's ordinary damage percentage, or zero without one.
Neither feature is fighter-specific. Reference pages explain both operations.

Full native Release build passed. Of 76 focused checks, 75 passed initially;
the new compiler fixture needed the actual separate parameter table and the
existing `Top` semantic name, then its targeted recheck passed. The generated
Cape module exercises local/expression wind values and victim damage at the
threshold and without a victim, retaining its original runtime assertions.
Evidence: `melee/.local/native-script-policy-values-acceptance.json`.
No game launch, Host ABI change or fighter activation occurred. Rook's installed
package and the immutable baseline8 remain valid and unchanged.

### 2026-09-30 â€” source comparison and independence preparation checkpoint

- Local commits: `fe7bf18a2` accepts generic editable wind/victim-damage values;
  `bf765132d` saves reproducible Bow/Arrow source fixtures; `6621186a3` records
  the remaining Captain/Ganon and Ness contracts. No upstream push occurred.
- Bow: 9,132 original-C/generated comparisons, including all 768 restored
  terminal-cleanup configurations. Arrow: 17,114 comparisons. Four additional
  missing-export cases preserve terminal removal and foreign recipient state.
  Full fighter constructors/cleanup still gate runtime activation. Exact input
  copies and the console runner are checked in under `melee/tests/native`.
- Captain/Ganon: 144 generated ranged cases plus 3,602 source-reference/common
  caller cases pass (3,746 total). The capture hook in this preparation is C,
  not accepted generated DSL. Owned asset/bone metadata is audited. Remaining
  generic implementation work: CPU command-script selection, fourth-swing
  policy, and common taunt state acceptance. Ganon's double taunt entry is
  straightforward to preserve and remains in the draft.
- Ness: five article declarations and ordered source/field/callback maps are
  prepared, uncompiled and unregistered. Missing generic handle signals,
  original creator input, collision-group read, internal-stage query and camera
  release form the next coherent article batch. Reusing full cross-component
  arithmetic plus existing normalization remains tentative pending comparison.
- Behavioral fidelity is the stock acceptance standard where exact reproduction
  would require awkward UB/RNG workarounds; document deliberate differences.
  Native replay/rollback determinism remains mandatory. No additional game
  process or audio was launched, and Rook's installed content was preserved.

### 2026-09-30 â€” independent Captain Falcon/Ganondorf policies and gameplay

- Local commits `694cf6c50`, `3c1c4460f` and `c1874bb8a` add generic authored CPU
  command selection, integer held-swing decisions and direct common motion
  policies, repair grapple owner scale, and close Falcon/Ganon script ownership.
  No stock special handler delegates or fighter-specific API shortcuts were
  added. The four common motion callback pointers are assigned directly;
  original nonnull-phase coverage remains required by the independent guard.
- Both templates own four specials, versus lifecycle decisions, CPU capture/
  ranged/spacing choices, and taunt/swing decisions, with 37/36 editable
  parameters and exact 52/50 semantic rig mappings. 3,905 actual generated-module
  comparisons against original C pass, preserving ordered calls and RNG.
- Sixteen hidden, muted, accelerated stock/clone gameplay pairs with drawing
  pass: seven special recordings and one taunt per fighter, 8,280 compared
  controller samples. The 1,800-sample CPU runs stayed idle on both sides and
  are explicitly excluded from active CPU gameplay acceptance. Investigate
  that activation path rather than counting matching inactivity as proof.
- Full Release build passed. The 76 focused compiler/runtime checks are covered
  by the initial run and two corrected targeted rechecks; the new grapple-scale
  regression separately passes six actual production-body cases. Owner scale
  now reads Fighter.x34_scale.y, fixing the native Item/Fighter offset mismatch
  in Hookshot and all ten Samus callers, without preserving the old alias.
- Evidence: `melee/.local/captain-ganon-independent-acceptance.json`. Runtime,
  SDK and compiler frozen under baseline9 with 3,164 hashes. Rook's unchanged
  authored source was rebuilt for the new layout and installed as
  `3bd6ed2c5e5a168a956d8c1f748615edb0b47d33cbbd7c4f5866791cf1ed4122`.
- Four registered templates now declare independent behavior: Yoshi, Mewtwo,
  Captain Falcon and Ganondorf. Sixteen registered templates still retain stock
  fallback; Peach/Zelda/Sheik/Popo-Nana/Game & Watch remain unregistered. Kirby
  stays excluded. Complete Mario/Dr. Mario and remaining Link behavior are
  being audited; no further complete-fighter acceptance is claimed.
- Player-visible fidelity remains the stock standard. Defined choices for
  stock UB/incidental RNG are acceptable and must be documented; native clients
  must still agree for deterministic replay and rollback.

### 2026-09-30 â€” active CPU acceptance and new-life generation fix

Local commit `626e18456` fixes the native callback continuation check in
Fighter_UnkProcessDeath: capture the new generation after the caller's own reset
advances x8_spawnNum. The prior check rejected the normal new life and returned
before CPU setup. A hidden/muted debugger snapshot established correctly
configured Player CPU type 4 / level 9 but zero fighter CPU configuration, despite
the AI tick counter advancing. This was not a stock RNG/UB fidelity issue.

The full eighteen-pair Captain/Ganon suite passes again after the fix, including
active level 9 CPUs: 11,880 compared controller samples, Falcon 62 action
transitions / 11 damage events and Ganon 87 transitions / 6 damage events in the CPU
cases. The runner rejects idle CPU evidence. The eight focused lifetime/event
checks pass, including six actual death-reset cases preserving valid new-life
continuation and rejecting removal/recycling at either callback. Both Release
builds passed. Baseline10 freezes corrected runtime/SDK/compiler with 3,164 hashes;
no layout change invalidates Rook's rebuilt package. Evidence:
`melee/.local/captain-ganon-generation-acceptance.json`.

Read-only Mario/Dr. Mario audit is complete under
`melee/.local/mario-doc-independence/AUDIT.md`: existing scripts own all nonnull
special-motion phases. Next versus closure needs exact owned rig maps and two
Mario CPU callbacks using existing APIs. Dr. Mario's two-pill versus-results
constructor also needs generic demo-entry/article-reference ownership and an
ordered constructor comparison; do not introduce a pill-specific shortcut.
Mario video motions and Dr. Mario Classic/intro-only demo pills remain deferred
with singleplayer scope. No Mario/Dr. Mario implementation or additional
complete-fighter acceptance is claimed by this audit. Link's resource-backed
Hookshot and Ness Thunder remain their subsequent coherent article batches.

### 2026-09-30 â€” independent Mario/Dr. Mario and authored presentation

Both templates now declare independent behavior and own the exact stock rig
maps (61/86 physical parts, 51 semantic pairs each). Mario's capture and ranged
CPU decisions use existing generic primitives and project-owned parameters.
Dr. Mario's two-pill results variant is authored through generic demo-entry,
article reference binding, appearance, spin and immediate-update operations;
no original fighter constructor is delegated. Presentation attachments follow
their original creator independently of reflected damage ownership. The shared
immediate item update stops safely after callback removal or pool recycling.

The actual generated Mario module passes 31,432 original-source comparisons
and four parameter-edit checks. Dr. Mario's generated results constructor
passes 2,199 comparisons including actual article creation, binding, creator
lookup and lifetime bodies. Twenty hidden, muted, accelerated drawn match pairs
pass across eight ground/air specials, taunt and active CPU per fighter: 12,800
compared controller samples. Two initial all-CPU launch configurations were
rejected by the client's human-slot requirement and excluded; corrected active
CPU cases pass. Full Release build and twelve focused callback/lifetime/API
checks pass. Evidence: `melee/.local/mario-doc-independent-acceptance.json`.

Baseline11 freezes the matching runtime/toolchain. Rook was rebuilt from
unchanged authored source for the host-layout extension and replaced with
`a7e67aaf3176342294f84b8222c829c32014128c5eb65d365b6caa85d4d7360f`.
Mario/Dr. Mario test packages remain in isolated stores. Whole results-scene
visual acceptance, single-player demo constructors and unrecorded gameplay
edges remain outside this checkpoint. Six registered templates are independent
(Yoshi, Mewtwo, Falcon, Ganon, Mario, Dr. Mario); fourteen retain stock fallback.
Luigi/Jigglypuff independence is the next source audit. Kirby remains deferred.
Behavioral stock fidelity is acceptable around UB/incidental RNG; native replay
and rollback determinism remains required.

### 2026-09-30 â€” Luigi CPU/rig ownership and native action metadata

Luigi's versus template now declares independent behavior, owns its exact
51-role/61-part rig, and authors CPU ranged/recovery/destination decisions and
damaging-taunt classification through generic APIs and editable parameters.
Cyclone's defined uncharged life initialization remains intentional; reproducing
stock undefined memory is not required. Video-only demos remain deferred.

The native action-descriptor path now explicitly decodes canonical console
words instead of reinterpreting their bytes as a Windows C bitfield layout.
This fixes the actual taunt-classification mismatch and keeps original attack
counter ordering, shared heavy-item handling and statistics. New generic
`on action_metadata`, `action.metadata(...)`, `position.delta` and CPU
destination queries do not introduce fighter-specific scripting primitives.
Removed/recycled callback sources stop both metadata and motion-entry callers.

Frozen baseline14 passes 4,918 actual generated/source comparisons and targeted
taunt/active-CPU stock/script pairs (2,240 controller samples, CPU 137 action
transitions and 15 damage events). The full Release build and thirteen focused
checks pass. Source evidence: `melee/.local/luigi-independence-source/baseline14-fixture`;
gameplay evidence: `melee/.local/luigi-metadata-runtime14`. Rook's unchanged
source was rebuilt for the host layout and replaces its installed package with
`b8c088249a44945b303440eafe75badc0710776e7c1a3243f1468650156cdd0e`.

This is not whole-Luigi visual acceptance: Mario/Dr. Mario Tornado and Luigi
Cyclone model-effect updates still read old private move storage rather than
script-owned collision state. A generic authored model-effect lifecycle is the
next shared component. Jigglypuff's extra jumps and shield-break decisions have
source-only designs; no implementation or acceptance is claimed yet. Existing
fighter trace acceptance does not prove these remaining visual/behavior edges.

### 2026-09-30 â€” paused at model-effect backend checkpoint

Typed effect-spawn results replace the three mixed model/generator pointer
dispatchers. An immutable 44-recipe construction catalog supports one-joint
model policies. The generic backend binds direct authored EF updates to their
original creator, validates attachment/generation before and after animation,
and ends callback lifetime immediately on cancellation. Scene cleanup releases
bound effects before fighter modules. The effect allocation serial is verified
in the compiled `.mstate$m` rollback section. Shared HSD joint traversal and
channel operations are extracted into `lb/native_model_joint`, with article
calls and tracked source fixtures updated directly. No compatibility API remains.

Full Release build and fourteen focused checks pass. A hidden/muted accelerated
drawn stock Cyclone match completes 520 samples and enters motion357. This is a
smoke check, not full effect parity acceptance. Typed dispatcher source review
classifies 297 explicit cases and preserves constructor/RNG/animation ordering;
compiled branch and effect-specific rollback restore checks remain pending.
Details and limitations: `melee/docs/native_model_effects.md`.

Baseline15 freezes the matching runtime, SDK and compiler. Rook was rebuilt from
unchanged source for the new module layout, replacing the installed package with
`5e9812181ee217c87dfda32562ded001e98e88d720a7e0e4117cba4e8f76ddb9`.
The unfinished model-effect compiler is saved outside production under
`melee/.local/authored-effect-policy/compiler-candidate`; its partial production
edits were restored. Resume by completing compiler context validation and actual
generated callback comparisons, then translate Tornado/Cyclone. No complete
scripted effect ownership or additional whole-fighter acceptance is claimed.

Goal and agents pause here at the user's request; do not start another feature
until resumed. Preserve the full remaining scope and user-directed deferrals.

### 2026-10-02 â€” authored model effects and aerial-jump candidate

Work resumed at the user's request. Named model-effect definitions now compile
direct EF callbacks using the original creator's script-owned state. Generic
joint channels, subtree visibility and cancellation support Tornado/Cyclone
without stock private-state aliases or original fighter handler delegation.
Helpers are validated in their actual fighter/effect context, including nested
calls, callback binding and article-to-fighter calls. The scripting reference
includes a linked model-effect guide and float-precision math guidance.

Acceptance: 44 compiler cases, 78 generated-module cases, 103,680 comparisons
against unchanged original Tornado/Cyclone updater bodies, and 24 production
registry/lifetime rejection cases. The full RelWithDebInfo build and thirteen
focused checks pass. Baseline16 passes three hidden, muted, accelerated drawn
stock/clone match pairs (Mario, Dr. Mario, Luigi), comparing 1,560 controller
samples and 3,120 fighter states. Source comparisons use synthetic HSD graphs
and explicit engine I/O boundaries; renderer pixels, actual dispatcher branches
and effect-specific rollback restoration remain outside this acceptance.
Evidence and limits: `melee/docs/native_model_effects.md`.

Generic owned multi-jump tuning, shared aerial-jump physics/input, and a
shield-break hook are implemented in the same host ABI batch. Jigglypuff owns
its rig roles and new jump decisions in the candidate template, but retains its
stock fallback declaration until its original-source and gameplay acceptance
are complete. Named tuning remains editable fighter parameters. The compiler's
fixed import-attribute capacity now matches the existing 128-field owned
parameter limit. No fighter-specific API or raw-offset runtime script access
was introduced. Linked reference: `melee/docs/scripting/jump_policies.md`.

Rook was rebuilt from unchanged authored source and its installed package was
replaced once with `e11268582e9b8c4aeeff060bc290229612836b41b7b79d919f45e06f7f068c4b`.
Prepared game tests use isolated content stores. The local MSVC 14.44 library
environment must be selected explicitly for this build cache; `TOOLING.md`
records the command and the diagnosed older-library linker failure.

Continue Jigglypuff acceptance, then the remaining stock fighter closures.
Kirby and the previously user-deferred collaborative scope remain deferred.

### 2026-10-02 â€” Jigglypuff independent behavior accepted

Jigglypuff's template now declares `behavior independent`. All 32 extended
motions replace their non-null gameplay callbacks; its 26 semantic bone roles
and 50 runtime parts are explicit. The unchanged-original source fixture passes
1,578 comparisons plus 58 lifetime/resource cases (1,636 checks total), including
all twenty multi-jump callbacks, entry, turns, input gates, editable tuning and
shield-break ordering. No additional Purin-specific versus CPU or combat-article
decision was found. Costume accessories retain their authored attachment path.

Four hidden, muted, accelerated drawn stock/independent pairs pass: full five-jump
turn sequences in both directions, actual Bunny Hood ownership during jumping,
and blue-hat Rollout/contact. All required jump states and turns are observed;
the batch compares 3,900 controller samples and 7,800 fighter states. Acceptance
is bounded by these paths and the source fixture's explicit I/O/physics boundaries.
See `melee/docs/jigglypuff_script_translation.md` and
`melee/.local/purin-jump-runtime16/results.json` for evidence and limits.

Eight templates now declare independent behavior. Fox/Falco is the next bounded
batch: explicit rig roles/omissions, authored Fire travel model rotation, and
capture/ranged/recovery/projectile-defense CPU decisions. Link/Young Link also
has identified common-action, sword-part and tether gaps; do not mistake its
existing stock fallback declaration for complete player-visible parity.

### 2026-10-02 - Fox/Falco independent versus behavior accepted

The shared Fox/Falco template now declares independent behavior. Its CPU capture,
ranged, recovery and projectile-defense choices use ordinary owned parameters
and generic callbacks. FireTravel's model rotation uses generic effect channels,
creator facing and named state. Each base has all 52 retail semantic bone bindings;
all four costumes agree with 73/67 runtime joints and no omissions. The closure
census covers 140 non-null motion callbacks per fighter and every non-null versus
lifecycle slot. Demo constructor helpers and Kirby copies remain outside scope.

Final independent source evidence passes 292,326 CPU checks plus 18,432 effect
comparisons and 16 registry/lifetime rejection cases. Twelve hidden, muted,
accelerated drawn stock/independent pairs pass: ground/air Fire both directions,
CPU levels 4/9 on each fighter. All requested Fire states and active CPU damage
are observed; 11,360 samples and 22,720 fighter states compare exactly. Full
native build and focused compiler/module/rig/parameter/lifetime/independence
checks pass. Renderer pixels, exhaustive costumes/CPU levels and effect-specific
rollback restoration are not claimed. Ledger: melee/docs/fox_falco_script_translation.md.

Nine templates now declare independent behavior, representing ten stock fighters.
Rook rebuilt from unchanged authored source for the Definition layout and replaced
its installed package once with
0a83eaf60c1c33e6253e4f8274db71e378ad90ac04660ed00e8c57da31dfa286.
The unused raw Fox/Purin DLL build targets were removed: full templates require
normal project parameter import before compilation. Actual imported-package
checks remain in place.

Marth/Roy's next closure candidate has exact 52-entry owned rig maps and passes
3,094 original/generated source comparisons; production templates remain
inherited pending actual loader and coordinated drawn gameplay. This candidate
uses existing generic APIs. Preserve the user-directed deferrals and Link/Young
Link tether, sword-part and common-action gaps for continued roster work.

### 2026-10-02 - Marth/Roy independent versus behavior accepted

Both templates now declare independent behavior. Exact owned rig maps retain
52 semantic bindings and 90/92 joints. The original-C source suite passes 3,094
comparisons against shipped templates and frozen18. Fifty-two hidden, muted,
accelerated drawn stock/independent pairs pass, comparing 30,000 input samples
and 60,000 fighter states, with explicit special/common/CPU/contact coverage.
Corrected normal-input lifecycle recordings prove boost reuse, common landing
reset, and actual death/rebirth reset before a confounding landing. Initial
trajectory coverage misses are preserved and excluded from acceptance.

Eight final shipped-package pairs also pass (8,800 samples/17,600 states).
Generated behavior is identical to the candidate after excluding only exported
source-hash metadata; owned parameter/CPU bytes are identical. No new script
API was required. Evidence: melee/.local/marth-roy-independent-acceptance18.json;
ledger: melee/docs/marth_roy_script_translation.md. Real results/demo scenes,
exact rendered ribbons, all costumes/CPU branches and broader interruptions
remain outside this bounded acceptance. Eleven independent templates now
represent twelve stock fighters. Shared DK/Bowser effect support and Pika/Pichu
closure audits are underway; their templates remain inherited until accepted.

### Positioned and linked model effects checkpoint (2026-10-02)

- Generic named world-position/deferred bone-position construction and indexed
  linked effect models now use typed recipes, fixed group storage, generation
  validation and atomic cleanup. No fighter-specific API operation was added.
- DK Hand Slap and Bowser Fortress own their original effect update decisions.
  Frozen snapshot19 build and twelve focused Ctests pass; generated/original
  source comparisons pass 46,080 DK/Bowser, 103,680 single-model and 18,432
  Fox/Falco cases plus 48 registry/lifetime rejection cases across those suites.
- Six hidden, muted, accelerated drawn gameplay pairs pass in both directions
  (3,060 controller samples, 6,120 fighter states). Rendered effect pixels and
  effect-specific rollback are not claimed. DK/Bowser behavior identity remains
  inherited until separate whole-fighter closure acceptance.
- Rook was rebuilt and its installed package replaced once for the updated SDK;
  authored script hash is unchanged. Evidence is in the backend effect ledger.

### Pikachu/Pichu independent versus behavior checkpoint (2026-10-02)

- Shared Pikachu/Pichu template now declares behavior independent. Own rig maps
  retain 29 semantic roles each and exact53/46 runtime parts, no omissions across
  all eight costumes. No stock special/item handler delegation was added.
- Shipped-source original-C checks pass39,006 comparisons +8 edited parameter
  checks;104own phases per fighter include all96originalnonnull callbacks.
- Frozen19 focused candidate acceptance passes48hidden/muted/fast/drawn pairs,
  40,800samples and81,600fighter states. Actual shipped template passes12selected
  pairs,12,000samples/24,000states. Generated C differs only in source hash and
  FMP/CPU profile bytes are identical to the accepted candidate.
- Live Thunder-chain death/removal, auxiliary attachments and runtime scaling
  remain gameplay limits; no new rendered-pixel/audio/results/workshop/rollback
  claim. Evidence: melee/.local/pikachu-pichu-independent-acceptance19.json.
- Twelve independent templates now represent fourteen stock fighters. Continue
  DK/Bowser acceptance and generic Link/Samus resource support; Kirby stays out.

### Generic article-resource foundation checkpoint (2026-10-02)

- Bounded article-owned HSD model/CollData resources now compile into the client: six collections, up to90 nodes each, full Item/creator generations and exact immutable Definition ownership. External rig reservations are generic metadata inputs, not fighter-kind dispatch.
- Partial-construction release detaches exact known capture/hitbox/part borrowers before freeing any model. Item removal, creator rig destruction, and Definition retirement cancel retained identities before storage is destroyed. Delayed old-generation cancellation cannot destroy a recycled Item life.
- Production-body allocation/identity matrix passes475,148 assertions, sweeping270 allocation failure sites and540 simultaneously live nodes plus teardown cases. Full build20 and six relevant Ctests pass. COFF symbol evidence confirms the mutable pool is in the existing .mstate$m rollback section.
- Evidence: melee/.local/article-resource-integration20/receipt.json and melee/.local/article-resource-source19-integrated/results.json; backend contract: melee/docs/scripting/article_resources.md. These checks do not accept active-pool heap/rendering/rollback gameplay. Immutable ArticleLogic declarations, script operations, geometry/animation solver operations and full Link/Samus tether/lifecycle translations remain pending.

### DK/Bowser independent versus behavior accepted (2026-10-02)

- Both templates now declare independent behavior, bringing14templates/16stock fighters to independent scripted versus behavior. Own rig52/48semanticbindings and75/76runtimeparts are preserved; all46/23motion rows replace original nonnull phases and fighter/article/lifecycle/CPU/common decisions remain authored.
- Actual shipped source passes5,461 original-C comparisons. Ten hidden/muted/fast/drawn candidate pairs pass14,550samples/29,100states; four shipped-package pairs pass6,600samples/13,200states. CPU, cargo/heavy-item, punch release, Flame, down-B victim contact and side-B back throw are covered. Generated behavior and FMP/CPU data match candidate after source-hash-only normalization.
- Fixed exact observed DK collision-mask loss by typing its local integer; retained original failing source evidence. No specialized API or engine decision replacement was added.
- Evidence: melee/.local/donkey-bowser-independent-acceptance19.json. Broader lifecycle/holding-damage entry, costumes, CPUbranches, pixel/audio, Workshop and network rollback remain outside this selected acceptance. Generic Link/Samus tether resources are compiled groundwork, not playable tether closure; Ness closure is under read-only audit.

Article-resource lifecycle follow-up: detached package validation/staging can share
the live scene's DLL/Definition, so only a bound slot may cancel that Definition's
resources. Actual extracted slot-release code now proves both detached preservation
and bound retirement, within a 476,261-assertion/six-teardown-case matrix. Full
build21 and the same six relevant Ctests pass; evidence is
melee/.local/article-resource-integration21/receipt.json. No additional game launches.

### Scripted article resources and Ness PK Flash checkpoint (2026-10-02)

- Generic immutable resource declarations, model/ECB transforms, probes, animation
  operations and same-author article signaling now work through the compiler and
  native Host. Scripts own move decisions; no stock tether solver is delegated.
- Full build23 and 31 selected checks pass. Source evidence covers 477,371 resource
  assertions, 56 original geometry/animation helper cases, full generated-C
  compilation and 10,444 signal-dispatch assertions, including removal/reuse.
- Ness now scripts eight PK Flash fighter phases, three Flash article phases and
  the explosion. Fresh-import source comparisons pass 11,006 checks and all 22
  imported tuning words. Ness remains partial/inherited: bat, yo-yo, Thunder,
  lifecycle/CPU/jump closure and new-module gameplay acceptance remain pending.
- Corrected proven native integer-bool collision-mask loss in the shared item
  collision family and Peach turnip. Original-MSL comparisons pass 45,080 cases;
  twelve retail function hashes were verified. This is a port mismatch correction,
  not acceptance of Peach's full behavior. See melee/docs/native_item_collision_masks.md.
- Rook was rebuilt/replaced once for the SDK with unchanged scripts. Snapshot:
  melee/.local/baseline23; integration evidence:
  melee/.local/article-resource-integration23/receipt.json. No games were launched.
- Independent roster remains 14 templates / 16 stock fighters. Link/Samus tether
  translations and further Ness closure are next; Kirby remains excluded.

### Generic Thunder API and CPU continuation checkpoint (2026-10-02)

- Generic fighter/article normalized cross-product components, internal stage ID,
  item collision-group reads and camera-box release now compile through the native
  Host. Decisions remain authored; no Ness-specific engine primitive was added.
- Native attack/travel/special-candidate callers now stop before reusing a fighter
  removed or recycled by an authored callback. Actual-source acceptance passes
  86 cases / 1,128 assertions; the former travel continuation reproduces an
  access violation. Outer CPU coverage is explicitly only its candidate block.
- Full build24 and all 33 selected checks pass. Frozen24 contains 3,186 files.
  Rook's module was rebuilt/replaced once with unchanged source and assets.
  Evidence: melee/.local/article-cpu-integration24/receipt.json. No game launched.
- The composed Thunder ball/trail translation stays in an unregistered test
  candidate. The shipped Ness draft remains the accepted Flash-only source until
  fighter Thunder setup/cleanup is translated; stock union writes cannot safely
  run on newly authored article state. Independent roster remains 14 / 16.

Thunder article source follow-up: the unregistered candidate now passes 170,957
checks against the unchanged stock ball/trail bodies with a fresh frozen24
parameter import. All five tuning words match the DAT exactly. IDs/flags/order
are exact; ordinary float comparisons allow the documented 2e-6 relative rule,
so this is behavioral source agreement. Six trail spawn attempts, reflection,
steering, history, collision and cleanup are covered. Evidence:
melee/.local/ness-thunder-prep/source24/acceptance.json; reproduction and explicit
boundaries: melee/docs/ness_script_translation.md. The default Workshop draft is
unchanged; nine Thunder fighter phases and whole lifecycle still gate activation.

### Article helper and terrain checkpoint (2026-10-02)

- Article-local typed void functions compile directly to C, with scoped arguments,
  forward calls, generic engine operations and transitive Item generation checks.
  Pure helpers add no lifetime check; helper/local-signal cycles are rejected.
  Required Host entries include operations used only inside helpers.
- Direct accessory-before/accessory-hitlag callback bindings and owned-pointer
  identity checks support cleanup without fighter-specific shortcuts. Generic
  line-island bounds copy the original engine endpoints through typed outputs;
  collision floor/wall masks are named constants.
- Full native build and 37 selected CTests pass. Actual terrain lookup/backend
  tests pass 4,553 assertions across 913 cases, including 64 floor chains.
  Frozen25 contains 3,194 files. Evidence:
  melee/.local/article-helpers-integration25/receipt.json.
- Peach's ordinary rare Bob-omb saved scale now uses the receiving item's union
  member. The original console offset overlapped with Turnip; widening the native
  owner pointer broke that alias. Retail-pinned constructor/throw comparisons
  pass 153 pairs / 6,456 checks; the old native body retains 22 negative cases.
  This is a native data-flow repair, not complete scripted Peach acceptance.
- Rook was rebuilt for the Host layout, with script bytes unchanged, 285 own-rig
  clips and zero borrowed clips. Install-project replaced her previous version;
  four installed packages remain. No game was launched.
- The nine Thunder fighter phases are now an authored unregistered candidate.
  Source comparison, lifecycle reset and bat/yo-yo cleanup still gate activation.
  Link's held-tether fixture has the shared APIs but still needs actual generated
  constructor/solver comparison. Independent roster remains 14 templates /
  16 stock fighters; Kirby stays deferred.

### Generic compiler and held-asset import checkpoint (2026-10-02)

- Fighter position components are directly writable from authored behavior, with
  no inferred velocity or collision sweep. The generic facing-side guard filter
  is independently editable from guard activation and reactive treatment.
- Shared expression validation now rejects calling scalar fighter/article
  properties, state fields and locals as functions, while retaining math calls,
  casts and grouped operators. Actual generated guard tests cover 216 facing
  pairs and the complete guard-create/reset body; full collision traversal is
  outside that fixture's scope.
- Hookshot asset import records now match both Link and Young Link OnLoad
  declarations (slot 2, owned kind attribute at byte 188). Fresh normal imports
  and generated modules succeeded. This is asset-import support, not acceptance
  of the complete tether move or lifecycle.
- Full native build and all 39 selected compiler/engine CTests pass. Frozen27
  contains 3,194 files; evidence is
  melee/.local/script-contracts-integration27/receipt.json. Frozen26 is retained
  as preflight evidence: the initial guard assertion failed, and further negative
  cases exposed invalid scalar calls. No Host layout changed, so Rook was not
  installed again.
- Composed PK Thunder fighter/article and held-tether source comparisons are
  still unregistered candidates. Full lifecycle closure and gameplay acceptance
  remain separate gates. Independent roster stays 14 templates / 16 fighters.
  No game was launched for this checkpoint.

### Thunder fighter and held-tether source checkpoint (2026-10-02)

- The unregistered Ness candidate composes all nine Thunder fighter phases with
  all five authored Thunder articles. Fresh owned imports pass 241,607 source
  checks across 1,088 fighter scenarios, plus 53 common-transition ordering and
  lifetime checks. Fifteen Thunder tuning words match the DAT exactly.
- Four shallow wall-slide configurations need a velocity-length-relative float
  bound because editable binary32 angle constants replace the original double
  constants. Maximum component difference is 0.00000238418579; state choices,
  callback order and IDs remain exact in the compared cases. See
  melee/docs/ness_script_translation.md for the precise numeric scope.
- Normal imported Link/Young Link held-tether candidates pass 1,710,332
  assertions across 1,927 original-source phase cases per fighter, with zero
  compared float difference. They cover constructor, held state, accessory
  updates and teardown; the remaining deployed states and fighter flow are open.
  Controlled allocation, math, stage and root-item boundaries are listed in
  melee/docs/link_tether_script_translation.md. This is not whole-engine or
  gameplay acceptance.
- Default Ness and Link examples are unchanged. Complete resource initialization,
  ordered fighter lifecycle and remaining shared policies still gate promotion.
  Independent roster remains 14 templates / 16 fighters. No game was launched.
  Verified receipts and hashes: melee/.local/source-candidates-integration27/receipt.json.

### Ordinary item constructor policy checkpoint (2026-10-02)

- The common constructor carries an explicit Engine / Parent / Author behavior
  selection independently of package identity. Actual selection was split out
  of item.c into native_item_behavior.c; existing parent and explicit-author
  creation preserve their established policy. The direct Item_CreateEngine
  entry validates loaded ordinary common-item assets before writes/allocation
  and bypasses both parent asset and callback substitution.
- Actual selector and extracted constructor entry tests pass 125 checks across
  24 source configurations, including conflicting parent declarations, exact
  original selection results, complete spawn inputs and failed construction.
  Allocation/model work and hold-kind classification are controlled boundaries.
- Full build and all 40 selected CTests pass. Frozen28 contains 3,196 files;
  receipt: melee/.local/item-behavior-integration28/receipt.json. No Host/Item
  layout changed, no package was installed, and no game was launched.
- Generic retained creator identity, held publication, attachment and remembered
  scale still gate ordinary-item scripting and Peach promotion. Ness and Link
  candidates remain unregistered. Independent roster remains 14 / 16.

### Next source-guided closure batch (2026-10-02)

- Source-only Ness preparation is frozen at
  melee/.local/ness-resource-closure/checkpoint.json. It records stock bat/yo-yo
  construction and Yoyo -> Flash -> Thunder -> Bat cleanup, including nested
  resume-before-handle-clear. The complete composition needs at least 156 named
  editable values; review a generic bounded increase from 128 to 256 rather
  than packing or discarding tuning. Existing byte imports, hitlag and reflect
  hooks will be reused. All fragments are uncompiled and unregistered.
- Remaining Link/Young Link tether drafts and exact source/API inventory are
  frozen at melee/.local/link-tether-rest/source_hashes.json. Both statically
  fit current storage/function limits; all nine article states and AirCatch
  decisions are drafted. Full lowering, generated-C compilation, source
  comparisons, ground capture and fighter lifecycle are still pending. Explicit
  landing setup already exists and needs no new API.
- Frozen27 ordinary attachment/scale preparation passes 120 cases / 6,202
  assertions; receipt is
  melee/.local/peach-ordinary-items/attachment-fixture/result.json. Actual
  descriptor traversal, callback lifetime and scale writes are exercised with
  controlled audio/debug/geometry boundaries. It does not claim owner-removal
  protection, constructor allocation parity or a completed scripted feature.
- The next implementation should batch the generic retained-item/attachment/
  scale contracts, construction flags and partial animation import, plus the
  audited item/tether primitives. Rebuild packages once per required SDK/Host
  layout change. Preserve all existing accepted candidates until full composed
  cleanup and phase comparisons close their activation gates.

### Shared item and import integration checkpoint (2026-10-02)

- Fighter scripts can create ordinary common engine items, attach them to a
  rig part, inherit current-owner scale, bind/clear held or auxiliary slots,
  and read exact 32-bit item IDs. Each step is explicit; clearing does not
  destroy an item. Engine behavior selection bypasses package substitution.
- Ordinary items retain their original creator independently of current
  ownership. Constructor and attachment callback checks reject removed or
  recycled sources. The actual ownership/backend fixture passes 97 cases /
  1,052 assertions; generated item modules pass 48 cases / 1,204 assertions
  across both lowering forms, plus 106 compiler checks. Existing authored
  article definition-identity semantics are deliberately preserved and named
  in melee/docs/native_item_owner.md; controlled engine boundaries remain.
- Bob-omb, Mr. Saturn and Turnip now share Item.throw_scale instead of private
  union aliases. Retail-pinned source comparisons pass 404 pairs / 16,022
  assertions, retaining throw/drop/timer order and the Beam Sword exclusion.
  The 22 historical Bob-omb alias mismatches remain recorded as negative
  evidence, not accepted parity. See melee/docs/native_item_throw_scale.md.
- Owned parameters now support 256 values under a central 64 KiB text bound.
  Joint, material and shape article animations import independently, including
  material-only final pointer words. Real importer and generated-module checks
  cover bounds, shared graphs, omitted components and publication on success.
  Generic pause/resume, creator instance-scale reads and callback-slot queries
  also have compiler/source comparisons; no fighter-specific shortcut was added.
- Full build and all 52 selected CTests pass. Dr. Mario's freshly imported and
  compiled results-pill path passes 2,199 source comparisons after direct
  binding callsite migration. Frozen29 contains 3,198 runtime/SDK/compiler files;
  receipt: melee/.local/item-contracts-integration29/receipt.json.
- Rook was rebuilt, verified and replacement-installed for this layout, with
  script unchanged, 285 own-rig clips and zero borrowed clips. No game was
  launched. The collection enumerates one valid installed package (Rook);
  three unrelated pre-existing marker files remain untouched.
- Ness, Link and Peach still need full composed resource initialization,
  cleanup, remaining phase lowering and relevant source comparisons before
  promotion. Default templates and accepted frozen candidates are unchanged;
  independent roster remains 14 templates / 16 stock fighters. Kirby remains
  deferred. This checkpoint does not claim whole-fighter or gameplay acceptance.

### Stage and item construction checkpoint (2026-10-02)

- Generic scripts now support stage segments, global sound, fast-fall, copied
  ledge probes, ledge occupancy and shared ledge-jump phase 2. Runtime fields
  expose position deltas, Z acceleration, collision position, tether use and
  special landing lag. No fighter-specific operation was added.
- Optional air_tether_input replaces the entire native predicate at the normal
  interrupt priority. Absent-hook stock gates and source invalidation pass
  5,183 actual-body checks. Typed compiler checks pass 138 cases; generated
  action/callback forms pass 4,449 checks each over 128 field/query cases.
- Authored construction can explicitly skip initial collision setup. Both
  actual descriptor values and the default are checked; direct fixture
  callsites were migrated. Ness copied assets now include Bat9 and YoYo10,
  matching all eleven original registrations without activating their scripts.
- Full build and 57 selected CTests pass. Fresh Dr. Mario import/generated
  source comparison again passes 2,199 checks. Frozen30 records 3,198 files.
  Rook was replacement-installed for this layout with unchanged source,
  285 own-rig clips and zero borrowed clips; no game was launched.
- Default templates and accepted fighter candidates remain unchanged. Source
  composition work continues independently; this is shared infrastructure
  acceptance, not a claim of full Ness, Link or Peach gameplay closure.
  Details: melee/docs/native_stage_item_contracts.md.

- Final compiler correction: runtime state.freefall now accepts typed runtime
  expressions, including the retained special landing lag. Removed the unused
  constant-only parser; tether-usage writes are strictly Bool. Frozen31 records
  the final compiler with identical Host/runtime contracts. Full build and
  all 57 selected CTests still pass: 158 compiler checks and 4,460 checks per
  generated movement module. Dr. Mario again passes 2,199 fresh comparisons;
  Rook verifies with the final tool without another package replacement.

### Source checkpoint after shared contracts (2026-10-02)

Tracked, unregistered Ness resource-closure fixtures now reproduce 67,270 lifecycle/publication checks and 122 actual Bat construction/pickup checks. YoYo starting-position initialization and full smash behavior remain pending. Link and Young Link deployed/retracting tether state 4/5 fixtures reproduce 2,496 actual source phases with zero measured float difference. Remaining tether/capture phases and complete-fighter activation are not accepted by those bounded comparisons. Independent root reproductions used frozen SDKs 30/31; no game windows were launched.

### Positioned resource construction (frozen SDK 32)

Article resources can now be constructed with an initial world XYZ position. The backend seeds node and ECB current position before native collision initialization propagates all history fields; later position writes retain their existing move semantics. A constructor descriptor is included in the exact-client layout fingerprint; call sites were migrated directly. Build and 57 affected checks pass. Existing Link/Young Link deployed/retraction source comparisons still pass on a fresh SDK32 import. Rook was rebuilt/replacement-installed with unchanged source and 285 own-rig animation clips. Actual YoYo initialization comparison and full smash behavior remain pending. No game process was launched.

### YoYo initialization and Peach phase source checkpoint

The new unregistered YoYo initialization fixture independently reproduced 504 original/generated pairs and 60 defined failure cases against frozen32, including actual resource backend and native collision initialization. Node/current/last/previous/x28 positions, instance-scaled tip ECB, anchor, reversal, material animation sequencing and initial tip/string draw gates agree in the tested range. Separate Peach Turnip phase fixtures reproduce 329 source pairs; thrown instance-scale restoration, masked stage response and ordered full resource cleanup remain pending. None of these bounded source fixtures activates a complete new fighter template.

### Article scale/contact contracts and AirCatch checkpoint (frozen SDK 34)

Scripts now read remembered item throw scale and live current-owner instance Y
scale, restore instance/model/gameplay geometry scale, and apply typed stage
contact responses. These are reusable engine operations, not fighter-specific
behavior delegates. Owner scale resolves a live fighter identity before reading
it; missing or stale owners return scale 1. Wrong argument types are rejected.
The final build and all 58 affected checks pass. Frozen34 records 3,198 verified
runtime/SDK/compiler files; frozen33 was an unreleased failed intermediate.
Rook was rebuilt and replacement-installed with unchanged script and 285
own-rig clips. No game process was launched.

Fresh Link/Young Link AirCatch fighter-phase comparisons reproduce 487,476
assertions over 8,730 source phases with zero measured float difference.
Article construction/solver and stage/state boundaries remain controlled;
complete tether states, capture, cleanup and activation remain open. Peach
full Turnip phases and Ness YoYo solvers are using the new shared contracts
in separate unregistered candidates. Details live in
melee/docs/link_tether_script_translation.md and
melee/docs/scripting/article_instances.md.

### Live saved-item binding contracts (frozen SDK 35)

Generic existing-item bindings now restore live item handles without creator
restrictions, ownership changes or reattachment. Handle-based visibility,
pause/resume and drop notification are available separately so scripts own
their ordering. Drop notifications validate source and item identities across
synchronous callbacks, and generated helpers stop invalidated callers.
The build and 58 affected checks pass; generated action/callback tests cover
high-bit handles and callback removal/reuse. A fresh Dr. Mario import and
results-pill source comparison again passes 2,199 checks after the shared
item lookup refactor. Frozen35 has 3,198 SHA-verified files. Rook was
replacement-installed with unchanged source and 285 own-rig clips; no game
was launched. This closes a generic contract needed by Peach, not complete
Peach lifecycle acceptance.

The full post-construction Turnip candidate independently reproduces 4,509
original/generated pairs and 92,827 checks against frozen34, including the
actual instance-scale and stage-response bodies. Geometry/stage subdependencies
remain controlled and constructor evidence is separate. Exact scope and
reproduction: melee/docs/peach_turnip_full_phase_translation.md.

### Generic Bool storage and YoYo solver checkpoint (frozen SDK 36)

The Link tether source comparison exposed truncation when a nonzero button
mask such as A (`0x100`) was stored directly in a Bool byte. Bool byte and
bitfield writes now canonicalize nonzero to true before narrowing; typed Bool
locals, arrays and setter arguments already use normal C Bool conversion.
Generated action and motion-callback regressions cover fighter state, creator
state and engine flag writes. Build and 58 affected checks pass. Frozen36 has
3,198 verified files with the same Host layout as35. Rook's unchanged script
was rebuilt/replacement-installed; no game was launched.

The separate unregistered Ness YoYo solver fixture independently reproduces
2,556 original/generated physics pairs, six defined lifetime/default-owner
cases and 5,075,363 checks with zero measured numeric difference. The actual
resource backend executes; stage responses and HSD graph construction remain
controlled. Animation, entry/release/retraction, smash phases and whole-fighter
lifecycle composition still gate activation. Scope and reproduction live in
melee/tests/native/ness_yoyo_solvers/README.md.

Fresh frozen36 Link/Young Link remaining-article reproduction now accepts the
bounded states1/2/3/6/7/8 and spool source slice: 244,184 graph comparisons and
113,372 spool comparisons, with maximum float difference 1.91e-6. The explicitly
substituted undefined stack-spill sqrt helper and spool dirty-flag differences
are documented. Grounded grab notifications, composed cleanup and complete
fighter activation remain open. The candidates remain unregistered; no game
was launched. Details: melee/tests/native/link_tether_remaining/README.md.

### YoYo entries, Peach cleanup and pickup lifetime checkpoint (2026-10-02)

The separate unregistered Ness YoYo/Bat entry and animation fixture independently
reproduces 2,604 original/generated pairs and 1,370,642 checks against frozen SDK36,
with no observed numeric difference. Full fighter smash integration and stale-owner
motion queries remain open. The Peach saved-item restore and ordered resource
cleanup fixture independently reproduces 1,500 source pairs and 33 separate safety
cases (35,898 checks) against SDK37. Changed Parasol holders need typed recipient
results; the successful producer constructor's attachment/debug notification
coupling remains excluded. The native pickup notification now stops its statistics
tail if the pickup callback removes or recycles the fighter. Healthy source
behavior is unchanged. No game process was launched; all commits remain local.

### Typed helper results (frozen SDK38)

Fighter and model-effect helpers now declare bool/int/float return types; exported
fighter helpers can return those values to typed recipient calls. Recipient
resolution checks the export result type and live creation ID, uses the recipient's
own state/parameters, and retains source-removal/recycling checks before subsequent
caller writes. Existing targets.call reports dispatch success and discards a typed
result. Typed calls return neutral values for absent/mismatched recipients. No
fighter-specific accessor or stock delegate was added. Helper argument effects are
evaluated once in source order. Integer IDs remain exact; callback slots reject
nonvoid helpers. Missing return paths and invalid scalar postfixes are rejected.
Build and 61 affected checks pass, including both generated routes, typed effect
cancellation, missing Host operations, nested argument ordering and source lifetime.
Frozen SDK38 has 3198 SHA-verified inputs; Rook was rebuilt and replacement-installed
with unchanged source and 285 own-rig clips. No game was launched. Peach's transferred
Parasol result path is being migrated into its separate source fixture.

### Grounded tether and live recipient-motion checkpoint (SDK39)

Link and Young Link's unregistered grounded timing and fighter cleanup-decision
fixture independently reproduces 608,690 assertions across 12,980 cases. Eight
common Catch/CatchPull/CatchWait hooks and full transferred article/resource
composition remain open; controlled article removal effects are not full teardown
acceptance. The generic targets.motion query now resolves a live creation ID,
returning -1 when absent. holder.motion uses the same direct Host path; its old
raw-pointer helper was removed. Build and 61 affected checks pass, plus targeted
invalid-handle grammar checks. SDK39 has 3198 SHA-verified inputs, and Rook was
replacement-installed with unchanged source. No game process was launched.

### Typed Peach cleanup and YoYo owner-resolution acceptance

The independently rerun Peach fixture now passes 3,420 original/generated pairs,
33 separate safety cases and 87,163 checks on frozen SDK38. Its 1,920 transferred
Parasol cases resolve the current holder's own saved-item state and typed Bool
result directly; the temporary creator-state reply field was removed. This
remains an unregistered source candidate. The successful producer constructor's
attachment/debug notification gap remains open; no full Peach activation is
claimed. Receipt: melee/.local/peach-resource-cleanup38-root/result.json.

The YoYo entry/animation fixture independently passes 2,604 healthy pairs,
16 separate owner safety cases and 1,370,954 checks on SDK39. Its owner motion
queries now execute the actual live-ID registry path. Retained old IDs reject
recycled objects; a current raw owner pointer still refers to its current live
object rather than recovering a historical identity. Full Ness smash integration
remains open. Receipt: melee/.local/ness-yoyo-entries39-root/receipt.json.
All changes are committed locally. No game process was launched.

### Link article import metadata checkpoint (frozen SDK40)

Stock archive metadata now includes each Link family's Boomerang slot and
Young Link's Milk slot, confirmed against the original OnLoad registration.
Fresh normal imports reproduce all 34 Boomerang attribute words exactly; Milk
is kept only for Young Link. Full build and four relevant parameter/import,
installed-clone and article-link checks pass. SDK40 records 3,198 verified
inputs with an unchanged Host layout. Rook's installed SDK39 module remains
compatible; no gameplay test or replacement installation was needed for these
metadata-only changes. Import receipt:
melee/.local/link-article-bindings40-import/receipt.json.

Full ordered transferred-article cleanup is still an unregistered candidate.
Its successful normal import does not prove callback or resource teardown
behavior. The new composition exposed a local signal declaration ordering
constraint during script generation; resolving generation and comparing actual
callbacks remain separate from this accepted import metadata checkpoint.

### Shared authoring and article composition checkpoint (SDK41/42)

The old weekly-usage cutoff was withdrawn by the user. Keep working until a
new explicit pause request or current usage constraint replaces it.

Link and Young Link's full ordered article cleanup now imports normally with
owned Bow/Arrow declarations and passes a fresh stock/generated comparison:
18,432 defined cases plus 66 separately classified safety cases. Actual item
removal, article destruction callbacks, typed references/signals, recipient
exports, resource release and exact borrower invalidation execute. Controlled
allocation, HSD/scene and effect/audio boundaries remain documented. This is an
unregistered component candidate, not complete fighter activation. Receipt:
melee/.local/link-article-cleanup40-root-verification/acceptance.json.

Peach's complete Parasol producer/attachment composition passes 3,840
original/generated pairs, six safety cases and 96,384 checks on both SDK41
and SDK42. It executes 54 complete original bodies, including actual drop,
attachment and both holder debug notifications. Final held-slot publication
follows the attachment chain. Invalid-rig cleanup is explicit authored safety
behavior. Full Peach and Parasol travel remain pending. Current receipt:
melee/.local/peach-parasol-creation42-root/result.json.

Generic SDK operations now expose shared smash charge storage, fixed-box
terrain sweeps, victim-history clearing, exact capsule state, stack-vector
rotation, held Attack input, Bool IASA and Song immunity. The misleading
reset-detection duplicate was removed: stock lbColl_80008434 only enables a
hitbox, already covered by set_enabled. Existing script callers were migrated
directly; no compatibility operation was retained. Charge interoperability
passes 480 source pairs and 120 bounded-write cases (11,378 checks), with
ordered presentation boundaries. Current receipt:
melee/.local/smash-contracts42-root/receipt.json.

The full native build and 49 related console checks pass. Frozen SDK42 records
3,202 SHA-verified files. Rook was rebuilt and replacement-installed against
the current Host layout, retaining her exact script source and 285 own-rig
clips. Installed package:
74ebb42c97f5cf2651a10faf5cd974cf86719995e3eef35c1a9cf18444f59435.
No game process was launched. All accepted source work is committed locally;
no upstream push. Next: generic common-entry hooks and typed local bone-query
outputs, then complete Ness/Link/Peach composition and template activation.

### Complete smash entry and local bone queries (frozen SDK43)

Scripts can now replace forward/up/down smash entry after common input and
held-item priority checks, before any stock fighter transition. Forward entry
receives the selected stick angle with facing already chosen. The existing
post-entry presentation hook retains its separate scope. Both action/state
source forms generate these callbacks, and independent modules clear inherited
undeclared callbacks. Documentation links these hooks to shared charge storage.

Bone world-position queries now accept distinct Float locals or fighter fields,
including mixed destinations, with optional local offsets. Inputs evaluate once
in order, and invalid/missing joints write zero. The compiler now correctly
classifies bones.index as Int; this fixes the existing Link fixture exposed by
the stricter integer joint contract. No persistent scratch state is required.

Full build and 49 related compiler/module checks pass. The independently rerun
common input/dispatch comparison passes 108,000 configurations plus 20 protected
or recycled callers (1,471,692 checks). Actual full input/dispatch bodies and
production lifetime code execute; whole motion initialization and item actions
are controlled ordered boundaries, not complete gameplay acceptance. Receipt:
melee/.local/smash-entry-dispatch43-root-accepted/receipt.json.

Frozen SDK43 records 3,202 SHA-verified inputs. Rook was rebuilt and
replacement-installed with unchanged script source and 285 own-rig clips:
67122b4274848267c4bd4c47eb7d0cc6e398dbef2a7e7ba096d2fb32819e1393.
No game process was launched. Ness smash and Peach component compositions
remain unregistered candidates; normal Peach clone-base/article metadata import
is pending, alongside its remaining fighter behaviors. Full Link grounded
capture and common presentation integration also remain pending. No quota
cutoff applies until the user supplies a new instruction.

### Authored grab policies and Ness yo-yo cleanup (frozen SDK44)

Grounded grab scripts now own eligibility, startup notification, animation
decisions, exit cleanup, pummel/throw resource release, pull anchors, readiness
and positional-follow policy. The common capture state flow stays in the
engine. Declared overrides replace fighter-specific decisions; absent hooks
retain the existing flow. Generation and retained-link checks stop callback
continuations after removal, recycling or replacement. Both script source forms
generate these hooks and independent modules clear inherited undeclared hooks.
The source dispatch fixture independently passes 154,694 cases and 2,930,526
checks, including 175 lifetime cases; frozen43's unguarded retirement path is
retained as an access-violation negative control. Actual function bodies and
production lifetime tracking execute; motion initialization, presentation,
world-transform outputs and selected engine decisions are controlled boundaries.
Receipt: melee/.local/catch-dispatch44-root/receipt.json.

Ness's composed seven-state smash candidate passes 6,528 source pairs and ten
lifetime cases on SDK43. It now intentionally fixes the stock yo-yo glitch:
the delayed refresh enabled its hitbox without setting the fighter-wide cleanup
marker, letting it survive ordinary motion changes. Generic script hitbox enable
now maintains that marker. Script comments explain the original cause and the
correction. Victim clearing, activation order and explicit preserve-hitbox
transitions retain their separate behavior. A fresh SDK44 reproduction passes
214,186 checks, including 30 cleanup regressions and 180 explicitly asserted
intentional marker differences. Original unlatched cleanup remains the negative
control. These checks execute the extracted common cleanup block, not the whole
motion engine or a rendered glitch reproduction. Receipt:
melee/.local/ness-smash-composition44-root/receipt.json. The composed candidate
remains unregistered: default partial Ness still inherits stock smash handlers
and does not receive this authored correction until full composition activates.

Peach's staged lifecycle/Turnip/Parasol-producer composition passes 827 source
pairs and 4,688 checks. Its real retail DAT article importer now produces all
18 Turnip values through the five stock article mappings; the complete candidate
owns 26 imported values and 14 named literals. Inherited motion rows and partial
Parasol/Toad articles remain explicit. Normal Peach clone-base registration and
its remaining fighter phases are pending; no default was promoted. Receipt:
melee/.local/peach-fighter-composition44-root/result.json.

Full native build and 49 related compiler/module checks pass. Frozen SDK44
contains 3,202 SHA-verified inputs. Rook was rebuilt and replacement-installed
with unchanged source and 285 own-rig clips:
0dd39e5cab6096947b3fc256983c0d36d82a12171a34f726e89bbc121694a44d.
All work is committed locally; no game process was launched or upstream push
made. Next: complete Ness lifecycle/common policies and combined template
activation, integrate Link/Young Link's authored grounded capture, and continue
Peach's remaining phase batches. The broad goal remains active.

### SDK45: larger authored programs and normal Peach cloning

Fighter helper capacity is now 128. Fixed two-word dependency sets replace the
single 64-bit mask throughout recursion rejection, effect-context propagation
and callback lifetime analysis. Cross-word regressions cover the final supported
helper, indirect recursion and effect-only dependencies; generated gameplay
functions retain direct calls with no new runtime allocation. Documentation and
all compiler callsites use the new capacity. Full native build and 51 related
compiler/module/metadata tests pass; frozen45 records 3,205 inputs.

Peach now has a normal stock asset-clone registration with copied retail rig,
action/demo, costume, article and CPU metadata. Its behavior remains inherited
until full authored composition is activated. A real clone-stock project and
parameter import reproduces the staged lifecycle composition: 827 source pairs,
4,688 checks, no failures. Receipt:
melee/.local/peach-fighter-composition45-root/result.json. This is a compiled
source comparison with controlled engine boundaries, not independent package
activation or rendered gameplay. No game process was launched.

Next: complete combined Ness/Link/Young Link/Peach move and resource behavior,
close the remaining generic Ness presentation, jump and CPU policy interfaces,
and verify complete clone activation. The broader goal remains active.

### SDK46: authored common policies and combined source comparisons

Generic hooks now cover death/sleep presentation item policy and successful CPU
projectile-hit classification. CPU special candidates return a Bool consumed
decision and explicitly write `cpu.selected_command`; true can preserve the
existing command, including zero. General air acceleration can use an owned
horizontal accumulator, and adjusted animation vertical velocity exposes the
existing equipped-item factor. Source generation and common continuation paths
check callback lifetime. Full native build and 52 grouped checks pass. An actual
original-C versus scripted Yoshi CPU comparison passes 9,297 exact comparisons.
Frozen46 records 3,205 inputs. Ness-specific original common-loop/presentation
comparisons are being assembled separately; this SDK contract does not itself
prove complete Ness behavior.

Fresh real clone/import comparisons also pass Peach pluck/smash/Explode (5,028
source pairs, 172,711 checks, six authored safety cases), plus combined grounded
Catch/tether and article cleanup for Link/Young Link (1,546 grounded cases,
18,432 cleanup source cases, 32 ordered timelines, 70 safety cases). Receipts:
melee/.local/peach-pluck-smash46-root-normal/result.json and
melee/.local/link-grounded46-root/acceptance.json. These execute actual generated
modules and extracted original decision bodies with documented controlled engine
boundaries; default templates are not yet activated.

Rook was rebuilt and replacement-installed against46 with unchanged source and
285 own-rig clips: 4e4af993ca42347a72786238129fa5bbbd708242dccc9068dc5375d0c5112307.
No game process was launched. All committed progress stays local. Next: complete
Ness assembly, Peach counter/Toad/Spore, and Link/Young Link full activation.

### SDK47: shared authoring bound and dynamic hitlag slots

One shared behavior source bound now permits 512 KiB through DAT parameter import
(input and converted output), compiler and article catalog, command-line reader,
workshop draft/template reads, multi-file composition/save and validation. This
is an authoring-only capacity increase; gameplay still executes compiled direct
callbacks. Tests cover acceptance beyond the former 128 KiB cap, the exact new
bound, and rejection one byte over through import/catalog/C generation. Parameter
text retains its distinct 64 KiB bound.

Generic named `callbacks.pre_hitlag` and `callbacks.post_hitlag` bind or clear the
actual engine slots using the same validated zero-argument generated-function
contract as existing callback binding. These allow counter/attachment scripts
to change pause/resume policies dynamically without stock fighter delegates.
Full native build and 55 grouped compiler/module/parameter tests pass; frozen47
records 3,206 inputs. Runtime ABI remains 46; Rook does not need another rebuild.
Ness full assembly and Peach counter use this freeze next; Link/Young Link full
closure composition is being prepared. No game process or upstream push.

### SDK48: common-entry completion and guard presentation policies

Generic wait, crouch-wait and aerial entry completion hooks now run after common
motion/held-animation setup. Actual entry-body tests verify ordering, stock
fallback, and early termination when callbacks remove/recycle or change the
fighter/definition. Both DSL forms bind these hooks. Saved previous visibility
variants are separately queryable from the current variant; both queries reject
indices outside the actual fixed visibility array. Two independent shared guard
flags are readable/writable: eligible-attack filtering and skipped contact details.
They do not imply reflectable attacks or replace collision traversal.

Full native build and 56 grouped checks pass. Frozen48 records 3,206 inputs.
Rook was rebuilt and replacement-installed with unchanged source and 285 own-rig
clips: 2f369b76d329afe6d49ce426c02dc28fbca8cc62fe9ad9f4416b2053a67a979e.
No game process was launched. Next: generic immutable model-graph insertion for
Link/Young Link and full composed fighter activation.

Peach counter/Toad/Spore was independently reproduced through fresh stock clone,
real parameter import and frozen47 compilation: 2,297 original/generated pairs,
18,652 checks and 11 separate safety cases. Receipt:
melee/.local/peach-counter47-root-repro/result.json. Controlled rendering/debug
boundaries and the inner raw debug-owner caveat remain explicit in the fixture
README. This does not promote a complete independent Peach definition; its
remaining full composition is being assembled separately.

### SDK49: owned reserved model parts and ordered common interrupts

Scripts can install an owned decoded joint descriptor into a copied omitted part
in both current/alternate poses. The implementation uses each package's immutable
joint-graph mask and copied placement tables, validates reservation/parents/source
ordinal, and performs shared model creation/insertion/registration. Physical rig
counts stay unchanged. Generic populated graphs append drawing objects instead
of overwriting costume entries; allocation failure leaves the rig unchanged.
Four individual common taunt/crouch/turn/walk attempts allow script-owned interrupt
ordering. Common lightshield recoil endpoints are named read-only values.

Full native build and 57 grouped checks pass, including 16 original/native
insertion pairs and failure/append checks. Five malformed/type/article-context
model-graph inputs are rejected. Frozen49 records 3,208 inputs. Rook was rebuilt
and replacement-installed with unchanged source and 285 own-rig clips:
295dcce893ced6da26aa3b6e96d7e8f69264a993a2cb2ab19cb3229b3b4d5bd8.
No game process was launched. Next: Link/Young Link whole-fighter closure with
these generic primitives, independent Ness activation, and Peach Float/SideB/
Parasol common policies.

Root reproduced full Ness composition on frozen47 versus frozen45: 1,135,039
checks, no unexpected failures, all 142 original non-NULL gameplay phase slots
and eight special entries audited. Receipt:
melee/.local/ness-fighter-composition47-root/receipt.json. Constructor positions,
ordered YoYo/Flash/Thunder/Bat cleanup, and the intentional yo-yo glitch fix are
preserved. Original NULL Fire IASAs become authored no-ops; 624 exact rounded
CPU threshold endpoints have explicitly documented tiny behavioral differences.
These controlled source comparisons do not claim gameplay or independent default
activation. A separate activation audit is starting.


### SDK50: shared aerial authoring policies and independent Ness

Added generic manual/automatic aerial-input policies at the original common
Fall/Jump/JumpAerial positions and a grounded-motion setup hook before common
flag resets. Original Peach input checks now live in those common policies;
old entry-check functions and call sites were removed. Scripts can read common
jump thresholds, logical jump hold, held-item use category and aerial attack
selection; invoke shared airborne shooting/throwing; supply an authored landing
callback; and query validated recipient rebound policies. Fastfall now has a
strict Bool setter. The four common interrupt operations added in49 now have
generated callback-lifetime checks.

Full Release build and58 grouped compiler/content/common-policy tests pass.
Actual aerial/contact/lifetime units pass424,257 independent source checks:
69,120 original Peach input pairs,34 authored aerial cases,1,509 contact cases
and4 recursive-dispatch/balance cases. Grounded setup's exact source fragment
has10 additional cases (429 total common-entry checks); the entire motion
transition is outside that fixture. Frozen50 pins3,212 inputs. Rook was rebuilt
without source changes, replacing her installed identity with
832dd47fde89fae389ac3cd0b61aacd919a2e77b29d6fa03cc1b828b85b564a8.
Docs: melee/docs/scripting/aerial_actions.md and connected reference pages.
Evidence: .local/sdk50-build-final.log, .local/sdk50-tests-final.log,
.local/aerial-policy50-root-repro/receipt.json,
.local/common-entry50-grounded/receipt.json.

Fresh Ness Workshop clones now use the fully authored independent template.
Root reproduced SDK49 and50 normal clone/import/DLL/package validation (856
ownership checks:31 Definition callbacks,142 original non-NULL fighter phases,
118 article callbacks). The shipped template matches every non-comment
statement of the built50 candidate. Existing saved drafts are preserved; full
gameplay acceptance remains pending. The yo-yo cleanup fix and original bug
explanation are included. Local commit6c854c206 enables the template;
.local/ness-fighter-activation50-root/receipt.json records the gate.

Root also reproduced Peach pluck/counter composition (7,325 source pairs,
191,363 checks,17 separate safety cases), committed as a542643ab, and Link/Young
Link's combined50 matrix (16,690,480 assertions). Link/Young metadata closure
and Peach Float/SideB/Parasol remain in progress. No game was launched; console
checks do not replace player-visible acceptance.


### Link / Young Link independent Workshop activation (2026-10-03)

Fresh clones now select independently scripted Link and Young Link templates.
Copied bases own 53 semantic bindings, their reserved sword-joint omission and
all five/six article assets. Root normal clone/import/DLL/bundle/verify passed
721/776 ownership checks against frozen51; composed source comparisons passed
16,690,480 assertions. Common forward-smash startup uses the existing post-entry
binding hook and retains native angle selection. Shared smash startup must stop
if nested motion/animation callbacks retire or replace the fighter. Source and
package gates do not replace future drawn/CPU/network gameplay acceptance.
Peach Float/Side-B source composition independently reproduced 16,424 pairs,
159,541 checks and 17 authored safety cases; Parasol remains in progress.

### Generic authored held-article protocol (2026-10-03)

Local commit8845a0055 adds paired Int status/Bool request callbacks owned by
the actual held article package, full-width held-instance observations, and
secondary-damage callback binding. Original common query cadence is preserved;
authored refusals do not fall through to stock item handlers. Identity and
continuation guards cover retirement/recycling and changed package/held slots.
Full Release build and62 grouped checks passed. Root separately reproduced
131 compiler,639 generated console and642 loaded-DLL checks. Frozen53 pins
3,216 inputs. Rook source remains unchanged; her refreshed installed package is
f04c752d227045eed0430d7716dd05b8f00543b31709db756b3a42eaa58da7a7.
Evidence: .local/held-protocol53-root-repro/result.json and
.local/sdk53-tests-first.log. Peach shared open/fall entry policy remains next;
these contracts alone do not activate a fully scripted Peach clone.

### Shared held-motion entry and further source batches (2026-10-03)

Local commit2ecca3f08 consolidates the original blend0/blend10 open entries,
adds generic script held-motion selection and validated raw motion.id publication,
and preserves the common physics/timer setup. Root reproduced6,492 original
pairs,2,160 authored mapping cases and76 safety cases (298,515 checks).
Release build and66 grouped checks pass; frozen54 pins3,218 inputs. Rook's
unchanged source is rebuilt as installed package
82ef87334db131bacc02de6ae90e74dab9e7a5a72d043acc8d75f7150d3615dc.
Evidence: .local/held-motion54-root-repro/result.json and .local/sdk54-tests-final.log.

Local commitc24ca05aa records separately reproduced Samus tether fighter/Catch
phases (442,325 checks/8,338 cases) and Game & Watch Chef/food behavior (742,990
checks/38,070 pairs). Their article-construction/rendering boundaries remain
explicitly unaccepted. Neither is activated as a new full independent fighter.
Peach held lifecycle/neutral/resume policy is the next generic contract batch.

### Held lifecycle and further composed source checks (2026-10-03)

Local commit 5c4272e2b adds generic neutral-enter, landing observation, held-timer expiration and airborne
size-change resume policies are implemented. Held articles independently receive
holder-neutral cleanup through their immutable author, including foreign holders.
Declared policies never fall through to donor-specific cleanup. Common entries
stop cached continuations after source retirement/recycling or changed identity.
Release build and 71 grouped checks passed. Root separately reproduced 1,661
healthy original54 entry pairs, 20 authored contracts and 46 safety cases
(41,471 checks), plus compiler/both-parser/loaded-DLL checks. Mushroom caller
propagation compiles but is not claimed as executed source-pair coverage.
Frozen55 pins 3,219 inputs. Unchanged Rook is installed as
5afb63cc6bbb104a3d83c1302aeecc3e1f09a7f624b0062e073c471e48941cb5.
Evidence: .local/held-lifecycle55-root-source/result.json,
.local/held-lifecycle55-root-compiler/result.json, .local/sdk55-tests-final.log.

Root independently reproduced Samus's nine healthy grapple solver states:
13,392 ordered timelines/61,642 phase comparisons, with 1,284 timelines reaching
the native decompiled C's undefined direction-pointer expression classified
separately. Authored code deterministically uses the already-computed direction;
this is not a claim of the same defect in retail PPC. Maximum observed Float
difference is 3.8e-6. Constructor/indirect animation/render assets, modified
grapple mode and full activation remain open.
Evidence: .local/samus-grapple-article53-root/acceptance.json.

Game & Watch Judge/Fire Rescue/Oil Panic source callbacks also independently
passed 45,880 pairs. Constructors and absorption remain drafts pending generic
hit-count, common stick-threshold, live target airborne and ordinary engine food
spawn operations. Paper rendering and full metadata/import acceptance remain
open. Neither partial source namespace activates a fully independent fighter.
Evidence: .local/gamewatch-specials53-root/receipt.json.

### Shared absorption, target state and engine food operations (2026-10-03)

The scripting API now reads absorbed damage and hit count as the actual signed
integer fields, queries the current common horizontal-stick deadzone, observes
a live fighter's airborne state through full-width creation IDs, and invokes
the existing ordinary engine food constructor directly. Food coordinates are
evaluated once in order; source retirement/recycling stops script result stores
and continuations. No Game & Watch-specific native delegate is introduced.
The food source fixture executes the actual constructor and counter increment
over 577 cases/12,197 checks, with availability/center/allocation controlled.
Only initialized spawn fields are observed; model allocation, spawned callbacks,
food variant RNG and rendering are not claimed by this test.
Release build and 74 grouped tests passed. Frozen56 pins 3,219 inputs. Normal
Ness clone/import/script-build also passes after the absorbed-damage type fix.
Unchanged Rook is installed as
c80141642ddd83e456818cb2c85552d9d69bb4abb926f435612e5319faaaae83.
Evidence: .local/engine-food56-root/result.json, .local/sdk56-tests-first.log,
.local/ness56-script-check/build.log. Root also independently reproduced 702
Oil Panic absorb/turn/pickup source pairs and 40 generated food continuation
cases (11,140 checks), including exact integer damage values above 2^24.
The full specials draft generates and compiles, but its new constructor paths
and paper setup remain syntax-only evidence. These operations do not activate
Game & Watch. Evidence: .local/gamewatch-specials-contracts56-root-final/receipt.json
and .local/sdk56-compiler-final-test.log for the strengthened context rejections.

### Composed Peach travel and generic live ownership (2026-10-03)

Local commit44c39b0b6 preserves the composed Peach Parasol/travel source batch.
Root independently reproduced 59,355 original/generated comparisons,870,596
checks and25 authored safety cases with zero measured Float difference. Complete
neutral/landing/resume bodies and the held timer block execute generated policies;
constructor/render assets, recipient lookup and common physics interiors remain
controlled or separately accepted. This is not complete Peach activation.
Evidence: .local/peach-parasol-travel55-root/result.json.

SDK57 adds generic read-only articles.ref(...).current_owner and
targets.smash_charge_active(reference), preserving integer creation IDs and
sampling live ownership/common charge state. Physical D-pad-up and Z are exposed
as fighter input bools. Compiler tests cover fighter/helpers/article paths,
missing Host bindings, readonly rejection and high-bit IDs. Runtime tests cover
removed/foreign/recycled article references, absent owners and charge phases.
Release build and74 grouped checks passed; immutable57 pins3,219 inputs.
Unchanged Rook's matching module is installed as
1ff5141e914950a6c4ef457fcc626204c9cec8a7fd784944ba0a2015265709c0.
Evidence: .local/sdk57-tests-first.log,
.local/rook-stage-item-contracts-sdk-refresh57.json. Game & Watch's remaining
props and Zelda's standalone behavior are in progress; paired-form packaging
and renderer foundations remain separate gates. No game was launched.

### Generic normal attack policies and package ownership (2026-10-03)

SDK58 exposes shared jab/rapid-jab entry and continuation, grounded pickup,
owned aerial-motion mapping and aerial-landing collision/entry. Scripts can read
ordinary landing lags, full fighter generation IDs and the live debug level.
No new fighter-specific primitive or stock handler delegation is introduced.
Release build and77 grouped checks pass. Root independently reproduced324
healthy original/current common-source pairs (8,508 checks) and164 separate
authored/removal/recycle/kind/Definition safety cases. The older jab-restart
continuation hazard is explicitly outside that frozen proof and will be fixed
separately. Evidence: .local/normal-attack58-root-repro/result.json and
.local/sdk58-tests-first.log. Immutable58 pins3,219 inputs. Unchanged Rook is
installed as4ce803dc7c50af77b46278bb7beb1ae7e244a7dad28bdfadc08038b5c1eb5326.

Root also reproduced Zelda's partial Neutral/Din/Explosion composition:
821 source pairs,40,123 checks and6 separate safety cases. It remains
unregistered; paired-form packaging, Side-B fighter phases and teleport remain
pending. Peach's independent module/import/package gate passes11,808 checks,
34 Definition callbacks,114 phases,56 article callbacks and30 real behavior
selection cases. Debug taunt is being added through the generic58 debug query.
Neither gate claims actual model construction/rendered gameplay acceptance.
Evidence: .local/zelda57-root-repro/result.json and
.local/peach-activation57-root/receipt.json. No game was launched.

### World-position effects and callback continuations (2026-10-03)

SDK59 adds generic article joint world-position output and explicit-position
radial wind for fighter/article scripts. Joint queries use the selected primary
or additional HSD graph, preserve child-path semantics, and calculate all three
coordinates once. Missing graphs yield zero outputs. Compiler/module checks
cover types, output alias rejection, Host requirements, ordered single argument
evaluation and absent graphs; production model-runtime checks execute the real
joint lookup and shared world-position calculation. Release build,79 grouped
scripting checks and native_model_runtime pass. Immutable59 pins3,221 inputs.

The older jab-restart continuation sites now validate source generation, kind
and Definition before continuing. Root reproduced384 healthy source pairs,
5,892 checks and84 separate source invalidation cases using frozen58 versus59.
The authored taunt continuation has the same guard; its composed source proof
is being finalized. Evidence: .local/jab-restart59-root-repro/result.json,
.local/sdk59-tests-first.log and.local/sdk59-model-runtime.log. Unchanged Rook
is installed as e5d599363d5c22806f86f376ccf18027cc437e30fa342f7e44f75ab14e6b7fad.

Game & Watch's normal articles/lifecycle source batch is independently reproduced:
53,612 grouped cases,4,164,287 checks, no failures. Constructors/paper material
setup remain explicitly unaccepted; the next batch composes all48 normal phases
with the new generic entry/landing/world-position operations. Evidence:
.local/gamewatch-normals57-root/receipt.json. Zelda Side-B and teleport phases
are now being authored with retained owner IDs; transform pairing remains open.
No game was launched.

### Independent Peach Workshop and roster source batch (2026-10-03)

Fresh Peach Workshop drafts now load examples/peach_full.fms. Its independent
module owns Float/attacks, all specials, forward-smash selection, lifecycle,
identified CPU policies, taunt and five articles. Normal project import and
script-build succeeded; generated C differs from the accepted SDK59 candidate
only in its declared source hash. Original recipes become 151 owned parameters.
Six initial plus ten extended actual hidden/muted gameplay comparisons pass,
covering ground/air/left specials, Float/neutral attack, repeated smash and CPU
movement/pluck/damage. Empty-handed air down-B intentionally remains a no-op.
CPU active traces stop at800 ticks despite1,200 replay samples. Full screenshot,
waveform, results-scene and all interaction branch acceptance remain open.
Evidence: .local/peach-runtime59-installed/,
.local/peach-runtime59-extended/ (preserved bad coverage setups),
.local/peach-runtime59-coverage-corrected/ and.local/peach-published60-project/.
Ledger: docs/peach_script_translation.md. Local commit7eb00fea4.

Root reproduced Game & Watch's normal entries59 batch:49,960 pairs,4,851,141
checks, zero failures. All48phases, entries, setup decisions and actual Manhole
world-wind tuple are authored; constructors/material layers remain controlled.
Root also reproduced Zelda travel58:10,993 pairs,655,214checks,20safety cases.
Side-B and teleport phases are authored; transform pairing remains outside the
standalone batch. Evidence: .local/gamewatch-normal-entries59-root/receipt.json
and.local/zelda-travel58-root-repro/result.json.

The frozen generic layered-render proposal requires typed immutable Article/Item
visibility metadata outside the private state union, actual three-pass reuse,
script-owned colors and constructor ordering. It also identified common RGBA
word-import reversal and unsafe controller-color indexing. A bounded typed
palette repair is underway; release build plus80grouped checks currently pass.
No default Game & Watch or Zelda activation is implied. Continue renderer/load
foundations and standalone Sheik. Goal remains active; no upstream push.
### Typed common palette repair checkpoint (2026-10-03)

Root reproduced the actual importer/query/material batch: 19,001 checks,
17 full common imports, 256 controller-selector cases, 4,608 healthy full
material-body pairs and 1,000 safe invalid-selector cases. Ordered RGBA bytes
are preserved at controller colors and the separate fifth material color;
scalar gameplay data remains separate. Invalid indexing and int/color pointer
aliases are removed at their real callsites. Controlled TEV/overlay sinks prove
material decisions and byte values, not rendered pixels. Local commit45aae5795.
Evidence: .local/common-palette-root/receipt.json and
.local/peach-palette60-build.log; release build and80grouped checks pass.
Immutable60 pins3,222 inputs. Public module layout has no new host entries in
this palette-only change; installed Rook still uses the matching59 contract.

Next: generic ArticleModelLayers data, unified real import decoder, typed
immutable Article/Item bindings outside private state and direct shared renderer
migration. Follow with generic packed-color/presentation scripting and authored
Game & Watch constructors/OnLoad; the full drawn activation remains a separate
acceptance gate. Sheik non-transform source translation is underway. No paired
Zelda/Sheik package UX has been chosen yet. Goal remains active.

### SDK61: typed model layers, owned visibility and generic presentation scripts

Unified article layer decoding and immutable Article/Item binding replace
private-state pointer aliases. Stock constructors use the item's own asset;
primary model replacement invalidates it. Root reproduction passes 4,775 checks
including ten full article imports and 240 draw-dispatch cases. Typed fighter
visibility slots validate costume graphs and publish during the owning fighter's
actual load window; scoped reproduction passes 277 checks.

Generic scripting gains packed RGBA, diffuse/outline and controller colors,
secondary material policy, instance width and owned visibility declarations.
Needle gaps gain previous vertical input, custom floor motion and bounded single
capsule disable. Release build and 87 grouped checks pass, including both generated
callback forms and actual four-costume visibility import. Evidence:
.local/sdk61-build-ready.log and .local/sdk61-tests.log. No GX/pixel/gameplay
acceptance is claimed by these data and callback checks.

Frozen61 verifies 3,226 hashes and source correspondence. C: filled while copying
the archive; the completed archive is preserved at
A:\FreakMelee-test-archives\baseline61 through the .local/baseline61 junction.
The partial copy is preserved/compressed; three recent immutable archives were
compressed without changing contents. Rook was rebuilt for SDK61, behavior source
unchanged, 285 own-rig clips and zero borrowed clips. Installed package:
059e60823536002ba081d782255108b656dded3075e9fecee446abbca8b9e737.

Game & Watch load and Sheik Needle source comparisons follow against61. The
fighter-pose graph proposal is audited; Chain implementation and paired
Zelda/Sheik packaging remain outstanding. Article-local typed helper returns are
a language gap, separate from existing fighter/export typed returns. Goal active.


SDK61 foundation committed locally as968614736. Subsequent Game & Watch load
source comparison is independently reproduced: 216,645 checks across4,608
original/generated pairs pass (512 stock,4,096 edited). Receipt:
A:\FreakMelee-test-archives\gamewatch-load61-root\receipt.json.
Actual OnLoad/attribute copying/material traversal execute; parameter import,
synthetic inherited base, visibility publication and article registration are
explicit fixture boundaries. Full clone/package/gameplay acceptance remains open.
The source-only fighter-pose graph proposal records generic shared primitives and
rig/topology/lifetime requirements; it is not yet an implemented API.


### Sheik Needle and Vanish source composition (2026-10-03)

Root independently reproduced the frozen61 combined module: 10,300 original/generated pairs, 990,738 checks and 18 authored safety cases pass. Needles covers neutral entries, all 32 fighter phases and complete Held/Thrown decisions; unchanged Vanish covers all 24 phases and its article. Shared lifetime, attachment and air-physics bodies execute; allocation, stage responses, motion/animation and rendering remain controlled boundaries. The scripts call no stock Sheik move handler. Evidence: A:/FreakMelee-test-archives/sheik-needle-vanish61-root-final/result.json.

Chain still needs generic owned pose-graph operations and current/previous capsule positions. Transformation, paired package behavior, registered clone/import and gameplay acceptance remain pending. Source-only comparison is not full Sheik activation. Article typed helper returns and creator/owner result expressions are the next generic language batch. Goal remains active.


### SDK62: typed article functions and collision capsule coordinates

Article-local helpers now return Bool/Int/Float values, including direct typed creator/owner helper results. Nested call arguments execute once in written order; actual generated callback tests include protected retired Item memory, recycling and recipient changes. Ordinary eight-argument calls avoid unnecessary temporaries; oversized ordered expressions are rejected without truncation. Both generator routes pass 1,232 runtime checks plus 20 compiler programs.

Generic hitbox.current(index).x/y/z and hitbox.previous(index).x/y/z expose distinct collision endpoints, preserving attachment/offset/victims/activation. Both routes pass 252 cases and 2,286 checks, including complete shared collision update/offset bodies with controlled joint conversion. This closes the capsule API gap, not Chain behavior.

The stock Game & Watch layout was still missing the raw {4,20} RGBA span in61; SDK62 repairs it. Full actual ISO imports now compare all four colors and outline bytes plus neighboring scalar attributes. The earlier owned byte recipes did not establish that stock importer correction. Release build and all93 grouped checks pass (.local/sdk62-build-accepted.log, .local/sdk62-tests-accepted.log). Frozen62 at A:/FreakMelee-test-archives/baseline62 verifies3,227 file hashes and live SDK source correspondence. Existing installed Rook needs no rebuild: this batch adds compiler surfaces and stock import metadata without changing the module ABI.

Next: original/generated Game & Watch constructors against62, then generic owned pose operations for Sheik Chain. Transformation/paired package and full newly supported clone activation remain pending. Goal active; no upstream push.

SDK62 committed locally as ceda0af25; Sheik source composition as3b60fbf7c. Frozen62 manifest SHA256:440b27db0595cc12a730a5e390c6cc9a74d00ed17b7f1ec14117ae6338f9dc4b. Constructor comparison is using that snapshot. The next coordinated batch implements generic pose-graph engine/asset validation and compiler declarations, with disjoint ownership and no shared edits to frozen62. No game process was launched for this checkpoint.

### Game & Watch retail import census (2026-10-03)

The native parameter importer now recognizes the ten retail paper article slots, in the exact order registered by the original OnLoad. Slot 10 remains outline visibility data. Read-only frozen-SDK62/retail-DAT census: 53 physical joints, 38 semantic bindings, no omissions, 323 actions, 14 demos, 40 custom motion states. Receipt: `A:/FreakMelee-test-archives/gamewatch-clone-metadata62/metadata.json`; reader: `melee/tests/native/gamewatch_clone_metadata/audit.py`. Live binding source passed frozen-Clang syntax checking. This prepares ordinary owned-value imports; it does not enable a default Workshop clone or establish full-fighter/gameplay parity. Constructor and full-lifecycle composition remain separate gates.


### SDK63: owned generic pose operations (2026-10-03)

Named pose graphs resolve immutable owned joint descriptors; live/alternate load, request, evaluate, copy and blend use the actual shared Melee operations. Two independent blend weights remain explicit. Registered asset/bank identity, descriptor topology, omissions, raw part starts, clip spans and callback lifetimes are checked before traversal. The descriptor iterator enforces its actual depth limit and a selected costume subtree does not include its ancestor siblings. Bool frame/weight operands reject compilation.

Both compiler routes pass31 programs and3,643 generated-runtime checks each. Actual resolver/shared-body fixture passes1,061 checks; full retail Sheik imports pass822 checks/40 graph-clip-destination combinations across five costumes. Actual semantic rig mapping and part initialization execute; lower pose math/material allocation and authored Chain gameplay remain separate acceptance scopes. Release client/content tool and all98 grouped checks pass: melee/.local/sdk63-build-final.log and sdk63-tests-final.log. Local commit f8e9bb308.

Frozen63 at A:/FreakMelee-test-archives/baseline63 verifies3,231 files and current source/runtime correspondence. Manifest SHA25634bea5f606ccd2560bf183675945bccbb53cd3b3158a4a21bec334c8d2d0a083. Rook's installed module was refreshed for the layout change; all20 immutable project inputs remain byte-identical,285 owned-rig clips and zero borrowed clips. Installed package e6f30bcae18e6b6cee8badfbe4872e3e263acfac3e0d4539e40af37b1f50fa46.

Game & Watch constructor source composition is committed039e536b6 and independently reproduced:123,648 constructor pairs,18,816 presentation-transfer pairs,11 authored safety cases,22,731,448 checks and ten complete retail article imports pass. Root receipt A:/FreakMelee-test-archives/gamewatch-constructor62-root/receipt.json. The complete copied-base/import/normal-clone activation follows next; constructor acceptance alone is not Workshop/gameplay completion.

Next coordinated batch: generic resource point collision distinct from ECB collision, shared stick-smoothing tuning query, complete Sheik Chain decisions, and Game & Watch copied-base/normal Workshop clone registration. No game window was opened in this checkpoint. Goal active; all commits local.


### SDK64: generic point collision and safe synchronous resource retirement

Scripts gain item.resource(index).sweep_point() with the real swept-point kernel, distinct from the existing ECB sweep, plus read-only controls.stick_smoothing from shared tuning. Both collision calls have generated callback lifetime guards. Fixed resource collections become unavailable immediately when retired and keep their collision storage until the outermost synchronous sweep returns; no collision algorithm is changed. Generic foundation committed locally0132df504.

Root reproduced7,168 old/current point-kernel pairs,544,313 checks and27 resource identity/retirement safety cases. Complete history/subdivision/contact/end callback bodies execute. Analytic stage intersection/remap, allocation/publication and model destruction are controlled boundaries, and the older ECB kernel is a separate dispatch check. Evidence A:/FreakMelee-test-archives/resource-point64-root-repro/result.json. Full release build and all98 grouped checks pass: melee/.local/sdk64-build-first.log and sdk64-tests-first.log.

Frozen64 verifies3,234 hashes and current source/runtime correspondence at A:/FreakMelee-test-archives/baseline64; manifest7301e8b4b16357d886bb305689a75e8ec1fe01f518e926c348db83cc714ab333. Identical files are hardlinked only between immutable archives (3,184); changed files remain private copies (50). Live checkout/build inputs are never hardlinked into an archive.

Rook refreshed to6112add84bfedde22a3f8a89e157e4758c6fd80d59aa890171b10fd72eebf2e4. An initial wrapper assertion used a shadowed variable; independent hashes verify all20 immutable project inputs still match the SDK63 refresh. Her authored behavior remains unchanged,285 own-rig clips, zero borrowed clips. Receipt melee/.local/rook-stage-item-contracts-sdk-refresh64.json.

Snapshot64 also includes an explicitly provisional Game & Watch copied-base/template registration candidate for normal import testing; it is not a full accepted Workshop clone. Its real clone/import validates all80 source field recipes; corrected namespace assembly passes generation/DLL ownership993checks over160motions/24Definitioncallbacks/95articlecallbacks. Bundle exposed a missing empty-string costume-matanim metadata sentinel; the candidate fix and proper attribute-reload callback are in the next coordinated batch. Full normal-phase comparison and generic muted-channel policy closure remain outstanding. Do not claim Game & Watch complete from constructor or pointer-ownership checks alone.

Sheik Chain's complete fighter/article draft now parses using the live generic API. Original/generated execution comparison against immutable64 follows; real package/base/transform pairing and drawn gameplay remain separate gates. Goal remains active; no game process or upstream push.


### SDK65: owned sound policy and Game & Watch copied-base packaging (2026-10-03)

Generic immutable `sound_mute_channels` controls all seven fighter channels without stock move delegates, mutable private-union fields or a fighter-specific Host operation. Ordinary bases preserve mask0x44; Samus/Game & Watch preserve0. Scripts may override with a strict decimal/hex literal0..127 in either generator route; omission preserves owned base metadata. Explicit stop/cleanup remains unconditional. Positioned command decoding/mapping and randomized/pitch RNG consumption retain their original order. Documentation: melee/docs/scripting/sound_channels.md.

Root independently reproduced36,960 raw/custom original/current pairs plus22,272 authored mask/stop cases,1,106,907checks,314compiler programs and four actual generated modules. Evidence: A:/FreakMelee-test-archives/sound-policy65-root-final/result.json. Complete dispatch/play/stop/resolver bodies execute; audio backend, mapped IDs, RNG results and active-slot publication are controlled boundaries. Audio mixing/pixels remain separate acceptance.

Combined release build and103grouped checks pass (melee/.local/sdk65-build-batch.log, sdk65-tests-batch.log); the final added compiler/independent-policy anchors also pass the five relevant rebuilt checks (sdk65-sound-tests-final.log). Frozen65 verifies3,236source/runtime/compiler hashes,3,165unchanged links only between immutable archives and71private copies. A:/FreakMelee-test-archives/baseline65 manifestefd4397a8cda373b29abbe365d98e1f66179cfa36135f32d6456a1a90e603d71. Rook refreshed to5ec3ee1ad2d5682e5e43326708e8b73140f397ef4b10e6bab99641e28c0729d3;285own-rig clips/0borrowed and all20immutable author inputs unchanged.

Game & Watch now has the actual copied-base registry, Workshop template and correct empty no-matanim sentinel. Full normal script composition independently matches174,336pairs/15,567,726checks/0failures: A:/FreakMelee-test-archives/gamewatch-normal-phases64-root-fresh/receipt.json. Actual original/generated bodies and lifetime registry execute; shared transition/input/physics/pose and constructor setup remain controlled boundaries. The corrected normal clone/import/DLL/bundle/verify/install path passes all80retail parameter comparisons and995ownership checks over160motion slots/24Definitioncallbacks/95articlecallbacks, including owned0soundmask. The first65 install test put its placeholder portrait outside the project; the corrected fixture uses the normal project portrait contract. Failed logs remain preserved. Accepted receipt: A:/FreakMelee-test-archives/gamewatch-activation65-root-final/receipt.json. Installed only in its isolated test store, package122772c1db94264043ce288f3f9e0c32520e621175277d90a814b6cc0a40474e. Full hidden/muted gameplay probe batch remains in progress; these source/package checks alone do not establish full playability. Generic sound and Game & Watch packaging committed locally54f7478ad.

Sheik Chain source comparisons exposed a native-width union alias: common.x1C is offset32 in native motion vars, while actual Sheik rehitx1C is28 and resumex20 is32. Shared item code now reads the typed Sheik rehit member (local commit recorded separately). This repairs original retail meaning rather than changing an authored move. Frozen64 failure and explicit old-versus-corrected offset/regression evidence are preserved. Chain's complete phase/article/render composition remains underway; source-only solver acceptance is not full Sheik registration. No visible game launch or upstream push; goal active.


### Requested pause checkpoint (2026-10-03)

Goal paused at the user's request. Last verified release checkpoint is local
commit 917e29e83 (SDK65). No SDK66 build, frozen SDK66, installed-character
refresh, or new gameplay acceptance is claimed.

Verified since the previous entry:
- Complete Sheik Chain source composition committed fd0b8a83d. Root reproduction
  passed 58,153 original/generated pairs and 24,077,957 checks with zero float
  delta: A:/FreakMelee-test-archives/sheik-chain-complete64-root. Full Sheik
  registration and paired transformation remain separate work.
- Oil Panic now updates current model groups, matching the stock operation rather
  than saved group selections. Bucket-only source comparison passed 224 pairs /
  8,130 checks; the corrected hidden/muted 550-frame gameplay trace passed and
  the release build succeeded. Receipts are under
  A:/FreakMelee-test-archives/gamewatch-bucket65-root and gamewatch-panic65-fixed.
  Chef/Rescue trace representation differences remain documented; no blanket
  comparison masks or claims of complete gameplay parity were added.
- Storage cleanup reclaimed approximately 23 GB on C:, preserving author inputs,
  current builds, failure captures and referenced baselines. Bulky future outputs
  belong on A:/FreakMelee-test-archives, per TOOLING.md.

Preserved unfinished source batch (not accepted as SDK66): generic direct sound
channel calls, item wall/ceiling queries, floor alignment/reflection, restitution
and stop-on-surface fields, and asynchronous joint effects. The sound compiler
fixtures and CMake integration recipe are present; they have not run. Item API
compiler/runtime checks remain to be added. The Host layout changes require a
coherent build, matching SDK freeze and Rook module refresh before rollout.

Sheik's unregistered owned base and activation census are preserved. Its partial
88-parameter module compiles against frozen65, but Transform still depends on
atomic owned paired-form registration, destination-authored completion and CPU
switch hooks. Ice Climbers census and article composition drafts are preparation,
not a registered clone or accepted paired fighter.

Resume with review/integration of the pending generic batch and its focused
checks, then one coordinated release build/check batch. Preserve the last working
SDK65 binaries until that passes. Continue paired-form architecture and article
composition afterwards. All progress stays local; no upstream push.


### Resumed: SDK66 generic item surfaces, joint effects and sound channels (2026-10-03)

The requested pause was saved as local WIP f358791ad; user resumed before any
new tests. The goal is active. Generic sound.channel(channel, id, volume, pan)
uses the seven existing engine entries directly, with strict Int operands,
full-width IDs, native byte narrowing and lifetime guards between effectful
arguments. Generic article APIs expose wall/ceiling response codes and contact
line publication, floor alignment, wall reflection, read-only restitution,
writable stop-on-surface policy and mode0/3 asynchronous joint effects. No
fighter-specific Host extension or stock move delegate was introduced.

Full release build and all109 grouped checks pass (melee/.local/sdk66-build-accepted.log
and sdk66-tests-batch.log). Sound compiler38 programs and both generated routes
2,102checks each pass. Item compiler40 programs and both generated routes
12,295checks each pass. Two initial fixture mistakes (ordinary Bool/Int coercion
and an invalid legacy item enum spelling) were corrected; failed logs remain.
Root independently reproduced222 actual-source cases /975checks including
contact side effects, floor orientation, reflection and queued/immediate effect
payloads, with11allocation attempts/10released packets/8dispatches:
A:/FreakMelee-test-archives/article-surface-engine65-root/receipt.json. Actual
complete source bodies execute; allocator and effect construction are controlled
sinks. The check does not establish particle pixels or joint retirement before
flush. This is shared API acceptance, not complete Ice Climbers gameplay.

Frozen66 verifies3,240files,3,189links only between immutable archives and51
private copies. A:/FreakMelee-test-archives/baseline66 (original .local junction)
manifest45ea32a9f4a95ef6d6382e148fce38a17615280e60a9ff19029c50feff869acd.
Rook refreshed for the changed Host ABI: all20immutable author inputs unchanged,
285own-rig clips /zero borrowed; installed package
32f059ff09cce3fa0ceb40f4db94cf0f22b442b373c37c799e3c767a83e4a6c9.
Receipt melee/.local/rook-stage-item-contracts-sdk-refresh66.json.

Ice Shot/Blizzard's composed source comparison now proceeds on66. Sheik and
Climbers remain unregistered preparation until their distinct owned paired
lifecycle contracts are implemented. No GUI/audio test or upstream push in this
batch. Current resumable checkpoint: melee/docs/SCRIPT_PORT_CHECKPOINT.md.


Ice Shot/Blizzard source composition independently reproduced by root on66:
2,478 original/generated pairs and394,279checks for each of Popo and Nana,
4,956pairs /788,558checks total, plus18authored safety cases. Receipt:
A:/FreakMelee-test-archives/ice-climbers66-root-repro/result.json.
Complete78original bodies and10shared bodies execute with29owned parameter
values/form; allocation, motion/pose, stage, VFX and audio remain controlled
boundaries. The actual ITEM_ANIM_UPDATE flag is2; the draft's1 was corrected,
with the pre-fix mismatch preserved. This accepts the two standalone components,
not paired creation, follower AI, full Squall/Belay, registration or drawn gameplay.
Next coordinated foundation separates owned family packaging/atomic staging from
alternate transfer and simultaneous member coordination. No stock partner fallback.


### User-requested pause: owned paired foundation / SDK67, 2026-10-03

Release build and 114 grouped CTests pass. Family codec: 6,992 checks. Original
versus guarded alternate transfer independently reproduced: 1,024 healthy pairs,
621,544 checks, 325 fighter lifetime cases, 15 entry checks, 33 retained-item
cases. Actual pickup callback guards retained items before the counter read.
Intrinsic attachment animation internals remain controlled boundaries.
Linked Climbers scalar source comparisons: 4,412 pairs, 119,774 checks,
2 authored safety cases. Paired moves/rope/follower/full cleanup and complete
Sheik/Zelda clones are not accepted yet.

Families carry independent members and use four-family/eight-actor storage,
separate member DLL cache paths, shared package ownership, complete-member
preparation before mapping publication, and unioned sound banks. Flat Workshop
projects remain single-member with explicit paired import rejection. Runtime
orchestration C matrix remains extraction-only WIP; finish before paired
registration acceptance. No new paired gameplay/UI claim.

Frozen67 manifest 4e1c05ce99fe14b6043fb51d76aabe2a845ed482ce68cfede0d3911a5876c408,
3,242 files on A:/FreakMelee-test-archives/baseline67. Rook refreshed without
changing its 20 authored inputs; 285 own-rig clips, zero borrowed. Package:
2a6a2559a6e2faa86bb2ac51c8f57dc366ffc5d21f45b877000e033b4e11d43b.
Continuation/receipts: melee/docs/SCRIPT_PORT_CHECKPOINT.md.
Goal paused as requested; all three agents stopped. Local commits only.
Local pause commit: 0550fcd40 (owned paired packages and guarded form activation).


### Resumed owned pair foundation / SDK68, 2026-10-03

Goal active after user resume. Release build and117 grouped CTests pass. Generic
live member lookup, directional stamped hitlag links, current-link query,
collision history copy, direct airborne publication, spawn/rebirth horizontal
offset, typed target-bone vector sampling and owned CPU pending-action policy
are implemented/documented. Handled travel counter mutation is script-owned;
Ness/Yoshi explicitly increment, while Zelda can return without increment.

Root independently reproduced family orchestration/member identity:109 cases /
93,068 checks on frozen68, and actual registry/collision/bone relations:
291 scenarios /11,564 checks. Actual pending caller/lifetime matrix:141 cases /
1,550 assertions, with unsafe old revision still rejected by access violation.
Controlled import/pose boundaries are retained; this does not validate real
paired asset import, follower startup, rope dynamics or drawn pair gameplay.
Extended shared recursive-link source checks and paired move comparisons follow.

Frozen68 manifest f7e6922ed2e38d19f8aae06dc8b804346c162388eeef1589042fc8b3108c56f2,
3,246 files on A:/FreakMelee-test-archives/baseline68. Rook refreshed to
b22d5c6a52649e747aa90577f25c3d8bbbd9d363bf99ccfe69d7ba54f223207f,
20 authored inputs unchanged /285 own-rig clips /zero borrowed. No GUI/audio
launch or upstream push. Current continuation: melee/docs/SCRIPT_PORT_CHECKPOINT.md.

### Real paired assets and source composition, 2026-10-03

Root independently reproduced both Transform candidates:17,920 original/generated
pairs /24 lifetime cases /four pending decisions /162 Zelda recovery cases.
Linked Ice Climbers candidate phases pass4,240 pairs /288,389 checks /six safety
cases; primary Belay/rope, primary Squall collision/effect transport and early
follower setup remain pending. Shared recursive hitlag checks now cover429
scenarios /13,355 checks with49 direct writers /25 source files audited.

Real two-member Ness/Yoshi asset preflight passes21 console operations using
actual separately rebuilt modules, DAT imports, member bindings and release.
Malformed secondary FMP, invalid DLL and incompatible costume reject cleanly.
The compose-family console command caches a canonical family; it does not
install or expose unfinished paired Workshop authoring. Full drawn gameplay,
actual paired fighter creation and complete lifecycle acceptance remain open.
Receipts and reproduction commands:melee/docs/SCRIPT_PORT_CHECKPOINT.md and
melee/tests/native/fighter_family_assets/README.md. Goal remains active.

### Generic model/CPU queries and rope source gates / SDK69, 2026-10-03

Release build and120 grouped console checks pass. Generic explicit-position
nearest-opponent and same-author primary model local-translation queries are
implemented in fighter/article scripts; actual original shared nearest filters
and tie order are preserved. Root source comparison52,194 checks,44 compiler
programs and1,480 generated query checks pass. Real family asset matrix repeats
with matching SDK69 modules:21 operations pass. Rook rebuilt/reinstalled with
all20 authored inputs unchanged. No game/audio launch or upstream push.

Root also reproduced Ice Climbers rope solvers and render matrices:15,904 pairs,
7,938,446 checks with maximum Float difference0 across the bounded gates. This
is source/controlled-resource evidence, not actualGPU or wholepairedplay.
Primary phases, constructor, follower and full lifecycle remain pending.
Early follower CPU initialization precedes Player role publication; retain
strict MemberId contract and design explicit validated event context.
Continuation and receipts:melee/docs/SCRIPT_PORT_CHECKPOINT.md. Goal active.

### Retained-position query / SDK70, 2026-10-03

Generic `targets.read_position` samples live gameplay position using a full
fighter creation ID in fighter/article scripts. Release build and120 grouped
console checks pass;72 compiler programs /4,220 generated checks /33 actual
registry cases pass. Matching real family asset preflight passes21 operations.
Rook module refreshed with20 authored inputs unchanged. No game/audio launch
or push. Frozen70 and reproduction receipts are in the script-port checkpoint.
Din CPU and expanded primary/follower gates remain pending; goal active.

### Din CPU behavioral source gate, 2026-10-03

Root independently reproduced118,897 checks /3,254 policy pairs with generic
queries and authored release/steer/finish decisions.48 documented Float/double
strict-threshold differences are accepted behavioral limits, not exact parity.
Normal Zelda/Sheik clone/import still rejects missing stock-base registrations;
owned metadata/base/catalog work and whole-pair activation remain pending.
See melee/tests/native/zelda_din_cpu/README.md and script-port checkpoint.

### Early CPU policies / SDK71, 2026-10-03

Generic early prepare/history-hint, platform/drop command and stick-step/limit
policies are implemented; strict family member lookup remains unchanged. The
actual life reset returns success/failure and every direct caller checks it.
Root comparisons execute generated scripts inside actual extracted CPU bodies;
source29,994 plus generated60,842 checks pass. Release build and124 grouped
console checks pass, real family asset preflight21 operations pass. Rook's20
authored inputs unchanged after SDK refresh. No game/audio launch or push.
Ongoing mode6 follower behavior and whole-pair clone/import/activation remain
pending. Frozen71, scope and receipts:melee/docs/SCRIPT_PORT_CHECKPOINT.md.

### Final primary Belay source checkpoint (2026-10-03)

Root reproduced all four gates against frozen68 generated callbacks and frozen66
original bodies: 23,836 pairs /11,352,604 checks /126 explicit safety cases.
The final primary launch selects airborne motion354 and rejects a creation handle
retired inside the authored constructor before binding it. No new Host API.
Receipts under A:/FreakMelee-test-archives: ice-primary68-root-final/result.json,
ice-constructor68-root-final/result.json, ice-rope68-solver-root-final/result.json,
ice-rope68-render-root-final/result.json. Coverage and controlled engine boundaries
are in tests/native/ice_climbers_rope/README-primary.md. Healthy solver/renderer
maximum Float difference remains zero; undefined allocation failures are an authored
safety choice, never a fabricated original parity claim. Full paired activation,
follower AI, remaining primary Squall and real rendered gameplay remain open.


### Owned Zelda/Sheik package checkpoint (2026-10-03)

Both copied retail bases are registered. No new ABI or stock-special delegate.
Full Release build and124 grouped console CTests pass, including original clone
metadata. Root independently reproduced the frozen70 callback/lifecycle census:
1,098 checks /168 phases /38 original lifecycle pairs. Matching live71 normal
clone/import/script-build/bundle/verify succeeds for both independent members and
an AlternateForm family, with876 ownership checks /168 owned phases. Each owns its
parameters, CPU data, rig, costume, article assets and compiled callbacks.
Receipt:A:/FreakMelee-test-archives/zelda-sheik-normal71-census/receipt.json.
Inputs were held during this gate; later SDK72 edits are not covered by it.

Four embedded shared Taro throw clips retain tag33 for correct retargeting through
immutable common rig metadata; they do not borrow another package or fighter
Definition. All262 Zelda/265 Sheik action blobs match their copied retail bytes.
Printed tag totals also count twelve nonempty undecoded demo records: those are
not an animation-payload completeness check. Normal package import/release is
accepted; real fighter creation/transform gameplay and Workshop paired drafts are
still pending. No default template or installed family is claimed.

Raw Bowser/GigaBowser down-throw pose selection now uses the victim's owned
animation family instead of requiring its stock kind. The complete actual selector
passes1,092 checks /214 cases /two source lifetime cases with immutable71 headers
and the explicit modified-body overlay. Receipt:zelda-sheik-selector71-overlay/
receipt.json under the same archive. Downstream throw execution remains controlled.

Custom rendered results currently skip the stock-only result archive table while
custom title/winner UI is present. Stock models load result clips later from ISO;
this is not an inferred custom gameplay crash. Owned editable demo payloads and
custom model result routing remain a general asset feature; singleplayer intro/
ending/video contexts stay deferred. Full commands and scope:
tests/native/zelda_sheik_owned_pair/PIPELINE.md. No game/audio launch or push.


### Generic follower/shared policies / SDK72 (2026-10-04)

Release and131 grouped console CTests pass. Generic CPU mode/output callbacks,
validated delayed input history, live target and bounded floor/island queries
replace common stock-ID dependencies. Root original/generated source proof passes
1,851,892 assertions; costume sound/KO/friction source proof passes115,352 checks.
Popo/Nana metadata bases are registered; complete authored follower behavior and
independent paired activation remain open. Native animation materialization has
actual import and retarget evidence, but is not yet in the normal constructor.
Frozen72 and matching Rook module refresh completed without changing its20 authored
inputs. No game/audio launch or upstream push. Detailed receipts and limitations:
melee/docs/SCRIPT_PORT_CHECKPOINT.md. Goal remains active.

### Normal Ice Climbers asset ownership (2026-10-04)

The normal clone constructor now materializes Nana's shared article table and
missing animation bank entries. Root reproduced both normal clone/bundle/load
paths and independently audited produced assets:268 decoded Nana actions,
207 bank symbols,3 owned articles and76,610 importer checks. Original commands,
flags and blends remain intact; Popo assets remain exact retail. Generic copier
passes193,601 checks with transactional rejection coverage. No live donor assets
are required. Raw behavior inheritance, authored follower completion, full scripted
pair packaging/activation and gameplay remain separate unfinished gates.
Receipts and detailed scope:melee/docs/SCRIPT_PORT_CHECKPOINT.md. No game or push.

### Authored follower source acceptance (2026-10-04)

Root reproduced full Nana follower/Popo capture scripts in both compiler routes:
21,580,064 checks per route, including551,300 eligibility pairs and17 safety
cases, with zero measured Float error. Two generic compiler mappings match the
verified overlay exactly; Release build and131 grouped console CTests pass.
Original Float capture thresholds and double-promoted smoothing are preserved.
The authored source replaces stock follower decisions; stage/planner/lifetime
boundaries in the source fixture remain explicit controlled inputs.
Receipt:A:/FreakMelee-test-archives/ice-climbers-cpu72-root-final/receipt.json.
A hidden/muted accelerated stock match completed1,200 rendered input samples and
all four specials, supplying a real-game comparison baseline. Full source
assembly, normal independent companion packaging and custom pair gameplay remain
the next gates. No upstream push; goal active.

### Scripted Ice Climbers package and rendered acceptance (2026-10-04)

Root independently reproduced the complete CPU-inclusive assembly and normal
independent companion pipeline. Both members own96 custom phases and44 article
callbacks; DLL ownership checks895/918 pass. A generic compound-key article-import
fix resolves Nana's same-name catalog entries correctly. Release build,131 grouped
console CTests and two existing import/compiler regressions pass.

The family was accepted only in an isolated test store. Hidden/muted rendered
stock and custom matches complete the same1,200 input samples; exact recorded
comparison passes2,400 P1/P2 fighter states,31 actions,247 item states and1,200 RNG
states, reaching all four specials. No installed user packages changed.
Receipts:ice-climbers-scripted-pipeline72-root-final,ice-pair-runtime72-stock and
ice-pair-runtime72-custom under A:/FreakMelee-test-archives.
Reproduction:melee/tests/native/ice_climbers_scripted_pipeline/runtime.py.

This bounded batch does not separately trace Nana's full state and contains no
damage/capture/KO/match-end. Family-aware workshop drafts, edited roundtrips and
owned demo/results payloads remain open. Goal active; no upstream push.

### Rendered Zelda/Sheik transform acceptance (2026-10-04)

Both grounded and airborne-entry stock/scripted transform cycles pass exactly:
6400inputs,12800P1/P2states,112actions,1031itemstates,6400RNG. Both transform
directions and all specials are exercised. Current72 modules were rebuilt through
the normal pipeline; stale71 ABI modules were correctly rejected rather than
adapted. Only an isolated A: store accepted the family; windows hidden, audio
muted and drawing retained. Receipts:zelda-sheik-normal72-runtime-worker and
zelda-sheik-runtime72-worker under A:/FreakMelee-test-archives. Reproduction:
melee/tests/native/zelda_sheik_owned_pair/runtime.py. Damage/KO/CPU/other costumes,
owned demo/results and Workshop paired editing remain open. Goal active; no push.

### Family-aware Workshop implementation (2026-10-04)

The approved header member dropdown edits one draft containing Zelda/Sheik or
Popo/Nana. Member assets, scripts, parameters, CPU profiles and code identities
are copied independently; packages retain their real family mode and stable keys.
A completed staged directory is published only after both members exist. Single
projects remain directly represented as single/main; no format migration layer.

Action/animation, model and script editing now retain two fixed member sessions,
including undo and raw invalid text. Successful behavior builds replace only
compiled module/source buffers without closing those sessions. The original
large editor source is split directly into action, animation, script and session
units. The four accepted paired templates ship with the client.

Full Release build and five relevant codec/content/visibility checks pass. Root
reproduced the actual CLI clone/package/bundle/verify/install workflow for both
pairs and a single Zelda package: 39 CLI operations, 35 exact member payload
chunks, unchanged family IDs on roundtrip, and title-only updates leaving exactly
one installed entry while retaining old cached bytes. Receipt:
A:/FreakMelee-test-archives/family-workshop72-root-repro/receipt.json.
Reproducer: tests/native/fighter_family_workshop/roundtrip.py.
All app-data paths and stores are isolated; no user-installed content changes.

Actual hidden/muted client opening exposed recursive Rml dropdown change events,
confirmed by a debugger stack; programmatic member-selector synchronization now
suppresses those events. Zelda's drawn frozen preview opens successfully.
Dormant alternate-form preview activation remains the next gate; current data
editing does not apply Sheik's pose to Zelda as a fallback. A failed model parse
keeps scripts/actions editable with an explicitly unavailable model. Build failure
publishes no family package; the two individual on-disk behavior builds are not
an atomic filesystem transaction. Owned results/demo assets remain separate work.

Goal active; no upstream push.

### Paired Workshop preview and session follow-up (2026-10-04)

Both alternate forms now enter frozen preview through normal fighter creation.
Only the selected owned alternate at the authoring port changes its initial
creation order; Test Character retains the whole family's ordinary ordering.
Member selection occurs before the editor restores that member's pose. Internal
save/build refreshes do not request preview restarts or leave unavailable-status
messages. Unreadable action/model data leaves that member's scripts editable.

Actual source fixtures pass 81 creation/lifetime checks and 69 Rml session checks,
including invalid text, cursor/scroll, per-member undo and unavailable assets.
Hidden/muted drawn client acceptance covers Zelda -> Sheik, compiling both scripts
through the Workshop, whole-family live practice, returning to selected Sheik,
and switching back to Zelda. The GUI-built pair passes normal package validation.
Full Release build and five focused codec/content/visibility CTests pass.
Receipts under A:/FreakMelee-test-archives: workshop-alternate-preview-source,
editor-member-sessions-headless and family-workshop72-ui-final.

This is bounded acceptance, not exhaustive UI testing or a drawn Ice Climbers
editing check. Existing transient font-layout warnings and owned results/demo
payloads remain open. No user-installed content changed; no upstream push.

### Owned demo animations and paired results (2026-10-04)

The real stock-clone constructor now copies results, intro, ending and wait clips
into each member's own animation bank. An independent ISO/FST/DAT oracle verifies
all26 supported stock sources /343 populated clips, preserving original names,
flags, commands, blends and intentionally empty entries. Kirby stays excluded.
Both normal paired package pipelines pass registration with13/13 populated demos
decoded per member. Source provenance is construction-only; no runtime donor.

The generic editor now addresses explicit Action/Demo clip banks. Actual editor,
DAT/FObj/spline tests pass44 checks, including structural growth and shared clip
aliases. Memory undo and pose-paste preserve both fighter and animation buffers.
Disk saves stage the matching pair; animation undo preserves later attribute
edits, while fighter undo preserves current bank addresses. Actual CLI growth,
undo/redo and failure of the second file replacement pass. This is operation-level
rollback, not crash/power-loss atomicity. Existing Workshop selection still uses
Action clips; demo selection remains UI work.

A real match-end failure traced to the ordinary scene reset discarding active
package data/costume pointers. The reset now retains registered owned assets
until explicit retirement. Root reproduces9,224 actual importer/extracted
creation/reset checks, including the previous failing reset and corrected lifetime.
Full build and eight relevant CTests pass.

Hidden/muted rendered1-stock matches now reach inspected winning results as Zelda,
as Sheik after transformation, and with both Popo and Nana. Each uses newly owned
clips from the normal package path. No missing-demo warning or crash; all task
clients closed. This accepts one winning variant per run, not every loser/team/
costume branch or exact visual parity. Original video modes are not activated by
copying their clips. Custom result text font/clipping and original portrait/taunt
texture selection remain presentation follow-up.

Receipts under A:/FreakMelee-test-archives: owned-demos72-cli-root,
owned-demos72-pairs-root, owned-demos72-save-root, fighter-owned-results-reset-root,
and owned-demos72-results-fixed-root. Reproducers are in
melee/tests/native/fighter_owned_demos, fighter_owned_results and
fighter_animation_editor. No user-installed content changed; no upstream push.
Goal active; no all-characters/exhaustive Workshop acceptance claimed.

### Paired Workshop demo-clip editing (2026-10-04)

The existing Animation browser now exposes owned results/other clips alongside
gameplay clips. Selection, curves, key editing, bone key insertion and pose
transfer use explicit Action/Demo bank identities. Each paired member retains
its own clip/frame/filter/history; changing modes does not replace a retained
demo with the gameplay action. Demo preview samples the live owned rig without
entering a gameplay motion or invoking subactions/callbacks, and release restores
the validated gameplay tree before resuming. Bone fields refresh after pose
evaluation, fixing stale startup values following member switches.

Full native build and nine focused CTests pass. Root reproduces 307 actual-source
preview checks, 126 pose-transfer checks and 53 real-Rml selection/session/history/
key checks. A hidden/muted drawn Workshop run shows Zelda Win1 frame 60, Sheik's
own action, restoration of Zelda's clip/frame/inspector, and Demo graph selection.
Receipts: pose-editor-demo-root, workshop-animation-preview-root,
workshop-clips-key-root and owned-demos72-workshop-ui-final under
A:/FreakMelee-test-archives. This is bounded UI/runtime evidence, not exhaustive
animation/costume/results acceptance. No user-installed content changed.

The current source-closure audit identifies 25 independent selected member
templates and a still-partial Samus among 26 supported copied bases. Fox/Doc
presentation-item omissions are a separate focused follow-up; original video
modes remain deferred. See melee/tests/native/fighter_clone_closure/audit.md and
melee/docs/native_character_workshop_progress.md. Goal active; local Git only.

Samus's next implementation gates are source-backed in
melee/docs/samus_clone_closure.md (local commit 105917d8f). Reuse the existing
fighter tether and healthy article solver components. First finish indirect
animation resources and endpoint effects/constructor ownership; then full
capsule initialization and eligible-target search; then owned throw accessories
and the correctly placed throw callback. Load/death resets and the own-rig map
are composition tasks. Keep the template inherited until this complete closure
passes; do not introduce fighter-specific scripting shortcuts.

### Fox/Doc authored presentation closure (2026-10-04)

Fox now scripts presentation gun creation in common modes 0/1/5/6; Falco retains
the original absence. Doc retains mode-1 result pills and scripts Sleep's mode-2
pill. Initialization, pickup, physics refresh and caller scaling preserve source
ordering. A real comparison failure corrected Fox presentation scaling to follow
the original creator when current ownership changes, using existing generic
creator properties. No new fighter-specific Host operation was introduced.

Root reproduces 10,700 exact source/generated comparisons, 43 safety cases and
811,442 checks, with fresh frozen-tool clones/imports and full generated DLLs.
All 45 extracted producer/API bodies per member match live source. Controlled
model/animation/allocator boundaries and undefined original NULL-scaling cases
are explicit in melee/tests/native/fighter_demo_presentation/README.md.
Full native build/current-tool parameter import pass; shipped templates match
source hashes. Receipt: A:/FreakMelee-test-archives/fighter-demo-presentation-root.
Rendered result-item parity is still unverified. Samus and original video modes
remain open/deferred respectively; no broader roster completion claim. Goal
active; local Git only, no user-installed content changed.

### Samus generic construction primitives and restart pause (2026-10-04)

Implemented typed direct/indexed animation components, primary-model subtree
animation selection with Bool Add results, and resource-GObj position/facing
effects with verified payload families and lifetime guards. Full native build
and14 focused CTests pass. Actual-disc import matches8 indexed animation graphs
and4 named roots against the typed stock importer;805-DAT audit proves7 external
channels are retail NULL bindings. Root reproduces196 model-routing checks,
29 effect compiler cases,881 generated checks per route and3,276 backend checks.
Receipts and controlled boundaries are in melee/docs/native_character_workshop_progress.md.

Samus is still inherited/partial. The constructor draft is isolated under
melee/tests/native/samus_grapple_constructor; normal clone/import passed, script
creation still fails on new callback indentation. No production activation or
complete Gate1 acceptance. Resume from its README before the capsule/search and
throw/lifecycle gates. User requested goal and agent pause to restart the app;
local commits only, no game launched or installed content changed this batch.

### Direct interpreter frontend checkpoint (2026-10-07)

Implementation has started in native_script_source, native_script_expression,
and native_script_statement. Direct source hierarchy covers all27 checked-in
examples; typed expression/statement IR handles semantic selectors, numeric
conversions, scoped locals, control flow and invalidation without generated C.
Client/tool build and seven focused CTests pass. This is frontend acceptance,
not an executable program loader or completed Workshop migration.

melee/docs/native_script_frontend.md records the precise checked scope and next
metadata/program, binding, bytecode, scheduler and package gates. The selected
binding census source-checks63 used contracts and leaves660 signatures explicit
unresolved. melee/docs/native_match_snapshot_inventory.md identifies actual
mutable owners, draw-side writes and object/free-list resurrection requirements.
No explicit snapshot serializer, sub1MB size or sub100us save/load is proven.
Keep the broad tracker as an oracle during migration; goal remains active.


### Typed script/item/animation owners and activation audit (2026-10-07)

Direct interpreted Fighter snapshots now pack declared variables, callback/motion
IDs, clocks and generation/program identities; peer GObj/Fighter ownership must
be restored first. Native AObj/FObj constructors/free paths now use typed stable
pools at the actual HSD heap lifetime, retaining the original animation evaluator.
Their packed serializer includes decoder/timing state, generations/reuse order
and end counters, with complete candidate validation before mutation. Broad
capture still owns external graph, item/stage and remaining global state.

Both packed overlays reproduce all 64 live eight-fighter rollback windows.
Script diagnostic owner peak 656 bytes; animation owner peak 335056 bytes. These
are partial components, not a complete match-state size. Full rollback still
averages 179.12 ms with 64 deadline misses. Actual one-stock VS to Results
cleanup/construction also completes with zero census errors. Details/evidence:
melee/docs/native_fighter_snapshot_ownership.md and
melee/docs/native_animation_snapshot_ownership.md.

Generic typed Item-self physics/lifetime/flags/audio pass 10966 source-backed
checks. The client/tool and 33 focused CTests pass. Ordinary Workshop/package
apply still uses the existing backend and is not fully migrated. The source
package transaction and dependency-grouped remaining work are documented in
melee/docs/native_source_package_loading_audit.md and
melee/docs/native_script_remaining_binding_closures.md. Next prioritize entity
metadata/assets, spawn, owned storage/snapshots, actual Item scheduler, retained
owner/creator/holder contexts, attachment/held behavior and retirement as one
coherent suite, then Mario effect/CPU closures and Rook holdable/common input
behavior. No fighter-specific shortcuts or legacy Host adapters. Goal remains
active; local Git only, no multi-PC acceptance claim or UI redesign.

### Connected interpreted entities (2026-10-07)

Direct programs now prepare and publish immutable Article/Item schemas, typed
parameters, animation states and recipes. Actual Item constructors/schedulers
dispatch authored bytecode with owned instance storage/snapshots and retained
creator, attached-owner, current-owner and holder identities. Shared holdable
pickup/throw/collision behavior preserves native contracts. Deferred retirement
and nested script faults stop further native setup. Metadata-only entities have
zero-byte owners without dummy fields; duplicate recipe settings are rejected.

Seven factory routes pass 24 cases/3149 checks, including 1024 actual native
constructor/free cycles. Metadata preparation passes 42 cases/247 checks; the
current Item owner fixture passes 32 cases/1193 checks; the empty-owner/holdable
fixture passes 5802 checks and 1024 native comparisons. The connected source
binding/factory suite passes 17 cases/135 checks, including arbitrary custom
Article names and logical held-item retirement. Controlled asset/model/
platform leaves and remaining acceptance are explicit in the corresponding
melee/docs/native_script_item_* documents. The client/content tool rebuild and
32 focused CTests pass. A hidden/muted eight-fighter 2000-input scene regression
has no script faults or lifecycle census errors; it is a common/lifecycle script
regression, not full Mario/Rook or source-item rendering acceptance.

melee/docs/native_script_entity_checkpoint.md records the implementation and
evidence. Ordinary Workshop/package loading remains unmigrated, including
transactional holdable texture preparation. Next close the remaining effect/
Fighter contracts and source asset/program publication so complete Mario/Rook
can run without generated modules, then remove the obsolete ordinary backend.
Full aggregate snapshots and their performance target remain open. Goal active;
local commits only; no UI redesign or multi-PC acceptance claim.

### Complete Mario source preparation and effect/Fighter closures (2026-10-07)

The entire current Mario source (77 functions) now compiles directly. Generic
CPU/control/collision/reflector/visibility/effect primitives have source-backed
comparisons; the specialized root-rotation callback convenience became an
authored helper bound through generic callback IDs. Typed Fighter event dispatch
preserves item flags and CPU Bool results separately from VM status. Attached
source actors do not fall into donor callbacks when a hook is absent. The neutral
special cue now consumes its actual one-shot subaction flag instead of executing
the article spawn every accessory tick. Packed callback identity includes this
trigger and reentrant actor slots remain pinned until invocation unwind.

Interpreted model effects now own code-ID updates, retained creator views and
component snapshots at actual EF update/cleanup sites. Native Tornado comparisons,
two-member groups, retirement, resurrection and 1024 lifecycle cycles pass 5201
checks/25 cases. Fighter scheduler/ABI checks pass 145; generic CPU/action,
collision and feedback comparisons pass 2289,1163,668 respectively. These have
explicit controlled native asset/geometry/graphics leaves, not complete live
Mario gameplay or whole-snapshot acceptance.

Data-only source asset records now copy owned rig/motion/article/audio facts
without authored addresses or donor behavior IDs. Real independent Mario DAT,
costume and animation graphs import into a stable candidate; no donor behavior
lookup is used at candidate preparation. The full source candidate validates
31 root and four Article attribute imports, two exact sidecar overrides and
two Item/one effect programs before any publication. The actual-disc test passes
1536 checks/25 cases, including failure isolation, actual attribute extent/raw-byte
schema checks, moved-owner rejection and selected-bank/lease release protection.
Its compile arena reserves 14175808 bytes while active buffers use 276722;
1741 instructions need at most 19 call registers. These are load-time capacities,
not mutable match snapshot size. Assets/previews/rollback history must retain
explicit leases until saved references drain.

Documentation: melee/docs/native_source_package_candidate.md,
native_source_asset_records.md, native_script_effect_ownership.md,
native_script_fighter_events.md and native_script_mario_binding_contracts.md.
Ordinary package-format/project/editor/runtime activation still uses the previous
backend; preparation alone does not finish that migration. Next publish the
prepared engine tables/program together through the actual preview/client path,
add owned icon resources and paired-family composition, and remove the obsolete
ordinary generated-module route after its call sites migrate. Full aggregate
snapshot and speed acceptance remain open. Goal active; local Git only.

### Ordinary Single source package integration (2026-10-07)

Normal Workshop/content packages now carry owned facts/DATs/FMS and run through
our typed interpreter, with no generated C or character DLL in the client/tool.
Generated C survives only in a separate behavioral-reference test executable.
Real source authoring and the actual Workshop hidden-process helper pass their
transaction checks. Actual runtime publication/retirement passes 43 checks,
including failure isolation, external leases and retained Windows page history.
Four source-only Mario clones completed 2,000 samples in a hidden muted match
without VM faults or nonfinite traced state; cape Articles were exercised.

See melee/docs/native_source_single_integration_checkpoint.md for receipts and
limits. This does not complete paired forms/companions, Rook's owned item icon
resources, arbitrary source resource graphs, interactive Workshop preview
acceptance or stock parity checks. Explicit mutable snapshots and rollback timing
remain unfinished; the broad tracker is still the oracle. Continue with those
source ownership/resource closures and preserve the approved UI layout.

### Owned Item resources and Rook source preflight (2026-10-07)

Source packages now carry canonical owned PNG/blob bundles and imported Item
blueprints. Visuals decode before activation, retain exact per-declaration
identity, and drain with their package. Generic atomic resource authoring and
facts export are available. A private copy of Rook's actual DATs now prepares
all51 script functions, nine recipes and eightPNG icons with an owned Capsule
blueprint; original user files remain unchanged. See
melee/docs/native_source_resource_integration_checkpoint.md for evidence/limits.

Rook is not yet accepted in gameplay: state341 still needs its four shared jump
phases and source entry/control closure (native_rook_jump_closure.md). Preserve
actual one-airjump values and unusual turn/count data; do not substitute basic
jump or silently normalize fields. Continue that generic scripting closure, then
verify actual Item drawing/throwing and Rook before replacing her installed copy.
Paired source families and explicit mutable rollback snapshots remain pending.

### Rook source jump and Item gameplay checkpoint (2026-10-07)

Rook now authors the air-jump entry and all state341 phases using generic source
bindings. Her actual one-air-jump values, animation295 and common drift remain
intact; raw turn/count words are not normalized. Shared jump begin/full input and
walljump paths guard actor retirement/fault. Animation-driven jump mode rejects
before mutation until its source physics policy is implemented.

The full private source Rook package prepares57 functions/five imports and runs
600 jump samples plus1900 snack samples in the hidden/muted client with no VM
faults or nonfinite traced Fighter state. The jump enters341/295 exactly once;
the snack sequence exercises held/thrown/resting Items and an actual damage hit.
Generic jump comparisons pass121 checks, typed catalog39 and exact nine-recipe
Item corpus997 checks/65 cases. The client/tool rebuild and35 selected focused
CTests pass. Evidence/limits: melee/docs/native_rook_interpreter_checkpoint.md.

Framebuffer images of the small held snack remain visually ambiguous; do not
count billboard rendering as accepted yet. Original user project/installation
has not been replaced. Continue the visual gate, deliberate source-project
transition, paired source families and explicit mutable rollback snapshots.
Goal remains active; local Git only.

### Direct source family authoring restored (2026-10-07)

The canonical FMSC packet and family project paths now preserve two independent
source members for alternate forms/companions. One direct family builder prepares
all members before output; the previous Single-only builder was removed and its
Workshop/CLI/install callers updated. Per-member source checks remain independent.
Actual project/payload/key roundtrips pass106 checks in all three modes, including
two raw stock asset pairs. Package/verify/check CLI cases pass; invalid second
source produces no new cache output. Packet transport passes5877 checks/26 cases,
and the client/tool rebuild. See melee/docs/native_source_family_authoring_checkpoint.md.

Runtime paired activation remains explicitly unsupported until both immutable
member banks publish atomically and source member/recipient identities, form
events, companion CPU/history and rope resources are connected. These authoring
checks do not claim actual Zelda/Sheik or Popo/Nana scripted gameplay. Continue
those direct runtime closures and complete mutable rollback snapshots; goal
remains active and commits local only.

### Direct alternate-family runtime infrastructure (2026-10-08)

Alternate source families now prepare/pin/publish both independent member banks
and imported tables before Player mapping, requiring both form_activated events.
The shared transform routine moved into ftformtransfer.c without a forwarding
wrapper, adds exact source program/storage/lifetime checks and invokes the
destination code ID directly. forms.activate_partner is a generic Void primitive;
framework rejection/fault does not become authored success. Ordinary dormancy
preserves storage; a destination already executing a script cannot be reset.

Actual imported-assets publication/lease/retirement checks pass69, Single Mario
regression43 and actual interpreter actor/snapshot fixture254. Scoped common
transfer comparisons pass682021 across1024 pairs/625 lifetime cases/23 preconditions/
45 Item cases; source view/controller/finish and graphics/status/attachment leaves
are controlled. Binding checks pass57. Client/tool build and35 focused CTests
pass. Evidence: melee/docs/native_source_form_runtime_checkpoint.md. User projects
and installs are unchanged; no actual paired game or rollback acceptance claimed.

Next connect mutable accessory helper IDs and snapshots, complete both source
templates/entities/primitives, and exercise actual rendered form changes. Companion
CPU/history/member helper/rope contracts and explicit mutable snapshots remain
open; companion runtime stays disabled until its contracts are connected.
Goal remains active; local commits only.



### Mutable interpreted motion helpers (2026-10-08)

All five motion phases now accept root-owned zero-argument Void script helpers
through generic callbacks methods; accessory exposes explicit every-frame/cue
policies. Compiler CodeRef arguments require a helper name from the current
program or None, rejecting foreign/computed callable IDs. Motion changes reset
bindings; native assignments supersede them; self-clear/rebind and persistent
NewLife state preserve the native scheduling contract. Packed snapshots carry
helper IDs/cue policy and validate exact program/schema/ABI before restoration.
The old clear-only entry was removed and its call sites updated directly.

Binding/ownership checks pass94, actual source/VM/accessory scheduler checks2404
and actor/snapshot checks307. Client/tool build and35 focused CTests pass.
Assets/presentation/native movement leaves are controlled in these fixtures; no
rendered paired game or aggregate rollback proof is claimed. Details:
melee/docs/native_script_motion_callback_checkpoint.md and the linked scripting
reference. Full form templates still fail direct preflight on obsolete base
metadata; their raw-joint effects, position wind and rate/flags contracts are
audited in native_source_form_primitive_closures.md for the next coherent slice.
Complete source templates/entities, companion CPU/history and full explicit
mutable rollback state/performance remain open. Goal active, local commits only.

### Paused at user request (2026-10-08)

Last verified local checkpoint: 525927f6d (motion helper callbacks and snapshots).
The next generic primitive batch is UNCOMMITTED and not accepted as an integrated
build/gameplay checkpoint. Source files currently add raw-joint/copy-position
feedback and rate/flags motion entry, plus native effect vararg ABI classification.
Canonical form templates have obsolete base declarations removed and entry calls
rewritten; Mario's fireball effect now supplies its required facing scalar through
a generic typed spawn method. No user project or installation was updated.

Focused receipts are available: script-form-primitive-catalog-oct08-scalar (56
catalog checks) and source-form-transition-final (83771 checks, controlled native
motion/publication leaves). Feedback implementation/tests and integrated client/
template checks remain unfinished. The feedback worker was interrupted for this
pause; other workers had completed and are stopped. No task-owned Melee process
was running at the pause audit.

Resume by reviewing the uncommitted feedback classifier and source guard work,
finishing its actual-signature/lifetime comparisons, rebuilding the full client/
content tool, and verifying Mario's typed scalar effect before committing this
batch. Full form source checks currently reveal Article asset metadata in Zelda
and pose_graph metadata in Sheik; those are still open, not gameplay acceptance.

### Generic form feedback and playback (2026-10-08)

Raw-joint effects, typed joint+Float effects, copied-position wind and flags/frame/
rate-preserving entry are implemented as generic operations. Native effect ABI
classification admits only source-verified signatures; the missing facing scalar
in Mario's fireball visual was corrected without substituting an effect. Canonical
form sources use owned layout facts and rewritten entry/spawn calls. Import errors
now identify the specific missing entity field rather than a compound diagnostic.

Focused feedback checks pass66735/61cases, existing feedback848, transition83771
and catalog56, with their controlled native/publication boundaries documented.
The updated complete Mario package builds/verifies in a private cache and runs
1900 hidden/muted input samples with fireball Articles and no script fault. The
client/tool build and35 selected CTests pass. Details and limits:
melee/docs/native_source_form_primitive_checkpoint.md. User drafts/installs unchanged.

Full form source preflight remains incomplete: Din's model descriptor has no
imported root joint, and Sheik's pose_graph is unsupported in the direct schema.
Next diagnose the actual stock asset/constructor contract and connect owned pose/
entity resources; do not invent dummy models or bypass validation to declare parity.
Paired gameplay, companion CPU/history and complete mutable rollback snapshots/
timing remain open. Goal active; local Git commits only.

### Owned Article and pose contract audit (2026-10-08)

Actual retail Zelda DAT import proves Din's Fire/explosion intentionally have
model descriptors with NULL mesh roots, zero dynamic bones and no dynamics.
The shared Item constructor already allocates their runtime root anchors.
Removed the extra script metadata root-joint requirement; required descriptor
and common-attribute checks remain. No dummy meshes or importer changes.
Retail import probe passes18; entity preparation passes283/45cases; native client
and content tool rebuild. Full Zelda source now reaches the unsupported
callbacks.track_reflect_hit binding at103; full Sheik still stops at pose_graph60.

Sheik's exact owned graph rows/paths are documented, with a coherent next batch
for typed program-owned selectors and the eight shared pose operations. Do not
accept metadata alone without execution support. Evidence/limits:
melee/docs/native_source_form_primitive_checkpoint.md and its linked audits.
No complete form gameplay, user draft migration or new snapshot performance
acceptance is claimed. Goal active; continue generic direct-script integration.

### Direct owned pose operations (2026-10-08)

Implemented the coherent generic pose surface: exact owned graph declarations,
fixed prepared row selectors, closed destination/graph types and all eight shared
Melee pose operations through the VM. Independent finite weights and wide integral
indices are preserved; source-only binding rules prevent foreign graph mixing.
Native calls validate owned topology and recheck physical/logical actor, package,
storage and both skeleton heads. Package/Fighter/Item metadata passes compose the
same prepared graph table. No generated-C path or stock fighter handler added.

Settled checks pass237/57 metadata attempts,139 selector/VM cases and1858/208 native
routing cases including182 retirement mutations. Ordinary source-check accepts
all8 operations with real Sheik owned assets (54instructions, original inputs
unchanged). Unified client/tool/affected fixture builds and36 selected CTests pass.
Native routing fixture controls pose math/topology/asset views; no live Chain or
full form parity is claimed. Details: melee/docs/native_source_pose_checkpoint.md.

Full Zelda still requires callbacks.track_reflect_hit at103; full Sheik now reaches
entity metadata at1543. Continue generic callback/entity bindings and complete
form source integration. Companion behavior and complete mutable rollback state/
timing remain open. Goal active; user installs unchanged, local commits only.

### Combat callback closure and complete form inventory (2026-10-08)

Four generic interpreted callback slots now cover reflector contact, completed
reflection turns, body contact and shield contact at their original native sites.
Helper/event binding, native takeover, reset/NewLife and packed snapshot checks
use the existing actor system. Recreating a reflector clears its previous script
contact callback. Zelda's semantic-bone effect calls retain the original active
flag while using the direct API. No fighter-specific shortcut or native fallback.

Scheduler/actor checks pass2476; shared feedback comparisons976. Client/tool and
affected fixtures build, all36 selected CTests pass. Complete-source admission is
still open: Zelda now reaches Article signal at145; Sheik resources at1543.
Details/limits: melee/docs/native_source_contact_callbacks_checkpoint.md.

A systematic production-lexer/current-catalog inventory covers151 Zelda and225
Sheik native paths (zero unparsed executable lines), distinguishing safe rewrites
from genuinely missing APIs. The Chain audit's actual importer probe passes12:
Link100/Tip104 both alias one already-owned graph. Do not create dummy Definitions
or accept resource metadata without the connected model/collision/lifetime and
matrix surfaces. Its source matrix basis is currently transposed relative to the
native helper; correct it with the connected matrix comparison when that surface
lands. Goal active; continue the documented generic dependency groups. No user
drafts/installs changed; local Git commits only.

### Shared fighter interpreter primitives (2026-10-08)

Implemented the inventory's common Fighter group: threshold facing, native
force-air/jump-use and movement-to-ground routines, soft-platform query, visibility
groups/color priority and common model/collision/control observations. Guarded
native execution preserves authored decisions and exact shared function ordering.
Indices retain integral width/sign; owned lookup/variant/table bounds precede
mutation. Facing sign matches the native zero/-zero/NaN convention.

Canonical form sources use the matching direct properties/cues/landing/ledge and
retained model-scale surfaces. The false Item stage-collision branch remains
distinct and unsupported, not silently replaced. Actual owned-data source-check
accepts the common helper surface for both forms and the creator query within
their Article contexts. Full sources still require Zelda Article signals and
Sheik resources.

Checks: movement1916/229cases,122 native comparisons/100 identity mutations;
visuals7430/1701 native comparisons; typed catalog179; property suite32829 with
9406 observations. Controlled boundaries and limits are recorded in
melee/docs/native_source_common_fighter_checkpoint.md. Unified client/tool/fixture
builds and36 selected CTests pass. No game, public draft or installed-content
update. Continue generic factory/recipient/feedback and complete Chain resource
closure, then whole-form runtime acceptance. Goal active; local commits only.

### Feedback payloads and pure callback identity (2026-10-08)

Direct APIs now support copied-position Fighter effects, actual-root Article
effects and shared Article effect cleanup. Full integral IDs and exact native
payload signatures precede mutation; source/Item program/storage/root guards
preserve retirement behavior. No creator is needed for cleanup. Generic pure
callbacks.matches with named Callback slots compares owned code IDs/None while
leaving replacement handlers intact; source-only admission avoids foreign-bank
helper mixing. Sheik's cleanup uses the generic query.

Classifier94Joint/32Scalar/63Position: all65536 IDs plus30 helper-copy cases pass
131268checks. Position dispatch67432/137cases with52 mutations; callback actor
checks2511; Article VM/pool282checks/13spawns; typed catalog93. Boundaries:
melee/docs/native_source_feedback_query_checkpoint.md. Current generator hook
audit preserves native post-create root-scale order; arbitrary future hooks and
per-JObj allocator-generation detection are not claimed.

Client/tool and affected fixtures build;36 selected CTests pass. Whole forms still
require Zelda's Article signal at145 and Sheik resources at1543. No game/installed
content update. Continue connected factory/reference/recipient signals and complete
Chain resource ownership, then full runtime acceptance. Goal active; local commits.

### Direct Article factories and indexed signals (2026-10-08)

The interpreter now loads positional Article factory declarations and signal
names into exact owned identities. Six-argument positional and eleven-argument
velocity creation preserve the native current/previous-position distinction,
full collision-group word, constructor defaults and callback ordering. The
origin-at-owner Bool selects the item center; it never meant grounded spawning.
Original creator is captured before Spawned for source Item children, separately
from reflected damage ownership. Public Item-source factory binding is still open.

Fighter-target articles.signal now names its Article declaration explicitly;
Article item.signal routes self callbacks. Source names never hash/parse during
ticks. Static signal/helper graph bounds and actual actor/VM lifetime checks
preserve faults, removal and output behavior. Article-to-Article target delivery,
retained Article refs/state/translation and ownership mutation remain next.

Checks: catalog71; connected signals99; positional bindings153/19cases; constructor10566/
26cases with1024 reuse cycles. Client/content-tool and36 selected CTests pass.
Boundaries and receipts: melee/docs/native_source_article_factory_signal_checkpoint.md.
Actual private-owned-data source preflight now stops at Zelda's Article ref196;
Sheik remains at Chain resources1543. Neither complete form is marked accepted.
No game, installed content or user draft migration. Continue refs/child ownership,
Chain resource closure and full paired runtime acceptance. Goal active; local only.

### Retained read-only Article references (2026-10-08)

Direct source articles.ref(handle,DeclaredArticle) now supports existence,
current damage-owner ID and declared scalar/indexed state reads from Fighter
and Article contexts. The literal declaration and field resolve at load; one
physical Item is retained before helper-produced indices execute. Missing refs
and invalid integer indices preserve typed-zero defaults. Captured retirement,
userdata/program/schema changes and physical reuse cannot retarget a read.
Fields are read-only; bare arrays are namespaces, never fabricated Objects.
Capture is logically pure and accepted in readonly predicate callbacks.

Primary model translation queries use the engine-typed JObj's local translation
and publish all three F32 outputs only on success. Current owner reads preserve
reflection and full32-bit Fighter IDs. No borrowed native Definition or generated
callback is needed. Receiver frames also pin caller userdata and storage owner.

Checks: readonly catalog93; receiver/core4352 with9 execution runs; connected refs84; client/tool
and36 selected CTests pass. Connected runtime evidence and controlled boundaries
are recorded in melee/docs/native_source_article_reference_checkpoint.md.
Actual private-owned-data source-check now reaches Zelda vector angle at287;
Sheik remains at Chain metadata1543. Full forms are still incomplete. Continue
generic vector math and owned child/lifecycle APIs, then Chain resource closure
and paired runtime acceptance. No game, user draft or installed-content update.
Goal remains active; local commits only.

### Shared native vector family (2026-10-08)

The interpreter exposes XY/3D angle and normalized cross components through the
actual shared Melee/Dolphin routines, without finite/epsilon/direction substitutes.
XY is four arguments consistently in Fighter and Article contexts; existing
Samus/test source and the test-only oracle parser were adjusted directly.
Numerical/typed VM suite7764 checks includes2560 bit-exact native comparisons.
Client/content tool,36 selected CTests and3 rebuilt oracle checks pass.

Zelda's owned-data source-check now reaches CPU target-management380; Sheik
remains at Chain resources1543. Details: melee/docs/native_source_vector_checkpoint.md.
Lifecycle audit identified distinct immutable original creator, mutable damage
owner and revocable script attachment/control relation. The latter needs real
owned state/full physical generation and snapshot coverage, with explicit source
rewrites where old creator access was revoked by detach. This audit is not an
implemented lifecycle milestone; no borrowed Definition/union fallback will be
used. See melee/docs/native_source_article_lifecycle_audit.md. Continue ownership/
child APIs and CPU queries, then Chain/full paired runtime acceptance. Goal active.

### Source Item relationship state and lifecycle (2026-10-08)

Source Item actors now own immutable original creator and mutable attachment
relations, with full physical pool/scene/native identity and userdata checks.
Actor snapshots save and restore both, including legitimate stale relations.
Constructors initialize original provenance before Attach/Spawned and copy a
parent's cleared attachment separately. Source factories no longer write legacy
union attachment pointers/creator IDs or recapture original identity after callbacks.

Generic clear-current, clear-attachment, detach, raw-part attach (optional auxiliary
registration), and original-creator auxiliary binding now execute in the interpreter.
Native pickup dispatch and post-callback lifetime checks are preserved. Existing
owner_bound remains a separate real engine flag. The new attachment_owner receiver
permits revocable control while creator remains immutable provenance. Current
template migration covers ZeldaDin and SheikChain, with explicit old zero/no-op
defaults and typed helper calls; other audited roster sites remain open follow-ups.

Evidence: catalog111; connected321 checks/16 native pickup comparisons/42 pickups;
actor snapshotCTest1206/33cases; actual constructor11624/27cases/1024 reuse cycles;
source factory bindings153/19cases; receiver/core4352. Client/tool and36 selected
CTests pass. Controlled boundaries: melee/docs/native_source_article_lifecycle_checkpoint.md.
No game, installed content or user draft update. CPU queries, remaining Item
operations/child binding, complete roster migration and Chain resources still
require work; full forms and final gameplay/rollback acceptance are not achieved.
Goal active; commit coherent batches locally only.

### Shared CPU command/target closure and roster attachment follow-up (2026-10-08)

The interpreter now reads/writes actual pending/behavior fields, reads previous
behavior/destination, clears the planner target and appends audited commands to
its real256-byte buffer.51 named native recipes retain exact arity/int-to-byte
encoding, with whole-command capacity checks before native append. Nearest enemy
uses the actual3D native selector/filter/list-order algorithm and returns a U32
ID; nonfinite inputs and tie-first behavior are preserved. Shared target position,
airborne and identity queries capture physical Fighters once, support foreign
targets and publish position outputs together only while identities stay valid.

Evidence: CPU7249 checks/612 queue-byte pairs/80 native selector comparisons;
constants6569/1041 entries; connected target475/64 native reference tuples.
Client/content tool and36 selected CTests pass. Exact controlled boundaries:
melee/docs/native_source_cpu_target_checkpoint.md. Zelda owned-data source-check
now reaches Article786 (past CPU/target calls); Sheik remains at Chain1543.

Eight remaining roster templates/twelve Article blocks now use revocable control
where appropriate, preserving explicit0/-1/1/no-op defaults and appearance
provenance.47 helper sites are typed and existing loop/state/signal structure
unchanged. Source audit: melee/docs/native_source_attachment_roster_audit.md.
This is not complete roster runtime/alias/primitive acceptance. No game/user draft/
installed update. Continue generic Item initialization and child/physics/collision,
Chain ownership and full paired/roster runtime checks. Goal active; local only.

### Source Article model/capsule/child controls (2026-10-08)

The interpreter now has prepared primary-joint visibility/rotation/scale controls,
native uniform scale and animation-frame updates; bounded selector storage lives
in an immutable aggregate pool. Native Item capsules and stage-contact fields
have generic indexed script operations. Item-source positional and child factories
retain separate original creator, attachment control and explicit damage ownership,
with native Spawned/Created ordering and full-word groups/backlinks.

Child evidence: 16826 checks/27 cases/1024 reuse cycles/public source factories.
Combat evidence: 262592 checks/84 native comparisons. Client/tool and36 selected
CTests pass after fixing a CDB-confirmed preparation stack overflow: pooled
selectors and a transition-only temporary Fighter candidate, no tick allocation.
Model controls:194 checks/16 native pairs/9 selectors/4 animation retirement stages;
actor publication/animation leaves are controlled. Scope and controlled leaves:
melee/docs/native_source_article_controls_checkpoint.md. Zelda's actual owned-data
source-check reaches802 (inherit_owner_debug_display); Sheik remains1543 (Chain
resources). No whole-form/runtime/gameplay acceptance is claimed.

No game or installed/user draft update. Continue missing Item primitives,
primary animation bundles/extra models, source-owned Chain resources and complete
paired/roster runtime validation. Full project goal remains active; local commits only.

### Article DATA/ownership/world-feedback closure (2026-10-08)

Source actor creation values are saved scalar state, zero during Spawned and
installed before Created; no overlapping union mirror remains. Primary Item
advancement executes actual HSD/frame/DATA order. Generic attachment/current-owner
queries, owner-debug inheritance, Bool map-collision modes, rumble and shared
camera quakes now execute in the interpreter. Misleading ownership spelling was
removed from tracked templates in favor of attachment_active, with its stale
immutable-creator implementation corrected.

Evidence: advancement398/32native comparisons; motion6315/3078; world257/32,
44quakeGObjs/36hardware-queue requests; actor1219/38cases; child18937/27cases/
1024reuse. Client/tool and36 selected CTests pass. Exact controlled boundaries:
melee/docs/native_source_article_advancement_checkpoint.md. Zelda all bodies now
compile; preparation rejects cpu_pending_action at421 because its typed scheduler
has not migrated. Sheik remains Chain metadata1543. Whole-form/gameplay acceptance
is not established. No game or installed/user draft update.

Continue typed Fighter hook schedulers, DATA operand admission, primary animation
bundles/secondary models and source-owned Chain resources, then complete paired/
roster runtime validation. Full project goal stays active; local commits only.

### Typed CPU scheduling and Item command admission (2026-10-08)

All 27 declared CPU events now have typed interpreted dispatch at native CPU
sites, sharing an immutable hook contract table. Caller lifetime checks stop
continuation after physical retirement or logical VM failure. Native common
spacing, recovery, classification and stick defaults remain intact.

Item command admission now checks actual Article geometry and native indexed/
instance topology, plus typed three-slot Call/Loop control flow. Stage scripts
receive their real common Random Article context. Legitimate nullable primary
joint paths preserve native default reads/no-op writes.

Evidence: CPU ABI 1786 checks/27 CPU hooks/28 original contracts/246 negatives;
sampled native CPU callsites 110 checks; command admission 2669 checks/47 cases,
33 fighter archives and 29 production stage specifications. Client/tool builds,
36 selected CTests and native_stage_data pass. Zelda owned-source preparation
passes 152 functions/56 imports/3154 instructions; whole-gameplay acceptance is
still unproven. Sheik remains blocked at Chain metadata. Exact boundaries:
melee/docs/native_source_cpu_scheduler_checkpoint.md.

Continue remaining Fighter callback families and generic resource/secondary-model
ownership, followed by paired/roster runtime validation. No game, installed content
or user draft changed. Full goal stays active; local commits only.

### Interpreted fighter presentation, movement and normal attacks (2026-10-08)

Twenty more typed Fighter root hooks now deliver at their actual native sites:
color/presentation/demo/taunt, held and aerial movement policies, Wait cleanup,
and normal attack/crouch startup. The shared lifetime contract was renamed
 directly to Fighter ownership; no compatibility aliases. Replacement versus
notification precedence, native pickup/common fallbacks and owned motion bounds
are preserved. State bindings now stop on exact userdata replacement; neutral
entry additionally retains its complete actor/program/storage lifetime.

Evidence: presentation321 checks; movement892/eight hooks/64 lifetime cases with
real state.wait/state.wait_blend VM dispatch; attack1015/102 cases/zero stock
handler dispatch. Existing primitive state277 and transition451 checks still
pass after moving synthetic positive neutral coverage to the connected fixture.
Client/content tool and all36 selected interpreter regressions pass. Controlled
engine leaves and limits: melee/docs/native_source_fighter_callbacks_checkpoint.md.

A real ten-draft admission census passes Mario and Zelda. Other templates remain
rejected at explicit missing primitives, legacy base declarations, Arrow render
metadata or Chain resources. These are diagnostic first failures, not an
exhaustive list or complete roster runtime acceptance. See
melee/docs/native_source_roster_admission_census.md. Continue generic binding/
owned-declaration and remaining scheduler/resource closures, then paired/roster
runtime validation. No game, installed content or user draft changed. Goal active;
local commits only.

### Generic normal operations and owned immutable policies (2026-10-08)

Five interpreter operations now run general jab/rapid-jab startup, owned rapid
completion, ground pickup and aerial landing completion. The common rapid body
returns a completion query; native/current advanced SDK and interpreter callers
invoke their own completion directly, without callback trampolines. Numeric
motions and helper signatures validate against the exact published source layout.

DrMario's root reset is now a plain owned helper using existing rotation and
callback APIs. Owned signals explicitly identify their Article and require both
current damage ownership and the caller's live attachment relation. Immutable
sound/friction/visibility/remap/KO recipes populate actual owned facts before DAT
import and revalidate exact facts; common-motion policies remain unimplemented
and rejected. Pure asset/layout validators were moved directly into small units.

Evidence: normal1250/58cases/64nativepairs/104sourceinvocations; root72/4pairs;
owned signals195; owned declarations404/69cases/4actualDATimports. Client/tool/
reference exporter builds and all37 selected CTests pass. Exact controlled leaves
and limits: melee/docs/native_source_normal_operations_checkpoint.md. DrMario
actual source admission still stops at items.clear_held333; no complete roster
or game acceptance is established. Original ISO, installed/user drafts untouched.

Continue direct C-oracle construction API and four redundant base removals;
then generic shared motion policies, held controls, source-owned resources and
remaining schedulers, followed by paired/roster runtime checks. Full goal active;
frequent local commits only, never push.

### Owned common phases, drawing and held control migration (2026-10-08)

Four canonical base hints were removed; the test-only C oracle now receives
explicit construction root/bounds from all65 adjusted callers. Climber neutral
and down specials have separate IceShot/Blizzard move containers with original
bodies preserved exactly. Ordinary source packages stay interpreted.

Common policies now apply canonical shared animation/interrupt/physics/collision
phases to real imported custom rows; numeric assets/camera remain independent,
and authored phases override. Source Article drawing selects its owned Standard
renderer before model load/Spawned; Workshop Billboard remains separate. Actual
instance-aware HSD topology admission is shared directly by command import and
source entity preparation, including indexed models without DATA.

Nine Fighter item controls distinguish originally created items from explicit
foreign restoration; attachment does not reactivate revoked control and scale
retains current damage owner independently. Native continuation guards now
retain full physical/data/class/program identity across held callbacks. Existing
raw C-policy fixture was migrated to actual pools, preserving its original
hazards and64depth tests instead of weakening runtime checks.

Evidence: oracle12samplers/9CLI +2unchangedClimber bodies; commonpolicy508/97cases/
4DATimports; render18constructor/113import +2669topology/47cases. Client/tool and
all37 selected CTests pass after final integration. Exact additional connected
held/control receipts and scope: melee/docs/native_source_owned_motion_render_checkpoint.md.
No GPU/fullfighter/game acceptance claimed; no game, installed/user draft or ISO
changes. Newactualfirstfailures: DrMario model.base_scale352; Peach held_id141;
GW costume.index147; Popo/Nana resources1621/1847. More generic primitives,
secondary/resource ownership, native schedulers and paired/roster runtime remain.
Goal active; all commits local, never push.

### Costume/material and held-query integration (2026-10-08, validation in progress)

Generic interpreter bindings now expose owned costume indices, common base model
scale, packed RGBA, fighter/article material colors and validated outline/layer
selection. Held/auxiliary identity queries and common visibility, pause and drop
notification are connected separately from creator-gated attachment. The drop
parent captures its full Fighter lifetime before continuing after callbacks.

Client/content-tool/reference builds and 37 selected integration CTests pass.
The new connected native/VM comparison fixtures are still being evaluated; do
not treat this entry as their acceptance. Full copied-draft preparation advances
DrMario to state.taunt401, Peach to notify_pickup160 and GameWatch to the missing
12th positional-factory collision flag172. Mario/Zelda still pass preparation.
Exact current census: melee/docs/native_source_roster_admission_census.md and
A:/FreakMelee-test-archives/source-color-held-census-oct08/receipt.json.

Secondary-model transition allocation/preflight is being implemented in the
actual HSD allocators. Source metadata remains rejected until real graph demand,
construction, destruction and rollback ownership exist. Current broad native
rollback remains its owner; compact actor snapshots alone do not cover graphs.
Full roster/paired gameplay and the complete project goal remain unfinished.

### HSD reservation foundation checkpoint (2026-10-08)

Committed locally as ba813105d. Real class/ObjAlloc entry points can consume
caller-reserved transition storage without splitting/growing the ordinary heap;
unknown allocations, direct HSD_MemAlloc and AddFree attempts fail the active
claim. Frees retain their backing owner outside the construction claim. Actual
native limit/counter policies remain in effect. A bounded Article descriptor
census reports core demands and explicitly marks uncovered features.

Evidence: hsd-reservation-final PASS86 across three actual allocator routes,
with source/header hashes and unchanged legacy warning sets. Client/tool build
and38 selected interpreter/content/pool/allocator CTests pass. The fixture uses
actual allocator bodies and synthetic trusted descriptors, not real graph
construction or gameplay. Secondary model loading remains unadmitted; TEV,
shape/envelope/reference/instance/animation demand, borrower cleanup and backing
snapshot registration remain open. See melee/docs/native_source_chain_resource_proposal.md.
The full goal remains active and incomplete; never push upstream.

### Color and held-query checkpoint accepted (2026-10-08)

Supersedes the validation-in-progress entry above. Generic material/costume
bindings pass 157 connected checks across 11 cases and three native comparisons.
Held queries and visibility/pause/drop controls pass 408 checks across 47 cases,
14 native comparisons and 38 source calls. Native client/content tool/reference
exporter builds and all 38 selected integration CTests pass. No game acceptance
is claimed; native asset publication and surrounding notification leaves remain
controlled in the fixtures. Full-width identities and noncanonical Bool failure
are covered. Details: melee/docs/native_source_color_held_checkpoint.md.

The final copied-draft census is source-color-held-census-final-oct08. Mario and
Zelda prepare; remaining first failures are DrMario taunt, Peach pickup, GW
initial-collision factory flag, Link/YoungLink secondary models, Sheik chain and
Climber resources. Complete roster gameplay and the full goal remain unfinished.
Continue generic engine operations and real graph/resource ownership; no donor
behavior fallback or name-only metadata acceptance. All commits stay local.

### Default material construction budget checkpoint (2026-10-08)

Real native material expression builders, compiler/optimizer/register scheduler
and cleanup now have bounded reserved-storage demands for supported default
texture combiners. Hidden comparisons pass 9215 checks across 289 material cases,
including live recompilation cleanup. Client/tool integration builds and 39
selected CTests pass with the current interpreter batch.

MaterialEnvironment remains explicitly uncovered: descriptor-only demand cannot
assume empty global toon/shadow graphs or prove default-method selection, renderer
resources, full model construction, animation or rollback ownership. No source
secondary-model/resource admission was enabled. See
melee/docs/native_source_chain_resource_proposal.md and
A:/FreakMelee-test-archives/hsd-tev-final/receipt.json. Goal remains incomplete.

### Common item, taunt and target-color checkpoint accepted (2026-10-08)

Interpreted common pickup/held-neutral/throw/engine creation, item reopen policy,
owned taunt motion entry and captured-target controller colors are connected.
Article velocity factories take an explicit initial collision Bool; existing
callers were migrated without an old-signature alias. Post-hook held-slot reads
now validate their caller first; a protected retired-payload test covers order.

Final connected evidence: 758 checks /79 cases /40 native comparisons /80 VM
calls for item policies; 6464 checks /330 comparisons for taunt and target
colors; 180 checks /25 cases for explicit factory collision. Client/tool/reference
builds and all39 selected CTests pass. Controlled asset and motion leaves remain
explicit; complete fighter gameplay is not claimed. See
melee/docs/native_source_common_policy_checkpoint.md for final receipt paths.

The full-source census advances to common taunt interrupts (DrMario), a stale
Void assignment (Peach) and legacy motion-entry calls (GameWatch). Real secondary
model and resource ownership remains open for Link/YoungLink/Sheik/Climbers.
Continue direct caller migration and generic APIs. Full goal remains incomplete;
all Git progress stays local.
### Direct script-call migration and shared taunt/clip checkpoint (2026-10-08)

Common item/taunt/color batch committed locally d2b41a88f. Follow-up committed
locally de9e28134: tracked templates, examples and composition scripts now use
state.enter_at_speed directly; unused Bool assignments from Void item clears
are removed. The ordinary interpreter supports the exact shared taunt interrupt
order and resident owned clip-length queries. No compatibility alias or stock
fighter behavior fallback was added.

Connected taunt/clip evidence: 2287 checks /106 cases /44 native comparisons /72
nested callbacks, with controlled input decisions and imported-table metadata.
Client/tool/reference builds,39 integration CTests and3 separately rebuilt
reference-exporter tests pass. No game was launched or gameplay acceptance claimed.
Receipt: A:/FreakMelee-test-archives/script-taunt-interrupts-final/receipt.json.
Full copied-source census: source-motion-calls-census-oct08/receipt.json.

Next coherent common-engine batch: actual nudge-aware ground collision (DrMario),
explicit ground-to-air conversion (Peach), exact native Foods creation (GW),
and creator-retained attachment/scale/binding for raw engine-created items (Peach).
Do not substitute similar-looking existing operations: their native behavior differs.

For real secondary-model activation, start by inspecting Link/YoungLink's four
static arrow trail DAT roots (offsets36/40, slots1/2); owned descriptors and the
native draw/destruction path already exist. Census actual graphs before admission.
Use transition-reserved scene backing with replacement headroom and lazy draw Vec
allocation covered, descriptors pinned through broad rollback history. Active toon,
shadow, secondary animations and unsupported graph features stay rejected until
implemented. Boomerang animation bundles and Hookshot/chain/rope collections remain
separate later work. Full goal remains incomplete; all commits remain local.
### Common motion, food and ordinary created-item checkpoint (2026-10-08)

Committed locally f06f76b3d: exact shared ground-to-air conversion, nudge-aware
collision and Foods spawning run through the interpreter. Creator-restricted
attachment/scale/held-slot controls now admit ordinary engine-created items
without a fabricated source specification. Legacy air_to_ground callers use
existing switch_to_ground directly. No compatibility alias or stock-special
handler fallback was added.

Connected evidence:511 checks/43cases/28native comparisons/12nested callbacks
for motion/collision;1023checks/114cases/42comparisons/109VM calls for ordinary
item controls/food. Refreshed older movement fixture:1916checks/229cases/
122comparisons/100identity mutations. Client/tool/reference builds,39selected
integration CTests and3rebuilt exporter tests pass. Controlled constructor,
asset, geometry and render boundaries remain explicit; no game was launched.
See melee/docs/native_source_common_policy_checkpoint.md for receipts/limits.

Real Arrow descriptor census committed locally2d3dbfee4: all four imported
trail roots have custom texture combiners and remain rejected. Their856B core
payload plus potential16B Vec does not cover generated shader allocations.
Implement generic bounded operand-shape demand against actual native combiner
construction/free before resource admission; do not admit names or hashes.

Fresh first-diagnostic source census:source-common-motion-census-oct08/receipt.json.
Next batch:owned appearance baking, shared aerial-jump threshold observations,
and direct migration to explicit declared-Article signals. Link/YoungLink/Sheik/
Climber real resource ownership remains open. Complete roster/gameplay/project
acceptance is not claimed. Goal remains active and unfinished; commits stay local.

### Owned appearance, common jump reads and custom combiners (2026-10-08)

Committed locally24b898f1d: ordinary interpreter Article appearance baking uses
owned descriptor/model data and exact native operation order, with guards between
animation stages so a reentrant state replacement is not cleared by outer work.
Four generic read-only jump threshold/multiplier properties use actual native
fields. GameWatch signals explicitly name their declared Article; existing air
and facing-ledged probes are called directly without aliases.

Appearance fixture:1693checks/52native comparisons (20bake pairs)/57negative
cases. Grouped binding fixture:33463checks/9435property observations/40semantic
expressions. Native client/tool/reference build and39integration+3reference
CTest checks pass. Animation evaluation/assets/actor boundaries are controlled;
no retail rendering or complete-fighter gameplay acceptance is claimed.
Fresh source census:source-appearance-jump-census-oct08/receipt.json. See
melee/docs/native_source_common_policy_checkpoint.md for exact receipts/limits.

Committed locallyd710d9739: generic bounded custom texture-combiner allocation
demand, verified through actual native builder/compiler/recompile/free across
341materials including four imported Arrow roots (11265checks). Correct real
trail demand is5expressions and2/3constants; previous descriptor-only prediction
was corrected after native execution. MaterialEnvironment remains uncovered128.
Construction allocation coverage is not universal packed-register legality;
future graph admission must prove environment/default methods and actual compile
legality/failure handling, ownership, replacement/draw/destruction and rollback.
No model metadata has been enabled based only on this budget.

Next common capability gaps:article accessory tracking, writable common interrupt
policy, native ledge entry. Audit remaining capabilities as batches alongside
continued real secondary-resource work. Complete plan/roster remains unfinished;
goal remains active and local commits are not pushed.

### Generic accessory, IASA and ledge checkpoint (2026-10-09)

Local commits `525decfb2` and `4cff41fdb` record the grouped Peach/Dr. Mario
capability audit, prepared dynamics/contact proposal, and ordinary interpreter
Item accessory tracking/clearing, writable Fighter IASA and common ledge entry.
Native ledge/animation continuations retain physical and logical source identity
across nested callbacks. Healthy native order is preserved. Existing scripts
and test-only reference export use the current direct spellings without aliases.

Native client/tool/reference builds pass. The selected 39 integration and three
rebuilt reference tests pass. Focused evidence: accessory 137 checks/33 cases/
12 source signals; ledge 806 checks/eight native comparisons/42 lifetime cases;
grouped binding/IASA suite 34125 checks; unchanged Item primitive regression
7904 checks. Exact receipts and controlled boundaries are recorded in
`melee/docs/native_source_common_policy_checkpoint.md`. No game was launched,
and no complete-fighter or rendering acceptance is inferred.

The ten-template preparation census now advances beyond these first blockers.
Mario/Zelda prepare; Dr. Mario still needs the shared Article lifecycle/model
cluster (spin reset, real nested phase refresh, removal, distinct scaling,
translation and single-node visibility/live-root observations). Peach also
needs float, guard, dynamics/contact, held transitions and optional receiver
helper delivery. Game & Watch needs further generic visibility support.
Link/YoungLink/Sheik/Climber real resource ownership remains open.

Next work should implement related capabilities as batches from the whole-script
inventories, retaining generic names and exact native contracts. The dynamics/
contact proposal specifies direct prepared source dispatch without donor fallback.
Complete plan and roster acceptance remain unfinished; the goal remains active.
Commits are local only and nothing was pushed.

### Generic Item lifecycle and model checkpoint (2026-10-09)

Local commit `18c82bd89` adds direct interpreter spin reset, distinct attribute,
instance and current-owner scaling; prepared primary joint translation and
single-node visibility; generic live rendered Fighter root-scale observation;
and actual immediate Item physics/collision plus terminating removal. Narrow
native continuations retain physical and logical source identity across nested
callbacks. Source removal, including reentrant Destroyed removal and deferred
proc cleanup, stops the invoking VM without touching a replacement actor.

Canonical Dr. Mario controller accesses now use attachment_owner, with generic
joint controls and authored two-motion incapacitation policy. Peach's creation
call directly uses the existing explicit velocity/creation-value form. No
fighter-specific APIs, generated-C runtime or compatibility aliases were added.
Dr. Mario's full source now prepares (111 functions, 35 imports, 2894
instructions); full gameplay acceptance remains separate.

Native client/content tool/reference builds pass, along with 40 selected
integration and three reference tests. Focused receipts: model 1928 checks,
34 new native spin/scale comparisons and 35 rejection/continuation cases;
lifecycle 232 checks/29 cases/23 actual VM signals; grouped bindings 34406
checks with 15 root-scale native comparisons and 13 related rejection cases.
Exact paths and controlled boundaries are in
melee/docs/native_source_item_model_lifecycle_checkpoint.md. No game was launched.

Current ten-template census: source-model-lifecycle-census-oct09/receipt.json.
Mario/Dr. Mario/Zelda prepare. Peach next stops at model hiding; Game & Watch
still needs current visibility selection. Link/Young Link, Sheik and Climbers
remain at real secondary-resource metadata boundaries. Continue shared
capability batches and actual resource ownership; complete plan/roster/gameplay
acceptance remains unfinished. Goal stays active; commits remain local only.


### 2026-10-09: interpreted current visibility, item expiry and world effects

Implemented generic current/saved visibility selection and queries, whole-Item
model visibility and descendant joint animation rate, native expiry setup, and
owned Item positional effects with audited native context requirements. Native
client/tool build and 40 integration + 3 reference tests passed. Grouped native
comparison receipts and explicit limits are recorded in
`melee/docs/native_source_visibility_expiry_checkpoint.md`.

No whole-fighter/gameplay acceptance is inferred. Mario/Dr. Mario/Zelda continue
to prepare. Next: holder held-motion/frame access for Peach, absorber operations
for Game & Watch, then secondary-resource metadata for the remaining templates.
Maintain generic interpreter APIs and owned data, not fighter-specific shortcuts.
Goal remains active; no pause or completion was requested at this checkpoint.

### 2026-10-09: interpreted held observations and absorption

Added generic physical held-item ID/kind/motion/frame observations, native
absorption geometry/flags/counters, the actual Fighter Absorb script hook and
common horizontal-stick deadzone reads. Canonical Peach holder queries and
Game & Watch creator helpers use the current interpreter interfaces directly.
Native client/tool/reference builds and 40 integration + three reference tests
pass. Focused held and absorber receipts pass 1243 and 275 checks respectively;
controlled boundaries are recorded in
`melee/docs/native_source_held_absorber_checkpoint.md`.

The fresh ten-template preparation census keeps Mario/Dr. Mario/Zelda passing;
Peach next needs cross-fighter exported calls, and Game & Watch needs the
remaining Fighter combat/input and Item observations. Link/Young Link, Sheik
and Climbers still require real secondary graph/chain/tether ownership. The
recipient and resource plans document native contracts before implementation.
No whole-fighter gameplay or final rollback acceptance follows from these
fixtures. Continue shared interpreter capabilities in grouped batches, with
fighter decisions authored in scripts. Goal remains active; commits stay local.

### 2026-10-09: interpreted ground controls, hitboxes and Item observations

Implemented shared Fighter capsule state/damage with native scale/smash/stale
arithmetic; twelve authored-priority ground input checks, common crouch entry
and landing lag reads; Item surface response, stage sweep flags and primary
animation completion. Common ground transitions validate lifetime before cached
continuation writes. Canonical Game & Watch callbacks, turn cue and guarded
current-owner helper calls use existing direct interpreter interfaces.

Native client/tool/reference builds and 40 selected integration + three reference
tests pass. Grouped native fixtures and controlled boundaries are recorded in
`melee/docs/native_source_ground_item_combat_checkpoint.md`. No game launched.
Game & Watch preparation advances to root Fighter parameters referenced inside
Article-local scope; resolve their actual ownership rather than adding implicit
attribute inheritance. Prepared joint-position outputs, foreign smash-charge
observations, Peach recipient exports and real secondary-resource ownership
remain next capabilities. Specialized Ness smash-entry hooks remain an explicit
direct-interpreter gap. Complete project/roster/gameplay acceptance remains open.
Goal stays active; commits stay local and nothing is pushed.

### 2026-10-09: explicit package-owned root parameters

Added generic read-only `program.attrs.NAME` access to the executing package's
immutable root tuning from Article/Item/Effect scopes. Local attributes keep
exact ownership; mutable root state remains inaccessible. Game & Watch's 32
Article reads now preserve the reference module's shared parameter ownership
without requiring a live creator. Native client/tool/reference builds and 40
selected integration + three reference CTests pass. Execution checks include
an Article reading its pinned parameters after the root Fighter retires.

Evidence and limitations are recorded in
`melee/docs/native_source_program_parameters_checkpoint.md`. The rebuilt ten-
template census reaches Game & Watch's Article airborne write at line1596;
recipient exports and secondary graph/collection ownership remain open. Audit
remaining Article operations as a batch before implementation. No game launched
and no complete fighter/gameplay acceptance claimed. Goal remains active and
commits remain local only.

Focused final receipts for this parameter batch pass with unchanged source/header
fingerprints: `script-program-parameters-final` (294 checks, 38 cases, 71 VM calls;
real Item actors, creator retirement and packed actor restore with controlled
assets/import supply) and `script-program-attrs-reference-oct09-final` (72 checks,
11 cases; test-only source emission/import oracle). Complete gameplay and whole-
match snapshot acceptance remain separate.

### 2026-10-09: complete the remaining Article API batch

Implemented generic source-owned Item airborne writes, prepared primary joint
world-position outputs, copied-position Article wind and retained foreign-Fighter
smash-charge queries. Scripts own state transitions and behavior; no stock fighter
handlers or primary-graph substitutes were added. Game & Watch's full template
now prepares successfully: 364 functions, 80 imports and 6,492 instructions.
Mario/Dr. Mario/Zelda still prepare; Peach recipient exports and Link/Young Link,
Sheik and Climbers secondary-resource boundaries remain open.

Native client/tool/reference targets build; 40 integration and three reference
CTests pass. Grouped actual-native/VM fixture receipts and controlled boundaries
are recorded in `melee/docs/native_source_article_queries_checkpoint.md`.
No game launched, and no whole-fighter gameplay/rendering or whole-match rollback
acceptance claimed. Recipient-owned exports are next, followed by real resource
construction/ownership. The overall goal remains active and commits stay local.

### 2026-10-09: interpreted recipient-owned exports

Implemented generic `targets.call/call_bool/call_int/call_float` against the
captured recipient's own prepared program, local helper ID, state and tuning.
Root export validation and bounded collision-safe literal-name tables run during
preparation. Runtime dispatch preserves actual VM status and caller/recipient
lifetime, without runtime hashing, allocation or stock-handler fallback.

Native client/tool/reference targets build and all 40 selected integration plus
three reference CTests pass. The connected three-bank fixture passes 757 checks
in 86 cases, including nesting, hash collisions, physical reuse and fault/retirement
publication. Evidence and controlled fixture limits are documented in
`melee/docs/native_source_recipient_exports_checkpoint.md`. No game launched.

Peach now advances to Item victim response at line651; Mario, Dr. Mario,
Game & Watch and Zelda still prepare. A complete remaining Peach API/hook audit
groups the next implementation batches rather than treating each next compiler
diagnostic as complete acceptance. Secondary-resource graph ownership for
Link/Young Link, Sheik and Climbers remains open. Full roster/gameplay/rollback
acceptance remains separate. The goal stays active; commits remain local only.

### 2026-10-09: generic Item responses and attachment identity

Added direct interpreter operations for common victim bounce, whole masked stage
response and physical attachment-owner comparison. The latter now uses the
truthful `item.attachment_owner_is` spelling across tracked canonical/reference
scripts; immutable creator provenance remains separate. Native collision-box,
position, presentation and outgoing-damage behavior retains original ordering.
Scripts own their state transitions; no stock fighter handlers were added.

Client/tool builds and all 40 selected integration plus six reference CTests pass.
The connected fixture passes 4,421 checks in 653 cases, with 27 actual VM calls
and 603 native comparisons, including owner/source retirement, reuse, faults and
the live Destroyed window. Evidence and controlled engine boundaries are in
`melee/docs/native_source_item_responses_checkpoint.md`; source/template receipt
fingerprints were independently verified. No game launched or whole-fighter
gameplay acceptance claimed.

Peach now reaches guard creation at line726; Mario, Dr. Mario, Game & Watch and
Zelda still prepare. Link/Young Link, Sheik and Climbers secondary resources
remain open. Next is the coherent guard-volume/owned-ShieldHit helper/field batch
in `melee/docs/native_source_guard_api_plan.md`, then the other grouped Peach
operations. The complete project goal remains active; commits stay local.

### 2026-10-09: generic guard volumes and owned ShieldHit reactions

Added direct interpreter guard construction, position refresh and scalar fields,
with explicit owned root helpers in the existing ShieldHit callback slot. Native
reconstruction clears old interpreted handlers; actual ProcessHit delivery checks
retained Fighter lifetime before cached native continuation. Raw costume-joint
indices, integer damage and independent flags preserve the real engine contract.
Canonical/reference sources now bind their own helpers without stock handlers or
a legacy guard-hit alias. Generated C remains a test oracle only.

Client/tool builds and 40 selected integration plus nine reference CTests pass.
The connected guard fixture passes 432 checks in 83 cases, 44 native comparisons
and 57 actual VM calls; root verified all 1,651 recorded input hashes. Evidence
and controlled boundaries are in `melee/docs/native_source_guard_checkpoint.md`.
No game launched or complete counter/gameplay/rollback acceptance claimed.

Peach now reaches async attached effects at line1000; Mario, Dr. Mario,
Game & Watch and Zelda still prepare. The other templates' secondary-resource
boundaries remain open. Later grouped Peach operations and complete roster
acceptance remain in scope. The full goal stays active; commits remain local.

The guard batch is committed locally as `4a75a405f`. Two C-reference Peach
counter fixtures also pass 4,594 comparisons and 38,050 checks; focused
Link/Young Link guard-only comparisons pass 48 cases and 538 checks. Root verified
their 2,024 recorded inputs. Historical full-Link regeneration has unrelated
metadata/predicate drift and was not claimed passing or silently repaired.
Next grouped contracts and concrete native boundaries are recorded in
`melee/docs/native_source_async_effect_api_plan.md` and
`melee/docs/native_source_float_air_api_plan.md`.


### 2026-10-09: interpreted airborne controls and native attached effects

Connected eight generic Fighter APIs: aerial motion/C-stick observation, shared
air gun/throw/special input, adjusted TransN vertical velocity, an owned landing
helper and async joint-only effects. Native callbacks retain Fighter/Item identity
before cached continuation. The actual throw/drop functions now carry an optional
retained record for their chosen Item, including replacement during a Fall hook;
normal self-removal still handles input, while faults and failed admission stop
publication. All call sites and C-reference host signatures are updated directly.
No compatibility adapter or donor special handler was added.

Client/content-tool/reference build and 50 selected CTests pass. Connected cohorts
pass 6,416 checks across 345 cases, including 711 native comparison pairs in the
airborne families. Root verified all 4,903 recorded cohort inputs plus the rebuilt
tool and ten census sources. Evidence and controlled boundaries are in
`melee/docs/native_source_float_air_checkpoint.md`; fresh receipts are on A: under
`script-air-inputs-final2`, `script-air-physics-final2`,
`script-async-effect-oct09-final2` and `source-air-async-census-oct09-final2`.
No game was launched. These checks do not establish complete Float, gun/throw
animation, rendering, native heap reservation or whole-match snapshot acceptance.
An extra historical held-lifecycle comparator build failed due to its unmigrated
lifetime/VM setup and original two-argument drop leaf; it is not counted as a pass.

Peach advances from line1000 to1163 (thrown-item statistics). Mario, Dr. Mario,
Game & Watch and Zelda still prepare; Link/Young Link, Sheik and Popo/Nana retain
their secondary-resource schema boundaries. Next coherent closures are typed
stats/observations, prepared contact-rebound and dynamics decisions, genuinely
source-owned animation-driven jump, and held-open transitions. Super Scope also
needs a separate typed native state cleanup: current byte-overlay entry matches
are not native layout acceptance. Full goal remains active; no push.


### 2026-10-09: interpreted observations, captured landing input and body rebound

Connected generic thrown-item counting, special-landing lag and debug-level
observations, plus recipient-owned prepared body-rebound decisions. Contact now
returns execution status separately from its Bool and retains both actors/the
contact relation after either result. Native Peach and reference Host callers
use the substantive API directly; changed Host layout rejects its old signature.
LandingAction receives captured event.interrupt_allowed, which cannot be changed
by reuse of the motion-state union. Current scripts/reference fixtures use the
new event contract, without an old-name compatibility alias.

Super Scope Start/Fire/Rapid now use typed common motion fields and actual native
GObj user_data. GameCube byte overlays are removed. End was inspected unchanged.
Client/tool builds and 55 selected CTests pass. Four connected/layout cohorts
pass 8,149 checks; the separate ordinary aerial/contact oracle passes 781,090.
Root verified 8,086 recorded input hashes and the fresh tool/ten-source census.
Evidence, controlled leaves and limits are recorded in
melee/docs/native_source_observation_contact_checkpoint.md. No game launched;
these are not full fighter/render/dynamics/rollback performance acceptance.

Peach advances to line1440 (motion-ID write before held-open entry). Next work
is held transition, prepared dynamics decisions, animation-driven jump and
owned Article resource admission. Other templates retain their documented
secondary-resource boundaries. The full goal remains active; local commits only.
The preceding airborne/async batch is locally committed as 2b3d0253b.

This observation/contact/Scope batch is committed locally as `45ac45dce`.


### 2026-10-09: owned held-open entry, animated jumps and dynamics decisions

Connected bounded motion.id publication and state.held_open through the actual
common entry, preserving predecessor selection, animation and timer order.
Animation-driven jump now installs an owned prepared Physics hook after native
jump bookkeeping/sounds. Peach, Yoshi and Mewtwo templates author its shared
physics body. Phase overrides and snapshots retain interpreted code IDs rather
than native callback pointers. Per-set dynamics interaction decisions also run
through prepared I32 callbacks with native graph continuation guards.

Client/content-tool/reference builds and 55 selected CTests pass. Three connected
cohorts pass 22,789 checks across 1,129 cases, with 1,000 native comparisons/traces.
Root verified all 4,992 recorded cohort inputs plus the rebuilt tool/ten sources.
Controlled native boundaries and limits are in
melee/docs/native_source_held_jump_dynamics_checkpoint.md. Dynamics solver math,
full scene rollback and in-game acceptance are not claimed. Source wind/ground
policies remain unmigrated; native graph allocations lack individual generations.
No game was launched. Older private synthetic-lifetime Jump tests remain historical.

Peach now passes full source preparation (261 functions, 67 imports, 4,241
instructions), including actual preparation of its five primary Article assets.
Next is actual-owned-asset constructor/lifecycle and live activation acceptance;
no new primary schema gap is established. Link/Young Link, Sheik and Popo/Nana
still stop at the documented secondary-resource schemas. Full goal remains active.

This held-open/jump/dynamics batch is committed locally as c3cf61173.

### 2026-10-09: actual owned Peach primary-article activation

Peach now prepares and installs as a direct-interpreter source package (261 functions, 67 imports, 4,256 instructions). Real native-library construction/lifecycle testing covers all five primary Articles and all 13 state/animation/DATA setups; the parent Fighter setup and effect-hook limits are explicit. A hidden, muted, drawing-enabled 2,400-sample match with four source Peaches completes without script faults, exercising item kinds 98/99/103/104. The independent constructor cohort covers Spore too. No stock trace parity, full graph rollback or screenshot acceptance is inferred.

Fixed Toad owner-vs-holder translation, explicit faulted-recipient diagnostics, and retained common capture continuation/VM collision dispatch. The old invalid package still exposes incomplete scene fault isolation in a later capture path; terminate/retire an errored scene coherently rather than continuing partial capture state. Evidence and limits: melee/docs/native_source_peach_articles_checkpoint.md. Next implementation boundary remains generic secondary models/animation bundles for Link/Young Link, then resource collections for Sheik/Popo/Nana. Full goal remains active.

Committed this Peach activation/lifecycle batch locally as d362b835f. Final constructor cohort: 202 checks, 3,045 unchanged input fingerprints including 47 DLLs; final four-Peach live run: 2,400 samples, exit 0, no script errors. Faulted-scene recovery remains open; no goal completion or pause is claimed.


### 2026-10-09: checked secondary material preparation and actual Arrow graphs

Added explicit current HSD class/method/environment admission, actual standalone material preparation under a prebound reservation, and checked status propagation through the same native TEV compiler. Expected capacity failures precede constructors; checked compilation preserves accepted shader output and discards temporary candidates. All native callsites use the changed API directly. Original/current output matches across341 materials including allfour Arrow roots;128 additional original compiler pairs and10 real packing failures pass.

The actual native-library cohort constructs24 owned Link/Young Link Arrow graphs, exercises16 matrix traversals, replacement/reuse/exhaustion and both descriptor-ID deletion orders, and rejects changed methods/nonempty toon without mutation.565 checks pass with3,055 unchanged recorded inputs; root also reverified both compiler comparison receipts. Client/content tool/runtime rebuild and8 selected CTests pass. No game launched. Evidence and boundaries: melee/docs/native_source_secondary_material_checkpoint.md.

Source secondary-model metadata remains disabled until indexed owner rows, actual Item slot lifetime, scene reservation backing, render claims and broad-history coverage connect. Material preparation does not prove drawable-input validity: zero texture repeats/absent images require validation before renderer admission. Boomerang animation and Chain/Gumstrings/tether collections remain open. The complete explicit match snapshot is still unfinished; live rollback retains broad page capture for unconverted owners. Full goal remains active.

Committed this checked-material/static-graph batch locally as 4124cefa5; no push. Remaining full snapshot and source secondary-runtime work stays in scope.

### 2026-10-09: immutable secondary imports and captured scene ownership

Prepared exact owner-scoped named Article roots through the owning DAT import
cursor and added bounded scene storage for independently constructed secondary
graphs. Scene backing comes from the main HSD heap, with fixed slot ownership,
package leases, replacement headroom, lazy matrix claims and ordered teardown.
Drawable-input admission now covers actual native texture/material/vertex
requirements. Still-live destruction callbacks can inspect their retained graphs;
an identity/removal-phase change during an individual claim rejects completion.

The current client/content/reference build and eight selected CTests pass. Three
focused cohorts cover 11,330 draw-admission checks, 142 immutable import/pool
checks and 80 actual compiler/Item actor/VM/scene lifecycle checks. Root verified
their documented input fingerprints; controlled staging and evidence boundaries
are in melee/docs/native_source_secondary_owner_checkpoint.md. No game launched.
This batch is committed locally as 8eb183dd0; no push.

Public secondary metadata and script operations remain gated until actual
history restoration and interpreter acceptance connect. The next batch tests
load/replacement/destruction through real page history, then enables the generic
static-model script path. Boomerang animation bundles and resource collections
remain separate boundaries. The complete compact match snapshot is unfinished;
existing broad page capture remains in use. Full goal stays active.

### 2026-10-09: secondary graph ownership restored through actual page history

The connected console cohort now uses production region discovery and Windows
page history to restore secondary graph replacement/destruction, lazy matrix
allocation, IDs, allocator metadata, script storage and GObj retirement/reuse.
Eight frame executions and two restores pass 275 checks; repeated simulation
matches full discovered-region hashes and explicit owner/topology observations.
History retains package leases and blocks scene retirement until shutdown.
Root independently verified all 3,249 frozen fixture inputs.

Evidence: melee/docs/native_source_secondary_owner_checkpoint.md. The private
83,583,012-byte tracked region set is not a compact savestate or full-match
performance result. Public interpreter model operations, full native Item
factory/scheduling and GPU drawing remain outside this controlled one-root test.
The test-only proof is committed locally as 1e239741c; no push or
game launch. Next work connects generic script model commands to this owner.

### 2026-10-09: direct interpreter static secondary-model operations

Enabled exact owner-scoped model_root declarations and generic load/destroy,
draw masks, joint/quaternion properties, bulk transforms and world-position
queries for primary/secondary slots. Runtime projections retain Item identity,
scene epoch and graph allocation generation; vacant defaults never alias the
primary model. Import and compiler share the actual declaration grammar.

The current client/content/source-assets/reference build and eight selected
CTests pass. Actual imported Link and Young Link scripts execute 434 checks
through compiler/publication, Item actor/VM, native HSD setters and the bounded
scene owner. Refreshed immutable import/pool checks pass 198 assertions; existing
primary-model comparisons pass 2,594. Root independently verified the connected
and import receipt inputs; primary code inputs remain unchanged with two later
documentation corrections. No game launched. Evidence and controlled boundaries:
melee/docs/native_source_secondary_owner_checkpoint.md.

Static support does not finish the complete Arrow translation. Generic remaining
ownership/contact APIs, animation bundles and resource collections remain open.
The actual page-history proof is independent; a compact whole-match snapshot is
still unfinished. Full project goal remains active; no pause/completion claimed.

Committed the static secondary-model interpreter batch locally as c187a8cc7; no push. Animation bundles, collection ownership and whole-match snapshot closure remain in scope.

### 2026-10-09: immutable animation bundles and native allocation census

Prepared exact named joint/material/shape animation triplets in the owning
character/blueprint DAT arena, preserving cached graph identities and independent
source-owner tables. Native null channels stay null. Added a fixed-work census
of actual controller/track applications with compressed scalar bounds checks;
unsupported callback/reference/shape/palette/channel families remain explicit.
The direct interpreter still rejects animation metadata until bounded runtime
attachment and operations are connected.

Client/content/source-assets and selected runtime targets rebuild; eight
allocator/Item CTests pass. The new animation foundation cohort passes 421
checks in 24 cases, including 40 actual native scalar evaluations. Real Link
and Young Link Boomerang bundles each need one controller and one track, with
no uncovered census channels. Refreshed static import/pool checks pass 198
assertions. Root independently verified both cohorts' 3,177 frozen inputs and
3,176 live source fingerprints. Evidence, controlled boundaries and next
runtime contracts: melee/docs/native_source_secondary_owner_checkpoint.md.

Committed locally as b7ab8c616; no push or game launch. Next connect generic
animation add/request/advance/remove to prepared selectors and bounded pools,
then prove mutable controller restoration. Sparse bundle retention and native
callback/storage ownership must be handled before public admission. Resource
collections, remaining Arrow ownership/contact operations and compact whole-
match snapshots remain unfinished. The full goal stays active.

### 2026-10-09: public interpreter animation playback and restoration

Connected owner-scoped model animation add/request/advance/remove/rate directly
to the interpreter. Literal model/path/bundle selectors prepare native demand
and controller origins before publication; actual fixed-pool capacity and full
retained graph provenance are checked before mutation. Sparse animation remains
native, including retained controllers and root-excluding subtree rate changes.
Descriptor packages stay leased until scene/history drains. Fixed publication
origin-pointer comparison and primary-only lease edge cases. Unsupported
palette/shape/reference/callback channels remain explicit gaps.

Actual client/content/source-assets and selected runtime targets rebuild; eight
allocator/Item CTests pass. The grouped Link/Young Link connected cohort passes
3,188 checks in 104 cases, with 244 native graph comparisons, sixteen executed
history frames and four actual Regions/Windows Pages restores. Primary-only
variants have no named roots/bundles but do retain native state origins; no
zero-origin fixture claim. Root independently verified frozen evidence and
unchanged code/library/asset fingerprints. Refreshed importer/census checks
pass 413 assertions with forty actual scalar comparisons. Detailed receipts,
controlled fixture boundaries and limits are in
melee/docs/native_source_secondary_owner_checkpoint.md.

Committed locally as 24644d78c; no push or game/GPU/audio launch. This finishes
bounded playback support, not complete Boomerang activation or whole-match
rollback performance. Continue with remaining Article ownership/contact and
resource collections, then the remaining interpreter/state closure work.
Compact whole-match snapshots remain unfinished; the full goal stays active.


### 2026-10-09: generic interpreter Item contact and collision operations

Connected six native Item operations (floor sweep, wall/ceiling contact flags,
wall reflection, floor alignment and velocity-facing alignment) and three generic
contact/reflection/recipient-detachment observations. Preserved native mutation,
wall precedence, damping and dynamic-bone selection; failed publication retains
caller outputs. Migrated remaining Link/Young Link attachment receiver aliases
with original eligibility/default semantics. Same-program helpers do not need
cross-program exports; review confirmed that existing calls remain valid.

Actual client/content/source-assets targets rebuild, eight selected allocator/
Item CTests pass, and grouped fixtures pass 8,595 and 6,426 checks. Native payload
comparison coverage and controlled boundaries are recorded in
melee/docs/native_source_item_contact_checkpoint.md. Root verified all 1,651 and
1,670 recorded input fingerprints plus ten whole-source census hashes/tool hash.
No game/visible/GPU/audio launch. Whole-source preparation still stops at resource
collections for Link, Young Link, Sheik and both Climbers; no declarations were
removed to obtain a pass.

Next replace legacy Definition/ArticleLogic collection ownership with immutable
Item-program recipes and retained scene node/graph registrations, actual fixed
backing, borrower-safe teardown and history restoration. Collections cannot be
aliased to secondary Item slots. Remaining whole-fighter acceptance and compact
whole-match snapshots stay in scope. The full goal remains active.


### 2026-10-09/10: interpreter-owned indexed resources

The source-owned Item/Article resource collection batch is integrated and verified at bounded console scope. Generic recipes (capacity 2-90, final-rig external-part reservation), captured dynamic selectors, decoded joint paths, owner-scoped model ordinals and owned Float[12] matrix declarations now reach the real kernel. Fixed scene ItemLink/graph backing, package leases, borrower cleanup, deferred release and orphan graph retirement replace Definition/ArticleLogic collection ownership. Omitted native parts are included in reservation validation. Resource animation groundwork uses an explicit origin ledger and canonical published program; public resource animation methods remain blocked pending exact source application preparation.

Actual client/tool/source-assets build and ten selected CTests pass. Grouped proofs: metadata 928 checks/310 cases; frontend 533/45; real connected resource kernel/history 270/9 (ten graph loads, two frames, one restore); existing Item-animation regression 3188/104 with 244 native graph comparisons, sixteen frames and four restores. Controlled boundaries and immutable receipts are documented in melee/docs/native_source_resource_checkpoint.md. No visible/audible game launches. Whole-source checks now advance past resources: Link/Young Link stop at explicit move entry metadata, Sheik at hitbox history coordinates, and Popo/Nana at positioned article constructor arity. No declaration was removed to obtain a pass.

The full goal remains active. Whole-fighter/gameplay acceptance, public resource animation Add, final interpreter gaps and compact whole-match save states are not completed by this batch; Regions/Pages remains the current owner.


### 2026-10-09/10: explicit entries, hitbox coordinates and article creation data

Committed resource ownership batch locally as 4733d2a9b. The next generic batch validates entry explicit (66 checks/14 cases), exposes independently writable current/previous hitbox coordinates and native world-position setup, and adds the eight-argument positioned Article factory with collision group and creation value. No fighter-specific convenience binding, source declaration removal or template rewrite was used. Naked Namespace selector call statements now fail during compilation.

Final client/tool/source-assets build and eleven selected CTests pass. Connected constructor proof: 270 checks/33 cases. Connected coordinates: 2063 checks/286 cases, including 253 raw-field/native-helper comparisons; unchanged state/damage regression: 11344 checks/3250 cases. Exact receipt scope and limitations are in melee/docs/native_source_entry_coordinates_checkpoint.md. Standalone catalog lists and direct declarations were updated, without claiming every historical runner executes.

Complete source checks now reach mutable sword-trail payloads in Link/Young Link, hitbox.set_enabled in Sheik, and effects.spawn_on_part in Popo/Nana. Mario, Dr. Mario, Peach, Game & Watch and Zelda still prepare successfully. No whole-fighter or whole-match gameplay acceptance was inferred; no visible/audible game launches. Compact snapshots and the remaining interpreter/runtime gaps remain open. Full goal stays active.

### 2026-10-09/10: hitbox controls and synchronous raw-part effects

Committed the explicit-entry/coordinates/article-data batch locally as ec85feb3d.
The next generic batch connects hitbox.set_enabled and clear_victims, preserving
independent victim history and the intentional Ness transition-cleanup correction.
Canonical effects.spawn_on_part now runs synchronous native raw-rig attachment;
the previous raw spelling/Feedback branch was removed, with direct catalog,
source and test migration. Scalar and semantic-bone variants remain distinct.
No fighter-specific extension or delegated stock callback was introduced.

Actual client/tool/source-assets build and eleven selected CTests pass. Connected
controls: 281 checks/112 cases, 42 native comparisons and 54 VM calls; unchanged
state/damage regression: 11344/3250 with 3207 native comparisons. Connected
raw-part effects: 247/51 with fourteen retirement mutations. Root independently
verified all 1696 and 1603 recorded inputs, and the rebuilt tool/ten-source census
hashes. Controlled boundaries and immutable receipts are in
melee/docs/native_source_hitbox_effects_checkpoint.md. No game/GPU/audio launch.

Committed locally as c7a5c826a; no push. Source preparation now reaches
hitbox.clear_all in Sheik and hitlag.active in Popo/Nana. Link/Young Link still
stop at mutable sword-trail data; the existing five successful source checks
remain successful. Next batch should connect generic callback output records,
Fighter-wide capsule cleanup and linked-hitlag observations/operations rather
than removing unsupported bodies. Public resource animation application,
whole-fighter behavioral acceptance and compact whole-match save/load remain
unfinished. Regions/Pages remains the current snapshot owner. Full goal active.

### Interpreter trail records and shared cleanup checkpoint

Committed locally as `7a3cf2306`; no push. Generic bounded event output records
now run through actual Execution/Fighter dispatch with staged publication.
`on trail` feeds the native afterimage reader using owned raw rig parts, with
descriptor and class admission. Added native `hitbox.clear_all` and the generic
active/linked-ID/link/unlink/release-request hitlag surfaces; no stock callback
delegates or compatibility aliases. Updated all affected call sites directly.

Actual client/tool/source-assets build and eleven selected CTests pass. The
record kernel passes 197 checks/16 cases. Connected trail: 255/57, six complete
native record comparisons and 22 explicit Fighter/Execution calls. Capsule
controls: 357/151 with 62 native comparisons; unchanged damage/state regression
11344/3250. Linked hitlag: 142/73 with 30 native comparisons and 51 VM calls.
Root independently verified all pinned inputs in the four receipts. Scope and
controlled boundaries are in
`melee/docs/native_source_trail_hitlag_checkpoint.md`. No game/GPU/audio launch.

The unchanged ten-source preparation census still passes Mario/Dr. Mario/Peach/
Game & Watch/Zelda. Link/Young Link now stop at `effects.spawn_between`, Sheik at
`stage.sweep_box`, Popo/Nana at the old `callbacks.clear_hitlag` spelling. The last
should migrate directly to existing pre/post-hitlag clearing calls; subsequent
effect pausing already has a catalog/backend path and needs connected acceptance.
Read-only audits identified the exact two-joint effect ABI and native stage-sweep
contract for the next batch. Retain source bodies and implement shared operations
with their actual callback/lifetime boundaries. No whole-fighter or rendered
acceptance claim. Public resource-animation application and compact whole-match
save/load remain unfinished; Regions/Pages is still the broad snapshot owner.
Full project goal remains active.

### Interpreter stage queries and effect ownership checkpoint

Committed locally as `d25ac9faa`; no push. Added synchronous two-joint effects,
independent ECB sweeps and remapped segment queries through the interpreter.
Effect controls now retain physical and owned-program identity; shared hitlag
pause/resume callbacks replace saved authored slots through actual actor APIs.
Popo/Nana directly clear pre/post callbacks instead of an obsolete alias.
No stock fighter delegates or stage gameplay changes. Sweep categories 1/2/3
reject because their stock stage callbacks require a real actor; the independent
query keeps category 5 and other audited actorless classifications.

Client/tool/source-assets build and 12 selected CTests pass. Two-joint effects:
66433 checks/76 cases/24 native comparisons; stage queries: 2062/128/86 native
comparisons and ten VM calls; effect hitlag: 380/35/16 native comparisons and
32 VM helper calls. Existing combat, raw-part, form-feedback and complete
effect-signature regressions pass. Root verified pinned inputs in all seven
console receipts and the current rebuilt-tool census. Controlled geometry,
allocation and package boundaries are documented in
`melee/docs/native_source_stage_effect_checkpoint.md`. No game/GPU/audio launch.

Ten-source preparation still passes Mario/Dr. Mario/Peach/Game & Watch/Zelda.
Correction from the next exact-line audit: Link/Young Link stopped on obsolete
`cue.consume(0)`, Sheik on root `attrs.needle_held_part` in an article callback,
and Popo/Nana on `family.member_id`. The adjacent bone/item-state calls already
had bindings. Retain their bodies and migrate canonical source ownership and
spellings directly. Full fighter behavioral acceptance, public resource-animation
application and compact whole-match save/load remain unfinished. Regions/Pages
is still the broad snapshot owner. Full project goal remains active.

### Interpreter bone, item-state and family checkpoint

Committed locally as `6fa6f67c8`; no push. Existing bone world queries now
retain native physical/program/storage/rig identity and stage XYZ publication
after native matrix sampling. Existing Item state transitions retain actual
actor/program/assets/model through the unchanged native state/revision path.
Added generic family member lookup and shared Sleep-retirement policy bindings.
The current member query is only self/entity-zero; immutable Alternate mapping
and simultaneous-companion activation remain unfinished.

Migrated Link/Young Link's obsolete cue spellings directly to existing article
and reverse cue methods; Sheik's article reads its root parameter through
`program.attrs`. Corrected the preceding checkpoint's misidentified diagnostic
lines. No new compatibility aliases or stock special-handler fallbacks.

Client/tool/source-assets and integration targets build; 12 selected CTests plus
the migrated real-pool presentation regression pass. Grouped direct consoles:
bone queries 1364 checks/80 cases/54 bit-exact native comparisons; Item states
7278/24/513 native comparisons; family queries 303/90/48 native comparisons.
Root independently verified 1706/1675/1696 pinned inputs and the actual tool's
ten-source preparation census. Native math, interpreter actors and native Item
transition code execute; controlled rig/assets/leaves and callback hazards are
documented in `melee/docs/native_source_bone_item_family_checkpoint.md`.
No game/GPU/audio launch or whole-fighter acceptance claim.

The census retains all bodies. Mario/Dr. Mario/Peach/Game & Watch/Zelda prepare;
next exact blockers are held-at-center creation for Link/Young Link, an Item
grounded write for Sheik and recipient raw-bone sampling for Popo/Nana. Read-only
audits identify the existing prepared-Article factory and required retention
boundaries for the next grouped slice. Whole paired gameplay, public resource
animation application, full fighter behavior and compact whole-match save/load
remain required work; Regions/Pages still owns broad snapshots. Goal stays active.


### Interpreter held creation, grounding and recipient-bone pause boundary

The user requested a good pause point. The current grouped implementation adds
prepared held-at-center Article creation, exact native Item grounded/air markers
and foreign recipient raw-bone queries. Canonical Link/Young Link factories and
owned signals now name their exact Article declarations; dead kind attributes
were removed. No stock special-handler fallback or compatibility alias was added.

Client/tool/source-assets builds and 13 selected integration checks pass.
Held creation: 2882 checks/70 cases/65 native argument comparisons/56 VM calls;
Item grounding: 378/62/29 full-Item comparisons/7 VM calls. Recipient query,
regression receipts and all controlled boundaries are recorded in
`melee/docs/native_source_held_ground_bone_checkpoint.md`.

Preparation still passes Mario/Dr. Mario/Peach/Game & Watch/Zelda. Exact next gaps
are Link521/Young525 `visuals.held_item_visible`, Sheik1320 `item.sweep_segment`,
Popo641/Nana671 `targets.motion`. All source bodies remain intact. Subsequent
shared bindings, actual paired roles/member mapping, full fighter behavior,
public resource animation application and compact whole-match save/load remain
unfinished. Regions/Pages still owns broad snapshots. No game/GPU/audio launch.

Resume from this checkpoint rather than repeating completed cohorts. Pause the
full goal and all agents after the coherent local commit; do not push.

Committed locally as `aaa3a4d05`; no push. Final recipient-bone cohort passes
260 checks/26 cases/17 native comparisons/2 query VM calls; expression destination
admission rejects unsupported path/vector/global callback channels. Root verified
1703 connected bone inputs, the actual costume descriptor audit, refreshed target
query/export regressions and the final tool/source census. All agents stopped at
this checkpoint. Full project goal paused at the user's explicit request.

### Interpreter motion, visibility and collision checkpoint

The user resumed the goal. Added recipient motion ID, held-item visibility,
three horizontal-smash settings, native velocity-alignment callback installation,
Item segment sweep, and stage line existence/normal/contact/speed operations.
These are shared primitives; fighter decisions remain authored scripts.
Popo/Nana directly migrate the obsolete landing spelling and fix a local name
that shadowed the engine facing namespace. No source bodies were removed.

Client/tool/source-assets and integration builds pass, as do 16 selected CTests.
Connected native comparisons and independently pinned receipts are recorded in
`melee/docs/native_source_motion_visible_sweep_checkpoint.md`. The selected Item
and map kernels execute unchanged over controlled geometry/asset leaves; this
does not claim whole-stage or full-fighter acceptance. No game/GPU/audio launch.

Ten-source preparation still passes Mario/Dr. Mario/Peach/Game & Watch/Zelda.
Next exact gaps: Link695/Young699 `controls.try_side_special`, Sheik1627
`audio.play_global`, Popo1002/Nana1006 `action.category`. Audits also identify
the adjacent ground specials and generic incapacitated read for the next batch.
Continue from here. Actual paired roles/member mapping, public resource-animation
application, full fighter behavior and compact whole-match save/load remain
unfinished; Regions/Pages still owns broad snapshots. The full goal is active.

Committed locally as `8e75c86bc`; no push. The Item sweep/line cohort passes
1,018 checks / 130 cases / 59 native comparisons / 12 actor VM calls. Root
verified all 1,818 pinned inputs and the refreshed rebuilt-tool census after the
final native joint-prefix correction. Continuing with the next grouped API gaps.

### Interpreter ground specials, action observations and global audio

Added all four ground-special checks, generic action-category/incapacitated
reads (including foreign/stock receivers), and global sound playback with native
instance handles. Existing native kernels remain unchanged. Sheik's two chain
Article references now use explicit immutable root tuning through `program.attrs`.
Public scripting documentation is maintained alongside the implementation.

Client/tool/source-assets and integration targets build; 16 selected CTests pass.
Grouped console evidence: ground controls 36,668 checks / 1,453 native pairs
(676 special pairs); observations 4,916 / 772 pairs / 357 VM calls including
192 receivers; global audio 499 / 25 pairs / nine VM calls through a silent
AX sink. Root independently verified all current pinned receipts. Controlled
boundaries and exact receipts are in
`melee/docs/native_source_ground_action_audio_checkpoint.md`. No game/GPU/audio
launch or full-fighter acceptance claim.

Six retained full sources now prepare: Mario/Dr. Mario/Peach/Game & Watch/Zelda/
Sheik. Exact next gaps: Link707/Young711 `controls.try_guard`, Popo1019/Nana1023
`collision.copy_from`. Read-only audits cover native collision-copy orientation
and remaining simultaneous-companion activation/member mapping. These latter
runtime gaps remain even when scripts prepare. Public resource-animation
application, full fighter behavior and compact whole-match save/load remain
unfinished. Regions/Pages still owns broad snapshots. The full goal is active.

Committed locally as `4b32ba62a`; no push.

### Interpreter guard, collision history and companion fields

Added native guard input, target-to-caller collision-history copying, the
read-only controller-synchronization flag and editable horizontal spawn offset.
Generic physical/source receiver routing preserves native fields and Float
bits. No compatibility aliases, donor fighter handlers or native kernel changes.

Client/tool/source-assets and integration targets build; 16 selected CTests pass.
Grouped native comparisons cover guard selection, exact collision-copy fields,
and real observation Read/Write routes including independent source and stock
receivers. Root verified all connected receipt inputs. Limits and receipts are
in `melee/docs/native_source_guard_collision_checkpoint.md`; no game/GPU/audio
launch or complete fighter acceptance claim. The companion audit was refreshed
to recognize already implemented source CPU hooks and focus on actual remaining
activation/member mapping, constructor history and native continuation gaps.

The retained full sources still prepare for Mario/Dr. Mario/Peach/Game & Watch/
Zelda/Sheik. Next first gaps are Link722/Young726 (old air-jump spelling),
Popo1395 (async joint effect) and Nana1372 (CPU destination preparation).
Continue direct canonical callsite correction and grouped input/Article/CPU
operations. Actual paired gameplay and public resource animation application
remain unfinished; compact whole-match save/load still uses broad Regions/Pages
as its oracle. The goal remains active.

Committed locally as `c95424f81`; no push. Guard/collision/observation receipts
were independently rehashed before commit. Continue from this checkpoint.

### Interpreter catch/taunt input, queued item effects and CPU destinations

Added generic native catch/taunt input checks, owned CPU destination preparation
and scalar controls, and actual Item async effects with immutable primary-joint
paths. Link/Young Link use the existing canonical aerial-jump spelling. Owned
ledge/jump predicates now retain program/storage identity. No native fighter
handlers, compatibility aliases or changed common kernels were introduced.

Client/tool/source-assets and selected targets build; all 16 selected CTests
pass. Grouped console fixtures compare actual bindings/VM actors with retained
native input, CPU and effect-queue bodies. Root independently rehashed every
receipt input. Limits and exact receipts are recorded in
`melee/docs/native_source_input_async_cpu_checkpoint.md`. No game/GPU/audio
launch; full fighter, pairing and rendered-effect acceptance remain open.

Six retained full sources still prepare. Exact next first gaps are Link754 /
Young758 accessory-before, Popo1414 creator helper-call syntax, and Nana1377
target position delta. Accessory-before/hitlag require their real native slots;
they cannot alias accessory/cue. Continue callback-slot integration, direct
canonical source migrations and shared CPU history/planners. Actual companion
activation/member mapping, resource animations and compact whole-match save/load
remain unfinished. Regions/Pages remains the broad snapshot oracle. Goal active.

Committed locally as `37c81e872`; no push. Continuing the next grouped callback,
CPU and target-query integration.

### Interpreter accessory phases, CPU history and target delta

Connected native accessory-before/hitlag slots with owned helper registration,
matching, motion retention and NewLife reset. Added direct CPU input-history
record/apply/pose operations, shared combat/recovery planners and editable
controller-output fields. Target position-delta queries retain live identities
and publish exact Float bits only on success. Popo/Nana creator cleanup uses
the canonical owned helper syntax with an existence gate.

Client/tool/source-assets and selected integration targets build; all 16
selected CTests pass. Grouped comparisons exercise the actual scheduler,
actor/VM paths, native history/planner and target kernels with documented
controlled leaves. Exact final receipts and limits belong to
`melee/docs/native_source_callback_history_target_checkpoint.md`.
No game/GPU/audio process was opened; no paired or full-fighter acceptance claim.

Six full sources continue to prepare. Next diagnostics are Link836/Young840
(position-plus-scalar effect), Popo1468 (ground-following item), Nana1381
(mid-walk attribute). Continue real catch/capture scheduler hooks, item terrain
callbacks and target/stage queries. Companion activation/member mapping,
resource-animation application and compact whole-match save/load remain open;
Regions/Pages remains the broad oracle. The goal remains active.

Committed locally as `50f2b476b`; no push. Final grouped inputs and census rehashed before commit. Continue catch/capture, Item terrain and follower-query integration from this checkpoint.

### Interpreter catch hooks, Item terrain and follower queries

Connected ten typed catch/capture policies at their real common call sites,
including holder-owned caught-state notification and accepted CatchWait release.
Pull/pummel/throw/caught continuations retain both physical participants across
authored callbacks. Added actual Item ground-following/landing kernels and
nested stage hooks, with a narrow entered-air cached-write lifetime correction.
Generic copied-position/scalar effects preserve each native recipe's argument
meaning; Link/Young Link use the canonical scalar API. Item contact reset takes
Bool rather than numeric flags.

Target damage/facing/CPU body width/sleep/floor observations, middle-walk reads,
floor probes, 2D/3D island bounds and CPU hazard/exclusion queries are now in the
interpreter. Common kernels remain direct; fighter-specific decisions stay in
source. No donor-handler aliases or gameplay policy substitutions were added.

Client/tool/source-assets and selected integration targets build; all 16
selected CTests pass. Full-source checking caught a null overload name in
binding lookup; both hidden overload names are now explicitly empty strings,
and Nana reaches a normal unsupported-API diagnostic instead of crashing.
Current receipts, comparison scope and limits belong to
`melee/docs/native_source_catch_terrain_query_checkpoint.md`.

Six retained full sources still prepare. Next first diagnostics: Link846 /
Young850 `capture.mask`, Popo1489 Article `floor.normal.x`, Nana1474
`items.reject_healing_pickup`. Source bodies remain intact. Companion activation,
remaining tether policies, resource animations and whole-fighter gameplay
acceptance remain open. Compact whole-match save/load still uses Regions/Pages
as its broad oracle. Goal active; no game/GPU/audio launch in this batch.

Committed locally as `1e9849431`; no push. Root rehashed all six grouped
comparison receipts and the full-source census against the frozen inputs.
Catch coverage includes 160 native comparisons and 59 retirement/fault cases;
complete tether and whole-fighter gameplay remain separate acceptance work.

### Interpreter tether movement and Article motion

Committed locally as `948cf3ed5`; no push. Connected capture exclusion masks,
damage-fall/ledge-jump phase-two entry and separate ledge probe/occupancy calls.
The actual ordinary, parasol and cliff-jump kernels guard cached Fighter
continuations after authored motion/animation callbacks.

Generic Article collision facing, scaled default ECB reset, ground sliding
with both authored stage hooks and command-stream clearing now execute actual
common kernels. Ground sliding supports physics-only Items without a model.
Article floor normals preserve native Float bits; healing-pickup rejection is
an editable Fighter Bool, including stock/independent receivers. Popo/Nana
attribute copies were removed because source specials read owned parameters;
genuine multijump scratch users retain the common operations.

Client/tool and selected integration targets build; all sixteen selected CTests
pass. Three grouped comparison receipts and the census were independently
rehashed by root. Exact evidence and controlled-leaf limits belong to
`melee/docs/native_source_tether_motion_checkpoint.md`.

Eight full retained sources now prepare: Mario, Dr. Mario, Peach, Game & Watch,
Zelda, Sheik, Popo and Nana. This is preparation, not paired gameplay acceptance.
Link906 / Young910 next require cached collision position observations, then
their real model/descriptor/attachment operations and target queries. No source
body was stripped. Companion activation/member mapping, full-fighter gameplay
and compact whole-match save/load remain open; Regions/Pages remains its broad
oracle. No game/GPU/audio launch in this batch. The goal remains active.

### Interpreter Item controllers, model observations and attachment release

Committed locally as `67954b7b5`; no push. Cached Fighter/Item collision
positions, Item wall/ceiling normals, target scale/guard/shield/camera queries,
native attachment release, dynamic-joint selection and existing-matrix rotation
reads are connected. Link/Young Link retain complete authored source bodies.
Arrow effects preserve native zero-jitter RNG consumption through generic calls.

Native descriptor Set/Add uses prepared controller demand and origins. Texture
matching follows actual HSD resource-ID reassignment; bounded constant TIMG
streams select pinned borrowed images. Genuine Bomb histories begin at
descriptor 0 or 1 and switch to 3, exercising fourteen image transitions per
owner. Interpolated image indices, animated palettes and external shadow/toon
contexts remain outside the admitted subset.

Client/tool/source-assets and selected integration targets build; sixteen
selected CTests pass. Root independently rehashed six focused comparison
receipts and the full-source census. Scope, exact hashes and controlled
fixture boundaries are in
`melee/docs/native_source_target_controller_release_checkpoint.md`.

Eight full sources still prepare: Mario, Dr. Mario, Peach, Game & Watch, Zelda,
Sheik, Popo and Nana. Link2228 / Young2241 next require the reflector identity
observation. Preparation and focused native comparisons are not whole-fighter
or paired-gameplay acceptance. Companion activation/member mapping and compact
whole-match save/load remain open; Regions/Pages remains the broad oracle.
No game/GPU/audio launch. The goal remains active.

### Interpreter reflector, hitlag observations and stage-line admission

Committed locally as `8cec730d6`; no push. The interpreter now exposes the
actual Item reflector relation and physical Fighter receiver `hitlag.active`.
Linked hitlag operations retain owned-source requirements. Invalid Article
line-speed IDs are refused before the native debug loop; valid remapping and
native false/output contracts are unchanged. Companion documentation now
separates supported alternate forms from pending simultaneous companions.

Client/tool/source-assets and selected integration targets build; sixteen
selected CTests pass. Root independently rehashed all three focused receipts
and the full-source census, including 3,292 live resource inputs. Exact hashes,
controlled comparison boundaries and historical geometry fixture ABI drift
are recorded in `melee/docs/native_source_reflector_hitlag_checkpoint.md`.

Eight retained full sources still prepare. Link3301 / Young3373 next require
`model_graphs.install`; their Article source bodies remain intact. Companion
activation/member mapping and whole-fighter gameplay acceptance remain open.
Compact whole-match save/load still uses Regions/Pages as its broad oracle.
No game/GPU/audio launch. Goal active.

### Interpreter reserved model parts and held-item/shield observations

Committed locally as `3faa5864c`; no push. `model_graphs.install` now directly
executes the existing native two-pose installer with exact owned rig/program
admission. Runtime publication copies the prepared program; the binding retains
the published pointer and compares package identity/bank correctly. Held-item
heavy classification and five common shield pushback reads are also connected.
No fighter-specific handler or native installer algorithm was added or changed.

Client/tool/source-assets and selected integration targets build; sixteen
selected CTests pass. Six production inputs retained their hashes across the
final grouped build. Root independently rehashed the two focused comparison
receipts and full-source census. Model insertion has sixteen native/VM pairs;
observations include fourteen heavy cases,356 shield comparisons and306 shield
VM cases. Exact hashes and controlled allocation/metadata/payload boundaries
are in `melee/docs/native_source_model_graph_observation_checkpoint.md`.

Eight retained full sources still prepare. Link3480 / Young3600 next require
`collision.air()` and the following shared tether collision paths. Full source
bodies and typed trail events remain intact. Companion member/constructor/history
and teardown work remains separate; activation is not established. Whole-fighter
and paired gameplay acceptance, and compact whole-match save/load, remain open.
Regions/Pages remains the broad rollback oracle. No game/GPU/audio launch.
Goal active.

### Interpreter tether collision and complete retained-source preparation

Committed locally as `f1062ac79`; no push. This connects the native catch-hit collision operation and
migrates the remaining Link/Young Link air-contact calls to the existing
`collision.landed` update. Owned Landing/Hammer continuations now retain the
exact caller across nested hooks. No geometry policy or stock-special delegation
was added. The complete source bodies, articles and trail events remain intact.

Client/tool/source-assets and selected integration targets build; all sixteen
selected CTests pass. Thirteen production inputs retained their hashes across
the grouped build. Root independently rehashed the focused receipt: 2,338 checks,
28 tether comparisons and70 guard cases across1,713 consumed inputs. Stage,
motion-dispatch and renderer controlled boundaries are explicitly recorded in
`melee/docs/native_source_tether_collision_checkpoint.md`.

All ten retained full sources now pass actual package preparation: Mario,
Dr. Mario, Link, Young Link, Peach, Game & Watch, Zelda, Sheik, Popo and Nana.
This is distinct from activation and full gameplay acceptance. Simultaneous
companion activation remains closed pending member/history, constructor and
teardown integration. Compact whole-match save/load remains unfinished;
Regions/Pages is the broad oracle. No game/GPU/audio launch. Goal active.

### Interpreter companion identity, history and Sleep continuations

Committed locally as `cf60c91c5`; no push. Immutable family member queries now
resolve both Player roles, including swapped and dormant alternate members.
The separate native constructor hint selects already-published member zero
only for simultaneous member one, retaining exact package/program/physical
identity and actor storage when present. The complete native CPU initializer
passes that full-width ID through the script hook and seeds all thirty history
records. Existing family/slot metadata is reused; no second registry was added.

Sleep/death/reset and the alternate-constructor Sleep tail now retain the
caller across nested callbacks before cached continuation. A retired peer
alone permits a healthy primary's match status; a faulted/replaced/reused
primary stops. Successful reset captures the new generation. The complete
constructor and unload ownership paths remain outside this bounded proof.

Client/tool/source-assets and selected integration targets build, with thirteen
stable production inputs and sixteen selected CTests passing. Root independently
rehashed both final focused receipts and the rebuilt ten-source census. Family
proof: 2,370 checks, 143 cases, 312 comparisons, 55 VM calls. Sleep proof: 906
checks, sixty cases, 54 authored VM callbacks and eight foreign callbacks.
Actual kernels/pools/VM and controlled metadata, motion/presentation/reset
boundaries are recorded in
`melee/docs/native_source_companion_identity_sleep_checkpoint.md`. That document
supersedes the pinned audit's historical pre-Sleep status.

Simultaneous companion activation remains closed. Next: correct constructor
failure and partial-unload ownership before admission, preserving publication
of the primary while the secondary initializes. The audit already found early
Load occurs before camera allocation, and unconditional Player role clearing
cannot safely handle unpublished/retired/replaced construction. These require
actual constructor/unload callsite fixes and connected proof, rather than just
opening the preparation gates. Whole paired gameplay and compact save/load
remain unfinished; Regions/Pages is the broad oracle. No game/GPU/audio launch.
Goal active.

Read-only constructor audit: recover Attach/Load/reset/startup failure at the
actual constructor; retain exact GObj/Fighter allocation identities so cleanup
never destroys replacements. Partial unload needs optional camera handling and
spawned-effect teardown before supporting joints are released. GObj destruction
itself must reject recursive destruction during userdata Remove; a Fighter-only
flag cannot prevent nested physical double-free. Change Player's role-clearing
contract and its callsites to clear only an expected original object. Failed
pairs should unwind secondary then primary while preserving callback-created
replacements and primary-before-secondary publication. Existing owned-Item
cleanup is selective, so constructor-child ownership requires its own source
audit before claiming every spawned child is covered. Connected constructor,
Unload, Player and real actor/VM proof should cover partial phases, nested
failure, role replacement, physical reuse and recursive Remove; controlled
asset/render/camera/audio leaves must remain explicit.

### Interpreter constructor failure and partial teardown ownership

Committed locally as `772721389` (actual HSD destruction) and `f8f4457e7`
(Fighter/Player construction, failed-child/resource cleanup and connected proof).
No push. The actual constructor now unwinds its original physical allocations
on Attach/Load/reset/startup failure, while preserving replacements. Player
publishes primary before secondary history initialization, unwinds failed pairs
in reverse order and clears only the expected original entity. Recursive GObj
removal runs userdata hooks once and retains physical ownership through nested
callbacks, deferred deletion and scene rebinding.

Unfinished construction has an actual pooled marker. Failure-only Item cleanup
uses immutable physical creator identity plus current damage ownership, covering
Load-era children across logical life changes while preserving transferred,
unowned and borrowed Items. Retiring constructors cannot spawn another abandoned
child at the ordinary factory admission boundary. Genuine resource cleanup
detaches borrowed rig/part/capture/hitbox pointers before immediate or deferred
graph disposal. Normal selective Item teardown remains unchanged.

Client/content-tool/source-assets and selected integration targets build;
seventeen selected CTests pass with fifteen production/build inputs unchanged
across the build. Eight focused finals and the rebuilt ten-source preparation
census were independently rehashed. Whole constructor proof: 27 cases/1,619
checks; Player: 66/5,805 including all thirty history records; HSD lifecycle:
16/20,734; child Item teardown: 10/889; genuine resource graphs: 12/2,654 with
two failed-constructor cleanup cases. Scratch, Sleep/death and healthy pool
consumers also pass. Exact receipts, kernels and controlled asset/renderer/
camera/audio/payload/factory boundaries are in
`melee/docs/native_source_constructor_ownership_checkpoint.md`.

Simultaneous companion activation remains closed in this checkpoint. A read-only
audit identifies two explicit runtime mode rejections to remove next; existing
member preparation/publication already handles independently owned packages,
CPU profiles and nontransform Player registration. Next: actual companion
package staging/activation/retirement and connected authored startup. Four-port/
eight-actor gameplay, linked-special acceptance and compact whole-match save/load
remain open. Regions/Pages is still the broad oracle. No game/GPU/audio launch.
Goal active.


### Interpreted simultaneous companion activation

Committed locally as `d2ee173f3`; no push. Independently owned companion members
now pass existing family preparation, publication and registration after the
constructor ownership work. No new actor/family registry or merged source owner.
The linked real-ISO family fixture passes 264 checks: full Popo/Nana bytes,
independent assets/banks/CPU data, mixed alternate/companion publication,
secondary failure preservation, leases/history barriers and whole retirement.

A private installed test package ran a hidden, muted, rendered Battlefield
match for 360 synthetic controller samples, exiting normally with eight live
interpreted actors and no faults. Each Nana's authored history target is its own
Popo. All four primary ports have final state/geometry traces. This proves full
source startup and ordinary ticking, not authored linked-special execution:
all eight common-motion actors report zero completed Update bodies. Input-only
playback is not a verified deterministic replay. No default collection changes.

Root rehashed 1,776 inputs. Exact receipts, initial private-manifest/all-CPU
command failures, hashes and limits are in
`melee/docs/native_source_companion_activation_checkpoint.md`.
Next: replace obsolete C-definition checks in source Workshop member/motion/pose
preview with actual selected package and actor lifetime. Linked-special/death/
match-end acceptance, full migration and compact game save/load remain open;
Regions/Pages remains the broad rollback oracle. Goal active.


### Source-owned Workshop member and pose preview

Committed locally as `7b0afcb9d`; no push. Removed obsolete runtime C-definition
checks from Workshop member selection, special-motion scanning and retained pose
cache/release. These paths now use selected source package and actual actor,
program, storage and physical lifetime. Companions select a live member; explicit
dormant recreation remains alternate-only. No UI layout or font changes.

Full client/tool/integration target build passes. An actual hidden/muted rendered
private Workshop smoke opened full Popo/Nana, scrubbed Nana down-air and SpecialN
to frame 11, returned to Popo and then drafts. Screenshot/log evidence is retained.
Both companion meshes remain drawn; selected-only presentation is open. The
updated extracted Action/Demo fixture passes 2,106 checks using real lifetime
header/fixed pools and explicitly controlled actor/package/render leaves. Root
rehashed all 1,905 fixture pins and the actual UI evidence. Exact receipts, hashes,
known font warnings and scope are in
`melee/docs/native_source_workshop_preview_checkpoint.md`.

No task-owned game remains running. Further paired gameplay/alternate callback
acceptance, source migration and compact whole-match snapshots remain open.
Regions/Pages is still the broad correctness oracle. Goal active.


## Interpreted companion specials checkpoint

Local commit `d56dc337f` fixes the full Popo/Nana Gumstrings callback to use its
current owner, with absent-owner cleanup, instead of requiring a held-item
relation. Native traces now include both Player roles. Eight hidden, muted,
rendered stock/source games compare all four grounded special entries over 360
samples each; both actors' action timelines and every emitted Fighter field
match exactly. Ice Shot, Blizzard and Gumstrings visible fields and detailed
Item digests also match. The actual publication fixture passes 264 checks; root
independently rehashed 1,801 current input pins and all sixteen game logs.

Evidence: `melee/docs/native_source_companion_specials_checkpoint.md` and
`melee/tests/native/source_companion_game/README.md`. A private accepted test
store was used; the installed collection was unchanged. No task-owned game
remains running. This is not full pair acceptance: airborne/interrupted moves,
injury, separation, companion death and full match-end remain. Compact
whole-match save/load and remaining stock-source migration retain their scope.
The full goal remains active; no push.

## Interpreted companion capture and results checkpoint

Extended rendered comparisons cover airborne entries for all four specials,
left-facing Belay, edge recovery and stock loss. Actual CPU-driven neutral/up
games then exposed native capture-entry crashes. CDB confirmed that holder
setup required a reciprocal grab link before the victim callback established
it, skipping anchor initialization. Setup now retains the existing peer link
across callbacks while preserving actor/program/storage/lifetime checks.

The tracked longer CPU regression now passes all eight stock/source games:
1,200 input samples each, level-9 Mario, damage in every case and capture in
neutral/up. All emitted Fighter fields and action timelines match; visible
Item fields and detailed digests also match. Independently, the results model
admission gate now uses published source packages rather than absent runtime
C definitions. Hidden/muted realtime win/loss runs draw both interpreted
Popo/Nana actors and exit normally. The installed collection is unchanged.

Evidence and limits are in
`melee/docs/native_source_companion_capture_results_checkpoint.md`.
The additional sixteen non-Kirby fighter examples currently fail fresh ordinary
source preparation; older generated-C completion is not current interpreter
acceptance. Samus still fails independent clone construction. Complete paired
interactions, remaining fighter migration and compact whole-match snapshots
remain open. Regions/Pages remains the broad rollback oracle. Goal active.

### Generic scalar effects and stock source signatures

Added `effects.spawn_async_with_scalar(effect, part, value)` using native joint
attachment mode 3 and copied Float queue semantics. The source/VM/native-queue
fixture passes 759 checks in 67 cases; scalar rendering receivers are controlled.
Falcon/Ganondorf now use that generic operation and correct landing/slope/wind
contracts. Luigi owns its root-reset callback and no longer declares empty
duplicate Missile phase stubs. Yoshi uses explicit Article signals and its
already-owned EggRoll state callbacks. Strict duplicate validation remains.

All four still fail source preparation at subsequent callback/event contracts;
none is claimed admitted or gameplay-complete. Exact diagnostics, source
references, receipt hashes and limits:
`melee/docs/native_source_signature_migration_checkpoint.md`.
Native client/tool/family/package targets build. No installed data changed.

### Interpreted grab registration and owner dispatch

`capture.arm(mask)` now uses the common detector and prepared owner events.
The actual collision scheduler runs `grab_catch` before `grabbed`, retaining
source/victim lifetime checks and passing a checked victim token. Both events
run in the holder's own program/storage; ordinary native grab assignments take
precedence. Optional item-catch dispatch is also installed.

The source/VM/physical-pool/scheduler fixture passes 160 checks in 13 cases,
including stock/scripted victims, stock holders, native takeover, retirement,
invalid masks and absent required events. Collision/item/audio leaves and
package asset metadata are controlled; no full special-move acceptance claim.
Falcon/Ganondorf also use the existing landing API directly and an explicit Int
conversion matching their original command assignment. Fresh source-check now
reaches `capture.begin` for both. Luigi and Yoshi's earlier boundaries remain.

Native client/tool/family/package targets build. No installed content changed.
Evidence, exact hashes and limits:
`melee/docs/native_script_capture_scheduler_checkpoint.md`.
Capture entry/release, remaining fighter migration, paired acceptance and
compact whole-match rollback remain open. Goal active.

### Interpreted capture entry, attachment and throw release

Capture entry, rig attachment, position following, ground-link assignment,
victim observations and throw release are now admitted by the interpreter.
Both grab events receive a retained victim token while running the holder's
own program/storage. Shared engine algorithms moved into a separate capture
source file; direct exporter call sites reuse them. No stock-special forwarding
or deprecated script aliases were added.

The source/VM/physical-pool fixture passes 2,280 checks in 108 cases, including
comparison with the unchanged common Falcon capture-entry body, all entry
options, stock/scripted victims, motion admission and callback retirement.
Rig/animation initialization and damage leaves are controlled; full gameplay
capture acceptance remains open. Native client/content/family/package targets
build. No installed content changed or game windows launched.

Falcon/Ganondorf source checks now pass capture and stop at
`effects.spawn_with_facing`; Luigi/Yoshi's previously recorded boundaries
remain. These fighters are not yet admitted or gameplay-complete. Evidence,
exact receipt hashes and limits:
`melee/docs/native_script_capture_scheduler_checkpoint.md`.
Remaining fighter migrations, paired gameplay acceptance and compact
whole-match rollback snapshots remain in scope. Goal active.

### Generic facing feedback in remaining stock scripts

Nine specialized facing-effect calls now use the existing raw-joint-plus-Float
API with `facing.direction`, without adding a fighter-specific operation.
Source checks for Falcon/Ganondorf pass those calls and now stop at hurtbox
contact target properties. Fox/Falco, Marth/Roy and Donkey Kong's earlier
admission boundaries remain; no new full fighter acceptance is claimed.

The direct effect ABI cohort passes 67,459 checks / 135 cases, comparing every
16-bit effect ID with actual native dispatcher argument use. Effect leaves
remain controlled. Exact current receipts and migration boundaries are in
`melee/docs/native_script_capture_scheduler_checkpoint.md`. Goal active.

### Generic contact receivers and action-metadata queries

The interpreter now exposes checked body-contact `target` receivers, including
read-only Fighter/Item identity, and the canonical action-metadata integer
query/constructor/decoder. Existing capture and callback systems are used
directly; no fighter-specific scripting operation was added. Luigi's taunt
classification runs through the interpreted query in the original counter /
classification / held-item / statistics order.

The combined production API/order batch passes 2,777 checks in 150 cases,
including stock/scripted recipients and retirement. Native client/content /
source-family/source-package build targets pass. Engine leaves are controlled;
full gameplay acceptance remains open. Nine-fighter source checks now reach
Falcon/Ganondorf's old transition syntax and Luigi's charge type conversion;
other admission boundaries remain. Exact receipts and limitations are in
`melee/docs/native_script_capture_scheduler_checkpoint.md`. The full remaining
fighter and compact rollback snapshot work stays in scope. Goal active.

The subsequent source pass replaces Falcon/Ganondorf's four generated-family
transition lookups with explicit `state.enter_at` calls and an editable owned
mask matching stock source. Luigi's charge setup now makes the original C
Float-to-Int conversions explicit. The nine-fighter admission cohort now reaches
Falcon's held-swing hook scheduler, Ganondorf's next binding and Luigi's next
animation helper. No new full fighter admission or gameplay acceptance is
claimed; see the same checkpoint document for source evidence and receipt.

### Held swing and partial-animation interpreter checkpoint

Falcon's held-item swing query and the generic partial-body animation operation
now run through the interpreter's native bindings. Luigi's deferred effect
marker is an ordinary accessory-phase script helper; immediate cues explicitly
spawn and mark their shared effect flag. The existing `effects.spawn` operation
also now routes correctly to the feedback backend.

The production binding/order batch passes 3,307 checks in 202 cases, covering
stock comparisons, owned rigs including omitted bones, missing assets/hooks and
source retirement. Native client/content/source-family/source-package targets
build. Falcon and Ganondorf pass full read-only source preparation; neither is
claimed gameplay-complete by this evidence. Luigi reaches hitbox editing, and
the six other cohort boundaries remain. Receipts, limitations and next gaps are
recorded in `melee/docs/native_script_capture_scheduler_checkpoint.md`.
Installed assets are unchanged. Remaining fighter and compact rollback snapshot
work stays in scope. Goal active.

### Accessory and attack-segment interpreter checkpoint

Luigi now passes full read-only source preparation alongside Falcon and
Ganondorf. Marth/Roy use explicit release accessory helpers and common attack
bookkeeping primitives; their source preparation now reaches the unmigrated
jab-restart scheduler. Yoshi's accessory timing follows native move phases.
Effect-joint Euler XYZ selectors and the existing wind dispatch are available.

The grouped binding checks cover native hitbox damage comparisons, real effect
construction/scheduling/actor restore, and native attack-segment operations.
The native client/content/source test targets build. Exact receipts, controlled
leaves and remaining source boundaries are in
`melee/docs/native_script_capture_scheduler_checkpoint.md`. No new complete
fighter gameplay or whole-match snapshot acceptance is claimed. Installed
content is unchanged; goal remains active.

The following checkpoint migrates the common jab-restart Bool query into
both native follow-up schedulers. Its grouped stock/source decision and
retirement batch passes 3,854 checks in 242 cases; all four native targets
build. Marth/Roy source preparation now reaches shield-start presentation.
Unsupported callback diagnostics name the hook and its source location.
The checkpoint document retains exact evidence and controlled-leaf limits.

### Shield, dynamics wind and secondary-damage interpreter checkpoint

Marth and Roy now pass complete read-only source preparation alongside Falcon,
Ganondorf and Luigi. Their shield presentation and dynamic-bone wind policies
run through generic interpreted hooks at the native scheduler phases. DK's
secondary-damage cleanup is an owned callback; its reset, native takeover and
actor snapshot behavior are included. DK's capsule-state predicate and Yoshi's
facing-ground/ledge call now use existing canonical APIs with stock semantics.

All four native targets build. The grouped scheduler/binding batch passes
4,118 checks in 262 cases. DK next needs live hitbox damage observations and
Yoshi model scale restoration; Fox/Falco's entity schema/assets remain open.
Exact receipts and controlled boundaries are in
`melee/docs/native_script_capture_scheduler_checkpoint.md`. Complete fighter
gameplay and compact whole-match rollback acceptance remain unfinished.
Installed content is unchanged; no game was launched. Goal active.

The generic live hitbox-damage query is also available through interpreted
source, with exact native Float preservation and retained-source guards. Its
expanded grouped hitbox suite passes 11,682 checks in 3,419 cases; all four
native targets build. DK's next source gap is deferred model-effect construction,
while Yoshi's model-scale and Fox/Falco's entity requirements remain open.
The scheduler checkpoint records exact receipts and limits. No new complete
fighter gameplay or aggregate rollback acceptance is claimed.

Generic rendered-root scale controls now support Yoshi's roll scale capture,
squash and cleanup without conflating root SRT with gameplay scale or mapped
TopN. The actual source/VM/native bone suite passes 2,084 checks in 112 cases;
all four native targets build. Yoshi next needs held/attached visibility.
DK's deferred model-effect path remains open: inspection confirms late bone
sampling is required, together with queue lifetime ownership. Immediate root
spawn is not substituted. Exact receipts and limits are in the scheduler
checkpoint; complete fighter and aggregate rollback acceptance remains open.

Held-item visibility now dispatches the interpreted fighter's own hooks at
the native common operation, with callback-retirement guards. The separate
`articles.attached_visible` Bool controls the native attached-slot draw gate.
The grouped scheduler suite passes 4,370 checks in 272 cases, and all four
native targets build. Yoshi's source preparation now reaches its effect
position/scalar call; the stock scalar is an effect attribute, distinct from
model scale. DK's deferred model effects and Fox/Falco's entity requirements
remain open. Exact receipts and controlled-fixture limits are in the scheduler
checkpoint. No full-fighter or aggregate rollback acceptance is claimed.

The common effect-size property is now interpreted independently of model
scale, and Yoshi's egg-break cue uses the existing position/scalar operation.
The production property/VM suite passes 34,918 checks and the native targets
build. Yoshi's next source boundary is grouped material-animation control;
the five previously prepared fighters still pass. Scheduler checkpoint
evidence distinguishes these preparation checks from rendered gameplay and
whole-match rollback acceptance. Installed content remains unchanged.

Generic grouped material-animation frame evaluation and freeze/end-frame
queries now run through interpreted source, preserving the native secondary
draw list and selected-node lifetime. The traversal uses bounded stack storage
and linear identity checks. The common visual batch passes 18,014 checks,
including 24 material comparisons and 26 lifecycle cases; the broader scheduler
batch remains passing and the native targets build. Yoshi's next preparation
gap is resetting the horizontal smash-input age, with its legacy raw-joint
call names and fractional local types corrected. The five previously prepared
fighters still pass. Exact evidence and controlled evaluator limits are in the
scheduler checkpoint; full fighter gameplay and compact aggregate rollback
acceptance remain unfinished. Installed content is unchanged; goal active.

Input-history consumption and native repeat-hit group tags are now available
to interpreted fighters. The property/VM suite passes 39,039 checks; the grouped
hitbox source/actor/VM suite passes 12,661 checks in 3,586 cases. Yoshi's group
predicate matches the stock non-disabled test, and source preparation now
reaches its oriented floor-dust recipe, requiring a generic two-scalar position
effect family. The four native targets build, while the previously passing
source cohort remains intact. Evidence/limits are in the scheduler checkpoint.
No complete fighter gameplay or aggregate rollback acceptance is claimed.

Generic position-plus-two-scalar effect construction is now available to
interpreted fighters, with exact native ABI and immediate pointer-consumption
proof. The all-ID fixture passes 524,631 checks; the owned feedback fixture
passes 1,341. Native targets build and five previously prepared sources remain
passing. Yoshi advances to legacy common-state spellings; DK authored deferred
effects and Fox/Falco entity assets/schema remain open. Exact receipts and
controlled-constructor limitations are in the scheduler checkpoint. No full
fighter gameplay or compact whole-match rollback acceptance is claimed.

Native stage-material friction is now available to Fighter and Article scripts.
The connected reader/VM batch passes 17,408 checks, using exact native material
lookup with controlled tables and stale-source guards. All four targets build;
five source preparations remain passing. Yoshi's old common-state and rotated
effect spellings have been migrated directly, and its next gaps are cached ECB
components and controller feedback. Evidence and runtime limits are recorded
in the scheduler checkpoint. Installed content remains unchanged; goal active.

All four cached Fighter ECB vertices now expose their X/Y coordinates to
interpreted scripts. The property/frontend batch passes 39,619 checks and
all native targets build. Five source preparations remain passing; Yoshi
advances to controller rumble. Exact read-only, float-bit and lifetime evidence
is in the scheduler checkpoint. Full fighter behavior and compact aggregate
rollback acceptance remain open; no installed packages were changed.


Generic Fighter controller rumble now preserves native eligibility, patterns,
duration and request conventions through the interpreted runtime. The connected
world-feedback batch passes 1,752 checks / 512 native comparisons with silent,
controlled hardware leaves. Four native targets build, five source preparations
remain passing, and Yoshi advances to the missing grounded-box edge query.
Evidence and acceptance limits are in the scheduler checkpoint. No installed
packages changed; the goal remains active.


The generic grounded edge-support box query now preserves native facing/mode
selection through the interpreted runtime. The collision/body/frontend batch
passes 1,108 checks with 60 native custom-box comparisons and controlled geometry.
Four native targets build; five source preparations remain passing, and Yoshi
advances to capture-item observations. The scheduler checkpoint records exact
evidence and limits. Installed packages and full gameplay acceptance are unchanged.


All seven shared holder capture tuning fields now have direct interpreter
read/write bindings. The connected capture compiler/actor/VM suite passes
4,932 checks / 405 cases; all four native targets build. Captured-item release,
enclosure admission, full fighter gameplay and compact aggregate rollback are
still open. The scheduler checkpoint records evidence and controlled-fixture
limits. Installed packages remain unchanged and the goal remains active.


Captured native Items now retain script ownership/lifecycle while normal phases
are inert. Shared attach/release guards exact actors/rigs and preserves a target
chosen during destruction. Actual scheduler evidence passes 353 checks / 31
cases; the connected helper fixture passes 2,889 checks / 528 native comparisons
with controlled geometry/statistics leaves. All four native targets build.
Script-facing item/enclosure bindings and full gameplay remain open; exact
evidence is in the scheduler checkpoint. Installed packages are unchanged.


Generic captured-item observation/attach/release now run through the ordinary
interpreter and shared engine path. The connected VM suite passes 3,034 checks /
528 native comparisons; the existing capture/settings suite passes 4,932 checks /
405 cases. Four native targets build and five source preparations stay passing.
Yoshi advances to enclosure settings at line 788; its full gameplay/article
acceptance and compact whole-match rollback remain open. Evidence and controlled
fixture limits are in the scheduler checkpoint. Installed packages are unchanged.


The ordinary interpreter now exposes thirteen enclosure settings and retained
victim visibility/body status. Capture tests pass 6,132 checks / 687 cases;
actual-lifetime Item tests pass 4,074 checks / 544 native comparisons against
original Yoshi/copied-Kirby routines. The enclosure reference retains 19,320
exact controlled samples; five native targets build. Five source preparations
stay passing and Yoshi advances to enclosure release at line 813. Safe release/
entry, caught-shape operations, owned articles, full gameplay acceptance and
compact whole-match rollback remain open. Evidence/limits are in the capture
scheduler checkpoint. Installed content is unchanged and the goal stays active.


Enclosure release now executes in the ordinary interpreter, retaining both
actors through damage/link clearing and common entry. Joint validation occurs
before throw damage and after motion selection; native callers honor failure.
Capture tests pass 7,023 checks / 736 cases, the original enclosure target keeps
19,320 exact controlled samples, Item/feedback tests keep 4,074 checks / 544
comparisons, and five native targets build. Five source preparations remain
passing; Yoshi advances to unowned article creation at line 828. Owned article
integration, caught-shape/constraint hooks, full gameplay acceptance and compact
whole-match rollback remain open. The capture scheduler checkpoint records
exact receipts and controlled-boundary limits. Installed content is unchanged.

Unowned positional article creation now runs through the ordinary interpreter,
selecting the caller's package code/assets independently of NULL gameplay
parents/creator/damage owner. The connected constructor/source-call fixture
passes 12,812 checks / 35 cases / 1,024 reuse cycles; child construction passes
18,937 checks / 27 cases, and state transitions pass 7,278 checks / 24 cases.
All five native targets build. Five source preparations remain passing; Yoshi
advances to its article root-attribute access at line 1108. Actual article assets
and full gameplay acceptance, remaining source integration and compact
whole-match rollback are still open. The capture scheduler checkpoint records
exact receipts and fixture limits. Tests were hidden with no game launch;
installed content is unchanged and the overall goal remains active.

Yoshi's three article definitions now pass ordinary source preparation through
their final callbacks; preparation advances to `shield.start_health` at line
1288. Root tuning uses pinned `program.attrs`, and generic Item operations now
support queued joint-position effects with an optional scalar, native stage
contact response with authored hooks, and the damage-behavior property.
The original collision/effect queue kernels retain their timing and order.
Five native targets build. Item effects pass 72,115 checks / 210 native pairs;
terrain/scalar checks pass 4,967 checks / 944 native pairs, and pinned parameter
checks pass 294 checks / 38 cases. Five previously passing source preparations
remain passing. These controlled backend fixtures do not establish rendered
effects, full stage geometry or completed Yoshi gameplay parity. Exact receipts
and boundaries are in the capture scheduler checkpoint. Tests were hidden, no
game was launched, installed content is unchanged and the goal remains active.

Shared shield/guard data migration adds 23 ordinary interpreter properties:
guard timers, input modes and powershield flags; shield health, light amount,
raw joint and base size; and common shield tuning. Scripts use the existing
native common guard slots, with no fighter-specific extension or C fallback.
The connected guard fixture passes 710 checks / 93 cases / 70 VM calls and
retains 44 native comparisons. Distinct values verify Yoshi/common layout and
native release-input consumption. Five targets build, and five source
preparations remain passing. Yoshi advances to hurtbox definition at line 1294;
its full shield operations/hooks and gameplay validation remain open. The
capture scheduler checkpoint records exact receipts and controlled boundaries;
the scripting guide distinguishes legacy exported features from interpreter
support. No game launched, installed content is unchanged and the goal remains
active. Compact whole-match rollback and other previously open work remain open.

Runtime hurtbox definition is now a generic interpreter operation, using the
shared capsule initializer with owned costume joints, existing slots, height,
grab eligibility and local endpoint/scale values. It allocates nothing and adds
no stock-fighter behavior dependency. The connected fixture passes 5,289 checks /
1,625 cases / 1,557 native comparisons / 1,584 VM calls, including the shield
capsule recipe and storage/type boundaries. Five native targets build; the
read-only source cohort keeps five passing preparations and advances Yoshi to
the missing model-attribute effect operation at line 1299. Complete Yoshi shield
behavior, remaining source migration and compact whole-match rollback stay open.
The capture scheduler checkpoint records receipts and controlled fixture limits;
the combat guide documents the new operation. No game launched or installed
content changed, and the goal remains active.

Deferred raw-joint position effects now run in the interpreter with optional
copied scalar payloads. Yoshi's old model-attribute effect call was replaced by
the generic operation plus `model.effect_scale`: native queue inspection proves
that only the attribute block's first Float was consumed. Joint position remains
sampled at processing time. The connected actor/VM/queue fixture passes 6,291
checks / 1,707 cases / 1,629 native comparisons / 1,659 VM calls, with immediate
and deferred timing, packet order, clearing and allocation failure. Existing
one-/two-joint effect regressions and five native builds pass. The read-only
source cohort advances Yoshi to joint translation at line 1308; full shield
behavior and compact rollback remain open. No game or source-package publication
occurred. Documentation records the generic APIs, exact receipts and controlled
geometry/recipe/allocator limits. The overall goal remains active.

Raw owned-joint translation and writable analog-shield/horizontal-tilt input
ages now run through the interpreter and production routing. Translation uses
the native joint setter; input writes validate their saturated byte range before
narrowing. Yoshi reuses the existing generic costume-pose copy operation instead
of an additional alias. The connected bone fixture passes 3,383 checks / 196
cases / 118 native comparisons / 159 VM helper calls; pose regression passes
1,858 checks / 208 cases. Five native targets build and five source preparations
remain passing. Yoshi advances to shared shield scaling at line 1310. The
scripting guide and capture scheduler checkpoint record exact receipts, the raw
translation versus semantic scale distinction, and controlled rig/topology
limits. No game launch or installed content change occurred. Complete shield
integration and compact whole-match rollback remain open; the goal stays active.

Whole shield-transition scheduling now runs through the interpreter at all six
entry requests and nine common Guard scheduler sites, including alternate
hold/release paths. Scripts receive named native transition modes and the
existing special-hit flag; source faults/retirement stop continuation rather
than selecting stock behavior. The connected guard cohort passes 6,524 checks /
1,728 cases / 1,629 native comparisons / 1,678 VM calls, and all five native
targets build. Five source preparations remain passing; Yoshi still awaits
shield scaling and other guard primitives at line 1310. Documentation records
exact receipts and controlled bookkeeping/drain/rig boundaries; these scheduler
checks do not establish motion or gameplay parity. No game launched or installed
content changed. The overall goal remains active, with full shield integration
and compact whole-match rollback still open.

Nine shared guard operations and shield radius/alpha queries now run in the
ordinary interpreter, with owned rig/actor admission and no fighter-specific
API. Common shield/reflection volumes, depletion, timers, SDI and nested
hold/release requests retain their native kernel behavior. The connected cohort
passes 7,281 checks / 1,910 cases / 1,753 native comparisons / 1,768 VM calls;
five native targets build. Five source preparations remain passing, and Yoshi
advances to powershield input at line 1386. Documentation records pinned native
recipes, fault/retirement/rig replacement checks and controlled fixture limits.
This is not full Yoshi or rendered/gameplay acceptance. Guard input, physics and
collision migration, other package work and compact whole-match rollback remain
open. No game launched or installed content changed; the goal remains active.

The interpreter now supports eleven further guard input/physics/collision
operations, plus roll/spotdodge entry hooks with native ordering. Misleading
roll/spotdodge API names and their script/reference-exporter call sites were
corrected; Yoshi's shield check now uses the original in-shield eligibility.
The connected cohort passes 16,492 checks / 2,857 cases / 2,654 native comparisons /
2,704 VM calls, including grouped input boundaries, ground movement, powershield
priority and retirement checks. Five native targets build; five source
preparations remain passing and Yoshi advances to aerial-jump animation at line
1418. Exact receipts and controlled motion/contact/query boundaries are recorded
in the capture scheduler checkpoint. Full Yoshi package/gameplay acceptance,
remaining fighter source migration and compact whole-match rollback are still
open. Tests were hidden, no game launched or installed content changed, and the
overall goal remains active.

Aerial-jump animation completion, the common fall-animation selector and
knockdown pose reversal now run in the interpreter using shared native rules.
Fall entry retains owned actor/program/storage before cached writes. The
connected cohort passes 17,539 checks / 2,962 cases / 2,744 native comparisons /
2,826 VM calls; all five native targets build. Five source preparations remain
passing, and Yoshi advances to capture enclosure at line 1465. Exact receipts
and controlled motion-initialization limits are recorded in the capture
scheduler checkpoint. Capture hooks, full package/gameplay acceptance and
compact whole-match rollback remain open. No game launched or installed content
changed; the overall goal remains active.

Caught capsule/constraint operations and holder-owned capture setup now run in
the interpreter with retained actor/program/storage and admitted rig mappings.
The connected capture cohort passes 7,585 checks / 778 cases, including six
pinned native capsule comparisons. All five native targets build; five source
preparations remain passing and Yoshi reaches the CPU floor-island query at line
1536. Its remaining catch-pull-frame/throw-victim hooks, owned article and whole
fighter acceptance remain open. The capture checkpoint records exact receipts
and controlled capsule-status/graph leaves. Compact whole-match rollback remains
unfinished. No game launched or installed content changed; the goal is active.


### Interpreted point-to-floor-island query (2026-10-10)

The generic point/island comparison now runs in the ordinary interpreter using
unchanged native vertical probes, island identity and current endpoint-bound
scanning. Distinct Bool/Float destinations publish after retained source and
geometry guards; missing islands publish false/false/zero/zero. Yoshi's outputs
use their correct types. The connected stage cohort passes 7,591 checks / 944
cases / 776 native comparisons / 94 VM calls, with all 1,750 pins independently
rechecked. Five native targets build; five source preparations remain passing.
Yoshi now stops at the old CPU target-position operand on line 1536, column 69,
rather than the island operation itself. CPU target access, remaining capture
hooks, owned articles and full fighter acceptance remain open. Exact receipts
and controlled geometry/observer limits are in
`melee/docs/native_script_stage_island_checkpoint.md`. No game launched or
installed content changed. Compact whole-match rollback remains unfinished;
the goal remains active.


### Interpreted selected CPU target (2026-10-10)

The planner's current selected Fighter is now exposed as a generic creation ID,
with physical scene/data and source/program/storage guards. Yoshi and Ness use
existing generic position/airborne/floor queries; obsolete target projections
and the shared-floor shortcut were removed from reference tooling and current
API documentation. The connected CPU cohort passes 31,213 checks / 1,291 cases /
3,925 native pairs / 1,567 VM calls; the command cohort passes 7,260 checks,
612 queue pairs and 80 selector pairs. All 1,726 connected input pins were
independently rechecked. Five native targets and the test reference exporter
build; a minimal migrated query exports and passes C syntax checking. Full
Yoshi/Ness C export still stops earlier at unrelated Article attributes.
Five source preparations remain passing; Yoshi advances to `cpu.travel_frames`
at line 1574 (corrected from the earlier ranged-query label). Capture hooks, owned articles, complete fighter acceptance and
compact whole-match rollback remain open. Exact receipts and controlled
boundaries are in `melee/docs/native_script_cpu_selected_target_checkpoint.md`.
No game launched or installed content changed; the goal remains active.

### Interpreted grab-frame and throw-victim setup (2026-10-10)

The ordinary interpreter now delivers the existing generic Float
`catch_pull_frame` and Void `throw_victim_setup` hooks from the shared native
capture schedulers. Fractional remapped frames, missing-hook defaults and
holder-owned victim policy are retained. VM faults, source/schema retirement,
peer retirement and changed capture relations stop native continuation.
The connected capture cohort passes 8,142 checks / 813 cases, including 35 new
dispatch cases; all 1,746 input pins were independently rechecked. Five native
targets build, and five full-source preparations remain passing. Yoshi still
stops at `cpu.travel_frames` on line 1574 (corrected from the earlier
ranged-query label). Exact receipts and controlled-leaf
limits are in `melee/docs/native_script_capture_frame_throw_checkpoint.md`.
These are scheduler checks, not full-character gameplay acceptance. No game
launched or installed content changed; compact whole-match rollback remains
unfinished and the goal remains active.

### Interpreted CPU planner fields (2026-10-10)

Five generic native planner fields now run in the ordinary interpreter:
travel count, selected attack command, elapsed count, cached floor availability
and body half-width. Mutable fields require the owned source; body width is
readable through retained references to stock and other authored fighters.
Exact signed writes and read-only fields preserve native state without adding
queries, increments or randomness. The connected CPU cohort passes 31,753
checks / 1,353 cases / 3,973 native field/kernel pairs / 1,603 VM calls; all
1,727 input pins were independently rechecked. The command cohort remains
passing, five native targets build, and five retained-body source preparations
remain passing. Yoshi advances to the floor-only segment query at line 1613.
Earlier line-1574 descriptions were corrected: that gate was `travel_frames`,
not the ranged query. Exact receipts and controlled fixture limits are in
`melee/docs/native_script_cpu_planner_fields_checkpoint.md`. Remaining roster
integration and compact whole-match rollback are unfinished. No game launched
or installed content changed; the goal remains active.

### Composed CPU floor policy and Yoshi source preparation (2026-10-10)

The floor-only CPU blocking check now composes the existing generic floor probe
and stage exclusion query. Yoshi, Bowser and shared-policy reference consumers
use this recipe; the obsolete exporter shortcut and host slot were removed.
No fighter-specific scripting API was added. The connected stage cohort passes
15,917 checks / 1,584 cases / 1,416 native comparisons / 734 VM calls, including
640 pinned original floor-policy comparisons. All 1,753 input pins were rehashed.
Native targets build and both shared scripting-policy CTests pass. Six retained
fighter sources now prepare, including Yoshi's 263 functions / 87 imports /
6,246 instructions, with no bodies omitted. Fox/Falco, Donkey Kong and Bowser
still have separate metadata/binding/conversion gates. Source receipt pins (27)
were independently rechecked. Exact receipts, controlled geometry boundaries
and the two historical C-reference fixtures not run are recorded in
`melee/docs/native_script_cpu_floor_policy_checkpoint.md`. Preparation is not
full-fighter gameplay acceptance. Remaining roster integration and compact
whole-match rollback remain unfinished. No game launched or installed content
changed; the goal remains active.

### Interpreted per-bone hurt status and callback migration (2026-10-10)

Generic `hurtboxes.set_bone` now uses the original first-matching-capsule setter
under retained source/program/rig guards. Native aggregate and position-update
flags are preserved. Bowser and Donkey Kong consumers use named accessory
helpers and existing generic operations; Float locals and Donkey Kong's missing
effect-active latch are corrected from stock source evidence. Connected checks
pass 8,257 checks / 851 cases, including 18 whole-Fighter setter comparisons;
broader combat checks pass 37,048 checks. All 1,747 / 1,678 input pins were
independently rechecked. Five native targets build. Six retained sources still
prepare; Bowser reaches item pickup blocking and Donkey Kong reaches floor
following. Neither fighter is fully admitted yet. Exact receipts, controlled
boundaries and remaining gates are in
`melee/docs/native_script_bone_hurt_status_checkpoint.md`. No game launched or
installed content changed; full-fighter and compact rollback acceptance remain
unfinished, and the goal stays active.


### 2026-10-10 - Interpreted connected floor following

Implemented generic `stage.follow_floor` using Melee's existing signed connected
terrain traversal, with guarded Bool/XYZ publication and origin fallback. Fixed
its invalid PPC stack-matching square-root destinations without replacing its
arithmetic. Migrated pickup-blocking consumers to the existing Boolean property.
Windows native targets and the reference exporter test pass. Stage-query cohort:
21,459 checks / 1,930 cases / 1,746 native pairs / 738 VM calls / 95 negatives.
Six roster sources still prepare successfully; DK advances to generic cue
consumption and Bowser to facing-directed ground/ledge probing. This is backend
query/source preparation progress, not complete fighter gameplay acceptance.
See `melee/docs/native_script_floor_follow_checkpoint.md` for pins and limits.


### 2026-10-10 - Generic interpreted cue consumption and ECB unlock

Added arbitrary Int-selected one-shot `cue.consume(0..7)` and shared
`collision.unlock_ecb()` with exact owned-source admission. Migrated facing
probes to the existing generic direction-taking API and DK Headbutt effects to
existing part-based spawning. Windows targets and reference CTest pass; capture
cohort 18,629 checks / 3,947 cases, stage cohort 21,461 checks, combat cohort 37,048
checks. DK source now reaches dynamic island bounds; Bowser reaches animation
air-motion physics. A cached-bounds composition failed its dynamic native
comparison and was removed; the original request remains until implemented.
See `melee/docs/native_script_generic_cues_checkpoint.md` for limits and receipts.


### 2026-10-10 - Dynamic point-to-island bounds

Implemented the generic interpreted `stage.island_bounds` query using the original
floor/island lookup and moving-endpoint refresh. Actor/state output stores retain
origin fallback and require current source/geometry before publication. Bowser
and Fox/Falco now use the existing plain animation-velocity operation; removed
the dead duplicate C reference host entry and rebuilt its test consumers.
Stage-query cohort: 21,997 checks / 1,970 cases / 1,758 native pairs / 750 VM calls /
114 negatives. Windows targets and reference CTest pass. DK advances to damage
reaction fields; Bowser to item hit-group allocation. Six prior roster sources
still prepare. The older standalone kinetic runner has a stale compile policy
and was not counted as acceptance. See
`melee/docs/native_script_dynamic_island_checkpoint.md` for evidence and limits.


### 2026-10-10 - Common reaction reads and shared collision groups

Added interpreted `damage.reaction_suppressed`, `damage.knockback_level` and
`articles.new_collision_group()` using original shared engine bodies. Consolidated
Bowser's duplicate allocator name into the existing generic article name and
removed its dead exporter/host entry. Owned interpreter cohort: 24,366 checks /
4,475 cases, including reaction thresholds, counter wrap and cross-fighter sharing.
Combat and stage regressions, Windows targets and rebuilt reference CTest pass.
Six prior roster sources still prepare; DK now reaches shared damage entry and
Bowser reaches positional projectile launch. No installed assets changed or game
window opened. These are primitive/source integration checks, not whole-fighter
acceptance. See `melee/docs/native_script_reaction_groups_checkpoint.md` for receipts
and remaining boundaries.


### 2026-10-10 - Interpreted common damage reaction entry

Added `damage.enter(motion, facing)` and `damage.default_reaction(facing)` as
direct common engine operations. The native initializer retains its arithmetic
and callback sequence, now stopping on retired/replaced actors or programs after
initial animation as well as its existing callback boundaries. Owned cohort:
30,254 checks / 4,811 cases, including 316 pinned native reaction comparisons,
compiled helpers and native/interpreted callback retirement. Combat/stage
regressions, Windows targets and reference CTest pass. Six roster sources still
prepare; DK reaches explicit held-item release, which remains to be admitted,
and Bowser still needs positional projectile launch. Corrected the prior docs
note that mistakenly called explicit release available. No installed assets
changed or game window opened. See `melee/docs/native_script_damage_entry_checkpoint.md`
for evidence, controlled leaves and limits.


### 2026-10-10 - Interpreted immediate held-item release

Added generic `items.release_held(vx,vy,vz,multiplier)` through the actual common
release routine, distinct from throw-motion entry. Exact actor/item admission
retains ordinary and authored Dropped callbacks; destruction/same-slot reuse
stop old-item continuation, while faults reject publication. Connected item
cohort: 1,430 checks / 154 cases, including ten new native release comparisons.
Capture/combat regressions, Windows targets and reference CTest pass. Six roster
sources still prepare; DK now reaches `capture.initialize_hold`, Bowser still needs
positional projectile launch. Fixture provider drift was repaired without fake
behavior fallback. Native release transform/ownership/collision leaves remain
controlled, so full release gameplay and whole-fighter activation are not claimed.
No installed assets changed or game window opened. See
`melee/docs/native_script_held_release_checkpoint.md` for receipts and limits.


### 2026-10-10 - Interpreted explicit projectile launch

Added generic world-position and bone-offset projectile launches using the
existing authored projectile constructor. Facing/group defaults retain their
reference semantics; source declarations are resolved before play. Connected
source cohort: 478 checks / 67 cases with fourteen native factory comparisons.
All-factory and positional regressions, seven Windows build targets and reference
CTest pass. The older all-factory fixture's uninitialized creator reference was
diagnosed with hidden CDB and replaced by actual creator-capture bodies. Bowser's
integer counters now use its original decrement/increment clamp decisions; its
source advances to capture-victim motion selection. Six prior roster sources
still prepare; DK needs hold-victim initialization and Fox/Falco retain metadata/asset
gates. Runtime kind-variable selection, whole-fighter/gameplay acceptance and
compact whole-match rollback remain open. No installed assets changed or game
window opened. See `melee/docs/native_script_projectile_launch_checkpoint.md`.

### 2026-10-10 - Interpreted victim capture and hold motion

Added generic attached-victim animation entry and shouldered hold initialization
using the original common engine routines. Motion admission uses the victim's
actual table; both actor lifetimes and reciprocal links remain retained across
authored callbacks. Sixteen pinned native/interpreter pairs preserve flags,
timers, donors, payloads and call order. Capture scheduler: 31,291 checks / 4,874
cases. Seven Windows targets and reference CTest pass; receipt inputs were
independently rehashed. Six earlier roster sources still prepare. DK now reaches
shared throw-tick processing; Bowser reaches attached-throw setup. Earlier DK
gate descriptions were corrected: hold initialization was missing, explicit-speed
state entry was already implemented. Controlled pose/geometry leaves do not prove
whole-fighter gameplay. No installed assets changed or game window opened. See
`melee/docs/native_script_victim_motion_checkpoint.md` for receipts and limits.

### 2026-10-10 - Interpreted shared throw operation group

Added direct interpreted held/thrown/attached-victim motion, facing sync, victim
frame observation, throw cue updates and shared throw movement. The script owns
move decisions; common engine routines retain native flags, timers, callbacks,
release and pause ordering. Removed the sole-use attached-throw inline forwarding
body and reused capture-pair lifetime predicates in native continuations. Full
capture/throw cohort: 34,730 checks / 5,076 cases, including 160 new pinned native /
interpreter comparisons. Seven Windows targets and reference CTest pass; all
receipt inputs were independently rehashed. Six existing roster sources prepare;
DK advances to common action finishing, Bowser to special-capture release. Full
release geometry/DI, gameplay activation and compact rollback remain open.
No installed assets changed or game window opened. See
`melee/docs/native_script_shared_throw_checkpoint.md` for evidence and fixture limits.

### 2026-10-10 - Interpreted common finishing and damage release

Added generic `state.finish()`, `capture.release_damage()` and
`capture.release_damage_fall()` directly through the shared engine routines.
Preserved native ground/air finishing, capture unlink ordering, optional upward
launch and follow-up collision credit; retained actor/program/storage checks
halt script continuation after retirement. Capture suite: 36,102 checks / 5,139
cases, including 63 new cases and 42 healthy native/interpreter comparisons.
Seven Windows targets and reference CTest pass; receipts independently rehash.
DK now advances to animation-facing observation, Bowser to drop-victim release.
Full throw damage/geometry, gameplay activation and compact rollback remain open.
No installed assets changed or game window opened. See
`melee/docs/native_script_action_finish_checkpoint.md` for evidence and limits.

### 2026-10-10 - Interpreted capture drop and animation facing

Added generic `capture.drop_victim()` using the unchanged common unlink and Fall
sequence, plus read-only `facing.animation_direction` through the Fighter
observation module. Preserved attachment-role swaps, model restoration and
native release-position decisions. Capture suite: 38,492 checks / 5,211 cases,
including 72 new cases and 58 pinned native/interpreter drop comparisons.
Seven Windows targets and reference CTest pass; receipts independently rehash.
DK advances to walk entry; Bowser advances to animation-driven ground physics.
Whole-fighter activation, real stage release geometry and compact rollback are
still open. No installed assets changed or game window opened. See
`melee/docs/native_script_drop_facing_checkpoint.md` for evidence and limits.

### 2026-10-10 - Interpreted shared walking and animation-facing physics

Added generic walk entry, velocity classification, animation-rate policy and
walking physics directly through the existing shared engine routines, plus
animation-facing ground motion. Native gameplay bodies remain unchanged.
Capture suite: 50,151 checks / 6,210 cases, including 999 new cases and 983
healthy pinned native/interpreter comparisons. Seven Windows targets and
reference CTest pass; receipt inputs independently rehash. DK now reaches throw
input thresholds; Bowser reaches victim animation-family observation. Motion
entry, pose evaluation and floor material multipliers are controlled fixture
leaves; complete fighter activation and compact rollback remain open. No
installed assets changed or game window opened. See
`melee/docs/native_script_walk_checkpoint.md` for evidence and limits.

### 2026-10-10 - Interpreted movement input and victim animation families

Connected nine generic operations: shared throw-input thresholds, walking /
stopping / turning / platform-drop / jump input predicates, and retained victim
animation-family metadata. Scripts keep the decisions; common native bodies
remain unchanged. Capture suite: 56,183 checks / 6,754 cases, including 544 new
cases, 496 pinned native/interpreter comparisons and 21 exact Float observations.
Seven Windows targets and reference CTest pass; receipt inputs independently
rehash. DK now reaches jump preparation. Bowser exposes an older swapped-actor
callback assumption requiring explicit translation correction and attached
capture initialization. Six previous roster sources still prepare. Complete
fighter activation and compact rollback remain open. No installed assets changed
or game window opened. See `melee/docs/native_script_cargo_input_checkpoint.md`.

### 2026-10-10 - Interpreted shared jump and platform-drop operations

Connected generic jump preparation, short-hop detection, launch, first-frame
jump physics and authored platform-drop entry. Preserved native velocity/input
rules and added retained lifetime checks after platform motion change before
floor/timer updates. Capture suite: 79,237 checks / 8,270 cases, including 1,516
new cases and 1,480 healthy pinned native/interpreter comparisons. Seven Windows
targets and reference CTest pass; receipt inputs independently rehash. DK reaches
damage timer updates; Bowser's callback-owner correction and attached-capture
entry remain open. Audio, air transition clearing/clamping and subsequent air
physics are controlled leaves, so full fighter gameplay is not claimed. Compact
rollback remains open. No installed assets changed or game window opened. See
`melee/docs/native_script_jump_platform_checkpoint.md` for evidence and limits.

### 2026-10-10 - Attached capture and consistent script ownership

Connected generic attached-capture initialization with retained holder/victim
lifetimes. Corrected the C reference callback's owner context and Bowser/Mewtwo
templates. Added a captured-victim freeze observation to preserve Mewtwo's
victim throw flag without reading the holder's flag. Capture suite: 90,000 checks
/ 8,512 cases, including 192 healthy pinned initializer comparisons, 24
retirement cases, 16 entry rejections and 10 freeze observations. Seven Windows
targets and reference CTest pass; final receipts independently rehash. Bowser
now reaches flame article owner-parameter access; six previous roster sources
still prepare. Cleanup/rig/motion/pose/air-transition leaves are controlled, so
full fighter activation and compact rollback remain open. No installed assets
changed or game window opened. See
`melee/docs/native_script_attached_capture_checkpoint.md`.

### 2026-10-10 - Interpreted common damage timer and physics

Connected common damage timer and damage movement without a stock Fighter
handler. Native timer stops after program retirement inside knockback-exit.
Bowser's flame now explicitly reads Fighter-owner parameters through the
existing generic receiver namespace. Capture suite: 99,586 checks / 9,360 cases,
including 828 healthy pinned native comparisons, 4 retirement cases and 16
rejections. Seven Windows targets and reference CTest pass; receipt pins rehash.
DK reaches heavy-item throw input; Bowser reaches article child visibility. Six
previous roster sources still prepare. A supplemental old receiver fixture
fails compilation because its setup is stale; refreshing that fixture and
runtime validation of the flame's parameter path remain open. Normal air
physics and knockback-exit callbacks are controlled; full fighter gameplay and
compact rollback remain open. See
`melee/docs/native_script_damage_tick_checkpoint.md` for evidence and limits.

### 2026-10-10 - Owner parameters and receiver fixture refresh

Repaired the stale strict receiver fixture and verified generic creator,
attached-owner, current-owner and executing-program parameter reads through
production lowering and Execution/VM. Different controlled tables distinguish
the selected receivers before/after reflection; retired tables reject reads
without publishing a value. Receiver suite: 4,444 checks / 17 Execution runs.
Capture regression remains 99,586 checks / 9,360 cases. Seven Windows targets
and reference CTest pass; receipt pins independently rehash. Catalog edits only
name argument fields, preserving values and zero flags. This verifies Bowser's
owner-parameter spelling, not full flame gameplay. Full fighter activation and
compact whole-match rollback remain open. No installed assets changed or game
window opened. See `melee/docs/native_script_receiver_parameters_checkpoint.md`.

### 2026-10-10 - Heavy-item input/entry and current roster audit

Connected generic common heavy-item throw input and the interpreted
`heavy_throw_enter` event, with retained actor/program checks around callback
boundaries. Ground suite: 115,191 checks / 1,846 cases; new cohort: 1,648 cases,
including 1,600 healthy comparisons pinned at `6e985dc6a`. Migrated seven
first-child visibility calls in Bowser/Ness/Pikachu/Pichu to the existing generic
joint interface. DK advances to heavy-item damage delivery, Bowser to vector
normalization. A fresh complete roster preparation audit now has 12 passes out
of 26 forms (Sheik/Nana counted separately, Kirby excluded); the other 14 have
recorded binding, callback or owned asset import gaps. A fresh Samus independent
draft import still fails. This is preparation, not a finished clone count.
Seven Windows targets and reference CTest pass; receipt pins rehash. Heavy-item
placement/release and motion/animation leaves are controlled. Full roster gameplay
acceptance and compact rollback remain open. No game window opened or installed
assets changed. See `melee/docs/native_script_heavy_throw_checkpoint.md`.

### 2026-10-10 - Callback constant ordering and owned accessory integration

Fixed an unsorted binary-search constant entry shared by Link/Young Link and
Sheik/Game & Watch. Full roster preparation now passes 16 of 26 forms, rather
than 12; this remains preparation, not completed-clone gameplay acceptance.
Exposed generic reflector ownership policy with owned source admission; migrated
Mewtwo/Jigglypuff accessory restoration and Rollout root scale to existing generic
APIs, preserving native charge truncation explicitly. Constants: 6,635 checks and
1,052 native comparisons. Connected guard regression: 17,635 checks / 2,983 cases.
Seven Windows targets and reference CTest pass; receipts independently rehash.
Remaining first diagnostics and controlled leaves are recorded in
melee/docs/native_script_callback_integration_checkpoint.md. Full roster gameplay
and compact rollback remain open. No game launch or installed asset change.

### 2026-10-10 - Batch compiler and full-roster source audit

Added read-only native_content_tool source-audit using the actual package and
compiler pipeline. It collects independent metadata/body errors in fixed caller
storage, rewinds failed body scratch, and never publishes an invalid candidate.
Normal source-check/script-build remain first-error paths. One roster pass exposes
37 body failures across 538 visited functions in Jigglypuff/Mewtwo/Bowser, allowing
shared capability batches instead of one-error implementation cycles. Structural
asset/schema failures explicitly stop dependent stages; raw repeated metadata
rows are not independent task counts. DK's missing scheduler remains visible
after its bodies pass. Source preparation remains 16/26, not gameplay acceptance.
Connected regression: 115,211 checks / 1,852 cases; seven Windows targets and
reference CTest pass; final input receipts independently rehash. See
melee/docs/native_script_multi_error_audit.md for the grouped inventory and limits.
Continue capability cohorts, then whole-fighter/paired gameplay and compact
rollback acceptance. Goal active; no game launch, user draft or installed changes.

### 2026-10-10 - Vector and Item collision-box integration cohort

Added ordinary interpreted 3D/XY normalization using shared native math and
generic Float input/output destinations, plus Item source-box edge properties.
Migrated all three Jigglypuff Fighter hitbox queries to the existing state API.
Grouped numerical/property regressions and a fresh roster audit pass: all 164
Bowser bodies now compile, exposing the separate throw_victim_motion scheduler
gap. Preparation remains 16/26; remaining first-per-body diagnostics are 34.
Seven Windows targets and reference CTest pass. No game window or installed
content changes. See melee/docs/native_script_vector_ecb_checkpoint.md for exact
evidence and boundaries. Continue grouping shared capability and scheduler work;
goal remains active and full gameplay/rollback acceptance remains outstanding.

### 2026-10-10 - Shared throw, heavy-item and dynamics scheduling batch

Implemented six generic prepared hooks together: victim throw-motion mapping,
throw animation/completion, heavy pickup/completion and damage response, and
secondary-bone floor policy. Donkey Kong and Bowser now pass complete package
preparation; the fresh roster audit admits 18/26 forms, not completed gameplay
clones. Catch/throw/heavy cohort: 9,187 checks / 224 native comparisons / 74
retirement or VM fault cases. Dynamics cohort: 2,808 checks / 253 cases / 120
native comparisons. Seven Windows targets and reference CTest pass; final
receipts independently rehash. Shared inner motion, item classification and
numerical solver leaves remain controlled. No game launch or installed-content
change. See melee/docs/native_script_throw_heavy_scheduler_checkpoint.md.
Continue grouping the recorded presentation, multijump, capture/receiver and
owned-resource work. Full gameplay/paired acceptance and compact rollback remain
open; goal active.

### 2026-10-10 - Grouped presentation and engine-policy integration

Implemented four generic ordinary interpreter operations: attack-color cleanup,
capture eligibility, hip/root animation compensation and top-blast KO policy.
Migrated Jigglypuff/Mewtwo templates together to existing effect, motion, callback,
article and owner-observation APIs, retaining native types/flags and arithmetic.
Fresh full-roster audit reports 34 -> 11 combined first-per-body diagnostics
(Jigglypuff 23 -> 10, Mewtwo 11 -> 1); complete package preparation remains 18/26,
not finished-clone gameplay acceptance. Common visuals: 18,184 checks / 2,265
native comparisons. Connected observations: 17,723 checks / 1,443 cases / 2,849
native pairs. Seven Windows targets and reference CTest pass; receipts rehash.
No game launch, installed content or user draft changes. See
melee/docs/native_script_presentation_policy_checkpoint.md for source semantics,
controlled leaves, evidence and remaining grouped capabilities. Continue grouped
resource/scheduler work, whole-fighter/paired acceptance and compact rollback.
Goal active.



### 2026-10-10 - Common idle/statistics/jump-sound scheduling cohort

Connected three ordinary interpreted Fighter hooks at four shared call sites:
idle variant policy, angled-motion statistics and worn-item jump sound in
ItemScrew and native multiple-jump entry. Native continuation retains source
lifetime and cached jump attributes across callbacks. Corrected Mewtwo's last
body to sample its grandchild world position and use the existing positional
effect operation, plus native explosion charge bounds. The roster now prepares
19/26 forms; Mewtwo's 182 bodies and package pass, not whole-fighter gameplay.
Scheduler checks: 4,233 checks / 388 native comparisons / 32 fault-retirement
cases. Seven Windows targets and reference CTest pass; receipts rehash.
No game window, audio, installed package or user-draft changes. See
melee/docs/native_script_common_policy_checkpoint.md for exact source, controlled
leaves and remaining acceptance. Continue grouped capabilities/resources,
whole-fighter/paired behavior and compact rollback. Goal active.

### 2026-10-10 - Ordinary part-animation control cohort

Connected generic part-animation selection/show/hide to shared native routines,
with owned costume/clip/rig validation before restoration and retained source,
program, storage and import identities before output. Selection is separate
from application; hiding preserves the saved choice. Jigglypuff's default
death visibility reset now explicitly selects native variant zero.
Connected regression: 99,711 checks / 9,382 cases, with inner pose restoration
leaves controlled; existing visuals regression: 18,184 checks / 2,265 native
comparisons. Fresh roster preparation remains 19/26 forms and Jigglypuff's
first-per-body diagnostics fall 10 -> 8. Attachments, hold-category queries,
collision/multijump policies and other forms' prerequisites remain open.
Seven Windows targets and reference CTest pass; receipts rehash. No game,
audio, installed content or draft changes. See
melee/docs/native_script_part_animation_checkpoint.md for evidence and limits.
Continue grouped capability/resource work, complete fighter/paired acceptance
and compact rollback. Goal active.


### Interpreted held-item category checkpoint (2026-10-10)

- Connected generic `items.hold_kind` to the actual native Item getter, distinct from item identity and use category. No generated C or per-tick allocation is needed.
- Grouped compiler/actor/VM Item-control fixture passes 1,524 checks / 166 cases, including all eight hold categories and existing pickup/drop/release coverage. Read-only 26-form audit remains 19 prepared; Jigglypuff failing bodies decrease from 8 to 6. Preparation is not complete gameplay acceptance.
- Seven Windows targets build; reference script CTest passes. No visible/audible launches or installed-content changes. Evidence and boundaries: `melee/docs/native_script_hold_category_checkpoint.md`.
- Next grouped work: costume attachment creation/dynamics/cleanup and collision-box/multijump policies. Goal remains active.


### Interpreted multi-jump and collision-box checkpoint (2026-10-10)

- Connected ordinary multi-jump configuration/impulse queries, common drift threshold, bounded air physics with fastfall, and direction-sensitive air collision boxes to their shared native engine routines. Editable tuning remains owned by the fighter. No generated module or per-tick allocation.
- Connected cohort passes 101,698 checks / 9,563 cases (181 new cases); guard regression passes 17,635 checks / 2,983 cases. Seven Windows targets build and reference script CTest passes.
- Real read-only audit remains 19/26 forms prepared; Jigglypuff has 3 reported failing bodies rather than 6. The collision body now reaches a missing oriented-effect operation; accessory creation/dynamics/removal are still pending. Preparation is not complete gameplay acceptance.
- Evidence: `melee/docs/native_script_multi_jump_checkpoint.md`. No visible/audible game launches or installed-content changes. Goal remains active.


### Interpreted costume attachment checkpoint (2026-10-10)

- Connected costume root lookup, model loading, cloth setup and cleanup to the ordinary interpreter. Allocation is restricted to initial fighter load; selected skins use their own assets. Rollout now uses the already available generic position/two-scalar effect call.
- Grouped connected cohort passes 101,962 checks / 9,598 cases; seven Windows targets build and reference script CTest passes. DAT root lookup and native attachment lifecycle execute; model import/construction, visibility/material and cloth allocator leaves are controlled. Rendered costume/cloth parity remains unverified.
- Read-only roster audit remains 19/26 forms prepared. All 192 Jigglypuff script bodies now compile; package admission reaches the unmigrated shield-break scheduler. This is not complete fighter or gameplay acceptance.
- Evidence: `melee/docs/native_script_attachment_checkpoint.md`. No visible/audible game launches or installed-content changes. Goal remains active.

### Interpreted shield-break checkpoint (2026-10-10)

- Connected the shared shield-break entry to owned synchronous hooks, preserving common launch and feedback while retaining the actor/program across callback boundaries. Jigglypuff's top-blast policy uses the existing generic damage flag; no stock handler delegation or fighter-specific API was added.
- Connected cohort passes 102,372 checks / 9,615 cases, including 17 new native-versus-interpreter scheduling and retirement cases. Common motion/effect/audio leaves are controlled; rendered shield-break and whole-fighter parity remain unverified.
- Read-only roster preparation rises to 20/26 forms. Jigglypuff passes all 192 bodies and package admission. Fox, Falco, Pikachu, Pichu, Samus and Ness remain preparation targets; Kirby stays deferred. Preparation is distinct from complete gameplay acceptance.
- Seven Windows targets and reference script CTest pass; receipt pins independently rehash. Evidence: `melee/docs/native_script_shield_break_checkpoint.md`. Goal remains active.

### Owned parameter import and Jigglypuff runtime checkpoint (2026-10-10)

- Fixed the shared CLI/Workshop importer to retain scoped typed literal parameter declarations alongside editable overrides. It previously stripped declarations needed by interpreter indexing. Converted Jigglypuff prepares all 192 bodies with zero DAT tuning imports; no generated DLL is required.
- Hidden/muted stock-versus-interpreter batch: both five-air-jump facings and worn Bunny Hood match exact traces over 900 samples each, with coverage checks. Blue-hat Rollout completes 1,200 samples without script faults but differs in its accessory digest; it remains unaccepted pending investigation.
- Seven Windows targets build; script/parameter CTests and connected lifecycle/import cohorts pass. Fresh preparation remains 20/26 forms. Normal installed content and user drafts are unchanged.
- Evidence and the initial health-assertion correction are documented in `melee/docs/native_script_owned_parameter_checkpoint.md`. Continue accessory parity, remaining six forms' preparation and full clone/compact-rollback acceptance. Goal active.

### Owned attachment render checkpoint (2026-10-10)

- Fixed a missing shared integration: owned accessory draw and visibility helpers had no callers. Both common fighter rendering paths and part-visibility changes now process authored accessories at the original accessory callback points. No stock delegation, cloth-math change or per-frame allocation.
- Hidden/muted ordinary-source comparisons pass both five-jump facings, worn Bunny Hood and blue-hat Rollout. A separate batch passes all five Jigglypuff costumes over 1,200 samples each, including required accessory rendering, cloth-hat pose digests, contact damage and RNG. The previous blue-hat mismatch is resolved for this recorded sequence.
- Seven Windows targets build; script/parameter CTests pass. Independent evidence rehashes 21 and 34 input pins across the two batches, with the later helper-workflow migration explicitly documented. Normal installed content and user drafts are unchanged.
- Evidence and limits: `melee/docs/native_script_attachment_render_checkpoint.md`. Roster preparation remains at the prior 20/26 checkpoint; full fighter and compact rollback acceptance remain open. Goal active.

### Ordinary interpreter constant tables (2026-10-10)

- Added generic immutable Bool/Int/Uint/Float tables in root and entity scopes. Tables are evaluated into a fixed program pool, shared across actors and read through checked compiled IDs. Local scope shadowing and explicit `program.table` reads are supported; there is no per-tick allocation or added mutable actor state.
- Connected cohort passes 102,541 checks / 9,661 cases, including 46 table cases. Checks cover actual actor/VM reads, exact scalar types, scope, bounds, immutable/nested-index rejection and fixed pool capacities. Seven Windows targets and script/parameter CTests pass.
- Converted isolated Fox/Falco drafts now pass metadata and reach all 257/255 bodies. Each still reports 47 body failures; neither is admitted. Existing shared import prunes unrelated variant articles. Next batch has an operation inventory covering accessory callbacks, typed signals, factories, presentation/physics and item/receiver queries.
- Evidence and limits: `melee/docs/native_script_table_checkpoint.md`; scripting reference: `melee/docs/scripting/tables.md`. Full fighter and compact rollback acceptance remain open. Goal active.

### Fox/Falco interpreted accessory callbacks (2026-10-10)

- Replaced old move-slot accessory assignments with the existing generic named-helper API. Charge/travel, reflector and side-dash effects retain their original bone lookup and self-clear behavior; blaster callbacks bind the authored firing helper directly. Side-B and neutral-B startup no longer automatically arm callbacks that stock Melee installs only later.
- Fresh isolated owned-source audits visit all 256 Fox / 254 Falco bodies and pass metadata. Each reports 32 body failures, down from 47; neither package is admitted yet. Remaining errors are grouped around article factories/signals, presentation/physics, stage calls and item/receiver operations. Each body still reports only its first error.
- Connected interpreter cohort passes 102,551 checks / 9,661 cases, including nongated accessory execution, native-callback takeover, self-clear and rearming. Asset/effect leaves remain controlled; no full Fox/Falco gameplay acceptance. No native production C changes or visible/audible launches.
- Final audit and cohort receipt inputs independently rehash. Evidence and historical generated-C distinction: `melee/docs/native_script_fox_falco_callback_checkpoint.md`. Normal installed packages and user drafts unchanged. Full fighter and compact rollback acceptance remain open. Goal active.

### Interpreted presentation/physics observations (2026-10-10)

- Added generic root Euler-Y, plain animation-Y velocity and read-only reflector contact direction APIs. Corrected the misleading native getter name and direct callers: the former `ftPartGetRotZ` reads Y, and its body is unchanged. Fox/Ness scripts and the reference exporter now use the correct Y name; an older Ness oracle stub's Z assumption was corrected explicitly.
- Fox/Falco ground animation velocity uses the existing exact stock path, and two Float timing/rotation locals no longer initialize as Ints. Fresh audits pass metadata and visit all 256/254 bodies, with 27 reported failures per form (previously 32). Neither is admitted yet; remaining article/effect/stage/item/receiver groups are inventoried across all bodies.
- Root, air-physics and guard cohorts pass: 180 checks / 20 native comparisons; 4,295 checks / 86 cases / 368 native pairs; 17,692 checks / 2,994 cases / 2,767 native pairs. Controlled assets/routing and unavailable leaves are documented. Initial stale-fixture link failures were repaired and retained separately.
- Seven Windows targets and script/parameter CTests pass; four receipts independently rehash 1,711 / 1,724 / 1,753 / 19 pins. Evidence: `melee/docs/native_script_presentation_physics_checkpoint.md`. Console-only checks; installed content and drafts unchanged. Full fighter and compact rollback acceptance remain open. Goal active.

### Fox/Falco explicit owned article sources (2026-10-10)

- Replaced the shared template with separate Fox/Falco sources and updated Workshop, shipping and comparison call sites directly. Each declares only its own gun, laser and trail. Factories and four-argument signals name those declarations; CPU, held-item, radio and sound choices are editable source-owned parameters rather than imported kind selectors. No new native APIs or compatibility layer.
- Side-special construction uses the existing positional factory and explicit ground/air creation value. Its authored initialization follows the stock lifetime/state/scale/debug/physics setup. Full rendered behavior remains pending interpreter package admission.
- Final isolated audits visit all 256 Fox / 253 Falco bodies, pass metadata and report 16 failing bodies each, down from 27. Neither package is admitted. Remaining errors are grouped around effects/wind, stage calls, bone queries and item/receiver operations; each body still reports only its first error.
- Generic factory and signal cohorts pass 67 cases / 478 checks and 195 checks. Seven Windows targets and script/parameter CTests pass. Stale fixture dependency failures are retained separately; final receipts independently rehash. Evidence: `melee/docs/native_script_fox_falco_articles_checkpoint.md`. Console-only work, installed content and drafts unchanged. Full fighter and compact rollback acceptance remain open. Goal active.

### Generic Item launch and segment sweep checkpoint (2026-10-10)

- Added reusable owned-Item launch angle/speed/origin and reflection-facing observations, previous-position segment stage sweep and pending hitlag-request clearing. Corrected Fox/Falco receiver paths directly; gun state 10 now copies the creator's root Y to joint XYZ without altering Item/collision scale.
- Grouped console matrix passes 9,384 checks / 1,428 cases / 1,137 native payload comparisons / 121 VM calls. Stage geometry and actor publication remain controlled; unrelated fixture leaves abort if reached. Seven Windows targets and script/parameter CTests pass.
- Fresh audits visit all 256 Fox / 253 Falco bodies, with complete metadata/traversal and 9 failures each (previously 16). Neither fighter is admitted; remaining effect/wind/stage/bone/model groups remain inventoried, with first failure reported per body. Evidence: `melee/docs/native_script_item_launch_sweep_checkpoint.md`. Console-only work; installed content and drafts unchanged. Full fighter and compact rollback acceptance remain open. Goal active.

### Scripted throw geometry and generic Item scalar effects (2026-10-10)

- Fox/Falco throw-shot angles now compose mapped bone world-position queries and `atan2` directly in scripts. Fire-bounce effects use the existing position/scalar operation; the gun muzzle uses a new generic owned-Item position/scalar operation. Ness's same existing call spelling was corrected without claiming full Ness acceptance. No fighter-specific binding, wrapper or fallback.
- Grouped checks pass: Item actor/VM feedback 2,223 checks / 212 spawn calls; source-derived effect ABI 524,631 checks / all 65,536 IDs / 48 native helper-copy cases; Fighter actor/VM geometry 3,815 checks / 232 cases / 154 native comparisons / 195 VM calls (36 new throw-angle comparisons). Source publication/geometry/render leaves and rejected unselected operations remain documented boundaries.
- Seven Windows targets and script/parameter CTests pass. Final audits visit all 256 Fox / 253 Falco bodies with complete metadata/traversal and 6 failures each (previously 9). Neither admitted; remaining first failures group into air jump, stage conversations and secondary model duplication. Evidence: `melee/docs/native_script_position_scalar_checkpoint.md`. Installed packages/drafts unchanged; full fighter and compact rollback acceptance remain open. Goal active.

### Generic stage events and action statistics (2026-10-10)

- Fox/Falco conversation decisions now use generic native stage channels and common action-statistics operations, with script-owned temporary death callback installation/cancellation. Reflector jump calls use the existing aerial-jump primitive directly. The down-D-pad timer is accurately named as press age; the original trigger condition is preserved. No stock special-handler delegation or compatibility aliases.
- Stage and observation cohorts pass: 22,330 checks / 2,053 cases and 20,463 checks / 2,441 cases. Actual event dispatcher and counter reader are exercised; stage providers/player tables/taunt notification remain controlled. Seven Windows targets and script/parameter CTests pass. No game launch or installed-content/draft changes.
- Fresh isolated audits visit all 256 Fox / 253 Falco bodies with complete metadata/body traversal and one failure each (previously six): trail model duplication. Neither fighter is admitted; later errors within that remaining body may still appear.
- Three receipts independently rehash 1,762 / 1,728 / 20 inputs, with binaries/shipping sources/build/test logs verified. Evidence and limits: `melee/docs/native_script_conversation_checkpoint.md`. Full fighter and compact rollback acceptance remain open. Goal active.

### Independent Article model cloning (2026-10-10)

- Added generic `item.clone_model(slot)` through the actual owned descriptor/scene pool. Clones copy primary rotation and uniform Item scale, with independent default graph state, guarded lifetimes and no gameplay heap allocation. Replaced the oversized 1,536-graph reservation with a shared bounded 128-graph capacity; exhaustion preserves existing slots. No stock special delegation.
- Actual-library Link/Fox/Falco fixtures pass: 630/96/96 checks, 13/6/6 cases and 137/4/4 constructed graphs. Covers simultaneous independent clones, matrices/teardown, stale generations, capacity exhaustion and replacement. Private transition tables/minimal Article sources are controlled; no rendered/full-match acceptance. Seven Windows targets and script/parameter CTests pass.
- All 256 Fox and 253 Falco bodies compile in fresh audits with complete metadata/body traversal. Neither package is admitted: the first remaining package gate is the unmigrated `wait_input` scheduler. Final receipts independently rehash 3,119 and 20 inputs, with binaries/shipping sources/logs verified. Evidence: `melee/docs/native_script_primary_model_checkpoint.md`. No game launch or installed-content/draft changes. Full fighter and compact rollback acceptance remain open. Goal active.

### Interpreted input/status hooks and Fox/Falco publication (2026-10-10)

- Migrated generic Bool `wait_input` and `action_status` hooks at their actual engine call sites, preserving common input ordering/state shortcuts and guarded continuation. Absent source hooks never inherit stock Fox/native authored decisions. No special-specific primitive, wrapper or compatibility alias.
- Full Fox/Falco sources now prepare with zero diagnostics: all 256/253 functions and complete metadata/body traversal. Both source packages save, verify, stage and publish through the actual runtime, each retaining three owned articles and one FireTravel effect. Connected gameplay acceptance remains pending; earlier generated-C comparisons are historical.
- Connected scheduler cohort passes 3,001 checks (Bool/order/absent/fault/retirement/native routes); real source publication passes 43 checks per fighter including lease and initialized page-history retirement gates. Seven Windows targets and script/parameter CTests pass. Controlled input/trace leaves and actor-free publication limits are documented. Five receipts independently rehash; shipping sources, binaries and logs verified. Evidence: `melee/docs/native_script_input_status_checkpoint.md`. No game launch or normal installed-content/draft changes. Full fighter and compact rollback acceptance remain open. Goal active.

### Fox/Falco connected interpreter gameplay (2026-10-10)

- Complete independent Fox/Falco source packages now pass twenty native-stock versus interpreter gameplay comparisons: both facing directions for blaster, ground side special and radio taunts, plus ground/air up special and reflector hold/turn/landing cases. Exact actions, fighter/item geometry and hit state, damage and RNG match for these recordings. Console/pixel-complete VFX, untested branches and compact rollback acceptance remain open.
- Fixed prepared Article declaration identity when names also denote stock ItemKind constants, corrected stock-versus-script secondary trail graph inspection in replay hashes, and extended generic named joint-model effects to attach at a raw rig part with interpreted callbacks and validated packed restore. No fighter-specific API, stock-handler delegation or new gameplay allocation. Added single-sample geometry diagnostics.
- Grouped factory/effect/restore regressions and seven Windows targets pass; script/parameter CTests pass 2/2. Seven final receipts independently rehash their inputs; tests use isolated content stores and hidden/muted launches. Evidence and limits: `melee/docs/native_script_fox_falco_gameplay_checkpoint.md`. Remaining fighter groups continue; goal active.

### Pikachu/Pichu interpreted source preparation (2026-10-10)

- Separated the full sources into their own three-article templates; changed Workshop selection, shipping inputs and comparison source consumers directly and removed the combined template. Corrected declaration names, explicit owner parameter reads and typed effect/RNG/costume/ledge/charge callsites together. Added generic `physics.clear_kinetics()` through the original common engine helper, without callbacks, allocation or stock-special delegation.
- Per-fighter failing-body count falls 35 -> 21 across all 209 visited bodies. Remaining failures cluster around linked-article relations, terrain/bone observations and typed action-category filtering. Both packages remain rejected; no interpreter gameplay acceptance is claimed. Linked-article docs explicitly mark the current migration gap.
- Movement regression passes 26,094 checks / 7,104 native-body comparisons / 91 double-sensitive cases; seven Windows targets and script/parameter CTests (2/2) pass. Fresh roster prepares 22/26 forms excluding Kirby, with Ness animation-resource metadata and Samus independent-draft construction also remaining. Preparation is not full gameplay acceptance. Two receipts independently rehash 182/1,658 pins; shipped templates match. Evidence: `melee/docs/native_script_pikachu_split_checkpoint.md`. No visible/audible launch or normal content/draft change. Goal active.

### Generic interpreted article links and signals (2026-10-10)

- Added owned one-way linking, full-width link IDs, reciprocal checks and callback-free unlinking, including Destroyed cleanup after native removal is marked. Existing Article signal preparation/runtime now also supports Article callers; Pikachu/Pichu callsites directly compose link IDs and exact declared signals. No fighter-specific API, legacy alias, allocation or stock-handler delegation.
- Grouped real actor/compiler/VM fixture passes 441 checks, including all 16 slot/backlink combinations against extracted existing native link bodies, reflection, ownership, stale IDs, address reuse and nested signal deletion/fault rules. Seven Windows targets and script/parameter CTests (2/2) pass; receipts independently rehash 1,702/189 pins and shipping templates match. No visible/audible launch or normal content/draft changes.
- Pikachu/Pichu failing bodies fall 21 -> 9 each across 209 visited bodies; packages remain rejected pending linked creation/observations, terrain/bone reads and action-category filtering. Fresh roster remains 22/26 preparation passes, with Ness resource metadata and Samus draft construction also open. Full gameplay/compact rollback acceptance is not implied. Evidence: `melee/docs/native_script_article_links_checkpoint.md`. Goal active.

### Generic linked article creation and terrain observations (2026-10-10)

- Added exact-declaration `item.create_linked`, establishing both links before Created through the existing authored child constructor, plus normal-producing stage sweeps, mutable article surface normals and native ground checks. Reflection retains original creator/control while transferring damage credit. Pikachu/Pichu use these generic operations directly; no fighter-specific primitive, compatibility alias, gameplay allocation or stock-handler delegation.
- Grouped constructor/actor/VM checks pass 19,369 checks / 48 cases, including sixteen slot combinations, occupied/stale slots, allocation failure, reflection and callback deletion; observation checks pass 9,626 checks / 1,141 native-body comparisons, and nested link/signal regression passes 441 checks. Seven Windows targets and script/parameter CTests (2/2) pass. Four receipts independently rehash; shipping sources and constructor/build inputs verified. No game launch or normal content/draft changes.
- Pikachu/Pichu failing bodies fall 9 -> 5 each across all 209 visited bodies. Roster remains 22/26 preparation passes. Linked model/collision and typed Fighter observations, Ness resource metadata and Samus independent-draft construction remain; full gameplay and compact rollback acceptance are still open. Evidence: `melee/docs/native_script_linked_creation_checkpoint.md`. Goal active.

### Generic interpreted article geometry (2026-10-10)

- Added read-only position/normal fields to exact declared Article references, indexed-bone world-position queries through the native helper, and complete linked collision-state copying. Fixed scratch and retained actor/table/parent-chain validation; no fighter-specific API, compatibility alias, allocation or stock-special delegation. Pikachu/Pichu use existing dynamic-joint rotations and action categories directly; stock Thunder quake/rumble decisions now live in an authored fighter helper using general engine primitives.
- Grouped real reference/compiler/VM, compiler-routing and link/collision fixtures pass 176/109/465 checks. Seven Windows targets and script/parameter CTests (2/2) pass. Four final receipts independently rehash; shipping templates and production/build inputs verified. Controlled asset/class/matrix-setup leaves limit this evidence to engine/VM contracts, not whole-fighter gameplay. No game launch or normal content/draft changes.
- Pikachu/Pichu now compile all 210 bodies each without body diagnostics, but package preparation still rejects an unmigrated auxiliary-item scheduler hook. Roster remains 22/26 preparation passes. Next: migrate actual scheduler callsites, then connected gameplay comparisons; Ness metadata, Samus draft construction and compact rollback acceptance remain open. Evidence: `melee/docs/native_script_article_geometry_checkpoint.md`. Goal active.

### Interpreted auxiliary-item and jab schedulers (2026-10-10)

- Connected both auxiliary-item hooks and integer jab-start flags at actual shared engine callsites. Preserve slot/timer ordering, the stock OR occupancy gate, source-only dispatch and native/stock detached paths. Retained fighter/item guards stop continuation on faults/removal; private attachment helper renamed directly. Generic callbacks, no stock-special delegation, adapter or tick allocation.
- Grouped actual compiler/VM/actor/scheduler fixture passes 3,126 checks; seven Windows targets build and script/parameter CTests pass 2/2. Fresh roster preparation passes 24/26 forms, including all 210 bodies each for Pikachu/Pichu. Independent receipts rehash 1,708/201 inputs and current shipping templates. No visible/audible game launch or normal content/draft changes.
- Preparation does not establish gameplay parity. Connected Pikachu/Pichu comparisons, Ness animation-resource metadata, Samus independent-draft construction and remaining gameplay/rollback acceptance are still open. Evidence: `melee/docs/native_script_auxiliary_scheduler_checkpoint.md`. Goal active.

### Connected interpreted Pikachu/Pichu gameplay (2026-10-10)

- Actual private source packages publish 210 functions / three articles each / zero native imports. Added a reusable grouped replay runner using current owned parameters and scripts, hidden/muted accelerated drawing, existing recordings and independent stock/clone trace comparisons. No generated C, native module, normal installed content or draft changes.
- Fixed generic visibility selection to match actual signed-byte native setters: empty rows record selectors and unmatched alternatives hide models rather than faulting. Ownership/rig/mesh/lifetime guards remain. Grouped fixture passes 18,673 checks / 2,337 native comparisons; seven Windows targets and script/parameter CTests (2/2) pass.
- Restored omitted authored effects.active assignments in both charge/launch helpers after a grouped 42/46 run and bounded particle-RNG diagnosis. The consolidated corrected matrix passes 46/46 exact pairs: 39,400 controller/RNG samples, 78,800 fighter states, 1,776 actions, 8,901 item states, four item events and 195 damage events. All four prior discrepancies pass without RNG normalization or dummy RNG calls. Independent closure rehashes 261 game / 1,607 visuals inputs, both publication receipts and current templates/build inputs, and scans 92 game logs. Wider lifecycle, costume, pixels/audio, Workshop and network acceptance remain outside these results.
- Evidence and limitations: melee/docs/native_script_pikachu_gameplay_checkpoint.md. Ness metadata, Samus independent draft, remaining gameplay and compact rollback acceptance remain open. Goal active.


### October 10: generic interpreted animation component declarations

The direct interpreter now prepares separate joint/material/shape animation
pointers, omitted channels and indexed pointer arrays through the existing typed
DAT importer. Ness's real Yo-yo material-only graph passes descriptor/compiler
validation; Link/Young Link triplets and explicit component bundles pass connected
Item VM/native graph comparisons. Seven Windows targets and script/parameter
CTests pass. Descriptor batch: 552 checks/52 cases; connected batch: both Link
forms 1,465 checks/48 cases each, plus primary-only cases. See
`melee/docs/native_script_animation_components_checkpoint.md` for receipts and
limits. Ness's full audit now reaches 418 functions with 56 first-errors-per-body;
Ness remains rejected, rather than claimed complete. The next work is to address
those generic binding/receiver/context gaps as a batch. Samus independent draft
preparation and wider gameplay/lifecycle/visual acceptance remain outstanding.


### October 10: Ness canonical bindings and generic common-state batch

Ness now uses the direct interpreter's canonical package-parameter, receiver,
helper, callback-identity and RNG APIs. Added generic per-Fighter smash state,
stored platform flags, native self-velocity reflection and shared knockdown
entry. PK Fire's ordinary accessory helper retains a single spawn-cue consume,
as stock code does. No stock special delegation or permissive compatibility
binding. Seven targets rebuild, script/parameter CTests 2/2, and the connected
Item/Fighter console batch passes 9,828 checks with 896 complete-field writes,
512 platform queries, 256 native mirror comparisons and four real interpreted
Fighter helpers. Existing animation/page comparisons remain passing. Current
roster preparation remains 24/26; Ness drops from 56 to 14 first-errors-per-body
across 418 checked functions and remains rejected. Samus independent draft and
wider gameplay, lifecycle, visual and compact rollback acceptance remain open.
See `melee/docs/native_script_ness_common_state_checkpoint.md` for receipts,
controlled setup and limits; knockdown is source/build checked, not live-tested.


October 10 Ness vector/collision batch: shared interpreter primitives now retain
native axis-vector rotation, unit-axis Fighter velocity rotation and full
ceiling/wall XYZ observations for Fighters/Items. Bat scale uses the existing
rendered-root property. Seven Windows targets and 2/2 focused CTests pass;
the controlled real-library/VM cohort passes 13,436 checks, including native
rotation comparisons and unchanged Item animation/Pages regressions. All 24
previously accepted forms still prepare. Ness's complete 418-body audit drops
14 to 10 first-errors-per-body but still rejects; Samus remains unprepared.
Ness construction/gameplay parity and whole-match rollback are not proved by
this fixture. See `melee/docs/native_script_ness_vector_checkpoint.md`.


October 10 Ness entity/environment batch: generic immutable package entity
parameters now let Fighter/Article/Item/Effect code read declared tuning without
a live actor. Shared Item terrain/contact/landing, loop-stop, stage identity and
camera release bindings retain native formulas/order and authored landing-hook
lifetime checks. Seven Windows targets, 2/2 CTests and three controlled cohorts
(19,219 checks) pass. All 24 previously accepted forms still prepare. The complete
418-body Ness audit drops 10 to three first-errors-per-body, still rejecting;
Samus remains unprepared. Native terrain dependencies/camera release are partly
controlled and active sound/Ness gameplay parity are unverified. See
`melee/docs/native_script_ness_entity_checkpoint.md` for receipts and limits.


October 10 interpreted resource-animation batch: all five joint-animation
operations are now public on retained Item resource graphs. The compiler and
scene derive fixed same-owner path/bundle/application recipes; the original HSD
kernels execute without tick-time lookup or recipe allocation. PK Thunder child
selection uses explicit script branches naming its authored declarations.
Seven Windows targets, 2/2 focused CTests and the linked resource cohort pass
(7,031 checks, 39 exact animated graph/controller/track comparisons, three
recipe refusals). All 24 previously accepted forms still prepare. Ness's 418
bodies now compile without body errors, but canonical package preparation
rejects at the unmigrated up-smash entry hook; both up/down smash entry schedulers
need integration. Samus remains unprepared. No full Ness gameplay, material/shape
resource playback or active-controller rollback acceptance is claimed. See
`melee/docs/native_script_resource_animation_checkpoint.md` for receipts/limits.

October 10 smash-entry closure: generic interpreted up/down-smash hooks now
replace accepted common entry after item-throw priority, including NoD0 up-smash.
Seven targets build, 2/2 focused CTests and the linked cohort pass (16,154 checks,
432 dispatch inputs). Ness's canonical 418-function audit accepts with zero
diagnostics; the read-only preparation census is 25/26, with Samus remaining.
A real private Ness source packet stages, publishes and retires in 43 checks
without a native fighter definition. No actors are created by that fixture.
Ness live construction, Yoyo behavior and stock/script gameplay acceptance
remain open. These are preparation/publication results, not 25 completed clones.
See `melee/docs/native_script_ness_smash_hook_checkpoint.md` for receipts/limits.

October 10 actual Ness scene/grounded comparison: the shared model census now
handles inactive custom TEV records with native accounting, and bounded class
piece pools admit the measured 47,553-slot resource plan. Full conservative
Yo-yo graph reservation is 23,847,352 bytes; compacting it remains open.
The scripted accessory now queries its current owner, matching native motion
and scale checks, rather than requiring a held-item relation. Seven targets,
2/2 focused CTests, actual HSD graph construction (1,116 checks) and large-pool
lifecycle/snapshot checks (153,015) pass. Sixteen hidden/muted rendered matches
pass all eight stock/source pairs: idle, four grounded specials, three smashes.
Action timelines and all emitted Fighter/Item traces match; no script fault.
Airborne/interruption/charge/damage/CPU/match-end/rollback acceptance stays open.
Only private test stores changed; no complete Ness/whole-roster or compact
snapshot performance claim. See `melee/docs/native_source_ness_scene_checkpoint.md`.

October 10 Samus authoring checkpoint: verified the exact retail PlCo semantic
rig map (41 bindings, 60 joints, no omissions) against all five costumes.
Samus now creates a real private owned asset draft; its old census path was
nonexistent because draft creation failed before compilation. The 172-body
source audit now reports 14 failing bodies instead of 28 after a batched
accessory/charge/landing/effect translation cleanup and four generic common
locomotion properties. Seven native targets, 2/2 compiler/parameter CTests,
retail rig checks and the connected observations cohort (21,415 checks) pass.
The evidence closes 1,730 cohort dependency pins. No Samus gameplay run,
independent package acceptance or user collection modification: grapple,
lifecycle and remaining article/metadata/target/collision gaps stay open.
Preparation remains 25/26 non-Kirby fighters, not 25 completed clones.
See `melee/docs/native_source_samus_authoring_checkpoint.md`.

October 10 Samus article-feedback checkpoint: added generic synchronous
primary-joint effects using prepared paths and actual owned Item/VM dispatch,
with no deferred-effect substitution or per-frame script allocation. Charge
Shot and missile feedback now select the native root/grandchild; Charge Shot
uses explicit root parameters, typed owner decisions and script-owned charge
values, preserving distinct release-speed and clamped visual-charge values.
Explosion velocity clearing directly matches native XYZ zeroing. Seven targets
build, 2/2 focused CTests and the connected feedback cohort (2,319 checks /
220 spawn recordings) pass. Full 172-body audit now has five failing bodies,
down from fourteen; Samus is still rejected, with grapple/lifecycle and real
gameplay validation open. No game launch or user collection change. See
`melee/docs/native_source_samus_article_feedback_checkpoint.md`.

### 2026-10-10 — Shared action metadata and ground-motion batch

Added live canonical action metadata, namespaced Item ground-slide components,
and the standard ground collision response with an authored box. The metadata
read reconstructs semantic bits rather than copying host union memory; slide
writes preserve ordinary velocity; box response preserves native facing,
position/history and loss-of-contact Fall behavior. Samus uses these direct
capabilities without stock-special delegation or compatibility aliases.
Seven native targets and 2/2 focused CTests pass. Connected observation tests
pass 30,913 checks and collision comparisons pass 20,185 checks, with controlled
assets/stage/Fall-entry leaves documented. The 172-body audit now reports three
failing bodies: damage cleanup, missile targeting and bomb-owner overlap.
Samus is still rejected, and grapple/lifecycle plus real match validation remain
open. No visible/audible game launch or user collection change. Evidence and
scope: `melee/docs/native_source_samus_action_motion_checkpoint.md`.

### 2026-10-10 — Shared interruption cleanup

Added generic owned-Fighter interruption cleanup using the existing engine
sequence: stop voice channels, run damage cleanup, then primary-death cleanup.
Generation/program/storage/scene guards reject success after retirement or a
script fault. Samus's bomb-jump helper prepares and no longer applies an extra
model-scale factor absent from its native launch velocity. Seven native targets
build and 2/2 focused CTests pass; the connected fixture records native/script
callback combinations, replacement precedence, exact audio arguments and faults.
The 172-body audit now reports two failing bodies, targeting and overlap;
recipient calls with arguments and further grapple/lifecycle work remain open.
Samus is not an accepted independent fighter. No game launch or collection
change. See `melee/docs/native_source_samus_interruption_checkpoint.md`.

### 2026-10-10 — Shared ahead-query comparisons

Article scripts can read the nearest eligible opponent or lockable Item ahead,
using native owner/player/team/hidden exclusions, facing, squared-XY ties and
camera/ECB-center positions. Three distinct Float outputs publish on success;
scripts choose priority and steering. Samus now uses those queries plus the
existing generic vector-rotation API. The full 172-body audit has one failing
body, bomb accessory overlap; recipient helpers with arguments are still
needed there. Connected host comparisons cover 128 opponent and 128 Item cases
through direct bindings and Article VM signals. Seven native targets and 2/2
focused CTests pass. Samus is still rejected; grapple/lifecycle ownership and
real-game acceptance remain open. No visible/audible launch or collection
change. See `melee/docs/native_source_samus_ahead_checkpoint.md`.

### 2026-10-10 � Generic typed recipient arguments

Exported root helpers accept up to 30 typed numeric arguments, using normal
language conversions and fixed-capacity execution with existing lifetime
checks. All four recipient call forms support these arguments; no fighter
specific operation or mailbox is introduced. The connected cohort passes
2,995 checks / 374 cases, including parameterized retirement mutations;
the broader assembly cohort passes 133 checks / 16 ticks. Seven native
targets and 2/2 focused CTests pass. Samus exports its bomb-jump helper, but
the 172-body audit still rejects bomb overlap. Retained control-owner wiring,
grapple and lifecycle/reset ownership remain open; no independent Samus or
real-game acceptance. See
`melee/docs/native_source_recipient_arguments_checkpoint.md`.

### 2026-10-10 � Generic hitbox overlap and bomb control-owner dispatch

Article scripts can test a selected hit capsule against a Fighter through the
shared native geometry kernel, preserving caches, scaling and early exit with
physical rig/lifetime admission. Samus's bomb accessory uses its retained
control owner and parameterized exported helper. The grouped geometry cohort
passes 18,181 checks / 1,184 cases, with 512 new direct/Article-VM comparisons
against the unchanged stock overlap loop; recipient regressions pass as well.
Seven native targets and 2/2 focused CTests pass. All 172 Samus bodies audit
without body errors; the remaining audit diagnostic is the intentional WIP
independence package gate. Grapple, lifecycle/reset and real-game acceptance
remain open. No user collection change or game launch. See
`melee/docs/native_source_samus_overlap_checkpoint.md`.

### 2026-10-10 - Samus grapple interpreter authoring batch

Reconciled historical tether components with current generic APIs, corrected
raw attachment joint51 and air-probe reach1.8, and composed real ground Catch
hooks and grapple launch/retract/capture/spark signals. All 271 candidate bodies
prepare; only the intentional independence package gate remains. The production
VM's geometry helper passes 480 comparisons against the native pre-allocation
arithmetic slice, plus six malformed-data cases (33,378 checks). Strict fixture
compilation passes. Receiver values and metadata are controlled; node/model/
animation order, signals/solver runtime, modified grapple, lifecycle/scaling,
throw attachments and gameplay remain open. Removed the obsolete generated-C
constructor runner. No game launch or collection changes. See
melee/docs/native_source_samus_grapple_interpreter_checkpoint.md.

### 2026-10-10 - Samus owned lifecycle and scaled values

Main source now owns load, primary death and attribute reload; move/roll code
uses ten derived scale-adjusted values computed from immutable owned parameters.
The composed tether candidate resets its own handle/sequence and follows stock
shot/spin/grapple cleanup before clearing three damage/death callback slots.
The full audits prepare 176 main / 276 composed bodies, with only the intended
independence gate. Actual VM/native lifecycle host comparisons pass 128 cases,
384 repeated/changing-scale reloads and 34,660 checks; geometry regressions pass
480 comparisons plus six safety cases. Both fixtures pass strict warnings.
Seven native targets and two focused CTests pass. Source cleanup is prepared,
but resource/borrower cleanup, full solver/constructor runtime, modified grapple,
throw attachments and gameplay remain open. No game launch or collection change.
See melee/docs/native_source_samus_lifecycle_checkpoint.md.


### 2026-10-10 Samus grapple policy checkpoint

The composed interpreter candidate now prepares 277 bodies. The owned Up/Down/Up/A helper matches the complete native helper in 8,200 VM comparisons; the combined lifecycle/input batch passes 75,785 checks, 128 load/death cases and 384 reloads. Existing constructor geometry comparisons still pass (480 plus 6 safety cases). Air wall-contact motion/effect dispatch is authored with existing generic primitives and passes preparation, but has no live resource/solver acceptance yet. Modified hang timer requires a physical L-button read (not Z or combined shield). Modified homing/capsule behavior, throw accessories and full lifecycle/gameplay closure remain open; Samus stays unregistered. Evidence and limits: melee/docs/native_source_samus_grapple_policy_checkpoint.md.


### 2026-10-10 Samus modified hang and physical triggers

Generic input.l_held and input.r_held observe individual physical L/R digital bits, separately from Z and logical shield. The composed Samus hang helper reproduces the native pre-exit timer rule in 240 VM comparisons; the batch also passes the 8,200 sequence comparisons, 128 lifecycle cases, 384 reloads, 480 geometry comparisons and 6 safety cases. All 278 composed bodies prepare; the intentional independent-Fighter gate remains. Seven native targets and two focused CTests pass. Modified homing/capsule behavior, throw accessories and live resource/solver/gameplay closure remain open. Evidence: melee/docs/native_source_samus_grapple_policy_checkpoint.md; receipt A:/FreakMelee-test-archives/samus-grapple-hang-oct10/receipt.json.


### 2026-10-10 Generic nearest-available query and Samus homing

Generic targets.nearest_available_fighter returns an Int creation ID using the unchanged common search (including allies, caller CPU blast margins, availability/death filters, XY distance and first ties). Physical list admission precedes registry lifetime checks. The connected compiler/actor/VM target cohort passes 139 nearest comparisons and malformed-context/list cases, alongside existing geometry/ahead/overlap checks (19,057 checks, 1,512 native comparisons, 70 negatives). Composed Samus source owns L-gated modified flight steering and prepares all 279 bodies; geometry/lifecycle/sequence/hang comparisons remain green. Steering with live resources is not yet accepted, and modified capsule creation/mode transition, throw accessories and full runtime closure remain open. Seven native targets build; no game, installed content change or push. Evidence: melee/docs/native_source_samus_homing_checkpoint.md and melee/docs/scripting/targets.md.


### 2026-10-11 Samus grapple descriptor portability prerequisite

Before adding generic Fighter hitbox creation, fixed a confirmed native packed-read mismatch: the stock grapple descriptor declares damage 0 but the original Windows halfword read supplied 17 from bone-ID bits. The native initializer now uses named fields; all three creation sites initialize the formerly undefined victim-only flag to false. Independent console-word decoding passes 65,537 descriptor/state/group cases (884,753 checks), preserving offsets, coefficient, flags, victim history and common-helper call order. Seven native targets build and two focused CTests pass. Controlled damage/angle/bone leaves and static caller checks are explicitly bounded evidence; no VM/live-resource/gameplay closure claimed. Generic immutable hitbox specification, reserved-joint binding and scripted modified-mode transition remain open. No game launch, installed-content change or push. Evidence: melee/docs/native_source_samus_hitbox_descriptor_checkpoint.md; receipt A:/FreakMelee-test-archives/samus-grapple-hitbox-descriptor-oct11-b/receipt.json.


### 2026-10-11 - Generic immutable hitbox creation checkpoint

- Root hitbox_definition data and hitbox.create(slot, Definition) now initialize complete fighter capsules without packed commands, raw pointers or tick allocation. Owned joints and declared live article reservations are supported; stale/replaced bindings are rejected and teardown detaches capsule borrowers.
- Samus authoring owns the modified-ground attack and mode transition in extending, falling and slack branches, before homing. No native Samus handler delegation.
- Connected fixture: A:/FreakMelee-test-archives/hitbox-create-oct11-g/receipt.json (19,897 checks; 128 new direct/VM native capsule comparisons). Damage/angle and graph construction remain controlled fixture leaves.
- Samus source composition: A:/FreakMelee-test-archives/samus-hitbox-source-oct11-a/receipt.json (280 bodies prepared; only the deliberately absent independent registration gate). Seven native targets build; two focused script/parameter CTests pass.
- See melee/docs/native_source_hitbox_creation_checkpoint.md for evidence and limits. Samus stays unregistered: throw accessory metadata/events, constructor/solver/lifecycle closure and gameplay acceptance remain open. Full project goal remains active.


### 2026-10-11 - Common throw presentation and timer checkpoint

- Generic interpreted on throw_presentation now runs after complete victim setup and before common invincibility, carrying typed motion/rate values. Retained reciprocal holder/victim identities guard continuation; absent scripts do not call stock fighter constructors.
- Confirmed a native portability bug: console MSL bool was signed int, but Windows bool collapsed a multi-frame invincibility duration to one. The field is now s32 throw_invincibility_frames with direct call sites and unchanged grant/decrement/expiry logic.
- Connected capture scheduler receipt: A:/FreakMelee-test-archives/throw-presentation-oct11-final/receipt.json (106,625 checks, 9,764 cases; 62 new presentation cases and 41 countdown cases). Motion, animation, overlay and stock accessory construction leaves remain controlled; no full rendered throw proof.
- Seven native targets build and two focused script/parameter CTests pass. See melee/docs/native_source_throw_presentation_checkpoint.md for source evidence and limits. Samus remains unregistered pending structured accessory recipes/controls, conditional joint rates and full constructor/solver/lifecycle/gameplay acceptance. Full goal stays active; no game launch, installed-content change or push.


### 2026-10-11 - Generic transient fighter accessory checkpoint

- Normalized owned accessory recipes replace the anonymous Samus payload directly: root, base animation and up to 16 joint/material/shape variants. Real DAT tests check retail Samus identities, generic variants and malformed/provenance rejection.
- Generic interpreted accessory controls support construction, animation, scale, geometry clock loop policy, own-bone follow and disposal. The authored Samus throw helper uses these primitives at on throw_presentation; no stock special handler delegation.
- Connected capture/VM batch: A:/FreakMelee-test-archives/accessory-controls-oct11-reviewed/receipt.json (113,212 checks, 9,934 cases; 170 accessory cases). Runtime graph construction/evaluation/follow leaves are controlled; actual HSD clock traversal executes.
- Full Samus source audit prepares 282 bodies with only the intended independence gate; existing geometry/lifecycle/input/hang comparisons pass. Receipt: A:/FreakMelee-test-archives/samus-accessory-source-oct11-reviewed-b/receipt.json. Eight native targets build; three focused CTests pass.
- See melee/docs/native_source_fighter_accessory_checkpoint.md. Samus remains unregistered pending full retail constructor/solver/catch/resource lifecycle and rendered gameplay acceptance. No game launch, installed-content change or push. Full goal stays active.


### 2026-10-11 - Retail Samus resource constructor and palette animation checkpoint

- Full composed Samus Created hook now runs through the interpreter on the real imported costume rig, primary model scene, resource kernels and HSD graphs. Unchanged native constructor comparisons pass 72 pairs across motion, scale, ground/air and modified-mode inputs.
- Generic bounded constant TCLT palette selection is supported; imported palette copies, selections and controller origins are checked. Interpolation and authored default-palette sentinel curves remain explicit gaps. No fighter-specific admission bypass was added.
- Frozen receipt: A:/FreakMelee-test-archives/samus-resources-oct11-final/receipt.json (837,301 checks; 15,612 palette comparisons; 17 census cases; 174 live refusals). Existing Arrow/Boomerang suite passes 7,033 checks and 39 animation pairs in A:/FreakMelee-test-archives/script-item-resources-palette-oct11/receipt.json.
- Client/source-asset builds and four focused CTests pass; changed production modules pass strict warnings. Removed duplicate unavailable-Item relations from the compiler fixture after a real link failure.
- See melee/docs/native_source_samus_resource_constructor_checkpoint.md for scope and limits. Samus remains unregistered: actual factory/attach/scale orchestration, solver/catch integration, lifecycle and gameplay acceptance remain open. Full project goal stays active. No game launch, installed-content change or upstream push.


### 2026-10-11 - Samus real factory, attachment and removal checkpoint

- Connected the complete interpreted Tether_Create helper to the unchanged shared Item factory using genuinely imported retail common Item data, actual primary/chain models, bone pickup and owner scaling. Parent Fighter construction/publication remains fixture setup.
- This exposed a real source bug: the final scale signal used a held-item holder for a tether that does not occupy the held-item slot. It now uses the retained attachment owner, matching the original native owner-scale behavior through generic scripting primitives.
- A:/FreakMelee-test-archives/samus-factory-oct11-f/receipt.json passes 841,193 assertions, including the previous 72 native constructor pairs and 72 connected factory cases across four motions, three scales, both facings and normal/modified input sequences. It covers 36 bound Death1 removals, complete callback/resource cleanup, four ignored motions and controlled fixed-pool allocation failure with no published child.
- The copied common bank matches the current ISO's ItCo.usd SHA-256 exactly. Client/source-asset builds pass. See melee/docs/native_source_samus_factory_checkpoint.md for evidence and limits.
- Samus remains unregistered pending complete owned rig metadata, full Fighter lifecycle and integrated solver/catch/gameplay acceptance. Goal stays active; no game launch, installed package change or push.


### 2026-10-11: Samus held animation and hitlag integration

- Corrected a generic animation-admission mismatch: the supported complete root world-matrix override no longer prevents the retained graph from advancing. Partial root and descendant flag substitutions still fail admission.
- Sealed connected retail/native comparisons in A:/FreakMelee-test-archives/samus-held-oct11-j/receipt.json: 576 normal held updates, 432 pre-launch hitlag updates, 72 constructors, 72 real Item factory cases, 144 bound normal callbacks and 108 bound hitlag callbacks. All 2,678,532 assertions pass; the shared Link resource regression and focused build/CTest checks also pass.
- Corrected earlier stale rig claims: Samus already owns all 41 retail semantic bindings, 60 joints and no omissions. The current ISO audit passes across all five costumes, and the imported draft facts are checked in the fixture. No rig implementation was duplicated.
- See melee/docs/native_source_samus_held_update_checkpoint.md for evidence, fixture corrections and limits. Samus remains unregistered pending full Fighter lifecycle, integrated flight/constraint/catch and gameplay acceptance. Goal stays active; no visible game, package change or push.


### 2026-10-11: Samus capture-retraction integration

- Connected real CatchPull anchor/readiness hooks and installed source accessory updates to mode5 retraction. Seventy-two cases compare the interpreter with unchanged native fn_802B8814 on real retail HSD graphs through completion, including RNG, positions/velocities, collision history, exact controllers/materials/palettes and override world matrices. Completed source removal releases the capture anchor, reserved part139 and callbacks.
- Fresh frozen-input receipt A:/FreakMelee-test-archives/samus-capture-oct11-final/receipt.json passes 3,900,510 assertions: 72 constructor pairs, 144 factory cases, 296 retraction updates and 72 completed removals. Existing held/hitlag and pool/refusal cases remain intact. Shared Link regression passes 7,033 assertions and 39 animation pairs against its prior sealed production snapshot with current fixture files.
- Corrected comparison setup, not gameplay: the native primary graph now undergoes the real shared Item state transition, and the factory counter avoids older manually staged fixture identities. No production code changed in this checkpoint.
- See melee/docs/native_source_samus_capture_retraction_checkpoint.md. Parent/capture/deployment setup is staged; actual victim, flight/contact, other constraints, full Fighter lifecycle and gameplay remain open. Samus remains unregistered. Full goal stays active; no visible game, installed-content change or push.


### 2026-10-11: Samus ordinary retraction and post-launch hitlag

- Extended the connected retail-resource corpus to ordinary mode4 retraction: ground/air speed, stochastic chain constraints, X/Z damping, mode0 completion and two following held updates. Both mode4 and mode5 cohorts now run four post-launch hitlag callbacks against unchanged native it_802BACC4 before continuing normal updates. No production code or scripting extension was needed.
- Fresh receipt A:/FreakMelee-test-archives/samus-retraction-hitlag-oct11-final-b/receipt.json passes 8,124,936 assertions: 216 actual factory cases, 72 mode4 pairs, 72 mode5 pairs, 576 post-launch hitlag constraints and 144 returned-held updates. Earlier constructor/held/pre-launch hitlag/refusal checks remain intact. Samus solver fixtures are organized directly in samus_solver_cases.h, replacing the capture-only filename.
- See melee/docs/native_source_samus_retraction_hitlag_checkpoint.md. Parent and deployment/capture state remain staged; flight/contact, wall modes, actual victim, full Fighter lifecycle and gameplay are still open. Samus remains unregistered. Full goal stays active; no visible game, installed package change or upstream push.


### 2026-10-11: Samus controlled-contact flight

- Added 144 mode-1/2 native/interpreter pairs on real retail graphs: extension, falling motion and controlled no-contact/wall/floor responses, through eight updates or the first state transition. Shared geometry inputs preserve original solver decisions; script deployment/floor arrays, contact data, RNG and full graph/controller/matrix state compare with native state.
- Corrected Samus's final special attribute from an unknown host pointer to the existing integer tether_hang_frames contract. Retail offset 0xD0 and total size 0xD4 are asserted; this changes data layout, not gameplay timing.
- Fresh receipt A:/FreakMelee-test-archives/samus-flight-oct11-final-b/receipt.json passes 11,473,912 assertions across the complete resource corpus, including 360 real Item factory cases and 480 flight updates. The fresh Link regression passes 7,033 assertions and 39 animation pairs. Client/source-asset build and four focused CTests pass.
- See melee/docs/native_source_samus_flight_checkpoint.md. The first frozen test compile exposed a copied-TU local-include collision; preserving its original directory fixes the runner. Actual stage geometry, aerial wall transition, downstream slack/wall constraints, full grabbed-victim lifecycle and gameplay remain open. Samus stays unregistered, and the full goal stays active. No visible game, installed-content change or push.


### 2026-10-11: Samus slack and wall constraints

- Continued all 144 flight pairs through eight ticks, including destination modes 2/3. Added 216 real Item factory pairs for modes 3/6/8: stochastic slack motion, wall extension, hanging constraints, moving-line math, parent displacement/velocity and hang timers with modified L hold and ground/attack transitions. No production API or script change was needed.
- A:/FreakMelee-test-archives/samus-constraints-oct11-final/receipt.json passes 19,213,448 assertions, 576 total factory cases, 878 new constraint updates and 140 transitions. Source/native unblocked segment coordinates/order/count match; original line remapping runs on controlled tables. Shared Link regression passes 7,033 assertions and 39 animated graph pairs using the prior frozen production inputs and current fixtures.
- See melee/docs/native_source_samus_constraints_checkpoint.md. Mode-7 pull, hang expiry/blocked terrain, actual stage geometry, aerial wall effects, victim lifecycle and full Fighter construction/gameplay remain open. The decompiled mode-7 pointer read is being reconciled against retail before using that branch as a reference. Samus remains unregistered; full goal active; no visible game, installed content change or push.


### 2026-10-11: Samus nonterminal pull and retail direction correction

- Reconciled the mode-7 uninitialized direction pointer with the supplied US 1.02 DOL. Retail loads the calculated vector directly from stack 0x28/0x2C/0x30; the pointer read was a decompilation artifact. Native it_802BA3BC now directly uses dir.x/y/z, as the script already did. The standalone retail_pull_audit.py verifies DOL hash and exact instruction words; receipt A:/FreakMelee-test-archives/samus-pull-retail-oct11/receipt.json records static evidence.
- Added 72 real Item factory mode-7 pairs with over-length endpoint gaps, comparing nonterminal pull through two updates or ground exit. All 72 verify the corrected clamp. A:/FreakMelee-test-archives/samus-pull-oct11-final/receipt.json passes 19,793,080 assertions, 648 total factory cases, 108 pull updates and 36 ground transitions. Fresh Link regression passes 7,033 assertions and 39 animation pairs. Rebuilt client/source-asset target and four focused CTests pass.
- See melee/docs/native_source_samus_pull_checkpoint.md. Terminal pull into ledge/jump, blocked-terrain/hang-expiry exits, aerial wall effects, actual victims and full Fighter construction/gameplay remain open. Samus remains unregistered, full goal active. No visible game, installed-content change or upstream push.


### 2026-10-11: Samus normal-loader gameplay, capture and terminal grapple paths

- Added a complete hidden/muted native-client replay batch using normal source audit, owned parameters, independent package preparation and a private installed collection on A:. All 282 bodies prepare successfully. No common-state CLI bypass, manually staged Fighter, stage/effect stubs or user collection changes.
- Actual wall contact exposed a generic effect payload error: facing generators consume f32*, but the resource API passed the scalar. The API now supplies the pointer. Strict hanging comparisons also exposed missing parentheses around the displacement in the translation; restoring stock evaluation order eliminates the numeric drift. No fighter-specific scripting API added.
- A:/FreakMelee-test-archives/samus-gameplay-oct11-final-d/receipt.json passes all16 cases with unchanged input/binary hashes: both facings of empty grab, air grapple, four throws with actual Mario victims, wall entry/hang expiry into DamageFall, and terminal pull into CliffJumpSlow2 followed by ledge catch. Exact comparisons cover 9,600 inputs, 19,200 fighter states, 392 actions, 1,070 item states, eight damage events and 9,600 RNG states. Client/content-tool rebuild and seven focused CTests pass.
- See melee/docs/native_source_samus_gameplay_checkpoint.md. The old standalone geometry runner could not compile against its obsolete baseline72 SDK and is not counted as proof. Modified-ground activation/homing/capsules, blocked exits, interruption/reuse, ledge-available terminal branch and visual acceptance remain open. Samus default template remains inherited/partial; full goal active. No visible/audible game or upstream push.

2026-10-11 Samus resource/modified-mode checkpoint: the private independent
candidate passes 22 actual stock/interpreter gameplay cases, including the
modified grounded chain/capsule and modified airborne wall hold/pull in both
facings. Exact traces compare 142,480 separate resource count/node records,
including motion, collision history and model/material hashes. Receipt:
A:/FreakMelee-test-archives/samus-resource-gameplay-oct11-b/receipt.json.
Client rebuild and seven focused CTests pass. Terrain-blocked exits, complete
interruption/retirement/reuse, exact ledge-available pull completion and visual
acceptance remain open; default Samus activation is still gated. See
melee/docs/native_source_samus_gameplay_checkpoint.md. Goal remains active.
2026-10-11 Samus Workshop source integration: fresh drafts select the exported
samus_full.fms independent interpreted template, including four combat articles,
complete grapple/resource policies, throw accessories, CPU and load/death cleanup.
All 282 bodies prepare (8,776 instructions, no legacy attribute imports). Source
components stay separately maintained; a CTest checks the shipped export. Existing
user drafts are untouched. The copied native Definition remains an asset-authoring
reference, not a runtime callback fallback after independent preparation.
The exact shipped source passes 28 stock/script grapple/lifecycle cases with
24,400 inputs, 48,800 fighter states and 175,424 resource count/node records:
A:/FreakMelee-test-archives/samus-workshop-gameplay-oct11-final-a/receipt.json.
Damage before/after launch removes the live chain; two natural live-chain deaths
and respawns restore ordinary 30-node grabs after modified-code use, both facings.
A wider specials refresh found the valid creation-ID zero was incorrectly rejected
as absent. The Charge Shot script now uses the -1 sentinel; all nine recordings
then match stock in samus-complete-specials-oct11-corrected-b/receipt.json on A:.
Current selected Workshop templates pass normal preparation for 26/26 supported
forms in clone-roster-current26-oct11-a/receipt.json; this is not roster gameplay
acceptance. Client/tool rebuild and eight focused CTests pass. Terrain-blocked
exits, exhaustive lifecycle/retirement, exact ledge-available completion, visual
acceptance, broader interactions and whole-match rollback remain separate work.
See melee/docs/native_source_samus_gameplay_checkpoint.md. Full goal stays active.
### 2026-10-11 - Shipped Samus CPU and repeated-projectile checkpoint

The rebuilt client passes all ten 1,800-sample stock/independent-Samus CPU
comparisons, covering levels 1/4/5/9, a shielding opponent, Battlefield, left/right
recovery, a lower target and Screw Attack against a CPU. Both 900-sample repeated
homing/Super Missile cases also pass through normal fresh draft/package creation.
All CPU clone logs report zero VM faults. These results supplement the pinned
28-case grapple/lifecycle and corrected nine-case Charge Shot receipts; they
are not one sealed 41-case receipt. The existing CPU/missile runners do not pin
inputs before/after execution. Eight focused CTests and client/content-tool build
pass, and the shipped full-template copy equals its source. See
melee/docs/native_source_samus_gameplay_checkpoint.md for artifacts and limits.
Terrain/rare interruption, visual, wider roster gameplay and whole-match rollback
acceptance remain open. The goal stays active; commits remain local.
### 2026-10-11 - Interpreted single-fighter roster CPU checkpoint

All 22 single-fighter templates pass one sealed normal-package stock/script CPU
batch: 1,800 samples each, 39,600 inputs per runtime, complete replay consumption,
no reported source/fighter/item VM faults, and unchanged before/after input pins.
Receipt: A:/FreakMelee-test-archives/source-roster-cpu-oct11-final-a/receipt.json.
The batch found and corrected move-entry resets, prop ownership, binary32/double
rounding, Hookshot instance scale and a native PPC stack-layout artifact. Generic
world-position/local-offset effect admission and live hand-attachment animation
references now support the original behavior; diagnostic graphs include stock
yoyo and projectile extras rather than treating their absence as script drift.
Client/tool builds, two refreshed focused CTests and the 524,674-check effect ABI
fixture pass. See melee/docs/native_source_roster_cpu_checkpoint.md for details.
This fixed-seed CPU cohort is not exhaustive fighter or whole-match acceptance.
Zelda/Sheik and Popo/Nana family gameplay, broader move/lifecycle interactions,
visual acceptance, compact whole-match savestates and rollback budget remain open.
Kirby remains excluded. The goal stays active; commits stay local.

### 2026-10-11 - Current interpreted family gameplay checkpoint

Fresh normal-source Zelda/Sheik (both starting orders) and Popo/Nana packages
pass three 1,800-input stock/script CPU comparisons after the client rebuild.
Both Player roles are compared, including Nana and dormant alternate forms.
Separate 3,200-input grounded and airborne Zelda -> Sheik -> Zelda sequences
pass every expected special phase group, exact Fighter/Item/RNG traces and
3,423 total linked resource records. Every source member reports zero faults;
all receipts retain unchanged before/after inputs. No generated behavior C/DLL.
Sheik chain now queries its actual current owner, matching stock accessory
ownership. Native diagnostics also enumerate stock chain nodes; no fields or
numeric drift are ignored. Three deliberate companion trace mutations fail
the comparator. Receipts: source-family-cpu-oct11-final, source-family-ground-
oct11-c and source-family-air-oct11-c under A:/FreakMelee-test-archives/.
See melee/docs/native_source_family_gameplay_checkpoint.md. This supplements
the 22-Single CPU cohort; exhaustive interaction/lifecycle, visual acceptance
and compact whole-match rollback remain open. Kirby remains excluded.
Goal active; local commits only; user installed content remains untouched.

### 2026-10-11 - Interpreted single-fighter special-entry checkpoint

All 176 ground/air entries for four specials across 22 Single bases now have
passing stock/source evidence across the initial batch and grouped reruns.
This is consolidated evidence, not a single final sealed 176-case run.
Initial batch: 157 PASS / 19 FAIL, all packages prepared. Grouped rerun passes
15 non-Game & Watch cases; the four remaining Chef/Rescue cases pass after
observing actual script state and attachment ownership instead of dead stock
scratch. Fresh Link arrow ground/air and DK deferred bone-position cases pass.
All gameplay receipts retain unchanged input hashes; hidden, muted, fully
rendered ordinary controller input, independent packages, no behavior C/DLL.
Corrections cover Jigglypuff disc-word pointer widening, generic typed joint
model VFX admission, named deferred position effects, arrow float rounding,
and source-aware diagnostic reads. Deferred positions sample the selected bone
at actual flush, retain identities rather than VM/bank pointers, reject stale
requests and release queue records. The connected 43-case fixture passes 5,280
checks and verifies all 13 new queue allocations free. 65,536-ID effect ABI
inventory, client/content-tool build and three focused CTests pass.
See melee/docs/native_source_roster_special_checkpoint.md for exact receipt
paths and limits. Special entry is not exhaustive charge/chaining/aiming,
reflection/capture/interrupt/death/lifecycle, visual/audio or results acceptance.
Compact whole-match state/rollback correctness and budget remain separate work.
Kirby excluded; overall goal active; local commits only; installed content intact.

### 2026-10-11 - Interpreted special follow-through batch

The shared source roster runner now accepts explicit recorded gameplay matrices,
with motion/damage/item contracts, fresh ordinary interpreted packages, per-case
samples/seeds and all-case failure collection. Its original entry mode still
passes the focused Luigi side-ground regression. Hidden, muted, rendering enabled;
no behavior C/DLL, user installed-content changes or production gameplay edits.
All 42 selected cases across Luigi, Jigglypuff, Marth, Roy and Mewtwo have passing
stock/source evidence: initial 39 PASS / 3 coverage failures, grouped corrections
for Roy full charge and upward-diagonal Teleport, then natural Luigi misfire.
The historical seed-3 misfire window no longer applies; restoring two stocks
and a seed-2 probe did not recover it. Keep seed 3 and move the controller press
and release nine frames earlier in stock's preceding idle, without forcing RNG.
All accepted trace/state/RNG/resource comparisons pass with zero VM faults and
unchanged per-batch pins. The client hash is identical across the accepted runs.
Consolidation verifies all 42 accepted recordings and coverage arguments against
the final matrix. This is successive-run evidence, not a final full sealed run.
See melee/docs/native_source_roster_followthrough_checkpoint.md for cases,
source-backed contracts, receipts and explicit airborne-placement boundaries.
Exhaustive special/lifecycle/interaction acceptance, visuals/audio, complete
mutable-state snapshots and rollback budget remain separate work. Kirby excluded;
overall goal active, local commits only. No claim of whole-fighter completion.

### 2026-10-11 - Interpreted fighter interaction batch

All 32 selected Marth/Roy/Mewtwo interactions now have passing stock/source
comparisons. Fresh ordinary interpreted packages, hidden/muted/rendered games,
explicit counterattack, victim/damage, reflector ownership, throw and worn-item
coverage. No production gameplay, scripts or native binaries changed this pass.
The initial batch passes 31; its one repeated-Confusion coverage check mistook
an action-ID log for a complete entry log. Same-ID restart at sample 236 is
visible in the fighter animation clock, so the generic window gate now observes
motion changes or clock resets. Its fresh-package rerun passes all three uses.
Seven positive/negative trace checks pass, including rejecting an interval where
the motion is active without restarting and rejecting wrong item state/owner.
One final trace-analysis batch passes all 32 actual pairs with coverage, accepted
recording/argument equality and original log hash checks. Both gameplay receipts
retain unchanged pins and identical client/content-tool hashes; this is not a
new final sealed 32-case gameplay run. See
melee/docs/native_source_roster_interactions_checkpoint.md for exact receipts,
controlled placement/item-spawn boundaries and live-index/clock-check limits.
Opponents here remain stock; both-source cross-fighter interactions, exhaustive
lifecycle/interruptions, visual/audio and whole-match rollback remain separate.
Kirby excluded; goal active; user installed content untouched; local commits only.

### 2026-10-11: two-source fighter interaction checkpoint

All 18 selected interactions pass stock/source comparison across the initial
batch (17 PASS / 1 coverage FAIL) and a corrected Roy/Mario fixture rerun.
Both participants use fresh ordinary independent interpreted packages from
one private shared collection. Six packages prepare: Mario, Fox, Marth, Roy,
Mewtwo and Peach. Cases cover counters, Toad, Confusion reflection/capture,
throws and Disable, including two Marths sharing one program but retaining
separate declared counter-damage state. Roy/Mario required delaying only the
opponent attack six frames to reach the stock counter guard; no production
behavior, source template or client binary changed.

The shared batch runner now prepares every required P1/P2 package before
play, records dependent failures, compares both independent actor identities,
and requires exactly two fault-free source actor reports. A single-source
Luigi entry regression passes. Games remain hidden, muted and rendered.
Final accepted recording bytes/arguments and trace hashes match the final
18-case matrix; binaries match across both gameplay batches. Consolidated
acceptance is not one final sealed 18-case gameplay run.

Evidence: melee/docs/native_source_roster_duo_checkpoint.md and
A:/FreakMelee-test-archives/source-roster-duo-oct11-b/consolidated.json.
All 26 scoped Workshop forms have their canonical source registered and
staged, checked statically; this is not a fresh live UI clone transaction.
Whole-match restored state/rollback budget, broader move/lifetime branches,
visual/audio acceptance and the remaining full project plan remain open.
Kirby remains excluded. Local Git only; no push.


## 2026-10-11 - Interpreted results and author reference checkpoint

The full results cohort prepared 25 bases (26 forms excluding Kirby): 84/100 cases passed and 16 failed across Donkey Kong, Link, Young Link and Peach, with pinned inputs unchanged. A generic active-animation-table fix preserves the stock demo actor clip-length query and missing-clip zero; no fighter script exceptions were added. Client/tool builds and the 26,148-check kinetic fixture pass. A fresh 24-case post-fix cohort is running; this is not final 100-case acceptance. See melee/docs/native_source_roster_results_checkpoint.md. The current scripting reference now describes typed bytecode execution, numeric checks, private receiver versus exported-call lifetime behavior, and current pools; historical generated-C fixtures remain labelled evidence. Local commits only. Whole-match compact state, wider lifecycle/Workshop acceptance and other plan work remain open.


### 2026-10-11 - Results rerun completed

The fresh post-fix results cohort passes all 24 outcomes across Donkey Kong, Link, Young Link, Peach, Mario and Ice Climbers; six preparations pass, and all 93 pinned inputs independently rehash unchanged. Together with the initial full cohort, all 100 unique selected outcomes now pass. This is combined evidence across two binaries, not a sealed 100-case final-binary run, and does not establish result-pose/pixel/audio parity, return-to-CSS/re-entry, full Workshop transactions or compact aggregate snapshots. Receipt: A:/FreakMelee-test-archives/source-roster-results-oct11-b/receipt.json. Details: melee/docs/native_source_roster_results_checkpoint.md. Goal remains active.

## 2026-10-11 - Actual Ground ownership and packed-record snapshots

Both native Ground constructors now allocate actual stage userdata directly from
stable scene-owned storage, coordinated with GObj/Fighter epochs. The real HSD
userdata lifecycle retires occupied records, including deferred scheduler removal.
Occupied-only serialization retains exact generations/free order and validates
stage root geometry before mutation. The original uninitialized Ground union is
now cleared on allocation; no stage-specific decision or callback was rewritten.

Native client/content-tool build passed. Focused Ground fixture: 12 cases/12,411
checks; existing GObj, Fighter, scene-transition and census fixtures passed too.
All 29 selectable versus stages were exercised with old/new binaries, identical
seed/2,000 controller samples, four CPU slots (including Ice Climbers), medium
items, full HSD rendering, hidden/muted fast pacing. 28 stages matched exact trace
families with complete inputs and zero census errors. Green Greens fails in BOTH
builds during /GrGr.dat import (descriptor status 6, offset 0x59024), before VS;
this is a separate existing importer follow-up. The complete cohort receipt stays
FAIL. All 12 input pins were independently rehashed unchanged after completion.
Largest observed Ground occupancy was 29 on Poke Floats; reservation remains
bounded by the existing 4,096-GObj capacity rather than imposing a smaller limit.

Evidence: melee/docs/native_ground_snapshot_ownership.md and
A:/FreakMelee-test-archives/ground-stage-cohort-oct11-c/receipt.json.

This is a partial explicit owner slice, NOT completed match save/load. Stage
module globals, moving collision buffers, graph objects, items/effects and the
remaining aggregate roots still require serialization. The production broad page
oracle remains enabled; no whole-match size or sub-100-us performance claim.
## 2026-10-11 - Stage import dependency fixed; selectable-stage cohort closed

Green Greens' failed descriptor is item opcode 10 (animation 0) on the common
Random article. Native stage import eagerly validates that article's model, but
all 15 scene call sites previously loaded the stage before the common item assets.
Common descriptor loading now precedes stage asset loading at those actual call
sites. Item actor/counter initialization remains at its original point. There
is no ownerless-script exemption, guessed model, lazy loader or duplicate import.

Client/content-tool/stage-data builds pass. The current stage-data test passes.
Full 29-stage cohort PASS: 28 exact old/new rendered trace comparisons; Green
Greens explicitly reproduces the old import failure and then verifies successful
current gameplay (not an impossible frame comparison against a crashed baseline).
Each candidate consumes 2,000 samples with medium items and four CPU slots, full
HSD draw, hidden/muted fast pacing, and zero census errors. All 27 pins independently
rehash unchanged. Fresh Mario, Zelda/Sheik and Popo/Nana packages also pass six
win/lose results transitions; all 72 pins rehash unchanged. Alternate demo scene
call sites are compiled and source-inventoried, not claimed as live acceptance.

Evidence: melee/docs/native_stage_asset_dependencies.md and
A:/FreakMelee-test-archives/stage-owner-order-oct11-roster/receipt.json and
A:/FreakMelee-test-archives/stage-owner-order-oct11-results/receipt.json.
The prior Ground checkpoint's Green Greens importer follow-up is resolved.
Explicit whole-match state ownership/serialization remains incomplete and active.

## 2026-10-11 - Actual Item owner and bone-table storage migrated

- Actual item construction now allocates stable Item-first slots; the optional
  100-pointer bone table belongs to that slot at the original model setup phase.
  Cancellation/script detach/retirement retain source order. No heap fallback.
- Item owner binds/retires with the GObj scene epoch; capacity matches the GObj
  limit, preserving uncapped hold categories and arbitrary authored articles.
- Occupied-record save/load checks typed roots and exact optional bone aliases.
  Two items occupy 45,656 snapshot bytes. This is one owner family only.
- Client/tool build and six focused fixture families passed, including actual
  model-table/destructor source bodies, malformed rejection and generation reuse.
  Fresh scripted Samus/Link/Ness/Peach CPU comparisons also passed.
- Seven old/new 2,000-sample CPU cases completed cleanly and matched all compared
  non-Item families, but strict receipts remain FAIL for Item hash differences.
  Investigate field-level differences; do not claim exact Item parity or weaken
  the comparer. Cleared reused heap bytes are a hypothesis, not a proven cause.
- 26,279,936 bytes of fixed Item backing remain in the broad page oracle while
  aggregate ownership is incomplete. No whole-match size/performance win yet.
- Details/evidence/limits: melee/docs/native_item_snapshot_ownership.md.
  ItemLinks, graphs, dynamics, globals, actor/storage and resource collections
  still need explicit ownership/save/load and aggregate restore validation.

- Follow-up: eight fresh source-vs-stock results cases passed (Ness, Peach,
  Game & Watch, Ice Climbers; win1/lose), exercising actual match-end/scene
  transitions. All 72 results receipt pins rehashed unchanged; no task-owned
  item-owner test client remains running. Local commit: a1a41b603.


## 2026-10-11 - Item allocation comparison closed; named-field diagnostics

The final allocation-isolated rendered CPU batch passes all seven cases: six
selectable stages plus Battlefield with Peach/Link/Ness/Samus, 2,000 samples
each. All compared families including Items match; full input consumption,
clean exits and zero census/VM/nonfinite errors. Fresh interpreted Samus, Link,
Ness and Peach packages also pass stock/source CPU comparisons, 1,800 samples
each. Hidden/muted tests; installed collections unchanged. Original and
intermediate failing receipts remain preserved, not relabelled as passing.

Named raw field tracing proved the former differences were collision query
outputs, disabled/empty victim payload, derived command-frame scratch, unused
stock Food command words, and two undefined collision initializer flags. The
actual shared initializer now sets its stop flag and previous environment
flags to zero. Previous collision flags, active geometry/damage/victim timers,
active command waits and authored Food command words remain compared.
These are read-only semantic digest corrections, not snapshot byte omissions;
the actual Item owner fixture still restores every occupied byte.

Client/tool builds, native_semantic_state and native_collision pass. The latter
now registers a real GObj for callback lifetime validation, preserving its
physics/wind/hitlag checks, and exercises poisoned reused collision bodies.
Item fixture passes 14 cases / 24,945 checks, including retained raw scratch.
The isolated baseline uses the old item.c allocation module and current shared
engine modules, so it does not claim undefined initial flags match the former
whole executable. Details: melee/docs/native_item_snapshot_ownership.md; final
receipts item-field-oct11-{stages,projectiles,source}-e on A: and independent
verification item-field-oct11-independent-verification.json.

The next explicit owners are actual chain/rope ItemLinks and their authored
resource collections, then model/constraint/animation/dynamic graphs, stage
collision and remaining globals. Broad page coverage remains in production;
complete aggregate save/load and measured rollback budget are not achieved.
Goal stays active. Local Git only; no push.

2026-10-11 checkpoint: Item diagnostic/allocation closure committed locally as eabe22ec1. Independent verification PASS: 1607 owner, 36 stage, 36 projectile, 55 source-package input pins unchanged; semantic/collision focused CTests PASS. Next ownership work: unified stock/authored ItemLink pool and occupied-record snapshot. Goal remains active; no push.

2026-10-11 checkpoint: unified stock/authored ItemLink ownership committed locally as d2bb39d07. All five stock constructors and authored resource nodes allocate directly from nativeItemLinkPool; old stock allocator, authored heap backing, and Allocate/Free forwarding APIs removed. Actual 536-byte records, generation/free order and occupied-only Save/Validate/Load; two live links 33896 bytes; no standalone match or timing claim. Actual Item+chain owner fixture PASS 24 cases/33228 checks; other GObj/Fighter/Ground/scene fixtures and four focused CTests PASS. Three old/new CPU stage cases PASS (Link/Samus/Ness/Ice Climbers, 2000 samples); fresh interpreted Samus/Link/Young Link/Ness PASS (1800 each). Scripted resource ordinary/animated and broad restore/replay PASS, with explicit actual authored-node record save/restore and live generation references checked. Nine receipts independently verified with unchanged pins and client/archive/baseline hashes. Evidence: A:/FreakMelee-test-archives/item-link-oct11-*. Complete model/animation graph, resource/script registry and module-global ownership plus aggregate restore and 15-tick performance acceptance remain. Interpreted fighter/item state and authored callbacks are mandatory snapshot acceptance, not stock-only coverage. Older extracted stock-handler fixtures need their controlled allocation boundaries updated before rerunning; not claimed as migration acceptance. Goal active; no push.


2026-10-11 explicit snapshot checkpoint - reserved HSD graph records
Local commit 17836cdb1. Actual registered authored model/resource reservations now save occupied class/ObjAlloc records, generations, exact free order, pool counters and ended claim failure state without heap allocation or reconstruction. All owners prevalidate before restore. Client/content/source-assets/ObjAlloc targets build; native_objalloc CTest passes. Focused actual allocator fixture:121 checks,10 malformed candidates,760-byte four-record component. Connected scripted Arrow/Boomerang fixtures PASS ordinary2720 checks/14cases and animated4447/15cases, including39 VM/native animation pairs and explicit exact transform/matrix/material/source-reference restore. All339/3334 receipt pins independently verified. Evidence:A:/FreakMelee-test-archives/hsd-snapshot-oct11-*; detailed scope:melee/docs/native_hsd_reservation_snapshot_ownership.md.
Production broad page oracle remains. Shared HSD allocator/class counters, ordinary stock graphs/heaps, registry closure and aggregate restore remain work; no whole-match size/timing claim. Existing Fighter/Item script-state serializers must join aggregate restoration:custom fighter variables,timers,persistent state,callback bindings and resources are mandatory acceptance. Eight-actor15-resimulation-tick frame target still unproven.
Correction to prior ItemLink note:older extracted stock-handler fixtures read pinned SDKs and are unaffected as historical evidence. Fresh current-SDK runs would need direct-pool fixture boundaries. Current owner/resource/client comparisons remain the ItemLink migration evidence. Goal remains active.


2026-10-11 explicit snapshot checkpoint - shared HSD ObjAlloc accounting
Local commit 200fafb9a. HSD_CreateMainHeap now forgets the old registry before HSD_ObjInit registers reset descriptors; the census no longer supplements missing named allocators. Actual registry descriptors and object-arena cursor have explicit save/load with transition epochs, fixed scratch and full prevalidation; no heap allocation or constructor callback during save/load. Ordinary free-link/payload backing, OS heap state/current selection, class counts and ID roots remain separate aggregate ownership. Source graph fixtures pair the reserved graph and descriptor components. Production broad oracle remains; no whole-match size/timing claim.
Acceptance:A:/FreakMelee-test-archives/hsd-descriptors-oct11-owner-c (236checks/115descriptorchecks/10descriptorrefusals); census-b (14cases48checks); animation-a (90cases1082checks); resource-a (ordinary2725/14 and animated4452/15 with39VM/native animationpairs); stages-b (Battlefield,GreatBay,Brinstar each2000sample exact previous-client comparisons withLink/Samus/Ness/IceClimbers/mediumitems); source-a (freshinterpretedSamus/Link/YoungLink/Ness each1800samplesPASS). Main/content/source-assets/ObjAlloc/census builds and focused ObjAlloc/census CTestsPASS. Sixreceiptpinsets341/1593/1604/3335/208/55 independently verified, with baseline bundle and compiled owner source matches. Earlier failed fixture/baseline bundle attempts are retained, not relabeled.
Details:melee/docs/native_objalloc_snapshot_ownership.md. Remaining ordinary HSD graph/allocator payload ownership and aggregate save/load stay required. Script-driven fighter/item state and resources remain mandatory companions. Overall goal active; nothing pushed upstream.


2026-10-11 explicit snapshot checkpoint - ordinary HSD class storage
Local commit 6ca50a6fc. Actual class-piece chunks, size-class tables/entries, free links and initialized class live/peak counters now have explicit save/load. Allocation sites record real block address/size in a fixed directory; only new OS chunk acquisition adds constant metadata work. Piece reuse is unchanged. Save/load has no allocation, page protection, destructor or callback; complete candidate validation precedes writes. Class definitions must be initialized before history; unexpected late types reject restore. OS Cell metadata and other satellites remain companion work; production broad oracle stays active.
Acceptance:A:/FreakMelee-test-archives/hsd-class-oct11-owner-b (343total/107classchecks/12classrefusals;4336-byte syntheticclasscomponent); census-a (14cases48checks); resource-a (ordinary2733/14,animated4460/15,39VM/nativepairs; actualordinaryJObj mutation/exactrestore/re-save alongside reservedgraphs/shareddescriptors); stages-a (Battlefield,GreatBay,Brinstar each2000samples exactpreviousclienttraces); source-a (interpretedSamus,Link,YoungLink,Ness each1800samplesPASS). Client/content/source-assets/ObjAlloc/census/heap builds and focused CTestsPASS. All344/1594/3336/210/55 receiptpins,baselinebundle and4compiledsources independently verified. Initial fixture link failure retained as owner-a.
Details:melee/docs/native_class_snapshot_ownership.md. Remaining:ordinary ObjAlloc/satellite/direct-HSD backing,OS heap metadata/globalroots/source registry closure and aggregate ring/restore. Existing Fighter/Item script serializers remain mandatory for variables,persistentstate,timers,callbacks,relationships and resources. No whole-match size/timing or15-resimulationtick deadline claim. Overall goal remainsactive; nothing pushed.


2026-10-11 explicit snapshot checkpoint - ordinary HSD ObjAlloc backing
Local commit 113edc77f. Existing shared descriptor/cursor serializer now also owns the actual ordinary backing blocks and their payload/free-link bytes, from fixed object arenas or OS heaps. HSD_ObjAllocAddFree records address/size/type directly into a fixed directory; slot reuse/free keeps existing behavior. Save/load copies only registered blocks, excluding unused arena/directory capacity and reserved payload. All candidate descriptors and blocks prevalidate before writes. OS heap/Cell metadata remains a required companion across growth; native script state/resources still must join aggregate restoration.
Acceptance:A:/FreakMelee-test-archives/hsd-blocks-oct11-owner-b (410totalchecks;115fixed-arena/descriptor checks with10refusals;67OS-backedobjectchecks with6blockrefusals; bothsyntheticobjectcomponents480bytes); census-a (14cases48checks); resource-a (ordinary2736/14,animated4463/15,39VM/native animationpairs,actualordinaryVec/Mtx value restore andexactre-save withoutpayloadcopycompanion); stages-a (Battlefield,GreatBay,Brinstar each2000samples exactpreviousclienttraces withLink/Samus/Ness/IceClimbers/mediumitems); source-a (interpretedSamus/Link/YoungLink/Ness each1800samplesPASS). Main/content/source-assets/ObjAlloc/census/heap builds and focused CTestsPASS. All345/1594/3336/210/55 receiptpins,baselinebundle and5compiledsources independently verified. Initial wrong-address fixture perturbation usedthecorrectaddress;failedowner-a retained.
Details:melee/docs/native_objalloc_snapshot_ownership.md. Remaining:OS heap/Cell metadata,ID/globalroots,directHSD buffers/source registry closure,classpreparation andaggregate ring/restore. Broadproductionoracle remains; nofull-matchsize/timing/eight-actor15-resimulationticksclaim. ExistinginterpretedFighter/Item serializers mustretainvariables,timers,persistentstate,callbacks,relationships/resources. Overallgoalactive; localcommitonly.

2026-10-11 explicit snapshot checkpoint - OS heap metadata and default ID roots
Local commits: melee a872927a9; Aurora 4468232. Actual 32-byte OS Cell headers, heap descriptor controls/current OS heap selector and default HSD ID bucket roots now have explicit save/validate/load. The OS component excludes payloads, uses fixed scratch with O(n log n) metadata validation, and retires history only across geometry/lifetime changes. Ordinary alloc/free paths add no snapshot tracking. ID roots restore without traversing intermediate entry graphs; actual entries/payloads remain their class/ObjAlloc/reservation owners. All companions must prevalidate, restore OS headers before payloads, and complete restoration before callbacks resume.
Acceptance: A:/FreakMelee-test-archives/hsd-controls-oct11-owner-a PASS (66088 checks;108 class,68 OS-backed ObjAlloc,32 ID,65644 OS; most OS checks are repeated minimum allocations reaching the scratch ceiling, not distinct gameplay cases). Resource-a PASS (ordinary2748/14cases,animated4475/15cases,39VM/native pairs,exact five-component restore including real ordinary JObj/Vec/Mtx and reused/growing ID entries). Census-a PASS14cases48checks;stages-a exact previous-client comparisons on Battlefield/GreatBay/Brinstar each2000samples;source-a interpretedSamus/Link/YoungLink/Ness each1800samplesPASS. Client/content/source-assets/ObjAlloc/census/heap builds and3focusedCTestsPASS. All351/1596/3339/214/55 receiptpins,baselinebundle,7compiled owner sources and mirrored OS headers independently verified.
Details:melee/docs/native_hsd_control_snapshot_ownership.md. Remaining:HSD heap selector and other module globals,direct mutable HSD buffers/source registries,class preparation,aggregate ring integration and measured whole-match size/timing. Existing interpreted Fighter/Item variables,timers,persistent state,callback bindings and resource relationships are required aggregate companions. Broad production oracle remains; no full-match performance or15-resimulation-tick deadline claim. Overallgoal active; nothing pushed upstream.

2026-10-11 explicit snapshot checkpoint - actual GObj scheduler and interpreted actor companions
Local commit5276f70da. Actual GObj/GObjProc pools and configured scheduler tables now restore together with invocation/deferred controls, render kinds/flags and the pointed process mask. Config/method bindings remain scene-frozen; lifetime and serialized topology validate before writes. Fixed host scratch is outside .mstate; normal allocations/dispatch add no snapshot tracking. No constructors/destructors/callbacks/page protection or runtime allocation during save/load. Other userdata/graph/VM/resource owners remain aggregate companions.
Acceptance:A:/FreakMelee-test-archives/gobj-snapshot-oct11-owner-c PASS17cases41334checks,including20576schedulerchecks/13candidate refusals. Most additional checks are repeated actual allocations reaching4096objects/16384procs,not distinct gameplay cases; three-object/four-proc component164872bytes. Exact callback/deferred removal/priority/render replay, slot generation/address reuse, empty/full pools and full configured table extents verified. Resource-c PASS ordinary2816/15cases,animated4543/16cases,39native/VM animationpairs,68scheduler-script checks per run: actual interpreted fighter/item variables, item tick/callback state, mutable fighter callback binding and scheduler state restore/re-save exactly, then original dispatch and next allocation repeat. Connected scheduler179584bytes is not a full-match size estimate.
Client/content/source-assets/GObj/census builds and2focusedCTestsPASS. Stages-a exact previous-client comparisons on Battlefield/GreatBay/Brinstar each2000samples withLink/Samus/Ness/IceClimbers/mediumitems and no census failures. Source-a interpretedSamus/Link/YoungLink/Ness each1800samplesPASS. All1616/3341/217/55 receiptpins,baselinebundle and6compiled/frozen sources independently verified. Failedowner-a (new proc flag3 incorrectly excluded),resource-a (fixture nativeItemmsid not published) andresource-b (program callback query used instead of instance binding) retained. Production VM contracts were not relaxed.
Details:melee/docs/native_gobj_snapshot_ownership.md. Broad production oracle remains. Remaining:HSD selector,other module globals,direct mutable HSD buffers/source registries,class preparation,complete aggregate/ring and measured full-match save/load/resimulation cost. No full-game timing/eight-actor15-resimulationtick/multi-PC/pixel acceptance claimed. Overallgoalactive; localcommitonly.

2026-10-11 explicit snapshot checkpoint - variable graph payloads and shape scratch
Local commit c4d13c6c9. Texture-animation palette pointer tables and additive shape weights now use actual class-piece storage; ordinary class/OS and reserved graph/class components restore them without a separate tracking directory. Original replacement/destruction order and exact sizes retained, including zero-byte shape handling. Reserved construction must include these sizes in its demand; existing source article shape admission limits remain.
Shape interpolation now uses stable 2000-entry vertex/normal arrays (48000 bytes) outside .mstate. Old lazy allocations/pointer flags/amnesia resets removed. Original unsigned capacity checks retained; display-list position/normal/NBT indices must stay inside the current regenerated prefix. No clearing, tick allocation or save/load copies for this scratch. RObj expression scratch remains open because counted flags do not always write argument values; no speculative exclusion.
Acceptance: A:/FreakMelee-test-archives/graph-buffers-oct11-resource-c PASS ordinary2911checks/17cases,animated4638/18;56 ordinary graph-buffer checks,39 reserved checks,39 native/VM animation pairs and68 scheduler/script checks per cohort. Actual constructor/destructor/palette replacement, mutation, reuse, exact component re-save and repeated next allocations verified. Scratch-d PASS2859checks/112 bit-identical interpolation and emitted-GX-value pairs/13 capacity-index refusals; poisoned fixed scratch recomputes all consumed values,prior lazy path allocates2/fixed0. Source math and display-list bodies execute;GX leaves controlled,no pixel claim.
Build-c client/content/source-assets/GObj/census targetsPASS. Stages-b Battlefield/GreatBay/Brinstar each2000 rendered CPU samples with Link/Samus/Ness/IC/mediumitems exactly match previousclient traces with no census errors. Source-b interpretedSamus/Link/YoungLink/Ness each1800samplesPASS. Actual productionCOFF verifies24000-byte .data arrays each. Independent verification all235/3342/219/55/4pins,87baselinefiles and6production/frozen sourcesPASS. Earlier evidence retained; scratch-a strict compile warning failure documented.
Details: melee/docs/native_graph_buffer_snapshot_ownership.md. Broad production oracle remains. Remaining:HSDselector,other module globals/source registries,direct mutable buffers including constraint scratch,classpreparation,complete aggregate/ring and measured whole-match size/save/load/resimulation. No complete snapshot,whole-match timing or eight-actor15-resimulationdeadline claim. Overallgoalactive;localcommitonly.
2026-10-11 explicit snapshot checkpoint - scripted article resource registries
Local commit ab57d0e4b. Actual occupied collection prefixes/created nodes and generation counter now save/load directly; retained Item/Fighter, ItemLink/GObj/model identities and borrower bindings restore with companion owners. Prepared programs/recipes/assets stay leased. Scene/model preparation identities reject old snapshots after same-epoch retire/reprepare. No mirror registry, tick tracking, snapshot heap allocation, constructors, destructors or callbacks. Complete candidate validation precedes live writes; cross-owner candidate closure remains aggregate work.
Acceptance: A:/FreakMelee-test-archives/resource-registry-oct11-resource-e PASS ordinary3082checks/18cases,animated4812/19; newregistry172/175checks,14refusals each; two-node registry580bytes (not full-match size). Eleven actual components restore after real animated graph/controller/endpoint destruction, Item VM mutation and future reuse. Fighter part/hitbox borrowers and exact component bytes return; next generation and GObj/Link/model allocation order repeat. OS Cell metadata remains unchanged in this bounded window. Fixture uses controlled small rig and Item construction; no production Item-constructor/full-aggregate claim.
Build-c targetsPASS. Stages-a Battlefield/GreatBay/Brinstar each2000rendered CPU samples with Link/Samus/Ness/IC/mediumitems exactly match prior client traces; no census errors. Source-a interpretedSamus/Link/YoungLink/Ness each1800samplesPASS. Independent verification all3345/228/66receiptpins,87baselinefiles and11production/fixture filesPASS. README/ownership docs updated after tests; compiled inputs unchanged. Earlier fixture compile and undeclared-child animation failures retained/documented; production animation admission unchanged.
Details: melee/docs/native_article_resource_snapshot_ownership.md. Remaining: direct mutable HSD buffers/constraint scratch, effects/dynamics/collision/world/match globals, class preparation, cross-owner aggregate/ring integration, and measured whole-match bytes/save/load/resimulation cost. Broad production page oracle remains; no complete snapshot or eight-actor15-resimulation deadline claim. Overall goal active; local commit only, nothing pushed.
2026-10-11 explicit snapshot checkpoint - stage collision and moving-platform companions
Local commit 4733b9140. Actual runtime collision arrays use stable original capacities and serialize active prefixes alongside writable source vertex/line/joint descriptors. Joint/island roots, dynamic attributes, bounds/culling, query state and update counters participate. Island allocations use the existing class owner; class/ObjAlloc/OS companions restore payloads and graph bindings. Scene identity plus GObj epoch and frozen address/count binding reject same-epoch map reload. Save/load invokes no callbacks and allocates nothing; class preparation and cross-owner candidate closure remain open.
Native mpLinesConnected now indexes typed sixteen-byte native records instead of the GameCube two-u32 stride. Controlled regression covers nonzero indices, both directions, disabled/hidden flags and distant endpoints. Original rules retained. Attribute validation permits records left outside both chains by the original non-head expiry path; every physical record and canonical next link is saved/validated. NaN runtime bits remain reproducible.
Acceptance: A:/FreakMelee-test-archives/collision-oct11-owner-e PASS145checks/4cases/14refusals,1184bytes for a synthetic6vertex/3line/3joint map,not fullmatch. Actual matrix/platform/floor/speed/island/attribute kernels, model destruction/reuse, four-component prevalidation/restore, exact re-save and repeated mutations verified. Boundary/alias refusals and same-epoch reload pass. Build-d targetsPASS. Resource-a ordinary3082/animated4812checksPASS (preceded final collision-only validation correction);39native/VManimationpairs and68scheduler/scriptchecks retained. Source-a interpretedSamus/Link/YoungLink/Ness each1800samplesPASS.
Stage parity remains partially OPEN: stages-a Battlefield/GreatBay each2000renderedCPUsamples exact previousclient/noerrors;Brinstar old/new FAIL first P2positionY onefloatbit atsample1072,with normal exits/noVM/nonfinite/censusfailures. Self-brinstar-a two same-new-client runs exact2000samplesPASS. Difference not attributed or waived. Independent component provenance PASS3330/3351/76pins,87baselinefiles,12compiledproduction/fixture sources; explicitly records stage parity FAIL separately. README/docs updated after tests,compiledinputs unchanged.
Details: melee/docs/native_collision_snapshot_ownership.md. Production broad page oracle remains. Remaining: Brinstar discrepancy attribution, other world/match/effects/dynamics/HSD control state, preparation, cross-owner aggregate/ring integration, measured whole-match bytes/save/load/resimulation and eight-actor15-resimulationdeadline. No full snapshot,complete stage parity or timing acceptance claim. Overall goal active; local commit only, nothing pushed.

2026-10-11 collision follow-up - Brinstar query correction attributed
Local commit 8145b6833. Independent control/unfixed-query mplib recompiles use production flags and read-only libraries outside the checkout. Unfixed substitutes exactly the three pre-migration connectivity functions; all owner changes stay current. query-control-b exact2000samplesPASS; original client vs isolated-unfixed exact2000samplesPASS. Original/current and isolated-unfixed/current have the same15differingfighterrecords starting1072; actions/damage/items/resources/RNGmatch. Thus native line-stride correction accounts for this bounded difference. Correct typed indexing remains, supported by collision rule fixture. Old traceFAIL is retained; no trace fields normalized/waived.
Archives: A:/FreakMelee-test-archives/collision-oct11-query-isolation-a,query-control-b,query-unfixed-b,query-attribution-final.json,query-independent-verification.json. AttributionPASS and independentPASS87buildpins/237comparisonpins each/258deploymentpins,exactsource substitution and executable hashes. Initial diagnostic launches lacked UI and failed before gameplay; failed receipts retained, runtime deployment now pinned. Production code/build unchanged this checkpoint.
HSD control audit: production selector writes are initialization/recreation and allocation/free helpers that restore prior selector before return; final aggregate must capture/enforce idle selector. RObj expression arguments remain mutable because counted flags can leave argument entries unwritten. Remaining other owners,aggregate/ring and live measured whole-match sizes/save/load/resimulation. No full snapshot,timing,eight-actor15-resimulation or console pixel acceptance. Overallgoalactive;localcommitonly.


Snapshot live census checkpoint (2026-10-11): Offline --snapshot-census now attempts all 16 existing Save/Validate components at complete post-draw boundaries, with independent pre-touched host scratch and no Load or page protection. Four hidden/muted runs of stock and interpreted four-pair Ice Climbers consumed 1200 samples each with exact diagnostic-on/off traces and eight actual actors; eight script actors save 1904 bytes. Thirteen owners save/validate at all ten samples; GObj/collision/article resources reject because their idle guards mistake the shadow renderer's retained current-render-object pointer for an active callback. Preserve that state and distinguish actual callback activity in the next integration step; do not clear original render context to make guards pass. Existing production page oracle remains, and full aggregate bytes, restoration/closure, remaining world/effect/dynamics ownership and the 15-tick performance deadline are not accepted. Details: melee/docs/native_snapshot_live_census.md; final evidence: A:/FreakMelee-test-archives/snapshot-census-oct11-eight-final/ (receipt and independent-verification PASS, 15 unchanged pins). Mean successful-component save costs about 734 us in the interpreted cohort, with separate size-query/validation costs; this excludes missing owners and is not full-match save/load timing.


Retained-render-context checkpoint (2026-10-11): GObj snapshots now save/restore the existing shadow renderer context, while actual callback depth distinguishes active/nested rendering from retained state. GObj/collision/article-resource guards and scene preparation/retirement no longer confuse those states. New capture requires live owned context backing; candidate restore permits resurrection after current object destruction/reuse. All 16 existing owners now save/validate at ten boundaries in both stock and interpreted eight-actor matches; all diagnostic-on/off and previous/current populated gameplay traces match across 1200 samples per run. Scheduler (41360 checks, 16 malformed refusals), collision (149 checks, 14 refusals) and connected script-resource ordinary/animated restoration (3083/4813 checks) passed. Local evidence: A:/FreakMelee-test-archives/render-context-oct11-*, with independent verification PASS. No original shadow/render-context behavior changed. Remaining: complete match/world/camera/effect/dynamics ownership, multi-heap/selector and variable-buffer coverage, aggregate restore order/closure and actual complete snapshot/resimulation timing; production page oracle remains. See melee/docs/native_snapshot_live_census.md and native_gobj_snapshot_ownership.md.


HSD selector/constraint snapshot checkpoint (2026-10-11): Direct owners in memory.c and robj.c now restore the separate HSD heap selector (16 bytes), persistent expression argument root/allocation heap and all 100 floats (24 bytes empty, 424 allocated). Existing lazy allocation and retained values are preserved. Snapshot operations refuse active expression/update and construction callbacks, retired heap geometry and overlapping storage. Physical provenance uses a fixed epoch-bound heap range catalog; live allocation checks require actual allocated-list membership, preventing stale interior Cell headers from authorizing payload writes. OS metadata must restore before argument payloads. Actual owner fixture: 137 checks, eight malformed refusals, first-allocation rewind, retained tail and exact re-save. Allocator fixture: 66106 checks including capacity, carved extents, coalescing and lifetime. All 18 components save/validate at ten sampled boundaries in both stock and interpreted eight-actor matches; all diagnostic-on/off and previous/current trace families match. Final evidence A:/FreakMelee-test-archives/hsd-control-oct11-{owner-final-b,alloc-final-b,eight-final}/ and hsd-control-oct11-final-verification.json. Remaining full engine/world/particle ownership, other OS heaps/payloads, cross-owner candidate closure, aggregate/ring restoration and measured complete snapshot/15-tick performance. Production page oracle unchanged; goal active; local commits only. See melee/docs/native_hsd_control_snapshot_ownership.md and native_snapshot_live_census.md.


Controller snapshot checkpoint (2026-10-11): Actual HSD raw queue, PadLib clamp/ADC/scale and circular queue state, Master/Copy/Game statuses and button/repeat history, native input extensions, four rumble channels and complete command-slot/free-list state now have one explicit component (1896 bytes for the boot layout). Physical controller mapping/output suppression stay host-owned; Load emits no device calls. Actual filter setter and backing initialization retire history on identity/layout changes; metadata is excluded from .mstate. Production input/rumble algorithms remain unchanged. Controlled owner fixture passes 354 checks and fifteen malformed refusals, with exact twelve-tick re-execution from a full wrapped queue and active rumble lists, exact next allocation, early-disconnect/activity/overlap/refusal tests and explicit final motor reconciliation. All nineteen components save/validate at ten boundaries in stock and interpreted eight-actor matches, with exact diagnostic-on/off and previous/current populated trace families. Evidence A:/FreakMelee-test-archives/controller-snapshot-oct11-{owner-final,eight-a}/ and controller-snapshot-oct11-independent-verification.json. Snapshot buffers have no per-tick allocation; diagnostic component save averages about half a microsecond, not a complete save/load performance result. Remaining: GM/LB clocks and scheduling/RNG, complete world/camera/effect/dynamics/other globals, other heaps/payloads, cross-owner command/graph closure, aggregate/ring restoration and measured complete rollback timing. Production page oracle retained; goal active; local commit only. Details: melee/docs/native_controller_snapshot_ownership.md.

2026-10-11 clock/RNG checkpoint: actual native input scheduler cadence (periods, deadlines, virtual time, both phase/update gates and lifecycle flags) now has a 56-byte explicit component, and actual shared RNG seed has an 8-byte component. Restore invokes no devices, audio polls or scene initializer; original scheduling and RNG algorithms remain unchanged. Actual source uses random.c's own seed binding; an external binding rejects rather than silently losing mutable target state. Controlled production fixture passes 255 checks with exact 24-tick re-execution of controller/clock/RNG state across speed transitions, stalls and skipped phase updates, two callback-context refusals, malformed/overlap/no-partial-write tests, and seed zero/wraparound. Existing 600-tick input timing tests pass. All twenty-one explicit components save/validate at ten sampled boundaries in both stock and interpreted eight-actor live matches; diagnostic on/off and previous/current populated trace families match exactly. Evidence A:/FreakMelee-test-archives/clock-random-oct11-{owner-b,eight-a}/ and clock-random-oct11-independent-verification.json. Tiny component timings do not establish full aggregate save/load or rollback budget. GM match clocks/pause/process masks, world/camera/HUD/effects/dynamics globals, other heaps/payloads, candidate graph/resident lifetimes, aggregate/ring integration and complete restoration/performance remain required. Production page oracle remains; goal active; local commit only. See melee/docs/native_clock_random_snapshot_ownership.md.

2026-10-11 GM loop/input checkpoint: actual GM counters, scene exit, current/previous pause and frame-step controls, process masks/selector/update flags, GObj configuration, scene/debug/scheduler roots and cadence masks now have a 216-byte component. Actual mapped-controller/combined-port and accelerated-repeat state now has a 264-byte component, with descriptive field names and direct callsites. Entry/scene/code identity guards stay host-owned in .data; actual mutable roots stay in .mstate$m. Loop activity guards balance early paths and preserve rollback/census boundaries. Load emits no device/audio/constructor/callback calls and requires exact restored GObj generations before any GM writes. The production loop fixture passes 1415 checks and exact 16-tick re-execution across pause/frame-step/cadence/input/RNG state, 13 malformed/stale refusals, 144 callback/activity attempts, real pool resurrection ordering and same-pointer scene-entry retirement; 128-frame mapped repeat/macro sequences also match. All 23 components save/validate at ten boundaries in stock and interpreted eight-actor live clients. Evidence A:/FreakMelee-test-archives/gm-loop-oct11-{owner-final,eight-final}/ and gm-loop-oct11-independent-verification.json verifies frozen/current pins, prior/current populated gameplay traces and compiled metadata section placement. Earlier live eight-a is disqualified because its executable pin changed during a concurrent build; final evidence is after completed build with stable pins. The single-call old pause/mask initializer was folded directly into its existing loop callsite, with obsolete body/declaration removed. Remaining: shared GM/GObj mask agreement in aggregate prevalidation; other player/world/stage/camera/HUD/effect/dynamics globals; multi-heap storage; resident/cross-owner closure; complete aggregate/ring restoration and actual rollback performance. Component timings are not full save/load acceptance; page oracle retained; goal active; local commit only. See melee/docs/native_gm_snapshot_ownership.md.


2026-10-11 explicit player-state checkpoint (goal remains ACTIVE; local Git only)
- Added a 23,080-byte explicit component for all six actual player records: stocks/spawn/physical transformation roles, stale tables, full action/hit/bonus/extended statistics and both 16-bit attack-instance counters. GObj and Fighter root references retain exact scene/generation identity; their companion payloads restore first. No tick allocation, constructors, device/audio calls or second player representation.
- Frozen family/common-data/storage identities and host scene epochs reject changed definitions and retired history. The actual GM idle predicate is shared directly; input/simulation/draw/callback activity rejects. All roots resolve before any player/counter write; last-member failures leave every earlier record unchanged.
- Split 122 original record functions directly into player_state.c (1,406 lines), leaving actor/asset interactions in player.c (<1,000). All 172 original function bodies compare exactly with c7f8b57cd. Respawn callback/setter signatures now agree on s32 instead of void-pointer conversion; its body is unchanged. Mutable roots remain in .mstate$m, frozen owner metadata outside it.
- player-oct11-owner-final: 2,018 checks, exact 16-tick rewind/re-execution, 17 malformed player candidates, extended stats/role order/counter wrap, actual GObj/Fighter generation resurrection and no-write refusal. Actor construction and scheduler/hardware endpoints are controlled; not full HSD graph restoration.
- player-oct11-eight-final: four hidden/muted rendered 1,200-sample stock/interpreted eight-actor runs. All 24 component Save/Validate checks pass at all ten boundaries per scenario; diagnostic on/off and previous/current populated trace families match. Census never loads. Native client/content/source-assets/GObj-consumer targets build. Independent verification checks 1,815 fixture and 49 live pins, logs, compiled sections and donor bodies.
- Remaining: other GM/world/stage/camera/HUD/effect/dynamics globals, multi-heap payloads, resident leases, candidate graph agreement, aggregate load ordering/history and complete full-match restore/resimulation timing. Player-only observations are about 4.5-5 us Save; total bytes, sub-100 us save/load and fifteen rollback ticks within a frame remain UNPROVEN. Production rollback still uses the broad page oracle.


2026-10-11 explicit camera-target checkpoint (goal remains ACTIVE; local Git only)
- Actual CmSubject targets and original active/free roots now have stable fixed storage and explicit Save/Validate/Load. Versus retains its 70-target logical capacity; physical capacity is 256. All requested records are saved, including free history and the obsolete next links intentionally retained by the original release method. The versus component is 8,456 bytes. No tick allocations or second target representation.
- Four actual allocation/reset/release entrypoints moved intact into camera_subjects.c; all 160 original camera bodies compare unchanged except the initializer's explicitly checked subject-allocation replacement. Camera pose/smoothing/quake reset remains unchanged. Host scene/storage identity remains outside .mstate$m; actual targets/roots remain covered by the broad oracle.
- Scene initialization, loop re-entry and retirement reject old history even at the same address/capacity. Candidate links are checked as numeric slot claims without foreign dereferences. Active reciprocity/tail plus free-prev traversal must cover the prefix exactly once. Complete validation precedes any Load write; Save validates before touching either output or its byte count. GM input/simulation/draw/callback/script/construction activity refuses.
- camera-targets-oct11-owner-final: actual target methods plus production GM/input/controller/RNG/player/Fighter-pool bodies repeat sixteen ticks exactly. 31,797 target checks, sixteen malformed target candidates, 144 activity boundaries, empty/full chains, free history, unaligned input, no-write failures and one/256-capacity endpoints pass alongside the 2,018 GM/player checks. External scheduler/host/device/render endpoints and actor construction are controlled; not full HSD graph restoration.
- camera-targets-oct11-eight-a: four hidden/muted rendered 1,200-sample stock/interpreted eight-actor runs. All 25 Save/Validate components pass all ten boundaries per scenario. Diagnostic on/off and previous/current populated gameplay traces match exactly. Independent verification checks 1,819 original/frozen fixture pins, 55 live pins, source bodies, logs and compiled sections. Client/content/source-assets/GObj-consumer targets build; GObj-consumer CTest passes.
- Component Save averages 1.51/1.57 us, Validate 0.46/0.43 us (ten observations stock/interpreted). These are not full-match or worst-case timings. Main-camera smoothing/pause/orbit/quake and CObj/WObj state, stage/world/HUD/effect/dynamics globals, other heap/payload ownership, resident leases, candidate borrower agreement, aggregate restore ordering/history and full-match differential/performance acceptance remain open. Production still uses the broad page oracle; total bytes and fifteen rollback ticks within a frame remain UNPROVEN.


## 2026-10-11 main-camera explicit snapshot checkpoint

Actual main camera transforms/smoothing/pause/orbit/quake/render masks, debug
state, coefficient table and auxiliary CObj root now have a 1,424-byte explicit
component. Actual camera globals remain in the broad oracle section while the
scene lease stays in host state. The camera uses generic fixed-span references
to retained ordinary class backing and generation references to primary/quake
GObjs. Class and GObj companions retain graph payload ownership; it is not
copied into the camera component again. Generic class references are backing
claims, not proof that a piece is occupied; aggregate candidate membership and
consumer type/role checks remain necessary.

A native-layout assumption in quake scaling was exposed by the source split:
separate globals were cast as one contiguous GameCube data block. Direct use of
the actual camera descriptor restores the original eight-fighter traces exactly.
World-to-screen projection also reserved only a 3x4 matrix for 4x4 SDK writers;
it now reserves Mtx44. The controlled AddressSanitizer negative control proves
the old overflow. The interest callback type now agrees with its bool-returning
setter and actual callers. Original camera function bodies and coefficients are
independently compared; only the documented retirement/type/layout changes are
accepted, and unused tail data definitions were removed.

Evidence under A:/FreakMelee-test-archives/:
- camera-state-oct11-owner-final: connected real class/GObj-pool camera owner
  restore, 114 camera checks, 14 malformed candidates, 16 repeated controlled
  updates, generation reuse, class payload restoration, lifetimes and activity.
  Total connected allocator/class/heap/ID/camera fixture checks: 66,237.
- camera-state-oct11-gm-b: existing actual GM/controller/input/RNG/player/target
  restore fixture remains passing; it has no main-camera graph.
- camera-projection-oct11-owner-final: 75 numeric checks, actual extracted
  projection/GXProject/quake/setter bodies and Aurora matrix/vector sources,
  controlled accessors/stage constants, ASan negative control and corrected pass.
- camera-state-oct11-eight-b: four hidden/muted rendered 1,200-sample runs with
  medium items and eight actual actors. All 26 components capture/validate at
  all 10 boundaries in stock and independent interpreted scenarios. Diagnostic
  on/off and previous/current populated gameplay trace families match exactly.
- camera-state-oct11-independent-verification.json: fixture/live/generated
  source and log pins, compiled state/host sections, all 156 donor camera bodies
  and documented exceptions. Client/content/source-assets/GObj-consumer builds
  succeed; focused GObj-consumer CTest passes.

Camera Save means are 1.03 us stock / 0.95 us interpreted; Validate means are
0.10 / 0.11 us (10 observations per scenario). These are component costs, not
complete snapshot or worst-case rollback performance. See the repository docs
native_camera_snapshot_ownership.md and native_snapshot_live_census.md.

Remaining explicit work: stage/world/other GM/HUD/effect/dynamics globals,
other heap/payload ownership and resident leases, cross-component candidate
agreement/prevalidation, aggregate restore/history, full-match differential
re-execution and eight-scripted-actor fifteen-resimulation-tick performance.
Production rollback still uses the broad page oracle. No complete size, full
Load, sub-100-us save/load, or complete goal acceptance is claimed here.


### 2026-10-11 shared stage explicit snapshot checkpoint

Implemented the actual shared StageInfo owner, 64 fixed joint-update flags,
joint-update counter, spawn cache, mutable ground scale and nine lighting
colors. Ground and grLib access the actual modular storage directly; the old
64-byte transition heap allocation is removed. Stable joint backing references
use a checked host cache; changed attachments refresh from the real class
allocator. Map objects carry GObj/Ground generation claims; named joints and
four camera-model pointers carry class backing/type claims. The camera slots
are CObj pointers, not GObj handles. Fixed scene resource/callback identities,
buffer independence, complete pre-write local validation and companion restore
ordering are explicit. No allocation, callbacks or drawing occur in Save/Load.

The GM loop starts before the stage's deferred startup callback queue drains.
The stage lease therefore begins at the real completion of Ground startup and
retires at Ground initialization or GM loop exit. Broad stage testing also
identified live GroundParam lighting-color changes through aliased setters on
Corneria and Venom. Those colors are mutable snapshot data, not frozen resident
configuration. The four dynamic surface-attribute records are already owned by
the collision component and are not duplicated here.

Evidence under A:/FreakMelee-test-archives:
- stage-state-oct11-owner-final-c: actual Ground/class owner companions; 140
  shared-stage checks, fourteen malformed candidates, no partial writes,
  sixteen repeated controlled updates, scale/colors/attachment changes,
  generation reuse, class type restoration and activity/lifetime refusals.
  Controlled GObj backing, selected-stage getter, scheduler and graph setup.
- stage-state-oct11-gm-final-c: connected GM/controller/input/RNG/player/camera
  targets fixture passes with sixteen repeated controlled ticks. No actual
  stage startup/graph is exercised by this fixture.
- stage-state-oct11-stages-final: all 29 selectable versus stages pass old/current
  rendered CPU gameplay traces and all ten shared-stage Save/Validate boundaries
  in 1,200-sample matches. This does not exercise every stage interaction.
- stage-state-oct11-eight-final: stock and independent interpreted eight-actor
  scenarios pass all 27 explicit components at every sampled boundary; previous/
  current and diagnostic-on/off populated traces match exactly. No live Load.
- stage-state-oct11-independent-verification.json: all source/frozen evidence
  pins, compiled simulation versus host storage sections, and 194 donor Ground/
  grLib/stage-state bodies independently checked. Four build targets and the
  focused GObj-consumer CTest pass.

Shared-stage snapshot size is 17,160 bytes. Stage Save averages 6.02 us in each
stock/interpreted eight-actor scenario; Validate averages 1.27/1.23 us. These
are component observations, not complete state size or whole rollback timing.
Production still uses the broad page oracle. Stage-private globals and mutable
attribute payloads, other world/GM/HUD/effect/dynamics owners, multi-heap and
resident closure, aggregate prevalidation/load/history, full-match differential
restore and the eight-scripted-actor fifteen-tick deadline remain unfinished.
See melee/docs/native_stage_snapshot_ownership.md for the exact contract.


### 2026-10-11 stage-private hazard control batch

Audited 286 top-level declarations across all 29 selectable-stage source modules
using compiler types and function-reference contexts. This inventory is not a
mutability proof: aliases, output arguments and escaped pointers still require
semantic review. In particular, Corneria's cannon-position cache is written by
lb_8000B1CC's output argument and now has explicit snapshot ownership.

Moved actual Corneria, Venom, Green Greens and Brinstar Depths private controls
into typed modular storage, with direct callsite field names and all original
initial values. Added a 296-byte active-stage snapshot with paired Arwing GObj/
Ground generation claims, resident parameter identity, canonical unused data,
slot/bool validation, scene/activity/overlap refusals and pre-write root checks.
The shared debug conversation selection is preserved. Dormant other-stage
records are not touched. No snapshot allocation, game callbacks or rendering.

Evidence under A:/FreakMelee-test-archives:
- stage-hazards-oct11-owner-a: 178 hazard checks and 25 malformed candidates,
  sixteen repeated controlled updates for four active shapes plus an unrelated
  stage, companion restoration, reused generations, parameter/role/refusal and
  inactive-record preservation. Actual Ground owner; controlled graph setup.
- stage-hazards-oct11-stages: five old/current rendered CPU scenarios, 2,400
  samples each, all 20 shared/private boundaries captured and validated;
  populated gameplay traces match exactly for Corneria, Venom, Green Greens,
  Brinstar Depths and Final Destination. Hazard mean Save 0.790-0.965 us per case.
- stage-hazards-oct11-eight: all 28 components pass at ten boundaries in stock
  and independent interpreted eight-actor scenarios; diagnostic-on/off and
  previous/current traces match exactly. Live runs do not Load.
- stage-hazards-oct11-gm-a: the existing connected control/player/camera-target
  restore cohort passes; no actual stage startup/graph in that fixture.
- stage-hazards-oct11-independent-verification.json: source and frozen evidence
  pins, actual compiled sections, declaration inventory and all 225 donor stage
  function bodies verified. Four build targets and the focused GObj-consumer
  CTest pass.

Remaining stage-private records, borrowed mutable attributes and variable data
(e.g. Green Greens' allocated block arrays), other globals, multi-heap/resident
closure, aggregate prevalidation/restore/history and full match performance are
still open. Production remains on the broad page oracle. See
melee/docs/native_stage_hazard_snapshot_ownership.md for scope and limitations.


### 2026-10-11 Brinstar private bubble snapshot expansion

Moved the actual Brinstar cooldown, debris counter, four base positions and
complete twenty-entry bubble array into the modular hazard record. Extended
the existing stage-hazard component to 2,800 bytes with twenty GObj/Ground
claims and optional ordinary class joint claims. Class identities are admitted
from initialized stage/live bubble joints; checked backing caches refresh on
attachment changes. Save/Load allocate nothing and run no game callbacks/draw.
All 56 original Brinstar function bodies remain unchanged except storage names.

Actual bubble merging transfers a joint/actor to another entry, clears the donor
actor/active fields, and retains its joint pointer. This dormant alias may later
dangle. Snapshots preserve those bits without following them or requiring a
live class claim; creation resets the pointer before reuse. Actor-backed joint
references are validated separately. GObj/Ground/class companions restore before
private writes, and aggregate candidate graph closure remains required.

Evidence under A:/FreakMelee-test-archives:
- brinstar-state-oct11-owner-b: 76 additional Brinstar checks, ten malformed
  candidates, repeated controlled updates, changed attachments, unmapped and
  freed dormant aliases, actor/class companion ordering and refusal cases.
  Existing four-stage hazard checks also pass; controlled model setup/idle.
- brinstar-state-oct11-stages-final: six old/current rendered CPU scenarios,
  4,800 samples each, all forty shared/private boundaries pass with exact
  gameplay traces: Brinstar, Corneria, Venom, Green Greens, Brinstar Depths,
  Final Destination. Observed Brinstar hazard Save mean 4.745 us.
- brinstar-state-oct11-eight: all 28 components pass in stock and independent
  interpreted eight-actor scenarios. Diagnostic-on/off and previous/current
  populated traces match exactly; live censuses do not Load.
- brinstar-state-oct11-gm-a and independent-verification.json: connected control
  fixture, owner/frozen/live pins, compiled sections and donor bodies verified.
  Client/content/source-assets/GObj-consumer builds and focused CTest pass.

Other private stage records, mutable parameter/variable payloads and remaining
world/GM/HUD/effect/dynamics state are unfinished. Multi-heap/resident closure,
aggregate prevalidation/restore/history, complete-match differential rollback
and the eight-actor fifteen-tick performance deadline remain open. Production
continues to use the broad page oracle. See
melee/docs/native_brinstar_snapshot_ownership.md for evidence and limitations.


### 2026-10-11 Kongo Jungle / Icicle Mountain private ownership

Extended the existing hazard component with actual Kongo parameter/bank-joint/
fifteen motion-link state, and Icicle Mountain parameter/three extra-row/six-row
state. Both initializer tables are preserved exactly. Kongo's private parameter
storage was in an included static header, outside the earlier source-only
compiler inventory; that definition is removed and actual record storage owns
it. Included private headers must be covered by the remaining global audit.

Kongo's seventeen model borrowers have class backing/type claims even without
an actor handle in this record. Populated Icicle Mountain handles have paired
GObj/Ground generation/role claims. The versus initializer leaves those three
extra handles null; populated fixture coverage does not establish live adventure
mode acceptance. No allocation/game callbacks/draw during Save/Load. Both shapes
fit the existing union and reference capacity, so the snapshot remains 2,800
bytes. All 121 original stage function bodies are preserved except direct storage
and local joint-motion type names.

Evidence under A:/FreakMelee-test-archives:
- moving-stage-oct11-owner-b: 113 added checks/thirteen malformed candidates,
  exact initializer assertions, repeated controlled updates, all seventeen
  Kongo borrowers, populated extra-row references, changed attachments, late
  refusals, class restore, generation reuse and scene/activity retirement.
  Actual class/Ground owners; controlled graph construction/idle/parameters.
- moving-stage-oct11-stages: five 2,400-sample old/current rendered CPU scenarios
  with exact populated gameplay traces and all twenty shared/private boundaries
  passing: Kongo Jungle, Icicle Mountain, Brinstar, Corneria, Final Destination.
- moving-stage-oct11-eight: all 28 components pass stock and independent
  interpreted eight-actor capture/validation; previous/current and diagnostic
  on/off traces agree. Live runs do not Load.
- moving-stage-oct11-gm-a and independent-verification.json: connected control
  restore fixture, source/frozen/live pins, compiled sections, donor bodies and
  initializer/header migration verified. Four build targets and focused CTest
  pass.

Other stage-private/header records, variable and borrowed mutable payloads,
remaining world/GM/HUD/effect/dynamics state, multi-heap/resident closure and
aggregate prevalidation/restore/history are unfinished. Full-match differential
rollback and the eight-actor fifteen-tick deadline remain unproven. Production
continues on the broad page oracle. See
melee/docs/native_moving_stage_snapshot_ownership.md for evidence and limits.


### 2026-10-11 Green Greens mutable grid-array ownership

Replaced the grid controller's two HSD allocations/clears and frees with actual
optional Ground-slot storage for thirty positions and the 5x6 block array.
Binding retains original zeroing and existing pointer callsites; all block
motion, hit, spawn, collision and destruction decisions remain unchanged.
Reservation supports every Ground slot, so it introduces no earlier satellite
capacity limit. It is separate from the Ground union and only occupied grids
are copied, avoiding enlarged Ground records or unused reservation payloads.

The new 29th component uses a sixteen-byte lifetime/count header and ordered
occupied entries, paired GObj/Ground owner claims, live role/address checks,
strict candidate shape checks, and whole-candidate local prevalidation before
writes. It scans sixty-four occupancy words and allocates nothing during
Save/Load. Controller cleanup and Ground userdata retirement drop ownership;
scene retirement clears occupancy. GObj/Ground companions restore first.

Block records' actor/item/joint/hit-source pointers remain borrowed. Aggregate
candidate graph/lifetime/role agreement, class membership and collision links
must still be proven before complete restore. This is array-payload ownership,
not a claim that all cell borrowers or whole stages are closed.

Evidence under A:/FreakMelee-test-archives:
- green-grid-oct11-owner-b: 153 checks/nine malformed candidates, empty and
  two-owner shapes, original clearing, repeated controlled updates, sparse
  data, late refusal without partial writes, owner resurrection and retirement.
- green-grid-oct11-ground-a: all twelve existing actual Ground consumer cases,
  12,411 assertions, pass with the satellite lifecycle integrated.
- green-grid-oct11-stages: four 4,800-sample old/current rendered CPU scenarios
  with exact populated traces and forty successful grid boundaries per scene:
  Green Greens, Brinstar, Kongo Jungle and Final Destination. One live grid is
  2,112 bytes with observed mean Save 1.173 us; empty scenes sixteen bytes.
- green-grid-oct11-eight: all 29 components pass stock/independent interpreted
  eight-actor capture/validation; previous/current and diagnostic-on/off traces
  match. Live censuses do not Load.
- green-grid-oct11-gm-a and independent-verification.json: connected control
  cohort, source/frozen/live pins, compiled sections and all fifty donor stage
  bodies verified, allowing typed binding/retirement. Four build targets and
  focused GObj-consumer CTest pass.

Remaining satellites, private/header records, mutable borrowed attributes,
other globals, multi-heap/resident closure, aggregate prevalidation/restore/
history and complete-match differential/performance acceptance remain open.
The broad page oracle is still active. See
melee/docs/native_green_grid_snapshot_ownership.md for exact scope and evidence.


### 2026-10-11 Green Greens active-cell snapshot references

Added paired block GObj/Ground and material GObj/Item generation claims to the
existing grid snapshot, plus retained ordinary-class spans and known joint type
identities. Capture checks roles, reciprocal actor links, normal/bomb map IDs,
and the material's grid-owner/joint relationship. Load proves all restored live
borrowers before changing any grid payload. The generic class identity lookup
never follows an unknown caller-provided type pointer. GObj, Ground, Item and
class companions must load first; no production block decisions changed.

Inactive cells retain their original stale pointer bits with zero live claims;
cleanup destroys actors without clearing those fields. Column compaction moves
records, so claims follow their contents rather than old positions. Capture uses
fixed stack storage and a bounded direct-mapped span cache, without tick-time
allocation. One grid is now 6,192 bytes; two controlled grids 12,368; empty scenes
remain sixteen bytes. These supersede the prior array-only snapshot sizes.

Evidence under A:/FreakMelee-test-archives:
- green-borrowers-oct11-owner-b: actual Ground/Item/class owners and companion
  resurrection, twenty malformed candidates, dormant invalid aliases, generation
  reuse, wrong material owner, class type corruption, moved cells and refusal
  before writes. Controlled GObj backing, idle predicate and material destructor.
  The 16,633 grid assertions include checking every byte of an unchanged 16 KB
  failed-capture buffer, not thousands of distinct scenarios.
- green-borrowers-oct11-ground-b, item-a, lifetime-b and gm-b: existing actual
  owner/lifecycle/control fixtures pass with the new dependencies. Fixtures with
  no model graph abort unexpected class-owner calls explicitly.
- green-borrowers-oct11-stages-a: Green Greens old/current rendered CPU runs,
  4,800 samples each, identical populated traces; all forty grid Save/Validate
  boundaries pass. Observed mean Save 5.915 us and Validate 2.265 us.
- green-borrowers-oct11-eight-a: stock and independent interpreted eight-actor
  runs pass all 29 components at ten boundaries each, with identical diagnostic
  on/off and previous/current traces. Live cohorts do not Load.
- green-borrowers-oct11-independent-verification.json: source/frozen/live pins,
  unchanged block/material/Item/pool code and actual/host compiled sections
  verified. Four build targets and focused GObj-consumer CTest pass.

Candidate-wide agreement before companion writes, class object occupancy and
complete graph/collision closure still belong to aggregate validation. Remaining
private/header state, variable satellites, mutable attributes, other globals,
multi-heap/resident closure, history and whole-match restore/resimulation plus
the eight-actor fifteen-tick deadline remain unproven. The broad page oracle
remains active. See melee/docs/native_green_grid_snapshot_ownership.md.


### 2026-10-11 shared secondary-bone dynamics and wind snapshot owner

The coverage audit identified shared state beneath multiple stages: 320 bone-
dynamics nodes and eight wind fields, previously two direct HSD allocations.
These now occupy actual fixed storage with their free/active heads, wind status
and update-disable control. Source consumers address that storage directly.
Removed the redundant allocation entry and obsolete allocation-container types;
all fifteen scene callers now use lbDynamics_Reset. Original reset fields,
allocation order, capacity, wind priority replacement, decay/expiry and bone
simulation arithmetic remain unchanged. Reset establishes a fresh host owner
epoch and the production match exit retires it.

The 30th explicit component is 54,312 bytes, including free-record history.
Numeric pointer-to-index validation checks the dynamics free chain and disjoint
acyclic allocated chains, plus the complete eight-entry wind active/free
partition. Foreign/interior pointers, cycles, duplicate membership and links
into free records reject before writes. Capture/restore uses fixed scratch and
sequential copies without allocation, callbacks, physics, rendering or audio.
Raw dormant joint bits are copied but never followed as live roots.

Evidence under A:/FreakMelee-test-archives:
- dynamics-oct11-owner-d: 463 checks/eleven malformed candidates, real selected
  reset/allocation/release/wind bodies, original capacities, exact next addresses,
  release/reuse, dormant bytes, exhaustion/replacement/expiry, 24 repeated wind
  ticks and unchanged output on failed capture. Matrix setup/idle are controlled;
  external descriptor records are restored explicitly as companions.
- dynamics-oct11-stages-b: Peach's Castle, Big Blue and Final Destination old/
  current rendered CPU runs, 4,800 samples each, identical populated traces and
  forty successful dynamics capture/validation boundaries per scene. Mean Save
  6.433-6.575 us, Validate 3.058-3.123 us; no live Load or total timing claim.
- dynamics-oct11-eight-a: all thirty owners pass stock/independent interpreted
  eight-actor capture/validation at ten boundaries each. Diagnostic-on/off and
  previous/current populated traces match exactly. No live Load.
- dynamics-oct11-gm-b, body-verification.json and independent-verification.json:
  connected control exit retirement, all 23 retained dynamics function bodies,
  fifteen direct constructor-call migrations, source/frozen/live input hashes
  and actual/host compiled sections verified. Four build targets and focused
  GObj-consumer CTest pass.

Fighter/item/stage DynamicsDesc records, model joints and caller-held wind
pointers remain companion dependencies. Candidate-wide graph/partition/borrower
agreement before any companion writes is unfinished. The stage audit also
identified Mute City's mutable car array in its private included header; private
headers, aliases and variable satellites remain required coverage work.
Other globals, multi-heap/resident closure, aggregate history/restore and full-
match differential/performance acceptance remain open. Production still uses
the broad page oracle. See melee/docs/native_dynamics_snapshot_ownership.md.


### 2026-10-11 Mute City car and Castle wind-control snapshot ownership

The existing stage-private component now includes actual Mute City car records
(thirty), their order permutation, collision control and parameter pointer,
alongside Castle's wind pointer, angle step and strong/weak mode. Original
initial values persist. Direct source references replace private globals; the
mutable car array and obsolete type header are removed. Material borrowers use
GObj/Item generations and actual item role/entity checks. Castle uses a generic
physical wind-slot/reset-epoch reference; the dynamics owner captures its payload
separately. Stock priority replacement retains aliases, so this is deliberately
not a per-wind-request generation or occupancy assertion.

The car sorter formerly accessed order[-1] when its inner loop reached zero.
Changed that loop boundary to j > 0 and documented the invalid native access.
The real sorter fixture forces every insertion to index zero and verifies all
thirty identities in descending order. All other source decisions/arithmetic
remain unchanged. The expanded component is 4,488 bytes with thirty root slots;
Brinstar still has exactly twenty bubble records. There remain thirty census
components overall, not a new component for each stage. Candidate permutation,
wind geometry/epoch and restored borrower checks complete before hazard writes.

Evidence under A:/FreakMelee-test-archives:
- car-wind-oct11-owner-c: 128 added checks/thirteen malformed candidates, real
  full-sort permutation, repeated updates, material slot generation reuse and
  Item/GObj companion resurrection, late refusals before partial writes,
  descriptor mismatch and exact Castle controls/wind-payload separation.
  Actual owners; stage/GObj setup, idle, parameters and destructor/physics
  endpoints controlled. car-wind-oct11-dynamics-a adds generic reference checks.
- car-wind-oct11-stages-a: Mute City, Castle, Brinstar, Corneria and Final
  Destination old/current rendered CPU runs, 4,800 samples each. All populated
  traces match, including after the sort bounds fix, and all forty private
  capture/validation boundaries pass per scene. Mean Save 1.915 us for Mute
  City and 2.208 us for Castle. No live Load or full rollback timing claim.
- car-wind-oct11-eight-a: stock/independent interpreted eight-actor scenarios
  pass all thirty components at ten boundaries each; diagnostic-on/off and
  previous/current traces match. No live Load.
- car-wind-oct11-gm-a, body-verification.json and independent-verification.json:
  connected control restoration, all 71 Castle and 63 Mute City source bodies
  with explicit allowed changes, final input hashes and compiled state/host
  sections verified. Four build targets and focused GObj-consumer CTest pass.

Complete material/joint/collision graph agreement before any companion writes,
other private/header state and variable satellites, mutable attributes, other
globals, multi-heap/resident closure and full aggregate history/restore/deadline
acceptance remain open. Production retains the broad page oracle. See
melee/docs/native_car_wind_snapshot_ownership.md for exact scope and limits.


### 2026-10-11 Poke Floats collision-map and Battlefield mode ownership

Moved the actual Poke Floats forty-four-entry collision-joint table with exact
initial values, sentinel and unused tail into shared stage-private storage.
Source callsites use it directly. The original updater still visits forty-two
entries. Scene setup freezes vertex/map/joint IDs and reserved fields; used
joint borrowers carry ordinary class backing/type claims. Unused tail pointer
bits are captured without being followed. Battlefield's parameter pointer and
demo-mode control also have actual storage and restore support; parameter
pointee/overlay payload is not implicitly owned by this pointer.

The existing stage-private snapshot grows to 5,720 bytes and forty-four root
slots, retaining original twenty-bubble/thirty-car limits for other variants.
There are still thirty census components. Generic safe class identity lookup
now precedes admission's type-size checks. Candidate identities and restored
joint types validate before any stage writes. No source gameplay decisions or
arithmetic changed; no capture/restore allocations or callbacks are introduced.

Evidence under A:/FreakMelee-test-archives:
- pura-oct11-owner-a: 72 added checks/eleven malformed candidates, original
  table values, forty-one populated mappings through three actual class joints,
  repeated binding changes, unused invalid tail alias, bad config/spans/types,
  late type refusal before writes, real class companion restoration, overlap,
  activity/reinitialization retirement and Battlefield mode restore. Stage/
  GObj/graph initialization, parameters and idle predicate are controlled.
- pura-oct11-stages-a: Poke Floats, Battlefield, Brinstar and Kongo Jungle
  old/current rendered CPU scenarios, 4,800 samples each. Populated gameplay
  traces match; all forty private capture/validation boundaries pass per scene.
  Poke Floats mean Save 5.383 us, Validate 0.915 us. No live Load/deadline claim.
- pura-oct11-eight-a: stock/independent interpreted eight-actor runs pass all
  thirty components at ten boundaries each; previous/current and diagnostic
  on/off populated traces match. No live Load.
- pura-oct11-gm-a, body-verification.json and independent-verification.json:
  connected control restoration, all 31 Poke Floats and 25 Battlefield bodies
  with only direct storage naming changes, exact initializer migration, final
  input hashes and compiled state/host sections verified. Four build targets
  and the focused GObj-consumer CTest pass.

Class backing/type claims do not establish complete object occupancy or model/
collision candidate agreement. Other private/header state, variable satellites,
mutable attributes, globals, multi-heap/resident closure and complete aggregate
history/restore/resimulation/performance remain open. Production still uses the
broad page oracle. See melee/docs/native_pura_snapshot_ownership.md.


### 2026-10-11 expanded stage audit and Big Blue offset/attribute ownership

Added tools/audit_stage_snapshot_state.py to inventory 29 selectable-stage
modules and three shared helpers, including private header storage and static
locals. Conservative pointer-alias unions and address/call/return escapes
identify candidates; this is not interprocedural/field-sensitive proof. Final
audit-c reports 241 eligible declarations/115 candidate roots across 32 units,
without reported eligible location gaps, with pinned source/header/compiler/
tool hashes. Verification confirms the previously missed Big Blue entry-pointer
writes and shared generator private-header state.

Big Blue's twelve actual track/offset records and cached parameter pointer now
live in explicit stage storage. Exact indices/padding/zero initial values and
all source arithmetic remain unchanged. Scene identities freeze before history.
The stage snapshot also captures/restores the actual loaded x18 mode flag,
which runtime code changes; no live mirror or unrelated parameter bytes are
introduced. The component is now 5,728 bytes, still thirty census components.
Buffer/identity/reserved checks complete before writes, without allocation or
callbacks. The retained parameter backing stays leased; other pointee payload
ownership remains separate work.

Evidence under A:/FreakMelee-test-archives:
- big-blue-oct11-owner-a: 42 added checks/six malformed candidates, all twelve
  records, exact initializer endpoints, sixteen repeated updates, actual flag
  restore preserving unrelated parameter bytes, identities, overlap/activity/
  epoch retirement. Stage/GObj/parameter/idle setup controlled.
- big-blue-oct11-stages-a: Big Blue, Poke Floats and Final Destination old/current
  rendered CPU runs, 4,800 samples each, exact populated traces and forty passing
  private capture/validation boundaries per scene. Big Blue mean Save 2.845 us,
  Validate 0.522 us. No live Load or total deadline claim.
- big-blue-oct11-eight-a: all thirty components pass stock/independent interpreted
  eight-actor capture/validation at ten boundaries each, with previous/current
  and diagnostic-on/off trace equality. No live Load.
- big-blue-oct11-gm-a, body-verification.json, independent-verification.json and
  stage-storage-oct11-audit-verification.json: connected control restoration,
  all 65 donor bodies/track initializer, final pins and compiled state/host
  sections verified. Four build targets and focused GObj-consumer CTest pass.

Shared generator header/data/configuration backing, remaining attribute and
variable satellites, other world/HSD/HUD/effect globals, multi-heap/resident
closure and complete aggregate restore/history/resimulation/performance remain
unfinished. The audit is a candidate inventory, not a closure sign-off.
Production retains the broad page oracle. See
melee/docs/native_big_blue_snapshot_ownership.md for exact scope and limits.


## 2026-10-11 shared stage generator ownership checkpoint

The shared 81-entry generator table now lives in actual fixed simulation storage
with its control values and driver, without a heap mirror. Original reset loops,
scan/update/retirement decisions and callback ordering remain; the allocation-only
helper/private mutable header are removed. Failed driver construction preserves
the prior table/binding. A 5,896-byte component captures driver and occupied
GObj/Item generations, checks the whole local candidate before writes, and loads
only after restored companions agree. Scene retirement invalidates history.

Evidence under A:/FreakMelee-test-archives:
- generator-oct11-owner-b: 165 added generator checks, nine malformed-candidate
  cases, actual source constructor/selected retirement bodies, repeated updates,
  stale late-entry refusals without partial writes, companion reuse/restoration,
  overlap/activity/epoch handling. Scanner and scheduling endpoints controlled.
- generator-oct11-gm-a: connected production control/player/camera fixture passes.
- generator-oct11-stages-a: Final Destination, Brinstar, Castle and Poke Floats,
  4,800 controller samples each, exact old/current populated traces and forty
  passing generator capture/validation boundaries per scene. Sampled component
  mean Save 1.198-1.295 us; no complete-match timing or live Load claim.
- generator-oct11-eight-a: stock/independent interpreted eight-actor scenarios
  pass all 31 components at ten boundaries, with diagnostic-on/off and previous/
  current trace equality. No live aggregate Load.
- generator-oct11-independent-verification.json: frozen/live source pins, eleven
  unchanged source bodies, preserved reset loops and compiled simulation/host
  sections verified. Four build targets and focused GObj-consumer CTest pass.

Remaining generator configuration/spawn-buffer owners are used by selectable
Corneria and Brinstar, not just non-versus modes; they are not covered by this
singleton table component. Other mutable attribute/world/HSD/HUD/effect storage,
multi-heap/resident closure, candidate-wide validation and complete aggregate
restore/history/resimulation/performance acceptance remain unfinished. Production
retains the broad page oracle. See
melee/docs/native_stage_generator_snapshot_ownership.md for scope and limits.

Timing clarification for the preceding checkpoint: the four stage component
means span 1.175-1.295 us (Castle was 1.175 us). These are sampled generator
Save means, not whole-match save/load latency or a worst-case bound.


## 2026-10-11 stage spawn configuration ownership checkpoint

The 32nd component now owns the actual configuration prefix, transformed spawn
segments and constructor cursor used by selectable Corneria and Brinstar. Their
source constructor no longer allocates configuration/buffer pairs in HSD heaps;
it uses fixed storage for 128 configurations with 32 segments each. Exhaustion
returns the existing NULL failure result with a diagnostic. Current source calls
fit (two configurations per stage, maximum ten segments). Original interpolation,
random decisions, callbacks and other generator/Ground behavior remain, apart
from the explicit stage-transition lease retirement/cursor reset. Pre-update
segments start defined zero rather than fresh heap bytes; source updates write
every requested segment before consuming it.

Snapshots serialize only the occupied prefix and requested segment counts:
sixteen bytes empty, 200 for Corneria and 464 for Brinstar. Retained source
definition identities/contents, callback/count/probability bindings and fixed
segment addresses validate before any load writes. Cursor rewind supports later
constructor reuse. No save/load allocation, source callback, transform, draw or
physics is invoked. Source positive-probability configurations keep gen=NULL;
the negative-probability persistent-particle branch remains gameplay source code
but requires its own particle owner before explicit snapshots can admit it.

Evidence under A:/FreakMelee-test-archives:
- spawn-oct11-owner-final: 2,105 added checks and eleven malformed candidates;
  actual source constructor/interpolation/update and fixed owner save/load,
  counts 1/2/4/10, sixteen repeated updates, callback counts, allocation cursor
  rewind/capacity, changed definitions, unaligned input, overlap/short outputs,
  activity and epoch retirement. Transforms/RNG/callback endpoints controlled.
- spawn-oct11-gm-a: connected production control/player/camera fixture passes.
- spawn-oct11-stages-a: Corneria, Brinstar and Final Destination old/current
  rendered CPU matches, 4,800 samples each, exact populated traces and forty
  passing spawn capture/validation boundaries per scene. Corneria sampled mean
  Save 0.205 us, Brinstar 0.265 us; not total match save/load or a worst-case bound.
- spawn-oct11-eight-a: all 32 components pass stock/independent interpreted
  eight-actor capture/validation at ten boundaries, with previous/current and
  diagnostic-on/off trace equality. No live aggregate Load.
- spawn-oct11-independent-verification.json: final frozen/live hashes, source
  body/transition review and actual simulation/host compiled sections verified.
  Four native build targets and focused GObj-consumer CTest pass.

Other mutable stage/attribute/world/HSD/HUD/effect state, persistent particle
generators, multi-heap/resident closure and complete aggregate candidate
validation, restore/history/resimulation/performance acceptance remain open.
Production retains the broad page oracle. See
melee/docs/native_stage_spawn_snapshot_ownership.md for exact scope and limits.


## 2026-10-11 Rainbow Cruise platform-array ownership checkpoint

The stage's seventeen moving-platform records and twenty disappearing-platform
records now reside in actual stage_hazard_state.c storage, with the owning GObj
and parameter binding. Source Ground map 1 binds both arrays directly; two HSD
allocations are removed without a heap mirror or replacement state machine.
Static assertions preserve source descriptor counts. The parameter structure is
unchanged in grrcruise_data.h. All initialization/update/collision decisions remain
apart from the direct storage/parameter naming substitutions.

The existing stage-hazard union captures both arrays, retaining all clocks,
phases, velocities/rest values and pointers. Its total component size remains
5,728 bytes, still 32 live census components. The owner has GObj/Ground generation
and role claims; all 37 joints have retained class spans and known type identities.
Collision IDs are frozen by the scene lease; state tags and required joints
validate. Wind is claimed from the actual Ground pointer through the existing
dynamics physical-slot reference, with no duplicated wind payload. Restored
companions must agree before either array is overwritten. Save/load does not
allocate, update physics, execute callbacks or draw.

Evidence under A:/FreakMelee-test-archives:
- rainbow-oct11-owner-d: 570 added checks and twelve malformed candidates;
  actual source moving-record initializer, platform updater and contact callback,
  owner storage/serializer, repeated sixteen-tick state updates, callback/RNG
  counts, root/ground resurrection, late joint/type/collision/tag/wind refusals,
  overlap/activity/epoch retirement. Joint dirty/visibility, collision backend,
  RNG endpoints and disappearing-record phase writes are controlled.
- rainbow-oct11-gm-a: connected production control/player/camera fixture passes.
- rainbow-oct11-stages-a: Rainbow Cruise, Castle and Fountain of Dreams,
  4,800 samples each, exact old/current populated traces and forty passing hazard
  captures/validations per scene. Rainbow sampled mean Save 6.147 us, Validate
  0.897 us. Component means only; no complete-match or worst-case timing claim.
- rainbow-oct11-eight-a: stock/independent interpreted eight-actor scenarios
  pass all 32 components at ten boundaries, with previous/current and diagnostic
  on/off trace equality. No live aggregate Load.
- rainbow-oct11-independent-verification.json: final frozen/live source pins,
  donor bodies with explicit storage substitutions, identical parameter layout
  and actual simulation/host compiled sections verified. Four native build
  targets and focused GObj-consumer CTest pass.

Other mutable stage/attribute/world/HSD/HUD/effect storage, Fountain reflection
userdata, multi-heap/resident closure, candidate-wide graph/collision validation
and complete aggregate restore/history/resimulation/performance remain open.
Production retains the broad page oracle. See
melee/docs/native_rainbow_snapshot_ownership.md for exact scope and limits.


## 2026-10-11 Fountain reflection ownership checkpoint

The source reflection userdata now resides directly in stage_hazard_state.c,
with its parameter/image bindings and camera GObj owner. The constructor binds
that actual record instead of allocating a heap mirror, and its userdata removal
callback clears the owner instead of freeing static storage. Source draw, camera
mirroring, EFB copy and texture-matrix calculations are unchanged. The existing
stage-hazard component captures this state and adds a CObj class span/type claim;
its camera payload belongs to the class companion. Component size is now 5,768
bytes (forty added bytes), still 32 live census components.

Image descriptor contents/address are frozen after source initialization. The
original native HSD_ImageDescCopyFromEFB calls GXCopyTex, whose Aurora path uses
the destination address as a GPU cache key and resolves pixels into GPU textures,
without writing destination RAM. Those host GPU handles/pixels are not duplicated
in the snapshot. The game does directly consume texture_matrix, which is captured.
Normal reflection drawing remains enabled; no drawing-disabled replay path is
introduced. Complete aggregate replay must verify graphics regeneration/order.

Evidence under A:/FreakMelee-test-archives:
- fountain-oct11-owner-b: 58 added checks/six malformed candidates, actual
  storage/serializer/retirement callback, repeated matrix updates/restore,
  camera span/type and root resurrection, frozen descriptor guards, wrong
  userdata callback refusal without partial writes, overlap/activity/retirement
  and untouched destination marker bytes. Camera/image/root setup and matrix
  updates are controlled; no renderer or full-match Load is exercised.
- fountain-oct11-gm-a: connected production control/player/camera fixture passes.
- fountain-oct11-stages-a: Fountain of Dreams, Rainbow Cruise and Final
  Destination old/current rendered CPU matches, 4,800 samples each, exact
  populated traces and forty passing stage-private captures/validations per
  scene. Fountain sampled component mean Save 3.205 us, Validate 0.562 us;
  component means only, not whole-match latency or worst-case bounds.
- fountain-oct11-eight-a: stock/independent interpreted eight-actor scenarios
  pass all 32 components at ten boundaries with previous/current and diagnostic
  on/off trace equality. No live aggregate Load.
- fountain-oct11-independent-verification.json: frozen/live source pins, 41
  source bodies with explicit constructor/storage substitutions and compiled
  simulation/host sections verified. Four native build targets and focused
  GObj-consumer CTest pass.

Other mutable world/attribute/HSD/HUD/effect storage, multi-heap/resident closure,
candidate-wide floor/Ground/camera/graph validation, complete aggregate restore,
graphics regeneration/history/resimulation and performance remain unfinished.
Production retains the broad page oracle. See
melee/docs/native_fountain_snapshot_ownership.md for exact scope and limits.


## 2026-10-11 effect-library control ownership checkpoint

The 33rd explicit component captures actual load mode, effect count, native
allocation serial, animation count/32-entry queue and eight alpha/ID rows in
1,880 bytes. Queue/table definitions moved into efdata.c; every source consumer
still accesses the original symbols directly. Effect-library function bodies,
spawning/update/queue-drain/setter behavior and generation arithmetic are
unchanged. Active queued joints have class span/type claims; the drained tail
retains old pointer bits as bytes without being followed. Parameter objects have
GObj generation claims. Actual scalar/table writes happen only after complete
local validation and restored GObj/class agreement, without callback/draw/audio
or allocation. Host geometry/epoch is leased to the production GM loop.

Effect records, deferred queue-node payloads and allocator descriptors already
belong to HSD ObjAlloc backing; this control component does not duplicate them.
Deferred queue-head bindings remain GObj state. Source asset registries and
particle/generator/AppSRT global controls still need explicit ownership.

Evidence under A:/FreakMelee-test-archives:
- effect-control-oct11-owner-a: 67 added checks/ten malformed candidates, actual
  storage/serializer and source alpha/ID setters, repeated sixteen-tick updates,
  generation serial restore, active/dormant queues, stale GObj/class refusals,
  companion resurrection and overlap/activity/retirement. Root/joint construction
  and counter updates are controlled; no source effect spawning/full-match Load.
- effect-control-oct11-gm-b: production control/player/camera fixture passes with
  effect lifetime hooks linked. Its ObjAlloc exclusion endpoint aborts if called;
  actual effect Save/Load and allocator exclusion run in the owner fixture.
- effect-control-oct11-eight-a: stock/independent interpreted eight-actor
  scenarios pass all 33 components at ten boundaries; previous/current and
  diagnostic-on/off populated traces match exactly. No live aggregate Load.
- effect-control-oct11-independent-verification.json: frozen/live hashes,
  unchanged effect-library bodies and actual simulation/host storage sections
  verified. Four native build targets and focused GObj-consumer CTest pass.

Remaining particle/asset/world/attribute/HSD/HUD state, multi-heap/resident closure,
candidate-wide graph validation and complete aggregate restore/history/resimulation/
performance acceptance remain open. Production retains the broad page oracle.
See melee/docs/native_effect_control_snapshot_ownership.md for scope and limits.


## 2026-10-11 retained ObjAlloc borrower-reference checkpoint

Source particle/generator records already occupy shared HSD ObjAlloc backing,
but their global list roots still need explicit control ownership. Added generic
ordinary physical block/slot references to the retained ObjAlloc directory so
those future controls can check numeric spans without following untrusted lists.
The API compares descriptor identity before dereferencing a caller-supplied
allocator pointer, validates alignment/size/range/epoch and rejects construction
reservations. It describes a physical slot, not occupancy or an invented gameplay
generation. Freed slots remain resolvable; consuming owners must validate their
candidate list/ID/callback agreement. Reserved slots keep their separate owner.

Evidence: objalloc-ref-oct11-owner-a adds 23 checks for foreign/interior/oversized
inputs, malformed references, construction exclusion, free-slot semantics,
directory-prefix rewind/regrowth and heap retirement. The independent verifier
checks all 1,732 source pins and unchanged existing allocator bodies. Four build
targets and ObjAlloc/GObj-consumer CTests pass. No gameplay callers change yet,
and no extra live-match or timing claim is made. Census stays at 33 components.

Remaining particle/generator/AppSRT counters, sixteen particle-list roots, pending/
active generator roots, bank definitions/callback/joint/camera bindings and asset
registries require actual control storage and serializers. Other world/HSD/HUD
state, multi-heap/resident closure and complete aggregate restore/history/
resimulation/performance remain open. Production retains the broad page oracle.


## 2026-10-11 particle/generator/AppSRT ownership checkpoint

Actual globals now reside in particle_state.c with direct source access and
private descriptive names. The old mutable header and unused matching-only
storage are removed. The 34th component captures scratch bits, six counters,
generator ID cursor, camera/eight joint attachments, sixteen particle heads,
pending/active generator and deferred SList roots, plus prepared bank/hook/
configuration bindings in 4,128 bytes. Ordinary physical slot and class span/type
claims connect to existing payload owners without copying them twice. Prepared
bindings are leased; changes require a new preparation/history boundary. Complete
resident-resource policy and candidate list/ID/callback/topology agreement remain
open. Save/Load walks no lists and executes no source callbacks, draw or allocation.

A native layout mismatch is fixed: efLib_Init used GameCube's 0xA4 request for
AppSRT, while x64 records are 184 bytes with a 179-byte used prefix and the old
aligned allocator slot was only 168 bytes. The call now uses sizeof(HSD_psAppSRT).
Source decisions/transform/interpreter/generator-ID wrap arithmetic otherwise
remain unchanged, with direct private-name substitutions only.

Evidence under A:/FreakMelee-test-archives:
- particle-control-oct11-owner-b: 117 added checks/thirteen malformed candidates;
  actual control serializer/storage and real ObjAlloc/class/list/AppSRT backing,
  source AppSRT constructor/ID wrap, repeated control updates, spans/types,
  prepared-binding guards, separate payload restore, overlaps/activity/retirement.
  Camera/joint/record setup and control mutations controlled; no full graph proof.
- particle-control-oct11-gm-b: connected production control/player/camera fixture
  passes with particle lifetime hooks. Particle allocator/list endpoints abort
  if Save/Load is attempted here; actual restoration runs in the owner fixture.
- particle-control-oct11-stages-a: Fountain, Brinstar and Corneria old/current
  rendered CPU matches at 4,800 samples each, exact traces and forty passing
  particle captures/validations. Sampled Save means include 29.825 us (Fountain)
  and 31.840 us (Corneria), requiring aggregate profiling/optimization; no full
  latency or worst-case bound is claimed.
- particle-control-oct11-eight-b: final stock/independent interpreted eight-actor
  scenarios pass all 34 components at ten boundaries with exact prior/current
  and diagnostic-on/off traces. Eight-a preceded the final Load binding guard.
  No live aggregate Load is performed.
- particle-control-oct11-independent-verification.json: final source/frozen/live
  pins, source bodies with direct renames/AppSRT size correction, simulation/host
  compiled sections verified. Four build targets and allocator/GObj-consumer
  CTests pass.

Physical slots do not prove active occupancy or gameplay generations. Remaining
asset/resident/world/attribute/HSD/HUD ownership, multi-heap/candidate graph closure,
complete aggregate restore/history/resimulation/graphics regeneration and timing
acceptance remain unfinished. Production retains the broad page oracle. See
melee/docs/native_particle_control_snapshot_ownership.md for scope and limits.


## 2026-10-11 retained-reference snapshot performance checkpoint

Class and ordinary ObjAlloc reference queries return spans by value. Every
consumer was updated directly; no output adapter, allocation cache, storage
mirror or format version was introduced. This removes repeated directory scans
for the tiny output records while retaining span/allocator/class/epoch checks
and explicit snapshot buffer-independence guards. Class epoch zero is valid at
startup: an initial live failure caught the incorrect zero-epoch marker, which
was fixed to use empty bytes and covered by three actual startup fixture checks.

Final evidence: reference-cost-oct11-owner-b and gm-b pass; final eight-b passes
all 34 owners at ten stock/interpreted eight-actor boundaries, with exact prior/
current and observation-on/off traces. reference-cost-oct11-stages passes
Fountain and Green Greens at 4,800 rendered samples, with forty passing camera,
stage, hazard, grid and particle captures/validations. Four build targets and
focused ObjAlloc/GObj-consumer CTests pass. Independent verification checks
final source/receipt/executable pins and unchanged allocator bodies except the
reference queries.

Sampled particle-control mean Save costs improve from 23.770 to 14.240 us
(stock) and 22.400 to 13.820 us (interpreted), ten boundaries per case. No full
snapshot timing, worst-case rollback bound or live aggregate Load is claimed.
Production retains the broad page oracle. Complete candidate-wide graph/asset/
world/HSD/HUD closure, aggregate restore/history/resimulation/graphics and timing
acceptance remain open. See melee/docs/native_snapshot_reference_cost.md.


## 2026-10-11 live snapshot self-restore checkpoint

The offline census optionally keeps distinct aligned captures for all 34 owners
at one complete post-draw boundary, requires every capture to validate, invokes
actual Loads in dependency order, then recaptures all owners with byte-for-byte
equality required. No tick/callback/render runs between capture and Load. Normal
HSD drawing is retained between boundaries. Observation remains default; no new
production/history loader or supplied-candidate file/network path was introduced.
Scratch is reserved once on the host, with no sampled-tick allocations.

self-restore-oct11-eight-b passes stock and interpreted eight-actor cases at ten
boundaries each, with 340 actual Loads/identical recaptures per case and unchanged
prior/current and diagnostic-off/on traces. self-restore-oct11-stages-a passes
Fountain, Green Greens and Corneria at 4,800 rendered samples, with all 34 owners
capturing, loading and recapturing at forty boundaries: 1,360 pairs per stage.
Total passing Load/recapture pairs: 4,760. Independent verification checks final
source/executable/log pins, load order and host-only census storage. Four build
targets and focused ObjAlloc/GObj-consumer CTests pass.

Peak known-component capture prefixes are 2,608,144 bytes (stock) and 2,779,992
bytes (interpreted) in the eight-actor cases. Diagnostic Load mean costs are
1,141.30 and 1,158.32 us, with maxima 4,437.70 and 3,982.80 us across ten samples.
Logging is outside Load timing; owner validation/readiness is included. These
are not complete state sizes, history rewind timings or worst-case bounds.
Script fighter/item/effect owners participate, but active item/effect VM actors
are absent in the eight-Ice-Climbers cohort and still need resurrection coverage.
World/asset/resident/HSD/HUD closure, candidate-wide graph prevalidation, history
rewind through creation/reuse, changed-input resimulation/graphics and full
rollback performance remain open. Production retains the broad page oracle.
See melee/docs/native_snapshot_self_restore.md for the exact diagnostic contract.


## 2026-10-11 pool candidate-view checkpoint

Generic pool snapshot validation now returns a read-only saved-state view rather
than a boolean. The old entry point was removed and every consumer adjusted,
without a compatibility adapter or byte-format change. Ordered occupied records
can be inspected through allocation-free binary search; saved pool generations
resolve to stable physical addresses even when live slots are free/reused.
Retired bindings and foreign/incorrect references refuse. Candidate payloads are
read-only and may be unaligned; callers use memcpy to inspect fields.

pool-view-oct11-owner-b passes 124 lifecycle cases, 17 malformed components and
153,030 checks, including fifteen new saved-versus-live/reuse/retirement checks,
with no callbacks. pool-view-oct11-companions passes actual allocator/class/HSD/
camera/stage/effect/particle owner restoration. pool-view-oct11-eight passes all
34 owners and 340 actual same-boundary Loads/identical recaptures in each stock/
interpreted eight-actor case, with unchanged current/reference and prior traces.
Independent verification checks final source pins and trace agreement. Five
native build targets and focused pool/ObjAlloc/GObj-consumer CTests pass.

These are generic inspection primitives for aggregate prevalidation, not a full
candidate graph or history loader. Cross-owner relationships, class/ObjAlloc
candidate membership, remaining resident/world/HSD/HUD state closure and genuine
rewind/resimulation/performance acceptance remain open. Production retains the
broad page oracle. See melee/docs/native_pool_snapshot_view.md.


## 2026-10-11 saved actor/GObj graph preflight checkpoint

Prepared candidate pool views now support bidirectional fighter/Ground/item and
GObj ownership checks before any diagnostic Load. Saved actors must point to
occupied saved roots with matching classifiers, userdata tags, physical actor
addresses and destructors; actor-classified/tagged saved GObjs must point back
to their saved occupied actors. Ground is distinguished by its destructor because
Fountain's reflection camera also uses userdata tag 3. No candidate pointer is
followed into live actor bytes, and no pool write, allocation or callback occurs.

actor-graph-oct11-owner-b passes 115 checks and ten malformed relationships with
controlled typed root construction and aborting destructor endpoints. It accepts
an immutable old candidate after live root/fighter reuse and rejects ownership/
root/tag/callback/classification/duplicate/orphan/missing/retired mismatches,
without changing live backing or metadata. actor-graph-oct11-eight passes ten
preflights and 340 actual same-boundary Loads/identical recaptures per stock/
interpreted eight-actor case, with unchanged prior/current and diagnostic traces.
actor-graph-oct11-stages passes Fountain, Green Greens and Corneria at 4,800
rendered samples, forty preflights and 1,360 Load/recapture pairs per stage.
Independent verification checks final pins and traces. Four native build targets
and focused pool/ObjAlloc/GObj-consumer CTests pass.

This is one aggregate structural relationship check, not complete candidate
closure or proof of a common historical frame. Class/model/animation/article/VM/
particle links, identity/callback registries and remaining resident/world/HSD/HUD
ownership still need closure. Full history publication, rewind/resimulation and
rollback performance remain open; production retains the broad page oracle.
See melee/docs/native_snapshot_actor_graph.md for exact contracts and evidence.


## 2026-10-12 ordinary-class candidate payload/type checkpoint

Caller-reserved class indexes provide saved payload reads and physical resolution
under the candidate block prefix, span and epoch, with retained address/size
agreement. Saved type identity/size/header checks never follow unknown type
pointers. Complete component and index-alias checks precede index writes; no tick
allocation or hidden allocator cache is introduced. Independent views coexist
with separate indexes. Pre-existing class functions and byte layouts are unchanged.

The offline preflight now checks main-camera private CObj types, stage joints/
cameras, stage-hazard/reflection joints, active Green Greens cells, effect queue
joints and particle camera/joints against their saved ordinary-class component
before any Load. One fixed 512-KB pointer index is host-only. This is physical
membership and saved header/type agreement, not class-piece occupancy or complete
HSD graph topology. Current retained prefix/immutable definition leases still apply.

class-view-oct11-owner-a adds 24 real class/allocator fixture checks (152 class
checks total), including epoch zero, overwritten live headers versus saved bytes,
later-block exclusion despite live Resolve, malformed/unknown type/span cases,
index alias/capacity refusal and independent views. class-view-oct12-eight passes
ten class/actor preflights and 340 Loads/identical recaptures per stock/interpreted
eight-actor case with prior/current and diagnostic-off/on trace equality.
class-view-oct12-stages passes Fountain, Green Greens and Corneria at 4,800
rendered samples, forty class/actor preflights and 1,360 Load/recapture pairs each.
Independent verification checks final pins, traces, unchanged pre-existing class
bodies and host index section. Four build targets and focused pool/ObjAlloc/GObj
consumer CTests pass. Total live class preflights: 140; pairs: 4,760.

Other borrowers/reserved graphs, allocator/free-list topology, particle list/ID/
callback links, article/VM/world/HUD/resident closure and full candidate publication,
history rewind/resimulation and timing acceptance remain open. Production retains
the broad page oracle. See melee/docs/native_class_snapshot_view.md. Class graph
preflight work is outside the logged diagnostic component-Load timing.


## 2026-10-12 bounded older-component restoration/coverage checkpoint

The offline census can retain all 34 own validated captures at sample 240,
advance eight normal input ticks, apply the older components and compare every
recapture. It then recovers the complete preserved current endpoint byte for
byte before continuing. No tick/render/callback runs in the partial older state.
A late Load refusal is diagnostic failure, not atomic arbitrary-candidate
rejection; endpoint recovery is the safeguard. Normal drawing remains enabled.

The broad oracle is used only for research/recovery here, with a fixed 512-MB
host reservation split into two endpoint halves. Observed region bytes are
193,228,836, separate from the approximately 2.6-2.8 MB used known-component
captures. These large copies and raw comparisons are diagnostic, not production
history or latency claims. No sampled-tick allocation occurs. Raw difference
reports are bounded in lines but count all mismatching bytes/ranges; inactive
payload/padding/catalog tails require classification, not blind exclusion.

rewind-oct12-endpoint-pinned passes nineteen safeguard tests with controlled
region discovery/tracking. rewind-oct12-eight-pinned passes 34 older Loads/identical
recaptures and complete endpoint recovery in each stock/interpreted eight-actor
case. Nineteen/twenty owner records advance, including fighter/RNG and scripted
fighter state. Two GObjs are created, one destroyed, one live slot reused across
the interval. rewind-oct12-stages-pinned passes Fountain, Green Greens and Corneria
at 4,800 rendered samples, with older restoration/recovery and creation/destruction
coverage. Normal self-restore remains passing. Independent verification checks
final pins and prior/current traces. Four build targets and focused pool/ObjAlloc/
GObj consumer CTests pass. Total older pairs: 170; normal pairs: 4,760.

Raw remaining differences are 2,389/2,449 bytes (stock/interpreted), 926/278/3,202
(Fountain/Green Greens/Corneria); these are cases, not completeness or upper-bound
proof. PDB/source candidates include VS controller, random item spawner, HUD,
particle display state, texture/TLUT IDs, plus pool inactive payload/free-order
tails requiring semantic classification. Full remaining ownership, allocator/list/
VM/article graph closure, safe candidate publication, changed-input/rendered
resimulation and performance acceptance remain open. Production retains the broad
page oracle. See melee/docs/native_snapshot_rewind_diagnostic.md.


## 2026-10-12 VS/item/HUD control checkpoint

The actual VS controller, ambient item-pick globals and damage HUD records now
have explicit storage and direct source consumers. Component 35 captures their
mutable control records and weighted table payload prefixes in 14,088 bytes.
Callback/opaque parameter, HUD descriptor and pick-backing/count identities stay
leased for history. Active HUD roots have GObj generation and class/type claims;
dormant joint pointer bits are preserved without being followed. No rebuild/RNG,
callback, allocation or draw runs on Load. Actual state is .mstate$m; the scene
lease is host .data. Original gmvs/itspawn/ifstatus bodies are unchanged apart from
private direct-name replacements. Begin/EndScene hooks join the GM lifetime.

versus-controls-oct12-owner-a adds fifty actual owner/pool/class/allocator fixture
assertions, including exact table/control restoration, malformed/changed-life/
overlap/dormant/retirement checks. versus-controls-oct12-gm-b passes connected
production lifetime hooks; this owner's Save/Load is exercised by the owner/live
fixtures, not fake class/allocator endpoints in the GM fixture. Eight-actor stock/
interpreted scenarios pass all 35 captures/validations, 350 normal and 35 older
Load/identical-recapture pairs each with endpoint recovery and unchanged traces.
Fountain/Green Greens/Corneria pass 4,800 rendered samples, 1,400 normal and 35
older pairs each. Independent verification checks final pins, source bodies,
data/host sections and prior/current traces. Four build targets and focused pool/
ObjAlloc/GObj-consumer CTests pass.

Raw target-oracle/PDB correlation no longer identifies the VS controller,
random item spawner or HudIndex in these eight-actor cases. Remaining nearest-
symbol ranges include pool inactive tails/payload, collision/animation scratch,
particle display state and texture identity counters; exact semantic classification
and other ownership remain open. This is not all HUD modules/descriptor payloads,
complete graph/asset closure, candidate publication or rendered/changed-input
resimulation/performance acceptance. Production retains the broad page oracle.
See melee/docs/native_versus_control_snapshot_ownership.md for scope and evidence.


2026-10-12 particle display snapshot checkpoint (local implementation)
- Moved actual display serial, sixteen sort serials and seventeen cached non-edge
  heads into psdisp_state.c with direct consumers; all 29 original display source
  function bodies are preserved after private storage-name changes.
- Existing particle component now captures these controls and ordinary allocator
  span claims in 4,824 bytes (+696), alongside original particle/generator/AppSRT
  controls. Sorting changes linked lists and AppSRT reuse depends on display
  serial; both require restoration with their owning allocation payloads.
- Documented draw-call matrix/color/TEV/size/blend cache rebuild paths. These are
  not copied merely to eliminate raw differences. Drawing/state/RNG remains part
  of normal simulation. Complete particle-list candidate topology is still open.
- Final owner fixture adds 33 assertions (150 particle checks, 17 malformed
  candidates), replaying the actual source sorter with real allocation restores,
  cached calls and controlled serial wrap. Connected GM fixture passes. Four
  build targets and focused allocator/pool/GObj-consumer CTests pass.
- Stock/interpreted eight-actor and Fountain/Green Greens/Corneria rendered cohorts
  pass all 35 owners: 4,900 normal and 175 older Load/recapture pairs total, with
  source/exe pins verified and unchanged gameplay traces. Older restoration does
  not tick the incomplete old world; broad endpoint recovery remains diagnostic.
- Capture peaks 2,622,928 / 2,794,776 bytes in the eight-actor cases are observed
  component storage, not a worst-case history/performance guarantee. Full state
  closure, candidate publication, rendered/changed-input resimulation, graphics
  regeneration and final timing remain unfinished. Production broad page oracle
  is unchanged; retain the full original goal.
- Evidence: A:/FreakMelee-test-archives/particle-display-oct12-owner-b,
  particle-display-oct12-gm-a, particle-display-oct12-eight-a,
  particle-display-oct12-stages and particle-display-oct12-independent-verification.json.


2026-10-12 exact restored-pool coverage checkpoint
- Bounded older-component diagnostic now classifies raw differences by actual
  restored pool geometry: GObj, GObjProc, fighter, Ground, item, ItemLink, AObj,
  FObj. Required object/metadata/free-prefix bytes remain distinct from inactive
  payload, stride padding and unused free-order tail. All raw differences remain
  reported, and unclassified bytes are never treated as safe by default.
- Caller-owned registration is validated for arithmetic, complete pool ownership
  and disjoint descriptor/metadata/order/payload spans before any output. Fixed
  stack scratch; no allocation/callback/tick/render/write. Scope is the restored
  idle boundary, not arbitrary candidate or HSD/OS graph membership proof.
- Fixture adds 27 assertions (46 total), including six injected differences that
  must split into three required and three inactive bytes, empty-registry unknown
  reporting, read-only operation and malformed/overflow/overlap refusal.
- Stock/interpreted eight-actor and Fountain/Green Greens/Corneria live cohorts
  pass all 35 owners and unchanged traces, with exact source/exe pins. Required
  pool differences are zero in every case. Raw totals 2420/2419/891/251/3126 split
  into inactive storage 2268/2265/713/61/2912 and unclassified 152/154/178/190/214.
  This substantially narrows the gap in these sampled intervals, not all scenes.
- Original endpoint Save/Restore/Matches bodies and production broad page oracle
  unchanged. Next trace residual texture identity/collision/animation/display
  state, then prove compact restore-and-resimulate with changed input and retained
  drawing. Full candidate closure, history lifecycles and final performance remain
  required; goal remains active with its original scope.
- Evidence: A:/FreakMelee-test-archives/pool-coverage-oct12-endpoint-b,
  pool-coverage-oct12-eight-a, pool-coverage-oct12-stages,
  pool-coverage-oct12-independent-verification.json and docs/native_snapshot_pool_coverage.md.


2026-10-12 Aurora texture identity snapshot checkpoint
- Added component 36 for actual process-lifetime texture/TLUT identity counters.
  Eight bytes, direct reads/writes via the existing Aurora storage-region query;
  no live mirror, callback/allocation or changed renderer/source allocation policy.
- Requires actual GM/script idle boundary and independent, correctly sized buffers.
  Whole preflight precedes output/state writes. Counter zero preserves the source
  pending-overflow contract rather than inventing a new identity-allocation rule.
- Source-extracted Aurora increment/query fixture passes 162 assertions, repeats
  128 identity pairs exactly after restore, checks refused buffers/activity without
  writes and preserves UINT32_MAX-to-zero exhaustion. Fixture idle predicates,
  initial counter storage and aborting diagnostic leaf are controlled; no GPU test.
- Stock/interpreted eight-actor and Fountain/Green Greens/Corneria cohorts pass all
  36 components: 5,040 normal and 180 older Load/recapture pairs total, final
  source/executable pins and unchanged traces. Native builds and focused CTests
  pass. Required registered-pool raw differences remain zero; residual unclassified
  bytes 149/151/175/188/212 still need ownership/transient classification.
- Next integration: extract one shared compact capture/load implementation from
  the census so an experimental rollback driver can use the same owners without
  duplicated packing or a compatibility layer. Then test retained-draw resimulation
  and changed-input correction against broad snapshots. Full graph prevalidation,
  resource lifecycles, graphics regeneration and final performance remain required.
  Production page oracle stays unchanged until replacement acceptance is proven.
- Evidence: A:/FreakMelee-test-archives/texture-identity-oct12-owner-a,
  texture-identity-oct12-eight-a, texture-identity-oct12-stages,
  texture-identity-oct12-independent-verification.json and docs/native_texture_identity_snapshot.md.


2026-10-12 shared compact capture/load checkpoint
- Extracted actual direct 36-owner capture into native_match_snapshot_capture.c
  and ordered load into native_match_snapshot_load.c, with shared private header.
  Old diagnostic loader source/header removed; all graph/build/test/census callers
  adjusted directly. No forwarding compatibility units or callback serialization
  table. Each code unit remains small; census shed its duplicated packing/loader.
- Shared API accepts caller-reserved independent host buffers, packs aligned slices
  and returns their part directory. Nullable measurements skip redundant size-query
  and timer instrumentation while retaining owner validation. Load returns optional
  structured timing/preflight data and performs no logging/allocation/callback/tick.
- Exact source audit confirms all 36 capture call expressions and 36 load calls/order
  unchanged. Structured class preflight replaces printing; normal original engine
  arithmetic/decisions unchanged. Full arbitrary-candidate prevalidation/publication
  still open: own immutable captures only, resident definitions, abort/recover on
  late Load refusal. Metadata host-buffer independence is a caller contract.
- Real first-boundary canaries refuse zero-capacity, overlap and overflowing spans
  without metadata writes. Stock/interpreted eight-actor and Fountain/Green Greens/
  Corneria cohorts pass 5,040 normal and 180 older Load/recapture pairs, exact
  source/executable pins and unchanged traces. Native builds and focused CTests pass.
  Null-measurement capture is exercised; live Loads currently supply reports.
- Capture peaks remain 2,622,936 / 2,794,784 bytes. Required known pool differences
  remain zero; residual raw bytes are not universally classified. These tests do
  not tick the partial older world. Next wire compact history/experimental rendered
  resimulation to this shared API, then changed-input and deadline acceptance.
  Production broad snapshots and safeguarded endpoint recovery are unchanged.
- Evidence: A:/FreakMelee-test-archives/shared-snapshot-oct12-eight-a,
  shared-snapshot-oct12-stages, shared-snapshot-oct12-source-equivalence.json,
  shared-snapshot-oct12-independent-verification.json and docs/native_match_snapshot_shared_core.md.


2026-10-12 first bounded compact restore-and-resimulate acceptance
- Exclusive offline replay restores shared compact owners at sample240, rewinds
  the existing input stream and executes eight actual input/game/HSD draw ticks.
  No page tracker or broad-state repair runs before/during this resimulation.
  Presentation/sidebar updates suppressed through the existing hidden-render path;
  HSD drawing and its RNG/state changes remain. Broad endpoint recovery is after
  every catch-up comparison, preserving continued test gameplay.
- Final stock/interpreted eight-actor and Fountain/Green Greens/Corneria cohorts
  reproduce ALL 37 compact records, every traced intermediate entry, and all six
  complete broad oracle regions with ZERO differing bytes. 40 resimulated ticks,
  185 catch-up comparisons, 5,180 normal and 185 older recapture comparisons.
  Explicit null-report compact reset also exercised. Builds / focused CTests pass.
- Source audio-intent journal reuses existing handles (4/4/1/0/4 starts across
  final cases, no new/canceled starts). Host transition-observer checkpoint stops
  the rewind itself being misreported as reverse transitions; no trace masks.
- Actual light list/active array/count/masks now owned by component37 (504 bytes),
  direct private storage, physical list/class claims and saved-class preflight.
  Restores heads with existing allocator/light payloads/refcounts, no add/delete.
  Actual source add/delete owner fixture adds33 checks, malformed/overlap/activity
  refusal; all lighting method bodies unchanged after private-name normalization.
- Diagnosed stage class mismatches as uninitialized Euler Quaternion W. Initialize
  that temporary; related full-rotation getter now has Quaternion storage rather
  than writing through a Vec3 buffer. Source body audit preserves all XYZ math.
  Earlier failed cases/dumps retained, not relabeled passing or masked.
- Production broad snapshots unchanged. This is a SAME-INPUT eight-tick proof in
  five known cases, not full history/deadline/candidate/lifecycle acceptance. Next
  changed-input and repeated/deeper compact-history tests, complete closure and
  measured15 rollback ticks with8 actors within16.667ms remain required. Compact
  observed storage still around2.6-2.8MB; broad oracle193,228,836bytes diagnostic.
- Evidence: A:/FreakMelee-test-archives/compact-resim-oct12-final-eight,
  compact-resim-oct12-final-stages, compact-resim-oct12-final-gm,
  compact-resim-oct12-light-owner-b, compact-resim-oct12-euler-body-verification.json,
  compact-resim-oct12-final-independent-verification.json and docs/native_compact_resimulation.md.


### October 12, 2026: compact corrected-input acceptance checkpoint

The offline eight-actor snapshot runner now exercises human P1 Ice Climbers
plus three CPU pairs, for stock and independent interpreted fighters. Corrected
inputs differ only across samples 240-247 (jump then right). Both compact-only
corrected passes match a fresh corrected run at every intermediate trace. The
second corrected endpoint reproduces all 37 compact owners and all six broad
raw regions exactly; original predicted endpoint recovery happens afterwards.
Predicted continuation remains exact. First correction changes 12 owners; P1
actually transitions from idle to jump startup and airborne jump. Repeated audio
reuses two starts with zero new/canceled starts in each case. Replay streams and
scratch are reserved before gameplay, not while correcting.

The initial experiment exposed the missing next-item serial it_804D6D10. This
counter advances even if a constructed item has been removed before capture.
It is now owned directly by the existing VS/item/HUD snapshot component. Source
constructor arithmetic is unchanged; freed native/script identity differences
resolve without raw-byte masks. The original failing receipts remain intact.

Evidence: A:/FreakMelee-test-archives/compact-correction-oct12-eight-b/receipt.json
(PASS, 1200 samples per stock/interpreted scenario), compact-correction-oct12-
owners-b/receipt.json (PASS, 86868 assertions), and compact-correction-oct12-
independent-verification.json (179 pins, exact input extent/reference prefix,
actual motion changes, first/second/fresh corrected traces and full endpoints).
Implementation and limitations: melee/docs/native_compact_input_correction.md.

The project remains incomplete. Next snapshot work: deeper and repeated history,
allocation/residency boundaries, complete candidate prevalidation, production
history integration, general output reconciliation and measured fifteen-tick
eight-actor performance. Production still uses the broad snapshot path.


### October 12, 2026: longer repeated compact history checkpoint

The offline live runner now accepts configurable depth up to 15 and repeated
intervals at later boundaries, using the existing transition-reserved buffers.
The accepted stock and interpreted eight-actor cohorts each rewind six separate
15-tick intervals at samples 240/360/480/600/720/840. All intermediate traces,
37 compact endpoint records per interval and complete broad endpoints match.
These intervals exercise GObj creation/destruction and physical slot reuse.
Both families also pass two 15-tick corrected-input passes against a fresh
corrected reference. Across the two cohorts: 240 real resimulation ticks, 518
compact endpoint record comparisons and 84 byte-identical broad raw regions.
Original endpoint recovery occurs only after resimulation/comparison; recovered
predicted continuation remains exact. No draw/RNG suppression or page tracker
participates in the compact experiment. This is correctness, not performance.

Longer intervals exposed missing numeric item population/cap controls and the
fighter reference-count array. The existing VS/item/HUD component now saves
both entire sets directly. Fighter references have a meaningful direct name,
ftData_CharacterReferenceCounts, at every call site. Original reset/increment/
assertion arithmetic is unchanged, verified against the preceding source.
The owner fixture mutates/restores those counters and rejects negative saved
references without publication. Earlier failing receipts remain preserved.

Evidence under A:/FreakMelee-test-archives/: compact-history-oct12-six-b and
compact-history-oct12-correction-b (PASS; 1200 samples per scenario),
compact-history-oct12-owners-b (PASS; 86874 assertions), and
compact-history-oct12-independent-verification.json. Documentation:
melee/docs/native_compact_history_experiment.md. Replay unit test passes.

These are repeated sequential older-boundary captures, not a production history
ring. Multiple corrected intervals currently reject because the diagnostic
recovers predicted state while the fresh reference retains corrected state.
Remaining: actual production history integration, complete candidate validation,
allocation/asset residency closure, full lifecycle/output reconciliation and
measured 15-tick eight-actor budget. Production continues using broad snapshots.
The full project goal is still incomplete.


### October 12, 2026: per-frame compact history storage checkpoint

Added native_match_history.[ch]: up to sixteen exact-frame slots in caller-
reserved host storage, scene epoch checks, consecutive capture publication,
wraparound/stale-frame protection and future invalidation after successful Load.
Capture/Load refusal poisons history; late compact Load can still partially
mutate the world, so callers must abort or independently recover. Discard after
known-world recovery invalidates the directory but never clears failure. There
is no runtime allocation, callback, rendering or file IO in the history module.

The offline --history cohort reserves/prefaults sixteen 8 MiB slots before play
and captures every post-draw boundary from sample 120, including resimulation.
All older/corrected restores now use the ring directory. Same-input stock and
interpreted cases each capture 1171 boundaries and perform 12 ring loads across
six 15-tick intervals. Corrected cases each capture 1112 boundaries and perform
three ring loads across two corrected passes. All intermediate/continued traces,
37 compact components per endpoint and complete raw endpoint bytes match.
Every restore discards all fifteen future slots and refuses a wrapped stale
frame. Predicted recovery discards corrected history before republishing B.
Observed peak payloads range from 2639072 to 2827496 bytes in these scenarios;
8 MiB is an experimental slot bound, not the final universal allocation policy.

Evidence: A:/FreakMelee-test-archives/compact-ring-oct12-six-a and
compact-ring-oct12-correction-a (PASS; 1200 samples per stock/interpreted case),
compact-ring-oct12-independent-verification.json and compact-ring-oct12-unit-
tests.log. Replay and 173 directory/failure assertions pass. Directory unit
tests use a fake world for bookkeeping/refusal policy only; actual serializer,
ring wrap/load and gameplay proof comes from the live cohorts. Documentation:
melee/docs/native_compact_match_history.md.

Production remains on page history. Its pre-input capture phase must be audited
against the accepted post-draw phase before wiring this storage into the live
session. Full candidate prevalidation, allocation/asset residency closure,
scene retirement, general output reconciliation and measured 15-tick eight-
actor budget remain required. These changes do not complete the full goal.


### October 12, 2026: production-phase/cold startup audit

Added an offline --before-input history hook at the real production BeginFrame
position, with capture from boundary zero and no post-draw double capture. The
first actual cold tests refuse exactly stage, stage-hazards, stage-generator
and stage-spawns. Those host leases are established only by original stage
start/driver constructors, invoked from the READY/start flow; pending startup
callbacks and initialization transitions remain outside the compact contract.
No lease guard was bypassed, no empty substitute snapshot was admitted, and
no initialization callback was moved/executed early. Production stays on pages.

The cold refusal exposed an independent Aurora exit crash before a first frame.
Debugger evidence identified the GX FIFO XF viewport decoder dereferencing a
missing graphics recording packet. The recorder now uses its existing ownership
state: target-size queries take the existing window fallback when no packet is
active, and viewport/scissor GPU commands are recorded only into an active
packet. GX logical state remains intact. No new allocation, task, lock or shared
thread state was added. The required Multi-Core By Default article was read.
Cold refusal now flushes all rejected owners and exits normally with status 1.

Evidence under A:/FreakMelee-test-archives/: compact-phase-oct12-six-a and
compact-phase-oct12-correction-a are preserved FAILED cold cohorts;
compact-phase-oct12-cold-b preserves the four-owner rejection plus access
violation; compact-phase-oct12-cold-debug.log records the native worker stack;
compact-phase-oct12-cold-c proves expected refusal with clean diagnostic exit.
compact-phase-oct12-normal-c PASSES stock/interpreted eight-actor post-draw
history, 480 samples and 15-tick resimulation, exact traces/compact/full raw
endpoints after the graphics fix. compact-phase-oct12-gm-b PASSES the connected
loop/controller/player/camera fixture (2018 plus 31797 checks). The independent
verification JSON checks unchanged source/fixture pins and distinguishes cold
refusal handling from successful snapshot acceptance. Documentation:
melee/docs/native_compact_frame_phase.md.

Next action: represent pending stage initialization/callback/definition lifetimes
and their transition into initialized owners, or establish a proven canonical
setup boundary before prediction, while accounting for original startup/RNG/
player-visible behavior. This is real remaining implementation work; an arbitrary
warm-up is not production acceptance. Candidate prevalidation, residency,
retirement, output reconciliation, production integration and performance also
remain. The full goal is incomplete and can continue without user input.


### October 12, 2026: pending stage callback ownership foundation

The actual Ground stage-start callback nodes now use fixed native scene storage
instead of individual HSD heap allocations. Registration remains a linked LIFO
list and Ground_801C0FB8 still saves next before invoking each source callback at
the original READY/start point. Nodes are cleared after the original traversal.
Capacity is 4096 events (the GObj capacity), with tracked targets and explicit
exhaustion failure. Multiple events may target the same object. No callback is
executed early by admission or snapshot code, and no runtime node allocation
occurs. Scene initialization resets the native queue/count/started phase.

native_stage_start_snapshot.[ch] owns pending records through source callback
definition identities and GObj generation claims. Save validates the actual
head/next chain; immutable candidate validation can resolve saved targets while
the current target has been freed. Load requires restored GObj companions and
checks all targets before rebuilding queue/head/phase, without invoking callbacks.
The component is not yet wired into the full aggregate or loaded/started stage
lease transition, so the same four cold owner refusals remain intentionally open.

Evidence under A:/FreakMelee-test-archives/: stage-start-oct12-live-b PASSES
480-sample stock and interpreted eight-actor comparisons against the frozen
pre-change executable: every recorded trace family/RNG matches. Its actual cold
component saves/validates/loads/recaptures one pending callback exactly (56 bytes)
for both families, while the full cold snapshot still refuses and exits status 1.
stage-start-oct12-history-a PASSES per-frame history and 15-tick resimulation:
traces, all 37 current compact records and complete raw endpoints remain exact.
stage-start-oct12-unit.log has 50 snapshot assertions and zero callback invocations.
stage-start-oct12-independent-verification.json checks unchanged pins and evidence.
The earlier live-a test setup failure is retained: the isolated baseline initially
lacked runtime DLLs. Final baseline has matched DLL/UI dependencies. Documentation:
melee/docs/native_stage_start_callback_snapshot.md.

Next: connect pending-node ownership to loaded/started stage phase snapshots and
the generator/hazard/spawn initialization transition. Aggregate candidate target
agreement is required before publication; this is not full cold capture or
rollback-across-start acceptance. Candidate prevalidation, residency, production
integration, lifecycle/output reconciliation and performance remain required.
The full project goal remains incomplete and work can continue autonomously.


## Compact loaded/started scene checkpoint - 2026-10-12

The aggregate now has 38 owners, with actual pending stage callbacks, matching
phase/list head and candidate GObj generations checked before publication. Stage,
hazard, generator and spawn leases retain loaded history across original match
start callbacks; actual scene retirement still invalidates captures. VS ownership
adds READY once flags, HUD coordinates/visibility, nametag visibility, typed lazy
debug-text camera identity and crowd-reaction counters. No callback is moved or
source gameplay arithmetic changed.

Evidence: stage-phase-oct12-transition-f PASSES stock/interpreted eight-actor
480-input pre-input histories and a 15-tick rewind from 78 across match start.
All 38 components, intermediate/continued traces and full raw endpoints match.
stage-phase-oct12-cross-source-b PASSES earlier/current rendered mixed-roster
480-input matches on Final Destination, Fountain and Green Greens, plus eight-
tick compact resimulation. Owner fixture h passes 86,913 assertions; connected
GM fixture e passes 2,018 loop and 31,797 camera checks. Focused history and stage
callback CTests pass. Final receipt pins are independently rechecked in
A:/FreakMelee-test-archives/stage-phase-oct12-independent-verification.json.
See melee/docs/native_compact_stage_start_transition.md for contracts and limits.

Frame-zero rewind remains INCOMPLETE: zero-b no longer crashes after restoring
the lazy camera's null state, but a class-backed JObj parent pointer still differs
(six address bytes). Gameplay traces match; this is still a failed acceptance.
Earlier READY, nametag, lazy-camera and crowd failures remain archived. Frame-zero
capture does not prove complete cold resimulation. Broader resource/graph lifetime
closure, candidate-wide prevalidation, production compact history integration,
output reconciliation and the final rollback performance target remain open.
The full goal remains active and incomplete; no multi-PC acceptance is claimed.


## Material texture roots and frame-zero acceptance - 2026-10-12

Correction to the previous parent-pointer diagnosis: the native debugger caught
the original first-frame writes in HSD_MObjDeleteShadowTexture from lbShadow
drawing. The field was a TObj next link, not a JObj parent. Rewind restored the
class-backed links but omitted the shadow chain head, so source cleanup could
not find the nodes again. New compact owner 38 (material-textures) restores the
actual shadow head and lazy toon root with typed class claims. It is 112 bytes,
uses no tick allocation, checks the saved chain against the immutable class view
before publication, and does not invoke add/delete/ref/render calls during Load.
Source material/shadow algorithms remain unchanged; real HSD drawing is retained.

The aggregate now has 39 components. material-textures-oct12-zero PASSES 480-input
stock/interpreted eight-fighter histories and 15-frame rewind from boundary zero,
with all components, complete raw endpoints and intermediate/continued traces
matching. Fountain and Green Greens frame-zero cohorts also PASS. The six-interval
cohort PASSES 1,200 inputs, 234 owner comparisons and six raw endpoints per family
across startup and gameplay. The corrected-input cohort PASSES fresh reference
and repeated 15-frame compact corrections, including audio deduplication and
original continuation. Controlled owner fixture b passes 86,952 assertions and
39 new material checks using exact original shadow list operations. New module
passes strict warning checks; focused history/callback/GObj consumer CTests PASS.
All final receipt pins are rechecked in material-textures-oct12-independent-
verification.json under A:/FreakMelee-test-archives/. See
melee/docs/native_material_texture_snapshot.md for contracts and evidence.

Frame-zero closure is now proven for these tested cases, not every possible
fighter/stage/resource lifecycle. Candidate-wide prevalidation, residency and
retirement, output reconciliation, production compact history integration and
the final performance deadline remain open. Production still uses broad pages;
16 x 8 MiB is diagnostic ring backing, not the final sizing policy. The full goal
remains active and incomplete. Historical failed receipts are retained.


## Full compact snapshot timing and heap validation - 2026-10-12

Load timing now includes candidate actor/class preflight and reports that phase
separately. Earlier total_ns diagnostics excluded it. An optional compact
benchmark records sixteen complete capture/restore calls per 120-input census
checkpoint, using null profiling outputs and existing reserved buffers. It checks
exact final component recapture outside measurement. This is warm same-boundary
cost, not the actual fifteen-tick simulation deadline.

Aurora heap candidate validation now uses the existing immutable geometry-epoch
ownership catalog instead of reconstructing and sorting the current allocation
lists on every validation. Saved ranges, totals, heap owner, links, alignment,
epoch and alias checks remain. Capture still validates current lists. A fixture
damages the later live next link and restores a valid earlier snapshot. No source
allocation/free behavior or savestate field changes.

Measured means of four checkpoint medians (microseconds, preliminary): stock
capture 1142.1 -> 1060.7, restore 787.4 -> 704.5; interpreted capture 1294.3 ->
1268.3, restore 871.3 -> 842.4. CPU/cache noise applies; no portable performance
guarantee is claimed. The sub-100-us aspiration and full rollback deadline remain
unmet. Heap validation, animation, class, GObj and reservation work remain useful
optimization targets; copying the compact bytes is only part of the cost.

compact-timing-oct12-b PASSES eight-fighter stock/interpreted exact frame-zero
15-tick resimulation, 39 owners, complete raw endpoints and benchmark recaptures.
The corrected-input cohort PASSES fresh reference/repeated correction and audio
deduplication. Owner fixture passes 86,956 assertions including damaged-list
recovery and existing malformed/multi-heap/capacity checks. Focused history, stage
callback and GObj consumer CTests PASS. Independent verification checks final
receipt/source pins and the frozen pre-optimization executable digest. Evidence
is under A:/FreakMelee-test-archives/compact-timing-oct12-*. See
melee/docs/native_compact_snapshot_timing.md. Production compact integration,
full candidate prevalidation, broader resource closure and final performance
acceptance remain open. The full goal remains active and incomplete.


## Saved allocator link lookup checkpoint - 2026-10-12

Aurora candidate heap validation now indexes already-validated saved Cell
addresses into fixed host scratch instead of binary-searching the sorted record
array for every allocated/free list link. Exact address membership, owners, prev
links, list counts, termination, sorted free order, ranges, epoch and alias checks
remain. The 131,072-entry table (1 MiB) supports the 65,536-cell limit at at most
half occupancy. Generation stamps separate candidates and avoid per-call table
clearing; wrap clears before reuse. No allocations or new threading. Expected
lookup cost improves for normal allocator addresses; adversarial collisions do
not have a constant-time guarantee. Snapshot records and source allocation/free
behavior are unchanged.

Four-checkpoint median means (microseconds, preliminary): stock capture
1060.7 -> 986.4, restore 704.5 -> 607.0; interpreted capture 1268.3 -> 1172.2,
restore 842.4 -> 708.4. Stock heap validation samples are about 111.6 us rather
than 171.4 us. CPU/cache noise and warm-state measurement limitations apply.

heap-index-oct12-timing-a PASSES stock/interpreted eight-fighter frame-zero
15-tick replay, all 39 records, complete raw endpoints and benchmark recaptures.
The corrected-input cohort also PASSES. Final owner fixture e passes 86,981
assertions, including maximum valid capacity, first unsupported larger state,
and a maximum-to-small same-epoch transition rejecting an address cached from
the older candidate. Existing malformed, epoch, alias and damaged-live-list
checks remain green, as do focused CTests. Final source/receipt pins are verified
in A:/FreakMelee-test-archives/heap-index-oct12-independent-verification.json.
See melee/docs/native_compact_snapshot_timing.md for evidence and limitations.
The sub-100-us aspiration, fifteen-tick frame budget, production compact history
integration and full candidate/resource closure remain open. The full goal
remains active and incomplete.


## Validating heap writer checkpoint - 2026-10-12

The idle heap snapshot writer now checks collected live extents against admitted
geometry as well as its existing topology/count/alignment/overlap/total checks.
It guarantees a valid immutable output before publishing. Compact capture relies
on this contract for the heap owner, avoiding a redundant validation/index pass
on that freshly written output. Restore retains complete candidate validation;
other owners and snapshot fields are unchanged. A forged structurally valid live
list using another heap's cell is rejected without output publication.

Mean four-checkpoint upper capture medians, microseconds: stock 986.4 -> 884.8,
interpreted 1172.2 -> 1073.2. These are preliminary warm costs; restore was not
optimized and no full rollback deadline or portable speed guarantee is claimed.
heap-writer-oct12-timing-a PASSES eight-fighter stock/interpreted exact frame-zero
15-tick replay, all 39 records, complete raw endpoints and benchmark recaptures.
Corrected-input replay and audio checks also PASS. Owner fixture a passes 86,987
assertions including forged topology/publication checks; maximum size, stale
index, geometry and damaged-live-list recovery coverage remains. Focused CTests
PASS. Final source/executable pins are independently verified under
A:/FreakMelee-test-archives/heap-writer-oct12-independent-verification.json.
See melee/docs/native_compact_snapshot_timing.md. Further capture optimization,
complete candidate/resource closure, production compact integration and the
actual fifteen-tick frame budget remain open. The full goal is still active
and incomplete.


## Animation capture writer checkpoint - 2026-10-12

Animation capture now uses the checked pool writers' byte counts instead of
rescanning both pools twice for sizes. It validates the resulting saved decoder/
controller graph before publishing: cursor/opcode bounds, links/cycles, controller
references, counters, geometry, aliases and idle restrictions remain checked.
The valid own result no longer receives a separate second graph check in compact
capture. Restore still validates candidates. The unused live-graph checking mode
is removed; no source animation algorithm, format, callback or allocation change.
Failed writes may leave partial backing but do not publish the byte count.

Final four-checkpoint upper capture medians average stock 751.2 us and interpreted
940.4 us, versus 884.8/1073.2 us at the prior heap-writer checkpoint. This warm
measurement had no competing game test, but setup briefly overlapped a test build
and background CPU/cache effects are not fully controlled. Earlier simultaneous
cohorts were noisier. No portable speed or fifteen-tick deadline is claimed.

animation-writer-oct12-timing-c PASSES eight-fighter stock/interpreted frame-zero
15-frame replay, all 39 components, raw endpoints and benchmark recaptures.
Corrected replay/audio checks also PASS. Dedicated real animation owner/decoder
fixture c passes 91 cases/1086 checks including new malformed live-state count
publication guards. Read-only packet playback, full replay/negative controls,
cycles, references, generations and retirement remain covered. Rebuilt animation
owner/heap plus history/callback CTests PASS. Final pins are checked under
A:/FreakMelee-test-archives/animation-writer-oct12-independent-verification.json.
See melee/docs/native_compact_snapshot_timing.md for details and limitations.
Production compact integration, candidate/resource closure, further optimization
and the actual fifteen-tick frame budget remain open. The full goal remains
active and incomplete.


## Complete rendered compact replay timing - 2026-10-12

Added opt-in fifteen-tick interval timing: final compact restore, actual per-
boundary history capture/publication, input/replay observers, source simulation,
full drawing and renderer begin/HSD work/renderer end subdivisions. Whole elapsed
time ends before endpoint capture, comparison and broad recovery. Each accepted
record requires fifteen calls for every phase and fifteen history saves; corrected
passes reset counters independently. Trace/object-census instrumentation, real
HSD drawing and render RNG remain enabled. No drawing-disabled path introduced.

Six interval medians (ms), stock/interpreted eight: history 14.967/18.277, input
2.526/2.500, simulation 4.051/6.145, drawing 14.035/14.137, final restore
0.939/1.009. Whole interval plus restore: 37.307/42.892 ms, ranges
35.263-41.557/41.267-44.955 ms. Drawing includes HSD work 5.433/5.306 and
renderer end 8.417/8.766 ms; subdivisions are not additional to drawing.
The measured 16.67-ms target is NOT met. Retained diagnostics/cache/background
work prevent claiming these are clean production costs. Warm capture benchmarks
understate the actual per-boundary costs observed here. Source end_frame(false)
still drains GX FIFO and finishes graphics; thread-mode drain waits on the
processed watermark. This combined end phase needs finer attribution before
changing behavior. History capture and renderer processing remain major targets.

compact-budget-oct12-six-b PASSES 1200-input stock/interpreted cases, six complete
15-tick timings, 234 compact comparisons and six full raw endpoints each.
Corrected-input cohort also PASSES two complete timing passes, fresh/reference
checks, exact endpoints, continuation and audio. Connected GM fixture b passes
2018 loop and 31797 camera checks with timing disabled and aborting timing leaves.
Focused CTests PASS. Final pins verified in compact-budget-oct12-independent-
verification.json under A:/FreakMelee-test-archives/. See
melee/docs/native_compact_replay_budget.md. Diagnostic-free budget measurement,
capture/renderer optimization, candidate/resource closure and production compact
integration remain open. The full goal is active and incomplete.


## Reduced-observer compact replay timing - 2026-10-12

Added explicit --quiet-replay-timing to the offline census runner. This omits
per-frame fighter/action traces and object census; input-only replay no longer
computes a state digest without an enabled observer. Verified replay/recording
digests retain their existing behavior. Timing labels derive from real observer
settings. Quiet mode keeps real source drawing and RNG, compact history checks,
all 39 owner comparisons, full raw endpoints and audio checks. It deliberately
has no intermediate trace or independent forward reference proof; receipt marks
trace_comparison_performed=false and differences=null. It does not accept
corrected-input or warm microbenchmark modes. Normal traced proof remains intact.

compact-budget-oct12-quiet-a PASSES 1200 inputs per stock/interpreted eight-actor
case, six 15-tick intervals, 234 owners and six complete raw endpoints each.
Whole interval plus restore median 34.996/41.015 ms (stock/interpreted), range
34.461-36.830/38.575-43.525 ms. History 15.490/18.473 ms; input 0.849/0.796;
simulation 4.135/6.290; drawing 13.872/14.095. Peak record bytes
2655680/2846512; diagnostic ring still reserves sixteen 8-MiB slots. These
are not total process memory figures or production sizing promises.

Same executable/source normal traced six-interval observer-regression and
fresh/repeated observer-correction cohorts PASS, including continuation and
audio checks. History/stage-start/GObj consumer CTests PASS. Source/executable/
receipt/log hashes independently verified in compact-budget-oct12-observer-
verification.json under A:/FreakMelee-test-archives/. See
melee/docs/native_compact_replay_budget.md for exact scope and limitations.

The 16.67-ms goal is still unmet. Removing observers helps input work; actual
history capture and retained render processing remain the primary costs.
This is reduced-observer offline evidence, not diagnostic-free production or
network benchmarking. Continue capture/render optimization and candidate/
resource closure, then production compact integration. Full goal active,
incomplete. No drawing-disabled path added.


## Reservation and GObj capture writers - 2026-10-12

HSD reservation save validates boundary/geometry once and streams each bin from
the checked pool writer's returned size. Removed full-registry/per-bin sizing
passes. Final header and written count publish only when all bins succeed;
failure can leave partial scratch but preserves written/live state. Candidate
restore still prevalidates all owners. Compact capture skips its second HSD
reservation validation and duplicate GObj graph validation; GObj Save already
validates its complete saved graph before success. No source gameplay change,
allocator policy change, wrapper, callback or tick allocation introduced.

reservation-writer-oct12-owners-a PASSES 88516 checks, including 1646 reservation
checks, every insufficient capacity 0..759, exact-capacity byte equality and a
corrupted final bin without size publication. Existing malformed/retired/alias/
construction-boundary and allocation continuation checks remain in place.

Quiet-a and regression-a cohorts PASS 1200 inputs per stock/interpreted eight-
actor case, six 15-tick intervals, 234 owners and six full raw endpoints each.
Correction-a PASSES fresh/repeated corrected inputs, continuation and audio.
Rebuilt history/stage-start/GObj consumer CTests PASS. Current source, executable,
receipt and log pins verified in reservation-writer-oct12-quiet-a-verification.json
under A:/FreakMelee-test-archives/. See melee/docs/native_compact_snapshot_timing.md.

Quiet actual 15-save history medians before/after: stock 15.490/14.109 ms;
interpreted 18.473/15.604 ms. Whole replay plus restore 33.753/37.168 ms after.
Sequential offline cohorts have uncontrolled background/cache variance; stock
has no populated reservations, so its timing change cannot be attributed to
populated reservation serialization. The 16.67-ms interval target and sub-100-us
snapshot aspiration remain unmet. Capture/render optimization, candidate/resource
closure and compact production integration remain open. Full goal active and
incomplete; real HSD drawing retained throughout.


## GObj/class/allocator capture batch - 2026-10-12

GObj capture streams its checked object/procedure writers without preliminary
pool sizing scans. Live-context proof and full saved graph validation still
precede successful size publication. Class backing references prove retained
geometry without repeatedly traversing the type tree; they remain explicitly
weaker than type/liveness proof. Type queries and class snapshot checks retain
tree/counter validation. The class writer now proves the size-class table header
shape and exact retained backing before publishing. Capture omits duplicate
class and ObjAlloc candidate validation after those checked writers succeed.
Restore validators remain intact. No allocator policy or source gameplay change.

Census source pins now include the full Melee C/C++ source/header tree alongside
the executable, selected Aurora sources, content and ISO. Writer/metadata changes
cannot silently drift during a measured cohort.

capture-batch-oct12-owners-a PASSES 88523 checks; malformed counters still refuse
class snapshot/type publication while retained geometry references remain usable.
GObj-a PASSES 17 cases/41381 checks including truncated first/second pools,
corrupt second-pool metadata, exact capacity and unchanged written count on
failure. Rebuilt history/stage-start/GObj consumer CTests PASS.

Quiet-a PASSES 1200 inputs per stock/interpreted eight-actor case, six 15-tick
intervals, 234 owner comparisons and six full raw endpoints each. Traced-zero-a
PASSES the full reference/traced cohort from frame zero through READY/stage
startup and five later intervals. Correction-a PASSES fresh/repeated corrected
inputs, continuation and audio checks. Fixture/source/executable/receipt/log pins
verified in capture-batch-oct12-quiet-a-verification.json under
A:/FreakMelee-test-archives/. See melee/docs/native_compact_snapshot_timing.md.

Quiet 15-capture history median: stock 12.998 ms, interpreted 13.098 ms; prior
batch 14.109/15.604 ms. Whole replay plus restore 32.347/34.842 ms. Sequential
offline costs have uncontrolled cache/background variance; no portable timing
guarantee. The 16.67-ms interval target and sub-100-us snapshot aspiration are
unmet. Continue shared pool scan consolidation, renderer cost attribution,
candidate/resource closure and production compact integration. Goal active,
incomplete. Actual source drawing and render RNG retained throughout.


## Shared pool writer consolidation - 2026-10-12

Pool save now validates and serializes generations/occupied records together,
then validates and writes the exact free order. Removed preliminary full live
metadata validation and separate generation/payload scans from the writer.
Geometry/count relationships remain checked up front, and occupied count is
checked before each extra payload copy. Corrupt under/over-declared occupancy,
zero live generations, bad peaks and duplicate/out-of-range/conflicting free
slots refuse successful publication without writing past capacity or changing
live state/written count. Scratch may contain partial bytes after refusal.
Private layout, generations/free order and allocator behavior are unchanged.
Successful candidates retain view/load validation; fixed stack scratch only.

pool-writer-oct12-core-a PASSES 589585 checks, including eight malformed live
writer cases with capacity guard bytes, every four-slot free-order/occupancy
permutation, full 65536-slot resource capacity and exact allocation continuation.
Owners-a PASSES 88523 checks, GObj-a 17 cases/41381 checks, animation-a 91 cases/
1086 checks plus owner cases, read-only streams and omission negative controls.
Rebuilt history/stage-start/heap/GObj/animation-owner CTests all PASS.

Quiet-a and traced-zero-a PASS 1200 inputs per stock/interpreted eight-actor case,
six 15-tick intervals, 234 compact owners and six raw endpoints each. Traced run
includes frame zero/READY/startup plus independent references. Correction-a
PASSES fresh/repeated correction, original continuation and audio checks.
Current source/executable/fixture/receipt/log pins verified in
pool-writer-oct12-quiet-a-verification.json under A:/FreakMelee-test-archives/.
See melee/docs/native_compact_snapshot_timing.md.

Quiet history median stock/interpreted: 11.670/13.412 ms for fifteen captures;
whole replay plus restore 31.014/34.793 ms. Prior batch was 12.998/13.098 ms
history and 32.347/34.842 ms total. Stock improved in this sequential cohort;
interpreted capture did not improve. Cache/background variation is uncontrolled,
so no universal speedup claim. Production game/renderer commands were inspected
and already use -O2; these costs are not explained by an unoptimized build.

16.67-ms interval and sub-100-us snapshot targets remain unmet. Continue cost
attribution/optimization, candidate/resource closure and compact production
integration. Actual source drawing/render RNG retained; full goal active and
incomplete.


## Direction: mutable match arena, then simulation-only rollback - 2026-10-12

User explicitly confirms intermediate rollback ticks should not render, and
snapshot cost followed by render cost are the next priorities. This supersedes
the earlier temporary request to retain drawing in the fast path. Preserve
gameplay-relevant updates by moving/separating them from rendering; do not retain
full graphics work just because the original code couples a state write to it.

The production snapshot target is a compact contiguous mutable match arena plus
a small explicit globals/header span, copied directly into fixed history storage
and restored to the same addresses. Ownership admission and scene construction
establish invariants. Retain exhaustive validators/oracles for diagnostic proof,
not repeated per-tick rediscovery. The 39-component packing implementation is
ownership inventory/diagnostic evidence, not the permanent production hot path.
Move actual storage/callsites, without mirrors or compatibility layers. Immutable
assets/programs and backend resources stay outside copied state. Measure mutable
high-water size; copying oversized old static pools wholesale is not a solution.

After state layout, separate fixed simulation/state updates from mesh emission,
GX recording, backend submission and presentation. Rollback executes fifteen
simulation-only ticks, then draws the current result for presentation. Rendering
frequency must not change gameplay/RNG or advance animations/effects twice.

Confirmed prior coupling: ftDrawCommon_80080E18_inline2 writes a star-KO fighter's
camera-projected cur_pos and joint translation. Prior repeated skip-draw trials
first diverged at sample4037; RNG differed later at4126, with seed changes in
simulation, not the immediately preceding draw. This is a source state-write
problem, not proof that GPU rendering is required. Audit and move such updates
with correct camera/physics ordering, then compare render-every-tick and no-render
replays, including paired/scripted fighters, transformations, particles and KOs.

See melee/docs/native_match_state_arena_plan.md for layout, migration and
acceptance. Latest interpreted quiet cohort mean: captures13.60ms, drawing13.84ms,
simulation5.75ms, restore1.03ms, input0.76ms, overhead0.09ms over fifteen ticks.
The separate memcpy-budget-oct12-a probe measures copy-only medians6us/82us/152us
for256KiB/1MiB/2846512bytes save; current-size restore386us. Different cache/
access patterns and no game integration; not a production timing prediction.

This checkpoint records the architecture direction only. Arena migration and
simulation-only production replay are not yet implemented. Goal remains active,
full scope intact and incomplete. Historical exact rendered replay remains
diagnostic evidence until the replacement is independently verified.


### 2026-10-12 - Live scene arena and construction-time snapshot admission

User direction: migrate actual mutable state to contiguous backing; validate ownership/code/data at scene construction instead of repeated graph discovery on snapshot ticks. Retain inexpensive capacity/alignment/lifetime checks needed for runtime allocations. Rendering separation follows complete state-layout migration.

Implemented native_match_arena.[ch] with an in-backing allocation cursor, fixed root admission and prebound private history slots. Alias/range checks are performed when roots and history storage are bound; own Save/Load use prefix/root memcpy with no typed graph walk, record packing or reference reconstruction. These slots are internal history, not an import format. The scene reserves/commits/first-touches backing before constructors, reuses it across scenes, and rejects old-scene history by lifetime.

Moved actual GObj/procedure payloads, slot generation/free bookkeeping and scheduler tables into the arena. Tables use configured link/priority extents rather than all possible extents; full public maxima still work when configured. Pool descriptors and mutable scheduler controls are admitted small external roots. The existing broad rollback oracle includes the new backing as well as old heaps/globals during migration.

This is the first owner migration, not completed snapshot replacement. GObj/procedure pools still reserve their full 4096/16384 capacities; the small fixture copies 1460780 bytes including roots. Do not extend this by copying the oversized item/VM slabs wholesale. Demand-based actor payload storage, remaining HSD/animation/VM/global state ownership, complete arena history integration, old-backend removal, and full-match timing remain required. No full-arena performance claim or simulation-only rendering path is made.

Validation: full native client builds; five CTests (arena, generic pool, actual GObj scheduler, match history, stage-start) pass. arena-oct12-gobj-final/receipt.json passes 18 scenarios / 41554 checks, including actual GObj/procedure resurrection, scheduling, saved roots, cursor rewind, repeated load, initialized reclaimed bytes, allocation continuation/exhaustion and stale-scene rejection. arena-oct12-replay-a and final-source arena-oct12-cold-final pass stock and interpreted eight-fighter cases, two repeated 15-tick intervals, 78 component comparisons per case, identical fighter traces and zero raw differing bytes. arena-oct12-correction-a passes fresh-reference and repeated corrected-input replay; this preceding cohort predates the final diagnostic buffer-alias guard adjustment. Tests are hidden and muted. Archives are on A:/FreakMelee-test-archives.

Implementation/evidence boundaries and next migrations are recorded in melee/docs/native_match_state_arena_plan.md. Whole project remains unfinished; this checkpoint does not request a pause or block.


### 2026-10-12 - Contiguous ordinary HSD state and one-time admission

Moved actual ordinary class-piece chunks, size-class tables/entries and ordinary ObjAlloc chunks into shared native_hsd_arena backing. Reuse/splitting/free lists remain source behavior. Reserved article/construction pools retain their existing separate ownership. Allocation consumes fixed backing; no ordinary OS heap growth or mirrored payloads.

Heap lifetime differs from GObj scene lifetime. Class-library amnesia retires definitions/history while preserving backing used by ordinary allocators. HSD_CreateMainHeap forgets both families before resetting backing. Scene transitions retire admission before preload. Frozen copy-only history forbids late class-definition and allocator-registration changes; ordinary allocation with admitted definitions remains supported.

Added counted root spans that copy initialized directory prefixes and restore their captured lengths, not 65536-entry maximum slabs. Admission verifies the counter itself is captured state. Append writers enforce capacity. HSD admission checks ownership/class/allocator structure once after construction, then prebound copy-only saves/loads copy the prefix, directory prefixes and known small roots. Diagnostic class/ObjAlloc records preserve the shared cursor and reject cursors that would overlap active records.

The real scene census now admits and seals HSD history at construction, then measures an isolated warm single-span copy before continuing its existing full-component oracle. Eight-fighter constructed HSD prefix is 1178432 bytes, 1265564 copied bytes including 49 roots. Final stock/scripted corrected-replay probes report saves 44.538/46.741 us, loads 37.878/38.737 us, respectively. This is NOT full match/ring-history timing.

Verification: native client builds; arena, actual GObj/scheduler and heap CTests pass. hsd-span-oct12-owners-final/receipt.json passes 88570 checks, including copy-only restoration across both families, free lists/counters/directories, same-address allocation continuation, ordinary object survival across class amnesia, heap reset and stale history. hsd-span-oct12-correction-final passes stock/scripted eight-fighter fresh-reference and repeated corrected replay, exact traces and zero broad endpoint differences. Cold-start same-input cohorts also pass; final renamed-source cold cohort is recorded alongside the corresponding documentation. All runs hidden/muted. Archives A:/FreakMelee-test-archives.

Production rollback remains broad during migration and experimental replay still packs all 39 components and renders. Actor/animation/VM/control ownership and footprint reduction, complete arena history integration, obsolete-backend removal, full-match timing and simulation-only rendering remain required. See melee/docs/native_match_state_arena_plan.md. No goal completion/pause/block is requested.


## Actor payload prefix migration (2026-10-12)

Fighter, Ground, Item and ItemLink payload arrays now occupy fixed windows in
one process-reserved backing created and first-touched at scene construction.
Stable base + slot * stride addressing, capacity, free order and generation
behavior remain unchanged. Metadata and descriptors are admitted as fixed roots
of scene history; each payload window is a counted root whose length is its
initialized high-water count. Free holes below that count remain copied. Unused
capacity above it is omitted, while full capacity remains available to gameplay.
Allocation updates the count and zeroes the payload; history restores the count
and allocator metadata, so slots first reached in a discarded future are safely
reinitialized on the same address and with the same generation on continuation.
The diagnostic packed pool header also restores that count. The broad page
oracle includes the new actor reservation during migration.

Source ownership evidence: actor-prefix-oct12-owners/receipt.json passes 19 cases
and 148,593 checks, including repeated actual typed-pool prefix restores and
allocation continuation. actor-prefix-oct12-pool/receipt.json passes 589,527
checks and 19 malformed-candidate cases. Heap, arena and actual scheduler CTests
pass. Updated standalone animation and GM fixtures construct the scene backing
explicitly: animation-final passes 73 cases / 668 checks; gm-final passes 2,019
checks plus 31,797 camera checks. All receipts are under
A:/FreakMelee-test-archives with the actor-prefix-oct12- prefix.

actor-prefix-oct12-cold and actor-prefix-oct12-correction both pass stock and
interpreted eight-fighter cases: exact traced replay, all 39 component endpoints
and zero raw differing bytes, including input-zero and corrected-input replay.
actor-prefix-oct12-cold-verification.json independently verifies input/output
hashes and the shared executable a195155a5ad0f061262e21281d4569e9e5669cf59c219440e302bca7d59fb4a9.

The census now probes both admitted HSD and scene spans after real construction.
The cold cohort's scene span copies 1,783,068 bytes (1,475,352 backing prefix plus
28 roots), saving in 62.509-64.981 us and loading in 50.462-54.309 us. HSD copies
1,265,564 bytes separately, saving in 40.413-41.247 us and loading in
35.816-38.159 us. These are 32 warm same-buffer operations at construction, NOT
a complete match, production history or a cold ring-buffer timing result.
The replay loop still uses the legacy serializers and rendered ticks; total
15-tick intervals remain roughly 33-41 ms in this cohort. No overall rollback
speedup is claimed yet. Animation storage, script VM/control owners and the
remaining mutable roots still need migration before final copy-only history
integration; render-state separation follows. Full pool reservations also
remain, including GObj/procedure capacities, though unused actor payload
capacity is no longer part of copy-only scene capture.


## Animation and fighter-VM copy history (2026-10-12)

AObj controllers and FObj curve decoders now occupy one process-reserved,
first-touched backing created at heap initialization. It retains main-heap
lifetime rather than being reset by scene binding. Descriptor/generation/free
order metadata and animation globals remain mutable; their initialized payload
prefixes are counted roots admitted to HSD copy history at scene construction.
The broad page oracle also includes the new backing during migration.

Fighter interpreter actors now track their initialized prefix. All lookups,
retirement and reporting honor that frontier, so actors first reached in a
rolled-back future cannot stay visible. Attach clears new/reused slots and keeps
the existing first-free allocation order. The legacy diagnostic header captures
the frontier; its restore clears the relevant old/saved extent before rebuilding
its active records. Copy-only admission validates live storage and execution
self pointers once, then captures inline variables, fault/tick state, installed
callbacks, cached bindings and identities directly with no rebuild on load.
Immutable programs and idle interpreter call/register scratch stay resident.
The item/effect interpreter owners have not yet been migrated.

The animation source fixture now exercises the copy-only path using actual
AObj/FObj interpretation, target transforms, callback counters and RNG: repeated
8-tick replay is exact, future slots restore their allocator frontier and
continue at the same addresses/generations, and heap recreation expires history.
Its receipt is A:/FreakMelee-test-archives/animation-span-oct12-owners/receipt.json
(92 cases, 1,144 checks). Focused rebuilt CTests pass for heap, arena, scheduler,
animation ownership, fighter scripts and animation/pose editors. The fighter
script fixture also copies a real bound actor, runs its counter behavior, loads
without rebuilding bindings, and verifies its packed recapture and next callback.
A stale fixture OSReport prototype and oversized stack-local prepared program
were corrected while rebuilding that target; no gameplay change was needed.

The real construction census verifies interpreter coverage independently: it
perturbs each actor's inline variables/storage identity and clears its state
pointer, then restores scene copy history before running callbacks. All eight
interpreted actors (608 bytes of authored variable state) recapture exactly.
Stock has no interpreted actors. animation-vm-span-oct12-cold/receipt.json and
animation-vm-span-oct12-correction/receipt.json both pass stock and interpreted
eight-fighter exact traces, all 39 owner endpoints and zero raw differing bytes.
Their inputs/outputs/shared executable are independently verified by
animation-vm-span-oct12-cold-verification.json. Executable SHA256:
00db4743309b61776d961260ffd26c3b5d4f98dbfeda214bbe6e174c5d51acd0.

The cold cohort's constructed HSD span is 1,566,004 copied bytes (58 roots),
including animation. Its 32 warm same-buffer saves take 47.775-50.744 us and
loads 44.709-46.909 us. Scene capture is 1,783,080 bytes for stock and 1,851,112
for interpreted fighters (31 roots); saves take 56.750-57.300 us, loads
51.053-53.069 us. These remain isolated construction probes, not whole-match
cold/ring history measurements. The full replay path still uses diagnostic
serializers and rendering, taking roughly 32-38 ms per 15-tick interval here.
No full rollback deadline or overall performance improvement is claimed.

Next: migrate item/effect script owners and remaining control roots, reduce
unused GObj/procedure payload copying, integrate complete prebound copy history,
and validate forward/corrected replay against the retained oracle. Simulation
and render-state separation still follow snapshot migration. The complete
PROJECT_PLAN objective remains active and incomplete.


## Item/effect VM and GObj prefix history (2026-10-12)

GObj and procedure payloads now occupy two more fixed windows in the shared
actor reservation, preserving base/slot/stride addresses, full capacity,
allocation order and generations. Their initialized prefixes are counted scene
roots rather than full-capacity arena allocations. Scheduler tables and slot/free
metadata remain in the small physical scene prefix. Broad migration regions
already track the complete actor backing; diagnostic buffer alias checks now
exclude that backing too. The source scheduler fixture's copied state falls
from 1,609,308 bytes to 397,196 with the same lifecycle/topology assertions.

Item and effect interpreter owners now track initialized extents. All live
lookup, boundary, pin, retirement and allocation scans honor those frontiers;
newly reached slots are initialized before becoming visible. Their diagnostic
headers retain the extents and clear both old/saved ranges before rebuilding
occupied records. Construction admission checks live program/peer/self-pointer
identity once, then captures each initialized owner prefix plus its complete
object index, scene epoch and frontier. Copy loads restore variables, callback
state, creator/attachment/member relations and cached execution/storage views
without reconstructing interpreter bindings. The source arrays remain fixed
process reservations and included in the broad global oracle during migration.

Evidence under A:/FreakMelee-test-archives:
- item-effect-span-oct12-gobj/receipt.json: 19 cases / 148,595 checks, including
  repeated copy restore, scheduler topology and allocation continuation.
- item-effect-span-oct12-item-final/receipt.json: 39 cases / 1,249 checks. Its
  added copy-history test perturbs inline variables, state pointers/identities
  and object indices, restores exact component bytes, discards a newly attached
  future actor, and recreates it twice at the same address/generation with
  initialized variables. Existing callback/fault/exhaustion coverage still runs.
- item-effect-span-oct12-effect-final/receipt.json: connected actual EF creation,
  source Tornado comparison and effect lifecycle coverage, 45 cases / 5,303
  checks / 1,551 model frees. Copy history restores perturbed variables, storage
  identity/pointers, creator views, member generations and lookup indices twice
  to exact component bytes. The fixture controls unavailable renderer/classes;
  it does not prove arbitrary effects in the complete game.

The older owner/scheduler fixtures were updated to link the current scene pools
and shared unavailable presentation boundary. The Item scheduler now constructs
manual test Items in the actual typed pool and verifies failed-constructor pool
occupancy/validity instead of counting obsolete ObjFree calls. Unexpected
class-backed practice-grid resolution remains a trapped fixture boundary.
Focused CTests for heap, arena, GObj, item actor and item scheduler all pass.

item-effect-span-oct12-cold/receipt.json and
item-effect-span-oct12-correction/receipt.json pass both stock and interpreted
eight-fighter exact traces, all 39 component endpoints and zero raw differing
bytes, including input zero and corrected-input replay. Independent receipt,
source/output and executable hash verification is recorded in
item-effect-span-oct12-cold-verification.json. Executable SHA256:
605718bd3e633a509b1bea06dedacf24866b9fe81cea6afc7643a51a3d8a5778.

The cold cohort's scene prefix is now 262,936 bytes, with 603,488 copied bytes for
stock and 671,520 for interpreted fighters (41 roots). Its 32 warm same-buffer
saves take 19.644/26.019 us and loads 19.941/21.428 us. The HSD/animation span
remains 1,566,004 copied bytes (58 roots), saving in 59.916/62.013 us and loading
in 49.022/49.550 us in that cohort. These isolated construction probes are NOT
a complete match or cold ring-buffer timing result; their combined operation
has not been measured as production history. The full replay loop still uses
the old component serializers and rendering, with roughly 32-42 ms 15-tick
intervals. No overall rollback deadline acceptance is claimed.

Next: admit the remaining match-control roots and ownership metadata, integrate
complete prebound copy-only history, and prove full forward/corrected replay
against the retained broad/component oracle. Scope must include reserved HSD
construction pools, ordinary allocation state and every remaining mutable
control/registry owner; current animation/HSD/scene spans alone are incomplete.
Renderer-state separation and complete production performance verification
remain after snapshot integration. The full PROJECT_PLAN goal stays active.


2026-10-12 checkpoint: direct-copy match controls, collision and reserved HSD pools

Construction now preflights the current full 39-component inventory once, then
admits real mutable spans directly from 23 control owners plus collision and
reserved HSD pools. Control coverage includes input/RNG/player, GM loop/input
clock, cameras/subjects, stage/hazards/generator/spawn/start, dynamics, effects,
particles, lights/materials, versus/HUD/spawner, controller queues/rumble,
allocator/ID selection, RObj arguments and texture identity. The borrowed GObj
process mask is captured exactly once, with GM loop prefix/suffix admission;
strict overlap rejection is preserved. All nine mutable stage colors are owned.
RObj's fixed 100-argument buffer is constructed before sealing rather than
appearing lazily during a retained tick.

Collision admits active runtime vertices/lines/joints AND mutable source map
arrays, source MapCollData, bounds/islands, dynamic attributes, query/counters
and joint roots. Reserved HSD owners admit their actual descriptors, slot
metadata/free order and initialized payload prefixes, preserving full capacity.
Registry bind/release refuse during sealed history. Private scene frames expire
before ordered teardown releases reservations; object/pool lifetimes remain
valid for destructor callbacks. Shared ObjAlloc descriptors retain one owner.

A diagnostic now overwrites every admitted prefix/root, directly loads its own
prebound frame without callbacks/validation and recaptures all 39 components.
Both stock/interpreted eight-fighter HSD and scene spans match byte-for-byte.
This proves admitted-span restoration, not full mutable-world closure.

Evidence under A:/FreakMelee-test-archives/: collision-reservation-span-oct12-
collision-b/receipt.json passes 5 cases / 171 checks / 14 refusals, including
actual collision/platform/attribute/query and source-map mutation followed by
two direct restores. collision-reservation-span-oct12-owners-c/receipt.json
passes 88,671 checks; new copy-only reservation tests preserve future allocation
addresses/free order, payloads, failed claims and shared descriptor state, and
reject registry rebind/release until expiry. Synthetic stage fixtures now use
actual nativeMatchScene backing transitions. Focused heap/arena/GObj tests pass
(control-copy-proof-oct12-ctest.log).

control-copy-proof-oct12-final-cold/receipt.json and -final-correction/receipt.json
pass stock/interpreted eight-fighter 480-sample cohorts, two same-input 15-tick
intervals from input zero, fresh/repeated corrected-input intervals at 240,
all 39 component endpoints, full continuing traces and zero raw differing bytes.
Independent final verification: control-copy-proof-oct12-final-cold-verification.json.
Executable SHA256: 669b5796db4490862937b9f39487026cc812ce3cc3f2f2b2864af5d2168408b9.

Constructed HSD/animation copy span remains 1,566,004 bytes (58 roots). Scene
span is 812,478 stock / 1,181,526 interpreted bytes (176 / 214 roots), prefix
262,936 bytes. Interpreted isolated 32-warm-save/load measurements: HSD
46.769 / 43.800 us; scene 38.391 / 34.600 us. These are separate same-buffer
construction probes, NOT complete production or cold-ring timings. Rendered
component-history replay intervals still measure about 31-37 ms. No final
rollback deadline is claimed.

Still required before production cutover: article resource/model/animation
registries, Green Greens optional storage, remaining OS heap ownership, complete
prebound copy-only ring history and forward/corrected replay through that path.
Production remains broad tracking; experimental replay remains component
serialization plus original drawing. Simulation/render separation and full
performance acceptance follow state cutover. The full project goal stays active.
See melee/docs/native_match_state_arena_plan.md for contracts and evidence.


2026-10-12 checkpoint: article/model registry and optional grid copy history

Article collections now admit every mutable prefix plus counted created-node
spans and generation state. Vacant prefixes prevent discarded future collections
from leaking back into first-free searches. Secondary model registration and
matrix-claim state are copied directly; reserved graph pools retain their own
owner. Prepared package/recipe/model/animation ledgers are immutable during a match
and their transition APIs reject mutation while scene history is sealed.
Green Greens retains all 4096 grid slots with captured occupancy and a mutable
initialized-prefix count; Bind clears a whole grid before publication and older
loads hide discarded future slots. The private component record includes that
count. No format versioning/compatibility layer was added.

The retained page oracle had merged adjacent independent VirtualAlloc reservations
and then failed VirtualProtect (error487, distinct allocation bases in one range).
Only overlapping regions are now merged. Actual resource page-history passes
again, including per-region allocation-base checks. Green Greens also exposed
late material-class registration at frame102 after admission. Scene construction
now initializes all20 compiled rendering classes with the source ClassInfoInit
guard before sealing; no instances, gameplay decisions or methods are changed.
Material/refraction class symbols have explicit names and direct declarations.
Late class errors identify the class instead of silently extending a sealed schema.

Evidence under A:/FreakMelee-test-archives/: registry-grid-span-oct12-owners passes
88689 checks (grid16652 checks/21 refusals). -models passes657 checks/14 cases/
137graphs including direct model/graph/shared-HSD/Item-pointer/ID restoration and
same-address reconstruction. -resources-final passes static/animated cohorts
(3105/4835 checks), 39 source/VM animation pairs and resource-registry direct copy
loads after real removal and future generation reuse. These are explicit owner
fixtures, with controlled staging where documented, not whole-match acceptance.

registry-grid-span-oct12-final-green/receipt.json and -final-correction/receipt.json
pass stock/interpreted eight-fighter live cohorts: Green Greens480samples/two
15tick intervals from0/120, and Final Destination fresh/repeated corrected input
at240. Full continuing traces, every39component endpoint, zero broad differing
bytes and admitted-span overwrite/load/recapture proofs match. Focused heap,
arena,GObj,page tests pass. Independent final verification:
registry-grid-span-oct12-final-green-verification.json. Executable SHA256:
52fa74602fd79c600c4cbb9729ee4f3bc682c31591b389a40ba568d2348e8645.

Final Destination spans: HSD1566292bytes/62roots, scene842586stock/1211634scripted
bytes with216/254roots. Green Greens: HSD1688556bytes/62roots,
scene1000306stock/1369354scriptedbytes with217/255roots. Separate warm construction
probes and concurrently executed live cohorts do not prove production timing.
Production remains broad tracking; diagnostic replay remains component history
plus original rendering. Remaining OS allocation/global ownership, complete
copy-only ring history and forward/corrected replay through it precede production
replacement. Renderer separation and final performance/full-project acceptance
remain afterward. The goal stays active. Details: melee/docs/native_match_state_arena_plan.md.


2026-10-12 checkpoint: selected OS allocator control spans

The selected HSD heap descriptor, existing32byte Cell headers and Aurora current
heap selection are admitted once as actual scene roots. Construction uses the
validated allocator snapshot to enumerate addresses; arena Save/Load only copy
fixed spans, with no allocator traversal/sort/validation. Immutable asset bodies
and unused heap capacity stay excluded. A retained-history lease freezes topology:
actor/script/animation/HSD-object allocation still uses fixed pools/arenas, while
unexpected direct OS allocation/free or global geometry change is a diagnosed
contract fault. GM expires private frame lifetime and releases the lease before
ordered teardown. Any broader-test runtime OS mutation site must be migrated
properly before production. Normal clients remain unchanged unless requesting
this experimental history admission.

Aurora local commit aa99fc757c9a13d4596758f566cbba21be20b8a0 adds the allocator's
OSLockHeapForHistory/OSUnlockHeapForHistory APIs directly; game SDK declarations
match. No wrapper or format version was added. os-heap-span-oct12-owners/receipt.json
passes88702checks (OS65710), including overwrite/direct-load/allocator recapture,
lease release/reacquisition, and hidden child rejection of allocation/free/geometry.
Focused heap/arena/GObj/page tests pass (os-heap-span-oct12-ctest.log).

os-heap-span-oct12-green/receipt.json and -correction/receipt.json pass stock and
interpreted eight-fighter480sample cohorts: Green Greens two15tick intervals at
inputs0/120, Final Destination fresh/repeated corrected input240. All39component
endpoints, full continuing traces, zero broad differences and direct-copy probes
match. Independent final verification: os-heap-span-oct12-green-verification.json.
Executable SHA256: 3cbaa526ebbb8276a6ea0483c4d421a683e44a2b1a01487ce35a373c49565d75.

Final Destination scene844950stock/1213614scriptedbytes (291/317roots), HSD1566292.
Green Greens scene1002670stock/1371334scriptedbytes (292/318roots), HSD1688556.
Selected OS control adds2364stock/1980scriptedbytes, with no asset-body copying.
These are separate warm construction spans, with concurrent live cohorts, not
production/cold-ring timing proof. Coverage is the selected heap at tested scene
lifetimes, not all OS heaps or all character/stage lifetimes. Remaining owner/global
closure and complete copy-only ring forward/corrected replay precede production
replacement. Production remains page tracking; diagnostic replay remains component
serialization plus drawing. Next: integrate new history frames against both retained
oracles and batch-report missing owners before recovery. Simulation/render separation
and full acceptance follow cutover. The full goal remains active. Details:
melee/docs/native_match_state_arena_plan.md.


2026-10-12 checkpoint: copy-only replay history ring

NativeMatchHistory now prebinds two arena spans in each of16slots and directly
copies them on save/load. The old component-directory hot path and fake-world
history fixture were removed. Binding checks aliases/capacities at construction;
restore checks all private arena lifetimes before either write. Actual copied
byte totals are recorded during save. Sequential publication, slot wrap, stale
leases, future invalidation, counted-prefix/allocator rewind and failure poisoning
are tested using real arenas. The census retains packed component data once per
requested target for comparison, and logs two-span copies honestly. Periodic
component validation remains diagnostic, outside the ring hot path.

PDB attribution exposed fighter collision log state and item tracking. The small
fighter damage-log arrays/counters/flags are consolidated into FighterCollisionState
with direct call-site updates; the complete arrays are captured because Fighter
fields can retain references after append counters reset. Item fighter-position/
ECB tracking, damage logs/counts, contact flags and birth/category counters are
captured too. Gameplay rules were not changed. Rendering caches and animation
traversal stacks remain an explicit ownership audit, not presumed exclusions.

copy-ring-oct12-item-correction/receipt.json and -item-green/receipt.json pass stock
and interpreted eight-fighter480sample cohorts through the NEW ring: Final
Destination fresh/repeated corrected input240 (512captures/3restores per case),
Green Greens two15tick intervals0/120 (511captures/4restores). Every39component
endpoint, continuing trace and resimulation broad endpoint matches; raw catch-up
differences are zero. Focused real-history/arena/heap/GObj tests pass, and owner
fixture copy-ring-oct12-owners passes88702checks. Independent final verification:
copy-ring-oct12-item-correction-verification.json. Executable SHA256:
36ed1a33f3a4b5c0a87af91e5bac34f2eb93bd13792abf300fd3b40ed99ca5b9.

Copied peaks: FD2833675stock/3210907scriptedbytes; Green3250555stock/3527563scripted.
Backing reserves all admitted capacities for16slots (~1.306GBstock/~1.349GBscripted),
not the copied state size. This memory cost needs improvement before shipping;
existing actor capacities were not reduced for easier tests. The earlier
uncontended cold ring run measured scripted restore281.5/301.9us and15saves
4.765/5.173ms (318/345us each), with full rendered interval28.7/29.1ms and drawing
14.3-14.6ms. These real ring timings supersede isolated warm probes; sub100us
save/load and15simulation-only ticks/16.67ms remain unproven. Final concurrent
cohorts establish fidelity, not timing acceptance.

Production netplay remains broad tracking. Before replacement: complete residual
owner/global audit, broader fighter/stage/lifetime and scripted-content coverage,
teardown checks, memory/traffic optimization and simulation/render separation.
Do not silently exclude residuals merely because endpoints match. New copy ring
integration is real, but full production cutover and project acceptance remain
unfinished. The goal stays active. Details: melee/docs/native_match_state_arena_plan.md.


## Private fighter traversal scratch (2026-10-12)

The three original ftAnim ancestor arrays are now caller-local stacks. Every
walk starts at depth zero and writes each occupied entry before reading it.
All real call sites pass their own array directly, including metal/part/Kirby
walks; no compatibility entry point or new gameplay rule was introduced.
This removes 720 bytes of traversal scratch from persistent globals and makes
interleaved/nested traversals independent instead of adding scratch to history.

Evidence in A:/FreakMelee-test-archives/: animation-tree-oct12-ctest.log passes
history, arena, heap, GObj and the actual three-type traversal fixture (129
checks, dirty scratch, interleaved walks, supported maximum depth).
animation-stack-oct12-correction/receipt.json and animation-stack-oct12-green/
receipt.json pass stock/interpreted eight-fighter 480-sample comparisons,
15-tick same/corrected-input history rewinds, all 39 component endpoints and
continuing traces with zero broad endpoint differences after catch-up.
Independent pins are in animation-stack-oct12-correction-verification.json;
executable SHA256 4f5c2bf52cf9527800621722051059bb1872246625971d967e12656c7149dec2.
These concurrent cohorts are fidelity evidence, not deadline measurements.

The old native_engine_fighter_pose/native_model_graph fixtures were also tried;
they fail on earlier NativeSourcePackage resolver-layout drift, not traversal
signatures. Their obsolete ancestor globals were removed, but this checkpoint
does not claim those broader fixtures pass. Actual game compilation and live
replay comparisons pass; fixture refresh remains separate follow-up work.

Pool audit: pool-capacities-oct12.txt records actual native sizeof values.
Largest fixed payloads are item 4096 x 6416 = 26,279,936 bytes and optional
Green grid 4096 x 2088 = 8,552,448 bytes. Core actor capacities were native
choices, not retail object-count limits. Original HSD ObjAlloc grows from the
heap with count-limit flags initially disabled; stock item spawn categories
have separate ItCo data thresholds, recorded from the USA 1.02 ISO in
retail-item-limits-oct12.json. No global item cap is implied by those thresholds.

User approved reducing fixed capacities (fighter 16; item 512 or 1024; broadly
4-8 times smaller), with measured stock demand/headroom checked first. Next
measure actual pool peaks and hot/cold snapshot copy cost before choosing the
remaining limits. The conservative 1.35 GB sixteen-frame backing is not a
retail requirement. Production rollback still uses page tracking; no claim of
complete owner closure or production cutover follows from this checkpoint.


## Match pool budgets and copy cost (2026-10-12)

User authorized substantially smaller fixed pools. Native capacities now are:
fighter 16 (was 64); Ground 512 (4096); Item 1024 (4096); ItemLink 1024 (4096);
GObj 1024 (4096); GObjProc 4096 (16384); AObj 2048 (4096); FObj 4096 (16384).
Script actor capacities are fighter 16, item 128, effect 128 (64/512/512).
Per-actor authorable script state capacity remains 8192 bytes. Optional Green
grid follows Ground's 512 slots, not the shared GObj ceiling. Scene arena is
2 MiB (8); ordinary HSD arena is 4 MiB (16). Generic pool validation ceiling
and per-asset measured model reservation demands are not preallocated arrays
and were not blindly divided. No runtime backing allocation was introduced.

Added construction/periodic actual pool reporting to native_snapshot_census.
Baseline eight-fighter Green Greens peaks: fighter 8, Ground 26, Item 23,
GObj 130, GObjProc 470, AObj 786, FObj 2482. Consequently controllers were
cut twofold, not fourfold; an eightfold channel reduction would already fail
the measured stock workload. These observations are not all-stage/all-fighter
maxima and cannot prove arbitrary custom content fits the new budgets.

The quiet late rewind exposed missing ifMagnify_804A1DE0 state. Its flags are
written by drawing, then consumed by Fighter's offscreen damage/bottom camera
logic and player statistics. The failed first cohort had two differing player
bytes (PDB attribution in pool-budget-oct12-residuals.json), not arena/pool
exhaustion. The real complete magnifier record is now a direct arena root.
The 39-component diagnostic format remains unchanged; the broad independent
oracle verifies this additional root. No gameplay rule or visual flag was masked.
This is a concrete counterexample to treating all HUD state as removable.

Final evidence in A:/FreakMelee-test-archives/:
- pool-budget-oct12-final-correction/receipt.json: stock/interpreted eight fighters,
  480 samples, fresh/repeated input correction and exact continuing traces.
- pool-budget-oct12-final-green/receipt.json: 600 samples, 15-tick intervals from
  inputs 240/360, exact continuing traces, all 39 component endpoints and zero
  broad endpoint differences after catch-up. Corrections and this cohort ran
  concurrently and are fidelity tests, not performance deadlines.
- pool-budget-oct12-quiet-magnifier/receipt.json: same Green Greens intervals,
  per-frame trace/object observers disabled; endpoints and raw state still exact.
  This timing cohort ran separately, before the other tests/build work.
- Independent input/output/executable pin verification is in
  pool-budget-oct12-final-correction-verification.json. Current executable SHA256
  ebb1ea24c84d82e7e7fc5edc72472092d08e5ac8b45987c94542a91ee55db335.
- pool-budget-oct12-final-cross-capacity.json: baseline vs reduced capacity stock
  and interpreted reference gameplay traces match exactly for the first 480
  inputs (fighter/partner/action/damage/item/RNG categories actually present).
- pool-budget-oct12-ctest.log: seven history/arena/heap/pool/GObj/animation tests.
- pool-budget-oct12-owners/receipt.json: 88,702 actual allocator/owner checks.
  Initial failed quiet run is retained in pool-budget-oct12-quiet, not accepted.

Green Greens peak copied bytes are now 2,690,907 stock / 2,967,819 interpreted,
versus 3,250,555 / 3,527,563. Interpreted reduction is 559,744 bytes (15.87%).
History backing is 341,598,720 stock / 385,170,560 interpreted bytes, versus
1,305,784,832 / 1,349,356,672. It still reserves full admitted capacities for
all sixteen slots and requires further reduction before release. No claim that
the remaining 2.97 MB is exclusively gameplay state has been established.

Measured plain CRT memcpy on this i7-10700K (memcpy-ring-oct12.txt), copying
3,527,563 bytes after allocation/first touch, five batches of 512 copies:
one reused buffer save/load medians 128.6/128.2 us; compact sixteen-slot ring
125.2/311.0 us; original 84,334,792-byte slot spacing 135.4/331.0 us. Reading
cold history alone can therefore cost about 300 us; warm single-buffer numbers
are not the realistic restore baseline. Benchmark sources/commands are retained
in .local/memcpy-ring-oct12.*. It is not an in-game gameplay-cost measurement.

Actual reduced interpreted quiet ring restores were 273.2/301.7 us; fifteen
saves totaled 4.660/4.665 ms (about 311 us each). Source dispersion/cache effects
vs the plain hot-source benchmark remain to be attributed. Endpoint Save
explicitly refuses initialized page tracking, so these direct-history timings
are not page-fault tracking costs. Full fifteen rendered ticks were 30.94/32.14
ms, including 17.56/18.37 ms drawing and 7.64/7.93 ms simulation. Rendering
separation remains a major required step; neither sub-100-us save/load nor the
15 simulation-only ticks within 16.67 ms target is established.

Production netplay still uses page tracking. Remaining snapshot cutover work:
owner/scratch closure, broader character/stage/longer-lifetime and teardown
coverage, tighter history storage, and gameplay/render separation. Only the
proven 0.56 MB reduction is counted; further exclusions need source ownership
evidence plus unobserved and observed replay comparisons.


## Match-history save-path profiling - 2026-10-12

Completed user-requested save attribution, documented in
melee/docs/native_match_save_profile.md. Optional CMake
MELEE_PROFILE_MATCH_SAVE (default OFF) instruments history guards/slot selection,
both arena guards, prefix memcpy, root length/copy loops, publication, dispatch
and callsite. Detailed mode records every root separately; coarse mode avoids
hundreds of clock pairs. Host profiler state is outside captured sections.
Post-replay copy controls use preallocated scratch, outside replay timing.

Eight interpreted Ice Climbers actors, Green Greens, two 15-tick replay
intervals: history Save 290.253/297.300 us. HSD prefix 135.247/140.447 us;
HSD 62 roots 41.873/40.940; scene prefix 9.327/8.193;
scene 329 roots 102.400/106.200. Guards/publication/dispatch about 1 us.
393 memcpy calls include two prefixes, 368 nonempty roots and 23 empty roots.
No runtime allocation/hash/compression/graph validation/page tracker in Save.

Warm controlled identical-byte/destination experiments: packed 393 chunks
242.394/231.584 us versus single memcpy 123.927/115.662 us. Live gather
236.686/225.784 us. Walking lengths/offsets without data copies 0.213/0.212 us
(does not include frame-length stores). Standalone 393 zero-byte calls cost
0.756 us; two equal chunks already cost 258.214 us versus one 123.436 us.
This implicates copy-pattern/size-dependent memory behavior, not mere call or
span-walk overhead. Exact CRT/cache mechanism remains unproven. Do not add
per-tick gather staging on this evidence; it would add another copy.

Coarse/detailed/inactive-before/inactive-after cohorts all PASS stock and
interpreted eight actors, 600 inputs, two intervals, all 39 component endpoints,
broad raw endpoints (0 differing bytes) and audio reconciliation. Independently
verified source/executable/receipt/log pins before rebuilding. Detailed clocks
are intrusive (312-387 us totals), not the normal budget. Scheduling/cache
variance remains; no universal timing guarantee is claimed.

Artifacts A:/FreakMelee-test-archives/save-profile-oct12-{final-control-a,
coarse,detail,final-control-b}; independent verification in
save-profile-oct12-final-control-a-verification.json. Instrumented executable
786e6c0e19da3f19f1a9e2d475b6e89d3b912c5e7bd08156f1f49fb44593f5d8
retained in save-profile-oct12-binary. Standalone memcpy-chunks-oct12.txt.

Restored profiling-OFF release; six focused CTests PASS and repeated normal
two-interval stock/interpreted cohort PASS. Independent verification:
save-profile-oct12-normal-verification.json. Normal executable
12461b1ddd9eab559053573190066372597a80e948f1fc398ba810bcb1e6c396.

Goal remains active: production cutover, snapshot closure, broader lifetime
coverage, contiguous mutable ownership/tighter storage, and simulation without
rendering remain required. This checkpoint profiles Save; it does not claim
sub-100-us save/load or fifteen simulation-only ticks within 16.67 ms.


## Selected production policy: confirmed checkpoint - 2026-10-12

User chose a single confirmed game-state checkpoint in place of per-predicted-
frame history. All inputs through the checkpoint must be actual and contiguous;
later complete frames do not bridge holes. Sub-100-us save/restore is not a
requirement. Minimize costs, but prioritize proceeding to simulation without
rendering over further snapshot micro-optimization.

Detailed cutover design: melee/docs/native_confirmed_checkpoint_plan.md.
Checkpoint C is the pre-input state for frame C (actual inputs 0..C-1). Keep
bounded input history and the live predicted state. Coalesce incoming updates;
restore C, simulate to the newest reconstructible confirmed boundary D, save D
once if it advanced, then replay to the live boundary P without intermediate
saves. Corrections beyond an earlier hole can replay without saving. Actual
confirmation can require reconstructing D even if predictions matched; that
extra simulation is the intentionally accepted tradeoff. A live confirmed
boundary can be saved directly. No mirrored authoritative game world.

Bound prediction distance/input retention from the usable checkpoint, retain
audio/rumble intent reconciliation, and publish checksums from reconstructed
confirmed state. Preserve stable addresses, script state, allocation/reuse and
scene lifetime closure. Test holes, out-of-order inputs, batched confirmation,
correct/mismatching predictions, shared PCs, stalls and retirement against the
existing independent reference. Measure total reconciliation including extra
checkpoint advancement work and final rendering.

This entry records a design decision, not a production runtime cutover claim.
Production still uses page tracking. The sixteen-slot copy-only ring and its
per-frame profiling remain diagnostic evidence. Finish minimum checkpoint
closure/cutover, then prioritize simulation/render separation. Sparse predicted
snapshots and additional copy tuning may be revisited only if measurements
warrant them. The previous sub-100-us aspiration is superseded by this decision.


## Confirmed checkpoint controller and network dependency cut - 2026-10-12

Implemented native_confirmed_checkpoint.[ch]: one prebound capture across two
real arenas, stable-address restores, scene/frame lifetime checks, contiguous
confirmed boundary reconstruction, one capture at the reconstruction frontier,
unsaved predicted replay ticks and a fifteen-tick prediction bound. Checkpoint
and input history remain host control state. An actual-input gap cannot be
bridged by a later complete packet; missing prepared replay inputs are refused.
Matching-prediction historical confirmation also reconstructs, while a fully
confirmed live frontier can capture directly. Tick functions do not allocate,
enumerate game graphs, hash or reconstruct allocators.

Packet/UDP/room reception now takes NativeRollbackInputHistory directly, with
all call sites updated. No wrapper/compatibility signatures were added. Peer
identity, ownership, transport lifetime checks and transactional batch behavior
remain in their owning layers. This removes network receipt's dependency on the
page-tracking session, preparing direct cutover.

Controller fixture passes 9,609 checks against independent all-actual-input
reference states: synthetic eight actors, counted satellite roots, local masks
0/1/5/15, out-of-order delayed arrivals, holes, corrections, matching predictions,
batched confirmation, direct live frontier, repeated input-ring wraps, stalls
and scene invalidation. Eight CTests PASS including real local two-peer UDP
exchange, packet atomicity, replay, existing page session, arena and history.
Release build PASSES. Repaired the replay fixture's preexisting missing
nativeReplay_StateObserversEnabled stub; no replay gameplay behavior changed.
Evidence/source/binary hashes: A:/FreakMelee-test-archives/
confirmed-checkpoint-oct12-core-receipt.json and confirmed-checkpoint-oct12-tests.log.

These are controller/storage and local transport checks, not live Melee or real
script-VM checkpoint parity proof. Production still uses page tracking. Continue
with direct session cutover, transition-time owner admission, audio/rumble intent
reconciliation, reconstructed confirmed checksums, input retention/stalls and
live stock/interpreted replay and scene/teardown comparisons. Then remove the
obsolete production page tracker and prioritize simulation without rendering.
No new timing claim or broad completion claim is made.


## Confirmed checkpoint production cutover and open particle closure - 2026-10-12

Production netplay now uses native_confirmed_checkpoint directly. Old page
tracking remains only in diagnostic session/probe/benchmark use. Scene-time
ownership admission is shared with the census via native_match_state_admission.
Input retention follows the saved boundary, protecting required slots against
future packet overwrite. Replay-input rewind, audio/rumble intents, confirmed
checksums and stalls are integrated. Normal and early exit branches complete
the network frame; predicted match finishes wait for actual inputs and can
restore the match when corrected. No normal per-tick graph validation was added.

checkpoint-cutover-oct12-live-c PASSES local hidden/muted stock-zero, stock-delay,
interpreted-delay cases, 600 frames, eight Ice Climbers actors, Green Greens,
items and shared-PC masks 5/A. Endpoints and RNG match within each peer pair.
Stock delayed runs match 2,136 selected settled actor/item/RNG records each
against the separate zero-delay run. Independent source/binary/log verification:
checkpoint-cutover-oct12-live-c-verification.json. This accepted batch precedes
later diagnostics and early-exit changes; final-source repetition remains due.

Longer one-stock Final Destination tests FAIL: matching-input checkpoint replay
can change RNG/gameplay state (boundary 822; another case 2577 during GAME).
Do not claim the cutover fully validated. RNG tracing isolates particle emission:
original/replay 817 starts at the same seed and makes 79 calls; 818 starts at the
same seed but makes 119 versus 69, first differing in generator/particle code.
Generator byte trace at 817 matches; at 818 the processed list has 16 versus 11
entries with different/missing one-shot emitters. Exact cause remains open.
Investigate creation/command processing, allocator/list ownership and retained
definitions before changing game behavior or masking RNG. Evidence:
checkpoint-cutover-oct12-match-end-{g,h}; scripts in .local compare RNG and
generator records. All these processes are terminal; no live handle is retained.

Optional FREAK_MELEE_NETPLAY_CHECKSUM_TRACE diagnostics retain portable
controller values, matching-input replay hashes and RNG callers. Optional owner
probe allocates two 64-MiB scratch buffers only when explicitly enabled; normal
netplay does not. The old serializer refuses a late finished-controller capture,
logged as diagnostic coverage rather than required hot-path validation.

Latest release builds and six focused tests PASS in
checkpoint-cutover-oct12-unit-tests.log. Remaining: fix particle replay closure,
repeat final-source live and broad oracle comparisons including real scripted
fighters, stages, transitions and match ends, then prioritize simulation without
rendering. No performance acceptance or complete snapshot closure is claimed.


## Confirmed checkpoint HUD closure and phase renewal - 2026-10-12

Production uses one confirmed checkpoint over admitted mutable spans, without
page tracking or intermediate predicted/resimulation saves. The late particle
failure was an object-lifecycle mismatch: private timer state was not restored,
so replay skipped freeing the countdown. Timer/countdown, stock-indicator
objects, stock counters/effect queues, and retained score-predicate state now
belong to the existing versus-control owner at their actual storage addresses.
No RNG masking or replacement gameplay rules were introduced.

The new scene history epoch is independent of actor epochs. Sudden death retains
live objects and transport, clears/rebuilds admission and invalidates old spans
without resetting payload. Lighting amnesia invalidates retired active-light
cache pointers before construction validation. Artificial delay flushes its
last withheld inputs at match end. An explicit phase-closed marker and final
log scan prevent the test runner from accepting an endpoint followed by a failed
transition; earlier scripted-end receipts are not sudden-death acceptance.

Final-source checkpoint-hud-oct12-final-c PASS: original/restore intervals at
813 and 2570 match all 39 owners and the broad memory image with ZERO differing
bytes; 600-frame stock-zero, stock-delay and interpreted-delay local peers match
endpoints/RNG; each delayed stock peer matches 2,136 selected settled records
against the separate zero-delay reference. Stock full-match endpoint 2687 is
58a69947afd7e6ff / e746cc8d on both clients.

The separately pinned interpreted eight-actor match reaches 3838 with matching
7dacb886e4392d6c / a4a156d6, then both peers bind a second checkpoint and advance
beyond 160 sudden-death ticks. Settled frame-120 phase digest/RNG matches:
a560f0ab07fef73b / a13a037c. The test stops there; complete sudden-death results
and distributed-PC acceptance are not claimed. All launches are hidden/muted;
owned processes are terminal. Reused scripted case: checkpoint-hud-oct12-scripted-end-d.

Independent source/binary/log verification: checkpoint-hud-oct12-final-c-verification.json
and checkpoint-hud-oct12-live-c-verification.json under A:/FreakMelee-test-archives.
Green Greens/items copied peaks: 2,570,103 stock and 2,940,591 interpreted bytes.
HUD/ownership fixture 88,737 checks, match-loop fixture, real GObj lifecycle
fixture 117,875 checks and six focused final-source CTests pass. Bounded optional
particle/emitter/object traces and construction admission failures are retained
for diagnosis; normal netplay reserves no trace payload.

Next: simulation/render separation and profiling. Rendering still executes on
resimulated ticks. Broader fighter/stage/lifetime cases remain bounded evidence,
not universal closure. No new save-time or fifteen-tick performance claim.
Goal remains active; no upstream push.


### 2026-10-12: opt-in CPU-only GX reconstruction

Confirmed-checkpoint production scheduling remains in place. The next render
optimization is implemented behind FREAK_MELEE_NO_GPU_RESIM=1: HSD callbacks,
RNG and matrix setup still run, while model primitives and Aurora GPU frame
allocation/uploads/passes/submission are omitted during reconstruction. Normal
frames resume output. The default is unchanged pending broader coverage.

Stock/interpreted eight-actor Green Greens/items same-input intervals and Final
Destination corrected-input intervals pass all 39 owners and per-frame traces.
Broad raw differences are independently resolved to 104-121 bytes of particle
render scratch, with no other differing bytes in the tested intervals. Actual
local UDP zero/delayed matches pass; delayed stock settled traces match the
independent zero-delay run. Scripted match end at 3838 and renewed sudden-death
checkpoint/settled frame 120 also match. Full sudden-death completion is not
claimed. All 206 GX tests and six focused checkpoint/input/session tests pass.

Sequential quiet controls measure about 19-20 ms drawing per 15 ticks, versus
about 8 ms with CPU-only GX. Whole diagnostic intervals still include fifteen
history saves; production uses one confirmed checkpoint. No sustained 16.67-ms
deadline claim. Next: broaden local fighter/stage coverage, enable the verified
production policy, and cut remaining HSD/particle CPU work without discarding
state-affecting callbacks. Full goal remains active. Details and pinned evidence
in melee/docs/native_cpu_only_resimulation.md and native_compact_snapshot_timing.md.
Aurora local commit: 883ddb3a386efa04615d0f87eace1741ea5375fc. No upstream push.


### 2026-10-12: default GPU-free netplay reconstruction

Netplay now defaults to CPU-only GX for reconstructed ticks. Full HSD callbacks
and matrix preparation remain because the original draw path mutates gameplay
and particle state; visible frames still render. Offline raw-memory probes keep
their rendered oracle unless explicitly opted in. CPU-only GX uses the same
decoder on the simulation lane, with a drained/locked ownership handoff to the
existing visible-frame worker. No extra worker or job system was introduced.

Final-source default-policy local UDP stock/scripted cases pass matching peer
endpoints and settled stock comparisons to a zero-delay reference. All hidden
ticks use CPU-only GX. Additional stock/scripted Fountain of Dreams and late
Brinstar intervals pass owner/trace comparisons; final Brinstar raw differences
resolve entirely to particle-display scratch, including its reset-before-use
TEV cache. All 207 GX tests and six focused native tests pass. The temporary
end profiler was removed after locating the approximately 247 KB per-tick
register stream and 152-155 us CPU decoding cost. Synchronous decoding has only
a modest measured benefit; do not claim a large performance breakthrough.

Next: move required fighter/pose/particle state updates into the simulation
phase, then omit unnecessary material/graphics preparation during resimulation.
Keep the full goal active; broader fighters/stages, sustained frame budget and
other remaining plan acceptance are not declared complete by this checkpoint.
Evidence and scope: melee/docs/native_cpu_only_resimulation.md. Local commits
only; no upstream push.


### 2026-10-12: Model pose updates separated from skinning uploads

Reconstruction now prepares shared/envelope joint dependencies and matrix marks without blended skin matrices, inverse-transpose normals or GX model matrix uploads. Visible rendering is unchanged. Stock/interpreted eight-actor Green Greens and late Brinstar, corrected-input replay, and default-policy local UDP tests pass; raw differences remain confined to identified particle-display scratch. Evidence: melee/docs/native_cpu_only_resimulation.md and A:/FreakMelee-test-archives/render-pose-oct12-*. Median remaining drawing cost is 5.211/5.407 ms per fifteen ticks in the quiet cohort; no sustained deadline claim. Material/texture/light and particle state separation remain active work.


### 2026-10-12: Material graphics work removed from reconstruction

Retained texture/light/camera and shader descriptor updates are separated from TEV and texture-coordinate graphics commands. Stock/interpreted Green Greens, Brinstar, Fountain, corrected-input and default-policy local UDP cohorts pass their owner/traces. Independently classified Fountain raw differences include inactive future Vec backing/directory rows, not live-state mismatch; see melee/docs/native_cpu_only_resimulation.md. Current median draw cost is 3.563/3.516 ms per fifteen ticks; complete production deadline is still unproven. New evidence lives on C:/FreakMelee-test-archives/render-material-oct12-* to avoid filling A:. Particle preparation/separation remains active.


### 2026-10-12: particle geometry omitted while effect state is reconstructed

Particle display is split into orchestration, geometry, and shared attachment
pose preparation. GPU-free replay omits particle geometry while preserving
batch traversal, particle sorting, attachment state, shader-kind normalization,
effect spawn callbacks, particle updates/lifetimes and RNG ordering. Frame-local
drawing matrices/colors/shader caches are excluded from match snapshots; active
particle/generator state, list roots and allocator payloads/free lists remain
restored. Discarded predicted spawns disappear through state restoration, and
corrected replay rebuilds the right effects at their correct age.

Evidence under C:/FreakMelee-test-archives/: render-particle-oct12-green-a and
brinstar-a each pass three 15-tick intervals for stock/interpreted eight-actor
matches (117 owner comparisons per case, matching independent rendered traces,
zero older-boundary raw differences). correction-a passes fresh/repeated changed
inputs, continuation and audio (39 owners per case, zero raw differences).
render-particle-oct12-raw-verification.json independently verifies hashes, raw
results, frame traces and nonzero omitted particle batches. live-a passes local
UDP stock zero/delay and interpreted delay, both delayed stock peers matching
2,136 settled records; default CPU-only policy and retained endpoint digest/RNG
are checked in live-a-verification.json. Six focused native tests pass.

Quiet timing drawing medians are 3.242/4.167 ms per 15 ticks stock/interpreted;
HSD 2.480/3.386 and GX end 0.672/0.686 ms. Interpreted simulation/history also
increased versus the preceding cohort: scheduling/cache load is uncontrolled,
so no universal speedup or sustained deadline is claimed. Production checkpoints
omit the fifteen diagnostic saves included here. Remaining camera/texture and
display setup still runs. Dedicated wavedash-smoke visual observation, all-stage
coverage, distributed-PC testing and production budget measurement remain open.
The full project goal remains active.


### 2026-10-12: pixel/channel/light and particle setup command omission

GPU-free reconstruction omits HSD pixel/channel/TEV commands and light uploads
while retaining cache/counter/dirty-flag updates and CPU light payloads. Fog
submission and particle billboard/alpha/vertex/coordinate setup are omitted;
screen erase retains depth-texture identity and HSD invalidation. Shared particle
attachment/camera updates, list traversal, shader-kind normalization, texture
identities and all effect state updates remain. Normal visible rendering keeps
its commands. Direct callback/state/resource preparation still remains work.

C:/FreakMelee-test-archives/render-state-oct12-green-a, brinstar-a and correction-a
pass stock/interpreted eight-actor owner/frame comparisons, corrected-input
continuation/audio and zero older-boundary raw differences. The independent
raw-verification.json verifies hashes/results/counters. live-a passes local UDP
stock zero/delay and interpreted delay; both delayed stock peers match 2,136
settled reference records, and all CPU-only replay counts match checkpoint stats.
Six focused native tests pass; no all-stage or distributed-PC acceptance claim.

Quiet draw medians are 2.853/2.782 ms per fifteen ticks stock/interpreted (HSD
2.380/2.317, GX end 0.400/0.376). Whole diagnostic medians are 10.966/13.661 ms,
including fifteen saves (2.722/3.330). These short runs have uncontrolled load;
subtraction of saves is not a production checkpoint benchmark. Actual checkpoint
advance plus corrected suffix and final visible frame is the next budget check.
Evidence: melee/docs/native_cpu_only_resimulation.md and
melee/docs/native_compact_snapshot_timing.md. Full project goal remains active.


### 2026-10-12: full production-cycle profile and historical input pacing

Optional fixed-buffer netplay profiling now measures visible submission to the
next visible submission, including checkpoint work, reconstructed ticks and
one normal rendered tick. It records explicit input/network sleeps separately,
does not allocate per tick, and prints after measurement. The full-window test
exposed a leftover six-frame input-exchange guard and insufficient sixteen-slot
network histories. Prediction now shares the existing fifteen-tick checkpoint
limit; artificial delay caps at fourteen. Small fixed input/outbound/checksum
histories retain 64 frames, while gameplay still keeps one checkpoint.

The first completed profile exposed historical ticks sleeping on live 60-Hz
sampling deadlines (roughly 256 ms full cycles, mostly explicit waits). Netplay
resimulation now polls the saved schedule's next sample immediately, preserving
raw queue/filter/channel updates and ordinary wall-clock sampling in live play.
Input tests cover fifteen scheduled samples, the next live deadline, retained
confirmations and actual ring expiration. Seven focused tests pass.

Final-source C:/FreakMelee-test-archives/netplay-timing-oct12-live-d passes two
local muted/hidden clients, 600 samples per peer, stock zero/delay and interpreted
delay. Default CPU-only replay counts, endpoints/RNG, checksums, source/binary/
log hashes and all 480 post-warmup timing rows per peer are independently checked
in its verification report. No all-fighter/stage or distributed-PC acceptance.

There are 121 interpreted cycles with fifteen TOTAL hidden ticks and one normal
rendered tick; each includes two reconstructions, two restores and two captures.
Median full elapsed is 16.884 ms, per-cycle elapsed minus explicit sleeps 16.572
ms; p90/p99 18.841/25.452 ms. Simulation median 7.125 ms, drawing 7.421, netplay
1.602 including checkpoint 1.250. Fifty-seven cycles exceed 16.667 ms even after
sleep subtraction. These are near-budget measurements, not sustained acceptance.
Stock packet timing creates common totals of 21/23/25 hidden ticks per visible
cycle; each reconstruction remains bounded but several can occur before drawing.
Remaining targets include final visible rendering and repeated reconstruction/
checkpoint work. Full goal remains active; detailed evidence and scope are in
melee/docs/native_cpu_only_resimulation.md and native_compact_snapshot_timing.md.


## 2026-10-12 - Final-frame UI cost and tighter rollback budget

Added optional caller-owned Aurora frame-end profiling, fixed 1,024-row storage,
clocks only when AURORA_PROFILE_FRAME_END=1, shutdown reporting. Separates FIFO,
texture maintenance, finish, ImGui, RmlUI update/recording and worker submission;
CPU-only reconstructed frames do not record UI. No GPU-fence/scanout claim.

The profile identified original-CSS and costume visibility properties assigned
unchanged every frame, forcing RmlUI document relayout. Direct call sites now
apply them only when displayed visibility changes. Context update/render remain
live. In equivalent two-local-client muted/hidden interpreted eight-Ice-Climbers
Green Greens/items cohorts, post-warmup UI update median falls 2.629 -> 0.263 ms,
overlay 2.868 -> 0.412 ms, frame-end 3.698 -> 1.225 ms. Verified final-source
frame-end-oct12-live-c retains endpoint 6701a8ecfe2a64a2 / RNG 9e464987, confirmed
checksums, every CPU-only replay count, 600 frame-end and 480 production-cycle
rows per peer and source/binary/log pins. Exact tested binaries/PDBs/sources
retained for baseline B and corrected C. Seven focused native tests pass.

After correction, eighty full cycles with fifteen TOTAL hidden ticks and one
normal rendered tick have active-wall median 14.915 ms, elapsed median 15.235,
active p90 17.722 and maximum 25.476. Nineteen exceed 16.667 ms. This is not
sustained acceptance. UI smoke captures show CSS/costume display, but injected
selection did not enter practice; no complete UI transition acceptance claimed.

User now requests 10 ms median and <=12 ms measured peak for the full fifteen-
reconstructed-tick + normal-visible-tick active path. Retain elapsed timing;
exclude intentional pacing sleeps, include checkpoint and render submission.
Current work does NOT meet this target. Next profile simulation and retained
state-changing display callbacks. Two clients share one machine/GPU; packet
arrival changes cycle populations. Peaks need a stated workload/sample window,
not an absolute OS scheduling guarantee. Full project goal remains active.


## 2026-10-12 - Callback attribution and CPU scalar access

Optional FREAK_MELEE_PROFILE_CALLBACKS=1 records caller-owned process/display/
camera callback totals in a fixed 512-bucket table, outside snapshots; warmup
frame120, visible/hidden lanes, post-measurement RVA reporting and offline PDB
symbols. Camera timings include child display work; never add them together.
Stock/script local UDP diagnostic cohorts pass endpoints/checksums. CPU decision
callback hidden mean is 4.71 us stock vs20.18 us scripted; animation/collision
and inclusive camera costs are much closer. These are independent populations
with intrusive callback clocks, not direct performance acceptance.

CPU scalar fields now admit live owned pool/program/storage source once instead
of constructing/rechecking a full continuation snapshot after each direct access.
No callback occurs in those field operations. Intrinsic kernels and target-ID
publication retain lifetime/target guards. Command/scalar/selector/VM fixture
passes7269 checks,612 command pairs,80 selector pairs;7 focused native tests pass.
Broader connected CPU fixture currently fails to link against newer scene/arena/
metadata dependencies; older generated-C CPU/follower fixtures fail compilation/
generation. Failed logs retained; no full connected/CPU acceptance claimed.

Final phase-only cpu-scalar-oct12-live-a completes600 samples/peer with endpoint
6701a8ecfe2a64a2 / RNG9e464987,confirmed checksums and CPU-only replay accounting.
Source/binary/log and phase/frame rows verified; tested EXE/PDB/sources retained.
Only25 exactly-fifteen cycles occurred; below earlier32 minimum. Verification
explicitly waives population eligibility for correctness, not performance.
Indicative active-wall median14.396ms,p90 16.435,max19.746;2 cycles exceed16.667.
Still misses user's10ms median/12ms measured-peak target. More substantial
generic binding/context and simulation/render-state work remains. Goal ACTIVE.


## 2026-10-13 - Linear registry walks and full-depth production sample

CPU planner admission walked each physical Fighter/Item list, then searched that
same list again for every node through nativeCallbackSource_Snapshot. Replaced
the redundant membership search with direct pool/epoch/class/non-null-payload/
backlink checks in the existing walk. Selected targets still retain lifetime
guards across kernels. Generic fighter item selection, article links and signal
recipient lookup now read IDs directly from visited live-list nodes; ownership,
program, removal and selected-target checks remain. No new language primitive.

Final command/scalar/selector fixture passes7333 checks,612 command pairs,80
selector pairs,including actual pre/post registry admission with a controlled
no-op leaf. Null payloads,wrong classes,bad backlinks,freed slots and cycles
reject before kernel entry; payload retirement after entry rejects publication.
Seven focused core tests pass. This is admission,not CPU planner behavior proof.

registry-linear-oct12-live-a passes complete stock zero/delay and interpreted
delay local UDP600-sample cohorts,all source/binary/log pins,480 timing rows/peer,
CPU-only replay counts,confirmed checksums and endpoints/RNG. Verified report
retained. Final EXE/PDB/changed sources under registry-linear-oct13-final-binary.
472 interpreted exactly-fifteen cycles each have ONE restore and ONE capture,
plus the normal rendered tick: active-wall median12.698ms,p90 13.471,p99 15.575,
maximum17.258; elapsed median16.660 including normal pacing. Stock480 such cycles
active median10.475,p90 11.229,p99 12.510,max13.644. Two clients sharePC/GPU;
previous populations often had two reconstructions,so no isolated speedup claim.
Both still miss requested10ms median/12ms measured-peak target. Script simulation
and final rendering remain the main targets. Full project goal stays ACTIVE.

An invocation-local metadata cache passed controlled retirement/replacement
cases but full-path speedup was inconclusive. Removed the cache,VM/context
changes and associated experimental tests; final code keeps the simple linear
walks. Historical experiments are not final-source performance acceptance.


### 2026-10-13 - Audio decoder batching; full-cycle target still open

- External owned-main-thread sampling found voice decoding/resampling among native leaves. Audio polling fills a wall-clock playback queue; it does not mix one frame for every resim tick. Sampling suspensions perturb scheduling and are diagnostic only.
- Batched the resampler's source reads into fixed stack buffers, consuming exactly the requested source interval. No skipped audio state/callback work, read-ahead, runtime allocation, threading or snapshot-format changes.
- 2,240 old/new complete voice-state and PCM comparisons plus analytic polyphase checks pass; all 17 selected audio/load/input/checkpoint/network tests pass. Eight alternating standalone QPC rounds measure about 1.7x faster resampling (2.030-2.049 us vs 1.180-1.188 us per 160-output-sample call); not a full-cycle gain claim.
- Both audio-batch-oct13-live-a/b complete stock-zero, stock-delay and interpreted-delay local UDP runs, 600 samples/peer, with pinned inputs, confirmed checksums, CPU-only counts and unchanged stock/script endpoints/RNG. Interpreted exactly-fifteen populations are zero/eight: normal minimum-32 verification rejects both. Explicit minimum-zero reports verify correctness only.
- Stock 480-cycle medians are 10.698/10.517 ms, peaks 14.112/14.540 ms. The prior qualifying interpreted 472-cycle result (12.698-ms median, 17.258-ms peak) belongs to the previous binary. The 10-ms median / <=12-ms measured-peak full path remains unmet. No completion, sustained deadline or distributed-PC acceptance claimed.
- Exact binary/PDB, changed sources/tests/docs, benchmark tools/output and test/verification records retained at C:/FreakMelee-test-archives/audio-batch-oct13-final. Goal remains active; local commit only.


### 2026-10-13 - CPU admission deduplication; next lead is repeated reconstruction scheduling

- Callback-free CPU scalar reads/writes now rely on their complete source-layout admission plus pool/scene/class checks, avoiding a duplicate lifetime capture. The generic layout proof explicitly validates the selected bank's exact parameter object. No invocation cache, context ABI or script API change.
- Initial CPU native-kernel admission stores the validated identities without a second immediate continuation capture/recheck. Post-kernel source/context, pool/scene, payload, fighter kind/data, selected program, storage/schema/parameter checks remain. Retained peer lifetime and registry guards are unchanged.
- scalar-admission-oct13-fixture-d passes 7,728 controlled checks, 612 queue pairs and 80 selector pairs. Added 27 invalid entry and 12 callback-retirement cases deny mutation/publication. One initially incorrect target-identity rejection expectation was corrected; broader connected fixture linkage is still unresolved and no full planner-parity claim is made. Seven core tests pass.
- An isolated before/after property-read benchmark using actual routing but controlled actor views measures approximately 72 vs 43 ns median over 32 timing windows/version, 200,000 reads/window. This is not full-match cost.
- scalar-admission-oct13-live-a completes stock-zero/delay and interpreted-delay local UDP runs with pinned sources/binary/logs, confirmed checksums, CPU-only counts and unchanged endpoint/RNG. Only three interpreted exactly-fifteen cycles: normal minimum-32 performance verifier rejects; minimum-zero report verifies correctness only. The 10-ms median / <=12-ms peak target remains open.
- Source-backed next lead: after CaughtUp the match loop calls BeginFrame, polls/reconciles new packets, and can initiate another reconstruction before rendering. Investigate scheduling/coalescing while preserving confirmation, input retention, prediction bounds and match-end correctness. No scheduling change implemented in this commit.
- Binary/PDB, changed source/tests/docs, exact fixture/benchmark tools and evidence retained at C:/FreakMelee-test-archives/scalar-admission-oct13-final. Goal remains active. Local commits only; no push.


### 2026-10-13 - Normal tick after catch-up; stable 15-tick measurement populations

- After a non-ending reconstruction catches up, mark a normal tick pending. BeginFrame uses the already reconciled input history rather than receiving/reconciling a second batch before that tick. The normal input filter still receives and EndFrame still reconciles. A full prediction window still waits/receives/reconciles; no hard resim cap or ignored corrections. Init/Shutdown clear the host-only flag; snapshot sizes unchanged.
- frame-cadence-oct13-fixture-b passes 534 checks using exact extracted production Begin/End bodies/state definitions with actual checkpoint/input/arena/audio-journal code. Controlled transport/clock/gameplay cover queued arrivals, normal sampling receive, propagated predictions, window exhaustion/holes, ending plus reconstruction, failures/timeouts and retired arena. No full-game/sudden-death/performance claim from the fixture. Seven core tests pass.
- Both frame-cadence-oct13-live-a/b complete stock-zero/delay and interpreted-delay 600-sample local UDP cohorts. Ordinary min-32 source/binary/log/timing/counter/endpoint verifiers pass. Each interpreted cohort has 480 exactly-fifteen cycles, all with one restore and one capture; endpoints/RNG and confirmed peer checksums are unchanged.
- Interpreted active wall medians 12.658/12.350 ms, p90 13.732/13.252 ms, p99 15.265/14.391 ms, maxima 16.470/18.256 ms. Simulation medians 6.797/6.733 ms, drawing 4.578/4.470 ms, checkpoint 0.544/0.498 ms (included in netplay). Do not add independent medians or discard the slower peak. Prior sparse populations do not isolate total gain.
- The 10-ms median / <=12-ms measured-peak target remains unmet. Two clients share one CPU/GPU; Green Greens, medium items, eight interpreted Ice Climbers/client, 14-frame artificial outbound delay. Explicit pacing sleeps excluded from active wall; OS/worker/GPU waits included. Render submission is not GPU completion. No all-fighter/stage/distributed-PC acceptance.
- Exact binary/PDB, changed sources/tests/docs, fixture/core evidence and verification records retained at C:/FreakMelee-test-archives/frame-cadence-oct13-final. Continue profiling simulation and retained HSD/final-frame drawing cost; goal active. Local commit only, no push.


### 2026-10-13 - Native ThinLTO, dead console math cleanup and exact snapshot endpoint checks

- Fresh callback/frame-end profiling still identifies CPU decisions and retained HSD drawing as large costs. Camera timings include display children; extra clocks perturb diagnostic timings. No camera/specular work was removed based only on this profile.
- Release/RelWithDebInfo now enable Clang cross-module optimization for game/runtime/client targets after C/C++ IPO support checks. Debug and external Aurora/dependency targets retain their configuration. Existing strict floating-point/float-cast settings and lifetime guards remain.
- Initial ThinLTO linking exposed unused lbtrigf.c standard-math definitions colliding with Windows imports plus an unused MSL infinity reference. Baseline executable imports acosf/asinf/atan2f/atanf from Windows. Removed that dead console-only source and its otherwise unused empty header; preserved current native math calls. Failed link log and before/after imports retained.
- All 17 selected audio/load/input/checkpoint/network tests pass. Both ipo-oct13-live-a/b finish the full stock-zero/delay and interpreted-delay UDP cohort with unchanged endpoints/RNG, confirmed checksums and snapshot peak sizes (2,570,103 stock / 2,940,591 interpreted bytes). Ordinary provenance/timing gates pass; each interpreted cohort has 480 exactly-fifteen cycles.
- Interpreted active wall medians 12.119/12.111 ms, p90 13.080/12.965 ms, p99 14.801/14.690 ms, maxima 16.212/15.137 ms. Not paired samples with prior builds. The 10-ms median / <=12-ms peak target remains unmet; continue optimizing simulation and retained/final drawing cost.
- Executable shrinks from 206,746,624 to 18,209,280 bytes, with separate PDB retained. No snapshot-size reduction claim.
- ipo-oct13-census-a adds stock/interpreted eight-actor Final Destination self-restore/rewind/15-tick CPU-only resimulation checks. All 39 components match, raw endpoint differences are zero, and 117 self-restore comparisons pass per case. Four audio starts reused; none new/canceled. Independent verifier checks pins, exits, owner counts and raw markers. One interval/case; not all-scene/fighter or production timing acceptance.
- Exact binary/PDB, compiler/build configuration, changed/deleted sources, profiling and test/verification evidence retained at C:/FreakMelee-test-archives/ipo-oct13-final. Goal active. Local commits only; no push.


### 2026-10-13 - Pose-only path for Z-scaled fighters

- Fixed the fighter PObj setup override: when geometry is disabled, it now uses the existing generic joint/envelope pose preparation even if Z scaling is active. This prevents graphics matrix/normal uploads during hidden reconstruction for Game & Watch / Flat Zone / authored scale changes. Normal rendered scale correction is unchanged; no new shim/API.
- Model-runtime checks rigid/shared polygons at Z scale 0.5: pose-only emits no uploads, retains joint marks, normal rendering still uploads. Complete model-runtime test passes.
- scaled-pose-oct13-stages-b compares exact pre-change/new binaries with four stock Game & Watch CPU9 fighters, 360 fast samples on Final Destination and Flat Zone. Gameplay/action traces match. Each candidate eight-tick GPU-free resimulation compares all 39 components, zero raw resim endpoint differences, and 117 self-restore comparisons/stage. Older rewind raw differences 1,561/2,033 bytes remain preserved.
- First baseline attempt failed because the archive executable lacked adjacent UI files. Failed cohort retained; accepted baseline is an exact old exe with copied unchanged DLL/UI support in a separate runtime directory. Stage runner supports fresh directories on any drive, records omission flags, pins changed source/runtime files. Independent report checks current pins/exits/counts/raw markers and hashes current logs; initial runner had no capture-time log hashes.
- Exact candidate exe/PDB, model test executable, changed source/tests/docs, build/test/verification evidence retained at C:/FreakMelee-test-archives/scaled-pose-oct13-final. No timing gain claim; the 10-ms median / <=12-ms peak target remains open. Goal active; local commit only.


### 2026-10-13: Reject unproven specular-light cache; retain full-cycle target

- The full performance target remains 10 ms median / <=12 ms measured peak for fifteen reconstructed ticks, checkpoint work and the normal rendered tick. No pacing sleeps are counted; OS/worker/GPU waits remain. Render submission is not a GPU completion fence.
- A specular-direction omission initially left 28 raw bytes stale. Deferred per-light calculation then exposed two active-list replacement failures (22 bytes each). These failed cohorts remain retained rather than accepted or hidden.
- The corrected experiment passed stock/interpreted eight-fighter Final Destination checks: fifteen GPU-free ticks, all 39 captured components equal, zero raw endpoint differences, 117 same-boundary recapture comparisons and four reused audio starts with no new/canceled starts per case. Eighteen selected core/audio/model tests also passed. This is bounded correctness evidence.
- Two full local UDP cohorts each measured 480 interpreted exactly-fifteen cycles. Active-wall medians 12.027 / 12.117 ms and peaks 17.229 / 15.927 ms do not meet the target or demonstrate a useful gain from the cache. The added cache, deferred runtime path and experimental fixture were removed. Existing accepted scaled-pose/ThinLTO code remains the foundation.
- The rejected binary/PDB/source/test versions, failed/passing census evidence and timing reports are retained under C:/FreakMelee-test-archives/specular-phase-oct13-*. The census runner keeps added light-header/display-traversal source pins. This is not final-source performance acceptance or whole-project completion.
- Next focus: larger simulation and display/submission costs, currently about 6.4 ms simulation and 4.3 ms drawing per complete experimental cycle, without discarding gameplay-visible effects or adding speculative runtime caches.


### 2026-10-13: Sampled stacks and simpler script fighter admission

- Kernel ETW capture rejected a circular configuration, then denied simplified capture access. No trace started or unrelated session was stopped. Added a standalone Windows owned-process stack sampler using local DbgHelp, exact PID/executable checks, primary-thread context/unwind, balanced suspend/resume attempts and recorded failures. It perturbs timing; sampling is attribution only.
- Two stock-zero/stock-delay/interpreted-delay diagnostic cohorts completed 600 samples each. The denser cohort collected 3,827 / 3,683 / 3,675 stacks. Process/display traversal, VM dispatch, callback-source liveness, actor lookup, animation/matrices/collision/materials, FIFO synchronization and UI remain visible. Inclusive sample counts overlap and are not milliseconds. All samplers recorded a final SuspendThread error 5 while the queried thread reported STILL_ACTIVE; exact cause unconfirmed, all owned processes exited.
- Simplified native_script_fighter_actor.c: all four private-dispatch callers admit the actor before program/slot access; duplicate entry walk removed. Callback entry explicitly retains admission. Post-execution generation and actor status are checked afresh once before any output/record publication, preserving retirement precedence and faulted continuations. Binding/VM native-reentry guards remain. No runtime cache, compatibility layer, ABI/state field or specific script primitive added.
- Nineteen selected fighter/core/audio/model tests pass; actual fighter fixture covers 8,392 callback/lifetime/motion/snapshot checks, 53 writes. Broader receiver fixture fails to link duplicate Setup/Combat/State/Transition definitions; retained as a failure, not suite acceptance. An obsolete jump target also failed before building and was removed from the batch.
- script-admission-oct13-census-b: stock/interpreted eight-fighter Final Destination cases each complete fifteen GPU-free ticks with all 39 components equal, zero raw endpoint differences, 117 same-boundary comparisons, four reused audio starts and no new/canceled starts. Independent pin/log/marker report passes. Census pins now include the changed actor, execution/lifetime source and snapshot API.
- Two unprofiled UDP timing cohorts provide 480 / 457 interpreted exactly-fifteen cycles. Active-wall medians 11.989 / 11.990 ms; peaks 15.224 / 26.697 ms. Ordinary verifiers pass provenance/phase/counter/checksum/endpoint checks. Snapshot peaks remain 2,570,103 stock / 2,940,591 interpreted bytes. The high tail is retained, with no unproven OS-interference attribution.
- The 10-ms median / <=12-ms measured-peak target remains unmet. Small median differences from older unpaired runs are not isolated speed claims. Artifacts remain under C:/FreakMelee-test-archives/{stack-sampling,script-admission}-oct13-*; tools/native_stack_sampler.cpp and docs/native_stack_sampling.md preserve the diagnostic method. Goal active; local commits only.


### 2026-10-13: Restore receiver and shared primitive verification

- Resolved the broader receiver fixture failure reported in the previous entry. Shared kinetic engine leaves no longer carry unavailable Setup/Combat/State/Transition binding definitions that collide with real bindings or another fixture. Standalone unavailable bindings are a separate test unit; complete targets select the intended source/attachment providers once. Removed stale duplicate actor/held-visual/collision-box definitions.
- Added explicit rejected boundaries for newly linked but unexercised Item/stage/vector/accessory/common-state operations, and updated standalone dependency lists directly. No production operation was replaced or ownership guard relaxed. Fresh standalone outputs can be stored on any drive. Failed build/run attempts remain retained.
- Receiver Fighter instances now use their shared package parameter prototype; the article retains its distinct parameter view. Preserved source/schema/identity/replacement checks. Added a mismatch case that rebinds a Fighter to the article parameter view and verifies motion entry rejects before effect/position/motion/result mutation. The old per-instance Fighter attribute override did not satisfy current source admission.
- All nine related CMake probes pass: native bindings, Execution, kinetics, state/collision, fighter lifecycle, Setup, combat, transitions and retained receivers. Strict standalone kinetics passes 26,148 checks / 7,104 native-body comparisons; combat 37,048 checks / 12,240 RNG pairs / 30 native-body comparisons; receiver 4,435 checks / 18 execution runs; transition 451 checks / 26 execution runs. Independent source/compiler pin and log/result checks pass.
- These are controlled fixture/engine-boundary checks, not whole-game, complete article creation, renderer or distributed-network acceptance. Production executable SHA remains 952eb8912a627ac8b01fc60920a0c27feac8623d023379fb93aa7ea2aab2447d, identical to script-admission-oct13-final. No game was launched and no new performance claim is made. The 10-ms median / <=12-ms measured-peak target and full project goal remain open.
- Evidence is retained under C:/FreakMelee-test-archives/receiver-fixture-oct13-*. Continue with larger simulation/display costs after this verification repair. Local commit only, no push.


### 2026-10-13: Batch complete texture and palette FIFO loads

- Aurora 89ce7b453ba6897cdc3f04a5ff6b8b67bbd27acb emits each texture/palette load as one fixed stack packet and FIFO append (67/72 texture bytes, 46 palette bytes). Retains all BP commands, metadata, object/frontend mutations and decoder order. No new threads, cache, tick allocation, ABI or saved state. Omitting loads entirely was rejected because later visible draws can reuse their bindings.
- All 212 GX correctness and 19 selected native fighter/core/audio/model tests pass. Independent scalar byte/frontend comparisons cover every map, RGBA/CI/mips and all 20 palette slots, plus display-list capture, growth and CPU-inline/visible-worker ownership. Valid captures remain identical. Invalid undersized display lists no longer receive partial malformed load fragments. Initial display-list fixture failure from unequal frontend contexts is retained; corrected fixture uses identical starting contexts.
- Optional alternating append-only samples measure roughly 33-to-14 ns texture loads and 22-to-9 ns palette loads, about 2.4x. This excludes decoder/GPU/snapshot costs and does not claim a whole-frame speedup.
- Fresh stock/interpreted eight-fighter FD census: 15 CPU-only ticks, all 39 components equal, raw differences 0, 117 same-boundary comparisons, four reused audio starts and no new/canceled starts each. Independent receipt/source/binary/log verification passes.
- Two full unprofiled local UDP cohorts each contain 480 interpreted exact-fifteen cycles including one normal rendered submission and checkpoint work. Medians 11.870/11.873 ms, peaks 17.166/15.474 ms. Peer checksums and endpoint/RNG unchanged, as are 2,570,103/2,940,591-byte stock/interpreted snapshot peaks. These modest differences from older unpaired runs are not isolated whole-cycle attribution. Tail samples are retained.
- 10-ms median / <=12-ms measured-peak acceptance remains unmet. Larger simulation/display reductions are still needed. Evidence: C:/FreakMelee-test-archives/texture-packet-oct13-final, census-a and live-a/live-b. All launches hidden/muted, no debugger or sampler attached. Local commits only; full goal active.


### 2026-10-13: Admit immutable bytecode once before execution

- Added nativeScriptBytecode_Prepare after complete lowering: validates immutable function/instruction extents, register/selector/argument ranges, call/return signatures, branches, literal ranges, opcodes and terminal control flow. Failed preparation clears its marker. Source banks and execution binding require prepared programs. Metadata arrays/extents remain immutable until their executions retire. No compatibility interpreter, format version, thread or tick allocation added.
- Removed repeated structural checks from VM dispatch. Kept dynamic entry argument/conversion/pool limits, runtime value/operator/selector checks, native result/output contracts and reentry/lifetime barriers. Snapshot peak sizes remain unchanged. Thirty malformed candidates prove rejection before host liveness, outputs or VM-stack mutation; the bytecode probe passes 8,475 checks / 12 functions / 282 instructions.
- All 35 selected native script/core/audio/model tests pass. Raw bytecode fixtures prepare explicitly; state/storage fixtures compile their whole declared program, and storage uses the event signature catalog. Obsolete negative event assertions corrected for now-supported effect physics/trail behavior. Item/presentation link boundaries repaired with explicit unavailable engine leaves and the exact null-safe AObj flags getter; these are test-only boundaries. Failed attempts retained.
- Six read-only source audits across Zelda-start, Sheik-start and Ice Climbers paired-member projects pass with zero diagnostics. Three strict standalone probes pass: between-joint effects, event records and airborne items. Eight older standalone probes still stop at stale binding/arena dependencies or native-source guards (article/common/feedback/pose catalogs, CPU control, vectors, async/raw-part effects). No acceptance is claimed for them; their raw constructors adopt preparation, and fresh outputs can use C:.
- Fresh stock/interpreted eight-fighter FD census: fifteen CPU-only ticks, 39 matching components, raw differences 0, 117 same-boundary comparisons, four reused audio starts and no new/canceled starts each. Independent pins/logs/markers pass. Two unprofiled real local UDP cohorts each contain 480 interpreted exact-fifteen full cycles, with medians 11.769/11.850 ms and peaks 17.507/14.851 ms. Simulation medians 6.286/6.257 ms; drawing 4.223/4.221 ms. Peer checksums, endpoint/RNG and 2,570,103/2,940,591-byte stock/interpreted snapshot peaks unchanged. Small unpaired differences are not isolated speed attribution; tails retained.
- 10-ms median / <=12-ms peak acceptance remains unmet. Continue with repeated source admission and remaining display preparation while retaining reentry/lifetime checks. Evidence C:/FreakMelee-test-archives/bytecode-prepare-oct13-*, hidden/muted match launches, no sampler/debugger. Full goal active; local commits only.


### 2026-10-13: Consolidate source checks at execution boundaries

- Private execution IO now checks mutable storage identity/generation without repeating actor-list walks before native/receiver/payload boundaries that already validate their source. VM entry, native reentry and full liveness before invocation output/callback publication remain. Receiver/schema/lifetime checks unchanged. Local storage/table access cannot reenter the engine. No pointer cache, ABI change, tick allocation or saved-state addition.
- Plain admitted coordinate reads return after direct field loading; source admission precedes the dereference. Callback-capable/non-coordinate paths retain their barriers. Added 36 stale-generation/unlinked-source rejection scenarios across 12 Fighter / six Item coordinates, with poisoned userdata and unchanged outputs. Property probe passes 39,699 checks / 9,804 observations; all 35 selected script/core/audio/model tests pass, including 8,392 fighter lifecycle/callback/snapshot checks. Previously recorded standalone gaps remain.
- Fresh stock/interpreted eight-fighter FD census: 15 CPU-only ticks, all 39 components match, raw differences 0, 117 same-boundary comparisons, four reused audio starts with no new/canceled starts each. Independent pins/logs/markers pass. Two unprofiled real UDP cohorts each provide 480 interpreted exact-fifteen full cycles: medians 11.824/11.816 ms, maxima 15.136/14.230 ms. Simulation medians 6.243/6.296 ms, drawing 4.244/4.212 ms. Peer checksums, endpoint/RNG and snapshot peaks unchanged. No substantial whole-cycle gain or isolated attribution is claimed; tails retained.
- 10-ms median / <=12-ms peak gate remains unmet. Investigate larger UI/display costs next: Aurora currently updates Rml every visible frame and the dependency offers a next-update-delay hint, whose dirty/input/animation contracts need inspection first. Evidence C:/FreakMelee-test-archives/script-source-gates-oct13-*. Hidden/muted matches, no debugger/sampler. Full goal active; local commits only.
