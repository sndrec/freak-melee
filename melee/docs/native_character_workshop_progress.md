# Character workshop implementation evidence

The approved feature and acceptance gates are in
`native_character_editor_handoff.md`. This file records implementation evidence
without marking the workshop accepted prematurely.

## 2026-10-07: owned Fighter pool, transitions, presentation and defaults

- Actual normal/demo constructors now allocate stable Fighter-first owner slots
  with full attribute backups, all 140 part entries and 124/32 drawing-pointer lists.
  The real shared Kirby/Purin extra-list allocator remains a separate owner.
  Actual-source pool tests passed 10 scenarios /1,037 assertions, including 64-owner
  exhaustion, resurrection/reuse and atomic invalid-snapshot rejection.
- Packed pool snapshots save occupied fixed owners and allocator identity/free
  order. Each actual aligned slot is 19,136 bytes. This is not a complete match
  snapshot: animation/dynamics/effects, globals and script owners remain separate.
  Scene boundaries also retire script actor views without a destructor sweep.
- Owned motion switches and contextual lowering now execute complete direct banks;
  451 checks /26 Execution runs cover targets, U32 flags, frame/rate/startup,
  ownership failures, source retirement and accepted-bank preservation.
- Generic sound/semantic-part/joint operations and staged world-position outputs
  passed 6,621 checks, with 672 sound/seed and 800 transform comparisons. Constants
  now work in owned parameter defaults; 6,191 checks include 11 typed defaults.
  UInt parameters/helpers/locals are supported; unsigned state and imported word
  attributes remain explicitly outside their current storage/import contracts.
- Real eight-fighter gameplay completed 2,000 samples with exact previous diagnostic
  counters/state hashes and no topology errors. All 64 broad-oracle rollback
  endpoints reproduced; mean 173.91 ms still misses 16.667 ms. Owner payload/capacity
  figures do not prove the full mutable-state size or an explicit-backend speed.
- Client/tool builds and 27 focused CTests pass. Source-only Workshop/package
  activation, remaining engine families and hidden old-generator template setup
  still need migration. Scope: `native_script_transitions.md`,
  `native_script_presentation_bindings.md`, `native_fighter_snapshot_ownership.md`.
  Goal stays active, with no full-template or whole-snapshot acceptance claim.

## 2026-10-07: motion scheduling, combat/constants and archive scratch removal

- The actual state-change and animation/input/physics/collision/accessory sites
  select interpreted phase IDs from validated owned motion layouts. All eight
  special input routes use selected source entries while retaining shared input
  priority, facing and ground-speed preparation. 156 assertions cover real actor
  binding, all eight extracted input routes, alias conflicts and retirement.
- The 978-name typed constant catalog lowers to literal IR at load, with 6,038
  assertions and 978 independent native comparisons. Generic damage/status/effect
  and RNG bindings passed 36,964 checks, including 12,240 result/seed pairs.
  Unsupported spawn/receiver/hitbox and transition operations remain explicit.
- Source/no-access proof confirmed x59C/x5A0 are unused native archive buffers,
  separate from the live interpolation skeleton. Native allocation/free is gone;
  73 cases retained 128 track attachments and eight skeleton cleanup calls.
- Real eight-fighter gameplay completed 2,000 samples with no topology errors.
  The partial census dropped exactly 524,288 bytes to 864,088; earlier diagnostic
  event counters matched. Per-state idle callbacks ran 92/154/54/101 times.
  All eight-Int state hashes matched independent reconstructions.
- The new motion registrations/state reproduced all 64 broad-oracle rollback
  endpoints with drawing retained. Mean 168.31 ms still misses the 16.667 ms
  target. The Fighter owner/satellite snapshot schema is a proposal; complete
  explicit state and source-only Workshop/package activation remain open.
- Native client/tool builds and all 25 focused CTests pass. Source/evidence and
  scope: `native_script_motions.md`, `native_script_combat_bindings.md`,
  `native_fighter_snapshot_ownership.md`, `tests/native/script_constants/README.md`.
  No UI change or full-template/backend acceptance is claimed. Goal stays active.

## 2026-10-07: instance callbacks, typed imports and scene audit

- Native instance callbacks now register source-owned function IDs directly:
  damage/death, pre/post-hitlag, dealt-damage and one-shot state change. The actual
  call sites and original reset masks have migrated without authored C thunks.
  84 checks exercise actual damage/hitlag bodies and retirement; 199 setup checks
  include 24 original item-presentation comparison pairs and CodeRef/None handling.
- Four CPU Popos each completed 2,000 direct interpreted ticks/late updates with
  actual damage/hitlag/dealt-hit callbacks and independently verified owned-state
  hashes. A subsequent eight-fighter benchmark reproduced all 64 full-state
  endpoints at 15 replay ticks; mean 170.36 ms still misses the deadline. Other stock
  behavior remains native, and the broad snapshot oracle remains in use.
- A transition-only parameter provider copies imported values into named owned
  parameters before publication. The 111-check compiler fixture rejects invalid
  imports/ambiguous supplies without changing the accepted bank. The new DAT
  reader validates root bounds/types and explicitly leaves entity imports open.
- The complete-source GM scene runner passed 13 transitions and 417 assertions with
  real HSD pools, including home practice, results and sudden death. Assets and
  frame-loop payloads are controlled; actual UI/async acceptance remains pending.
- DAT evidence includes 35 synthetic checks and 144 real Mario checks: 31 root
  imports, four explicitly unresolved entity imports, 132 actual attribute bytes.
  Client/tool builds and all 23 focused CTests pass.
- Scope and receipts: `native_script_live_loading.md`,
  `native_script_setup_bindings.md`, `native_gobj_scene_lifetime_audit.md`.
  Motion/entity/asset metadata, wider primitive families and Workshop/package
  source-only integration remain; this is not full-roster/backend acceptance.

## 2026-10-07: live interpreted lifecycle and first native pool consumers

- Direct compiler banks now publish only fully validated candidates and preserve
  the accepted bank on failure. The 96-check fixture covers schema/parameter
  readiness, helper/callback graph rules and buffer aliasing; broader package
  schema and Workshop loading remain open.
- Load/tick/removal sites can execute a selected source-only program using actual
  actor owners. Reset/respawn rebinds the changed creation ID without clearing
  persistent script variables. Four CPU Popos each completed 2,000 interpreted
  ticks with exact expected eight-byte state and no faults. The earlier attempt
  counted requests with zero state and is excluded from acceptance.
- The real eight-actor benchmark reproduced all 64 complete endpoints after 15
  replay ticks; script clocks/state ended at 664 advancing ticks. Mean175.53ms
  still misses the16.667ms deadline; page tracking remains. This is partial live
  lifecycle correctness, not complete fighter/backend or speed acceptance.
- Actual native GObj/proc allocation/final free now use process-reserved pools
  with scene lifetimes, captured by the broad oracle. 20,615 actual-source
  assertions cover deletion/reuse/exhaustion/ordering; eight-actor live populations
  match the former census. All-scene transition and other-owner migration remain.
- General state/collision/input primitives now have explicit independent-owned
  guards and 179 checks. Shared fall entry has its missing post-motion retirement
  guard. Client/tool builds and 20 focused CTests pass. See live loading and pool
  consumer documents for evidence/limits. The goal remains active.

## 2026-10-07: actual actor execution entry, payloads and kinetic family

- Native execution now combines real actor liveness, immutable code, typed
  instance variables/parameters and scoped event payloads. Load-time function
  binding avoids source lookup/parameter scans at invocation. A 418-check fixture
  executes 128 fighter ticks with actual actor position/storage writes and
  validates article signals, history IDs, classes and retired generations.
- Self item state/parameters/imports resolve directly with exact owner checks.
  Payloads retain descriptor identity, typed inputs and synchronous lifetime;
  the 156-check fixture covers nesting, high-bit IDs, Object handles and protected
  retired sources. Changing creator/owner receivers remain separate work.
- General animation, motion and physics bindings preserve native operation order
  and Float/Double intermediates. 25,582 checks include 6,976 actual-body pairs,
  91 Double-sensitive cases and retirement suppressing subsequent writes.
- The untimed live census completed 2,000 eight-fighter Battlefield samples with
  zero topology errors. GObj/proc peaks were 111/196; combined counted payload
  1,388,376 bytes is a partial ownership estimate, not complete rollback state.
- Client/tool builds and 17 focused CTests pass. Native scheduler selection,
  broader bindings/metadata and Workshop/package loading remain pending; the
  working game still runs its prior ordinary behavior path. See the frontend and
  census documents for source/evidence limits. The goal remains active.

## 2026-10-07: owned storage and source-audited event contracts

- Actual indexed declarations now supply fixed instance state buffers and shared
  immutable parameters. Imports remain unresolved until values are supplied.
  The 713-check storage fixture runs 128 VM ticks and reproduces a 64-tick endpoint
  after restoring active state; owner/generation/layout/config changes reject
  old snapshots before writing. Native object resurrection remains separate.
- Source-audited event metadata distinguishes fighter/article/item/effect
  callbacks and retains native slot/type provenance. Normal callback defaults
  are explicit bytecode returns. Generic read-only and exact/truth return
  policies preserve the audited contracts; Bool helpers retain documented
  nonzero return conversion. The 301-check event fixture runs 28 callbacks and
  three helpers. The selected 16-source census resolves 1,861 signatures and
  explicitly rejects two mutable trail payloads.
- Native client/tool builds and 13 focused CTests pass. No game was opened.
  Native payload families, scheduler code references, broader metadata and
  Workshop/package activation remain open. The ordinary C route is not removed
  until those call sites migrate; the approved interpreter direction is unchanged.

## 2026-10-07: direct bytecode runtime and explicit pool foundation

- Direct source indexing now checks helper signatures, parameter constants and
  owned array layout across all 33 recursive example files (4,762 checks / 8,783
  declarations). Missing callback signature contracts remain unresolved rather
  than acquiring inferred signatures.
- Typed statements lower directly to bytecode. Fixed register/frame pools execute
  helpers, branches, loops, typed numeric operations, captured selectors and
  native outputs. The 8,322-check runtime fixture covers synchronous reentry and
  retirement/recycled generation, including protected actor storage. No authored
  C or native fallback executes this fixture.
- Actual native Fighter/Item property and math bindings pass 27,225 checks,
  including direct FMS-to-VM actor writes. Other engine families, changing
  receivers, full metadata and production event binding remain open.
- Occupied-object pool snapshots preserve generations, free ordering and linked
  payload across creation/destruction/reuse. The source scene-lifetime audit
  identifies a single process-reserved GObj/proc family with scene epochs as the
  proposed consumer. Actual consumers and the whole-game snapshot are not wired.
- The client/content tool build and 11 focused CTests pass. No game was opened.
  Workshop/package loading still uses the existing ordinary behavior path until
  native scheduling and owned-state integration are ready. The goal is active.
  See `native_script_frontend.md`, `native_interpreter_transition.md` and
  `native_match_pool_first_consumer.md` for supported scope and remaining gates.

## 2026-10-07: direct interpreter frontend begins

- New caller-owned native source, expression and statement IR reads `.fms`
  directly. No generated C, Clang AST or per-function C ingress is part of this
  production frontend. The working client retains its current backend until
  program/binding/runtime integration is complete.
- The source hierarchy covers all 27 checked-in examples. Typed expression and
  statement fixtures establish semantic selectors, scopes, scalar conversions,
  short circuiting, source spans, invalidation effects and rejection of partial
  bodies. The client/tool build and seven focused CTests pass.
- `native_script_frontend.md` records exact acceptance and remaining gates.
  The binding inventory leaves 660 signatures unresolved rather than guessing
  runtime support. Metadata/program compilation and bytecode execution are next.
- `native_match_snapshot_inventory.md` audits actual state owners and object
  resurrection. In-payload HSD freechains, class allocation and OS heap metadata
  must restore with object graphs; a small live-Fighter copy is insufficient.
  No snapshot size, save/load time or completed serializer is claimed.

## 2026-10-07: interpreter evaluation and approved production transition

- The user approved interpreted ordinary fighter behavior and explicit audited
  savestates, superseding generated C as the planned production backend.
  `native_interpreter_transition.md` is the implementation handoff. The current
  production client remains on generated modules until that migration is complete.
- An isolated typed-lowering evaluator executes all 523 authored Popo/Nana
  bodies without native authored fallbacks. The existing engine primitives,
  immutable setup and explicitly listed generic state accessors remain native.
  VM storage is fixed; author variables remain in existing rollback-owned bytes.
  Its Clang AST input and fixed C ingress are evaluation scaffolding only.
- Real hidden/muted runs used eight live paired CPU fighters and 64 windows of
  15 full replay ticks plus one advancing tick. Native/interpreted semantic
  endpoint hashes and observed function counts match at every measured endpoint;
  interpreter errors remain zero. Measured game coverage was 14/523 bodies.
  Additional scalar and 40-node rope fixtures are bounded correctness evidence.
- The interpreted replay simulation mean is 18.425 ms versus 17.099 ms for C.
  Both full windows miss the 16.667 ms target: 210.871 ms interpreted versus
  197.843 ms C. Snapshot begin/end dominate across about 193 MB of registered
  heap/global ranges; dirty pages are around 1.7 MB per tick. Write-fault cost
  remains inside simulation/drawing timings. Future save/load speed is unproven.
- Evidence and reproduction live under `tests/native/interpreter_evaluation/`.
  Root comparison: `A:/FreakMelee-test-archives/interpreter-eight-actor-comparison.json`.
  The approved decision is based on modest observed interpretation cost, not
  a claimed deadline pass. No ordinary package migration or UI redesign is done.
- Samus constructor indentation is now corrected and generation/DLL/harness
  compilation pass. Its first matrix found a real draft mismatch: literal part4
  was incorrectly equated with `FtPart_ThrowN`. The complete matrix is still
  unaccepted; that source mismatch awaits correction. Preserve this independent
  authored work while changing execution backends.

## 2026-10-04: generic grapple construction primitives, pause checkpoint

- Article animation components can select a validated constant element of an
  immutable pointer array, independently per joint/material/shape channel.
  Direct components and indexed components share graph identities in the own
  arena. Malformed, external, out-of-bounds and allocation-failure cases never
  publish a partial table. Runtime and module validation use the same typed
  recipe; the module fingerprint rejects a mismatching layout.
- Named model animations now select a subtree. Root shorthand lowers directly
  to the changed Host signature with an empty path; there is no compatibility
  backend. Add can return Bool, allowing authored construction to reject a
  missing animation target. Generated Add/Advance/Remove continuation guards
  stop after item retirement. Actual HSD first-child cases pass.
- Resource effects target the real node GObj. Position and position+facing
  payloads admit only source-verified independent-generator signatures; wrong
  families, joint/model and unknown IDs fail before variadic dispatch. Dispatch
  retains node storage through nested release and validates creator/author.
- Source audit across805 disc DATs proves the seven Samus grapple external
  channels are deliberately bound to NULL by retail initialization. The eight
  actual indexed animation graphs and four model roots match typed stock graph
  identities in the real-disc import CTest. No graphs were fabricated.
- Full native build and14 focused CTests pass. Root reproduces196 routing checks
  across16 model/path pairs, both generated root/subtree/Bool-retirement routes,
  and the effects matrix:29 compiler programs,881 generated checks/28 lifetime
  cases per route,3,276 actual backend/geometry checks. Evidence:
  `A:/FreakMelee-test-archives/article-model-subtrees-root/receipt.json` and
  `A:/FreakMelee-test-archives/article-resource-effects-root/receipt.json`.
  HSD/particle/ECB boundaries in scoped fixtures are explicit; no gameplay or
  complete Samus closure is claimed. No game was launched in this batch.
- User requested a pause for an app restart. Samus remains inherited/partial;
  constructor composition is an isolated unaccepted draft under
  `tests/native/samus_grapple_constructor/`. Fresh clone/import passed, but its
  new callback body indentation still prevents script generation. Resume from
  that namespace's README, then continue the three closure gates in
  `samus_clone_closure.md`. No user-installed packages were changed.

## 2026-10-04: owned demo clips in the paired Workshop

- The approved single-draft member selector now also retains each member's
  selected animation bank, clip, frame, search and undo history. The existing
  Animation browser separates gameplay clips from results/other clips using
  their actual owned descriptor names. No replacement editor layout was added.
- Key selection, graph transforms, clip-length edits, pose capture/paste and
  bone key insertion address the selected Action or Demo bank explicitly.
  Same-index clips in different banks cannot alias in the editor. Changing
  banks resets the newly selected clip to frame zero; changing work modes does
  not overwrite a retained demo selection with the gameplay action.
- Frozen Demo preview samples the selected member's live rig directly from its
  owned decoded clip. It does not enter a gameplay motion, execute subactions
  or dispatch stock fighter callbacks. A validated actor/rig/clip cache avoids
  repeated attachment and validation during scrubbing. Release restores the
  retained gameplay tree, frame and rate before resuming its actor.
- Bone inspector refresh now follows live pose evaluation. A drawn paired
  check caught and corrected startup/rest values surviving a member switch;
  a later check restored Zelda's Win1 frame 60 and matching inspector values
  after visiting Sheik. Inspector inputs are not overwritten every UI tick.
- Full native build and nine focused CTests pass. Independently reproduced
  source fixtures pass 307 preview/lifetime checks, 126 pose-transfer checks
  across three bank directions, and 53 real-Rml selection/session/history/key
  checks. Hidden/muted native UI screenshots show Win1 scrubbing, Sheik's own
  action preview, restored Zelda clip/frame/inspector and Demo graph selection.
  No OS input or user-installed content was modified; task clients were closed.
- Evidence is under `A:/FreakMelee-test-archives/`: `pose-editor-demo-root`,
  `workshop-animation-preview-root`, `workshop-clips-key-root`, and
  `owned-demos72-workshop-ui-final`. These bounded cases do not establish every
  costume, animation, result variant, UI interaction or paired gameplay branch.
- The source-closure census is in `tests/native/fighter_clone_closure/audit.md`:
  26 copied bases, 25 independent selected member templates, Samus still partial.
  Fox/Doc result-item omissions and unsupported original video modes are
  recorded separately; owned clip construction alone is not behavior closure.

## 2026-10-04: authored Fox/Doc presentation items

- Fox's selected template now owns gun creation for common presentation modes
  0/1/5/6. It initializes commands before thumb attachment chooses state 10;
  modes 0/1 apply the original caller scale afterwards. Falco intentionally
  creates no presentation item, preserving the original Fox-only policy.
