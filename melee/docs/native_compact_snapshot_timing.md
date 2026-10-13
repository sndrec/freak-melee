# Compact capture and restore timing

`FREAK_MELEE_COMPACT_SNAPSHOT_BENCHMARK=1` enables sixteen complete capture/load
measurements at each census checkpoint. The live census runner exposes this as
`--benchmark`. Buffers, scene definitions and scratch are already reserved.
The measured calls use null measurement/report outputs: per-owner profiling
timers and optional size-query passes are disabled. They still perform their
normal ownership and candidate validation. Timer pairs surround each whole call;
logging, sorting and final exact component recapture happen afterward.

This is a warm same-boundary snapshot microbenchmark, not a simulation budget.
It does not measure fifteen resimulation ticks, GPU completion, shader compilation
or production page tracking. Tests retain normal HSD drawing/RNG and separately
verify rendered resimulation. Trace/census work elsewhere in the process can
affect CPU/cache conditions, so small timing differences should not be overstated.

## Full restore reporting

`NativeMatchSnapshotLoadReport.total_ns` now begins at loader entry and includes
candidate actor/class preflight. `preflight_ns` separates that initial work.
Earlier diagnostic totals started *after* preflight and were incomplete restore
measurements. Null reports do not call these profiling timers.

## Heap validation improvement

Aurora's `OSValidateHeapSnapshot` previously traversed and sorted the current
allocation lists to reconstruct ranges on every validation. Heap ownership
geometry is already admitted and cached for its geometry epoch. Allocation/free
only splits/coalesces cells inside those ranges. Validation now checks saved
cells against that existing sorted catalog, retaining alignment, bounds, total
size, selected heap, owner, linked-list topology, epoch and alias checks.

Capture still discovers and validates the actual current cells. No source
allocation/free algorithm or snapshot field changed. Candidate validation no
longer depends on the later live list being structurally intact: the fixture
damages its next link, then verifies and restores the valid earlier snapshot.

## Initial results

Values below are means of four checkpoint upper medians, in microseconds, for
480-input stock/interpreted eight-fighter runs. Each checkpoint has sixteen
measurements. The hardware is this Windows workstation; these are preliminary
observations, not portable guarantees or deadline acceptance.

| Case | Before capture | After capture | Before restore | After restore |
|---|---:|---:|---:|---:|
| Stock eight | 1,142.1 | 1,060.7 | 787.4 | 704.5 |
| Interpreted eight | 1,294.3 | 1,268.3 | 871.3 | 842.4 |

Before optimization, heap validation was the largest single measured owner:
about 237 us for warm restores and 219 us for the extra capture validation.
Afterward, the stock capture validation sample averaged about 171 us. Candidate
preflight was about 70-140 us in the initial cohort. Animation, ordinary class,
GObj and reservation work also remain significant. Snapshot copy size alone does
not explain the total. The aspirational sub-100-us snapshot cost is not achieved.

## Evidence

Under `A:/FreakMelee-test-archives/`:

- `compact-timing-oct12-a`: pre-optimization timings, exact component recapture
  after each measurement batch, matching continued gameplay, and exact 15-frame
  frame-zero rendered replay for both stock/interpreted eight-fighter cases.
- `compact-timing-oct12-baseline/melee_native.exe`: frozen pre-optimization
  executable; its digest agrees with the earlier receipt's executable pin.
- `compact-timing-oct12-b`: same measurements and correctness checks after the
  heap change; all 39 owners and complete raw replay endpoints match.
- `compact-timing-oct12-correction`: fresh corrected reference and repeated
  15-frame compact corrections pass for both families after the change, including
  raw endpoints, original continuation and audio deduplication.
- `compact-timing-oct12-owners`: 86,956 controlled owner/allocator assertions,
  including four new damaged-current-list validation/restoration checks. Existing
  malformed candidate, aliases, multiple heaps/ranges and capacity checks pass.
- Focused history, stage callback and GObj consumer CTests pass.

Production compact integration, complete candidate prevalidation, broader
resource lifetimes and the actual fifteen-tick frame-budget benchmark remain open.

## Saved-cell lookup checkpoint

The next optimization replaces a binary search for each saved list link with a
fixed candidate-local address index. It is built while the existing sorted,
disjoint cell/range checks run. List traversal still requires exact address
membership, owner, previous link, length and termination; free links must still
increase in address. This changes lookup cost, not accepted list topology.

The table has 131,072 entries (1 MiB of fixed host scratch), with at most 65,536
saved cells. Generation stamps separate candidates and avoid clearing the table
on every call. Wrap clears the stamps before reuse. Linear probing has expected
linear aggregate cost for normal allocator addresses; it is not a worst-case
constant-time guarantee for adversarial collisions. No allocations or new
threading are introduced, and snapshot buffers cannot alias this scratch.

Observed means of four checkpoint upper medians, microseconds:

| Case | Prior capture | Indexed capture | Prior restore | Indexed restore |
|---|---:|---:|---:|---:|
| Stock eight | 1,060.7 | 986.4 | 704.5 | 607.0 |
| Interpreted eight | 1,268.3 | 1,172.2 | 842.4 | 708.4 |

