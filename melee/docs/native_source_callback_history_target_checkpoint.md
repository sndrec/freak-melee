# Interpreter accessory phases, CPU history and target delta

The interpreter now owns Melee's accessory-before and accessory-hitlag slots,
the fixed CPU input-history operations, four shared CPU planners, five output
fields, and target position-delta reads. Fighter-specific choices remain in
scripts. Common engine kernels are called directly.

## Callback timing and ownership

`callbacks.accessory_before(helper_or_None)` uses native slot 2, before
translation and accessory slot 1. `callbacks.accessory_hitlag(helper_or_None)`
uses native slot 3 during hitlag, after which this scheduler returns. These are
distinct from the later accessory/cue phase. Helpers are same-program,
zero-argument Void functions. The two registrations survive ordinary motion
changes, clear on NewLife with the corresponding native pointers, and are
saved/validated through the existing instance snapshot schema.

`callbacks.matches(Callback.accessory_before, helper_or_None)` and its hitlag
equivalent observe the actual registration without mutating it. Explicit native
assignments take over at dispatch. Link and Young Link use these generic queries
directly, including the already available `Callback.death1` query; their cleanup
leaves replacement handlers intact. The owned native scheduler checks retained
Fighter/program/storage identity before continuing after a callback.

See [callback slots](scripting/callback_slots.md) and
[motion callbacks](scripting/motion_callbacks.md).

## Shared CPU and target operations

- `cpu.record_input(id)`, `apply_recorded_input()` and `read_history_pose(...)`
  use the existing 30-record native ring and cursors. No second history pool or
  per-tick allocation was added. Creation IDs remain exact Int values; pose
  outputs must be four distinct Float destinations and publish only on success.
- `prepare_combat_plan()`, `update_recovery_destination()`, `plan_attack()` and
  `plan_without_target()` call the actual shared kernels. The script owns
  follower permission, recovery state and whether to consume CPU phases.
  Bounded physical registries and actually dereferenced cached peers are retained;
  equality-only caches and targets overwritten by refresh are not overvalidated.
- `cpu.output.left_x/left_y`, trigger bytes and buttons are editable. Byte
  values outside their native ranges leave the previous output unchanged;
  buttons preserve all 32 bits. These fields are source-only.
- `targets.read_position_delta(id, x, y, z)` copies the live native delta,
  including Float bits, into distinct outputs on success. Stock and independent
  source targets are supported. The shared target selector now diagnoses a
  nonphysical/stale registry link before following it and bounds cyclic scans.
  Valid engine-allocated Fighter userdata remains a registry prerequisite.
- Popo and Nana's article cleanup now uses `creator.exists` followed by
  `creator.helpers.forget_ice(id)`. The existence gate preserves silent missing
  creator behavior; the helper requires the matching owned creator program.

See [CPU history](scripting/cpu_history.md), [CPU policies](scripting/cpu_policies.md)
and [target queries](scripting/targets.md).

## Verification and remaining work

The client, content tool, source-assets and selected integration targets build;
all 16 selected CTests pass. Grouped console comparisons exercise actual
compilation, actor/VM dispatch and native scheduler/CPU/query bodies. Controlled
geometry, package facts, native setup leaves and allocation prerequisites remain
explicit fixture boundaries. No game, GPU or audio device was opened.

Final grouped evidence (receipts outside the repository):

- `A:/FreakMelee-test-archives/script-callback-accessory-oct10-final/receipt.json`:
  818 checks, 16 retained velocity comparisons, 16 accessory scheduler
  comparisons, and 19 accessory cases. Receipt SHA256
  `b31ceb92773f64619713e02ccefa7666f7bc557b61626f885798a78d0e869823`.
- `A:/FreakMelee-test-archives/script-target-delta-oct10-final/receipt.json`:
  1,644 checks, 326 native comparisons and 257 actual VM output queries.
  Receipt SHA256
  `ad5002e01dae1e29b8efd915b84f57621d52b2766b598d2dedb977dc06e040ce`.
- `A:/FreakMelee-test-archives/script-cpu-history-planners-oct10-final3/receipt.json`:
  connected cohort: 31,129 checks, 1,277 cases, 3,922 native comparisons and
  1,559 VM calls. Retained command/selector cohort: 7,251 checks, 612 queue
  comparisons and 80 selector comparisons. Receipt SHA256
  `84ea652c09cde40971eb90c198dd3cf5e15cbb1382041cfb4bc5b7be2ba67de2`.

Root independently rehashed all 1,705 callback inputs and all 1,696 target
inputs without a mismatch. Native callback bodies and geometry dependencies
are controlled where their fixture READMEs specify; these counts do not prove
whole-character gameplay or paired publication.
All 1,710 CPU receipt inputs also rehash without a mismatch. CPU comparisons
run actual history and planner bodies; geometry, RNG, deep decisions and target
selection leaves are controlled. Bool-false output checks distinguish domain
scratch preservation, central typed scratch transport and actual VM
SuccessOnly destination publication.

The rebuilt tool still prepares the retained full Mario, Dr. Mario, Peach,
Game & Watch, Zelda and Sheik sources. Next first diagnostics: Link 836 / Young
Link 840 at `effects.spawn_scaled_at_position`, Popo 1468 at `item.follow_ground`,
and Nana 1381 at `ground.mid_walk_point`. All source bodies remain present.
The ten-source census is
`A:/FreakMelee-test-archives/source-callback-history-target-census-oct10-final2/receipt.json`,
SHA256 `6eb6211385e18699efb10efe08af23f0b5d51d58edfc2d6632cf7e3000380c4a`.
Root verified the rebuilt tool and all ten source hashes against this receipt.

Preparation is not complete fighter acceptance. Catch-policy hooks, additional
item and stage/CPU queries, public resource-animation application, actual
simultaneous companion activation/member mapping and whole-fighter gameplay
remain unfinished. Compact whole-match save/load still uses broad Regions/Pages
as its oracle. The full goal remains active.
