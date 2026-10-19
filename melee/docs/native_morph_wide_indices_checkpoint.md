# 32-bit morph target indices

Morph targets now use 32-bit indices into their stored position and normal data.
A controller can hold several full-sized expressions whose combined data exceeds
65,536 entries. Its logical position and normal counts remain limited to 65,536.
Each NBT normal entry still holds three lighting vectors.

## Representation

`SHAPESET_INDEX32` selects four-byte, big-endian entries in both target index
tables. Target descriptors retain `GX_INDEX16`; the shape flag specifies the
target-table width. Parent and private draw streams continue using their existing
logical indices. Their topology, matrix palettes and single animation controller
retain their existing ownership.

The asset loader checks table widths and bounds before admitting a shape. It
traces the complete physical target arrays for compaction. The shared evaluator
reads the selected width once per attribute and uses the same scalar decoding
and blending. Stock eight- and sixteen-bit target tables keep their formats.
All callers supply the actual index width directly.

The painting writer emits wide tables. Rest compensation also emits wide tables,
retaining source sharing with a physical-index map. Additive base positions and
offsets retain separate entries when translation makes them different. Names,
initial weights, private batches and controller references stay attached to the
same shape.

This changes target storage, not scratch allocation. Drawing and Model picking
keep their existing shared 3.75 MiB scratch, and the runtime controller has no new
fields. Construction and authoring allocate the source tables; evaluation adds
no allocation. The rebuilt global `.mstate` section remains **1,085,229 bytes**,
matching the recorded capacity checkpoint. Actual model objects still use their
normal construction storage.

## Verification

- `.local/native-morph-wide-shape-1.log`: **438 cases, 5,830,520 checks**.
  Complete/additive models use all 65,536 logical entries and 196,608 physical
  entries per channel. XYZ and NBT cases exercise encoding, construction, readback,
  actual posed corners, rest compensation and compaction. Out-of-range 32-bit
  indices and invalid descriptor widths reject before runtime construction.
  The existing controller, painting, pose and snapshot cases also pass.
- `.local/native-gltf-morph-wide-1/receipt.json`: **32,530,120 checks**, source
  draft unchanged. The larger GLBs now use **36,000 vertices**, crossing the old
  physical index limit. Rigid, generated and compatible source bindings pass
  painting, rest edits, history, save/reopen, snapshot restoration and named
  target reimport. The 17 combined attachment/rest cases also pass.
- `.local/native-morph-wide-regressions-1.log`: independent rest-edit and posed
  Model-vertex suites pass.
- `.local/native-morph-wide-sources-1/receipt.json`: existing source reimport
  passes **409 checks** and its saved-project cases.
- `.local/native-morph-wide-live-1/receipt.json`: a hidden, muted native Workshop
  draws the saved 36,000-vertex expression mesh and queries all vertices. Its
  full-UI PNG and tested binary hashes are recorded; the client is stopped.
- `.local/native-morph-wide-sections.json`: current snapshot section size and
  the prior recorded capacity-checkpoint size match.

Build: `.local/build-native-morph-wide-1.log`. Combined receipt and source pins:
`.local/native-morph-wide-receipt.json`.

## Remaining limits

The per-controller logical limit remains 65,536 position/normal entries. Total
target data remains subject to costume/package storage budgets. Tangent-only,
UV and color GLB target channels, unusual helper/independent texture-matrix
domains and broader user workflow acceptance remain open.