- Dr. Mario retains the two ordered result-pill draws in mode 1 and now owns
  Sleep's mode-2 pill. Appearance, physics refresh, pickup and manual-lifetime
  calls follow the original order. Allocation and invalid-rig paths are
  explicit; no stock fighter/item callback or new Host API was introduced.
- Transfer comparisons found and fixed a real Fox state-10 discrepancy:
  presentation scaling follows the original creator, not a later current owner.
  The script now uses the existing generic creator queries. The failed source
  comparison is retained; no observed fields were masked to pass it.
- Root reproduces 10,700 exact ordered stock/generated comparisons, 43 safety
  cases and 811,442 checks using fresh frozen-tool clones/imports and three full
  generated DLLs. All 45 extracted producer/API bodies per member match the
  live checkout after newline normalization. Real allocation/model/animation
  installation remain controlled boundaries, as detailed in
  `tests/native/fighter_demo_presentation/README.md`. This is source-policy
  evidence, not rendered result-scene or whole-fighter acceptance.
- Full native build and the current-tool parameter-import roundtrip pass;
  shipped Fox/Doc templates match source hashes. Root receipt:
  `A:/FreakMelee-test-archives/fighter-demo-presentation-root/receipt.json`.
  Original mode-0/1 NULL-scaling undefined behavior is excluded from exact
  pairing and covered by the authored failure policy instead.
- Samus remains inherited/partial. Its remaining generic resource, capsule,
  target-search and throw-presentation contracts are recorded in
  `docs/samus_clone_closure.md`; source independence must wait for full closure.

## 2026-09-28: Dr. Mario special entries and replay input mapping

- Eight isolated inputs now cover ground and air entries for all four Dr.
  Mario specials. Stock and the installed scripted clone match across each
  520-sample path, including traced fighter states, actions, items and RNG.
  The cases are stored in `tests/native/parity_inputs/`; contact and later
  branches still need their own comparisons.
- A separate grounded Tornado input taps B during its lift window, then runs
  a second Tornado. The first tap changes upward velocity at sample 105; both
  uses complete by sample 280, and stock and clone match through all 520
  samples. This covers the lift and repeat branches beyond basic entry.
- Close-range grounded Super Jump Punch, Cape, and Tornado inputs now match
  stock against the scripted clone through five, one, and five hits on Zelda
  respectively. The hit timings and 14%, 12%, and 14% total damage match over
  each 520-sample trace. Their input fixtures are checked in alongside the
  prior neutral-special pill contact case.
- Four corresponding airborne contact inputs now match stock as well: an 8%
  Megavitamin hit, a 12% Cape hit, five Super Jump Punch hits totaling 14%,
  and five Tornado hits totaling 14%. Each is a 520-sample stock-versus-clone
  comparison with P1 airborne at special entry. Landing, other contact
  geometry, and interruptions remain unverified.
- Diagnostic replay recording now captures controller inputs after native
  profile binding and calibration, before netplay's four-port input filter.
  Playback skips the current profile mapping and feeds the recorded inputs
  through the same netplay filter. This prevents a later binding change from
  changing a local replay's input while preserving the netplay replay path.
  The replay unit test, a late-jump runtime input probe, the eight special
  comparisons and all 380 CTest cases passed. Existing recordings made before
  this change may reflect their old raw-device capture point.

## 2026-09-28: authored Dr. Mario pill and taunt path

- `examples/drmario_full.fms` now authors Dr. Mario's taunt animation,
  damage/death cleanup, and all seven vitamin article states. The taunt and
  neutral special use the same script-owned vitamin history; neutral special
  uses the generic authored projectile constructor. The article's decisions
  use common item/fighter primitives rather than stock Dr. Mario callbacks.
- An isolated, installed package of this draft matched stock on the existing
  520-input taunt-only replay: 1,038 fighter states, 17 actions, 114 item
  states and 519 RNG states. The taunt-then-neutral replay matched 1,038
  fighter states, 16 actions, 154 item states and 519 RNG states. Eight
  different initial seeds matched that second recording, including item
  texture selection. After changing neutral special to the authored spawn
  constructor, seeds 1 and 8 matched again.
- A separate recorded taunt path takes Zelda's jab during taunt and matches
  stock across 520 inputs: 1,038 fighter states, 29 actions, 24 item states,
  four damage events and 519 RNG states. Its input fixture is checked into
  `tests/native/parity_inputs/`. The normal taunt reaches command 2 and
  removes the attached pill before its action ends. A second fixture brings
  Zelda near Dr. Mario, then throws the authored pill into her at sample 284
  for 8%; all 520 inputs, 1,038 fighter states, 26 actions, 79 item states,
  the damage event, and 519 RNG states match stock. Pill pickup, reflection,
  clank contact, other interruption timings and the exact bounce
  threshold remain unverified. Dr. Mario is not yet accepted as fully
  cloneable.
- A shield variant of the close-range pill fixture reaches Zelda's shield-hit
  motion 181 at sample 284 with no damage. Stock and clone match all 520
  inputs, 1,038 fighter states, 30 actions, 79 item states and 519 RNG states.
  This covers one shield contact, not all shield-bounce behavior.
- Dr. Mario's two taunt motion states now script all four state callbacks.
  The interrupt uses the shared taunt input routine, physics uses common
  ground friction, and collision uses the common grounded nudge/ledge path.
  Neither state calls the Dr. Mario taunt wrappers. The rebuilt package
  matches stock on taunt-only, damage-interrupted taunt, pill hit and pill
  shield recordings. All 90 native tests pass after rebuilding module
  fixtures for the changed host interface. Death-state reset, pickup,
  reflection and other article interactions still need parity work.
- Dr. Mario's stock death callback hides model group 0 and clears Tornado
  charge, Cape boost/reference and taunt-pill tracking. The clone now does
  that in `on death`, including a script operation that forgets the Cape
  reference without invoking any stock Dr. Mario handler. A two-stock input
  fixture matches stock through an offstage death, respawn and subsequent
  neutral special: 520 inputs, 1,038 fighter states, 26 actions, 36 item
  states and 519 RNG states. It does not establish charged-Tornado or
  active-Cape cleanup parity across death.

## 2026-09-27: stock-versus-scripted special-move comparisons

- The native fighter trace now includes raw hitlag-frame and animation-speed
  words alongside motion, input, position and velocity, plus deterministic
  pose, hitbox and hurtbox digests for both fighters. The reusable
  `tools/compare_fighter_parity.py` checks completed stock/clone
  `--replay-inputs` runs, reporting the first differing fighter state, action
  or damage event. It ignores the expected stock/custom kind only on declared
  clone ports in action records; other fighters' kinds must match. A two-port
  Captain Falcon contact comparison passed (380 samples, 758 fighter states,
  24 actions, one damage event, 379 RNG states), while an intentionally altered
  P2 kind was rejected at sample 1. The same rendered fast-match path is used
  in both runs.
- With an identical seed and controller inputs, stock and scripted Ganondorf
  matched across 520 samples containing grounded Warlock Punch and Gerudo
  Dragon start; 420 samples containing airborne Warlock Punch and its landing
  transition; and 420 samples containing airborne Gerudo Dragon start and
  landing. Their traced state words matched at every observed sample.
- A separate Final Destination contact replay walked Zelda into range and
  triggered Gerudo Dragon's grounded hit state. Stock and clone matched all
  758 fighter-state lines, 23 action transitions and the 17% damage event over
  380 samples. The corresponding Captain Falcon contact replay matched 758
  fighter-state lines, 24 actions and the 7% damage event. A 420-sample
  Captain replay also matched airborne Falcon Punch and Raptor Boost start,
  including landing and one defender damage event.
- Repeating those grounded contact replays with pose, hitbox and hurtbox
  digests still matched every observed state for both fighters. Ganondorf's
  separate 420-sample airborne Gerudo Dragon replay likewise matched the
  complete traced pose and collision geometry through the sampled path.
- The trace also records each live item's semantic core, object relationships
  and joint pose. A newly built Dr. Mario draft matched stock across 520
  samples with grounded Megavitamin and Cape: 1,038 fighter-state lines, 17
  actions and 110 live item states from the pill and Cape. This tests the
  current projectile path but
  does not cover his stock taunt, all pill callbacks or the other specials.
- A taunt-then-Cape replay exposed a native clone-routing error: Dr.
  Mario's clone entered the common taunt state (264) instead of his stock
  state (341). The shared taunt dispatcher now uses the fighter's behavior
  kind, preserving clone identity. The same 520-sample replay then matched
  1,038 fighter states, 17 actions and 114 item states, including the attached
  taunt pill and the later Cape. A Ganondorf taunt replay also matched.
  Dr. Mario's taunt still runs his inherited stock C callback, so this fixes
  the clone base's behavior while leaving full script ownership unfinished.
- Fighter scripts can now own the generic taunt-entry hook and choose an
  action's right/left motion pair through `state.taunt(...)`. Dr. Mario's
  taunt entry reproduces his stock command initialization in script. Rebuilding
  and replaying the same taunt-then-Cape fixture still matched all 1,038
  fighter states, 17 actions and 114 item states. His taunt animation callback
  and attached-pill cleanup are still inherited stock C.
- The same-input comparator now checks the engine RNG seed and live item
  material/texture selection at every traced sample. This caught a visual
  difference on the *first* neutral Megavitamin even though fighter state,
  item physics, pose and RNG matched. An untouched, data-only Dr. Mario clone
  matched stock on the same 520-sample replay, isolating the error to the
  scripted move. Stock Dr. Mario begins with zeroed vitamin history; the script
  had initialized both entries to nine, excluding different colors from its
  first draw. Initializing them to zero restored an exact match: 1,038 fighter
  states, 17 actions, 110 item states and 519 RNG states.
- A taunt-then-neutral-Megavitamin replay still fails at the second pill's
  visual selection on sample 254. Stock taunt updates `u.mr` vitamin history,
  while scripted neutral special reads a separate module history. The attached
  taunt pill and neutral special need one script-owned history before this
  combined path can pass; Dr. Mario is not yet fully cloneable.
- Ganondorf's grounded side-special contact replay still matches with the
  stronger trace: 380 samples, 758 fighter states, 23 actions, one damage
  event and 379 RNG states. A separate airborne side-special replay matches
  through its air-side motion: 420 input samples, 672 fighter states, 17
  actions and 336 RNG states; it does not exercise a hit. These comparisons
  support only the recorded branches, not the remaining fighter-specific code.
- Captain Falcon's rebuilt draft likewise matches his stock grounded
  side-special contact path (380 samples, 758 fighter states, 24 actions, one
  damage event, 379 RNG states) and an airborne side-special miss path (420
  input samples, 648 fighter states, 17 actions, 324 RNG states). The aerial
  replay reaches the air-side motion but does not exercise its hit callback.
- Fighter scripts now support a general `on deal_damage:` callback, installed
  with `callbacks.track_deal_damage()`. A compiled Captain Falcon probe bound
  it through `on tick:` and deliberately changed command state on side-special
  contact; the stock comparison first differed at sample 256, confirming live
  dispatch. Binding only at side-special entry did not survive its contact
  state transition. Falcon Kick can bind at its own entry, as stock does;
  that move and its speed-after-hit behavior remain to be translated.
- An earlier contact comparison differed after the hit-state transition, but
  the current relinked client and repeated clone runs matched. The exact cause
  of that earlier binary's difference was not isolated. These are recorded-path
  checks, not proof of every stick angle, target type, aerial hit, effect,
  article, or fighter-specific callback. Up/down specials still delegate to
  stock C. Neither fighter is accepted as fully cloneable yet.

## 2026-09-27: stock fighter clone expansion

- Captain Falcon and Ganondorf now have independent stock clone bases. Their
  own fighter/animation/costume DATs create isolated drafts; script-only
  lifecycle modules build and package without declaring a special move.
  Isolated package registration reports 283 own-rig clips for Captain and 272
  for Ganondorf, plus five borrowed clips each. The normal costumes omit a
  material-animation root; the importer now handles that optional field.
  A generic part-animation script operation covers Ganondorf's knockback
  accessory animations. Their special states still inherit Captain Falcon's
  C callbacks, and death/side-special state cleanup is not yet scripted.
  Neither fighter is accepted as fully cloneable or gameplay-equivalent.
- Fighter scripts can declare `attribute float Name BYTE_OFFSET` before any
  moves and use `attrs.Name` in expressions. The compiler rejects duplicate,
  unaligned or out-of-bounds offsets. A compiled callback fixture reads a value
  from the fighter's imported extended-attribute storage and uses it at
  runtime. This keeps new fighter scripts from accumulating hard-coded
  Mario-structure references; Captain and Ganondorf special translations are
  still pending.
- The stock-clone draft command accepts Mario, Dr. Mario, Ganondorf and
  Captain Falcon as source fighters.
  The project and runtime lookup use a stock fighter definition, so Dr. Mario
  imports his own fighter, animation, and costume DATs instead of borrowing
  Mario's. His 303 action entries and 86 joints were checked against the
  imported data. Packaging and registration passed in an isolated content
  store, including 261 own-rig and four borrowed animation clips.
- Fighter scripts now have integer state and bounded integer RNG. A separate
  test verifies per-fighter state and the expected RNG bounds for choosing
  from a set that excludes the previous two results. A general projectile
  spawn operation and integer-exclusion choice support a first Dr. Mario
  script. It compiles, packages and registers with his own pill article.
- Dr. Mario is **not yet fully cloneable**. His stock taunt also creates pills
  and shares pill-color history with neutral special. The taunt still follows
  its stock callback while the new neutral-special script owns its own history;
  those paths must be unified before gameplay parity can be claimed. His
  other specials, hitlag, article lifecycle, and clone-vs-stock traces also
  need live comparison. The long-term target is every selectable vanilla
  fighter, accepted individually with independent assets and scripted unique
  behavior. Support for a fighter's DAT alone does not satisfy that target.
- The four-special script format can now append named `action Name:` blocks
  with `on state_NUMBER_animation|interrupt|physics|collision:` callbacks.
  A compiled fixture verifies that a selected callback changes on the copy
  of the motion table while the source table stays intact, and rejects a base
  whose motion range excludes the named state. This gives future stock ports a
  general place for taunts and other unique states, but does not yet supply
  the attached-pill spawn and item behavior needed by Dr. Mario's taunt.
- Both fighter script formats now accept `on damage:` and `on death2:` handlers.
  Scripts install them with `callbacks.track_damage()` and
  `callbacks.track_death2()` at the relevant state transition. Compiler checks
  reject an install with no corresponding handler. Module tests verify that
  these callbacks execute with the owning fighter's state. This is the general
  interruption/cleanup boundary needed by Dr. Mario's taunt and other fighter
  articles; Dr. Mario's attached-pill spawn and item-to-owner callbacks still
  need to move out of stock behavior before his clone is complete.
- The four-special format can also override `on load:`, `on tick:`,
  `on death:`, and `on landing:` in an action block. Its motion/lifecycle
  handler capacity is now 64, matching the already broader base-script path.
  A fixture checks both a selected motion callback and `on tick:` on the
  copied definition. These hooks allow a stock port to own initialization,
  per-frame decisions, and death cleanup without retaining those stock
  callbacks; article behavior itself remains to be translated.
- A custom fighter module can now supply an item logic table per article kind.
  The item engine selects that table when the custom fighter spawns the item,
  while continuing to load the article model and attributes from the fighter's
  own DAT. Module loading rejects missing callback tables and duplicate item
  kinds. The following script declaration pass builds on this runtime seam.
  Dr. Mario's pill has not been switched to an authored table yet.
- The stock-clone compiler now accepts the existing holdable-item declarations
  alongside four-special/action scripts, using the same parser as custom-base
  scripts. A compiled fixture dispatches a declared item's throw callback.
  The generated module hashes the original authored source, including item
  declarations. This prepares one module to own a fighter's
  moves and spawned items. The following pass adds stock-article logic tables.
- Fighter scripts can now declare an owned article kind, its indexed item
  states and animation IDs, each state's animation/physics/collision callbacks,
  and the item-wide interaction callbacks. The compiler emits one module-owned
  `ItemLogicTable` per declaration and binds it to the article kind. A compiled
  fixture checks the generated state table, callback dispatch and fighter
  ownership. The statement language remains small; Dr. Mario's pill needs
  additional general item operations before its seven states and taunt can be
  translated faithfully. This does not yet make him fully cloneable.
- Generic item operations now let authored article callbacks change a declared
  state, set both lifetime timers, decrement/read the live timer, and use the
  engine's reflection behavior. A module fixture executes these operations and
  compiler checks reject an out-of-range state transition. Dr. Mario's attached
  taunt pill and its collision/owner decisions still need more language support.
- Fighter scripts can now store an item's creation number in rollback-tracked
  state, query whether that live item still belongs to the fighter, and remove
  it through generic `articles.owned(...)` and `articles.remove_owned(...)`
  operations. The host rejects a stale number or an item owned by another
  fighter, including after normal reflection ownership transfer. An isolated
  module test covers dispatch and missing-host rejection; live item destruction
  and reflection parity still need runtime tests. The attached spawn and the
  pill's authored item callbacks remain open.
- A generic `articles.spawn_attached(...)` expression now creates a fighter-
  owned presentation article from an owned DAT asset and authored item logic,
  returns its live item number, and follows the original fighter origin. The
  item engine initializes separate script scratch before `on spawned`; a
  generated-module test checks the spawn arguments and host requirement. This
  does not yet replace Dr. Mario's taunt or demonstrate live pill parity.
- Item scripts can now inspect their original attachment owner’s motion and
  four animation command variables, with a guard against a missing or changed
  owner. They can edit their own four commands, collision mode, and ordinary
  world-interaction flag. A generated-module test covers those fields and
  ownership loss. Generic item stage-collision, bounce-test and sound calls
  now have generated-module coverage, including a missing-host rejection.
  Guarded owner-model-scale reads, item uniform scale and child-model
  visibility are also available and covered by the generated-module fixture.
  Texture selection and hit effects are still needed before Dr. Mario's pill
  can be fully authored or compared against stock gameplay.
- Article scripts now declare named float parameters at imported DAT byte
  offsets and read them as `item.attr.name`. A generated callback reads two
  fixture values; compiler checks reject duplicate offsets and unknown names.
  Import-time extent validation and the authored Dr. Mario spawn and callbacks
  remain open.
- Article declarations can now bind their named speed, angle and lifetime
  parameters to a generic projectile constructor. The new
  `articles.spawn_authored_projectile(...)` operation dispatches through the
  native host after the engine creates the item, preserving the stock pill's
  setup order. A generated-module fixture covers metadata and dispatch, while
  a special-move compiler check covers neutral-cue syntax. The current Dr.
  Mario script still uses its stock spawn, and live projectile parity is open.
