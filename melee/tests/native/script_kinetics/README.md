# Fighter animation, motion and physics binding family

The shared `engine_fixture.c` owns only engine leaves and controlled package
views. The standalone primitive runner links `unavailable_bindings.c` for
binding families it does not exercise; complete CMake fixtures instead link
their real implementations. `unavailable_runtime.c` supplies explicit rejected
Item/stage/vector boundaries for the broader primitive and receiver probes.
Fresh standalone outputs can be placed on any drive.

The current runner also publishes a controlled owned Fighter identity for
`animation.clip_length`. It checks gameplay and demo tables, empty clips and
IDs absent from each table, including the gameplay IDs queried by load callbacks
during results construction. A wrong table/count pairing, null table or invalid
index must fail without publishing an output. This tests the actual binding and
source-layout admission; imported assets and actor publication are fixture
boundaries, not a full results-scene proof. Item and stage operations remain
unavailable in this console fixture.

The October 11 run at
`A:/FreakMelee-test-archives/script-kinetics-demo-oct11-c/receipt.json` passes
26,148 checks, including 7,104 existing native-body comparisons. Earlier attempts
`-a` and `-b` stopped at compilation (a signed comparison and missing isolated
fixture dependencies); neither is runtime acceptance.

The production semantic catalog resolves only source-checked names in this
batch. All operations are fighter-context generic primitives. It adds animation
finished/rate/ID/length/delta properties, numeric motion ID/launch angle,
ground velocity/acceleration/friction and common air drift coefficients.
Current animation frame remains the existing `frame`/`frame.integer` spelling;
there are no invented `animation.frame` or `animation.remaining` aliases.

Calls cover animation start/rate/timer/sample clearing, direct motion velocity
operations and generic friction, drift, acceleration, deceleration, gravity,
clamping, animated movement and fastfall primitives. `physics.fall(gravity)` uses
the caller's common terminal velocity; its two-argument form uses the explicitly
supplied terminal velocity. Ground/air acceleration retains the original implicit
zero threshold, while `air_accel_common` uses the common drift threshold.

Native calls enter the substantive engine functions directly. No legacy
NativeFighterHost adapter, stock-special handler or authored C callback is used.
State transitions, text/motion-name resolution, facing-speed entry operations
requiring ground/air entry metadata and unimplemented names remain unsupported.

Marshalling follows actual source semantics. Physics/native animation formals
are Float. Whole/planar velocity scaling evaluates a Float scale, as the previous
authored expression explicitly did. Compound vertical add, horizontal multiply
and horizontal divide retain NumericAny so Double arguments perform Double
arithmetic before Float storage. These direct native field operations also retain
their original IEEE floating zero-divisor behavior; general VM arithmetic keeps
its separately documented checked-division contract.

Animation start preserves the actual `ftAnim_8006EBA4` four-stage order and checks
the original actor generation after each stage. Physics/animation engine calls
carry conservative source-invalidation flags; VM continuation checks stop a
retired/recycled caller before publishing results or subsequent authored writes.
This does not claim that every internal native engine callback loop has been
audited for retirement; the stages themselves remain engine-owned.

`extract_primitives.py` extracts actual generic ftmovement/ftcommon/ft_084E/
ft_081B/ftAnim bodies from the live source without editing them. The generated
header is test-only. `engine_fixture.c` supplies controlled HSD graph, audio,
immutable Definition observation and four animation-stage leaves. Healthy start
ordering compares against the actual original body, while retirement is injected
at controlled stage exits. The fixture does not execute real animation graphs,
rendering or subaction scripts and makes no gameplay claim.

The main console test compares native binding results/mutable fields with those
actual bodies across gravity, acceleration, clamping, drift, friction, animated
movement, fastfall and rate deferral. Direct compound operation cases compare
against source C arithmetic and explicitly require Double-sensitive examples.
The direct production frontend/bytecode/VM executes a Cape/Tornado-style physics
composition and proves animation-stage retirement prevents the following write.

```powershell
python tests/native/script_kinetics/run.py `
  --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe `
  --output A:/FreakMelee-test-archives/script-kinetics-example
```

Other isolated native-binding/execution fixtures may reuse `engine_fixture.c`
with `script_kinetics_actual.h` generated into their build directory. Include
that directory and compile the new production `native_script_binding_kinetics.c`
alongside the existing native binding units. Test dependency symbol definitions
must be reconciled explicitly; these controlled boundaries never enter a
production binary. Source extraction retains the original bodies; known unused
locals/parameters and signed loop-bound warnings are suppressed only around the
extracted header. New production/fixture code retains strict warning checks.

Primitive-only targets deliberately return NULL from the package getter and
expose absent Fighter program/View/Status. State, transition and receiver targets
set SCRIPT_FIXTURE_SOURCE_ACTOR and provide controlled package/data/motions,
selected bank, root storage, View and Status. They call the real ownership
validator; twenty mismatches reject before native mutation or output publication.
Transition/receiver use actual compiled banks and Execution/VM. This fixture
claims neither DAT preparation nor the actual Fighter scheduler.

The exact nativeScriptMoves_Contains body is included in the extracted primitive
header. Complete-bank targets set SCRIPT_FIXTURE_FULL_MOVE_UNIT and link its
actual source unit instead. Neither path substitutes a bounds rule.

The reset cohort also covers `physics.clear_kinetics()` through the actual
`ftCommon_8007E2FC` body and compiler/VM. Nonzero values are seeded in every reset
channel and compared against the original helper, including knockback and shield
knockback. This primitive-only fixture does not publish a source Fighter actor;
unselected binding implementations remain unavailable and fail closed. Empty
GObj pool storage uses the real pool reference implementation. Whole-package
activation and rendered gameplay are separate checks.
