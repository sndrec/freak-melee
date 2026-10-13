# GM loop and mapped-controller ownership

2026-10-11 checkpoint. Two components capture actual state in `gm_1A45.c`
and `gm_1A36.c`, without a second loop or input representation. Production
rollback retains the broad page oracle; this is not complete match resurrection.

## Loop component

The 216-byte `NativeGmLoopSnapshot` captures the actual simulation/render/game
counters, scene-exit request, current/previous pause and frame-step bytes,
computed base/active process masks, cadence-mask selector and update flags in
`gm_80479D58`. It also captures GM's GObj initialization descriptor/pad, scheduler
object and debug-display roots, resident debug callback, current scene pointer
and both cadence masks. Original scheduling and callback order are unchanged.
The sole-call pause/mask initializer was placed directly at its existing match
loop callsite; its obsolete function/declaration were removed.

Frozen host metadata records an entry epoch, GObj scene epoch, scene pointer and
descriptor identity, configured pause/frame-step/pre-process/debug callbacks and
GObj config. It is in `.data`; actual mutable GM roots remain in `.mstate$m`.
Production object-symbol inspection confirms both placements. Scene setup and
loop exit disable acceptance. Every loop entry retires old history, even when
the scene pointer is reused. Descriptor rebinding rejects old candidates.

An activity guard covers input polling, simulation/callbacks and drawing, with
all early-break/continue paths balanced. Accepted capture/load boundaries are
outside that activity and outside HSD callback/deferred removal activity.
The guard is released before the existing census and rollback end-frame calls;
it remains clear before the next rollback begin-frame call. Load runs no scene,
process, render or native UI callbacks and samples no device.

Validation checks exact size and nonoverlapping buffers, epochs, resident scene
and configured code/config identity, legal exit/cadence selectors and structural
object references. It never dereferences serialized foreign pointers. Scheduler
and debug roots carry actual pool lifetime/slot/generation references. Structural
validation permits an old occupied slot to be described while its current slot
is free/reused; Load requires the restored exact generation and occupancy before
any GM writes. The GObj component must therefore restore before GM's roots.

GM's process-mask field is also pointed to by the GObj scheduler component.
Both current serializers save it. Aggregate prevalidation must require their
candidates to agree; independent component validation is not that cross-owner
proof. Descriptor data, callback code and borrowed scene enter/exit payloads must
stay resident for the history interval. OS quit requests, native UI, network
history and the rollback coordinator's local frame counter remain host state.
GM loop state does not cover all globals in other GM modules.

## Mapped-controller component

The 264-byte `NativeGmInputSnapshot` is the actual `controller_map` type, not a
mirror. It includes four ports plus their combined fifth port, button/trigger/
release/repeat/accelerated-repeat values, repeat countdown and held-frame count,
the resident repeat callback and all repeat acceleration thresholds/intervals.
Private offset-style member names were replaced with descriptive names at the
actual callsites. The layout and algorithms are unchanged.

Save/Load copy this complete fixed record without allocation. Validation admits
the actual resident repeat callback or the all-zero pre-initialization state;
foreign callback pointers, wrong size, short capacity, buffer/count overlap and
capture during mapping reject. The default record is a read-only initialization
prototype in the current source; it is not duplicated as mutable match data.
The HSD pad queue/status and LB clock remain separate required companions.

## Evidence

Under `A:/FreakMelee-test-archives/gm-loop-oct11-*`:

- `owner-final`: 1,415 checks run the production GM loop, input mapping, LB
  cadence, controller/rumble, RNG and generic pool bodies. At frame eight the
  fixture saves five components, advances to frame 24, restores, then repeats
  the sixteen-tick window. Every component record matches exactly, including
  pause/frame-step and draw-consumed RNG. Thirteen malformed/stale-generation
  refusals and 144 actual scene/process/draw activity attempts reject. Late
  generation failures leave GM unchanged; actual companion pool restoration
  resurrects a freed/reused root. Same-pointer loop re-entry and modified scene
  descriptor refuse history. Load advances no device/audio/constructor counters.
  A separate 128-frame macro/repeat sequence repeats exactly.
- Fixture host/network/profiling/render endpoints and scheduler callbacks are
  controlled in `boundaries.c`; no client/window/audio opens. Earlier `owner-a`
  through `owner-d` were fixture compile/link/setup failures, including passing
  NULL to the generic allocator's required output-reference argument. That
  fixture call was corrected, not the allocator. `owner-e` was the first passing
  subset. Accepted evidence includes the later expanded final cases.
- `eight-final`: four hidden/muted rendered 1,200-sample runs with actual stock
  and independent interpreted four Ice Climbers pairs/eight actors. All 23
  components save/validate at all ten sampled boundaries in both scenarios.
  Diagnostic on/off gameplay traces match. `eight-a` is not accepted: its runs
  overlapped a client relink, so the executable pin changed and the verifier
  correctly marked the batch FAIL despite passing trace comparisons. Final
  evidence was rerun after the build completed with unchanged executable pins.
- `independent-verification.json`: 1,808 frozen/original fixture pins, 38 live
  pins, zero exit/log errors, exact populated previous/current gameplay trace
  families and actual compiled GM data-section placement verified. Observed
  mean loop saves are 0.59/0.90 us stock/interpreted, with mapped saves 0.22 us
  in both; these are ten diagnostic observations per scenario, not full restore
  or worst-case rollback performance.
- `build-final.log`: native client, content tool and source-assets fixture build.

Remaining: complete world/player/stage/camera/HUD/effect/dynamics globals, other
heaps and mutable storage, frozen resident definitions and cross-owner graph
closure, aggregate prevalidation/load ordering/history and actual full-match
restore/resimulation timing. Total snapshot size and the fifteen-tick deadline
remain unproven. The goal is active; commits remain local.
