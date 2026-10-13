# Peach remaining direct-source closures

Read-only audit of `examples/peach_full.fms`, the live typed catalog and native/reference implementations, 2026-10-08 after `24b898f1d`. No source compilation, asset preparation or gameplay acceptance is claimed. This is a whole-script static pass: property writability, overloads, callback preparation and runtime contracts were checked separately from name presence. The starting census diagnostic was the assignment to `controls.interrupt_allowed` at line 347; root reports that exact write has now landed. `item.track_accessory`/`clear_accessory` and `state.catch_ledge`/helper lifetime barriers are separately in flight.

## Small exact source migrations

These need call-site changes, rather than additional aliases:

| Existing Peach surface | Current direct equivalent | Evidence/qualification |
| --- | --- | --- |
| `physics.switch_to_air()` (847,859) | `physics.ground_to_air()` | Reference code maps both to `ground_to_air`; native module maps that to `ftCommon_8007D5D4`. This sets jumps used to one, not the force-air/all-jumps policy. |
| `controls.aerial_attack_actions()` (1052) | `controls.try_aerial_jump()` | The old module assigns this misleading name to `ftCo_800CB870`. Do not substitute the full `controls.aerial_jump_actions()` IASA policy. |
| `callbacks.facing_changed(helper)` (1182,1204) | `callbacks.reflect(helper)` | Both designate `Fighter.x21F8`; current native reflection dispatch is in `ftCo_0C35.c`. |
| `item.reflect_velocity()` (658,971) | `item.reflect()` with discarded Bool result | Both use `it_80273030`; the existing save/restore of lifetime stays authored. |
| Eight-argument `articles.create_at_position` (417) | Twelve-argument `articles.create_with_velocity(Article,x,y,z,0,0,0,facing,false,0,float(alternate),true)` | `native_fighter_script_projectile.c:7` emits zero velocity and default initial-collision `true` for this eight-argument form. The current direct positional form has six arguments; the velocity form now has twelve. Preserve group and creation value. |
| Old `holder.motion` (477,921,929) | `current_owner.motion`, with the existing nullable/retained admission considered | The old Article emitter queries current damage owner, not the physically holding Fighter. Modern `holder` has different semantics. |

Do not migrate `item.set_instance_scale` to `item.set_scale`: `it_80274484` also writes Item `scl` and refreshes collision/physics through `it_80275534`, `it_80274DFC`, `it_80274E44`; the current model setter only sets the JObj scale.

## Fighter fields and input/float policy

| Required operation/property | Native contract and current gap |
| --- | --- |
| `controls.interrupt_allowed = Bool` (347,1013) | The original write is `Fighter.allow_interrupt`. Root has now implemented the guarded write to the existing property; this audit does not independently claim its runtime verification. |
| `input.cstick_attack_edge()` (1020) | Bool `ftCo_800DF478(Fighter*)`. |
| `controls.aerial_attack_motion()` (1011,1022) | I32 `ftCo_AttackAir_GetMsidFromCStick(Fighter*)`. Do not confuse this query with the prepared `aerial_attack_motion` decision hook. |
| `controls.try_air_item_throw()` (1050) | Bool `ftCo_80095328(gobj,NULL)`. |
| `controls.try_air_special()` (1079) | Bool `ftCo_SpecialAir_CheckInput(gobj)`. |
| `state.air_gun()` (1024) | Void `ftCo_800CDDA0(gobj)`. |
| `physics.animation_vertical_velocity_adjusted()` (1093,1103) | Void `ft_800851D0`; existing animation-velocity operations use `ft_800851C0`/`ft_80085134`, so are not equivalent. |
| `collision.air_landing_callback(helper)` (1085,1095,1105,1393) | `ft_800831CC(gobj,ftCo_80096CC8,helper)` ordering. Existing `air_aerial_landing_callback` uses a different normal-aerial landing policy. Existing direct `air_special_landing(Bool,F32)` can express the simple hi landing outcome, but cannot replace the float helper's velocity-dependent choice. A direct helper code-ID collision continuation is needed. |
| `jump.begin(..., animation_driven=true, ...)` (332) | Current catalog accepts the signature, but `native_script_binding_jump.c:68` explicitly rejects this mode. Native animation-driven jump installs physics policy; provide source-owned phase policy before claiming Peach's aerial-jump callback executes. |

