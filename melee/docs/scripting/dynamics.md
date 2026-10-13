# Dynamic bone policies

Dynamic bones use the shared native solver for secondary motion such as hair,
skirts and tails. Fighter scripts choose policies at its existing decision
boundaries; they do not replace the integration algorithm.

## `on dynamics_interaction_start`

Returns an **Int** threshold for the current dynamic-bone set. `event.set` is
the zero-based set index; `event.set_count` is the total number of sets for
this update. The solver enables interaction for entries whose index is at
least this threshold. This is a threshold, not a joint index: negative and
large signed values are preserved without clamping.

```text
on dynamics_interaction_start:
    if event.set == event.set_count - 1:
        return 0
    return attrs.first_interacting_dynamic_bone
```

The attribute name belongs to your fighter. Missing handlers use zero. The
engine requests a decision once per set, after collision/wind reset work and
before integrating that set. A fault, retirement or replacement of the actor,
program, root or captured dynamics graph stops the remaining update. On
Flatzone, a temporary root-scale adjustment is restored after a live script
fault; a retired or replaced root is never written through.

## `on dynamics_ground_collision`

Returns **Bool** to enable floor collision for dynamic bones in this update.
It runs before the per-set solver work and has no arguments. A declared hook
owns this decision even when it returns false; reaching its end without a
return also means false. It can use the fighter's own attributes and state
rather than relying on a stock fighter kind.

An absent hook retains the generic normal-size, grounded policy. As with
`dynamics_interaction_start`, a fault or invalidated actor/program/root/graph
stops the remaining update. This is secondary-bone floor collision, not the
fighter's own stage collision or grounded/airborne status.

## Current boundary

The interpreter still has no authored wind decision: `dynamics_wind` remains
unmigrated. Source fighters never borrow stock Mewtwo/Koopa policies or a native
Definition callback. Declare the generic ground hook for a custom ground
policy; do not rely on a donor fighter's special case.

These scheduler checks do not establish solver numerical parity, rig import
correctness or whole-match rollback performance.
