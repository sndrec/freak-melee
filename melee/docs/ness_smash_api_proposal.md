# Ness smash ownership: proposed generic contracts

Source census, 2026-10-02. Proposal only: no SDK, template, candidate or runtime
changes, builds or games. Scope is the seven versus smash states, their entry
routing and common charge interaction; whole Ness independence remains separate.

## Exact coverage boundary

`ftNs_Init_MotionStateTable` in `ftness.c` supplies four gameplay callbacks per
state (28 total), with the common camera callback unchanged:

| State | Motion | Original callback prefix |
|---|---|---|
| 341 | forward smash | `ftNs_AttackS4` |
| 342 | up smash start | `ftNs_AttackHi4` |
| 343 | up smash charge | `ftNs_AttackHi4Charge` |
| 344 | up smash release | `ftNs_AttackHi4Release` |
| 345 | down smash start | `ftNs_AttackLw4` |
| 346 | down smash charge | `ftNs_AttackLw4Charge` |
| 347 | down smash release | `ftNs_AttackLw4Release` |

The accepted unregistered Bat/YoYo constructors, initialization, four article
physics solvers, animation/material/entry callbacks and ordered resource release
remain reusable. They do not yet replace these fighter callbacks. Fighter-local
frame, rehit timer, charge-disabled flag, slope-position flag, hit-position XYZ,
spin rate and retained Bat/YoYo IDs belong in ordinary authored state. All
thresholds, frames, motion IDs, tuning and presentation IDs belong in named
owned parameters; no `mv.ns` or item-DAT runtime accessor is proposed.

## 1. Entry replacement hooks, separate from presentation

Existing `on forward_smash_start` is a post-common-transition presentation hook.
`ftCo_AttackS4.decideFighter` sets facing, calls ordinary `doEnter` and then the
hook. `doEnter` selects common angled motions, clears interrupt/command0/throw
flags and transitions before invoking it. Ness instead enters state 341 directly
and does not perform that ordinary selection or clear throw flags explicitly.
Adding a second motion transition inside the presentation hook is observably
wrong (state-exit callbacks, subactions, attack bookkeeping and creation order).

Propose full, optional replacement hooks:

```text
on forward_smash_enter:   # event.angle Float; facing already selected
on up_smash_enter:
on down_smash_enter:
```

Definition shapes: `void (*on_forward_smash_enter)(Fighter_GObj*, float angle)`
and `HSD_GObjEvent on_up_smash_enter/on_down_smash_enter`. At the existing
fighter dispatch branch, a supplied hook replaces the entire fighter entry and
returns; absent hooks preserve existing stock dispatch and presentation-hook
behavior. Do not automatically run the presentation hook after a replacement:
the replacement owns that sequence. Input/tilt-stick checks and held-item
throw/swing priority remain common. Up smash must cover **both**
`ftCo_AttackHi4_CheckInput` and `CheckInputNoD0`; down covers
`ftCo_AttackLw4_CheckInput`; forward covers both callers of `decideFighter`.
Capture/revalidate source generation at authored dispatch boundaries before any
retained fighter access or further callback. A terminal return must not silently
introduce cached-entity continuation in its caller.

Original ordered script responsibilities:

- Forward `ftNs_AttackS4_Enter`: clear interrupt, command0=false; change state341
  with flags0/frame0/rate1/blend0; start animation; construct Bat at current
  fighter position/raw joint42/facing with initial collision false; publish ID;
  install damage/death2 cleanup only if creation succeeded.
- Up/down initial entry: clear interrupt and charge-disabled; command1=0 then
  command0=0; frame=1, rehit=0, slope-position=true; zero hit XYZ and spin;
  change state342/345 flags0/frame0/rate1; start animation; set Song immunity;
  install deal-damage rehit and accessory hit-position callbacks.
- Charge entry343/346: transition with `SkipItemVis`, frame12/rate1; start
  animation; set animation rate0; apply the ordered manual-charge setup below;
  set Song immunity; reinstall deal-damage/accessory callbacks.
- Release entry344/347: transition with `SkipItemVis`, frame13/rate1; start
  animation; set authored frame14; signal Held with zero input if YoYo exists,
  then set released spin; play210090; apply charged hitbox damage; Song immunity
  and callbacks last. The common motion transition resets charge **mode**, but
  retains charged frames for the following damage calculation.

