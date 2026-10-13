# CPU-only GX reconstruction

The confirmed-checkpoint path already avoids intermediate production saves.
Netplay now reconstructs ticks without GPU output by default. Ordinary frames
still render normally. `FREAK_MELEE_NO_GPU_RESIM=0` explicitly restores the
rendered diagnostic path; `=1` also enables GPU-free reconstruction in offline
snapshot experiments. Raw-memory rollback probes retain their rendered oracle
unless opted in. `FREAK_MELEE_SKIP_MODEL_RESIM=1` remains a narrower diagnostic
that omits model primitives while retaining GPU frame processing.

Aurora implementation checkpoint: local commit `883ddb3a386efa04615d0f87eace1741ea5375fc`.

The match loop retains the original HSD callbacks, their order, RNG calls,
camera/projection updates and particle attachment updates. On reconstructed
ticks, `PObjSetupMtx` prepares only the pose dependencies: shared skin joints,
the envelope joints consumed by the original ten-entry palette, and the same
matrix marks and single-weight shortcut. It does not blend skin matrices,
compute inverse-transpose normals or upload model matrices to GX.
`HSD_PObjDisp` then omits primitive emission. Vertex-description caches
are cleared at the end of each frame while this policy is enabled; these cache
pointers describe submitted geometry and must not depend on a preceding draw.
The host policy and diagnostic counters are outside mutable match state.

Aurora's `aurora_begin_frame_without_output` / `aurora_end_frame_without_output`
pair runs GX decoding without `gfx::begin_frame`, mapped upload buffers, pipeline
creation, GPU passes, `gfx::finish`, `gfx::end_frame` or queue submission. BP,
CP, XF, texture identity, viewport and scissor changes still decode. Standard,
auto-sized and indexed draw payloads are consumed without buffer uploads.
Draw-done callbacks still execute. GPU-only offscreen passes, EFB copies, debug
markers and depth snapshot requests are omitted. EFB textures and readbacks
retain their last rendered contents; Melee's current C sources do not call
`GXPeekZ`. A future gameplay dependency on GPU readback would require explicit
handling before using this policy.

Visible frames publish to the existing FIFO worker; CPU-only frames execute the
same decoder on the calling lane. Ownership changes only after drain, under the
buffer lock. The worker rechecks ownership and the processed watermark under
that lock, so an old publication cannot consume a retired or already decoded
range. Draw-done callback completion precedes the watermark. The next GPU frame
restores the logical viewport/scissor and frame-local draw bindings. No second
simulation or rendering worker was added; the unused drain-only mode was removed.

## Local evidence, 2026-10-12

Archives are under `A:/FreakMelee-test-archives/`:

- `render-state-oct12-traced-c`: model omission alone, stock and interpreted
  eight-actor Green Greens/items, three 15-tick intervals each. All 39 owners,
  per-frame traces and broad raw endpoints match with zero differences.
- `render-cpu-oct12-traced-a`: CPU-only GX, same eight-actor cases and intervals.
  Each case exercises 45 CPU frames (28,974 stock / 28,999 interpreted omitted
  model primitives); all
  117 owner comparisons and per-frame traces match.
- `render-cpu-oct12-correction-a`: stock/interpreted corrected human inputs on
  Final Destination, fresh reference plus repeated 15-tick reconstruction,
  original continuation and audio reconciliation. Each exercises 30 CPU frames;
  all 39 endpoint owners and compared traces match.
- `render-cpu-oct12-raw-verification.json`: source, executable, receipt and log
  hashes independently verified. CPU-only broad endpoints differ by 104-121
  bytes in particle-display color/matrix/billboard/current-matrix scratch.
  Every differing byte is mapped to actual private PDB or emitted COFF symbols;
  no other raw differences occur in these intervals. `psDispParticles` refreshes
  these values before using them. This is intentionally not a claim that the
  entire raw memory image is equal.
- `render-cpu-oct12-live-a-verification.json`: actual local UDP clients using
  confirmed checkpoints, zero/delayed stock and delayed interpreted eight-actor
  Green Greens/items. Stock endpoint `0c26c8db91f3f243`, RNG `e685d7b0`;
  interpreted endpoint `6701a8ecfe2a64a2`, RNG `9e464987`. Both delayed stock
  peers match 2,136 selected settled actor/item/RNG records against the separate
  zero-delay run. Hidden reconstruction is exercised, without page tracking.
- `render-cpu-oct12-fifo-tests-b.log`: all 206 GX/FIFO tests pass, including
  zero vertex/index uploads, retained register state/callbacks, skipped GPU copy
  work and resumed output without repeating the format/blend setters.
- `render-cpu-oct12-gm-unit-a`: 2,019 production-loop checks and 31,797 camera
  checks pass with controlled external render endpoints.
- `render-cpu-oct12-scripted-end-a` and `render-cpu-oct12-end-verification.json`:
  interpreted eight-actor Final Destination match end at 3838 retains endpoint
  `7dacb886e4392d6c` / RNG `a4a156d6`. Both peers exercise CPU-only reconstruction,
  bind the second checkpoint, and advance beyond 160 sudden-death ticks. Their
  settled frame-120 digest/RNG is `a560f0ab07fef73b` / `a13a037c`. Full sudden-death
  completion remains untested.

The six replay/checkpoint/input/session/UDP CTests also pass on this release.

These are bounded local cases. Broader fighters/stages, complete no-render
callback separation and a sustained 15-tick/frame
deadline remain work. See `native_compact_snapshot_timing.md` for measured
costs; the diagnostic history still performs fifteen saves per replay interval
and is not the production checkpoint policy.

## Default netplay cutover and decoder ownership

The final-source default-policy batch `render-inline-oct12-live-final-a` runs
without either render-policy environment variable. Every hidden netplay tick
uses CPU-only GX (429/433 delayed stock ticks and 436/450 delayed interpreted
ticks); endpoint digests/RNG retain the values above. Independently verified
settled stock traces match the zero-delay reference. See its
`default-policy-verification.json` and the adjacent live verification report.

Additional bounded intervals cover Fountain of Dreams and late Brinstar frames
1200/1320/1440 with eight stock or interpreted actors and medium items. The final
Brinstar source/binary/log pins and all 117 owner comparisons per case are
verified in `render-inline-oct12-final-raw-verification.json`. Raw differences
are confined to particle-display scratch, including the scalar `prevTev.0`
emitted from `psdisptev.c`. That cache resets through `psSetupTevInvalidState`
before each display pass; no other raw differences occur in these intervals.
This does not prove every stage interaction or every fighter's special move.

