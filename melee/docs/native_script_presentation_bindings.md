# Direct interpreter sound and fighter joint transforms

The presentation family uses the single typed native binding catalog and
`native_script_binding_presentation.c`. It is fighter-only. Source generation,
argument types and output permissions are checked before native execution, and
publication requires that the same source remains alive. Scripts receive
numbers and scalar values, never native fighter/joint pointers.

## Semantic parts and joint indices

Semantic fighter parts describe a role in the native part schema. They are not
indices into the current model. These source-backed bare names retain their
existing meanings:

| Name | Native semantic part |
| --- | --- |
| `Top`, `TransN`, `TransN2`, `XRotN`, `YRotN`, `Hip` | TopN, TransN, TransN2, XRotN, YRotN, HipN |
| `LeftHand`, `RightHand` | L1stNb, R1stNb |
| `LeftThumb`, `RightThumb` | LThumbNb, RThumbNb |
| `LeftFoot`, `RightFoot` | LFootJA, RFootJA |
| `LeftShoulder`, `RightShoulder` | LShoulderN, RShoulderN |

`bones.index(part: Int) -> Int` resolves a semantic part through the current
fighter's native `part_to_joint` table. It validates the fixed 54-part schema,
mapping and instance joint before returning an index. It is a pure observation.
`bones.count -> Int` returns the validated native joint count. A custom model
can map `LeftThumb` to any valid owned joint; the part's enum number is never
silently used as that joint index.

Operations taking an explicit joint use a model index, usually obtained from
`bones.index`. Both selector families validate the installed table, instance
storage, maximum native joint count and any nonzero Definition joint-count
declaration. Missing mappings/joints and inconsistent counts fail explicitly.

## Rotations and scale

| Call/property | Selector and type |
| --- | --- |
| `bones.root.rotate_x/y/z(angle: Float)` | Native joint 0. |
| `bones.root.rotate_x/y/z(joint: Int, angle: Float)` | Explicit native joint. |
| `bones.root.reset_z()` | Sets native joint 0's Z rotation to zero. |
| `bones.root.rotation_x -> Float` | Reads native joint 0's X rotation. |
| `bones.rotate_x/y/z(part: Int, angle: Float)` | Resolves a semantic part, then rotates the mapped joint. |
| `bones.set_scale(part: Int, x: Float, y: Float, z: Float)` | Resolves a semantic part, then sets joint scale. |
| `bones.set_direct_animation_copy(joint: Int, enabled: Bool)` | Sets the instance joint's animation-copy flag. |

Rotation/scale arguments receive the native binary32 conversion exactly once,
including Double literal/intermediate results. Rotations call the actual
`ftPartSetRotX/Y/Z` primitives and retain native matrix-dirty propagation.
Quaternion-backed fighter joints use their native Euler copy. A missing copy
or a copy still marked quaternion fails before entering an asserting primitive.

The original `ftPartGetRotZ` implementation returns **Y** in both branches.
It is now named `ftPartGetRotY`, with all callers updated directly. The generic
`bones.root.rotation_y` property reads that Euler angle, including the copy
joint when the rendered joint uses a quaternion. Fox and Ness scripts use the
correct Y name; the getter and turn calculations retain their original behavior.
`bones.root.rotation_z` is not a misleading alias for this operation.

General transforms allow damage cleanup to be authored as an ordinary helper,
then bound using the owned callback API:

```text
func ResetRoot():
    bones.root.rotate_x(0)

func InstallCleanup():
    callbacks.damage(ResetRoot)
    callbacks.death2(ResetRoot)
```

`callbacks.reset_root_x_on_damage()` remains unsupported; no native authored
surrogate is added. See `native_script_setup_bindings.md` for callback ownership
and zero-argument Void signature rules.

## World position and output arguments

Both calls return Void:

```text
bones.world_position(joint: Int, out_x: Float, out_y: Float, out_z: Float)
bones.world_position(joint: Int, out_x: Float, out_y: Float, out_z: Float,
                     offset_x: Float, offset_y: Float, offset_z: Float)
```

The output arguments must be distinct writable Float lvalues, such as Float
locals or declared state fields. Structurally identical output targets are
rejected. Use `bones.index(LeftThumb)` when selecting a semantic
part. The operation calls the actual `lb_8000B1CC`: root translation, parent
matrix and offset transform branches retain the native computation order.

Outputs are computed into local storage and staged only after the matrix
operation's source-lifetime check. The catalog and VM publish to authored
lvalues after their own guard. Retirement or generation replacement prevents
output publication and later authored statements. World position has output
effects and is not allowed in a read-only callback.

## Sound

These calls return Void and invoke existing generic engine sound primitives:

```text
sound.play(sound_id: Int, volume: Int, pan: Int)
sound.loop(sound_id: Int, volume: Int, pan: Int)
sound.channel(channel: Int, sound_id: Int, volume: Int, pan: Int)
```

Channels 0..6 map to the existing play, alternate, voice, loop and remaining
native channels. `play` selects channel 0 and `loop` selects channel 3. Invalid
channels fail. Volume and pan retain the existing Int-to-unsigned-byte modulo
conversion before native audio scaling; this is not a new clamp policy.

Sound ID resolution, costume/size modifiers, Definition sound rules, muted
channel behavior, stop sentinels and playback-handle fields remain in the
actual engine primitives. Muting channel 0 still preserves its existing pitch
RNG consumption. These calls can consume the engine seed and mutate sound
handles, so they are not pure calls for read-only events. No fighter-specific
sound body or legacy generated-module host table is invoked.

## Focused evidence and remaining limits

`tests/native/script_presentation/run.py` builds the production direct source
frontend, typed catalog, bytecode and VM. The fixture compiles actual part
mapping, rotation, world-position, sound-policy and RNG units, and extracts
actual sound resolver/channel and JObj-dirty bodies. Audio device functions
and matrix construction are controlled; unrelated engine families are
explicitly unavailable in this isolated executable.

The focused fixture passes 6,621 checks, including 672 sound/native-seed pairs
and 800 transform comparisons. Comparisons cover sound handles, channel stage sequences, seed advancement,
mute/size policies, semantic mappings deliberately different from joint
numbers, quaternion copies, Float rounding, matrix offsets and source-to-VM
output publication. These checks do not establish live audio/render parity,
article/model-effect joint receivers or effect spawning.
