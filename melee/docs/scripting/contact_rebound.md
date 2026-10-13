# Body contact and rebound decisions

## collision.contact_rebound_allowed() -> Bool

Queries whether the fighter recorded by the caller's current body contact
permits rebound. This method operates on the calling fighter; it is not a
general overlap query and does not choose a new target.

With no recorded contact or no recipient, it returns true. For an interpreted
recipient it runs that recipient's `on body_rebound:` callback. An absent
callback returns false. It never substitutes the recipient's original stock
special handler for a missing script. Ordinary native fighters retain their
native rebound policy.

```text
if collision.contact_rebound_allowed():
    state.rebound = true
```

Define your own `state.rebound` variable and decide what the result should do
in your move. This query does not itself apply velocity or change motion.

## on body_rebound

A zero-argument callback returning Bool, owned by the contacted fighter's
script. Use it to decide which of your states permit an incoming fighter's
rebound behavior.

```text
state bool rebound_enabled = false

action ContactPolicy:
    on body_rebound:
        return state.rebound_enabled
```

The interpreter validates the caller and recipient, their selected programs,
and the original contact relation after either result. A retired actor,
changed relation or script failure ends the operation; it is not published as
an ordinary false result. The bounded interpreter also rejects exhausted
nested calls rather than continuing with an invented decision.

See [synchronous callbacks](synchronous_callbacks.md) for continuation rules and
[Fighter hitboxes](fighter_hitboxes.md) for the separate attack-volume API.
