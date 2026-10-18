# Extra-bone authoring checkpoint

The animation-node remapper, rig-aware bank rewriting, coordinated core-asset
save, stable script bone references and physical fighter-reference rewriting
are implemented.
A coordinated in-memory add/reparent/remove transaction is implemented and
passes the complete test sequence on 25 stock fighters and Glim. Sheik's alternate
pose graphs and the Model controls/history remain open. Metal visibility now
follows the retained mesh instances when their draw order changes.
The Model workspace does not expose a partial structural edit.

## Animation remapping

`src/melee/ft/native_fighter_animation_nodes.{h,cpp}` rewrites an animation DAT
from an explicit destination-to-source node map for each root symbol. An empty destination entry
creates an unanimated bone; omitted source entries remove their tracks. Retained
nodes keep their track order, serialized track metadata and compressed stream
bytes. Tree type, flags, duration, root names and root aliases survive the edit.

The operation constructs a fresh archive from its figatrees. It validates every
root before returning an owned output buffer, and leaves input/output unchanged
on failure. It rejects external dependencies, duplicate names, invalid/repeated
map entries and roots with an unexpected source node count. Scratch allocation
and output construction happen during authoring, with the existing asset size
limit. The function does no match-tick work.

These maps address **animation nodes**. The rig-aware bank writer below derives
each clip's map from physical parts and animation masks. The complete hierarchy
transaction must supply the right physical part map. The utility
accepts up to 512 nodes; it does not raise the live fighter limit of 140 parts.

The C++ declaration of `lbAnim_DecodeFigaTree` now uses C linkage, matching its
existing C implementation. The remapper and its test use that decoder directly.

## References a complete rig transaction must preserve

| Data | Existing source contract | Required structural edit |
| --- | --- | --- |
| Costume hierarchy | `native_fighter_model_editor.c` enumerates child/sibling preorder. Meshes, envelopes and inverse binds reference DAT objects. | Preserve retained object identities and skin bindings, change hierarchy, then derive old/new traversal maps. |
| Gameplay roles | `native_source_asset_facts.h` owns semantic-role mappings, joint counts and reserved-part insertion data. `native_source_asset_import.c` constructs runtime tables from them. | Update counts and role/omission indices together, retaining required helper relationships. |
| Animation | `ftanim.c` selects tracks by part order, masks, animation family and reserved-part policy. Action/demo descriptors address the animation bank. | The bank writer now derives per-clip maps and rebases both tables. The hierarchy transaction must supply its physical part map. |
| Fighter data and commands | `ftcharacter_assets.c` imports model parts, hurtboxes, dynamics, collision data and IK; hitbox/subaction data also carries bone selections. | The reference writer remaps physical fields while retaining semantic selectors. The hierarchy transaction must supply the physical map. |
| Scripts | Named roles resolve to bone IDs; script boundaries translate IDs to physical parts. The fighter DAT owns the ID table. | Preserve each retained bone's ID when moving its physical slot, reserve new IDs for inserted bones, and preserve article reservation IDs. Numeric script parameters keep their values. |
| Editor state | Names use DAT offsets. Model Undo currently restores costume data and changed selector bytes. Animation, facts and script data have other owners. | Publish/undo the entire structural change atomically; remap selection and temporary poses by retained object identity. |
| Saving and loading | Project facts must agree with imported costume joint counts. | Save and reopen the matching costume, animation, fighter, facts and authoring data as one coherent project. |

The existing reusable rest-transform and painting operations remain available.
They do not change traversal order and do not imply structural editing is ready.

## Coordinated hierarchy transaction

`native_fighter_rig_edit.{h,cpp}` prepares all four core assets privately,
validates their matching facts and imports the resulting graphs before swapping
them into a draft. Failure leaves the draft and output selection unchanged.
It writes no files and changes no live scene. `nativeFighterProject_ValidateAssets`
now exposes the existing complete-asset admission used by Save to this operation.

- Add appends an empty child at its chosen parent's origin. The bone receives a
  new stable ID, a unique name and an inverse bind matrix for weight painting.
- Reparent applies to authored extra bones. Retained bones keep their world
  rest transforms. The writer uses HSD's actual parent-scale compensation and
  verifies the decomposed local transform; a change requiring shear rejects.
