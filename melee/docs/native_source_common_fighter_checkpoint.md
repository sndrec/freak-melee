# Shared fighter interpreter primitives

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Implemented 2026-10-08. The dependency inventory's common movement,
presentation and field-observation group now executes through the direct
interpreter. No fighter-specific handler or generated-C route was added.
Public contracts are in [shared fighter reference](scripting/common_fighter.md).

## Native contracts

Movement invokes the actual threshold-facing, force-air/use-jumps,
movement-to-ground and soft-platform test routines. They preserve player/
knockback/ECB/jump and landing-callback ordering rather than synthesizing a
state transition. The new units validate the physical pool reference/full epoch,
generation, selected package/program and owned actor/storage/parameters, and
recheck before output publication after native reentry.

Visibility writes the native invisible flag; group selection uses the actual
prev/dirty update and color selection uses the native priority rules. All supplied
integral indices retain width/sign before validation. Model alternatives must
belong to an active owned channel and fit signed8 (-1 or0..127); an absent
alternative in another channel still hides it. The selected variant/fallback,
outline lookup, model/import spans, actual color tables and current overlay
indices are checked before mutation. These are native rendering controls, not
material overrides or immediate forced model evaluation.

Common observations expose terminal velocity, smoothing, instance/native model
scale, separate ceiling/left/right wall normals, current ECB offsets and full
unsigned spawn identity. Pickup policy is a Boolean property. Native model scale
preserves Y-scale times common model-scaling arithmetic. Facing sign follows
ftGetFacingDirInt, including zero/-zero/NaN behavior, so facing-relative ledge
checks do not truncate fractional facing into mode0.

Canonical Zelda/Sheik sources use property assignment for pickup policy,
`facing.sign` with the existing ground/ledge query, explicit raw-joint root-X
rotation, basic landing, the correct cue0 consume operation and retained
`creator.model.scale`. The distinct false Item stage-collision branch was **not**
rewritten to the existing true-only operation. Article factories/signals,
position-payload effects and Chain resources remain separate missing families.

## Evidence and limits

Receipts under `A:/FreakMelee-test-archives/`:

- `script-common-movement-settled-root/receipt.json`: 1,916 checks, 229 cases,
  122 native comparisons and100 identity mutations. Actual native helper chains,
  binding/guards/pool/lifetime; platform/floor, player/knockback and landing
  callback leaves are controlled. No stage geometry or full gameplay acceptance.
- `script-common-visuals-settled-root/receipt.json`: 7,430 checks and1,701 complete
  Fighter comparisons against shared native statements. Imported/model/selected
  views and native lower color leaves are controlled. Reentry/lifetime injection
  is test-only; those primitives do not currently call scripts themselves.
- `script-common-catalog-settled-root/receipt.json`: 179 frontend/VM/catalog checks;
  all seven calls reach deliberately rejecting capture leaves. Wide signed/
  unsigned selectors retain their values. This is not native operation acceptance.
- `script-common-properties-settled-root/receipt.json`: 32,829 checks /9,406 property
  observations. Actual configured frontend/VM/property units; model-scale and
  facing comparisons use extracted native getters. Distinct surface offsets,
  identity width, pickup assignment, nil data, wrong context, readonly writes
  and retired outputs are exercised. Existing engine/actor/asset leaves remain
  controlled or explicitly unavailable.
- `source-common-owned-preflight-oct08-second/receipt.json`: ordinary source-check
  accepts a two-helper/68-instruction common surface against both actual copied
  Zelda and Sheik data. `source-common-recipient-preflight-root/receipt.json`
  accepts a retained creator model-scale query inside each actual Article owner.
  The invalid fighter-self creator example was rejected, as required. These are
  owned candidate/import/frontend checks, not native tick or recipient execution.

The client/tool and affected fixtures rebuild, with36 selected regression CTests
passing (`.local/source-common-unified-build.log`,
`.local/source-common-ctest-final.log`). Full form source gates remain Zelda's
Article signal at145 and Sheik resources at1543; evidence is
`source-form-template-common-preflight-oct08-root/receipt.json`.
Original drafts/installed content unchanged; no game was launched.

Continue the [form dependency inventory](native_source_form_binding_inventory.md)
with owned callback identity queries, position/Article-root feedback and connected
Article factories/references/recipient signals. Chain must then have its complete
owned resource/model/collision/matrix/lifetime surface; accepting metadata alone
does not make it usable. Full paired gameplay, companions and complete explicit
mutable rollback state/performance remain open.
