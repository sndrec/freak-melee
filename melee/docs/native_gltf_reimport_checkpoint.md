# glTF reimport checkpoint

Imports now record their source file and mesh identity in the editable project.
The matching backend can find those meshes in a new export even when its mesh
or material-slot arrays are reordered. The weight-preserving reimport operation
and its Workshop controls remain open.

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

## Remaining work

- Preserve current weights by verified source-corner correspondence when the
  source topology is unchanged. Keep source bind placement coherent with the
  edited rig.
- Require an explicit transfer/rebinding choice when correspondence changes.
- Expose material preservation/replacement independently of weight handling.
- Connect selected-mesh and whole-source reimport to the existing import flow,
  with matched destinations and clear failure messages.
- Preserve each mesh's import scale when a source has been imported with
  different scales, and save source links coherently with their geometry.
- Verify painted weights, edited/shared materials, new source geometry,
  Undo/Redo, save/reopen, and Test Character together in the native client.

The Model plan item stays unchecked until this complete workflow passes.