- Remove applies to authored extra bones. Its children move to its parent and
  retain their world rest transforms. Mesh, constraint, physical gameplay and
  other serialized references must be cleared before the bone can be removed.
  Removed joints have their old hierarchy links cleared. Required base joints
  retain their parent relationships and first-child helpers.

Costume object offsets stay stable for retained bones, meshes and materials.
The separate metal skeleton follows the new preorder. Costume material
animation nodes keep their matching bone and gain empty nodes where needed.
Partial animation variants receive new descriptor chains in the edited body
order, retaining their AObj/RObj pointers and flags. Shared variants must agree
on their positional map. Original compressed animation streams are retained by
the bank writer. Draw/texture selectors follow retained costume object IDs.

Rig facts receive the new counts, role indices and omission positions while
retaining omission identities and mask bits. Scripts keep stable bone IDs.
When the growing body consumes a physical article-reservation slot, the writer
parses the actual resource declarations and moves an occupied reservation to an
unused spare. A referenced spare cannot be retired to make room. Rig names are
now available to the Model outliner through the owned ID table.

The transaction first copies each retail hitbox's effective victim-only flag
into its owned header. Command widths, addresses, clear events and hit history
remain unchanged. It does not convert action timing to perform a bone edit.
The shared rest-matrix calculation now lives in the public
`nativeFighterModel_RestMatrices` implementation, used directly by both rest
compensation and hierarchy editing. These changes add no match-tick work.

`native_rig_edit_test` exercises a full sequence: protect a required bone, add
an extra while relocating an occupied reservation, paint onto the new bone,
refuse its deletion while weighted, reject a duplicate name and a parenting
cycle, add and pose a child, remove its parent while preserving the child's
world pose, reparent the child, then remove it. Each failure compares all four
assets and facts with their previous contents. Actual imports run after edits.
The fixture restores its in-memory paint data; this is not structural editor
Undo acceptance.

Results before metal-selector remapping:

- **25 of 26** stock definitions pass the entire sequence: **250 cases** and
  **66,600 checks**. The aggregate review correctly reports failure because
  Sheik's alternate pose graphs remain incomplete.
- Flag ownership followed by duration conversion admits all **26** stock asset
  sets. Running the complete hierarchy sequence on these pre-normalized inputs
  passes the same **25** fighters: **275 cases**, **66,675 checks**. Sheik reaches
  its alternate-pose guard after successful normalization/admission.
- Glim passes **10 cases** and **1,892 checks**. Its four source-file hashes
  remain unchanged; the test operates on loaded memory only.
- The physical-reference and owned-flag regression passes all **26** definitions
  (**260 cases**, **785,421 checks**, **39,911 command comparisons**). Glim's offset-zero model path also passes
  the reference sequence (**10 cases**, **1,200 command comparisons**, **21,870 checks**).
- Shared rest compensation passes its native CTest with **15,289,600 checks**.
- Receipts: `.local/native-rig-explicit-retail-3/receipt.json` and
  `.local/native-rig-explicit-normalized-1/receipt.json`.
- Glim output: `.local/native-rig-explicit-glim-2.log`.
- Regression receipts: `.local/native-rig-explicit-references-2/receipt.json`,
  `.local/native-rig-explicit-glim-references-1.log` and
  `.local/native-rig-edit-rest-regression-1.log`.
- Hierarchy test SHA-256: `558433df72ed11d94b3af113354e3ed557fa5f4fbdcc6d3f7ef4080253770468`.

The early Roy/Fox/Ness failures came from composing an offset-moving duration
conversion with rig steps that still addressed the input archive. Explicit
flag ownership keeps addresses fixed inside the hierarchy transaction. The
separate normalized-input review above checks conversion first, admits its
result, and then starts the rig transaction from that matching asset set.

Remaining acceptance work:

1. Sheik's alternate pose graphs have a caller-supplied start bone and consume
   a positional sequence. Their source/target bone correspondence must be
   represented before hierarchy edits can preserve those operations.
2. Connect the completed transaction to the approved Skeleton controls,
   selection/temporary-pose remapping and complete structural Undo/Redo. Verify
   Save/reopen, animation, weight painting and Test Character in the native UI.
   The game client has not been relinked or launched for this backend checkpoint.