Stock heap candidate validation samples dropped from about 171.4 to 111.6 us.
The same measurement limitations above apply; the 100-us aspiration and final
fifteen-tick budget remain unmet.

Evidence under `A:/FreakMelee-test-archives/`: `heap-index-oct12-timing-a` passes
all benchmark recaptures, 39-owner frame-zero replay and complete raw comparisons
for stock/interpreted eight-fighter cases. `heap-index-oct12-correction` passes
fresh/repeated fifteen-frame corrections, original continuation and audio checks.
The final owner fixture `heap-index-oct12-owners-e` has 86,981 assertions, including
the exact 65,536-cell maximum, the first unsupported larger state, and shrinking
the heap without retirement before rejecting an address left in the previous
candidate's index. Existing malformed, epoch, alias and damaged-live-list checks
also pass. The intermediate owner-c archive retains a test-only member-name
compile error; owner-d first passed the expanded cases before a dead local
initialization was removed for owner-e. Focused CTests remain green.

## Heap writer validation checkpoint

`OSSaveHeapSnapshot` now guarantees a valid immutable candidate at an idle
allocator boundary. It already collected both actual lists, checking ownership,
previous links, alignment, size totals, disjoint sorted cells and bounds. Before
publishing, it additionally checks the collected extents against the admitted
geometry catalog. Copying those checked cells and known descriptor fields then
produces a candidate that does not need a second list traversal/index build.

Compact capture trusts that writer contract for this owner only. Restore still
calls the complete `OSValidateHeapSnapshot`; other component validation remains
unchanged. This is not an unchecked restore path, a second format or a skipped
ownership check. Snapshot bytes and source allocation/free behavior are unchanged.

The fixture forges a structurally valid current allocated list using a cell from
another heap. Capture refuses it without publishing the byte count or header,
then the original descriptors/cell are restored. Existing candidate rejection,
maximum capacity, stale lookup, geometry changes and damaged-live-list recovery
remain covered.

Observed mean checkpoint capture medians, microseconds:

| Case | Indexed writer + second validation | Validating writer alone |
|---|---:|---:|
| Stock eight | 986.4 | 884.8 |
| Interpreted eight | 1,172.2 | 1,073.2 |

These remain preliminary warm measurements. Restore was not optimized in this
checkpoint; observed variation does not establish a restore speedup. The
fifteen-tick frame budget and sub-100-us snapshot aspiration are still unmet.

Under `A:/FreakMelee-test-archives/`: `heap-writer-oct12-timing-a` passes benchmark
recapture, stock/interpreted eight-fighter frame-zero fifteen-frame replay,
all 39 records, complete raw endpoints and original continuation.
`heap-writer-oct12-correction` passes fresh/repeated corrected-input replay and
audio deduplication. `heap-writer-oct12-owners-a` passes 86,987 assertions,
including the forged live topology and publication checks. Focused CTests pass.
Source/executable pins are checked in `heap-writer-oct12-independent-verification.json`.

## Animation writer checkpoint

Animation capture previously called the checked size queries twice and then each
checked pool writer, scanning both free/occupied tables three times. It now uses
the direct writers' returned sizes, then validates their saved animation graph
before publishing the outer header/count. Decoder operation/cursor bounds, next
links, cycle rejection, controller references, counters, typed geometry, idle
boundaries and aliases remain checked. Restore retains candidate validation.

The writer's successful immutable result is valid by construction, so compact
capture also omits its separate second graph-validation pass. The old live-graph
mode has no remaining callers and was removed. No wrapper, extra format, source
animation change, new allocation or callback is introduced. A failed write can
leave partial backing bytes, as before; it does not publish the caller's count.

Observed mean checkpoint upper capture medians, microseconds:

| Case | Heap-writer checkpoint | Final animation writer |
|---|---:|---:|
| Stock eight | 884.8 | 751.2 |
| Interpreted eight | 1,073.2 | 940.4 |

The final cohort had no competing Melee test. A short test-target build also ran
in that process's initial setup period; CPU/cache/background work is not fully
controlled. Intermediate timing-b overlapped correction clients and was noisier.
These remain preliminary warm snapshot costs, not complete simulation latency.

Evidence under `A:/FreakMelee-test-archives/`: `animation-writer-oct12-timing-c`
passes exact benchmark recaptures, stock/interpreted eight-fighter frame-zero
fifteen-frame replay, all 39 records, raw endpoints and original continuation.
`animation-writer-oct12-correction` passes fresh/repeated corrected replay and
audio checks. `animation-writer-oct12-owner-c` compiles real AObj/FObj/RNG/lbAnim
and owner sources: 91 cases/1,086 checks, including malformed live pool counts
and decoder operations without count publication, source playback on read-only
streams, complete-state replay, negative controls, cycles, generations, aliases,
capacity and heap retirement. Rebuilt animation-owner/heap and focused history/
callback CTests pass. Owner-a was a retained test setup failure: this particular
runner expects the compiler directory, not the executable. Earlier timing and
owner receipts remain historical evidence for intermediate steps.

