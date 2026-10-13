# Remaining complete Peach source closures

Read-only pass of current `examples/peach_full.fms`, catalog/resolvers, prepared
Fighter contracts, and native/reference bodies on 2026-10-09. Root reports the
rebuilt source reaches line651 after recipient exports now accept line490. No
build, source-check, asset import, or gameplay run was performed for this audit.
Catalog acceptance, prepared scheduler acceptance, and runtime acceptance are
separate below. This supersedes the missing-operation status in the older
`peach_common_binding_inventory.md`; its original native evidence still applies.

The later [Item-response checkpoint](native_source_item_responses_checkpoint.md)
closes the first three rows below. The truthful canonical comparison name is now
`item.attachment_owner_is(other)`; the historical reference emitted an attachment
comparison under the old creator name. Peach now reaches guard creation at
line726. The [guard plan](native_source_guard_api_plan.md) also corrects the
earlier guard row: its joint is a raw costume-table index, not a semantic FtPart,
and recorded hit damage is I32. The later
[guard checkpoint](native_source_guard_checkpoint.md) closes that row and the
legacy guard-hit preparation boundary with an explicit owned helper. Peach now
reaches the async attached effect at line1000. The subsequent
[airborne and async checkpoint](native_source_float_air_checkpoint.md) connects
that effect, the five airborne input operations, the owned landing helper and
adjusted animation velocity. The table below preserves the original audit;
animation-driven jump, contact/dynamics dispatch, held transitions, observations
and full package/gameplay acceptance remain separate work.

The subsequent [observation/contact checkpoint](native_source_observation_contact_checkpoint.md)
connects thrown-item counting, special-landing lag, debug level and prepared
body rebound. Landing input is now the captured Bool `event.interrupt_allowed`,
not a global union observation. Canonical Peach reaches line1440's motion-ID
write before held-open entry. The historical table below remains source evidence;
dynamics, animation-driven jump, held transitions and Article admission are open.

## Remaining generic operation groups

| Source sites | Current gap and exact native contract | Coherent direct implementation |
| --- | --- | --- |
| 651,655,661 `item.bounce()` | No descriptor. `native_fighter_module.c:1283` selects `itColl_BounceOffVictim`; `it_2725.c:400` multiplies X by common x58 and computes Y*x5C+x60. It is neither shield bounce nor stage bounce. | Source-owned Article zero-argument Void victim-response primitive, with retained Item/common-data admission. |
| 693,706 `item.respond_stage_mask(mask)` | No descriptor. `itmaplib.c:677` calls `it_8027781C`, then `it_80276D9C`; on response, calls `it_8027321C`, `it_80277C40(mask)`, `it_80275640(attr.x58)`. Existing sweep returns the mask; `bounce_stage()` returns a different policy result. | Integral mask input, Void, actual ordered response composition and continuation barriers. Keep the authored state change after response. |
| 672 `item.creator_is(other)` | No descriptor, and its old name is misleading. `native_fighter_item_script.c:583` emits `other != NULL && other == native_article.attachment_owner`, not immutable creator. | Generic retained **attachment-owner identity** comparison. Do not replace it with creator identity or plain Object `==`: `native_script_value.c:202` compares retained token identity/generation/kind, while separately captured records can have different identities. |
| 726–844 Toad guard: `guard.create(joint,x,y,z,radius)`, `front_only`, `active`, `melee_hitlag`, `article_hitlag`, `hit_facing`, `update_position()` | Guard namespace/family absent. Constructor is `ftColl_8007B1B8(ShieldDesc, callback)`, used in native `ftpeachspecialn.c:183,266`. Fields are respectively x221B_b3, x221B_b0, shield_unk0, shield_unk1, specialn_facing_dir; update clears shield_hit.skip_update_pos. Reference constructor checks semantic bone and finite nonnegative radius (`native_fighter_script_primitives.c:989`). | Owned semantic bone/guard-volume family plus an owned zero-Void helper bound to existing shield-hit callback slot. Convert `on guard_hit` body to an authored helper with explicit binding; constructor must preserve that code-ID policy. No authored C callback, duplicate static delivery, or stock Toad handler. |
| 999 `effects.spawn_async_attached(effect,part)` | No descriptor. Native `ftpeachfloat.c:29` uses `efAsync_Spawn(gobj,&fp->x60C,0,1236,joint)`. Reference resolves a semantic bone. Existing synchronous effect APIs differ. | Audited async joint ABI with owned semantic joint, effect-counter/source lifetime and matching cleanup. |
| 1011,1020,1022,1024,1050,1079 float input: `controls.aerial_attack_motion()`, `input.cstick_attack_edge()`, `state.air_gun()`, `controls.try_air_item_throw()`, `controls.try_air_special()` | All absent. Exact shared bodies: `ftCo_AttackAir_GetMsidFromCStick` (`ftCo_AttackAir.c:132`), `ftCo_800DF478` (`ft_0DF1.c:72`), `ftCo_800CDDA0` (`ft_0CDD.c:36`), `ftCo_80095328(gobj,NULL)` (`ftCo_ItemThrow.c:195`), `ftCo_SpecialAir_CheckInput` (`ftCo_SpecialAir.c:15`). Native Peach uses these in `ftpeachfloatattack.c:28–81` and `ftpeachfloat.c:64`. | Group motion selection/C-stick observation and effectful air gun/item-throw/special input policies. Preserve Bool short-circuit ordering and false-return retirement/fault barriers. Do not replace them with full aerial-jump IASA or a stock float handler. |
| 1085,1095,1105,1393 `collision.air_landing_callback(helper)` | Absent. Oracle `native_fighter_script_functions.c:430` uses `ft_800831CC(gobj,ftCo_80096CC8,helper)`; actual collision/wall/cliff order is `ft_081B.c:515`. Existing `air_aerial_landing_callback` uses a different collision policy; `air_special_landing(Bool,lag)` has a fixed landing action. | Genuine owned zero-Void landing CodeRef branch in the shared special-air collision policy, retained before collision and rechecked before helper/native tails. |
| 1093,1103 `physics.animation_vertical_velocity_adjusted()` | Absent. `ft_084E.c:145` sets self_vel.y to transNOffset.y, multiplied by Fighter_804D6520[5] only when x197C is nonnull. Oracle selects `ft_800851D0`. | Direct common animation-velocity primitive with owned animation/source admission; preserve F32 multiplication and condition. |
| 1162 `stats.count_as_thrown_item()` | Absent; oracle `native_fighter_script_code.c:1486` sets x2070.count_thrown_items=true. | Generic writable thrown-item counting flag (or exact zero-Void flag operation); no stock-specific stat handler. |
| 1208 `collision.contact_rebound_allowed()` | Absent. `native_fighter_contact.c:8` tests x221C_b5/unk_gobj then consults the **recipient's** rebound policy. Current native path still uses Definition/kind fallbacks. | Pair with the prepared recipient Bool body-rebound hook below, exact source/recipient retention, no source Definition fallback. |
| 1439 `motion.id = Motion.fall_special`; 1440 `state.held_open(10)` | Motion.id descriptor is read-only; held_open absent. Native `ftpeachspecialhi.c:206` performs this exact field write before `ftCo_ItemParasolOpen_Enter`. That common routine stores prev_msid, selects the owned held-motion decision, changes motion, starts animation, then initializes held timers (`ftCo_ItemParasolOpen.c:49`). | Deliberate bounded motion-ID field write plus shared held-open entry with F32 blend. Entering FallSpecial first or raw entering ItemParasolOpen would change native ordering/previous-motion input. |
| 1460 `landing.interrupt_allowed`;1472 `movement.special_landing_lag`;1516 `engine.debug_level` | Absent proper parent properties. Oracle reads mv.co.landing.allow_interrupt, x2EC, and DbLevel (`native_fighter_script_code.c:273`; `native_fighter_script_primitives.c:38,67`). Existing controls.interrupt_allowed is a different field. | Pure Bool/F32/I32 observations under their actual generic namespaces, with source/class admission. DebugLevel.debug_rom constant already exists. |

