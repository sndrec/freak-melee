# Morph surfaces in the Model viewport

The [weighted morph checkpoint](native_morph_skin_checkpoint.md) extends this
path to private envelope batches and records the newer runtime/rest-edit tests.

Morph meshes now participate in viewport picking, outliner hover highlighting
and posed corner queries. These paths evaluate the same local positions as the
normal renderer, then apply the owning joint's current matrix.

`HSD_ShapeSetEvaluate` owns morph evaluation in `sysdolphin/baselib/shape.c`.
Both polygon drawing and editor queries call it directly. The old private
decoder and blending code in `pobj.c` has been removed. It supports adjacent
complete-shape interpolation and a base shape with weighted additive offsets.
XYZ and NBT normal vectors use the same target evaluation.

The evaluator supports signed/unsigned 8/16-bit components, floats, each
descriptor's fraction, native/big-endian payloads and 8/16-bit shape indices.
Quantization uses a positive floating-point scale through fraction 31.
Complete-shape endpoint selection clamps before converting to an integer.

Scratch storage is fixed, reused and excluded from mutable match state.
Evaluation changes only the supplied output arrays. It allocates no memory.
Position-only queries skip normal evaluation. Import validation enforces the
existing 2,000-position and 2,000-normal-vector limits before scene creation,
including NBT's three vectors per normal entry. Inconsistent blend modes reject
during import.

## Verification

The posed-vertex suite compares quantized and float evaluation across byte
orders, index widths, XYZ/NBT normals and complete/additive modes. Cases include
indices above 255, fraction 31, negative weights, endpoint clamping, output
guards and an unchanged controller. World-position and ray-picking checks use
multiple owning-joint matrices.

The rest-edit suite now evaluates imported shape descriptors through the actual
runtime function and compares their world-space positions and lighting vectors
with its independent double-precision DAT oracle. It runs before and after
translation, rotation and nonuniform scale edits, across both blend modes and
eight blend settings.

`tools/review_native_morph_view.py` creates complete/additive shape fixtures in
copies of a real Mario draft. It retains stock material/texture objects because
fighter texture-animation tables still address them, while removing their
geometry from the fixture. Only the new morph quad draws. Hidden, muted clients
then exercise outliner hover and picking by moving the cursor over the actual
rendered surface. PNG pixel counts verify that each route highlights the quad.

Current receipts:

- `.local/native-morph-query-tests-2.log`: both posed-vertex and rest-edit CTests
  pass. The posed-vertex suite reports **22,452 checks**. The rest suite reports
  **24,247,840 checks**, including **1,008**
  format/transform cases and **360** morph cases.
- `.local/native-morph-view-4/receipt.json`: complete/additive native reviews
  pass. Each fixture changes from **zero** highlight pixels to **38,590** with
  either outliner hover or viewport hover. The source draft stays unchanged.

The live fixtures use their default blend. The geometry suites cover the other
blend settings, quantization and lighting-vector formats. Native build:
`.local/build-native-morph-query-4.log`. The live receipt records the executable
hash. `.local/native-morph-query-receipt.json` collects the reviewed hashes and
test outputs.

## Later morph work

The [weighted morph runtime](native_morph_skin_checkpoint.md) and
[painting checkpoint](native_morph_paint_checkpoint.md) add private envelope
batches while keeping the original controller. The
[authoring design](native_morph_skin_design.md) records the remaining GLB target
encoder and target-aware source reimport. The broader Model painting/storage
milestone remains incomplete.
