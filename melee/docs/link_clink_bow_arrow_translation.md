# Link / Young Link Bow and Arrow translation proposal

Status: the owned source fixtures pass 9,132 Bow and 17,114 Arrow comparisons.
Complete fighter cleanup and construction coverage remain open. The accepted
template still overrides Spin Attack and Bomb; no Bow or Arrow override is registered.
The stock Arrow vibration callback has separately passed 288 direct cases after
its native float-table address repair.

## Generic operations required

The operations below are implemented and their generic model, import, parser,
and target tests passed in the coordinated Release build. Bow/Arrow themselves
remain source fixtures awaiting complete fighter construction and lifecycle acceptance.
Each decision stays in script; Host operations expose general engine work.

| Script operation | Host contract / stock operation |
| --- | --- |
| `item.sweep_segment(px, py, pz, nx, ny, nz, line, flags, hit)` | `bool item_segment_sweep(HSD_GObj*, Vec3*, Vec3*, Vec3*, int*, u32*) = it_8026EA20`. First three expressions form the previous position; current position is `item->pos`. The remaining arguments are distinct typed article-state output lvalues: normal xyz floats, line and flags ints, hit bool. Copy outputs exactly as returned; do not attach or select a motion. Existing `item.sweep_stage_normal` discards line/flags and is insufficient. |
| `stage.line_exists(line)` | `bool stage_line_exists(int) = mpLib_80054ED8`. |
| `item.bind_stage_line(line)` | `void set_collision_line(CollData*, int) = mpColl_80043558`, with this item's collision data. |
| `item.read_stage_line_speed(line)` | `bool stage_line_speed(int, Vec3*, Vec3*) = mpGetSpeed`, with item position and velocity. Preserve its result without inventing a state transition. |
| `stage.line_normal(line, nx, ny, nz)` | `Vec3* stage_line_normal(int, Vec3*) = mpLineGetNormal`, with three distinct float output state fields. Script chooses when to query and computes normal-angle differences. |
| `item.animation_frame` getter / `item.set_animation_frame(frame)` | Getter reads `x5CC_currentAnimFrame`; setter calls `Item_80268DD4`, which requests the animation frame as well as storing it. Existing `item.advance_animation()` remains `Item_802694CC`. |
| `model_root NAME OFFSET` article declaration; `item.load_model(slot, NAME)` | Import a relocated immutable `HSD_Joint` root from an article special-attribute DAT offset, then `HSD_JObjLoadJoint`. Additional model slots 1/2 use the existing fixed native model array and existing destroy/model-channel operations. Loading replaces/removes an existing slot at this explicit transition. The Arrow roots occupy console DAT offsets 36/40; these are **not native C pointer offsets** (native alignment widens the struct). Resolve through owned import metadata, not a cast at DAT offset. |
| `item.joint(model, path).quaternion.xyzw` | Fourth quaternion component getter/setter, retaining the existing xy/z components. `HSD_JObjGetRotation` and `HSD_JObjSetRotation` transfer the whole `Quaternion`; the Arrow history copies all four components, even for non-quaternion joints where w is usually unused. |
| `item.set_joint_transform(model, path, tx, ty, tz, rx, ry, rz, rw, sx, sy, sz, force_dirty)` | Optional bulk operation to apply authored scalar state as a full translate/rotation/scale transform. This avoids 10 separate tree lookups and preserves the stock animation callback's unconditional `WithMtxDirty` setters. Ordinary collision-history updates use non-forced setters; animation trail updates force dirty. A direct model-joint primitive is useful generally; it must not own history, delay, visibility, or trail decisions. |
| `item.contact_fighter` and `targets.is_fighter(handle)`, `targets.guard_active(handle)`, `targets.model_scale(handle)`, `targets.shield(handle, cx, cy, cz, radius)` | Capture current contact `xCF4_fighterGObjUnk` as a rollback-safe fighter reference. Classification uses `it_80272D40(gobj)==0`; guard uses `ftLib_80086A18`; model scale uses `ftLib_800869D4`; shield center/radius uses `ftCo_80094098`. The script retains the contact target, computes the midpoint angle, and follows the shield. Resolve the same fighter while live; do not substitute the creator or most recent collision target. Invalid references return absent / guard false. Use existing safe fighter-reference infrastructure if available; never save a GObj address as an ordinary int. |
| `hitbox.set_damage(index, damage)` in article callbacks | `void item_set_hitbox_damage(HitCapsule*, u32, Item_GObj*) = it_80272460`. Stock Arrow stores its charge-derived damage in a **u32**, so conversion happens when the script stores that value, before this call. The fighter damage helper is different. |
| `item.suppress_destruction_effects` writable bool | `xDCF_flag.b2`, which suppresses the default destruction effect/sound path in `Item_8026A8EC`. |
| `item.spawn_effect(effect, bone, ox, oy, oz, vx, vy, vz, synchronous, arg)` | Direct `it_80278800`, with authored origin/variance vectors. Its default branch consumes three `HSD_Randf` calls even at zero variance. Arrow expiry uses effect0x421/bone0/zero vectors/synchronous1/arg0. Existing `spawn_root_effect` loses this RNG contract. Existing `spawn_effect_at_position` is also not equivalent. |
| `item.spawn_joint_effect_at(effect, path, ox, oy, oz)` | Direct `efSync_Spawn(effect, gobj, selected_joint, &offset)`. Young Link stage impact uses0x448/root/offset(0,0,5); no randomization. |
| `visuals.held_item_visible` read-only bool | Fighter `x221E_b3` stores the requested held-item visibility state and gates the end-motion one-time group2 restoration. This is an ordinary shared visual flag, not a Link rule. |