## Metal-model visibility

`native_fighter_metal_references.{h,cpp}` remaps the metal visibility layer using
the original joint and draw-object offsets. Each draw occurrence retains its
identity even when the same draw descriptor is attached to two different bones.
New empty bones add no draws. Removing a bone with attached metal meshes still
requires detaching those meshes first.

Costume visibility is remapped first. When a metal table shares its indices with
a costume table, the transaction appends a separate metal table with the correct
indices and reconnects only the metal layer. Costume values remain unchanged.
Shared metal rows and index lists reuse their corresponding replacement records.
Unchanged selectors keep their existing records. This work runs during authoring
and adds no match-tick work.

Verification:

- Focused CTest: **12 permutation/alias cases**, **621 checks**, passing. Every
  order of three joints preserves the selected draw instances, both with and
  without an earlier costume edit to shared selector bytes. Missing meshes and
  invalid ordinals are rejected. An identity edit produces identical bytes.
- Complete hierarchy sequence on **25 of 26** stock definitions: **252 cases**,
  **78,770 checks**, passing. Game & Watch additionally moves a real metal draw
  chain onto an extra bone, reparents it to change draw order, checks that all
  visibility variants still select the same objects, and refuses to remove the
  mesh-bearing bone. The other stock models have no suitable pair of separate
  draw chains for this added fixture. Sheik reaches the existing alternate-pose
  guard; the aggregate review therefore reports failure.
- Pre-normalized stock assets: all **26** admit after flag ownership and duration
  conversion; the complete hierarchy sequence passes the same **25**, with
  **277 cases** and **78,845 checks**. Sheik reaches the same guard.
- Glim: **10 cases**, **2,205 checks**, passing. Its four source files retain
  their original hashes.

Receipts:

- `.local/native-rig-metal-retail-1/receipt.json`
- `.local/native-rig-metal-normalized-1/receipt.json`
- `.local/native-rig-metal-glim-1.json`

Hierarchy test SHA-256:
`3d4f86d31a7c7e78ce92186d37bf3dafd1d2ba2dcd2bde45f1ff03f7a67302ca`.
These checks include actual asset admission/import. Metal form rendering in a
live match and structural editor Undo remain part of the later UI acceptance.

Reproduce the whole roster check:

```powershell
python melee/tools/review_native_rig.py --suite hierarchy `
  --exe melee/build/native/native_rig_edit_test.exe `
  --tool melee/build/native/native_content_tool.exe `
  --iso 'Super Smash Bros. Melee (USA) (v1.02).iso' `
  --output .local/rig-edit-review
```

The hierarchy suite selects a matching neutral costume by its declared joint
root. It reports every fighter failure, uses temporary extracted inputs and
does not create drafts or install content. The test also accepts a single
project-directory argument for a read-only pass over an existing draft.
Add `--normalize-intervals` to check clone-time flag ownership and duration
conversion before the hierarchy sequence. Failed normalization retains its
generated fighter DAT in the review output for diagnosis; no failures occurred
in that admission phase in the current roster review.

The reference writer now accepts the character importer's valid offset-zero
model record when its source field has no relocation, as in Glim. Source graph
import errors now include their actual data offset and status for diagnosis.

## Explicit hitbox flags

Retail create-hitbox instructions read `hit_grabbed_victim_only` from bit 19 of
the next source word. That word belongs to another command. Reordering events
or changing that command's bone can therefore change collision eligibility.

`native_fighter_action_flags.cpp` traces every reachable fighter hitbox and
copies its effective flag into its own header. It then changes the opcode to
the five-word explicit-flag form, **60**. The operation gathers all changes
before writing, so shared subroutines and adjacent hitboxes read their original
flags exactly once. Addresses, other operands, timers, clear commands and
group/victim history remain unchanged. Repeating it changes no bytes.

The native dispatcher sends opcode 60 directly to the existing hitbox handler
and skip function. That handler reads owned flags from the header for opcodes
59 and 60, while retaining the retail next-word rule for opcode 11. Opcode 60
uses ordinary clear-command lifetime; opcode 59 continues to own its duration.
The duration owner no longer repeats the flag assignment. Import validates
owned hitbox slots and decodes their fields through the shared hitbox layout.

