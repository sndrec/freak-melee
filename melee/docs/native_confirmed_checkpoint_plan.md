# Confirmed checkpoint rollback policy

Selected by the user on 2026-10-12. Production now uses the confirmed checkpoint
with admitted mutable spans; its page tracker is diagnostic-only. The sixteen-slot ring
is an offline diagnostic and reference path, not the selected runtime policy.

## State and frame identity

Keep one saved mutable game state at the newest reconstructed contiguous
confirmed input boundary, plus the current live predicted game state. Retain
the bounded per-frame input history. Do not save each predicted frame or each
intermediate resimulation tick. No separate authoritative world is introduced.

Boundary C is the state immediately before applying input frame C. Its state
contains the result of frames 0 through C-1, all using actual inputs. Receiving
inputs for a later frame does not advance C across a hole. Do not interpret the
minimum of each player's newest received frame as proof of a contiguous prefix.
Check required-player masks for every intervening frame. Local shared-PC inputs
are actual inputs too; inactive ports retain their defined neutral semantics.

Frame zero is the initial checkpoint, captured at the agreed pre-input phase.
Both arenas and every required external root share this boundary/scene epoch.
Resident immutable definitions and stable live addresses persist throughout
its lifetime. Input receipt, connection state and checkpoint bookkeeping remain
host state: restoring game state must not forget newly received inputs.

## Reconciliation

Coalesce incoming input updates at a simulation boundary. Let P be the current
live boundary and D the newest contiguous confirmed boundary reconstructible
with available inputs, no later than P.

1. If neither the confirmed boundary advances nor used predictions change,
   continue normal simulation without restoring or saving.
2. Otherwise restore the checkpoint C to the live game addresses.
3. Run actual-input ticks from C to D. Capture D once if D advanced beyond C;
   publish its frame identity only after successful capture.
4. Run from D back to P using actual inputs where available and the ordinary
   prediction policy elsewhere. Capture no intermediate predicted states.
5. Continue normal forward play. If the live state itself is at a new fully
   confirmed boundary, it can be captured directly without historical replay.

A correction beyond an earlier input hole may require replay from C while D
remains unchanged; in that case restore but do not save another checkpoint.
An old late packet cannot revise already confirmed inputs: reject conflicting
duplicates according to the input protocol.

Correct predictions do not eliminate checkpoint reconstruction. If D is behind
P and there is no saved D, replay is needed to advance the checkpoint even when
the old predictions matched. This cost is an accepted part of this policy.
Process confirmation/corrections together rather than once per received packet.

The checkpoint must remain within the retained input window. Bound prediction
distance from the usable saved checkpoint and stall before exceeding the replay
limit or losing inputs needed for reconstruction. Do not discard required input
frames merely because their packets arrived. Scene transitions retire the old
checkpoint and initialize a new one; wrong-epoch loads fail before mutation.
Capture/restore failure aborts the match rather than exposing partial state.

## Presentation and checksums

Preserve audio/rumble rollback reconciliation and bounded frame-indexed intent
records even though game snapshots are no longer kept per frame. Replaying
already correct ticks must not emit duplicate sound or other external effects.
Publish confirmed gameplay checksums only after corrected state reconstruction;
old predicted checksums becoming input-confirmed are not automatically valid.

Current diagnostics still draw during replay. The next major optimization is
simulation without mesh emission/GX recording/submission, preserving gameplay
updates previously coupled to draw callbacks. Do not silently skip those updates
or change gameplay RNG. Render the final live state once per displayed frame.

## Implementation and acceptance priorities

Finish the minimum state-closure/lifetime work needed for compact checkpoint
correctness, then cut production session call sites over directly. Remove the
obsolete production page tracker when its replacement passes; do not put an
adapter or dual-save strategy around it. Retain diagnostic rings only while they
provide an independent test/reference purpose.

Test matching predictions, mismatching predictions, out-of-order packets and
holes, confirmation without correction, batched frontier jumps, corrections
beyond a hole, prediction stalls, shared-PC masks, and scene retirement. Compare
against per-frame reference runs with stock and scripted actors, paired fighters,
articles and allocation/reuse; verify state/RNG and audio behavior at boundaries.

Report saves per confirmed advancement, restores and extra reconstruction ticks
separately. Budget complete reconciliation including advancing C, rebuilding P,
input work and one final render. The existing fifteen-tick target is 16.67 ms;
include required work rather than assuming save cost disappeared. Sub-100-us
save/restore is not required. Further copy tuning and sparse predicted snapshots
can be revisited if measured checkpoint replay costs warrant it.

## Controller implementation checkpoint (2026-10-12)

