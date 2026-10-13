# Interpreted capture registration, entry and release

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

`capture.arm(mask)` now arms the existing common grab detector. It requires
prepared `grab_catch` and `grabbed` events on the capturing fighter and accepts
an exact Int mask in 0..32767. The optional `item_catch` event handles item
catches. This adds no stock-fighter forwarding or generated machine-code
callback pointers.

The actual collision scheduler invokes the owner catch event first, retains
the existing source/victim generation checks, and then invokes the owner
`grabbed` event. Both receive a physical-pool/lifetime-checked victim token and
use the holder's own program/storage. Running holder code against the victim's
script state would be incorrect, particularly with stock or unrelated custom
victims. Common grabs which install native function pointers take precedence;
no extra mutable registration flag is needed.

The native `ftCommon_8007E2D0` body still writes the detector and mask fields.
Arming validates the current source package, motion layout, program, storage
and physical lifetime before mutation, and retains the existing post-operation
publication guard. The exclusion operation `capture.mask` remains separate.

`capture.begin(motion, same_facing, force_air, track_position, holder_animation)`
now enters the selected victim through the common cleanup/link/motion/mask/
animation/movement sequence. The holder owns the script. The victim motion is
admitted against its own imported tables, including owned custom states for
scripted victims. A reciprocal link is not required before entry. Both actor
lifetimes are checked after each callback-capable step.

`capture.attach_to_victim` names the actual rig attachment instead of the old
misleading `victim_to_air` spelling. `capture.follow_victim`, `clear_movement`,
`release_throw`, readable `has_victim`/`victim_damage`, and writable Bool
`ground_link` are implemented. Throw damage intentionally unlinks the pair;
the continuation retains the original victim identity for the common thrown
response and stops if either actor retires. The reusable engine code lives in
`native_fighter_capture.c`; the exporter host calls it directly, without a
forwarding wrapper or second copy of the algorithms.

## Evidence

The tracked `tests/native/script_capture_scheduler` console fixture passes
2,280 checks in 108 cases. It runs the real source compiler, hook ABI preparation,
Fighter actors, interpreter, receiver-token handling, physical pools, complete
grab scheduler and native arm/mask bodies. Cases cover scripted and stock
victims, a stock holder, native callback takeover, item-catch dispatch, skipped
processing, holder/victim retirement, invalid mask types/ranges and missing
required hooks. It also compares the unchanged common Falcon capture-entry
body with the new generic entry, covers every entry-option combination,
stock/scripted victims, victim payload reads, attachment/following, observations,
unlink-then-release ordering, motion-table rejection and retirement during entry
or throw damage. Collision candidates, animation/rig initialization, damage,
audio, item contact and package assets are controlled leaves. This does not prove
rendered attachment, throw physics or complete match behavior.

Receipt: `A:/FreakMelee-test-archives/script-capture-entry-release-ready-oct10/receipt.json`.
SHA-256: `dfe0db887a1c193fd2168121d928b57bfab3df7654f1c85928c1a75ef30dbd1b`.
Native client, content tool and source family/package integration targets build.
No installed content changed and no game windows were launched.

## Stock source boundaries

Falcon/Ganondorf now call the existing `state.landing_special(false, lag)` API
directly, matching `ftCo_LandingFallSpecial_Enter` in the original callbacks.
Their command-1 assignment explicitly converts the native Float attribute to
Int, matching the original assignment's truncation. No compatibility aliases
or weakened conversion checks were added.

Fresh read-only source preparation against the private independent census
clones now stops at:

| Source | First remaining boundary |
| --- | --- |
| Captain Falcon | Hurtbox-detect target object properties, line 529. |
| Ganondorf | Hurtbox-detect target object properties, line 533. |
| Fox | Unsupported entity metadata, line 1170. |
| Falco | Entity declaration lacks owned asset binding, line 1158. |
| Marth / Roy | Native binding, line 130. |
| Donkey Kong | Effect joint property, line 55. |
| Luigi | `event.move_id` action-metadata payload, line 319. |
| Yoshi | `callbacks.track_accessory(Up)`, line 162. |

Receipt: `A:/FreakMelee-test-archives/source-generic-facing-cohort-ready-oct10/receipt.json`.
SHA-256: `e5a6207a31fae8c20ae1133f378d0189547e7ae7f40d8167d20754991c87aa05`.

The nine old `effects.spawn_with_facing` calls in Falcon/Ganondorf, Fox/Falco,
Donkey Kong and Marth/Roy sources now call the existing generic
`effects.spawn_on_joint_with_scalar(effect, raw_joint, facing.direction)`.
The original helper passes a raw joint and the facing Float to `efSync_Spawn`;
the current generic ABI admits those exact effect signatures without a new
specialized binding. The direct form-feedback fixture passes 67,459 checks in
135 cases and compares all 65,536 IDs with actual dispatcher argument use.
Its effect leaves are controlled, so it is not rendered feedback acceptance.
Other than Falcon/Ganondorf, the source checks stop before these changed call
sites; their source migrations are still incomplete.

Feedback receipt: `A:/FreakMelee-test-archives/script-facing-scalar-proof-oct10/receipt.json`.
SHA-256: `fc553ff0e26f1dbf6ac2fdd482fa0b0bbbfc895941f551935a90ec9e6d97344a`.

All nine checked fighters are still not interpreter-admitted or gameplay-complete.
Remaining fighter migrations, paired interactions and compact whole-match
rollback snapshots also remain in scope. The goal stays active.

## Contact targets and action metadata

Fighter callbacks now expose `target`, the selected native body-contact actor,
using the existing retained receiver system. Read-only `is_fighter`, `is_item`,
`is_effect` and `kind` observations validate class, physical generation, native
identity and any interpreted view before publication. Identity reads admit
stock Items without pretending they are Fighters. Fighter projections and
authored state still require an appropriate Fighter receiver. No new callback
signature, raw pointer API or fighter-specific target property was added.

The interpreted `action_metadata` integer query now runs in `ft_800895E0` after
the existing counter update and before common held-item classification/statistics.
An absent hook preserves the incoming canonical descriptor. Generic pure
`action.metadata(move_id, category, animation_flags, statistics_flags)` and
`action.move_id(metadata)` expose the word convention without host-layout
dependencies. Luigi reads the incoming classification through the latter;
the obsolete `event.move_id` spelling was removed from this source and docs.

The capture/contact/metadata batch passes **2,777 checks / 150 cases**. Its
production entry body compares stock Luigi classification with an interpreted
taunt query, covering retained original words, counter order, held-item
classification, query absence and callback retirement. Production contact
receivers cover stock/scripted Fighters, a stock Item, retained identity,
physical slot reuse and read-only access. Collision candidates, rig/animation
initialization, damage/audio leaves, counter allocation and the held-item
predicate remain controlled. This is API and ordering evidence, not full
fighter or rendered gameplay acceptance.

Receipt: `A:/FreakMelee-test-archives/script-contact-metadata-final2-oct10/receipt.json`.
SHA-256: `f3e77d1c80ad16ac785bfb768c7ade28c2a1537c20d7d86988c1961d9e778873`.

The native client, content tool, source-family and source-package targets build.
Read-only nine-fighter source checks now stop at Falcon/Ganondorf's old
`state.transition` spelling (lines 538/542) and Luigi's Float-to-Int missile
charge assignment (line 351). The other six cohort results retain their
previous boundaries. None of these nine sources is yet interpreter-admitted.

Source-check receipt: `A:/FreakMelee-test-archives/source-contact-metadata-cohort-final-oct10/receipt.json`.
SHA-256: `f43bfdef6e075c29bbcaa5af69b5ad30898d291810c4b21a8c671f5850677e2f`.

Installed assets were not changed and no game window was launched. Remaining
fighter admission, gameplay comparisons and compact whole-match rollback
snapshots stay in scope. Goal active.

## Explicit entries and numeric conversions

Falcon/Ganondorf's four side-contact transitions now use the existing
`state.enter_at(motion, flags, frame)` directly. Their editable owned parameter
`side_contact_transition_flags = 0x0c4c5082` reproduces the exact mask in
`ftcaptainspecials.c` and the two stock clone definitions. This removes the old
generated-definition lookup implicit in `state.transition(motion, Side, frame)`;
no compatibility spelling or new engine primitive was added.

Luigi's Green Missile setup now explicitly truncates the Float charge-rate
attribute into its Int frame count and the Float misfire denominator into
`random.int`'s Int input, matching the stock C assignment and `HSD_Randi` call.
The authored charge variable stays an ordinary Int fighter variable.

Read-only source checks pass these sites and now stop at Falcon's
`item_swing_motion` scheduler contract (line 610), Ganondorf's subsequent native
binding (line 583), and Luigi's animation helper (line 366). Other cohort
boundaries remain. This checks source admission and stock call/mask/type
semantics; it does not establish full runtime move parity.

Receipt: `A:/FreakMelee-test-archives/source-explicit-entry-cohort-oct10/receipt.json`.
SHA-256: `cc10948c4453f16eb0148428833eaf82ae9ca6e496ed978d357cde0028a0db2d`.

All nine checked sources still have admission work remaining. Goal active.

## Held swings, partial animations and deferred effect helpers

The interpreted `item_swing_motion` query now runs in the real shared swing
selector for variant 3. Ordinary variants retain their common animation table.
Both paths require the source's own motion tables; a missing hook, absent
motion or retired source cancels before swing-rate lookup and entry dispatch.
No stock fighter policy is borrowed after an authored query.

Generic `visuals.apply_part_animation(group, variant, duration)` now applies an
owned partial-body animation through the native traversal and blend setup.
The five groups and eight variant slots match the existing importer. Admission
checks the owned tree, copy joints, omission table and native traversal depth
before mutating a pose. The third argument is blend duration, not playback
speed; zero and finite negative values retain native semantics. Invalid
descriptors, nonfinite resulting rates and unavailable assets reject first.

Luigi's deferred effect marker is now an ordinary helper registered through
`callbacks.accessory`, keeping the effect flag unchanged until that phase runs.
His four immediate launch cues use `effects.spawn` followed by an explicit
effect-active assignment. The top-level interpreter routing for `effects.spawn`
was missing despite its existing catalog and implementation; it now reaches
the generic feedback backend. No specialized Luigi binding was added.

