# Complete compact replay timing

The census runner's `--replay-timing` records whole rendered replay intervals,
their actual compact history captures and the restore immediately before replay.
It requires pre-input history and resimulation. Existing correctness checks stay
enabled: immutable own captures, all 39 components, intermediate/continued
traces, complete broad endpoint comparisons and audio checks.

The match loop times input polling, source simulation and drawing. Drawing is
also subdivided into Aurora begin, HSD source drawing and Aurora end. Subdivision
times are contained within the complete drawing time; do not add them twice.
Each accepted fifteen-tick record must include exactly fifteen calls for every
phase and fifteen actual history saves. Correction resets its timing counters
independently for the first and repeated corrected pass.

The whole interval ends before endpoint capture/comparison and broad recovery.
Reported total adds the final null-report compact restore to that elapsed replay
time. Earlier diagnostic loads, file seek and observer checkpoint reset are
outside the interval. History timing includes direct per-boundary serialization,
validation and ring publication, rather than extrapolating warm same-boundary
microbenchmarks. Optional instrumentation makes no allocation during the interval.

## What these results establish

These are instrumented offline replay measurements on this PC. Actual HSD
drawing, render RNG, Aurora recording/processing and audio-intent handling remain
enabled; presentation is suppressed during replay. Fighter/action tracing and
object census are retained. Input includes their replay-observer work, and total
elapsed includes other diagnostic/loop housekeeping. Graphics setup, cache state
and background CPU work are not rigorously controlled. These are not a clean
production deadline benchmark, a network benchmark or proof of portable speed.

Six fifteen-frame intervals at 78, 198, 318, 438, 558 and 678 produced these
median phase totals (milliseconds) in the final `compact-budget-oct12-six-b`
cohort; values summarize actual phase records, not per-frame estimates:

| Phase | Stock eight | Interpreted eight |
|---|---:|---:|
| Compact restore | 0.939 | 1.009 |
| Fifteen history captures | 14.967 | 18.277 |
| Input / replay observers | 2.526 | 2.500 |
| Source simulation | 4.051 | 6.145 |
| Complete drawing | 14.035 | 14.137 |
| Renderer begin (within drawing) | 0.151 | 0.162 |
| HSD work (within drawing) | 5.433 | 5.306 |
| Renderer end (within drawing) | 8.417 | 8.766 |
| Whole interval + restore | 37.307 | 42.892 |

Whole measured totals ranged 35.263-41.557 ms for stock and 41.267-44.955 ms for
interpreted fighters. The measured 16.67 ms target is not met. Retained diagnostics
prevent claiming these are the clean production costs, but history and drawing
clearly deserve attention. Warm repeated-capture microbenchmarks understate the
actual per-boundary capture costs observed here.

Aurora source inspection shows `end_frame(false)` still drains the GX FIFO,
ends texture/FIFO processing, finishes recorded graphics and enqueues frame end.
Thread-mode FIFO drain waits for the worker's processed watermark. The measured
end phase combines these operations; it does not yet attribute the delay to a
specific wait, decoder function or GPU task. No renderer behavior was changed
and no drawing-disabled path was introduced.

## Evidence

Under `A:/FreakMelee-test-archives/`:

- `compact-budget-oct12-six-b`: stock/interpreted eight-fighter cases, 1,200
  controller inputs each, six fifteen-tick intervals, all timing counts complete,
  234 compact owner comparisons and six complete raw endpoints exact per case.
- `compact-budget-oct12-correction`: two separately timed fifteen-tick corrected
  passes per stock/interpreted case; fresh reference, component/raw equality,
  original continuation and audio deduplication all pass.
- `compact-budget-oct12-gm-b`: connected ordinary match-loop/controller/player/
  camera fixture passes 2,018 loop and 31,797 camera checks. Timing is disabled;
  its timing leaves abort if invoked, proving the opt-in branch remains inactive.
- Earlier six-a/GM-a receipts retain the initial coarse-phase checkpoint.
- Focused history, callback and GObj consumer CTests pass. Final pins are checked
  in `compact-budget-oct12-independent-verification.json`.

Remaining work includes a production budget cohort, further capture cost
reduction, renderer end attribution/optimization while preserving source drawing,
candidate/resource closure and production compact rollback integration.

## Reduced-observer timing

`--replay-timing --quiet-replay-timing` omits fighter/action traces and the
per-frame object census. Input-only playback now computes a state digest only
when a state, action or fighter observer is enabled. Verified replay and recorded
state digests remain unchanged. The timing log derives its instrumentation label
from the actual observer and object-census settings; the runner requires
`endpoint_only` for this mode and `retained` for the ordinary traced mode.

The quiet cohort runs only the census scenario. It retains real source drawing,
render RNG, the history directory/invalidation checks, actor/class graph checks,
same-boundary restores, all 39 replay owner comparisons, full raw endpoint
comparisons and audio checks. It explicitly does **not** provide intermediate
per-frame trace comparison or an independent forward reference. Receipts mark
`trace_comparison_performed=false` and `differences=null`, rather than presenting
absent traces as a successful comparison. Correction and warm microbenchmarks
are refused in this timing mode; ordinary traced runs remain the stronger
correctness cohort.

Six fifteen-tick interval medians in `compact-budget-oct12-quiet-a`, milliseconds:

| Phase | Stock eight | Interpreted eight |
|---|---:|---:|
| Compact restore | 0.942 | 0.999 |
| Fifteen history captures | 15.490 | 18.473 |
| Input | 0.849 | 0.796 |
| Source simulation | 4.135 | 6.290 |
| Complete drawing | 13.872 | 14.095 |
| HSD work (within drawing) | 5.375 | 5.512 |
| Renderer end (within drawing) | 8.386 | 8.694 |
| Whole interval + restore | 34.996 | 41.015 |

Whole intervals range 34.461-36.830 ms stock and 38.575-43.525 ms interpreted.
Peak history records are 2,655,680 and 2,846,512 bytes respectively. The
diagnostic ring still reserves sixteen 8-MiB slots; these record sizes are not
claims about total process memory or final production history sizing.

Quiet-a passes 1,200 inputs, 234 replay owner comparisons and six raw endpoints
per case. `compact-budget-oct12-observer-regression` repeats the full traced
six-interval stock/interpreted cohort against the same executable and sources.
`compact-budget-oct12-observer-correction` checks fresh and repeated corrected
inputs with traces, raw endpoints, original continuation and audio deduplication.

Removing observers lowers input cost but does not meet the 16.67-ms target.
History capture and retained renderer processing still dominate this offline
measurement. This is reduced-observer evidence, not a diagnostic-free production
or network benchmark. Compact production integration and state/resource closure
remain open.
