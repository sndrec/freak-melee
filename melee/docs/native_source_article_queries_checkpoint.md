# Direct interpreter: Article state and shared queries

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

The remaining four API gaps found in Game & Watch's Article bodies now execute
through the direct interpreter. The APIs expose shared operations; scripts still
own motion selection, attachment policy, animation timing and wind decisions.
No stock Game & Watch handler is called by these bindings.

| API | Preserved contract |
| --- | --- |
| `item.airborne = Bool` | Assign native `GA_Air`/`GA_Ground` on an exact source-owned Article. Do not change motion or run physics. Workshop holdables explicitly reject assignment because their getter uses motion state. |
| `item.joint_world_position(0, "path", x, y, z)` | Prepare literal primary-model paths once. Execute `lb_8000B1CC` for a present joint. Missing root/path publishes three zeros. Stage all outputs until source, storage, graph and selected-joint checks succeed. Reject overlapping output destinations at compilation. |
| `effects.wind_at_position(...)` in an Article | Use copied world coordinates and the actual shared wind pool. No Fighter creator or model required. Preserve native exhaustion/no-op behavior and completed wind side effects if continuation later fails. |
| `targets.smash_charge_active(handle)` | Read `smash_attrs.state == 2` from a captured live physical Fighter. Permit foreign/stock targets; missing targets return false. Require integral full-width handles and retain caller/target identity through publication. |

Primary-model selectors are bounded immutable recipes owned by the prepared
Article, not per-tick parsed paths. Secondary slots remain explicitly unsupported
by this query until their actual resource lifecycle is implemented. Existing
joint transform properties and model controls keep their previous admission.
The Item model invocation accepts mutable typed argument records because it now
publishes output formals; the direct caller and absent console boundary were
updated together without a compatibility entry point.

## Source preparation

`A:/FreakMelee-test-archives/source-article-api-census-oct09-final/receipt.json`
pins the rebuilt tool and ten full templates. Game & Watch now prepares with
364 functions, 80 imports and 6,492 instructions. Its world query uses bare
literal `0`, as required by the prepared selector contract, instead of `int(0)`.
Mario, Dr. Mario and Zelda continue to prepare unchanged. Peach still stops at
recipient exported calls; Link/Young Link, Sheik and Popo/Nana still stop at their
secondary-resource metadata boundaries.

This is complete template preparation, not complete fighter gameplay or asset
activation acceptance. Tests do not launch a game or claim rendering parity.

## Grouped evidence

The native client, content tool, reference exporter and selected targets build.
All 40 selected integration CTests and three reference CTests pass. Focused
receipts passed against the final integrated sources with unchanged fingerprints:

- `A:/FreakMelee-test-archives/script-item-airborne-oct09-integrated/receipt.json`:
  278 checks, 50 cases, 12 whole-Item native field comparisons and four VM runs.
  Actual outer property routing, compiler, VM, typed storage and GObj lifetime
  guards execute. Article assets and actor Spec/View/Status publication are
  controlled; the native constructor/scheduler and gameplay are not tested.
- `A:/FreakMelee-test-archives/script-item-world-position-oct09-final/receipt.json`:
  2,594 checks in the expanded existing model suite, including six native
  coordinate comparisons, four world-query VM cases and 15 runtime rejection/
  continuation cases. The suite retains its prior 151 native model comparisons.
  Complete `lb_8000B1CC` executes; matrix construction is a controlled leaf.
  Missing paths/root, signed-zero/nonfinite coordinates, invalid formals,
  malformed literal recipes, source/graph/schema replacement and withheld output
  are covered. Duplicate outputs, wrong types, non-writable expressions,
  secondary model selection and malformed paths are compilation rejections.
- `A:/FreakMelee-test-archives/script-article-wind-oct09-integrated/receipt.json`:
  Article suite: 89 checks, 41 cases, 12 exact native wind-pool comparisons and
  four VM calls. The eight-record pool's fields and free/active ordering are
  compared through exhaustion/reuse. The existing Fighter counterpart also
  passes 67,432 checks, 137 cases and 52 retirement mutations. Actor/VM/storage/
  fixed-pool and complete native wind kernels execute; assets and unrelated
  engine leaves are controlled. The native wind kernel has no callbacks;
  post-kernel mutations are explicitly adversarial probes.
- `A:/FreakMelee-test-archives/script-target-smash-charge-oct09-integrated/receipt.json`:
  268 checks, 16 native comparison cases, 16 Fighter/Article VM queries and 14
  identity mutation cases. Actual `ftLib_800876D4` and the reference target
  helper execute. Package/rig facts are controlled; pure-query mutations are
  explicit guard observations, not invented native callbacks.

Selected JObj identity checks use pointer, class, extent and retained graph
ancestry. They do not add an independent JObj allocator-generation proof.
Whole-match snapshots, imported asset construction and gameplay remain separate
acceptance boundaries.

## Next work

Implement recipient-owned exported calls from the
[recipient dispatch plan](native_source_recipient_export_plan.md), then actual
[secondary-resource construction and ownership](native_source_secondary_resource_plan.md).
Keep source preparation, native operation comparisons and whole-fighter gameplay
acceptance distinct. The overall project goal stays active; commits remain local.