The expanded production batch passes **3,307 checks / 202 cases**, including
stock/source swing selection, source retirement, native part traversal/blend
comparison, inserted/omitted bones, invalid assets and deferred helper timing.
Actual compiler/VM routing reaches an explicitly unavailable feedback leaf and
stops before the following flag write. JObj animation evaluation, effect flag
assignment and effect callback leaves remain controlled, alongside the earlier
collision/damage/audio and bookkeeping leaves. This does not prove rendered
partial poses or complete fighter gameplay parity.

Receipt: `A:/FreakMelee-test-archives/script-swing-part-owned-omissions-oct10/receipt.json`.
SHA-256: `2d5b36e1fa15bcd259a97c9862a81d0431cbba7a590d0480c871ab1f074961ed`.

The native client, content tool, source-family and source-package targets build.
The final read-only cohort passes full source preparation for Falcon (92
functions / 1,845 instructions) and Ganondorf (93 / 2,051). These are source
checks, not published packages or gameplay acceptance. Luigi now stops at
`hitbox.active` on line 429. Fox/Falco still require entity metadata/assets;
Marth/Roy stop at line 130, Donkey Kong at effect-joint metadata on line 55,
and Yoshi at line 162.

Source receipt: `A:/FreakMelee-test-archives/source-swing-part-cohort-owned-oct10/receipt.json`.
SHA-256: `61f5cece1898038fdae0975969383297364aed5a2cd164a78fd4e98b2e0ff0f6`.

Installed packages were unchanged and no game window was opened. Full fighter
comparisons, the remaining interpreter migrations and compact whole-match
snapshots remain open. Goal active.

## Accessory, hitbox and attack-segment interpreter checkpoint (October 10)

The retained Luigi, Marth/Roy and Pikachu/Pichu sources now test
`hitbox.state(slot) == Hitbox.enabled`, using the existing native-state API
instead of adding a boolean alias. Luigi passes full source preparation.
Marth/Roy release feedback is an explicitly installed one-shot accessory helper:
it attaches to raw part zero, marks the shared effect flag, installs hitlag
callbacks and clears itself. Startup/hold no longer inherit release feedback.
Roy's existing effect IDs are preserved as editable parameters. Counter
cleanup clears the pre/post-hitlag and accessory slots explicitly.

Yoshi's Egg Throw and Ground Pound now install owned accessory helpers at the
native phases. Egg Throw finishes into Wait on the ground and Fall in the air;
Ground Pound creates its stars only after entering the landing phase. Its
pickup restriction uses the existing writable property. This is source-backed
timing migration, not a complete Yoshi gameplay claim.

Generic `stats.refresh_attack()` and `stats.record_attack()` now dispatch to
the common native statistics reset and fresh attack-instance operations. The
script chooses their order and timing. `effects.wind` also reaches its existing
feedback backend. Model-effect selectors admit all three Euler rotation axes.
No fighter-specific scripting API was added.

Grouped evidence:

- Native client, content tool, source-family and source-package targets build.
- Hitbox source/VM versus native damage operations: **11,344 checks / 3,250
  cases / 3,207 native comparisons**. Receipt
  `A:/FreakMelee-test-archives/script-hitbox-predicates-oct10-final/receipt.json`,
  SHA-256 `bf8d88417f5a4a303c88fc4dc1cdb2252e9c34962fc34a14e9ea52dfc536c3dc`.
- Connected effect construction/scheduling/XYZ rotation and actor-state
  restoration: **5,207 checks / 26 cases**. Receipt
  `A:/FreakMelee-test-archives/script-effect-rotation-xyz-oct10e/receipt.json`,
  SHA-256 `438fbf6349ac3df5a85a265c8dc9b2fae846a4502ee3d08b6151e62c66b822d7`.
- Capture/metadata/partial-animation/accessory/attack-segment batch: **3,773
  checks / 235 cases**. Receipt
  `A:/FreakMelee-test-archives/script-accessory-stats-wind-oct10-final/receipt.json`,
  SHA-256 `30d420ebc301e995a07377da537286d51b34edb05e489084e0211a157f8ed0df`.

Hitbox geometry/collision, effect model loading/drawing, player bookkeeping
leaves and whole-match snapshots remain outside these fixtures. The routing
checks for effect spawn/wind use explicitly unavailable feedback leaves; they
do not claim generated wind or rendered cues. No game window was opened.

The read-only nine-fighter source cohort passes Falcon, Ganondorf and Luigi.
Marth/Roy now reach the unmigrated `jab_restart` scheduler contract. Yoshi
reaches its facing-ground/ledge query, DK its secondary-damage callback, and
Fox/Falco still require entity schema/assets. Receipt
`A:/FreakMelee-test-archives/source-accessory-stats-cohort-oct10b/receipt.json`,
SHA-256 `4094ee5d4acacd657c9d134f3bb52845d237525994233c65b4eaa345fdecc107`.
These are preparation checks, not publication or complete match parity.
Installed packages are unchanged. Goal active.

## Common jab-restart query checkpoint (October 10)

The interpreter now prepares `on jab_restart` as a Bool query receiving the
requested motion ID. Both common jab-two and jab-three paths call it before
choosing a restart or ordinary follow-up. Missing hooks use the ordinary
follow-up; faults and retirement stop before native continuation. Stock
fighter branches remain available to stock fighters, while source fighters
use their own authored policy.

The expanded grouped fixture passes **3,854 checks / 242 cases**. It compares
native Marth's repeating third jab and native Mario's ordinary third jab,
checks both second-jab outcomes, missing-hook fallback and retirement from
either query. Native item pickup, first-jab entry and motion initialization
are controlled leaves; full rendered jab parity is not claimed. Receipt
`A:/FreakMelee-test-archives/script-jab-restart-oct10-final/receipt.json`,
SHA-256 `dc4e345180d2228ad8b2aee8132b28e51da8f66780d16c3bc4319cfcef528e24`.

All four native targets build. The final read-only source cohort still passes
Falcon, Ganondorf and Luigi; Marth/Roy now reach `shield_start`. Unsupported
Void hooks now report the hook name and source location rather than a generic
line-zero error. Other cohort boundaries are unchanged. Receipt
`A:/FreakMelee-test-archives/source-jab-restart-cohort-oct10-final/receipt.json`,
SHA-256 `e630b302594c8a92d2d3aa27ac6c64665b87fb2aa45eb5697c192d4852a1fefb`.
Installed content is unchanged; no game window was opened. Goal active.

## Shield, dynamics wind and secondary damage (October 10)

Interpreted `shield_start` and `shield_reflect_start` hooks now run after
their respective common native setup, retaining the actor across that setup.
Retirement stops before authored presentation. The two native dynamics-wind
paths now use the package's Bool `dynamics_wind` query instead of opting every
interpreted fighter out; missing hooks opt out and a fault or retirement stops
the continuation. Full replacement shield transitions remain unmigrated.

Generic `callbacks.secondary_damage(helper_or_None)` owns the native
secondary damage slot. It runs in the knockback-applied branch after damage
and knockback calculation, before the usual reaction entry. Motion resets,
native takeover, callback identity and actor snapshots include this slot.
DK's punch binds an ordinary cleanup helper and explicitly clears secondary
damage and effect hitlag callbacks when appropriate. No stock DK handler or
fighter-specific primitive is used.

The retained DK predicate now reads the exact capsule state and accepts every
non-disabled state, matching both stock punch callbacks. Yoshi's Ground Pound
uses `collision.check_ground_ledge(facing.sign)`, preserving the native
negative-versus-nonnegative facing convention without a compatibility alias.

Grouped evidence:

- Native client, content tool, source-family and source-package targets build.
- The expanded actual scheduler/binding/VM batch passes **4,118 checks / 262
  cases**. Receipt
  `A:/FreakMelee-test-archives/script-shield-wind-secondary-oct10-final/receipt.json`,
  SHA-256 `f52a3be27f415e35f30e977d38a4fe6532bd60a2485dc3f2725059e69dc94507`.
- Read-only complete-script preparation now passes Falcon, Ganondorf, Luigi,
  Marth and Roy. DK next needs live hitbox damage observations; Yoshi next
  needs model scale restoration. Fox/Falco still need owned entity schema and
  asset bindings. Receipt
  `A:/FreakMelee-test-archives/source-shield-wind-secondary-cohort-oct10b/receipt.json`,
  SHA-256 `a916c0b219bff062064e633d3574a235444cbb1fec96fcfeea9e7bd05e661caf`.

Shield common initialization, graphics and sound are controlled in the fixture.
Wind direction and bone operations are controlled while the actual two
schedulers and interpreted queries run. Secondary damage checks project the
exact dispatch branch, not the entire hit scheduler or knockback/reaction
calculation. Actor snapshots restore code/state while native Fighter and peer
state remains stable. These checks establish neither full rendered fighter
parity nor a complete match savestate. Installed content is unchanged; no game
was launched. Goal active.

### Live hitbox damage observations

`hitbox.damage(index) -> float` now reads the current native capsule damage
through the same retained source Fighter and bounded index contract as the
state query. It returns already-scaled/staled damage without recomputing it.
DK's existing charge logic can capture that value before subsequent setters;
its original Float-to-Int conversion remains explicit in the script.

The expanded hitbox frontend/preparation/VM/backend suite passes **11,682
checks / 3,419 cases / 3,367 native comparisons**, including 160 exact damage
observations across all slots and capsule states. Invalid queries leave output
and Fighter state unchanged; signed zero, subnormal, infinite and NaN bits are
preserved. Geometry, collision and charged-move gameplay remain unverified.
Receipt `A:/FreakMelee-test-archives/script-hitbox-live-damage-oct10/receipt.json`,
SHA-256 `bdd655f5efde9f9b8d6ae083a2a065c400910926d6e539396850a5375734b406`.

All four native targets build. The nine-fighter read-only source cohort still
passes Falcon, Ganondorf, Luigi, Marth and Roy. DK now reaches its deferred
model-effect construction call; Yoshi still needs model-scale restoration and
Fox/Falco entity schema/assets. Receipt
`A:/FreakMelee-test-archives/source-hitbox-live-damage-cohort-oct10/receipt.json`,
SHA-256 `159aa3a03eb906779e5f361b7428db2d3d5d6c03181096b6c4efdf1c252d7ef6`.
These are preparation and isolated backend checks, not whole-fighter or
aggregate savestate acceptance. Installed content is unchanged; goal active.

