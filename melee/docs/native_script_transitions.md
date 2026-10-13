# Direct interpreted move transitions

These fighter-only operations require independently owned behavior and validate
motion targets against the actual installed common/extended tables before changing
state. Integral target values retain their original width until bounds checking;
flags marshal to U32 and animation values to binary32.

| Operation | Contract |
| --- | --- |
| `state.enter(motion)` | Change motion with flags0, frame1, rate1. |
| `state.enter(motion, frame)` | Use the supplied frame and rate1. |
| `state.enter(motion, frame, rate)` | Use supplied frame/rate. |
| `state.begin(motion)` | Change at frame0/rate1, then run the shared startup animation sequence. |
| `state.enter_flags(motion, flags)` | Change at frame0/rate1 with U32 flags. |
| `state.enter_at(motion, flags, frame)` | Change with supplied flags/frame and rate1. |
| `state.switch_to_air(motion, flags)` | Run general ground-to-air preparation, then change motion preserving the current animation frame. |
| `state.switch_to_ground(motion, flags)` | Run general air-to-ground preparation, then change motion preserving the current animation frame. |

Within a validated named move, `state.to_air()` and `state.to_ground()` lower
once at load into the corresponding explicit target/flags calls. Helpers outside
that context must use explicit switches; absent/conflicting ownership fails
compilation without replacing the accepted program. Named special entry
`state.enter(NeutralSpecial)` (and equivalent side/up/down/air names) uses begin
semantics, preserving the existing language's startup rule. Numeric forms retain
frame1 behavior. This is typed lowering, not a runtime source-name search or
stock-special delegation.

The shared `ftAnim_8006EBA4` body now checks a tracked fighter's creation identity
between its four reentrant startup stages. Untracked demo presentation retains
its existing sequence. Parent retirement/replacement stops before a later stage,
result/output publication or subsequent authored write.

Fighter-specific entry initialization belongs in source. Existing templates must
still migrate any hidden setup formerly emitted by the C generator (including
command/throw-cue resets) and remaining callbacks/assets before production
source-only activation. These operations do not establish full Mario/roster or
Workshop/package parity.

Evidence: `tests/native/script_transition/README.md` and
`A:/FreakMelee-test-archives/script-transition-final-oct07/receipt.json`:
451 assertions /26 real Execution runs from a complete compiled bank. Actual
plane-transition and startup bodies execute; motion installation, geometry and
related engine leaves are controlled. Targets, unsigned flags, frame/rate,
constant defaults, contextual ownership, accepted-bank preservation and source
retirement are checked through state/output observations. The client/tool build
and27 focused CTests pass; no UI redesign was made.
