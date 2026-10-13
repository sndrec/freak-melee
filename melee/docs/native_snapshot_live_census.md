# Live explicit-component census

`--snapshot-census` measures the existing owner serializers in an offline match.
Every 120 controller samples, after normal simulation and HSD drawing, it queries
each available size, saves into independent host scratch, and validates the
candidate against the current live owners. All 26 components are attempted even
when an earlier owner rejects. It never loads state or substitutes a new game
representation.

Current main-camera checkpoint: `camera-state-oct11-eight-b` passes all 26
components at ten boundaries in both stock and interpreted eight-actor scenarios.
Diagnostic on/off and previous/current gameplay traces match exactly. The
1,424-byte camera component preserves the actual scalar/debug/coefficient state;
HSD class and GObj components retain its graph payloads and generations. See
[main camera ownership](native_camera_snapshot_ownership.md) for connected
restore evidence, native layout corrections and candidate dependency limits.
This census still performs no aggregate Load or deadline acceptance.

Current camera-target checkpoint: `camera-targets-oct11-eight-a` passes all 25
components at ten boundaries in both eight-actor scenarios. Diagnostic on/off
and previous/current gameplay traces match. The target component adds 8,456
bytes for the requested 70 records and original active/free topology, including
free history. See [camera target ownership](native_camera_subject_snapshot_ownership.md)
for actual sixteen-tick restoration and candidate/lifetime limitations. Main
camera state and full aggregate Load/timing remain open.

Current player checkpoint: `player-oct11-eight-final` passes all 24 components
at ten boundaries in both eight-actor scenarios, with exact previous/current
and diagnostic on/off traces. The player component adds 23,080 bytes for all six
records, stale/statistic storage, two counters and object/member references.
See [player ownership](native_player_snapshot_ownership.md) for the separate
restore fixture, direct source split and dependency limits. No aggregate Load
or complete timing acceptance is performed by this diagnostic.

Current GM checkpoint: `gm-loop-oct11-eight-final` passes all 23 components
at ten boundaries in both eight-actor scenarios, with exact previous/current
and diagnostic on/off gameplay traces. The loop and mapped-input components
add 216 and 264 bytes. See [GM ownership](native_gm_snapshot_ownership.md) for
actual-loop restore, scene/root guards and dependency limits. This diagnostic
still performs no Load and does not establish aggregate performance.

Set `FREAK_MELEE_SNAPSHOT_CENSUS` to an output filename, or leave it unset for
stdout. The diagnostic reserves and touches one 64 MiB host scratch buffer before
scene entry, then frees it at shutdown. It does not allocate scratch per tick,
use the HSD heap, protect pages, or include its buffer/control/timings in `.mstate`.
The buffer capacity is a diagnostic ceiling, not measured match-state size.

Current clock/RNG checkpoint: `clock-random-oct11-eight-a` passes all 21
components at ten boundaries in stock and interpreted eight-actor matches.
The scheduler and generator add 56 and 8 bytes respectively. Exact component
restoration and previous/current trace evidence are described in
[clock/RNG ownership](native_clock_random_snapshot_ownership.md). The live
census still performs no Load and does not establish full-match timing.

The flag refuses netplay and rollback probes/benchmarks, including a subsequent
room adoption. It accepts all-CPU direct matches. `--object-census` can accompany
it; the verification cohort uses that same untimed observation in both runs.

## Verified cohort

`tests/native/native_snapshot_census/run.py` runs four stock Ice Climbers pairs
and four independent interpreted pairs, each with and without the new diagnostic.
The accepted source family is copied into an isolated content store; its canonical
package and installation record are pinned. This reuses an existing fighter
translation rather than republishing source or modifying the user's collection.

Final evidence: `A:/FreakMelee-test-archives/snapshot-census-oct11-eight-final/`:

- Four hidden, muted, rendered Final Destination runs consume 1,200 controller
  samples each with medium items. Both scenarios have eight actual native actors.
- The interpreted scenario has eight script actors at every sampled boundary.
- All populated fighter, companion, action, damage, item, resource, contact and
  RNG trace families match exactly with the diagnostic enabled versus disabled.
- Thirteen components save and validate at all ten sampled boundaries in each
  scenario. Three reject; their rejection is explicit in every batch and summary.
