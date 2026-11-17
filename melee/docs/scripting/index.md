# Fighter scripting

Fighter scripts (`.fms` files) define a character's move behavior. Use them to
start actions, change velocity, spawn items, and respond to input, hits and
collisions. The Action Editor controls animation events such as hitbox timing;
scripts handle the decisions and behavior around those events.

In the Workshop's **Scripting** mode, keep each special move in its own file
and shared fighter variables in **Fighter state**. Use **Parameters** to adjust
settings such as speed, damage or cooldown duration.

Right-click a name in the editor and choose **Lookup symbol** to read its
reference page. **Open reference** lets you browse or search the documentation.

## Start here

- [AI agent tools](agent_tools.md) explains the installed CLI, MCP setup and
  structured project inspection.
- [Model authoring](agent_models.md) covers mesh import, source-aware reimport,
  rest transforms, extra bones, skin weights and materials through agent tools.
- [Resource authoring](agent_resources.md) covers owned images, item blueprints,
  grouped imports/removals, resource export and supported asset formats.
- [Workshop preview capture](agent_preview.md) covers camera controls and
  saving PNGs tied to the rendered pose and draft revision.
- [Script structure and callbacks](language.md) explains move and article
  blocks, callback order, and the difference between a script and a subaction.
- [Fighter parameters](language.md#fighter-attributes) explains project-owned
  constants, the Workshop Parameters section, stock-value import, and adjusting values in the Workshop.
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

## More topics

- [Animation command variables](command_variables.md): connect animation events to move logic.
- [Aerial actions](aerial_actions.md): hovering, airborne input and landing callbacks.
- [Article instances](article_instances.md): item scale and stage contact.
- [Dynamic bones](dynamics.md): movement of flexible parts such as tails.
- [Catch callbacks](catch_hooks.md): catching, pulling and releasing a fighter.
- [Item ground following](item_terrain.md): moving along terrain and landing.
- [Tether movement](tether_controls.md): ledge probes and airborne movement for a tether.
- [Smash charge](smash.md): charge duration and shake state.
- [Numbers, angles and vectors](math.md): calculations for movement and item steering.

## Examples

Start with a stock fighter's script. Open a custom fighter's draft to read
its own move files:

- [Mario](../../examples/mario_full.fms): fireballs, Cape, Super Jump Punch and Tornado.
- [Dr. Mario](../../examples/drmario_full.fms): pills and their movement and lifetime.
- [Peach](../../examples/peach_full.fms): hovering and selecting aerial attacks.
- [Fox](../../examples/fox_wip.fms) and [Falco](../../examples/falco_wip.fms): lasers, Reflector and movement specials.
- [Donkey Kong](../../examples/donkey_wip.fms): carrying and throwing fighters and heavy items.
- [Samus](../../examples/samus_full.fms): charged projectiles, bombs, missiles and grapple movement.

## Using the reference

Each page explains what a feature does, its inputs and return values, and when
to use it. Follow the links between related callbacks, properties and functions.
Examples use supported script syntax.

For project development and test results, see the
[Workshop progress notes](../native_character_workshop_progress.md) and
[Mario translation notes](../mario_script_translation.md).
