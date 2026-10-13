# Samus controlled-contact flight checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

2026-10-11. Samus remains an unregistered authoring candidate. This expands the
[retraction and hitlag comparisons](native_source_samus_retraction_hitlag_checkpoint.md)
to extension and falling flight on real retail graphs. The runtime still
interprets scripts; compiling original C here provides the comparison reference.

## Scope

The connected factory corpus adds 72 cases each for modes 1 and 2, covering
four starting motions, three scales, both facings and full/half/endpoint-only
deployment. Each source accessory callback compares with the original native
`fn_802B805C` or `fn_802B8384`, through eight normal updates or the first state
transition. Starting chains and state entry are explicit fixture inputs.

The complete original Samus translation unit runs with its sweep symbol
renamed. The unchanged `it_802A3D90` sweep body is extracted from the original
Hookshot source; its external geometry leaf receives the same controlled inputs
as the source resource kernel. Those inputs shift the contact point and provide
no-contact, left-wall or floor flags, surface indices and a floor normal. This
compares steering, gravity, deployment, contact responses, damping and state
transitions without substituting the solver's decisions.

Motion 357's mode-1 aerial wall branch invokes full Fighter state and effect
construction. That case uses floor contact instead; the missing wall path is
not replaced with a handler stub. Neither actual stage geometry nor subsequent
slack/wall-state updates are accepted by this cohort.

Every solver update also checks contact flags, surface indices/normals and the
script-owned deployment/on-floor arrays against original link state. Joint,
controller, material/palette and override-matrix comparisons remain intact.
Controller and RNG progression compare exactly; float comparisons retain
`2e-6 + 2e-5 * max(abs(actual), abs(expected))` tolerance.

## Scalar attribute correction

Strict compilation of the original Samus reference exposed the final special
attribute's incorrect unknown-pointer declaration. Its sole reader treats it
as an integer hang duration, consistent with the script's existing
`tether_hang_frames` attribute. The native declaration is now `s32`, and its
reader uses that name directly. Compile-time assertions check retail offset
`0xD0` and total attribute size `0xD4`; an eight-byte host pointer previously
inflated that data structure. This corrects a native data-layout mismatch,
without changing the gameplay timer.

The first fresh frozen comparison compile also exposed an include-resolution
issue in the runner: both `it/` and `it/kinds/` have an `inlines.h`. Moving the
original Samus source into the snapshot root chose the wrong one. The copied
translation unit now stays beside its original source, preserving quoted
include lookup. The failed receipt is retained separately.

## Evidence and limits

`A:/FreakMelee-test-archives/samus-flight-oct11-final-b/receipt.json` passes:

- 144 flight pairs, 480 normal updates and 96 state transitions.
- 360 actual Item factory cases, 720 initial held callbacks and 540 pre-launch
  hitlag callbacks, retaining allocation-refusal and cleanup checks.
- All previous constructor, held, retraction and post-launch hitlag cohorts.
- 11,473,912 assertions and 113,836 material/palette comparisons.

The fresh Link regression in
`A:/FreakMelee-test-archives/script-item-flight-regression-oct11-final/receipt.json`
passes 7,033 assertions across 25 cases, including 39 animated graph pairs.
Both receipts confirm frozen inputs unchanged and matching their live inputs.
The native client/source-asset build and all four focused asset, parameter,
resource-metadata and script compiler CTests pass.

Parent Fighter construction/publication, initial chain/capture state and flight
entry remain staged. Actual stage geometry, aerial wall transition, downstream
slack/wall constraints, grabbed-victim lifecycle and rendered gameplay still
require connected acceptance before independent Samus registration. Native
comparison graphs are explicitly freed rather than claiming full native
completed-removal parity. No visible game, installed content change or upstream
push occurred. The full project goal remains active.
