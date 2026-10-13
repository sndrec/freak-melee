# Effect library control ownership

The allocation serial, load mode, asynchronous effect count, animation count,
32-entry animation queue and eight-entry alpha/ID table now have an explicit
1,880-byte component. Actual table storage moved from `eflib.c` to `efdata.c`;
all consumers retain direct access to the original symbols. No source effect
spawn, update, queue drain, alpha setter or generation arithmetic is replaced.

Animation queue entries inside the active count have class span/type claims.
Draining leaves old pointer bits in the tail; these bytes are retained but never
followed or given live references. Parameter-table objects have GObj generation
claims. Restored GObj/class companions must agree before any effect control is
written. Overlap checks exclude actual controls, GObj storage, ordinary ObjAlloc
backing, active joints and their known type records. Snapshot operations allocate
nothing and execute no effect, animation, draw or audio callbacks.

The lease begins/ends with the production GM loop. Its geometry and epoch are
host state, while counters and tables remain actual simulation storage. This
component does not duplicate effect/queued-node payloads or allocator descriptors:
their HSD ObjAlloc backing already belongs to the shared allocator component.
GObj queue-head bindings remain GObj state. Source effect asset registries and
particle/generator/AppSRT globals, borrower/graph agreement and complete aggregate
restoration are still separate work. Production retains the broad page oracle.

Evidence under `A:/FreakMelee-test-archives/`:

- `effect-control-oct11-owner-a`: 67 added checks and ten malformed candidates;
  actual counters/storage/serializer and source alpha/ID setters, repeated
  sixteen-tick control updates, allocation serial restore, active/dormant queue
  handling, stale GObj generations and class types, companion resurrection,
  overlaps/activity/retirement. Root/joint construction and control updates are
  controlled; no source effect spawning or full-match Load is claimed.
- `effect-control-oct11-gm-b`: connected production GM/control/player/camera
  fixture passes with the effect-control lifetime hooks linked. Its ObjAlloc
  exclusion endpoint aborts if used; effect Save/Load is tested by the owner
  fixture with the real allocator implementation, not this GM fixture.
- `effect-control-oct11-eight-a`: stock/independent interpreted eight-actor
  scenarios pass all 33 components at ten boundaries each, with exact prior/new
  and diagnostic-on/off populated traces. No live aggregate Load is performed.
- `effect-control-oct11-independent-verification.json`: source/frozen/live input
  hashes, unchanged effect-library function bodies and actual `.mstate$m` versus
  host `.data` storage sections verified. Four build targets and focused
  GObj-consumer CTest pass.

Live capture/validation is observation-only. It does not establish total snapshot
size, full restore/resimulation or the rollback deadline.
