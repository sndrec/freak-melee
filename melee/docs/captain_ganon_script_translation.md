# Captain Falcon and Ganondorf script translations

The [Captain Falcon](../examples/captain_wip.fms) and
[Ganondorf](../examples/ganon_wip.fms) templates own their four special moves
and fighter-specific versus callbacks. They use shared engine operations;
neither template calls a stock special handler. Stock C remains the reference.
The generated modules now declare independent behavior. The acceptance ledger
below distinguishes source comparisons, recorded gameplay and remaining limits.

## Source audit

| Behavior | Authored decisions | Original reference |
| --- | --- | --- |
| Falcon Punch / Warlock Punch | Entry, cues and effects, airborne steering, movement, ground/air transitions and completion | `ftCaptain/ftcaptainspecialn.c` |
| Raptor Boost / Gerudo Dragon | Start, contact and miss phases, hurtbox target classification, movement, landing and damage/death cleanup | `ftCaptain/ftcaptainspecials.c` |
| Falcon Dive / Dark Dive | Entry, catch, victim launch, throw cues, ledge/landing conditions, freefall and interruption | `ftCaptain/ftcaptainspecialhi.c` |
| Falcon Kick / Wizard's Foot | Ground/air travel, hit-speed scaling, end phases, landing traction and wall bounce | `ftCaptain/ftcaptainspeciallw.c` |
| Fighter events | Attribute loading, item pickup/drop/visibility, death model visibility and effect state | `ftCaptain/ftcaptain.c`, `ftGanon/ftganon.c` |
| Family differences | Falcon wall-jump capability; Ganondorf wind effects, part animations during knockback, and death visibility | Same files and kind-dependent branches in shared specials |

Source paths are relative to `src/melee/ft/kinds`. Ganondorf shares the original
Captain Falcon special handlers; the two authored templates preserve their
distinct kind-dependent decisions. Motion slots 341–346 use common item-swing
handlers, not fighter-specific specials. Neither fighter has a separate
fighter-specific article or taunt callback to translate in these tables.

