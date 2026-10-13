# Ness immutable entity parameters and native Item environment

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

The current read-only Ness audit visits all 418 indexed functions and reports
three first-errors-per-body, down from 10 in the vector/collision batch.
Metadata and body coverage remain complete, but the package still rejects.
This is not an exhaustive task count or a gameplay acceptance result.

## Implementation and native references

`program.entity_params.ENTITY.NAME` selects an immutable declaration from the
executing package. Names and types resolve at preparation, and reads use the
existing pinned parameter table. Article, Item and Effect declarations share
this compile-time namespace. No live entity, creator, recipient, new actor state
or allocation is needed. Mutable entity state remains private, root
`program.attrs` remains root-only, and parameters cannot be written at runtime.
Ness's Yo-yo smash code now reads its Yo-yo tuning through this path.

The native Item environment additions are shared operations:

- `item.hits_stage()` uses `it_8026E058`, matching PK Fire's fresh ground/left-wall
  sweep, position/floor updates and Bool contact/removal result.
- `item.land_on_stage()` uses `it_8026E414`, matching PK Fire pillar's ground,
  wall/ceiling response and synchronous authored `on stage_landed` callback.
  Native writes retain their order; caller identity is checked after callbacks.
- `item.stop_loop_sound()` uses `Item_8026B074`, matching PK Flash's stage-stop
  loop-voice release and `SFX_NONE` handle reset.
- `stage.internal_kind` reads `Stage_80225194`, the current internal `stkind`.
- The next PK Thunder statement exposed `item.release_camera()`. It now uses
  `Item_80267454`, retaining native camera-mode/non-null-subject conditions and
  pointer/flag cleanup. It does not detach creator or call a fighter handler.

No stock Ness behavior handler is delegated, compatibility alias added or
fighter-specific extension introduced. The definitions and examples are in
[program parameters](scripting/program.md), [Item](scripting/item.md),
[terrain](scripting/article_terrain.md) and [stage](scripting/stage.md).

## Evidence

Seven Windows targets build and the script/parameter CTests pass 2/2.
Three controlled cohorts pass 19,219 checks in total:

- 402 checks / 43 cases / 99 actual VM calls prove same-package immutable entity
  reads with two separate banks, type shadowing, exact NaN/signed-zero imports,
  creator retirement and packed restore. The other declared entity has no actor
  or published spec. Mutable reads, writes, unknown scopes, unresolved values
  and mismatched identity/layout still reject without output publication.
- The real-library owner/primary cohort passes 13,512 checks / 124 cases. Four
  actual Fighter helper executions read an Article parameter. It retains native
  rotation/normal/animation comparisons and four Pages restores. Native stage
  getter, absent loop voice and absent camera subject paths run directly in
  the linked library; the latter preserves all four existing mode values.
- The unchanged native-body terrain cohort passes 5,305 checks / 968 pairs /
  198 terrain cases / 22 VM calls. Landing adds 16 floor/wall/ceiling pairs with
  the authored landing hook. Camera release adds eight mode/attachment pairs
  using an explicit controlled subject-release leaf.

The read-only 26-form non-Kirby census retains 24 accepted forms. Ness rejects
with three diagnostics; Samus has no valid independently prepared draft.
The first isolated parameter run found a missing unrelated attachments fixture
unit. A temporary compile-time namespace collided with event namespaces;
existing source-body diagnostics exposed it before acceptance. Both were fixed,
and final current-source runs pass. Failed outputs are retained privately.

Receipts:

- `A:/FreakMelee-test-archives/program-entity-parameters-oct10-c/receipt.json`,
  SHA-256 `e9a4589481d755fc50935ff28c8d0aa8514f40d7e1742abb0f712c29e661cf17`.
- `A:/FreakMelee-test-archives/source-item-entity-environment-oct10-a/receipt.json`,
  SHA-256 `61b8f18d9b6c1ec39217162b09a0ef4b2350075144537c000811b966d7151fc0`.
- `A:/FreakMelee-test-archives/item-landing-camera-oct10-a/receipt.json`,
  SHA-256 `b8170dc444d4956f61f7a55d5036365ec6fb8f36e84a3b5acaca6e153b4b8a4b`.
- `A:/FreakMelee-test-archives/source-roster-entity-environment-oct10-a/receipt.json`,
  SHA-256 `d385b182115fc168c6c57e7aef5d72a1f7f963902cf28a1062c07723410930cb`.
- `A:/FreakMelee-test-archives/ness-entity-environment-oct10-final/receipt.json`
  independently verifies current and frozen code, assets, binaries and audits.

## Limits and next work

The terrain cohort controls stage sweep/impact dependencies and camera-subject
release; it does not prove the full stage solver or camera manager. The linked
library tests do not play an active audio voice. PK Fire contact has native
source/build coverage, not new live-stage acceptance. Typed imports in the small
parameter cohort are supplied at preparation; it does not itself decode DAT.
No cohort proves full Ness construction, gameplay replay parity, rendering/audio
or whole-match rollback.

The remaining source diagnostics are dynamic authored child selection and two
retained resource-joint animation removal paths. Do not weaken their ownership
or graph admission requirements to accept the package. Further preparation can
reveal additional errors once those bodies compile.
