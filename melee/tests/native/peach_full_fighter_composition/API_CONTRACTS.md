# Remaining versus decision boundaries

This is a source census against frozen48, not an API commitment or independent
fighter declaration. `inventory.json` pins the source files, exact retail motion
slots, references and the immutable accepted components. Only this namespace is
changed. No stock fighter handler is called by the composed scripts.

## Already composed

The setup/load/death/presentation/CPU/dynamics/aerial-jump policies, complete
Turnip construction/phases and ordered cleanup, successful Parasol producer,
forward smash349..351, pluck352/353, counter365..368, Explode, Toad and Spore are
now in one inherited candidate. There are 34 authored custom phase pointers;
80 nonnull retail phases remain inherited. State356 stays entirely null.

The five article declarations have 13 rows: Explode2, Turnip5, Parasol3, Toad2,
Spore1. Parasol's row count is complete but its animation phases/common flight
contracts remain incomplete; row count alone does not prove authored coverage.
The named cleanup helpers use typed recipient results and full item handles.

## Float common entry and availability

`ftpeachfloat.c:ftPe_8011BA54` tests stickY<=-common.x88 and held XY, then
availability, entering Float with timer reset. `ftPe_8011BAD8` tests descending
selfY<=0, availability and stickY>=tap-jump threshold or held XY, then enters
with reset. Both return whether an action was entered. Their exact ordered
call sites are Fall136/142, Jump188/191 and JumpAerial346/356. Jump's motion
variable gate and aerial-jump command0 gate belong to the callers and must stay
where they are. A generic optional aerial-action hook must run at each original
boundary, returning Bool and stopping that caller's remaining IASA work. It
must preserve source lifetime after a callback. Existing `on_aerial_jump` is an
entry override, and cannot substitute for these checks.

`fighter.c:Fighter_ChangeMotionState` resets Float availability in grounded
motion setup, before later callbacks (line1213 in frozen48). A generic ordered
grounded-transition event can reset the ordinary owned Bool. Existing landing
events miss grounded transitions and occur at a different phase.

The Float states341..343 need an owned timer, current release state, and real
DAT tuning at offsets8 and12. Existing load policy reads clip18/19 lengths and
already stores both end frames. Float entry changes341 without `animation.start`,
consumes availability, optionally initializes timer, zeroes vertical velocity,
marks effects active and asynchronously spawns particle1236 on mapped TransN.
Direction exit selects342/343 using common.x78 and starts at the stored clip end
minus the imported offset. This is ordinary state/math/particle policy; it needs
no native Float helper.

## Float aerial attack selection

`ftpeachfloatattack.c:ftPe_8011BE80` uses the common C-stick edge predicate
`ftCo_800DF478(Fighter*)`, and the selector
`ftCo_AttackAir_GetMsidFromCStick(Fighter*)`. The selector is called once for the
65/66 gun test, and again in `ftPe_8011BF34` for an ordinary Float attack. Preserve
both calls and priority; do not cache them into a differently ordered policy.
The gun branch requires actual held-item use category `itGetUseKind()==3` and
common aerial-gun entry `ftCo_800CDDA0(gobj)`. Other attacks enter selector+279,
344..348, resetting the script-owned release latch.

These direct common queries/actions are missing from frozen48. A generic
current-held use-kind query is distinct from ItemKind identity. The C-stick edge
query is distinct from ordinary attack selection; exposing previous C-stick
coordinates is another valid implementation if the entire threshold test is
authored faithfully. The ordinary shared item-throw predicate
`ftCo_80095328(gobj,false)` is also needed during interruptible Float attacks.
Existing `controls.aerial_attack_actions()` binds `ftCo_800CB870`, the aerial-jump
check, and cannot stand in for any of these four operations.

`ftCo_AttackAir_EnterFromMsid` clears interrupt permission, command0 and throw
flags, changes the requested state with KeepFastFall, then starts animation.
Those operations are expressible directly. Float attack animation decrements
the retained timer, consumes reverse cue and chooses Fall versus Float without
timer reset. Interrupt latches release once; physics chooses the exact common
aerial motion body versus Float drift. `physics.air_accel_normal()` already
binds the actual `ftCommon_CalcSelfAccel_Drift`, so it is not a missing API.

Float collision is `ft_800831CC(gobj,ftCo_80096CC8,ft_80082B1C)`. The existing
special-landing primitive shares the same collision body but supplies a fixed
special landing callback. A general named authored landing callback accepted by
this actual collision contract would cover Float and Parasol. Float aerial
attacks use `ft_80082C74(gobj,ftCo_LandingAir_EnterWithLag)`; its motion344..348
fallback to basic landing must be preserved. A generic AttackAir landing action
can directly bind that common callback if existing common landing methods do
not already cover it.

## Side-B recipient rebound

All six live Side-B states354/355/357..360 require 24 phase pointers.356 is null.
Existing state-change binding, Int smash age, command bits, motion transitions,
root position, grounded acceleration, air friction/deceleration, named callback
binding and the accepted Explode constructor cover most of this source. Use an
owned Bool for the smash decision and imported attributes48..108. Start and end
transition masks must be Int values, not untyped Float locals.

The missing boundary is `ftpeachspecials.c:doAirEnd0`. If contact flag x221C_b5
is false, rebound proceeds. If true, the current `unk_gobj` recipient's existing
authored `on_body_rebound` policy is consulted; the native stock fallback permits
Marth/Roy motions369/371 and Peach365/367, rejecting other live fighters. Null
recipient/userdata follows the original fallthrough; stale/recycled pointers
need a separate safe policy rather than dereference. A generic validated contact
rebound predicate should implement that actual recipient dispatch, retaining
source/recipient generation across callbacks. No raw pointer should escape.
The script then owns velocityX=0, nonnegative velocityY=0, command2=1 and end
transition. Exposing just a target kind is insufficient.

