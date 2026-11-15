# Current rollback architecture

Updated: 2026-10-13. This describes production, not the historical page-based
probe or sixteen-slot diagnostic history. Optimization is deferred; preserve
the accepted behavior and recorded unresolved budget.

## Confirmed checkpoint and input history

Both direct diagnostic UDP and room-signaled ICE use the same confirmed-
checkpoint controller. Keep one saved state at the newest reconstructed
contiguous fully confirmed input boundary plus the current predicted live state.
Keep bounded per-frame inputs. A boundary C is immediately before input frame C;
its state includes frames through C-1. A later received frame cannot advance
confirmation across an earlier hole, including with multiple local players.

When confirmation advances or used predictions change, restore C, reconstruct
the confirmed prefix to D, capture D once if it advanced, then replay the suffix
to the current live boundary. Do not save each predicted or resimulated frame.
Correct earlier predictions can still require reconstruction to advance the
checkpoint; this is an accepted cost of the chosen policy. Stall rather than
lose inputs required by the prediction window.

After catch-up, schedule the normal visible tick using the reconciled history;
do not repeatedly start another reconstruction before rendering. Full-window
waiting and normal input receive/reconcile remain. Match-end/scene phases and
retained-transport sudden death renew the required history leases.
See [confirmed checkpoint policy](native_confirmed_checkpoint_plan.md).

## Saved state

Construction admits scene-owned mutable arena prefixes and audited external
roots. Ordinary save/load copies sealed spans; it performs no graph walk, hash,
compression or page-fault rearming. Immutable assets/programs and host UI,
network/device state remain resident outside the saved game state.

Saved ownership includes fighters/items, animation/effect/stage state, RNG,
process scheduling, object generations/free order and live links, authored
variables, callbacks and interpreter clocks. Stable addresses and pinned
programs remain valid within the admitted scene lifetime. Restore must reproduce
subsequent allocation and object creation/destruction, not only scalar positions.

Latest recorded real stock/interpreted eight-actor populations capture
2,570,103 / 2,940,591 bytes. Those are observed snapshot bytes, not every scene's
maximum. Pool capacity and address reservation are distinct from copied state.
The broad page capture and old copy-history paths remain diagnostic references.
See [arena ownership](native_match_state_arena_plan.md) and its dated evidence.

## Reconstruction and presentation

Hidden ticks preserve callbacks, pose evaluation, necessary material/animation
updates, render-side CPU state writes and RNG. GPU geometry/upload/output work
is omitted where the dependency audit permits it. A normal visible tick submits
one final rendered frame. This is not a license to skip all display callbacks.

Particles/effects spawn, age and retire during reconstruction. Restoring the
corrected state removes effects created by wrong predictions; correct events
remain even if they occurred in hidden ticks. Sound intents reconcile with the
live audio timeline, reusing stable starts and canceling discarded ones.
Physical rumble is suppressed during hidden ticks and applied at the endpoint.
See [CPU-only reconstruction evidence](native_cpu_only_resimulation.md).

## Current acceptance boundaries

Stock/interpreted eight-actor Final Destination census runs reproduce fifteen
GPU-free ticks with all 39 components equal, zero raw endpoint differences,
117 same-boundary recaptures and four reused audio starts with no new/canceled
starts in the recorded cases. Real local UDP runs check confirmed checksums,
input/correction counters and semantic endpoint/RNG agreement.

These are bounded scenes/workloads, not all fighter/stage/match transitions or
proof across distinct PCs. Raw process-local bytes and portable semantic checks
are different comparisons; do not silently waive unknown raw differences.

Latest committed uninstrumented local full-path cohorts provide 480 exact-
fifteen cycles each. Active-wall medians are 11.824 / 11.816 ms, peaks 15.136 /
14.230 ms. They include checkpoint work and one normal rendered submission,
exclude explicit pacing sleeps, retain OS/worker waits and share one CPU/GPU
between two clients. Render submission is not a GPU-completion fence.
The user target of 10 ms median / <=12 ms measured peak remains unmet and
deferred. Profiler/sampler captures are attribution, not acceptance timing.

Remaining work is targeted restored-state/lifetime coverage, integrated room/
Workshop/content checks, eventual separate-PC acceptance and deferred tuning.
