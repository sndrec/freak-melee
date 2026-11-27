# Fighter observations and action statistics

## `damage.quiet_ko()`

Take a stock from the fighter running this script. The fighter disappears in
place and respawns after the usual KO delay if it has stocks remaining. The
match records the KO and updates the score as usual. No blast effect, camera
quake, camera chase, KO sound or star-KO animation plays.

The engine applies the request during the fighter's next KO check. Repeated
calls before that check take one stock. This command applies to active fighters.

## `damage.interrupt_cleanup()`

Run shared interruption cleanup before forcing a new action. This stops the
fighter's two movement/attack voice channels, then invokes the registered damage
and death cleanup hooks in Melee's normal order. It does not deal damage, select
a damage action, or clear those callback registrations. Scripts retain their
own decisions and cleanup implementations; no stock special handler is called.

```text
damage.interrupt_cleanup()
state.enter_at_speed(MyMove, 0, 0, 1)
```

The operation takes no arguments, returns no value and requires the current
live owned Fighter. If a hook faults or retires the actor/program, execution
fails before publishing success. Sound and callback side effects already
performed by the native engine are retained. The operation allocates no
script-side storage. Physical actor retirement and script detachment differ:
the native routine's existing physical-generation guards remain in effect,
and the binding additionally checks the captured program/storage afterward.

## `action.current_metadata`

Read-only Int containing the active action's canonical 32-bit metadata,
reconstructed from live fields, including counters changed since motion entry.
It never copies the host compiler's packed union layout. Use
`action.move_id(action.current_metadata)` to read its low-byte move ID.

Bits 20–23 contain `action.category`; bits 19–16 contain the four original
motion flags in console order. The original `x2071_b5` flag uses mask `262144`
(bit 18). The high byte and second-lowest byte retain their native metadata
and statistics fields. High-bit words use a signed Int with unchanged bits.
Retained stock Fighters support this physical observation too; it cannot be
assigned. The read invokes no callback and allocates nothing.

These operations use a live Fighter context. They read native state directly;
they do not call a stock fighter's special-move handler or allocate script state.
See [movement](movement.md), [ground controls](ground_controls.md) and
[held articles](held_articles.md) for the related transitions and callbacks.

## Properties

| Property | Type | Meaning |
| --- | --- | --- |
| `movement.special_landing_lag` | Float, read-only | The fighter's current special-landing lag value. |
| `engine.debug_level` | Int, read-only | The native engine's current debug level. |
| `fighter.player_slot` | Int, read-only | The source Fighter's logical player slot, distinct from its controller port or actor handle. |
| `input.down_press_age` | Int, read-only | Frames since the latest down-D-pad press, saturated at 255. It does not measure time since release. |
| `shield.pushback_damage_scale` | Float, read-only | Common shield pushback damage coefficient. |
| `shield.pushback_base` | Float, read-only | Common shield pushback base amount. |
| `shield.pushback_velocity_scale` | Float, read-only | Common shield pushback velocity multiplier. |
| `shield.pushback_light_min` | Float, read-only | Common minimum light-shield pushback coefficient. |
| `shield.pushback_light_max` | Float, read-only | Common maximum light-shield pushback coefficient. |

The five `shield.pushback_*` properties read the actual common-data fields
`x28C`, `x290`, `x294`, `x2E4` and `x2E8`, respectively. They are available to
the calling owned Fighter and preserve native Float values without clamping.
Missing common data diagnoses without publishing a value. The reads retain the
source actor/program/storage and common-data pointer, allocate nothing and
invoke no callback. The generic values let authored policy reproduce the
native `ftCo_80092ED8` formula; the operation does not apply recoil itself.

### movement.special_landing_lag

This is the native `Fighter.x2EC` value used by the common special-fall and
landing routines. It is distinct from a normal aerial attack's landing lag.
The interpreter preserves its Float bits without clamping or replacing it with
a donor fighter's attributes.

```text
state.freefall(1, 0, true, 1, movement.special_landing_lag)
```

### engine.debug_level

This reports the current native `DbLevel` as an Int. Use the `DebugLevel`
constants when authoring debug-only behavior. It is an observation, not a
setting that a fighter script can change.

```text
if engine.debug_level >= DebugLevel.debug_rom:
    pass
```

## Methods

### stats.action_count(slot, action_id) -> UInt32

Reads the native action-statistics counter for a logical player slot. Both
selectors must be integral; slots are 0 through 5. The native counter IDs extend
beyond the partially named attack array: conversation action `0x72`, for
example, is valid. IDs outside the containing statistics object return zero,
as in the native bounds-checked reader. Negative or wider-than-UInt32 selectors
are rejected before accessing player storage. This method does not modify counts.

```text
if stats.action_count(fighter.player_slot, 0x72) != 0:
    pass
```

### stats.notify_taunt() -> Void

Runs the common native taunt-statistics notification using the source Fighter's
logical player slot and secondary-member flag. It does not enter a taunt action
or choose a fighter-specific animation. Invalid slots are rejected. Publication
is guarded against source retirement during the operation.