- Independent verification checks all 15 pinned inputs, the complete owner set,
  attempt/success counts, four incompatible CLI combinations, and the diagnostic
  object's exclusion from `.mstate`.
- Earlier `eight-a` stopped in the runner, `eight-b` rejected invalid CLI setup;
  `eight-c` and `eight-d` are intermediate successful probes. Only `eight-final`
  is the final source/output checkpoint.

Representative interpreted-component high-water sizes:

| Component | Bytes |
| --- | ---: |
| Eight Fighter records and owned satellites | 153,656 |
| Eight interpreted fighter actors | 1,904 |
| Animation pools | 312,024 |
| Ordinary class-piece chunks and class state | 1,273,184 |
| Ordinary ObjAlloc payloads and descriptors | 233,776 |
| Reserved HSD graph storage | 189,432 |
| Selected OS heap metadata, without payload | 184,104 |

These are component peaks, not simultaneous aggregate totals. Empty item/effect
VM components still carry their 16-byte owner headers. This workload does not
exercise active scripted item or effect actors; their connected restore evidence
remains in the separate article-resource and scheduler fixtures.

The interpreted cohort's sum of mean successful-component save costs is about
734 us, size-query costs 296 us, and validation costs 411 us on this run. Those
are instrumented observations with repeated readiness checks and full rendering,
not an optimized aggregate benchmark. No load is timed, and missing owners are
excluded from these costs. They do not establish the fifteen-resimulation-tick
deadline or a sub-100-us snapshot. Save and validation must be profiled separately
after aggregate integration; the expensive animation/class/heap paths are now
visible instead of being attributed to the script interpreter.

## Retained render-context integration

The initial census found GObj, collision, and article-resource capture rejected
because their idle guards required `HSD_GObj_804D7814 == NULL`. In the real game,
`lbshadow.c` assigns the
fighter GObj before shadow rendering and retains that pointer afterward.
`gobj.c`'s ordinary render callbacks save and restore that previous value. Thus
the field is a retained render context even after callbacks have finished.

The census reports this as `render_context_present`, separately from the actual
invoked proc, proc GObj, max-GX callback marker, deferred flags, script activity,
and construction claim. In this cohort all those active-callback indicators are
idle, collision epochs/source/description agree, and article scene/model owners
are prepared. A non-null retained render context alone is not proof of an active
callback.

That integration is now implemented. GObj snapshots preserve the retained context
pointer, validated against serialized occupied GObjs. Actual render dispatch
maintains `HSD_GObj_RenderCallbackDepth`, including nested callbacks; idle guards
use that counter instead of interpreting the retained pointer as activity. Scene
preparation/retirement uses the same distinction. Existing shadow/render context
semantics are unchanged, and no new wrapper callbacks or pointer translation run.

New captures reject a foreign/free context because its payload is not covered by
the occupied-record component. Loading validates the candidate's context instead
of the current context's occupancy, allowing restoration after the saved object
was destroyed or reused. Arbitrary stale context backing is not silently omitted.

Evidence at `A:/FreakMelee-test-archives/render-context-oct11-*`:

- `owner-a`: actual scheduler/render dispatcher fixture; 41,360 checks, including
  retained-context destruction/resurrection, exact re-save, nested callbacks and
  16 malformed-candidate refusals. The counter returns to zero and the original
  context survives dispatch. Capturing/loading inside callbacks still refuses.
- `collision-b`: actual collision/allocator/graph fixture, 149 checks and 14
  malformed candidates; retained context permits idle capture/load, and active
  callback depth still refuses. `collision-a` was a fixture compile failure from
  a missing free-function declaration and is not acceptance evidence.
- `resources-a`: connected ordinary/animated scripted resource restore, with
  3,083/4,813 checks. The registry capture/load executes with a retained fighter
  context; actual graph, scheduler, Fighter/Item VM and borrowed parts are restored.
- `eight-a`: four rendered hidden/muted 1,200-sample runs, with eight actual actors
  and eight interpreted actors in the source-family scenario. **All 16 owners**
  save and validate at all ten sampled boundaries in both scenarios. GObj peaks
  at 197,504 bytes and Final Destination collision at 1,680 bytes in the stock run.