All 207 GX tests pass, including 512 alternating worker/caller ownership changes
with callbacks and unpublished tails. The six focused native tests pass on the
final release. A temporary end profiler (now removed) showed approximately
247 KB of pending GX commands and 152-155 us decoding per CPU frame, versus
100-200 ns clearing draw caches. The CPU-only path still generates graphics
registers; synchronous decoding is not a large performance breakthrough.
Further work should separate state updates from visual preparation inside HSD,
while retaining the required pose, particle and gameplay updates.

## Pose dependencies without model matrix uploads

`PObjPreparePose` retains the shared/envelope joint evaluation order and matrix
marks during reconstruction, without skin blending, normal matrices or GX
model matrix uploads. Ordinary rendered frames are unchanged. Material,
texture and light preparation still runs: those paths also change resource
assignments, texture matrices, descriptors and light state, and require their
own separation rather than an unconditional early return.

The stock/interpreted eight-actor cohorts `render-pose-oct12-green-a` and
`render-pose-oct12-brinstar-a` each pass three 15-tick intervals, including late
Brinstar boundaries 1200/1320/1440. All 117 owner comparisons and per-frame
traces match per case. `render-pose-oct12-correction-a` passes fresh and repeated
corrected-input replay, continuation and audio reconciliation. Its 39 compared
owners match per case. Independent source/binary/log checks in
`render-pose-oct12-raw-verification.json` classify every differing raw byte as
the same previously identified particle-display scratch; no other raw
differences occur in these bounded cases.

`render-pose-oct12-live-a` passes actual local UDP zero/delayed stock and delayed
interpreted matches. Both delayed stock peers match 2,136 settled records from
the independent zero-delay run. Every reconstructed tick uses CPU-only GX with
the default policy, and the stock/interpreted endpoint digest and RNG retain
the values above. Six focused replay/checkpoint/input/session/UDP tests pass.
This remains local evidence, not distributed-PC or all-fighter acceptance.

## Material graphics commands separated from retained updates

GPU-free callbacks now use a separate host graphics policy. HSD still updates
texture resource assignments and identities, dirty texture matrices, borrowed
shadow/toon links, shader descriptor map/coordinate fields, light state and
shader cache invalidation. It omits TEV stage command encoding, constant color
evaluation used only by GX, texture coordinate generation, and texture matrix
projection/uploads. Highlight light attachments and shadow inverse-camera
accessors still run because they can finish dirty CPU state. The narrower
model-only experiment retains material graphics commands, and visible frames
use the original graphics path.

Current evidence is under `C:/FreakMelee-test-archives/` rather than the nearly
full A: drive. `render-material-oct12-green-a`,
`render-material-oct12-brinstar-a`, and `render-material-oct12-correction-a`
pass their stock/interpreted owner and frame-trace comparisons. Source, binary,
log and all raw differences are checked independently in
`render-material-oct12-raw-verification.json`. `render-material-oct12-live-a`
passes the default-policy local UDP cohort; stock endpoints and both delayed
peers' 2,136 settled records match the zero-delay reference, while interpreted
peer endpoints retain the earlier values. The six focused native tests pass.

Fountain's three intervals also pass all 117 owner comparisons and frame traces
per case. Its first interpreted older-boundary comparison has 73 additional
raw bytes outside the restored live prefixes: 34 bytes in three future scale
vectors and 39 bytes in their three allocation-directory rows. This is separate
from particle-render scratch. A debugger watchpoint identifies the storage as
`HSD_VecAlloc` backing, cleared by `nativeMatchArena_Allocate` before reuse.
At that restored boundary the HSD cursor is 1,506,336 and directory count 3,629;
the extra rows lie beyond that count. Counted-root save/load restores the live
prefix and count; append allocation overwrites these inactive rows. The heap
vector ranges also match the earlier CPU-only Fountain log byte-for-byte.
`render-material-oct12-fountain-raw-verification.json` verifies actual PDB
symbols, the recorded cursor/directory boundary, all three row fields and every
raw byte. It grants no blanket exemption for arena or directory tails.

## Particle geometry separated from effect state

Particle geometry is now omitted during GPU-free reconstruction. Display
orchestration remains in `psdisp.c`; actual geometry is in
`psdisp_geometry.c`, and the shared attachment-matrix update is in
`psdisp_pose.c`. Visible frames use those same geometry algorithms. Point and
trail batching still consume the same particle-chain entries, and the four
untextured shader modes still normalize the particle's PrimEnv flag. Sorting,
attachment matrices, generator callbacks, particle commands, motion, lifetimes
and RNG retain their original ordering.

This distinction is necessary for effects such as landing smoke. An effect
created during reconstruction must exist at the corrected visible boundary,
with its lifetime advanced through the replayed ticks. Conversely, a particle
created by a discarded prediction must disappear. Production history restores
the particle/generator list roots and counters, their HSD allocation payloads,
attachments, and allocator descriptors/free lists. Reconstruction then runs
the corrected effect creation and updates. There is no separate append-only
visual particle stream that can retain a discarded spawn. Already presented
images cannot be undone, but subsequent frames draw the corrected effect state.

Frame-local drawing matrices, colors and shader caches now live outside the
snapshot section and are initialized by each nonempty display pass. The
particle sort caches and attachment data remain owned snapshot state. Camera
accessors, texture identities and remaining graphics setup still run; this is
not yet a simulation loop with all drawing preparation removed.

`C:/FreakMelee-test-archives/render-particle-oct12-green-a` and
`render-particle-oct12-brinstar-a` pass three 15-tick intervals for stock and
interpreted eight-actor matches with medium items. Each case matches 117 owner
comparisons and the independent rendered frame traces. The older-boundary raw
comparisons now have zero differing bytes in these cases. Green Greens omits
3,810/2,923 particle batches across 45 reconstructed ticks; late Brinstar omits
1,115/1,834. `render-particle-oct12-correction-a` passes fresh and repeated
corrected-input reconstruction, continuation and audio reconciliation, with
39 matching owners per case and zero older-boundary raw differences. Source,
binary, log, omission counters and raw results are independently checked in
`render-particle-oct12-raw-verification.json`. These are aggregate effect-state
checks, not a dedicated visual observation of frame-perfect wavedash smoke.

