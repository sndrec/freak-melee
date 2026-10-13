# Item surface and animation observations

These operations require the current live source-owned Item and its exact
prepared program/storage. They use no stock Definition or authored native callback.

| Authored API | Result | Native behavior |
| --- | --- | --- |
| `item.disable_surface_response()` | Void | Sets `Item.xD5C` to zero. |
| `item.cancel_hitlag_request()` | Void | Clears the pending Item hitlag request (`xCA8`); does not end active hitlag. |
| `item.sweep_stage(previous_x, previous_y, previous_z)` | Bool | Sweeps from the supplied previous position to the Item's current position through `it_8026E9A4`. Resolves Item position on contact. |
| `item.sweep_stage_normal(previous_x, previous_y, previous_z)` | Bool | Same native sweep as `sweep_stage`; also preserves the engine's returned normal in `item.surface_normal`. |
| `item.surface_normal.x`, `.y`, `.z` | Float, read/write | Last stored point-sweep normal, distinct from the collision box's floor normal. Requires ordinary source Article storage, not a native holdable recipe. |
| `item.check_ground()` | Bool | Runs the original `it_8026DA70` grounded collision check, including collision setup and its native side effects. |
| `item.launch_angle` | Float, read only | Reads the initial projectile launch angle in radians. |
| `item.launch_speed` | Float, read only | Reads the initial projectile speed. |
| `item.launch_origin.x`, `.y`, `.z` | Float, read only | Reads the initial projectile origin. |
| `item.reflect_facing` | Float, read only | Reads the native reflection-facing value (`xC68`). |
| `item.spawn_with_scalar_at_position(effect, x, y, z, scalar)` | Void | Spawns an audited world-position/one-scalar effect from the owned Item. |
| `item.sweep_stage_flags()` | Int | Runs `it_8026DAA8`, preserving its collision/position updates and returned mask. |
| `item.animation_finished` | Bool, read only | Inverts `it_80272C6C` / `lb_8000B09C` over the primary graph. |
| `floor.normal.x`, `.y`, `.z` | Float, read only | Reads the Item's stored floor collision normal without changing its native bits. |
| `wall.left.normal.x`, `.y` | Float, read only | Reads the stored normal of the Item's left-facing wall. |
| `wall.right.normal.x`, `.y` | Float, read only | Reads the stored normal of the Item's right-facing wall. |
| `ceiling.normal.x`, `.y` | Float, read only | Reads the Item's stored ceiling collision normal. |
| `item.sweep_floor()` | Void | Runs `it_8026E0F4`: sweeps the floor, copies the resolved position and selects the contacted floor. |
| `item.wall_contact_flags()` | Int | Selects the contacted wall and returns 8 for the left-facing wall or 4 for the right-facing wall; the latter wins if both are present. |
| `item.ceiling_contact_flags()` | Int | Selects the contacted ceiling and returns 2, or returns zero without a ceiling. |
| `item.reflect_from_wall()` | Bool | Reflects inward velocity about the selected wall normal, applies native attribute damping and reports whether a wall was present. |
| `item.align_to_floor()` | Void | Rotates the native Item joint onto its contacted floor using the Item's native rotation axis. Does nothing without floor contact. |
| `item.align_facing_to_velocity()` | Void | Updates facing from horizontal velocity using Melee's threshold, then updates collision facing. |
| `item.contact_fighter` | Int, read only | Returns the physical Fighter creation handle at the Item's current contact, or zero when absent. This is not the attachment owner. |
| `item.reflector` | U32, read only | Returns the current live Fighter creation handle recorded by reflection, or zero for null, non-Fighter or physically retired objects. |
| `item.reflection_owner_transfer_disabled` | Bool, read only | Reports the Item's native policy that prevents reflection from transferring its damage owner. |
| `attachment_owner.fighter.action_detaches_items` | Bool, read only | Reports whether the attachment owner's current action requests attached-item detachment. Guard access with `attachment_owner.exists`. |

The sweep updates last/current collision position, performs the actual stage
sweep, copies collision position into Item position, and updates the selected
surface in native floor, wall, ceiling order. Its result is the native combined
contact mask. This call changes Item collision data; it is not a pure query.

`item.sweep_stage` is a separate segment query. It returns whether the previous
to current XY segment intersects the stage. On contact, Melee replaces all three
position components with the resolved point; on a miss, position is unchanged.
This operation does not publish an ECB contact mask or update the cached
`collision.position`. Coordinates must be finite. The native query ignores the
previous Z component, although the authored call accepts a complete position.
It needs no model graph. A source retired during the native query cannot publish
a result; native mutations already performed are not undone.

