# Mario script translation

The [Mario template](../examples/mario_full.fms) owns all four special moves,
the Fireball and Cape article logic, and Mario's versus fighter events.
Both it and the [Dr. Mario template](../examples/drmario_full.fms) now declare
independent behavior. Their baseline11 runtime comparisons cover specials,
taunts and active CPU play; complete fighter acceptance remains broader.
Its authored callbacks use shared engine primitives rather than calling a
stock Mario special or article handler. Original C remains the comparison
reference. This describes translation coverage, not complete fighter acceptance.

## Ownership

Attached articles use ordinary script-owned creation-number handles and the
generic `articles.*_owned` operations. The old `articles.spawn_cape`,
`callbacks.track_cape`, `articles.remove_cape`, and `articles.forget_cape`
compiler operations have been removed, along with the Mario-specific module
hooks. The focused Cape fixture now tests authored item callbacks and owned
handles, independently of Mario's private union fields. The original stock
Cape still uses its original handlers when testing the stock fighter.

| Behavior | Script ownership | Original reference |
| --- | --- | --- |
| Fireball move | Ground/air entry, animation, interrupts, movement, collision transitions, hand spawn and effect cue | `ftMario/ftmariospecialn.c` |
| Fireball article | Launch setup, lifetime, gravity, stage bounce/removal, sound/effect, damage, reflection, clank, shield, absorption and interaction callbacks | `it/kinds/itmariofireball.c` |
| Cape move | Ground/air phases, boost, reflector decisions, tracked article handle, cue and cleanup callback installation | `ftMario/ftmariospecials.c` |
| Cape article | Both animation states, effect commands, lifetime tied to the owner's motion, attachment, destruction cleanup and reference interaction | `it/kinds/itmariocape.c` |
| Super Jump Punch | Entry, steering/reversal, ground/air phases and completion/landing decisions | `ftMario/ftmariospecialhi.c` |
| Tornado | Ground/air phases, repeated lift, charge and collision decisions | `ftMario/ftmariospeciallw.c` |
| Fighter events | Wall-jump capability, attribute loading, death, landing-action charge resets, held-item visibility and knockback callbacks | `ftMario/ftmario.c`, common landing action |
| CPU policy | Capture escape-script selection, first-attempt/level/chance gate, Fireball range query and bounded ranged candidate insertion | Mario branches in `ftcpuattack.c` |

The source paths above are relative to `src/melee/ft/kinds` for fighter code
and `src/melee` for item code. Mario's taunt uses common fighter behavior; its
otherwise unused two special-table taunt slots do not contain Mario callbacks.

`state.cape_article` is an ordinary rollback-tracked integer item handle.
The script resolves it only against live articles still owned by this fighter,
pauses/resumes it during hitlag, removes it on damage/death callbacks, and
clears its handle when the article is destroyed. It never stores a GObj pointer
in Mario's private fighter union.

Move constants become named project-owned [fighter parameters](fighter_owned_parameters.md)
on import. Charges and move bookkeeping are mutable script state. Article
launch/bounce constants still use named imports from the article's own DAT;
the reflector description and ordinary animation/subaction data also remain
engine-readable DAT structures. They are data dependencies, not delegated
special-move decision code.

## Generic effect attachment correction

