# Independent Peach Workshop template

[peach_full.fms](../examples/peach_full.fms) is the starting script for a fresh
Peach draft. It owns Float and its aerial attacks, all four specials, forward
smash selection, lifecycle, identified CPU decisions, Debug-ROM taunt cosmetics
and all five associated articles. The direct interpreter declares independent
behavior; its callbacks never delegate to a stock Peach handler.

The owned source package supplies copied assets, layout facts and import recipes.
Its prepared programs contain the independent callbacks. Existing saved
Workshop scripts retain their own text. The template starts as an editable
draft, following the other Workshop starting templates.
Article rendering uses the existing standard renderer explicitly; no new paper
or fighter-kind renderer routing is involved.

The [interpreter article checkpoint](native_source_peach_articles_checkpoint.md)
records current preparation, real construction/cleanup and live-match evidence.
The generated-C comparisons below are historical evidence, not a claim that
the interpreter has repeated every stock comparison.

## Editable ownership

Normal project import copies all five article resources, costume/animation/demo
data, sound/effect banks, 114 physical joints and 53 semantic bone bindings.
There are no inserted omission joints. The template's `attribute` declarations
are one-time stock import recipes. Import turns their 67 raw declaration words
into owned project values among 151 editable parameters. Neither gameplay nor
subsequent editing reads tuning from a donor fighter.

Script state owns Float availability/timing, smash history, saved article handles
and move-local decisions. Shared motion, physics, collision, HSD material and
article construction operations remain native engine primitives. Helpers use
ordinary typed returns and live references; they do not expose Peach's private
C union as public scripting state.

## Source and package checks

The [full travel composition](../tests/native/peach_parasol_travel_composition/README.md)
compares 59,355 original/generated source pairs, including the complete Parasol
producer/travel policies and preceding Float, smash, Toad, Spore, Turnip,
side-special and resource cleanup components. Allocator, motion, collision and
render boundaries are identified in each component; source tests are not
rendered-game acceptance.

The [independent activation composition](../tests/native/peach_activation58_composition/README.md)
verifies normal clone/import, generated DLL ownership, immutable parameters,
package creation and actual behavior-selection cases: 11,810 checks, 35
Definition callbacks, 114 nonnull phases, 56 article callbacks and 30 selection
cases. Of the 114 phases, 106 are script-owned and eight bind ordinary shared
common policies. Taunt comparisons add 7,680 healthy source pairs and 107,592
checks, plus separate invalidation cases for generated callbacks and the guarded
common caller. The original saved model group `.prev` is distinct from current
visible `.idx`; the debug cosmetic toggle preserves that distinction.

The published template's executable code matches the reviewed assembled
candidate; only comments differ. Frozen SDK59
prepared package: `f1a99ce55067e73de5df47fe12e689459827c2622b49b9055b2b1908a08f5987`.
This package is accepted only in an isolated test content store.

## Actual gameplay batch

Six initial and ten extended hidden, muted, drawn comparisons run the actual
native game against the retail Peach path with identical recorded inputs:

| Cases | Exercised behavior |
| --- | --- |
| Toad, pluck, Parasol, side-B | Grounded special startup, article construction and ordinary exit |
| Air Toad, Air Parasol, Air side-B | Airborne startup and travel/landing |
| Four left-facing specials | Mirroring, orientation and corresponding owned article paths |
| Empty-handed airborne down-B | Original deliberate no-op, continuing ordinary jump/fall |
| Float and Float neutral attack | Held jump/down input, Float entry, authored aerial mapping and release |
| Repeated forward smash | Actual owned motion selection and RNG history |
| Level-nine CPU Peach | Actual CPU decisions, movement, pluck, held-item handling and damage events |

All sixteen comparisons match fighter/action/item/damage/RNG traces after
normalizing only the declared independent fighter identity. The first six use
550 controller samples each; the extended human cases use 650. The CPU input
recording has 1,200 samples; active gameplay finishes after 800 ticks, so this
case claims those active traces rather than 1,200 gameplay ticks. There are six
damage events and 391 item-state records in that CPU comparison.

Evidence: `.local/peach-runtime59-installed/`,
`.local/peach-runtime59-extended/` and
`.local/peach-runtime59-coverage-corrected/`. Two preliminary extended cases are
excluded: airborne pluck wrongly demanded motion353, which retail air down-B
does not enter when empty-handed; the CPU setup initially supplied no human
slot and was rejected before gameplay. Corrected cases preserve their inputs
and full exact trace comparisons. No move logic was altered to pass them.

Reproduce the prepared source/package gate and the complete runtime batch:

```powershell
python -B tests/native/peach_activation58_composition/prepare.py --snapshot .local/baseline59 --output .local/peach59-reproduction
python -B tests/native/peach_runtime/verify.py --snapshot .local/baseline59 --prepared .local/peach59-reproduction --output .local/peach59-gameplay-reproduction
```

The batch accepts only the prepared isolated package, uses `--hidden --mute`,
retains drawing and runs faster than real time. It never installs a fighter in
the user's Collection, sends OS input or changes a user's Workshop draft.

## Remaining limits

These runtime comparisons execute real constructors, resource loading, animation
and the common rendering path. Trace equality includes pose, hitbox, hurtbox,
visibility and article visual hashes; it does not establish screenshot/pixel or
audio-waveform equality. A complete results-screen visit, every Float attack,
every rare item/Toad counter/contact branch and all multiplayer combinations
have not been separately exercised in this batch. The corresponding source
comparisons provide broader branch coverage, with their disclosed boundaries.

An untouched stock Bowser still selects a stock Peach victim animation by raw
kind; that mixed stock/custom interoperability limitation is thrower-owned.
The scripted Bowser path queries the generic animation family correctly.
Kirby copy behavior remains out of scope. Original unsafe null or uninitialized
branches are not promises of parity; safe authored choices are documented in
their component ledgers. This template's promotion is a usable independent
Workshop starting point, not a claim that the whole project goal is complete.
