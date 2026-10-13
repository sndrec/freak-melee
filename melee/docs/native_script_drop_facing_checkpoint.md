# Interpreted capture drop and animation-facing observation

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Date: 2026-10-10

## Change

`capture.drop_victim()` now executes through the ordinary interpreter. It uses
the original `ftCo_800DC920` unlink operation, retains the original pair across
that operation, then enters the original victim's ordinary Fall state. It
does not apply throw damage. Missing victims are no-ops; a present victim must
have an established reciprocal capture relation. Actor/program/storage changes
prevent result publication and subsequent VM statements.

The common unlink body is unchanged. Its attachment-role swap, model restoration,
floor lookup/connectivity/height checks, collision fallback and position updates
remain intact. Its `Fighter_UnkSetFlag_8006CFBC` helper marks `x221A_b1` when
hitlag is active; it does not invoke the hitlag-end callback in the adjacent
`Fighter_8006CFE0` function. No speculative callback guards were retained there.

`facing.animation_direction` reads the native `facing_dir1` Float through the
existing Fighter observation module. It is read-only and independent of logical
`facing.direction`. It does not compute a new orientation or transform a model.

## Evidence

- Seven configured Windows targets build; reference `native_fighter_script`
  CTest passes. No installed assets changed or game windows opened.
- Capture suite passes **38,492 checks / 5,211 cases**. This adds
  **2,390 checks / 72 cases**: 58 native/interpreter drop comparisons, three
  actor-generation/program retirement cases during Fall entry, seven
  animation-facing bit observations, and four argument/context/relation rejects.
- Native unlink comparisons use the body pinned at `c090df153`, with only its
  entry renamed. The actual production unlink and flag helper bodies execute.
  Both victim ownership classes, absent victims, both holder/attachment roles,
  ground/air state, floor absence, disconnected floor, insufficient height and
  fallback landing outcomes are compared. Whole Fighter payloads, joint state,
  engine order and compiled helper continuation are checked.
- The observation binding preserves finite values, signed zero and nonfinite
  native Float bits; compiled helper checks cover ordinary values. Native field
  observation deliberately does not sanitize or rewrite the stored orientation.
- Ten-source census still has six passing sources: Captain Falcon, Ganondorf,
  Marth, Roy, Luigi and Yoshi. DK now reaches `walk.enter(...)` at line 678;
  Bowser reaches `physics.ground_animation_facing_motion()` at line 512.
  Fox/Falco remain gated on owned entity declarations/resources. This is source
  preparation, not installed-fighter activation.
- Independent verification rehashes all 1,777 runtime inputs and 27 census
  inputs unchanged, including the pinned reference and generated fixture files.

## Receipts

- `A:/FreakMelee-test-archives/script-drop-facing-oct10-c/receipt.json`
  SHA-256 `fbf98d82e7ee5711b5b15094518db1ba74a5219001d6f94112933bbade9a503b`
- `A:/FreakMelee-test-archives/source-drop-facing-oct10-b/receipt.json`
  SHA-256 `43fda2c015ceaf3e1dbc6f2321fadd06a8b8ce53e5a8abd69e3831e2149d1d26`
- `A:/FreakMelee-test-archives/drop-facing-independent-pins-oct10.json`

Earlier `script-drop-facing-oct10-{a,b}` runs are failed fixture attempts, not
acceptance evidence. Their callback-retirement injection targeted the flag
setter rather than the actual callback routine. The fixture was corrected to
inject retirement at Fall entry, and the unnecessary native-body changes were
removed rather than weakening an assertion or treating those runs as passing.

## Limits

World pose sampling, stage queries/collision results, model-constraint removal
and ground/air state conversion are controlled leaves. The matrix verifies the
original unlink's decisions and data/order against those inputs, not a real
stage's release geometry or rendered animation. Fall entry is controlled for
dispatch and lifetime tests. Whole DK/Bowser activation, remaining walk/physics
operations and compact whole-match rollback remain open.
