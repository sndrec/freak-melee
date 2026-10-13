# Item ground following and landing

`item.respond_to_stage()` runs the native airborne floor, wall and ceiling sweep,
copies the resolved position, and invokes `on stage_contact` if it touches any
surface. It applies the native side-wall response and floor contact reset before
your callback. `item.respond_to_stage(impact_response: Bool)` uses the same
routine when false; true applies the broader wall/ceiling impact response instead.
The Bool is evaluated once; numeric values are rejected. Both return Void.

```text
on collision:
    item.respond_to_stage(true)
    return false
on stage_contact:
    item.signal("explode", 0)
```

This call does not choose the item's next state, lifetime or destruction policy.
Put those decisions in the contact hook. It uses the shared native collision
routines `it_8026E71C` (default) and `it_8026E5A0` (impact response).

`item.follow_ground()` follows the native contacted floor. The engine updates
collision history, Item position and the floor line. Losing the floor changes
the Item to airborne and immediately invokes its `on stage_left` hook.

`item.slide_on_ground()` runs the native grounded sliding response. It updates
collision history, Item position and the contacted floor, applies floor motion
and wall response, and invokes `on stage_left` after losing the floor. It invokes
`on stage_landed` when the native grounded response selects settlement. These
are separate native tail callbacks; this operation returns Void.

`item.bounce_to_ground()` runs the shared native air collision and landing
response. `item.bounce_to_ground(reset_contacts: Bool)` selects the contact-reset
variant when true; false selects the same native kernel as the zero-argument
form. Both return Void. The Bool is an expression evaluated once. Numeric
arguments are rejected.

```text
on collision:
    item.bounce_to_ground(true)
    return false
on stage_landed:
    item.change_state_flags(0, 2)
```

The native response owns restitution, stopping thresholds, landing count,
possible destruction, spin and ECB updates. The true variant additionally
resets hitbox contact history before completing the landing. `on stage_landed`
runs only when the native routine settles. Native surface motion may instead
invoke `on entered_air`. `item.align_to_floor()` is the existing separate
orientation primitive; authored code chooses when to call it.

Hooks use the actual owned Item program and shared synchronous VM. The callback
ABI has no userdata, so the backend temporarily links its retained invocation
on the C stack and restores the previous scope after the native call. Nested
hook invocations allocate no callback record. Physical identity, program and
storage are checked before hook dispatch and publication. A failure does not
undo completed collision, sound, particle, damage or callback side effects.

Native collision geometry, Item attributes and imported model graphs remain
engine prerequisites. The terrain kernels can perform native sound/effect
operations and allocations. This binding does not promise arbitrary stage
callback containment, arbitrary graph replacement safety or allocation-free
native collision responses. The focused fixture uses silent controlled leaves;
in-game appearance and behavior are outside that proof.

`item.set_collision_facing(direction: Int)` writes the collision actor's native
signed 32-bit facing value. It accepts the full Int range and does not change the
Item's separate visual facing. `item.reset_default_ecb()` copies the Item's
default ECB into its working ECB, installs a fixed collision source scaled by
the Item's instance scale, and selects collision facing from visual facing.
Both return Void and require the Item's own collision actor. They dispatch no
authored hook. `item.refresh_physics()` performs a separate native operation.

`item.clear_animation_commands()` clears only the native animation command
stream pointer. It leaves command variables, loop count, timer, animation
descriptor and model controllers unchanged. It returns Void.