Fresh clone preparation owns flags before attempting duration conversion.
Workshop opening also owns any remaining retail flags and marks the action data
dirty. Hitbox inspection reads the effective retail flag; editing an ordinary
hitbox writes an explicit header. Event tracing, copying, insertion, timing
edits and interval conversion recognize all three create forms. The user-facing
name for an ordinary explicit-flag instruction remains **Create hitbox**.

Verification:

- The source-derived evaluator fixture compares the current handler with the
  handler at local commit `6876936`, across **8,192 cases** and **102,401 checks**.
  It changes the following GFX bone's flag bit and compares complete fighter
  state for both owned forms, then checks opcode 11 retains retail behavior.
  Conditional thrown-owner spawns, all slots, group activation/history, lifetime
  clearing and smash-timer bookkeeping are covered.
- Damage, angle and semantic-role lookup are controlled leaves in that fixture.
  It does not establish full gameplay, staleness, geometry or rollback parity.
- The 26-fighter reference cohort imports the original, owned and remapped DATs
  through the actual character importer. It checks every legacy flag against
  its original following command, header ownership count, idempotence, preserved
  operands/addresses and hitbox Get/Set round trips. A one-slot bone move now
  succeeds for all owned inputs while unsafe raw inputs still reject.
- The production duration-lifetime fixture passes expiry, playback-speed,
  handoff, refresh, gap, snapshot and cancellation cases after assigning flag
  ownership to the common evaluator.

Evidence: `.local/native-hitbox-flags-6/receipt.json`,
`.local/native-rig-explicit-references-2/receipt.json` and
`.local/native-hitbox-lifetime-explicit.log`.

```powershell
python melee/tests/native/hitbox_flags/run.py `
  --compiler C:/Users/loser/scoop/apps/llvm/current/bin/clang.exe `
  --output .local/hitbox-flags-review
```

## Physical fighter references

`native_fighter_rig_references.{h,cpp}` prepares a rewritten fighter DAT from a
physical old-to-new part map. The map covers all 140 slots, including article
reservations. Referenced removed parts and repeated destinations reject the
edit. The source project stays unchanged; output is published only on success.

The writer updates the common camera bone, model placement/foot/shield parts,
hurtboxes, dynamics roots/colliders, thrown-fighter placement, collision bubbles,
ECB bones, effect bones, IK pairs and part-animation groups. It follows reachable
action command graphs, including shared subroutines, and updates physical bone
operands in GFX, hitbox, hurt-state and wind commands. Common-role selectors,
model-selected effect selectors, native sentinel values and article-local
indices retain their meanings. Repeated references use the original data when
mapping, so aliases cannot be remapped twice.

Native dynamics follow a child chain while incrementing the physical part
number. Edits must retain that chain's contiguous numbering. The retail
create-hitbox command has another dependency: it reads its victim-only flag
from bit 19 of the following command. A following GFX or hurt-state bone field
overlaps that bit. If remapping would change it, this operation rejects before
publishing. Owned hitboxes carry the flag in their own header and have no such
dependency. The hierarchy transaction owns these flags first, preserving both
ordinary clear-command lifetimes and existing duration-based hitboxes.

`native_rig_references_test` imports original and rewritten archives with the
actual character importer, compares the typed metadata and native command
unions, and checks every other command operand remains exact. Applying the
inverse map restores the entire original DAT byte for byte. Duplicate maps,
referenced removals, truncated data and changed retail hitbox flags leave
outputs empty.

The initial raw-map review passed all **26 supported stock definitions**: **156 cases**, **16,955 command
comparisons** and **340,101 checks**. The one-slot move exercises the retail
hitbox dependency on **14** fighters; their edits reject without publishing.
The four-slot map changes every physical index while retaining that flag,
allowing metadata and command comparisons for every fighter. This is archive
and importer coverage; live hierarchy edits and structural Undo remain open.

Evidence: `.local/native-rig-references-retail-2/receipt.json`.
Test SHA-256: `a45b03f855466b010ff61d5019f27c445b9a15d38d4eb74d406a29d2aff908a4`.

```powershell
python melee/tools/review_native_rig.py --suite references `
  --exe melee/build/native/native_rig_references_test.exe `
  --tool melee/build/native/native_content_tool.exe `
  --iso 'Super Smash Bros. Melee (USA) (v1.02).iso' `
  --output .local/rig-reference-review
```