Reference/native comparison sources: `tools/native_fighter_script_primitives.c`, `native_fighter_module.c:1080`, `ftPeach/ftpeachfloatattack.c`, `ftpeachfloat.c`, `ftpeachfloatfall.c`, `ftpeachspecialhi.c`, shared `ftCo_AttackAir.c` and `ft_081B.c`. Effectful input/collision branches need caller status/generation barriers even when their Bool result is false.

## Guard, dynamics and body-contact callbacks

* `dynamics_interaction_start` (333) is typed I32(I32 set,I32 count) in the event catalog, but is absent from the actual Fighter hook contract table. `ftdynamics.c:415` still obtains this policy from Definition, otherwise uses stock-kind decisions. Direct source dispatch must precede each `lb_8001044C`, preserve per-set ordering/default policy, and guard continuation.
* `body_rebound` (336) is typed Bool() in the event catalog but absent from the prepared Fighter contracts. Couple its actual recipient dispatch with `collision.contact_rebound_allowed()` (1208): `native_fighter_contact.c` checks collision state and the retained recipient, invokes Definition's decision, then falls back to kind policy. Source actors need their own prepared decision, never that fallback. The original Peach special-S path also contains this decision.
* `guard_hit` (829) is typed Void() but not a prepared static hook. The actual `Fighter.shield_hit_cb` scheduler already has direct typed `callbacks.shield_hit(helper)` support in `fighter.c:3041`; converting this event to an owned helper is a valid route. A new guard setup primitive must preserve/bind that code-ID instead of installing an authored C callback or overwriting it.
* Missing guard volume/state family: `guard.create(joint,x,y,z,radius)` -> `ftColl_8007B1B8` (owned semantic bone, size and offset); `front_only` -> `x221B_b3`; `active` -> `x221B_b0`; melee/article hitlag -> `shield_unk0`/`shield_unk1`; hit-facing read -> `specialn_facing_dir`; `update_position()` -> `shield_hit.skip_update_pos=false`. These are the complete Toad guard surface at 726–844, and should land with its typed callback setup.
* `stats.count_as_thrown_item()` (1162) only sets `x2070.count_thrown_items=true`; expose the genuine generic flag policy, not a new fighter-specific stats shortcut.

The other root events declared in Peach have current prepared contracts, including the recently migrated normal-attack, held-item, taunt and CPU families. Event-catalog signature acceptance alone does not establish scheduler acceptance.

## Common held-item transitions and recipient helpers

* `state.held_open(F32 blend)` (1440) is `ftCo_ItemParasolOpen_Enter`: selects the prepared held-motion decision, saves the previous motion, changes state, starts animation and initializes held timer policy. A raw `state.enter` does not reproduce it.
* The preceding `motion.id = Motion.fall_special` (1439) is an intentional field write used as the held transition's previous-motion input. Current motion ID is read-only. Entering FallSpecial would add a different transition and is not an equivalent rewrite.
* `landing.interrupt_allowed` (1460) reads `mv.co.landing.allow_interrupt`; it is not `Fighter.allow_interrupt`. `movement.special_landing_lag` (1472) reads `x2EC`.
* Old `holder.held_motion`/`holder.held_frame` (488) resolve the damage owner's current `item_gobj`, then that Item's state/frame. Add guarded generic held-item observations; do not substitute this Article's state/frame or modern physical-holder fields.
* `targets.call_bool(current_owner,"restore_held")` (490,530,535,540), and `targets.call(current_owner,"forget_toad")`/`"create_spore"` (900,903,922), use missing named exported-helper dispatch. `native_fighter_events.c:35` returns false/default for absent/foreign exports. Existing `current_owner.helpers.Helper()` can resolve a known owned function, but blindly rewriting would change the foreign-program absence behavior to an error. Specify a load-resolved optional helper admission/delivery contract with retained receiver/program identity; do not restore tick-time name hashing/Definition lookup.

