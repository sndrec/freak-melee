# Extra-bone authoring checkpoint

The animation-node remapper, coordinated core-asset save and stable script bone
references are implemented.
Adding, removing and reparenting bones in the Workshop remain open. The Model
workspace does not expose a partial structural edit.

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
| Scripts | Named roles resolve to bone IDs; script boundaries translate IDs to physical parts. The fighter DAT owns the ID table. | Preserve each retained bone's ID when moving its physical slot, reserve new IDs for inserted bones, and preserve article reservation IDs. Numeric script parameters keep their values. |
| Editor state | Names use DAT offsets. Model Undo currently restores costume data and changed selector bytes. Animation, facts and script data have other owners. | Publish/undo the entire structural change atomically; remap selection and temporary poses by retained object identity. |
| Saving and loading | Project facts must agree with imported costume joint counts. | Save and reopen the matching costume, animation, fighter, facts and authoring data as one coherent project. |

The existing reusable rest-transform and painting operations remain available.
They do not change traversal order and do not imply structural editing is ready.

## Stable script bone references

`native_fighter_rig.{h,c}` stores an immutable bone-ID lookup in each imported
fighter. A fighter without an authored table starts with its original numbering.
An authored table lives at the `Freak_RigBones` root in the owned fighter DAT.
It contains IDs, optional names, editing flags and the next available ID. The
writer reuses a fixed allocation on subsequent saves. Original root symbols,
external references and data offsets stay intact when adding the root.

Script IDs identify bones independently of physical part order. Named roles
still use the fighter's role mapping; `bones.index(role)` returns the selected
bone's ID. Joint queries, transforms, pose starts, hitbox/hurtbox creation,
shields, reflectors, absorbers, effects and fighter attachments translate IDs
at their script boundary. Target queries use the target fighter's own table.
Native descriptor fields, deferred queues and article-local model indices keep
their physical meaning. In particular, shield/camera observations and queued
effect reentry must not translate a physical slot a second time.

The table also tracks slots reserved outside the body rig for article resources.
A future insertion must preserve an occupied resource's ID when relocating its
physical reservation. Initial IDs occupy the original 140-slot namespace;
new IDs increase monotonically. Removing an ID leaves it unavailable. The live
engine still supports at most 140 physical parts. This metadata does not raise
that limit or expose structural editing before the complete transaction exists.

Lookup uses a fixed 512-entry hash table prepared during import. Unauthored rigs
use their original numbers directly. Runtime lookup allocates no memory. Import
rejects duplicate IDs/names, inconsistent counts, malformed pointers or flags,
missing live-body IDs and unterminated names before publishing the assets.

Checks completed for this checkpoint:

- Rig codec: **2,004 checks**, including a 140-entry hash collision chain,
  insertion/reordering/removal metadata, bounded repeated rewrites and unchanged
  original symbols/external references.
- Bone-query compiler/actor/VM cohort: **4,884 checks**, including an ID of
  500,000 resolving to physical joint 2, role-to-ID queries, offsets, native
  matrix comparisons, translation and missing-ID behavior.
- Target-query/hitbox cohort: **19,899 checks**, including foreign fighter IDs,
  unchanged native shield/camera observations, and **128** direct/VM hitbox
  comparisons against the native initializer with a nonidentity bone ID.
- CMake regression group: **19 suites**, passing. This covers pose, resource
  metadata and the affected script/runtime cohorts; it is not full gameplay
  coverage of every attachment/effect path.
- All **130 retail costumes**, **8,149 meshes** and **123,904,853 checks** pass
  the skin-editor/archive regression after the shared DAT footer change.
- Copied Mario and Yoshi coordinated-save tests: **427 checks each**, including
  adding the ID root, import/reopen and complete-asset Undo/Redo. Original draft
  hashes remain unchanged.

The direct-source graph cohorts now link the current scene arena and native
Ground/Item pool owners. Profiling is disabled, RNG tracing has no sink and
unselected accessory/class-grid operations remain explicit fixture boundaries.
Native graph construction and complete rendering/gameplay remain separate
acceptance work.

Evidence:

