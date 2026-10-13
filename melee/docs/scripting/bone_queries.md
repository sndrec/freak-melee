# Fighter bone queries

`bones.count` is the current rig slot count. `bones.index(semantic_part)` returns the raw joint slot from the admitted Fighter part map; for example, `bones.index(LeftThumb)`. Semantic parts and raw joint slots are different selectors.

`bones.world_position(raw_joint, x, y, z)` writes three distinct mutable Float destinations. An optional XYZ offset is accepted as the final three arguments, or by the explicit `bones.world_position_offset(raw_joint, x, y, z, offset_x, offset_y, offset_z)` spelling. The selector must be integral. Positions and offsets are F32. These methods use the actual native `lb_8000B1CC` sampling and HSD matrix construction, including native root-joint and parent-matrix behavior.

```text
let float x = 0
let float y = 0
let float z = 0
bones.world_position(bones.index(LeftThumb), x, y, z)
```

The queries retain physical Fighter generation, scene lifetime, current interpreted program/storage when attached, selected source rig tables, and selected joint class/extent/identity. Invalid or unmapped joints reject; no zero position is fabricated. A selected source package requires its real attached program and matching owned rig. Existing retained Fighter receiver projections also admit physical stock recipients under their native lifetime contract; no caller/recipient program equality is introduced.

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
