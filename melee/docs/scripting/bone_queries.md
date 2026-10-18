# Fighter bone queries

## Bone IDs

Each bone has a numeric ID belonging to its fighter. These IDs let scripts keep
referring to the same bones when the rig's hierarchy changes. A removed bone's
ID stays unused. `bones.index(LeftThumb)` returns the ID for the left thumb role.
Cloning a stock fighter starts with its original bone numbers.

`bones.count` reports how many joints the fighter has. Bone IDs can have gaps.
Use an ID obtained from the fighter whose bones you are querying.

## World position

`bones.world_position(bone, x, y, z)` writes the bone's world coordinates into
three different Float variables. You can also supply an offset in the bone's
local coordinates: `bones.world_position_offset(bone, x, y, z, offset_x,
offset_y, offset_z)`.

```text
let float x = 0
let float y = 0
let float z = 0
bones.world_position(bones.index(LeftThumb), x, y, z)
```

Querying a missing bone produces a script error. The output variables keep their
previous values if the query fails.

## Rendered root scale

`bones.root.scale_x`, `scale_y` and `scale_z` read the three Float components
of the Fighter GObj's actual rendered root. They are separate from gameplay
instance scale, the common damage-scale multiplier and a semantic TopN bone.
The root does not have to occupy raw rig slot zero.

`bones.root.set_scale(x, y, z)` sets those components together through the native
JObj scale operation, including its usual matrix-dirty policy. All three
expressions evaluate once in order. Negative and zero scale are permitted;
native Float bits are preserved. It does not rewrite gameplay scale, hitbox
fields, root translation or the costume's saved rest pose. Later native bone
sampling can observe the transformed root, just as it does for other joint
transforms.

```text
state float saved_scale_x
state float saved_scale_y
state float saved_scale_z

func save_root_scale():
    state.saved_scale_x = bones.root.scale_x
    state.saved_scale_y = bones.root.scale_y
    state.saved_scale_z = bones.root.scale_z

func restore_root_scale():
    bones.root.set_scale(state.saved_scale_x, state.saved_scale_y, state.saved_scale_z)
```

These operations require the direct live Fighter's admitted root JObj class,
extent and owned rig. Missing roots are errors rather than fabricated unit
scale. Native class initialization can run nested work; the retained identity
is checked afterward, before reading or changing the root. Retirement or root
replacement leaves outputs unpublished. Save scale in ordinary fighter state
when a move needs to restore it after a temporary deformation.

World sampling completes into a local Vec3. XYZ publication occurs together only after the retained source, storage and rig remain current. Class initialization is followed by a guard before sampling. A lifetime or graph change during sampling withholds all output destinations and the return value. This protects script publication; it does not make arbitrary native callbacks that destroy the graph while an HSD kernel is traversing it safe.

`tests/native/script_bone_queries` exercises real compiler, Prepare, Fighter actors, VM, fixed physical pools, property/catalog routing and unchanged native matrix kernels. Source rig metadata and graph construction are controlled; IK/RObj policy, retail asset import/rendering and complete Link gameplay are not claimed. Controlled class/RObj leaves inject retirement, schema and physical reuse hazards. The older `script_presentation` fixture retains its stock sound/rotation/matrix comparisons using a real physical MatchPool payload; an unprepared source package now rejects honestly.
