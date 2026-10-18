# Stock mesh weight painting

The Model weight brush now edits ordinary stock meshes as well as imported
meshes. It preserves their existing materials, colors, UV seams, normals and
polygon render flags. Work happens when the author starts or commits a stroke;
the change adds no simulation-tick allocation.

## Implementation

`native_fighter_skin_mesh.cpp` reads GX vertex layouts and envelope bindings.
Each editable vertex has an identity based on its source position, coordinates
and bone weights. Display-list corners retain their separate attributes and
refer to that vertex. This lets UV seams share a painted vertex while allowing
one position-array entry to have multiple bone assignments.

The reader handles direct or indexed attributes, quantized coordinates, XYZ
normals, NBT and NBT3 data, colors, multiple UV sets, triangles, strips, fans and
quads. Root-owned single-bone envelopes are converted from bone-local positions
to their bind coordinates. Normals receive the corresponding inverse-transpose
transform. Non-root attachments retain their model-node coordinate domain.

`native_fighter_skin_edit.cpp` rebuilds the selected mesh in an asset
transaction. Positions and normals use indexed floats; normal values are shared
when equal. Other attributes retain their source bytes and arrays. Triangle
chunks respect the ten-envelope matrix limit and display-list bounds. The
result passes the game's costume importer before replacing the project's bytes.
Untouched envelopes keep their authored float weights; reading them repeatedly
does not renormalize them.

`native_model_view.c` returns current world positions in original corner order.
The brush maps those corners to editing vertices. This supports direct
positions and repeated indices with different posed positions. GX attributes
are read in hardware order, including NBT's normal slot, using stack storage.

The former glTF-specific painting functions are removed. Workshop strokes,
the content tool and root-owned weight transfer use the shared skin reader.

## Offset-zero references

Nine retail costumes exposed a DAT reference bug: their color arrays begin at
data offset zero. A local reference to offset zero is distinguished from null
by its relocation entry. `NativeAssetDat::link_local` preserves that reference;
`link(..., 0)` continues to write null. Skin serialization and rest-compensation
copy paths now distinguish the two cases. The test also exercises transitions
between a local-zero reference and null.

## Verification

All checks below used the current native build. No visible or audible game
launches or network service were needed.

- `.local/native-stock-skin-roundtrip-4/receipt.json`: **PASS** for all 130
  retail costume archives, **8,149 meshes**, **227 brush cases** and
  **51,000,004 checks**. Each complete rebuilt costume passes the game importer.
  Triangle corners retain positions, weights, normals/tangents/binormals,
  color/UV values, texture-matrix attachment, flags and mesh/material ownership.
  Brush cases check saved strength and preservation of the other attributes.
  Geometry comparisons use an absolute float tolerance of 1e-5; unchanged
  weights and raw color/UV values compare exactly.
- `.local/native-stock-skin-live-review-4/receipt.json`: **PASS** through the
  native Workshop on a copied Yoshi draft. A mouse stroke changes vertex 29's
  selected-bone weight from 0 to approximately 0.2; Save, Undo and Redo preserve
  the expected complete costume bytes. The 293 unpainted visible vertices move
  by at most **6e-8** in normalized screen coordinates. These positions come
  from the live HSD pose, independently of the asset writer's bind-space data.
- `.local/native-generic-skin-workflow-1/receipt.json`: **PASS** for the
  asymmetric imported mesh in Pose and Animation modes, empty-stroke playback
  pause, exact Undo/Redo, rest compensation, reopening without the GLB, and
  Test Character. The strict untouched-vertex preview comparison still passes.
- `.local/native-generic-skin-precision-2`: **PASS** for saved floating-point
  weights, root-owned replacement transfer, single-bone inverse binds, sixteen
  influences and atomic rejection of a seventeenth influence.
- `.local/native-stock-skin-build-5.log`: game and content tool build; skin
  weights, rest compensation and posed-corner CTests pass. The corner query
  test includes unsorted descriptors, direct positions and one source index
  under two different matrices.

The raw imported-mesh oracle now follows the actual normal indices and compares
every referenced normal by vertex. It therefore checks normal values even when
serialization shares equal normal entries.

Reproduce the retail batch from the repository root after building
`native_stock_skin_edit_test`:

```powershell
python melee/tools/review_native_stock_skin.py `
  --iso "Super Smash Bros. Melee (USA) (v1.02).iso" `
  --test melee/build/native/native_stock_skin_edit_test.exe `
  --output .local/stock-skin-review
```

The runner processes every archive and retains the complete failure list. It
keeps one temporary extracted costume at a time and removes it after the scan.
`review_native_stock_skin_live.py` runs the mouse-brush check against a copied
profile; its arguments identify the existing draft and mesh.

## Remaining Model work

The overall Model painting/storage task remains open. Morph-target painting,
secondary-joint rigid polygons, independent texture-matrix bindings and helper
attachments under a root with a nonidentity inverse bind need further support.
These cases are rejected rather than silently converted. The retail inventory
does not contain them. Painting still has a 65,536-vertex mesh limit and sixteen
bone influences per vertex. Unreachable DAT data still accumulates after edits;
compaction and larger authored models remain work to do.

Root-owned weight transfer now reads stock meshes and painted imported meshes.
Non-root transfer still needs conversion into a common bind space. Extra bones
and source-aware reimport remain separate Model tasks.
