# Paired fighters

[Alternate forms](forms.md) | [Recipient calls](functions.md) | [Movement](movement.md) | [CPU policies](cpu_policies.md)

A paired package owns two independently authored members. These operations
resolve the current live member and share ordinary engine state; the script
still decides when to link, move, release or switch. No operation depends on a
stock fighter name. They are fighter callbacks, not article or model-effect
callbacks.

## Member identity

The interpreter resolves immutable package members across both published Player
roles. Either interpreted member can query both members after a form swap,
including a dormant alternate actor; role permutation does not change member
indices. Simultaneous-companion packages now prepare and activate both owned
members. The [activation checkpoint](../native_source_companion_activation_checkpoint.md)
records a full Popo/Nana package running at four ports with eight live actors;
this is startup evidence, not complete linked-special or rollback acceptance. See
[Family queries](family_queries.md) for the retained identity contract and
connected verification scope.
The [actual special comparison](../native_source_companion_specials_checkpoint.md)
also matches both Popo and Nana throughout four grounded special scenarios,
including rope scheduling. Interrupted moves, partner death and full match-end
acceptance remain separate.

`family.member_id(index) -> int`

Return the complete creation ID of live member `0` or `1`, or zero when the
member is absent. The index identifies the package member, not the player's
current active/dormant role. Switching forms swaps those roles without changing
member identity. Both source and destination must belong to the registered,
owned family. An unrelated fighter with the same player number is insufficient.

Keep IDs in `int` variables. High-bit IDs may look negative; that does not make
them invalid. Passing them to these operations preserves all 32 bits. Do not
convert an ID through `float`.

## Hitlag links

| Member | Type | Meaning |
| --- | --- | --- |
| `hitlag.link(id)` | `bool` | Point this fighter's shared hitlag link at the live target. Zero clears it; a missing target or self-link is rejected without changing the existing link. |
| `hitlag.unlink()` | method | Clear this fighter's outgoing link. |
| `hitlag.linked_id` | read-only `int` | Current live endpoint ID, or zero if the retained endpoint has retired or been recycled. |
| `hitlag.active` | read-only `bool` | Current shared hitlag state. |
| `hitlag.request_release()` | method | Request release through the shared engine's existing hitlag flag operation. |
| `fighter.incapacitated` | read-only `bool` | Current shared incapacitated flag. |

A link is directional. If a behavior requires a reciprocal pair, call the
recipient's authored helper to link it back explicitly. Clearing the source
does not automatically clear the endpoint. Engine links carry creation stamps;
shared recursive entry/exit ignores stale endpoints instead of following an
address that now belongs to a different fighter.

## Collision and entry state

`collision.copy_from(id) -> bool`

Copy the target's collision history into this fighter using the shared
`mpCopyCollData` mode 2 operation. This is the same history operation used by
common linked motion entry. It does not copy the entire fighter, rerun collision,
or replace arbitrary script state. Missing targets return false without change.

`airborne` is a readable and writable `bool`. Assignment publishes only the
ground/air classification. Use normal movement transition operations when you
also need their velocity, collision and jump bookkeeping. Paired entries can
already have performed that work before entering their motion; publishing the
classification afterwards avoids performing it twice.

`spawn.horizontal_offset` is a readable and writable `float`. It is the shared
spawn/rebirth horizontal positioning offset, multiplied by instance X scale
when the common spawn code reads it. It is not an air-speed multiplier or
capture-spacing parameter.

## Target bone position

`targets.read_bone_position(id, part, x, y, z) -> bool`

Read one current world-space joint position. `id` is a complete creation ID and
`part` is an integer raw imported skeleton index. The three outputs must be
distinct declared `float` locals or scalar fighter-state fields declared as `float`. Use the actual
package rig; stock indices are not portable between unrelated skeletons.

The operation validates the live target, part count and joint before evaluating
the position. It rechecks target and caller identity afterwards. An absent target
or bone returns false and leaves all outputs unchanged. A changed retained
actor, program or rig faults the current callback before publishing outputs.
The operation makes one vector query, not three separately sampled axis queries.
See [recipient bone queries](target_bone_queries.md) for native matrix evaluation
and imported rig requirements. Fighter and Article callbacks may use it.

```text
let float x = 0
let float y = 0
let float z = 0
let int peer = family.member_id(1)
if targets.read_bone_position(peer, 2, x, y, z):
    position.x = x
    position.y = y
    position.z = z
```

The package/runtime foundation and these primitives do not by themselves
complete a stock pair translation. Partner setup, articles, follower policy,
cleanup and move timing remain responsibilities of its authored behavior.
