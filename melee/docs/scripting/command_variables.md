# Animation command variables

`command(index)` reads one of the fighter's four integer command variables.
`command(index) = value` writes it. Indices run from 0 through 3.

Command variables connect animation events to move behavior. An animation can
set one at a particular frame, and the move's script reads it to decide what to
do. Each move assigns its own meaning to the four indices and their values.

## Editing timed cues

Choose **Set command variable** in the Action Editor. **Variable index** selects
variable 0, 1, 2, or 3. **Value** is assigned when the event runs. A repeated
event in a loop assigns the value on every marked occurrence.

## Aerial landing

Common aerial attacks use variable 0 to choose their landing behavior:

| Value | Landing behavior |
| --- | --- |
| 0 | Autocancel: use the normal landing state. |
| Nonzero | Use the aerial attack's landing animation and landing lag. |

Set variable 0 to a nonzero value when the landing-lag window begins. Set it
back to 0 when the move should autocancel again. The five aerial landing-lag
attributes set the recovery duration for each attack direction.

## Example

```text
move Example:
    on ground_animation:
        if command(0) == 1:
            command(0) = 0
            # Perform this move's animation-triggered event once.
```

Reset the command after consuming a one-shot event. Otherwise the same value
will still be present on a later update, unless an animation command or another
callback changes it. Existing stock scripts provide concrete examples of this
pattern.

## Choosing storage

Use command variables when the Action Editor's animation events need to
communicate with a move. Use a declared `state` variable for named, typed
fighter-owned state. Use a `parameter` for an editable setting, such as a speed or duration. All of these fighter-owned runtime values participate in rollback.

See [Language](language.md), [Common fighter access](common_fighter.md).
