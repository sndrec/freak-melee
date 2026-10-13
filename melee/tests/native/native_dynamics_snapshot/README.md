# Actual secondary dynamics and wind snapshot fixture

Runs unchanged selected production `lb_00F9.c` functions against the real new
storage and snapshot owner. This extraction omits unrelated render/physics
methods; it is not a replacement production implementation. Joint matrix
setup is controlled and unused normalization aborts. No model graph or complete
match restore is claimed. Live old/current rendered comparisons complement it.

Covers the original 320/8 ceilings, constructor/reset, dynamics allocation and
release, retained free-node payload, wind exhaustion/replacement/decay/expiry,
exact next addresses, candidate topology and buffer refusal, scene retirement,
and repeated real wind ticks with identical full component payloads. External
DynamicsDesc records are restored explicitly by the fixture as companions;
production aggregate graph/caller agreement remains open.