Ground/air collision transition callbacks continue into ceiling/wall tests after
changing state; they do not return early. The initial ground launch position
adds -4*facing*instanceScaleY and +3.5*instanceScaleY. Explosion reads raw part4,
sets Z=0, constructs before replacing XY velocity, and installs the final facing
callback. Both x21F8 bodies can be ordinary authored facing/velocity policy; no
fighter-specific helper is needed. Allocation/lifetime failure has already
defined generic boundaries and must stay distinct from healthy source parity.

## Parasol travel and actual-held article protocol

Four private travel states361..364 contribute16 phase pointers. Their producer
is already accepted and must be reused. Entry clears commands0..2/throw cues,
sets song immunity and requests a new article only if the actual held item is
neither ordinary nor owned Parasol. Startup consumes command2 even though the
helper always returns false. On animation completion, source explicitly changes
the *previous motion recorded by common open entry* to FallSpecial; common
`ftCo_800CEFE0` uses blend10, common.x59C timer, and ignores its Int duration
argument (including Peach's DAT offset144). A generic common open-entry contract
should accept the previous-motion selector explicitly; mutating raw motion ID
from a script as a workaround would violate ordinary state lifecycle.

Travel steering/root rotation, imported damping/gravity, saved/restored Y and
freefall are existing generic operations. Startup steering's `1.0` denominator
is double in the source and must be checked against generated arithmetic in a
real comparison rather than hand-waved. The actual collision body chooses
ft_80083B68 for non-command flight/upward flight and ft_800831CC with authored
special landing for descending command flight, else grounded standard collision.

`ftGetParasolStatus` uses the actual held item's state: ordinary engine Parasol
retains its original seven-case mapping; owned Parasol state1=>4, state0/2=>6.
`ftCommon_8007E83C` computes frame rate using the actual article's animation
length15/16, fighter rate and divisor, then opens/closes that same held article.
Its raw native callback table delegates to `it_802BDD40`/`it_802BDDB4`, and
`it_802BDA40` selects the length via the article state table's animation ID.
Future generic named article status/value/action policy must target the actual
held article, not `state.parasol_handle` (which can identify a different item).
Typed fighter recipient exports do not provide typed article recipient calls.
Dispatch-success Bool cannot return an article's status or duration.

The complete article animation calls a current-holder-owned inactive predicate,
which permits travel361..364, owned369/370, ordinary common Parasol fall/open,
and common special falls. LandingFallSpecial additionally tests actual held
article state2 and frame>=16. A live held article state/frame query or a typed
read-only article policy is needed; arbitrary saved transport state is not an
equivalent query. Article open/close can be authored with existing state/rate,
sound0xF7 and named empty accessory callback once common dispatch is generalized.

The common 369/370 pointers can be bound by `motion_policy` after dependencies
are closed. `ftCo_ItemParasolGetFallMotionId` currently selects them using raw
Peach kind plus private Parasol pointer. Use a generic package-owned motion
query at this same selection boundary, allowing script-owned live handle state.
Do not infer this selection merely from actual held kind; stock conditions are
different. Mushroom resume `fn_800D2890` must also route saved motion370 to
special Fall without the raw Peach equality check.

## Common lifecycle decisions still needing routing

Landing writes common allow-interrupt, then clears the air-counter boost and
conditionally cleans Parasol. Existing `on_landing_action` is later and the
allow-interrupt argument is not exposed. A generic ordered landing policy with
that Bool supplies the exact decision. Neutral entry `ft_8008A348` cleans the
owned held Parasol after down/hammer/air gates before Wait. An optional ordered
neutral-entry policy must preserve that location and be lifetime checked.
The fighter Parasol timer expiry branches on actual owned item kind before
special Fall; it must honor the resulting owned motion/article policy too.

Stock Bowser/Giga down-throw selects the Peach/Zelda victim animation by raw
kind in `ftCo_Throw.c`; independent Peach needs the intended animation-family
contract for this stock-opponent case. Bowser's already-authored
`on_throw_victim_motion` solves the attacker-owned case but cannot alone repair
a raw stock Bowser opponent. This is a common animation metadata decision, not
permission to call Peach handlers.

## Already owned metadata and nonblocking limits

The registered inherited Peach base now exists in frozen48. It explicitly owns
53 retail PlCo semantic bindings,114 physical joints, zero omission entries,
five article slots0..4, effect bank15, sound bank22, demo root names and the
Peach animation family. The prior activation proposal's "no Peach base" claim
is historical, not the current state. `inventory.json` hashes the current base.
Normal clone/import now works and real raw-DAT parameters are byte-verified.

CPU ranged candidate38/range32, attack motion>343, dynamic set exclusions/first
three elements, custom aerial jump and all nonnull versus load/death/item/
knockback callbacks are already script-owned. No additional Peach CPU recovery
branch exists in this census. The debug-ROM taunt's model group2 toggle is behind
DbLevel>=DebugRom; it is an explicit optional debug-only policy, outside versus
activation. No private-union EF updater remains: Float/pluck/Spore effects are
the already-supported particle constructor families. Video-only demo handling
and Kirby copy behavior remain deferred.

The Spore final raw debug-owner notification source-invalidation boundary is
still controlled in the accepted counter fixture. Original undefined accesses,
failed allocations, stale handles and invalid rigs are not healthy retail parity
references; keep their authored safety cases separately counted.
