# Direct source jump bindings

The zero-argument Void `aerial_jump` hook is prepared into the actual Fighter
program and dispatched directly at the basic and multi-jump entry sites.
A present source hook is terminal, including error/retirement; it cannot fall
through into a native special handler. An absent hook keeps the general engine
basic/multi-jump policy. Attached source actors do not select the native
Ness/Yoshi/Peach/Mewtwo entries or a Definition callback.

| Source API | Contract |
| --- | --- |
| `jump.begin(Int motion, Float x, Float y, Float z, Bool reset_timer, Bool animation_driven, Bool worn_sound)` | Enter an owned/common motion at frame zero, set jump flag/velocity, optionally reset the input timer, increment the native used-jump counter and perform the common jump/worn-item sound sequence. |
| `jumps.used` | Int read/write of the actual UInt8 engine counter; writes must be 0–255. |
| `jumps.max` | Read-only Int from the actual common fighter attributes. |
| `bones.root.add_rotation_y(Float radians)` | Add to the selected owned Euler root joint. |
| `controls.aerial_jump_actions()` | Void full shared jump IASA, preserving action order and command-0 gates. |
| `collision.basic_landing_velocity_threshold` | Read-only Float from actual common scale/landing calculations. |
| `collision.try_wall_jump()` | Bool from the actual general walljump engine operation. |

`controls.try_aerial_jump()` remains the separate Bool input check for another
jump. It does not stand in for the full input phase.

The `animation_driven == true` jump mode is explicitly unsupported in this
slice. The native engine mode installs a per-instance physics callback; a source
animation policy needs its own code phases before that variant can be enabled
without superseding authored physics. Rejection occurs before any motion,
counter or velocity mutation. Rook's ordinary first jump uses false.

Jump entry and lifecycle-capable control operations require the live selected
source package, bank, imported motion table and actor storage view. The shared
jump begin/input bodies now guard physical generation and source logical
status between continuation stages. The actual walljump routine guards after
its motion-entry operation before subsequent timer/counter reads or writes.
Normal state changes keep the source live; retirement/fault stops continuation.

No new operation performs allocation or source parsing during ticks. No new
native authored callback or compatibility registry selects jump behavior.
The shared begin kernel's UInt8 increment semantics are retained, including
wrap when explicitly invoked at counter 255; input eligibility remains the
separate general control check or an authored decision.

`tests/native/script_jump` supplies source/VM/native-body contract evidence.
Real Rook asset, animation, collision and gameplay acceptance is separate.