- Stock-clone registration also copies fighter-kind callbacks beyond specials.
  Modules can now replace teardown, absorb, the four pickup/drop events, item
  visibility, knockback enter/exit, and special-attribute loading.
  `event.flag` is available only to
  pickup/drop event handlers. Generated module tests cover both script formats;
  the full native build and focused fighter-script tests pass. These hooks are
  only an authoring boundary: unoverridden callbacks still use the donor's C
  code, and every fighter requires its own audit and gameplay comparison.
- Shared fighter operations for item pickup/drop, item visibility, knockback
  enter/exit and extended-attribute copying are callable from action scripts.
  Dr. Mario's draft now authors these callbacks with the same parameters as
  his stock functions. A module fixture checks their dispatch and argument
  values. His death reset, taunt, attached pill and article logic remain stock,
  so this is a partial translation rather than full cloneability.
- The donor's mid-update and color-animation callback tables have distinct
  `on late_update:` and `on color_animation_reset:` script hooks. They bind
  directly at the original dispatch sites rather than running through the
  earlier generic `on tick:` hook. A module fixture verifies both generated
  callbacks. Kirby, Bowser and several fighters still inherit the stock
  implementations until their decisions are translated.
- Attribute initialization and reload are now separate script operations.
  Dr. Mario's load copies his DAT attributes into owned storage; later
  attribute reloads update the current storage without changing its pointer.
  The compiled callback fixture checks the reload dispatch. Initializing both
  vitamin-history slots to nine was an early incorrect translation; the later
  stock replay comparison above established that they begin at zero. Stock
  taunt still maintains a separate vitamin history from the scripted neutral
  move, so the combined path does not yet have gameplay parity.

## 2026-09-23: draft and registration foundation

- Fighter package registration now releases only the candidate slot when its
  bone mapping is invalid. A prior loaded fighter stays registered. The
  `bundle-fighter` probe loaded compiled Rook, rejected an invalid second
  instance, checked the original registration, then loaded a valid second
  instance. This exercised real Rook DATs and the current package cache.
- `nativeFighterProject_CreateMario` creates a separate draft directory with
  the original fighter DAT, animation bank and default costume DAT. Its
  manifest is written last; `nativeFighterProject_Open` rejects unfinished
  directories. `clone-stock-mario` reads those three archives directly from
  the configured USA v1.02 ISO. `inspect-project` reopens the draft.
- Two local drafts, Mario Lab A and Mario Lab B, reopened with independent
  paths. SHA-256 checks confirmed all six copied DATs match the corresponding
  original Mario samples byte for byte. This confirms source asset preservation
  at creation, not in-game clone behavior or edited DAT round trips.
- The changed `native_content_tool` target builds with the local Windows
  toolchain. Existing warnings in `cobj.h` remain unrelated to these changes.

- The Mario-derived runtime definition now imports the project's independent
  DATs, inherits Mario's stock callbacks and part mapping, and retains a
  separate custom identity. The character importer uses Mario's article map
  for the untouched clone, so fireball and cape descriptors are present.
  Early launch preflight validates the assets before the stock part table is
  available; final match registration requires the real part table.
- `bundle-project` built and cached Mario Lab A from its independent draft,
  and `accept` installed it in an isolated content store. Registration checked
  inherited callbacks, article descriptors, animation count, separate part
  table ownership, two simultaneous registrations, and cleanup. The existing
  compiled Rook bundle probe passed again after these changes.
- A direct Mario Lab A versus CPU Luigi match reached the results screen.
  Another launch captured a gameplay screenshot with the Mario clone's model
  and Luigi rendered on Final Destination. The results screen displayed the
  package title using a temporary native text overlay; the custom portrait
  and stock-style name lettering remain presentation work. The screenshot and
  process logs are local diagnostics, not a full move-by-move parity result.
- Mario Lab B was built and installed as a separate package. A direct A-versus-B
  match reached the results screen, exercising two distinct custom package IDs
  in one game process. The imported Mario effect bank appeared in the startup
  log. The internal game-canvas screenshot still shows stock fallback result
  labels and empty portraits; the client shell supplies temporary custom names
  over that canvas.
- Four focused fighter asset tests passed: common data, character DAT, costume
  DAT, and animation bank.

- Mario draft common attributes now have a read/edit/save path. The first
  properties are walking speed, gravity, terminal velocity, weight and model
  scale. Edits validate the resulting DAT record and save `fighter.dat` through
  an atomic replacement. The `project-attribute` tool exercises this path until
  the in-client inspector uses it.
- A fresh Mario Lab B draft was changed from 1.1 to 1.4 model scale, saved,
  reopened and built as a new package ID. Mario Lab A remained at 1.1 with its
  original fighter DAT hash. An attempted NaN edit was rejected without
  changing B's saved hash. The rebuilt B package installed, and a direct
  A-versus-B one-minute match reached the results screen. This proves the
  independent edit/save/build/load path for that attribute; it does not yet
  prove the editor UI or all Mario moves. The same four focused asset tests
  passed after this change.
- Disc archive reading now has one native implementation used by the command
  tool and fighter draft creation. The new `CreateMarioFromDisc` path expects
  an already-open game disc, so the client workshop can use the same asset copy
  operation as the tool without starting a second disc session. A new draft
  created through that path reopened with the expected three archive sizes and
  built into a cached package after the refactor. It is not wired to the client
  UI yet.
- Registered custom fighters now receive separate copies of their motion-state
  tables at the match transition. The Rook bundle probe and Mario draft probe
  verified that two registrations have distinct table addresses with identical
  initial contents, and that neither table aliases its compiled template.
  This removes one live shared table dependency; it does not make the inherited
  Mario callbacks or package behavior fully independent yet.
- Collection now opens an in-client workshop page. It lists installation-local
  draft manifests without loading their full DATs, creates a Mario draft from
  the already-open game disc, and edits/saves the five common attributes through
  the same validated DAT path used by the command tool. The game presentation
  shrinks between the library and inspector rails and returns to its normal
  layout on exit. While the page is open, local start requests are cancelled
  and the room publishes no ready fighter selection; this gating still needs
  a two-client live check.
- A live home-CSS run opened Collection and the workshop, created "Mario
  Workshop Probe", set model scale to 1.4, saved it, and confirmed 1.4 from the
  on-disk draft with `project-attribute`. An invalid NaN edit left the DAT hash
  unchanged. Window captures showed the game viewport between the two workshop
  rails, and Back restored Collection and the normal home layout. A separate
  live run entered home practice, opened the workshop there, and selected the
  saved draft while Dr. Mario and Sandbag continued to render and simulate in
  the smaller viewport. This is a draft authoring slice, not the intended clone
  preview: opening from home CSS still shows CSS, and preview apply, moves,
  animation and script editing are not wired to the page.
- The workshop now accepts a PNG portrait and builds, caches, and installs the
  selected saved draft through the native package store. A live UI run used a
  generated test portrait and an isolated `--content-root`; the install created
  a verified cache package and installed marker, Collection listed the new
  fighter and its package ID, and Edit CSS's Add fighter list showed its name.
  The existing installation-wide CSS layout was deliberately not changed by
  the test. This proves the in-client save/build/install route for a Mario
  draft, not a complete authoring or live preview loop. The installed package
  still uses the provisional `mario_clone` behavior binding.
- A direct local practice run loaded that exact edited package ID and rendered
  its Mario model with the changed 1.4 model scale beside Sandbag. A recorded
  four-B-input sequence replayed all 12,023 controller samples with matching
  fighter-state and RNG checks. New opt-in `--trace-actions` diagnostics tied
  each motion change to a replay sample: neutral B entered motion 343, side B
  345, and up B 347. The initial down-B edge arrived during up-special
  recovery, so a separate recording exercised down B at motion 350 and 349;
  its 3,828 samples also replayed without divergence. Sandbag entered a hit
  motion after the neutral-special input. This establishes that four special
  state families can be entered and replayed in this clone, not their full
  stock parity, hitbox accuracy, CPU behavior, Kirby copy, or match-end paths.
- Package validation now imports a candidate into detached storage before
  touching the live fighter registry. The command tool validated a built Mario
  package both before and while a registered instance occupied the first live
  slot, with its fighter-data and motion-state pointers unchanged. A separate
  corrupted fighter DAT failed validation before installation. The workshop's
  Build & install path also runs this check before creating an installed marker.
- The in-client Preview in practice button builds from current form values,
  caches and validates the candidate without saving the draft or installing
  the package, and restarts home practice with that fighter. In a live run,
  the saved draft remained at model scale 1.4 while the unsaved 1.8 preview
  displayed an enlarged Mario in practice. Exiting the workshop restored the
  previously selected Dr. Mario on CSS. The isolated content root still held
  only its earlier installed package. The editor overlay stayed visible during
  both scene transitions. This covers a common-attribute preview, not hot
  swapping a fighter inside an active frame or editing moves/animations.

Mario clone audit, partial and source-based:

- Registration currently copies Mario's callback table into a new fighter-kind
  slot and assigns the clone its own imported fighter DAT, animation bank,
  costume, and motion-state table. This keeps asset edits local, but the
  `mario_clone` code key and `source_kind` still select inherited compiled
  behavior rather than package-owned behavior.
- The neutral-special fireball branch, landing reset, two common action
  branches, and three CPU branches explicitly use `behavior_kind`, so the
  current Mario-based clone takes those Mario paths despite its distinct kind.
  The four special-state families were entered in recorded practice; their
  timing, articles, hitboxes, interruptions and stock parity are not proven.
- Kirby's copy dispatch and hat/resource tables still use stock fighter-kind
  indexing. A custom kind has no audited copy ability or dedicated hat entry.
  This remains a fidelity gap rather than a reason to treat the clone as Mario.
- The next behavioral matrix must also cover full normals, throws, item and
  cape ownership, CPU recovery/attack choices, death/respawn and results. A
  data-only editable normal should be proven before broadening the workshop UI.

Original-action editing checkpoint, backend only:

- A source-linked action walker now enumerates all 303 Mario actions from the
  copied fighter DAT and follows the same original command widths, branches,
  subroutines and termination rules as the native importer. All 303 action
  graphs parsed from the isolated Mario draft. It exposes frame timers and
  hitbox-damage commands for editing; it does not flatten control flow into a
  guessed timeline or translate a normal into behavior script.
- A command edit requires the chosen action to reach that exact source offset.
  The backend rejects commands reached by multiple actions until an explicit
  make-local operation exists. The `project-actions` and `project-event` tool
  commands exercise the same backend without an editor UI.
- In an isolated copy of the draft, Mario's Attack11 first hitbox changed from
  3 to 9 damage. Reopening showed the changed field with the other two hitboxes
  still at 3; a byte comparison against the original draft found exactly one
  changed byte at file offset `0x3FB3`. A wrong-action edit was rejected. The
  edited draft bundled, validated, cached and installed in an isolated content
  root. A direct native practice-stage match loaded it as a separate fighter;
  the action trace reached P1 motion 44 / animation 46 and P2's hit state.
  This proves loading and executing an edited normal, not precise on-hit damage
  parity or the complete save/reopen/undo workflow.
- A temporary in-client action list confirmed that the archive names, filter,
  event values and pending edits rendered correctly. It was removed after the
  proof run. Per the user's direction, core authoring, module, animation and
  copy contracts come before collaborative interface design.
- The headless action backend now reads and writes the complete five-word
  CreateHitbox record: damage, radius, signed offsets, bone, angle, three
  knockback values, element, shield damage, sound and collision flags. Values
  stay in the original 1/256-unit integer encoding; the hitbox ID remains
  read-only because later commands may refer to it. Bone indices and encoded
  ranges are checked before writing. Editing an isolated draft's first jab
  hitbox changed only the expected five DAT bytes for damage, radius and angle.
  Negative offsets and shield damage reopened correctly; an out-of-range bone
  was rejected. All 242 reachable Mario hitbox occurrences completed a
  read/write no-op round trip with an identical fighter DAT SHA-256, and the
  changed draft bundled and validated again. This validates archive encoding,
  not every hitbox's gameplay result.
- Saving fighter DAT edits now retains the immediately previous valid file.
  `project-undo` swaps the saved draft with that previous state, so repeating it
  toggles undo/redo. An isolated draft changed hitbox damage 9 to 10, restored
  the exact previous SHA-256, preserved redo through a no-op save, and toggled
  back to the changed SHA-256. This is one saved fighter-DAT revision, not yet
  multi-step in-memory undo or an atomic transaction across animation, costume,
  script and portrait files.
- A native Windows module boundary now loads a separately compiled fighter DLL,
  resolves its definition and motion table, checks its declared per-instance
  state size, and unloads it. A CMake fixture module was loaded twice. Its
  callback wrote to two distinct fighter state blocks (2 increments and 1),
  then both instances were destroyed before the DLL unloaded. The state block
  is stored in the native Fighter allocation and initialized at fighter load.
  All 361 native tests pass after that struct change. This is a compiler/ABI and
  instance-isolation proof only: modules are not yet bound into a live match,
  replaced across rollback history, or generated from scripts.
- Fighter drafts can now optionally contain `module.dll` and declare its own
  behavior `code` in `project.txt`. The `package-project` command loads the
  DLL to check its exported definition, code key and source kind, then stores
  the exact DLL bytes alongside the existing fighter assets. A package with
  different module bytes gets a different content ID. An isolated Mario draft
  packaged the 12,800-byte test DLL; the cached package contained byte-for-byte
  identical module data. The content-store tests cover both data-only and
  module-carrying packages, including a malformed module length; all 361 native
  tests pass.
- Runtime registration now reads the package before resolving its behavior.
  With a module present it materializes the verified bytes under the content
  ID, loads the DLL, checks its code key and imported assets, and retains the
  library while the fighter callback tables are registered. The loader checks
  an existing materialized DLL byte-for-byte before using it. The isolated
  package passed detached validation and full registration/release using the
  original `PlCo.dat` from the ISO; the original data-only Mario draft still
  passed its complete bundle/import/registration check. This is not yet a
  gameplay run of a module-authored move: the current DLL fixture has only one
  motion state, sufficient for the initial callback lifetime test. Later work
  below supplies the full inherited Mario state table.
- The module entry point can now receive the stock base definition. The test
  DLL copies the complete Mario motion table and other inherited definitions,
  gives the clone its own code key and one byte of instance state, and replaces
  its neutral-special callback. The packaged version passed import, full
  registration, callback invocation against an isolated fighter instance, and
  release. It was explicitly installed in an isolated content root and a
  direct native one-stock match against CPU Luigi reached results. The result
  screenshot is `screenshots/melee-1790194962-34140-0000.bmp`. This proves a
  module-backed clone can complete a match, but the run did not trigger or
  observe its replacement special callback in gameplay, and the results UI
  still displayed an incorrect stock name/empty portrait for the custom slot.
  The module lifetime fixture now places two Fighter allocations in a protected
  rollback page region, invokes the DLL callback in separate frames, rewinds,
  and checks that their independent state bytes return exactly to 2 and 1.
  This proves that the page snapshot mechanism can restore module instance
  state. It does not yet prove that the live match's region inventory includes
  every fighter allocation, or that a real correction resimulates a module
  move identically. Safe mid-session module replacement remains open.
- A first readable move-script slice now parses a named move with ground and/or
  air neutral-special entry handlers and a facing-relative speed operation.
  It rejects unsupported syntax and nonfinite/out-of-range speeds with source
  line diagnostics, generates C, and `script-build` uses the exact CMake-selected
  compiler to replace a draft's DLL only after a successful build. A malformed
  script failed at line 3 and left the prior DLL hash unchanged. A two-handler
  script compiled in 250 ms on this machine, packaged with the Mario draft,
  and passed native asset registration. A CMake test regenerates and compiles
  that script into a DLL, then confirms that the ground callback changes
  `gr_vel` by facing-relative 0.8 while the air callback changes `self_vel.x`
  by facing-relative 0.4. The untouched velocity fields remain zero. All 362 native
  tests pass. This tiny subset does not yet handle state transitions, cues,
  articles, cleanup, or readable stock templates.
- A successful `script-build` now keeps `behavior.fms` beside the draft DLL.
  Fighter packages carry those exact editable source bytes after the module,
  and the content ID covers both. The isolated script draft packaged as
  `e96880199fd39cc674b9139f85b7307581e24bb76c5653f1ed226f3198d7961b`;
  extracting the source from its cached package matched the draft byte for
  byte. Invalid script input preserved both the previous DLL and saved script.
  The original data-only Mario bundle still validated, and all 362 native
  tests pass. Script/DLL replacement spans two filesystem moves, so crash
  recovery or a single atomic multi-file draft transaction remains open.
- A direct practice launch loaded an isolated package containing the compiled
  script and reached Mario plus Sandbag in the grid room. A live visual movement
  comparison was inconclusive because user input occurred during the run; the
  generated callback test above is the current evidence for the speed operation.
- Generated modules now export a fingerprint of their exact source bytes.
  `package-project` compares it with the saved `behavior.fms`, rejecting a
  stale script/module pair and asking for a rebuild. A compiled draft packaged
  successfully, while appending a harmless comment to its saved source caused
  packaging to fail; restoring the source restored the match. This detects a
  split save but does not yet make the two-file replacement crash-atomic.
- Installed Mario-based fighter bundles can now be copied back into a local
  draft with `clone-installed`. The draft owns copies of its fighter, animation,
  costume, portrait, module and available script bytes; the manifest is written
  last so incomplete copies are not listed as valid projects. An isolated-store
  test cloned the installed script fighter, matched all six source-file hashes,
  changed the clone's weight from 100 to 101 while the parent remained 100,
  built a distinct content ID, and verified its native registration. The copied
  module keeps its behavior code key, but the draft has no live link to the
  parent package. This path currently requires the `ftDataMario` archive root;
  other fighter bases and opaque module editing still need explicit support.
- The first script subset can now explicitly call `stock.enter()` before its
  facing-speed change. The Mario base exposes its original ground/air neutral
  entry callbacks to the generated module; a unit test checks both callbacks
  run and rejects a missing inherited entry. An isolated compiled package
  reached the practice stage. Injected B entered Mario motion 343 twice, and
  a framebuffer capture showed the stock fireball while the authored ground
  speed moved Mario. The 5,119-sample input recording replayed through both
  specials with matching RNG and tracked fighter-state digests. This verifies
  a playable inherited move plus a script adjustment, not yet a fully authored
  special or live rollback correction through the callback.
