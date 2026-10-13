# Interpreted damage timer and movement checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

2026-10-10. The ordinary runtime remains the interpreter.

Exposed two shared operations through the existing combat binding:
`damage.update_timer()` and `physics.damage_motion()`. Both retain their exact
owned Fighter, program, storage, layout and parameter identity. They invoke the
native shared body rather than a stock fighter's decisions. Added full retained
lifetime checking in the native timer after knockback-exit: retiring a program
with a still-live actor cannot resume hammer recovery or later authored writes.
Healthy counter, hitstun, gravity, friction and recovery arithmetic is unchanged.

Bowser's flame constructor now reads its owner's declared maximum length/size
through `owner.params`, separately from the article's own speed parameters.
The generic receiver resolver already provides this namespace; no fighter-
specific primitive or attribute offset was added. Documented the distinction
between article parameters and recipient Fighter parameters.

Validation:

- Seven configured Windows targets build, including the native client, content
  tool, source-family/package runtime executables and reference compiler targets.
- `native_fighter_script` CTest passes.
- Capture scheduler: **99,586 checks / 9,360 cases**, including **848 new cases**:
  108 healthy timer comparisons, 720 healthy damage-physics comparisons,
  4 actor/program retirement cases and 16 entry rejections. Whole Fighter
  payloads and engine order are compared through bindings and owned VM helpers.
- Native bodies pinned at `2be04ddb3` cover timer update, damage-physics branch
  selection, airborne hitstun gravity/friction, ground friction/slope movement,
  hammer classification and hammer recovery assignment. Only entry/internal
  names are renamed in reference bodies.
- DK advances from line 825's timer operation to line 895's heavy-item throw
  input. Bowser advances from line 782's implicit parameter read to line 791's
  child-model visibility. Captain Falcon, Ganondorf, Marth, Roy, Luigi and Yoshi
  still prepare; Fox/Falco asset/entity gates remain unchanged. Preparation is
  not full fighter activation or gameplay acceptance.

Final receipts, independently rehashed against current inputs:

- `A:/FreakMelee-test-archives/script-damage-tick-oct10-a/receipt.json`
  SHA-256 `37b44c5daaaf5c26b4c8f2ff32a2c75f4ae010db98bdd4eacddc9a0d19ade8bb`;
  **1,810 input pins** unchanged.
- `A:/FreakMelee-test-archives/source-damage-tick-oct10-a/receipt.json`
  SHA-256 `56f34631b4ab5114a96e3f0e09eabe2dad3fe8b4cc4f2cdeac1c287f46d28967`;
  **27 input pins** unchanged.
- `A:/FreakMelee-test-archives/damage-tick-independent-pins-oct10.json`
  records the independent verification and the failed supplemental test below.

A supplemental older `script_receiver` fixture failed compilation at
`receiver-damage-tick-oct10-a/build.log`: its strict compiler rejects existing
positional argument descriptor initializers, and its controlled `ft_80082838`
duplicates the body now included by its extraction. The initializer macro is
unchanged from `2be04ddb3`. No runtime receiver receipt was produced; this attempt
provides no receiver or Bowser flame gameplay acceptance. Its fixture setup
still needs refreshing. Full Bowser reference export was not exercised.

Normal airborne movement and the knockback-exit callback are controlled capture
fixture leaves. Full collision/knockback integration, actual authored exit-hook
behavior, DK cargo/holding gameplay and complete flame behavior are unverified.
Compact whole-match rollback remains open. No installed assets changed and no
game window opened.
