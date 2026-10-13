# Source entity paths after ordinary source-package activation

An ordinary source slot deliberately has no `NativeFighterDefinition`.
`nativeFighterRuntime_SourcePackage` and the exact published program/owner are
the data and execution authority. This audit follows the actual direct typed
binding call graph; a residual legacy Definition lookup alone does not show that
an interpreted source reaches that route.

## Corrected reached held-article path

`ftCommon_8007E83C` / `ftGetParasolStatus` call
`nativeHeldArticle_Request` / `nativeHeldArticle_Status`. The dispatcher formerly
returned ordinary policy when the held Article author had no Definition, despite
the prepared Item program containing `held_status`, `held_request`, and
`holder_neutral` code IDs. `ft_08A1.c` also has the existing neutral notification
callsite. Its surrounding Fighter entry policy remains a separate concern.

`native_held_article.c` now branches on the actual Item's `native_script_owner`,
looks up its current immutable spec and logical status, and invokes those code
IDs directly through the Item owner. It retains holder/Item callback generations
and GObj pool references, including scene lifetime and allocation generation.
Status publishes only a typed I32 result from an unchanged live held relationship.
Requests claim the operation even when the authored Bool result is false or the
VM faults/retires. Neutral completion requires the same live relationship.
Absent source status is -1, absent request is claimed, and absent neutral is
claimed with false completion. A missing, faulted, detached or retiring source
owner cannot enable an ordinary native held policy.

Actual console acceptance is 88 checks / 14 cases at
`A:/FreakMelee-test-archives/source-held-article-final-review/receipt.json`.
Source callback bodies, event payloads, storage, execution, GObj allocation/free
and holder slot reuse are real. Immutable specs/assets, native userdata and
position-write mutation boundaries are controlled. The whole Fighter state
caller and gameplay/render parity are outside this receipt. The legacy dispatcher
fixture separately passes 354 checks at
`A:/FreakMelee-test-archives/source-held-legacy-review/receipt.json` with explicit
absent interpreted-owner/pool boundaries.

## Existing direct paths

* `native_script_binding_spawn.c` and `native_script_item_spawn.c` select exact
  prepared source owner nodes, retain caller program/schema and native GObj
  identity, and use `Item_CreateAuthored(spawn, author, owner)`.
  `native_item_behavior.c` selects the source spec/assets before native model
  construction. Root's constructor route bypasses the stock bank lookup for a
  nonzero source owner. The direct factories never call legacy Article creation
  or authored C-table selection.
* `native_script_binding_item.c` reads the actual Item/common attributes and
  invokes shared native physics, sound and lifetime operations. These are common
  engine resources, not donor Fighter behavior. Source Workshop protocol uses
  `native_holdable_protocol.c` and the pinned data-only `spec->recipe`; the legacy
  `native_holdable.c` Definition-based state table is not its source scheduler.
* `native_script_receiver.c` explicitly chooses `nativeItem_Creator` for source
  Items. That creator helper accepts a NULL captured Definition; it does not
  require a non-NULL one. Receiver and factory operations separately validate
  the current exact interpreted view/program/schema and native object identity.
* `native_script_effect_actor.c` owns the source effect update code ID, typed
  storage, creator view/schema, group references and native recipe identity.
  `nativeModelEffect_Resolve`, `Snapshot`, `Creator`, and `Joint` dispatch to this
  owner first. Source group members have no legacy author/policy or authored
  `EF_UpdateFn`. `effect.joint` uses the prepared decoded path/member selector;
  an absent runtime joint keeps the audited shared default/no-op behavior.
* `native_script_binding_presentation.c` checks the selected SourcePackage's
  actual rig/table/data. `native_script_binding_feedback.c` uses actual Fighter
  parts or imported Item model metadata for common engine effects; it does not
  resolve a Definition-owned model effect policy.

Receiver `definition` fields remain an additional native identity observation;
their NULL equality is not the interpreted program authority. The independently
retained source view/program/schema supplies that authority. No dummy Definition
is introduced by these paths.

## Residual advanced helpers and unsupported resource scope

`nativeArticle_Creator` still requires a non-NULL Definition, as do legacy Article
creation, Article reference operations, resource reservations and native model
effect policy spawning. Current direct source binding routes do not invoke those
helpers. Adding a physical-kind-only branch would lose source declaration
identity, especially when multiple articles share one native archetype; this
audit therefore does not invent one.

`native_article_model.c` operates on actual `Item.xC4_article_data` and its named
model/animation records, with no Definition lookup. This does not establish a
source resource schema: `native_script_item_program.c` accepts only the prepared
projectile/attachment metadata and rejects unknown entity metadata. Named model
root/animation/resource declarations do not become supported merely because a
shared native helper exists. Ordinary candidates also explicitly reject Workshop
Item declarations until owned PNG/blueprint resources and non-Single families
until their lifetime/resources are implemented. No full resource or paired-family
closure is claimed.

The main client's source Fighter transition fault and its direct layout guards
are owned by the separate Fighter work; no runtime, Fighter transition or
collision files were changed for this audit.
