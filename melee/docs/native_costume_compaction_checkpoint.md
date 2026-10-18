# Costume storage compaction

Painting, mesh/texture imports and compensated rest edits now reclaim
unreachable costume data before publishing the edited asset. Repeated edits
reuse the available storage around the live rig and materials.

## Layout and references

`native_fighter_costume_compact.cpp` imports every recognized costume root and
collects the serialized records and buffers that each graph uses. The importer
reports complete record sizes, strings, display lists, indexed vertex buffers,
shape targets, compressed animation streams, texture pixels and palettes.
Tracing is enabled during compaction; ordinary scene imports keep the callback
empty. The operation runs as part of an explicit asset edit.

Overlapping ranges remain together, preserving interior pointers and shared
buffers. Bone, mesh, material, texture and texture-sequence records keep their
offsets. These offsets identify saved labels, editor selections and preview
poses. An allocation already at offset zero keeps that address; other
allocations receive nonzero addresses. Every moved range keeps its original
alignment modulo 32.

The writer packs the remaining live data into gaps between those stable
records, rewrites local references and root targets, and rebuilds the relocation
table. The complete output passes the importer again before publication.
Independent roots use separate temporary import contexts; this also handles
Giga Bowser's combined model and material-animation data within the existing
per-graph capacity.

Pinned records can leave gaps in the file. Compaction bounds repeated-edit
growth while preserving the identities used by the project. New live geometry,
textures and authoring objects can still increase the asset's size.

Unknown roots, external symbol chains and references to opaque data retain the
complete input. The compaction report records the reason. Invalid known graphs
and oversized retained output fail atomically. Temporary costume transactions
may exceed the package limit while obsolete data is present; the final compacted
asset must fit the existing package limit.

## Shape geometry validation

The shape renderer computes position and normal arrays from the shape targets.
Its display-list indices address those generated arrays. Import validation now
checks those indices against the generated vertex/normal counts and validates
the source target arrays separately. Color and UV attributes retain the normal
serialized-array checks. This follows `setupShapeAnimArrayDesc` and the shape
draw path in `pobj.c`.

The rest-test fixtures now provide valid UV arrays for complete model import.
Their assertions check retained mesh identities, flags, shared-bind isolation,
immutable source bytes and evaluated geometry. Old unreachable polygon records
can be reclaimed. The native rest-review harness checks that an edit changes
the asset; file size can increase or decrease.

## Verification

- `.local/native-costume-compact-retail-5/receipt.json`: **PASS** for all **130
  costumes**, **8,149 meshes**, **227 brush cases** and **123,904,853 checks**.
  This includes exact preservation of texture pixels and mip levels, palette
  entries, compressed animation streams, referenced vertex data and retained
  non-pointer bytes. Pointer aliases, symbols and stable object identities are
  checked. Compacting the result again produces identical bytes.
- The retail batch reduces its staged archives from **86,789,233** to
  **84,726,361 bytes** after rewriting all meshes to float-based skin data.
  These figures compare staged and compacted output, including the new live
  geometry; they are separate from the original quantized retail file sizes.
- `.local/native-costume-compact-stress-2/receipt.json`: **PASS** for **128
  consecutive Yoshi brush edits**. The original costume is 294,834 bytes. The
  maximum edited size is **328,710 bytes**, the final size is **328,638 bytes**,
  and every edit after the initial warmup stays within that observed bound.
- `native_costume_compact` CTest: **PASS** for reclaiming an oversized temporary
  transaction to its original 108-byte live archive, ordinary package-cap
  rejection, atomic failures and retaining oversized opaque input.
- `.local/native-costume-compact-full-build-1.log`: game and content tool build;
  DAT, skin-weight, posed-corner, compaction and rest tests pass. The rest suite
  covers **1,008 cases / 15,289,600 checks** across ordinary and morph geometry.
- `.local/native-compact-model-workflow-1/receipt.json`: **PASS** for posed and
  animated painting, rest edits, exact Undo/Redo, reopening without the GLB and
  Test Character.
- `.local/native-compact-stock-live-1/receipt.json`: **PASS** for the native
  Yoshi brush, saved strength and exact Undo/Redo. The 293 unpainted visible
  vertices retain their positions within 6e-8 normalized screen units.
- `.local/native-compact-morph-average-1/passed.json` and
  `.local/native-compact-morph-additive-1/passed.json`: **PASS** through native
  Edit/Pose/Animation, Save/Undo/Redo and practice. Both viewport comparisons
  have **zero changed pixels**.
- `.local/native-compact-skin-precision-1`: **PASS** for float weights,
  replacement transfer, sole-influence handling, sixteen influences and atomic
  rejection of a seventeenth influence.

Native clients used isolated profiles, hidden windows, muted audio and the
local-only chat endpoint. The test clients and build processes were stopped
after verification.

The retail runner is `review_native_stock_skin.py`. Its native fixture accepts
an optional stress count after the output path:

```text
native_stock_skin_edit_test.exe costume.dat joint_root roundtrip.dat 128
```

Additional Model work remains for morph/secondary-rigid painting, unusual
bindings, extra bones, source-aware reimport and larger authored models. Opaque
costume extensions require their data layout to be described before their
unused storage can be reclaimed.
