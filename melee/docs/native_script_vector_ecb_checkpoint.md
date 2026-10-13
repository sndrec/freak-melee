# Vector and Item collision-box integration batch

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

The multi-error roster audit identified two shared missing capabilities behind
Bowser's three failing bodies: normalization and Item collision-box extents.
Both now run through ordinary interpreted bindings. This batch also replaces
Jigglypuff's three Fighter-side `hitbox.enabled` queries with the existing
`hitbox.state` contract; native Rollout tests for `HitCapsule_Disabled`, rather
than requiring one particular enabled state.

`vectors.normalize` and `vectors.normalize_xy` accept three writable Float
destinations in Fighter and Item contexts. They use the original `lbVector`
functions, stack storage and generic VM input/output publication. XY leaves Z
unchanged. Literal/Int destinations reject; retiring the caller prevents output
publication. There is no new normalization formula, epsilon or nonfinite clamp.

Item `ecb.up/down/front/back` properties read/write the same `ecb_source`
fields assigned in `itkoopaflame.c` before stage collision. Fighter world-space
ECB points remain separate read-only observations. No item collision callback
or fighter-specific behavior is moved into these bindings.

## Verification

- Native vector/component and parser-to-VM comparison: **7,973 checks / 2,592
  native pairs**, including 32 new normalization pairs. Exact F32 results,
  zero/nonfinite behavior, local writeback, destination rejection and guarded
  publication pass. Caller lifetime availability is controlled in this numerical
  fixture; it does not prove a live gameplay actor lifecycle.
  `A:/FreakMelee-test-archives/script-normalization-oct10-b/receipt.json`
  SHA-256 `d715b2e36140cfac1023a5fda08e214e99e81aba599e313c792da641e02883ca`.
- Real Item property dispatch and native Item regression: **8,144 checks**,
  1,664 physics pairs, 96 sound pairs and 128 VM ticks. Four source-box fields
  are compared against whole-Item native assignments across six values;
  wrong-context/stale-generation reads and writes reject. Existing Item VM
  scenarios continue to pass. Scene search/audio leaves are controlled.
  `A:/FreakMelee-test-archives/script-item-ecb-oct10-final/receipt.json`
  SHA-256 `267c964ff420f3fbe8e71fc36b5ccabcbf56b4b9989f06c4860c7efb6da54e4d`.
- Fresh whole-roster read-only audit: all **164 Bowser bodies pass**. His package
  still rejects the unmigrated `throw_victim_motion` scheduler; preparation
  remains **16/26 forms**, not 17. Jigglypuff still reports 23 first-per-body
  errors (Rollout now reaches attack-color cleanup), Mewtwo 11. Remaining body
  diagnostic count is 34, with structural/hook/resource failures tracked
  separately. Source compilation is not full clone gameplay parity.
  `A:/FreakMelee-test-archives/source-full-roster-audit-oct10-math-final/receipt.json`
  SHA-256 `96ee33f81b1fee8750a7f7ef833bcdba49cd9c9d8a4485ed005916d054f86e0b`.
- Seven Windows targets build, `native_fighter_script` CTest and diff check pass.
  No visible/audible game launch and no installed-content change.

Next cohorts remain explicit callback scheduling, presentation primitives,
Fighter multijump policy and owned entity/import metadata. Continue using the
[batch audit](native_script_multi_error_audit.md) before implementing each cohort.