The generic bone-attached constructor now invokes `on_created` before pickup,
matching both stock constructors: create item,
set attachment creator/facing, invoke authored `on_created`, call
`Item_8026AB54`, then optional existing held-spec registration/debug setup. Bow
and Arrow both declare `held off`; neither replaces the ordinary held item.
This ordering is also what the accepted center-held Bomb constructor already
uses.

Existing operations cover ordinary scale, commands, lifecycle callbacks,
manual lifetime, lifetime tick, state flags, removal, release attachment with
ordinary-held preservation, ownership queries, fighter world bones, owner motion,
owner item-detachment flag, and model slots. No new Arrow lifetime helper,
vibration primitive, charge primitive, embed primitive, or trail policy is needed.

## Import data

Both fighters have Arrow slot3 (kind attribute offset12) and Bow slot4
(kind attribute offset16). Fighter attributes are max-charge float0,
startup animation-rate float4, air-end landing-lag float8, Arrow kind int12,
Bow kind int16. Arrow scalar attributes occupy offsets0 through32: flight life,
minimum speed, maximum speed, minimum damage, maximum damage, damage-contact
velocity multiplier, embedded life, gravity, flight angle limit. Immutable trail
root declarations refer to console offsets36 and40. Root0 is not a trail clone.
Semantic LeftThumb and RightThumb already provide the attachment joints.

## Stock call order and decisions

Fighter entry clears charge-enabled, charge, visibility timer and all four
commands. Ground entry normalizes ground physics and clears vertical velocity.
It changes to344/347, installs authored damage/death callbacks, sets the imported
animation rate, and starts animation. It then attempts Bow creation at RightThumb.
An existing Bow is a failure too: clear fighter references and enter Wait or the
attribute-selected Fall/freefall. Start animation consumes command2 to enable
charging. Only ground start reapplies the startup rate every animation callback.
Command0 equal to1 is consumed and attempts Arrow creation at LeftThumb when no
Arrow reference is present. A successful update records both thumbs/root and
updates the held Arrow's current/previous position; animation completion enters
345/348 with flags SkipModel|SkipItemVis and starts animation. Loop only updates
positions. Start interrupt increments/clamps charge only after enabled; loop
interrupt sets full charge. Releasing B enters346/349 without calling animation
start. Ground/air continuation preserves the frame with stock collision flags,
reinstalls callbacks, then calls animation start.

