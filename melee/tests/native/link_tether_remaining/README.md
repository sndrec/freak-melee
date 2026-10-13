# Remaining Hookshot article source slice

This namespace is an unregistered full draft and a standalone console source
comparison. It preserves the accepted held, deployed-retraction and AirCatch
namespaces unchanged. It does not activate Link/Young Link or run a game.

```powershell
python -B tests/native/link_tether_remaining/source_compare.py --snapshot .local/baseline36 --output .local/link-tether-remaining36-final
```

Use a fresh output directory. The normal frozen content tool clones and imports
each owned project, generates the complete C module and builds its DLL. The
fixture loads those actual DLLs, executes their article callbacks, and compares
the unchanged extracted original callbacks and solvers. `--link`, `--young` and
`--iso` overrides are recorded. `--prepared` is a source-identity-checked debugging
option; it is distinguished from fresh imports in the receipt. Frozen SDK,
compiler and tool hashes are verified before and after.

The final receipt is `.local/link-tether-remaining36-final/acceptance.json`:

| Fighter | Assertions | Live graph comparisons | Spool comparisons | Maximum float difference |
| --- | ---: | ---: | ---: | ---: |
| Link | 84,169,950 | 122,684 | 57,192 | 0.000000953674316 |
| Young Link | 59,528,074 | 121,500 | 56,180 | 0.00000190734863 |

Combined: **143,698,024 assertions**, **244,184 live graph comparisons** and
**113,372 spool comparisons**. Graph comparisons include the selector-only
physics phase and the subsequent accessory2 update; these are not gameplay
frames. Per-state deferred updates are reported in order `[1,2,3,6,7,8]`:
Link `[19536,7344,6776,6504,4880,20964]`, Young Link
`[19536,7344,6720,6484,4768,20836]`.

The matrix covers three positive scales (0.5, 1, 1.75), both facings, three
first-deployed positions, 17 controlled stage/island/moving-line/obstruction/
ledge responses, ground/air, A pressed/absent, valid/invalid creator motions and
up to three ordered frames. State1 also covers launch equality in standing212,
dash214 and AirCatch360, using the corresponding owned timing parameter and
normal constructor count. State8 independently tests timers -1, 0, 1 and 180.
Physics installs the deferred solver; accessory2 performs the actual update.
The next frame uses the actual transitioned state. Terminal cleanup verifies
all allocated nodes are freed on both sides.

State1 covers deployment, wall bounce, floor/full-chain outcomes, the air wall
transition to361/state6 and ordered effects. State2 covers gravity/friction,
continued deployment/bounce and state3. State3 covers slack constraints and
invalid-motion retraction. State6 covers both short-circuit obstruction rays,
moving wall lines, extension/state8, ground retraction and A-triggered state7.
State7 covers incomplete reel, completed reel, ledge probe/occupied decisions,
cliff jump and removal. State8 covers hanging constraints, creator displacement
and velocity accumulation, ground/A exits and expiry before decrement.

The receipt includes actual observation counts for particles, floor sounds,
damage fall, ledge probes/occupants/catches, cliff jumps and obstruction rays.
Both fighters execute every named outcome category. The fixture compares node
positions/velocities, collision history/environment, deployed/floor/sound flags,
stored surface lines, model matrices and draw behavior, creator position/
velocity/delta, state transitions and engine-boundary call order. Spool tests
every contiguous deployed fraction, including zero/all deployed, then repeats
animation on reachable update states.

The existing full drafts required one new source-backed correction here:
`FlightStep` stores a wall line only in state1. Original `it_802A4BFC` does so;
state2's `it_802A5320` does not change it. Right-only wall fixtures use differing
left/right IDs to make this distinction visible. These new candidates retain
the accepted AirCatch frame0 and failed-constructor continuation corrections.
No accepted candidate, core/compiler/Host/default or CMake file changed.

Frozen34 originally exposed a generic Bool-store defect: `input.attack_pressed`
returned the A mask0x100, and byte storage truncated it to0. That failed the
state6→7 comparison. The partial no-A diagnostic is preserved under
`.local/link-tether-remaining34-noattack/diagnostic.json` and explicitly is not
acceptance. Root repaired canonical Bool storage; the final frozen36 matrix
includes all A-bearing configurations. The current runner has no filter that
can omit them.

The original `it_802A4BFC_sqrtf_offset` writes beyond a single local float to
reconstruct retail stack placement. That native C undefined behavior is not
executed. The fixture substitutes exactly that helper with deterministic
`sqrtf`; all other extracted bodies remain unchanged. Its source comment calls
it an MSL sqrt expansion; the three reciprocal-square-root Newton refinements
followed by multiplication by the input establish the intended square-root
computation. No Hookshot retail assembly is present in this checkout. This
boundary is behavioral native math acceptance, not retail instruction rounding.
The measured differences use the explicit absolute/relative tolerance in the C
fixture, and discrete state/call/flag decisions must still match exactly.

Spool rotation/XY-scale values match within that tolerance. The generic joint
writer marks its joint dirty; the original raw assignments do not explicitly do
so. The fixture records 512/368 dirty-flag differences in its isolated spool
census. It does not claim equality of those flags or prove a renderer effect.
The production joint implementation supplies its existing safe dirty semantics.

Production resource, joint, lifetime and DAT model-root import bodies are linked;
the actual Item model-slot selector and joint writer bodies are extracted as
well. Controlled boundaries remain creator/active Definition lookup, HSD graph
allocation/model creation/free, matrix/vector helpers including normalization,
ECB/stage/island/moving-line responses, root Item creation/pickup/scale/removal,
fighter hit/ledge/damage/cliff-jump operations, and particle/sound emission.
Actual generated signal callbacks execute through controlled dispatch lookup.
Hitbox-clear sequencing is checked with inactive capsules. Unused required Host
operations trap. At least one node is deployed during solver updates; invalid
all-undeployed searches and stale-pointer stock teardown are not executed.

Remaining closure includes ground Catch/CatchPull/CatchWait integration, complete
damage/death composition with the other authored articles, retained capture
borrower gameplay and final default activation. Constructor/held/retraction and
AirCatch input acceptance remain their separate scoped receipts. No native
tether solver, stock fighter delegate, new API, global build or game was added.