## 2. Common charge mode4 fields, not an extra per-frame script counter

Source: `ftNs_AttackHi4_YoyoApplySmash`, `ftCo_800DEF38/DEEE8/DF0D0/DEEB8`
(`ft_0DF0.c`), and their call sites in `fighter.c`.

Expose semantic direct common fields:

| Proposed RW property | Actual common field | Type |
|---|---|---|
| `smash.charge_mode` | `smash_attrs.state` | Int enum |
| `smash.charge_frames` | `x2118_frames` | Float |
| `smash.charge_limit` | `x211C_holdFrame` | Float |
| `smash.saved_animation_rate` | `x2124_frameSpeedMul` | Float |
| `smash.charge_color` | `x2128` | Int |
| `smash.shake_index` | `x212C` | Int, bounded byte/table index |
| `smash.shake_period` | `x212D` | Int, bounded table period |

Add `Smash.manual_charge`=4 to the common enum and
`smash.shake_table_count` RO Int from `Fighter_SmashChargeShakeTable->x4`.
Require bounded index/period writes against that immutable table; no arbitrary
memory indexing. Other enum constants are None/PreCharge/Charging/Release.
An engine-defined table count is common resource metadata, not duplicated Ness
FMP tuning. No exposed private union and no new stock charge-handler delegate.

Exact ApplySmash ordering: authored frame=0; if YoYo exists, signal Released
with named release velocity (original X*facing and Y), **before** common fields;
mode=4; saved rate=current `frame_speed_mul`; frames=0; limit=60; shake index=0;
period=engine table count; color=119; start color if color!=123; rumble36/arg0;
play210087/volume127/pan64. Existing `color.start`, controller rumble and SFX
operations suffice. Since rate was set to0 first, the saved rate here is **0**.
Do not capture the pre-pause rate instead.

Important untouched fields: ApplySmash does **not** initialize damage multiplier
`x2120_damageMul` or `x2130_sfxBool`. Ordinary PreCharge initializes the sound
latch, but manual-charge setup does not. Preserve this source behavior; do not
reset the latch as an implicit convenience or claim a newly corrected behavior.

`Fighter_8006A360` calls `ftCo_800DEF38` **before anim_cb**. Mode4 increments
Float charge frames, increments/wraps the byte shake index, and plays common
SFX0x7B once at common threshold `x7C8` if the retained latch is false. It does
not clamp to limit, restore rate, remove color or stop rumble. `ftCo_800DEEE8`
uses the same shake table for both mode2 and mode4. `ftCo_800DF0D0` only handles
PreCharge/Charging, so mode4 ignores common A-release; the authored charge IASA
owns release. `Fighter_ChangeMotionState` calls `ftCo_800DEEA8`, clearing only
mode. Do not add a second authored increment/shake loop: it would double count
and change stage ordering.

The authored YoYo frame counter is separate: charge animation increments it,
updates spin, performs rehit reset, then compares against owned charge duration.
Release sets it to14. Common charge frames supply the damage multiplier; the
existing `hitbox.set_damage(0,value)` casts to the actual engine u32 signature.
Keep the source's binary32 multiplier `0.0039059999398887157` and truncation;
no float-damage Host extension is necessary.

## 3. Victim-history reset followed by enabling

`YoyoCheckTimedRehit`, its inlined copies and Charge animation: only when
command0==0 and the authored positive timer decrements to0, call
`lbColl_80008440(&x914[0])`, then `lbColl_80008434(&x914[0])`.

Add `hitbox.reset_victims(index)` backed directly by
`void (*reset_hitbox_victims)(HitCapsule*) = lbColl_80008440`, with existing
fighter-hitbox bounds. It clears **only** `victims_1[].victim`, x44,
`victims_2[].victim`, x45; retained victim timers and all other hitbox fields
are unchanged. Follow with existing `hitbox.set_enabled(0,true)`, which exactly
sets `HitCapsule_Enabled`. Do not combine reset with enabling or use clear_all
(which disables/changes a larger set of hitbox state).

