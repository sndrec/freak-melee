# Interpreter catch hooks, Item terrain and follower queries

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

The interpreter now enters common catch/capture paths at their original native
decision points, runs Item ground-following and landing responses with authored
stage hooks, and exposes the target/terrain observations needed by follower
scripts. Fighter-specific decisions remain in source. The bindings call shared
engine kernels directly and introduce no donor-handler compatibility aliases.

## Catch and capture timing

Ten typed hooks cover catch permission, start/frame/exit, accepted CatchWait
release, pull anchor/follow/readiness, captured-state notification and air-tether
input. A declared false result replaces that fighter's policy; it does not ask
the engine to run a donor handler. The common held-item throw priority, input
checks and motion/setup ordering remain intact.

`capture_wait_enter` executes in the holder's program after the caught Fighter
has entered its high/low wait motion and completed common setup. Its scoped
`other` denotes the caught Fighter. Pull, pummel, throw and caught-state
continuations retain both physical participants and their reciprocal relation
across authored callbacks. Fault, retirement, program/storage replacement or
slot reuse stops subsequent cached native writes. Completed side effects are
not undone. The authored throw-animation branch retains its original ownership
of the selected victim motion rather than subsequently applying stock mapping.

See [catch policy hooks](scripting/catch_hooks.md) and
[attached captures](scripting/capture.md). These hooks do not by themselves
complete an authored tether move or add every fighter-specific throw policy.

## Item terrain and effect recipes

`item.follow_ground()` uses the actual floor-following kernel and dispatches
`on stage_left` when it loses the floor. `item.bounce_to_ground()` and its
Bool `reset_contacts` overload use the common native air/landing kernels;
`on stage_landed` runs when they settle. The Bool overload rejects numeric
arguments. Nested hooks run through the existing synchronous VM, using a
temporarily linked retained stack invocation rather than an allocated callback
record. The entered-air continuation also checks its retained Item identity
before its own cached write; an outer binding guard alone would be too late.

`effects.spawn_with_scalar_at_position(id, x, y, z, scalar)` admits audited
native recipes consuming a copied Vec3 followed by a copied Float. The scalar
can mean facing, rotation or scale according to the recipe. Native creators,
root/model requirements and actual pointer argument order remain explicit.
Link and Young Link use this generic spelling instead of treating every recipe
as a scale operation. Popo/Nana's contact-reset calls use an actual Bool.

