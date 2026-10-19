# Source reimport after rest edits

Rest edits now retain the coordinate correction for each imported mesh. Choosing
**Keep current painted weights** applies that correction when loading the source
again. Rotation, scale and position edits can be combined before reimport.

Previously, an attachment could stay in place after a rest edit and then move
when the same source was reimported. The saved geometry had been compensated,
but its source placement did not retain that correction. The failing native
probe is `.local/native-gltf-attachment-probe-3/receipt.json`: an unchanged
source moved a sampled coordinate from -1.2375 to -2.638069.

## Ownership and application

The correction belongs to the existing project-owned source record. It is
separate from the source node transform and import scale. Successive rest edits
compose their corrections. Reimport applies the result after source placement
and scale, including base positions and additive offsets. Normals use its
inverse transpose after their source normalization; normal target offsets use
the same source-normal scale as their base.

An optional `mesh-rest` row in `workshop_names.txt` stores the affine matrix for
its source mesh. Source rows with no correction have the identity behavior.
Parsing checks the complete matrix and its referenced source before publishing.
Invalid or singular matrices leave authoring data unchanged. This metadata is
excluded from fighter packages with the existing local authoring facts.

The geometry candidate and updated source facts publish together after
validation. Model history captures both. A first paint stroke on a rigid root
attachment also records the conversion into bind coordinates, so later source
reimport uses the painted mesh's basis. These operations run during authoring;
they add no match tick or drawing allocation.

## Verification

The GLB review now checks actual HSD world corners and normal directions.
Attachment cases combine translation, rotation and nonuniform scale, rigid and
morph geometry, painting, repeated reimport, save/reopen and exact history
restoration. Root, deform-bone and helper attachments are included. The root
cases cover both implicit and explicit inverse binds and painting before or
after rest edits. A changed import scale is compared against the same fighter
before its rest edits. The original draft is hashed before and after. Malformed
correction rows reject atomically.

The native hidden/muted review in `.local/native-gltf-rest-live-2/receipt.json`
edits a painted morph on Mario's TransN attachment. Full costume and source-fact
Undo/Redo are byte exact. The rest edit has **zero** screen displacement;
reimport has **6e-8** viewport-unit displacement. Full-UI PNGs and tested hashes
are recorded. The review client is stopped. Its first attempt selected a stock
mesh; that failed review is retained separately and was not counted as evidence.

The existing source-reimport batch passes its **409 checks** and saved-project
cases in `.local/native-gltf-rest-sources-2/receipt.json`. Rest and weighted-shape
CTests pass in `.local/native-gltf-rest-ctest-2.log`. The final weighted-shape
executable records **434 cases and 3,471,072 checks** in
`.local/native-gltf-rest-shapes-1.log`.

The final GLB batch is `.local/native-gltf-rest-3/receipt.json`:
**10,858,120 checks**, including all **17 attachment cases** and the existing
12,000-vertex import workflows. The source draft stays unchanged.

The final native root review is `.local/native-gltf-rest-root-live-2/receipt.json`.
A mouse stroke sets one vertex's TransN weight to **0.200000003**. Both costume
and source-fact Undo/Redo are byte exact. Five unpainted vertices have **zero**
screen displacement. Reimport preserves weights and has **6e-8** viewport-unit
displacement. Its six temporary corner vertices become the source's four
shared vertices; weighted geometry is checked in both directions. The earlier
review compared temporary vertex numbers and failed on that representation
change. The review now checks the authored weights and geometry instead.

The TransN rest-edit UI review predates the additional root-paint path and its
explicit allocation ownership fix. Its receipt records that tested binary.
The root mouse review and final backend batch use the final client/tool build:
`.local/build-native-gltf-rest-6.log`. Full-UI PNGs, binary/source hashes and
stopped-client checks are collected in `.local/native-gltf-rest-receipt.json`.

## Remaining domains

Imported meshes whose parts need different rest-coordinate corrections require
an explicit per-part source representation. Such edits reject as a whole.
Unusual animated helper/secondary-joint and independent texture-matrix bindings
remain open. Larger physical morph indices and additional target channels retain
their existing limits. User workflow acceptance remains in the project plan.