### Rendered root scale

Generic `bones.root.scale_x/y/z` observations and `bones.root.set_scale(x, y, z)`
operate on the actual rendered GObj root with native class/extent admission and
retained Fighter/program/storage/rig guards. This is separate from instance
scale and semantic TopN selection. Yoshi's roll scale capture, temporary squash
and cleanup now use these operations directly rather than an obsolete model
vector spelling. The native JObj setter preserves matrix-dirty policy and does
not rewrite the Fighter's gameplay scale.

The grouped source/VM/native bone suite passes **2,084 checks / 112 cases / 70
native comparisons**, including 16 new root-scale comparisons and guarded
class/lifetime negatives. It uses real physical pools and unchanged native
matrix kernels with controlled source rig graphs and class/RObj continuation
injection. Complete Yoshi roll, rendered deformation and whole-fighter
gameplay remain unverified. Receipt
`A:/FreakMelee-test-archives/script-root-scale-oct10c/receipt.json`, SHA-256
`b36107dbc6191271ec5d36186032dfdee99afd49c5d64acabd4dcd8501f61b2e`.

The fixture dependency list now links the real Item animation descriptor
preparation unit required by the current source-program loader. It does not
replace that preparation with a success stub. Invalid root/class test inputs
are restored before native fixture cleanup; the production guard does not
fabricate a valid root for them.

All four native targets build. The read-only nine-fighter preparation cohort
continues to pass Falcon, Ganondorf, Luigi, Marth and Roy. Yoshi next reaches
held/attached visibility controls; DK still needs declared deferred model
effects, and Fox/Falco their entity schema/assets. Receipt
`A:/FreakMelee-test-archives/source-root-scale-cohort-oct10/receipt.json`, SHA-256
`bfc0197d1dd9ec77e8c4729b6630c97e52c8986ad4eec0419282db325f61fdb6`.

The deferred-effect inspection confirms the stock request queues a bone
reference and samples world position during the effect phase. Authored source
effects currently support immediate root construction only. The remaining
queue path must retain program/creator/rig identity through flush and clear;
an immediate root spawn would change the original timing. Scripting docs now
mark those unmigrated reference forms explicitly. Installed content unchanged;
no game launched. Goal active.

### Held and attached presentation visibility

`items.set_held_visible(bool)` now invokes the actual common held-item
visibility operation through an owned source/lifetime guard. Authored
visible/invisible hooks run synchronously; retirement prevents both the
post-callback latch write and helper continuation. Repeated values and absent
held references follow native behavior. `articles.attached_visible` is a
separate writable Bool for the native two-slot attached-object drawing gate;
Yoshi's callers now use the property directly, with no compatibility alias.

The expanded capture/scheduler batch passes **4,370 checks / 272 cases**.
Its eight stock/source visibility rows cover absent/present held references,
both previous values and both requested values. Two additional rows retire
the source during either visibility transition. The actual native attached
draw-eligibility predicate observes the independent Bool gate.
Receipt `A:/FreakMelee-test-archives/script-held-presentation-oct10b/receipt.json`,
SHA-256 `2b5dc8b7662a0119ce9513401a09efba1b5bc58aefc796611bedc01eb7b09a2c`.

The fixture forwards this selected property family to the real common backend;
the full property router is not exercised by this projection. Held references
are opaque Item GObjs, with no rendered item models. These checks establish
binding, callback and draw-gate behavior, not complete Yoshi gameplay or a
whole-match snapshot. All four native targets build.

The nine-fighter read-only source cohort still passes Falcon, Ganondorf, Luigi,
Marth and Roy. Yoshi now reaches its position/scalar effect call at line 307;
stock inspection confirms that its scalar is the common effect attribute
`co_attrs.xBC.size`, not model scale. DK still needs authored deferred model
effects; Fox/Falco require their entity schema/assets.
Receipt `A:/FreakMelee-test-archives/source-held-presentation-cohort-oct10/receipt.json`,
SHA-256 `6a64ea8f865365e2b1e5ba99ed8c47329427dd3522aa5a58c1840fea55de487e`.
Installed content is unchanged; no game launched. Goal active.

### Common effect-size observation

`model.effect_scale` now reads the common effect attribute's first Float,
independently of model scale. Yoshi's egg-break cue uses that exact scalar
through the existing `effects.spawn_with_scalar_at_position` recipe family;
the old source call spelling is removed. The native call samples bone 4,
then supplies its world position and `co_attrs.xBC` to effect `0x4CF`.
That recipe consumes the first Float of the attribute block, not a model
scale multiplier. No fighter-specific operation or compatibility alias is
introduced.

The actual property/frontend/bytecode/VM suite passes **34,918 checks /
9,716 property observations / 46 semantic expressions**, including 129
independent effect-size samples, parsed execution and wrong-context/read-only
rejections. Receipt
`A:/FreakMelee-test-archives/script-common-effect-scale-oct10b/receipt.json`,
SHA-256 `d64078ee72f90c79035f38a8c16a4954c27cfc4c6e82b891186f44f5b7f7d871`.
The fixture does not construct/render the egg-break effect; its unused
held-presentation operation explicitly aborts rather than simulating it.
The prior literal effect ABI inventory admits `0x4CF` in the position/scalar
family. The native four-target rebuild passes.

The grouped read-only source check retains its five passing fighters. Yoshi
now reaches grouped material-animation frame requests at line 327; DK's
deferred authored effects and Fox/Falco's entity schema/assets remain open.
Receipt `A:/FreakMelee-test-archives/source-effect-scale-cohort-oct10/receipt.json`,
SHA-256 `4f56e5f59e5df5c9b825517a06b93d038c66cd2988039910edb7ee3df2ef84f8`.
Installed assets remain unchanged. Neither full Yoshi gameplay nor aggregate
rollback snapshot acceptance is established. Goal active.

### Grouped material-animation control

Both `visuals.set_group_material_frame(table, group, variant, frame)` and
`visuals.initialize_group_material_animation(table, group, variant, prior_end)`
now execute through interpreted source bindings. The first requests then
evaluates each selected material; the second freezes its AObj rate and returns
the shared end frame. Channel 2 uses the native secondary draw list. Missing
channels/nodes preserve native behavior, while invalid imported selection data,
nonfinite frames and inconsistent lengths are rejected before mutation.

The operations retain the owned Fighter/program/storage/rig, both draw lists,
selected lookup and material/AObj identities. Each node is checked before use
and after its native leaf, with whole-selection scans bracketing the loop.
The traversal is linear and uses bounded stack storage without allocation.
Retirement or graph replacement stops subsequent evaluation and leaves the
result unpublished; previously evaluated nodes are not rolled back locally.

The expanded common-visuals suite passes **18,014 checks**, retaining 2,245
existing native comparisons, 544 current-selection comparisons and 774 queries.
It adds **24 material comparisons / 26 retirement or replacement cases**.
Receipt `A:/FreakMelee-test-archives/script-group-material-animation-oct10-final/receipt.json`,
SHA-256 `d05403410ddc77a4cc6f9e504a32a90da5d3a37f7853ab4caad669efb34d8c0d`.
The shared native material helpers and AObj rate statements are used as the
reference; material request/evaluation leaves record calls rather than render
pixels. Imported data, actor views and lifecycle injections remain controlled.
This fixture does not execute source frontend/VM or real material interpolation.

The full capture/scheduler fixture still passes **4,370 checks / 272 cases**;
its unrelated material leaves explicitly abort if called. Receipt
`A:/FreakMelee-test-archives/script-material-scheduler-link-oct10/receipt.json`,
SHA-256 `374e730019e9fccf28dc18187af265000859bb46092d59e5f3dcda9c11fd5627`.
All four native targets build. Yoshi also now uses canonical raw-joint rotation
names, preserving the stock calls' numeric selectors, and Float locals for its
fractional squash/slope multipliers.

The nine-fighter source-preparation cohort retains its five passing fighters.
Yoshi's next gap is the writable horizontal smash-input age at line 368;
DK's authored deferred model effects and Fox/Falco entity schemas/assets remain
unfinished. Receipt
`A:/FreakMelee-test-archives/source-material-animation-cohort-oct10-final/receipt.json`,
SHA-256 `0cca4617b1556501a7070663de137d748a2c1e3311e5d138caad28c7fc15e1f8`.
Neither full rendered Yoshi parity nor compact whole-match rollback acceptance
is established. Installed content is unchanged; no game launched. Goal active.

### Input history and repeat-hit groups

`input.horizontal_smash_age` is now writable in interpreted source. Native
Yoshi explicitly consumes/reset this shared byte in its Egg Roll start logic.
The binding accepts the native input scheduler's range 0..254 and validates
the original scalar width before conversion, so a wide integer cannot wrap
into a valid age. It does not rewrite stick coordinates or neighboring timers.
The actual property/parser/VM suite passes **39,039 checks**, including all
255 timer values for signed/unsigned 32/64-bit input and invalid-context/value
rejections. Receipt
`A:/FreakMelee-test-archives/script-input-history-write-oct10/receipt.json`,
SHA-256 `6c97bb5cfd1a2d73af57a178ed8019f2db2392bb0bf13053dbea68eb30c5cf5f`.
Per-frame input scheduling and Egg Roll gameplay are outside that fixture.

Generic `hitbox.group(index) -> uint` and `hitbox.set_group(index, group)` read
and write the native repeat-hit tag. Group assignment retains native UInt32
narrowing and changes neither victim history nor activation state. Slot indices
remain bounded without truncation. Yoshi's predicates now explicitly compare
`hitbox.state(0) != Hitbox.disabled`, matching its stock code's treatment of
non-disabled states instead of an article-only enabled query.

The actual source/compiler/prepared actor/VM/native backend hitbox batch passes
**12,661 checks / 3,586 cases / 3,849 native pairs / 3,890 helper calls**.
New cases cover all four slots and five states, high-bit group tags, alternating
group policy, native signed/wide narrowing and invalid/lifetime inputs. The
comparison retains entire Fighter storage, including victim records.
Receipt `A:/FreakMelee-test-archives/script-repeat-hit-groups-oct10b/receipt.json`,
SHA-256 `21a5b58994982746b7c41471d5f6c009e3f0abeae8ca86d36b876a857eb34b8e`.
Collision-contact scheduling and rendered gameplay remain unverified.

