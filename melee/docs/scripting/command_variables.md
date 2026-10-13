# Animation command variables

`command(index)` reads one of the fighter's four integer command variables.
`command(index) = value` writes it. Indices run from 0 through 3.

Command variables connect animation events to move behavior. An animation can
set one at a particular frame, and the move's script reads it to decide what to
do. Each move assigns its own meaning to the four indices and their values.

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
