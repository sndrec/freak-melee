# Bounded older-component restoration and raw coverage diagnostic

The offline census can now retain all 34 own validated component captures at
input sample 240, advance normally for eight input ticks, load the older captures
and require every recapture to match. Immediately afterward it restores the
complete preserved current endpoint byte for byte before any further tick or
render. This tests changed-state restoration while incomplete state coverage
still exists. It does not simulate the older state or claim atomic rejection of
arbitrary candidates; a late component refusal can temporarily leave a partial
state, which this diagnostic recovers through its endpoint safeguard.

Enable `--rewind` in tests/native/native_snapshot_census/run.py (implies self-
restore), or use --snapshot-census --rewind in the selectable-stage runner. Only
the diagnostic client receives FREAK_MELEE_SNAPSHOT_REWIND=1. Normal gameplay and
production netplay do not run this experiment. Original drawing remains enabled
between captures; no rendering/simulation/callback runs during Load/comparison.

The safeguard uses the existing broad region inventory, with pre-reserved host
storage for target and current endpoints. Its 512-MB capacity is split into two
256-MB halves; it is diagnostic recovery/coverage scratch, not serialized compact
state or the proposed production history. The current observed region inventory
is 193,228,836 bytes. Component captures use a separate fixed 64-MB host buffer,
with approximately 2.6-2.8 MB used in the eight-actor cohort. No sampled-tick host
allocation occurs. Active page-tracking sessions are refused. Large oracle copies
and raw comparisons are outside component timing and do not establish latency.

After older component restoration, the diagnostic compares every tracked target
oracle region and prints up to 4,096 raw difference ranges per region, with a
complete differing-byte/range count. Differences can be legitimate inactive
payload, padding or catalog tails; they must be classified from source, not
silently treated as desyncs or all excluded. Continuing gameplay follows recovery
of the original current endpoint, whose raw regions must compare exactly.

Final evidence in A:/FreakMelee-test-archives/:

- rewind-oct12-endpoint-pinned: actual safeguard Save/Restore/Matches/difference
  code with controlled region discovery and tracking status. Nineteen checks:
  exact paged/metadata restoration, mismatch reporting, undersized/aliased/null
  refusal and refusal when page tracking or discovery is unavailable.
- rewind-oct12-eight-pinned: stock and independent interpreted eight-actor runs
  each pass all 34 older Loads/identical recaptures and complete endpoint recovery.
  Nineteen/twenty owner records differ before restoration, including fighter/RNG
  and interpreted fighter-VM state. Two GObjs are created, one destroyed and one
  live slot reused across the interval. Normal ten-boundary self-restore also
  passes; continuing gameplay equals diagnostic-off and prior accepted traces.
- rewind-oct12-stages-pinned: Fountain, Green Greens and Corneria pass 4,800
  rendered old/new samples, 34 older Loads/identical recaptures and endpoint
  recovery each. Fountain shows creation/destruction/reuse; Green Greens shows
  destruction; Corneria shows creation. Normal forty-boundary self-restore passes.
- rewind-oct12-independent-verification.json: final source/executable pins and
  prior/current traces verified. Total older Load/recapture pairs: 170, plus
  4,760 normal same-boundary pairs. Four build targets and focused pool/ObjAlloc/
  GObj-consumer CTests pass.

Final raw target differences are 2,389 bytes (stock), 2,449 (interpreted), 926
(Fountain), 278 (Green Greens), 3,202 (Corneria). These are observed cases, not an
upper bound or proof of complete state coverage. Initial PDB/source correlation
identifies likely coverage targets in VS controller, random item spawner, HUD and
particle display state, plus texture/TLUT identity counters. Pool free-order tails
and inactive payload arrays appear too and need separate exact classification.
The raw symbol-candidate archive records nearest PDB symbols, not automatic proof
of each symbol's semantic ownership.

Full state closure, class/ObjAlloc/free-list and particle/VM/article relationships,
complete candidate prevalidation/publication, changed-input and rendered
resimulation, history through other lifecycles/scenes and final performance
acceptance remain unfinished. Production retains the broad page oracle.


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