`native_confirmed_checkpoint.[ch]` now owns one prebound two-arena capture,
scene/frame identity, contiguous confirmation reconstruction and replay cursor.
It captures input-zero state at construction, restores to stable live addresses,
saves only at the selected confirmation frontier, and leaves predicted replay
ticks unsaved. Confirmation-only advancement of a historical frontier also
reconstructs; a fully confirmed live frontier captures directly. No allocation,
graph discovery or allocator reconstruction occurs in its tick functions.

`nativeRollbackInput_ConfirmedBoundary` checks the retained prepared frames and
their required-player masks. It stops at an actual-input hole and rejects missing
simulated records; the older newest-packet query is not used as prefix proof.
The controller bounds prediction to fifteen ticks from its usable checkpoint,
checks all owner lifetimes before a load, and publishes only complete captures.

Packet acceptance and UDP/room receive now take `NativeRollbackInputHistory`
directly rather than `NativeRollbackSession`. Call sites were changed; no adapter
or alternate compatibility signature was added. Transport activity/identity and
peer ownership checks remain in their owning layer. Batch input acceptance remains
transactional. This removes network receipt's dependence on the page tracker.

The controller fixture uses real arena and input-history implementations with
synthetic eight-actor state and counted satellite roots. Its 9,609 checks cover
holes, corrections beyond holes, matching predictions, batched frontier jumps,
direct live confirmation, local masks 0/1/5/15, delayed/out-of-order arrivals,
multiple input-ring wraps, prediction stalls and stale-scene rejection. Saved
checkpoints match an independent all-actual-input reference. This is not a live
Melee or real script-VM parity claim.

Release build and eight focused CTests pass, including the real local two-peer
UDP exchange test (shared-PC ports, input redundancy, checksums and phase reset).
The replay fixture's missing observer-query stub was also repaired so it links
against the existing replay source. Evidence and source/binary hashes are retained
under `A:/FreakMelee-test-archives/confirmed-checkpoint-oct12-*`.

Production still uses its original session/page tracker. Next: direct session
cutover with construction-time owner admission, audio/rumble reconciliation,
confirmed checksums, correct boundary/input retention and stalls; then live
stock/interpreted replay comparisons and scene/teardown checks. Remove obsolete
production page tracking after those comparisons pass. Rendering separation
remains the next major performance target; no new timing claim is made here.

## Production cutover in development (2026-10-12)

Netplay now uses the confirmed checkpoint directly. The page-tracking session
remains used by the older diagnostic probes/benchmark, not production netplay.
Network receipt, replay-input rewind, audio intent frames, rumble suppression,
confirmed checksums and prediction stalls are integrated with the new controller.
Owner preflight/admission was moved into `native_match_state_admission.[ch]` and
is shared with the offline census; it runs only during scene construction.

Input retention now follows the saved boundary explicitly. A future packet cannot
replace an older required input at the same ring index; packet batches skip only
released inputs. The diagnostic page session releases its own expired window.
Both normal and early match-exit branches now finish the netplay frame; predicted
finishes wait for actual inputs and can restore the match if corrected.

First live batch: `checkpoint-cutover-oct12-live-c` PASS, two hidden/muted local
clients per stock-zero, stock-delay and interpreted-delay case, 600 input frames,
three CPUs plus human P1 Ice Climbers (eight actors), Green Greens and items.
All peer endpoints/RNG match; stock delayed runs match 2,136 selected settled
fighter/companion/item/RNG records per peer against the separate zero-delay run.
Independent verification: `checkpoint-cutover-oct12-live-c-verification.json`.
This batch predates the subsequent diagnostic and early-exit changes.

Longer one-stock Final Destination runs reveal an unresolved particle replay
closure issue, so the cutover is not fully validated. The lightweight same-input
replay probe caught a changed gameplay checksum/RNG at boundary 822 and another
at 2577 during the frozen GAME display. Do not mask RNG or claim match-end parity.
The old serializer also refuses a late finished-controller capture; that is
logged diagnostic coverage, not required production per-tick validation.

RNG call-site tracing and generator bytes narrow the early failure:
`checkpoint-cutover-oct12-match-end-g` original/replay frame 817 has the same
starting seed and 79 RNG calls. Frame 818 starts with the same seed but makes
119 versus 69 calls; the first differing sequence is particle generation
(`generator.c` and `particle.c`, symbolized with the matching executable/PDB).
`checkpoint-cutover-oct12-match-end-h` has identical processed generator bytes
at 817; at 818 the processed list has 16 versus 11 entries, with missing/new
one-shot emitters. This points to particle emission/lifecycle state, not a
proven repair yet. Check generator creation, particle command processing,
allocator/list state and retained definitions before changing gameplay code.

