# Texture attachments during weight painting

The native reader/writer and current retail regression batch pass.

Meshes can use a bone matrix to generate reflected texture coordinates. A
texture can follow the surface's painted weights or stay attached to a separate
bone combination. Painting needs to retain that distinction.

The authoring reader now records following, independent and identity selectors
for each texture-coordinate channel. Independent attachments keep their authored
weights through a stroke. The writer assigns them their own matrix slots while
partitioning draw sections. Identity selectors remain identity selectors.

Equal weights retain separate position and independent-texture slots. This
matters when a stroke makes the surface temporarily match the texture's bone
weights: a subsequent stroke must still leave that texture attachment alone.
The read-side hash lookup shares equal independent attachments across morph
batches without searching every earlier binding on each corner.

Ordinary envelopes and weighted morph batches use this representation. The
morph parent records topology and whether a texture uses identity; private
batches retain the actual matrix selectors. Admission checks every selector
against its batch's matrix count. Bone preparation and drawing continue through
the existing envelope path. The new bookkeeping exists during authoring;
saved matrix groups use the existing envelope storage. No runtime object fields
or tick allocations are added.

## Verification

- `.local/native-skin-texture-core-1.log`: rest edits, weighted morphs and rigid/
  ordinary skin suites all pass. The complete suites cover **450 morph cases,
  5,837,762 checks**, and **66 rigid/ordinary cases, 513,464 checks**.
- The new ordinary-envelope fixtures cover root, deform-bone and helper owners,
  eight/sixteen-bit indices, XYZ/NBT normals, three texture channels, equal
  position/texture weights and two actual brush strokes. An independent raw-DAT
  sampler checks the generated texture normal vectors at **19 animated poses**.
  Maximum component error remains **0.000000238419**. Palette exhaustion leaves
  the project's pointer, size and bytes unchanged; a differing basis rejects on
  read.
- New complete/additive morph fixtures cover root/deform/helper owners and
  XYZ/NBT normals. Actual runtime surfaces match through a no-op rewrite.
  Repeated writes and compaction keep fixed texture weights through convergence
  with a position binding and subsequent weight edits. Five malformed selector
  cases reject during importer admission.
- `.local/native-skin-texture-retail-1/receipt.json`: all **130 costumes, 8,149
  meshes, 227 brush cases and 123,904,853 checks** pass. This compares the retail
  attributes, weights and material/mesh ownership and checks compaction and
  runtime importer admission.
- `.local/build-native-skin-textures-3.log`: full native client, content tool and
  the test targets build. `.local/native-skin-texture-receipt.json` pins sources
  and binaries.

These are native backend and runtime-pose checks. No native UI or GPU texture
pixel comparison was run for the new independent attachment fixtures.

## Remaining limits

Each draw section uses the existing ten GX matrix slots for positions and
generated texture coordinates. Sections split between triangles. A triangle
requiring more than ten distinct position/independent-texture bindings rejects
the edit atomically. Supporting that case needs separate texture and position
matrix palettes.

An independent attachment also needs the same normal coordinate basis as its
surface. Root shortcuts or secondary rigid slots with a different basis require
a texture-specific correction; these reject before painting. The supported
envelope fixtures use the same basis. Engine-controlled helper ownership remains
a separate open domain.

This checkpoint covers the skin reader/writer. It does not complete the overall
Model workspace acceptance or replace visual review of authored materials.
