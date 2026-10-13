# Shared stage snapshot ownership

The shared stage component owns the actual `StageInfo`, 64 joint-update flags,
the joint-update counter, four cached spawn vectors, `GroundParam.y`, and its
nine mutable lighting colors.
`stage_state.c` holds the actual storage; Ground and grLib use it directly.
The old heap allocation for the 64 flags is removed. The counter and spawn
cache retain their original initialization behavior; they are not reset merely
because a new snapshot lease begins.

`NativeStageSnapshot` is 17,160 bytes in this Windows build. It captures the
complete native records, including padding, plus typed backing/generation
claims. Save/Validate/Load use fixed stack storage and do not allocate, run
callbacks, or render. Stable joint backing references are cached in host state
and checked with the allocator's O(1) Resolve; changed attachments refresh the
claim through the real allocator. The cache never proves object occupancy.

## Lifecycle and resident data

The real stage startup queue is still pending when the GM match loop begins.
The lease starts in `Ground_801C0FB8`, after `on_start`, all queued callbacks,
their deallocation, and the shared stage procedure setup. Ground initialization
and GM loop exit retire it. Save/Load require the existing idle GM boundary,
unchanged selected stage and pool backing/lifetimes, and no pending start queue.

Scene-resident resource pointers are frozen: collision data, item descriptors,
ground parameters, animation command arrays, particles, texture generators,
yakumono attributes, lights, the unused x6C8 resource slot, and quake descriptors.
The parameter bytes outside the scale float and color span are byte-checked as
resident data. The nine color setters use local GroundParam aliases; Corneria
and Venom change those colors during play, so they are restored alongside scale.
This does not establish
that all payloads reachable through those pointers are immutable: collision,
class graphs, stage-specific mutable attributes and other owners remain separate.

The scene fixes the touch-line, shadow and event callbacks. The two optional
position predicates admit null or their startup identity. Currently selectable
versus stages do not install another predicate later. Non-versus modes with
late-installed predicates require additional admission/lifecycle evidence;
they are not silently accepted by this component.

## References and restore order

Map entries use both GObj and Ground generation references. Load requires a
restored stage-class GObj, Ground userdata kind/destructor, and the reciprocal
Ground owner link. Fog uses its actual GObj generation/class/kind. The special
stage-object handle has a generation claim.

The 261 named joint attachments use ordinary class-piece backing claims and
the class identities admitted at startup. They may change during play. The
four x694 entries hold **CObj pointers**, not GObjs; the actual constructor
passes the camera model to registration. Those entries receive class backing
and camera-type checks rather than fabricated GObj references.

Validate checks candidate geometry, leases and admitted identities without
requiring current pool occupation. GObj, Ground and class payload companions
must restore before Load. Load checks every root before writing any shared
stage value. Ordinary class references establish an epoch/block/span, not a
per-object generation or occupied graph. The aggregate must prove candidate
class-block membership, object roles, live graph membership and cross-owner
agreement before any component writes. These local checks do not replace that
aggregate validation.

## Evidence and remaining work

`A:/FreakMelee-test-archives/stage-state-oct11-owner-final-c/receipt.json` records
140 shared-stage checks, including fourteen malformed candidates, unchanged
state on rejection, sixteen repeated controlled updates, mutable scale/colors and
attachment replacement, generation reuse, companion restore ordering, class
type restoration, buffer overlap, activity, resident changes and retirement.
It uses actual Ground/class owners and serializers; the eight-slot GObj backing,
selected-stage getter, idle scheduler and model initialization are controlled.
It does not run a real stage or animate a model.

Final receipts under `A:/FreakMelee-test-archives/`:

- `stage-state-oct11-stages-final`: all 29 selectable versus stages complete
  1,200-sample old/current rendered CPU matches, with exact populated gameplay
  trace families. Shared-stage Save/Validate passes all ten sampled boundaries
  per stage, including Corneria after its lighting colors change. Per-stage
  mean Save observations range from 5.37 to 7.87 us. This is not every hazard
  interaction or a live restore test.
- `stage-state-oct11-eight-final`: four rendered runs, stock and independent
  interpreted Ice Climbers pairs/eight actors, with diagnostics on/off. All 27
  components pass all ten boundaries per scenario. Previous/current and
  diagnostic-on/off populated trace families match exactly. Stage Save averages
  6.02 us in both scenarios; Validate averages 1.27/1.23 us. No live Load.
- `stage-state-oct11-gm-final-c`: the existing connected GM/controller/input/
  RNG/player/camera-target fixture passes with sixteen repeated controlled ticks.
  No real stage startup/graph runs in that fixture.
- `stage-state-oct11-independent-verification.json`: rechecks 1,689 connected
  owner pins, both 1,829-pin GM source sets, 71 eight-actor and 246 stage-cohort
  pins, actual compiled simulation/host sections, and all 194 donor Ground,
  grLib and stage-state function bodies. Body changes are direct storage names,
  removal of the flag-buffer allocation, and stage-lease startup/retirement.
  Client/content/source-assets/GObj-consumer targets build; the focused
  GObj-consumer CTest passes.

These timings measure one component; they do not establish total snapshot cost
or the eight-actor fifteen-resimulation-tick deadline.

The four dynamic surface-attribute records and their active/free lists already
belong to the collision component; they are not copied a second time here.

Still open: stage-private globals, mutable per-stage attribute payloads and hazards
outside Ground records, other world/GM/HUD/effect/dynamics owners, multi-heap ownership, complete
resident leases, aggregate prevalidation/restore/history, differential rollback
against the page oracle, and the complete eight-actor fifteen-tick deadline.
Production rollback continues to use the broad page oracle.