Production integration, remaining candidate/resource closure, further cost
reduction and the actual fifteen-resimulation-tick deadline remain open.

## Reservation and GObj writer checkpoint

HSD reservation save now checks the idle boundary/immutable geometry once, writes
each bin through its checked pool writer, and uses the returned byte count. It
does not first scan every pool to size the complete registry or rescan each bin
for its length. The owner publishes the final header/count only after every bin
succeeds. A late failure can overwrite caller scratch, but does not publish a
size or mutate live pools. Restore still validates the complete candidate before
writing any owner.

Compact capture also omits its second reservation-validation pass, trusting the
writer's successful own capture. Likewise, the GObj writer already checks the
complete saved graph before returning success, so compact capture omits its
duplicate GObj graph validation. GObj restore and candidate preflight checks
remain intact. No pool layout, allocation order, callback, script or rendering
behavior changes.

The controlled owner fixture checks all 760 insufficient buffer capacities,
exact-capacity byte equality, invalid final-bin free metadata and unchanged
published count/live state on refusal. It passes 88,516 checks, including 1,646
reservation checks and the existing retired-owner/epoch, active-claim, alias,
candidate-corruption and exact allocation continuation tests.

Six-interval reduced-observer history capture medians, milliseconds for fifteen
actual saves, before/after this checkpoint:

| Case | Prior reduced-observer cohort | Reservation/GObj writers |
|---|---:|---:|
| Stock eight | 15.490 | 14.109 |
| Interpreted eight | 18.473 | 15.604 |

Whole replay plus restore medians are 33.753/37.168 ms. These sequential cohorts
are not controlled performance experiments; other phase costs and background
work also vary. The stock scenario has no active reservation records, so its
observed change must not be attributed to populated reservation serialization.
The 16.67-ms interval target and sub-100-us snapshot aspiration remain unmet.

Evidence under `A:/FreakMelee-test-archives/`:
`reservation-writer-oct12-owners-a`, `reservation-writer-oct12-quiet-a`,
`reservation-writer-oct12-regression-a` and
`reservation-writer-oct12-correction-a`. Quiet and traced cohorts retain six
fifteen-tick exact compact/raw endpoints; traced reference, corrected-input,
continuation and audio checks provide separate correctness evidence.

## GObj, class and allocator capture batch

GObj capture now streams its two checked pool writers directly into the remaining
buffer and derives their lengths from successful writes. It no longer calls the
two full metadata size queries first. The live-context ownership check and full
saved scheduler/object/procedure graph validation still precede publication.
Insufficient space or an invalid second pool can change scratch but never
publish a byte count or change live state.

Ordinary class backing references now check only retained address geometry and
the idle ownership boundary. They do not traverse the class tree for every
reference. They remain explicitly weaker than object liveness or type proof:
`nativeClass_TypeKnown`, the class snapshot writer and saved class graph checks
continue validating types and counters. The class writer additionally checks
the live size-class table's header shape and exact retained allocation before
publishing. Its successful own capture is valid, so aggregate capture omits its
duplicate candidate validation. ObjAlloc's existing writer already checks the
registry, descriptor state and heap geometry; capture likewise avoids validating
that immutable output a second time. Candidate restore validators remain intact.

The census runner now pins the full Melee C/C++ source/header tree, including
all native pool writers and metadata contracts, alongside its executable,
selected Aurora sources, ISO and content. This broadens source drift detection
without changing replay or measurement behavior.

Six fifteen-tick reduced-observer interval medians, milliseconds:

| Phase | Stock eight | Interpreted eight |
|---|---:|---:|
| Compact restore | 0.914 | 0.984 |
| Fifteen history captures | 12.998 | 13.098 |
| Input | 0.746 | 0.845 |
| Source simulation | 3.805 | 5.756 |
| Complete drawing | 13.790 | 13.807 |
| Whole interval + restore | 32.347 | 34.842 |

Prior reservation-writer quiet history medians were 14.109/15.604 ms, and whole
intervals 33.753/37.168 ms. These are sequential offline cohorts with uncontrolled
cache/background variation, not a portable speed guarantee. Rendering remains
enabled and the 16.67-ms interval target and sub-100-us snapshot aspiration
remain unmet. This does not complete production compact integration or general
candidate/resource closure.

Evidence under `A:/FreakMelee-test-archives/`:

- `capture-batch-oct12-owners-a`: 88,523 checks; malformed class counters still
  refuse snapshot publication/type queries while retained geometry references
  remain usable, plus existing allocation/candidate negative controls.
- `capture-batch-oct12-gobj-a`: 17 cases and 41,381 checks, including scheduler
  callbacks, malformed candidates, first/second pool truncations, exact-capacity
  equality and corrupt second-pool refusal without publishing a size.
- `capture-batch-oct12-quiet-a`: 1,200 inputs per stock/interpreted case, six
  fifteen-tick intervals, all 234 owner comparisons and six raw endpoints exact.
- `capture-batch-oct12-traced-zero-a`: the full traced/reference cohort from
  frame zero, through READY/stage start and five later intervals, all exact.
