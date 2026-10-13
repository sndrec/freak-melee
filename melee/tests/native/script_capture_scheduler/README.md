This console fixture runs the actual grab scheduler against prepared interpreted
owner hooks and physical Fighter/GObj pools. It checks owner/peer callback order,
stock callback takeover, optional item catch, source/victim retirement, and the
generic capture arm/mask, entry, attachment, observations and throw release.
The unchanged common Falcon capture-entry body is compared with the new generic
entry using the same controlled engine leaves. Additional cases exercise all
entry options, stock/scripted victims, motion-table admission and retirement
during native callbacks. Collision candidates, animation/rig initialization,
damage application, audio and item contact are controlled leaves. This does not
prove complete special-move behavior or rendered attachment/throw physics.

The same batch checks the contact-target receiver against stock/scripted
fighters and a stock Item, retained receiver capture, identity observations,
read-only access, caller/recipient generation and physical slot retirement.
It also runs the production action-metadata decoder and action-entry body
against a scripted Luigi taunt classification, including unchanged descriptors,
counter order, common held-item classification, absent query hooks and
retirement during the query. Counter allocation and the held-item predicate are
controlled leaves; the query, interpreter and statistics reset are production
code. This is API/order parity evidence, not full Luigi/Falcon/Ganon admission.

The batch also runs the actual held-item swing selector and dispatch body
against stock Captain's six swing types and five variants. It verifies owned
motion tables, missing hooks, invalid returned motions and query retirement
before a controlled swing entry. The owned partial-animation binding invokes
the production part traversal and blend setup, comparing node order, selected
copy joints and layer fields against a direct native call. JObj animation
evaluation is controlled, so these checks do not establish rendered pose parity.
Malformed trees, absent variants/bones and nonfinite durations/rates reject
before changing the layer or publishing output.

Part selection/show/hide also run through prepared script helpers and the VM.
The actual native hide traversal is compared for costume-only, main-animation
and blended restoration; the inner costume/animation restoration leaves record
their arguments rather than evaluate a rendered pose. The production owned pose
validator checks descriptor records, rig topology, clip spans, partial traversal
and animation-family remapping. Selection/no-op behavior, malformed restoration
assets and seven post-native retirement cases are checked as one cohort.

An ordinary script helper registered in the accessory phase reproduces the
deferred effect marker: installation leaves the flag unchanged, execution sets
it and calls the real binding that installs effect pause/resume hitlag callbacks.
The flag assignment and effect callbacks themselves are controlled leaves.
An explicitly unavailable feedback leaf verifies real compiler/VM/top-level
`effects.spawn` routing and that a failed spawn stops before the flag write.

The same routing check covers `effects.wind` with its typed bone, offset,
integer strength and Float falloff/angle arguments. This fixture does not
execute the wind generator. Attack-segment methods run their actual native
statistics reset and attack-instance operations through compiled source and
the VM, comparing them with direct native calls across condition flags.
The statistics counter, stale-instance allocator and player-recording leaf
are controlled identically in both paths.

The actual common jab-two and jab-three schedulers query an interpreted
`jab_restart` hook with the requested motion ID. Native Marth's repeating
third jab and native Mario's normal third jab provide decision comparisons.
Second-jab restart, missing-hook fallback and retirement before either native
continuation are covered. First-jab entry and item pickup remain controlled.

Run `run.py --compiler <clang.exe> --output <fresh private A: directory>`.

Shield cases run the two actual common entry dispatch bodies with controlled
common initialization. They verify authored post-entry timing and retirement
before presentation; native shield graphics, sounds and collision setup are
not evaluated here. Both actual dynamics-wind paths compare stock Marth's
enabled policy and stock Mario's disabled policy against interpreted queries
across negative, absent and positive global wind. The wind-direction and
bone-operation leaves are controlled; rendering and dynamic-chain evaluation
remain unverified.

Secondary-damage cases extract the exact dispatch branch from the native hit
scheduler, then check the real binding, interpreter, callback takeover,
motion reset and actor snapshot roundtrip. Retirement stops the continuation.
The rest of that hit scheduler, knockback calculation and reaction entry are
outside this projection. Native Fighter and peer state is kept stable while
actor state is restored; this is not a complete match savestate test.

