# Remaining generic contracts and source census

All paths below are under `src/melee/`. `original_inputs.json` pins the actual
unchanged SDK57 sources used in the comparison. The accepted component sources
are hashed independently in `composition_inputs.json` on each run.

## Complete normal entry and phases

`ft/kinds/ftGameWatch/ftgamewatch.c` owns numerical motions341..352: Jab,
RapidStart/Loop/End, DownTilt, ForwardSmash, Neutral/Back/UpAir and their three
landing phases. Each has animation/IASA/physics/collision and shared camera.

The new pending draft accounts for all 48 phase slots. The only pure common
callbacks are Jab IASA (`ftCo_Attack11_IASA`), rapid-loop IASA
(`ftCo_Attack100Loop_IASA`), and the three aerial IASAs
(`ftCo_AttackAirN/B/Hi_IASA`); `motion_policy` binds ordinary common handlers.
All remaining decisions are readable script bodies. Shared camera is metadata.
No GameWatch handler is used as a delegate.

| Shared caller / original body | Required generic contract and exact order |
|---|---|
| `ftCommon/ftCo_Attack1.c` raw decideFighter -> `ftGw_Attack11_Enter` | Exclusive `on jab_enter` before raw dispatch. `attack.enter_jab(motion)` Bool: common pickup94790 first; IASAfalse/x2218_b1false; change motion(frame0/rate1/flags0); animation; jab2 input window -> hitlag_mul; unk_msid44; x2218_b2false; ordinary common jab scratch reset; x1A54 reset. Script assigns accessory only on success. Existing `on_jab_start` flags event stays unchanged. |
| `ftCommon/ftCo_Attack100.c` -> `ftGw_Attack100Start_Enter` | Exclusive `on rapid_jab_enter`. Actual common800D6B00 with explicit motion342, then accessory assignment even if pickup consumed entry. Shared rapid-loop800D6C60 accepts authored End callback; its x0/x4 and input logic remain ordinary common engine state. Owned Loop343/End344 use flags0 and no animation start. |
| `ftCommon/ftCo_AttackLw3.c` -> `ftGw_AttackLw3_Enter` | Exclusive `on down_tilt_enter`; generic pickup94790 Bool query, then IASAfalse, state345, animation, accessory. IASA checks S4/Hi4/Lw4/S3/Hi3/Lw3/Jab/Jump/Dash/Crouch/Turn/Walk in that order. |
| `ftCommon/ftCo_AttackS4.c` -> `ftGw_AttackS4_Enter` | Existing exclusive full `on forward_smash_enter` suffices: IASAfalse, cmd0zero, state346, animation, accessory. Existing post-start presentation is not a full replacement. |
| `ftCommon/ftCo_AttackAir.c` decideFighter -> `ftGw_AttackAirN_DecideAction` | `on aerial_attack_motion(event.motion)->Int` remaps common65/67/68 to347/348/349 before actual common EnterFromMsid. F66/Lw69 unchanged. Existing post-entry event assigns accessory. |
| `ft_081B.c`80082C74 -> owned landing entries | `collision.air_aerial_landing_callback(Land)` uses actual common callback-taking collision. Landing resumes P/T/S first. cmd0nonzero -> exact `state.landing_aerial(motion,lag)` -> accessory; zero -> common BasicLanding -> nine-prop cleanup. |
| `ftCommon/ftCo_LandingAir.c` EnterWithMsidLag | Actual AirToGround, state change, then rate=(animation end+.1)/lag. RO current common `stats.landing_air_neutral_lag` / `stats.landing_air_back_lag`; both B and Hi choose Back lag. These current-resource reads are available in SDK58, pending this57 draft's final comparison. |

Entry operations and hooks are pending root integration. The current comparison
does not execute any normal entry, rapid-loop update or normal phase matrix.
It compares article/accessory/lifecycle callbacks only; accepted specials retain
their earlier independent source evidence.

## Manhole and Torch

`it/kinds/itgamewatchmanhole.c` animation:
cmd0!=0 -> cmd0zero -> root+descendants visible -> first child
`lb_8000B1CC(child,NULL,&Vec3)` -> `lb_800119DC(&Vec3,120,3,.1,1.0471976)`;
then owner/motion345 retention check. Provisional generic syntax:
`item.joint_world_position("0/0", x,y,z)` writes Float locals from one Vec3 read;
`item.wind_at_coordinates(x,y,z,frames,strength,decay,frequency)` calls actual
common wind with explicit position. Existing item.wind_at_position uses Item.pos
and is not equivalent. Primary joint matrices are not article-resource node
matrices. Both pending operations remain outside executable comparison.

`itgamewatchfire.c` Torch checks current owner through `ftLib_800876D4`, exactly
`smash_attrs.state==2`. SDK57 `targets.smash_charge_active(item.current_owner)`
now supplies this Bool without mirrored fighter state, union reads or handlers.
Owner absence returns false; animation still follows its prior remove predicate.

## Lifecycle, constructor and paper boundaries

Full `ftGw_Init_OnDamage` and conditional-air8014A538 order are authored and
compared across all 512 live-prop masks. Full OnDeath field decisions and11
visibility-group initializations are compared. Both common knockback tables
have intentional NULL entries for GameWatch. The four ordinary item
pickup/drop/visible/invisible slots use common engine callbacks with authored
flags(1,1). UnkMotionStates4 maps to generic color_animation_reset, color5 when
Panic charge>=3. OnLoad currently compares only charge/handle initialization;
full presentation is still open.

Normal constructors use actual Item_InitSpawn: both parents owner, previous and
current XYZ identical, zero velocity/damage, facing input, initial collisiontrue,
group0. Then Item_AttachToParent: Item8026AB54 -> db80225DD8 -> paper setup.
The raw32/1/17/21 attachment/world origins are distinct from semantic maps.
SDK57 authored constructors capture creator identity and can express the tuple
and attach/debug order, but no real paper allocation/render comparison is made.

Actual paper `it_8027CE64` sets paper flag, takes owned body/outline palette,
uses frame-map table/body shape via ftLib800870BC, configures render routing and
retains immutable model_visibility metadata. It must become immutable authored
construction/render metadata; no call to that raw fighter-dependent helper is
proposed. OnLoad sets capture freeze false, original paper flags, owned widthZ,
outline RGBA, selected costume body color, visibility extra item10 and10 article
assets. Required renderer/load/palette contracts are already described in the
paper census `.local/gamewatch-chef-next-batch/report.md`; this fixture does not
pretend ordinary root visibility implements the three-pass paper renderer.

No CPU move-specific closure or whole-fighter independence is asserted here.
Metadata registration, immutable paper bindings, complete entry source comparison
and drawn native acceptance must precede default activation.
