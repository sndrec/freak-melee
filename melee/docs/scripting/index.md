# Fighter scripting

Ordinary packages now run `.fms` through the native interpreter. Source is
validated and lowered to typed bytecode; character builds need no generated C,
behavior DLL or C toolchain. Generated C remains only in the separate behavioral
reference tests. See [Interpreter transition](../native_interpreter_transition.md)
and the [frontend](../native_script_frontend.md).
The [single-fighter roster CPU checkpoint](../native_source_roster_cpu_checkpoint.md)
records fresh normal-package gameplay comparisons and shared script/engine fixes.
The [family gameplay checkpoint](../native_source_family_gameplay_checkpoint.md)
covers paired packages, both Player roles and Zelda/Sheik transform sequences.
All 26 stock forms except Kirby now pass ordinary source-package preparation,
including Zelda/Sheik and Popo/Nana. The
[clone closure audit](../../tests/native/fighter_clone_closure/audit.md) records
that admission result. The gameplay checkpoints establish the cases they cover,
not every move interaction or lifetime branch. See the
[special-entry](../native_source_roster_special_checkpoint.md),
[follow-through](../native_source_roster_followthrough_checkpoint.md) and
[two-source interaction](../native_source_roster_duo_checkpoint.md) evidence.
The [results checkpoint](../native_source_roster_results_checkpoint.md) records
match-end/model admission, the shared demo-query fix and the passing rerun.
Additional custom operations still require checked generic bindings; an unknown
operation is rejected rather than falling back to a stock handler. Complete
compact match snapshots and exhaustive Workshop/gameplay acceptance remain
separate work.
See [controller feedback and camera quakes](world_feedback.md) for shared
presentation requests and their native eligibility and lifetime rules.
See [bone queries](bone_queries.md), [article state transitions](article_state_transitions.md)
and [family queries](family_queries.md) for the current direct bindings and their
ownership boundaries. Simultaneous companion packages now prepare and activate
through the existing family transaction; see the
[companion activation checkpoint](../native_source_companion_activation_checkpoint.md)
for publication and eight-actor startup evidence; the newer family gameplay
checkpoint above records subsequent gameplay comparisons.
The [Workshop preview checkpoint](../native_source_workshop_preview_checkpoint.md)
records source-owned member selection, special-motion lookup and retained pose
identity, with an actual Popo/Nana dropdown and frozen-pose smoke.
Earlier integration checkpoints preserve the gaps and evidence at their dates:
[bone/item/family](../native_source_bone_item_family_checkpoint.md),
[held/ground/bone](../native_source_held_ground_bone_checkpoint.md),
[motion/visibility/collision](../native_source_motion_visible_sweep_checkpoint.md)
and [ground/action/audio](../native_source_ground_action_audio_checkpoint.md).
Their old preparation blockers are not the current roster status.

Cloned packages own their [asset layouts, article bindings and rig maps](../fighter_asset_ownership.md)
separately from scripted behavior. Partial ports still declare their remaining
stock gameplay dependencies in their translation ledgers.

Stock translations aim to preserve what players see and feel. Exact source
comparisons remain useful where the original behavior is defined, but matching
undefined memory or an incidental stock RNG sequence is not a reason to add
fighter-specific API shortcuts. Initialize authored state deliberately and
document intentional differences in the script and its translation ledger.
Determinism between native clients remains required for replay and rollback.

Freak Melee fighter scripts (`.fms`) describe fighter decisions and article
behavior. The Workshop validates them and prepares interpreted bytecode when a
package is built. A script controls *when* to enter a state, change velocity,
spawn an article, or respond to a collision; the engine supplies shared
operations for those actions. An authored callback does not need to call a
stock fighter's special-move handler.

This reference describes the authored language and shared API baseline. Some older design
sketches in [Move behavior language](../native_character_script_examples.md)
show syntax that is not implemented yet. Use the examples below and the
compiler's validation result as the authority for a buildable script.

See [colors and model presentation](colors.md) for typed RGBA colors, controller
palettes and owned visibility tables.

