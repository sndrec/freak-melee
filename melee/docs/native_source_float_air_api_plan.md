# Generic float and airborne input API plan

Read-only source audit on 2026-10-09. No production/template edits, build,
source-check or game run. These seven operations remain absent from the direct
catalog; earlier held/guard/Article closures do not implement them. All proposed
calls are Fighter-context methods; effectful calls retain the executing owned
program, storage, physical pool identity and scene. No Peach policy or native
Fighter-definition handler belongs in the backend.

| Source call / signature | Actual native contract | Admission and behavior |
| --- | --- | --- |
| `controls.aerial_attack_motion() -> I32` | `FtMotionId ftCo_AttackAir_GetMsidFromCStick(Fighter*)`, `kinds/ftCommon/ftCo_AttackAir.c:132` | Return the common aerial selection directly. Use current C-stick only when `ftCo_800DF478` says it crossed a threshold; otherwise left stick. Neutral uses strict absolute-axis thresholds; angle is `atan2f(y, abs(x))`, then high/low, then facing-relative front/back. Do not invoke the distinct prepared AerialAttackMotion mapping hook: Peach adds its owned motion offset itself. |
| `input.cstick_attack_edge() -> Bool` | `bool ftCo_800DF478(Fighter*)`, `ft_0DF1.c:72` | Current/previous absolute X or Y crosses common xDC/xE0. No canUseCstick, game-mode, held-item or button gate occurs in this function. Do not substitute `ftCo_800DF50C`. |
| `state.air_gun() -> Void` | `void ftCo_800CDDA0(Fighter_GObj*)`, `ft_0CDD.c:36` | Physical held Item kind L_Gun/F_Flower/S_Scope selects their shared common air entries; other kinds do nothing. Healthy absent held Item should be a no-op admitted before the unsafe raw kind read. Retain actual held Item identity/assets/spec as applicable, not immutable creator or attachment control. |
| `controls.try_air_item_throw() -> Bool` | `bool ftCo_80095328(Fighter_GObj*, bool*)`, `ftCo_ItemThrow.c:195`; pass NULL output pointer | Preserve C-stick/game-mode or A+held-shield/use-category input, stick ages, thresholds, exact common throw motion and native drop alternative. This Bool means native input was handled, not item removal success. Preserve healthy false; fault/retirement must stop the caller rather than masquerade as false. |
| `controls.try_air_special() -> Bool` | `bool ftCo_SpecialAir_CheckInput(Fighter_GObj*)`, `ftCo_SpecialAir.c:15` | B press selects up, down, side, neutral in that order, with exact facing reversal before selected source hook. Run the selected prepared Void0 SpecialAirHi/Lw/S/N hook, never a stock authored table for an attached source. Missing selected hook is false; do not try a lower-priority special. Successful native handling sets x2227_b5 after guarded continuation. |
| `physics.animation_vertical_velocity_adjusted() -> Void` | `void ft_800851D0(Fighter_GObj*)`, `ft_084E.c:145` | Assign self_vel.y from transNOffset.y; multiply once in F32 by bunny modifier x14 (`Fighter_804D6520[5]`) only when x197C is nonnull. That Item pointer is a presence test, not a dereference. Require modifier data only on the multiplying branch. Preserve zero/NaN/infinity arithmetic; no authored facing factor or clamp. |
| `collision.air_landing_callback(owned Void0 helper) -> Void` | `ft_800831CC(gobj, ftCo_80096CC8, native_cb)`, `ft_081B.c:515` | Same-program root non-event helper, not integer/receiver callable or None. Exact collision policy below; call a code ID directly without an authored native function pointer. |

## Ordering and continuation requirements

Peach `try_float_attack` calls the selector for its gun test, then
`begin_float_attack` calls it again (`examples/peach_full.fms:1012-1025`). Preserve
two evaluations. Float IASA tries air-special before float attack (1080);
float-attack IASA tries air-item throw before float attack and aerial jump (1051).
Bool short circuit order must remain authored. Full aerial IASA or a stock float
handler changes that order.