Temporary move resources use ordinary rollback-tracked
[`state`](scripting/language.md#fighter-state): effect flags, side-special
gravity, Dive throw readiness and velocity, and Kick speed-scaling count and
friction. They do not access the original fighter union.

## Editable parameters

Each source template declares 24 named stock-value imports, including an
integer maximum hit-scaling count, plus owned inline CPU/taunt defaults (37
parameters for Captain Falcon and 36 for Ganondorf in total). The generic
importer copies the original
binary32/int defaults into the project's `fighter_parameters.fmp`, then removes
the offset declarations from the working script. Authored callbacks read
`attrs.Name` from that owned table. See
[fighter parameters](scripting/language.md#fighter-attributes) for the Workshop
workflow and rebuild rules.

The original DAT attribute block is retained for shared engine readers. This
does not claim that every engine-facing attribute has become an owned
parameter, or that the imported defaults are independent of the initial stock
asset extraction.

## Recorded comparison batch

The 2026-09-28 checkpoint builds fresh modules and installed packages after
parameter import, then compares identical inputs against stock with rendering
enabled. Each fighter uses seven checked-in recordings:

| Recording | Samples | Required evidence |
| --- | --- | --- |
| `fighter-left-neutral-inputs.fmr` | 380 | Left-facing neutral special, motion 347 |
| Family `*-side-contact-inputs.fmr` | 520 | Motions 349 and 350; damage to P2 |
| Family `*-side-air-contact-inputs.fmr` | 520 | Motions 351 and 352; damage to P2 |
| `fighter-dive-catch-inputs.fmr` | 380 | Motions 353, 355 and 356; damage to P2 |
| `fighter-dive-interrupted-inputs.fmr` | 380 | Motions 353 and 80; damage to P1 |
| `fighter-kick-wall-bounce-inputs.fmr` | 1100 | Onett wall-bounce motion 363 |
| `fighter-air-kick-hit-inputs.fmr` | 420 | Motions 359 and 360; damage to P2 |

Family prefixes are `captain` and `ganon`. The setup uses two stocks, items off,
Zelda as human P2, and Final Destination except for Onett. Required-event checks
prevent a rejected input or missed contact from counting as coverage. Fighter
state, action, item, damage and RNG traces are compared by
[`run_fighter_parity.py`](../tools/run_fighter_parity.py). See the
[input directory](../tests/native/parity_inputs/README.md) for reproduction
commands and additional earlier recordings.

All fourteen stock-versus-clone comparisons matched over their complete input
recordings, including RNG. Four Mario/Dr. Mario Cape regressions also matched
after removal of the obsolete Cape-specific module hooks. The configured
Release build and nineteen focused scripting/content checks passed.

These inputs exercise defined branches. They do not prove every stick angle,
item target category, capture escape/interruption timing, effect and hitlag
branch, slope/ledge transition, common action, alternate costume, CPU choice,
death/result path, or visual equivalence. Whole-fighter acceptance remains open.

## Independent closure preparation, 2026-09-30

This checkpoint prepares additional policies in
`.local/captain-ganon-closure`; it does not change the accepted examples or
activate independent behavior. Both generated fighters still inherit stock
behavior. The complete nonnull stock callback inventory is load, attribute
reload, death, eight special entries, extended and ordinary item pickup/drop,
and item visibility/invisibility. Ganon additionally has knockback enter/exit.
Those callbacks are already authored in the examples. Neither stock family
has userdata removal, absorption, extra demo callbacks, or fighter articles.
Motion slots 341–346 use common fourth item-swing callbacks; 347–363 contain the
seventeen special states. Common camera callbacks remain shared engine policy.

Remaining kind-dependent decisions are Falcon's capture CPU choice, the shared
family's ranged candidates, Ganon's common taunt wind, and Falcon's fourth held
item-swing motion selection. Existing CPU hooks can own default approach spacing
as named values 3.5 and 4, preserving the shared defaults.
Ordinary attack/grab classification already uses the same common defaults for
both families; no additional Captain/Ganon policy branch was found there.

The scratch generated ranged callback preserves queries at margins 102 and 58,
reloading the current target between them. It appends scripts 38, then 27/55/56,
with the original eight-slot capacity checks. Both queries still run when full.
The Falcon capture draft draws only when grab attempts are zero and level is
greater than 4; a draw below `0.05f * level` chooses script 57 below 100 damage,
otherwise 58. Returning false continues the common grab/pummel logic; Ganon has
no special capture draw or callback. Selected scripts are shared controller
bytecode, not stock fighter special handlers.

One generic operation remains proposed: `cpu.run_script(index)`, with Host
`void (*cpu_run_script)(Fighter*, int)` directly bound to `ftCo_800B4880`. An
integer index is evaluated once, accepted for `1 <= index < 62`, and otherwise
ignored. This operation selects the existing shared command script without
drawing RNG, modifying grab counters, or allocating. Its supported context is
CPU command-building decision hooks, not pure attack classifiers.
`capture.victim_damage` is now accepted in compiler checkpoint `fe7bf18a2`;
the capture draft remains inactive until command-script selection is implemented.

For fourth held swings, the proposed generic Definition hook is
`int (*on_item_swing_motion)(Fighter_GObj*, int swing_type, int variant)`.
The caller invokes it only for variant 3 before the stock kind switch, with
type 0..5; integer event fields expose those inputs. Falcon selects 341+type;
Ganon must retain the original unsupported fourth-variant boundary. Returned
motions must be valid authored states; a negative result means unsupported.
Variants 0/1/2/4 continue using the existing common matrix. This hook is not yet
implemented and requires no new Host primitive.

Ganon's wind already fits `effects.wind(Top, 0, frames, strength, decay,
frequency)`, with named defaults 80/1/0.003/1.0471976. Retail semantic Top maps
exactly to raw part 0. This uses the shared radial wind pool.
Stock `ftCo_800DEA28` falls through from the Ganon case to default:
wind is followed by two common taunt entries, then one statistics notification.
The draft preserves both entries. Existing `state.taunt(right,left)` binds
`ftCo_800DEAE8` but currently requires owned motions; accepting common
AppealSR 264 / AppealSL 265 is a remaining compiler-only range-policy proposal.
No new wind or taunt backend is needed.

The isolated baseline8 original-C fixture passes **3,746 comparisons**:
144 generated ranged cases, 1,800 planned Falcon capture cases, 1,800 Ganon common
capture cases without an override, and two default/missing-victim cases.
It checks ordered calls, RNG draws, grab counters, candidate contents and
completion/target clearing. Capture uses a planned C hook inside the actual
frozen common caller; it does not establish acceptance of unimplemented DSL.
Ranged comparison covers its exact family case, not earlier common candidate
assembly. Island endpoints, chosen bytecode execution, spacing integration and
gameplay are untested. Reproduction and SHA256 evidence are in scratch
`README.md`, `compare_cpu.py`, `comparison_result.json` and `results.log`.
The unsuccessful initial taunt syntax check is retained separately.

Retail PlCo/character asset inspection provides **52 Captain and 50 Ganon
semantic bone mappings**, with 63/84 physical and runtime parts, no omissions,
no article/joint-graph masks, no typed item table, and no raw byte spans. Effect
banks are 4/19; sound banks 6/30. Both retain 318 actions and 14 demo actions and
four explicit result/intro/ending/viwait demo names already present in the raw
clone definitions. Exact bindings, source-table entries and asset hashes are in
`owned_metadata_audit.json`, with `captain_bones.inc` and `ganon_bones.inc`.
Installing explicit bone metadata, completing the proposed policies and
validating full generated callback closure precede any independent declaration
or new hidden, muted, drawn gameplay acceptance.

## Implemented independent callback closure

The coordinated compiler/runtime batch implements the three prerequisites
above: `cpu.run_script`, integer `on item_swing_motion` callbacks, and common
taunt state selection. Both templates now declare independent behavior after
their complete generated modules passed the strict motion-field guard. The raw
catalog definitions retain stock fallback for untouched clones. Generated
modules clear `source_kind`, own the listed callbacks, and use explicit imported
bone, demo, effect and asset-layout metadata.

Six `motion_policy` declarations explicitly bind the engine's shared item-swing
callbacks to owned motion slots 341–346 from common slots 122/126/130/134/138/142.
These assign the four existing callback pointers directly, preserving each
owned motion's animation, flags and camera. The independent guard checks all
original nonnull phases across all 23 motion states. Missing common tables,
targets outside the owned motion table, and missing required common phases
reject the module; there is no blanket exception for inherited callbacks.

The templates now import **37 Captain / 36 Ganon parameters**: the prior 24
special-move values, eight shared CPU defaults, five Falcon capture decisions,
and four Ganon wind defaults. The audited 52/50 semantic bone bindings are
installed in their respective copied-base definitions. All named Captain/Ganon
kind branches outside their own source were audited: capture/ranged CPU,
fourth swing and common taunt are covered; other references concern catalog
metadata, Kirby's excluded copy-hat dynamics, and semantic diagnostics. No
additional family recovery, size or aerial-jump decision was found.

The actual full generated modules pass **3,905 original-C policy comparisons**:

| Coverage | Cases |
| --- | ---: |
| Ranged query order, target reload and candidate capacity | 144 |
| Generated Falcon capture and Ganon common capture, inside the actual shared caller | 3,600 |
| Default/missing-victim capture cases | 2 |
| Ganon root-position wind, two taunt entries and one caller notification | 8 |
| Six held-item types and five variants, including Ganon's unsupported fourth variant | 60 |
| Actual common spacing caller, timer boundary and RNG ordering | 84 |
| Named initial spacing defaults | 2 |
| CPU script indices 1/61 accepted, 0/-1/62 ignored | 5 |

The fixture uses the original source at `4c76e0752` and the matching new SDK
callers, loading both generated modules with their imported FMP tables. It
checks call order, exact RNG counts, counters, candidate contents and floating
results. Typed trap stubs make any unexpected special-move Host call fail;
the tested callbacks and lifetime checks use meaningful fixture implementations.
Both candidates return `source_kind == Ft_Kind_None`. The strict common-policy
rejection checks are additional assertions. No undefined-behavior or RNG
divergence was introduced or hidden to make these comparisons pass.

Reproduce with `tests/native/captain_ganon_policy_stock_compare.py`; matching
snapshot and projects are under `.local/captain-ganon-closure/generated`.
`fixture/policy-fixture-result.json` pins original/frozen source, generated
modules, scripts, FMP tables and fixture hashes; build/results logs retain
failed attempts. This replaces the earlier planned-C capture comparison as
evidence for the new generated policies. The older fourteen gameplay recordings
remain historical acceptance of the earlier stock-backed modules.

## Independent recorded gameplay checkpoint, 2026-09-30

Fresh independent packages pass **16 stock/clone comparisons**, eight per
fighter: the seven recordings above plus a 440-sample right-facing taunt.
The batch compares 8,280 controller samples with matching motion transitions,
fighter state/geometry, damage and RNG. Each run is hidden, muted and accelerated
with drawing enabled. The comparator verifies the custom behavior identity and
normalizes only that identity when comparing to stock. Packages live in isolated
test stores; this does not add duplicate installed fighters to the Collection.

Reproduce with `tests/native/captain_ganon_independence_parity.py --iso PATH
--out-dir NEW_DIRECTORY`. The runner accepts `--fighter Captain|Ganon` and
`--case NAME` to select a bounded subset. Its CPU case is diagnostic: the first
1,800-sample runs matched, but both stock and clone stood idle after spawning.
Those runs are **excluded from active CPU gameplay acceptance**. The 3,905
source-policy comparisons remain the evidence for authored CPU decisions until
the activation issue is resolved; matching idle states does not validate AI.

Evidence is pinned in `.local/captain-ganon-independent-acceptance.json`, with
the first batch and corrected remaining cases preserved under
`.local/captain-ganon-independent-gameplay` and
`.local/ganon-independent-gameplay-corrected`. Initial runner failures are kept:
the cloning tool expects `Ganon`, while match launch expects `Ganondorf`, and a
direct match requires a human slot. These were runner argument errors.

The full Release build passed; all 76 focused compiler/runtime checks are
covered by the first run and its two corrected targeted rechecks. The additional
Hookshot/Samus native owner-scale regression passes six production-body cases.
Baseline9 freezes runtime, SDK and compiler with 3,164 hashes. Rook was rebuilt
for the new Host/Definition layout without changing its authored source; its
installed package is
`3bd6ed2c5e5a168a956d8c1f748615edb0b47d33cbbd7c4f5866791cf1ed4122`.

This closes the generated callback ownership and the recorded special/taunt
paths. It does not prove every input branch, long match, costume or active CPU
scenario. No intentional stock gameplay or RNG mismatch was needed in these
accepted comparisons. Where future stock code relies on undefined behavior,
player-visible fidelity and a documented defined initialization take priority;
native replay and rollback still require native clients to agree.

## CPU activation and normal life initialization, 2026-09-30

The idle CPU diagnostic exposed a native lifetime-check error shared by stock
and custom fighters. `Fighter_UnkInitReset_80067C98` advances `x8_spawnNum` for a
new life. The death/reset caller captured its callback generation before that
reset, so its later live-source check rejected an ordinary callback and returned
before `ftCo_800A101C` initialized CPU configuration. A debugger snapshot showed
P1's Player slot correctly set to CPU type 4 / level 9, while the live fighter's
CPU configuration remained zero even as its AI tick counter advanced.

The caller now captures the generation after its own reset. Removal or recycling
by later callbacks still stops processing. An actual-production-body fixture
checks healthy callback ordering through CPU setup and the tail, four
removal/recycling cases at the common or death callback, and the no-callback
path. All eight focused lifetime/event regressions pass after the full build.

The complete coordinated gameplay batch was rerun after this correction:
**all 18 stock/independent pairs pass**, including genuinely active level-9 CPU
matches. Falcon records 62 action transitions and 11 damage events; Ganondorf
records 87 transitions and six damage events. The runner explicitly rejects
idle CPU traces. The seven special recordings and taunt for each fighter also
pass again, comparing 11,880 controller samples across the eighteen pairs.

Updated evidence lives in `.local/captain-ganon-generation-acceptance.json` and
`.local/captain-ganon-generation-gameplay`. Baseline10 freezes the corrected
runtime, SDK and compiler. Baseline9 and its excluded idle CPU evidence remain
historical. There is no Host/Definition layout change, so Rook's refreshed
package remains valid. This is a native initialization repair, not a deliberate
stock behavior or RNG deviation. Full branch/costume coverage remains unproven.
