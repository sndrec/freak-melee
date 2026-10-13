# Direct interpreter item and article self bindings

This family resolves through the common typed native catalog and executes in
`native_script_binding_item.c`. It operates on the current live Item source,
checked by full creation generation. It uses typed engine fields and generic
native primitives, with no authored C callbacks, legacy host adapter, native
offsets in the program or stock fighter-special dispatch.

## Native fields

Existing `position.x/y/z`, `velocity.x/y/z` and `facing` retain their Item
storage. The following `item` members have native scalar types:

| Type/access | Members |
| --- | --- |
| Float read/write | `life`, `half_life`, `animation_rate`, `spin_speed` |
| UInt32 read-only | `creation_id` |
| UInt32 read/write | `command0`, `command1`, `command2`, `command3`, `collision_group` |
| Int read-only | `kind`, `motion_state`, `age`, `accumulated_damage`, `damage_received` |
| Int read/write | `collision_mode`, `exit_category` |
| Float read-only | `animation_frame`, `base_spin_speed`, `reflection_multiplier`, `drop_speed`, `base_scale`, `scale`, `throw_scale`, `damage_direction`, `surface_restitution` |
| Bool read-only | `grounded`, `airborne`, `ground_contact` |
| Bool read/write | `physics_disabled`, `custom_floor_motion`, `remove_at_bounds`, `suppress_destruction_effects`, `world_interaction`, `owner_bound`, `reverse_spin`, `throw_speed_damage`, `damage_hitlag`, `contact_hitlag`, `harm_owner`, `harm_teammates`, `harm_sibling_items`, `can_be_hit_by_owner`, `can_be_hit_by_teammates`, `can_be_hit_by_sibling_items`, `subaction_sounds_muted`, `stop_on_surface`, `dynamic_ecb` |

UInt32 values retain all 32 bits, including high-bit creation IDs and collision
groups. Command slots use the actual native unsigned type; they do not adopt
the fighter command API's separate signed Int view. Normal Int-to-UInt32
conversion preserves low bits modulo 2^32. Exit category preserves the native
four-bit truncation. Float fields use binary32 stores; native attributes must
be present before attribute-backed reads or physics calls.

`item.state` and `item.attr` remain declaration namespaces. Authored state and
immutable imported parameters use the typed storage/metadata architecture;
this field family never aliases those declarations into native offsets.

## Holdable storage exception

Rook's custom snack items use the explicitly tagged `It_Kind_NativeHoldable`
layout. Only that kind may read `age` or `ground_contact`; articles fail those
queries rather than interpreting unrelated union bytes.

Age is the native unsigned 16-bit counter, promoted exactly to Int.
Ground contact reads the cached holdable collision-mask bits 0xE. For this
holdable schema, grounded means native motion state 1 and airborne means state
2 or 3. Other articles use the engine's `ground_or_air` classification.
Reading this schema does not authorize calling its old authored native hooks;
interpreted item scheduler integration remains separate work.

The holdable's existing age-based lifetime and an article's `life` timer are
distinct. Writing `life` does not silently rewrite an immutable holdable
lifetime declaration or its age counter.

## Generic physics and lifetime

| Call | Result and behavior |
| --- | --- |
| `item.gravity()` | Void; actual native falling physics plus spin setup. |
| `item.gravity(setup_spin: Bool)` | Void; false applies only falling physics; true retains normal spin setup. |
| `item.set_lifetime(lifetime: Float)` | Void; sets life and computes half-life with the native common-data factor. |
| `item.expire_tick()` | Bool; decrements life first, then returns whether life <= 0. |
| `item.reflect()` | Bool; native XY velocity/reflection multiplier, facing flip and half-life restoration; returns false. |
| `item.shield_bounce()` | Bool; mirrors native XY velocity against the shield normal, updates facing/collision facing and returns false. |
| `item.bounce_stage()` | Bool; actual native response to four surface normals, moving-platform velocities and restitution. |
| `item.stage_collision()` | Void; runs the native stage query/integration; its underlying helper return is discarded as in the source call. |
| `item.mirror_velocity()` | Void; mirrors against the item's native reflection normal. |
| `item.mirror_velocity(x: Float, y: Float, z: Float)` | Void; supplies the normal explicitly. The native mirror operation uses X/Y. |
| `item.pause()` / `item.resume()` | Void; preserve native pause/resume flag semantics. |
| `item.use_manual_lifetime()` | Void; preserves the legacy mapping that clears native world-interaction bit x15. It does not itself replace the outer scheduler's expiry policy. |

`gravity(false)` intentionally omits **spin setup**, not a friction or reflection
stage. Lifetime Float calculations and scalar conversions retain native
precision and operation order. `expire_tick` differs from the engine's separate
pre-decrement `Item_TickLifetime` inline; this binding uses the exact helper
selected by the authored source contract.

Reflection does not transfer damage ownership or resolve another actor. Those
steps belong to the outer interaction/lifecycle pipeline, outside this binding.
Stage collision and bounce use general native primitives, not an article-kind
or fighter-special handler.

## Item sound

```text
item.play_sound(sound_id: Int, volume: Int, pan: Int)
item.loop_sound(sound_id: Int, volume: Int, pan: Int)
```

Both return Void. IDs must be 0..0xFFFFFF and byte controls 0..255. Invalid
inputs fail before native audio calls. Argument order is preserved: the native
Item helper's misleading parameter names are not used to reorder authored
values. The first byte reaches audio volume and the second reaches audio pan.
Native stop/no-op sentinels, loop-handle replacement and playback handles
remain in the actual Item sound bodies.

## Explicit pending boundaries

This section records the earlier Item-self fixture boundary. The connected
entity suite now supplies state changes, attachments, retained recipients,
owned removal and factories in separate runtime units. See
`native_script_item_actor_ownership.md`, `native_script_item_scheduler.md`,
`native_script_holdable_protocol.md` and the source-package loading checkpoint.
The isolated Item-self fixture below still intentionally excludes those units.
Model/effect owner scheduling and ordinary package activation remain open.

Item/article state changes, attachment activation/queries, creator/current-owner
reads and writes, recipient/function calls, interaction recipients, removal,
spawn/food lifecycle, model/effect receivers and effect spawning remain
unsupported in this family. These need owned lifetime, metadata and interpreter
scheduler integration. No neutral placeholder or stock callback fallback is
provided. This includes cape ownership/attachment behavior even though its
command fields are supported here.

Every public binding checks source class/generation before accessing native
storage and checks again before result publication. There is no allocation
during these reads, writes or operations. This family implements self operations
and does not claim complete live Mario/Cape/Rook item closure.

## Source-backed fixture

`tests/native/script_item/run.py` builds the production source index, expression
and statement compiler, catalog, bytecode and VM, plus mechanically extracted
actual item fall/spin/lifetime/reflect/shield/stage-bounce/integration/pause/sound
and vector bodies. Scene search, moving-platform velocity, audio-device and
dynamic-ECB observation leaves are controlled. Bulk comparisons include
four-surface combinations, Float marshalling, unsigned identities, pause flags,
holdable schema rejection and real fireball/Rook source-to-VM behavior.

The focused fixture passes 10,966 checks: 2,688 native physics/body comparisons,
96 Item sound comparisons and 128 authored source-to-VM ticks. It also checks
read-only expiry rejection, pure unsigned identity reads, source-context and
generation mismatch rejection, and explicit rejection of pending state,
attachment and food-spawn operations.

These are isolated native contract checks. They do not prove live scene/audio
parity, item callback scheduling, creation/destruction, cross-owner behavior or
aggregate item rollback integration.