`render-particle-oct12-live-a` passes actual local UDP zero/delayed stock and
delayed interpreted matches. Both delayed stock peers match 2,136 settled
records from the independent zero-delay run. Endpoint digest/RNG retain the
previous values. All reconstructed ticks use the default CPU-only policy;
delayed stock peers replay 444/456 ticks and interpreted peers 437/448.
`render-particle-oct12-live-a-verification.json` checks hashes, peer endpoints,
settled stock records and omission counters. Six focused replay, checkpoint,
input, session and UDP tests pass. Distributed-PC, every-stage/effect and
sustained performance acceptance remain separate work.

## Pixel, channel, light and particle setup command omission

GPU-free reconstruction now omits HSD pixel/channel/TEV register commands and
light register uploads while preserving their cache writes, counters, dirty
flag transitions and light-object payloads. Fog submission only reads CPU
objects, so its GPU-only calculations are omitted. Screen erase retains the
optional depth texture's resource identity and the original HSD cache
invalidation, while omitting the erase geometry and register setup. The match
camera calls screen erase before its normal passes; this invalidates HSD
submission caches before the next visible frame. Light setup still initializes
its objects and marks them dirty for that visible pass.

Particle display retains the camera's viewing-matrix accessor and shared
attachment updates, sorting/batch traversal and shader-kind normalization.
Inverse billboard/projection matrices, alpha-test calculations, vertex setup,
texture-coordinate matrices and form lookup are skipped with geometry. Texture
and palette initialization/loading remains, including native resource identity
updates. All visible-frame branches retain their normal commands. Direct game
draw callbacks, camera/pose updates and resource metadata still run, so this
does not finish the complete simulation/drawing separation.

The eight-actor stock/interpreted cohorts `render-state-oct12-green-a` and
`render-state-oct12-brinstar-a` pass 117 owner comparisons and frame traces per
case across three 15-tick intervals, with zero older-boundary raw differences.
`render-state-oct12-correction-a` matches fresh/repeated corrected inputs,
continuation and audio, with 39 owners per case and zero raw differences.
Independent hashes, raw comparisons and omission counters are verified in
`C:/FreakMelee-test-archives/render-state-oct12-raw-verification.json`.

`render-state-oct12-live-a` passes local UDP zero/delayed stock and delayed
interpreted matches. Both delayed stock peers match the 2,136 selected settled
records from the independent zero-delay run; all endpoint digest/RNG values
retain the earlier results. Every reconstructed tick uses default CPU-only GX
(435/443 delayed stock, 437/449 interpreted). The zero-artificial-delay clients
also perform 126/132 reconstruction ticks due to ordinary arrival/scheduling;
zero configured delay does not imply zero rollback. The verification report
checks those counts against the checkpoint statistics. Six focused native tests
pass. This remains bounded local state evidence, without distributed-PC or
dedicated pixel-comparison acceptance.

## Full production-cycle profiling and replay input pacing

`FREAK_MELEE_PROFILE_NETPLAY=1` records fixed host-side timing rows between
successive visible-frame submissions. A row includes checkpoint work after
the preceding submission, all reconstructed ticks, and one normal rendered
tick. Initialization, the first boundary and the unfinished shutdown tail are
excluded. Records are retained in a fixed 1,024-row buffer and printed after
measurement. Input/network sleeps are measured separately; subtraction leaves
active wall time, including OS preemption and GPU/worker waits. Submission is
not a GPU fence or display-scanout measurement.

The full-window test exposed two transport limits that the previous short
delay cohorts did not reach. Input exchange still imposed six predicted ticks,
while confirmed checkpoints allowed fifteen. Prediction now uses the checkpoint
limit; diagnostic outbound delay is at most fourteen, leaving one input tick
of lead. Small fixed input, outbound and checksum histories retain 64 frames
to cover delay/redundancy and checksum round trips. Gameplay still has one
saved checkpoint and a fifteen-tick prediction window; this does not add
gameplay snapshots. Packet batches remain eight input frames.

The first completed full-window profile (`netplay-timing-oct12-live-c`) also
exposed resimulation waiting on the live 60-Hz input clock. After its first
sample, the overdue-sample logic reanchored to wall time, so the next historical
tick slept until a new hardware sampling deadline. Netplay reconstruction now
calls `lbInput_Poll` with the saved schedule's next sample time. Raw controller
queue/filter/channel processing still runs; normal play still samples wall
time. The input timing test covers fifteen scheduled samples followed by the
next live deadline, and the input history test checks extended confirmation
retention and actual ring expiration.

`C:/FreakMelee-test-archives/netplay-timing-oct12-live-d` passes stock zero/delay
and interpreted delay with two hidden, muted clients, eight Ice Climbers actors,
Green Greens and medium items. Each peer completes 600 samples and records 480
cycles after frame 120. Semantic peer checksums and stock endpoints agree with
the zero-delay reference; the interpreted endpoint retains `6701a8ecfe2a64a2`
and RNG `9e464987`. Every reconstructed tick uses CPU-only GX. Source, binary,
log hashes, row boundaries, phase accounting and counters are independently
checked in `netplay-timing-oct12-live-d-verification.json`. Seven focused input
timing, input history, replay, checkpoint, session and UDP tests pass.

New packets can trigger several reconstructions before the next submission.
The 121 interpreted cycles with exactly fifteen total hidden ticks each have
two restores and two captures; they are not single fifteen-tick reconstructions.
Stock's busy cycles predominantly contain 21/23/25 hidden ticks. The verification
report groups by actual total, rather than assuming every correction has the
configured prediction depth. This first full-cycle sample is close to the frame
budget but does not establish sustained deadline acceptance.

## Final-frame UI layout cost

Aurora's optional `AURORA_PROFILE_FRAME_END=1` now separates FIFO drain,
texture maintenance, recording completion, ImGui freezing and RmlUI
update/recording. It uses fixed caller-owned storage and reports after the
measurement, leaving the rendering worker and game snapshots untouched.

`frame-end-oct12-live-b` identified repeated layout work: the original CSS
overlay assigned its two visibility properties every frame, and the costume
panel repeatedly hid itself during matches. RmlUI marks assigned properties
dirty even when their values do not change; `display` forces document layout.
These call sites now apply visibility only when its displayed state changes.
The context still updates and renders every visible frame, so chat, input,
animations and actual layout changes remain live.