- An exact-client module host now supplies the original motion-state change and
  animation-start functions directly. The entry export changed to
  `FreakMelee_GetFighterModule`; older draft DLLs must be rebuilt. The Mario
  base declares its ground/air neutral motion IDs, and a script may use
  `state.enter(NeutralSpecial)` instead of the inherited entry. The generated
  callback clears the stale accessory slot, starts the named state through the
  game's normal scheduler, and applies the authored speed. A mock-host test
  checks both state IDs, field resets, animation start, velocity changes,
  missing-host rejection and that stock entry is not called. A live practice
  run entered Mario motion 343 without a fireball; its 3,653 recorded inputs
  replayed through the new callback without tracked fighter-state or RNG
  divergence. This was state entry only; the inherited Mario collision callbacks
  could rearm the fireball handler when changing between ground and air.
- Authored neutral specials now copy the base motion table and own the ground
  and air collision callbacks. They retain the original collision checks,
  state-change flags and animation frame while clearing the inherited fireball
  accessory on either transition. The fixture verifies both directions without
  changing the base callbacks. A live practice input recording crossed from
  aerial state 344 to grounded state 343 at animation frame 25; its 6,092
  samples replayed without tracked fighter-state or RNG divergence. All 363
  native tests pass. This proves one live landing path, not article ownership,
  authored cues, interruption cleanup, live rollback correction or non-Mario
  state maps. Generated DLLs from the previous host contract must be rebuilt.
- The action backend now reads each original 24-byte action record's animation
  offset, size and raw flag word, and can edit the flag word while preserving
  the two source bits the native importer does not consume. An isolated Mario
  draft changed action 44's flag from `0` to `0x80000000`, then reopened with
  the expected word; exactly one fighter-DAT byte changed. A write touching a
  reserved bit was rejected, the edited bundle registered and released, and
  disk undo restored the original word. All 363 native tests pass. The flags
  are still exposed in source bit positions; a semantic UI for named flags and
  interrupt rules remains open.
- Readable authored neutral specials can now handle `on neutral_special_cue:`
  with a facing-relative speed impulse when the original subaction raises its
  throw-flag cue. The generated accessory callback consumes the cue once,
  follows the current ground/air state and is restored after either collision
  transition. The callback fixture checks both orientations, no-cue frames,
  cue consumption and a cue after each transition. A live package completed
  grounded and aerial specials in practice; its 3,009 recorded controller
  samples replayed with matching tracked state and RNG. All 363 native tests
  pass. The live trace proves state entry and transition, while the fixture is
  the direct evidence for the cue impulse. This does not yet author an article,
  expose multiple cue operations or independent move interruption cleanup.
- The Mario base now exposes its original neutral-special article operation
  separately from move entry. An authored cue can explicitly call
  `stock.neutral_article()` before its speed impulse, while the generated
  callback still owns cue consumption and remains installed after a ground/air
  transition. The fixture verifies article invocation once per cue on two
  fighters and rejects a base without that operation. A live Mario-based
  package showed a fireball and Sandbag's hit state after grounded neutral,
  then another hit after aerial neutral landed; all 1,712 recorded samples
  replayed without tracked-state or RNG divergence. All 363 native tests pass.
  This reuses Mario's immutable stock item constructor and article asset; it
  is a playable template, not package-owned custom article support.
- A script cue can now use `state.once()` to consume at most one cue per move
  across ground/air transitions. Its one-byte state belongs to each Fighter's
  rollback-tracked allocation. The generated code uses Melee's existing
  `x21EC` motion-change callback to retain the marker on the paired
  ground/air transition and clear it when leaving the move, including an
  interruption. The fixture verifies two independent fighter instances,
  duplicate suppression, transition preservation, exit cleanup and a fresh
  cue on re-entry. A live package fired at Sandbag on two separate neutral
  specials after returning to Wait; all 672 recorded inputs replayed without
  tracked-state or RNG divergence. All 363 native tests pass. The fixture
  directly proves the marker logic; live rollback correction over the marker
  and article creation/destruction is still open.
- The corrected-rollback diagnostic now accepts a target frame in the recorded
  match (240-591), keeping frame 240 as its default. Against the authored
  `state.once()` package, late P1 corrections at frames 493 and 501 around the
  first neutral special each restored and twice resimulated an eight-frame
  interval to an identical complete tracked-state hash. Both runs reconciled
  two audio starts. This is live corrected-resimulation evidence for the
  scripted fighter, but the probe does not independently label the article's
  exact birth/destruction frame or yet test four authored fighters.
- The original action walker now identifies later hitbox-scale and hitbox-flag
  commands, plus read-only interrupt and neutral-special cue markers at their
  source offsets. A scale/flag edit keeps the hitbox ID fixed; a flag edit can
  change only its type and enabled bit. On isolated Mario Attack13, changing
  the later scale command from 900 to 1000 changed exactly one DAT byte, and
  the edited package registered/released successfully. Disk undo restored 900.
  Mario SpecialN's cue and Attack11's interrupt marker are now visible in the
  action listing. All 363 native tests pass. The altered scale was not yet
  checked against a target in live gameplay; Mario's corpus did not exercise a
  later hitbox-flag command, so that setter still needs a fixture or another
  supported fighter.
- The animation backend now opens a Mario draft's source action clip from its
  independent animation bank, exposes joint/channel track order and encoded
  keys, and evaluates transform channels through the game's FOBJ interpreter.
  An isolated SpecialN joint-3 translation-X track changed key 5 at frame 11
  from 0.118713379 to 0.25. Exactly two bytes changed in the 1,259,328-byte
  bank. Native import still accepted all 252 used clips, and the evaluator
  produced 0.25 at frame 11 and 0.178985596 at frame 11.5 (the unedited
  frame-11.5 value was 0.0805206299). The edited draft built and registered
  as package `b62d71aa9c0ad044a7adc159246d320cbfe5e3cc7dfe888d7db61220c11fd0da`.
  Unrepresentable quantized input and a tangent-only key were rejected; a
  separate animation-bank undo restored the original SHA-256. The edit path
  preserves unrelated encoded bytes and does not yet insert/delete keys,
  change interpolation/timing, edit several clips transactionally,
  or compare full poses and hitbox attachments.
- The same source-linked path now exposes spline tangent values and edits their
  original quantized bytes without re-encoding the stream. In an isolated
  SpecialN track, changing key 0's tangent from -0.000793457031 to 0.25
  changed exactly two bank bytes. Melee's FOBJ evaluator changed its frame-1
  sample from -0.0168457031 to 0.0458526611; the changed package also
  registered. An unrepresentable tangent and a non-spline key were rejected,
  and disk undo restored the original bank. The round-trip integration test
  now covers both key values and tangents. Timing, key insertion/removal and
  interpolation mode changes still require safe stream re-encoding and bank
  offset updates.
- The first timing operation moves an existing timed key strictly between its
  immediate timed neighbors by rewriting their two wait bytes, preserving the
  next key's frame and the clip's duration. Moving SpecialN track 0 key 5 from
  frame 11 to 12 changed exactly two bank bytes; Melee's evaluator gave
  0.0593566895 at frame 11 instead of 0.118713379. Native import accepted the
  bank and the edited package registered. The command rejected a move onto
  the next key and a tangent-only neighbor; undo restored the original bank.
  This deliberately excludes insertion/removal, retiming across packed-width
  boundaries, and multi-key retiming; those need a clip re-encoder and offset
  repair before they can be safe.
- The move language no longer requires a dummy zero-speed operation when a
  handler only enters a state or an animation cue only emits Mario's inherited
  article. The compiler keeps entry, once, article and speed in their explicit
  engine order. A generated article-only module entered Mario's ground and air
  neutral states, emitted one article per move, reset its per-fighter cue marker
  on exit, and left velocity unchanged in the callback fixture. The separate
  draft built that script in 234 ms, packaged it as
  `17282aa0e3acb1e60ec5907f38692362d76a22247f4f0624b3759c297b2f1e07`,
  and registered/released against the ISO. This verifies the backend operation,
  not a new article definition or live gameplay with this exact package.
- Original SetCommandVar subaction commands are now listed with their variable
  index and editable 24-bit value at the original source offset. The editor
  preserves the opcode and index and still refuses to edit a command shared
  by several actions without a make-local copy. In an isolated Mario draft,
  changing SpecialS's variable 1 at `0x3538` from 1 to 0 changed exactly one
  fighter-DAT byte. The edited package `032533f397fb0956ebf5a2300ace9c264d2c44dc031b644324301a2e503c35f1`
  registered and released; an out-of-range value was rejected without changing
  the saved DAT, and undo restored its original SHA-256. All 303 Mario action
  listings and six targeted action/script/animation tests passed. This proves
  source editing and import, not the modified side special's live behavior.
- The script compiler now accepts a single stock side, up, or down special per
  script, with optional ground/air facing-speed adjustments after the original
  entry callback. Mario's clone definition exposes the six stock callbacks
  explicitly so those generated modules can bind them. A compiled-module test
  exercises all three button families, both ground and air callbacks, stock
  invocation, facing and velocity direction, untouched neutral callbacks, and
  refusal of a missing base entry. Isolated Mario drafts built and registered
  one package for each family (side `7340e4578cc9833d86ccac4df756ad8d494e861a864dc710b27134425ec4ef9b`,
  up `7f378bd39fd68b9cd5e8faec1d80f2f75e095fd4747d4f8e726f895f47afb166`,
  down `b766051ad510851b3f70c7f4dc5cec1bf07b5905841d705655819b0cc4c5884c`).
  The parser rejects mixed special families, non-neutral authored state entry,
  and a non-neutral handler without `stock.enter()`. This is a stock behavior
  modification path, not fully authored side/up/down state or live match proof.
  The release build and all 366 native tests pass.
- The original action inspector also exposes body collision state, all-bone
  hurt state, and targeted bone hurt state commands. State edits accept only
  the three values handled by the original functions; the targeted command
  keeps its bone ID fixed. Mario's action listings exposed 51 body-state
  commands and two hurt-state commands, with all 303 actions still traversing.
  In isolated drafts, EscapeN body state 2-to-1 and AttackHi4 bone-23 hurt
  state 2-to-1 each changed one DAT byte. Both edited packages registered
  (`59a6e50fe629e628e74526175534c9d58f7e6e5e04104eef91333d8bd3e56d9d`
  and `3c4cbd3d0d338bf522042ddcc95818fcd14144340294c0f0b73802de9d4122d1`);
  out-of-range values were rejected and undo restored the original DAT hash.
  Mario AttackHi4's all-bone reset also changed by one byte and undid, though
  it was not separately packaged. The release build and all 366 native tests
  pass. Live collision behavior remains untested.
- A first move-transfer backend now copies a same-index, stock-shaped normal
  action between independent Mario drafts. It traverses every reachable source
  command (including currently uneditable opcodes), rejects shared commands or
  clips and differing pointer/topology layouts, then copies the command graph,
  complete FigaTree clip and animation flags. The source and destination must
  have the same archive geometry and no native behavior module. In a real
  Attack11 transfer, source hitbox 0 damage changed from 3 to 8 and one
  animation key changed to 0.25; the destination's fighter and animation files
  then matched the source byte for byte and package
  `9deb5af1c2a126bdc1bae05c3edff570bf4d8ba85c5bed963c49612c3d1574a3`
  registered and released. A later source edit left the destination unchanged;
  separate fighter/animation undos restored their stock SHA-256 values. The
  integration test also rejects self-copy, special-action copy and a mismatched
  command opcode, and preserves an unrelated destination weight edit. The
  release build and all 367 native tests pass. The stock Mario inventory has
  19 normal action slots in this range with complete, exclusive command and
  clip data; eight other slots have no independent action script or clip.
- The same checked transfer now groups the three jab steps or three high,
  straight and low variants of forward tilt or forward smash. It prepares all
  three copies in private draft buffers and changes the destination in memory
  only when all succeed. A source with edited Attack11/12/13 commands and
  Attack11/12 clips copied into a second draft byte-for-byte as package
  `8da73cc5f24f72c83bfb99e810dcd7b0a1b9f4dc2285c73c148f0303b0ec5df8`,
  which registered and released. The integration test covers all three groups,
  rejects a self-copy and unknown group, and restores both destination file
  hashes through undo. This does not yet transfer arbitrary multi-state moves,
  different archive layouts/skeletons, behavior code, new articles or effects.
  The fighter and animation files are still saved sequentially, so a crash
  between their disk commits needs recovery.
- One readable script can now compile up to four separate move blocks, one per
  special family. The mixed fixture combines an authored neutral fireball cue
  with stock-entry side, up and down callbacks. Its generated DLL test exercises
  neutral state/article behavior, all three stock entry callbacks, signed
  facing-relative speeds, untouched air callbacks, whole-source hashing, and
  rejection of missing stock entries, duplicate specials, a fifth block and
  mixed families. An isolated Mario draft compiled the combined module in
  438 ms; its package
  `dbd5f8b36b16f7bd3979f71943777074a037075b4f295eb9d2b3ee681c954289`
  registered and released against the ISO. This proves module composition and
  package loading, not live move fidelity or authored side/up/down states. The
  `bundle-project` at this checkpoint still assumed Mario's stock callbacks;
  it was generalized later. `package-project` plus `verify-package` already
  supported this scripted draft.
- Original opcode-17 fighter sound commands are now source-linked in the
  action listing. The backend reads their behavior, SFX ID, volume and pan,
  and edits the latter three fields for behaviors 0-6 while preserving the
  original opcode, behavior and padding. The normal `project-event` path
  refuses these multiword commands; `project-sound` owns them. In a fresh
  Mario draft, Attack11 sound at `0x3FCC` changed from ID 166/volume 127/pan
  64 to 167/100/50 with exactly three DAT bytes altered; wrong-action and
  out-of-range edits left the DAT unchanged, and undo restored its SHA-256.
  Package `d5afc077b84a3d00a5846f085dcc8dc13d7638989b248df2763ad270ae0b1ee6`
  registered and released against the ISO. Canned behaviors 10-15 remain
  read-only, and random/stage SFX opcodes are not yet editable. Sound bank
  existence and audible in-game playback of this edited ID remain unproven.
- Original opcode-10 five-word fighter GFX spawn commands are now visible by
  source offset, with typed effect ID, bone, lifetime flag, signed offsets and
  ranges. The setter preserves the command's unidentified low bits and raw
  auxiliary value while checking bone and field bounds. A fresh Mario draft
  changed AttackS4S's effect at `0x43E4` from offset Y 0 to 256 in exactly one
  DAT byte, rejected wrong-action/out-of-range/bone edits, and undid to the
  original SHA-256. Package
  `08e0ae436334db139f8f8d93bd041e01ab24ef048f03158518eb6152c9bff2c0`
  registered and released against the ISO. This does not yet import new effect
  assets, validate every GFX ID, or prove the visual result of the edited spawn
  in a live match.
- The full `bundle-project` registration probe now checks the module's bound
  callbacks and motion table instead of assuming every clone retains Mario's
  stock callbacks. A new integration test creates a fresh Mario draft in a
  path containing spaces, compiles the four-special script, bundles it into
  two distinct custom fighter slots, and verifies its package against the ISO.
  It then edits the saved script without rebuilding: both package and bundle
  commands reject the stale DLL/source pair. Rebuilding produces a different
  valid package, and a data-only Mario bundle still passes the same probe.
  The test also caught and fixed Windows compiler arguments being split at
  spaces in project paths. This validates registration and consistency, not
  simultaneous live gameplay for the two scripted fighters.
- Normal move copying now includes each of Mario's five aerial attacks with
  its separate landing action. An isolated down-air source changed the attack
  and landing hitbox damage and a landing animation key; copying the pair made
  both destination DAT files byte-identical to the source. Its package
  `49c224ca6549c7e5b9bc137aad8ee36fb44bd4cfcaba258813e258f5bd43c557`
  registered and released against the ISO. The integration test covers all
  five aerial groups, reads the changed values, and restores both destination
  files to their original SHA-256 values through undo. This remains limited to
  same-layout, data-only Mario drafts with exclusive command graphs and clips;
  it does not copy native code, new article/effect assets, or different
  skeletons. Fighter and animation files are saved sequentially on disk, so a
  crash between commits still needs recovery. Live attack/landing behavior is
  not yet proven.
- Original opcode-34 throw-hitbox commands now appear at their source offsets
  with hitbox ID and damage. The event setter changes only the 23-bit damage
  field, retaining the ID and following angle, knockback, element and sound
  words. Mario's action 247 command at `0x5B48` changed damage 9 to 15 in
  exactly one DAT byte; wrong-action and out-of-range edits left the DAT
  untouched, and undo restored its original SHA-256. The remaining throw
  fields and live throw behavior are still untested.
- A scripted Mario package now passes a distribution and recloning test with
  the author's project directory removed. The test installs and exports the
  original package, caches and installs it in a separate local store, creates
  an editable draft from that installation, and compares fighter, animation,
  costume, portrait, module, and readable script bytes. Editing the recipient's
  weight produces a distinct package while the first installed build still
  verifies; the recipient can rebuild its included script with only its own
  draft. This proves local source and asset portability for that scripted
  Mario path, not handwritten-C source packaging or live match behavior.
- A direct one-stock Battlefield match loaded the packaged scripted Mario from
  an installed local store against CPU stock Mario, reached the results screen,
  and returned to CSS after a control-channel START. The recording is
  `build/native/clone-live.fmr`; the results viewport capture is
  `screenshots/melee-1790208662-68724-0000.bmp`. P1 was idle and lost, so this
  proves a complete match path for the loaded package, not special-move or
  article behavior. The game-only screenshot shows the original fallback name
  texture; the native shell draws custom titles separately, and their final
  composite presentation remains for later UI review.
- The Mario clone audit found that the inherited OnLoad callback wrote its
  imported fireball and cape articles into the global original item-kind
  table, allowing a clone to replace stock Mario's item data. Native custom
  Mario now leaves those slots owned by stock Mario. During a custom fighter's
  item spawn, the item takes its article from that fighter's own imported
  `ftData` for the fireball or its configured cape kind. A focused test runs
  stock and two custom Mario OnLoad calls, keeps stock globals unchanged, and
  resolves distinct fireball/cape articles for each clone. The full 374-test
  native suite passes. In a separate match without stock Mario, the clone
  entered its neutral-special motion (sample 6113 in
  `build/native/clone-fireball-live.log`) and continued playing for thousands
  of frames. In a second rendered match on Final Destination, the user saw the
  projectile spawn while the trace recorded Mario's neutral-special motions
  343 and 344. This supports the live spawn path without proving which article
  pointer the item used or exact projectile parity. Mario package loading now
  requires both imported article slots, and a mapped but absent custom article
  cannot silently fall back to stock Mario's global slot. The full 374-test
  suite passes. Simultaneous stock/clone item identity and other article
  families still need direct checks.
