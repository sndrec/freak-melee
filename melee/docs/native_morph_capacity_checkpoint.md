# Larger morph meshes

The runtime and Model tools share a limit of 65,536 logical positions and
65,536 logical normals per morph controller. Each NBT normal entry holds a
normal, tangent and bitangent. The former 2,000-vector limit is removed.

The drawing scratch contains 65,536 position vectors and 196,608 lighting
vectors. Model picking has its own 65,536-position scratch. These shared buffers
total 3.75 MiB. They retain stable addresses outside `.mstate`, and each use
overwrites the consumed prefix. Drawing, picking and simulation allocate no
additional storage for them. Source target data and mutable blend weights keep
their existing ownership.

Weighted-shape admission allocates temporary correspondence maps for the
controller's actual position and normal counts. It frees them before returning.
These checks run during asset construction. The maps use neither the Windows
stack nor resident gameplay storage. Painting and rest compensation use the same
logical limits, including all three NBT vectors. The target writer accepts the
full 65,536-entry index table, including index 65,535.

## Remaining limits

Target data still uses 16-bit physical indices. A channel can reference at most
65,536 distinct stored vectors across its targets. Separate equal vectors can
share storage. Logical target identity and bone-weight seams remain separate.
Meshes above these limits need a further representation change. Capacity errors
leave the draft unchanged.

Tangent-only, UV and color GLB target channels, unusual helper/independent
texture-matrix domains and broader user workflow acceptance remain open.

## Verification

The capacity suite exercises 2,001, 65,535 and 65,536 entries in complete and
signed-additive modes, with absent normals, XYZ normals and NBT triples. Guarded
outputs check the final written entry and surrounding memory. Invalid counts
reject before changing output.

The weighted-shape suite writes, imports, constructs, evaluates and queries
65,536-entry complete/additive models with XYZ and NBT data. Their corner stream
crosses a GX primitive/private batch boundary and uses index 65,535.

The GLB workflow suite adds 12,000-vertex sources to its rigid, generated-weight
and compatible source-skin cases. It compares evaluation with the source,
paints, edits rest poses, saves/reopens, restores history/snapshots and reimports
reordered/appended target names with retained weights.

The obsolete source-extraction scratch runner was removed. Its historical
receipt remains in the [original checkpoint](native_graph_buffer_snapshot_ownership.md).
The current evaluator, weighted-shape and capacity tests link the runtime used
by the client.

`.local/native-gltf-morph-capacity-1/receipt.json` records **6,593,185 checks**
and an unchanged source draft. The evaluator capacity suite records
**5,589,150 checks**. The current CTest batch covers rest edits, posed Model
vertices, weighted shapes and capacity guards:
`.local/native-morph-capacity-regressions-1.log`.
The final weighted-shape test adds explicit checks for every lighting vector at
the full index limit: **434 cases, 3,471,072 checks**, recorded in
`.local/native-morph-capacity-shape-2.log`.

The hidden, muted native Workshop review displays the saved 12,000-vertex
expression mesh and queries all 12,000 vertices. Its full-UI PNG, binary hashes
and stopped client are recorded in `.local/native-morph-capacity-live-1/receipt.json`.
This is drawing/query evidence; the existing smaller native brush review and
the larger backend workflows provide the painting/history checks.

The rebuilt client's `.mstate` section remains **1,085,229 bytes**. The ordinary
data section grows by exactly the additional **3,860,160 scratch bytes**, while
its file-backed size is unchanged. See `.local/native-morph-capacity-sections.json`.
Build: `.local/build-native-morph-capacity-1.log`.
