# Check a custom fighter

Use a small, repeatable practice test after each group of related changes.
Keep an unchanged copy of the fighter for comparison. Record which move,
costume, direction and inputs produce a problem.

## Move logic

Run `scripts.check` through the [agent tools](agent_tools.md) or use the
Workshop's script check before testing gameplay. Repair its diagnostics, then
test the move from both facing directions, on the ground and in the air where
applicable. Check its first active frame, last active frame and interrupt.
Also try interrupting it with damage, landing and a stock loss.

Compare an edited move with the unchanged copy using the same inputs. Look at
movement, hit timing, damage and recovery. The relevant values should be
editable in the move's source or [parameters](language.md#fighter-attributes).

## Captures and throws

Follow [Capture API](capture.md) and
[held fighter transitions](fighter.md#held-fighter-transitions) to connect the
holder and victim. Check the victim's attachment throughout the animation,
including when the holder turns or moves vertically. Verify the release pose,
damage and launch direction.

Test escape, a missed grab, damage to the holder, damage to the victim and either
fighter being removed. After each release, verify that both fighters can move
and be grabbed again. If a callback stops because an actor has been removed,
changes completed before that point remain applied.

## Articles and resources

Use [article resources](article_resources.md) to supply owned geometry, motion
and collision data. Spawn the article facing both directions and check its
initial position, speed, hit eligibility and lifetime. Check stage contact on
flat ground, slopes and near an edge.

For [model animation](article_models.md), check the model, material and texture
tracks independently. Scrub important frames and then play the whole move.
For delayed effects, verify when the position is sampled and what happens if
the creator or attached model disappears before the effect runs.

## CPU behavior

Use [CPU policies](cpu_policies.md) and [history queries](cpu_history.md) to define
the fighter's decisions. Test the intended situations at several CPU levels:
an opponent in range, an opponent out of range, recovery and an unavailable
target. For paired fighters, also test separation and loss of either member.
Check that input copying and recovery decisions use the current actor's state.

## Stage queries and targets

Read [stage line queries](stage_lines.md), [item sweeps](item_sweep_segment.md)
and [target queries](targets.md) for each operation's outputs and missing-target
values. Test ordinary contact, no contact, a slope and an edge. Reset output
variables before repeating a query so an unchanged destination is visible.
Test what happens when the chosen fighter or item disappears between selecting
it and acting on it.

## Costumes and attachments

Check every outfit that uses an edited [attachment](model_attachments.md).
Inspect its location, orientation, visibility and animation during idle,
movement, the relevant move and hitlag. Test both directions and confirm that
removing an item or ending the move restores the intended appearance.

## Report a problem

For a reproducible bug, keep the steps, expected result and observed result
together. Include the fighter revision and a useful frame or screenshot. A
feature request should describe the behavior you want and the existing
operations you tried. Use F2 in the client to send the report.
