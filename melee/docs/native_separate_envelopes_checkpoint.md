# Separate position and texture attachments

2026-10-19. Painting can now change surface weights without consuming the
matrix slots used by independently attached generated textures. Positions and
normals have ten GX slots, and texture matrices have their own ten. Previously,
the writer combined both sets into one table. A triangle could therefore fail
to save despite fitting both sets individually.

## Asset and runtime path

`POBJ_SEPARATE_ENVELOPES` uses the previously unused polygon type `3 << 12`.
Its descriptor references a 20-byte record containing a position-envelope table,
a texture-envelope table and ten follower markers. A marker names the position
slot that a texture follows, or contains 255 for an independent attachment.
This distinction survives equal weights followed by another paint stroke.
Ordinary polygons without independent attachments retain the stock envelope
representation. Each set splits between triangles when a draw section fills.
A triangle needing more than ten texture matrices rejects atomically.

The importer validates table sizes, normalized finite weights, inverse binds,
matrix selectors and follower correspondence during construction. Compaction
traces both tables through the typed descriptor. The runtime loads and resolves
both envelope lists, including their joint references, then frees both during
teardown. The added per-polygon runtime record holds two list pointers; the
ordinary polygon object stays the same size.

Rendering loads position/normal and texture matrices independently. Fighter
normal adjustments also apply to the new texture set. Hidden reconstruction
evaluates both sets' joints without GPU submission. Picking and weight displays
use the position set. Rest edits and the Model inspector read that same set.
The fighter's refraction loader preserves the new authored selectors.
Allocations occur during asset/scene construction, with no new tick allocation.

## Verification

- The 66-case rigid suite passes **516,443 checks**. Its former shared-capacity
  rejection case now saves multiple position bindings alongside nine fixed
  texture bindings, paints successfully and preserves the reflected vectors.
  Existing checks compare repeated animated poses, retained attachment identity,
  ordinary/NBT normals, compaction and subsequent paint strokes.
- Eight malformed separate-table cases reject: invalid follower markers or
  correspondence, position and texture selectors outside their tables, an
  unsupported rigid shortcut, non-finite weights, a missing table and an
  oversized texture table. Admission leaves the source bytes intact.
- The 450-case morph suite passes **5,837,906 checks**. It covers ordinary and
  additive targets, normal seams, painting and compaction. Twelve additional
  runtime checks load separate tables, execute hidden pose setup and restore
  mutated position weights, texture weights and the texture-list pointer through
  the existing HSD arena snapshot path.
- All **130 retail costumes, 8,149 meshes, 227 brush cases and 123,896,453
  checks** pass the current writer/importer/compaction batch.
- Three hidden, muted Workshop clients render the original, rewritten and
  deliberately incorrect reflected-texture fixture. The selected sphere has
  **zero changed RGB pixels** after rewriting at rest and with HipN rotated
  40 degrees. The incorrect attachment changes **12,944 pixels** after rotation.
  Full-UI screenshots were inspected; all clients stopped. The full preview
  comparison has one changed pixel outside the sphere, on the rewritten stock
  fighter. This is not a claim of whole-screen pixel identity.

The new high-capacity case is a backend geometry/paint check. The native visual
fixture exercises the separate-table renderer with fewer bindings; it does not
claim an interactive nine-texture editing workflow.

## Receipts

- `.local/native-separate-envelopes-tests-3.log`: final focused CTest results.
- `.local/native-separate-retail-1/receipt.json`: every retail archive and totals.
- `.local/native-separate-texture-visual-1/receipt.json`: binary hashes, full-UI
  PNGs, comparisons and client cleanup. `rewrite.log` records the actual writer.
- `.local/build-native-separate-envelopes-3.log`: native client, content tool and
  geometry targets. `.local/build-native-separate-envelopes-4.log` adds the final
  snapshot cases.
- `.local/native-separate-envelopes-receipt.json`: source/binary identities and
  links to the checks above.

## Remaining boundaries

Independent attachments requiring a different normal coordinate basis still
reject before painting. Unusual helper model-node domains remain open. This
change preserves independent weights and expands their matrix storage; it does
not add texture-specific normal corrections. The overall Model milestone and
its collaborative usability review remain incomplete.