## Article model, lifetime and contact policies

| Required surface | Exact backing / distinction |
| --- | --- |
| `item.begin_expiry()` (437) | `it_8027518C`. |
| `item.remove()` (496,533,538,667,898,901) | `Item_8026A8EC`; removal terminates the source invocation, rather than publishing success and executing later statements. |
| `item.bounce()` (651,655,661) | `itColl_BounceOffVictim`. |
| `item.sweep_stage_flags()` / `respond_stage_mask(mask)` (691–706) | `it_8026DAA8` full collision mask and `it_80276FC4`; current contact Booleans / stage-bounce policy are insufficient. |
| `item.set_instance_scale(F32)` / `inherit_owner_scale()` (646;442,908,914) | `it_80274484` / `it_80274594`, including Item scalar scale and collision/physics refresh. Owner-scale input is the actual current owner. |
| `item.hide_model()` / `show_model()` (430,907–927,950) | `it_8026BB44` / `it_8026BB20` use native whole-model flag helpers. Implement the real traversal contract, not primary-root visibility only. |
| `item.set_model_animation_rate(F32)` (911,913) | `lb_8000BA0C` on the current model; changing `item.animation_rate` only changes the Item frame-speed field. |
| `item.track_accessory()` (500,519,526) | Selects the current declared Article accessory callback into the mutable Item callback slot. No native authored function pointer. Implementation is in flight with a separate worker. |
| `item.creator_is(other)` (672) | The reference emitter actually compares `other` to `native_article.attachment_owner`, despite its name. Preserve attachment identity intent; do not silently compare immutable creator. Current Object `==` compares token identity/generation/kind, so independently retained handles cannot be assumed to compare engine identity without checking/canonicalizing that contract. |
| `item.spawn_effect_at_position(effect,x,y,z)` (431) | Genuine Item world-position effect operation. Existing Fighter-only `effects.spawn_at_position` and Item joint-root effect are different contexts/ABIs. |

Existing direct Item animation advancement, appearance, manual lifetime, hitbox damage, reflected velocity, signals, current/attachment-owner lifecycle and owner debug-display are available. `item.gravity(false)` is the supported one-argument gravity overload, not a gap. Five standard primary-model Article declarations do not require Chain/secondary resources, but their real owned model/DATA/dynamics/effect import validation still must pass.

## Remaining presentation observations

* `effects.spawn_async_attached(effect,part)` (999): native `efAsync_Spawn(gobj,&fp->x60C,0,effect,ownedJoint)`; synchronous effects are not interchangeable. Verify the exact effect varargs ABI and preserve its Item/Fighter effect-counter lifetime.
* `engine.debug_level` (1516): actual `DbLevel` read; `DebugLevel.debug_rom` is already a typed constant.
* `visuals.previous_group(group)` (1517): model-part visibility selection's previous value, not current group selection. A bounded owned-group read is missing.

Suggested implementation order: existing spelling rewrites plus the two ordinary writes; guard/callback/contact closure; float input and helper collision continuation; held transitions/optional recipient delivery; Article lifecycle/model/contact operations; async presentation and small observations. Keep animation-driven jump as an explicit runtime closure, rather than treating catalog acceptance as complete Peach activation.

## Cross-check: canonical Dr. Mario Article closure

Read-only whole-script pass of `examples/drmario_full.fms` after Item accessory tracking landed. The supplied latest source-check diagnostic is `item.reset_spin()` at 528. No new source-check/build/game run was performed for this audit. The remaining surfaces group into the same ordinary Article lifecycle/model family, rather than new Dr. Mario-specific APIs:

| Current source surface | Exact native contract / action |
| --- | --- |
| `item.reset_spin()` (528) | `it_80274740` (`it_2725.c:1139`): model descriptor bits 6–7 choose root child depth; zero `xD3C_spinSpeed`; select rotation axis from `xDC8_word.flags.x17`. Neither setting `item.spin_speed=0` nor zeroing the primary root's Z rotation reproduces it. Admit the owned descriptor/actual selected joint, preserve the native axis contract and guard the source. |
| `item.refresh_physics()` (532) | `it_802750F8` (`it_2725.c:1419`): set `xDCC_flag.b3=0`, call actual `Item_802697D4` physics phase, guard, call `Item_80269978` collision phase, guard, then set b3=1. This genuinely reenters the source Item phase scheduler (`item.c:1650,1709`); it is not a flag-only refresh. Retired/faulted nested phases must stop the final write and outer script continuation. |
| `item.remove()` (540) | Shared missing removal operation described above; `Item_8026A8EC`, with source termination/deferred lifetime preserved. |
| `item.scale_by_attribute(value)` (543) | `it_8026BAE8` (`it_26B1.c:686`): F32 multiply value by owned Item attributes `x60_scale`, store `Item.scl`, then `it_80272F7C` uniform JObj scale. Existing `item.set_scale` lacks the Item scalar write; do not substitute it alone. This is distinct from `set_instance_scale`'s additional physics/collision refresh. |
| `item.joint(0, "").translation.x/y/z` (550–552) | Existing closed primary selector supports rotation/scale/visibility, but rejects translation. Extend the same load-prepared selector table with native translation channels 0–2, preserving `nativeModelJoint_Write`'s finite-value/no-op policy and exact root/bank lifetime guard. No secondary model is requested. |
| `item.child_visible(Bool)` (577,598) | Reference native helper (`native_fighter_module.c:302`) toggles `JOBJ_HIDDEN` on **root->child only**, absent child is no-op. Existing `.visible` uses `HSD_JObjSet/ClearFlagsAll` (recursive, channel10); it is not equivalent. A genuine single-node visibility selector/operation can use the existing native channel15 semantics with load-resolved primary child identity. |
| `owner.model_scale` (585,606) | Exact source rewrite to `owner.model.scale`; old getter `ftLib_800869D4` calls `ftCommon_GetModelScale`, already implemented as `x34_scale.y * co_attrs.model_scaling`. No additional numeric getter is needed. |
| `creator.incapacitated` (618,628,638,648) | `ftCo_800BF228` (`ft_0BF0.c:115`) tests exactly `ftCo_MS_DeadUpFallHitCameraIce` or `ftCo_MS_Sleep`. Both literal constants and retained `creator.motion` already exist, so an authored two-motion predicate is an exact migration; no donor decision or new specialized incapacitation API is needed. |
| `creator.root_scale_y` (same four sites) | Reference getter (`native_fighter_module.c:296`) reads the live root JObj's scale Y, default1 for no root. This is absent as a receiver property and differs from `model.instance_scale_y` (`Fighter.x34_scale.y`) and `model.scale`. A generic guarded live-root scale observation is needed. |

Relationship semantics need a deliberate call-site pass alongside these migrations. Old Article `owner.*` resolves the revocable attachment/controller relation (`native_fighter_item_script.c:671`); current `owner` is original creator only while damage owner equals creator (`native_script_receiver.c:374`). The canonical script already uses explicit `attachment_owner` in its result accessory. For the remaining Sheet/taunt control accesses, use `attachment_owner` when that is the original control intent; replacing only the property spelling would leave a different relation after reattachment/reflection. Custom owner state writes and callback clearing already have typed retained receiver support, subject to exact pinned root program/schema.

No additional missing Fighter root hook, old reset-root convenience, three-argument signal, or factory arity was found in this canonical file: ResetRootRotation uses the existing rotate primitive and damage/death2 bindings; signals have explicit Article declarations; create-with-velocity has the explicit twelfth Bool. Its two Articles (Sheet states2, Vitamin states7) plus Tornado effect use existing schema shapes, a primary model and an ordinary projectile directive. Their actual owned DAT/model/DATA/appearance/effect validation still must pass; the static pass does not claim full program/package acceptance. Neither Chain resources nor secondary-model aliases are required by this script.

Recommended coherent next batch: reset-spin + real nested physics/collision refresh + guarded removal + the two distinct Item scaling policies; primary translation/single-node visibility + live root-scale read; then the exact receiver/predicate source migrations. Tests should separate native scale/axis/visibility parity from actual nested scheduler retirement/fault behavior.