In the same two-client interpreted Ice Climbers/Green Greens/item setup,
`frame-end-oct12-live-c` completes both 600-sample replays, retaining the
previous endpoint/RNG and passing confirmed peer checksums. All 600 frame-end
rows per peer and 480 production-cycle rows per peer are independently verified
alongside source/binary/log pins. The two `*-frame-verification.json` reports
compare 960 post-warmup frame-end rows each. UI update median falls from
2.629 ms to 0.263 ms; overlay recording from 2.868 ms to 0.412 ms; frame-end
from 3.698 ms to 1.225 ms. There are 160 texture misses and 3,967,488 upload
bytes across each complete measured population, so this was not a zero-upload
test. The profiling binary and changed sources are retained in each cohort's
`*-binary` directory. Seven focused native input/checkpoint/network tests pass.

The hidden UI smoke captures exercise inactive/active CSS and the costume
display but did not successfully enter practice with the injected selection
sequence. They are not evidence for a complete CSS/practice/return transition.
Local UDP tests cover actual match rendering; distributed-PC and broader
UI interaction acceptance remain separate.

## Callback attribution and pure CPU properties

`FREAK_MELEE_PROFILE_CALLBACKS=1` records inclusive GObj process, display and
camera callbacks in a fixed caller-owned hash table outside snapshot state.
It starts at game frame 120, separates visible/reconstructed calls, records
relative code addresses and prints at match closure. Offline PDB symbolization
keeps symbol lookup out of the measured path. Camera totals include nested
display callbacks, so those two categories must not be summed. Extra clocks
perturb this diagnostic cohort; use the ordinary production phase profiler for
budget claims.

`callback-profile-oct12-live-a` and `callback-profile-oct12-stock-a` complete
interpreted and stock local UDP replays with retained endpoints/checksums.
The scripted CPU decision callback averages 20.18 us per hidden invocation,
versus 4.71 us for stock. Animation/map callbacks are much closer, and inclusive
main-camera callbacks average about 204 us in both. This points to authored CPU
binding/execution work as a useful next target, rather than assuming all script
overhead is negligible. Exact counters, symbolized rows and tested binaries are
retained in the archive. Per-call means are over independent populations, not
matched per-instruction comparisons.

CPU scalar reads/writes previously created and revalidated a full CPU
continuation snapshot, although their bodies invoke no engine/authored callback.
They now admit the live selected pool/program/storage source once and directly
access those fields. Exact types, numeric ranges and no-op rules are unchanged.
Engine kernels and target-ID publication retain their continuation/target checks.
The command/scalar/selector/VM fixture passes 7,269 controlled checks, 612 queue
pairs and 80 selector pairs (`cpu-scalar-oct12-command-fixture-b`); seven focused
input/checkpoint/network tests also pass. Hidden local UDP reconstruction still
retains endpoint `6701a8ecfe2a64a2`, RNG `9e464987` and confirmed checksums.

The broader connected CPU fixture currently fails to link against newer scene,
arena and metadata dependencies. Older generated-C CPU/follower CTest fixtures
also fail compilation/generation against the current source contracts. These
failed attempts are retained in the archive and are not included in the passing
fixture claim. The narrow command/scalar fixture now has an explicit selection
flag and refreshed hitbox-metadata linkage; unrelated stage reads abort there.
This is not full CPU/receiver/scene-boundary acceptance.

## Linear registry admission and item lookup, 2026-10-13

The CPU planner's pre/post registry admission walked the physical Fighter/Item
list, then called `nativeCallbackSource_Snapshot` on each visited node. That
helper searches the same list again, making the walk quadratic. The outer walk
already proves membership; it now validates pool epoch, class, non-null payload
and payload backlink directly. Selected peers still carry their full lifetime
guards across kernel callbacks. Generic fighter item selection, article links
and signal recipient lookup likewise read creation IDs directly from the nodes
they are already visiting; selected recipients retain the existing lifetime,
ownership, program and removal checks. No callback/order/RNG decision changes.

The command/scalar/selector fixture adds actual pre/post planner admission with
a controlled no-op leaf. It rejects null payloads, wrong classes, bad backlinks,
freed slots and cycles before kernel entry, plus payload retirement afterward,
without publishing output on failure. The final fixture passes 7,333 checks,
612 command pairs and 80 selector pairs. This is admission evidence, not native
planner behavior. Seven focused input/checkpoint/network tests pass.

`registry-linear-oct12-live-a` completes stock zero-delay, stock delayed and
interpreted delayed local UDP replays, 600 samples per peer. Source/binary/log
pins, 480 production timing rows per peer, default CPU-only reconstruction
counts, confirmed checksums and endpoint/RNG are independently verified. The
interpreted endpoint remains `6701a8ecfe2a64a2` / `9e464987`; the stock delayed
endpoint matches its zero-delay reference. The 472 interpreted exactly-fifteen
cycles each have one restore and one capture. This differs from earlier cohorts
with two reconstructions per submission, so the full-cycle change cannot be
attributed solely to registry optimization. Detailed timing and remaining target
are in `native_compact_snapshot_timing.md`.

An invocation-local metadata cache was also evaluated. It passed controlled
replacement/retirement cases but did not show a convincing full-path gain, so
the cache, context fields and VM changes were removed. Historical experimental
timings are not the final-source claim. The final change is the direct linear
walk, without a new cache, compatibility path or language-specific primitive.

## Audio sampling and decoded batches, 2026-10-13

The external main-thread instruction-pointer sample (`main-thread-oct13-live-a`)
shows voice decoding/resampling among the native leaves. This is diagnostic:
brief thread suspensions affect scheduling, and instruction-pointer counts are
not stack attribution or elapsed CPU time. Each sampler's final `SuspendThread`
call returns error 5 after more than 6,000 successful samples, near process
shutdown. Primary-thread exit is a plausible explanation, but the sampler did
not query thread exit status, so that cause is unconfirmed. All game and sampler
processes exit, and no thread remains suspended.

Audio output polls fill a wall-clock playback queue, rather than advancing one
audio frame for every reconstructed gameplay tick. We have not skipped voice
advancement, allocation, completion callbacks or audio mixing during rollback.
The resampler previously called `HSD_AudioVoiceRead` for each source sample,
copying decoder controls in and resulting state out on every sample. It now
decodes exactly the samples needed for bounded 32-output-sample blocks, using a
128-sample stack buffer. Larger ratios consume additional bounded batches;
there is no read-ahead past the requested output interval, heap allocation,
ownership-thread change or snapshot-format change.

