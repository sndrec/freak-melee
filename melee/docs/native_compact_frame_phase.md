# Production frame phase and pending stage startup

The first pending-node ownership implementation and cross-executable behavior
checks are documented in [native_stage_start_callback_snapshot.md](native_stage_start_callback_snapshot.md).
The loaded/started aggregate transition remains open.

`--before-input --history --resimulate` places the offline compact history hook
at the top of the original GM loop, immediately before production's
`nativeNetplayMatch_BeginFrame` position. It captures from boundary zero, before
input polling and before `loop_snapshot.activity` becomes nonzero. The post-draw
hook is disabled in this mode; there is no double capture or skipped bootstrap.

The source audit found no ordinary game writes in the gap between the accepted
post-draw census point and the next iteration's pre-input position. Optional
object observation is followed by rollback driver handling, then the loop test.
Production's BeginFrame starts its host audio intent frame before page capture.
This phase audit does not establish complete startup ownership or atomic Load.

## Cold capture exposes a real missing phase

The first actual boundary-zero captures refuse exactly four components: stage,
stage hazards, stage generator and stage spawns. The other compact owners capture
and validate. This is not a size limit or an interpreter preparation failure.

`Ground_801C0FB8` invokes the source stage `on_start`, processes the pending
`stage_info.x6A4` callback chain, then establishes stage/hazard snapshot leases.
The generator lease is established by its real source driver constructor,
`grZakoGenerator_801CAE04`, which also leases spawn definitions. The VS startup
flow invokes stage start from the READY/start callback; these leases do not yet
exist at the first GM boundary. Stage resident validation deliberately rejects
the pending startup callback chain.

Capturing after an arbitrary warm-up would conceal this gap. Production remains
on page history. The next work must represent pending startup and its transition
to initialized stage controls, including callback-node and definition lifetimes,
or establish a proven canonical setup boundary before prediction. The original
initialization/RNG/visible behavior must remain accounted for. No lease guard has
been disabled, no fake empty owner snapshot is published, and no callback is
eagerly executed by this diagnostic.

Capture refusal now flushes the complete rejected-owner batch. The pre-input loop
stops immediately on diagnostic failure, and its Load report initializes the
failed-owner sentinel before a history directory lookup can refuse.

## Independent graphics shutdown fix

That immediate pre-frame exit exposed an Aurora crash while draining queued GX
viewport commands. The debugger stopped in `gfx::get_render_target_size`, reached
from the FIFO worker's XF viewport decoder. Its condition accessed the recording
packet's render-pass vector before any packet existed. The subsequent viewport
and scissor setters also attempted to enqueue into a nonexistent packet.

`aurora/lib/gfx/recording.cpp` now checks its existing recorder ownership state:
target-size queries use the existing window fallback without an active packet,
and GPU viewport/scissor commands are recorded only while a packet exists. GX's
logical CPU state remains handled by the GX decoder. No extra frame, allocation,
task, lock, atomic or cross-thread state was added. Active-frame command behavior
is unchanged. The required Multi-Core By Default article was read before changing
this worker-owned lifecycle code; existing recorder ownership remains intact.
Reference: [Multi-Core By Default](https://www.dgtlgrove.com/p/multi-core-by-default).

## Evidence

Under `A:/FreakMelee-test-archives/`:

- `compact-phase-oct12-six-a` and `compact-phase-oct12-correction-a`: retained
  failures. Both stock and interpreted cold histories refuse before sample zero;
  cleanup then crashes before the graphics fix. These are not acceptance passes.
- `compact-phase-oct12-cold-b`: flushed four-owner refusal and original access
  violation retained, exit `0xC0000005`.
- `compact-phase-oct12-cold-debug.log`: actual native debugger stack through the
  GX FIFO viewport decoder into the absent recording packet.
- `compact-phase-oct12-cold-c`: after the graphics fix, the same four owners still
  refuse at zero, output is retained and cleanup exits normally with status 1.
  This proves refusal handling, not cold snapshot acceptance.
- `compact-phase-oct12-normal-c`: PASS, 480 samples per stock/interpreted
  eight-actor case, post-draw per-frame history and fifteen-tick resimulation.
  Intermediate/continued traces, all 37 compact records and complete raw endpoints
  remain exact after the graphics and phase-hook changes.
- `compact-phase-oct12-gm-b`: connected original loop/input/controller/player/
  camera fixture PASS, 2,018 checks plus 31,797 camera checks. Its compact phase
  mode is disabled; live tests provide actual hook evidence.
- `compact-phase-oct12-independent-verification.json`: pins, expected cold
  refusal/exit, normal accepted cohorts and connected fixture checked separately.

Later loaded/started ownership and match-start transition results are recorded
in [Compact snapshots across match start](native_compact_stage_start_transition.md).
The receipts above describe the earlier checkpoint and retain its failures.

Cold stage ownership, candidate-wide prevalidation, residency, retirement, output
reconciliation, production integration and the final performance deadline remain
open. The overall project goal is incomplete and can continue without user input.
