# Zelda Neutral / Din Fire / Explosion source composition

This is an unregistered partial authored component, not an independent Zelda
clone or an installed package. It changes no defaults, source-kind selection,
transform destination, paired-form metadata or CSS presentation.

## Reproduce

Run from the repository root with an immutable SDK57 snapshot and a fresh
output directory:

```powershell
python tests/native/zelda_script_composition/run.py --root . --baseline .local/baseline57 --output .local/zelda57-root-repro
```

The runner decodes the actual ISO's `PlZd.dat`, stages 29 named parameters in
`fighter_parameters.fmp`, emits the real module with the frozen content tool,
compiles both a DLL and the console fixture with the matching frozen compiler
and SDK, then executes actual generated callbacks alongside extracted original
C. It records commands, exact source/body hashes, retail offsets/values,
compiler/SDK/input hashes and output hashes in `result.json`. No game is run.

This parameter staging is an independent big-endian DAT reader. Normal Zelda
clone registration and the normal project/import pipeline remain pending; this
does not claim those paths have been exercised. Article slots are the actual
Zelda OnLoad slots: Din Fire 0 and Explosion 1. The retail DAT SHA256 is
`e523bb0fe551f64aa6ae5b3ae59cd1e62434baeded076d4a5e1ea993e122a79a`.

## Authored decisions and comparisons

`candidate.fms` owns Neutral entries 341/342 and all eight phase callbacks,
reflect-hit no-op and accessory particle choice/flags/hitlag binding. It also
owns Din's two article states, Explosion's single state, their initializer
decisions, cleanup/reflection/interaction and other registered item callbacks.
The stock union state is projected to ordinary named script variables; no
script calls a stock Zelda/Din handler.

The final matrix covers 821 original/generated pairs and 40,123 checks, plus
six separate authored source/item retirement or same-address recycling cases.
It checks ordered engine observations and individual semantic values,
including exact float bits. It compares base hitbox radius and resulting radius
separately and never compares struct padding. Cases cover delayed gravity,
reflection commands and counterpart collisions; steering gates, clamping,
reflected/null/changed owners, missing control source, charge/expiry ordering
and duplicate burst; Explosion scale/damage/radius, preserved X velocity and
feedback thresholds; detach/holder cleanup; held-item presentation, knockback
and death reset. Child visibility checks also preserve the selected child's
next sibling, matching actual HSD subtree traversal.

The actual originals come from `ftzeldaspecialn.c`, selected getter/cleanup
bodies in `ftzeldaspeciallw.c`, Din/Explosion files, `ftmovement.c`, `it_2725.c`
and lifecycle bodies in `ftzelda.c`. The extractor records exact line/body/file
hashes. Shared fall/deceleration, lifetime setters/ticks and child visibility
helpers execute their real source bodies. The actual callback generation
registry is linked; source safety does not use an always-true liveness mock.

## Controlled boundaries and remaining scope

Allocation/model construction, motion publication/animation availability,
stage probes, particle rendering, recipient dispatch, child Explosion
allocation, debug notification and hitbox damage engine mutation are explicit
controls. Both compared sides observe the same boundaries. Recipient export
lookup exercises the generated exported callbacks, but the controlled lookup
is not evidence for the production recipient dispatcher (covered separately).

Din's initializer is compared after its real constructor's pre-initializer
timer setup; that setup is controlled. Explosion's entire real initializer is
compared directly, including immediate animation, expiry and feedback order.
Neither comparison claims the complete SpawnItem constructor/allocator path.
Load/reload callbacks are authored but their attribute-copy/asset-registration
composition is not part of this source matrix. No RNG, asset rendering,
rollback or gameplay acceptance is claimed.

Side-B fighter phases 343..348 remain pending SDK58's generic read-only Int
`fighter.creation_id` so their retained article current-owner comparison can
preserve full source identity. SDK57's `articles.ref(...).current_owner` is
available, but the current Neutral/article component does not need it yet.
Teleport 349..354, transform 355..358, CPU policy and paired CSS/package coupling
remain separate. Debug dress toggling is deferred outside this versus slice.
The component is intentionally not marked behavior-independent.

Two reflector callback assignments differ internally: the generic reflector
creation installs no hit callback, immediately followed by the authored
reflect-hit binding. No intervening operation dispatches reflection, and the
bound no-op behavior is compared. Native item/fighter retirement cases are
authored safety extensions and are kept separate from healthy original-C
pointer parity. No digests or semantic fields are masked.

Historical `.local/zelda55-*` attempts contain harness prototype and callback
slot diagnostics. `.local/zelda57-source-final` retains a harness member-name
diagnostic; these failed preparations are not acceptance receipts.
