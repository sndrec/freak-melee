# Package-authored holdable items

This page covers holdable pickups declared with `item NAME ...`. For the
separate `article KIND ...` declaration used by stock-style fighter
projectiles and attached effects, start with the linked
[fighter scripting reference](scripting/index.md),
[article declaration](scripting/article.md), and [Item API](scripting/item.md).

A fighter script may declare up to 24 holdable item types. The declarations are
content, not engine-specific handlers for any particular fighter. Their order
is their zero-based item index within the fighter package.

```text
item Example damage 5 angle 45 lifetime 180:
    icon items/example.png
    billboard_size 8
    hitbox_size 4
    knockback_growth 70
    base_knockback 25
    on thrown:
        velocity.x = facing * 2
        velocity.y = velocity.y + 1
    on tick:
        velocity.x = velocity.x * 0.99
    on ground_collision:
        if item.ground_contact: item.land()
        return false
    on hit:
        item.bounce()
        return false
```

`items.pull(index)` spawns a holdable item in the fighter's hand and returns
whether it succeeded. `items.held` is true while the fighter already holds an
item. Use these in conditions to avoid spending a charge when the item limit
prevents a spawn. The normal pickup, drop, and throw controls handle it once
spawned. `fighter.self_damage(amount)` is available for moves with a self-cost.

`icon` names a PNG relative to the draft project directory. `script-build`
embeds it in the fighter module, so the installed package includes the image.
The client decodes it when the fighter loads and draws a camera-facing quad
for that item. `billboard_size` is its world-space width and height; it does
not determine collision size. `hitbox_size`, `knockback_growth`, and
`base_knockback` configure the thrown item's hitbox. Those fields default to
4, 70, and 25. An item without an icon continues to show the capsule.

Item callbacks can read `velocity.x/y/z`, `position.x/y`, `facing`, `item.age`,
`item.grounded`, `item.airborne`, `item.ground_contact`, and `item.state0` through
`item.state7`. They can assign velocity and state fields. Expressions support
arithmetic, comparisons, booleans, `sin`, `cos`, `abs`, `min`, and `max`. The
general item operations are `item.gravity()`, `item.land()`, `item.bounce()`,
and `item.spawn_food()`. A ground-collision or hit callback returns `true` to
destroy the item and `false` to retain it. Without a callback, thrown items
use ordinary falling physics, land after ground contact, and bounce on hit.

Mutable item age, collision flags, and eight state floats live in the Melee
item instance and are part of rollback state. Callback code and definitions
are immutable while a match runs. Drawing does not advance item behavior.
The initial visuals are billboards. A later model format can replace them
without changing the item behavior callbacks.

## Fighter-owned article callbacks

The [article reference](scripting/article.md) documents current authored
projectile setup, including named DAT attributes, literal launch values, and
automatic lifetime selection. The [Item API](scripting/item.md) gives
signatures and examples for article operations.

An existing article kind from the fighter's DAT can have module-owned behavior
instead of inheriting its stock item logic. Declare every reachable item state
and its animation index, then author the needed state and interaction callbacks:

```text
article It_Kind_Mario_Fire states 2:
    state 0 anim -1:
        on animation:
            item.life = item.life - 1
            if item.life <= 0: item.change_state(1)
            return false
        on physics:
            velocity.x = facing * 3
        on collision:
            return false
    state 1 anim 2:
        on animation:
            return true
    on spawned:
            item.set_lifetime(45)
    on reflected:
            return item.reflect()
```

State callbacks are `animation`, `physics`, and `collision`. Item-wide callbacks
are `spawned`, `destroyed`, `picked_up`, `dropped`, `thrown`, `damage_dealt`,
`damage_received`, `entered_air`, `reflected`, `clanked`, `absorbed`,
`shield_bounced`, `hit_shield`, and `interaction`. Predicate callbacks return
whether the item should be removed. `item.change_state(index)` accepts only a
declared state index. `item.set_lifetime(frames)` sets both the live and half-life
timers; `item.life` reads or changes the live timer. `item.reflect()` applies
the engine's ordinary item-reflection behavior and returns its result.

These operations cover only a part of the stock item behavior vocabulary. An
article should not replace its stock table until all its reachable state and
interaction paths have been translated and compared in gameplay.