All four native targets build. The nine-fighter read-only cohort retains its
five passing sources; Yoshi now reaches oriented floor dust at line 507. That
native recipe takes a copied world position followed by two scalar arguments,
which the current typed feedback API does not yet admit. DK's deferred authored
effect path and Fox/Falco's entity schemas/assets remain open. Receipt
`A:/FreakMelee-test-archives/source-repeat-hit-groups-cohort-oct10/receipt.json`,
SHA-256 `d064fe509e72d5b2a00302582185f396ac0fdc68515abd75275ef790a5ecf152`.
Installed content is unchanged. These checks do not close full-fighter or
compact whole-match rollback acceptance. Goal active.

## Generic position plus two-scalar native effects -- October 10

Interpreted fighters can call
`effects.spawn_with_two_scalars_at_position(effect, x, y, z, first, second)`.
The API describes the argument shape; scalar meanings belong to the selected
native recipe. Currently audited recipe `0x3FF` copies position, reads facing
by value and immediately copies Z rotation. Its common constructor does not
read the creator's model root. Other three-pointer recipes remain rejected.
Yoshi's floor-dust call now uses this generic API and retains native Float32
`atan2_float`; no Yoshi-specific primitive or stock-handler fallback was added.

The literal ABI fixture passes 524,631 checks across all 65,536 effect IDs,
including unsupported-ID output preservation and 48 actual helper-copy cases.
The feedback fixture passes 1,341 checks: its new cases check exact ordered
argument copies, malformed/nonfinite inputs, other recipes and ownership before
and after construction. Nine post-spawn retirement cases preserve result
publication without undoing an already completed construction. Its constructor
leaf is controlled, while pointer consumption is independently source-audited.
Unselected creator-root and owned Item wind dependencies abort explicitly.
This is backend and source preparation evidence, not rendered effect acceptance.

All four native targets build. The nine-source read-only cohort retains five
passing sources; Yoshi advances to legacy `state.finish` at line 520. DK's
deferred authored model effects and Fox/Falco entity metadata/assets remain
open. Installed content is unchanged. Complete fighter gameplay and compact
whole-match rollback acceptance remain unfinished; goal active.

Evidence:

- `A:/FreakMelee-test-archives/script-position-two-scalars-oct10/receipt.json` -- SHA-256 `64f3dc08920b0b29cf554402cff3e75019d681e77e49d2a3f46451ae8c12d8e2`.
- `A:/FreakMelee-test-archives/script-feedback-two-scalars-oct10d/receipt.json` -- SHA-256 `0a67f0edd07d12da5fa5c4defea811b06f61929bc6c563437a9593281d0ba4c0`.
- `A:/FreakMelee-test-archives/source-two-scalars-cohort-oct10/receipt.json` -- SHA-256 `83bd728ad4d42e73a818a99f56c450d4d7784a09a9fd16f10c463d97f7f03715`.

## Native floor-material friction and common-state migration -- October 10

`floor.friction` is a generic read-only Float32 property for Fighters and owned
Articles. It calls the unchanged native `mpLib_800569EC` lookup on the current
collision cache. The low flag byte selects one of twenty stage-material
records; upper collision flags do not affect selection. Invalid material
indices reject before lookup. Source generations, epoch, program and storage
remain checked around result publication. No Yoshi-only primitive was added.

The connected observations cohort passes 17,408 checks / 1,369 cases / 2,801
native comparisons / 1,557 VM calls / 576 receiver VM calls. It uses the actual
native lookup and material record layouts with two controlled stage tables,
all twenty entries, four upper-flag patterns, Fighter and Article VM reads,
unchanged payload comparisons and stale/malformed/write rejection. Full loaded
stage assets and Egg Roll gameplay are not covered. Unselected attack-accounting
leaves abort explicitly; their separate scheduler evidence remains unchanged.

Yoshi's obsolete `state.finish()` calls now use existing `state.wait()`: the
stock `ft_8008A2BC` enters `ft_8008A348(actor,0)` for non-boss fighters, exactly
the generic wait primitive. Its wall effect uses the existing position/scalar
API. A local acceleration scalar was renamed to avoid shadowing the engine
`acceleration` namespace. No compatibility alias or stock callback was added.

All four native targets build. Five sources still pass whole-source
preparation; Yoshi advances to its missing cached ECB-left read at line 605.
This is preparation evidence, not full runtime acceptance. The next batch
includes cached ECB components and controller feedback. DK deferred authored
effects and Fox/Falco owned entity schema/assets remain open. Installed content
is unchanged. The broader project goal remains active.

Evidence:

- `A:/FreakMelee-test-archives/script-floor-material-friction-oct10b/receipt.json` -- SHA-256 `10941a02164b111bb5e0fedaff6d43d24bf55066e9b7f019f55820af05ac7e92`.
- `A:/FreakMelee-test-archives/source-floor-friction-cohort-oct10/receipt.json` -- SHA-256 `fba9efd3b9f5abb4f2b22717d4cb12b25b805ac0afb78d417a7b6b3e0327b5f7`.

## Complete cached Fighter ECB coordinates -- October 10

The generic `ecb.top`, `ecb.bottom`, `ecb.left` and `ecb.right` namespaces now
each expose read-only X/Y Float32 coordinates. The missing six components read
the same actual current collision cache as the existing top/bottom Y values.
They are offsets, not world positions or a collision recalculation. These
properties are Fighter-context only; no species-specific API, compatibility
alias or tick-time allocation was introduced.

The actual property and typed frontend batch passes 39,619 checks / 9,804
property observations / 70 semantic expressions, retaining its previous root
scale and input-history cases. New coverage checks all eight vertices with
distinct neighbor values, whole-Fighter preservation, signed zeros, subnormals,
infinities and NaN payloads. Writes, selectors, stale generations and Article/
Effect contexts reject. This proves cached read contracts, not collision solver
or rendered wall-bounce behavior.

The four native targets build. The nine-source cohort retains five passing
sources; Yoshi advances to `controller.rumble` at line 612. Its wall-effect
position and existing Float32 rotation expression now prepare through the
canonical position/scalar effect signature. DK deferred authored model effects
and Fox/Falco entity metadata/assets remain open. Full fighter acceptance and
compact whole-match rollback remain unfinished. Installed content is unchanged;
the project goal remains active.

Evidence:

- `A:/FreakMelee-test-archives/script-complete-ecb-oct10/receipt.json` -- SHA-256 `b7bcb4e901b6399f3e7cefce1947b21554af5b713257e4145f4a2f2d4a40177f`.
- `A:/FreakMelee-test-archives/source-ecb-cohort-oct10/receipt.json` -- SHA-256 `ba7873072690b6d95e66167b38ab48b648b471e6bd28fb42bc1a9e5eca50dfe4`.


## Generic Fighter controller feedback -- October 10

`controller.rumble(pattern, duration)` now dispatches the shared Fighter
rumble primitive. Arguments are integral native-width pattern/duration
values, not a fighter-specific effect or a strength. Native player eligibility,
request ID, loaded pattern priority/commands and zero-duration one-shot rules
are preserved. Eligible requests validate loaded patterns and controller slots;
ineligible requests preserve the native successful no-op. Owned physical,
program and storage identity is retained across the request.

The connected compiler, actual Fighter helper/VM and feedback batch passes
1,752 checks / 512 native comparisons. New coverage includes 480 Fighter
pattern/duration/eligibility comparisons against the unchanged native helper,
native-width conversion, malformed/context rejection and seven post-request
source/program/storage retirements. Hardware queue and pattern/player data are
controlled; this is not physical-controller or rendered gameplay acceptance.
The fixture's unavailable binding list was refreshed for the current catalog.

All four native targets build. The read-only nine-source cohort retains five
passing preparations; Yoshi now reaches the missing grounded-box edge query
at line 632. DK deferred authored effects and Fox/Falco entity ownership/schema
remain open. No installed content changed; full fighter and compact whole-match
rollback acceptance remain unfinished.

Evidence:

- `A:/FreakMelee-test-archives/script-fighter-rumble-oct10c/receipt.json` -- SHA-256 `ff57e1d596cb0036deb74189709f59780221a1de4edbd10be3d101b977328bd5`.
- `A:/FreakMelee-test-archives/source-controller-rumble-cohort-oct10/receipt.json` -- SHA-256 `a7b7c988e536672f0057b38c08cfa1df56c9db1125abf8acce5c2baa266e6135`.


## Generic grounded edge-support box query -- October 10

`collision.ground_box_edges(top, bottom, left_x, left_y, right_x, right_y)`
now selects the native `ft_80082978` mode-2 query used by low-speed rolling
moves. Existing ground/air queries keep their original native selection. All
three now retain physical/program/storage identity through collision, require
six finite Float32 coordinates and preserve failed output on retirement.
No fighter-specific policy, compatibility alias or tick allocation was added.

The native body/frontend/VM batch passes 1,108 checks / 60 custom-box comparisons.
Its old simplified box mirror was removed in favor of the unchanged native
function. Controlled geometry captures query mode and all six coordinates;
whole-Fighter comparison checks position/cache side effects. Negative/positive/
zero facing, asymmetric boxes, supported/unsupported contact, nonfinite input,
malformed/context rejection and seven post-query identity retirements are
covered. Attachment metadata is controlled for this cohort; policy hooks remain
unavailable. This is not stage geometry, actual actor scheduler or gameplay
acceptance. The current fixture catalog dependency closure was refreshed.

The four native targets build. Five source preparations remain passing; Yoshi
advances past rolling collision to capture-item observation at line 767. DK
deferred authored effects and Fox/Falco entity metadata/assets remain open.
Full fighter acceptance and compact whole-match rollback remain unfinished;
installed packages are unchanged and the goal remains active.

Evidence:

- `A:/FreakMelee-test-archives/script-ground-box-edges-oct10h/receipt.json` -- SHA-256 `6225057b1c18c549e3d40e4e1a458f16978e248fbb4de7332455738440aec71c`.
- `A:/FreakMelee-test-archives/source-ground-box-edges-cohort-oct10/receipt.json` -- SHA-256 `0da0dde009416a98d7f4a9b29ce1e238973a526a61ee7c535c99f1c77ff78880`.