- A four-player Battlefield run used the same installed Mario clone for idle
  human P1 and level-nine CPU P2, with stock CPU Mario P3 and Zelda P4. It
  reached the results screen (`screenshots/melee-1790210490-18544-0000.bmp`).
  The action trace includes the CPU clone's cape motion 348 and stock Mario's
  neutral-special motion 343; it does not directly identify the spawned item
  article. Playback of `build/native/clone-mirror-stock.fmr` ended after all
  11,297 recorded controller samples with no state/RNG divergence and exit
  code zero. This covers simultaneous fighters and one CPU clone, not two
  different installed clone packages or per-project article identity.
- The installed scripted Mario was then recloned into a second project, given
  weight 126 and packaged as a different installed ID. A four-player
  Battlefield match loaded the original clone as human P1, the edited clone as
  level-nine CPU P2 (runtime kind 35), stock Mario P3 and Zelda P4. It reached
  results (`screenshots/melee-1790210763-40992-0000.bmp`); the action trace
  includes P2 cape motion 348 and stock Mario's fireball motion 343. Playback
  of `build/native/clone-distinct-stock.fmr` ended after 14,021 samples with
  no tracked state/RNG mismatch and exit code zero. This proves two different
  installed Mario packages coexist through a full match, but does not yet
  inspect their article pointers or compare the visible project edit in play.
- The semantic replay/rollback digest now reads the stock behavior family for
  a custom fighter, including Mario's tornado charge and cape item relation,
  and hashes its fixed authored module-state bytes. Focused tests change each
  field and confirm that the digest changes. A fresh four-player match with
  two distinct installed Mario packages, stock Mario, and Zelda reached
  results (`screenshots/melee-1790211276-70752-0000.bmp`); all 18,849 recorded
  samples in `build/native/clone-distinct-state-digest.fmr` replayed without
  a tracked state/RNG mismatch. This checks the expanded digest and recorded
  playback, not live rollback correction or complete authored-state ownership.
- Forced late-input rollback now runs with installed scripted Mario packages.
  A four-fighter time match used both distinct clones, stock Mario, and Zelda;
  `build/native/clone-forced-correction.fmr` reproduced the complete tracked
  endpoint state after corrections at frames 240 and 590. A separate two-clone
  recording used the native control channel to press P1 B at samples 232-233.
  The action trace confirms the original clone entered its scripted neutral
  special (motion 343) at sample 233. Replays of
  `build/native/clone-scripted-active.fmr` with late P1 corrections at frames
  245 and 250 both passed the repeated corrected-timeline comparison over
  134,733,860 tracked bytes; the logs are
  `build/native/clone-scripted-active-probe.log` and
  `build/native/clone-scripted-active-probe-250.log`. This establishes local
  correction while an authored special is active; the targeted state and
  projectile observations are recorded below. It is not a networked rollback
  acceptance test.
- The correction probe now observes P1's authored module marker, the number of
  Mario fireballs, and whether each fireball's owner is P1. It compares those
  observations at every corrected frame in both resimulation passes, alongside
  the existing complete tracked-state hash. In the controlled two-clone
  recording, P1 entered scripted neutral special at sample 233; the original
  timeline acquired marker 1 and one P1-owned fireball at frame 244, reset the
  marker at frame 274, and removed the fireball at frame 319. A forced
  correction from frame 240 recreated marker 0 -> 1 and P1-owned fireballs
  0 -> 1; a correction from frame 315 reproduced the fireball's 1 -> 0
  removal. Both probes exited successfully with identical first and second
  corrected timelines (`build/native/clone-owned-fireball-240.log` and
  `build/native/clone-owned-fireball-315.log`). All 374 native tests pass.
  This covers the package's authored callback using Mario's inherited article
  constructor. New package-defined article types, mixed owned-object families,
  interruption cleanup, and actual network correction remain open.
- A second controlled recording pressed B for both independently installed
  Mario clones at samples 232-234. P1 and P2 entered neutral-special motion
  343 at samples 233 and 234, then spawned separate fireballs at frames 244
  and 245. The probe saw one fireball owned by each clone, and each item's
  article pointer matched slot zero of its spawning fighter's imported article
  table, with no mismatches in this unreflected fixture. The items disappeared
  at frames 319 and 320. Forced corrections from frames 240 and 315 repeated
  the two creations and two removals with matching per-frame observations,
  zero article-pointer mismatches, and matching tracked endpoint state. Evidence:
  `build/native/clone-both-articles.fmr`,
  `build/native/clone-both-article-correction.log`, and
  `build/native/clone-both-article-removal.log`. This closes the earlier
  live Mario-fireball article-identity question for two distinct packages.
  Reflection, non-Mario articles, package-defined article types, and networked
  correction remain open.
- Match package preflight now validates each candidate in detached storage
  instead of temporarily loading and clearing the live fighter registry. This
  keeps practice fighters and their module callbacks registered while another
  package is inspected for a later match. The 374-test native suite passes, and
  the same two-clone replay still enters scripted neutral special at sample
  233 and passes forced correction at frame 245 after the change. A UI-driven
  practice-to-match transition remains to be exercised directly.
- Preview replacement now retains the last working cached package ID until the
  new practice scene has entered. If scene-exit package validation fails, the
  CSS transition retries that prior preview once, then restores the original
  fighter selection if the retry fails too. A failed script build already
  leaves the active practice scene untouched. A live control-channel session
  loaded the saved 1.4-scale Mario clone in practice, applied an unsaved
  1.8-scale replacement through the workshop, and returned to practice with
  the visibly larger fighter. Entering an invalid scale of 9999 showed
  "Invalid model scale" without leaving or changing that working preview.
  A temporary environment-gated fault at CSS-side package validation then
  rejected the 1.8 replacement after scene exit. The game restarted practice
  with the visibly smaller prior 1.4-scale preview while the unsaved 1.8 edit
  remained in the workshop field. The fault hook was removed and the normal
  client rebuilt; all 374 native tests pass. Captures are in
  `build/native/workshop-preview-a.png`,
  `build/native/workshop-preview-invalid-edit.png`, and
  `build/native/workshop-preview-recovered.png`. The live test covered the
  first recovery retry, not failure of that retry; failure during the later
  fighter registration step was still a separate failure point at this stage.
- Home practice and lobby match setup now fully import selected fighter
  packages into detached slots while the prior scene is alive. This includes
  the stock skeleton-part check; a failed replacement can return to CSS and
  use the existing prior-preview retry. Scene startup only binds those
  already-prepared slots after clearing the old fighter registry, instead of
  reopening and importing packages inside the registration assertion. The
  fresh-CSS path also loads the common part table before staging, so it does
  not depend on a previous practice match. A live isolated client showed an
  installed Mario clone on the original CSS, selected it for P1, and entered
  practice with P1 and a stock Ness P2. A separate live test replaced an
  active workshop preview with a larger build. The normal native build and
  all 374 tests passed serially. Captures are
  `build/native/stage-first-css-portrait.png`,
  `build/native/stage-first-css-practice.png`, and
  `build/native/workshop-staged-replacement.png`. The scene-registration
  invariant can still assert if future callers skip staging or violate the
  slot lifecycle.
- Initial direct command-line matches now stage their selected packages after
  native archive setup and before entering the first scene. The same installed
  Mario clone reached the direct practice room without registration-time
  import; capture: `build/native/stage-direct-practice.png`. A temporary
  pre-scene failure injection ended launch with exit code 1 and a diagnostic,
  without reaching the registration assertion. The hook was removed and the
  normal client rebuilt; all 374 native tests passed serially. A hypothetical
  later direct-mode rematch still uses the old registration-time import and is
  outside this first-scene check.
- A two-client UI-driven room match selected an installed Mario clone on the
  host and stock Falco on the guest. The guest started with no installed clone;
  match admission downloaded the bundle to its cache without installing it.
  Both clients entered the same Battlefield match and compared over 7,000
  confirmed peer checksums without rejected packets. The tied round then
  exposed a scene-lifecycle gap: sudden death cleared the first match's
  consumed fighter slots and registration asserted on both clients. Sudden
  death now stages the selected packages while the completed scene is still
  alive, then activates them after the next scene clears its fighter registry.
  In a repeated one-minute, no-items room match, both peers entered sudden
  death with 300% fighters, compared 3,833/3,832 confirmed checksums in the
  timed round and 1,665/1,664 in sudden death, reported zero rejected input
  packets and zero timeline corrections, then reached matching results with
  the clone winning. Pressing START on results returned both clients to the
  same room's CSS. This covers the uncorrected same-machine room path; a
  forced network rollback correction with the custom fighter is still open.
  Evidence: `build/native/stage-sudden-host.log`,
  `build/native/stage-sudden-guest.log`,
  `build/native/stage-sudden-match.png`,
  `build/native/stage-sudden-fight.png`, and
  `build/native/stage-sudden-results.png`, and
  `build/native/stage-sudden-return.png`. The normal native build and all
  374 tests pass serially.
- A further two-client room run temporarily held outbound input for three
  frames to force network prediction while the installed Mario clone fought
  stock Falco. The host sent a B input during the timed Battlefield round.
  The guest corrected two timelines across eight hidden frames; the host
  corrected none. They compared 3,825/3,752 confirmed peer checksums and
  reported zero rejected packets or checksum mismatch. Logs are
  `build/native/stage-delay-host.log` and
  `build/native/stage-delay-guest.log`. The diagnostic delay was removed and
  the ordinary client rebuilt afterward. This validates an actual network
  correction in a custom-fighter match, but the current log does not identify
  the exact corrected frames or prove that a correction crossed the authored
  fireball callback or its owned article state.
- The readable special script now accepts `motion.add_vertical_speed(number)`
  after an aerial stock or authored entry, and in an authored neutral cue.
  The generated code changes `self_vel.y` only for an airborne fighter; a
  ground handler using the operation is rejected with its source line. Fixed
  module fixtures cover neutral and up-special air entries, a one-shot cue,
  and the unchanged ground velocity. The full native build and all 374 tests
  pass. This is a bounded vertical impulse, not
  a general launch, charge, or transition language.
- The earlier temporary room-input delay is now a repeatable
  `--netplay-delay` launch option for normal home boot. Each local client
  carries its own diagnostic outbound delay into room matches, including a
  guest that receives the host's proposal. With the host at three delayed
  frames and the guest at zero, a custom Mario versus Falco match entered
  sudden death and the guest corrected two timelines across six hidden
  frames there; 1,669 confirmed peer checksums compared in that scene, with
  no rejected packets or reported mismatch. The logs are
  `build/native/room-delay-flag-host.log` and
  `build/native/room-delay-flag-guest.log`. The exact authored callback and
  article state crossed by those corrections remain unproven.
- Direct raw-UDP matches now accept the same installed custom fighter ID on
  both clients. A Mario clone versus Falco reached matching results after a
  timed round and sudden death. The first direct run exposed a checksum
  monitoring gap when one client ran ahead: a single latest-checksum slot was
  replaced before the slower peer could compare it. The receiver now keeps a
  bounded pending-checksum window and reports overflow instead of silently
  dropping an unchecked state. In a repeated same-machine run with three
  frames of host outbound delay, the host/guest compared 3,834/3,831 confirmed
  checksums in the timed round and 1,580/1,577 in sudden death, with zero
  rejected packets and matching result screens. Both clients must already
  have the exact package installed for direct raw-UDP play; the room path
  handles verified match transfer separately. Evidence is in
  `build/native/direct-clone-{host,guest}3.log` and
  `build/native/direct-clone-checksums-results-{host,guest}.png`. The full
  native build and 374 tests pass.
- A four-instance Mario-clone run exposed that imported custom fighter kinds
  had empty CPU attack lists. Two cloned CPU slots stood at their spawn points
  through a full synchronized network round. Mario-based clones now copy all
  seven imported stock CPU attack families and the distance threshold into
  each runtime slot's own fixed storage; clearing the slot restores safe empty
  entries. The package verification tool binds the same imported common CPU
  descriptor as the client. A local rerun showed three clone CPUs moving,
  attacking, causing damage and completing a one-minute match; captures are
  `build/native/four-clone-cpu-fighting.png` and
  `build/native/four-clone-cpu-results.png`. A two-client rerun with four
  instances of that one installed bundle reached matching results after the
  timed round and sudden death. With three frames of host outbound delay and
  one host B input, the guest corrected two timelines across eight hidden
  frames; the host/guest compared 3,823/3,828 confirmed checksums in the timed
  round and 1,187/1,185 in sudden death, with zero rejected packets. Evidence:
  `build/native/four-clone-fixed-{host,guest}.log` and
  `build/native/four-clone-fixed-results-{host,guest}.png`. The full native
  build and 374 tests pass. This copies immutable Mario CPU choices as an
  independent starting profile; editable package-owned CPU behavior is not
  yet established.
- Four separately installed packages were then cloned from the same authored
  Mario project with distinct package IDs and titles. A direct two-client
  match loaded all four runtime fighter slots (Alpha/Beta human, Gamma/Delta
  CPU) and reached matching results with Delta winning. A host B input with
  three delayed outbound frames caused two guest corrections across eight
  hidden frames; host/guest compared 3,752/3,349 confirmed checksums and
  rejected no packets. `build/native/four-distinct/ids.txt`, `host.log`,
  `guest.log`, and `results-{host,guest}.png` record the run. The host also
  waited 14.9 seconds across 1,474 short input stalls during this run, so
  performance under four loaded bundles remains open. These are distinct
  package identities and module loads, all derived from the same Mario data
  and behavior; this is not proof of four unrelated fighter implementations.
- `script-build` now stages the new source and DLL, backs up the previous DLL,
  and restores it if saving the source fails. An integration test held the
  saved script open without delete sharing so the DLL replacement succeeded
  but the script replacement failed. Both previous file hashes remained
  unchanged, and the draft still bundled under its original content ID; the
  full native build and all 374 tests pass. This covers a reported operation
  failure, not a process crash or power loss between the two file replacements.
- The action trace now also records fighter damage changes at replay sample
  boundaries. Two isolated installed Mario clones differed only in Attack11's
  three fighter-DAT bytes at `0x3FB3`, `0x3FC7` and `0x3FDB`, encoding hitbox
  damage 3 versus 20. In the native practice room,
  Attack11 motion 44 / animation 46 repeatedly hit Sandbag for 3 damage with
  the untouched clone and 20 with the edited clone. The baseline package was
  `739a9c95a9d5246a39b54a8134c9966b73d41794e727bf168d17167979363a36`;
  the edited package was
  `0afcc59361fd433a8fc4129cc86be137a4e9ee75efaab1e0a7b71ad8b4d84bcc`.
  `build/native/jab-live/{stock,edited}.log` and their recordings retain the
  evidence. A new local `--replay-inputs` comparison then fed the stock
  recording's exact seed and 1,849 controller samples into the edited package:
  both runs entered Attack11 at sample 145 and hit Sandbag at sample 146, for
  3 versus 20 damage. The comparison completed in accelerated mode with exit
  code zero; `build/native/jab-live/edited-from-stock-inputs.log` records it.
  This proves the saved original subaction edit changes the live hit result on
  identical input. It does not establish other hitbox fields or full Mario
  move fidelity.
- Per the later priority decision, complete arbitrary move transfer remains
  planned but does not gate the base clone backend or the collaborative
  workshop interface design. The current same-layout copy support is a
  useful backend slice, not a condition for calling the initial clone/edit
  workflow ready.

Next: expand authored state, cue, article and cleanup operations beyond the single
neutral-special example, then prove those callbacks and their owned state
through live rollback correction. Broaden original-command fields and verify
a complete edited normal with reproducible input and hit-result checks. Audit Mario's remaining
special moves, items, landing, CPU and Kirby-copy behavior before calling him
fully cloneable.

## 2026-09-24: first in-client three-mode editor

- The workshop now opens a chosen fighter draft in Action Editor, Scripting,
  or Animation mode. The draft collection and Clone Mario action stay on the
  collection page. Fighter attributes, portrait selection, practice preview,
  and package installation remain accessible through Fighter & Package.
- Action Editor searches the imported action list, shows reachable source
  commands in archive order, and selects individual hitbox commands. Its
  inspector edits five existing encoded hitbox fields through the native
  action backend. It does not yet author arbitrary commands or present a
  complete execution-aware event timeline.
- Scripting uses the native parser for immediate diagnostics and the existing
  content tool for C generation and module building. Save draft keeps an
  uncompiled `behavior.draft.fms` separate from the last successfully built
  `behavior.fms` and DLL. Preview/install reject a changed script until it is
  built. A live Mario draft accepted a valid Fireball script and built its
  module; an invalid edit survived Save, Back, and reopen while the prior
  compiled source remained unchanged. The editor currently requires the
  development content tool and configured C compiler; release packaging of
  the authoring toolchain is still open.
- Animation mode plots multiple selected tracks using values sampled from
  Melee's native evaluator, with normalization and key inspection. Existing
  representable key values, tangents, and interior frame positions can be
  edited transactionally and saved to the draft. Interior keys can also be
  dragged horizontally in the graph; a live drag of RunBrake's joint 3 Rotate
  Y key changed its frame from 5 to 6 in the inspector and curve. It cannot
  yet insert keys, copy poses, drag values/tangents, or undo/redo edits. The graph
  currently belongs to the selected action's clip; a rig viewport and
  animation selection independent of action selection remain to build.
- A hidden live native-client run exercised the three modes and captured
  whole-UI screenshots. Nine focused fighter action/script/animation tests
  passed. These checks establish a functional editor foundation, not the
  approved workshop's full acceptance criteria in
  `native_character_workshop_ui.md`.

## 2026-09-24: source-aware workshop navigation

- Scripting now places a navigable file/handler outline next to the code
  editor, while the real game viewport remains visible. The outline reads the
  draft's actual `move` and `on` lines;
  it does not pretend that the mockup's multiple `.move` files or broad
  GDScript-like operations compile yet. The parser's Check script result is a
  clickable problem row. In a live run, the invalid operation in the isolated
  Mario draft reported line 6; clicking it placed the text caret at line 6.
  Build behavior now runs this parser check before invoking the content tool.
- Action and script modes link to each other and to the selected action's
  animation without changing action selection. The Action inspector exposes
  the backend's existing bone index, signed three-axis hitbox offsets, and
  ground/air collision flags alongside damage, angle, radius and knockback.
  These fields retain the original encoded units. A live client screenshot
  displayed Mario Attack11's first hitbox, including its fighter bone 9 and
  offsets `0, 0, 600`. New fields have not yet received an in-game edited-hit
  comparison; the earlier damage-only proof remains separate.

