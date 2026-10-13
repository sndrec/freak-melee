`absorber` accesses the Fighter's native absorption volume and accumulated
absorption result. It does not choose a move, heal the fighter, or implement a
fighter-specific charge policy.

| Source operation | Type | Native state |
|---|---|---|
| `absorber.create(raw_bone, x, y, z, radius)` | `Void(Int, Float, Float, Float, Float)` | `ftColl_CreateAbsorbHit` |
| `absorber.active` | Bool, read/write | `x2218_b6` |
| `absorber.hit` | Bool, read/write | `x2218_b7` |
| `absorber.skip_position_update` | Bool, read/write | `absorb_hit.skip_update_pos` |
| `absorber.damage` | Int, read-only | `AbsorbAttr.x1A44_damageTaken` |
| `absorber.hits` | Int, read-only | `AbsorbAttr.x1A48_hitsTaken` |
| `absorber.direction` | Float, read-only | `AbsorbAttr.x1A40_absorbHitDirection` |

Creation uses an actual raw index into the source-owned imported part table,
separate from the semantic `Part` mapping. It enables the volume, clears `hit`,
and copies its joint, offset and radius. It leaves accumulated damage/hits and
`skip_position_update` unchanged. Negative or out-of-span indices retain the
reference language's healthy no-op. A missing joint inside the admitted span,
malformed rig, or different selected source assets is an error before mutation.
The native helper copies Float values without a radius clamp or normalization.

Scalar access requires the retained physical Fighter identity and, for an
interpreted recipient, its exact live program/storage. It does not require the
recipient to share the caller's bank or have an imported source rig. Thus native
properties on a retained foreign Fighter can be used through a typed receiver;
creation still requires that recipient's own selected source assets. Direct
Bool writes reject noncanonical payloads. Errors leave result outputs unchanged.

Fighter `on absorb` is a zero-argument Void hook, distinct from Item
`on absorbed`. The actual collision code sets direction to -1 or +1, records
the absorbing Fighter on the Item, and accumulates native signed integer damage
and hit counts when the hitbox permits it. `Fighter_ProcessHit_8006D1EC` calls
the source hook at the existing absorption branch, before the later native
direction/damage/hit-count resets. An attached actor with no hook stays neutral
and does not call a stock authored handler. A callback fault, retirement, or
changed retained identity stops the native continuation before those resets.

Policies remain authored, for example:

```text
action Absorption:
    on absorb:
        state.charge += absorber.hits
        state.accumulated_damage += absorber.damage
```

The existing `damage.heal(amount)` uses `nativeFighterDamage_Heal`: subtract the
Float amount, clamp the resulting percent at zero, then update native player HP
and healing statistics in their original order. The absorber backend adds no
healing policy. Negative amounts retain the native arithmetic; callers choose
any integer rounding their source policy needs.

Absorber fields and joint links belong to the native Fighter peer. Packed
interpreted actor snapshots alone restore authored state, not this native
volume; rollback must restore the Fighter, animation graph and scheduler peers
coherently. The focused `tests/native/script_absorber` fixture exercises actual
source preparation/VM/actor ownership and native field operations with controlled
asset, position-write, HP/statistics and unrelated engine leaves. It does not
claim complete collision, rendering or in-game absorption acceptance.
