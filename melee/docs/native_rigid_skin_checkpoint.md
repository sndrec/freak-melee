# Painting rigid attachments

The weight brush now accepts rigid imported meshes and HSD polygons with a
second rigid joint. It converts their matrix slots into editable bone weights
while preserving their placement. Painting, weight-preserving reimport and
weight transfer keep the existing mesh owner.

## Coordinate domains

HSD's rigid slots use the owning joint and an optional second joint directly.
An envelope on a skeleton root uses bind coordinates. An envelope on a deform
bone also applies that owner's model-node matrix. The reader converts each
rigid slot's positions and normals into the corresponding envelope domain.

For a deform-bone owner, the secondary slot uses
`owner_inverse_bind * inverse(secondary_inverse_bind)`. Normals receive its
inverse transpose. A root owner uses the referenced bone's bind matrix.
Helper owners can retain their own rigid slots through their nearest skeleton
bone. A distinct second joint under an animated helper remains unsupported.

`nativeSkinMesh_EnsureInverseBind` supplies the identity matrix that an unbound
skeleton root used implicitly. Both the brush writer and glTF envelope writer
use it. A local reference to offset zero remains a local reference, including
when an envelope references the root joint at data offset zero.

Reimport and transfer retain the selected mesh's coordinate domain. Their
explicit weights use the existing attachment; automatic weight generation
continues to use the compatible skeleton root. Source geometry for an attached
mesh should keep the same attachment-relative placement when re-exported.

All conversion and allocation happens during authoring operations. The match
tick and renderer retain their existing paths.

## Verification

- `native_rigid_skin_test`: **52 cases, 114,148 checks**, including root and
  deform-bone owners, self/secondary joint slots, helper self-attachments,
  missing root binds, offset-zero joint references, indexed XYZ/NBT normals,
  eight/sixteen-bit indices and rejection without changing the project.
  Output: `.local/native-rigid-skin-unit-3.log`.
- Each case compares the original and rewritten mesh through the actual HSD
  corner-position query at **19 poses**. Maximum position error is
  **0.00000143051 Melee units**. A separate normal-matrix comparison has a
  maximum component error of **0.000000238419**.
- Every supported fixture also goes through the actual brush, costume
  compaction/import validation and saved weight readback.
- `.local/native-rigid-skin-ctest-1.log`: rigid-skin and the existing rest-edit
  suite pass.
- `.local/native-stock-rigid-skin-1/receipt.json`: all **130 costumes**, **8,149
  meshes**, **227 brush cases** and **123,904,853 checks** pass. This also checks
  material/mesh ownership, attributes, compaction payloads and importer admission.
- `.local/native-gltf-rigid-skin-2/receipt.json`: the **409 native checks** and
  saved-project reimport batch pass. New cases paint root/bone-attached rigid
  imports, retain their weights through reimport, transfer them across changed
  topology and verify that their attachment owner stays unchanged.
- `.local/native-source-skin-rigid-1/passed.json`: nine source-skin import and
  weight-preserving reimport cases, plus 16 rejection cases, pass.
- `.local/native-rigid-skin-live-2/receipt.json`: the fixture imports a four-vertex
  attachment on Mario's TransN joint, then opens it in a hidden, muted native
  client. A mouse stroke sets
  one vertex's XRotN influence to **0.200000003**. Save/Undo/Redo restores exact
  costume bytes. The three unpainted vertices have **zero normalized screen
  displacement**. Reimport retains all weights and has **zero screen
  displacement**. These positions come from runtime HSD projection; the PNGs
  record the rendered heatmap. The original draft's 13 files stay unchanged.

The live review uses the Skinning mesh dropdown, so it can select attachments
below the visible outliner rows. It waits for Save to finish before reading the
saved bytes. Its optional `--reimport FILE` checks the combined workflow.

The first unit fixture used a single-object importer for an attribute list.
The fixture now imports the full PObj graph, including its terminated list.
The first live review's Reimport button lay below its small scroll viewport;
the completed review uses a 1440×900 test window. Neither result was treated
as a passing application check.

Game SHA-256: `1e5e0da905de323a7eb09cc49a4a3fdabe82b3123414339d8b2930ea2766cade`.
Tool SHA-256: `ec79218eb834770c40cfdf1e04706cf812a79a35ec2e1e6b870b69edeffae461`.
Rigid-test SHA-256: `edbfaf557dcf5644a2f604e0af58ae15986b4a6385fc61df5d332354ab9cadbb`.

## Remaining painting work

Morph targets, independently bound texture matrices, helper attachments whose
model-node transform cannot be represented by the existing envelopes, and
larger meshes remain open. The overall Model painting/storage plan item stays
unchecked.
