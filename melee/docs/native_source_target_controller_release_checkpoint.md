# Interpreter target geometry, attachment release and Item controllers

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

This batch continues the full retained Link and Young Link source integration
after `948cf3ed5`. Reusable engine operations remain native; authored decisions
remain in the interpreter.

## Connected operations

- `collision.position.x/y/z` observes each Fighter or root Item's cached native
  collision position, independently of its actor position and resource records.
  The read-only components preserve native Float bits.
- `targets.model_scale()` and `targets.guard_active()` use the actual native
  observations. Shield geometry and camera-target positions use their native
  raw part indices and matrix sampling, with distinct typed outputs published
  only on success. Link's scripts explicitly clear their fallback outputs where
  the original exporter returned zeros for a missing target.
- `item.release_attachment(x, y, z[, preserve_held])` runs the actual common
  release routine. XYZ is initial velocity; native throw-speed multiplication
  occurs once. Optional held-slot preservation uses the original creator and
  its pre-release ownership relationship. Nested ItemDropExt continuations
  retain the physical participants and their authored program/storage identity.
- `item.set_animation_descriptor(Int)` and
  `item.add_animation_descriptor(Int)` select the imported Article state
  triplet. Set removes controllers, restores the descriptor pose and requests
  frame zero; Add attaches controllers without those changes. Neither changes
  the authored state or command-stream pointer. Controller demand and origins
  are prepared at loading transitions in bounded storage.
- `item.dynamic_joint(index)` selects the actual native dynamic-bone table
  entry. Its index is a prepared literal; the selected joint must belong to the
  Item's admitted graph. Existing matrix rotation reads for model joints and
  attachment bones call native `HSD_MtxGetRotation` without refreshing matrices.
  This preserves the original animation-phase timing.
- Item wall and ceiling normal components read their stored collision records,
  preserving native Float bits without requiring a current contact or sweep.

The Arrow disappearance effect uses existing position-effect and joint-position
primitives. Its script preserves the three random samples consumed by native
zero-jitter creation. Bow cleanup already uses the general recipient export
call rather than a fighter-specific API.

See [target queries](scripting/targets.md),
[cached Item observations](scripting/item_observations.md),
[Item controls](scripting/item.md) and
[Article models](scripting/article_models.md).

## Validation and remaining work

The native client, content tool, source-assets target and selected integration
targets build; all sixteen selected CTests pass. These hidden checks open no
game, GPU or audio device.

Frozen comparison receipts (independently rehashed by root):

| Check | Receipt | SHA256 | Scope |
| --- | --- | --- | --- |
| Target geometry | `A:/FreakMelee-test-archives/script-target-geometry-oct10-final4/receipt.json` | `5e7a91798b633dce2db8b790ff0ff533d459ce58d8648fae99d8d64abbb63c9d` | 605 native comparisons, 370 VM calls; controlled rig/matrix leaves. |
| Cached collision position and normals | `A:/FreakMelee-test-archives/script-collision-normals-oct10-final/receipt.json` | `3b0f439c0d36b272cb065d99cafb26f64bc6c1dc15fe5f47e0bd729f312e5182` | 2,432 native comparisons, 886 VM calls, including 552 receiver calls. |
| Attachment matrix rotation | `A:/FreakMelee-test-archives/script-attachment-rotation-oct10-final/receipt.json` | `d78dc8074baff93231248edc7475705f56004ad92b712d1b2690d67a9bbeffd6` | 1,150 native comparisons, 946 VM calls and 34 negative cases; cached matrices and retained attachment identities. |
| Attachment release | `A:/FreakMelee-test-archives/script-item-release-oct10-final4/receipt.json` | `0f51935b6832f9b94f88a01f937231a3156af4d383437ee61ce85d4c4b0365d8` | 130 native comparisons/VM calls; actual ItemDropExt chain with six retirement/fault cases. |
| Dynamic joints and existing matrices | `A:/FreakMelee-test-archives/script-item-joint-model-oct10-final/receipt.json` | `60df50bbb37c299d4977632ac1c75fa4673798df95153a60e26139280768bda8` | 630 dynamic-channel comparisons and 24 actual matrix-rotation comparisons; controlled actor/constructor boundaries. |
| Descriptor controllers | `A:/FreakMelee-test-archives/script-item-descriptor-animation-oct10-final3/receipt.json` | `0177a5e88ddd3654bca042b41e9cb3f24e6be976e45e89c9243e7411970d3c97` | 450 native graph comparisons, 1,280 effective texture-ID comparisons, 640 Arrow RNG/argument cases; 40 Pages frames and 10 restores. |
| Full-source preparation | `A:/FreakMelee-test-archives/source-target-controller-release-census-oct10-final4/receipt.json` | `e868712d5cd6760e8e877003de526cf49754d2419187aad7c15f9ff150e55c16` | Ten intact sources, current tool and source hashes; eight prepare. |

The descriptor-controller proof uses genuine imported Bomb and Boomerang
descriptors. Native HSD renumbers texture IDs
during material construction; controller demand, provenance and live graph
admission now use the effective resource IDs. Material flags alone do not imply
an external texture: refusal depends on the external shadow/toon context
actually selected by native material compilation.

Bounded constant TIMG streams now select their pinned borrowed image-table
entries through the native controller. Bomb histories start from native
descriptor 0 or 1 and switch to descriptor 3; both owners exercise fourteen
image transitions each. Removing animation retains the native borrowed table
and selected image. Interpolated TIMG streams, palette animation and external
shadow/toon contexts remain outside this admitted subset. The synthetic
sibling-controller case is labelled separately from imported-resource proof.

The current ten-source preparation census still accepts Mario, Dr. Mario,
Peach, Game & Watch, Zelda, Sheik, Popo and Nana. Link and Young Link now reach
`item.reflector` in their Boomerang reflection callbacks (2228 and 2241).
Their full source bodies are intact. The reflector observation remains the
next ownership integration operation.

These focused native comparisons and source preparation checks do not establish
complete tether, paired-fighter or whole-fighter gameplay. Companion activation
and member mapping remain separate. Compact whole-match save/load still retains
Regions/Pages as its broad oracle. The goal remains active.
