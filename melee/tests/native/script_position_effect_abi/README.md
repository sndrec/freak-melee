# Literal effect argument ABI proof

The separate `PositionScalar` family admits 34 source-verified recipes. Its
inventory resolves mutually exclusive facing branches, the random `0x3E8`
model branch and `0x435/0x436` jumps into the `0x437` rotation body. Each executed
path reads one `Vec3*` and one `f32*`; unresolved forwarding, two independent
scalars and scalar-ignored `0x4D0` remain excluded. Native bodies are checked
against checkpoint `50f2b476b` before compilation.

`PositionScalarRequirements` also compares all 65,536 IDs, preserving the
output on unsupported input. It identifies fourteen model constructors,
creator-root `0x4D9` and Fighter-parent callback `0x447`. The proof records the
actual common constructor, installed callbacks and Bury's Fighter consumer.
Copied scalar consumption includes the exact facing and rotation helpers.
The fixture runs the unchanged translate/facing helper, immediate position
copies and signed-zero/NaN/infinity comparisons with controlled allocations.
Connected native branch and actual Fighter VM coverage for `0x3F1` and `0x4CF`
belongs to `script_item_terrain`; neither cohort runs audio, rendering or a game.

The sixth-batch receipt is written to
`A:/FreakMelee-test-archives/script-position-scalar-oct10-final/receipt.json`.
It includes the scalar requirements inventory and exact compiler/source pins.

The inventory now admits a separate `TwoJoint` family for `0x48F`, `0x4BB`,
`0x4BC` and `0x50B`. Extraction checks each exact pointer signature and ordered
construction, including the second model's dependency on the first succeeding.
The `void*` arguments in three of these native cases are audited JObj inputs;
other erased or unresolved families remain unsupported. Runtime pair creation
and lifetime acceptance belong to the connected `script_effect_between` cohort.

```powershell
python tests/native/script_position_effect_abi/run.py --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe --output 'A:/FreakMelee-test-archives/script-position-effect-abi-new'
```

Fresh private A: output, hidden `CREATE_NO_WINDOW` console processes, 120-second compile/run bounds. No global build/game. The runner records exact commands/source hashes and rejects inputs changing during its run. Importable `extract(root,out)` emits `expected.inc`, exact actual copy helper bodies and `proof.json` containing all literal case bodies, argument lists, pointer consumption and helper source.

Earlier acceptance: `A:/FreakMelee-test-archives/script-position-effect-abi-review2/receipt.json`, **PASS131268 checks /65536 IDs /30 actual helper-copy cases**.

- Joint:94 IDs; preserves all previous82 and adds the twelve literal `efLib_CreateGenerator_Attach_Scale` callers: `0x4A5..0x4A8`, `0x4DB..0x4DF`, `0x4F8..0x4FA`.
- JointScalar:38 IDs, including all six exact forwarded
  `efLib_CreateGenerator_AppSRT_SetFacingDir` callers: `0x48D`, `0x49A`,
  `0x4B0`, `0x4EB`, `0x4F1`, `0x510`. Source proof verifies a joint plus a
  synchronously consumed Float pointer; native generator attachment retains and
  refcounts the joint. The connected Item async cohort executes these six cases
  and the unchanged helper with controlled particle resource/allocation leaves.
- Position:63 literal exact single-Vec3 pointer cases, enumerated in receipt/proof. Source Zelda uses1285 (`0x505`); Sheik uses1283–1285 (`0x503..0x505`). Their generator/AppSRT values are copied, with no caller position pointer retained.

The inventory reads the actual `efSync_Spawn`, `efAlt_Spawn` and `efAsync_Dispatch` switch extent and top-level labels. It excludes unknown forwarders, erased `void*`, extra/nonmatching argument sequences. Conditional cases are admitted only after their executed argument path is audited; two independent scalars remain unsupported. The final asynchronous `0x476` literal body ends at the switch boundary; an inactive `va_end` in the function tail is not a forwarded payload.

Every Position pointer is consumed by `efLib_CreateGenerator` (three copied components), `efLib_Create_Attach_Pos` → inline `HSD_JObjSetTranslate` (value copy), or a direct local/params/AppSRT value copy. The extractor rejects an unproved exact-Vec3 consumer or changed helper pointer use. The C fixture executes the actual two copy helpers and inline JObj setter with controlled constructor/model leaves, comparing exact bytes after mutating the caller's local vector; covers ordinary/signed-zero/NaN/infinity values and constructor failure. It does not execute whole dispatcher branches or particle allocation/rendering.

Attach_Scale takes exactly one HSD_JObj through its forwarded va_list. The underlying generator deliberately **retains and refcounts** that JObj; this is native graph lifetime, distinct from retaining a stack scalar/Vec3 pointer. The helper reads creator-root scale after generator creation and copies scale.y into AppSRT. Source proof records this ordering. Runtime source/root retirement checks and full particle lifetime acceptance belong to the parent integration, not this classifier fixture.

