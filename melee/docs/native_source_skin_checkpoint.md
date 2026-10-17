# Compatible source skin import — 2026-10-17

The Model import wizard offers **Keep source weights (compatible rig)**. It is
the default when every selected mesh has a skin. This imports the mesh's existing
bone weights onto the fighter's rig. Generated weights, rigid attachment and
transfer from a replaced mesh remain available in the same Binding dropdown.

Every source skin bone needs a unique matching fighter bone name or canonical
bone role, such as HipN. Mapped parent relationships must agree. Weighted bones
must also have compatible relative bind poses. The importer accepts a shared
placement transform and applies the selected unit scale to both geometry and
bind translations. A differing rig, ambiguous name or unsupported influence
count produces an error before the project is changed.

Weights retain floating-point precision and support up to sixteen influences
per vertex. Duplicate influences are combined and positive weights normalized.
Additional JOINTS/WEIGHTS sets, normalized unsigned byte/short weights, sparse
accessors and interleaved vertex data are supported. Imported mesh and material
data are saved in the project; gameplay does not need the source GLB.

## Binding and placement

The source inverse-bind matrices define the mesh's binding. The importer checks
that each weighted bone produces the same mesh-to-fighter bind transform from
the source inverse bind and target bind. Current source bone poses can differ
from their bind poses. The ordinary skinned mesh node transform is ignored, as
specified by [Khronos's glTF skinning documentation](https://github.khronos.org/glTF-Tutorials/gltfTutorial/gltfTutorial_020_Skins.html).
Other binding modes continue to use the mesh node placement.

The fighter keeps its existing skeleton and gameplay bone roles. This feature
supports meshes authored for a compatible rig. Importing arbitrary armatures,
adding bones and retargeting different body proportions remain separate work.

`native_fighter_gltf_skin.cpp` reads and validates source binding. The existing
HSD geometry writer saves the resulting weights. Single-bone envelopes retain
the inverse-bind rendering behavior documented in the
[floating-weight checkpoint](native_skin_weights_checkpoint.md). Import uses
the existing staged asset transaction, so failure in a later mesh also discards
earlier meshes from the same import.

## Verification

Both native executables built. The `native_skin_weights` CTest passed, and the
existing paint/transfer/precision/capacity asset checks passed again.

`review_native_source_skin.py` builds GLBs from an isolated Mario draft's bind
data. Its independent raw-DAT reader verifies saved positions and every envelope
weight. Nine valid cases cover floating weights, normalized byte/short weights,
interleaved and sparse accessors, duplicate influences, sixteen influences,
unit conversion and an already posed source skeleton. The fixture reverses the
skin joint table and gives the mesh an unrelated node transform to check both
index mapping and placement.

Sixteen invalid cases cover missing/ambiguous names, hierarchy and bind mismatch,
singular/perspective binds, invalid indices and weights, missing attribute pairs,
missing skin, seventeen influences, incorrect units and a failing second mesh.
Every rejected import leaves all project file hashes unchanged.

The hidden, muted native UI check loads a compatible GLB through all three
wizard steps, verifies the default source binding, imports and saves it, and
checks the saved weights independently. Reviewed screenshots show the surface
in the Skinning heatmap and in live practice. The check switches Pose/Animation
modes, enters Test Character and returns to the editor. The test client exits.

Evidence: `.local/native-source-skin-live-3/passed.json`, `tool.log` and
`preview/screenshots`. The paint regression is in
`.local/native-skin-weight-source-regression/passed.json`; build output is in
`.local/native-source-skin-build.log`.

The first harness decoded UTF-8 bone labels using the Windows default encoding;
it now uses UTF-8 explicitly. A later UI harness waited for a status message only
set on initial editor entry. It now verifies a frozen action pose after refresh.
These were harness corrections; the source imports passed without code repairs.