End collision launches **before** checking ground/air continuation: when
command1 is true and an Arrow reference exists, consume command1, sample thumbs,
flatten both z coordinates, launch at5 degrees with charge/max-charge, clear the
fighter Arrow reference, preserve/restore the ordinary held item, then notify
item pickup(false). Ground end animation only clears Bow; air end clears both.
Both restore current/persistent group2 once when held-item visibility is
set, then select Wait or attribute-selected Fall/freefall on completion.

Bow constructor: clear commands, disable remove-at-bounds, save creator model
scale, retain creator, pickup. Pickup changes to0 for owner phase0 or3 for phase3
or absent Bow phase6; then advance stored animation/commands and set scale. Other
phases leave motion unchanged. Animation always sets saved scale. End states2/5
remove at frame0 or >=24 and clear the owner reference. Other states synchronize
to the owner's Bow phase0..6; paired ground/air changes save frame, change with
flags2, advance animation/commands, then request the saved frame. Non-paired
changes omit frame restoration. Remove if creator is absent, ownership differs,
or owner is outside344..349 / requests item detachment. State6 animation **and
collision** return true; its physics does nothing.

Arrow constructor: clear commands, set flight lifetime, launched=false,
trail-age0, empty trail slots, retain creator and scale, line=-1, both normal
angles0, pickup; pickup changes to state0 with flags2. Held animation sets scale;
absent creator removes, creator-owned Arrow removes and clears creator when the
owner leaves344..349 or requests detachment. Transferred ownership does not
itself remove this held state in stock.

Launch stores angle, temporary combined lifetime, and charge first. If creator
still owns the Arrow: reset flight lifetime, change1 flags2, perform the stock
unused owner-bone rotation query, release attachment with zero offset, clear
throw-speed-damage flag, switch to manual lifetime, compute speed and truncated
u32 damage, copy creator facing, set root rotationY, set current/previous
positions, set velocity, set root rotationZ, launched=true, initialize four
position/quaternion history entries from previous position (z-=0.02), load two
immutable model roots, reset trail age, and perform initial segment sweep. The
fighter ignores that sweep's result. No initial embed decision belongs here.

Flight animation: set saved scale; set hitbox0 damage exactly once via command2;
after trail age>0 enable command0 and after>1 enable command1; when enabled and
loaded, unhide each model and apply history entries1/2 using forced-dirty setters;
increment trail age; tick lifetime; expiry destroys both trail roots and removes.
Flight physics saves current position as previous then subtracts abs(gravity).
Flight collision computes/clamps facing-relative displacement angle, writes root
rotationZ, performs segment sweep, checks line existence, binds collision line,
gets moving-line speed, copies sweep-normal angle to both saved normal angles,
then enters embed: saved scale, embedded life, Randi(4)+3 vibration index, tail0,
state4 flags2, rotationZ, destroy trails, sound; Young Link adds joint effect0x448.
Finally every flight collision shifts all four authored history entries and
applies current history entries0/1 to the two trail models.

Shield hit: check contact object then classification0; set saved scale,
embedded life, velocity0, state2 flags2, destroy trails, retain contact fighter,
query scale and shield, calculate midpoint(previous,current) angle from shield
center, place on that radius, return false. Otherwise return true. State2 removes
when retained fighter is no longer guarding; otherwise tick lifetime and destroy
trails on expiry. Physics re-queries scaled shield radius and follows the same
stored angle. State3 is inert.

Stage-embedded animation consumes one Randf for indices0..6, computes exactly
(table spread*random + base)*degrees-to-radians, alternating sign, writes
rotationZ, decrements the u32 vibration index, then ticks lifetime. On first
expiry destroy trails, spawn randomized-offset effect0x421 (three extra RNG
calls), collision-mode2; every expired tick hide model, suppress default
destruction effects, increment tail; remove only when tail>300. Embedded
collision shifts trail history first; retain previous normal angle; valid line
binds/query speed/query normal and adjusts stored/root angle only when normal
changes. Missing line-1 zeroes velocity; deleted nonnegative line does not.
Both absent cases remove.

