# Match-history save profiling (2026-10-12)

Walking the root directory is cheap. Data copying dominates, and copying the
same bytes in chunks is substantially slower than one contiguous copy in these
controls. This profiles the experimental copy-only history, not production
netplay's page tracker.

## Actual replay saves

Eight interpreted Ice Climbers actors on Green Greens, 600 input samples,
two 15-tick replay intervals starting at input boundaries 240 and 360. Both
intervals render normally. Profiling surrounds only history Save. The main
phase pass uses a handful of clocks per arena, rather than clocks per root.

| Save phase | Interval 1 (us/save) | Interval 2 (us/save) |
| --- | ---: | ---: |
| History guards and slot selection | 0.113 | 0.107 |
| HSD arena guards/current cursor | 0.187 | 0.120 |
| HSD used-prefix memcpy (~1.51 MB) | 135.247 | 140.447 |
| HSD root loop (62 spans, ~0.40 MB) | 41.873 | 40.940 |
| HSD publication | 0.060 | 0.047 |
| Scene arena guards/current cursor | 0.393 | 0.447 |
| Scene used-prefix memcpy (78,616 bytes) | 9.327 | 8.193 |
| Scene root loop (329 spans, ~0.93-0.97 MB) | 102.400 | 106.200 |
| Scene publication | 0.027 | 0.107 |
| History publication | 0.040 | 0.033 |
| Arena unassigned/profiler intervals, combined | 0.134 | 0.120 |
| History dispatch/unassigned/profiler | 0.453 | 0.540 |
| **Whole history Save** | **290.253** | **297.300** |

The caller adds another 0.160/0.133 us. Each save calls memcpy 393 times:
two arena prefixes and 391 roots. Of the roots, 368 are nonempty and 23 are
zero-length. Root-loop time includes reading counts, computing lengths,
storing frame lengths, pointer advancement and memcpy. It is not copy-only
time; separately printed preparation=0 in coarse logs means unmeasured.

There is no allocation, hashing, compression, object-graph validation,
allocator reconstruction or page-fault tracking in this measured save path.
Owner admission and capacity/alias checks happen during construction. Save
retains constant private lifetime/order/cursor checks.

## Controls that separate walking from copying

After the replay clock stops, a controlled experiment gathers the same live
sources into preallocated scratch. All variants write the same bytes into
the same compact 16-slot destination ring. Five batches of 64 copies each;
reported medians use repeated warm sources. Every destination is checked
against the gathered bytes outside timing.

| Experiment | Interval 1 (us) | Interval 2 (us) |
| --- | ---: | ---: |
| Live sources, existing 393 segments | 236.686 | 225.784 |
| Packed source, same 393 segments | 242.394 | 231.584 |
| Packed source, one memcpy | 123.927 | 115.662 |
| Walk lengths and accumulate offsets, no data copy | 0.213 | 0.212 |

Copied sizes are 2,918,547 and 2,965,883 bytes. The walk-only experiment does
not store frame lengths and is not a substitute for the live phase timings.
Packing the source alone did not help appreciably; retaining the chunk
pattern was roughly twice as slow as a single copy. Gathering into a staging
buffer each tick would add another copy, so this is not evidence to add one.

A separate standalone CRT memcpy control used a contiguous 2,965,883-byte
source and 16 compact destinations, with external memcpy calls retained:

| Equal-sized chunks | Median us/copy |
| --- | ---: |
| 1 | 123.436 |
| 2 | 258.214 |
| 4 | 245.181 |
| 393 | 287.832 |
| 393 calls, each copying zero bytes | 0.756 |

The 16/64-chunk runs were noisier (407.418/313.391 us). Call dispatch and
walking alone cannot explain the roughly 100-us difference. Splitting even
two chunks changed copy performance substantially. Size-dependent CRT/cache
or write-traffic behavior is a plausible explanation, **not yet proven** by
assembly inspection or hardware counters. Reducing copied bytes and arranging
mutable owners into fewer contiguous spans remain the useful next targets.
Do not claim that reducing 393 calls alone will save 100 us.

## Profiler overhead and validation

The optional detailed pass clocks preparation/copy for every root and emits
each root's address, index, byte count, nonempty count and average timings.
The empty clock interval measured approximately 26-27 ns. Hundreds of clock
pairs and statistic updates perturb the copy loop; detailed totals ranged
312-387 us in these runs. They are attribution hints, not the normal budget.
Inactive controls showed scheduling/cache variability, including one 379-us
outlier; their other interpreted intervals were approximately 298-316 us.
The coarse table is therefore a measured range, not a precise universal cost.

The detailed pass identifies several large roots rather than hundreds of
uniform tiny copies. Largest measured interpreted entries (two intervals):

| Arena / root index | Bytes | Copy us (intrusive clocks) |
| --- | ---: | ---: |
| HSD / 61 | 178,704 | 17.167-18.540 |
| Scene / 17 | 153,088 | 13.233-15.733 |
| Scene / 25 | 121,904-147,568 | 10.927-14.847 |
| HSD / 29 | 86,856-87,120 | 8.953-9.020 |

Addresses and every other root's measurements remain in the detailed log.

Stock and interpreted eight-actor cases passed both replay intervals in
inactive-before, coarse, detailed and inactive-after runs. All 39 component
endpoints matched at each interval, broad raw endpoint differing bytes were
zero, and audio rollback reconciliation passed. Input/source/executable/log
hashes were independently verified before rebuilding. This establishes the
tested intervals, not complete snapshot closure for every match.

After restoring the profiling-OFF release build, the same stock/interpreted
two-interval cohort passed again, independently verified in
`save-profile-oct12-normal-verification.json`. Six focused CTests passed:
history, arena, heap, match pool, GObj pool consumer and animation owner.
Normal executable SHA256:
`12461b1ddd9eab559053573190066372597a80e948f1fc398ba810bcb1e6c396`.

Artifacts are under `A:/FreakMelee-test-archives/`:

- `save-profile-oct12-final-control-a/`, `save-profile-oct12-coarse/`,
  `save-profile-oct12-detail/`, `save-profile-oct12-final-control-b/`.
- `save-profile-oct12-final-control-a-verification.json` (independent checks).
- `save-profile-oct12-binary/` (instrumented executable/PDB), executable SHA256
  `786e6c0e19da3f19f1a9e2d475b6e89d3b912c5e7bd08156f1f49fb44593f5d8`.
- `memcpy-chunks-oct12.txt` (standalone control), benchmark source/command in
  `.local/memcpy-chunks-oct12.*`.

## Reproduce

Configure CMake with `-DMELEE_PROFILE_MATCH_SAVE=ON` and rebuild `melee_native`.
The option defaults OFF and removes the profiler code from the normal build.
Run the census runner with an accepted Ice Climbers family receipt:

```powershell
python tests/native/native_snapshot_census/run.py `
  --family-receipt A:/FreakMelee-test-archives/source-family-cpu-oct11-final/receipt.json `
  --output A:/FreakMelee-test-archives/FRESH-PROFILE-DIRECTORY `
  --samples 600 --stage green-greens --resimulate --depth 15 `
  --history --before-input --target 240 --rounds 2 `
  --replay-timing --quiet-replay-timing --profile-save
```

Add `--profile-save-detail` only for intrusive per-root attribution. Remove
both profile switches for an inactive control. Tests launch hidden and muted.
Afterward configure `-DMELEE_PROFILE_MATCH_SAVE=OFF` and rebuild.