- `.local/native-rig-binding-ctest-1.log`
- `A:/freak-melee-reviews/stable-bone-queries-3/receipt.json`
- `A:/freak-melee-reviews/stable-target-queries-4/receipt.json`
- `.local/native-rig-stock-skin-1/receipt.json`
- `.local/native-rig-project-assets-mario-1/receipt.json`
- `.local/native-rig-project-assets-yoshi-1/receipt.json`
- `.local/native-rig-model-workflow-1/receipt.json`

The rebuilt client passes the native Model workflow on the copied Mario draft
with an authored ID table: posed/animated painting, rest compensation, exact
Undo/Redo, reopening without the source GLB and Test Character/return. Both
clients were hidden and muted and used isolated profiles. No test process remains.
Game SHA-256: `7ad20c0913ba0a5bfa9f30276dd42081ae83a9d26e7fdefba592ae5c16561a10`.
Tool SHA-256: `1f9fa312503787b69c1b60eeca7180b5125637511ae7073e83af031c7b4733f7`.

## Coordinated core-asset saving

The Workshop Save button now calls `nativeFighterProject_SaveAssets` once per
edited family member. This publishes `fighter.dat`, `animation.dat`,
`costume.dat` and `assets.fma` together. It imports the complete candidate and
checks that the in-memory facts match the serialized facts before writing.
Counts and bone mappings may change while the draft's identity remains the same.
The editor marks its animation, action and model state saved after the save flow
succeeds. The old UI helpers that performed separate animation/model saves were
removed; the command-line tool retains its explicitly scoped editing operations.

The save stages all four candidate files and their four matching predecessors.
It then moves original destinations aside and replaces them. If any replacement
fails, it restores the destinations in reverse order, including predecessor
files already replaced. A no-change save keeps the existing predecessor.
Files left by an interrupted operation are retained for inspection. This is
rollback for a failed save operation; it does not promise crash/power-loss
recovery or an atomic save of an entire two-member family.

`nativeFighterProject_UndoLastAssetSave`, also exposed as
`native_content_tool.exe project-assets-undo PROJECT_DIRECTORY`, restores the
matching four-file predecessor. Repeating it toggles the two coherent asset
sets. The files use the suffix `.rig.previous`. Scripts, parameter files and
editor names keep their existing save paths; a future structural edit must also
preserve their references and participate in in-memory editor Undo.

`native_project_asset_save_test` creates simultaneous changes in all four
assets: a new clip and its descriptor, a rest transform, and an owned gameplay
option. It checks Save/reopen, no-change saves, invalid candidates, mismatched
draft identity, pre-existing staging/recovery files, complete-asset Undo/Redo
and unrelated-file preservation. It locks each of the eight destination files
in turn to force failure at every replacement position, then compares all eight
files with their exact pre-save contents.

Results on copied Mario and Yoshi drafts: **424 checks each**, passing. Source
draft hashes remain unchanged. The native Model workflow also passes through
the new Save path: posed/animated painting, rest compensation, byte-exact
Undo/Redo, reopen without the source GLB, and Test Character. The command-line
complete-asset Undo/Redo round trip passes.

Receipts:

- `.local/native-project-assets-mario-1/receipt.json`
- `.local/native-project-assets-yoshi-1/receipt.json`
- `.local/native-project-assets-model-workflow-1/receipt.json`
- `.local/native-project-assets-cli-1/receipt.json`

Game SHA-256: `c49d27c674f20baccbd4f4fc7662f805286f7ceb9027c5ea79b2ba3e497f72fc`.
Tool SHA-256: `cbe942dd1000880f7316368ee1c1fb8c0f3594df8bab229c5053035646612aee`.

Reproduce the save-failure checks with a fresh output directory:

```powershell
python melee/tools/review_native_project_asset_save.py `
  --exe melee/build/native/native_project_asset_save_test.exe `
  --draft PATH_TO_DRAFT --output .local/project-asset-save-review
```

The review copies the draft before running the native test and checks that the
source files remain unchanged. All validation/import and filesystem work happens
at authoring transitions, with no added match-tick work.

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
The animation checks establish archive remapping. Live extra-bone animation,
skinning, gameplay references and structural editor Undo/Redo still require
the complete rig-edit transaction.
