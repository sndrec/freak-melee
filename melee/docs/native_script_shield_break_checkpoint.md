# Interpreted shield-break checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

The common shield-break entry now dispatches an owned fighter's synchronous
`on shield_break` hook through the ordinary interpreter. This connects
Jigglypuff's existing authored top-blast KO decision without calling its stock
handler. The engine still owns the original motion, launch velocity, effect,
sound, rumble and hurt-status sequence. Owned fighters without this hook make
no extra decision; stock fighters retain their original behavior.

The entry retains the fighter and published program before initialization and
rechecks them at callback boundaries. A hook fault or actor/program retirement
stops continuation. No generated module, fighter-specific scripting extension
or per-tick allocation was added. See
[shield-break policy](scripting/jump_policies.md#on-shield_break-and-top-blast-damage-policy).

## Verification

- The connected cohort passes **102,372 checks / 9,615 cases**. Seventeen new
  cases compare the shared entry against its pre-change body at `b091887fa`,
  including normal/top-blast decisions, absent owned hooks, retirement at six
  engine boundaries and retirement inside an authored hook.
- The real compiler, actor, VM, hook dispatch and damage-flag binding execute.
  Common reset, motion/animation setup, effect, sound and hurt-status leaves
  are controlled. This is scheduling and lifecycle evidence, not rendered
  shield-break or full Jigglypuff gameplay acceptance.
- The read-only roster audit prepares **20/26 forms**. Jigglypuff now passes
  complete metadata, all **192** bodies and package admission. Fox, Falco,
  Pikachu, Pichu, Samus and Ness still need preparation work. Kirby remains
  deliberately deferred and is not part of this audit.
- Seven Windows targets build and the `native_fighter_script` reference CTest
  passes. No game launch or installed-content change was needed for this
  checkpoint.

Evidence, independently rehashed against the final source:

- `A:/FreakMelee-test-archives/script-shield-break-oct10-complete/receipt.json`:
  SHA-256 `9f0e7392cb283973b991f19fa91d488d4c9e41c43eee4e14797c9f786922c0b3`,
  **1,831** input pins.
- `A:/FreakMelee-test-archives/source-full-roster-audit-oct10-shield-break-a/receipt.json`:
  SHA-256 `6de59166cef33a21a93258e00b2c656be2f0372ba1c1a2d76526b9f832726856`,
  **214** input pins.
- `A:/FreakMelee-test-archives/shield-break-independent-pins-oct10.json`.

Passing preparation is not complete fighter acceptance. Hidden ordinary-runtime
checks, remaining package/resource prerequisites, full clone behavior coverage
and compact whole-match rollback acceptance remain open. The goal stays active.
