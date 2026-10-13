# Jigglypuff script translation

The template now authors the multi-jump entry and five states' phases
and shield-break flag, with owned rig metadata and extended item notifications.
The generated independent module passes 1,636 source checks
against baseline15 with the matching baseline16 SDK. It declares
`behavior independent`; four focused rendered stock/independent comparisons
pass over 3,900 controller samples. The accepted special and lifecycle
recordings below remain historical evidence for their recorded paths.

## Multi-jump and shield-break source checkpoint — 2026-10-02

The generated callbacks match 1,578 pairs from unchanged baseline15 stock
entry/phase/common-jump/shield-break bodies, compiled against baseline16. An
additional 58 cases check removed/recycled-source continuations, edited FMP
values and atomic resource validation, for 1,636 checks total. The driver
hashes original/current source, generated C, owned script/FMP and module.

The comparisons cover five impulse/motion rows, both facings, reverse input,
11/12/13-frame turns, animation completion, exhausted jumps, command/input
gating, interrupt short circuits, collision thresholds and ordered shield-break
effects/sounds/rumble/hurt status. I/O and existing gravity/drift/collision
primitives are fixture boundaries; this is source/dispatch acceptance rather
than a rendered physics or costume claim.

All 32 extended motions' non-null gameplay callback slots are replaced; camera
and immutable metadata remain shared. The generated module has source kindNone,
26 owned semantic rig bindings and 50 runtime parts. Purin-specific raw-kind
shield-break policy is now the authored hook. Common dynamics skip attachment
sets at `ftCo_8009CB40`; the tree cursor also skips them, so the owned hat path
does not need the stock Purin branch. Stock global hat caches, draw/visibility
and getter callbacks are replaced by the authored attachment system. No Purin
CPU, combat-article or versus demo/results special decision was found. Kirby
copy and unavailable single-player/video contexts remain deferred.

Reproduce with a fresh project and the coordinated tool/SDK:

```powershell
native_content_tool import-parameters PROJECT examples/purin_wip.fms PROJECT/behavior.fms
native_content_tool script-build PROJECT PROJECT/behavior.fms
python tests/native/purin_jump_stock_compare.py --snapshot .local/baseline16 --reference .local/baseline15/sdk --project PROJECT --out-dir NEW_SOURCE_OUTPUT
```

The source driver explicitly selects VC tools14.44. The focused gameplay
runner, `tests/native/purin_jump_parity.py`, prepares two complete five-jump
turning sequences, a Bunny Hood route and the prior blue-hat Rollout/contact
recording. It requires actual jump states/turns and worn-item ownership, uses
hidden/muted/fast matches with drawing, and validates independent behavior
identity. All four pairs pass against the frozen baseline16 host: right/left
five-jump turning sequences and the Bunny Hood route each contain 900 samples;
the blue-hat Rollout/contact regression contains 1,200. They match 7,800 fighter
states, 3,900 RNG samples, the actual worn item, accessory/model pose and
Rollout contact. Independent behavior identity, all five ordered jump motions,
turns within each motion and actual worn-owner state are required. Evidence is
`.local/purin-jump-runtime16/results.json`, its manifest and per-case traces.
These inputs do not establish every item, in-game shield-break, interruption
or costume path.

Accepted runtime package: `29e243b36fd81d968bc96a8af6a675fd0509e96c1ebd0bf45a806c862686fc90`.
The tested template SHA256 is
`a3c26c1a652495381f1a945dbd55792e224ed55b7c009722695b4d5ceb952ee6`;
module SHA256 is
`02411405805479230f554e089e910b92367138f9bec8e19f7182a16f45381852`.
The completion comment was updated afterward without changing executable
script statements. The exact tested template is retained as
`.local/purin-jump-source/accepted-template.fms`; the combined source/runtime
hash inventory is `.local/purin-jump-source/accepted_baseline16.json`.

The template imports 13 additional named jump values after the existing
43 Pound/Rollout values, preserving their parameter order. It configures the
actual owned common jump resource during load/reload, before instance/item
scaling. An ordinary integer turn counter replaces the stock motion-union
alias. See [generic jump policy contracts](scripting/jump_policies.md).

The [Jigglypuff template](../examples/purin_wip.fms) owns Rollout, Pound, Sing and Rest
in versus play. These moves use shared engine primitives and do not call
their stock special handlers. Costume attachment and fighter-wide lifecycle
decisions are also authored. Jigglypuff is available as an independent
asset base in `clone-stock` and the Workshop. Whole-fighter acceptance remains
bounded by the recorded input paths below.