Launch observations preserve stored Float bits, including signed zero and
nonfinite values. They describe the factory's initial launch, not current
velocity after reflection or other movement. The reflection-facing observation
is independent of the creator and current damage owner.

Floor normals read the current Item's `x378_itemColl.floor.normal`. They do not
require current floor contact, a model graph or a new stage query. Each component
preserves its stored Float representation, including signed zero and nonfinite
values. `item.resource(node).collision.floor_normal` observes that resource's
separate collision data. Reading the Item's floor normal does not select a
collection resource.

The wall and ceiling normal properties have the same cached-read contract.
They require no current contact, model or additional collision sweep. Left and
right refer to Melee's left-facing and right-facing surface records. These
properties also remain available in Fighter scripts, reading that Fighter's
own collision data.

The wall and ceiling operations also change the selected surface; they read
existing collision flags rather than performing another sweep. Wall reflection
applies native damping even when the Item is already moving away from the wall.
Floor alignment uses the native dynamic-bone entry when present, otherwise the
primary model joint. An invalid graph produces an error instead of selecting a
different model. Facing alignment leaves tiny horizontal velocities alone when
the Item already has a nonzero facing direction.

Animation completion preserves native AObj active flags and JObj instance
traversal. A missing primary graph is finished. Existing JObj subclasses are
accepted with their native class ancestry and object extent; a wrong live graph
class is an error. This property does not advance animation or DATA commands.

Logical/physical retirement or a changed source program/storage stops output
publication. Native mutations already performed by the sweep remain native
mutations. No secondary model index is redirected to the primary graph.

The native reflection pipeline records the reflecting object, transfers damage
ownership when its policy permits, then runs `on reflected`. The callback can
read `item.current_owner` and `item.reflector` independently. When
`item.reflection_owner_transfer_disabled` is true, the damage owner stays
unchanged and `item.reflector` still identifies the live reflecting Fighter.
Neither property substitutes the original creator or attachment owner.
The reflector field records a raw native relation rather than a historical
contact generation: this property reports its current live identity at the time
of the read. Handles preserve all 32 bits and use zero for a missing Fighter.
The read retains the exact owned Item, program/storage and captured physical
relation through publication; it performs no allocation or callback.
Native reflection supplies a registered Fighter contact. The host target getter
searches the Fighter registry, while this read admits the recorded physical
pool/class/generation relation. A deliberately unlinked but physically live
Fighter is outside the native-getter comparison in the focused fixture.

[`item.joint_world_position(model, path, x, y, z)`](article_models.md#itemjoint_world_positionmodel-path-x-y-z)
samples an admitted model joint through its prepared literal path and three typed
output destinations. The focused console fixture documents
native comparisons and controlled stage/actor-publication boundaries in
`tests/native/script_item_observations/README.md`.

## Cached collision position

`collision.position.x`, `.y` and `.z` read the root Item's stored native
`x378_itemColl.cur_pos` as Float values. They are read-only and preserve raw
Float bits. The values are separate from `position` and from each
`item.resource(i).collision.position` record. Floor contact and a model graph
are unnecessary. The interpreter retains the current owned Item's physical
identity, scene, generation and source storage through publication, with no
allocation or callback.

## Attachment matrix rotation

[`item.read_attachment_matrix_rotation(x, y, z)`](article_models.md#matrix-rotation-reads)
writes Euler XYZ from the original creator's recorded attachment joint into
three distinct Float destinations. It samples the existing matrix without
updating it. Detached/reassigned or healthy missing relations write zero;
retirement or changed physical rig/storage produces a diagnostic before output
publication. Stock and independently authored creator rigs are both supported.

## `floor.friction`

Read the active stage material's friction multiplier from the current source's
cached floor contact. The low byte of the floor flags selects the material;
other flag bits do not affect friction. Use this with authored ground movement
after the frame's collision state has been established. This property is
read-only and available to Fighters and owned Articles. Invalid material indices
are rejected; it does not perform a collision query or use the fighter's base
friction attribute. See [collision observations](combat_visuals.md) and
[stage queries](stage_queries.md).

The normal sweep passes an initialized local copy to the native routine and
publishes it after source identity/storage checks. A miss does not force a zero
normal; any native normal side effects are retained. Explicit Float normal
assignments preserve their bits without normalization or a new finite filter.
Previous/current sweep positions must remain finite. The normal data belongs to
ordinary source Article storage; holdable recipes use a different native schema.
The operation allocates nothing and does not dispatch a stock item decision.