The analytic polyphase checks still pass. Another 2,240 comparisons against the
previous sample-at-a-time algorithm verify PCM and the complete voice object,
including fractional phase, predictor/history, streaming/loop retargeting,
early completion, storage bounds, split blocks and extreme pitch ratios. All
17 selected audio/load/input/checkpoint/network tests pass. A standalone QPC
benchmark with 64 looping voices and alternating old/new execution order reports
2.030-2.049 us versus 1.180-1.188 us per 160-output-sample call (eight rounds),
about 1.7x faster. That bounded decoder benchmark is not full-match acceptance.

Both `audio-batch-oct13-live-a` and `audio-batch-oct13-live-b` complete stock
zero-delay, stock delayed and interpreted delayed 600-sample local UDP runs.
Confirmed peer checksums, production checkpoint counters, CPU-only frame counts,
source/executable/log pins and endpoint/RNG are independently verified. The
interpreted endpoint remains `6701a8ecfe2a64a2` / `9e464987`. Their interpreted
exactly-fifteen cycle populations are zero and eight, below the ordinary minimum
of 32: the normal performance verifier rejects both. The explicit zero-population
verification mode checks correctness only. Do not attribute full-cycle gains or
claim the user's 10/12-ms target from these runs.

## CPU source admission without duplicate continuations, 2026-10-13

CPU scalar fields previously called the complete source-layout validator, then
captured a separate `NativeFighterLifetime` that repeated pool, generation,
program, actor-view and storage admission. Scalar reads/writes invoke no native
or authored callbacks. They now use the existing complete source-layout proof
after their scene/pool/class checks, without capturing a continuation. The
common source-layout proof now explicitly requires the selected bank's exact
parameter object, retaining the relationship formerly checked by the duplicate
lifetime proof. No invocation cache, context ABI, or script API change is added.

CPU kernel admission similarly stores the identities established by that proof
without recapturing and immediately revalidating them. After a native kernel,
the original continuation gate still verifies source/context, pool/scene,
payload, fighter kind/data, selected package/program identity, source-layout
admission and stored view/schema/parameter identities before publication.
Retained target Fighter/Item lifetime guards and registry checks are unchanged.

`scalar-admission-oct13-fixture-d` passes 7,728 controlled checks, 612 command
pairs and 80 selector pairs. Added cases reject 27 invalid scalar/kernel entry
states without field mutation or output publication, including foreign parameter
objects, faulted actors and freed pool slots. Twelve controlled retirements
inside a native target-selection callback reject publication after changes to
source kind, data, payload, pool, backlink, storage, parameters or program identity.
A proposed rejection for a target changing creation ID before it was selected
was incorrect: it does not retire the caller. That expectation was removed;
the existing selector cases cover the target path. Broader connected fixture
linkage remains unresolved; this controlled cohort is not full planner parity.
Seven core input/checkpoint/network tests pass.

A standalone before/after QPC benchmark exercises actual property routing with
four synthetic physical Fighters and controlled package/program/actor views.
Across four alternating process-order pairs, each with eight timing windows of
200,000 reads, median read cost is approximately 72 ns before and 43 ns after.
It does not include the actual actor lookup or eight-fighter scheduler and must
not be extrapolated to a full-match speedup.

`scalar-admission-oct13-live-a` completes the same stock zero-delay/delayed and
interpreted delayed 600-sample local UDP batch with source/binary/log pins,
confirmed checksums, CPU-only counts and unchanged endpoint/RNG. It contains
only three interpreted exactly-fifteen cycles, so normal minimum-32 performance
verification rejects it; the explicit minimum-zero report verifies correctness
only. Source inspection shows that BeginFrame polls/reconciles again immediately
after a reconstruction catches up, permitting another reconstruction before
the next visible tick. The next scheduling investigation must preserve input
retention, confirmation, prediction bounds and match-end handling; no scheduling
change is part of this commit.

## Normal tick after reconstruction, 2026-10-13

The match host now marks a normal visible tick pending after a non-ending
reconstruction catches up. Its next BeginFrame uses the input history already
reconciled, instead of immediately ingesting another packet batch and starting
another reconstruction before a normal tick. The normal input filter still
receives packets before preparing that tick, and EndFrame still reconciles
corrections/confirmation afterward. New input is processed in the ordinary
sampling path; it is not discarded or held until some later rendered frame.

The fifteen-frame prediction bound remains authoritative. A pending normal tick
with an exhausted window must receive, wait and reconcile until it can advance;
this may require another reconstruction. There is no hard cap that ignores
corrections or silently advances past an unconfirmed hole. Match-end delayed-
input flushing/confirmation remains unchanged. Init and Shutdown clear the
host-only pending flag, including retained-transport scene transitions. The
snapshot byte counts do not change.

`frame-cadence-oct13-fixture-b` executes exact extracted production BeginFrame /
EndFrame bodies and host-state definitions with actual checkpoint/input/arena/
audio-journal implementations. Its 534 checks cover queued packets during
reconstruction, the normal sampling receive, propagated predictions, an exhausted
window with an earlier hole, match ending followed by correction/reconstruction,
send/receive failure, timeout and arena retirement. Transport/clock/gameplay
are controlled; diagnostic leaves abort. It is not full-game/sudden-death or
performance acceptance. Seven core input/checkpoint/network tests also pass.

Both `frame-cadence-oct13-live-a` and `frame-cadence-oct13-live-b` complete the
stock-zero/delayed and interpreted-delayed 600-sample full-game local UDP batch.
Each has 480 interpreted exactly-fifteen cycles; every such row records one
restore and one capture. Both ordinary minimum-32 verifiers pass source/binary/
log pins, timing/counter accounting, CPU-only counts, confirmed checksums and
unchanged stock/script endpoints and RNG. The predicate that permits a pending
normal tick is not a performance pass: both runs still miss the 10/12-ms target.
Full costs and limits are recorded in `native_compact_snapshot_timing.md`.

## Cross-module native optimization, 2026-10-13

The refreshed diagnostic callback profile (`render-attribution-oct13-live-a`)
still puts CPU decision callbacks first among process costs. Hidden camera
callbacks include their display children, so their totals must not be added.
Extra callback/frame-end clocks perturb these measurements; this run is
attribution evidence, not deadline acceptance. No camera/specular calculation
was removed based solely on that profile.