Damage dealt multiplies velocityX by the imported multiplier, destroys trails,
returns true; clank destroys trails and returns true. Reflection destroys trails,
reverses facing and xy velocity, clears z velocity, advances position by reversed
velocity, clears z position, wraps pi+angle using **pi** subtractions/additions
(not2pi), writes root rotationY/Z and translation, returns false. Interaction
uses the existing generic interaction operation. Destroyed clears the creator's
Arrow reference only while unlaunched and creator-owned, destroys trail slots,
clears creator/current-owner/owner-bound.

## Remaining lifecycle audit

The stock damage/death callback additionally removes boomerang, tether, and Young
Link milk. The no-op return query `ftLk_Init_BoomerangExists` has no side effects
and needs no replacement. Full authored Bow cannot delegate cleanup to
`ftLk_800EAF58`; safely exposing/removing those remaining stock-owned references
and clearing their corresponding flags requires a separate generic cleanup
contract or their complete authored article translation. This remains an
activation gate.

A first direct Bow matrix compiled the whole actual `itlinkbow.c` and unchanged
fighter query bodies. It caught and corrected the scratch motion range before
activation: neutral-special motions are **344..349**, after forward tilt and two
appeal states. The corrected matrix ran 8,734 comparisons and found 192
remaining failures, all in synthetic end-state expiry cases where constructor
creator is absent but current owner exists. Stock destruction clears the current
owner's Bow reference; creator-only `owner.call` cannot express that recipient
contract. Transferred-holder end cleanup cases are separately excluded and
unaccepted. No production template or runtime test used the incorrect range. After separating
768 absent-creator and transferred-holder end-cleanup fixtures, the remaining
8,350 actual-stock-C callback comparisons passed exactly. This is partial
article acceptance, not full Bow move acceptance.

`it_8026B894` (Bow interaction) clears an owner matching a destroyed reference;
it does not transfer ownership. Stock `linkbow.x4` is assigned only in the
constructor and is never cleared or replaced by any Bow callback, so the
absent-creator/non-NULL-current-owner failures describe synthetic malformed
ownership rather than demonstrated stock play. Generic attachment `it_802742F4` and reflection
`Item_80269F14` can replace current owner, but stock Bow eligibility for those
paths has not been established. A current-holder call must resolve an authored
recipient with a compatible state layout or dispatch through that recipient's
own handler; it cannot write this module's state offsets into arbitrary fighters.

## Compiled scratch and focused Arrow evidence

Both full owned Link and Young Link scratch candidates import parameters and
compile to DLLs with the coordinated content tool and frozen SDK snapshot.
Each has Bomb, Arrow and Bow article metadata and imports trail roots 36/40.
Generated code contains no donor attribute reads or attribute declarations.
Import-default assertions retain Spin joints 24/26 and Bomb joints 64/68.
Evidence: `.local/link-bow-owned-syntax-20260930/compile-results.json`. No
package is activated or registered and no Bow/Arrow runtime game has launched.
Both packages and DLLs were regenerated together after the double-pi and
signed-zero flight-physics corrections; syntax evidence records their final
source/generated-C/module hashes. They use the frozen SDK toolchain and must
be regenerated against the final shared SDK before runtime acceptance.