Held visibility runs the actual native common hide/show bodies through the
source binding and interpreter. Stock callbacks and authored hooks are compared
for absent/present held references, unchanged visibility and both transitions.
Retirement inside either hook stops the script and preserves the native
pre-transition visibility flag. The held reference is a controlled opaque
Item GObj; item model changes are not rendered. Attached-article visibility
compiles as a Bool property and executes the real common property backend;
the fixture's property router forwards this selected family while retaining
its existing controlled position-write retirement leaf. Full production
property routing and rendered article visibility are outside this projection.
The actual native attached-object draw eligibility predicate observes the
authored flag change independently of held-item visibility. Unselected common
data remains absent; this fixture does not execute those parameter queries.


The generic capture parameter batch covers all seven native holder settings:
timer damage multiplier/decay, mash reduction/pause/rate and wiggle scale/limit.
It compares 77 property/float-bit combinations against the exact native fields,
including signed zeros, subnormals and NaN payloads, checks whole-Fighter write
isolation, and rejects 56 field/invalid-owner combinations without mutation.
A real prepared Fighter helper writes and reads all seven through the VM;
fixture property routing remains controlled and dispatches to the actual
production capture binding. This is not full victim-timer or rendered-grab
acceptance, and captured Item/enclosure operations remain outside the batch.

The subsequent enclosure-settings batch adds eleven Float32 fields and two
native-width Int fields. The Float32 matrix now covers 198 field/bit-pattern
combinations and 144 invalid-owner combinations; the Int matrix covers both
signed limits, type/range rejection and invalid owners. A prepared helper
writes/reads all thirteen settings, including a Bool expression converted to Int.

Victim visibility and body status run through real owned helpers, capture-pair
lifetime guards and mechanically extracted `ftColl_8007B62C`. Both stock and
owned victims compare all three statuses with direct native calls. Missing
victims, stale generations, malformed arguments and retirement inside the color
update are covered. The color-animation leaf is controlled, so these tests do
not prove rendered invincibility or complete command-grab gameplay. Enclosure
release/shape operations and Yoshi's owned articles remained open at that checkpoint.

The release batch runs `capture.release_into_enclosure()` through the actual
compiler, VM, retained capture helper, shared entry body and hurtbox initializer.
Stock and owned victims cover both starting ground/air states and facings.
Settings are copied by value, original victims survive link clearing, and the
real enclosure callbacks/scales/capsule are installed. Missing model settings
and malformed mappings reject before damage. Source/victim retirement at damage,
air transition, motion change and accessory creation prevents subsequent steps
and the script's continuation write. Native damage, motion dispatch,
accessory loading and JObj constraints are controlled traced leaves, not full
match/geometry evidence. The separate original-C enclosure target compares
the native timer/mash/shrink/damage/escape algorithms on real physical pool
references with controlled stock registration and engine leaves. Actual source
actor ownership is exercised here. Caught-shape/constraint operations, owned
articles and full Yoshi gameplay remain open.

The caught-policy batch adds interpreted `enclose_caught`, `constrain_caught`
and holder-owned `capture_wait_setup` delivery. The complete caught capsule
recipe is compared with `nativeCapture_EncloseCaught` pinned to `d2ba54c30` for
stock/owned victims and three model scales. The actual common scale query and
hurtbox initializer execute. Setup faults and source/program/schema retirement
stop the victim mask; graph-leaf retirement of either actor prevents the
following script-state write. Missing/malformed joints, scales and links reject
before shape/attachment work. The common all-capsule status setter and JObj
constraint leaf remain controlled and traced. This is capsule/dispatch/admission
evidence, not rendered constraint geometry or full grab gameplay acceptance.

The catch/throw hook batch extracts the actual `fn_800D9CE8` and
`ftCo_800DE3FC` scheduler bodies. Owned programs provide typed float
`catch_pull_frame` and void `throw_victim_setup` hooks. The fixture checks
fractional frame publication, standing versus dash branches, stock and owned
victims, missing-hook defaults, callback faults, source/schema retirement,
peer retirement and capture-link changes. Motion selection, animation startup,
JObj constraints, throw statistics and overlay application remain traced
controlled leaves. The native victim body-status setter executes. These are
scheduler/VM checks, not a full Yoshi throw animation or match comparison.