## Generic capture tuning fields -- October 10

The interpreter now reads/writes the seven existing native holder capture
settings: damage-scaled timer initialization, timer decay, mash reduction,
mash pause/rate and wiggle scale/limit. Each is an ordinary named Float32 field,
not a fighter-specific helper, DAT offset or configuration tuple. Native float
bits are preserved; source physical/program/storage admission is unchanged.
No new gameplay algorithm or tick-time allocation was introduced.

The actual owner/compiler/actor/VM capture suite passes 4,932 checks / 405 cases.
New coverage includes 77 setting/float-bit combinations, whole-Fighter write
isolation, 56 invalid-owner/property combinations, malformed-type rejection and
a prepared script helper that configures and reads all seven. Physical pools
and actor execution are actual; the fixture's property router, package assets
and earlier collision/animation/damage/audio leaves remain controlled. This
does not prove victim timer progression or complete scripted capture gameplay.

All four native targets build. The previous read-only cohort's five passing
sources remain the preparation baseline. Yoshi still needs captured-item
observation/release, enclosure bindings and owned article integration. Those
larger paths are not made complete by exposing these holder settings. DK and
Fox/Falco gaps and compact whole-match rollback remain open. Installed packages
are unchanged and the goal remains active.

Evidence:

- `A:/FreakMelee-test-archives/script-capture-settings-oct10b/receipt.json` -- SHA-256 `7ed56fa37571cd2005082e5f68f4cece6c604756b265a102d4a67e267dc22870`.

## Captured-item native integration (2026-10-10)

Captured Items now retain their authored program/storage and lifecycle hooks
while the shared native state setup and animation/physics/collision schedulers
suppress ordinary behavior. The new capture-inert flag belongs to native Item
state; the actor's existing phase records preserve the suppression without a
second script-owner record or snapshot format. Native constructors initialize
the flag. No fighter-specific special handler or tick-time allocation was added.

The shared attach/release helper retains actual Fighter/Item physical,
program/storage and rig identities across native boundaries. It validates raw
bone access before touching the model. Release clears the old holder link before
destruction, so a destruction callback's replacement target survives. Native
Yoshi/Kirby callers use the helper's result and no longer clear that link again.
The historical reference SDK signatures match the helper's Bool return.

Actual Item scheduler/VM lifecycle evidence passes 353 checks / 31 cases,
including thirty inert ticks, retained storage, actor-phase snapshot restoration,
authored destruction and unowned native phase suppression. Native Item state is
restored explicitly in this actor-only snapshot probe. The connected world
fixture passes 2,889 checks / 528 native comparisons: sixteen new capture cases
compare four bones, stock/owned Items and both cleanup options against frozen
native attach/release ordering; eight boundary mutations stop invalid attach
continuations, and a replacement target survives release. Graph constraints,
position transforms, statistics and destruction leaves in that helper fixture
are controlled; actual scheduler destruction runs in the separate Item fixture.
Neither suite establishes actual rendered capture geometry or whole-match save/load.

All four native targets build. Script-facing captured-item bindings are the
next integration step; enclosure/owned article integration and full Yoshi
gameplay remain open. No installed package or source-cohort result changed.
Compact aggregate rollback remains open and the goal stays active.

Evidence:

- `A:/FreakMelee-test-archives/script-capture-inert-oct10e/receipt.json` -- SHA-256 `fc41e19a67c267289479244842bbd4011e1d70f65efecf85dbeae3618d499685`.
- `A:/FreakMelee-test-archives/script-capture-item-oct10f/receipt.json` -- SHA-256 `e092caaecb8a35d02fb7cee80237bfee6f9fb2d349608713205e27016e4d9e7a`.

## Captured-item interpreter bindings (2026-10-10)

`capture.has_item`, `capture.attach_item(raw_bone)` and
`capture.release_item(raw_bone, clear_destroy_type)` now have direct Fighter-only
interpreter catalog/runtime bindings. They call the shared captured-item path,
retaining exact owner/Item lifetimes and validated rig access. Missing-item
release is a no-op, attachment requires a selected Item, and malformed inputs
or inconsistent relations leave failed outputs unpublished. No private union,
fighter-specific special handler, new package format or tick-time allocation
is involved. The historical C reference emitter no longer clears a replacement
capture link after the helper already unlinked the old item.

The connected world fixture passes 3,034 checks / 528 native comparisons. New
coverage executes prepared attach/release helpers through real Fighter/Item
actors and the VM, observes stock and owned Items, tests missing-target behavior,
rejects fourteen argument/type/range combinations and fifteen invalid-source/
relation combinations, checks read-only assignment and stops a release after
source retirement. Its property router delegates the selected capture read to
the actual backend; graph/position/statistics/destruction helper leaves remain
controlled. Actual Item scheduler destruction/phase snapshots are established
by the preceding batch, not by this helper fixture alone.

The existing victim/settings scheduler still passes 4,932 checks / 405 cases;
its unselected captured-Item leaves now abort explicitly instead of pretending
to provide Item coverage. All four native targets build. The read-only nine-source
cohort retains five passing preparations (Falcon, Ganondorf, Marth, Roy, Luigi).
Yoshi advances from line 767 `capture.has_item` to line 788
`capture.enclosure_model_slot`. DK's authored bone effects, Fox/Falco entity
schemas/assets, Yoshi enclosures/owned articles and full gameplay acceptance
remain open. Installed content is unchanged; compact aggregate rollback is
still open and the goal remains active.

Evidence:

- `A:/FreakMelee-test-archives/script-capture-api-oct10c/receipt.json` -- SHA-256 `6f78139c819a70741b3a084d4689743348c858ecd861897439ec89050fe5ee92`.
- `A:/FreakMelee-test-archives/script-capture-item-settings-oct10a/receipt.json` -- SHA-256 `33754f5b43ab309af18bc918155ab6e329f9a2146739befcdde529116ededaa7`.
- `A:/FreakMelee-test-archives/source-capture-item-cohort-oct10/receipt.json` -- SHA-256 `7ed04362423b5f2e1df4da9044623e5bdce40bb5e8e68d205da70b817e8a17a3`.


## Enclosure settings and victim presentation (2026-10-10)

Thirteen generic enclosure settings now have direct ordinary-interpreter
read/write bindings: model slot, launch/escape velocity components, damage mode,
shrink fraction/duration, lifetime, damage-time scale and escape invincibility.
They use the existing Fighter-owned settings, Float32 bit preservation and
native-width Int validation. `capture.victim_visible` and
`capture.set_victim_body_status(status)` operate on a retained linked victim;
body status uses the existing common collision/color routine. Missing victims
are no-ops (visibility reads false); inconsistent links fault. No fighter-specific
API, new format, native special handler or tick-time allocation was introduced.

The actual compiler/VM/capture suite passes 6,132 checks / 687 cases. The matrix
includes all enclosure settings, whole-Fighter write isolation, both Int limits,
invalid ownership, stock/owned victim statuses, interpreted hide/show helpers,
absent victims and retirement inside the color update. Property routing and
color-animation leaves remain controlled; this is not rendered grab acceptance.

Captured-Item comparisons moved out of the enclosure fixture's obsolete stack
actors into the connected actual-lifetime Item fixture. Its shared attach/release
path now compares directly with both original Yoshi and copied-Kirby routines,
rather than a frozen earlier helper. The suite passes 4,074 checks / 544 native
comparisons, including 32 captured-Item cases. Native graph/position/statistics
and teardown leaves remain controlled. The separate enclosure target still
passes 19,320 exact original-C samples of the common algorithms; that evidence
does not establish callback lifetime safety in the release/entry path.

All five requested native targets build, including the enclosure reference
target. The read-only source cohort retains five passing preparations (Falcon,
Ganondorf, Marth, Roy, Luigi). Yoshi now advances to line 813
`capture.release_into_enclosure()`. Fox/Falco entity schemas/assets, DK authored
bone effects, safe enclosure release/entry, caught-shape operations, Yoshi owned
articles and full gameplay acceptance remain open. Installed content is
unchanged. Compact whole-match rollback remains open and the goal stays active.

Evidence:

- `A:/FreakMelee-test-archives/script-victim-policy-oct10b/receipt.json` -- SHA-256 `7fd76584217aebadf951b9b7f394749b3c61e0470dd124833f79e2f46c34e1ba`.

- `A:/FreakMelee-test-archives/script-capture-original-items-oct10c/receipt.json` -- SHA-256 `fad3abac5898f53b4eccae4e19e428de5d9a64a4934a75941390c8b8d01c6153`.

- `A:/FreakMelee-test-archives/script-victim-policy-oct10b/enclosure-reference.json` -- SHA-256 `4576d0124a1ef423e180e120bd464e175a59fc38465ec0ae83a4ab8d1dc60d91`.

- `A:/FreakMelee-test-archives/source-victim-policy-cohort-oct10/receipt.json` -- SHA-256 `d733a078e0af7d724f2e607684c63abd07986a5df6387f8c333270674081a1cd`.


## Retained enclosure release and entry (2026-10-10)

`capture.release_into_enclosure()` now runs through the ordinary interpreter.
The shared native helper retains the original pair, snapshots settings/model/
facing, applies throw damage, checks both identities after intentional unlinking
and enters the common enclosure on the original victim. Missing victims are
no-ops; missing models, invalid mappings and retired actors fault without
publishing a successful result. The native host member now returns Bool directly;
no compatibility wrapper or fallback was added.

The common entry also returns Bool, retaining physical/actor/program/storage
identities across air transition, motion selection and accessory creation. Stock
Yoshi and copied-Kirby callers retain their pair around damage and honor failed
entry. A reusable checked semantic-joint resolver validates actor-owned tables,
bone counts and imported motion/parts associations. Release uses it to check
TransN2/root mappings needed by the original throw-damage routine before that
routine can clear the grab links, plus XRotN when detaching an attached actor.
Enclosure entry checks its TransN mapping before and after motion change.
No per-tick allocation or fighter-specific script API was introduced.

The connected compiler/actor/VM suite passes 7,023 checks / 736 cases. The release
matrix uses both stock and owned victims, ground/air starts and facings; checks
settings snapshot isolation, engine callback installation, capsule fields,
scales and operation order; rejects malformed model/rig inputs before damage;
and stops continuation on either actor's retirement at four native boundaries.
The actual shared entry and hurtbox initializer are extracted production bodies.
Damage, motion dispatch, model creation and JObj constraints are controlled
traced leaves. This is not whole-match/rendered enclosure acceptance.

