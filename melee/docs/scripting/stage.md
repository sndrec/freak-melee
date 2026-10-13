# Stage events

## `stage.internal_kind`

Read-only integer identifying the current internal stage variant, read through
Melee's `Stage_80225194` (`selected_stage.stkind`). Available in
fighter and article expressions. Internal IDs distinguish variant stages and
are separate from the visible stage-select index. Querying the ID does not
change the stage or run collision. Keep any variant-specific decision in your
script; use [terrain queries](#terrain-queries) for actual geometry.

## Terrain queries

### Floor and line-island reads

```text
stage.read_floor(x0, y0, x1, y1, out_x, out_y, out_line)
stage.read_line_island_bounds(line, first_x, first_y, last_x, last_y)
stage.read_line_island_bounds(line, first_x, first_y, first_z, last_x, last_y, last_z)
```

These Bool queries work in fighter and article callbacks and helpers. Coordinate
inputs accept numeric expressions; line inputs require integral expressions
within the native signed 32-bit index range. Output destinations must be distinct
writable scalar state fields or typed locals: Float coordinates and an Int floor
line. Inputs run once in written order. False preserves every destination.

`read_floor` calls the native floor-segment probe with offset zero, no skipped or
required line/joint, and no filter callback. Nonfinite inputs return false without
probing. A successful read returns the native contact X/Y and collision-line index.
It does not change the caller's collision record.

The two island-bounds forms copy the engine's stored `x8` and `x14` endpoints in
that order, with either X/Y or all three components. They do not sort, normalize
or clamp the endpoints. Missing, disabled or invalid lines return false. The
interpreter preserves outputs on failure in both forms; the older article C
exporter instead wrote six zeros on failure.

`stage.same_line_island(first, second)` returns true only when both valid lines
resolve to the same non-null native island. `stage.cpu_floor_excluded(line)` runs
the common Big Blue, Mushroom Kingdom, Corneria and Venom floor exclusions.
`stage.cpu_island_blocked(line)` runs the common active-item and stage hazard
assessment. Missing or invalid lines return false. These queries expose no island
pointer, allocate no storage and select no follower behavior. Source retirement
or stage-geometry replacement rejects publication.

### `stage.sweep_box(previous_x, previous_y, previous_z, x, y, z, top, bottom, left_x, left_y, right_x, right_y, mode)`

Returns an Int containing the engine's raw stage-contact flags. All positions
and box dimensions are numeric expressions; `mode` is an Int. Inputs are
sampled once in the written order. The query initializes a temporary collision
record and sweeps the specified fixed box between the two world positions.
It does not alter the fighter's collision record, scale the box, allocate
memory, or select a response for the move.

Test the returned flags with `Collision.floor_mask`, `Collision.wall_mask`,
`Collision.left_wall_mask`, `Collision.right_wall_mask`, and
`Collision.ceiling_mask`. The script owns
any remapping or response to contacts. A sweep's internal Boolean result does
not discard its contact flags.

### `stage.compare_floor_islands`

```text
stage.compare_floor_islands(x, y, z, other_x, other_y, other_z, depth, compare_other, found, same, left_x, right_x)
```

Probe the first world position for a floor island. Probe the second only if
the first exists and `compare_other` is true. `same` requires actual non-NULL
island identity; identical endpoints from separate islands do not match.
Available directly in the interpreter for fighter and article helpers/callbacks.
The final four arguments must be distinct writable destinations: two Bool locals
or state fields, followed by two Float locals or state fields. They receive
first-island existence, shared identity and first-island X bounds. Missing first
islands publish false, false, zero and zero. Disabling the second probe publishes
`same = false` even when the first island exists.

The native probe runs vertically from `(x, y)` to `(x, y + depth)`; Z is retained
in the world-position input but does not change this collision test. Bounds use
the native island endpoint query, including current moving-floor endpoints,
rather than only the island's stored initial endpoints. Coordinates, depth and
each requested probe endpoint must be finite. Source retirement or stage geometry
replacement rejects all output publication. Input expressions are evaluated once,
and probes keep their stated order. The operation allocates nothing and draws no random value.
It does not change a fighter's floor contact. Collision-line identity is a
separate query: [`stage.same_line_island`](#floor-and-line-island-reads).

### `floor.line`

Read the current floor collision-line index. Use it after the fighter's collision
update has established floor contact; an airborne fighter can retain an old index.

### Floor blocking in CPU decisions

Use `stage.read_floor` to probe only floor geometry. Its Bool result reports a
segment contact; its three outputs provide the contact X/Y and line identity.
Walls and ceilings are not included. When reproducing the native CPU floor
policy, also reject the stage-specific lines identified by
`stage.cpu_floor_excluded`:

```text
let float probe_x = 0
let float probe_y = 0
let int probe_line = -1
let bool blocked = stage.read_floor(position.x, position.y, other.position.x, other.position.y + 5, probe_x, probe_y, probe_line)
blocked = blocked and not stage.cpu_floor_excluded(probe_line)
```

The second query executes only when a floor was found. This composes the native
CPU helper's zero-thickness, no-excluded-line probe and subsequent stage policy.
Use the unfiltered `read_floor` result if your move should treat every floor as
blocking. Neither operation changes fighter contact state or draws randomness.
See [CPU threat queries](cpu_policies.md#on-cpu_threat).

### `stage.follow_floor(line, x, y, z, distance, max_gap, found, out_x, out_y, out_z)`

Starting from a world position, follow a signed distance along the floor line and
its connected segments using the shared stage collision helper. Positive distance
walks in the helper's positive direction; negative distance walks the other way.
`max_gap` limits the accumulated distance across intervening wall segments.
Connected-line selection retains the native primary/secondary-link rules.
This is a floor-following query, rather than a vertical ray cast.

The last four arguments must be distinct writable destinations: a Boolean and
three Floats. Locals, fighter state, and article state are supported. Input
expressions may read the same fields that are later written as outputs.
`found` becomes true on success and the other outputs receive the world position.
On failure, `found` is false and the position outputs receive the supplied origin.
No fighter position or contact state is changed. Only the supplied output
destinations are written, after the source and geometry remain current. Missing
terrain or an inactive/out-of-range line returns the origin with `found = false`;
invalid argument types, nonfinite input values, negative `max_gap`, or a retired
source fault without publishing outputs. Native stage topology must be valid.

```text
let bool found = false
let float x = 0
let float y = 0
let float z = 0
stage.follow_floor(floor.line, position.x, position.y, position.z, 12, 10, found, x, y, z)
if found: hitbox.set_world_position(0, x, y + 2, z)
```

See [world hitbox placement](combat_visuals.md#hitboxset_world_positionindex-x-y-z).
The [DK template](../../examples/donkey_wip.fms) deliberately uses the failure
origin as well, matching Hand Slap's original fallback.

### `stage.island_bounds(x, y, z, probe_y, found, left_x, left_y, right_x, right_y)`

Probe a vertical segment from the supplied world position to `y + probe_y`.
If it intersects a floor, query the outer bounds of that connected collision
island, including its current moving-platform transform. The island can contain
multiple collision lines. This differs from the whole stage's camera or blast
bounds, and from following one floor line.

The last five arguments must be distinct writable Boolean/Float destinations.
Local variables, fighter state and article state are supported.
On success, `found` becomes true and the other outputs contain the island's
left/right world X/Y coordinates. On failure, `found` is false and both points
receive the supplied origin X/Y. The query does not move the actor or alter contact state, consumes no random
number and exposes no native pointers. It publishes only the supplied outputs
after its source and native geometry remain current. Missing terrain is a normal
false/origin result; malformed types, nonfinite coordinates, probe overflow or a
retired source fault without publishing. Native stage topology must be valid.
Unlike `stage.read_line_island_bounds`, this operation refreshes dynamic endpoints
through the common engine bounds helper.

```text
let bool found = false
let float left_x = 0
let float left_y = 0
let float right_x = 0
let float right_y = 0
stage.island_bounds(position.x, position.y, position.z, -10, found, left_x, left_y, right_x, right_y)
if found:
    let closer_right = abs(position.x - right_x) < abs(position.x - left_x)
```

The [CPU policy reference](cpu_policies.md) shows how a fighter can use this
query to choose where to carry a captured opponent.

## Named stage events

`stage.event(channel, value)` sends a named event to the active stage and
returns a boolean result. It can also be used as a statement when the result
is not needed. The stage owns its objects, animations and event decisions;
the fighter owns the decision to send an event.

```text
if stage.event("conversation.available", 0):
    if stage.event("conversation.start", 0):
        callbacks.track_death1()
```

The channel must be a quoted, nonempty name of up to 61 ASCII letters, digits,
underscores or periods. Names are case-sensitive and hashed when the script
is compiled. `value` is a float expression. No allocation or string lookup
occurs in the simulation. Unsupported channels and stages without a provider
return `false`. A query must still go to the appropriate provider: the engine
does not infer semantics from the spelling of a channel.

## Conversation channels

Corneria and Venom currently implement these channels for their existing radio
sequence. The fighter's [wait-input callback](language.md#predicate-callbacks)
decides whether the input may start a taunt; its animation callback starts
the stage sequence at the authored subaction cue.

| Channel | Value | Result and effect |
| --- | --- | --- |
| `conversation.available` | Ignored | `true` when the stage provides conversations. Does not start one. |
| `conversation.active` | Ignored | Whether a conversation controller exists. Does not change it. |
| `conversation.start` | `0` or `1` | Starts the provider's corresponding speaker sequence, returning whether it started. Other values return `false`. |
| `conversation.cancel` | Ignored | Stops the stage conversation and its text/audio overlay. Corneria returns whether a controller existed; Venom accepts the cancellation even when none exists. |

Starting a sequence can consume the stage's gameplay RNG. Calling `start`
just to test availability is therefore incorrect. These channels describe
this provider's contract, not fighter-specific scripting operations.
Future stages can implement their own channels in `StageData.on_event`.

The [Fox script](../../examples/fox_wip.fms) keeps phase and facing
in named fighter state, then sends a cancellation from its registered
[`on death1`](fighter.md#callbackstrack_death1-and-callbacksclear_death1)
callback. Its [translation ledger](../fox_falco_script_translation.md) records
the tested paths and remaining acceptance.

## Sound-bank dependencies

A native fighter definition's `sound_banks` bitmask declares the ISO banks
its sounds require. Cloned definitions carry explicit defaults independently
of their behavior family. The ordinary scene audio loader combines these
dependencies with those of the stage and other fighters before play; radio
lines in a fighter bank are included too. Only bank indices `0..54` are valid.
The loader reads the prepared next-scene definitions before those fighters
become active, so switching matches does not reuse an outgoing fighter's banks.
Indices refer to the audio loader's SSM list, not fighter or character IDs.
This is native definition metadata, not an editable `.fms` declaration yet.
It does not import or package new custom sound banks.

Dialogue completion uses the original playback-status query. The native mixer
currently advances with audio-device demand, so accelerated replays are not
an audio-duration oracle. The radio comparison suite uses normal pacing.

## Article collision lines

These operations are available in [article callbacks](article.md). Collision
lines are indices into the live stage geometry, not persistent asset IDs.

- [`stage.line_exists(line)`](stage_lines.md) tests whether a line remains present.
- `stage.line_normal(line, x, y, z)` writes its normal into three distinct
  writable float destinations.
- `item.bind_stage_line(line)` dispatches the native floor or ceiling contact
  callback for that line using the article's actual collision data.
- `item.read_stage_line_speed(line)` replaces the article's velocity with the
  stage's moving-line remap calculation, using the article's current position.
  It returns the shared operation's bool result when used as an expression.

[`item.sweep_segment(previous_x, previous_y, previous_z, normal_x, normal_y,
normal_z, line, flags, hit)`](item_sweep_segment.md) sweeps from an explicitly
supplied previous position to the article's current position using the shared
item collision operation. The last six arguments are distinct writable outputs:
three floats, two ints and a bool. The normal, line and flags are initialized
from their existing values and preserved on a miss. A hit snaps the article's
native position and copies the engine result directly, including its line and
all collision flag bits. The script owns attachment, reflection, embed and
expiry choices. No move-specific policy is hidden inside the sweep.

Check line existence when retaining a line between callbacks. Querying a
normal or binding a line requires a valid stage line; those operations do not
silently choose another surface.