Deal-damage callback sets the ordinary rehit timer to int(owned rehit rate).
Existing `callbacks.track_deal_damage`, accessory binding, enabled/damage
queries and `hitbox.set_world_position` already suffice. Hit-position update
runs only for a non-disabled hitbox and nonzero authored X or Y; existing world
position primitive calls `ftColl_8007B8A8`, clearing hitbox jobj and assigning
b_offset exactly.

## 4. Fixed-ECB swept query, raw environment flags

`ftNs_AttackHi4_YoyoCheckEnvColl` constructs **scratch** collision data, not
fighter collision data: compute radius*instanceY; top/rightX=scaledRadius;
bottom/leftX=-scaledRadius; sideY=0; `mpColl_80041EE4`; push position A, then B
(`last_pos=cur_pos; cur_pos=new` twice); category bits `x34_flags.b1234=5`;
`mpColl_8004730C(&coll,&box)`; return translated environment bits. `mpCollPrev`
inside the solver assigns x28=current and updates its ordinary stage scratch.
The actual fixed-ECB solver is required; stage rays/floor probes/resource node
sweeps do not have the same shape, mode or initialization/history contract.

Propose a common stack-only descriptor query, e.g. engine
`uint32_t nativeStage_SweepFixedECB(const Vec3* from,const Vec3* to,
const ftCollisionBox* box,uint32_t category)`, and DSL
`stage.sweep_fixed_ecb(fromXYZ,toXYZ,top,bottom,rightX,rightY,leftX,leftY,
category,out_flags)` (expanded scalar arguments; output is an Int local).
Its substantive engine implementation owns scratch initialization and ordered
history/category setup, then calls the actual fixed-ECB solver. No retained
pointer, allocation, fighter field mutation or Ness collision helper delegation.
Return actual `coll.env_flags`; script owns all remapping and decisions. Initialize
the scratch object deterministically before `mpColl_80041EE4`: the original local
starts uninitialized and that initializer reads cur_pos, although the two pushes
replace final last/current and `mpColl_80043754` replaces prev_pos at each step.
Do not imitate undefined stack data; compare defined solver observations and
document this initialization boundary. Validate
finite descriptors and representable category; invalid input returns0 explicitly.
Do not claim the stage's internal scratch side effects are absent.

Source remapping remains authored: FloorMask->32768; LeftWallMask->1;
RightWallMask->64; CeilingMask->8192. Constants for raw masks can be generic.
First sweep: A=fighter position plus half(topECB+bottomECB) on Y, B=hit position,
radius0.5*instanceY. Only if all four categories are absent, perform the second
sweep from hit position+instanceY to hit position-instanceY, radius1.5*instanceY;
charge allowed only on translated FloorPush. Keep this short circuit and order.
The frame13/charge-disabled decision belongs to the fighter script.

## Two small compiler surfaces and one naming correction

Slope-corrected hit position uses raw joint61 and semantic XRotN world positions,
floor normal XY, subtract/rotate-Z/add. `bones.world_position`, scalar atan2 and
owned XYZ fields cover the reads and arithmetic. However, the fighter compiler
currently rotates only fighter self velocity, not arbitrary XYZ. The source
`lbVector_Rotate` uses fifth-degree polynomial sine/cosine approximations;
ordinary `sin`/`cos` is not an exact substitute. Expose generic
`vectors.rotate(x,y,z,axis,angle,out_x,out_y,out_z)` with Int axis (1/2/4), Float
outputs and stack Vec3 lowering through the **existing** Host `vector_rotate`,
already bound directly to `lbVector_Rotate`. No new Host, native wrapper or
velocity scratch mutation is needed. Keep subtract/rotate/add ordering and
leave Z unchanged for axis4. Up-release interpolation
uses clamp(0.1*(frame-14)) before24; down uses clamp(0.2*(frame-14)) before19.
No additional joint/position API is needed.

Add `hitbox.state(index)` RO Int and generic `Hitbox.enabled`/`Hitbox.disabled`
enum constants (with bounded index). Charged damage tests state exactly equal
to `HitCapsule_Enabled`; existing `hitbox.enabled(index)` returns state !=
Disabled and therefore also accepts enum states2/3/4. It correctly suffices for
the accessory position update, but cannot express the stricter damage gate.
This requires only a scalar compiler read, no Host extension.