Release and RelWithDebInfo now enable Clang link-time optimization for
`melee_game`, `melee_runtime` and `melee_native`, using the toolchain's ThinLTO
configuration. CMake checks C/C++ IPO support first; Debug and external Aurora/
dependency targets retain their existing configuration. The strict floating-
point and float-cast options remain. This enables cross-file helper optimization
without a new invocation cache, interpreter API or relaxed lifetime checks.

The initial link exposed `lbtrigf.c`, which only contained console definitions
of standard math functions plus an MWCC-only atan implementation/table. Its
empty header had no other includers. The pre-change native executable imports
acosf/asinf/atan2f/atanf from Windows; these duplicate console definitions were
unused in the native build. Removing that dead source/header resolves the
duplicate-symbol and unused MSL infinity reference while preserving the current
Windows math calls. Before/after import lists and the failed link log are
retained rather than treating the initial failed build as a pass.

All 17 selected audio/load/input/checkpoint/network tests pass. The optimized
binary completes two full stock-zero/delayed and interpreted-delayed local UDP
cohorts, each with 480 interpreted exactly-fifteen cycles, unchanged endpoint/RNG,
confirmed checksums and production snapshot peak sizes. Ordinary source/binary/
log/timing population verifiers pass. Full active-wall medians are 12.119 and
12.111 ms; maxima are 16.212 and 15.137 ms. This remains above the 10/12-ms target.

An additional offline Final Destination stock/interpreted eight-actor census
restores and resimulates fifteen ticks with CPU-only output. Each case compares
all 39 captured components and reports zero raw endpoint differences; 117
same-boundary Load/recapture comparisons also pass. The baseline/current
fighter/companion traces match within each case, and four audio starts are
reused with no new/canceled starts. The independent verifier checks source/log
pins, exits, component counts and raw endpoint markers. This is one interval
per case, not all-scene/fighter or production timing acceptance.

The final executable shrinks from 206,746,624 to 18,209,280 bytes; its separate
PDB remains available. Compiler/configuration evidence, binary/PDB, imports,
changed/deleted sources and test records are retained in `ipo-oct13-final`.

## Scaled fighter pose path, 2026-10-13

`ftParts_PObjSetupMtx` used the generic HSD setup only when the current fighter
had no Z-scale correction. With nonuniform Z scaling it continued through the
custom matrix/normal upload path even when geometry was disabled. Game & Watch
sets that width on load; Flat Zone also applies a common Z scale to fighters.
The override now uses the existing generic pose-only setup whenever geometry
is disabled, retaining joint/envelope preparation and matrix marks. Its normal
rendered scale-correction path is unchanged. No new shim or script API is added.

The model-runtime fixture checks rigid and shared-joint polygons with a 0.5 Z
scale. Pose-only mode emits no matrix uploads and keeps the expected joint mark;
normal geometry mode still emits matrices. The complete model-runtime test passes.

`scaled-pose-oct13-stages-b` compares the pre-change and new binary with four
stock Game & Watch CPU9 fighters on Final Destination and Flat Zone, 360 fast
input samples each. Gameplay/action traces match. Each candidate performs an
eight-tick GPU-free reconstruction: all 39 components match and raw resimulation
endpoint differences are zero; 117 same-boundary comparisons also pass per stage.
The older rewind raw differences (1,561 / 2,033 bytes) remain in the evidence,
rather than being relabeled as zero. These are correctness checks, not timings.

The first baseline launch failed because the retained executable had no UI
assets beside it. That failed cohort remains retained. The accepted baseline
uses the exact old executable with copied unchanged DLL/UI support in a separate
runtime directory. The stage runner now accepts any fresh output directory,
records GPU/model omission flags, and pins the changed source plus runtime files.
An independent report verifies current source pins, exits, component/endpoint
markers and hashes the logs; the runner originally did not pin logs at capture.
No 10/12-ms performance gain or all-fighter/stage acceptance is claimed here.

## Rejected deferred specular-light cache, 2026-10-13

An experiment deferred per-joint specular half-vector normalization during
GPU-free reconstruction, retaining the latest position/light-vector inputs on
each light and closing the calculation at display/list-replacement boundaries.
The initial omission left 28 raw bytes different. Two subsequent versions left
22 bytes different because an active-list replacement path discarded pending
work. Those failed cohorts remain retained. Consolidating both replacement
paths and closing camera callbacks finally passed the complete comparison.

`specular-phase-oct13-census-d` completes stock and interpreted eight-fighter
Final Destination cases, each with fifteen GPU-free ticks, all 39 captured
components equal, zero raw endpoint differences and 117 same-boundary recapture
comparisons. Four audio starts are reused with no new/canceled starts. An
independent report checks source/binary/log pins and the actual diagnostic
markers. All 18 selected core/audio/model tests pass for that candidate.

Two subsequent full local UDP timing cohorts each contain 480 interpreted
exactly-fifteen cycles with unchanged endpoint/RNG and confirmed checksums.
Active-wall medians are 12.027 / 12.117 ms; measured maxima are 17.229 / 15.927
ms. HSD-draw medians are 2.607 / 2.557 ms, compared with the earlier 2.594 /
2.567 ms. These unpaired populations do not demonstrate a useful gain from
the added cache. The experimental runtime fields, deferred calculations and
fixture were therefore removed rather than retained as speculative complexity.

The rejected executable/PDB/source/test versions and reports are retained in
`C:/FreakMelee-test-archives/specular-phase-oct13-rejected`; live/cohort logs
remain in their sibling directories. The census runner retains only the useful
additional `lobj.h` / `gobj.c` provenance pins. Correctness of the rejected
candidate is not a performance or final-source acceptance claim. The existing
accepted scaled-pose/ThinLTO implementation remains the runtime foundation.
After removing the experiment, that source was rebuilt and the same 18 selected
tests pass again. The restored build is not assigned the candidate's timings.

## Script fighter admission without duplicate checks, 2026-10-13

The owned-process stack sampler identified callback-source liveness and actor
lookup among interpreted native leaves. Its method, captures and limitations
are recorded in `native_stack_sampling.md`; those sampled runs are not deadline
measurements. Kernel ETW capture was unavailable with the current token.