The current [shared fighter interpreter reference](common_fighter.md) covers
movement-space transitions, platform skipping, native model/collision observations,
visibility groups and color selection.
See [target queries](targets.md) and [math operations](math.md) for generic
article steering and unsigned serial arithmetic.
See [Fighter observations and action statistics](fighter_observations.md) for
special-landing lag, engine debug level and thrown-item counting.
See [body contact and rebound](contact_rebound.md) for recipient-owned decisions.
The [observation/contact checkpoint](../native_source_observation_contact_checkpoint.md)
records the connected interpreter evidence and remaining Peach boundaries.
See [Fighter hitboxes](fighter_hitboxes.md) for capsule state and native scaled,
charged and stale damage; [ground controls](ground_controls.md) for authored
input arbitration and landing lag; and [Item surface and animation observations](item_observations.md)
for native collision sweeps and primary-graph completion.
See [held articles](held_articles.md) for authored status/action policy and live
observation of a holder's current item.
See [article object receivers](object_receivers.md) for the distinct creator,
current-owner, held-item and attachment-owner relations.
See [ordinary engine items](engine_items.md) for fighter item construction and
food spawning, and [absorbers](absorber.md) for typed damage and hit counts.
See [fighter sound channels](sound_channels.md) for package-owned sound
suppression, costume-dependent sound routing and channel defaults.
See [global audio](global_audio.md) for source-owned global sound calls and
returned native instance handles.
See [alternate forms](forms.md) for independently owned members and authored
activation, and [motion callbacks](motion_callbacks.md) for helper-based phase
registration and cue policy.
See [joint feedback and playback](joint_feedback.md) for typed raw/semantic
joint effects, copied-position wind and rate-preserving motion entry.
See [synchronous two-joint effects](effect_between.md) for retained raw-rig
attachments and native construction order, and [effect hitlag controls](effect_hitlag.md)
for shared pause/resume callbacks and explicit authored callback replacement.
See [paired fighters](paired_fighters.md) for live member IDs, directional hitlag
links, collision history and current-world bone sampling.
See [stage and movement queries](stage_movement.md) for collision segments,
ledge probes, air movement, tether input and global sound.
The [direct stage-query reference](stage_queries.md) documents independent ECB
sweeps and segment tests currently connected to the interpreter.

## Start here

- [Script structure and callbacks](language.md) explains move and article
  blocks, callback order, and the difference between a script and a subaction.
