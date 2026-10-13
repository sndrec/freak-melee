# Stage queries

## stage.event(channel, value) -> Bool

Sends an integral UInt32 channel ID and a finite Float payload to the current
stage's event provider. Available to the calling Fighter or Article. It returns
the provider's result, or `false` when the stage has no provider. The channel is
not restricted to conversations: custom stage providers may define their own
IDs. Negative IDs and values exceeding UInt32 are rejected without dispatch.

The native conversation channels are exposed as `StageEvent_Conversation_available`,
`StageEvent_Conversation_active`, `StageEvent_Conversation_start` and
`StageEvent_Conversation_cancel`. These are native channel constants, not
fighter-specific scripting operations.

```text
if stage.event(StageEvent_Conversation_start, attrs.radio_speaker):
    callbacks.death1(cancel_conversation)
```

Stage providers may mutate state. The result is published only if the calling
actor, program and storage remain live after dispatch. This method allocates
nothing and calls the existing stage dispatcher directly. See
[fighter statistics](fighter_observations.md) and [callbacks](callback_slots.md) for
the authored policy around the event.

`stage.sweep_box(px, py, pz, x, y, z, top, bottom, left_x, left_y, right_x,
right_y, category)` returns the native signed 32-bit environment flags for a
Fighter source. It initializes an independent collision probe, moves it from
the previous position to the current position, and sweeps the supplied ECB.
The result is the raw flags, not the sweep routine's Boolean. All twelve
geometry arguments are F32. Category is integral; the native classification
uses its low four bits, including for signed or full-width values. Low-bit
categories 1, 2 and 3 are rejected before collision work: native stage callbacks
use those categories for actual Fighter/Item contacts and can dereference their
GObj. An independent sweep has no associated actor. Categories 0 and 4 through
15 retain native semantics; category 5 is used by the native Sheik/Ness query.

`stage.segment_blocked(x0, y0, x1, y1)` returns the native segment-intersection
Boolean for Fighter or Article sources. It includes the remapped stage lines
selected by `mpCheckAllRemap`, with no requested output intersection and no
skip/only line restriction.

Geometry must be finite. Sweep admission also checks the actual native signed
subdivision conversion and the engine's position bounds when debug assertions
are enabled. It does not clamp values or run the sweep twice. Native stage
callbacks run normally, so a sweep may mutate the stage. Results are withheld
if the caller retires, faults, changes physical identity or changes owned
program/storage while the operation executes.

These operations use the engine's existing serialized collision scratch. They
do not establish concurrent or recursively nested collision-query safety.