All four callers of the private fighter `runFunction` now establish actor
status before accessing the program or callback slots. The hook/motion/helper
paths already did so; callback delivery now checks there too. Contract checks
between admission and `runFunction` execute no callback. Its duplicate entry
walk is removed. After actual script execution, generation and actor status are
checked afresh once, preserving retirement precedence, nested faults and the
rule that records/results are published only after a successful continuation.
All binding/VM guards around native reentry remain. There is no new cache, ABI
field, script primitive, wrapper or snapshot state.

The actual fighter fixture passes 8,392 callback/lifetime/motion/snapshot checks
with 53 writes, including source retirement and faulted helper rejection. All
19 selected fighter/core/audio/model tests pass. An attempted broader receiver
fixture fails to link duplicate Setup/Combat/State/Transition definitions; that
failure is retained, and receiver-suite acceptance is not claimed. The first
batch also named an obsolete jump target; it failed before building and is
retained separately.

`script-admission-oct13-census-b` checks stock/interpreted eight-fighter Final
Destination cases: fifteen GPU-free ticks, all 39 captured components equal,
zero raw endpoint differences, 117 same-boundary comparisons and four audio
starts reused with no new/canceled starts each. The independent report checks
the actual source/executable/log pins and diagnostic markers. The census runner
now pins fighter actor/execution/lifetime source and the fighter snapshot API.

Two unprofiled production local UDP cohorts have 480 / 457 interpreted
exactly-fifteen cycles. Active-wall medians are 11.989 / 11.990 ms, with measured
maxima 15.224 / 26.697 ms. Peer checksums and endpoint/RNG remain unchanged;
peak snapshot sizes remain 2,570,103 stock / 2,940,591 interpreted bytes. The
small median difference from earlier runs is not a controlled attribution of
all full-cycle cost to this cleanup. The 10/12-ms target remains unmet.

## Receiver and shared primitive fixture repair, 2026-10-13

The receiver link failure above is now resolved. Shared kinetic engine leaves
previously also defined unavailable Setup/Combat/State/Transition bindings,
which collided with real implementations or another fixture's boundaries.
Those unavailable bindings now live in a separate unit used only by the
standalone primitive runner. Complete CMake targets select their existing
controlled source/attachment providers exactly once. An old synthetic
collision-box helper is removed because the actual extracted body now owns it.
Missing unexercised Item/stage/vector/accessory/common-state leaves receive
explicit rejected boundaries; no production operation is replaced by a fixture.