- [Fighter parameters](language.md#fighter-attributes) explains project-owned
  constants, the Workshop Parameters section, stock-value import, and why
  editing a value needs no behavior rebuild.
- [Program parameters](program.md) explains explicit root tuning shared with
  articles and effects, independent of any live creator or holder.
- [Functions and loops](functions.md) explains typed reusable helpers, temporary
  arguments, fighter-local state, forward calls, direct motion-callback binding
  and `while` loops.
- [Synchronous callbacks](synchronous_callbacks.md) explains recipient-owned
  named events, callback ordering and source lifetime after item removal.
- [Callback slots](callback_slots.md) covers direct accessory-phase bindings,
  clearing callbacks and checking whether a slot contains your own function.
- [Fighter API](fighter.md) lists move transitions, cue helpers, and
  fighter-wide events used by current authored callbacks. It also documents
  [input-history and action-stat APIs](fighter.md#inputhorizontal_smash_age)
  needed by upcoming stock move translations.
- [Movement and collision API](movement.md) covers steering, freefall,
  animation-driven physics, and special landing, with Luigi's Super Jump
  Punch as a connected example.
- [Common smash charge](smash_charge.md) explains shared charge timing,
  animation-rate storage and presentation, with script-owned release decisions.
- [Horizontal smash controls](horizontal_smash_controls.md) describes the shared
  stick threshold and timing values used to classify an authored move's input.
- [Held-item visibility](held_item_visibility.md) explains the stored visibility
  request independently of whether a held item or its model is present.
- [Item segment sweeps](item_sweep_segment.md) covers previous-to-current path
  collision, live stage lines, surface normals and moving-stage velocity.
- [Normal attacks](normal_attacks.md) covers jab and rapid-jab startup,
  aerial motion mapping, pickup priority, owned landing decisions and
  [shared common-motion policies](normal_attacks.md#shared-motion-phases).
- [Fighter item controls](held_item_controls.md) explains held and auxiliary
  slots, creator-gated binding, foreign-item restoration, attachment and scaling.
- [Jump and shield-break policies](jump_policies.md) covers owned multi-jump
  tuning, aerial-jump phases and the common top-blast damage flag.
- [Attached captures](capture.md) covers command grabs, editable escape/mash
  settings, captured victim poses and explicit releases.
- [Luigi Cyclone translation](cyclone.md) shows persistent charge, repeated
  taps, shared movement methods, and the tested stock behavior boundary.
- [Collision capsule coordinates](collision_capsules.md) explains current and
  previous world positions, distinct from bone offsets and activation.
- [Combat and presentation](combat_visuals.md) covers live hitboxes, model
  scale, named-bone rotation, explicit motion rates, custom collision boxes,
  effects and reflection.
- [Reflectors](reflector.md) covers editable bubbles and projectile-contact callbacks.
- [Absorbers and healing](absorber.md) covers editable absorption volumes,
  absorbed-projectile responses, damage reduction and direct-joint effects.
- [Guards](guard.md) covers editable interception volumes and script-owned hit reactions.
- [Common action policies](common_policies.md) covers jab-entry bookkeeping,
  jab-chain decisions, complete smash entries, smash/shield presentation,
  saved/live visibility selectors, dynamic-bone wind and
  floor collision for dynamic bones, grounded grab policies and body-attack
  rebound permissions.
- [Owned CPU profiles](cpu_profiles.md) explains editable candidate tables,
  stock import, package ownership, and the shared planner's scope.
- [CPU behavior policies](cpu_policies.md) covers authored classification,
  charging/capture/travel decisions, target and floor identity queries, spacing,
  falling-recovery eligibility, available jumps and bounded input queues.
- [Follower CPU and delayed inputs](cpu_history.md) covers mode/output ownership,
  recorded controller history, destination planning and bounded floor queries.
- [Weapon ribbons](trail.md) describes joint-attached trails with owned colors,
  dimensions, fade settings, and a read-only rendering callback.
- [Linked hitlag](hitlag.md) covers observing active hitlag, retaining a peer
  relation, requesting release and clearing that relation without dispatching
  callbacks from the binding itself.
- [Reserved model parts](model_graphs.md) explains installing an owned graph
  descriptor into both current and alternate rig poses at scene setup.
- [Fighter poses](poses.md) covers owned descriptor graphs, animation sampling,
  costume-pose copying and independent blending of the live and alternate rigs.
- [Animated model effects](model_effects.md) describes owned visual update
  scripts, creator-state reads, joint controls, and cancellation.
- [Stage events](stage.md) covers named stage queries/notifications, radio
  sequence contracts, sound dependencies, and their callback timing.
- [Articles](article.md) describes authored projectile setup, article states,
  [owned article parameters](article.md#parameters), [constant tables](article.md#constant-tables),
  [signals](article.md#signals-from-the-fighter), and
  [shared callbacks](article.md#sharing-state-callbacks).
- [Article functions](article_helpers.md) covers typed reusable functions local
  to an article, forward calls, callback payloads and lifetime checks.
- [Article terrain queries](article_terrain.md) covers collision-line island
  bounds and named floor/wall masks.
- [Article models](article_models.md) covers owned immutable roots, independent
  model instances, raw quaternion transforms and draw eligibility.
- [Article resources](article_resources.md) records the remaining fixed
  model/collision-node collection design; it is not a direct-interpreter API yet.
- [Read-only article references](article_references.md) explains typed state
  reads by creation ID, forward declarations, package identity and bounds.
- [Fighter engine-item operations](engine_items.md) covers ordinary item creation,
  attachment, scale inheritance, held slots, and exact integer handles.
- [Item API](item.md) lists the article operations used by the current stock
  translations, with signatures, return values, and examples.
- [Package-authored holdable items](../native_fighter_items.md) covers the
  separate item type intended for pickups and throwable snacks.
- [Costume model attachments](model_attachments.md) covers accessory graphs,
  cloth descriptors, held-item categories, and part-animation layers.
- [Transient fighter accessories](fighter_accessory.md) covers owned model
  recipes, animation variants, independent geometry clocks and bone following.

## Examples

- [Bowser](../../examples/koopa_wip.fms) authors all four specials, Flame
  articles, capture/throw decisions, hurtbox/visibility changes, dynamic-bone
  floor policy and identified CPU decisions. It declares independent behavior.
  See the
  [translation ledger](../bowser_script_translation.md) for exact coverage.

- [Mario's full script](../../examples/mario_full.fms) includes all four authored
  specials, Fireball/Cape articles and versus fighter events. Its
  [translation ledger](../mario_script_translation.md) records tested input
  paths and independent versus acceptance.
- [Dr. Mario's full script](../../examples/drmario_full.fms) includes an
  authored pill with manually managed lifetime and owned results presentation.
  [Fighter presentation](presentation.md) documents generic demo events,
  article reference binding, appearance and initial physics updates.
- [Peach's independent template](../../examples/peach_full.fms) owns Float,
  specials, forward-smash selection, lifecycle, CPU policies, taunt and five
  articles. Its [translation ledger](../peach_script_translation.md) distinguishes
  source/package coverage from sixteen actual gameplay comparisons and the
  remaining visual and interaction limits.
- [Captain Falcon](../../examples/captain_wip.fms) and
  [Ganondorf](../../examples/ganon_wip.fms) own their four specials and versus
  fighter events, CPU decisions, taunt and shared item-swing policies. They
  declare independent behavior. Their
  [translation ledger](../captain_ganon_script_translation.md)
  records 3,905 source-policy comparisons and eighteen independent gameplay
  recordings, including active CPU matches after the native life-reset fix.
- [Luigi's work-in-progress script](../../examples/luigi_wip.fms) includes
  an authored Fireball with automatic lifetime, common fighter events, and
  the [Green Missile](../luigi_green_missile_translation.md) entry, charge,
  launch, flight, and end phases. Its
  [Super Jump Punch](movement.md) and [Cyclone](cyclone.md) are also authored.
  Grounded and airborne contact and repeated-tap Cyclone recordings match
  stock, apart from its deliberate fresh-life charge initialization. The
  Green Missile parity recordings cover specific input paths; see its ledger
  for remaining branch coverage.
- [Pikachu source](../../examples/pikachu_wip.fms) / [Pichu source](../../examples/pichu_wip.fms)
  authors Skull Bash, Thunder Jolt (including both linked projectile parts),
  Quick Attack/Agility and Thunder, plus owned lifecycle, common and CPU policies.
  [Linked articles](article_links.md) explains reusable relationships,
  signals and indexed joint reads. Its
  [translation ledger](../pikachu_pichu_script_translation.md) distinguishes
  records shipped source/drawn acceptance and remaining interaction limits.
- [Rook's Workshop sources](../../examples/rook_workshop/) show a custom
  character split into move files and fighter state.
- [Jigglypuff's script](../../examples/purin_wip.fms) authors Rollout, Pound,
  Sing and Rest with facing-dependent variants, generic accessory callbacks
  and all five aerial jumps. It declares independent versus behavior.
  Its [translation ledger](../jigglypuff_script_translation.md) records stock
  comparisons, including all five costumes and the authored fighter events.

- [Fox](../../examples/fox_wip.fms) and [Falco](../../examples/falco_wip.fms) own Reflector,
  neutral-B, side-B and its trail, gun/laser articles, throw shots, and related
  cleanup, up specials, radio behavior and CPU decisions. Each declares
  independent versus behavior and uses an authored FireTravel model effect;
  see their [ledger](../fox_falco_script_translation.md) and current
  [interpreter admission limits](../native_script_fox_falco_articles_checkpoint.md).
  [Laser callbacks](../../examples/fox_falco_laser.fms) and
  [gun callbacks](../../examples/fox_falco_blaster.fms) are also compiled
  separately for focused stock comparisons. The combined fighter uses
  [throw events](fighter.md#throw-animation-events),
  [bone directions](combat_visuals.md#bone-directions-and-instance-scale),
  explicit projectile launches, and named article signals.

- [Marth](../../examples/marth_wip.fms) and [Roy](../../examples/roy_wip.fms)
  author all four specials, fighter item and knockback events, common action
  policies, and [weapon ribbons](trail.md). They declare independent behavior. Their
  [translation ledger](../marth_roy_script_translation.md) records comparisons
  and the shared-engine paths still requiring broader coverage.

- [Donkey Kong](../../examples/donkey_wip.fms) authors all four specials,
  cargo acquisition/carrying, heavy-item carrying, eight cargo throw motions
  and CPU policies, with
  punch charge in ordinary fighter state and owned tuning. It declares
  independent behavior. Its
  [translation ledger](../donkey_script_translation.md) separates these authored
  paths from holding-damage entry and broader acceptance still required.
  [Shared throw operations](fighter.md#throwsupdate) and
  [walking/jumping primitives](movement.md#authored-walking) keep the engine
  operations reusable across fighters.

Kirby cloning and custom copy abilities are deferred. Stock Kirby remains
playable; implementing his custom copy dispatch is not a current requirement
for other fighter templates.

## Math reference

[Numbers, signed angles and vectors](math.md) describes shared arithmetic,
article normalization, and angle operations. The [Item API](item.md#stage-contact-and-collision-box)
links those operations to stage contact normals and editable collision boxes.

## Reading a reference page

Each API entry states its signature, what it changes, where it can be called,
and any important return value or limitation. Links between declarations,
callbacks, and item operations connect concepts that are usually used
together. This follows the navigation and example-driven style of Godot's
GDScript reference without pretending that `.fms` is GDScript.

The API is still growing as stock fighters are translated. New operations
should be documented here when they are added, and examples must reflect code
that currently compiles. A parity recording proves only its particular input
path; it is not a blanket promise that every branch of a translated move is
finished.

**API design rule:** New script operations must describe reusable engine
behavior, never a named fighter, move, or old fighter-union field. Declare
charges and cooldowns as rollback-tracked [`state` variables](language.md#fighter-state).
The full Mario and Dr. Mario templates use ordinary charge variables;
fighter-specific charge properties and hard-coded numeric attribute aliases
have been removed from the compiler. A stock move's uninitialized state may be
intentionally initialized in the script; document that correction in the
source and keep parity claims limited to the tested, defined behavior.

### Costume variants and CPU recovery

See [costume variant metadata](model_attachments.md#properties),
[offline recovery fixtures](../native_match_launch.md), and the
[Pikachu/Pichu comparison ledger](../pikachu_pichu_script_translation.md#costume-variants-and-cpu-recovery-checkpoint---2026-09-30).

### Samus specials and combat articles

The [independent Samus template](../../examples/samus_full.fms) owns Charge Shot,
both missile variants, bombs/morph movement, Screw Attack, all four combat
articles, the full grapple solver, throw attachments, load/death cleanup, held-item
and CPU policies. Its [translation ledger](../samus_script_translation.md) and
[gameplay checkpoint](../native_source_samus_gameplay_checkpoint.md) distinguish
stock/script comparisons from remaining terrain, lifecycle and visual limits.
[Normal air acceleration](movement.md#physicsair_accel_normal)
can be calculated separately from vertical physics for animation-driven moves.

Bombs also demonstrate [owner function calls](functions.md#calling-the-owner-from-an-article),
[editable hurtbox capsules](combat_visuals.md#editable-hurtbox-capsules),
[ground-following article callbacks](item.md#ground-following-and-transitions), and
[individual input checks](fighter.md#individual-input-checks).

## Parallel stock-translation checkpoint

[Samus CPU and rolls](../samus_script_translation.md#cpu-decisions-and-roll-checkpoint)
use generic [CPU candidate queries](cpu_policies.md), [roll hooks](common_policies.md#on-roll_enter)
and [authored grounded collision boxes](movement.md#collisionground_standard_box).
[Link and Young Link](../link_clink_script_translation.md) remain partial;
their ledger records the remaining tether and shared-policy boundaries.
The separate [held-tether source comparison](../link_tether_script_translation.md)
covers generated construction, held updates and teardown, with its controlled
engine boundaries and outstanding activation work recorded explicitly.
[Yoshi](../yoshi_script_translation.md) and [Mewtwo](../mewtwo_script_translation.md)
declare independent behavior. Their ledgers distinguish accepted source and
gameplay comparisons from broader untested interactions.

The [item reference](item.md#model-animation-and-spin) documents shared model
animation, spin, held-special attachment, stage response and sound-channel
operations. [Joint-spanning effects](combat_visuals.md#effectsspawn_between)
accept own-rig index expressions; [additional death cleanup](fighter.md#callbackstrack_death3)
reuses the authored cleanup handler. None delegates fighter decisions to a
stock special handler. Kirby remains deferred.

- [Article instance scale and stage contacts](article_instances.md): remembered throw scale, engine geometry refresh and typed contact-mask response.

- [Aerial actions](aerial_actions.md) documents shared input policies, held-item aerial decisions, landing callbacks and validated contact queries.

- [Airborne and attached-effect interpreter checkpoint](../native_source_float_air_checkpoint.md)
  records direct airborne input selection, owned landing helpers, adjusted
  animation velocity and native async effects, with bounded comparison evidence.

- [Article state and query checkpoint](../native_source_article_queries_checkpoint.md): direct-interpreter ground/air writes, primary joint world positions, copied-position wind and foreign smash-charge queries, with verification limits.

- [Dynamic bone policies](dynamics.md): per-set interaction thresholds and the
  remaining ground/wind policy boundary.

- [Held-open, animated-jump and dynamics checkpoint](../native_source_held_jump_dynamics_checkpoint.md):
  owned common entry and script phase selection, with comparison scope and limits.

- [Owned Peach article checkpoint](../native_source_peach_articles_checkpoint.md):
  actual Item construction, animation and cleanup, a hidden four-Peach replay,
  and the remaining scene fault-isolation boundary.

- [Held-at-center Article creation](held_center_factory.md),
  [Item ground state](item_ground_state.md) and
  [recipient bone queries](target_bone_queries.md) document shared interpreter
  operations used by held articles and paired-fighter behavior.

- [Collision history copy](collision_copy.md) documents direct native history
  transfer without Fighter motion or family policy.
- [Guard, collision and companion-fields checkpoint](../native_source_guard_collision_checkpoint.md)
  records the current native comparisons, receiver coverage and remaining work.
- [Input, queued item effects and CPU destinations](../native_source_input_async_cpu_checkpoint.md)
  records the next shared operations, native comparison boundaries and remaining
  callback, CPU and companion integration work.
- [Accessory phases, CPU history and target delta](../native_source_callback_history_target_checkpoint.md)
  records owned callback timing, shared CPU operations and source migration limits.

- [Catch policy hooks](catch_hooks.md) documents holder-owned catch, pull,
  release and air-tether decisions, and caught-state notification timing.
- [Item ground following and landing](item_terrain.md) documents shared ground
  tracking and landing kernels with nested authored stage hooks.
- [Catch hooks, Item terrain and follower-query checkpoint](../native_source_catch_terrain_query_checkpoint.md)
  records native call order, interpreter integration and current validation limits.
- [Tether movement primitives](tether_controls.md) documents exclusion masks,
  damage-fall entry, ledge probes and the common second ledge-jump phase.
- [Tether movement and Article motion checkpoint](../native_source_tether_motion_checkpoint.md)
  records the retained-source migration and its current validation boundaries.
- [Target geometry, attachment release and Item controllers](../native_source_target_controller_release_checkpoint.md)
  records shared queries, native attachment release and imported state controllers.
- [Reflector, hitlag observations and stage-line admission](../native_source_reflector_hitlag_checkpoint.md)
  records physical receiver reads, actual reflected-entry ordering and current source preparation limits.
- [Model installation and held-item/shield observations](../native_source_model_graph_observation_checkpoint.md)
  records two-pose insertion, common native reads and the remaining tether collision gap.
- [Tether collision checkpoint](../native_source_tether_collision_checkpoint.md)
  records native landing continuations and successful preparation of all ten retained full sources.
- [Companion identity, history and Sleep checkpoint](../native_source_companion_identity_sleep_checkpoint.md)
  records immutable role-independent queries, constructor history and retained native cleanup boundaries.
- [Construction and teardown ownership checkpoint](../native_source_constructor_ownership_checkpoint.md)
  records failure unwinding, replacement-safe Player roles and unfinished creator-child cleanup.

- [Smash charge state](smash.md): per-instance charge modes, timing and native shake state.