- `independent-verification.json`: rechecks the fixture/source pins, all live-owner
  success counts, and every populated old/new trace family against the previous
  client. Both stock and interpreted gameplay traces match exactly. Diagnostic
  on/off comparisons also match. This is not full aggregate restoration evidence.

## Remaining acceptance

The [controller checkpoint](native_controller_snapshot_ownership.md) adds the
complete HSD raw queue, status history and rumble pool as the nineteenth owner.
`controller-snapshot-oct11-eight-a` passes all nineteen Save/Validate components
at all ten boundaries in stock and interpreted eight-actor matches, with exact
diagnostic-on/off and previous/current traces. Controller state is 1,896 bytes
in both scenarios. The separate actual controller restore fixture reproduces
twelve queued-input/rumble ticks exactly using controlled external devices.

The HSD control checkpoint at
`A:/FreakMelee-test-archives/hsd-control-oct11-eight-final/receipt.json` passes
all 18 components at ten boundaries in both 1,200-sample stock and interpreted
eight-actor scenarios. HSD selector state is 16 bytes in each. The expression
buffer is allocated (424 bytes) in the stock scenario and empty (24 bytes) in
this interpreted source-family scenario; the actual restore fixture exercises
both forms and the transition between them. All diagnostic-on/off and previous
`render-context-oct11-eight-a` reference/current trace families match exactly.
`hsd-control-oct11-final-verification.json` independently rechecks the 3,335
frozen/original owner pins, 354 allocator pins, 22 live pins and mirrored OS
snapshot headers. These are component capture/validation and standalone
restoration checks, with the limitations below.

This is a partial owner measurement, not a complete explicit match snapshot.
It excludes remaining match/world/camera/effect/dynamics globals, other OS heaps
and still-unowned variable storage. HSD heap selection and the persistent RObj
expression buffer now have explicit components; see the
[HSD control-state contract](native_hsd_control_snapshot_ownership.md).
Successful validation only checks an immediately saved candidate against current
live companions; it does not prove resurrection or cross-owner graph closure.

The production page oracle remains in place. Finish the ownership manifest and
aggregate restore order, test real creation/destruction and scripted borrowers,
compare resimulation against the oracle, then benchmark complete save/load and
fifteen replayed ticks with eight live scripted actors. HSD drawing and its RNG
effects remain part of those ticks.


## 2026-10-11 shared stage checkpoint

The [shared stage owner](native_stage_snapshot_ownership.md) is the 27th
component. It captures StageInfo, joint-update storage, spawn cache and mutable
ground scale/lighting colors; map roots have GObj/Ground generation claims and joint/camera
models have class backing claims. Stage startup establishes the lease only
after its deferred callback queue has drained.

The census runner now requires every component's saved and validated counts to
match all sampled boundaries; presence of a summary alone is not acceptance.
The selectable-stage runner's `--snapshot-census` also requires shared-stage
capture/validation on every sampled boundary. Both retain drawing and compare
actual populated gameplay traces; neither performs a live Load.

This shared component does not own stage-private globals or prove every hazard
has a complete snapshot. Aggregate restore and complete-match performance
remain open; production uses the page oracle.


2026-10-11 stage-private batch: the 28th component captures actual Corneria,
Venom, Green Greens and Brinstar Depths control records with paired actor
references and the indirect cannon-position cache. All 225 donor function bodies
are preserved except direct storage names. See
[stage-private hazard ownership](native_stage_hazard_snapshot_ownership.md) for
fixture, 2,400-sample stage and interpreted-eight-actor evidence and limits.
Other private globals, borrowed/variable payloads and aggregate restoration
remain unfinished; the 286-declaration audit is not an ownership-completion claim.


2026-10-11 Brinstar expansion: the existing stage-hazard component now owns the
actual cooldown, debris count, base positions and twenty bubble entries. Live
actor/joint borrowers have generation/class backing claims; dormant merged
joint aliases are preserved without dereference. The component is 2,800 bytes,
with no allocation/callbacks/draw during capture/restore. See
[Brinstar ownership](native_brinstar_snapshot_ownership.md) for the 76 added
restore checks, six 4,800-sample cohorts and interpreted-eight-actor evidence.
All 28 components still capture/validate; this does not complete stage-private
or aggregate restoration ownership.