The receiver world now supplies the selected fighter package's actual parameter
prototype to both instances of that same package. The article keeps a separate
parameter view (11 versus the fighter prototype's 2), preserving a discriminator
between program and receiver reads. The old per-instance fighter gain override
was incompatible with current owned-source admission. Identity/schema/source
replacement checks remain. A new negative case deliberately substitutes the
article prototype into a Fighter view, rebinds that execution view and proves
that native motion entry fails before any effect/position/motion/result write.
No production ownership guard was relaxed to accommodate the fixture.

All nine selected CMake probes pass: native bindings, Execution, kinetics,
state/collision, fighter lifecycle, Setup, combat, transitions and retained
receivers. Strict standalone runs also pass with current source/compiler pins:
kinetics 26,148 checks / 7,104 native-body comparisons; combat 37,048 checks /
12,240 RNG pairs / 30 native-body comparisons; receivers 4,435 checks /
18 execution runs; transitions 451 checks / 26 execution runs. Standalone
dependency lists are updated directly and accept fresh directories on any drive.
Failed build/run attempts remain retained under `receiver-fixture-oct13-*`.

These probes use controlled graph/animation/motion installation and other
explicit engine boundaries. They are not full match, article-creation, renderer
or distributed-network acceptance. The production executable hash is unchanged
from `script-admission-oct13-final`; no new performance measurement or claim is
made here. The 10/12-ms performance target and the full project goal remain open.


## Complete texture/palette FIFO packets, 2026-10-13

Aurora commit `89ce7b453ba6897cdc3f04a5ff6b8b67bbd27acb` serializes each
texture or palette load into a fixed stack packet and performs one FIFO append.
Texture loads retain six BP registers, the optional CI palette register and
37 bytes of host-pointer/identity metadata (67/72 bytes total). Palette loads
retain both texture-state flush registers, the two palette registers and
26 metadata bytes (46 total). Frontend object/dirty-state updates and decoder
command order remain. No thread, cache, allocation, schema or snapshot state
is added. Skipping these loads outright was rejected: CPU-only decoding must
retain bindings for later draws that do not repeat their setters.

All 212 GX correctness tests and 19 selected native fighter/core/audio/model
tests pass. New probes compare the old scalar encoding against packet bytes and
frontend mutations for every texture map, RGBA/CI and mip variants, and all 20
palette slots. They cover public display-list capture, FIFO growth and inline
CPU-only to worker-visible ownership. An undersized display-list buffer now
receives none of that complete load, rather than malformed scalar fragments;
valid captures remain byte identical. The first display-list fixture compared
different dirty-state starting contexts and failed; the corrected fixture uses
the same frontend context for both encoders. That failed log remains retained.

The optional append-only benchmark alternates eight scalar/packet samples of
65,536 loads with warmed, reused capacity. Texture append medians are about
33/14 ns and palette append medians about 22/9 ns (roughly 2.4x). This excludes
decoding, GPU work and snapshot cost; it is not a whole-frame speed claim.

Stock/interpreted eight-fighter Final Destination replay comparisons each pass
15 CPU-only ticks, all 39 captured components, zero raw differences, 117
same-boundary comparisons and four reused audio starts with no new/canceled
starts. Two unprofiled local UDP cohorts each provide 480 interpreted full
15-tick cycles plus a normal rendered submission and checkpoint work. Their
active-wall medians are 11.870/11.873 ms and measured maxima 17.166/15.474 ms.
Peer checksums, endpoint/RNG and snapshot peak sizes remain unchanged. These
are modest unpaired median differences from older runs, not isolated attribution
of the full-cycle cost. Explicit sleeps are excluded; OS/GPU/worker waits remain.
The 10-ms median / <=12-ms measured-peak target remains unmet.

Exact binaries, tests, configuration, source and reports are retained under
`C:/FreakMelee-test-archives/texture-packet-oct13-final`; fresh census and live
cohorts retain their own source/executable/log pins. No profiler or debugger
was attached to the timing runs. All launches were hidden and muted. Continue
with the larger simulation and display costs. The full project goal stays open.


## Immutable bytecode admission, 2026-10-13

`nativeScriptBytecode_Prepare` checks every immutable function and instruction
after lowering and before the source bank can be accepted. It checks function
extents/signatures, destination/operand/selector/argument ranges, helper call
signatures, branch and return destinations, text/closed-selector constants,
known opcodes and terminal control flow. A program carries a prepared marker;
the source, instruction/function arrays and their extents must remain unchanged
through its executions. Failed preparation clears the marker. The VM and
Execution binding reject unprepared programs; there is no fallback runtime
validator, second interpreter or serialized format migration.

The VM no longer repeats those structural checks per instruction. Entry helper
IDs/argument arity, stack/register pool capacity, argument conversions, dynamic
values/operators/selectors, result/output types, native-reentry source barriers
and actor/receiver/storage lifetime checks remain. Prepared source-bank metadata
is outside mutable match state; snapshot peaks remain unchanged. Preparation
allocates no runtime storage, and no new threads or tick allocations are added.

Thirty malformed-program cases verify failed preparation and rejection before
host liveness, any output publication or VM stack mutation. Existing numeric,
reentry, retirement, output and pool-exhaustion tests remain; the bytecode fixture
passes 8,475 checks over 12 functions / 282 instructions. All 35 selected native
script/core/audio/model tests pass. Manual bytecode fixtures now prepare their
programs explicitly. State/storage probes compile their complete declared
programs instead of exposing uncompiled function slots; the storage probe now
uses the event signature catalog. An obsolete negative event assertion treating
supported model-effect physics / trail contracts as unavailable is corrected.

The wider rebuild exposed stale Item/presentation fixture link dependencies.
Unexercised engine operations now have explicit unavailable/rejected boundaries;
the presentation fixture also supplies the exact null-safe AObj flags getter.
These are test-only changes, not production substitutes. Initial build, partial
program and event-test failures remain retained. The read-only content tool
passes six paired-member preparation audits across Zelda-start, Sheik-start
and Ice Climbers projects, with zero diagnostics. This is preparation evidence,
not all-fighter gameplay acceptance.

Three strict standalone probes also pass: between-joint effects (66,433 checks,
76 cases, 24 native comparisons, 14 retirement mutations), event records (197
checks / 16 cases), and airborne items (278 checks / 50 cases, 12 native field
pairs / four VM calls). Eight older standalone harnesses still fail: article
lifecycle, common and feedback-query catalogs and pose catalog lack current
binding dependencies; CPU control lacks current arena/scene dependencies;
vectors lacks lbVector_Rotate; async and raw-part effects stop on native source
comparison guards after prior effect-owner / particle-pool changes. Their
failure logs are preserved, and acceptance is not claimed for those harnesses.
Their manual bytecode constructors adopt preparation; fresh output directories
can now use C: rather than the nearly full A: drive.

The fresh stock/interpreted eight-fighter FD census passes fifteen CPU-only
ticks, all 39 captured components, zero raw endpoint differences, 117
same-boundary comparisons and four reused audio starts with no new/canceled
starts each. Independent source/binary/log checks pass. Two unprofiled real UDP
cohorts each provide 480 interpreted exactly-fifteen cycles plus a normal
rendered submission and checkpoint work. Active-wall medians are 11.769/11.850
ms, with maxima 17.507/14.851 ms; simulation medians are 6.286/6.257 ms and
drawing medians 4.223/4.221 ms. Endpoint/RNG and snapshot peaks are unchanged.
The small differences from older unpaired runs are not isolated attribution to
this change. Tail samples remain. The 10-ms median / <=12-ms peak gate is unmet.

Evidence is retained under `C:/FreakMelee-test-archives/bytecode-prepare-oct13-*`.
All match launches were hidden/muted, with no profiler/debugger attached. The
next performance work should target repeated mutable-source admission and the
remaining display preparation, retaining necessary reentry/lifetime checks.
The full project goal remains active; local commits only.


## Source checks at execution boundaries, 2026-10-13

Execution now distinguishes its live storage identity/generation from engine
source liveness. Private read/write/invoke/assignment-validation entry checks
retain storage validity without walking the actor list before a native,
receiver or payload boundary that already admits its source. VM entry and
native-reentry barriers remain. Native invocation still checks full source and
storage liveness before publishing arguments/results; execution completion
still checks it before exposing the callback result/record. Receiver scope,
schema and lifetime checks remain. Local storage/table access performs no
engine reentry. There is no cached actor pointer, changed ABI, allocation or
new saved state.

Plain position, velocity, position-delta, acceleration and admitted Item slide
coordinate reads return after the source-admitted scalar load. Address selection
and copying that field perform no engine calls. Non-coordinate reads retain
their final source check; callback-capable specialized binding paths retain
their own barriers. The property fixture adds 36 generation/unlinked-source
rejections across 12 Fighter / six Item coordinates, poisoning unlinked userdata
and verifying outputs remain untouched. It passes 39,699 checks / 9,804 property
observations. All 35 selected native script/core/audio/model tests pass, including
the existing 8,392 fighter callback/lifetime/motion/snapshot checks and retained
receiver/reentry/output tests. Older standalone gaps remain as recorded above;
no acceptance is added for those harnesses.

The stock/interpreted eight-fighter FD census again matches all 39 components
after 15 CPU-only ticks, with zero raw differences, 117 same-boundary comparisons
and four reused audio starts, no new/canceled starts each. Independent receipt,
source/binary/log and diagnostic checks pass. Two unprofiled local UDP cohorts
each provide 480 interpreted exactly-fifteen full cycles. Medians are
11.824/11.816 ms, maxima 15.136/14.230 ms; simulation medians 6.243/6.296 ms and
drawing medians 4.244/4.212 ms. Checksum/endpoint/RNG and snapshot peaks remain
unchanged. This is not a substantial full-cycle speedup, and small unpaired
differences from previous runs are not isolated attribution. Tails remain.
The 10-ms median / <=12-ms peak requirement remains unmet.

Exact artifacts are retained under
`C:/FreakMelee-test-archives/script-source-gates-oct13-final`; the census and live
cohorts retain their own input/log pins. All match launches were hidden/muted,
without debugger or sampler. Aurora still calls Rml context Update every visible
frame; its current dependency exposes a next-update-delay hint. Investigate the
remaining UI/display costs and their actual update contracts before changing
their cadence. The full project goal stays active; local commits only.