Forward Anim already fits `reflector.active/create/disable`, followed by
`callbacks.track_reflect_hit()` to install the authored SFX0xE0 handler (create
currently installs NULL). Forward Phys uses `physics.ground_motion()` then
`reflector.update()` (=ftColl_8007AEF8); the other states use ground_friction.
All seven Coll callbacks use the existing standard grounded collision then
ordered YoYo cleanup if airborne. Common Wait IASA, frame checks, transitions,
article signals/removal, pause/resume, conditional callback binding and auxiliary
item binding are present. No extra reflector/physics/item primitive is missing.

`capture.mark_special()` currently writes x2222_b2, whose real shared consumer
`ftCo_800C3538` rejects Song damage. Add/rename a semantic Bool
`damage.song_immune` field and migrate call sites directly; do not introduce a
Purin/Ness-specific primitive or expose the raw offset. Existing operation can
technically express the bit, so this is a naming correction, not new behavior.

## Next coherent acceptance batch

After the four main contracts and two small compiler surfaces land: one unregistered complete seven-state candidate
with Bat/YoYo entries and ordered full cleanup. Actual original-C extraction
should cover all entries (both up routes), creation failure, existing callbacks,
rehit histories with nonzero retained timers, hitbox disabled/enabled, charge
mode4 before-animation tick/shake wrap/latch already true vs false, edited named
parameters, both release forms and damage truncation; fixed-ECB compare must
exercise the actual stage solver with meaningful floor/wall/ceiling geometry,
plus fixed controlled branch/order cases. Include lifecycle invalidation at
entry/transition/creation/signal boundaries and retained ID replacement. Keep
undefined native stale-pointer inputs separate from defined source pairs.
No default activation or whole-fighter acceptance is implied by this proposal.

## Source receipts

SHA-256 of live source read for this proposal (no source bodies edited):

```text
e72afa9f2e19367cce248493a50ef4100b64276770ca9b4f78b82076eafaae8f  src/melee/lb/lbvector.c
4f99bce97309d37a5e96bce98ed01715f42b4f7a48dfbaa3e64812dce41f60f5  src/melee/ft/kinds/ftNess/ftness.c
050aaeabc4f7b27df6a83d0d8ea77ce7e9e429996783c45345308c1f4498ee61  src/melee/ft/kinds/ftNess/ftnessattacks4.c
d624fe75cf5ab96767f05d6750690ac930692887f12d297844835481b69b19e6  src/melee/ft/kinds/ftNess/ftnessattackhi4.c
75de1eff0dc34d43b28221a1ef7dd810b3070f83a8d897e5e8503fdd13a41c83  src/melee/ft/kinds/ftNess/ftnessattacklw4.c
8f74740c6c0d20f4f651026da2fb560f3bf931eae69cfc13cf34f66625e9bf14  src/melee/ft/kinds/ftCommon/ftCo_AttackS4.c
07e71d2057fc722c5f05549ca2b89260109b6d11dc84b438f2d2b4c0c0a9850f  src/melee/ft/kinds/ftCommon/ftCo_AttackHi4.c
e3569af49078a6bd54a2930811ae77327a6dfc4ccc8e78fa3fa6211117df13d0  src/melee/ft/kinds/ftCommon/ftCo_AttackLw4.c
38836511908f55b2c5860ff30174374b980abf9e99869b151ad9b1c48d72673e  src/melee/ft/ft_0DF0.c
ff0448d911ef932ca5a77b96c5ca74c23226a38759603095aaeabf726f1074f9  src/melee/ft/fighter.c
3a28b63098482e46b48d6a15bf2f0802cc60019be840374ca39ed7dc2c285c6c  src/melee/lb/lbcollision.c
e76841854d6f27b786ba57e431e915ebba705cb86c00cda942ff0ba44059945a  src/melee/mp/mpcoll.c
eed335bce591f7e8cf591b520b05272a1bf257df43811428aae1322fc28374ea  tools/native_fighter_script_primitives.c
011d6c7d52e0641b3c28c38bcdf948fff3ed2216b5ec75c2124b5b871be17f23  tools/native_fighter_script_terrain.c
96237f282ba290e34fd1f8888e2eb70a10a4a32e8c97d4bc5c1bbcd262f9d5f3  tools/native_fighter_script_code.c
```
