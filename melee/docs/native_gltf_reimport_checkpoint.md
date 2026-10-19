# glTF reimport checkpoint

Imports now record their source file and mesh identity in the editable project.
The matching backend can find those meshes in a new export even when its mesh
or material-slot arrays are reordered. The backend can preserve painted weights
and edited materials while replacing geometry. Source links save with the
matching assets. The Model workspace exposes selected-mesh and whole-source
reimport through the existing three-step import dialog.

## Source identity

Each imported mesh records its costume DObj offset, canonical source path,
source identity, vertex/corner counts, ordered-topology hash, scale, binding
mode, winding and source transform. DObj offsets retain the connection through
costume compaction and editor renaming.

A named mesh with one primitive uses its mesh name. A mesh with multiple
primitives also uses each primitive's material-slot name. An unnamed mesh can
use its node name. Names must identify one source primitive uniquely. An
unnamed or ambiguous primitive can still be imported explicitly; it receives
no automatic source link. Replacing an existing mesh with such a primitive
clears its previous link.

`nativeFighterGltf_PlanReimport` matches either one selected mesh or all imported
meshes from its source document. The caller can supply a relocated source file.
Missing or ambiguous identities reject the whole plan before publishing any
matches. Additional source meshes remain available for explicit import.

Source records live in `NativeFighterAuthoring` and tagged `mesh-source` rows in
`workshop_names.txt`. This keeps them in the existing editor metadata owner and
Model history. The package builder supplies gameplay assets, scripts,
parameters, resources and CPU data from each member; it excludes authoring
metadata and local source paths.

## Import and history fixes

Batch replacement resolves every destination DObj before conversion, then finds
that same DObj in each intermediate candidate. A mesh moving to another joint
can change traversal ordinals without redirecting later replacement requests.
Each destination can be replaced once per batch. Source metadata changes join
the existing private asset candidate and publish with a successful batch.

Model history now records whether an asset entry was actually created.
Changing only a source link creates its own metadata entry. Repeating a no-op
import leaves history unchanged. This also preserves fighter-only asset edits
when costume bytes happen to be identical.

## Verification

`native_gltf_reimport_test` passes **43 checks** covering replacement identity
through ordinal changes, failed later imports preserving pointers and bytes,
duplicate destinations, metadata serialization, percent escaping, long keys,
subnormal transform values and malformed-row rejection.

`tools/review_native_gltf_sources.py` additionally verifies saved-project
matching after mesh/primitive reordering, relocated files, source material
edits and editor renaming. Missing and duplicate identities fail without file
changes. A selected-mesh plan can still succeed when an unrelated source mesh
is missing. Whole-source matching stays within its original document.

Receipt: `.local/native-gltf-sources-2/receipt.json`.
Tool SHA-256: `490dbdb85761ff976d22f5d28b71f76de869f100cfdf7d3a01d71e10aba6edfa`.
Test SHA-256: `f3dd3b6ed9454e7b72e9075b78278c306fd362585c8ef85d7ac648a746b6d1f1`.

`tools/review_native_gltf_sources_ui.py` drives the real hidden, muted Workshop.
It imports identical geometry from a relocated file, verifies byte-identical
core assets with a changed source link, checks source-only Undo/Redo, repeats
the same import to check no-op history, and reopens the saved project.

Receipt: `.local/native-gltf-source-ui-final/receipt.json`.
Game SHA-256: `43361eb1e6a8f4ead606d572fa038d73ace1c156337c436f8d319a7d0cf92593`.

The native extra-bone workflow also passes with source metadata present:
`.local/native-rig-source-metadata-final/receipt.json`. It covers structural
history, pose selection, painting, new animation channels, reopen and practice.
The UI review helpers wait for the Save completion response before reading
files. This avoids reading an earlier saved state while a slower save is still
staging its replacement files.

Both reviews copy their input drafts and verify the originals' four-file
hashes. Import parsing, metadata and matching work happens at authoring
transitions and adds no match-tick work.

## Native interface

An imported mesh's inspector shows its source filename and **Reimport source…**.
The full local path is available in the filename's tooltip. The dialog offers
selected-mesh and whole-source scope, shows named matches, retains a scale for
each destination, and separates material choice from binding choice. Missing
or ambiguous matches keep the operation on the source step with an error.

Reimport defaults to current painted weights and edited materials. A manual
replacement of a source-linked mesh also defaults to current weights. New
meshes use compatible source skin or generated weights. Changed skinned topology
requires explicit transfer or a new binding. Reimport keeps each destination's
attachment bone. Matching uses the source file's mesh and material-slot names;
editor display names are independent of them.

The implementation lives in `native_shell_workshop_model_import.cpp`; the main
Model file directly calls its dialog renderer and input handler. Both files are
under 2,000 lines.

## Weight and material preservation backend

`NATIVE_GLTF_BIND_KEEP` copies current floating-point weights through the saved
source-corner correspondence. It retains all 16 influences without renormalizing
them. The source's ordered topology must match; changed topology requires an
explicit transfer or new binding. Conflicting corner weights also require that
choice. Rigid attachments keep their attachment. Source-skinned geometry keeps
its recorded bind placement, while ordinary node geometry uses the new export's
placement. Each request carries its own scale.

Material replacement is a separate request option. Keeping materials retains
the current MObj. Replacing a material creates a new MObj for that destination;
other meshes using the old shared material retain it. Material-animation remap
retains an old track only when the destination still uses the same MObj.

`native_content_tool project-reimport` exposes selected/whole-source matching,
binding choice and material choice for saved-project checks. The normal Workshop
import caller uses the revised batch API. The native interface described above
now exposes those backend choices.

Verification:

- `.local/native-gltf-preserve-final-2/receipt.json`: 409 native checks and saved
  project cases pass. Coverage includes 16 authored influences, mirrored corner
  order, a change in winding, different import scales, edited/shared materials,
  independent material replacement and atomic rejection of changed topology.
- `.local/native-source-skin-preserve-1/passed.json`: nine source-skin cases pass
  weight-preserving replacement, including sparse/interleaved accessors, integer
  weights, 16 influences, source poses and unit conversion; 16 rejection cases
  retain the original saved assets.
- `.local/native-model-gltf-preserve-regression/receipt.json`: the existing native
  Model workflow passes painting in pose/animation preview, rest compensation,
  Save/Undo/Redo, reopen without the GLB, Test Character and return. This is a
  regression of those workflows, not acceptance of the future Reimport controls.

Game SHA-256: `517615b657e7294e52fb7f089b1813996c19430eabf7ca85b57edf5fc979135d`.
Tool SHA-256: `1f91b79150a415d87a72c0f41b40495d68953ad6f000ff9260a0fc354da15773`.
Test SHA-256: `7032fbc1a57ac74605a5c9898ee9e561e05803e69be993bc2a2ee368c0aa48ee`.

## Coordinated editor metadata saving

Workshop Save now stages `workshop_names.txt` together with the four core
assets and all five predecessor files. Failed replacements restore the matching
geometry, rig facts, names and source links. `project-assets-undo` restores that
complete set. A source-only change participates in the same transaction, while
a no-change save retains the existing predecessor. An absent metadata file is
represented by an empty predecessor.

Names, hidden-action choices, clip labels and import metadata now stay in memory
until Save. Model's dirty check includes metadata, so a source-only edit counts
as an unsaved change. The name buttons say **Apply name**. Undo/Redo changes the
working draft; Save publishes it. Command-line imports and label operations use
the coordinated save directly.

Metadata encoding produces a complete private byte buffer. Decoding publishes
only after every row is valid. Bad later rows, embedded zero bytes, oversized
rows and invalid source records leave the existing metadata owner unchanged.
The former independent metadata-file writer has been removed.

Current backend verification:

- `.local/native-project-assets-metadata-1/receipt.json`: **679 checks** pass,
  including all ten locked current/predecessor destinations, source-only edits,
  empty predecessors, malformed metadata and complete save Undo/Redo.
- `.local/native-gltf-coordinated-save-1/receipt.json`: **409 native checks** and
  saved reimport cases pass through the coordinated save path.
- `.local/native-source-skin-coordinated-save-1/passed.json`: all nine source-skin
  import/reimport cases and 16 atomic rejection cases pass.
- `.local/native-model-coordinated-metadata-1/receipt.json`: native Model painting,
  animation preview, rest compensation, Save/Undo/Redo, reopen and practice pass.
- `.local/native-gltf-coordinated-save-ui-2/receipt.json`: source-only import
  changes stay unsaved until Save, Undo/Redo and reopen pass, and repeated import
  completes without adding history. The review checks the selected dropdown
  value and requires the import dialog to close after success. This replaces
  the earlier no-op check, which could mistake a failed import for a no-op.

Game SHA-256: `6babab80370a08af53e949b3efcb57c5b823a74d7525751fecaaf442edc5ff78`.
Tool SHA-256: `ae532c497e725a2c68b996afe094b5dd64f68fbda52793d859836134e197a7b4`.
Save-test SHA-256: `1518e86ac8af6a7c3af2d6c37c70ba08c5d34972e8cc518569d5c4080e69b146`.

## Combined native reimport acceptance

`tools/review_native_gltf_reimport_ui.py` prepares two imported meshes at
different scales, paints a vertex, and gives both meshes an edited shared
material. It drives the real hidden, muted Workshop and inspects the saved DAT:

- Missing whole-source matches fail before changing the draft.
- Whole-source reimport replaces geometry while retaining the painted float
  weights, both scales and the edited shared material.
- Undo/Redo and Save restore the exact five-file states.
- Selected-mesh material replacement retains weights and leaves the other
  shared-material user unchanged.
- Changed topology rejects current-weight preservation. Explicit transfer
  succeeds through the same dialog.
- Reopen without any source GLB retains the saved assets. Test Character and
  return to editing succeed.

Receipts `.local/native-gltf-reimport-ui-1/receipt.json` and
`.local/native-gltf-reimport-ui-2/receipt.json` pass. The second records the
Placement and Binding dialogs for visual review. Ordinary import, source-only
Undo/Redo, successful no-op import and reopen also pass after the code move:
`.local/native-gltf-import-regression-ui-1/receipt.json`.

These checks use copied drafts and verify that the source draft stays unchanged.
Current-weight correspondence uses source vertex/index order; it requires
explicit transfer when that order changes. Source skins still require a
compatible fighter rig. Reimport adds no match-tick work.

Final reviewed executable:

- `.local/native-gltf-reimport-ui-final/receipt.json`: complete workflow passes,
  with Placement/Binding screenshots and a visible practice frame.
- `.local/native-gltf-import-regression-ui-final/receipt.json`: ordinary Import
  starts at Add, source-only Undo/Redo passes, and repeated no-op import completes
  successfully. Ordinary Import initializes its destination to Add each time.
- Final backend batch: `.local/native-project-assets-reimport-final/receipt.json`,
  `.local/native-gltf-backend-reimport-final/receipt.json` and
  `.local/native-source-skin-reimport-final/passed.json` pass the 679 save checks,
  409 native reimport checks, saved reimport cases, nine source-skin cases and
  16 atomic rejection cases on the final build outputs.

Game SHA-256: `b07c2c2fd3a8886e4c9c2fc71cd186bf284aeb079556ea1172713d97ea8476f9`.
Tool SHA-256: `3506647e73e62c1974c0223c57e56723d02a808a4ae246765a76a085cfb62e59`.
