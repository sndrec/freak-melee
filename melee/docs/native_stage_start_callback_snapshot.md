# Pending stage callback ownership

The source queues stage-start events through `Ground_801C10B8` during loading,
then `Ground_801C0FB8` invokes stage `on_start` and traverses those queued events
in last-in-first-out registration order. The traversal saves each node's next
pointer before invoking its callback. This timing and traversal remain unchanged.

Pending nodes now occupy actual fixed native storage in `stage_start_state.c`
rather than individually allocated HSD heap nodes. Capacity is 4,096 events,
matching the native GObj capacity; multiple events may target the same GObj.
Registration requires a tracked target and admitted callback and fails explicitly
on exhaustion. Startup consumes the original linked traversal and clears occupied
storage afterwards. Events registered during consumption do not change that saved
next-chain traversal; clearing the final queue retains the original behavior.
No callback is executed by serialization, loading or definition admission.

The native state stores the actual nodes, occupied count and whether stage start
has run. Ground initialization clears it at the real scene transition. The host
definition lease retains the native callback functions admitted during registration
until the next real scene bind. It is separate from mutable game state.

`native_stage_start_snapshot.[ch]` serializes occupied records with callback
identities and GObj generation claims. Structural validation checks count, size,
phase, callback definition, epoch and target geometry. Save also checks the actual
source head/next chain. Candidate-object validation can resolve the saved targets
without consulting live occupancy. Load requires restored GObj companions and
checks all live targets before changing the queue. It rebuilds the same native
linked order, head and phase and clears unused occupied records.

At checkpoint `39623753e`, this component was not yet part of the 37-component aggregate. It provided
actual pending-node ownership for the next stage lifecycle change; stage,
hazards, generator and spawn leases still refuse at boundary zero. It does not
prove a complete cold snapshot or rollback across the READY/start transition.

Current integration and later verified match-start rewind results are recorded
in [Compact snapshots across match start](native_compact_stage_start_transition.md).

## Evidence

Under `A:/FreakMelee-test-archives/`:

- `stage-start-oct12-baseline`: frozen executable before replacing node storage,
  with matching runtime DLLs/UI for the cross-executable comparison.
- `stage-start-oct12-live-a`: retained failed test setup; the isolated baseline
  initially lacked its DLLs and could not enter the program. Not gameplay evidence.
- `stage-start-oct12-live-b`: PASS, 480 samples per stock and independent
  interpreted eight-actor match. Pre-change/current traces match in every family,
  including RNG, poses, physics, actions, damage, items and companion events.
  Actual boundary-zero pending callbacks save/validate/load/recapture exactly:
  one pending event, not started, 56 bytes. The aggregate's four stage refusals
  remain explicit and cleanup returns status 1.
- `stage-start-oct12-history-a`: PASS, 480 samples per family with per-frame
  compact history and fifteen-tick resimulation. Intermediate/continued traces,
  all 37 compact records and full raw endpoint bytes match.
- `stage-start-oct12-unit.log`: 50 callback snapshot assertions, 88 bytes for
  two pending records and zero callback invocations. Checks include exact list
  order/recapture, saved-object validation after a live target is freed, refusal
  before mutation until GObj companions restore, malformed fields, unknown
  callback definitions, idle-boundary refusal, aliasing and scene rebinding.
- `stage-start-oct12-independent-verification.json`: unchanged pins and accepted
  cross-executable, actual cold component, history and unit evidence verified.

Next work is integrating this state with loaded/started stage phases and the
remaining stage controllers. Pending callback targets need aggregate candidate
agreement before publication. Other cold definition/allocator lifetimes,
candidate-wide prevalidation, residency, production integration and the rollback
performance deadline remain open.
