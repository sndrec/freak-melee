# Animation authoring with an agent

The animation tools edit a fighter's owned clips and use Melee's curve evaluator
to sample them. Saved-draft tools start with `animations.`. Live Workshop tools
start with `session.animations.` and operate on unsaved changes.

See [Agent tools](agent_tools.md) for installation setup, revisions, Save, Build,
practice and export. Inspect the tool catalog for complete argument schemas.

## Find a clip and its bones

Call `animations.read` with `view: "clips"`. Each row contains a clip reference:

```json
{"bank": "action", "index": 72}
```

Use the reference returned for your fighter. `action` clips belong to its action
table. `demo` clips include results and other non-gameplay animations. The
`available` field indicates whether the owned bank contains that clip.

The read views support pagination through `offset`, `limit` and `next_offset`:

| View | Contents |
| --- | --- |
| `clips` | Clip references, names, availability, frame counts and track counts. |
| `bones` | Bone IDs, names, roles, parents, depth and local rest transforms. Supply a clip to include its node mapping. |
| `info` | Clip length, binding flags and supported capacities. |
| `tracks` | Track IDs, bone associations, channels, units and key counts. |
| `keys` | One track's key IDs, integer frames, encoded values, tangents and editable fields. Supply `track`. |
| `pose` | Evaluated bone-local transforms at `frame`, including rest values for unanimated channels. |
| `samples` | A track evaluated at a sorted `frames` array, including fractional frames. |

Bone IDs identify costume skeleton joints in hierarchy order. Animation nodes
can also include reserved parts. The tools resolve that mapping for clips bound
to this fighter. A foreign-rig clip still exposes its source tracks and samples;
bone-local posing and bone-key insertion require a matching rig binding.

Positions use game units, rotations use radians and scales use multipliers.
`samples` returns `null` before a channel has supplied a value. A pose uses that
bone's rest value until the channel supplies its first value.

## Create or duplicate a clip

Use `operation: "create"`, a `target_action`, an alphanumeric/underscore `name`
and a whole `frames` length from 1 to 1000. The new clip starts from the fighter's
rest transforms.

Use `operation: "duplicate"` with a source `clip`, `target_action` and `name` to
assign an independent copy. The copied clip carries its source rig binding.
The target action keeps its events and playback/root-motion settings.

`operation: "duration"` sets an existing clip's `frames` end point. To shorten
it, first move or delete keys inside the old playback range that extend past the
new end. State callbacks and action events control transitions and interrupts;
inspect those in Actions and Scripting when adjusting a move's overall timing.

## Insert and edit keys

`operation: "insert"` takes an `insertions` array. Each entry supplies `bone`,
`channel`, integer `frame` and `value`. Channels are `position_x/y/z`,
`rotation_x/y/z` and `scale_x/y/z`.

```json
{
  "insertions": [
    {"bone": 4, "channel": "rotation_z", "frame": 10, "value": 0.5}
  ]
}
```

Resolve the bone ID from the actual fighter first. An insertion updates an
existing keyed frame or adds a new one. A newly animated channel starts from its
rest value at frame zero before the authored key.

For `operation: "change"`, supply inspected `track`/`key` pairs and their changed
`frame`, `value` or `tangent` fields:

```json
{
  "keys": [
    {"track": 8, "key": 2, "frame": 12, "value": 0.75},
    {"track": 17, "key": 3, "tangent": 0.125}
  ]
}
```

Read the keys' editable flags first. A tangent is a slope measured in channel
units per frame. The native formats include constant, linear and Hermite spline
segments, plus slope-only records. Tangent handle timing is fixed by that curve
format. Packed tracks quantize values and slopes; replies include applied values.

Keys at one source frame move together. Distinct source frames retain their
order. The operation rejects a batch that crosses neighbours, merges distinct
frames or exceeds an encoding limit. The whole draft is preserved on rejection.

## Work across bones

Deletion and transformation accept explicit key references or a `selection`:

```json
{
  "selection": {
    "bones": [4, 5],
    "channels": ["rotation_z"],
    "start_frame": 5,
    "end_frame": 20
  }
}
```

Omitting a filter includes its full range. An empty selection object selects all
keys from frame zero onward. One edit supports up to 32768 selected keys.
Deleting every value in a channel removes that channel, allowing its rest value
to apply.

`operation: "transform"` also takes a `transform` object. It supports
`pivot_frame`, `pivot_value`, `time_scale`, `value_scale`, `rotation`,
`frame_offset` and `value_offset`. Scales default to 1; other values default to 0.
The operation scales around the pivot, rotates, then adds offsets. Rotation uses
radians in the graph's frame/value plane, where one frame and one channel unit
form the two axes. Transform channels separately when their units need different
scaling. Tangents follow the transform; a vertical tangent is rejected because
the source curves represent finite slopes. Key frames round to whole frames.

`applied_keys` includes the first 200 selected keys' applied data. The reply gives
the full selected/changed counts and flags a truncated detail list. Inspect
tracks and keys again to read a large result. Track/key indices belong to the
inspected revision; insertion and deletion can change subsequent indices.

## Copy an endpoint pose

Use `operation: "copy_pose"` with destination `clip` and `frame`, plus
`source_clip` and `source_frame`. An optional `bones` list selects a subset.
`include_root_translation` controls whether the source's root translation is
included. Source and destination use matching rig bindings.

The current Workshop pose buffer supports the joint capacity reported by
`info`. It copies matching destination channels and returns `written_channels`
and `skipped_channels`. Add missing channels with insertions when needed. This
is useful for matching a ledge getup to hanging idle, or an attack's ending pose
to standing idle.

## Verify the visible result

In the live Workshop, `session.animations.select` chooses a clip and seeks its
frozen preview. `session.animations.preview` reads the actual rendered actor's
applied clip/frame, bone-local transforms and world matrices. Poll while it
returns `preview_pending`. This distinguishes the pose reached by the actor
from a fresh sample of the draft. Root display/facing transforms can contribute
to the actor's root values.

Live animation edits share Workshop Undo/Redo. Save preserves the draft. Refresh
the preview to load the current complete fighter, then use Test Character or
`session.practice.start` to check the move during gameplay. Build/install the
reviewed revision and export its exact package when it is ready to share.