The bone-status batch runs generic `hurtboxes.set_bone` through the owned source
binding and compiled helpers. Eighteen full-Fighter comparisons with the actual
`ftColl_8007B128` setter cover three statuses, duplicate matching capsules and
both initial aggregate flags. Missing/out-of-range bones and empty capsule lists
are no-ops; enabling one capsule retains the sticky aggregate flag and position
update flags. Malformed types/widths/statuses, stale source/program/epoch and
invalid imported rig/counts reject without mutation or result publication.
Joint-table assets remain controlled; this does not establish live Bowser shell
animation, collision detection or whole-fighter gameplay acceptance.


## Generic cue and ECB operations

`cue_cases.h` compares all eight cue selectors over all 256 cue combinations
with a direct named-bit consume recipe, preserving the entire Fighter payload
and unrelated cue bits. Repeated calls return false. Compiled helpers consume
each cue from a state-defined selector. Invalid types/ranges and foreign or
stale source contexts do not mutate or publish. The recipe follows native named
fields; it is not a console execution comparison.

The same cohort extracts the actual common `ftCommon_UnlockECB` body and compares
1024 combinations of frame-count and collision flags with the typed operation,
plus a compiled-helper case and rejected source contexts. No physics tick or
collision query runs during unlocking. These operations do not allocate.

## Common reaction observations and collision groups

`reaction_cases.h` runs the production damage-read bindings and shared item
group allocator on an owned fighter and through compiled interpreted helpers.
The expected values use the actual native `ftCo_8008E984`, `ftCo_ScaleBy154`,
`ftCo_Damage_CalcKnockbackLevel` and `Item_8026AE60` bodies, extracted separately
from their call sites. Samples exercise zero/negative/threshold knockback,
four common scales, SDI/reaction flags and stored thresholds, preserving the
entire Fighter. Group samples cover signed-word boundaries and counter wrap,
with exact post-call counter comparison. No article is created.

Invalid generations, scenes, source classes, absent common parameters and
unowned fighters reject without publishing results or advancing the counter.
Those observation/group cases do not exercise reaction entry, collision
detection or fire-breath construction.

## Common damage entry

`damage_entry_cases.h` compares the actual native damage initializer against
the body pinned at `94abb5792`, renaming only the two entry functions and their
inter-function call. The production binding and compiled helpers run on actual
owned actors. Comparisons cover requested/automatic motion, ground/air,
zero and increasing knockback levels, five angles including 361, all three
facing arguments, whole Fighter payloads and native call order. Default entry
also covers all eight ordinary element values with and without sandbag status.

Player credit, floor-normal angle, color/flash/camera effects, bone rotation,
damage-item cleanup and song/ice/animation initialization are controlled leaves.
Knockback angle/velocity, air-motion checks, shake-threshold selection, damage
entry arithmetic and knockback script dispatch use actual source bodies.
This does not prove rendering, the hit scheduler, SDI or the entire DK path.

Victim motion entry compares sixteen native/interpreter pairs against the common
capture and hold initializers pinned at `5cf0c21d6`. The only reference alterations
rename entry functions and the internal hold-motion call. Stock and scripted
victims, freeze flags, same/different starting motions, exact Fighter payloads,
animation donors, timer truncation, engine order and real compiled helpers are
checked. Invalid signatures, source identities, recipient tables, reciprocal
links and hold timer data must leave output unpublished and actors unchanged.
Actor generation, victim link and published program identity changes at motion
entry and hold movement clearing halt the common continuation. Missing victims
are no-ops. Motion loading, pose evaluation and attachment geometry remain
controlled leaves; these checks do not establish complete cargo/command-grab
gameplay or stock fighter activation.