See [Item ground following and landing](scripting/item_terrain.md) and
[position/scalar effects](scripting/joint_feedback.md#effectsspawn_with_scalar_at_positioneffect-x-y-z-scalar). Engine
geometry, Item attributes and imported model graphs remain prerequisites;
native terrain and effect kernels may perform sound/effect operations and
allocations. No allocation-free whole-engine claim follows from the binding.

## Follower observations and stage queries

- Target damage, facing, cached CPU body half-width, sleeping state and floor
  line are source-only queries available to Fighter and Article scripts. They
  accept stock or independent source targets and preserve native Float bits.
- `ground.mid_walk_point` reads the common middle-walk attribute, including on
  retained stock and independent Fighter receivers. It neither moves the
  Fighter nor creates a new attribute-storage copy.
- Floor probes and 2D/3D line-island bounds use distinct typed output
  destinations and publish only on success. A miss preserves destinations;
  nonfinite floor inputs produce a normal miss without probing.
- Same-island, CPU floor exclusions and island-hazard assessment call the
  actual common kernels. Bounded physical Item-registry admission rejects
  stale/cyclic links and invalid hazard floor indices before native traversal.

See [targets](scripting/targets.md), [stage terrain](scripting/stage.md) and
[Fighter observations](scripting/fighter_observations.md). Existing engine
geometry and valid allocated actor payloads remain prerequisites.

## Current validation and remaining work

The client, content tool, source-assets and selected integration targets build;
all 16 selected CTests pass. A full retained-source check exposed two empty-name
overload entries incorrectly initialized with null names: failed lookup entered
`strlen(NULL)`. The catalog now explicitly stores empty strings for those
internal overloads. Nana source checking reports its next unsupported API
normally rather than crashing. The rebuilt check and integration suite pass.

Grouped console evidence already independently rehashed by root:

- Catch policies and participant lifetime guards:
  `A:/FreakMelee-test-archives/script-catch-policy-oct10-final/receipt.json`:
  6,844 checks, 160 native comparisons, 59 retirement/fault cases and three
  throw-mapping regressions; all 1,714 input pins match.
  SHA256 `3da997e9d427cc07482e5b8b60dc473d0701fe35773b2635e98d61dd1954db6a`.
  Actual compiler, actor and VM paths execute selected catch kernels against
  the previous checkpoint. Motion delivery, presentation and capture geometry
  leaves are controlled; full tether gameplay remains outside this proof.
- Item terrain and connected positional effects:
  `A:/FreakMelee-test-archives/script-item-terrain-oct10-final/receipt.json`:
  1,383 checks, 64 full-Item native comparisons, 16 actual VM calls and ten
  native effect calls; all 1,716 input pins match.
  SHA256 `75fef56ca974a824a1ea081a7d87d8e11b3ca91281f0ee4ec8faa150bb72ee60`.
  Nested stage hooks, authored faults/removal and userdata substitution run
  through the actual VM. Stage intersections, impact/surface response and
  particle/model allocation leaves are controlled.
- Effect argument-family audit:
  `A:/FreakMelee-test-archives/script-position-scalar-oct10-final/receipt.json`:
  393,552 checks across 65,536 IDs and 48 native helper-copy cases;
  all 1,584 input pins match.
  SHA256 `a9d231f6b311cbdbbe7eae7a8308cd521eb3b441c3e081c188f1b0d620c9113d`.
  The 34 new position/scalar recipes include one creator-root requirement,
  one Fighter-parent requirement and fourteen native model constructors.
- Target observations:
  `A:/FreakMelee-test-archives/script-target-scalars-oct10-final2/receipt.json`:
  4,791 checks and 686 native comparisons; all 1,699 input pins match.
  SHA256 `47869cd63c0ae3f56e7ba6c21cba44dc0faeae47013cc5e2876185e90a2f9866`.
  Actual Fighter and Article callers observe stock and separately compiled,
  independently published source targets.
- Middle-walk/retained observations:
  `A:/FreakMelee-test-archives/script-midwalk-observations-oct10-final2/receipt.json`:
  9,082 checks, 1,689 native comparisons and 742 VM calls, including 480
  receiver calls; all 1,696 input pins match.
  SHA256 `1adf06841a7d768b2f3c943d07334a453235219aa8c33768a9994f2d8253e493`.
- Stage terrain queries:
  `A:/FreakMelee-test-archives/script-terrain-queries-oct10-final3/receipt.json`:
  6,791 checks, 896 cases, 752 native comparisons and 88 VM calls;
  all 1,737 input pins match. The cohort includes 65 negative compiler cases
  and descriptor-name coverage for the overload-lookup regression.
  SHA256 `0ec801cb76a6cdccd6563beeb0aaf1d7b1c45b6e931b72b1cd7af273e0945860`.
  All thirteen extracted terrain kernels match their unchanged bodies at
  `50f2b476b`; segment intersections, bounding state and dynamic response
  leaves are controlled. This is not a complete stage-solver comparison.

Fixture-controlled package publication, common setup and engine leaves remain
documented in their READMEs; these counts do not imply whole-fighter gameplay.

The ten-source census is
`A:/FreakMelee-test-archives/source-catch-terrain-query-census-oct10-final2/receipt.json`,
SHA256 `7a712c989a3655bcdeeddfbd8c0812d490169e0193ea837ab9937fa5b15262dd`.
Root independently rehashed the actual content tool and all ten full sources.
Mario, Dr. Mario, Peach, Game & Watch, Zelda and Sheik still prepare with their
complete retained source bodies. Next diagnostics are Link 846 / Young Link
850 (`capture.mask`), Popo 1489 (Article `floor.normal.x`) and Nana 1474
(`items.reject_healing_pickup`). No source body was removed to obtain these
results.

Preparation is not whole-fighter acceptance. Complete tether policies,
simultaneous companion activation/member mapping, resource-animation
application and gameplay acceptance remain unfinished. Compact whole-match
save/load still uses broad Regions/Pages as its oracle. The goal remains active.
