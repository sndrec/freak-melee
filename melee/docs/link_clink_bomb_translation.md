# Link and Young Link Bomb Pull translation

The accepted fighter/article translation is registered in
`examples/link_clink_wip.fms`. Its source of truth is
`ftLink/ftlinkspeciallw.c`, `it/kinds/itlinkbomb.c`, and the ordinary item
operations those files call. Both fighters share the authored decisions.

## Owned data and creation

| Field | Link | Young Link |
| --- | --- | --- |
| Ground / air motion | 358 / 359 | 358 / 359 |
| Ground / air common throw motion | 108 / 112 | 108 / 112 |
| Fighter article slot | 0 | 0 |
| Article kind import | Fighter attribute `0x48` | Fighter attribute `0x48` |
| Raw holding-joint import | 64 | 68 |
| Article name | `It_Kind_Link_Bomb` | `It_Kind_CLink_Bomb` |

The holding joint comes from `ft_data->x8->x10`, independently of the thumb
used for spawn history. The shared template's unchanged literal 64 must
import 68 for Young Link; customized literals remain customized. Article
attributes 0 and 12 are unsigned integer lifetime/fast-fuse thresholds;
attribute 16 is signed integer detonation damage. Offsets 20 through 48
contain floating point hop, bounce, impact-speed and rolling parameters.
Owned import must preserve these types, rather than reinterpret integers as
floating point. Arithmetic subtracts the unsigned lifetime operands before conversion,
matching the stock source.

The local USA v1.02 ISO gives both articles lifetime 300, fast-fuse threshold
96 and damage threshold 6. Float offsets 20 through 48 are respectively
0.67, 0.60, 0.26, 0.67, 0.80, 0.70, -0.03 and 0.03. These are import evidence,
not replacement literals in authored behavior.

The stock constructor creates an item at the fighter's current ECB center,
using the prior left-thumb position as `SpawnItem.prev_pos`. Velocity, damage
and hit-group are zero; both parent pointers refer to the fighter and initial
collision is enabled. It then initializes the fuse, saved creator, hop flag,
scale-derived fuse steps and muted subaction sounds. Only after initialization
does `Item_8026AB54` attach it and invoke the authored pickup callback. Pickup
changes to state 0 and unmutes sounds. The fighter assigns `item_gobj` after
construction, displays visibility group 2 even if creation fails, and sends
the ordinary pickup notification only on successful creation.

The new generic held constructor must preserve this ordering. The existing
presentation attachment constructor calls `on_created` after pickup and is
not suitable unchanged. No initialization may read or write the stock Link
item union.

## Complete authored decisions

Bomb Pull first examines the ordinary held item. A Link or Young Link Bomb
enters the common throw motion; another held item prevents the pull. With no
held item it resets throw cues, enters 358/359, starts animation and installs
the spawn accessory. The accessory consumes cue 0. Ground/air transitions
preserve frame with flags `0x0C4C5080` and reinstall that accessory. Animation
completion, ordinary friction and ordinary air physics remain generic.

| Article state | Animation | Behavior |
| --- | --- | --- |
| 0 held | 0 | Restart held animation as needed; tick fuse |
| 1 falling | 0 | Gravity without rotational update; E15C bounce/landing |
| 2 thrown | 1 | Falling physics; swept impact detonation or rolling |
| 3 dropped | 1 | Two intentional animation queries; same impact collision |
| 4 rolling | 0 | Signed acceleration and stop threshold; follow floor |
| 5 exploding | 2 | Expiry callback, no motion or collision response |
| 6 entered air | 0 | Ordinary slide/contact callbacks; tick fuse |

Every ordinary state change preserves dynamic joint 3's translation Y and
rotation X. Before the fast fuse it uses `ITEM_ANIM_UPDATE`; afterward it
uses `ITEM_CMD_UPDATE`. Throw/drop also preserve their own drop/hit flags.
Fast fuse chooses animation descriptor 3 independently of logical state,
loads it once and advances it once. Timer expiry is checked before decrement.
Only the slow fuse advances its scale-derived dynamic joint components.

Thrown/drop collision saves velocity before `it_8026DAA8`, tests flags `0xF`,
and chooses detonation from either absolute impact speed. A non-detonating
floor hit sets ground state, resets the default ECB and enters rolling.
Rolling direction is the sign of velocity, including positive for zero;
the stock `MUST_MATCH` impossible OR condition has no reachable alternate.
Damage received checks accumulated event damage against the integer threshold.
The one-time hop preserves the stock signed X comparison, random-call count,
facing update and facing-scaled Y velocity.

