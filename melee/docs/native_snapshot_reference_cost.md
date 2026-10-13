# Retained-reference snapshot cost checkpoint

The ordinary ObjAlloc and class span queries now return references by value.
Previously each query checked its small output record against all retained
allocation blocks; class queries also checked initialized class-info storage.
Every consumer now uses the value directly. There is no output-parameter adapter,
new allocator cache, allocation, storage mirror or persistent format change.

Class queries retain their initialized-tree validation and backing-span search;
ordinary queries retain allocator identity, aligned-slot, range, size, epoch and
construction checks. Class type/role checks remain at the consuming owners.
Snapshot Save/Load buffer independence and overlap checks remain unchanged.
Queries establish physical backing, not occupied-object liveness. Candidate
pool/list/ID/callback agreement is still required for complete aggregate restore.

Class epoch zero is valid before the first heap retirement. Failure is an empty
span, not epoch zero. The first live eight-actor run caught an incorrect failure
marker that disabled camera/stage captures without changing gameplay. That run
is failed evidence and has been superseded by the corrected final cohort. The
owner fixture now checks an actual startup hsdNew reference at epoch zero.

Final evidence in A:/FreakMelee-test-archives/:

- reference-cost-oct11-owner-b: actual allocator/class/particle owner fixture;
  three startup checks added, existing malformed spans, reservations, retirement,
  rewind/regrowth, overlap refusal and companion restoration remain passing.
  Optional controlled benchmark uses one million queries with Resolve and
  100,000 particle captures; timing-loop checks are excluded from coverage totals.
- reference-cost-oct11-gm-b: connected GM/control/player/camera fixture passes.
  Particle/effect Save/Load are exercised by the owner fixture, not this fixture.
- reference-cost-oct11-eight-b: stock and interpreted eight-actor runs pass all
  34 components at ten boundaries, with exact prior/current gameplay traces and
  observation-on/off trace equality. No aggregate Load is performed.
- reference-cost-oct11-stages: Fountain and Green Greens pass 4,800-sample
  rendered old/new matches; camera, stage, hazards, grid and particle components
  each save/validate at all forty boundaries.
- reference-cost-oct11-independent-verification.json: final source/executable/
  receipt pins, prior/current traces and unchanged allocator bodies except the
  reference query methods verified. The retained baseline executable matches
  the previous accepted eight-actor receipt. Four build targets and focused
  ObjAlloc/GObj-consumer CTests pass.

Sampled particle-control mean Save cost fell from 23.770 to 14.240 us in stock
and from 22.400 to 13.820 us in interpreted eight-actor runs. Those are ten
boundary samples per case, not a worst-case or full snapshot measurement. Effect
control Save costs were essentially unchanged in this cohort. Production retains
the broad page oracle; complete aggregate save/load/history, graph closure,
restore/resimulation/graphics regeneration and rollback latency remain open.