2026-10-11 moving-stage expansion: the existing hazard component now includes
Kongo's two bank/fifteen motion-joint borrowers and Icicle Mountain's row table/
extra-row handles. Actual storage and initializer values are preserved; the
private Kongo parameter definition is removed from its included static header.
The component remains 2,800 bytes. See
[moving-stage ownership](native_moving_stage_snapshot_ownership.md) for the 113
additional restore checks, five live cohorts and interpreted-eight-actor proof.
All 28 components capture/validate; aggregate restoration remains unfinished.


2026-10-11 Green Greens satellite update: the 29th explicit component owns the
actual position/block arrays through optional Ground-slot storage, replacing
two heap allocation/free pairs. It serializes occupied records only: one grid
is 2,112 bytes; empty scenes are sixteen bytes. See
[grid ownership](native_green_grid_snapshot_ownership.md) for lifecycle,
controlled/live evidence, reservation versus payload size, and cell-borrower/
aggregate closure limits. This replaces the array payload gap; it does not
establish whole-stage graph restoration or complete-match performance.


2026-10-11 Green Greens active-cell borrowers: the existing 29th component now
includes GObj/Ground and GObj/Item generation pairs plus retained class spans
and known type identities for live cells. Capture/load check actor roles,
material ownership and joint agreement; inactive stale aliases are never
followed. One grid is now 6,192 bytes, empty scenes remain sixteen bytes.
See [grid ownership](native_green_grid_snapshot_ownership.md) for controlled
companion-reuse/restoration and 4,800-sample live evidence. This closes local
cell claims, not candidate-wide graph validation or whole-match restoration.


2026-10-11 secondary dynamics/wind owner: the 30th component captures the actual
320 bone-dynamics nodes, eight wind fields, internal free/active heads and
control values in 54,312 bytes. Direct heap allocations and the redundant old
initialization entry are removed; scene callers use `lbDynamics_Reset` directly.
Source arithmetic/update decisions are preserved. Candidate topology validates
before writes, without following raw model joints or external descriptor heads.
See [dynamics ownership](native_dynamics_snapshot_ownership.md) for exact owner
and live evidence; external descriptor/joint/wind-borrower agreement and complete
aggregate restoration remain open. The broad page oracle is still active.


2026-10-11 car/wind stage expansion: the existing hazard component now owns
Mute City's thirty car records/order/collision control and Castle's wind
pointer/angle/mode. Mute materials use Item generation/role claims; Castle
borrows a generic retained wind-slot reference from the dynamics owner. The
component is now 4,488 bytes, still thirty census components overall. The old
private mutable car header is removed and the sort's order[-1] access is fixed.
See [car/wind ownership](native_car_wind_snapshot_ownership.md) for connected
restoration, source-preserving changes and bounded live evidence. Candidate-
wide graph/collision agreement, other private/header state and full aggregate
restore/history/performance acceptance remain open.


2026-10-11 Poké Floats/Battlefield expansion: the existing hazard component owns
actual forty-four collision-map records with unchanged initial values and the
original forty-two-entry updater, plus Battlefield's mode/parameter pointer.
Scene identities are frozen; used joint borrowers have class span/type claims.
Unused tail pointer bits stay dormant. Safe generic class identity admission
replaces unchecked type-size reads. The component is 5,720 bytes, still thirty
census owners. See [Poké Floats ownership](native_pura_snapshot_ownership.md) for
connected restoration and bounded live evidence; class occupancy/model/collision
agreement and complete aggregate restoration/performance remain unfinished.


2026-10-11 Big Blue/coverage-audit update: the existing stage-private component
owns twelve actual track/offset records and captures the actual loaded x18 mode
flag without a live mirror. Indices/padding remain frozen by the scene lease;
other parameter bytes are not implicitly claimed. Component size is 5,728 bytes.
The compiler-backed stage audit now includes private headers/static locals and
conservative pointer aliases, finding 241 eligible declarations/115 candidate
roots across 32 units with no reported eligible location gaps. See
[Big Blue ownership](native_big_blue_snapshot_ownership.md) for evidence/limits;
this is not an interprocedural, pointee or complete snapshot coverage proof.
Shared generator backing and other world/variable/HUD/effect state, candidate-
wide closure and full aggregate rollback acceptance remain unfinished.