Explosion enables outgoing owner, teammate and sibling-item collisions. A
bound item releases with zero velocity, becomes intangible and restores
current/persistent owner visibility only when the current owner is its saved
creator. It then disables automatic world interaction, stops velocity, hides
the model, disables physics, starts expiry, remains intangible, emits effect
1038 and sound `(116,127,64)`, clears contact hitlag, changes to state 5 while
preserving the fuse joint, and emits wind `(120,1,0.02,1.0471976)` at its position.
Sound muting is not cleared by this explosion path.

Pickup, drop, throw, damage, entered-air, reflection, shield contacts,
interaction, floor transitions and bounds callbacks are all included in the
draft. Interaction clears the saved creator only when it equals `other`,
then calls the ordinary interaction cleanup. Pickup/reflection never replace
that saved creator.

## Minimal generic API contracts

Root owns central integration. These names are ordinary fighter/article
operations; all gameplay branches, numbers and state decisions remain authored.

| Operation | Exact engine contract |
| --- | --- |
| `items.held_kind` | `itGetKind(fighter->item_gobj)` when held |
| `items.throw_held(motion)` | Common `ftCo_800957F4`, without stock special dispatch |
| `articles.create_held_at_center(...)` | Spawn setup above; authored created, then `Item_8026AB54`/pickup, then held registration; return live item number |
| `item.dynamic_joint(index).translation.y` / `.rotation.x` | Dynamic-bone table index, not model preorder path; checked reads and HSD setters |
| `item.set_animation_descriptor(index)` | Choose article descriptor and `Item_80268D34`; preserve logical state, commands and lifetime |
| `item.advance_model_animation()` | `HSD_JObjAnimAll` only; preserve stored item frame and commands, unlike `item.advance_animation()` / `Item_802694CC` |
| `item.sweep_stage_flags()` | `it_8026DAA8`: update collision/current position/floor and return its complete flags |
| `item.grounded = true` | `it_802762B0` |
| `item.reset_default_ecb()` | `it_80275D5C(gobj,&item->xC0C)` |
| `item.bounce_to_ground(1)` | `it_8026E15C` with authored landed callback; existing no-argument E248 remains a separate flavor |
| `item.set_collision_facing(direction)` | `mpCollSetFacingDir(&item->x378_itemColl,direction)` |
| `item.damage_received`, `damage_direction`, `scale` | Read `xCA0`, `xCCC_incDamageDirection`, `scl` |
| `item.subaction_sounds_muted` | Read/write `xDCD_flag.b2`; ordinary pickup/drop/throw/subaction sound suppression |
| `item.attachment_owner_is(other)` | Compare live revocable attachment owner to the captured interaction argument |
| `item.attachment_active` | Live current damage owner equals revocable attachment control |
| `item.wind_at_position(...)` | Existing `spawn_wind` host operation with `item->pos` |

Existing `collision_mode = 2` corresponds to `it_802756D0`, which sets the
hurtbox state; it does not remove effects. Existing `contact_hitlag = false`
corresponds to `it_80274C60`, which clears `xDC8.flags.xC`; it is not the
subaction-sound mute flag. Existing `visuals.set_current_group(group,value)`
calls `ftParts_80074B0C` with bounds checks; `visuals.set_group` calls
`ftParts_80074A4C`. Existing clear-attachment-owner and owner-call operations
suffice for saved-creator cleanup and visibility restoration.

Collision permissions have independent outgoing/received meanings:

| Authored property | Field | Meaning from `ithitbox.c` and `itcoll.c` |
| --- | --- | --- |
| `harm_owner` | `xDCD.b5` | Allow outgoing collision with owner |
| `harm_teammates` | `xDCD.b6` | Allow outgoing teammate collision |
| `harm_sibling_items` | `xDCD.b7` | Allow outgoing same-owner item collision |
| `can_be_hit_by_owner` | `xDCE.b0` | Allow owner's fighter/thrown hitboxes |
| `can_be_hit_by_teammates` | `xDCE.b1` | Allow incoming teammate hitboxes/items |
| `can_be_hit_by_sibling_items` | `xDCE.b2` | Allow incoming same-owner item hitboxes |

Throw/drop set outgoing sibling collision and incoming sibling collision
(`it_80275414` / `it_802754A4`). They do not set incoming owner collision.