The original-C enclosure target still passes 19,320 exact timer/mash/shrink/
damage/escape samples. It now uses the real physical pool implementation and
joint validation; stock actor registration and engine leaves remain controlled.
Owned actor lifetime validation is established separately by the connected VM
suite. The Item/feedback suite remains passing at 4,074 checks / 544 comparisons.
All five native targets build. No game process was launched.

The read-only source cohort still prepares Falcon, Ganondorf, Marth, Roy and
Luigi. Yoshi advances to line 828 `articles.create_unowned_at_position(...)`.
Its owned articles, remaining caught-shape/constraint hooks and full gameplay
acceptance remain open, alongside Fox/Falco entity schemas/assets, DK bone
effects and compact whole-match rollback. Installed packages are unchanged;
the goal stays active.

Evidence:

- `A:/FreakMelee-test-archives/script-enclosure-release-oct10e/receipt.json` -- SHA-256 `481a95696b1661a94c2b8160e2ef6a9ef3656c3f05987ec527e1068572afe720`.

- `A:/FreakMelee-test-archives/script-enclosure-release-oct10e/enclosure-reference.json` -- SHA-256 `cb0d2a747fbb82dd8cf4a3a53fce6e9d8b4d3550cfdb0e9364feb139ea588213`.

- `A:/FreakMelee-test-archives/script-enclosure-release-world-oct10a/receipt.json` -- SHA-256 `59b45157d2546c53afa470d803ae6a78589c85acfe19c80310da8a1794b5aead`.

- `A:/FreakMelee-test-archives/source-enclosure-release-rig-cohort-oct10/receipt.json` -- SHA-256 `84f80ee5c77b240bdf2e13055335f278b9993fb14de21611ad2f3706971ce1a8`.

## Unowned article construction through the ordinary interpreter

`articles.create_unowned_at_position(ArticleName, x, y, z, vx, vy, vz, facing)`
now resolves an exact same-package Article declaration and runs the shared
positional constructor with explicitly absent gameplay ownership. Both SpawnItem
parents, current damage owner and original creator are NULL from construction
onward. The caller's immutable author/program still selects assets, parameters
and lifecycle callbacks. No generated-C or stock-article fallback is involved.
The native positional constructor has an explicit ownership argument; all
existing owned call sites retain their previous behavior.

The actual constructor fixture now executes a prepared Fighter helper through
the public compiler, controller, shared VM and typed source-resolved binding.
It passes 12,812 checks / 35 cases / 1,024 reuse cycles. Spawned and Created
observe NULL ownership; full velocity is supplied before construction; the
returned ID is saved into script state. Allocation failure publishes -1, while
caller retirement at either hook and Created removal/fault stop continuation.
An unowned item can still execute its script after the author retires. The
shared constructor also covers unowned Item authors and rejects owner-center
origin without an owner.

The child-article regression passes 18,937 checks / 27 cases / 1,024 reuse cycles;
the state-transition suite passes 7,278 checks / 24 cases / 513 native pairs.
Their stale test linkage was updated with explicit unavailable unrelated
bindings and controlled stock-shaped asset metadata. No production fallback
was added. All five native targets build. Tests run hidden; no game process
was launched and installed content is unchanged.

These fixtures use actual Item construction/scheduler/actor/VM/GObj/creator
code with controlled assets, geometry, model and physics leaves. They establish
constructor and source-call semantics, not DAT/rendered gameplay acceptance.

The read-only nine-source cohort still prepares Falcon, Ganondorf, Marth, Roy
and Luigi. Yoshi advances from line 828 to line 1108, the article's root
attribute access `attrs.egg_hand_joint`. Source article integration and full
gameplay remain open. Fox/Falco schemas/assets, DK bone effects and compact
whole-match rollback are also still open; the overall goal remains active.

Evidence:

- `A:/FreakMelee-test-archives/script-unowned-article-oct10d/receipt.json` -- SHA-256 `f76c7f06405136c15016e1117ae52300fe92ce1fb703965dfaed9b67db94e391`.
- `A:/FreakMelee-test-archives/script-unowned-child-oct10b/receipt.json` -- SHA-256 `f5f835e3a8d3b0f9df7324b537287f716b63e7403a6471166f94e6e3d651ae88`.
- `A:/FreakMelee-test-archives/script-unowned-state-oct10c/receipt.json` -- SHA-256 `dabe9adf10af5ef9fdf5e0bfae0534011cfbfb9ebbe097dc3c6f00578b40aa60`.
- `A:/FreakMelee-test-archives/source-unowned-article-cohort-oct10/receipt.json` -- SHA-256 `04e70317a9bee7ed21ecc26b7c26754c61a764be358ef5ce3f62468b46eeeb51`.

## Yoshi article source migration and generic effect/contact operations

The ordinary Item effect call now admits native position queue mode 1 and
position-plus-scalar mode 4, alongside modes 0/3. The primary joint path stays
immutable and same-owner. The native queue samples joint position when processed
and copies mode 4's finite Float when enqueued. Recipe admission uses the generic
native ABI classifier and rejects Fighter-parent recipes for Item calls.
No Yoshi-specific effect wrapper, generated-C bridge or renderer change was added.

`item.respond_to_stage([impact_response: Bool])` runs the shared native
`it_8026E71C`/`it_8026E5A0` collision responses and invokes the source-owned
`stage_contact` hook. Position, floor reset and wall/ceiling response retain
their native order. Authored decisions remain in that hook. Exact Bool values
select the variant; the backend retains its source through nested VM dispatch
and refuses publication after a fault, removal or userdata replacement.
The ordinary `item.damage_behavior` property exposes native `xC98` directly.

Yoshi's article scripts now use immutable `program.attrs` for fighter tuning,
the shared Item rumble primitive, velocity fields for native velocity reset,
the generic queued position effects, and typed Bool collision options.
The read-only cohort gets past all three article definitions and stops at
`shield.start_health`, line 1288. Falcon, Ganondorf, Marth, Roy and Luigi still
prepare; Fox/Falco metadata/assets and DK bone effects remain open.
This is source preparation progress, not complete Yoshi clone acceptance.

Verification: all five native targets build; Item async effects pass 72,115
checks / 289 cases / 210 native pairs / 10 actor VM calls; terrain and scalar
effects pass 4,967 checks / 944 native pairs / 182 terrain cases / 22 VM calls.
Pinned program parameters pass 294 checks / 38 cases / 71 VM calls; stale fixture
linkage was corrected with explicit unavailable unrelated binding families and
the physical GObj-pool declaration. No production fallback was introduced.

The fixtures execute the actual native queue/contact kernels, compiler, VM and
actors. Stage geometry, impact/reflection, assets and allocation leaves remain
controlled. The two new position recipes use typed captures and a controlled
joint-position sampler; their particle/model constructors and HSD world-matrix
geometry are not tested here. Native queued joints remain borrowed, so arbitrary
graph replacement before flush is not made safe. Tests were hidden and no game
was launched. Installed content, full gameplay acceptance and compact whole-match
rollback remain unchanged/open. The overall goal remains active.

Evidence:

- `A:/FreakMelee-test-archives/script-item-position-queue-oct10-finalb/receipt.json` -- SHA-256 `dbced025cd5de3a1f22b3915ac3be64e47be49c938dc816260df73fd50c5a67b`.
- `A:/FreakMelee-test-archives/script-item-contact-oct10-final/receipt.json` -- SHA-256 `6c629e86f5d9fb030fd5c7c5127cc5d61d0a84a87f3ecc57f9275eefaed3da66`.
- `A:/FreakMelee-test-archives/script-yoshi-root-parameters-oct10c/receipt.json` -- SHA-256 `5f771409a38fcb27c36ab70561a260e7ca9d849a1af1090caa6d5d5e94e5287e`.
- `A:/FreakMelee-test-archives/source-yoshi-articles-cohort-oct10/receipt.json` -- SHA-256 `f87b1c07a9b7d53de311c0688ae61d3705e3fff4639e43b0fccb771453aecab7`.

## Shared shield and guard data through the interpreter

Twenty-three additional generic properties are now ordinary interpreter
operations: twelve guard timers/input modes/window flags; current shield health;
read-only light amount, raw shield joint and base size; and seven common shield
tuning values. The timers use the existing common motion-local guard slots,
including signed word-sized input slots shared with Yoshi. No fighter-specific
guard state API or generated-C fallback was added.

The runtime exposes Float, exact signed Int and canonical Bool contracts.
The public property dispatcher routes them directly to the retained guard/common
data backends. Tuning publication rechecks the captured source and common-data
pointer after final layout admission. Script authors use named package parameters
for their own tuning; the global common data remains read-only.

The connected guard fixture passes 710 checks / 93 cases / 44 existing native
pairs / 70 VM helper calls. Added source helpers write distinct timer, integer
and flag values, check Yoshi/common slot agreement, let the unchanged native
release-input routine consume them, then read results through the VM. Signed
integer endpoints, signed zero, missing shield/common data, invalid scalar widths
and a controlled identity substitution at final admission are covered. The
release-input body matches pinned `829748389`; shared scalar properties are not
a complete scripted-shield behavior comparison.

All five native targets build. The read-only source cohort still prepares Falcon,
Ganondorf, Marth, Roy and Luigi. Yoshi now advances past the shared health query
to `hurtboxes.define`, line 1294. Shield volume creation, transition/scale hooks,
input timers, pose copying and remaining guard operations still need migration.
The guide explicitly distinguishes those older C-exported features from current
interpreter support. Fox/Falco assets/schema, DK effects and compact whole-match
rollback remain open.

The fixture uses actual frontend, program/actor/VM/storage, guard/common backends
and selected native kernels; imported rig metadata, common-data values, property
routing, collision/audio leaves and mutation observers are controlled. Native
targets compile the production property router; the fixture uses its explicit
router. No full shield transition, rendered appearance or gameplay acceptance
is claimed. Tests were hidden, no game launched, installed content is unchanged
and the overall goal remains active.

Evidence:

- `A:/FreakMelee-test-archives/script-shared-guard-data-oct10-final/receipt.json` -- SHA-256 `d324f5cc044a5ac3f621e753b6f9092baf23b95c3b0333a28a183b24157fdb90`.
- `A:/FreakMelee-test-archives/source-shared-guard-data-cohort-oct10/receipt.json` -- SHA-256 `b581d40a6e74b4ffa13b986d91646d71fb0484d99ad3b8e91a3ed441195ac356`.

### Runtime hurtbox definitions through the interpreter

`hurtboxes.define` now uses the shared capsule initializer directly for owned
fighter scripts. It selects an existing capsule slot and raw costume joint,
accepts the three generic height classes, grab eligibility, local endpoints and
native Float scale. New `Hurt.height_low` and `Hurt.height_high` literals join
the existing middle class. No fighter-specific entry point, stock rig lookup,
heap allocation or C-module fallback was introduced.

Source admission retains the physical Fighter, selected program/storage and
owned character data, validates its imported joint table and capsule bounds,
then calls `ftColl_HurtboxInit`. Absent numeric slots/bones remain healthy no-ops;
wide joint integers never narrow into another joint. Malformed types, missing
joint/table data and invalid storage extents fail before mutation/publication.
The native initializer's unchanged behavior enables the capsule and marks
hurtbox update; cached world endpoints and existing update flags remain intact.

The connected fixture passes 5,289 checks / 1,625 cases / 1,557 native pairs /
1,584 VM helper calls. Added comparisons cover 1,512 slot/joint/height/grab/scale
combinations and the scripted shield-capsule recipe. The initializer and shared
all-status routine match pinned `7fde757c9`. Complete Fighter comparisons include
transient cache preservation, signed zero and native nonfinite scale values.
Bad argument widths, missing rig entries, excessive capsule count and controlled
identity changes during admission are rejected. Height literals run through the
actual compiler/VM; an initially unsorted constant insertion was corrected to
preserve the catalog's binary lookup invariant before the passing runs.

All five native targets build. The read-only nine-source cohort still prepares
Captain Falcon, Ganondorf, Marth, Roy and Luigi. Yoshi advances past hurtbox
definition to `effects.spawn_async_model_attributes`, line 1299. Its complete
shield hooks/operations, rig pose copying and remaining input/state operations
are still open; Fox/Falco ownership and DK effects remain open too.

The fixture executes the actual combat backend, actor/VM/storage and the selected
native capsule routines. Rig/package values and property routing are controlled;
unrelated combat leaves abort if reached. Native targets compile/link the real
dispatcher. No capsule contact, animation/rendering, full scripted shield or
gameplay acceptance is claimed. No game launched and installed content is
unchanged. Compact whole-match rollback and the overall goal remain active work.

Evidence:

- `A:/FreakMelee-test-archives/script-hurtbox-definition-oct10-final/receipt.json` -- SHA-256 `3b0925a0cdb37090c29f180590ae3e79d02556d921bcca0c9e1ca4ec9711f6ec`.
- `A:/FreakMelee-test-archives/source-hurtbox-definition-cohort-oct10/receipt.json` -- SHA-256 `fdba7e002abe9eaf005f58964dcfaa62e7754efd5539fa77f2f51dd0d08726f1`.

### Deferred joint-position effects without native struct arguments

The interpreter now provides `effects.spawn_async_position_on_part` and
`effects.spawn_async_position_scalar_on_part`. Both use an exact owned rig
index and the native effect queue; the former admits position-only recipes and
the latter position-plus-scalar recipes. Timing and policy remain in scripts.
Existing synchronous one-/two-joint operations share their retained rig
admission and keep their original behavior.

Inspection of Yoshi's native shield restoration showed that the old
`spawn_async_model_attributes` source spelling overstated the payload contract:
native code passes `co_attrs.xBC`, but `efAsync_Spawn` copies just its first Float,
the common effect size. Yoshi now uses the generic scalar method with
`model.effect_scale`. The obsolete struct-specific exporter branch and its
documentation were removed, without a compatibility alias. No stock source
handler, generated C fallback or fighter-specific runtime API was added.

The connected Fighter actor/VM fixture passes 6,291 checks / 1,707 cases / 1,629
native comparisons / 1,659 VM calls. The four queue bodies are exact against
pinned `7f6ab2bd6`. Seventy-two added comparisons cover immediate/deferred
processing, both payload modes, raw index limits and scalar values. Scalars are
copied at enqueue; moving the joint before processing changes the sampled world
position, while changing the original scalar does not alter the packet. Native
packet order, clearing and allocation failure execute. A source helper reads
the actual common effect-scale backend. Wrong recipe signatures, malformed
integers/types and nonfinite scalars are rejected before effects are queued.

Existing raw-part effect regression passes 247 checks / 51 cases; two-joint
regression passes 66,433 checks / 76 cases / 24 native comparisons. Their unrelated
async leaf remains an aborting fixture boundary rather than a successful stub.
All five native targets build. The nine-source cohort retains five passing
preparations and advances Yoshi to `bones.set_translation`, line 1308. The source
and installed content are not conflated: no incomplete source package was
published and no game was launched.

Queue construction/processing and source execution are actual; imported metadata,
geometry sampling and typed effect-recipe delivery are controlled. This does not
prove particle/model construction or rendered appearance. The fixed sixteen-packet
allocator is not production allocation-free evidence, and the native queue still
borrows joints until processing. Arbitrary later rig replacement remains outside
this operation's guarantee. Complete Yoshi shield operations/hooks and compact
whole-match rollback remain open; the overall goal stays active.

Evidence:

- `A:/FreakMelee-test-archives/script-raw-part-async-position-oct10-final/receipt.json` -- SHA-256 `b80250f2abd382db95b4e8616a8d08735426865a600363a9cf97d5b8af49888b`.
- `A:/FreakMelee-test-archives/script-part-effect-async-regression-oct10/receipt.json` -- SHA-256 `c7d99ffe3700e9ffe1a14e52b9794307df5e5b47a618a6cddb20f808b9e14d48`.
- `A:/FreakMelee-test-archives/script-between-effect-async-regression-oct10/receipt.json` -- SHA-256 `ed01e3e3e8a0412028d9d9da6ad4ebe3bcf382bad9303b121a6289fce0e9b6a2`.
- `A:/FreakMelee-test-archives/source-async-position-cohort-oct10/receipt.json` -- SHA-256 `d94ccd8ff784c9d78ff1256fdee9be74953fcfb7bfb03a6b87f9b4770ffa76a6`.

### Raw joint translation and shared input ages

`bones.set_translation` now addresses an exact owned costume joint and uses the
native HSD setter. Its matrix dirty policy is unchanged; it does not alter
gameplay position or scale. Signed 32-bit index/type checks and retained rig
admission precede mutation. Negative/out-of-range indices are healthy no-ops.
The existing `bones.set_scale` continues to take a semantic role, a distinction
now corrected in the joint-pose documentation.

`input.analog_shield_age` and `input.horizontal_tilt_age` now read/write the
shared native input slots. Writes validate 0 through 254 before narrowing and
preserve neighboring timers. Native input processing still advances/resets
these ages; no new fighter-specific policy is added.

Yoshi's rest-pose restoration now uses the already implemented generic
`pose.copy_costume(Pose.live, start_joint)`. Inspection confirms the matching
costume descriptor lookup and live native traversal; the obsolete
`bones.copy_costume_pose` reference-exporter branch was removed without an alias.

The connected compiler/actor/VM/production router bone fixture passes 3,383
checks / 196 cases / 118 native comparisons / 159 VM helper calls. Added tests
include 48 whole-rig translation comparisons, nonfinite native Float values,
invalid arguments/owned rig data and input-age bounds/neighbor checks. The pose
dispatcher regression passes 1,858 checks / 208 cases / 182 retirement
mutations / 8 native operations. Its stale strict compiler configuration needed
the existing catalog's intentionally partial aggregate-initializer warning
disabled before it compiled; no production contract was weakened.

All five native targets build. The read-only source cohort retains five passing
preparations and advances Yoshi to `guard.scale_joint`, line 1310. Guard
operations/transition hooks and complete Yoshi acceptance remain open. The bone
fixture executes actual matrix/setter code against controlled rig metadata;
the pose regression controls descriptor/topology/math leaves. Neither proves
imported Yoshi deformation or gameplay parity. No game was launched and no
incomplete package was published. Compact whole-match rollback remains open.

Evidence:

- `A:/FreakMelee-test-archives/script-joint-translation-input-ages-oct10-final/receipt.json` -- SHA-256 `e32d15ebc7138e292ed9745f57d0adc6933f0bf136e9c438c0b3b72e5f4081ca`.
- `A:/FreakMelee-test-archives/script-pose-copy-yoshi-regression-oct10b/receipt.json` -- SHA-256 `0fa8efd8d45deb38656e23ce8bf28f073831ad86a3ee05cba7e01d1f02f61d18`.
- `A:/FreakMelee-test-archives/source-joint-translation-input-cohort-oct10/receipt.json` -- SHA-256 `e5b1508ac3d332ff75fc00b4351858938ea3615709bcdddca7694f0b1c648ef4`.

### Whole shield-transition scheduling in the interpreter

The shared Guard scheduler now delivers `on shield_transition` to an ordinary
owned Fighter actor. Its exact typed arguments are the generic transition mode
and existing special-hit Boolean. All six modes have named `Shield` literals
from the authoritative native enum. Start and powershield start deliver before
common motion initialization when the whole-entry hook is present; their narrower
post-entry hooks retain their existing behavior when it is absent.

Nine scheduler sites are wired, including alternate hold/release entries after
shield stun and startup completion. Shared release statistics precede delivery.
The shared special-hit tail executes only after successful script delivery and
retained physical Fighter/program/storage admission. A VM fault or retirement
never chooses stock fighter behavior as a substitute for the authored entry.
No generated module, callback wrapper or fighter-specific runtime API was added.

The connected guard cohort passes 6,524 checks / 1,728 cases / 1,629 native
comparisons / 1,678 VM calls. Ten added scheduler cases execute complete current
production entry bodies, covering six modes, both hit arguments and alternate
paths. Two wrong argument contracts preserve output, and nine fault/retirement
cases stop the special-hit continuation, including physical deletion/reuse,
userdata/classifier replacement and schema/parameter changes. Startup retried on
a faulted actor does not enter stock code. Native shared guard/effect/capsule
comparisons from the previous cohorts remain passing.