## 2026-09-24: approved workspace structure in the native client

- The workshop header, context path, editor rails, reduced live game viewport,
  and full-width lower timeline now follow the approved three-mode layout.
  Action, Scripting, and Animation each have their own workspace arrangement.
- Action Editor now has grouped/searchable actions, separate lanes for stable
  hitbox IDs, a ruler and cursor, real clip durations, and a switch to the
  source-order command list. Frame labels apply only to the straight-line
  command prefix; commands after loops, calls, or gotos remain in the list
  without an invented timestamp. The inspector can edit existing hitboxes and
  numeric original events. It cannot yet insert/delete/reorder commands.
- Animation has a clip list, joint/channel tree, full-width shared curve
  graph, frame cursor, keyed local-transform inspector and transactional edits
  to existing representable keys. Structural key insertion and pose copying
  are described below; viewport bone manipulation and box selection remain
  open.
- Scripting has the real single-file compiler source, handler outline,
  diagnostics, a smaller game preview area and a compact reference to actual
  supported operations. Its Watch panel samples P1's actual action, animation
  frame, velocity and grounded state while practice is active; inspecting
  arbitrary player slots and script-local variables remains open.
- This batch passed a native release build and source diff checks. Per the
  user's request, no lengthy in-game interaction pass was run here; visual
  layout and interaction acceptance await hands-on testing.
- The header now provides document-level Undo/Redo for in-place action and
  animation edits. History stores only changed bytes (up to 32 edits), can
  cross Action/Animation modes, and resets when the draft is reopened.
  Script typing still uses the text editor's own undo and is not part of this
  byte-level history.
- The Action inspector now also exposes existing sound commands (ID, volume,
  panning) and visual-effect commands (effect ID, bone, three offsets). The
  corresponding native backend setters preserve their other encoded fields,
  and these edits enter the same Undo history. Canned sound behaviors remain
  read-only. The native release build and the focused sound/effect round-trip
  tests pass; the new in-client fields await the user's UI test.
- Native animation authoring can now insert a key between existing frames, at
  the beginning of a track, or after its last key. It re-encodes that track's
  FigaTree stream into a larger valid DAT clip, preserving other source tracks
  and original key payloads. A Mario down-air frame-6 insertion reopened with
  seven keys, its later clip still resolved, and the package registered.
  Structural animation edits now have whole-bank Undo/Redo snapshots.
- Copy pose samples the source clip's existing local transform channels through
  the native evaluator. Paste writes matching destination channels at the
  cursor, start or end as one transaction, with selected-bone and root-motion
  options. An isolated Mario down-air to back-air transfer wrote 161 channels;
  root translation was excluded, and the rebuilt package registered. Channels
  absent from the destination are reported rather than invented. This does not
  yet create absent tracks, sample unanimated base-pose channels, or provide
  fractional-frame selection in the UI.
- Action event inventory now includes the original clear-one and clear-all
  hitbox commands. The timeline draws each hitbox's active span until the next
  clear/replacement where the source has straight-line timing; the source-order
  list marks branch-dependent commands as dynamic flow. The Action preview's
  hitbox/hurtbox toggles use Melee's own collision debug renderer through
  render-only switches; no fighter simulation bits are changed. A native build
  and focused action/animation tests pass. The user will test these UI changes
  in-game; their visual and interaction acceptance is still open.
- The Action browser now groups real actions into collapsible ground, aerial,
  special, movement/defense and other sections, retaining search. The timeline
  ruler can move the frame cursor. Animation defaults to a dope sheet built
  from the imported track keys, with joint/channel rows, key selection, and
  frame scrubbing; the Curves tab shows up to 32 channels and supports
  multi-key value/time dragging, box/shift selection, rotate/scale tools with
  selection or cursor pivot, plus graph pan and zoom. Selection and the key
  inspector are shared between both views. Invalid transformed positions or
  unrepresentable track encodings roll back the entire gesture. The scripting
  editor gained a
  synchronized line-number gutter. These additions compile in the native
  client; in-game visual and interaction testing remains with the user.
- Mockup parity is still incomplete: independent Action event dragging is
  absent. The
  Animation graph lacks tangent handles and bone posing in the viewport. First/last
  key retiming and changes that require wider source wait encoding are still
  rejected by the backend. Scripting still
  uses one compiler source file and does not provide colored code or the
  mockup's multiple-file workflow. Do not treat the current Workshop as
  accepted solely on the basis of these layout changes.
- `Add event` can now copy the selected command or an event-kind template from
  another stock action into a new action-local script, including actions that
  call shared subroutines or branch. The editor
  gives a copied hitbox an unused ID
  so it can be edited independently, and records the whole fighter DAT as one
  undoable structural edit. The archive rebuild inserts that script before the
  fighter's 24-byte action table, shifts later data, relocations and roots, and
  retargets only this action's entry. A Mario Jab test added IDs 3 then 4,
  edited ID 4's damage, confirmed neighboring Jab and Down Air action listings
  were unchanged, and packaged and verified the rebuilt fighter through the
  native importer. A later test copied SpecialN's visual-effect command into
  Jab, then duplicated a command in Mario's shared Wait
  subroutine for one action, verified the other action still used its original
  source, and packaged and verified the result again. The rebuild now copies
  reachable command bytes, remaps local branch targets, adds corresponding DAT
  relocations and leaves unrelated action pointers intact. No in-game
  interaction test has been run for the button.
- Action events can now be removed from an action-local rebuilt source graph.
  Deletion rejects a branch target or the final command, preserves other
  actions' source, and enters the whole-DAT Undo history. The focused test
  deletes a copied visual effect and a command inside a shared subroutine,
  then verifies the resulting package through the native importer.
- The Action inspector now exposes the selected event's frame when a
  straight-line source timer controls it. Editing that frame changes the
  preceding original synchronous or asynchronous timer, so events sharing
  that timer and later events may move together; control-flow-dependent
  events and shared source timers still reject the edit. The source timer
  edit, unlike a viewport animation, does not simulate the move.
- The Animation curve graph now draws two handles for a selected key whose
  source encoding stores an editable Hermite slope. Dragging either handle
  edits that original tangent as one Undo entry; its horizontal handle
  length is a graph control because FigaTree stores slope, not separate
  Bezier handle coordinates. In-game interaction remains for the user to test.
- The curve tools accept Blender-style B/G/R/S shortcuts when focus is not in
  an editor text field. Action event templates are refreshed only after a
  fighter archive structural edit or Undo/Redo, avoiding a 303-action scan
  each time the menu opens.
- After Test action loads the cached draft in practice, Action and Animation
  modes now use the original fighter animation/action path to hold the
  selected action at the editor's frame in the live game viewport. Its fighter
  processes are suspended during the pose preview and restored on mode exit,
  preview restart, practice exit or Workshop close. Scrubbing uses original
  action command evaluation; it can create the source action's effects. This
  route compiles, but the in-game appearance, hitbox overlay, and lifecycle
  still need the user's interactive check. Unsaved edits require Test action
  again to enter the cached fighter package.
- Both Action and Animation modes now expose a Play control. It releases the
  paused fighter and runs the selected action with the original game update
  path; the editor follows the fighter's actual animation frame and holds a
  pose again when that action ends or the user stops. This path has build
  verification only; the user's interactive test is needed before claiming
  that playback and all action callbacks behave correctly.
- Editing a stock command shared by multiple actions no longer requires a
  manual copy step for the hitbox, value, sound and visual-effect inspectors.
  The editor localizes that action's source graph, maps the selected command
  to its new offset, applies the edit and records the whole operation as one
  Undo entry. The focused round-trip localizes a shared Wait, edits one
  action's timer, checks the other action is unchanged and verifies package
  import. Frame-group retiming still needs this same automatic localization.
- This does not yet construct a blank event independent of a template or
  provide property fields for every event kind. `ftActionData_Import` requires commands and branch targets
  before the action table and validates DAT relocations; simply appending
  commands after the table would be invalid.
- The Animation inspector now offers Auto key for editing source channels at
  an unkeyed frame, plus one operation to key every available channel on the
  selected bone. Multi-channel edits are one Undo entry and restore the bank
  if any source track rejects its new key. Channels absent from the imported
  clip still cannot be authored through these controls.
- The Action timeline now presents hitboxes as a collapsible overview with
  separate ID lanes, and the Event list calls out simultaneous source
  commands and their execution order. Clicking the Action or Animation ruler
  scrubs to a specific frame. Animation mode reserves a larger lower pane for
  the shared graph and moves the native game preview above it. These
  interaction changes compile; live visual/interaction acceptance is
  delegated to the user.
- Action timeline blocks can now be dragged horizontally to retime their
  source timing group. This uses the same timer contract as the inspector,
  including automatic action-local copying when a timer is shared by another
  action; a command with branch-dependent timing still rejects the drag.
  Drag behavior itself has build verification only, pending the user's test.
- Animation's shared graph has a Focus graph / Restore layout control. Focus
  uses the workspace below the Workshop header, hides the preview and side
  inspectors, and recalculates the graph when the window or pane size changes.
- Action and Animation work modes now have a draggable divider between the
  native game preview and lower timeline. Each mode keeps its own size while
  the graph and dope sheet scale within the available pane. This is build
  verified; the user will assess the rendered layout and pointer behavior.
- The Action Event list now emphasizes frame, order within that frame,
  command and affected target. Original source offsets remain available as
  hover details for low-level debugging rather than occupying the main column.
- Switching between Action and Animation now carries the current frame across
  modes, while selecting a different clip starts that clip at frame zero.
  Selecting a bone channel keeps the current pose/frame until a key is
  explicitly selected; Auto key also stays in its chosen state across clips.
- A native authoring-only bone query now projects the preview fighter's
  original joint matrices through Melee's game camera. Both practice
  viewports can display those joints; clicking one opens/selects its actual
  animation channels. It reads the cached practice fighter and does not edit
  the draft or game state. Projection and pointer placement need the user's
  live visual check before direct viewport posing is built on them.
- The Workshop screen now uses the approved Action Editor structure more
  consistently: action resources sit in the left rail, the selected move
  exposes its hitbox count, and status occupies a full-width footer. The
  browser and inspector now follow the actual resizable timeline boundary
  instead of retaining a fixed `32vh` cutoff. Mario drafts open on Down air
  so simultaneous hitboxes are visible immediately. Action and animation
  browsers filter while typing; the clip browser searches every imported
  action/clip. The Scripting source editor renders simple syntax colors over
  the editable textarea and follows its scroll position. These layout and
  pointer interactions are build verified only; the user's in-game review
  will determine whether the Rml text overlay and pane proportions render
  as intended.
- Opening a draft from home character select now requests practice with the
  already selected local human roster, preserving the Workshop screen through
  the CSS-to-practice transition. It does not build or apply the draft merely
  by opening it; Test action still applies current edits to the practice
  fighter. If any active player has no fighter selection, the editor asks for
  a selection instead of launching an invalid practice session. This path is
  compiled but has not yet been exercised interactively.
- The Animation browser now has selectable joint rows above their channels.
  After Test action loads the cached fighter, those rows indent using the
  live fighter's actual JObj parent chain. Before a preview is loaded, the
  same source joints remain a flat list because no live hierarchy is present.
  The user will check the hierarchy and joint selection in-game.
- Action mode now projects the live practice fighter's four active hitbox
  centers into the viewport as numbered selection handles. Clicking a handle
  selects the corresponding source creation event and opens its real hitbox
  inspector; the original hitbox renderer remains responsible for showing
  its actual volume. The action transport also has a previous-frame control.
  The handles need an in-game alignment and pointer check, especially for
  actions whose control flow revisits the same hitbox ID.
- Curves mode now gives the shared graph its own channel rail, with the real
  selected joints' channels, per-channel visibility controls and matching
  curve colors. A readout names the active channel and key selection count;
  graph-specific interaction hints sit below it. The existing curve/key
  editing backend is unchanged. This layout compiles but needs the user's
  rendered review for proportions and scrolling.

## 2026-09-25: clip length and visible rig hierarchy

- Animation mode now exposes the figatree end frame in Clip settings. A native
  edit changes that metadata in the project's animation bank and refuses to
  shorten a clip past any encoded key. The edit participates in document
  Undo/Redo; the action timeline, clip list, and animation frame bounds
  refresh from the changed data. Fighter looping is controlled by action
  state flags in the original game, so the mockup's clip-local Loop toggle
  is intentionally not presented as a working figatree field.
- The live viewport bone markers now connect along the preview fighter's
  actual JObj parent hierarchy, and the selected bone inspector names its
  parent. Links appear after a practice preview supplies the hierarchy.
  They are visual only; direct pose manipulation in the viewport remains
  unimplemented. The native release build and diff checks passed. The user
  is handling the in-game layout and interaction review.
- Action Editor's inspector now lists the original commands executing on
  the selected straight-line frame in source order. Every row can select its
  own command, including simultaneous hitboxes at the same timestamp. This
  list deliberately omits branch-dependent commands, which remain in the
  full Event list. Clicking a projected bone while a hitbox is selected now
  fills that hitbox's attachment joint field instead of switching work modes;
  the user still applies the field edit explicitly. Native build passed;
  in-game interaction remains unverified.
- The source fighter's per-action animation loop flag now has an Action
  Playback control. The original importer maps DAT bit 30 to the native
  fighter loop bit; the editor changes that original flag through the
  existing action metadata setter and includes it in Undo/Redo. This is
  intentionally an action property rather than a fictitious figatree flag.
- The shared graph now draws frame/value guides and can hide source channels
  with no animation beyond one key. When more than 32 selected channels exceed
  the current curve renderer capacity, the readout says so instead of
  silently implying all are visible. The selectable channel rail follows
  the same filter. These display changes compile; the user's in-game review
  will judge readability and proportions.
- Animation mode can now delete a selected interior timed transform key.
  The backend re-encodes that one track into a new source stream, preserves
  its other key payloads/frames and all other tracks, and rejects source
  endpoints or slope-only keys rather than changing boundaries implicitly.
  Deletion is one structural Undo/Redo entry in the workshop. The focused
  Mario round-trip test now covers insertion, deletion, sampling, disk undo
  and unchanged original bytes after undo; it passes. The same focused test
  also extends a clip's end frame, confirms the new value through the native
  decoder, rejects a shortened clip that would hide existing keys, and
  restores original bytes with disk undo. In-client interaction remains for
  the user's test.

## 2026-09-25: live workshop layout review

- Full-window captures of Action, Animation and Scripting exposed two RmlUi
  flex-text omissions: action group names and curve channel names were not
  drawn even though their counts and swatches were. Both labels now have
  explicit child spans and appeared in the rebuilt client.
- Switching from Action to Animation had left the selected action's event
  deletion/timing controls and `EVENT INSPECTOR` heading visible. Mode switching
  now clears those controls and shows the animation inspector. The live capture
  confirmed the Animation mode has only its bone/clip/key controls.
- The active clip is scrolled into view after its browser has been laid out;
  a live Down air capture showed the selected clip rather than the list's
  first entries. The dope sheet uses more of its pane, curve channels now have
  visible names, and curve ruler labels use appropriate display precision.
- Removed four `!important` declarations rejected by the bundled RmlUi CSS
  parser. The next launch emitted no stylesheet syntax warnings. The native
  build and focused action/animation round-trip tests pass. Actual editing,
  viewport projection, dragging and practice playback remain for the user's
  in-game acceptance; the captures here were from home character select.

## 2026-09-25: pose-transfer acceptance slice

- A fresh Mario draft now has a repeatable pose-transfer test. It gives Down
  air's source root a different representable translation, copies its frame-16
  pose to Back air's start, then reopens the project to sample the destination
  through Melee's native evaluator. Two changed limb/rotation channels match
  the source within the destination's quantization; the destination root and
  source remain unchanged. An out-of-range destination leaves the animation
  bank unchanged, and disk Undo restores the exact pre-paste bank, including
  the deliberately edited source root.
- In Animation mode, Shift-clicking joint rows now adds/removes whole bones
  from the channel selection used by `Selected bones only`; every selected
  joint row remains highlighted. Paste also refreshes the dope sheet when it
  moves to a start/end frame. The native build and pose/animation tests pass;
  these new pointer interactions await the user's in-game review. Pose capture
  still samples only tracks encoded in the source clip and cannot create
  absent destination tracks or sample base-pose channels.

## 2026-09-25: readable velocity replacement

- Special scripts now accept `motion.set_facing_speed(number)` on ground, in
  air, or in the authored neutral cue, and `motion.set_vertical_speed(number)`
  in air or in that cue. Additive forms retain their existing behavior; set
  replaces the corresponding Melee velocity component after state entry or
  inherited stock entry. The parser still limits each handler to one operation
  per axis and rejects ground vertical edits with source-line diagnostics.
- The workshop's Fighter API panel lists both forms. Generated-module tests
  start with nonzero velocity and verify that stock side/down handlers and an
  authored neutral cue replace rather than add; existing tests continue to
  cover additive movement. This expands the small readable subset but does
  not implement persistent variables, charge state, arbitrary article
  authoring or a complete special-move language.

## 2026-09-25: graph-first animation workspace

- Animation mode now opens on the shared curve graph, as in the approved
  mockup. The dope sheet remains a secondary tab, and reopening a fighter
  draft resets the mode to the graph. The graph gets more of the lower pane.
- Action timeline ruler clicks now use the same 92% horizontal span as the
  event bars, drag math and playhead. The final tick is aligned with the
  displayed end frame instead of appearing earlier than the events.
- The curve graph now handles wheel zoom around the pointer, plus Shift+wheel
  or horizontal-wheel panning within the clip. The graph consumes those
  mouse-scroll events so the surrounding Workshop pane does not scroll too.
  Clicking empty graph space also places the animation frame cursor.
- These are client presentation and input fixes. The Release client builds;
  the user will judge the rendered layout and pointer behavior in-game.

## 2026-09-25: unified Mario scripting template

- The generated four-special Mario module now authors Fireball, Cape, Super
  Jump Punch and Tornado without calling stock Mario special handlers. A
  fresh Mario workshop draft loads the script as an uncompiled template; the
  build remains explicit. The template is copied alongside the client UI.