The bounds hook is a separate lifecycle contract, not a physics operation.
`it_8026C368` dispatches on collision flag `x34_flags.b7` and currently chooses
stock callbacks by item kind. Add a void `on out_of_bounds` notification before
that switch. When an authored callback exists, its presence suppresses the
entire stock switch, even if the authored callback chooses no action. Bomb's
script chooses explosion only when not owner-bound and not already state 5.
It must never fall through to `it_8029D968` and the stock Bomb item union.

## Integration and validation boundary

Stock import now maps Link/Young Link Bomb slot 0 and imports unchanged
holding-joint defaults 64/68. Generic metadata uses an explicit
`kind_attribute_offset` of `0x48` in both article resolvers. The fighter's
spawn kind independently imports from that source attribute into owned data.
The current script format names each article kind literally, so the eventual
shared template needs both equivalent article declarations. Parameter import
retains only the donor family's block; maintain them from one audited body.

The canonical draft generates and compiles against the integrated generic
operations. `tests/native/fighter_link_bomb_script_test.c` passes 256 direct
comparisons with the actual stock `ftlinkspeciallw.c` and `itlinkbomb.c`.
The harness compares ordered calls, resulting item/fighter state, dynamic
fuse transforms, flags and callback results. Coverage includes constructor
ordering, failed allocation, held-item branches, consumed creation cues,
fighter ground/air continuation, all seven article states, both fuse phases,
animation restart, saved impact velocity and both detonation thresholds,
rolling direction/acceleration/stop, signed/random hops and repeat damage,
reflection, shields, ground contacts, bounds, transferred owners and creator
cleanup. Common engine behavior is stubbed consistently on both sides;
this check does not establish live collision, effects or asset parity.

Both full-owned packages compile and bundle under
`.local/link-bomb-owned-20260930-repaired`, with holding-joint assertions 64/68 and
Spin endpoint assertions 24/26. Generated C reads neither fighter DAT
attributes nor article special attributes. The repaired batch includes the
separate article-owner state packing fix. Bomb does not itself read `owner.state`.

The first two 750-sample live cases (ground/right, both fighters) failed at
sample 426, exactly when the fuse switched to descriptor 3. Fighter state,
item motion/relations, pose and visuals matched; item animation state differed.
The draft used `item.advance_animation()`, which calls `Item_802694CC` and
updates stored frame/commands. Stock only calls `HSD_JObjAnimAll`. The first
comparison stub had incorrectly modeled both as model-only advancement; its
254 passes therefore did not establish this distinction. The strengthened
stub now reproduces the mismatch and compares stored frame/descriptor too.
The draft now uses the integrated `item.advance_model_animation()` operation.
The strengthened 256 comparisons pass against that implementation, including
a precision-edge lifetime/threshold pair. Both
packages and matching runtime/header snapshots are rebuilt under
`.local/link-bomb-owned-20260930-final` and
`.local/link-bomb-toolchain-20260930-model`. Failed logs/results remain in the
earlier repaired batch. The corrected full matrix passes all sixteen cases.

`tests/native/link_clink_bomb_parity.py` prepares and compares ground, air,
throw and drop trajectories for both fighters and facings. It uses hidden,
accelerated matches with normal drawing and checks actual article states
before claiming coverage. All sixteen cases pass with 750 controller samples,
1498 fighter states and 749 RNG states per case, plus exact action, damage,
article state, relations, pose, visibility, hitbox and hurtbox traces.
Ground/air held-fuse cases include fast fuse and held explosions; throws and
airborne neutral-Z drops include their requested article states and explosion.

The first corrected trajectory batch had exact traces for all sixteen cases,
but six missed branch coverage: left throws exited before explosion and
grounded Z selected a throw. Its `results-first-trajectories.json` is retained.
Only fixture positions changed: left throws start at x55 to travel toward the
floor; drops are placed airborne at sample269 before neutral Z. The six reruns
pass, and `acceptance-results.json` combines the sixteen unique accepted cases.
No source/module change was needed for those trajectory repairs.

Accepted package hashes are
`ebc03a2cb9ed71ea98d6be9e2281d6d6342cb9f2372dd9e1e64edb9391e6c8b5`
(Link) and
`1d8c4e8bcd4e6245ec7c0990b65f4b0ed5ea657d536ba37545dbc6b35adce6f8`
(Young Link). The live matrix exercises the ordinary held/throw/drop lifecycle;
the direct harness covers reflection, shields, rolling and owner transfer.
Full-fighter parity and an authored Bow/Arrow/Hookshot remain outside this batch.