## Hidden preparation/runtime boundaries

- Source333 `dynamics_interaction_start` is typed I32(I32,I32), and336
  `body_rebound` is Bool(), in `native_script_events.c:585,603`, but neither
  exists in `native_script_fighter_hooks.c`. `native_script_fighter_program.c:105`
  explicitly rejects unmigrated non-Void/argument hooks. `ftdynamics.c:415`
  still calls Definition before each lb_8001044C; native contact still queries
  the recipient's Definition. These are a coherent prepared-dispatch/lifetime
  migration, not metadata that can be ignored once body compilation passes.
- Source829 `guard_hit` has a typed event signature (`events.c:615`) but neither
  a static hook row nor an instance callback name in PrepareProgram. Prefer the
  explicit shield-hit helper binding above, preserving the native constructor's
  delivery role and owned helper policy.
- Source332 calls `jump.begin(...,true,true,true)`. Its signature compiles, but
  `native_script_binding_jump.c:68` rejects the animation-driven argument before
  mutation. Peach requires genuine source-owned animation-driven jump physics;
  changing that Bool to false or delegating to stock Peach jump is not parity.
- Five Article declarations (Explode419, Parasol455, Turnip558, Toad887,
  Spore931) use standard primary rendering. Current Item metadata/constructor
  supports `render standard`; these are not Chain or secondary models. Full
  preparation still must validate their actual owned model/DATA/dynamics,
  declared animation indices, attribute extents, effect ABIs, and class graph.
  This static pass does not assert package/resource admission.

## Already closed, and remaining source semantics

Do not reimplement the previously missing held observations/exports, direct Item
animation advancement, appearance, instance/owner scaling, hide/show/rate,
removal, track/clear accessory, expiry, masked sweep, current/attachment-owner
lifecycle, ordinary Item-airborne assignment, Item position effect, Item
root-effect cleanup, root-scale/group queries, four jump observations,
interrupt-allowed write, generic taunt, stats landing-lag fields, common ground
transitions, or contact callback binding. Their current descriptors/backends
exist; each remains subject to its actual typed/context/runtime contract.

`holder.motion` at477,921,929 is a separate semantic migration: the reference
emitter (`native_fighter_item_script.c:682`) reads the **current damage owner's**
motion, while the current retained holder role checks physical item_gobj.
Use current_owner.motion with deliberate existence/default policy; do not leave
the physical-holder substitution after reflection/drop. The already-migrated
held-motion/frame source uses canonical receiver items observations.

Suggested next implementation clusters: Item victim/masked response plus honest
attachment identity; guard/callback and dynamics/contact dispatch; float input,
owned landing callback and animation-driven jump; common held transition and
three pure observations; audited async presentation. No extra compatibility
aliases or per-Peach engine policy are needed.
