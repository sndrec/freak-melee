# Large GLB/glTF meshes

The model importer accepts up to 1,000,000 source vertices and 1,000,000 triangle
corners per mesh. Rigid attachments, generated weights and compatible source
weights use the same geometry writer. Each draw section has at most 65,532
corners and, for skinning, at most ten distinct weight combinations.

Meshes above 65,536 vertices use compact position, normal and UV arrays in each
section. Saved corner IDs preserve vertices shared across sections and keep
separate coincident vertices distinct. The source vertex and triangle order
remains available for reimport with current painted weights. Smaller meshes
keep their shared attribute arrays.

The model reader follows the whole polygon chain and rejects a mesh above its
65,535-section capacity. It previously stopped at 4,096 sections and silently
omitted the remaining geometry from authoring operations. Rest compensation
and painting use the same section capacity.

## Runtime construction

Dense skins can contain thousands of polygon sections and many more envelope
references. Polygon construction now walks the chain iteratively. It preserves
forward object allocation, reverse class loading and an initialized tail for
each class callback. Reference resolution, animation and removal already walk
these chains iteratively.

The native HSD scene arena starts with 4 MiB committed in a stable 64 MiB
address reservation. During construction it can commit additional pages,
doubling capacity as needed. Existing objects keep their addresses. Binding
snapshot history freezes that capacity. Starting a new heap releases the
additional committed pages and returns to 4 MiB.

Ordinary, unrestricted object allocators reserve small groups of slots during
construction. This keeps dense skins from consuming one snapshot-directory
entry per envelope-list node. Explicit allocator limits and sealed match heaps
retain their existing allocation behavior. Native object reference counters
use 32 bits so one joint can serve more than 65,535 envelope references.

Renderer array uploads use four-byte alignment inside their shared storage
buffer. The buffer is bound at offset zero, and shaders use byte addresses
within it. Applying the device's separate-buffer alignment to each small array
wasted enough space to exhaust the 8 MiB frame buffer on the gradient fixture.
The array-upload cache also limits each lookup to sixteen probes, evicting a
cached entry when necessary. Eviction can repeat an upload while preserving
the recorded data for earlier draws.

The uniform upload budget is 32 MiB, increased from 24 MiB for dense skins
drawn across the scene's passes. This adds 8 MiB to the GPU buffer and each
existing staging slot. Buffers are allocated at renderer initialization;
drawing introduces no new allocation.

Snapshot copies include the initialized arena prefix and registered roots.
Unused committed space and the uncommitted address reservation are excluded.
Large models add actual runtime state and rendering work. This checkpoint does
not establish rollback performance for those models.

## Verification

`tools/review_native_large_gltf.py` creates rigid, generated-weight and
source-weight GLBs with 70,002 vertices and 70,005 corners. A gradient skin
requires more than 4,096 sections. Each fixture includes distant shared indices
and separate coincident vertices.

`tests/native/large_gltf_test.cpp` checks every position, normal, UV and shared
vertex ID. It paints a vertex shared by distant sections, saves and reopens,
reimports changed geometry with retained weights, and checks exact saved
weights. It also exercises weight transfer and atomic rejection of an invalid
source. The original draft is hashed before and after.

- `.local/native-large-gltf-2/receipt.json`: **12,390,828 checks** pass. Section
  counts are **2 rigid**, **6,261 generated**, **2 constant source skin**, and
  **7,779 gradient source skin**. The source draft remains unchanged.
- `.local/native-gltf-large-import-1/receipt.json`: the existing **409 checks**
  and saved import/reimport batch pass.
- `.local/native-source-skin-large-import-1/passed.json`: the existing source
  skin and KEEP cases, including malformed-source rejection, pass.
- `.local/native-large-gltf-existing-1.log`: rest compensation and rigid-skin
  suites pass.

`tests/native/polygon_chain_test.c` covers long base, refraction and derived
class chains, construction order, stable arena growth, object-pool packing,
snapshot restoration, reference counts and heap reset. Its current runtime
receipts are recorded alongside the live Workshop review below.

- `.local/native-large-gltf-runtime-final-tests-6.log`: heap, polygon-chain and
  animation-owner suites pass. The construction fixture uses **9,121,056
  initialized bytes** with **16 MiB committed**, then restores its snapshot
  and returns to the initial 4 MiB heap without moving the base.
- `.local/native-large-gltf-live-13/receipt.json`: the saved gradient skin
  opens in the actual Workshop. Its live surface reports all **70,002
  vertices** across **7,779 sections**. The full-UI PNG shows its colored
  lattice in Model/Skinning mode.
- `.local/native-large-gltf-live-14/receipt.json`: a hidden, muted client
  imports the 70,002-vertex rigid GLB through the normal three-step dialog,
  saves the draft and queries its live editable surface. The full-UI PNG shows
  the lattice alongside Mario in Model/Skinning mode.

Both live reviews use isolated copies, leave their source drafts unchanged and
complete without logged renderer errors. `.local/native-large-gltf-receipt.json`
collects the receipts and reviewed executable hashes. These checks establish
construction and editing readiness for the fixtures; their match performance
has not been measured.

## Remaining work

Morph painting and the unusual helper-node and independent texture-matrix
bindings listed in the Model plan remain open. Fixed renderer frame buffers
still limit total scene complexity; the source admission limit alone does not
establish that every accepted mesh fits a rendered match. The overall Model
and project acceptance items remain incomplete.