Together with [stage events](stage_queries.md), these operations let a script
own conversation decisions and install its own [callback](callback_slots.md),
without delegating to a stock fighter's conversation handler.

### stats.count_as_thrown_item() -> Void

Marks the current action for thrown-item counting by setting the native
`count_thrown_items` flag. It does **not** immediately increment a statistic,
spawn or throw an item, or clear the flag. Repeated calls leave the flag set;
the existing common action lifecycle owns its reset and subsequent counting.
This method operates on the calling fighter.

```text
stats.count_as_thrown_item()
```

## Landing callback input

Inside `on landing_action:`, use [`event.interrupt_allowed`](held_articles.md).
The common landing entry captures this Bool before invoking the script. It
remains the same if the callback changes motion and the native motion-variable
union is reused. Pass it explicitly to helpers; it is unavailable outside that
callback. It is distinct from `controls.interrupt_allowed`, the live IASA flag.

## Action category and incapacitated flag

These read-only properties observe the current physical fighter without
invoking a handler, changing a motion or allocating memory.

| Property | Type and native meaning |
| --- | --- |
| `action.category` | Int `0..15`: the unsigned four-bit `x2070.x2071_b0_3` action category. |
| `fighter.incapacitated` | Bool: the native `x221F_b3` flag is set. |

The category is legacy action metadata, separate from `motion.id` and the
eight-bit action move ID. It reads the decoded native field rather than
interpreting the host union's byte layout. `fighter.incapacitated` reads its
native flag directly; it does not infer a value from damage, animation or
motion ranges.

Both properties are available in Fighter callbacks and helpers. Article
callbacks can observe a retained Fighter receiver, including a stock fighter
or one running another source package:

```text
if attachment_owner.exists:
    let int category = attachment_owner.action.category
    let bool inactive = attachment_owner.fighter.incapacitated
```

The receiver keeps the physical target captured for that observation. It does
not need the caller's program or state layout. An attached target retains its
own selected program and storage; a stock target retains its native identity.
Generation, class, kind, userdata, scene or physical pool-slot changes reject
the read before publication. Changes to an attached target's program, storage
or schema also reject. The caller must survive publication. A missing receiver
is not assigned an invented category or flag; test `exists` before reading it.

Neither property can be written. Retain custom gameplay decisions in authored
state rather than modifying these engine observations.

## Controller synchronization and spawn offset

| Property | Type | Native meaning |
| --- | --- | --- |
| `motion.allows_controller_sync` | Bool, read-only | The current `Fighter.x2225_b3` motion flag. |
| `spawn.horizontal_offset` | Float, read/write | The unscaled `Fighter.x40` spawn offset. |

Controller synchronization reads the current native flag, which the common
motion transition obtains from motion metadata. The observation does not copy
controls or apply recorded CPU input.

The offset is the scalar used by `ftCommon_800804EC`, which multiplies it by
the instance's X scale when common spawn positioning needs the offset. Reading
or assigning the property itself does not multiply, move or respawn the fighter.
Assignments accept numeric expressions through ordinary Float conversion and
preserve the resulting Float bits without finite-value clamping.

Both fields also work on a retained Fighter receiver. The same caller and
target lifetime requirements described above apply to reads and writes; an
Article can use its attachment owner even when that owner runs another source
package or is a stock fighter. Missing or stale receivers reject before a write.

```text
spawn.horizontal_offset = attrs.primary_spawn_offset
if attachment_owner.exists:
    let bool sync = attachment_owner.motion.allows_controller_sync
    attachment_owner.spawn.horizontal_offset = 2.5
```

The write changes only the offset field and invokes no callback or allocation.

## Middle-walk reference

`ground.mid_walk_point` is a read-only Float copied from the current Fighter's
common attributes. It is the native middle-walk velocity reference, rather than
current speed or an instruction to move. The read preserves Float bits and works
through the same retained stock or independently authored Fighter receivers as
the observations above. It invokes no callback or allocation.

## Cached collision position

`collision.position.x`, `.y` and `.z` are read-only Float observations of the
native Fighter collision record's `cur_pos`. They preserve the stored Float
bits, including NaNs, infinities and signed zero. This saved collision position
can differ from the Fighter's current gameplay `position`; it is not an alias.
Fighter callbacks and retained Fighter receivers can read it, including stock
and independent source Fighters. Physical identity and scene checks precede
publication. The read performs no allocation or callback.

Article callbacks read the corresponding root Item collision record; see
[Item observations](item_observations.md).

## `floor.friction`

Read the active stage material's friction multiplier from the current source's
cached floor contact. The low byte of the floor flags selects the material;
other flag bits do not affect friction. Use this with authored ground movement
after the frame's collision state has been established. This property is
read-only and available to Fighters and owned Articles. Invalid material indices
are rejected; it does not perform a collision query or use the fighter's base
friction attribute. See [collision observations](combat_visuals.md) and
[stage queries](stage_queries.md).
