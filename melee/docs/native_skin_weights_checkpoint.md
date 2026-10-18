# Floating-point skin weights — 2026-10-17

Imported meshes now support up to 16 bone influences per vertex. Adding weight
to one bone reduces the others proportionally. Subtracting redistributes that
weight among the remaining bones. A vertex with one influence remains fully
bound. Weights retain floating-point precision when saved or transferred to a
replacement mesh. Automatic binding still starts with the two nearest bones.

The editor reports a full vertex instead of dropping an existing influence.
A failed stroke leaves the costume unchanged. The brush also requires a deform
bone with a valid inverse bind matrix. These operations run while editing
assets; they add no per-tick allocation or new renderer state.

## Implementation

`native_fighter_skin_weights.c` owns bounded normalization and weight editing.
It merges duplicate bones, removes zero influences and sorts the result for
stable comparisons. The glTF/DAT writer, its saved-mesh reader, brush and nearest
replacement transfer use the same full influence lists. The former two-bone
structure and 1/32-step quantization were removed. The native tool prints every
influence at floating-point precision.

HSD selects its rigid envelope shortcut from the first weight. At or near 1,
that path can skip the inverse bind needed by an imported skeleton-root mesh.
The writer therefore puts the smallest influence first and stores a sole
influence as two half-weight entries on the same bone. Reading merges those
entries into one logical influence. This preserves the original renderer.

## Verification

Both native executables built successfully. The `native_skin_weights` CTest
passed 25,064 checks covering normalization, duplicate/zero bones, invalid data,
floating precision, proportional edits, full-capacity rejection and extreme
floating-point inputs.

`review_native_skin_weights.py` copied an existing Mario draft into an isolated
directory and generated an asymmetric four-vertex GLB. Its independent raw DAT
reader checked the saved envelope records and geometry after each native tool
operation. It verified:

- Additional influences retained the previous bones and their proportions.
- Values smaller than 1/32 survived saving; positions and normals stayed exact.
- Replacement using transferred weights retained all four influences in that case.
- Near-1 and single-bone weights used the inverse-bind blending path.
- Sixteen influences were saved; a seventeenth was rejected with all project
  file hashes unchanged.
- The source/asset check passed after the input GLB was deleted.

The native UI pass used a hidden, muted client and a separate profile. It opened
the uninstalled weighted draft, displayed the Skinning heatmap, switched through
Edit, Pose and Animation modes, entered Test Character with Sandbag, and returned
to the editor. Reviewed PNGs show the asymmetric mesh's weight colors and live
practice. The client was stopped afterward.

Evidence: `.local/native-skin-weights-live-2/passed.json`, `tool.log`, and its
`preview/screenshots` directory. Build evidence is in
`.local/native-skin-weights-build-envelope.log`. The first asset harness selected
the skeleton-root flag instead of the deform-bone flag; its candidate selection
was corrected. The first live fixture sat inside Mario's body, so the final
fixture places the surface beside him for clear visual inspection.

## Remaining work

Compatible source JOINTS/WEIGHTS now import through the
[source-skin path](native_source_skin_checkpoint.md), and
[rest compensation](native_model_rest_checkpoint.md) covers rigid, skinned and
morph geometry. Imported skins also support posed and animated brush targeting.
The [Model workflow checkpoint](native_model_workflow_checkpoint.md) records its
native editing checks and the unresolved projection discrepancy after refresh.

Arbitrary stock DAT layouts, continuous triangle-surface brush hits, additional
bones and DAT compaction remain open. User usability acceptance and the broader
animation-authoring gates remain separate from these saved-weight checks.