- `capture-batch-oct12-correction-a`: fresh/repeated corrected-input replay,
  complete component/raw endpoints, original continuation and audio checks.

## Shared pool writer consolidation

`nativeMatchSnapshotPool_Save` now checks fixed geometry/count relationships up
front, then validates and writes generations/occupied records together. Its
free-order pass validates bounds, duplicates and occupied-slot conflicts while
serializing the exact order. It no longer scans complete live metadata in a
size query and then separately scans generations and occupied records to write.
No layout, generation, free-order or allocation behavior changes.

The occupied-record counter is checked **before** copying another record. A
corrupt declared used count can therefore never overrun the capacity computed
from that declaration. Too many/few occupied slots, zero occupied generations,
duplicate/out-of-range/conflicting free entries, bad peaks and inconsistent free
counts refuse publication. Failure may leave scratch bytes, but does not change
live state or the caller's written count. Successful candidates retain complete
view/load validation. Fixed validation scratch remains on the stack; no runtime
allocation, wrapper or second format is introduced.

Core fixture `pool-writer-oct12-core-a` passes 589,585 checks, including eight
malformed live-writer scenarios with guard bytes beyond the claimed capacity,
all four-slot free-order permutations/occupancy states, full 65,536-slot resource
capacity, real fighter/item payloads and exact generation/allocation continuation.
Separate reservation, GObj/scheduler and actual animation decoder/owner fixtures
also pass; animation streams remain read-only and omission negative controls
still diverge as expected.

Six fifteen-tick quiet interval medians (stock/interpreted): history captures
11.670/13.412 ms; simulation 3.643/5.748 ms; drawing 14.186/13.869 ms; whole
replay plus restore 31.014/34.793 ms. The preceding capture batch measured
12.998/13.098 ms history and 32.347/34.842 ms total. Stock improved in this
sequential cohort, while interpreted capture did not improve. Cache/background
variation is uncontrolled; these numbers do not prove a universal speedup.
The 16.67-ms target remains unmet and source drawing/render RNG stay enabled.

Evidence under `A:/FreakMelee-test-archives/`: `pool-writer-oct12-core-a`,
`pool-writer-oct12-owners-a`, `pool-writer-oct12-gobj-a`,
`pool-writer-oct12-animation-a`, `pool-writer-oct12-quiet-a`,
`pool-writer-oct12-traced-zero-a` and `pool-writer-oct12-correction-a`.
Match cohorts retain all compact/raw endpoint checks; the traced cohort includes
frame zero/READY/startup and independent references, while correction also checks
fresh/repeated changes, original continuation and audio intent deduplication.

## Opt-in CPU-only GX, 2026-10-12

Three sequential quiet offline Green Greens/items runs bracket the CPU-only
experiment with normally rendered controls. Each stock/interpreted case has
three 15-tick intervals and eight actual actors. Median drawing totals are
19.745/19.332 ms in control A, 7.991/8.042 ms with CPU-only GX, and
19.931/19.142 ms in control B. This removes roughly 11-12 ms of drawing per
interval in these cases. HSD state callbacks, matrix setup, particle work,
geometry command encoding for non-model primitives and FIFO decoding remain.

CPU-only median whole intervals, including restore and fifteen diagnostic
history saves, are 18.389 ms stock / 21.245 ms interpreted. History contributes
4.123/4.504 ms. Median per-interval totals minus that diagnostic history work
are 14.187/16.976 ms, but subtraction is not an actual production checkpoint
benchmark: it excludes any checkpoint advance, a final visible frame and other
host work. No sustained 16.67-ms deadline claim follows from this measurement.
Simulation alone is 4.888/7.434 ms for fifteen ticks in this cohort.

Evidence: `render-cpu-oct12-quiet-control-a`, `render-cpu-oct12-quiet-a`,
`render-cpu-oct12-quiet-control-b`, and independently verified source/binary/log
pins plus exact rows in `render-cpu-oct12-timing.json`. Background scheduling
and cache effects are uncontrolled. Correctness is from the separate traced,
corrected-input and real UDP cohorts in `native_cpu_only_resimulation.md`.

## Model pose preparation, 2026-10-12

After separating joint updates from model skinning/normal matrices and GX
uploads, `render-pose-oct12-quiet-a` measures three 15-tick Green Greens/items
intervals per eight-actor case. Median drawing totals are 5.211 ms stock and
5.407 ms interpreted; HSD callbacks contribute 3.421/3.534 ms, while GX end
processing contributes 1.734/1.761 ms. Earlier inline-decoder measurements in
`render-inline-oct12-quiet-a` had drawing medians 7.712/7.257 ms. These are
separate cohorts with uncontrolled scheduling/cache variation; one current
stock interval takes 8.128 ms, so this is not a universal speedup guarantee.

