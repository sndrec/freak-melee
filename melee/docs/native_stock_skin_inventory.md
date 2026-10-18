# Stock mesh weight editing inventory

The imported-mesh brush expects floating-point positions, normals and UVs with
matching indices. Retail costumes also use quantized coordinates, separate
normal and UV indices, multiple texture-coordinate sets and vertex colors.
Painting those meshes needs to preserve their existing vertex attributes and
bone transforms.

`melee/tools/audit_native_model_layouts.py` reads the retail v1.02 ISO and checks
every costume archive with a `PlXXYy.dat` filename. This includes Captain
Falcon's grey (`Gy`) costume, paired fighter members, bosses and Sandbag. It
follows costume joint, mesh and polygon lists and inspects referenced display
list attributes and envelope weights. It writes metadata only.

Run from the repository root:

```powershell
python melee/tools/audit_native_model_layouts.py `
  --iso "Super Smash Bros. Melee (USA) (v1.02).iso" `
  --output .local/native-stock-model-layouts.json
```

The completed scan covers 130 archives, with no skipped archives or parse
failures. The receipt is `.local/native-stock-model-layouts-5.json`.

| Property | Count |
| --- | ---: |
| Meshes | 8,149 |
| Polygon records | 10,320 |
| Vertex layouts, including format and quantization | 40 |
| Display-list corners | 987,377 |
| Enveloped polygons, all owned by skeleton roots | 8,607 |
| Rigid polygons | 1,713 |
| Single-bone envelope records using the root shortcut | 21,743 |
| Meshes with multiple bindings for a position-array entry | 136 |
| Position entries with multiple bindings | 300 |
| Meshes with split normal, color or texture-coordinate indices | 4,546 |
| Position/binding combinations with attribute seams | 67,026 |
| Largest mesh, counting position/binding combinations | 1,482 |
| Largest envelope | 6 influences |

These are totals across costumes; shared geometry in different archives is
counted for each archive. Envelope comparisons merge duplicate bones and sort
the remaining influences, so a different influence order alone does not count
as a different binding. The seam count records different indexed attributes at
the same position and binding; it does not measure whether their values differ.

The display lists contain triangles, triangle strips and quads. Texture matrix
indices occur in 41 polygons and use texture matrix 30. Some layouts use NBT
normal/tangent/binormal data, indexed colors or direct colors. No shape-animation
polygons or shared-rigid secondary joints occur in this retail costume set;
custom assets can still use those features.

## Requirements for the brush implementation

- Give each position and binding combination a stable identity. An array index
  alone is insufficient for the 136 meshes with multiple bindings.
- Preserve each corner's normal, colors, UV sets and texture matrix references.
  UV or normal seams should not become extra independently painted vertices.
- Preserve polygon flags, mesh order, materials and visibility references.
- Account for HSD's root single-bone shortcut. It uses the bone's world matrix
  directly; blended envelopes also use inverse binds. Converting those vertices
  to blended weights must compensate their positions and normals.
- Keep helper-owned attachments in their original coordinate domain. The
  model-node transform in `_HSD_mkEnvelopeModelNodeMtx` can depend on the current
  animation, so flattening everything into a fixed world space can change motion.
- Match the brush's vertex identities to the live rendered surface, including
  repeated position indices under different envelopes.

The relevant engine behavior is in `baselib/pobj.c` (`SetupEnvelopeModelMtx`)
and `baselib/displayfunc.c` (`_HSD_mkEnvelopeModelNodeMtx`). The existing rest-edit
compensation in `native_fighter_rest.cpp` also handles these coordinate domains.

This checkpoint establishes the retail input cases. Stock weight painting,
geometry round-trip comparisons and native brush acceptance remain open.
