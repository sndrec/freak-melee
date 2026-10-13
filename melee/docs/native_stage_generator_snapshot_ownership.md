# Shared stage generator snapshot ownership

The shared generator's 81-entry table now resides in actual fixed storage in
`stage_generator_state.c`, alongside its control state and driver object. All
source consumers access that storage directly. The allocation-only constructor
helper and private mutable header are removed. Successful construction retains
the original entry reset loops, scan cursor and callbacks; failed driver creation
leaves the existing table and binding intact. Unused `x8` payloads are not reset,
just as the original reset loops did not initialize them. Activated extra entries
set their payload before use.

`native_stage_generator_snapshot.c` captures 5,896 bytes, including the table,
control values, driver reference and paired GObj/Item generation references for
occupied entries. Its host scene lease remains outside simulation storage. Scene
retirement invalidates the lease. Capture requires an idle simulation boundary
and stable pool geometry, definition binding and table identity.

Validation checks the whole local candidate before writes. Loading requires the
GObj and Item companions to have been restored first, with matching generations,
roles, removal callbacks and reciprocal item ownership. A stale late-table entry
must reject restoration without changing an earlier timer. No snapshot operation
allocates, invokes game callbacks, advances physics or renders.

This component owns the shared singleton table, not the separate variable-size
configuration and spawn buffers constructed by `grZakoGenerator_801CA394` for
Corneria and Brinstar. Those now have a separate
[spawn component](native_stage_spawn_snapshot_ownership.md). Descriptor-array
contents, candidate-wide callback/graph agreement and complete aggregate
restoration remain separate work. Production still uses the
broad page-based snapshot system while explicit ownership is assembled.

Evidence under `A:/FreakMelee-test-archives/`:

- `generator-oct11-owner-b`: 165 generator checks and nine malformed-candidate
  cases; actual owner save/load, constructor/reset and selected source retirement
  bodies, controlled scheduler/item endpoints, companion slot reuse/restoration,
  repeated timer updates, overlaps, activity and scene retirement. The scanner
  itself is not executed by this controlled fixture.
- `generator-oct11-gm-a`: connected production control/player/camera restoration
  fixture passes with the new owner linked. This is not whole-match restoration.
- `generator-oct11-stages-a`: Final Destination, Brinstar, Castle and Poke Floats
  old/current rendered CPU matches pass, 4,800 samples each. Populated traces
  match, and all forty generator Save/Validate samples pass per scene. Generator
  mean Save is 1.175-1.295 us; these are this component's sampled means only.
- `generator-oct11-eight-a`: stock and independent interpreted eight-actor
  scenarios pass all 31 components at ten boundaries each. Previous/current and
  diagnostic-on/off traces match. No live Load is performed.
- `generator-oct11-independent-verification.json`: frozen/live input hashes,
  eleven unchanged source bodies, reset loops (with the local pointer renamed),
  actual `.mstate$m` storage and host `.data` lease verified. Four native build
  targets and the focused GObj-consumer CTest pass.

Rendered match comparisons and the eight-actor census are observation-only
Save/Validate evidence; they do not establish aggregate Load or the final rollback
size and frame-time target.
