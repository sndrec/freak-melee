# Fighter callback slots

Bind a zero-argument, Void helper to an engine callback without calling it immediately:

```text
func clear_attachment():
    pass

callbacks.damage(clear_attachment)
callbacks.death2(clear_attachment)
callbacks.reflect_hit(None)
```

The interpreter retains an owned code ID, not a generated function pointer.
`None` clears the slot. Helpers must belong to this fighter program; functions
with arguments, non-Void results or another entity owner are rejected. A named
event can only be installed in its matching slot.

## Named events and registration

Defining an instance event does not automatically arm it. Register it at the
point where the move needs the callback:

```text
state int reflections

action Contacts:
    on reflect_hit:
        state.reflections = state.reflections + 1

func create_bubble():
    reflector.create(attrs.reflect_bone, 50, 0, 0, 0, 5, 1.5, 1, false)
    callbacks.track_reflect_hit()
```

Create the reflector first: replacing it clears the previous contact handler.
See [reflectors](reflector.md) for the bubble's data and lifetime.

| Helper binding | Named event registration | Native timing |
| --- | --- | --- |
| `callbacks.damage(helper)` | `track_damage()` / `on damage` | Primary damage response. |
| `callbacks.secondary_damage(helper)` | No named-event tracker yet; use an owned helper. | Knockback-applied damage branch, after damage/knockback calculation and before the normal reaction entry. |
| `callbacks.death1(helper)` | No named-event tracker yet; use an owned helper. | First death callback. |
| `callbacks.death2(helper)` / `death3(helper)` | `track_death2()` / `track_death3()` / `on death2` | Separate native death slots; the existing named death2 event supplies both registrations. |
| `callbacks.pre_hitlag(helper)` / `post_hitlag(helper)` | `track_hitlag()` arms both matching named events. | Entering/leaving hitlag at the existing engine call sites. |
| `callbacks.deal_damage(helper)` | `track_deal_damage()` / `on deal_damage` | Dealing damage. |
| `callbacks.state_changed(helper)` | `track_state_change()` / `on state_change` | One-shot state change; clears after execution, including a registration made inside that callback. |
| `callbacks.reflect_hit(helper)` | `track_reflect_hit()` / `on reflect_hit` | Projectile contact with the reflector. |
| `callbacks.reflect(helper)` | `track_reflect()` / `on reflect` | Completion of the shared reflected facing-turn animation. |
| `callbacks.hurtbox_detect(helper)` | `track_hurtbox_detect()` / `on hurtbox_detect` | Existing body-contact branch. |
| `callbacks.shield_hit(helper)` | `track_shield_hit()` / `on shield_hit` | Existing shield-contact callback branch. |
| `callbacks.accessory_before(helper)` | Use an owned helper. | Nonhitlag accessory slot 2, before slot 1 and the later motion accessory phase. |
| `callbacks.accessory_hitlag(helper)` | Use an owned helper. | Accessory slot 3 during hitlag; the scheduler returns without running slots 2 or 1. |

Methods in the registration column belong to `callbacks`, for example
`callbacks.track_reflect()`. Tracking requires the matching event to exist.
`callbacks.clear_damage_death()` clears primary damage and death2 only.

## Reset, takeover and saving

The shared motion-change reset clears primary and secondary damage, death2, pre/post-hitlag,
deal-damage and the four combat-contact registrations, matching the native
fields it clears. Moves using a preservation path keep the existing registration.
An explicit native callback assignment takes over its slot; the interpreted
registration is dropped when that slot is checked. Explicit helper binding or
`None` clears the native pointer and makes the authored choice authoritative.

A NewLife generation transition preserves authored state while clearing
death1/2/3 and accessory-before/hitlag, matching their native life reset.
Accessory-before/hitlag survive ordinary motion resets. Instance snapshots save code IDs alongside
owned state. Restoration validates the pinned program and callback ABI; the
native Fighter and other engine peers still need to be restored by the aggregate
match snapshot. A helper fault or retirement stops the enclosing script/native
scheduler path. See [synchronous callbacks](synchronous_callbacks.md).

Both accessory instance slots are suppressed by the native accessory-disable
flag. After accessory-before, the engine updates the root translation, then runs
slot 1 and updates translation again if it exists. An owned callback's fault,
retirement or replaced program/storage identity stops that continuation.
These bindings do not consume the article cue.

Animation, interrupt, physics, collision and slot 4 accessory callbacks use the separate
[motion callback API](motion_callbacks.md).

## Comparing the current handler

`callbacks.matches(Callback.slot, helper_or_None) -> bool` compares this fighter's
current interpreted registration with an owned zero-argument Void helper or an
empty slot. Slot constants are `damage`, `secondary_damage`, `deal_damage`, `death1/2/3`,
`pre_hitlag`, `post_hitlag`, `state_change`, `reflect_hit`, `reflect`,
`hurtbox_detect`, `shield_hit`, `accessory_before` and `accessory_hitlag` in the `Callback` namespace.

```text
if callbacks.matches(Callback.damage, clear_attachment):
    callbacks.damage(None)
```

This observation does not call a handler or mutate the slot. A native handler
matches neither a script helper nor None. Invalid slots or incompatible helpers
reject the call rather than returning a guessed answer. The query is source-only:
a helper identity cannot be mixed into another fighter's bank through a recipient
relation. Use that recipient's own helper if it needs to answer such a question.
Snapshots restore the current registration, so the query observes the restored
choice. This permits cleanup to leave a replacement handler intact.

Use `callbacks.matches(Callback.accessory_before, helper_or_None)` and
`callbacks.matches(Callback.accessory_hitlag, helper_or_None)` for identity-safe
cleanup and empty-slot checks. Older per-slot `_is` spellings are not aliases.

Secondary damage is a separate slot, not a second primary-damage notification.
For example, a move can register effect cleanup there while its primary damage
helper also resets charge. It runs only when the native knockback-applied branch
dispatches it. Binding does not change damage, hitlag, knockback or reaction
rules, and `callbacks.clear_damage_death()` does not clear this slot. Use
`callbacks.secondary_damage(None)` explicitly when the move finishes.