The shared review command also supports `--suite animation`; its renamed entry
point passes the full animation cohort again with **48,094,335 checks**.
Receipt: `.local/native-rig-animation-retail-3/receipt.json`.

## Rig-aware animation-bank rewriting

`native_fighter_rig_animation.{h,cpp}` prepares an owned fighter DAT and animation
bank pair from old/new rig facts and a destination-to-source physical part map.
It leaves the source project untouched. The full hierarchy transaction will
publish this pair together with the costume, facts and stable bone-ID table.

Each action/demo descriptor supplies its animation family and 13-bit omission
mask. The writer filters the reserved parts for that mask, derives the matching
animation-node map and rewrites the appropriate trees. Inserted nodes have no
tracks; retained nodes keep their compressed streams and track metadata.
Foreign-family clips retain their own layout. Independent unreferenced trees
keep their tracks; unused symbols aliasing an edited tree follow that tree's
map. The lower-level remapper now accepts one map per root and checks alias
agreement. Its call sites use this interface directly.

Both descriptor tables receive the new physical archive offsets and lengths,
including aliases and entries originally resolved by symbol lookup. Missing
external demo clips retain their records. Existing clip names, flags, command
pointers, tree type, duration and unrelated fighter bytes remain intact.
Original inter-archive padding is retained. Conflicting maps for a shared tree,
invalid part maps, malformed archives and mismatched rig/mask node counts reject
without publishing either output. Omission identity/order and the animation
family are retained across this operation.

`native_rig_animation_test` compares actual serialized tracks and descriptors
after identity, insertion, removal of the inserted bone and reordering. Its
synthetic fixture includes different masked node counts within one DAT, a
foreign-family tree, an independent unused tree, an unused alias, descriptor
aliases, padding and an unresolved demo. Conflicting ownership maps and a
truncated later archive leave outputs empty. It also checks that only resolved
descriptor address/length bytes can change in the fighter DAT.

Results:

- Synthetic bank transaction: **7 cases, 1,932 checks**, passing.
- All **26 supported stock fighter definitions**: **130 cases**, **26,804
  descriptor/clip comparisons**, **48,094,335 checks**, passing.
- Glim's current draft: **5 cases, 1,140 descriptor/clip comparisons** and
  **1,300,003 checks**, passing. The test reads the draft without writing it.
- The original **33-bank** node-remapping corpus remains passing with the
  per-root-map interface: **6,245 trees**, **58,683,586 checks**. The synthetic
  node suite now has **2,503 checks**, including conflicting alias maps and a
  missing root map.
- Both production rewrite units pass strict syntax checks, using the existing
  exception for the original Dolphin matrix header's array-parameter warning.

Evidence:

- `.local/native-rig-animation-retail-2/receipt.json`
- `.local/native-rig-animation-glim-1.log`
- `.local/native-animation-nodes-retail-3/receipt.json`
- `.local/native-rig-animation-strict-1.log`

Reproduce after building `native_rig_animation_test`:

```powershell
python melee/tools/review_native_rig.py --suite animation `
  --exe melee/build/native/native_rig_animation_test.exe `
  --tool melee/build/native/native_content_tool.exe `
  --iso 'Super Smash Bros. Melee (USA) (v1.02).iso' `
  --output .local/rig-animation-review
```

The review evaluates every supported stock definition and reports all failures.
It extracts temporary inputs, exports facts and deletes only its own temporary
directory. It does not create drafts, install content or open the game. This
establishes animation-bank rewriting, not live structural editing, skinning or
editor Undo acceptance. Rook's older user-profile draft has no `assets.fma` and
was not admitted by this source-project test; it was left unchanged.

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

- Synthetic CTest: **2,503 checks**, passing.
- All **33** retail `Pl??AJ.dat` banks: **6,245** trees and **58,683,586** checks,
  passing. Retained descriptor and stream payloads match byte for byte.
- Native game object target compiles with the new implementation and decoder
  declaration. No GUI launch was needed for these archive checks.

Receipt: `.local/native-animation-nodes-retail-3/receipt.json`. The receipt records
the corpus test executable's SHA-256.

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
