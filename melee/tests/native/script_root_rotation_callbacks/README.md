This fixture compiles owned zeroarg Void helpers, registers damage/death2 through
the real Setup/actor/VM and invokes actual Presentation + ftPartSetRotX. Its
native comparison uses the exact old reset operation at raw index0. The semantic
part mapping deliberately differs from0. Euler and quaternion-copy paths preserve
Y/Z and unrelated joints; callback clearing, rebinding, native takeover and actor
snapshot restore are checked. Missing root rejects without a guessed substitute.

The root-Y cohort also compiles read/turn helpers and compares their reads to
the original getter body pinned at `1b8671834`, with only its function name
changed. Both Euler and quaternion-copy branches use a raw root index different
from the semantic role mapping. Signed zero, infinities and NaN bits are retained;
wrong source identity, missing joint and quaternion-only copy reject without
publishing a result. The test's top-level property routing is controlled and
enters the production presentation reader; production catalog routing is checked
by client compilation and owned-source audits separately.
An unrelated rig table with identical contents also rejects before mutation and
leaves the native call output untouched. No presentation runtime change was
needed; this suite exercises its existing owned-rig guard.

The owned rig/program metadata is controlled. Actual Fighter/GObj pools, context,
actor, callback dispatch, source compiler and setters execute. Asset/math/audio
leaves remain controlled; no full DrMario asset or gameplay acceptance claim.
The shared presentation-family fixture boundary is reused read-only, with only
its controlled package getter replaced by this exact owned-rig fixture.

DrMario's two original reset convenience calls are replaced by the helper plus
explicit damage/death2 bindings in the same order. It resets only X; declared
damage/death2 cleanup events are not implicitly called by that helper. Existing
clear_damage_death behavior stays authored. All seven owned Vitamin signal calls
use the new explicit four-argument declaration contract.

The test-only reference exporter accepts a focused helper/registration source
and its emitted C passes Clang syntax. The complete DrMario reference currently
stops at Article state metadata (line498), a separate oracle boundary; no whole
oracle parity claim is made. Direct source-check advanced past the reset helper
and currently stops at line333, `items.clear_held()`. That first-error result
does not exercise the owned signal calls beginning at335. No unowned production
oracle/parser files were edited.

Final focused receipt:
`A:/FreakMelee-test-archives/script-root-rotation-frozen/receipt.json`.
It records72 checks and four native comparisons across both callback slots and
both Euler/quaternion-copy cases. No presentation or shared catalogue production
change is required for the reset helper.