## Source audit

| Behavior | Authored decisions | Original reference |
| --- | --- | --- |
| Rollout | Charge/release/turn/end/hit states, ground/air transitions, slope braking, wall and landing rebounds, hit-group/damage changes, reflection and cleanup | `ftPurin/ftpurinspecialn.c` |
| Pound | Entry command resets, stick-angle launch, velocity scaling, drift/gravity phases, collision transitions and completion | `ftPurin/ftpurinspecials.c` |
| Sing | Facing-dependent entry, effect accessory callback, movement, ground/air variants, ledge checks and completion | `ftPurin/ftpurinspecialhi.c`, `ftPurin/ftpurin.c` |
| Rest | Facing-dependent entry, accessory callback removal, movement, ground/air variants and completion | `ftPurin/ftpurinspeciallw.c`, `ftPurin/ftpurin.c` |

Source paths are relative to `src/melee/ft/kinds`. Hitboxes and animation
commands remain in the copied subactions. Sing's stadium mini-game-only
hitbox mutation is outside this versus translation. Multi-jump motion selection,
turns, completion and collision decisions are now authored. Shared input gating,
gravity/drift, aerial-jump interrupt ordering and state transitions remain
generic engine primitives.

Pound's stock counter is represented by ordinary rollback-tracked
`state.pound_count`; its value does not control a move branch. Forty-three named
Pound and Rollout constants are imported exactly into the project's
`fighter_parameters.fmp`, and their offset declarations are removed from the
working script. The original DAT block remains for shared engine readers.
See [owned fighter parameters](scripting/language.md#fighter-attributes).

## Reusable callback support

[`on accessory`](scripting/language.md#special-accessory-callbacks) now works
for any special slot, replacing the former side-only declaration. Sing uses
it for an effect and reinstalls it after collision transitions with
[`callbacks.track_accessory(Up)`](scripting/fighter.md#callbackstrack_accessoryslot).
Mario and Dr. Mario's Cape use the same mechanism. The script owns all
move-specific choices; no Jigglypuff-specific API was added.

Explicit numbered entry through
[`state.enter_flags`](scripting/fighter.md#stateenter_flagsmotion-flags-and-animationstart)
allows the script to choose the left/right Sing and Rest variants. Shared
`animation.start()` performs their initial animation advancement.

Typed [script functions](scripting/functions.md) share Sing's accessory setup
and reinstallation between its callbacks. The helper-backed template matches
the four checked-in ground/air, left/right Sing recordings over all 520 samples
per recording, including fighter state, actions and RNG. Rollout now also uses shared typed helpers for its charge, collision, damage,
rotation and cleanup decisions.

## Recorded comparisons

The 2026-09-28 batch builds and packages a fresh script after importing owned
parameters, then compares stock and clone under identical recorded inputs
with rendering enabled. Fourteen checked-in recordings each contain 520
controller samples:

| Input family | Variants | Required P1 motions |
| --- | --- | --- |
| `purin-pound-*-inputs.fmr` | Ground/air, left/right; air up/down steering | 363 ground, 364 air |
| `purin-sing-*-inputs.fmr` | Ground/air, left/right | 365/366 left, 367/368 right |
| `purin-rest-*-inputs.fmr` | Ground/air, left/right | 369/370 left, 371/372 right |

All fourteen current recordings match fighter state, action and RNG traces.
The leftward Pound inputs were corrected to retain negative horizontal input
at entry and both comparisons rerun; those results supersede the initial
Pound-left runs. Required-motion checks prevent missed inputs from passing.
There are no damage events in these recordings, so they do not establish
contact or sleep behavior. See the
[input directory](../tests/native/parity_inputs/README.md) and
[`run_fighter_parity.py`](../tools/run_fighter_parity.py) for reproduction.

Four fresh Mario/Dr. Mario Cape comparisons also match after the generic
accessory change: Mario airborne Cape, interruption cleanup and pill
reflection, plus Dr. Mario airborne Cape. The full configured Release build
and nineteen focused scripting/content checks pass. Workshop template copies
are updated for all six available stock bases; UI usability remains user
acceptance work.

Additional item/contact/interrupt alternatives, wall rebounds and fighter
reflection, ledges/slopes and whole-fighter acceptance
remain open. These comparisons prove the recorded paths, not every possible
behavior or visual equivalence.

## Rollout checkpoint

The script now owns every Rollout animation, interrupt, physics and collision
callback across Start, Loop, Full, Release, Turn, End and Hit, including their
left/right and airborne variants. Damage-taken/death cleanup, damage-dealt
rebound, state-change attack statistics and fighter reflection are authored
callbacks. No Rollout handler is delegated to stock C. Sixteen named Rollout
variables plus Pound's counter occupy 72 bytes of tracked fighter state; no
move-specific union alias is used.

The 38 Rollout tuning values and five Pound values import into the project's
owned parameter file. Generic [combat/presentation primitives](scripting/combat_visuals.md)
provide model scale, hit groups, color, sound, collision boxes and explicit
motion rates. The compiler permits 64 state declarations within the existing
256-byte tracked budget and 128 numbered/lifecycle action callbacks.

Eight checked-in `purin-rollout-*-inputs.fmr` recordings have 1,200 samples each:
short/full grounded charges, short airborne charges and ground turns, each
facing left/right. They match stock actions, fighter pose, hitbox/hurtbox
state and RNG. The two rightward grounded charge cases also require damage
to P2 and motion 362, exercising the post-hit rebound. Turning cases require
motion 351; air cases require motion 358. Test inputs were corrected when a
jump landed before release or a leftward rollout left the edge before turning,
so an unvisited branch is not counted as passing.

The original aerial Turn expiration chooses a ground ending motion; this
unusual defined behavior is retained and noted in the script. Scratch fields
that stock writes but never reads are omitted. Rotations use the actual YRotN
part (index 3), not TransN. Shared stock/native engine behavior is unchanged.
Wall rebound, fighter reflection and uncommon interruption alternatives are
translated from source but not covered by this recorded batch.

## Fighter events and costumes checkpoint

`FighterEvents` now owns load, special-attribute reload, death, destruction,
item pickup/drop/show/hide, and knockback enter/exit. Multi-jump setup still
uses the shared engine's common jump data, copied into the fighter-owned
attribute block. Defined stock choices are retained: pickup hold categories
3 and 4 select part-animation 0 or 1; heavy items skip that pose, and the pickup
flag controls when it is shown. A compiled callback fixture covers held/no-item,
heavy/light, hold-category, and show/hide branches, plus a helper-created
attachment and destruction callback. This is callback/dispatch proof, not an
in-game pickup recording.

Costume decisions use [generic model attachments](scripting/model_attachments.md).
The selected costume's public root chooses the accessory, actual part 6 is the
parent, and the copied visibility descriptor selects its original costume row.
Blue and green hats add their own two dynamic chains after the body chain.
No `ftPr_Init` callback is invoked by these authored events or draw hooks.

The DAT importer now retains dormant accessory dynamic descriptors after the
initial active body count, and all rows of an accessory visibility table. These
are generic descriptor imports. Authored cloth sets do not enter body-only
animation array walks. Their joints, dynamic data, controls, and cleanup belong
to tracked fighter/game-heap storage, with no allocation in the draw loop.

A stock native crash on the blue/green costumes was confirmed in the debugger
at `ftCo_8009DC54`. That routine read the hat through `u.kb.hat.jobj`, which aliases
`u.pr.x223C` on the original 32-bit layout but occupies a different pointer offset
on the native 64-bit layout. Access now uses Jigglypuff's actual field. This fixes
a native data-layout mismatch; it does not change the stock gameplay decisions.

The 2026-09-28 release batch creates a fresh parameter-owning script package and
four independent skin packages, then compares all five costumes using the same
1,200-sample short grounded Rollout recording. Every run requires motions 346,
350 and 362 plus damage to P2; non-default costumes require an actual attachment.
The traces include accessory pose/material hashes, primary fighter pose,
hitboxes/hurtboxes, actions, damage and RNG. All five comparisons match after
the native field fix. The full Release build and nineteen focused checks pass;
the callback fixture is included in that batch. Six additional shared asset,
model, and semantic-state regression checks also pass. This does not establish every
item, metal/cloak, ledge, reflection, or interruption branch.

To reproduce into a new output directory (never the live collection):

```powershell
python tests/native/purin_costume_parity.py --iso "../Super Smash Bros. Melee (USA) (v1.02).iso" --out-dir .local/purin-costume-comparison
```

The helper normalizes only each skin's main public symbol names to the custom
fighter's single slot, preserving all descriptor byte offsets and accessory
roots. It installs test packages only in its isolated content store.
