# Samus article feedback checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

2026-10-10, following `88aabc8a4`. The batch source audit now reports **five**
failing bodies instead of fourteen, with all 172 bodies and their metadata
covered. Samus remains rejected and incomplete; this is not gameplay parity or
an accepted independent clone.

## Generic synchronous joint effects

`item.spawn_joint_effect(effect, "path")` extends the existing Item model path
preparation and feedback dispatch. It records a primary-model child path once
and calls `efSync_Spawn` with the selected live joint. It does not substitute
deferred effects, assume a fighter identity, parse paths at runtime or allocate
script-side state per frame. The existing asynchronous operation keeps its own
recipe signatures, negative-ID no-op behavior and deferred dispatch.

The API checks the owned actor/program/storage, exact prepared selector, native
Joint argument ABI and engine joint class around construction. Missing joints
are no-ops; changed graphs/selectors or incompatible recipes reject. Existing
direct root effects remain direct root operations. See the
[model API](scripting/article_models.md#synchronous-joint-effects).

## Samus translation changes

- Charge Shot's charging effect and both missile effects now call synchronous
  feedback with the exact native root/grandchild selection.
- Article callbacks read root immutable settings through `program.attrs` rather
  than an unsupported implicit Fighter attribute scope.
- Charge Shot owns Float charge/maximum values, matching native Item storage.
  Held updates retain the original Fighter helper's maximum-to-Int conversion.
  Release retains the original clamped input separately from the charge clamped
  to 7 for animation/visual scale. Speed uses the former; size and full-charge
  effects use the latter, exactly as the stock Item functions do.
- Release assigns the creator's current facing, matching native release code.
- Held animation decisions use the typed owner's detachment flag. Destruction
  guards the original/current owner relation and calls an exported script helper
  to clear its handle rather than a stock Samus handler.
- Missile and bomb explosions clear XYZ velocity directly, matching the complete
  `itResetVelocity` body called by native `it_80273454`; no alias host operation
  was introduced for three scalar assignments.

The source audit now clears the Charge Shot bodies and the two explosion bodies.
Its remaining first errors concern action metadata, missile target searches,
bomb-owner overlap, ground-slide velocity and custom ground-standard collision.
Further errors may be present inside those five bodies. Grapple and lifecycle
translation remain separate open work. The native charge-damage value `xDF8` has
only an assignment and no named read in the C source inspected in this checkout;
no fighter-specific script API was added for that apparently unused field.

## Evidence and boundaries

- Seven native build targets pass.
- `native_fighter_script` and `native_fighter_parameters`: 2/2 CTests pass.
- Connected Article-feedback cohort passes **2,319 checks / 220 native spawn
  recordings**. New cases compile/prepare the actual source path selector, run
  through the actual owned Item actor/VM, and check the selected grandchild,
  publication and source/program/root retirement during synchronous construction.
  Missing paths, wrong native argument ABI, negative IDs, changed selectors and
  wrong joint classes are covered. Effect rendering remains a recorded leaf.
- Full source audit: 172 bodies, complete coverage, five diagnostics, no outer
  package diagnostic, **package rejected**.

Receipt: `A:/FreakMelee-test-archives/samus-sync-joint-feedback-oct10-b/receipt.json`.
Combined checkpoint evidence, copied audit/build/test logs and closure of all
1,693 cohort dependency pins:
`A:/FreakMelee-test-archives/samus-article-feedback-oct10-c/receipt.json`.
The earlier a run failed fixture compilation due to an incorrect test descriptor
type and is not acceptance evidence. No game was launched or user package
installed in this batch. End-to-end Samus constructors, appearance, combat,
grapple/lifecycle, gameplay comparisons and rollback remain unproven.