The existing form-feedback fixture now reuses this complete ABI proof and keeps its own dispatch/retirement checks. Unknown argument families remain unsupported. The classifier uses numeric range checks and sorted static uint16 tables with bounded binary searches, with zero allocation.

## Position creator requirements

The same source inventory now derives `PositionRequirements` for every literal
Position case and compares query acceptance/output against all 65,536 IDs.
Unsupported ABI and null output reject without publication. A valid Position
with no dependencies returns true with zero flags. Argument classification is
unchanged; the new query describes a separate creator-context contract.

- `Root`: creator-root scale is read synchronously by `0x40D`, `0x4CC`, `0x4D8`,
  `0x504`, `0x505`.
- `FighterParent`: only `0x4CC`; its installed
  `efLib_Cb_SetRotY_FromFighterDir` later casts parent userdata to Fighter.
- `CreatesModel`: `0x3EB`, `0x3EC`, `0x3EE`, `0x3F6`, `0x3FA`, `0x3FB`,
  `0x3FC`, `0x41D`, `0x427`, `0x4CC`, `0x4D2`, `0x4E2`, `0x4E8`.

Creating a model does not itself require the creator's JObj. The actual common
`efLib_Create` stores the parent GObj, initializes update to NULL and creates a
separate model root; `efLib_Create_Attach_Pos` copies position into that new
root. Source proof records both bodies and the assigned update callbacks.
`0x427` installs `efLib_Cb_Fall_FromParamY`, which reads its own model, whereas
`0x4CC` installs the Fighter-dependent callback. No admitted Position body
reads Item userdata. New unresolved creator userdata or update assumptions
cause extraction failure instead of receiving zero requirements silently.

Effect 1040 (`0x410`) uses `efLib_CreateGenerator(0x22A, Vec3*)`; the three
position components are copied synchronously. It needs no Item model root and
retains no pointer to the caller's copied vector. Other Position IDs still need
the requirements above checked by their actual binding before construction.

Final capability acceptance is recorded at
`A:/FreakMelee-test-archives/script-position-effect-context-final/receipt.json`:
262,346 checks, 65,536 IDs, 30 actual helper-copy cases, five Root/one
FighterParent/thirteen CreatesModel entries. Capability proof remains source
inspection plus actual classifier execution; it does not execute complete
native effect constructors, parent retirement, graphics or game behavior.

## Position with two scalars

Recipe `0x3FF` is admitted as an exact `Vec3*, f32*, f32*` family.
The extracted native branch copies position through `efLib_Create_Attach_Pos`,
dereferences the first scalar for facing, and immediately copies the second into
the created model's Z rotation. The inventory verifies the complete branch and
rotation helper; other three-pointer recipes remain rejected. The independent
classifier and requirement comparisons cover all 65,536 IDs, including output
preservation on unsupported IDs. This recipe creates a model without requiring
the creator's model root. This is source/ABI proof, not rendered acceptance.

## Numeric generator families

The early `efSync_Spawn` branches for IDs `0..591` and `30000..30999`
admit one copied world position. The source extractor verifies the exact
`Vec3*` read and immediate `efLib_CreateGenerator` call before any literal
switch dispatch. These 1,592 IDs add no creator-root, Fighter-parent or model
requirements. The independent fixture compares all 65,536 IDs and executes
the unchanged generator copy helper for ordinary ID 27 and bank-30 ID 30000.
This admits the original full-charge flash used by Mewtwo without a
fighter-specific scripting operation. Real particle assets and rendered
acceptance remain separate from this controlled ABI fixture.

## Joint and local offset

The `JointOffset` family admits recipe `0x448`. Its two erased dispatcher
arguments are resolved against the typed `efLib_CreateGenerator_AppSRT_SetPos`
callee: `HSD_JObj*` followed by `Vec3*`. Extraction checks their order, the
retained joint, copied `effect->params` and installed update callback. The
fixture compiles that actual helper body and verifies offset copying, joint
attachment, callback identity and allocation failure with controlled leaves.
It does not establish arbitrary joint retirement safety or rendered VFX.

## Typed attached-model joint recipes (2026-10-11)

The Joint family now admits all six single-joint calls to the typed
`efLib_Create_Attach_Scale_FacingDir` constructor: `0x4C6`, `0x4C7`,
`0x4F2`, `0x4F3`, `0x511`, `0x512`. They were excluded because the
dispatcher spells their variadic argument `void*`, though the consuming callee
requires `HSD_JObj*`. The independent extractor proves that actual callee's
attachment, copied creator-root scale, and later Fighter-parent facing update;
other erased arguments remain rejected. The classifier therefore has 100 Joint
IDs. The Fighter joint-effect binding validates and rechecks its creator root
for every synchronous selected-joint construction, including single joints.

This audit does not claim runtime retirement or rendered acceptance; the
separate stock/source special-entry cohort exercises the live model recipes.