Diagnostic controls: `FREAK_MELEE_NETPLAY_CHECKSUM_TRACE=1` records controller
fields, self-replay hashes and RNG call sites; optional
`FREAK_MELEE_NETPLAY_CHECKPOINT_OWNER_PROBE=1` reserves two 64-MiB scratch buffers
and compares serializer owners on matching-input replay. Neither is a timing
cohort. Normal runs allocate no owner-probe buffers. The root cause and further
live stock/interpreted, stage/lifetime and match-end checks remain required.
Latest release builds; six focused tests pass. Evidence is in the named A:
archive directories and `checkpoint-cutover-oct12-unit-tests.log`. No claim of
complete snapshot closure or a simulation-only performance deadline is made.

## HUD ownership and phase renewal (2026-10-12)

The late particle mismatch came from an earlier object-lifecycle difference:
`ifTime_FreeCountdown` freed the countdown in the original pass, but replay
retained it because the private timer state had not been restored. Its live
object then changed allocation and emitter traversal. The timer/countdown state
now belongs to the existing versus-control owner. The adjacent stock-indicator
pointers, stock counters/effect queues, and retained score/ranking predicate
state are also captured from their actual storage. Call sites access that
storage directly; no alternate gameplay rules or RNG masking were introduced.

The deterministic offline intervals at 813 and 2570 now match all 39 owners and
the broader memory image with zero differing bytes. This replaces the earlier
unresolved late HUD failure, but is bounded interval evidence, not universal
state-closure proof. Source/model definitions remain retained outside mutable
copies where their lifetime is fixed.

Sudden death retains live objects and transport but needs a new root admission.
Scene-history epochs now advance independently of object epochs; renewal clears
the sealed directory without moving or resetting live payload. Old spans refuse
to load even when their backing is reused. Lighting amnesia also invalidates the
active-light cache: retired class pointers cannot survive until the next camera
pass or enter construction-time validation.

The artificial-delay harness flushes its last withheld inputs before match-end
confirmation. An explicit phase-closed marker lets the runner distinguish a
finished match from a transition into sudden death, and its final scan rejects
failure lines written after the first endpoint. Earlier scripted-end receipts
that stopped at that first endpoint are not accepted as transition proof.

`checkpoint-hud-oct12-scripted-end-d` passes the interpreted eight-actor match
through frame 3838, with peer endpoint `7dacb886e4392d6c` and RNG `a4a156d6`.
Both clients bind a second checkpoint and advance beyond 160 sudden-death ticks;
their settled frame-120 digest/RNG match (`a560f0ab07fef73b`, `a13a037c`). The test
stops there; a complete sudden-death result and distributed-PC play are not
claimed. All launches are hidden and muted, with owned processes terminated
after evidence collection.

Focused controller/transport tests, HUD owner tests (88,737 checks), match-loop
tests and real GObj lifecycle tests pass. Evidence is retained under
`checkpoint-hud-oct12-unit-d`, `checkpoint-gm-oct12-unit-e`, and
`checkpoint-gobj-oct12-unit-c`. The bounded trace recorder compares repeated
particle, emitter and GObj operations; its backing exists only in explicitly
enabled diagnostic scenes. Owner admission reports failures at construction.

Production still renders during reconstruction. Simulation/render separation
and performance profiling are the next targets; no new latency claim is made.

Final-source batch `checkpoint-hud-oct12-final-c` PASS: both raw restore intervals,
stock zero/delayed and interpreted delayed 600-frame comparisons, full stock
match end at 2687, and the separately pinned interpreted/sudden-death case above.
`checkpoint-hud-oct12-live-c-verification.json` independently verifies source,
binary and log hashes; both stock delayed peers match 2,136 selected settled
actor/item/RNG records against the independent zero-delay run. Copied peaks are
2,570,103 stock and 2,940,591 interpreted bytes in that Green Greens/items cohort.
Six final-source focused CTests pass. This establishes these local cases, not
all fighters/stages, entire sudden-death completion or an unrendered tick budget.

The next opt-in rendering checkpoint is documented in
`native_cpu_only_resimulation.md`. `FREAK_MELEE_NO_GPU_RESIM=1` preserves original
HSD callbacks/matrix setup while omitting model primitives and Aurora GPU frame
allocation, uploads and submission during reconstruction. Local stock/scripted
same- and corrected-input tests and delayed UDP matches pass; broad raw memory
differences are confined to independently identified particle-render scratch.
CPU-only reconstruction is now the default for netplay; normal frames still
render. Final-source default-policy stock/scripted UDP cases and late Brinstar
intervals pass. HSD callback separation and broader fighter coverage remain.
