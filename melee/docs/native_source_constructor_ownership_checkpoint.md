# Interpreted fighter construction and teardown ownership

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

This checkpoint corrects the actual native construction paths before admitting
simultaneous custom companions. It does not open the companion activation gates
or claim complete paired gameplay or compact whole-match snapshots.

## Creation and failure

`Fighter_Create` retains the original GObj and Fighter pool allocations before
callbacks. Failure to attach an interpreted actor, enter a required stock Load
callback, complete reset, or retain the original actor through startup now
returns NULL after tearing down that still-owned allocation. A faulted source
needs cleanup; a replacement that reused its address does not belong to the
failed constructor. Successful reset may publish a new logical life, while
the original physical owner, program, data and persistent storage must remain.

The `native_constructing` field belongs to that actual allocation. It remains
set throughout Load, reset and startup, including callbacks that destroy their
own source, and clears only after successful callback-free final bookkeeping.
No transient owner registry or alternate constructor adapter was introduced.

`Player_80031AD0` still publishes the primary before creating the secondary.
The secondary CPU initializer can therefore read the primary's history, as in
the original code. A failed primary prevents secondary creation; incomplete
pairs unwind secondary before primary. Validation also follows the final
Player callback. Replacement publications survive failure cleanup.

`Player_80031FB0` now requires the expected original object at every callsite.
It follows that object across the two transformed roles and leaves replacements
and the transformed mapping intact. Slot state clears when both physical roles
are empty; a surviving replacement keeps even a single-member slot active.

## Partial teardown and creator children

Unload retains physical ownership across stock/script Remove, article resources,
Item interactions and effect destruction. A camera is optional before Load has
finished. Existing `efLib_DestroyAll` runs before supporting joints are released.
Manual transplantation of arbitrary userdata into a live GObj is outside the
supported engine contract; `GObj_InitUserData` already refuses that operation.

Normal Item teardown remains selective. Only unfinished construction additionally
abandons Items whose immutable creator allocation and current damage owner both
refer to the original fighter. Transferred, reflected, unowned and borrowed Items
survive. The creator stamp now includes its physical GObj pool reference, so an
old Item cannot acquire a different creator merely because an address is reused.
Existing logical creator-generation rules remain in force for normal gameplay.

This failure sweep does not compare against the fighter's current spawn ID:
Load can create children before reset changes that ID. Resource attachment
cleanup likewise matches the original physical owner, and detaches Load-era
borrowed pointers before freeing their graphs. Rig, parts and schema checks
remain required. The common Item initialization boundary refuses another child
of an unfinished fighter already being removed, including Item-parent creation;
the cleanup does not rely on an unbounded drain loop.

## Actual GObj destruction

Removal hooks retain their live userdata during execution. Recursive Free or
RemoveUserData cannot execute the same hook twice. Standalone userdata removal
honors a nested Free request after its hook returns; scheduler-delayed deletion
keeps its existing order. The final physical free uses the captured allocation
reference. Native callbacks that retire and rebind the scene cannot make the old
removal clear userdata or unlink a same-address object from the next scene.

The three destruction flags are in the existing pooled payload, covered by
its packed snapshot and the broad global-region oracle. Allocation resets them.

## Verification boundaries

The connected constructor fixture executes complete source-exact Create,
InitReset, ProcessDeath and Unload bodies with the real compiler, interpreted
actor, storage/VM and GObj/Fighter pools. It also executes the actual two-list
effect destruction body. Asset/model/camera/audio/motion leaves and effect
allocation/generator/destructor dependencies remain controlled. Its Item and
resource calls are controlled wiring boundaries; populated ownership has its
own connected fixture rather than being claimed by those stubs.

The separate Player fixture executes the complete actual Player constructor and
role-clear bodies, real actors/pools, immutable family lookup and the native CPU
initializer. Its Fighter factory is controlled. All thirty CPU history records
are compared byte-for-byte with independently expected native records.

The GObj fixture links complete actual HSD lifecycle files, including recursive
and deferred removal, peer replacement, physical reuse, scene rebinding and
global-region restoration. The scratch fixture retains the inaccessible legacy
animation buffers and verifies current selectors/track attachment/Unload never
touch them. The Sleep corpus now excludes its obsolete constructor-tail fragment;
whole constructor coverage supersedes that fragment, while real Sleep/death
continuation cases remain.

