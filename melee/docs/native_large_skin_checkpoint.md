# Painting across draw sections

The native weight writer can rebuild meshes with more than 65,536 editable
vertices. Each draw section stores its own position array and 16-bit indices.
The writer also saves a vertex ID for each display-list corner. Copies of one
vertex in different sections keep the same ID. Separate vertices at identical
positions keep different IDs.

The brush reader combines matching IDs, positions and weights within a mesh.
This preserves shared vertices across section boundaries while retaining
intentional differences in position or binding. Normals, UVs and other corner
attributes keep their existing independent values.

The IDs are stored in the costume DAT under `freak_vertex_ids`, so they travel
with the fighter package. The renderer continues to consume ordinary HSD/GX
geometry. ID parsing and allocation take place in authoring transactions.

Rest compensation copies IDs when it replaces a polygon or expands indexed
positions into direct floats. Compaction rebuilds the ID table using the
remaining polygons and their relocated addresses. It removes retired ID arrays.
Meshes that become small enough can return to a shared position array and drop
the ID rows.

## Verification

`tests/native/rigid_skin_test.cpp` includes two 70,002-vertex fixtures, each with
70,005 corners. One attaches to the root; the other attaches to a helper node.
They cover a triangle spanning distant indices, shared vertices across draw
sections, and separate coincident vertices. The fixtures check every vertex's
position and weights after encoding and compaction, then exercise repeated
painting. The helper fixture also goes through rest compensation.

Additional checks cover reordered archive roots, retiring a section, rewriting
the reduced mesh without IDs, and atomic rejection of malformed ID data.

Current receipts:

- `.local/native-large-skin-unit-2.log`: **54 cases and 394,297 checks** pass,
  including the existing 52 matrix-layout cases. Their runtime corner-query
  error stays at **0.00000143051 Melee units**; the large fixtures use encoded
  position/weight readback. The reduced meshes retain 4,471 vertices and shrink
  to **103,845 / 103,877 bytes** after retiring their first section.
- `.local/native-large-skin-existing-1.log`: existing rest-compensation and
  costume-capacity suites pass.
- `.local/native-stock-large-skin-1/receipt.json`: all **130 costumes**, **8,149
  meshes**, **227 brush cases** and **123,904,853 checks** pass.
- `.local/native-gltf-large-skin-1/receipt.json`: the **409 native checks** and
  saved import/reimport batch pass. The source draft remains unchanged.

`.local/native-large-skin-receipt.json` records the reviewed binary hashes.
These are backend and runtime-query checks; no new live visual acceptance is
claimed for the large fixtures.

## Source import

The [GLB/glTF importer](native_large_gltf_checkpoint.md) now emits the same
local arrays and vertex IDs. It admits up to one million source vertices and
corners per mesh, including source skin weights and saved correspondence for
reimport. That checkpoint records its separate import and runtime checks.