2026-10-11 shared stage generator: the 31st explicit component captures the actual
81-entry table, control/driver state and occupied-entry GObj/Item generation
claims in 5,896 bytes. The allocation-only helper/private header are removed;
source scanning/retirement decisions and constructor reset loops are preserved.
See [generator ownership](native_stage_generator_snapshot_ownership.md) for
controlled save/load, slot resurrection, source/body/section evidence and four
4,800-sample old/current rendered stage comparisons. Stock and interpreted
eight-actor scenarios pass all 31 Save/Validate components at ten boundaries;
previous/current and diagnostic-on/off traces match. No live aggregate Load is
performed. Corneria/Brinstar variable generator configurations and transformed
spawn buffers remain outside this table owner, alongside other world/HSD/HUD/
effect state, candidate-wide closure and full restore/history/performance work.
Production retains the broad page oracle.


2026-10-11 spawn configuration owner: the 32nd component captures the actual
configuration prefix and transformed segments used by Corneria and Brinstar,
including the constructor cursor. Configurations now use fixed scene storage;
original interpolation/RNG/callback decisions remain. Serialized sizes are
sixteen bytes empty, 200 for Corneria and 464 for Brinstar. See
[spawn ownership](native_stage_spawn_snapshot_ownership.md) for controlled source
update/restore and malformed/lease/occupancy evidence. Three 4,800-sample stage
comparisons pass, plus stock/interpreted eight-actor capture/validation of all
32 components. Previous/current and diagnostic-on/off traces match. Persistent
particle-generator configurations, other world/HSD/HUD/effect state and complete
candidate-wide restore/resimulation/performance remain open. Production retains
the broad page oracle; no live aggregate Load or total latency claim is made.


2026-10-11 Rainbow Cruise satellite update: the existing stage-hazard component
now owns actual seventeen moving-platform and twenty disappearing-platform
records, their owning GObj and parameter binding. Two direct HSD allocations are
removed; original initializers/state machines/collision callbacks remain with
direct storage binding. All 37 joints have class span/type claims, and the actual
Ground borrows wind through the dynamics owner. The component remains 5,728 bytes,
still 32 census components. See [Rainbow ownership](native_rainbow_snapshot_ownership.md)
for source-update/restoration, stale companion and bounded live evidence. Three
4,800-sample comparisons and stock/interpreted eight-actor Save/Validate pass with
previous/current and diagnostic-on/off trace equality. Remaining world/attribute/
HSD/HUD/effect state, reflection userdata, candidate-wide closure and complete
aggregate restore/resimulation/performance are unfinished. The broad page oracle
remains active; no live aggregate Load or complete latency claim is made.


2026-10-11 Fountain reflection owner: the existing stage-private component now
captures actual fixed reflection userdata, its matrix/image/parameter bindings,
camera GObj generation and retained CObj class span/type. The userdata heap
allocation is removed; source reflection rendering/matrix calculation remains.
The component is 5,768 bytes, still 32 census owners. GPU framebuffer pixels are
native renderer cache output keyed by the destination address; normal drawing
remains enabled. See [Fountain ownership](native_fountain_snapshot_ownership.md)
for source classification and local restoration guards. Three 4,800-sample
rendered comparisons and stock/interpreted eight-actor Save/Validate pass with
previous/current and diagnostic-on/off trace equality. Other world/HSD/HUD/effect
state, aggregate graph/floor/camera agreement, graphics regeneration during full
rollback and complete restore/performance remain open. The broad page oracle
remains in production; no live aggregate Load or full timing bound is claimed.


2026-10-11 effect controls: the 33rd component captures actual load mode,
asynchronous effect count, allocation serial, animation count/queue and eight
alpha/ID rows in 1,880 bytes. Table definitions moved to efdata.c; source consumers
and effect-library bodies remain unchanged. Active animation joints have class
claims; dormant queue tails retain raw pointer bits without being followed.
Parameter objects have GObj generation claims. See
[effect control ownership](native_effect_control_snapshot_ownership.md) for
controlled restoration and source setter evidence. Stock/interpreted eight-actor
capture/validation passes all 33 components with previous/current and diagnostic
on/off trace equality. Effect/queue payloads retain ObjAlloc ownership; particle/
generator/AppSRT globals, asset registries, candidate-wide graph agreement and
complete aggregate restore/performance remain unfinished. Production retains the
broad page oracle; no live aggregate Load or full timing bound is claimed.