A focused fixture compiles unchanged original `itLinkArrow_Logic98_Reflected`
and its trail-destruction helper, the actual generated authored reflection
callback, and the real generic model setter/destructor bodies. The first 216
cases found 192 mismatches: decimal script literals have float suffixes, while
stock `M_PI` / `M_PI_2` constants are doubles. At initial angle 0, stock's rounded
float pi compares above double pi and takes another subtraction; a comparison
against float pi skipped that branch. Existing builtin `pi` already emits an
unsuffixed double constant, so the candidate uses `pi` and `(pi * 0.5) * facing`.
Float state assignments preserve stock rounding boundaries. **No new numeric
API was needed.** The corrected 432 cases pass for both kinds, both facings,
angle/limit/pi boundaries and customized angles, all trail-presence masks,
model dirty flags, destruction order and exact position/velocity/rotation bits.
The vibration degree conversion remains float: stock `MTXDegToRad` explicitly
uses `0.01745329252f`. Evidence: `.local/link-script-port/arrow-reflection-results.json`.

The fixture now includes all substantive Arrow callbacks and legal launch:
**17,114 comparisons pass**. It compiles unchanged original callback/helper
bodies, the actual generated authored bodies, and real generic model
read/write/full-transform/named-root-load/destruction functions. External stage,
target, animation, sound and allocation operations use matched contract stubs.
Coverage includes flight animation/hitbox update/expiry, exact negative-zero
gravity, quaternion history, moving-line embedded collision, shield contact and
following, flight sweeps/angle limits, creation initialization, held ownership
and motion predicates, destruction/pickup/interaction, charge interpolation and
launch poses, plus both families' impact sounds and Young Link's joint effect.
The fixture files retain their original reflection names while this scratch
batch is unregistered.

The initial flight-physics comparison exposed stock `ABS(-0.0f)` preserving
negative zero while `fabsf` clears it. The authored branch adds negative gravity
or subtracts nonnegative gravity, preserving the original resulting velocity
bits without another API. Legal launch tests require creator ownership and
non-null trail descriptors: stock's helper has an undefined return outside those
conditions. These direct cases establish callback decisions under the specified
external contracts, not full runtime move acceptance. Bow cleanup still awaits
the complete Boomerang/Hookshot/milk ports; see
the remaining integration requirements summarized at the end of this document.

## Baseline 4 SDK regression

The unregistered owned packages were regenerated and compiled against the frozen
`.local/baseline4_sdk` and content tool in `.local/baseline4_runtime`. The validated
creator helper now resolves through the real item entity; direct fixtures model
that Host contract. Arrow again passes all 17,114 original-C/generated comparisons,
and Bow again passes 8,350 with zero failures. The same 768 absent-creator or
transferred-owner cleanup cases remain explicitly outside acceptance. There was
no gameplay launch and no Bow/Arrow registration. Hashes and scope are recorded in
`.local/link-script-port/bow-arrow-baseline4-direct-results.json` and the packages'
`.local/link-bow-owned-syntax-baseline4/compile-results.json`.


## Baseline 5 SDK regression

Both owned script sources were regenerated by the frozen baseline5 content tool
and compared with the unchanged original callbacks using baseline5 headers and
compiler. Arrow passes 17,114 comparisons and Bow passes 8,350 with zero failures.
The same 768 absent-creator/transferred-owner terminal cleanup cases remain outside
acceptance. No gameplay or registration occurred. Evidence and hashes are saved
in `.local/link-script-port/bow-arrow-baseline5-direct-results.json`.

Boomerang's unregistered fighter/article source fixture now passes 15,452
comparisons; see the Boomerang proposal for precise fixture boundaries and the
remaining Hookshot/milk/recipient-event proposals. This advances the ordinary
handle constructor dependency without claiming closure of the full damage/death
cleanup gate.

## Recipient callback proposal (historical baseline6)