- Grounded Cape, aerial Cape, Up-special steering and grounded Tornado each
  reached the end of a 3,670-input local replay with all 3,669 traced P2
  fighter-state lines equal to stock Mario. The Cape module test covers its
  attached article, item-to-owner reset/removal, hitlag callbacks, reflector
  activation, wind arguments and air boost. Two independently installed Mario
  clones completed a 3,670-input match with simultaneous Cape entries. A
  custom Mario CPU recording passed an eight-frame tracked-state rollback
  restore. A stock Mario Fireball thrown into scripted Cape was reflected back
  onto its owner for 9% damage on sample 238. Running the same 3,670-input
  replay with stock Mario in the Cape slot gave identical tracked fighter
  state on all 7,338 logged player samples, including the reflection and
  damage frame. An earlier Fireball timing hits the Cape user for 6% on sample
  202, one frame after Side-special entry; stock and scripted Mario match all
  798 logged player-state lines through that interrupted 400-sample replay.
  Article lifetime after interruptions later in the move still needs coverage.
  A later input-comparison fixture entered scripted Cape at sample 226;
  restore/replay of frames 232-240 matched all 134,840,356 tracked bytes.
  A late P1 input correction at frame 240 then matched its corrected endpoint
  on two full-draw passes. These local probes cover active Cape's ordinary
  state. The correction probe now counts Cape items and checks ownership,
  article binding and the fighter's held handle. One shifted fixture created
  Cape inside the corrected eight-frame window (0 to 1 item); another removed
  it on normal action completion (1 to 0). Both corrected timelines matched on
  two full-draw passes with no binding mismatches. A Fireball timed to hit one
  frame after Cape entry removed the attached item in the same corrected
  window: Cape count 1 to 0, 6% damage, no binding mismatch, and two matching
  full-draw corrected passes. With no correction, stock and scripted Mario
  matched all 1,438 fighter-state lines in that 720-sample input fixture.
  Damage/death cleanup later in the move remains untested.
- Three 400-sample airborne replays now exercise the other scripted special
  entries: Fireball spawns its projectile, Tornado transitions through its
  grounded continuation, and Super Jump Punch reaches freefall/landing. Each
  run matches stock Mario on all 798 logged player-state lines. These are
  sampled paths, not exhaustive combinations of input, collision and hitlag.
- The fresh-clone Scripting template now resolves relative to the game
  executable, where the build copies `mario_full.fms`. Previously it used the
  process working directory, so the normal root-folder launcher could open a
  blank script. The Release client rebuilds with the corrected path; the
  Workshop's live render and interaction still await in-game review.
- An installed four-special Mario bundle was cloned back into a fresh editable
  project. Its source script was preserved byte for byte, rebuilt to a DLL,
  packaged under a new content ID and installed. The rebuilt clone completed
  the 400-sample airborne Up-special replay with all 798 tracked player-state
  lines equal to stock Mario. This checks the installed-package-to-editor
  round trip separately from cloning the disc's stock Mario.
- The normal root-folder game launch opened a saved Mario draft in Scripting
  and displayed the full uncompiled four-special template. `Check script`
  reported no problems and generated C. A window capture revealed that the
  syntax-highlight overlay collapsed comment newlines and leading indentation;
  explicit line breaks and nonbreaking indentation now render each source line
  in its intended position. This visual check did not save or build into the
  user's existing draft.
- A separate in-client Mario clone opened the same template and built it with
  the Workshop's `Build behavior` button. The client reopened the compiled
  source with no problems, and the project contained its generated C and DLL.
  The test did not install a package or enter practice. Its local test draft
  remains because automatic approval review blocked cleanup of that directory.
- A second clone changed only Cape's aerial boost script expression, rebuilt
  and installed independently. Its aerial-Cape replay first differed from
  stock at the boost on sample 201 and completed all 3,670 inputs. Both the
  unchanged and edited clones then entered aerial Cape together in one
  match: their traced vertical velocities differed exactly at that boost
  frame, showing the installed modules did not share behavior state.

## 2026-09-25: first hands-on workshop feedback pass

- The Action timeline now traces reachable commands through bounded loops,
  subroutines and gotos instead of stopping at the first control-flow opcode.
  Repeated event instances appear at their traced frames. The trace explicitly
  stops at an animation wait or safety bound. Its frame stack follows the
  traced execution order. The expanded-hitbox lane alignment, timeline
  scrolling, and free-frame ruler seeking were corrected. Clicking empty lane
  space deselects the event; G followed by a ruler click moves a selected
  event's editable source timing group. Loop instances still share their
  source timer, so moving one occurrence affects all repetitions.
- Entering a fighter draft now builds a cached preview package (using a
  temporary portrait when the draft has none) and requests practice with that
  draft fighter. Action and Animation modes hold its pose at the selected
  frame. Test action rebuilds the preview and reveals live practice; Return
  to editor restores the frozen editor view. If script edits have not been
  built, testing uses the last built behavior and says so. This transition and
  its visual result need the user's in-game check. No claim of runtime acceptance follows
  from the successful build alone.
- The script editor uses its native textarea text and caret; the separately
  rendered syntax-highlight layer was removed because its glyph positions
  drifted from the actual caret. New or existing combined Mario source is
  split into an editable `fighter_state.fms` and ordered per-move files under
  `moves/` when the draft is saved. The Variables sidebar lists and edits the
  state declarations in that file. Check/Build compose the files into the
  single generated behavior source required by the module compiler. A source
  error in Check selects its owning file and local line. The file switching,
  saving, compiler diagnostics, and practice preview await in-game review.
- The archive action list is indexed by animation ID, while live fighter
  playback uses motion-state IDs. Preview now resolves each animation through
  Mario's motion tables instead of treating those IDs as interchangeable;
  this corrects aerial previews such as Neutral Air (animation 68, motion 65).
  Editing uses a single centered fighter against a dark X/Y-axis backdrop;
  Test action loads the regular grid room and Sandbag, and Return to editor
  reloads the editing view. These visuals and transitions are build verified
  and await the user's in-game check.

## 2026-09-27: stock fighter clone expansion

- The Workshop library now lists every registered stock clone base in a
  selectable field instead of assuming Mario. Captain Falcon and Ganondorf
  can be created as independent drafts from their own imported data.
- Their neutral specials now have provisional script translations in
  `examples/captain_wip.fms` and `examples/ganon_wip.fms`. The compiler gained
  general operations for attached effects, wind, throw cues, trigonometry,
  gravity/friction, drift clamping and planar velocity scaling. Both scripts
  build into native modules; isolated packages bind 283 Captain and 272
  Ganondorf own-rig clips plus five borrowed clips each.
- These builds prove compilation, loading and content registration only.
  The provisional scripts now also express Captain Falcon/Ganondorf side
  specials, including their hit detection, transitions, effects, collision,
  and cleanup. A focused test confirms the new script-owned hurtbox detection
  callback can be installed and dispatched. Captain Falcon's down special now
  has a provisional script translation. Its grounded hit and airborne landing
  recordings match stock frame by frame for fighter states, actions, damage,
  and RNG. Wall contact, airborne hit, and remaining ending branches still
  need source review and recorded parity checks. Other untranslated specials
  still use stock C. No full cloneability is claimed yet.
- Falcon Kick's wall response now checks the stock wall-hug flag in the facing
  direction, distinct from the wider wall-contact mask. Its ground ending uses
  the stock special ground collision test when command 1 is active. A focused
  callback test distinguishes those flags, and recorded stock/custom runs match
  on grounded hit, grounded miss with command 1 and 2 set, a miss that leaves
  the platform, airborne kick landing, and airborne kick contact with 9% damage
  on the same sample. A separate Onett run also matches through the miss and
  stage-item traces. Replay inputs and their expected motions live in
  `tests/native/parity_inputs/`; `tools/run_fighter_parity.py` runs both sides
  and requires the specified motions and damage before accepting the trace.
  Actual wall bounce is not yet covered by a recording.

## 2026-09-28: scripted Falcon Dive capture path

- Captain Falcon's Falcon Dive entry, air physics, interrupt, collision,
  catch, and throw decisions now live in `examples/captain_wip.fms`. The
  script's state-change callback runs at the same motion transition point as
  stock. The native host supplies reusable air-acceleration and fighter
  capture operations; the script chooses when to arm a grab, enter catch,
  follow a victim, and release the throw. It does not call Falcon's stock
  special handlers.
- Same-input stock and packaged-clone recordings match exactly on a grounded
  whiff (380 controller samples), a grounded catch/throw (380 samples, 5% catch
  hit and 12% throw hit), an airborne whiff (420 samples), a grounded Falcon
  catching airborne Zelda (380 samples), and airborne Falcon catching grounded
  Zelda (380 samples). The comparison includes both players' traced fighter
  states, action transitions, damage, and RNG. These fixtures are checked into
  `tests/native/parity_inputs/`. Ledge grabs, interrupted throws, and other
  unrecorded branches still need parity checks before claiming 1:1 move
  coverage or a fully cloneable Captain Falcon.

- Ganondorf's stock up-special uses the same Captain Falcon handlers and
  motion-state layout, with Ganondorf's own attributes and action data.
  `examples/ganon_wip.fms` now owns its Dark Dive entry, physics, collision,
  catch, and throw decisions using the same generic capture primitives.
  A packaged Ganondorf clone matches stock on all five recorded Falcon Dive
  input paths: grounded and airborne whiffs, grounded catches of grounded
  and airborne Zelda, and an airborne catch of grounded Zelda. Each
  comparison includes both fighters' traced state, actions, damage, and RNG.
  Ledge catch, interrupted throw, and other unrecorded branches remain open.

- Ganondorf's Wizard's Foot now also uses a script-owned entry, callbacks,
  physics, and collision path. Its flame uses Ganondorf's effect ID (0x50c)
  rather than Captain Falcon's (0x490); both characters still use the same
  stock down-special handlers as the parity reference. A rebuilt packaged
  Ganondorf clone matches stock on all five recorded kick paths: grounded
  contact, grounded miss, leaving the platform, airborne landing, and
  airborne contact. The comparison includes both fighters' traced state,
  actions, damage, and RNG. Wall bounce and other unrecorded branches remain
  open; these passes do not establish fully cloneable Ganondorf.

- `bundle-project` now validates a draft's own definition and source profile
  instead of assuming Mario. It registers two independent instances of the
  resulting package. The roundtrip test covers scripted Ganondorf as well as
  scripted and data-only Mario. An existing independent Rook draft also bound
  through the command in a local smoke run. This verifies package binding,
  not move parity.

- A grounded Falcon Kick recording on Onett reaches the wall-bounce state
  (motion 363), which Final Destination and the decorative practice-room
  walls could not exercise. The first scripted run diverged on landing:
  both Falcon and Ganondorf entered a special landing state where stock uses
  the common air-catch-hit collision routine and returns to basic landing.
  The reusable `collision.air_catch_hit()` script operation now invokes that
  existing engine routine. With the same 1,100 controller samples, packaged
  Falcon and Ganondorf clones both match their stock references through the
  bounce and landing, including both fighters' state, action, item, damage,
  and RNG traces. The input fixture is checked in; other wall contacts and
  interruption timings remain unverified.

- Separate Final Destination input recordings now reach a ledge while Falcon
  and Ganondorf are in airborne Dive, rather than grabbing before the move or
  landing onstage. Falcon enters motion 354 at sample 236 and ledge catch
  (252) at sample 293; Ganondorf reaches the same states at samples 221 and
  280. Their packaged clones match stock across all 650 input samples, with
  both fighters' traced states, actions and RNG. This covers one left-ledge
  catch trajectory per fighter. Interrupted throws and other ledge approaches
  remain unverified.

- A new Final Destination recording interrupts grounded Dive before it can
  catch Zelda. Her dash attack starts at sample 229, Falcon/Ganondorf enters
  Dive at sample 231, and the 13% hit sends P1 into damage state at sample 240.
  Packaged clones of both fighters match stock across 380 controller samples,
  758 fighter states, 22 actions, one damage event, and 379 RNG states. This
  covers damage during the rising Dive, not interruption after a victim has
  been captured; that later cleanup path remains unverified.

- A second input recording turns P1 left before grounded Falcon Punch or
  Warlock Punch. The fighters face left at sample 202 and enter motion 347 at
  sample 231. Both packaged clones match stock for all 380 samples, including
  fighter pose, action and RNG traces. Neither punch contacts Zelda, so the
  hit branch and left-facing hit effect remain separate acceptance work.

## 2026-09-28: Luigi clone base

- Luigi now has an independent stock clone definition. It imports `PlLg.dat`,
  `PlLgAJ.dat`, and the base-color costume into a package with 312 actions,
  16 demo entries, 18 Luigi motion states, and 61 costume joints. The clone
  reads its own Fireball article from DAT slot 0 and does not replace stock
  Luigi's global item registration on load. The Workshop and content tool
  recognize Luigi as a stock base.
- An isolated Luigi draft bundled and installed as package
  `6e0ad7afb02763c1a3d59473eb10091500c350f05efbd1d95fb048c9ec7fde79`.
  Six stock-versus-clone recordings matched: `fighter-left-neutral`,
  `drmario-pill-contact`, `fighter-kick-contact`, `fighter-air-kick`,
  `captain-side-contact`, and `captain-side-air-contact`. They cover Fireball
  creation, ground/air Cyclone entry, and ground/air Green Missile contact;
  the latter two each damage Zelda. All fighter, item, action, damage and RNG
  trace entries match on these recorded inputs.
- This is an inherited-handler baseline, not a fully cloneable Luigi. His
  fighter callbacks, Fireball article logic, Green Missile charge/misfire,
  Super Jump Punch, and Cyclone decisions still need script ownership and
  stock comparison. The six recordings provide regression targets for that
  translation.

## 2026-09-28: Scripted Luigi Fireball and scripting reference

- `examples/luigi_wip.fms` now owns grounded and airborne neutral-special
  entry and phase decisions, the Fireball release cue, and the Fireball's
  article states/interactions. The other three specials and fighter-specific
  callbacks still inherit stock. Luigi is therefore not yet fully cloneable.
- Generic authored-projectile setup now accepts a finite numeric literal or
  named DAT float for speed, angle, and lifetime. `auto_lifetime` preserves or
  clears the engine item's automatic-lifetime flag. The article script can
  call the shared shield-bounce operation and choose an effect scheduler
  channel when spawning an effect on a model bone. Dr. Mario's pill uses
  `auto_lifetime off`; its contact and reflection recordings still match
  stock after the shared setup change.
- Four Luigi neutral-special input recordings match stock through the whole
  trace: left-facing neutral, grounded Fireball contact, airborne Fireball
  contact, and Fireball contact with Zelda's shield. The shield fixture is
  `tests/native/parity_inputs/luigi-fireball-shield-inputs.fmr`. In it Zelda
  enters guard at sample 279 and the Fireball contacts her shield at sample
  287. The item disappears within that frame; the parity comparison covers
  the 520 input samples, fighter/action/RNG state and no-damage outcome, not
  an end-of-frame item state for that contact. This takes the `hit_shield`
  removal path, not `shield_bounced`. Reflection, clank, shield bounce,
  stage bounce, and interrupt paths remain unverified.
- The new [fighter scripting reference](scripting/index.md) uses linked
  language, article, and Item API pages with signatures and examples. Current
  buildable behavior is separated from the older design-sketch document.

## 2026-09-28: Luigi fighter events and Green Missile baseline

- Luigi's `.fms` now authors load/attribute reload, item pickup/drop and
  visibility, and knockback enter/exit through shared fighter primitives.
  Its death reset and three remaining specials still inherit stock behavior.
  An interruption recording damages Luigi three times at samples 196, 200,
  and 204; stock and the packaged clone match across all 520 samples. Item
  pickup/drop and visibility events still need direct runtime recordings.
- A [Green Missile translation ledger](luigi_green_missile_translation.md)
  maps the six paired ground/air motion phases, the one-shot state-change
  misfire roll, and generic script API still required. Four new checked-in
  recordings cover held release, forced maximum-charge launch, seeded
  misfire, and an air-to-ground transition during charge. Along with the
  existing quick-release recording, stock and the current clone match all
  traced fighter, action, item, damage, and RNG records on these paths.
  Green Missile still runs inherited stock handlers; the recordings are
  acceptance targets for its future script translation.
