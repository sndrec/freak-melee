# Samus Grapple Beam article phase composition

This is an **unregistered, incomplete source fixture**, not an installable Samus
template. It composes the accepted Samus specials/combat articles and tether
fighter helpers unchanged with thirty readable Item-context helpers, all nine
Grapple Beam physics callbacks, deferred normal updates, hitlag constraints,
interaction detachment and resource removal. It delegates no solver decision
to the stock Samus handlers.

The final receipt is saved by the runner as `acceptance.json`. The SDK53 matrix
contains 13,392 normal timelines (up to three ordered physics → accessory2
updates), 270 accessory3 cases and nine interaction configurations. Of the
normal timelines, 12,108 never reach the undefined native pointer read and
1,284 require the explicitly substituted boundary described below. Those
1,284 are not counted as defined original-C equivalence.

## Reproduce

Run from the repository root with a fresh output directory:

```powershell
python -B tests/native/samus_grapple_article_composition/source_compare.py --snapshot .local/baseline53 --output .local/samus-grapple-article53-repro
```

The runner checks the frozen SDK/compiler/content-tool hashes before and after
execution, clones Samus normally, imports every scalar parameter from the real
stock DAT, generates C, builds a standalone DLL and runs a console executable.
It extracts only actual function definitions from the frozen source. Generated
C, DAT/FMP, executable, DLL, logs and packages stay in the output directory.
The receipt records commands and all source/input hashes. No game process,
package installation or broad build is involved.

The runner reads `examples/samus_wip.fms` and the committed
`samus_tether_composition/fighter.component.fms` directly. It does not alter or
copy alternate versions into accepted namespaces. The synthetic comparison
base supplies the real special-entry numbers and 400 fixture motion rows; it
does **not** establish copied-base ownership or independent package acceptance.

## Executed phases

| State | Actual source update | Authored decisions compared |
| --- | --- | --- |
| 0 | `fn_802B7E34` | Endpoint attachment matrix, ordered root/node animation advancement, allowed-motion removal |
| 1 | `fn_802B805C`, `it_802B9328` | Standing/dash/air launch reset, endpoint integration/ECB result, stochastic chain gravity, deployment, minimum/maximum length, floor reflection and state changes |
| 2 | `fn_802B8384`, `it_802B99A0` | Stochastic gravity, horizontal friction, endpoint collision/bounce, full extension |
| 3 | `fn_802B8524`, `it_802B9CE8` | Slack-chain gravity/integration/minimum and maximum length, endpoint friction, retract on disallowed motion |
| 4 | `fn_802B8684`, `it_802BA194` | Ground/air retraction speeds, flag removal, forward random gravity, collision-history damping, held-state return or removal |
| 5 | `fn_802B8814`, `it_802BA2D8` | Actual shared `Item_RetractChain`, capture retraction, completion flag and next-update removal |
| 6 | `fn_802B895C`, `it_802B9FD4` | Two segment obstruction checks, moving wall, deployment, immediate transition to hang, grounded retract/attack pull |
| 7 | `fn_802B8B54`, `it_802BA3BC` | Two deterministic-gravity forward passes, anchored endpoint, retraction and reverse constraints, fighter displacement, ledge/jump completion |
| 8 | `fn_802B8D38`, `it_802BA5DC` | Moving wall, deterministic forward constraints and slack termination, fighter velocity/position delta, hang timeout and exit priorities |

All nine actual `itSamusgrapple_UnkMotionN_Phys` bodies execute. They only store
the pending update pointer. The actual `it_802BAC80` accessory2 body then invokes
that pointer, and the generated physics callback only stores the ordinary
integer solver selector before the generated accessory callback signals it.
The matrix checks the intermediate phase before executing that signal.

The hitlag matrix executes actual `it_802BACC4`/`it_802BA760`, comparing the
before-launch matrix path and reverse constraints followed by forward random
gravity. It asserts that **neither path advances animation**. The interaction
matrix executes actual `itSamusGrapple_Logic53_EvtUnk`; creator detachment remains
independent of current damage owner/reflection owner, and does not invent an
article removal at that event.

Scale0.5/1/1.75, both facings, fully/partly/tip-only deployed chains, floor/left
and right wall/no-contact responses, moving lines, obstruction, ledge outcomes,
ground/air, attack input, disallowed motion and timer expiration are batched.
Positions, velocities, collision history/flags/lines, deployment, cached model
matrices/flags, fighter motion/position/velocity/delta, completion flags and
ordered engine events are compared. RNG draw counts and animation call counts
must match. Every surviving timeline then executes original and generated
cleanup and verifies every allocated node was released.

Production resource storage/geometry/joint operations, shared model-joint
operations, callback lifetime checks, parameter parsing, DAT import and named
model-root import are linked. The actual shared `it_802A7168`, `it_802A6F80`,
`it_802A3C98`, integration/collision-history/moving-line helpers and actual
`lbVector_Mirror` body execute on the original side. Mirror intentionally changes
only x/y; the authored translation preserves z.

## Boundaries and qualifications

Node construction and attribute scaling are **shared controlled setup**, not
an original/generated constructor comparison. Setup reads real DAT values and
imports all four named joint roots. HSD allocation/model construction/free,
renderer setup and animation advancement are controlled; advancement order
is tested, not actual animation evaluation or rendering. The indirect
animation-component arrays are not installed by this candidate.

Other controlled boundaries are matrix/vector arithmetic (including normalize),
ECB/stage/moving-line responses, live creator/Definition lookup, article lookup
and synchronous signal dispatch, item scale application and fighter engine
transitions/ledge/capture operations. Unused required Host operations trap.
The random stream uses reproducible dyadic samples that exercise both stochastic
gravity branches; this is not a general RNG bit-equivalence claim. Existing
specials/combat articles compile in the same DLL but are not retested here.

In native decompiled `it_802BA3BC`, the first x constraint dereferences
`dir_ptr` before its later assignment to `&dir`. Only that expression is
substituted with the already-computed `dir.x`, consistently with y/z. The fixture
counts each timeline reaching it separately. It makes no claim that retail
PPC has the same defect, and does not edit stock C or replay pointer UB.

Native unsuffixed double literals0.9/0.12 use ordinary authored Float expressions.
The observed maximum difference is measured; no numeric hacks or float IDs are
introduced. The source's hang-duration cast from `ftSs_DatAttrs.xD0` (currently
typed `UNK_T`) remains extracted as written, with its pointer-narrowing warning
visible. Fixture setup copies the real32-bit DAT word into a zeroed field; it
does not dereference that value as an animation pointer.

All scenarios keep original creator/item generations live through synchronous
engine boundaries. Production source-retirement safety is separately tested
by accepted engine lifetime/resource fixtures. This matrix does not pretend
its controlled transitions prove those real engine side effects.

## Still pending

The physical Up/Down/A modified-grapple sequence, mode1 homing/full hitbox
descriptor, AirCatch extension wall-hit particles, full constructor and indirect
animation assets, actual renderer, throw attachment and independent lifecycle
activation remain gated. AirCatch wall-contact dispatch is excluded by an
explicit matrix condition; standing/dash wall contact and air floor/free
extension still execute. The candidate's healthy-mode domain is narrower than
the complete original article and must not be activated as a full port.

`API.md` gives the exact remaining generic contracts. No default template,
shared compiler/runtime, accepted component, snapshot or CMake file is changed.
