# Variable graph payloads and fixed shape scratch

2026-10-11 checkpoint. Texture-animation palette pointer tables and additive
shape-animation blend weights now use the same class-piece allocator as their
owning graph records. Ordinary and reserved graph snapshots therefore capture
these mutable payloads directly. The broad production page oracle remains;
this is a component ownership change, not a complete match snapshot.

## Payload storage

`HSD_TObjAddAnim` allocates its null-terminated `HSD_Tlut*` table with
`hsdAllocMemPiece`. Animation replacement and `TObjRelease` retain their existing
palette-removal order, then return the table using its exact pointer count,
including the terminator. Palette records already use class pieces. No wrapper,
secondary tracking directory or duplicate copy is introduced.

`loadShapeSetDesc` allocates additive blend weights with `hsdAllocMemPiece`;
`HSD_ShapeSetRemove` returns the same number of float bytes. Initialization and
animation writes are unchanged. A zero-shape direct constructor retains a null
weight pointer, matching the old zero-byte allocation; the DAT importer already
rejects zero-shape descriptors.

Ordinary storage belongs to [the class component](native_class_snapshot_ownership.md)
and its OS-header companion. Admitted reserved storage belongs to
[the reservation component](native_hsd_reservation_snapshot_ownership.md), with
class-count companions. Construction demands must include the table/weight
size and all palette records, rounded into existing class-piece bins. Unknown
demand still refuses; no heap escape is added. This does not remove the existing
source article shape-animation admission limits or automatically extend every
package's model demand census. The reserved fixture supplies explicit demands.

## Shape evaluation scratch

The vertex and normal interpolation buffers now have fixed 2,000-entry storage
at stable addresses: 48,000 bytes total. They occupy host `.data`, outside
`.mstate`; their pointers, lazy allocation flags and amnesia resets are removed.
Every consumed vertex/normal is overwritten by the current shape interpolation
before display-list emission. Average and additive interpolation, clamping,
normal/NBT layout and decoding math are retained. Original unsigned capacity
checks remain, including rejection of negative counts.

The native display-list reader additionally rejects position/normal/NBT indices
outside the current regenerated prefix. Malformed lists cannot depend on stale
scratch left by another graph or frame. This is an invalid-data guard, not a
change to valid animation behavior. No scratch clearing, per-tick allocation,
tracking, save/load copy or callbacks are needed for these buffers.

This reasoning applies only to these shape buffers. Constraint-expression
`RObj` argument scratch remains an open owner: its argument count includes
flags whose switch cases do not write a value. Without a provenance audit,
overwriting or excluding that scratch could change an expression's result.
Render caches, matrices, actual decoder state and other mutable buffers likewise
remain required until separately owned or proven regenerable.

## Evidence

Final archives use prefix `A:/FreakMelee-test-archives/graph-buffers-oct11-`:

- `resource-c`: actual ordinary TObj/PObj constructors, palette replacement,
  destruction, shape/table address reuse, original payload and exact OS/class
  re-save, repeated allocation order, empty shape/table handling: 56 checks.
  Explicit reserved construction and class/reservation restore: 39 checks.
  Ordinary full cohort: 2,911 checks/17 cases; animated: 4,638/18, retaining
  39 native/interpreted animation pairs and 68 scheduler/script companion checks.
- `scratch-d`: 112 bit-identical prior/fixed kernel and emitted GX-value pairs,
  2,859 checks and 13 capacity/index refusals. Average/additive, negative/clamped
  weights, 8/16-bit indices, big/native-endian floats, normals/NBT and alternating
  vertex-only draws. Fixed scratch is poisoned before every comparison. Prior
  lazy path allocates twice; fixed path allocates zero times. Actual decoding,
  interpolation and display-list bodies execute; GX leaves capture values.
- `build-c.log`: native client, content/source-assets and GObj/census targets
  build. Existing unrelated warning in `util.h` remains.
- `stages-b`: Battlefield, Great Bay and Brinstar, each 2,000 rendered CPU
  samples with Link/Samus/Ness/Ice Climbers and medium items. All populated
  trace families match the previous compiled client; no census failures.
- `source-b`: freshly prepared interpreted Samus/Link/Young Link/Ness, each
  1,800 samples, pass stock comparisons.
- `compiled-scratch.json`: actual production COFF symbols each occupy a
  24,000-byte `.data` section, excluded by `.mstate` registration.
- `independent-verification.json`: all 235/3,342/219/55/4 receipt pins, the
  previous 87-file client bundle and six production/frozen sources match.

Earlier resource cohorts are retained. `scratch-a` failed strict compilation on
the prior kernel's existing signed comparisons and an unrelated `util.h` inline
warning. The fixture now includes only needed headers and allows prior signed
comparisons; production retains the original unsigned capacity contract.

Rendered gameplay comparisons, interpreted source comparisons, compiled section
placement and independent input pin verification are recorded in the companion
receipts. No GPU pixel proof, all-asset shape/palette coverage, whole-match
snapshot size/timing or eight-actor rollback deadline is claimed. Other direct
HSD allocations, semantic globals, source registries, class preparation and the
complete aggregate/ring still need closure.