- The [fighter API reference](scripting/fighter.md#fighter-event-callbacks)
  now explains the authored fighter-wide events and distinguishes their
  verified knockback path from pickup/drop paths not yet exercised.

## 2026-09-28: Luigi Green Missile collision transitions

- The generic `state.switch_to_air(motion, flags)` and
  `state.switch_to_ground(motion, flags)` operations preserve the current
  animation frame while switching collision/physics context and entering a
  specified motion. The linked [fighter API reference](scripting/fighter.md#stateswitch_to_air-and-stateswitch_to_ground)
  gives signatures, a compiling example, and the motion-mask caveat.
- `luigi_wip.fms` now owns eight Green Missile collision callbacks for Start,
  Hold, normal Launch, and Misfire Launch. Flight/end collision, charge setup,
  and other unlisted callbacks still inherit stock behavior. The generated
  module binds all eight collision callbacks.
- The installed clone matches stock on six 520-sample recordings: held release,
  maximum charge, seeded misfire, air Hold landing, quick release, and a
  Fireball contact regression. The parity comparison includes fighter state,
  actions, damage, items when present, and RNG. Ground-to-air transitions in
  these phases and several contact/interruption branches remain unrecorded.

- The Hold and both launch variants now script their grounded and airborne
  physics callbacks too. Their stock operations are ground friction
  (`ft_80084F3C`) and air gravity/friction (`ft_80084EEC`), not the separate
  ground/air motion helpers. The six-recording batch still matches stock after
  replacing these six callbacks. The
  [fighter API reference](scripting/fighter.md#physicsair_gravity_and_friction)
  explains the airborne operation and links its grounded counterpart.

- Start and End now script four more physics callbacks. They read named DAT
  floats at offsets `0x1C`, `0x20`, `0x3C`, and `0x40` and call the generic
  grounded deceleration, grounded-to-self movement, bounded fall, and air
  deceleration operations in stock order. All six recordings still match.
  The [language guide](scripting/language.md#fighter-attributes) explains
  attribute declarations and links to these physics methods. Flight physics
  and the move's charge/launch decisions remain stock-owned.

## 2026-09-28: Luigi death reset and charge-callback timing

- The source-ledger timing was corrected: Luigi registers `x21EC` before the
  Start motion change, and `Fighter_ChangeMotionState` invokes it during that
  **Start entry**. Charge initialization and the misfire RNG call do not wait
  for Hold. This determines where the eventual authored side-special entry
  must run its one-shot setup.
- Luigi's `on death` callback now scripts the common model-group reset. The
  stock death handler's other write zeros `u.lg.x2234`, a field with no
  native source reader found; the script omits that unused write instead of
  adding a fighter-specific API. The existing death/respawn replay was run
  with stock and cloned Luigi and matches over 520 inputs, 1,038 fighter
  states, 26 actions, 33 item states, and 519 RNG states. P1 leaves the blast
  zone and enters motion 0 at sample 206, then uses Fireball after respawn at
  sample 471.
- The [fighter API reference](scripting/fighter.md#visualshide_group) now
  documents the model-group operation and links it to lifecycle callbacks.

- Two shared Green Missile setup concepts are now exposed to fighter scripts:
  `input.horizontal_smash_age` reads the horizontal stick-smash timer, and
  `stats.count_as_thrown_item()` sets the shared action-stat flag. The native
  action-handler fixture exercises both at a nonzero timer value and passes.
  The [fighter API reference](scripting/fighter.md#inputhorizontal_smash_age)
  describes their distinction from current stick position and item spawning.
  They are preparatory API; the stock-owned Green Missile entry has not yet
  been replaced.

## 2026-09-28: Luigi Green Missile Flight and End phases

- Luigi now scripts Flight physics, its air-to-End animation and collision
  decisions, End animation/collision, and the ten stock no-op interrupt
  callbacks across Start, Launch, Flight, and End. The generic
  [`collision.wall_contact`](scripting/fighter.md#collisionwall_contact)
  expression checks either wall from the collision flags; it is separate
  from the facing-dependent `collision.wall_ahead` expression.
- The six-path stock-versus-clone batch still matches all 520-sample traces.
  The airborne Hold case reaches Flight 351 at sample 335, Air End 352 at
  375, Ground End 346 at 377, and Wait 29 at 380. This exercises scripted
  flight-to-end and end-landing behavior. Wall-impact and ground-to-air
  branches are not covered by that replay.
- The remaining side-special core is the authored entry and one-shot charge
  setup, Start/Hold/Launch animation and Hold interrupt decisions, launch
  hitbox/damage adjustment, and effect callbacks. The translation ledger
  retains these as explicit work rather than claiming the move is complete.

## 2026-09-28: Script-owned Luigi Green Missile

- The authored side-special entry now performs stock traction adjustment,
  enters Start, and invokes its one-shot charge/misfire setup during that
  motion change. Script state owns the charge count and misfire result; the
  original Luigi motion-variable union is no longer needed by this move.
- Start-to-Hold, charge loop and release, normal/misfire launch, active
  hitbox damage scaling, effects, Flight velocity, facing alignment, and
  on-damage End transition are now scripted. Generic APIs added for precise
  motion entry, hitbox updates, effect-bone spawn and hitlag cues, and the
  shared facing-alignment callback are documented in the
  [fighter scripting reference](scripting/fighter.md#method-summary).
- A single stock-versus-clone batch matched all four Green Missile recordings,
  the quick-release path, and the Fireball contact regression over 520
  samples each. The [translation ledger](luigi_green_missile_translation.md)
  names the unrecorded wall, ground-to-air, shield, and interrupt branches.
  Super Jump Punch and Cyclone remain inherited, so Luigi is not yet fully
  cloneable.

## 2026-09-28: Script-owned Luigi Super Jump Punch

- Luigi's grounded and airborne up-special entry, animation, interrupts,
  physics, and collision now run from `luigi_wip.fms`. The translation uses
  Luigi's eight DAT floats at offsets `0x50`–`0x6C`, rather than Mario's
  values, and uses the existing common movement/landing primitives.
- The grounded and airborne Dr. Mario up-special contact recordings were
  first run against inherited Luigi to establish baseline paths, then rerun
  against the scripted Luigi clone. Both match stock over 520 samples, with
  two damage events each. The six Green Missile and Fireball regression paths
  also still match. These paths do not cover every steering, turnaround,
  ledge, interruption, or landing branch.
- The linked [movement and collision API](scripting/movement.md) now explains
  the existing steering, freefall, velocity scaling, and special-landing
  methods used by this translation. Cyclone remains inherited; Luigi is not
  yet accepted as fully cloneable.

## 2026-09-28: Script-owned Luigi Cyclone

- Luigi's down-special entry, animation, physics, and collision callbacks
  now run from `luigi_wip.fms`. It uses named Luigi DAT attributes, script
  state for current horizontal speed/collision status and persistent lift
  charge; no Luigi-only scripting property is needed.
  Shared movement and collision primitives are documented in the linked
  [movement API](scripting/movement.md).
- The clone definition now supplies Cyclone's actual ground/air transition
  flags, `0x0C4C508A`. The previous generic flag set stalled its grounded
  animation for two frames and changed hit timing. The
  [translation guide](scripting/cyclone.md) records that diagnosis.
- Grounded contact, airborne contact, and repeated-tap traces all match
  stock over 520 controller samples. The nine-path batch of four Green
  Missile recordings, two Super Jump Punch recordings, Fireball contact,
  and two additional contact regressions also matched. These recordings
  are branch samples, not exhaustive acceptance of every ledge, slope,
  interrupt, shield, or charge-across-lives case.
- The stock source reads persistent Cyclone charge but has no explicit load
  initializer. The scripted clone intentionally clears it on load and death
  so a fresh life starts uncharged. That behavior change is called out
  separately from the stock-parity result. All four Luigi specials and the
  visible fighter-wide callbacks are now script-owned; broader parity
  coverage and UI dogfooding remain.

## 2026-09-28: Generic Cyclone charge state

- Removed the Luigi-specific `charge.cyclone` compiler property. Cyclone now
  declares `state bool cyclone_charge` alongside its other per-fighter state.
  This keeps private stock union layout out of the script API and leaves the
  value in the clone's rollback-tracked module state.
- The stock-versus-clone comparator checks fighter, action, damage, item, and
  RNG traces; it does not require matching the old private charge field.
  The same three Cyclone recordings still match over 520 samples each after
  rebuilding the clone. The script compiler test also rejects the removed
  property; the three focused native tests pass.
- Stock Luigi's missing load initialization is deliberately corrected in the
  script with a comment and explicit load/death resets. This is outside the
  stock-parity guarantee for undefined fresh-life charge.

## 2026-09-28: Authored blaster laser and generic launch data

- Added general explicit projectile launches from bone-local offsets, a
  post-launch article callback, per-instance launch metadata, stage sweeps,
  contact-normal velocity reflection, writable world position/facing and
  float angle math. There are no Fox/Falco behavior helpers in the host API.
- Translated both laser kinds/states to a separate compiled script. Actual
  stock C callback comparisons match 35,520 samples; 960 synthetic sweeps check
  collision-origin, position-restoration and expiry contracts. Complete firing
  and real-stage gameplay comparison remain pending until gun/neutral/throw
  ownership switches together.
- Native build and 22 focused tests pass. The unchanged Rook source rebuilt
  successfully with 285 own-rig clips and replaced its prior installed package.
- See [the roster ledger](fox_falco_script_translation.md#laser-callback-checkpoint-2026-09-28)
  and linked API reference for evidence and remaining acceptance. This does not
  mark Fox/Falco fully cloneable; the broad goal stays active.

## 2026-09-28: Authored gun model behavior and generic article communication

- Added named, synchronous signals for owned attached articles, constant float
  tables, direct callback sharing between article states, and joint existence /
  recursive visibility properties. These operate on ordinary named state and
  common engine operations; there is no blaster-specific host API.
- Translated both gun variants' opening, closing, recoil, and visibility into
  a separate compiled script. Original C comparisons match 68,040 samples of
  transforms, matrix flags, and counters. Sound requests and trigger counts,
  table bounds, missing joints, signal arguments, and shared callback pointers
  are also checked. Live ownership/firing acceptance remains pending.
- Native build and 23 focused checks pass. Rook's unchanged source rebuilt;
  all 285 own-rig clips verified, and the package replaced its prior install.
- [The ledger](fox_falco_script_translation.md#gun-model-checkpoint--2026-09-28)
  records evidence and limits. Gun ownership, accessory effects, neutral/throw
  firing, and cleanup still need to switch together before playable clones use
  the separate gun/laser scripts. Continue that work; the broad goal is active.

## 2026-09-28: Gun ownership and accessory lifecycle

- Extended the separate Fox/Falco gun script with pickup, state following,
  owner cleanup, incapacity scaling, interaction, and muzzle effects. Added
  generic attachment creation settings, expression-selected item kind/state,
  explicit accessory tracking, directional effects, and terminal item removal.
  There are no stock blaster callbacks or private union aliases in the script.
- The native build and 23 focused checks pass. The original model comparison
  still matches 68,040 exact samples; 70,576 additional owner/state cases pass
  with transition/removal/effect boundaries mocked. No GUI test was launched.
- Rebuilt Rook without source changes, verified 285 own-rig clips, and replaced
  her previous install. See the [ledger](fox_falco_script_translation.md#gun-lifecycle-checkpoint--2026-09-28)
  and [article API](scripting/article.md#attachment-creation) for details.
- Next: complete fighter neutral-special and throw firing, combine the gun and
  laser translations, then verify recorded gameplay before enabling the
  overrides. Fox/Falco remain partial; the broad goal remains active.

## 2026-09-28: Integrated Fox/Falco blaster and throw shots

- Enabled scripted neutral-B, gun/laser articles, throw shots and related
  cleanup together in the shared template. The five blaster tuning fields
  import into owned parameters. New operations expose general throw events,
  article cues, pickup notification, bone directions and instance/base scale.
- Native build and 23 focused checks pass. Fourteen integrated recordings
  match stock fighter/item/damage/RNG traces for both variants. The first batch
  found a missing contact-removal callback, now fixed. See the
  [ledger](fox_falco_script_translation.md#integrated-blaster-checkpoint---2026-09-28)
  for evidence and limits; these fighters are still partial.
- Refreshed Rook against the host layout without source changes and verified
  285 own-rig clips. Her installed package was replaced without an extra entry.
- Next: side/up specials, illusion/phantasm, radio and remaining lifecycle
  behavior. The broad goal remains active.

## 2026-09-28: Scripted Fox/Falco side dash and trail models

- All six side-B phases and both illusion/phantasm articles now use the shared
  script. Twelve additional owned tuning fields bring the template total to 31.
  Trail history is ordinary state; extra model graphs use fixed per-item slots
  and tracked HSD pools with automatic cleanup.
- Added reusable world article creation, post-setup initialization, model
  instances, common rendering, TransN movement, root rotation reads and facing
  ledge queries. Explicit move entry lets scripts own command reset and effect
  scheduling; parity found and resolved an early compiler-installed effect.
- Fourteen side recordings match stock, including shortened travel and 7%
  contact damage for both variants. Fourteen blaster/throw regression recordings
  still match, and the native build plus all 23 focused checks pass. Secondary
  trail model poses/visuals are included in the comparison. See the
  [ledger](fox_falco_script_translation.md#side-dash-checkpoint--2026-09-28).
- Refreshed Rook without source changes, verified 285 own-rig clips, and
  replaced the prior install without creating another collection entry.
- Next: migrate article parameter defaults into owned FMP fields, translate
  up-special/radio and remaining lifecycle behavior, then continue the roster.
  These fighters remain partial; the broad goal stays active.

### Owned article tuning checkpoint — 2026-09-28

- Added article-scoped, typed package parameters to the existing FMP store.
  `item.params.Name` resolves to immutable compiled storage, including after
  reflection or owner changes. Projectile defaults read this same store.
- Stock import copies original scalar bits, removes article offset declarations,
  and retains only the current variant's article blocks from shared templates.
  Asset bindings use one catalog shared by import and runtime lookup.
- Workshop conversion processes every move file and preserves existing owned
  values; conflicts leave the source intact. No new interface layout was added.
- 14 side-dash and 14 blaster/throw recordings match stock; the build and all
  23 focused checks pass. Import is checked across five fighter/article families,
  and loaded-module tests change typed article values without rebuilding or
  dereferencing a DAT. See the [ledger](fox_falco_script_translation.md#owned-article-parameter-checkpoint--2026-09-28).
- Rook rebuilt with unchanged source, all 285 own-rig clips verified, and its
  prior installed package replaced without a duplicate collection entry.
- Next: Fox/Falco up-special, radio and remaining lifecycle behavior, then
  continue the roster. Shared item asset properties remain in owned DATs;
  raw offset declarations remain available only as stock translation drafts.
  Neither fighter is fully script-owned, and the goal remains active.

### Fox/Falco up-special checkpoint - 2026-09-28

- All seven up-special motions are translated to named script state and shared
  engine primitives. Sixteen defaults become owned editable FMP values.
- Generic APIs expose surface normals/contact flags, XY vector angles,
  pass-through-platform skipping, named-bone rotation, vertical animation
  velocity and transient ground-item pickup exclusion. Documentation explains
  their effects; no fighter-specific mutable-state aliases were added.
- All 22 ground/air steering, default, rebound and platform recordings match
  stock, including pose/hitbox/hurtbox and RNG traces. The native build and all
  23 focused checks pass. Compiler coverage includes the 64-helper limit and
  indirect recursion through the highest helper index.
- Rook's unchanged source rebuilt, its 285 own-rig clips verified, and its
  installed package replaced. Full evidence and untested branches are in the
  [translation ledger](fox_falco_script_translation.md#up-special-checkpoint---2026-09-28).
- Next: radio taunts and remaining Fox/Falco lifecycle decisions, then further
  roster translation. Neither fighter is fully script-owned; the goal is active.

### Fox/Falco radio and lifecycle checkpoint - 2026-09-28

- All six radio motions and their input/status/death decisions are scripted
  with named state. Generic stage-event, input-history, action-counter and
  callback-registration operations are documented with linked references.
- Extended pickup/drop callbacks now use authored generic item operations.
  The audit covers startup/death, held-item visibility, knockback layers,
  gun cleanup and radio interruption; null stock destructor hooks stay null.
- Native stage import now caches SIS roots during loading and preserves
  Venom's indexed radio overlay. Arwing laser spawn positions no longer read
  uninitialized stack data. The high action-counter query avoids native C
  out-of-bounds array indexing while preserving its containing-object offset.
- Custom fighter sound banks load from staged next-scene definitions before
  activation. Explicit bank defaults use the SSM list, not internal fighter IDs.
  The initial Falco bank mistake caused early dialogue completion and was
  diagnosed from the RNG divergence rather than changing move behavior.
- All eight final normal-paced, rendered radio comparisons match stock on
  Corneria/Venom facing right/left. Build and 24 focused checks pass. Rook's
  unchanged source rebuilt with 285 own-rig clips and replaced the old install.
  The [ledger](fox_falco_script_translation.md#radio-and-lifecycle-checkpoint---2026-09-28)
  records the failed preliminary runs and final evidence separately.
- Next: generic contracts for remaining shared fighter-kind exceptions and
  CPU audit, then continue roster translations. Whole-fighter acceptance,
  audio waveform/text-pixel equality and untested branches remain open. The
  broad goal stays active.

### Shared idle/statistics query policies - 2026-09-28

- Generic `idle_variants` and `motion_is_angled` predicates replace two stock
  fighter-kind exceptions in scripted Fox/Falco. The latter receives the queried
  motion ID as `event.motion`, rather than confusing it with the current action.
- Compiled callbacks cover held/unheld idle decisions and match the original
  classifier across motion IDs 0..400. Invalid event contexts are rejected.
  No new held-item gameplay recording or end-screen statistics check is claimed.
- Both script forms now support 512 handlers; boundary checks cover acceptance
  and rejection above the limit, including generated host requirements. This
  compiler capacity change adds no simulation allocation or interpreter.
- Native build and all 24 focused checks pass. Rook rebuilt unchanged and
  replaced its installation for the host layout. CPU-specific decision paths,
  cutscene helpers and remaining branch acceptance are still tracked in the
  [Fox/Falco ledger](fox_falco_script_translation.md#shared-idle-and-statistics-policies---2026-09-28).
  Continue roster work; the broad goal remains active.

### Generic weapon-ribbon contract - 2026-09-28

- Added a script-authored `on trail` description for joint, axis, endpoint
  distances, fade factors, alpha and RGB. Sampling and drawing consume this
  description directly rather than treating a custom fighter as a stock sword
  fighter. The original three-sample renderer and subaction timing remain.
- Both script forms reject gameplay writes, cue consumption, helper calls,
  and RNG in this rendering-data callback. There is no allocation or new
  persistent fighter state. Stock weapon trails and held Beam Swords retain
  their source settings; unsupported/missing descriptions disable the trail
  instead of dereferencing an uninitialized stock attribute pointer.
- Added import-only unsigned byte declarations for packed colors and other
  byte-valued stock defaults. They become ordinary editable owned integers.
  The [reference](scripting/trail.md) includes source offsets, field meanings,
  examples, and the remaining acceptance boundary.
- Build and all 25 focused checks pass. Compiled descriptor checks cover both
  script forms, null donor data, exact output fields, unchanged fighter bytes,
  parameter edits and invalid descriptors. Byte imports match raw ISO bytes.
  Rendered sword-trail parity is still open; these are not gameplay recordings.
- Rook rebuilt from unchanged source with 285 own-rig clips and replaced its
  installation for the changed host layout. Next: Marth/Roy clone bases and
  shared special translations using this generic contract. They are not yet
  available as fully scripted clone bases. The broad goal remains active.

### Marth/Roy neutral and up specials - 2026-09-28

- Registered independent Marth/Roy asset bases and Workshop template loading.
  ISO costume/common part tables verify 90 and 92 joints respectively; both
  have 327 actions and 14 demos. Clone assets and imported tuning are owned by
  the project. Side-special, counter and death/reset still retain stock code;
  neither fighter is accepted as fully script-owned.
- Templates script the eight neutral phases, two up phases, item/knockback
  events and weapon-ribbon description. Charge, release, steering, turnaround,
  root-motion/descent, physics, landing and hitlag-effect decisions are authored
  without calling stock special handlers. The named charge counter is ordinary
  fighter state; tuning and ribbon bytes become editable owned parameters.
- Added generic read-only animation deltas and gravity, plus friction-aware
  air acceleration. The [movement reference](scripting/movement.md) documents
  signatures and how these differ from no-friction drift. No fighter-specific
  policy was added to the scripting API.
- Build and all 25 focused checks pass. The final evidence selects 46 complete
  stock/script comparisons: 24 neutral, 18 up and four contact paths. Both
  facings, air/ground entry, charge levels, landing during charge, steering and
  actual damage are covered; Roy's full-charge self-damage remains intact.
  Preliminary inputs failing coverage gates are excluded. Reproduction paths,
  generated-behavior checks and remaining limits are in the
  [translation ledger](marth_roy_script_translation.md).
- Refreshed copied client templates and rebuilt Rook unchanged, replacing her
  installed package for the host layout without another Collection entry.
  Rendered ribbon comparisons, side-special/counter and shared-engine/CPU
  acceptance remain open. Continue roster translation; the broad goal is active.
