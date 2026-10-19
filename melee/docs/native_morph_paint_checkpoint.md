# Morph-target weight painting

Verified: 2026-10-19. The existing Model brush now paints complete-shape and
additive morph meshes, including the first stroke on an unweighted morph mesh.
The [runtime checkpoint](native_morph_skin_checkpoint.md) describes the shared
controller and private draw-batch representation.

## Authoring behavior

The reader imports and validates the costume graph once per read transaction,
then decodes every target through `HSD_ShapeSetReadVector`. Drawing and authoring
share this format/fraction decoder. Logical positions retain their identities
across targets, including coincident base positions with different target values.
Complete/base positions and additive offsets enter the same envelope coordinate
domain as the existing rigid skin editor. Normal targets use its inverse transpose.

The writer keeps each authored shape polygon in its animation-table position.
Palette partitioning happens only in its private envelope chain. It preserves
position targets, material ownership, UVs, colors and the parent's class/flags.
When painted vertices give a shared normal different bindings, all targets of
that normal receive matching private logical entries. Target arrays use float
components; their index tables can share equal physical vectors while logical
vertices remain separate. Each array retains a dense base prefix for admission.

Paint rebuilds, compacts and validates a candidate costume before publishing it
through the existing asset-history transaction. Retired arrays and batches are
reclaimed by compaction. Runtime evaluation keeps its fixed scratch arrays and
introduces no tick or draw allocation. The current limits remain 2,000 morph
positions and 2,000 normal vectors; NBT uses three vectors per entry.

Source-aware **Keep binding** reimport explicitly rejects morph meshes until
target-aware reimport can preserve their targets. This rejection leaves the
draft unchanged. GLB morph-target encoding remains open; ordinary GLB source
skin import, weight transfer and reimport retain their existing behavior.

## Geometry and ownership evidence

`native_shape_skin_test` passes **430 cases / 325,224 checks**. The new coverage
extends the previous runtime suite with:

- **180** no-op authoring conversions across complete/additive targets, all five
  component formats, root/deform/helper owners, unweighted/weighted sources and
  absent/XYZ/NBT normals. Actual runtime corners and lighting vectors agree at
  three poses and six blend settings. Coincident base positions retain distinct
  target identities.
- **12** public brush cases with shared normals. They check the selected weight,
  untouched weights and target values, split normal targets, sixteen further
  strokes with bounded compacted storage, and a reopened no-op rewrite.
- **Four** multi-controller cases with thirty distinct bindings. Each of two
  public shape polygons draws through four private envelope batches while
  retaining its independent controller, blend values and animation index.

`.local/native-morph-author-tests-1.log` records six passing CTests: DAT, heap,
rest edits, Model posed vertices, weighted morphs and rigid skin. The unit output
is `.local/native-morph-author-unit-4.log`. Native runtime/tool build:
`.local/build-native-morph-author-4.log`; final unit build:
`.local/build-native-morph-author-5.log`.

`tools/review_native_morph_paint_capacity.py` fills an XYZ morph with 2,000 logical
normals, then paints one vertex sharing a normal with other vertices. Splitting
would require 2,001 entries. The operation rejects and every draft file remains
unchanged: `.local/native-morph-paint-capacity-1/receipt.json`.

## Native Workshop evidence

Two hidden, muted runs use the real mouse brush, Save and byte-exact Undo/Redo:

- `.local/native-morph-paint-live-1/receipt.json`: an already weighted complete
  morph changes the selected bone from **0.5 to 0.7** on one vertex.
- `.local/native-morph-paint-live-2/receipt.json`: an unweighted additive morph
  receives its first skin stroke, changing the selected weight from **0 to 0.2**.

Both retain the other three projected vertices with **zero screen displacement**.
The full-UI PNGs show the selected bone's weight heatmap. These runs exercise
the fixture's default blend; the geometry suite covers other blends and poses.

Related regressions also pass after the reader/writer changes:

- All **130** retail costumes: **8,149 meshes**, **123,904,853 checks** and **227**
  brush cases in `.local/native-morph-author-stock-1/receipt.json`.
- **54** rigid cases / **394,297 checks**, including large helper attachments:
  `.local/native-morph-author-rigid-1.log`.
- **12,390,828** large-GLB checks across rigid, automatic, source and gradient
  skins, each with **70,002** vertices: `.local/native-morph-author-large-1/receipt.json`.
- Target-aware reimport rejection and complete unchanged draft hashes:
  `.local/native-morph-reimport-guard-1.json`.

The combined receipt is `.local/native-morph-author-receipt.json`. This closes
the morph painting reader/writer gap. GLB target encoding, target-aware reimport,
unusual helper/texture-matrix domains and broader user workflow acceptance
remain in the Model plan.