2026-10-11 particle controls: the 34th component captures actual particle/
generator/AppSRT counters, scratch bits/ID cursor, sixteen particle heads,
pending/active generator heads, deferred SList root, camera/joint attachments and
prepared bank/configuration/hooks in 4,128 bytes. Actual globals reside in
particle_state.c, with direct source access and no live mirror. Ordinary physical
slot and class span/type claims connect to existing payload companions. AppSRT
now uses sizeof(HSD_psAppSRT), correcting the x64 184-byte record versus the old
164-byte request/168-byte slot. See
[particle ownership](native_particle_control_snapshot_ownership.md) for limits.
Three 4,800-sample comparisons and final stock/interpreted eight-actor Save/Validate
pass all 34 owners, with exact prior/current and diagnostic-on/off traces. List/
ID/callback/graph agreement, asset/resident/world/HSD/HUD closure and complete
aggregate restore/resimulation/graphics/performance remain open; broad pages
remain the production oracle. No live aggregate Load or full latency claim.


2026-10-11 retained-reference optimization: class and ordinary ObjAlloc queries
return spans by value, removing repeated output-buffer directory scans. Class
epoch zero remains valid at startup. All 34 components pass stock/interpreted
eight-actor observation checks; Fountain and Green Greens pass rendered old/new
comparisons. Sampled particle-control Save means fall to approximately 14 us.
See [reference cost checkpoint](native_snapshot_reference_cost.md) for exact
measurements, startup regression evidence and unchanged validation boundaries.
This is not aggregate restore or worst-case rollback acceptance.


2026-10-11 live self-restore update: the offline census can now retain distinct
captures for all 34 owners, invoke their actual Loads at the same boundary, and
compare every recapture byte for byte. Stock/interpreted eight-actor runs and
Fountain/Green Greens/Corneria pass 4,760 Load/recapture pairs, with unchanged
rendered traces. See [self-restore checkpoint](native_snapshot_self_restore.md)
for exact capture sizes, Load timings, host scratch and scope. Observation-only
mode remains the default. Same-boundary restoration does not establish history
rewind, changed-input resimulation, complete candidate graph closure or rollback
latency; production retains the broad oracle.


2026-10-12 bounded older-capture update: the census can restore its own capture
after eight normal ticks, compare all 34 records, report raw target-oracle gaps,
and recover the complete current endpoint before continuing. Stock/interpreted
eight-actor and three stage cases pass 170 older Load/recapture pairs, including
creation/destruction/reuse, with unchanged continuing gameplay traces. See
[rewind diagnostic](native_snapshot_rewind_diagnostic.md) for the recovery-buffer
contract, raw coverage findings and limitations. This supersedes the earlier
current-boundary-only diagnostic precondition for this explicitly safeguarded
experiment; it does not add arbitrary candidate loading or run resimulation.


2026-10-12 VS/item/HUD update: component 35 captures actual VS controller, ambient
item-pick state/payloads and damage-HUD state in 14,088 bytes. Prepared callback/
descriptor/table bindings are leased; HUD GObj/class references are checked before
Load, with dormant pointer bits preserved. See
[versus control ownership](native_versus_control_snapshot_ownership.md) for actual
storage, source-body verification, fixtures and live old/current restoration.
All 35 owners pass stock/interpreted eight-actor and three-stage checks; the raw
comparison no longer identifies the three named control records. Remaining state
classification/ownership, full graph preflight and resimulation/performance remain
open; the production broad oracle is unchanged.


2026-10-12 particle display update: actual display serial, sixteen per-link sort
serials and seventeen cached non-edge heads now reside in psdisp_state.c and are
captured by the existing particle control component (4,824 bytes, +696). The
source sorter mutates particle chains, and AppSRT matrix caching depends on this
serial, so restoring only particle payloads was incomplete. Actual source sorter
replay, malformed/overlap checks, 35-owner stock/interpreted eight-actor and three
stage cohorts pass; display function bodies are unchanged except direct storage
names. See [particle ownership](native_particle_control_snapshot_ownership.md)
for the source-backed distinction between persistent caches and draw-call scratch.
The final particle-display-oct12 independent receipt checks source/exe pins,
sections and unchanged traces. Remaining gaps, complete candidate closure,
rendered/changed-input resimulation and total timing remain open. Production
continues to use the broad oracle.