Current median whole intervals are 13.168/16.515 ms, including fifteen
diagnostic history saves (2.784/3.340 ms). Simulation contributes 4.380/6.637 ms.
The confirmed-checkpoint production path omits those intermediate saves, but
this measurement still does not include its checkpoint advance, final visible
frame or complete host workload. No sustained 15-tick/frame deadline is claimed.
Exact rows and current input/log hashes are independently checked in
`render-pose-oct12-timing.json`; traced and real UDP correctness evidence is
recorded in `native_cpu_only_resimulation.md`.

## Material command omission, 2026-10-12

`C:/FreakMelee-test-archives/render-material-oct12-quiet-a` measures three
15-tick eight-actor Green Greens/items intervals per stock/interpreted case.
Median remaining drawing totals are 3.563/3.516 ms, HSD callbacks
2.710/2.716 ms, and GX end processing 0.750/0.760 ms. The preceding pose-only
cohort measured drawing medians 5.211/5.407 ms. Scheduling and cache variation
remain uncontrolled; these sequential measurements are not a universal
speedup guarantee. Texture identities and mutable preparation remain active;
the new omission targets TEV and texture-coordinate graphics work.

Median complete diagnostic intervals are 11.678/14.418 ms, including fifteen
history saves (2.820/3.193 ms). Simulation contributes 4.093/6.729 ms. These
remain offline measurements with diagnostic saves, not the production
checkpoint advance plus a final visible frame or a sustained deadline claim.
Input/log hashes and exact rows are checked in `render-material-oct12-timing.json`.
Particle geometry and required display-state callbacks remain optimization work.

## Particle geometry omission, 2026-10-12

`C:/FreakMelee-test-archives/render-particle-oct12-quiet-a` measures the same
three 15-tick Green Greens/items intervals with eight stock or interpreted
actors. Median drawing totals are 3.242/4.167 ms, HSD callbacks 2.480/3.386 ms,
and GX end processing 0.672/0.686 ms. Complete diagnostic intervals are
11.088/17.954 ms, including fifteen history saves (2.593/4.890 ms); simulation
contributes 4.276/7.710 ms. Exact rows and input/log hashes are independently
checked in `render-particle-oct12-timing.json`.

Stock drawing improves modestly over the preceding material cohort, while
interpreted drawing, simulation and history all increase. Scheduling/cache
load is uncontrolled, so this batch establishes geometry omission and state
equivalence rather than a universal measured speedup. Required particle state
updates and remaining display setup still run. The diagnostic includes fifteen
intermediate saves that production confirmed checkpoints omit, and excludes
the final visible frame and complete host workload. It is not a sustained
15-tick/frame deadline measurement.

## Pixel/channel/light and particle setup omission, 2026-10-12

`C:/FreakMelee-test-archives/render-state-oct12-quiet-a` measures three 15-tick
eight-actor Green Greens/items intervals per stock/interpreted case. Median
drawing totals are 2.853/2.782 ms, HSD callbacks 2.380/2.317 ms, and GX end
processing 0.400/0.376 ms. The preceding particle cohort had drawing medians
3.242/4.167 ms. These separate short runs have uncontrolled scheduling/cache
load, so the difference is encouraging evidence rather than a universal
speedup guarantee.

Complete diagnostic medians are 10.966/13.661 ms, including fifteen intermediate
history saves (2.722/3.330 ms); simulation contributes 4.174/6.624 ms. Subtracting
diagnostic saves yields about 8.245/10.330 ms, but that is not a production
measurement: checkpoint advance, a final visible frame and the full host
workload are not covered. Production confirmed-checkpoint timing is the next
budget check. Exact rows and source/binary/log hashes are checked in
`render-state-oct12-timing.json`; traced/corrected-input and local UDP evidence
is recorded in `native_cpu_only_resimulation.md`.

## Production cycles with fifteen total reconstructed ticks, 2026-10-12

The production profiler measures between successive visible-frame submissions,
including confirmed-checkpoint operations and the next normal rendered tick.
The final-source local UDP cohort `netplay-timing-oct12-live-d` has 121
interpreted cycles with exactly fifteen hidden ticks. Each includes two restores
and two captures because incoming packets cause more than one reconstruction.
Both clients share one PC and GPU; no per-frame trace/census or owner comparisons
run inside these timing intervals. Initialization and the unfinished tail are
excluded; reporting occurs afterward from fixed storage.

| Per-cycle measurement | Median | p90 | p99 |
| --- | ---: | ---: | ---: |
| Complete elapsed time | 16.884 ms | 18.841 ms | 25.452 ms |
| Elapsed minus explicit sleeps | 16.572 ms | 18.841 ms | 25.452 ms |
| Simulation (15 reconstructed + 1 normal tick) | 7.125 ms | 8.014 ms | 9.709 ms |
| Drawing/state callbacks and final submission | 7.421 ms | 8.861 ms | 11.446 ms |
| Input processing, including any sleeps | 0.401 ms | 1.330 ms | 1.911 ms |
| Netplay begin/end operations | 1.602 ms | 2.119 ms | 3.005 ms |
| Checkpoint operations, included in netplay | 1.250 ms | 1.679 ms | 2.408 ms |

