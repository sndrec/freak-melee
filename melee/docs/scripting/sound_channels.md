# Fighter sound channels

Fighter sounds can be requested by [scripts](combat_visuals.md#effects-sound-and-color)
or by ordinary subaction sound events. The engine maintains separate channels
so a voice, movement sound and looping effect can coexist. This policy controls
which channels respect the fighter's temporary sound-suppression flag. It does
not mute the application, change sound banks, or suppress music and global audio.

## `sound_mute_channels`

Declare this once at the top level of a fighter script:

```text
sound_mute_channels 0x44
```

Each bit selects one channel. A selected channel skips requests while the
engine's fighter-suppression flag is set. Unselected channels continue normally.
The value must be an integer literal from `0` through `127` (`0x7F`); decimal
and hexadecimal forms are supported. Expressions and duplicate declarations
are errors. This is fixed package data, rather than per-frame fighter state.

| Channel | Bit | Ordinary retained sound |
| --- | --- | --- |
| 0 | `0x01` | General fighter sound. |
| 1 | `0x02` | Channel 1. |
| 2 | `0x04` | Channel 2; playback stops channels 2 and 6 first. |
| 3 | `0x08` | Channel 3. |
| 4 | `0x10` | Channel 4. |
| 5 | `0x20` | Channel 5. |
| 6 | `0x40` | Channel 6; playback stops channels 2 and 6 first. |

For example, `0x44` selects channels 2 and 6, `0` selects none, and `0x7F`
selects all seven. Omitting the declaration preserves the copied package base's
policy. Stock-equivalent bases use `0x44`, except Samus and Game & Watch, which
use `0` to reproduce their original sound exceptions. Independent scripts can
author their own policy without checking a stock fighter identity.

The policy applies to direct fighter requests and positioned and randomized
subaction sound events. Suppressed subaction events still consume their commands. Positioned events
keep their sound mapping, and randomized events keep their sound selection.
Channel 0 also keeps its pitch RNG draw when suppressed. Ordinary channels
1 through 6 retain their existing early-filter ordering, before sound mapping.
The policy does not skip the event stream or its shared RNG consumption.

## Costume sound routing

Use `sound_remap` when costumes share an audio bank but use different sound IDs:

```text
# Costumes 2..31 use the alternate voice range in bank 13.
sound_remap 0xFFFFFFFC 13 0x1FBFD 0x1FC62 0x66
# Costumes 0 and 1 use the first range, even if an event names the second.
sound_remap 0x3 13 0x1FC63 0x1FCC8 -0x66
sound_ko_cue 0xFFFFFFFC 0x1FBD1
```

The arguments to `sound_remap` are `COSTUME_MASK BANK FIRST LAST OFFSET`.
Bit 0 selects costume 0, bit 1 costume 1, and so on through costume 31.
`FIRST` and `LAST` are inclusive canonical sound IDs. A matching rule adds
`OFFSET`. Rules run in declaration order, so a later rule sees the mapped ID.
The bank is the original canonical sound's bank throughout the pass. Other
common sound routing still runs afterward. IDs outside the range, other banks,
and costume indices beyond 31 pass through unchanged.

`sound_ko_cue COSTUME_MASK SOUND_ID` replaces the common KO cue for the selected
costumes. Other costumes keep the copied fighter data's default cue. It does not
change KO behavior or add another sound on top of the existing one.

Both declarations belong at top level before code blocks and accept integer
literals, including hexadecimal masks/IDs and negative offsets. A package can
have up to 16 remapping rules and 8 KO cue rules. KO masks must not overlap.
Bank indices are 0..63; masks must be nonzero; input and mapped IDs must remain
nonnegative signed 32-bit integers. Invalid declarations fail at their source
line. Omitting routing declarations retains the copied base's immutable policy.
These tables belong to the compiled package, require no per-frame allocation,
and never dispatch a fighter callback from the audio resolver.

## Stopping sounds

Explicit channel-stop and fighter sound-cleanup operations remain unconditional.
A sound event carrying a stop sentinel is different: it passes through that
event's channel filter, as in the original channels 2 and 6. Suppressed requests
leave the channel's retained handle unchanged; they do not stop an existing
sound. The policy adds no callbacks, allocations or fighter-specific handlers.

## Direct channel requests

### `sound.clip(channel, index, volume, pan)`

Play a clip from this fighter's [sound bank](agent_resources.md#import-fighter-sounds).
`index` starts at zero. Use channel 0 for a one-shot sound or channels
1 through 6 to retain a handle on the corresponding fighter sound channel.
A clip with looping audio needs a retained channel. Volume ranges from
0 to 127, and pan uses 64 for center.

```text
parameter int takeoff_sound = 0
sound.clip(0, attrs.takeoff_sound, 127, 64)
```

To play a clip on an animation frame, add a **Sound effect** event in the
Action Editor. Turn on **Use fighter sound bank** and enter the clip index.
In `actions.edit`, create a `sound` event with
`"properties": {"package_clip": true, "sfx_id": 0}`. The `behavior` property
selects the same fighter sound channel.

### `sound.channel(channel, sound_id, volume, pan)`

Fighter callbacks and fighter helper functions can request any retained channel:

```text
sound.channel(channel, sound_id, volume, pan)
sound.channel(2, -1, 127, 64)
```

All four operands must have Int type. Dynamic channels0..6 are supported; Bool
and Float values require an explicit `int(...)` conversion. Arguments run once,
from left to right, including when the channel is outside0..6. An invalid channel
skips the call. This statement has no result and is unavailable in article and
model-effect callbacks. An article can call a validated creator's fighter helper
when that fighter owns the sound decision.

The generated code directly indexes a read-only seven-entry engine callback
table. Module initialization requires the table and every entry. Sound IDs remain
full-width signed integers, including the native stop sentinel `-1`; volume and
pan convert to native unsigned bytes (for example300 becomes44). The actual
channel functions retain their mapping, stop, mute-filter and handle behavior.
No stock-kind dispatch or additional runtime sound wrapper is involved.

The ordinary callback lifetime checks run after each effectful argument and
after dispatch. If an argument or callback retires or recycles the source,
remaining arguments and later authored statements do not execute. Existing
`sound.play` and `sound.loop` remain available for their established channels.

`tests/native/sound_channel_calls` tests both generated action and motion routes,
ordered argument side effects, all seven entries, explicit stop IDs, integer
bounds, byte conversion and missing-table/slot rejection. Its channel callbacks
are controlled boundaries; actual channel policy has the separate accepted
`fighter_sound_mute_policy` source fixture. Neither console fixture emits audio
or claims acoustic/gameplay acceptance.

For audio that belongs to the world rather than a fighter channel, see
[`audio.play_global()`](stage_movement.md#global-sound).
