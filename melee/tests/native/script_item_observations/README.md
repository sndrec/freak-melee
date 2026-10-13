This console fixture compiles seventeen authored Item helpers and executes them
through the real frontend, bytecode VM and catalog. Item actor publication,
attachment/spec/status/view and immutable descriptor inputs are controlled;
the physical pool, callback registry, schema checks and observation backend are
real. It does not establish actual Item actor/scheduler or asset import acceptance.

The grouped matrix covers `disable_surface_response`, `sweep_stage_flags`,
`animation_finished`, `sweep_floor`, `wall_contact_flags`,
`ceiling_contact_flags`, `reflect_from_wall`, `align_to_floor` and
`align_facing_to_velocity`.

It also covers initial projectile launch angle/speed/origin, reflection facing,
pending hitlag-request clearing, and previous-to-current stage segment sweeps.
All six new scalar reads execute through the VM and preserve Float bits,
including signed zero, subnormals, infinities and quiet NaNs. Stage hit/miss
payloads are compared against the mechanically extracted `it_8026E9A4` with a
controlled `mpCheckAllRemap` geometry leaf. The leaf checks native arguments and
can inject source retirement before publication. This is a controlled geometry
boundary, not an assertion that the native geometry routine invokes scripts.
Hitlag clearing compares the complete Item payload against the extracted stock
`itFoxIllusion_Logic14_DmgDealt`. These operations require no model graph.

Complete native bodies are mechanically extracted from current source. Cases
compare the whole Item payload and four JObj payloads against direct native
execution, including floor/wall/ceiling line priority, optional adjustment order,
inward-only wall reflection with the actual `lbVector_Mirror` and damping,
floor orientation across contact priority/facing/axis and primary/dynamic-bone
selection, and the exact facing deadzone, signed-zero, NaN and infinity behavior.
The real `lbVector_Angle`, `it_80272CC0`, `mpCollSetFacingDir` and JObj dirty
propagation execute. Stage geometry and optional ECB adjustment are controlled
leaves; the geometry helpers publish explicit test positions/results.

Animation observations execute actual `it_80272C6C`/`lb_8000B09C` traversal over
JObjs/AObjs with instance flags; a NULL graph is finished. Real class ancestry
admits sufficient JObj subclasses. Floor alignment rejects an absent required
model descriptor, dynamic-bone table, joint, wrong class or insufficient extent;
without floor contact its native no-op does not require a model. Wall reflection
requires attributes only when its native wall branch consumes them.

Operation-specific post-complete observers are inserted after each tested native
kernel, before result publication. Nested field helpers do not trigger another
operation's observer. Mutations test logical/physical retirement, pool reuse,
schema changes and graph replacement; these observations do not assert that
native stage/vector routines execute authored callbacks. Actual class ancestry
initialization runs a controlled initializer to test the pre-native source/class
guard. Failures leave the binding/VM result untouched.

The receipt reports grouped check/case/native-comparison/VM-call counts for the
current inputs. The runner records all input hashes and the
generated native-body hash and rejects changes during execution. Unused native
parameters/stack pads and a native Bool switch warning are suppressed only around
the extracted include; runner exclusions for Dolphin array prototypes and missing
field initializers are recorded separately. The production backend remains
compiled with `-Wall -Wextra -Werror`.

Class/extent checks do not prove arbitrary pointer allocation bounds or detect
arbitrary dangling joints. There is no DAT import, real stage geometry, rendering
or gameplay claim. Prepared joint-world-position and secondary resource selectors
are outside this fixture.

Run `run.py --compiler <MSVC-target clang.exe> --output <fresh A: directory>`.

The fixture also checks normal-producing point sweeps and grounded checks against
extracted `it_8026E9A4` / `it_8026DA70` bodies with complete Item comparisons,
three writable normal components (including signed-zero/NaN bits), VM reads and
writes, and retained-output rejection after source mutations. Stage geometry
remains a controlled leaf; these cases do not establish native stage geometry or
whole Thunder Jolt gameplay acceptance.