The shared-throw cohort compares held-pose changes, ordinary thrown-victim entry,
attached throw entry, throw cues and throw movement with native bodies pinned at
`ac62f15ba`. It covers signed/zero playback rates, explicit frames and flags,
initial-update options, facing and current-frame helpers, simultaneous release /
reversal / pause cues, missing victims, and both captured actor classes. Exact
Fighter payloads and engine order are compared; script helpers execute through
the real compiler/VM. A separately extracted actual release-finish body runs
through the real damage initializer and a controlled DI leaf, proving that actor
or program retirement there prevents the following DI call. Release damage and
damage entry in the main cue matrix remain controlled leaves. Ordinary ground/air
movement and animation-rate propagation are controlled leaves as well. This
cohort does not validate throw collision geometry, full DI, rendered animations,
whole-fighter activation or complete release gameplay.

Retirement injections at initial damage bookkeeping, motion entry and initial
animation change actor generation or the published program identity. Each
halts continuation without result publication. A real interpreted knockback
hook is also faulted through the controlled position-write boundary; the native
initializer stops before its final ice initialization. These are lifecycle
checks, not changes to healthy gameplay math.

The finishing/release batch executes the production state and capture bindings.
Ground/air finish and DamageFall entry are compared with bodies pinned at
`f3acd8313`; ordinary release compares the original exporter's common damage /
launch sequence. Both actor classes, absent victims, ground/air victim state,
collision-credit and upward options, compiled helper calls, invalid arguments
and callback retirement are covered. DamageFall runs its actual native body;
parasol choice, drift clamping, ground/air finish entries, throw-hit application
and launched damage remain controlled leaves. These checks do not prove full
throw damage, collision-credit target lifetime, rendered gameplay or whole
fighter activation.

The drop-victim batch executes the actual common unlink body and compares it
with `c090df153`, retaining the original model restoration and floor/collision
branching. Both holder/attachment orientations, both victim ownership classes,
absent victims, ground/air state, floor lookup/connectivity/height outcomes and
fallback landing results are covered. Stage queries and world-pose samples are
controlled leaves; no real stage collision or rendered pose is claimed.
Identity changes during Fall entry stop the caller VM. The separate
animation-facing property checks preserve native Float bits and execute through
the actual observation binding and compiled helper for ordinary values.

The walk batch compares entry, classification, animation-rate policy, walking
physics and animation-facing ground movement with native bodies pinned at
`8010badfe`. Whole Fighter payloads are compared through both direct bindings
and compiled helpers. Signed speeds, slippery surfaces, floor normals, zero
acceleration, reverse facing and invalid motion/rate arguments are covered.
Actor/program retirement during entry prevents subsequent native continuation.
Acceleration, friction arithmetic and slope projection use the actual native
bodies; floor material multipliers, motion entry and animation evaluation are
controlled leaves. This does not establish rendered walking or activation of
the complete Donkey Kong or Bowser source packages.

The input-query batch compares walk, stop, turn, platform-drop and jump input
with bodies pinned at `6a69c522d`. Stick thresholds, facing, tilt timers,
pass-through floor flags and profile jump bindings are varied. The actual
platform classifier executes; only its floor line flags are controlled.
Queries must preserve whole Fighter payloads, publish matching typed results
and run through compiled helpers. Common throw-threshold reads preserve exact
Float bits. Captured-victim animation tags use actual native owned-asset lookup
versus the pinned body, including stock/scripted/absent victims and rejected
stale relations. No rendered stage, jump launch or complete cargo flow is claimed.

The jump/platform cohort executes the production jump and movement bindings,
comparing short-hop detection, jump launch, the first-frame physics gate and
platform-drop entry with bodies pinned at `95d8528b3`. Typed startup preparation
is compared with the original authored field-assignment recipe. Ground/air,
motion/frame/flags, floor skip, released/held/profile jump input, signed/zero
launch multipliers and worn-item sound requests are varied. Whole Fighter
payloads and engine order are checked through direct calls and compiled helpers.
Motion-change retirement must stop native floor/timer updates and the following
VM statement. Audio, ground-to-air clearing, drift clamping and subsequent air
physics are controlled leaves. Native launch arithmetic, short-hop decisions
and floor-skip assignment execute. This does not prove rendered platform drops,
the whole airborne physics path or complete DK activation.

