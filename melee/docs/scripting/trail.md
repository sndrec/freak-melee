# Weapon ribbons

`on trail` describes a ribbon attached to any costume joint. It uses Melee's
existing three-sample afterimage renderer, so custom fighters can keep the
same appearance without being identified as Link, Marth, or Roy. The action's
trail subactions still start and stop sampling; this callback describes what
to sample and how to draw it.

The callback is available inside an `action` block or a `move` block in the
explicit-base script form. The interpreter writes a bounded native output
record through `trail` fields and may read
[owned parameters](language.md#fighter-attributes), pose/state properties,
and local expressions, including pure engine queries. It cannot call mutating
engine operations or helpers, change
fighter state, consume action cues, or draw random numbers. Rendering and
sampling may request the description more than once per frame; it is not a
simulation tick.

## Describe a ribbon

Put tunable values in `fighter_parameters.fmp`:

```text
int weapon_joint = 38
float weapon_inner_distance = 2.25
float weapon_outer_distance = 13.5
float weapon_inner_fade = 0.15
float weapon_outer_fade = 0.85
int weapon_start_alpha = 192
int weapon_end_alpha = 0
int weapon_inner_red = 255
int weapon_inner_green = 93
int weapon_inner_blue = 17
int weapon_outer_red = 101
int weapon_outer_green = 142
int weapon_outer_blue = 255
```

Then describe the ribbon in the fighter's script:

```text
action WeaponRibbon:
    on trail:
        trail.bone = attrs.weapon_joint
        trail.axis = 0
        trail.inner_distance = attrs.weapon_inner_distance
        trail.outer_distance = attrs.weapon_outer_distance
        trail.inner_fade = attrs.weapon_inner_fade
        trail.outer_fade = attrs.weapon_outer_fade
        trail.start_alpha = attrs.weapon_start_alpha
        trail.end_alpha = attrs.weapon_end_alpha
        trail.inner_red = attrs.weapon_inner_red
        trail.inner_green = attrs.weapon_inner_green
        trail.inner_blue = attrs.weapon_inner_blue
        trail.outer_red = attrs.weapon_outer_red
        trail.outer_green = attrs.weapon_outer_green
        trail.outer_blue = attrs.weapon_outer_blue
```

These illustrative values are not defaults for any particular stock fighter.
Use the raw part index in your own costume's rig; replacing a rig can change it.
The callback does not remap it through a donor fighter's semantic bone table.

## Output fields

| Field | Meaning |
| --- | --- |
| `bone` | Raw costume part index selecting the joint that anchors the ribbon. |
| `axis` | Direction along that joint's matrix column: `0` X, `1` Y, `2` Z. |
| `inner_distance` | Inner endpoint along the chosen joint axis. |
| `outer_distance` | Outer endpoint along the chosen joint axis. |
| `inner_fade` | Inner endpoint's interpolation factor at the oldest sample. |
| `outer_fade` | Outer endpoint's interpolation factor at the oldest sample. |
| `start_alpha` | Opacity at the newest sample, integer `0..255`. |
| `end_alpha` | Opacity at the oldest sample, integer `0..255`. |
| `inner_red`, `inner_green`, `inner_blue` | Inner edge RGB, integers `0..255`. |
| `outer_red`, `outer_green`, `outer_blue` | Outer edge RGB, integers `0..255`. |

Distances use the character's joint/world units. Fade factors interpolate
within the inner/outer interval: `0` means the inner endpoint and `1` the
outer endpoint. They are not opacity. The renderer retains the original
distance-weighted fade and angular subdivisions.

Outputs start at zero, except `bone` starts at `-1`. An absent callback,
unset/out-of-range bone, absent joint, invalid axis, non-finite distance/fade,
or color/alpha outside `0..255` disables the ribbon. Held Beam Swords retain
their item's own trail description, independent of the holder's callback.
No runtime allocation or new persistent fighter state is needed.

## Import stock defaults

The [parameter importer](language.md#fighter-attributes) supports
`attribute byte Name OFFSET` for packed unsigned bytes. It copies the byte
into an ordinary owned `int`, including unaligned offsets. This declaration
is **import-only**: run the importer before compiling, then edit the named
parameter rather than reading a byte offset during play.

Marth and Roy's original `SwordAttrs` starts at byte `120` of their special
attribute block. A translation draft can declare:

```text
attribute float weapon_inner_fade 120
attribute float weapon_outer_fade 124
attribute byte weapon_start_alpha 128
attribute byte weapon_end_alpha 129
attribute byte weapon_inner_red 130
attribute byte weapon_inner_green 131
attribute byte weapon_inner_blue 132
attribute byte weapon_outer_red 134
attribute byte weapon_outer_green 135
attribute byte weapon_outer_blue 136
attribute int weapon_joint 140
attribute float weapon_inner_distance 144
attribute float weapon_outer_distance 148
```

The source ignores the byte at offset `133`; it is not a trail output.
Link and Young Link use the same field layout starting at `100`. Offset
metadata is only for importing the source defaults, not an authoring API
for custom characters. Marth/Roy now have registered asset bases and
[partial special templates](../marth_roy_script_translation.md). Complete
sword-fighter behavior translations remain in progress; Link's base is still
pending.

## Verification boundary

The generic event-record fixture executes the actual frontend, bytecode and VM.
It checks field types, callback scope, reserved storage, nested payload lifetime,
and transactional output publication. No native pointer is visible to a script;
the event descriptor specifies the scalar fields that a callback may write.
Assignments to persistent fighter state and impure engine calls remain forbidden.

The Fighter scheduler stages outputs until the callback has completed and its
actor, program and storage are still current. The afterimage reader additionally
checks the owned rig, raw joint slot, native JObj class and descriptor limits
before using the output. A failed callback does not fall back to stock behavior.
These checks do not establish a rendered stock-versus-clone comparison or
whole-fighter gameplay parity. See the [integration checkpoint](../native_source_trail_hitlag_checkpoint.md)
for the current verification scope and receipts.
