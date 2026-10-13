# Direct interpreter fighter setup bindings

These operations resolve through the typed catalog in
`src/melee/ft/native_script_bindings.c` and execute in
`native_script_binding_setup.c`. They require a live fighter source with the
recorded generation. They use semantic IDs and fixed caller-owned storage;
scripts contain no machine addresses, C callback pointers or native offsets.
This is the implemented setup family, not acceptance of every operation in the
stock templates.

## Setup, attributes and commands

| Expression or call | Result | Contract |
| --- | --- | --- |
| `fighter.enable_walljump()` | Void | Sets the fighter's wall-jump capability. |
| `fighter.copy_ext_attributes()` | Void | Copies the Definition's extended-attribute block into the fighter's existing instance scratch and selects that scratch. |
| `fighter.reload_ext_attributes()` | Void | Restores that scratch from the immutable extended-attribute source; requires scratch already selected. |
| `fighter.enable_multi_jump()` | Void | Sets multi-jump capability and its attribute reference; requires selected instance scratch. |
| `air.gravity`, `air.friction` | Float | Read-only general fighter attributes; these are typed engine fields. |
| `command(index)` | Int | Reads command slot 0..3, or supplies a writable assignment target. Reads are pure and allowed in read-only events. |

`command(index) = value` converts the value to Int and preserves its 32-bit
representation in the engine's command slot. Reads interpret those bits as a
signed Int, including the high bit. Integral selectors, including Bool 0/1,
are checked at their original width: negative, floating and out-of-range
selectors fail. Read-only events still reject command assignment.

Attribute copying uses `FT_CHARACTER_EXT_ATTR_CAPACITY` (0x424 bytes). Missing
source/scratch and scratch aliasing the immutable source fail before the copy.
There is no allocation. Script `attrs` declarations remain independently typed
immutable parameters; copying native scratch does not grant raw access to them.

## Item and knockback presentation

| Call | Contract |
| --- | --- |
| `fighter.item_pickup(catch_flag: Bool, item_part: Int, hand_part: Int)` | Selects held-item presentation by hold kind, then optionally reveals the hand channel. Heavy items skip these stages. |
| `fighter.item_drop(drop_flag: Bool, item_part: Int, hand_part: Int)` | Clears the item channel, then optionally hides the hand channel. |
| `fighter.item_visible(part: Int)` | Reveals the channel for a held non-heavy item. |
| `fighter.item_invisible(part: Int)` | Hides the channel for a held non-heavy item. |
| `fighter.knockback_enter(channel: Int)` | Applies rate 3 to the selected animation channel and then channel 0. |
| `fighter.knockback_exit(channel: Int)` | Applies rate 0 to the selected animation channel and then channel 0. |

All return Void. Item/hand selectors are 0 or 1; knockback selectors are 0..255.
Pickup and visibility require a held item. These implement the generic engine
stages from `Fighter_OnItemPickup/Drop/Visible/Invisible` and
`Fighter_OnKnockbackEnter/Exit` in `ft/inlines.h`, calling the animation primitives
directly. No stock fighter special handler or legacy host table is called.

## Animation cues

`cue.reverse.consume()`, `cue.article.consume()` and `cue.throw.consume()`
return Bool and clear their respective reverse/article/throw signal bit. A
second consume returns false until the signal is raised again.
`cue.throw.reset()` returns Void and clears the complete throw-signal word.
Consumes mutate state and are rejected in read-only events.

## Authored callbacks

Each of these accepts a code reference or `None` and returns Void:

```text
callbacks.damage(handler)
callbacks.death1(handler)
callbacks.death2(handler)
callbacks.death3(handler)
callbacks.pre_hitlag(handler)
callbacks.post_hitlag(handler)
callbacks.deal_damage(handler)
callbacks.state_changed(handler)
```

A handler must be a same-owner zero-argument Void helper, or the matching
zero-argument Void event. Validation uses the attached actor's published program.
`None` clears that slot. A function reference stores a program function ID in
the actor's rollback-owned callback state; it never installs an authored C
thunk or exposes a pointer. Missing actor, wrong owner/signature, invalid
function ID and overflowing IDs fail explicitly. Death3 accepts the Death2
event, preserving the existing contract.

The following zero-argument methods select cached event IDs:

| Call | Bound event(s) |
| --- | --- |
| `callbacks.track_damage()` | damage |
| `callbacks.track_death2()` | death2 |
| `callbacks.track_death3()` | death2 in the Death3 slot |
| `callbacks.track_hitlag()` | pre_hitlag and post_hitlag; both must exist |
| `callbacks.track_deal_damage()` | deal_damage |
| `callbacks.track_state_change()` | state_change |
| `callbacks.clear_damage_death()` | Clears Damage and Death2 only. |

These calls use `nativeScriptFighter_CallbackFunction` and
`nativeScriptFighter_BindCallback`; they do not scan source names during
execution. Motion/life resets clear authored slots. A later native pointer
written into a slot supersedes the authored binding, which the scheduler
invalidates before dispatch.

`callbacks.reset_root_x_on_damage()` remains unsupported. Its behavior must
be authored using a general bone-transform operation when that family exists;
there is no fighter-specific surrogate. Accessory, capture and hurtbox callback
signatures, article-owned callback clearing and model-effect receivers are
outside this setup family.

Every public native binding checks the source generation before execution and
before result publication. Multi-stage item/knockback operations check again
between animation stages. If a stage retires or replaces the fighter, the VM
stops before subsequent authored statements or result publication.

## Focused evidence

`tests/native/script_setup/run.py` builds the real source frontend, typed
catalog, bytecode and VM with controlled presentation leaves and callback actor
boundary. It compares pickup stage sequences with the actual inline engine
body, tests 32-bit command values, cue mutation, callback code-reference/None
marshalling, attribute ownership, read-only command reads/write rejection, and
retirement before the next authored write. The separate
`tests/native/script_fighter` fixture exercises the real owned callback registry
and engine dispatch bodies. Neither fixture establishes live game parity or
article/effect closure.