2026-10-12 exact pool coverage update: the safeguarded older-capture report now
classifies every differing raw byte using restored current GObj/GObjProc/fighter/
Ground/item/ItemLink/AObj/FObj pool geometry. Full raw ranges/totals remain intact;
occupied payload, metadata and active free-order prefix are required state, while
free payload, stride padding and inactive free-order tail are counted separately.
All eight pool bindings/spans are validated before reporting; malformed/overlapping
registrations refuse. No state writes, callbacks, simulation or allocation occur.
Stock/interpreted eight-actor and Fountain/Green Greens/Corneria cases pass all35
restoration checks and exact continuing traces, with zero required-pool state
differences. Only 152/154/178/190/214 bytes remain unclassified in these intervals;
this is not a global completeness bound or permission to ignore those bytes.
See [pool coverage](native_snapshot_pool_coverage.md) and the final
pool-coverage-oct12-independent-verification receipt for precise counts/evidence.
Remaining state ownership, full candidate topology, compact resimulation and total
performance remain open. Production broad snapshots and endpoint recovery are
unchanged.


2026-10-12 texture identity update: component 36 captures/restores the actual
Aurora texture/TLUT identity counters in eight bytes, using its existing region
query at the GM/script idle boundary. No Aurora allocation/render decisions change;
all counter bit patterns, including pending overflow at zero, are preserved.
See [texture identity ownership](native_texture_identity_snapshot.md) and the
texture-identity-oct12 receipts. Source-counter repeat fixture (162 assertions),
stock/interpreted eight-actor and three-stage rendered cohorts pass all36 owners,
with final source/executable pins and unchanged traces. Required registered pool
state has zero raw differences; unclassified residuals are 149/151/175/188/212 bytes
in these intervals. These are not universal completeness or performance bounds.
Next extract a shared compact aggregate capture/load path from census and exercise
rendered/changed-input resimulation. Complete candidate closure, residual ownership,
other resource lifecycles and final timing remain open; production broad oracle
and the bounded diagnostic's endpoint recovery remain unchanged.


2026-10-12 shared compact API update: moved the direct 36-owner Capture/Load
implementation into native_match_snapshot_capture.c / native_match_snapshot_load.c
with native_match_snapshot.h. Removed old self-restore implementation/header and
updated callers, graph/build/test dependencies directly; no forwarding aliases.
The census now only owns reporting/comparison and uses shared calls for all captures
and Loads. Optional measurements/reporting are outside engine state, with null
capture mode avoiding timing/size-query instrumentation. Structured class preflight
replaces diagnostic printing. Source-equivalence verifies all36 expressions and
load order; real argument-span canaries, two eight-actor and three selectable-stage
cohorts pass 5,040 normal and 180 older pairs with unchanged traces and final pins.
See [shared core](native_match_snapshot_shared_core.md) for independent host-buffer
requirements and limits. Private own immutable captures still require resident
bindings; late Load failure is not atomic. Production remains broad, and the
compact history/resimulation driver plus full closure/timing are next required work.


2026-10-12 compact rendered resimulation checkpoint: the exclusive offline replay
experiment now restores the earlier compact state, rewinds the replay cursor and
runs eight original input/simulation/HSD draw ticks before comparing the catch-up
endpoint. Both stock/interpreted eight-actor and Fountain/Green Greens/Corneria
final cohorts pass all37 owner records, every intermediate traced entry and all
six broad oracle regions with ZERO byte differences. Broad recovery happens only
after these comparisons. Existing audio journaling preserves instance handles;
host action-transition observer history is checkpointed separately, without trace
masks. Component37 restores actual lighting roots/array/count/masks in 504 bytes.
Confirmed Euler W initialization and full-width temporary corrections preserve
all XYZ arithmetic; source body and final pin checks pass. See
[compact resimulation](native_compact_resimulation.md) for failure evidence,
contracts, exact receipts and limits. This supersedes the earlier no-old-world-tick
diagnostic limit only for this safeguarded same-input experiment. Production
remains broad; changed input, longer history, complete candidate/lifecycle closure
and final eight-actor / 15-tick timing remain required before replacement.