Quantiles are computed independently for each column; component medians should
not be added or subtracted as though they describe one identical cycle. Active
wall time subtracts each cycle's explicit sleeps before computing its quantiles,
but retains preemption and GPU/worker waits. Fifty-seven of the 121 cycles
exceed 16.667 ms even after explicit sleep subtraction. The median is close;
sustained fifteen-tick/frame acceptance remains unproven.

Stock has no exactly-fifteen cycles in this run: packet timing produces several
reconstructions per visible cycle, commonly totaling 21/23/25 hidden ticks.
Those groups have active-wall medians 16.222/17.317/18.642 ms. The report retains
all actual-depth groups, including higher totals. Zero-artificial-delay stock
cycles with no reconstruction have a 5.369-ms active-wall median and
16.624-ms elapsed median, including ordinary input pacing.

The profile also led to a concrete fix: historical netplay ticks now advance
the saved sampling schedule without sleeping on the live 60-Hz clock. Before
this fix, completed fifteen-tick cycles in `netplay-timing-oct12-live-c` took
roughly 256 ms, dominated by explicit waits. That historical run is diagnostic
evidence, not the current-source timing claim. Final hashes, rows, endpoints,
counter accounting and depth groups are independently checked in
`netplay-timing-oct12-live-d-verification.json`. Remaining work includes final
visible rendering cost and repeated reconstruction/checkpoint work; these
numbers do not claim all-stage/fighter or distributed-PC acceptance.

## UI layout fix and tightened performance target

The subsequent `frame-end-oct12-live-b` and `frame-end-oct12-live-c` cohorts
use the same interpreted eight-actor, Green Greens, medium-item and fourteen-
frame outbound-delay setup, with both profilers enabled. Removing repeated
unchanged UI visibility assignments reduces the final frame-end median from
3.698 ms to 1.225 ms, mainly by avoiding document relayout. The same visible UI
continues updating/rendering; this does not suppress gameplay callbacks or
effects. Full-cycle timings include the corrected final-frame path.

| Fifteen total reconstructed ticks + one normal tick | Before | After |
| --- | ---: | ---: |
| Sampled complete cycles | 135 | 80 |
| Active wall median (explicit sleeps subtracted per row) | 16.163 ms | 14.915 ms |
| Full elapsed median | 16.163 ms | 15.235 ms |
| Active wall p90 | see retained report | 17.722 ms |
| Active wall maximum | see retained report | 25.476 ms |
| Active cycles exceeding 16.667 ms | 39 | 19 |

Packet arrival/scheduling changes which rows contain exactly fifteen hidden
ticks, and both clients share one CPU/GPU. These are independent populations;
do not attribute the entire full-cycle difference to one phase by subtracting
unpaired medians. The independently checked frame-end measurements isolate the
UI improvement. Exact rows, component quantiles, counters, endpoints and hashes
are in each cohort's `*-verification.json` and `*-frame-verification.json`.

The user's updated target is **10 ms median and no more than 12 ms measured
peak** for fifteen reconstructed ticks plus the normal visible tick, using
active wall time and retaining full elapsed time separately. This includes
confirmed-checkpoint work and render submission, but not intentional frame-
pacing sleeps or a GPU completion fence. Current measurements do not meet it.
Simulation and retained state-changing drawing callbacks are the next profiling
targets. An observed peak must be evaluated over a stated workload and sample
window; this is not a guarantee against arbitrary OS scheduling delays.

The next phase-only cohort, `cpu-scalar-oct12-live-a`, disables callback clocks
and retains all source/binary/log, controller endpoint, checksum and CPU-only
frame checks. Packet timing produces only 25 exactly-fifteen cycles, below the
earlier minimum population of 32. Its verification explicitly requests no
minimum exact-depth population; correctness verification is not performance
acceptance. Those 25 cycles have active-wall median 14.396 ms, p90 16.435 ms
and maximum 19.746 ms; two exceed 16.667 ms. This is a small indicative sample,
does not isolate the modest scalar-access improvement, and misses the 10/12 ms
target. Callback attribution and the narrower CPU property fixture are recorded
in `native_cpu_only_resimulation.md`; broader stale fixture failures are also
recorded there rather than counted as passing checks.

## Final linear-walk cohort, 2026-10-13

`registry-linear-oct12-live-a` passes the complete stock zero/delay and
interpreted delay batch. All timings below include fifteen reconstructed ticks,
one normal rendered tick and production checkpoint/network work. Intentional
sleeps are subtracted per row before computing active-wall quantiles.

| Exactly-fifteen active cycles | Stock | Interpreted |
| --- | ---: | ---: |
| Cycle population | 480 | 472 |
| Active wall median | 10.475 ms | 12.698 ms |
| Active wall p90 | 11.229 ms | 13.471 ms |
| Active wall p99 | 12.510 ms | 15.575 ms |
| Active wall measured maximum | 13.644 ms | 17.258 ms |
| Simulation median (15 reconstructed + 1 normal) | 4.635 ms | 6.838 ms |
| Checkpoint work median, included in netplay | 0.470 ms | 0.606 ms |