The landing body currently shared by `ft_80083090`, `ft_800831CC` and
`ft_800835B0` is `ft_80083090_inline` (471). It copies last/current collision
position; chooses mpColl_80047AC8 when ledge cooldown or x2224_b2 is set, otherwise
sets facing then uses mpColl_80047E14; copies resolved position; runs
ft_80081A00 environmental response; contact calls landing; otherwise walljump
precedes cliff. The ledge predicate `ftCo_80096CC8` rejects line -1 and platform
lines unless stick Y exceeds common x25C. Environmental response is native Item
intersection/state work, not an authored landing callback. Walljump/cliff entries
can change motion and run source lifecycle decisions. Landing calls in this
batch must not add the aerial-jump check used by ft_800835B0.

Concrete hazards in current code requiring narrow fixes before exposure:

- SpecialAir currently bypasses native NULL-table checks for every attached
  actor, then runs absent hooks as neutral Void success. It also snapshots
  generation **after** a hook and writes cached fp->x2227_b5. Capture full
  NativeFighterLifetime before the selected hook, test HasHook, and validate the
  retained identity before the flag write. A replacement must not be reacquired.
  Test selected-handler availability before side/neutral facing changes, matching
  the native NULL-handler early return. Missing hooks must leave facing unchanged.
- Air gun entries ftCo_800CE010 (`ft_0CDF.c:34`), ftCo_800CE480
  (`ft_0CE3.c:32`) and ft_800D7770 (`ftCo_ItemScopeStart.c:58`) continue cached
  field/accessory/held-Item writes after ChangeMotionState and animation. Guard
  those actual shared bodies; backend post-validation alone is too late. Their
  native common gun callbacks are engine protocol, not donor Fighter handlers.
- Air throw's neutral-drop branch calls ftCo_80095744, then writes x2224_b1.
  The drop function can enter Fall/FallSpecial before fetching item_gobj, and
  Item_8026ABD8 invokes source Dropped/holdable hooks before native Item tails.
  Retain source and the selected/current Item at each actual effect boundary;
  explicitly handle legitimate held-slot changes instead of dereferencing NULL
  or reacquiring replacements. Existing ftCo_800957F4 already guards motion and
  animation with Fighter/Item identities; its final accessory call can still
  invalidate the caller, so validate before Bool publication.
- Landing copies cached CollData after map queries and calls further common
  entries. Add retained validation inside the shared resolver before cached
  writes, then before/after owned helper and each wall/cliff continuation.

No explicit RNG draws occur in the audited selector/check/entry bodies. This is
not a transitive RNG-free claim: actual animation, Item and source callbacks must
keep their ordering and RNG side effects. The gun entry also installs ordinary
engine accessory callbacks; later shooting/render behavior is a separate scope.

## Implementation ownership and grouped proof

Use a new focused `native_script_binding_air_inputs.c/.h` for the five zero-arg
input/entry operations. Put animation adjustment and the owned landing helper
in a separate `native_script_binding_air_physics.c/.h`. Shared catalog/property
routing stays root-owned. Narrow native ownership spans SpecialAir, ItemThrow,
Item drop, the three gun entries, and the shared collision resolver/header.
Promote/refactor the substantive ft_80083090_inline body to a descriptive Bool
resolver (its native callback parameter is unused), adjusting all three callers
directly. Source landing invokes that resolver and its own code-ID helper, then
actual wall/cliff routines. No callback trampoline or forwarding shim is needed.

All motion entries must use the published common/owned motion layout and actual
imported common assets. Pure selector results are common IDs; returned values do
not themselves enter a motion. Existing SpecialAir Void0 hooks suffice; no new
hook enum is needed. Existing AerialAttackMotion/AerialAttackEnter hooks are
separate scheduler contracts. Full Peach animation-driven jump remains an
independent open dependency, not a reason to alter its source Bool.

One connected compiler/actor/VM/pool cohort should compare native threshold and
angle boundaries, both facing directions, C-stick edge histories and repeated
selection order; common gun kinds/ammo/unsupported/absent Item; all throw/drop
branches and exact selected motion/speed; four special directions, absent hooks
and facing/flag timing; bunny pointer presence and exact F32 bits; landing contact,
environmental interruption, walljump and cliff order. Use actual common bodies
and real prepared hooks/owned helpers. Inject fault, detach, free/reuse,
userdata/class/program/schema replacement at each reached callback boundary;
require unpublished output, stopped subsequent authored statements where the VM
retires/faults, and no cached native continuation after successful identity change.
Control geometry/model/animation/stat leaves explicitly where necessary. Such a
fixture proves contracts and continuation, not complete firing, collision, float,
render or game acceptance. No per-Peach state policy belongs in native C.
