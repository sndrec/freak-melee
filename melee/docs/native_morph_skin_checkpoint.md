# Weighted morph runtime and rest edits

Verified: 2026-10-19. The runtime now draws and queries meshes that combine
morph targets with bone weights. Weight painting and GLB morph import still
need the authoring reader and writer described in the
[implementation design](native_morph_skin_design.md).

The later [painting checkpoint](native_morph_paint_checkpoint.md) adds the
authoring reader/writer and native brush/Undo/Redo coverage. The results below
describe the initial runtime checkpoint.

## Representation and lifetime

`SHAPESET_SKIN` adds a private envelope-batch pointer at byte 28 of the serialized
shape descriptor. The parent polygon keeps its controller, blend values and
animation index. Private batches contain triangles in the parent's corner order
and use its evaluated position/normal indices. Each batch has at most ten weight
combinations, with up to sixteen influences each. Full single-bone bindings use
two half influences so every native batch applies inverse binds.

Drawing evaluates the shape once into the existing fixed scratch arrays. Each
batch then uses HSD's envelope matrices and the existing GPU skinning path.
Editor corner queries, picking and highlighting evaluate those same targets
through the same bindings. Construction validates the entire graph and its
capacities; drawing introduces no allocation. The existing limits remain 2,000
positions and 2,000 normal vectors, with three vectors per NBT normal entry.

The DAT decoder tracks the extended descriptor and private graph for compaction.
The polygon owns the batches and resolves their joint references during the
normal reference-resolution phase. It releases them with the parent controller.
Private batches receive no independent shape animation objects. The
drawing-disabled setup path prepares their envelope-bone dependencies.

Rest edits preserve the blended world-space skin matrices by updating inverse
binds. The remaining owner-domain change is applied to every complete target,
additive offset and normal target. Root, deform-bone and helper ownership are
covered by the same model-node calculation used for ordinary envelope meshes.

## Evidence

`native_shape_skin_test` constructs actual DAT graphs and HSD runtime objects.
It passes **234 cases and 85,924 checks**:

- Complete and additive targets with absent, XYZ and NBT normals, two private
  batches and shared logical vertices, across nine poses and six blend settings.
- Compaction, runtime construction, one-controller ownership, drawing-disabled
  setup and destruction. The mutable-arena save/load restores blend values,
  controller time, envelope weights and private-chain links.
- Rest translation, rotation and scale on all four fixture joints, comparing
  posed corners and transformed lighting vectors at six blend settings.
- Seventeen invalid graphs, including cycles, mismatched corners/attributes,
  conflicting position/normal bindings, invalid matrix slots, weights, missing
  inverse binds and excess palettes. Invalid input remains unchanged.

`.local/native-shape-skin-tests-2.log` records all five passing CTests:
DAT, heap, rest edits, posed Model vertices and weighted morphs. The existing
rest suite still passes **24,247,840 checks**. Build receipt:
`.local/build-native-shape-skin-6.log`.

`tools/review_native_morph_view.py --weighted` creates isolated copies of a real
Mario draft with complete/additive morph quads. Four distinct vertex bindings
and a bone rotated away from its inverse bind visibly deform each quad. Hidden,
muted clients draw the fixture, hover its outliner entry and pick its actual
surface. For both modes, the **38,379** highlight pixels overlap **38,379** of
**38,402** white surface pixels. Source files remain unchanged. The reviewed PNGs,
executable hash and counts are in `.local/native-morph-skin-view-3/receipt.json`.

The live review uses each mode's default blend. Other blends, normal formats
and owner domains are covered by the geometry suite. This checkpoint establishes
the runtime and rest-edit representation; morph painting, repeated authoring
transactions, target-aware reimport and GLB morph-target encoding remain open.
