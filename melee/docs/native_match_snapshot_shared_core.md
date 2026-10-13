# Shared compact match capture and load

The direct implementations now contain 39 owners and live in
`src/melee/gm/native_match_snapshot_capture.c` and
`src/melee/gm/native_match_snapshot_load.c`, with their private API in
`native_match_snapshot.h`. The old diagnostic-only loader source/header are
removed; its callers and graph/build/test dependencies refer directly to the
shared API. No forwarding/compatibility unit is retained.

`nativeMatchSnapshot_Capture` packs all owners into distinct eight-byte-aligned
slices of caller-reserved host backing and publishes the part directory. It keeps
all owner Save/Validate call expressions and ordering from the census. Optional
per-owner measurements retain the diagnostic size query/timing contract; passing
null skips those queries and timer calls, while still invoking owner validation.
The API performs no allocation, output logging, callback or tick. Incomplete
capture returns false with invalid parts; the backing can be partially written.
Caller-provided capacity is fixed before simulation, not grown during capture.

Backing, directory, byte-count and optional measurement spans must be disjoint,
with overflow and alignment checked before any write. Invalid argument spans
leave output metadata unchanged. All metadata outputs are required to be
independent **host** storage, not arbitrary pointers into other engine owners.
Per-owner buffer guards cover their own state; this is not a general scan proving
that caller-selected metadata avoids every mutable byte in the whole game.

`nativeMatchSnapshot_Load` keeps the exact prior dependency order: HSD memory / OS
heap metadata, ordinary/reserved allocation state, typed object pools, then roots,
scene/control state and script actor bindings. Actor/root and saved-class preflight
remain in place. Class preflight returns structured data rather than printing.
Optional load reporting records per-owner timing and preflight counts; the shared
loader itself never prints. Its null-report path skips timer queries. No class or
actor payload behavior changes were made as part of this extraction.

This is an own-capture API with immutable bytes and resident code/scene/package
bindings. It trusts capture-time validation and does not provide complete arbitrary
candidate prevalidation or atomic multi-owner rejection. A late Load refusal can
leave a partial world. Until the remaining closure and resimulation gates pass,
experimental callers must recover/abort and production keeps the broad page oracle.

The census owns reporting and comparison separately. It calls the shared API for
normal samples, same-boundary recaptures, older restoration and older recaptures.
Recaptures and silent endpoint captures use null measurement mode; existing Load
cohorts use the report-enabled path. Observation-only mode now also packs distinct
slices in its existing 64-MB host reservation, rather than reusing one scratch slice.
No extra per-tick allocation was introduced.

On the first real census boundary, fail-fast canaries exercise zero capacity,
directory/backing overlap, byte-count/backing overlap, measurement/directory
overlap and overflowing address spans. They must refuse without changing directory,
measurement or byte-count metadata, before reaching any owner serializer.

The source-equivalence audit compares all 36 capture call expressions and all 36
load expressions/order against the previous committed implementations. Live
stock/interpreted eight-actor and selectable-stage cohorts independently require
exact component recaptures, aggregate preflight canaries, eight-pool raw accounting
and unchanged gameplay traces. This is a shared foundation for compact history;
it is not yet a production history or a compact resimulation acceptance result.


Final 2026-10-12 evidence under A:/FreakMelee-test-archives/:

- shared-snapshot-oct12-source-equivalence.json: exact ordered equality of all 36
  Capture owner call expressions (after moving local buffer names) and all 36 Load
  expressions/order; no allocation/logging calls in the shared implementation.
- shared-snapshot-oct12-eight-a: stock and interpreted eight-actor cases each pass
  360 normal and 36 older Load/recapture pairs, all preflight canaries, exact
  continuing diagnostic-on/off and previous/current reference traces. Capture
  peaks remain 2,622,936 / 2,794,784 bytes. Null measurement captures are exercised
  by same-boundary/older recaptures and silent current-endpoint captures.
- shared-snapshot-oct12-stages: Fountain, Green Greens and Corneria each pass 4,800
  rendered samples, 1,440 normal and 36 older pairs, preflight canaries and exact
  old/new traces. The initial build exposed missing diagnostic-readiness includes;
  the final build adds only those required declarations and passes.
- shared-snapshot-oct12-independent-verification.json: final source/executable
  pins, aggregate canaries, eight-pool accounting and unchanged traces verified.
  Four build targets and focused allocator/pool/GObj-consumer CTests pass.

Both report-enabled Loads and null-measurement Captures are tested here. The
null-report Load path has the same ordered owner expressions with timer calls
conditional on the report pointer; it has not yet been exercised by the live
rollback driver. Full compact resimulation, changed-input correction, arbitrary
candidate closure/atomicity, history lifecycles and final performance are open.
Raw residuals remain 160/148/176/188/212 bytes in these five cases; byte counts
can change with addresses/padding and are not an across-build semantic metric.
Required registered pool state remains exact. The broad safeguard/oracle is
unchanged and no partial older world is ticked by these tests.


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