Each interpreted cycle has exactly one restore and one capture. Previous
populations frequently had two, and packet/client scheduling still affects
these measurements. Both clients share one Windows PC/GPU; this is not a paired
experiment isolating the effect of registry changes. The interpreted full elapsed
median is 16.660 ms with ordinary pacing; active wall is 12.698 ms. Its drawing
median is 4.527 ms, including HSD callbacks 2.788 ms and render end 1.604 ms.
Quantiles are independent and must not be added as if they describe one row.

Neither stock nor interpreted meets the tightened 10-ms median/12-ms peak
target. The 2.2-ms median simulation difference and final rendered-frame work
remain useful profiling targets. The final binary contains the linear registry/
item lookup changes; the experimental invocation cache was removed after
inconclusive timing. Exact rows, source/executable/log pins and counter/endpoint
checks are in `registry-linear-oct12-live-a-verification.json`.

## Audio batch follow-up, 2026-10-13

The resampler now batches decoder reads with exact output/voice-state equivalence
and an approximately 1.7x standalone resampler improvement. See
`native_cpu_only_resimulation.md` for scope, tests and sampling limitations.
Two full netplay cohorts preserve stock and interpreted peer checksums and
endpoints, but packet scheduling yields zero and eight interpreted exactly-
fifteen cycles. Both fail the normal minimum population of 32. Correctness-only
reports explicitly set that minimum to zero; these are not performance passes.

Stock has 480 exactly-fifteen cycles in each run: active-wall medians 10.698 and
10.517 ms, measured maxima 14.112 and 14.540 ms. The eight interpreted cycles in
the second run have median 13.617 ms and maximum 15.109 ms, an insufficient
population for acceptance. Multiple reconstructions per submission produce
other depths up to 34 total hidden ticks. No sustained full-cycle gain has been
established by this local audio optimization. The latest qualifying interpreted
15-tick population remains the earlier 472-cycle 12.698/17.258-ms result; it is
historical evidence for the previous binary, not a measurement of this binary.
The **10-ms median / 12-ms measured-peak** target remains unmet.

## CPU admission follow-up, 2026-10-13

Duplicate CPU source continuations are removed for callback-free scalar
operations and initial native-kernel admission. Post-kernel source/target guards
remain; controlled rejection/retirement and isolated read-cost evidence are in
`native_cpu_only_resimulation.md`. The full `scalar-admission-oct13-live-a`
batch preserves confirmed checksums and endpoint/RNG but yields just three
interpreted exactly-fifteen cycles. Their 14.636-ms median / 14.881-ms maximum
are insufficient for acceptance or a full-path gain claim. The normal population
gate fails; the explicit zero-minimum report is correctness-only. Fifty stock
exactly-fifteen cycles measure 10.159-ms median / 11.387-ms maximum. Neither
population establishes the requested interpreted 10/12-ms target.

Several reconstructions can occur between visible submissions. The match loop
calls BeginFrame again after CaughtUp; it receives packets and reconciles before
the next normal tick, so newly arrived input can immediately initiate another
reconstruction. This is a source-backed scheduling lead, not a reason to discard
slow timing rows. Investigate it before adding more timing-only reruns, while
retaining production confirmation/history/window and match-end correctness.

## Stable reconstruction cadence, 2026-10-13

After catch-up, the host gives the normal tick its turn before another
BeginFrame packet batch. Normal input sampling and post-tick reconciliation
remain active; an exhausted prediction window still receives/waits/reconciles.
This is a scheduling change, not a resimulation cap or removal of confirmation.
Boundary fixture and full-game correctness scope are detailed in
`native_cpu_only_resimulation.md`.

The same final binary completes two stock-zero/stock-delay/interpreted-delay
cohorts. Each has 480 interpreted exactly-fifteen cycles, with one restore and
one capture per row; the ordinary population and provenance gates pass.

| Fifteen reconstructed ticks + normal rendered tick | Run A | Run B |
| --- | ---: | ---: |
| Interpreted cycle population | 480 | 480 |
| Active wall median | 12.658 ms | 12.350 ms |
| Active wall p90 | 13.732 ms | 13.252 ms |
| Active wall p99 | 15.265 ms | 14.391 ms |
| Active wall measured maximum | 16.470 ms | 18.256 ms |
| Simulation median | 6.797 ms | 6.733 ms |
| Drawing median | 4.578 ms | 4.470 ms |
| Checkpoint median, included in netplay | 0.544 ms | 0.498 ms |

Stock's 480-cycle populations measure 10.263/10.635-ms medians and
13.499/14.048-ms maxima. The previous source's three exactly-fifteen cycles
were too few to isolate a total-cost gain. The scheduling correction produces
a reproducible exact-depth population in these two runs, while the remaining
simulation/render costs dominate. Do not add independent phase medians or
discard the slower peak. Both runs miss **10 ms median / <=12 ms measured
peak**. Keep optimizing; no sustained deadline acceptance is claimed.