[`item.spawn_effect_on_bone()`](scripting/item.md#itemspawn_effect_on_bone)
supports both indexed bone tables and the engine's model-root/child-chain
attachment convention. The earlier helper rejected items without a bone table,
silently dropping Mario's first bounce effect. Stock and clone then diverged
in RNG at sample 126 although the projectile's position and velocity matched.
The shared helper now resolves that root correctly; no Mario-specific API was
added. Its optional mode is attach (`0`) or position (`1`), not a scheduler
channel. Parameterized effect modes require a different method and are rejected.

## Recorded comparisons, 2026-09-28

The Release client was explicitly rebuilt for these comparisons. The normal
native CMake build now includes `melee_native` so it cannot silently leave an
older client executable while updating the shared game library.

Thirteen stock-versus-packaged-clone comparisons each completed 520 controller
samples. Both actors' fighter states, action transitions, article core/pose/
visual/ownership state, damage events, item events and RNG traces matched.
Every article-producing case requires its expected item kind in the trace.
The shield case additionally requires P2's shield-hit motion 181.

| Input in `tests/native/parity_inputs` | Exercised path |
| --- | --- |
| `drmario-special-neutral-ground-inputs.fmr` | Ground Fireball, repeated stage bounce and lifetime |
| `drmario-special-neutral-air-inputs.fmr` | Air Fireball |
| `mario-fireball-contact-inputs.fmr` | Fireball deals 6% to Zelda |
| `mario-fireball-shield-inputs.fmr` | Zelda's shield receives the Fireball |
| `mario-fireball-reflected-inputs.fmr` | P2 Mario reflects P1's Fireball |
| `mario-fireball-clank-inputs.fmr` | Two Mario Fireballs clank |
| `drmario-special-side-ground-inputs.fmr` | Ground Cape and normal article removal |
| `drmario-special-side-air-inputs.fmr` | Air Cape and indexed-bone effects |
| `drmario-cape-interrupted-inputs.fmr` | Zelda damages Mario; Cape disappears on that damage sample |
| `mario-cape-reflects-pill-inputs.fmr` | Mario's Cape transfers P2 Dr. Mario's pill to P1 |
| `drmario-death-respawn-inputs.fmr` | Death, respawn and subsequent Fireball |
| `drmario-special-up-air-inputs.fmr` | Air Super Jump Punch |
| `drmario-tornado-repeat-inputs.fmr` | Repeated Tornado |

The new Mario inputs remove the preliminary Dr. Mario taunt and its rejected
early B press; contact spacing is adjusted for Mario's different projectile.
The shield remains held until the projectile arrives. Those adjustments are
controller inputs, not gameplay changes. The files retain the replay container
but their old state digests are not an oracle; use `--replay-inputs` via the
parity runner.

Two shared-helper regressions also match across 520 samples: Dr. Mario's air
Cape (item kind 84) and Luigi's ground Fireball (item kind 105). Eighteen
focused native scripting/content checks pass, including effect-mode rejection,
article ownership, module loading, parameters and package round trips.

The local evidence is in `.local/mario-scripted-articles-verified-20260928`
and `.local/mario-article-helper-regressions`. Diagnostic folders are not
shipped content.

## Reproduce a case

Use the frozen baseline11 content tool to create a Mario draft with
`clone-stock`, import this template's parameters with `import-parameters`, then
`script-build`, `bundle-project`, and `accept` it into an isolated content
store. The project must have its owned `cpu_profile.fcp`; `clone-stock` creates
it, or `import-cpu PROJECT ISO Mario` supplies it for older projects. Run:

```powershell
python tools/run_fighter_parity.py --exe .local/baseline11/bin/melee_native.exe --iso '..\Super Smash Bros. Melee (USA) (v1.02).iso' --inputs tests/native/parity_inputs/mario-fireball-shield-inputs.fmr --stock-fighter mario --package-id YOUR_PACKAGE_ID --content-root YOUR_STORE --stocks 2 --p2 human:zelda --independent --expect-motion 343 --expect-item-kind 48 --expect-port-motion 2 181
```

For reflection use `--p2 human:mario` and
`--expect-item-owner-transfer 48 1 2`. For Cape reflecting a pill use
`--p2 human:drmario --expect-item-owner-transfer 49 2 1`.
For clanking use `--p2 human:mario --expect-item-event clank 48`.
The runner uses hidden, muted, accelerated simulation with drawing enabled by
default. For CPU inputs use `--p1-type cpu --cpu-level 9 --p2 human:zelda`;
the direct-match engine requires a human participant.

## Independent comparisons, baseline11

Twenty stock-versus-independent pairs pass across 12,800 controller samples.
All runs were drawn, hidden, muted and accelerated, with explicit checks that
the custom fighter uses its own behavior identity. The accepted cases are:

| Cases for each fighter | Pairs across Mario and Dr. Mario | Controller samples |
| --- | --- | --- |
| Ground/air Neutral, Side, Up and Down specials | 16 | 8,320 |
| Taunt | 2 | 880 |
| Active level-9 CPU against a human opponent | 2 | 3,600 |

Fighter/action, article, event and RNG traces match for these paths. The
specials/taunt manifest is `.local/mario-doc-independent-gameplay/results.json`;
the corrected active-CPU manifest is
`.local/mario-doc-independent-cpu/results.json`. The first two CPU launches in
the gameplay manifest exited before a valid match because their harness asked
for an all-CPU match. They are excluded from the accepted twenty pairs; the
corrected human-opponent runs each compare 1,800 controller samples.

This brings the independent template inventory to six: Yoshi, Mewtwo, Captain
Falcon, Ganondorf, Mario and Dr. Mario. It does not establish every gameplay
edge or deferred single-player demo.

### Owned rig and independent source checkpoint

Both templates declare `fighter: behavior independent`. Their generated
definitions set `source_kind = Ft_Kind_None`; the raw `mario_clone` and
`drmario_clone` catalog bases retain stock fallbacks for untouched packages.
The 2026-09-28 recordings above describe an earlier template revision and remain
historical evidence and useful regression inputs.

The Mario and Dr. Mario base definitions each now own all 51 semantic joint
pairs from the stock `ftLoadCommonData` part descriptors. Mario has 61 runtime
parts and Dr. Mario 86; neither has part omissions. The standalone fixture
decodes the supplied unmodified common DAT again and checks every authored
pair. Semantic part 53 has no enum name and is preserved as its numeric part ID.
Article masks/bindings, sound/effect banks and demo root names remain explicit
owned metadata.

[`mario_cpu_stock_compare.py`](../tests/native/mario_cpu_stock_compare.py)
compiles the actual generated Mario source together with extracted stock C
using a frozen SDK. Its 31,432 comparisons cover 30,720 capture cases across
levels 0–9, attempts 0–2 and 1,024 RNG seeds; 648 ranged cases across facings,
target presence, stage results, positions and queue counts 0–8; and 64 candidate
command bounds. It compares the RNG seed/draw count, capture return, command
script index and bytes, attempt counter, complete candidate queue and ordered
range-query arguments. The reference uses the original `HSD_Randf`, command
parser/writer, range query and bounded candidate helper. Collision queries are
controlled test boundaries; the fixture does not simulate stage geometry.
Only the stock capture branch's terminal `return` becomes a consumed predicate
at the authored callback boundary. Four additional checks change the named
parameters and confirm the edited policy takes effect; those are not stock
comparisons.

The fixture copies the actual ten-entry Mario motion table and checks that all
32 non-NULL animation/input/physics/collision callbacks are replaced. Both
original NULL taunt slots remain allowed, shared camera callbacks are retained,
and adding an uncovered gameplay callback makes independent module entry fail.
The reference, base metadata, generated source and common DAT hashes are saved
with the matching baseline11 fixture results under
`.local/mario-independence-source/baseline11-direct`. The earlier baseline10
evidence under `.local/mario-independence-source/direct` is preserved.

Dr. Mario's authored results-pill constructor and callbacks pass 2,199 actual
source comparisons against baseline11. They cover ordered two-pill creation,
color draws, lifetime and ownership/binding checks. The
[results-pill ledger](drmario_results_script_translation.md) records the exact
coverage and deliberate lifetime differences from stale-pointer stock behavior.
These source comparisons do not accept the complete rendered results variant
or its asset traversal and presentation.

To reproduce, create a fresh isolated Mario project with stock assets and
`project.txt`, then import/generate using a matching frozen toolchain. Import
outputs must not already exist:

```powershell
& .local/baseline11/bin/native_content_tool.exe import-parameters YOUR_PROJECT examples/mario_full.fms YOUR_PROJECT/behavior.fms
& .local/baseline11/bin/native_content_tool.exe script-build YOUR_PROJECT YOUR_PROJECT/behavior.fms
python tests/native/mario_cpu_stock_compare.py --snapshot .local/baseline11 --project YOUR_PROJECT --out-dir YOUR_RESULTS --common-data YOUR_STOCK_COMMON_DAT --independent
```

The driver invokes only Clang and the standalone fixture. It requires the
Visual Studio C runtime environment and frozen `native_fighter_parameters.c`;
it does not start Melee or require a CMake target. Stock CPU recovery shares the
ordinary diagonal recovery policy; Mario's existing stock case performs the
same commands as the default and needs no distinct authored hook.

## Remaining acceptance

The recordings do not cover absorption, shield bounce, every ledge/slope or
ground/air transition, all steering/reversal timings, or Cape removal at every
possible death/interruption timing. Object-reference cleanup is explicitly
scripted but does not have a dedicated notification recording yet. The second
Cape animation state is retained
but was not entered in the recorded attachment paths. Common normals, grabs,
throws, broader CPU paths, alternate costumes and rendered results still need a complete clone
acceptance matrix. A passing script comparison for these inputs does not
establish that whole matrix.

A later source census also found that the stock Tornado effect updater reads
legacy private move storage when aligning its model to a slope. The fighter
decisions are authored, but that visual decision still needs a generic authored
effect update. The recorded flat-stage traces do not establish that alignment;
do not treat behavior independence as complete visual acceptance.

Mario's video-only VIWait/VIWalk demo callbacks and Dr. Mario's single-player
Sleep demo constructor are deferred outside the initial versus scope. The Sleep
demo and results-pill constructor are separate contexts from ordinary versus
right-side KO and the attack-induced sleep state.