The attached-capture cohort compares the original initialization body pinned at
`fc4b737ec` with the production binding and compiled interpreter helper. It
varies stock/owned victims, both actors' ground/air state, facing, victim freeze,
captured poses and capture timers. Whole Fighter payloads, animation donor and
engine call order must agree. Retirement of either actor or the published
program at eight boundaries stops further VM writes. Invalid arguments, stale
capture links and mismatched victim tables must fail before mutation. A separate
observation matrix gives the holder and victim different freeze flags, ensuring
the captured-victim query never substitutes the holder's value. Cleanup, bone
attachment, motion entry, pose evaluation and ground-to-air clearing remain
controlled leaves; this is not complete Bowser or Mewtwo gameplay acceptance.

The damage-tick batch compares the common timer and physics bodies with
`2be04ddb3`. Timer boundaries, hitstun flags, held hammer/non-hammer/no-item,
recovery policies, gravity, terminal speed, friction, velocity and slopes are
varied. Actual airborne-hitstun and grounded movement math executes through
both owned bindings and compiled helpers. Retiring the actor or published
program inside knockback-exit prevents hammer recovery and the next VM write.
Normal airborne physics and knockback-exit callbacks are controlled leaves;
this does not validate full damage collision, authored exit-hook behavior or
the complete DK cargo flow.


The attachment cohort runs the actual compiler, VM, actor Load scope, attachment
binding and native attachment lifecycle. It checks selected skin and fallback
costume root lookup, bone binding, retained draw lists, cloth descriptor ranges,
mask ownership, body-chain preservation and repeated cleanup. Visibility schemas
and nested imported spans, malformed text and stale contexts reject before
allocation/output publication. Source retirement during JObj loading prevents
subsequent scripted cloth setup. DAT root lookup executes, while graph import,
JObj construction, material/visibility initialization and cloth allocator leaves
are controlled. The render pass is not executed; this is not rendered costume
or cloth parity evidence.

The shield-break cohort compares common entry against the body pinned at
`b091887fa`. Ordinary and top-blast KO policies run through the compiled owned
hook and actual damage-flag binding. Missing owned hooks do not dispatch stock
fighter handlers. Physical actor or published program retirement at six engine
boundaries stops continuation; retirement inside the authored hook prevents
subsequent effects and sound. Common reset, motion/animation setup, effect,
audio and hurt-status leaves are controlled. Callback scheduling and launch
assignments execute. These 17 cases do not establish rendered shield-break
effects or full Jigglypuff gameplay parity.

The throw-presentation extension executes the actual common throw entry and
invincibility initializer through the ordinary source compiler/actor/VM.
Its 62 cases cover four motions, stock/owned victims, three rates, absent
hooks, callback faults/retirement, both reciprocal links and generations,
and retirement at holder/victim motion and animation boundaries. Callback
observation requires complete victim setup and precedes invincibility.
Native holders (Mario, Samus, Kirby) are compared by whole fighter bytes and
observer order against the exact entry body pinned at `55124e067`.

The grant and decrement logic also expose a native portability bug: console
MSL `bool` is `int`, while native C bool normalized the throw-invincibility
duration to one. The field is now an explicit signed countdown, and 41 cases
check zero/one/multiple frames, timer expiry, existing intangible conditions,
overlay presence and preservation of longer grants. The actual fighter tick
slice is extracted; only its overlay-clear call is redirected to a controlled
observer. Full fighter ticking and rendered throw accessories are not proved.
Unselected newer statistics/rebound/shake bindings abort if reached.

The transient-accessory extension compiles the exact authored
`Tether_CreateThrowAccessory` helper from the Samus constructor component.
It compares its VM execution with the current native constructor body (only
entry names changed): 144 combinations of four throws, three scales, three
rates and geometry loop flags. Six retirement cases stop at construction,
base/variant animation loading, frame request, evaluation and follow setup;
12 invalid runtime calls preserve output and perform no engine operation.
Eight additional cases test both loop policies. Actual HSD ForeachAnim and
rate/flag helpers execute; material and texture clocks stay independent.

This cohort supplies controlled descriptor records and observes runtime graph
construction, animation loading/evaluation and constraint installation.
Retail recipe import/provenance is separately checked by
`native_fighter_character_assets`, using the real DAT/HSD importer. Neither
cohort proves rendered accessories, complete Samus activation or live solver
acceptance. The former anonymous stock payload is normalized into the generic
`FtAccessoryAsset` type; the scripts call no stock Samus special handler.
