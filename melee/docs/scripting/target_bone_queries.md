`targets.read_bone_position(handle, raw_part, x, y, z)` returns Bool and writes
three distinct Float destinations only on success. Fighter and Article callers
must have a live selected source program. Handles and raw parts are integral;
wide indices are checked before native-width conversion. Missing targets,
missing joints and out-of-range parts return false without changing outputs.

The raw part indexes the actual target's imported part table, including omissions;
it does not pass through semantic-part remapping. Foreign stock Fighters and
Fighters belonging to a different selected source package are allowed. A faulted
target's still-live owned storage is readable. A captured target's physical or
logical retirement, substituted userdata/schema, or rig changes withhold all
outputs and fault the caller operation.

The native sampler calls `lb_8000B1CC(joint, NULL, position)` exactly once. A
parentless joint contributes local translation; a parented joint contributes
matrix translation after native setup. Cached matrices need no new evaluation.
No finite-value normalization changes native Float bits.

Evaluated dependencies retain the actual native rig. Standard HSD matrix math,
IK, JObj constraints, limits and native animation anchors are allowed. Active
native expression callbacks must be the actual HSD NULL-function dummy;
bytecode must match the exact trusted imported descriptor. Custom matrix methods
and unproven expression origins reject before sampling. Active expressions aimed
at PATH4, UFC/effect/sound channels20..42, or vector channels50..53 reject before
sampling: scalar expression results cannot prove those additional native contracts.
Native expression inputs
must fit the engine's default 100-argument buffer.

Publication checks do not make arbitrary graph-destroying native callbacks safe
inside HSD traversal. JObj/RObj have no per-address generation token; arbitrary
dangling pointers or same-address graph resurrection are not detected. Existing
HSD lazy scale-vector/expression allocations remain native behavior. The backend
adds no tick heap allocation. This API does not activate companion roles.
