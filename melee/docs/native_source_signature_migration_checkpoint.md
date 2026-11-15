# Stock source signature migration

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

This batch updates Captain Falcon, Ganondorf, Luigi and Yoshi to the current
interpreter contracts. It does not publish a complete package or establish
gameplay acceptance for any of those four fighters. Their next preparation
failures are recorded below rather than bypassed.

## Generic attached scalar effects

`effects.spawn_async_with_scalar(effect, part, value)` supplies one semantic
bone role and one finite copied Float to a source-admitted joint/scalar effect.
It uses native attachment mode 3, retaining the existing source/program/storage,
rig mapping, joint class/extent and post-call publication checks. The effect's
ordinary native queue scheduling and scalar-copy semantics are unchanged.
This is available to any fighter; no species-specific callback was added.

The native async fixture passes 759 checks across 67 cases, including actual
source parsing, bytecode/VM invocation, the unchanged native async queue,
immediate/deferred mode-3 delivery, signed-zero/scalar preservation, invalid ABI,
nonfinite/type rejection and thirteen retirement mutations. The scalar effect
rendering receivers are controlled leaves; this is not rendered kick acceptance
or compact effect snapshot proof. The existing joint-only generator case still
executes its exact native dispatcher body and attachment/ref ownership.

Native client, content tool and source family/package integration targets build.
The initial fixture compile exposed obsolete duplicate controlled/native class
definitions and missing scalar helper names. The fixture now removes the class
stub before extracting the actual class body and reuses its existing Float value
constructor; unavailable Item leaves stay controlled in its shared base.

Receipt: `A:/FreakMelee-test-archives/script-async-scalar-ready-oct10/receipt.json`,
SHA-256 `b89133474a934644dce31a145ecf2e742587b820826df054ef6bd5cb802bf02c`.
See the [API reference](scripting/joint_feedback.md#effectsspawn_async_with_scalareffect-part-value)
and `tests/native/script_async_effect/README.md` for usage and fixture boundaries.

## Direct script changes

- Falcon/Ganondorf landing calls pass the stock `false` flag as well as landing
  lag, matching `ftCa_SpecialAirSStart_Coll` and `ftCa_SpecialAirS_Coll`.
  Ground slope rotation follows `ftCommon_8007E5AC` through root Z rotation and
  float `atan2`. Kick flame calls use the new generic attached-scalar operation,
  matching `ftCa_SpecialHi_800E3EAC`'s joint/Float attachment mode.
- Ganondorf's punch wind supplies the fighter's world position through the
  existing generic position-wind operation. It reproduces the former helper's
  `lb_800119DC(&fighter->cur_pos, ...)` call without a compatibility alias.
- Luigi owns the root-X reset helper and installs it in damage/death2, matching
  `ftLg_SpecialLw_UpdateRot`. Empty named Green Missile phase stubs are removed;
  `GreenMissilePhases` already owns their actual 343/349 motion callbacks.
- Yoshi's owned EggThrow signals name their Article declaration explicitly.
  Redundant named EggRoll phase callbacks are removed: the actual copied asset
  facts map both initial entries to motion 360, whose complete callbacks are
  already owned by `EggRollStates`. Its other 356..363 callbacks remain intact.

Duplicate-callback validation remains strict. No last-wins dispatch or ignored
callback policy was introduced.

## Diagnostics at this signature checkpoint

Fresh read-only `source-check` against the private census clones reports:

The later [capture-registration checkpoint](native_script_capture_scheduler_checkpoint.md)
supersedes Falcon/Ganondorf's first failures below. Its scheduler test and
current source-preparation boundaries are recorded separately.

| Source | First remaining boundary |
| --- | --- |
| Captain Falcon | `capture.arm(2)` has no current native binding, line 283. |
| Ganondorf | Same grab registration boundary, line 287. |
| Luigi | `event.move_id` is not a property of the action-metadata event, line 319. |
| Yoshi | Legacy `callbacks.track_accessory(Up)` is unsupported, line 162. |

These require substantive generic callback/event contracts and direct script
call-site migration. They are not fixed by accepting unknown names or
delegating to stock handlers. Further source errors may follow those first
diagnostics. No installed package or original DAT was changed.

Receipt: `A:/FreakMelee-test-archives/source-signature-batch-oct10/receipt.json`,
SHA-256 `8e45c26bd6aabd4b07f2d90b86f55a2e595da20ce8250af66d52fee7b4e3e1de`.
This batch extends the [remaining-fighter census](native_source_companion_capture_results_checkpoint.md#remaining-fighter-migration).