These are two hidden, muted clients sharing one Windows CPU/GPU, Green Greens,
medium items and eight interpreted Ice Climbers actors per client, with an
artificial fourteen-frame outbound delay. Active wall subtracts explicit sleeps
per row; OS preemption and worker/GPU waits remain. Timing includes normal
render submission, not a GPU fence or display scanout. This is not all-fighter/
stage or distributed-PC acceptance. Exact rows, hashes and independent checks
are retained in `frame-cadence-oct13-live-{a,b}-verification.json`.

## ThinLTO cohort, 2026-10-13

Clang now optimizes across the game/HSD translation units in Release and
RelWithDebInfo. Strict floating-point options, interpreter checks and the
confirmed-checkpoint policy remain unchanged. The toolchain check, dead console
math cleanup, import evidence and additional raw endpoint census are described
in `native_cpu_only_resimulation.md`.

| Fifteen reconstructed ticks + normal rendered tick | Run A | Run B |
| --- | ---: | ---: |
| Interpreted exact-depth population | 480 | 480 |
| Active wall median | 12.119 ms | 12.111 ms |
| Active wall p90 | 13.080 ms | 12.965 ms |
| Active wall p99 | 14.801 ms | 14.690 ms |
| Active wall measured maximum | 16.212 ms | 15.137 ms |

Both ordinary minimum-32 verifiers pass provenance, counter/phase accounting,
default CPU-only output counts, peer checksums and the unchanged 600-sample
endpoint/RNG. Production peak snapshot sizes remain 2,570,103 stock and
2,940,591 interpreted bytes. Stock medians are 10.156/10.147 ms and maxima
13.367/14.403 ms. The preceding non-IPO interpreted medians were 12.658/12.350
ms, with independent packet/scheduling populations; these are not paired
samples isolating every source of timing variance. The executable-size change
is directly measured: 206,746,624 bytes before, 18,209,280 after.

The **10-ms median / <=12-ms measured-peak** target remains unmet. These two
hidden/muted local clients share one CPU/GPU, with the same Green Greens,
medium-item, eight-interpreted-Ice-Climbers and fourteen-frame outbound-delay
workload. Active wall excludes explicit sleeps and includes OS/worker/GPU waits;
render submission is not a GPU completion fence. The separate fifteen-tick
Final Destination raw/component census is correctness evidence, not a timing
sample or all-scene/fighter acceptance.

## Specular-cache experiment rejected, 2026-10-13

Coalescing specular light directions during hidden reconstruction passed a
fifteen-tick full-state census after correcting the list-replacement boundaries,
but did not establish a meaningful complete-cycle gain. The added runtime cache
and its deferred path were removed; the evidence remains retained.

| Experimental fifteen-tick full cycle | Run A | Run B |
| --- | ---: | ---: |
| Interpreted exact-depth population | 480 | 480 |
| Active wall median | 12.027 ms | 12.117 ms |
| Active wall p99 | 14.542 ms | 14.400 ms |
| Active wall measured maximum | 17.229 ms | 15.927 ms |
| Simulation median | 6.390 ms | 6.358 ms |
| HSD draw median | 2.607 ms | 2.557 ms |
| Render end median | 1.621 ms | 1.636 ms |

The full drawing-phase medians are 4.344 / 4.351 ms; they include hidden and
normal output work. Component medians are not additive identities. Both usual
minimum-32 verifiers pass source/log pins, phase/counter accounting, production
checkpoint operation, confirmed peer checksums and unchanged endpoint/RNG.
Neither cohort meets the **10-ms median / <=12-ms measured-peak** target. These
are historical experimental timings, not a speed claim for the restored source.
The next optimization should address larger simulation or display/submission
costs rather than accumulating an unproven cache. All prior one-machine,
one-stage and render-submission measurement limits still apply.

## Consolidated script fighter admission, 2026-10-13

The private fighter dispatcher no longer repeats caller admission or repeats
its post-execution status walk without an intervening callback. It retains
generation/retirement/fault checks around actual reentry and publishes outputs
only after those checks pass. No cache or saved-state size is added. See
`native_cpu_only_resimulation.md` for fixture and fifteen-tick raw census scope.

| Unprofiled fifteen-tick full cycle | Run A | Run B |
| --- | ---: | ---: |
| Interpreted exact-depth population | 480 | 457 |
| Active wall median | 11.989 ms | 11.990 ms |
| Active wall p90 | 12.805 ms | 13.170 ms |
| Active wall p99 | 13.911 ms | 20.255 ms |
| Active wall measured maximum | 15.224 ms | 26.697 ms |
| Simulation median | 6.351 ms | 6.381 ms |
| Drawing phase median | 4.350 ms | 4.317 ms |
| Checkpoint median | 0.587 ms | 0.590 ms |

Both ordinary minimum-32 verifiers pass source/log pins, phases, counters,
CPU-only counts, peer checksums and unchanged endpoint/RNG. Run B's high tail
is retained, not trimmed as presumed OS interference: the cause has not been
isolated. These workloads still share one Windows CPU/GPU between two clients.
The **10-ms median / <=12-ms measured-peak** target remains open. Do not infer
a controlled 0.1-ms gain from unpaired populations or substitute the separate,
perturbed call-stack sampling runs for this measurement.