All five native targets build. The source cohort retains the five existing
passing preparations; Yoshi still stops at `guard.scale_joint`, line 1310. This
batch installs the scheduler contract rather than completing the full package.
Scale/alpha query hooks and remaining guard primitives still require migration.
The fixture controls imported metadata/property routing, release statistics,
special-hit bookkeeping and the startup-completion drain result. Stock/fighter
transition leaves abort if reached. Actual motion changes, full drain policy,
input arbitration and rendered/gameplay parity are outside its acceptance.
No game or incomplete source publication occurred. Compact whole-match rollback
and the overall goal remain active work.

Evidence:

- `A:/FreakMelee-test-archives/script-shield-transitions-oct10-final/receipt.json` -- SHA-256 `3839e4f5cfde88261a34665e020cb01ad899f57567ad7c4cda8f2d7e6006b673`.
- `A:/FreakMelee-test-archives/source-shield-transitions-cohort-oct10/receipt.json` -- SHA-256 `5941ec3ab9edab0f309719e918d85199c03b255a7740d36abc144b71f26bff6e`.

### Shared guard kernels and radius/alpha queries

Nine ordinary-interpreter operations now expose shared shield scale, volume and
powershield construction, release-input consumption, depletion, window timers,
shield SDI, and hold/release requests. They use existing native common kernels
and owned rig/state admission, without fighter-specific policies or generated
modules. Readonly `reflector.active` exposes the existing reflection flag.
Common reflector installation requires and binds an owned `on reflect_hit`.
SDI clears an earlier interpreted post-hitlag binding before assigning the native
shield callbacks, preserving the shared callback ordering.

`on shield_scale` and `on shield_alpha` now run through the interpreter at their
common query sites. Scaling retains the fighter/program/storage and selected
rig; a fault, retirement or rig replacement stops joint updates. The stock
radius/alpha calculations remain unchanged when no authored hook exists.
Shield depletion rechecks the retained actor after break entry before playing
sound or returning to the interpreter's native continuation.

The connected cohort passes 7,281 checks / 1,910 cases / 1,753 native comparisons /
1,768 VM calls. Current operation recipes, SDI and stock scale/alpha are compared
with bodies pinned to `7ff672421`; the shared reflector constructor is checked
against that revision. Cases include six scalar values with four joint flags,
60 guard recipe variants, 32 SDI/post-SDI variants, both alpha decisions, nested
authored transitions and readonly reflection status. Wrong arity, missing rig or
common data, undersized classes and absent reflect-hit handlers reject without
output. Faults/eight actor-storage mutations stop query and break continuation;
five rig/table mutations inside radius queries preserve the original joint.
VM scratch stays balanced.

All five native targets build. The read-only source cohort retains Captain
Falcon, Ganondorf, Marth, Roy and Luigi preparation; Yoshi advances from line
1310 to `controls.try_powershield()` at line 1386. Fox/Falco owned entity schemas
and Donkey Kong's effect operation remain their previous gates. Remaining guard
input/physics/collision operations, other Yoshi root hooks, package publication
and gameplay acceptance are open. The fixture controls rig/class data, property
routing, alpha delivery and break/statistics/audio leaves; it does not establish
collision traversal, motion transitions or rendered/full-match parity. No game
was launched or installed content changed. Compact whole-match rollback remains
open and the overall goal remains active.

Evidence:

- `A:/FreakMelee-test-archives/script-guard-primitives-oct10-final2/receipt.json` -- SHA-256 `c6dd02413f0f49d053b1cf18f93a712439ec7aebe8cff5eec36a36521c546a9c`.
- `A:/FreakMelee-test-archives/source-guard-primitives-cohort-oct10-final/receipt.json` -- SHA-256 `d55135786efa059a306296c38fdc9dd9585df5d1e5df1d866f256eb499116d84`.

### Guard input/physics/collision and escape-entry scheduling

Eleven further ordinary-interpreter operations expose powershield and held-item
throw checks, roll/spotdodge checks, platform dropping, powershield interrupt
traversal, guard/guard-hit physics and their collision policies. They invoke the
original shared kernels. The misleading public names `try_ground_roll` and
`try_ground_escape` were removed and their templates/reference-exporter call
sites adjusted: `try_spotdodge` requires held shield, `try_guard_spotdodge` uses
the stock in-shield predicate without that requirement, and `try_roll` selects
forward/backward from horizontal input and facing. Yoshi's shield interrupt
now uses the in-shield predicate, matching the original guard eligibility.

Shared escape entry delivers authored `on roll_enter` after common roll
initialization and `on spotdodge_enter` before common spotdodge initialization.
Source faults/retirement stop subsequent initialization or statistics. Motion
and animation initialization retain the source before cached Fighter writes.
The complete powershield priority chain rechecks it after each unsuccessful
input decision, preventing a false-returning retired source from entering the
next branch. No fighter-specific runtime extension was introduced.

The connected cohort passes 16,492 checks / 2,857 cases / 2,654 native comparisons /
2,704 VM calls. Added comparisons include 768 input variants, 128 ground-movement
and collision-selection variants and five powershield priority cases, against
complete recipes pinned to `926b6e33d`. Eleven arity errors preserve output;
27 hook fault/retirement cases preserve roll/spotdodge ordering, and eight
false-returning decision retirements stop priority traversal. Previous guard,
capsule, effect, query and lifetime cases remain passing.

All five native targets build. The source cohort retains five passing fighters
and advances Yoshi to `jump.update_animation()` at line 1418. Shield APIs and both
escape entry hooks are accepted; other Yoshi aerial-jump/root properties and
package publication/gameplay validation remain open. The predicate oracle uses
an unowned stack object with controlled powershield/release delivery. Motion,
animation, statistics, throw/drop entry, platform/friction queries and terrain
collision leaves are controlled. The cohort does not establish complete motion,
stage-contact, rendered or whole-match parity. No game launched or installed
content changed. Compact whole-match rollback remains open; the goal is active.

Evidence:

- `A:/FreakMelee-test-archives/script-guard-flow-oct10d/receipt.json` -- SHA-256 `4e6adb49840d9ab2b649e89b576de68951a0ce0ba84b5f60e76aef19ba04b04b`.
- `A:/FreakMelee-test-archives/source-guard-flow-cohort-oct10a/receipt.json` -- SHA-256 `59d45c1d5ddb9db1950a0de9614bce4e4f3fca6babbd45d2e6d034449483d87f`.

### Aerial-jump completion and common pose selectors

`jump.update_animation()`, writable `motion.fall_animation` and writable
`damage.reverse_knockdown_pose` now run in the ordinary interpreter. Jump
completion uses the actual shared normal/blended joint animation query and
FallAerial entry. Admission requires the selected owned rig and each active
pose joint. Fall entry retains the physical actor/program/storage across motion
initialization before writing its cached fall state. The fall selector rejects
out-of-range integers before conversion and preserves the native unrelated-state
read of -1 / no-op write policy. Knockdown reversal is a generic rig convention,
not a Yoshi-specific operation.

The connected cohort passes 17,539 checks / 2,962 cases / 2,744 native comparisons /
2,826 VM calls. Added cases cover 64 animation flag/blending combinations, 24
selector/state/signed-range combinations, both knockdown flags, malformed
arguments, missing active normal/blended joints, stale contexts and eight
actor/storage retirements during motion initialization. Complete joint query,
jump completion and fall-entry bodies are compared with `040bc5613`. Motion
initialization remains controlled, so this does not establish skeletal
deformation, rendered pose or full aerial-jump gameplay parity.

All five native targets build. Five source preparations remain passing; Yoshi
advances to `capture.enclose_caught()` at line 1465. Capture setup/frame hooks,
remaining source migration, package/gameplay acceptance and compact whole-match
rollback remain open. No game launched or installed content changed. The
overall goal remains active.

Evidence:

- `A:/FreakMelee-test-archives/script-jump-policy-oct10-final/receipt.json` -- SHA-256 `6d29027128fdb3ca93395b1b91e4f5414c583c10385db43d4b4c56bb9bf5294a`.
- `A:/FreakMelee-test-archives/source-jump-policy-cohort-oct10a/receipt.json` -- SHA-256 `6603d139920ce13148fac7ea8ff1acec456610ffed6d196678d8253dd1519a8b`.

### Caught capsule/constraint and holder-owned setup

The ordinary interpreter now exposes `capture.enclose_caught(radius)` and
`capture.constrain_caught()`, plus the actual holder-owned `capture_wait_setup`
scheduler. Both operations retain the linked actors and admit their own semantic
joint mappings. Enclosure rejects nonfinite/negative radius and invalid common
model scale before mutation. The existing native capsule/constraint recipes
remain shared engine operations; no Yoshi-specific binding was introduced.
Setup receives a lifetime-checked caught-fighter token. Hook failure or either
actor's retirement stops the victim mask and subsequent cached work.

The connected capture cohort passes 7,585 checks / 778 cases, including six
complete caught-capsule comparisons against `d2ba54c30`, stock/owned victims,
setup delivery and seven source/program/schema faults per victim kind, absent
hook behavior, malformed links/joints/scales/arguments, and both actor
retirements at the constraint leaf. The actual common model-scale query and
hurtbox initializer execute. The all-capsule status setter and graph constraint
are controlled traced leaves. This proves capsule/dispatch/admission behavior,
not rendered graph constraints or whole-grab gameplay parity.

All five native targets build. Five source preparations remain passing; Yoshi
advances from line 1465 to `stage.compare_floor_islands()` at line 1536. Its
catch-pull-frame and throw-victim-setup scheduler migration, owned articles and
full package/gameplay acceptance remain open; reaching this body diagnostic
does not establish preparation of those remaining hooks. No game launched or
installed content changed. Compact whole-match rollback remains open and the
overall goal remains active.

Evidence:

- `A:/FreakMelee-test-archives/script-caught-policy-oct10-final2/receipt.json` -- SHA-256 `f804535832f21c6578d05bad599ee0d94e84fcba1b97c72016314a24b292fa59`.
- `A:/FreakMelee-test-archives/source-caught-policy-cohort-oct10a/receipt.json` -- SHA-256 `cf28a49e880ba91f090f0f310ef03ce8667d303b9a8a07588a1060687d5a83f9`.
