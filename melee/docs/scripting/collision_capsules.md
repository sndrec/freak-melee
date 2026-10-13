# Fighter collision capsule coordinates

Fighter callbacks and their helpers can read and write the four fighter hitbox capsule endpoints:

```text
let int index = 2
hitbox.current(index).x = position.x
hitbox.current(index).y = position.y + 4
hitbox.current(index).z = position.z
hitbox.previous(index).x = hitbox.current(index).x
hitbox.previous(index).y = hitbox.current(index).y
hitbox.previous(index).z = hitbox.current(index).z
```

| Expression | Type | Engine field |
| --- | --- | --- |
| `hitbox.current(index).x/y/z` | Float, read/write | `fighter->x914[index].x4C` |
| `hitbox.previous(index).x/y/z` | Float, read/write | `fighter->x914[index].x58` |

`index` must be an Int expression. It is evaluated exactly once. Indices outside 0..3 read as zero and writes do nothing. A write evaluates its right-hand expression once, after checking the index; an invalid index skips that expression. Values must have Float or Int type. After conversion to Float, NaN and infinity leave the coordinate unchanged. Bool values require an explicit numeric conversion. Only the literal axes `x`, `y`, and `z` are accepted. These scalar accessors are not callable and are unavailable in article and model-effect contexts.

These are raw collision-space coordinates. A write changes only the selected coordinate: it does not enable the capsule, change its attached joint or bone offset, clear victims, or initialize the other endpoint. Scripts make those decisions explicitly. The common collision update `ftColl_8007AD18` can subsequently copy the current endpoint to the previous endpoint and recompute current from the joint and offset. Callback placement therefore determines when the authored positions are used.

The existing `hitbox.set_world_position(index, x, y, z)` has a different engine contract: `ftColl_8007B8A8` clears the joint and stores the supplied vector in `b_offset`. It does not immediately set either endpoint. Use explicit current/previous writes when controlling the collision sweep itself.

Console acceptance in `tests/native/fighter_capsule_positions/` runs actual generated callbacks through both action and owned-motion generation paths. It covers all axes, invalid/full-width indices, evaluation order/count, finite filtering, and preservation of other capsule fields. Complete engine update/offset bodies also execute with joint-to-world conversion controlled. This establishes the coordinate API, not Sheik Chain policy or full collision/gameplay parity.
