# GLB morph import and named-target reimport

Verified: 2026-10-19. GLB/glTF model import now carries position and normal morph
targets into the same representation used by
[morph painting](native_morph_paint_checkpoint.md). Sparse accessors, omitted
zero-delta attributes, source placement and compatible source skins are covered.

The import keeps initial expression weights. A node's weights override its mesh
defaults; absent defaults become zero. Imported additive targets accept signed
weights. These rules follow the [glTF morph specification](https://github.com/KhronosGroup/glTF/blob/main/specification/2.0/Specification.adoc).
Position deltas receive the linear placement transform and import scale. Normal
deltas receive the corresponding normal transform and base-normal scale.

## Runtime properties and authoring

`SHAPESET_PROPERTIES` extends the native serialized descriptor to 40 bytes.
Byte 28 retains the optional skin-batch pointer, byte 32 references initial blend
values, and byte 36 references target names. Additive shapes use one initial
value per target; complete-shape interpolation uses one scalar. Names have one
entry per target. Both arrays are optional and checked during construction.
The importer traces them for compaction and the rest editor retains their links.

`SHAPESET_SIGNED_WEIGHTS` selects signed additive evaluation. Existing shapes
retain their original positive-only evaluation. Construction copies defaults
into the mutable blend storage; names remain in the resident asset graph.
Drawing and picking use the existing shared evaluator with fixed scratch
storage. There is no added tick or draw allocation.

`nativeSkinMesh_Encode` emits geometry without assuming an existing DObj. Fresh
imports own their descriptor/corner bytes; read edits retain their source DAT.
Both routes use the same target, normal-seam and private-batch writer. Callers
attach its result directly. The former mesh-writing API has been removed and
its paint/test call sites updated.

## Reimport

**Keep current painted weights** preserves the source vertex correspondence and
matches named morph targets to their existing slots. Source target reordering
keeps animation references stable; new targets append after the existing slots.
A missing named target rejects the transaction with its name in the error.
Unnamed target sets retain their ordinal slots and require the same count.
Source defaults are reordered with the target data.

This route handles additive morphs produced by GLB import. Complete-shape DAT
animation layouts require an explicit conversion policy and currently reject
this reimport route. Choosing a new binding still creates a new source layout.

## Evidence

`tools/review_native_gltf_morph.py` generates isolated GLBs and runs
`native_gltf_morph_test` against a real Mario draft. The final receipt is
`.local/native-gltf-morph-4/receipt.json`: **4,507 checks**, with source files
unchanged. Coverage includes:

- Rigid, generated and compatible source binding, with nonuniform placement,
  source bind-shape transforms, sparse deltas and missing zero-delta attributes.
- Actual HSD target evaluation compared with the source GLB at four weight
  settings, including negative and greater-than-one weights. Separate cases
  cover mesh defaults, zero defaults, unnamed targets and mirrored placement.
- Public painting, byte-exact core-asset Undo/Redo, rest rotation after painting,
  matching world corners before/after that edit, and saved-project reopening.
- Named target reordering and an added target, retained painted weights, and
  unchanged assets on missing targets, bad counts, nonfinite data and unsupported
  tangent targets.
- Mutable-arena save/load restoring blend values, target-name pointers and
  private batch links in actual constructed HSD objects.

`.local/native-gltf-morph-regressions-2.log` records six passing CTests: DAT,
heap, rest edits, Model posed vertices, morph runtime/painting and rigid skin.
The existing source-aware reimport review also passes **409 native checks** and
its saved-project cases:
`.local/native-gltf-morph-source-regression-1/receipt.json`.

The hidden, muted native Workshop review is
`.local/native-gltf-morph-live-1/receipt.json`. The mouse brush changes the chosen
weight from **0.17 to 0.37**, Save/Undo/Redo restore exact costume bytes, and
the other three vertices have **zero** screen displacement. Reimporting the
source through **Keep current** retains all weights; maximum screen displacement
is **5.9e-8** viewport units. Full-UI PNGs and tested binary hashes are in that
receipt. Later changes add per-primitive count guards and the snapshot checks;
the UI review covers the same valid import/paint/reimport path.

Runtime/tool build: `.local/build-native-gltf-morph-4.log`. The combined receipt
is `.local/native-gltf-morph-receipt.json`.

## Remaining Model work

Morphs still use the existing 2,000-position / 2,000-normal-vector scratch
capacities. Tangent, UV and color target channels need additional representation
work; the importer rejects these channels with an error. Mesh import does not
import glTF animation clips. Unusual helper-node/texture-matrix domains and
broader user workflow acceptance remain in the project plan.
