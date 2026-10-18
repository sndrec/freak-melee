# Extra-bone authoring checkpoint

The animation-node remapper is implemented and checked against the retail
animation corpus. Adding, removing and reparenting bones in the Workshop remain
open. The Model workspace does not expose a partial structural edit.

## Animation remapping

`src/melee/ft/native_fighter_animation_nodes.{h,cpp}` rewrites an animation DAT
from an explicit destination-to-source node map. An empty destination entry
creates an unanimated bone; omitted source entries remove their tracks. Retained
nodes keep their track order, serialized track metadata and compressed stream
bytes. Tree type, flags, duration, root names and root aliases survive the edit.

The operation constructs a fresh archive from its figatrees. It validates every
root before returning an owned output buffer, and leaves input/output unchanged
on failure. It rejects external dependencies, duplicate names, invalid/repeated
map entries and roots with an unexpected source node count. Scratch allocation
and output construction happen during authoring, with the existing asset size
limit. The function does no match-tick work.

These maps address **animation nodes**. The future rig transaction must derive
the right map for each clip, including its reserved parts and animation masks.
It must not assume every clip has one node for every costume joint. The utility
accepts up to 512 nodes; it does not raise the live fighter limit of 140 parts.

The C++ declaration of `lbAnim_DecodeFigaTree` now uses C linkage, matching its
existing C implementation. The remapper and its test use that decoder directly.

## References a complete rig transaction must preserve

| Data | Existing source contract | Required structural edit |
| --- | --- | --- |
| Costume hierarchy | `native_fighter_model_editor.c` enumerates child/sibling preorder. Meshes, envelopes and inverse binds reference DAT objects. | Preserve retained object identities and skin bindings, change hierarchy, then derive old/new traversal maps. |
| Gameplay roles | `native_source_asset_facts.h` owns semantic-role mappings, joint counts and reserved-part insertion data. `native_source_asset_import.c` constructs runtime tables from them. | Update counts and role/omission indices together, retaining required helper relationships. |
| Animation | `ftanim.c` selects tracks by part order, masks, animation family and reserved-part policy. Action/demo descriptors address the animation bank. | Remap each applicable tree, rebuild bank descriptors and preserve aliases. |
| Fighter data and commands | `ftcharacter_assets.c` imports model parts, hurtboxes, dynamics, collision data and IK; hitbox/subaction data also carries bone selections. | Classify each field as a semantic role, raw part index or object reference before changing it. |
| Scripts | `native_script_binding_presentation.c` distinguishes semantic roles from raw joints; effects and hitbox definitions can use raw indices. Examples also obtain indices from editable attributes. | Keep named roles working and give raw references an explicit migration path. Rewriting all matching numeric literals would change unrelated values. |
| Editor state | Names use DAT offsets. Model Undo currently restores costume data and changed selector bytes. Animation, facts and script data have other owners. | Publish/undo the entire structural change atomically; remap selection and temporary poses by retained object identity. |
| Saving and loading | Project facts must agree with imported costume joint counts. | Save and reopen the matching costume, animation, fighter, facts and authoring data as one coherent project. |

The existing reusable rest-transform and painting operations remain available.
They do not change traversal order and do not imply structural editing is ready.

## Verification

`native_animation_nodes_test` compares decoded trees and retained serialized
tracks independently of the writer. It checks insertion inside a rig, reverse
ordering, deletion, restoration, canonical rewrite stability, multiple distinct
roots, aliased roots, shared streams and a 512-node destination. Malformed maps,
duplicate root names, truncated streams, a bad later root and occupied outputs
fail without publishing data.

Current results:

- Synthetic CTest: **2,484 checks**, passing.
- All **33** retail `Pl??AJ.dat` banks: **6,245** trees and **58,639,871** checks,
  passing. Retained descriptor and stream payloads match byte for byte.
- Native game object target compiles with the new implementation and decoder
  declaration. No GUI launch was needed for these archive checks.

Receipt: `.local/native-animation-nodes-retail-2/receipt.json`.
Test executable SHA-256:
`2421b75735ed45588e745b3ac5e94905123cb3e111a07476d2d12342a9c6130c`.

Reproduce the corpus check after building `native_animation_nodes_test`:

```powershell
python melee/tools/review_native_animation_nodes.py `
  --exe melee/build/native/native_animation_nodes_test.exe `
  --iso 'Super Smash Bros. Melee (USA) (v1.02).iso' `
  --output .local/animation-node-review
```

The output directory must be new. The review records every bank failure, uses
temporary extraction and does not change installed fighters or project drafts.
These checks establish archive remapping; live extra-bone animation, skinning,
gameplay and Save/Undo/Redo still require the complete rig transaction.