All focused executables are hidden consoles with no game, GPU or audio launch.
Receipts pin the compiler, source, fixtures and headers and reject input drift.

### Final evidence

The grouped client/content-tool/source-assets build succeeds. All seventeen
selected integration CTests pass, including the current Item owner backend.
Fifteen edited production/build inputs retained their hashes across that build.
The historical Item owner test no longer extracts an obsolete complete factory:
its factory leaf is explicitly controlled, while pickup/scale kernels remain
source-exact. The connected child-ownership cohort separately exercises the
current factory's common admission prefix.

| Connected boundary | Final cases | Checks | Receipt directory under `A:/FreakMelee-test-archives/` |
| --- | ---: | ---: | --- |
| Whole Fighter constructor/reset/death/unload | 27 | 1,619 | `native-fighter-constructor-oct10-final` |
| Player pair constructor and all thirty history records | 66 | 5,805 | `script-pair-constructor-oct10-final` |
| HSD recursive/deferred destruction and pool reuse | 16 | 20,734 | `native-gobj-recursive-oct10-final` |
| Legacy animation scratch isolation | 73 | 667 | `fighter-animation-scratch-constructor-oct10-final` |
| Sleep/death continuation corpus | 52 | 812 | `script-companion-sleep-constructor-oct10-final` |
| Healthy Fighter pool consumer | 10 | 1,650 | `native-fighter-pool-consumer-constructor-final` |
| Genuine resource graphs and old-life attachment cleanup | 12 | 2,654 | `script-item-resources-constructor-oct10-final` |
| Failed-constructor children and actual Item teardown | 10 | 889 | `script-constructor-items-oct10-final` |

The respective `receipt.json` SHA256 values are:

```text
fc915f8ecfe3e25ef2967d4763d7eb7affb7b2c30b33e0bff3069343cebf7d84
1810fccff19085d2e73da86ba99544a32e253fb200d1d0f27598e01cb4fd15e8
2c1a8770d5ee236f51604171ec3d17f7c47d0a69f44d1e48e9da2959c0e17185
265366fec67fbba5c4f0588985c9a16817d472814454b00fca4645ba9770891d
890f129aad42eba8bb432d96cf555fd860c87893e3ec7635d2e1b786f0bbdb8e
149a22a1a92beaeb140fb070ad8d069322d32f154a73b7c441133401dadafdae
f0defb7ae1e81d7c0cdbb3b080cfc9410d3cf1279f72b73ff6a5bf7775240334
c36f4afbbe1105c6260051118945177fd89671172cb3c6e63d4e61821469d7eb
```

Each receipt's consumed compiler/source/header/fixture inputs was independently
rehashed. A rebuilt content-tool census also passes actual package preparation
for all ten retained complete sources (Mario, Dr. Mario, Link, Young Link,
Peach, Game & Watch, Zelda, Sheik, Popo and Nana). Its receipt is
`source-constructor-final-census-oct10/receipt.json`, SHA256
`33a4f62d14747f77958ab1259bada6f5a1474f27742107cf754dd3715c663833`.
Preparation does not create actors or establish complete gameplay parity.

The resource cohort includes two immediate/deferred failed-constructor cases,
twelve genuine imported Arrow graph loads, all borrowed-pointer detach kernels
and exact malformed-reference refusals. Its attachment rig is a controlled
two-joint graph; stage geometry/effect leaves are explicit. Animation controls
use a static root without active tracks. Its bounded Regions/Pages restoration
does not establish whole-match save/load or performance.

The child cohort runs actual Item removal, Fighter Unload, pool ownership,
creator stamping and authored Load/Remove/Interaction/Destroyed callbacks.
It removes both Load-era and current children of failed constructors, preserves
transferred/unowned/borrowed Items and rechecks callback-driven transfer/reuse.
Load self-retirement and an actual interpreted divide-by-zero fault also unwind
their published children. Common Item factory provenance/admission is literal
source; later model/attribute/ECB initialization and payload construction remain
controlled. It is not a full Item factory or whole-Fighter gameplay proof.

## Remaining acceptance

Simultaneous preparation and activation remain closed pending integration with
the real custom package admission path and four-port/eight-actor gameplay.
Imported rigs, controllers, rendering and whole-match rollback are not accepted
by these bounded ownership proofs. Compact whole-match save/load remains
unfinished; Regions/Pages is still the broad oracle. There is no new performance
claim or change to the interpreter backend.
