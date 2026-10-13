# Hitbox intervals

Hitbox duration is measured in animation frames. An interval starting at frame
10 with a duration of 3 is active at frames 10, 11 and 12; it ends at frame 13.
The inspector accepts fractions of a frame, rounded to 1/256 of a frame.

Drag a bar to move its start and end together. Drag an edge to resize it.
G moves the selected bar. Confirm with left click or mouse release; cancel
with right click or Esc. A confirmed gesture creates one undo step. Editing
a repeated event applies the same change to every repetition, keeping the
loop period. Each of the fighter's four hitbox slots holds one interval at
a time, so intervals sharing a slot must stay non-overlapping.

Refresh Hit Eligibility controls consecutive intervals in the same slot.
Enable it to allow a new hit on opponents already hit by the preceding
interval. Disable it to retain that history while changing the hitbox's
properties. A gap between intervals starts fresh history.

Opening a Workshop draft converts statically resolved create/clear sequences
to duration-owned hitboxes. The conversion preserves other event times,
finite loops and the original hit eligibility of adjacent intervals. Save
persists the converted data. Runtime-dependent endings, conditional spawns,
zero-length sequences and ambiguous sharing of another slot's hit history
remain in their original command form. The event list exposes those commands.
New hitboxes added from an ordinary create command start with a one-frame
duration. Copying a timed hitbox keeps its duration.

## Runtime storage

The native timed-hitbox subaction uses opcode 59. Its first five words use
the ordinary create-hitbox fields. A sixth word stores duration in its low
30 bits, Refresh Hit Eligibility in bit 31, and clear-all cleanup provenance
in bit 30. The importer validates slot and duration before running the scene.

Each fighter stores four fixed lifetime records beside its other mutable
state. Normal animation steps decrement the records; hitlag leaves them
unchanged. Expiry runs after that step's subactions so an adjacent spawn can
inherit victim history. The command timer accounts for fractional-step
overshoot and distinguishes an adjacent interval from a gap crossed during
the same tick. Explicit clear, ordinary create and script create operations
cancel the old lifetime. Fighter snapshots include lifetime and hit history
together through the existing pool save path.

The timeline compiler rewrites event times into waits. It copies the full
action graph, verifies the resulting execution trace and interval ownership,
then publishes the edit. An invalid move or resize leaves the draft unchanged.

## Verification

`tests/native/hitbox_interval_audit.py` independently decodes saved DAT command
streams. It checks conversion, payloads, loop occurrences, duration edits,
endpoint movement, refresh flags, fractional durations, atomic rejection and
unresolved actions. `native_hitbox_lifetime_test` exercises the production
lifetime functions with controlled engine boundaries, including expiry,
handoffs, gaps crossed by fast steps, refresh, save/restore and cancellation.
These checks cover data and lifetime contracts; the user's Workshop pass
evaluates the authoring workflow with real fighters.
