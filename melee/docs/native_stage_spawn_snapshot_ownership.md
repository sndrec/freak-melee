# Stage spawn configuration and segment ownership

Corneria's two source configurations contain one and two spawn segments;
Brinstar's two contain ten and four. Their constructors now use actual
`stage_spawn_state.c` storage instead of separate HSD allocations. All original
stage call sites and source interpolation, random decisions and callbacks remain
direct. There is no heap mirror or second update implementation.

The fixed reservation holds 128 configurations with 32 segments each. Exhaustion
or an invalid segment count returns NULL with a diagnostic, using the existing
constructor failure result. Current source call sites fit with ample headroom.
Ground initialization retires old history and resets the allocation cursor before
stage constructors run. The shared generator constructor establishes the new
snapshot lease after those configurations exist. Later allocation of another
copy of an admitted definition is supported. There is no tick-time allocation.

The update writes every active transformed segment before consuming it. New
records give those pre-update bytes defined zero values instead of fresh heap
contents. Snapshots preserve the occupied prefix, configuration fields and only
the requested segments, not the unused reservation. Sizes are sixteen bytes
without configurations, 200 for Corneria and 464 for Brinstar on this native ABI.

The host lease retains source definition identities, callback/probability/count
bindings and descriptor contents. Unknown definitions, changed descriptors,
foreign segment pointers, malformed counts and partial/trailing payloads reject.
Load validates every record before any writes, then restores configuration data,
transformed segments and the allocation cursor. Rewinding the cursor lets later
source constructors reuse the same backing. Unoccupied records are not history
state; the next constructor initializes every field and requested segment it uses.
No save/load callback, transform, draw, physics or allocation is performed.

Source configurations in the selectable stages use positive probabilities and
leave `config.gen` NULL. The original negative-probability branch maintains a
persistent HSD particle generator; its behavior remains in the source updater,
but this component rejects such a configuration until a companion particle owner
and generation claims exist. This component also does not establish candidate-
wide Ground borrower/callback/graph agreement or complete-match restoration.
Production retains the broad page oracle during migration.

Evidence under `A:/FreakMelee-test-archives/`:

- `spawn-oct11-owner-final`: 2,105 added checks and eleven malformed candidates;
  source constructor/interpolation/update bodies, actual fixed storage and
  serializer, counts 1/2/4/10, sixteen repeated updates after restore, callback
  counts, full cursor capacity/exhaustion, occupancy rewind, changed descriptors,
  unaligned input, overlapping output/length pointers, untouched short output,
  inactive boundaries and scene retirement. Joint transforms, RNG values and
  callback endpoints are controlled. This is not stage gameplay acceptance.
- `spawn-oct11-gm-a`: connected production loop/control/player/camera restoration
  fixture passes with the new owner linked. It is not full aggregate restoration.
- `spawn-oct11-stages-a`: Corneria, Brinstar and Final Destination old/current
  rendered CPU matches pass, 4,800 samples each, with exact populated traces and
  forty passing spawn captures/validations per scene. Corneria's 200-byte owner
  has sampled mean Save 0.205 us; Brinstar's 464-byte owner has 0.265 us. These
  component means do not establish full-match latency or worst-case timing.
- `spawn-oct11-eight-a`: stock and independent interpreted eight-actor scenarios
  pass all 32 components at ten boundaries each. Previous/current and diagnostic
  on/off traces match exactly; neither live cohort performs aggregate Load.
- `spawn-oct11-independent-verification.json`: frozen/live input hashes, source
  updater and other generator/Ground bodies (except the constructor and explicit
  transition reset), compiled actual `.mstate$m` storage and host `.data` lease
  verified. Four native build targets and focused GObj-consumer CTest pass.

Rendered stage comparisons and eight-actor capture/validation are separate from
the controlled Load fixture. They do not prove full rollback or its final timing.
