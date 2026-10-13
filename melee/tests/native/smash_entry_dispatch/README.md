# Replacement smash-entry dispatch

Source fixture accepted against the coordinated43 SDK. No gameplay, default fighter
activation or complete common-transition acceptance is claimed.

```powershell
python -B tests/native/smash_entry_dispatch/source_test.py --snapshot .local/baseline43 --baseline .local/baseline42 --output .local/smash-entry-dispatch43-FRESH
```

`--source` selects the checkout containing the actual modified functions. Use
an immutable source checkout or keep these three source files frozen for the
run. `--prepare-only` extracts and verifies inputs without compiling/executing.
Preparation against42 is possible, but42 lacks the new Definition members and
cannot compile the new fixture. The43 acceptance requires its matching SDK.

## Actual bodies

The runner reads full bodies from `src/melee/ft/kinds/ftCommon/` and their
frozen42 counterparts. Only source-local function identifiers are renamed;
the statements/conditions are retained. There are no forwarding adapters.

| File | Extracted functions |
| --- | --- |
| ftCo_AttackS4.c | checkLStick, checkFacingDir, checkItemThrow, decideFighter, ftCo_AttackS4_CheckInput, ftCo_AttackS4_8008C114 |
| ftCo_AttackHi4.c | checkLStick, checkLStickNoD0, ftCo_AttackHi4_CheckInput, ftCo_AttackHi4_CheckInputNoD0 |
| ftCo_AttackLw4.c | checkLStick, ftCo_AttackLw4_CheckInput |

The SDK's real pad-profile inline checks, actual
`Fighter_SetEffectHitlagCallbacks` inline and production
`native_callback_lifetime.c` are used. The live fighter list is controlled.
The fixture supplies actual Fighter/HSD_GObj structures in Windows virtual
memory pages and uses real generation numbers.

## Coherent matrix

The108,000 input configurations span five entry paths, all combinations of
A-button/tilt-stick/c-stick predicates, nine held-item routes, five stick values
including exact thresholds, two timer values, six kinds and five Definition
modes. Definition modes are absent, empty, presentation-only, replacement-only
and replacement plus presentation. The kinds exercise generic, Ness, Peach,
Game & Watch, Pikachu and Pichu fallbacks.

Actual frozen42 traces establish unchanged rejected-input, item throw/swing/fire
priority and fallback behavior. Accepted replacement routes must contain one
replacement event and no preceding common transition, presentation callback,
stock fighter handler or effect-hitlag setup. Forward entry receives its exact
selected stick angle and already chosen facing. Existing presentation-only
behavior remains common-entry then presentation.
Rejected inputs and item-priority routes make no active Definition lookup.
Every accepted non-item route makes exactly one lookup before dispatch.

Twenty additional accepted routes remove the caller and make both GObj and
Fighter storage PAGE_NOACCESS, or reuse both addresses with another generation
and poisoned fields. The actual functions must return without accessing removed
storage or altering replacement bytes. Generation validation links the real
production implementation; model-effect branches trap if reached.

Frozen42 deliberately ignores the new replacement members, so every healthy
accepted new replacement case must reject that old dispatch behavior. The
receipt counts those semantic rejection cases separately. It does not claim
the old input functions had a post-replacement use-after-free: they did not
invoke the newly introduced hooks at all.

## Controlled boundaries

Common `doEnter` transitions, stock fighter special-smash handlers,
held-item throws/weapon actions and presentation callbacks record their
identity/order and selected data. Their complete implementations are not
executed. Stick-angle calculation, item use/fire/throw classification and active
Definition lookup are controlled too. The entire input acceptance/priority and
dispatch functions themselves execute unchanged.

Effect-hitlag setup is not replaced: the actual inline assigns `efLib_PauseAll`
and `efLib_ResumeAll` to the fighter's callback fields. The fixture compares
those assignments on fallback paths and asserts both fields remain NULL before
replacement dispatch. The two effect function bodies trap if called; the
dispatch functions only bind them and do not execute effect update behavior.

This fixture does not test actual generated script policies or DLL declaration
metadata; the coordinated compiler/module suite must cover both generated
routes and their required Host/lifetime checks separately. No rendering, physics
or game scene is launched.

## Integration recipe

Root can run the command above once after freezing43. A Python console CTest
may invoke the same command with fresh build-owned output, or remove only that
specific previously checked build-owned output before rerunning. The standalone
runner intentionally rejects existing output and never deletes data. It hashes
both snapshot manifests, all SDK/compiler inputs, the three current/frozen
source files, fixture, runner and production lifetime source before and after.
Generated headers, executable, logs and receipt stay under the chosen output.

Historical preparation receipt:
`.local/smash-entry-dispatch-prepare42/receipt.json`; acceptance=false. It
verifies frozen42 and extracts live modified dispatch without a compiler/game
launch. The first root compile receipt at
`.local/smash-entry-dispatch43-root/` records the original extraction/redefinition
errors. The corrected final full matrix is
`.local/smash-entry-dispatch43-final/receipt.json`:1,471,692 checks,
108,000 input configurations,10,128 strict frozen42 semantic rejection cases,
10,148 replacement invocations including20 protected/recycled caller cases.
Compilation emitted no warnings. Root independently reproduced the same results
after the source handoff at
`.local/smash-entry-dispatch43-root-accepted/receipt.json`. This fixture launched
no gameplay and performed no global build.