Baseline6 proposed recipient exports and source lifetime protection to close
these cases. Both are implemented in baseline8; see
[recipient callbacks](scripting/functions.md#exporting-a-recipient-callback) and
[synchronous lifetime](scripting/synchronous_callbacks.md) for the current API.
The following checkpoint closes article cleanup without promoting the full fighter.

## Baseline 8 recipient cleanup acceptance

The accepted recipient exports and lifetime backend now close the Bow article's
previously excluded cleanup cases. The frozen baseline8 source batch passes
**9,132 Bow comparisons and 17,114 Arrow comparisons, with zero failures**.
All **768 absent-creator/transferred-recipient terminal configurations** are
restored. Four additional policy cases verify that an expired Bow returns true
even when its current recipient has no export, records unsupported cleanup,
and leaves the recipient's entire state unchanged. This safe foreign-recipient
extension prevents a leaked article; it does not cast a foreign fighter's union.

The Bow fixture loads two independent generated DLLs. The author stores its Bow
handle at byte 4; the recipient stores its own handle at byte 20 after unrelated
base and authored state. Dispatch uses the actual `nativeFighterEvent_Call` and
`nativeCallbackSource_Alive` sources from the frozen SDK, with live-list entity
IDs including 16,777,217 and `0x80000001`. The exported cleanup runs against the
recipient's own module and offsets; unrelated recipient bytes remain unchanged.
The original Bow callbacks and their fighter queries are compiled unchanged.
The increase from the earlier 8,350 accepted comparisons is the restored 768
terminal cases plus 14 additional destruction configurations.

The original `ftLk_SpecialN_UnsetFv14` clears the Bow reference, then calls
`ftLk_Init_BoomerangExists` only if both Arrow and Boomerang are absent. The actual
query in `ftlink.c` only returns whether the Boomerang reference exists; this
return is ignored. It performs no callback removal, flag mutation or other
cleanup. The fixture now compiles that actual query body too. Consequently the
recipient export's ordinary `state.bow_handle = -1` preserves the unsetter's
observable behavior without a stock fighter delegate.

Evidence and all authored/generated/fixture input hashes are in
`.local/link-bow-cleanup-baseline8/acceptance.json`. The runner verifies the frozen
SDK/compiler/content-tool inputs against `freeze.json` before and after its
single console batch. The exact full Arrow candidate inputs were:

- `.local/link-bow-owned-syntax-baseline8/Link/behavior.fms`
- `.local/link-bow-owned-syntax-baseline8/Young Link/behavior.fms`
- Each candidate's adjacent `fighter_parameters.fmp`.

Byte-identical copies of those four accepted inputs are preserved under
`tests/native/link_bow_arrow/link/` and
`tests/native/link_bow_arrow/young_link/`. Each contains `behavior.fms` and its
adjacent `fighter_parameters.fmp`; these are the runner's default inputs.
Copy hashes were checked against the final acceptance record without replaying
the source matrix. Reproduce the console batch with:

```powershell
python tests/native/link_bow_arrow_source_compare.py --snapshot .local/baseline8 --output .local/link-bow-arrow-reproduction
```

Optional `--arrow-link` and `--arrow-young` arguments select other authored
inputs. Neither the tracked inputs nor this runner registers a fighter.

These are unregistered owned scratch candidates containing Spin, Bomb, Bow and
Arrow. Their Bow unsetter is now exported and their Bow article dispatches
cleanup to its current recipient through `targets.call`. The durable checked-in
Bow and independent recipient policy sources are
`tests/native/link_bow_article_fixture.fms` and
`tests/native/link_bow_recipient_fixture.fms`; the batched runner is
`tests/native/link_bow_arrow_source_compare.py`. The Arrow callback and legal
launch acceptance boundaries above remain unchanged. The Bow harness does not
exercise its allocation/attachment constructor leaves or the complete fighter
neutral-special phase matrix. No runtime game was launched.

Bow/Arrow remain **unregistered source fixtures**. The actual
`examples/link_clink_wip.fms` remains the accepted Spin/Bomb template. Full
`ftLk_800EAF58` damage/death cleanup removes Boomerang, releases Hookshot linked
resources, removes Arrow/Bow and removes Young Link milk. The scratch fighter
currently owns only Arrow/Bow removal in that callback chain. Boomerang and milk
have separate source evidence but are not yet integrated into its complete
constructors and fighter lifecycle; Hookshot remains unported. Recipient Bow
cleanup is closed, but this full fighter cleanup gate still prevents coherent
promotion of the neutral-special move to the examples.
