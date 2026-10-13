# Remaining complete Zelda/Sheik direct-source binding closures

This census predates the [combat callback checkpoint](native_source_contact_callbacks_checkpoint.md).
That checkpoint records the subsequently landed callback slots and two Zelda
effect rewrites; the remaining dependency groups below are still open.

Read-only source audit, 2026-10-08. Inputs are the current complete
`examples/zelda_full.fms` and `examples/sheik_full.fms`, the direct descriptor
catalog/resolvers/runtime units, the old translations, and the original native
special/article bodies. No production or template files changed; no game or
global build ran. This is a static inventory, **not complete package admission**.

`tests/native/source_form_binding_inventory/run.py` compiles the production
expression lexer and the exact current descriptor table plus Descriptor function
in a private executable. It reuses the balanced-postfix census, retaining call
arguments and chained selectors. Its descriptor executable never calls runtime
operations. The archived audit artifact is
`A:/FreakMelee-test-archives/source-form-binding-inventory-oct08-final/inventory.json`:
Zelda **151 distinct native structural paths**, Sheik **225**, zero unparsed
executable lines. Full occurrences, source lines, owners, access modes, descriptor
formal types, context masks and source hashes are archived there. An absent exact
catalog row is a candidate gap: receiver/event/owned-value resolvers are separate.
The tables below account for those exceptions explicitly. The old contract JSON
is not used to assert current acceptance. Root's combat callback cluster is
in flight after this census; its new rows are not claimed landed by this audit.

## What already exists

The complete templates already use genuine direct state entry/switching,
animation, numeric/math, random, physics, fighter load/item/knockback, reflector,
hurtbox, phase-helper callbacks, form activation, jump counters and pose methods.
The direct eight-operation pose catalog has typed `Pose.live`/`Pose.blend` and
owned graph literals; it is not a remaining numeric-graph compatibility problem.
Sheik's two `pose_graph` declarations still require their exact owned facts rows
and imported graphs. `event.flag` is the audited Bool item pickup/drop payload,
resolved by the event-payload resolver, not a missing native descriptor.

Item self life/half-life/commands/physics/collision flags, motion state,
`change_state_flags`, `interact`, lifetime expiry, gravity, reflection, sound,
pause/resume and manual lifetime already have Article-context descriptors.
`creator.exists`, `creator.input.*`, `creator.position.*`, and `holder.motion`
are retained receiver projections. Root parameters resolve as `attrs.*`;
Article imports as `item.attr.*`/`item.params.*`, with exact owner scopes.
These should not be reimplemented as a legacy Host table.

Two exact existing-descriptor arity mismatches remain in Zelda:

| Source | Direct rewrite | Reason |
|---|---|---|
| line198 `bones.root.rotate_x(0,0)` | `bones.root.rotate_x_joint(0,0)` or one-argument root `rotate_x(0)` | Old translation's first argument is raw joint; current root method takes only angle. |
| line870 `item.stage_collision(false)` | New exact Item ground-response primitive required | The Bool chooses two distinct native routines: true -> `it_8026D9A0`, false -> `it_8026DA08` (`native_fighter_item_script.c:1032`, module.c:1037). Current zero-argument method only calls the first; deleting false changes behavior. |

| Source | Exact descriptor paths | No exact row | Native occurrences |
|---|---:|---:|---:|
| Zelda | 93 | 58 | 539 |
| Sheik | 117 | 108 | 965 |

These counts include supported receiver/event projections among the no-exact-row
column; they are not counts of missing APIs.

## Callback and cue closure (first bounded implementation)

| Authored use | Actual old/native contract | Current route / next step |
|---|---|---|
| Zelda103 `callbacks.track_reflect_hit()` and488 `on reflect_hit` | Install the authored event into `Fighter.reflect_hit_cb`; original neutral-special reflect hit turns command0 back to1. `tools/native_fighter_script_primitives.c:850`, `ftZelda/ftzeldaspecialn.c`. | Root is implementing generic ReflectHit/Reflect/HurtboxDetect/ShieldHit tracked events and helper bindings, actor reset/native delivery/snapshots together. Do not substitute `deal_damage`. |
| Sheik149 `callbacks.death2_is(sheik_resource_cleanup)` / `damage_is(...)` | Compare the currently installed corresponding authored callback identity before clearing both. `tools/native_fighter_script_functions.c`; this prevents cleanup from erasing a callback installed by nested work. | Missing identity queries. A generic owned-helper/None CodeRef comparison against the actor's current code-ID slot is sufficient; require current bank, correct slot and helper ABI. No pointer introspection or blanket clear rewrite. |
| Sheik707/721 `cue.consume(0)` | Read and clear `throw_flags_b0` once. `tools/native_fighter_script_primitives.c:530`. | Rewrite to existing `cue.reverse.consume()` (same bit0). `cue.article.consume` is bit1, `cue.throw.consume` bit2. No general cue API is required for these uses. |

Existing `callbacks.damage/death2/pre_hitlag/post_hitlag/accessory` take a real
owned root helper or None. All Chain pause/resume/destroy ordering remains
authored; no serialized per-fighter callback policy should replace it.

## Effects and presentation

| Source family | Contract and actionable disposition |
|---|---|
| Zelda107/113 `effects.spawn_on_bone(id,TransN)` | The old emitter resolves semantic FtPart, calls `efSync_Spawn(id,gobj,joint)`, **then sets x2219_b0 true** (`native_fighter_script_code.c:1435`). Rewrite to `effects.spawn(id,TransN)` followed by `effects.active=true`; existing spawn alone omits that state write. Preserve order and retirement semantics. |
| Zelda239; Sheik226/553/565 `effects.spawn_at_position(id,x,y,z)` | Old `efSync_Spawn(id,gobj,&Vec3)`, no active-flag write. Genuine missing verified position-payload dispatch; joint/scalar APIs and model `effects.spawn_root(Name)` cannot preserve it. Admit only IDs proven to consume the exact synchronous Vec3 ABI, with caller postguard. |
| Zelda795/810/939 `item.spawn_root_effect(id)` | Old `efSync_Spawn(id,gobj,gobj->hsd_obj)` on the actual Item root, no automatic flag. Missing Article root-joint effect primitive. Existing `item.spawn_effect_on_bone(id,raw)` operates Item part table and supports audited mode; it is not a root/path substitute. |
| Zelda807 etc `item.destroy_effects()` | Actual `efLib_DestroyAll(gobj)` through old `destroy_effects` slot. Direct `effects.destroy_all` is Fighter-only. Add the genuine Article-context primitive/dispatch; retain effect_active source writes. |
| Zelda/Sheik `visuals.set_visible(bool)` | Set `Fighter.invisible=!bool`; not equivalent to held-item visibility or model-group hide. Missing typed Fighter visibility property/operation. |
| Zelda496/497, Sheik930/931 `visuals.set_group(group,value)` | `ftParts_80074A4C` via `set_visibility_group`; validated group and value(-1..255). Current `visuals.hide_group(group)` cannot select arbitrary group alternatives. Add generic selection contract, preserving owned rig bounds. |
| Sheik172/934 `color.start(id,bool)` | `ftCo_800BFFD0(fighter,id,mode)`; native color-animation state, not a joint/material substitute. Missing generic color-animation operation. |
| Zelda918..926 `item.rumble(pattern,frames)`, `camera.quake(id,x,y,z)`; Sheik1627 `audio.play_global(sound,volume,pan)` | Existing native feedback leaves are `it_80273598`, `Camera_RequestQuake`, and `lbAudioAx_800237A8` respectively (old module bindings and translation evidence). Genuine distinct recipient/global effects, not Item local play_sound. Require exact native signatures and ownership/lifetime publication before admission. |

`effects.spawn_on_part`, `wind_at_position`, semantic `wind`, scalar effect
variants, and `pause_during_hitlag` are already direct. Do not add spelling aliases
that hide semantic/raw joint or Vec3/scalar ABI differences.

## Fighter movement, terrain and combat data

| Used path | Actual contract / closest current API |
|---|---|
| `collision.check_facing_ground_ledge()` | `ft_CheckGroundAndLedge(gobj, facing<0 ? -1 : +1)`. Rewrite using existing `collision.check_ground_ledge(mode)` with explicitly authored mode; `int(facing.direction)` only preserves this if the facing invariant is established. `ftGetFacingDirInt` also treats zero as+1. |
| `collision.skip_soft_floor()` | `ftCo_8009A134(gobj)` Bool; tests stick/platform conditions, can mutate collision state. Genuine missing query, not ground-contact/landed. |
| `facing.from_input_threshold()` | `ftCommon_8007DA24(fighter)` versus current `from_input`=`ftCommon_UpdateFacing`. These differ at stick thresholds; add the exact generic threshold operation. |
| `physics.force_air_and_use_jumps()` | `ftCommon_8007D60C`: grounding/jump-count lifecycle, not simply airborne=true+jumps.used=max. Missing generic primitive; use actual function. |
| `physics.switch_to_ground()` | `ftCommon_8007D7FC` transfers movement to ground; current state switch methods also select an owned motion/start frame. No exact existing API. Add the actual movement-space switch if needed; do not call a state transition then undo it. |
| `items.block_ground_pickup(bool)` | `Fighter.x2223_b4` read/write policy bit. Genuine missing typed property/operation. |
| `state.land()` | Old `enter_landing_basic`; rewrite existing `state.basic_landing()` with its audited basic-landing contract. |
| `air.terminal_velocity` | F32 common fighter terminal velocity (`co_attrs.terminal_vel`). Missing readable common-attribute property; `air.gravity`/`friction` do not supply it. |
| `controls.stick_smoothing` | F32 shared `NativeFighterCommon_StickSmoothing` common attribute. Missing, not local input or source parameter. |
| `model.instance_scale_y`, `creator.model_scale` | First is actual `fp->x34_scale.y`; latter is `ftLib_800869D4` target model-scale query. Distinct values; no existing scale namespace. Add genuine read properties before receiver rewrite. |
| `ceiling.normal.{x,y}`, `wall.left.normal.{x,y}`, `wall.right.normal.{x,y}`, `ecb.{top,bottom}.y` | F32 members of actual CollData; floor.normal exists, these do not. Retain separate collision surfaces and actual ECB offsets. |
| `fighter.creation_id` | U32 native spawn identity `x8_spawnNum`. Missing public typed read. Needed only for the old scalar owner comparison; preferred retained owner equality avoids inventing raw engine references. |
| `vectors.angle_xy(ax,ay,bx,by)` | Connected direct native XY routine with four F32 inputs, zero Z and native zero/nonfinite/rounding behavior. Shared3D angle and normalized cross components also execute directly. [Evidence](native_source_vector_checkpoint.md). |
| Sheik `stage.sweep_box` | 12 F32 geometric inputs plus integral mode -> environment flags. Initializes CollData, pushes previous then current, sets b1234, sweeps fixed ECB (`native_fighter_script_collision.c`). Missing generic bounded query; no allocation/per-fighter case. |
| Sheik `hitbox.current(i).{x,y,z}`, `previous(i).{x,y,z}` | Fighter x914 capsule x4C/x58 F32 components, bounded0..3. Missing captured-index RW selector family; retain both positions. |
| Sheik `hitbox.set_enabled`, `clear_victims`, `set_world_position`, `clear_all` | Native capsule enabled state, `lbColl_80008440` victim-list clear, world hit position, and `ftColl_8007AFF8` all disable. Direct `set_enabled`, `clear_victims` and `set_world_position` now preserve these contracts. Remaining clear-all admission is separate; Article hitboxes are a different layout. |
| Zelda Article `hitbox.enabled/radius/set_damage/set_radius`, `hitboxes.disable_all`; Sheik `item.disable_hitbox` | Item x5D4 hit capsule, native damage/radius setters (`it_80272460` etc) and `it_80272560`/all disable. Add Article-specific bounded generic contract, do not reinterpret Fighter x914. |

Zelda's Din collision body uses exact catalog spellings `collision.ceiling_contact`,
`floor_contact`, `left_wall`, `right_wall` **in Article scope**. Current descriptors
are Fighter-only; accepting names would otherwise conceal the wrong native
layout. Add Item-CollData context implementation along with the Item collision
family, not just the A mask.

## CPU planning and retained target query closure

Both sources use `cpu.pending_action` (Bool RW actual cpu.xF8_b5). Zelda additionally
uses I32 `cpu.behavior` RW/x18, `previous_behavior` R/x1C, destination.x F32 R/x54,
`cpu.clear_target` (x44=NULL), `cpu.nearest_enemy_at(x,y,z)` and `cpu.queue`.
The direct CPU family now covers pending/behavior/previous/destination fields,
clear-target, queue and nearest-enemy operations. [Evidence](native_source_cpu_target_checkpoint.md).

`cpu.queue` is an authored fixed queue action, not a `run_script` alias. Used
recipes are `Cpu.done`, `release_b`, `stick_x`, `stick_y`, `wait`,
`x_toward_destination`, `press_b_for`; the old translator selects actual CpuCmd
and zero/one byte operands, including defined int->u8 modulo for negative sticks.
The load-time Cpu constant family now selects51 audited native recipes with
exact arity and whole-command capacity preflight before native byte appends.

`cpu.nearest_enemy_at` uses `ftCo_CpuNearestEligibleTarget`, returns a scalar
Fighter spawn handle in the old query; `targets.read_position` uses
`nativeFighterTarget_Position`, writes three F32 destinations on success only.
Captured target position/airborne/identity queries now preserve output publication
and distinguish missing IDs from captured/caller retirement without raw pointer
ScriptValues. The CPU body retains its finish-without-target/reset behavior;
omitting the hooks is not complete source activation.

## Article construction, reference and authored dispatch closure

| Used family | Actual reference contract and required direct ownership |
|---|---|
| `articles.create_at_position(kind,x,y,z,facing,origin_at_owner)` / Sheik `create_with_velocity(kind,x,y,z,vx,vy,vz,facing,origin_at_owner,collision_group,value,initial_collision)` | Connected direct factory lowers a literal owned declaration. The Bool selects current damage-owner center, not grounding; supplied previous position remains unchanged. Preserves full collision-group word, scalar creation value, native constructor/defaults, normal allocation failure(-1), creator identity and source epoch. [Evidence](native_source_article_factory_signal_checkpoint.md). |
| Zelda `item.create_child(kind,x,y,z,facing,current_owner,group,charge,variant)` | `nativeArticle_CreateChild`, propagates author/creator/current owner and creation_value into explosion. Existing spawn resolver accepts only root Fighter factory calls. Need actual Item-source factory+retained owner admission; no per-kind explosion handler or native authored callback fallback. |
| `articles.ref(handle,DeclaredArticle).state.*` / `.current_owner`, `articles.read_primary_translation(handle,DeclaredArticle,outx,outy,outz)` | Direct source references now lower exact Article/field identities and capture one live Item in the invocation receiver pool. Missing references/OOB indices retain zero defaults; captured retirement/schema changes fault instead of retargeting. Primary model **local translation** uses typed engine JObj and SuccessOnly outputs, never Item position/world point. [Evidence](native_source_article_reference_checkpoint.md). |
| `articles.signal(handle,DeclaredArticle,"name",0)`, `item.signal("name",0)` | Connected compiler/runtime lowers names to exact callback code identity. Targeted callers currently Fighter only; self delivery requires Article owner. No runtime name lookup. Preserves synchronous nested dispatch/source retirement and current state/helper frame. Missing target returns false; handler/caller faults do not masquerade as absence. [Evidence](native_source_article_factory_signal_checkpoint.md). |
| Sheik `articles.bind_auxiliary(handle)` | Direct same-bank source Item/original-creator admission and actual Fighter auxiliary assignment, reflection independent. [Evidence](native_source_article_lifecycle_checkpoint.md). |
| `creator.call(Name,args...)` | Rewrite `creator.helpers.Name(args...)` to existing typed retained helper projection. Original helper/result/formal ABI and same pinned root program required; no runtime string search. |
| `targets.call_bool/int(item.current_owner,"Name")`, `targets.call(...)` | Old recipient dispatch returns default if target/export missing. A source rewrite to `current_owner.helpers.Name()` can use typed helper IDs, but must explicitly check `.exists` when old absent/default is meaningful and validate the same owned program. Cannot silently demand any foreign owner provide this schema. Sheik Cleanup discards old target call's presence Bool; typed Void helper should be called as a statement under the intended presence guard. |
| `item.attachment_active`, `item.attachment_owner_is(other)` | Attachment activity compares mutable damage owner with revocable attachment control. The identity predicate compares a captured object with that attachment relation; neither operation compares immutable creator provenance. |
| `item.clear_current_owner`, `clear_attachment_owner`, `detach_owner`, `attach_to_owner`, `track_accessory` | Clear/detach/attach now use real actor-owned relations and native pickup; creator and revocable attachment are distinct. Accessory tracking remains open: use actual prepared code-ID callback substrate, not native authored callbacks or no-ops. [Evidence](native_source_article_lifecycle_checkpoint.md). |

`creator.input.*`, `creator.position.*`, `.exists`, holder.motion need no new
native API. `creator.model_scale` does. `attrs.chain_attachment_part` inside
Chain created is a **root import**, while Article attrs resolve exact Article
owner. Rewrite to `creator.attrs.chain_attachment_part` under the actual retained
creator contract; do not make article attributes inherit Fighter parameters.
Article local `item.attr.*` and `item.params.*` should keep their existing scopes.
Sheik's `targets.airborne(item.current_owner)` can rewrite to the existing
`current_owner.airborne` receiver property under the intended presence guard;
the old scalar target query's absent-false behavior must remain explicit.
`other` in Item interaction is already the event-payload Object alias. No guessed
local or raw engine pointer is needed. `nativeScriptItem_RunSignal` is the real
signal-execution substrate; what is missing is load-time symbol/recipient
lowering and guarded binding admission, not a second event scheduler.

Additional Item self gaps: `begin_expiry`=`it_8027518C` differs from set_lifetime;
`advance_animation`=`Item_802694CC`; `inherit_owner_debug_display` uses actual
current owner; `set_scale` writes root SRT uniform scale without changing Item.scl (old helper
in native_fighter_module.c), while existing `item.scale` is read-only;
`hide_model` sets root hidden flags; `creation_value` is constructor payload,
not creation_id. These need real generic contracts. `item.remove()` is not a
mere Void leaf: old emitter calls Item_8026A8EC **and returns from the authored
callback/helper**. Define direct retirement/unwind semantics before rewriting
its failure paths; an error fault is not an equivalent successful deletion.
The existing expire_tick uses `it_802751D8`; original Din and explosion use
`it_80273130`. Both actual bodies decrement the same life timer and test≤0
identically, so this is an audited shared equivalent, not a remaining alias gap.

## Sheik Chain resource / Item terrain closure

Chain metadata declares `resources capacity 30`, `model_root Link 100`,
`model_root Tip 104`; runtime uses dynamic link_count (retail source comment20),
bounded≤30, three endpoints,15-entry history and12-F32 matrix. Model-root numbers
are DAT pointer-field byte offsets, not raw part IDs or guessed graph slots.
Current Item metadata accepts article states/imports/projectile/attachment;
resource/model_root lines have no typed metadata admission. The new generic
source resource **bundle codec** is separate from these runtime ItemLink pools.
Import exact model descriptors and immutable roots before accepting the bank.

The coherent runtime family required by all Chain uses is:

* `item.resources.begin(count)->Bool`, `release()->Void`.
* `item.resource(index).create(raw_part,F32 radius,category,x,y,z)->Bool`,
  `load_model(Link|Tip)->Bool`, `set_draw(Bool)->Bool`.
* `position.{x,y,z}` and `velocity.{x,y,z}` F32 RW; CollData position/previous
  components F32 RW, environment flags R, surface_line I32 RW.
* `joint("").translation.{x,y,z}` F32 RW on the actual resource model.
* `sweep_point()->Bool`, `read_attachment_point(x,y,z,outx,outy,outz)->Bool`,
  `read_attachment_matrix(matrix[12])->Bool`, `set_world_matrix(matrix[12])->Bool`.

`tools/native_fighter_article_resources.c` retains each index/path and typed
channel; `src/melee/it/native_article_resource.c/.h` has substantive bounded
ItemLink/model/sweep/matrix functions, but collection ownership currently
requires legacy Definition+article logic+native_article_creator. Port that
ownership to actual pinned source owner/program/scene/creator generation as part
of the real pool implementation; do not wrap that failing Definition path or
claim codec availability makes runtime resources ready. Retain deferred release
during synchronous sweeps/model dispatch, captured selector before RHS,
per-resource generation, scene reset and snapshot behavior. Matrix args are
bounded typed authored arrays, not raw native addresses or copied pointer views.

Needle terrain additionally needs `item.sweep_segment(previousxyz,outnormalxyz,
outline,outflags)` ->Bool via `it_8026EA20`; success-only outputs;
`item.bind_stage_line(line)` via `mpColl_80043558`; `item.read_stage_line_speed`
via `mpGetSpeed` uses actual item position and writes actual velocity;
`stage.line_exists`=`mpLib_80054ED8`, `line_normal`=`mpLineGetNormal` with typed
outputs. Existing self stage_collision/bounce_stage are different algorithms.

`item.joint(model,path)` across Din/Needles and all resource joints requires
prepared decoded ordinal paths+model selection at load, actual nativeModelJoint
channel reads/writes and caller/owner generation checks. The existing Effect
joint selector is not an Article receiver; common model primitive can be reused
internally. Preserve the already-audited missing-path default/no-op contract
where applicable, and reject malformed declarations/indices before publication.

## Order and acceptance boundary

1. Root's bounded combat callback cluster, then owned callback identity queries
   and safe spelling rewrites (cue0, raw root rotation, basic landing, bone spawn
   active flag, creator helpers). This does not by itself complete either form.
2. Common Fighter terrain/presentation/CPU primitives with native body/reference
   comparisons; no special-fighter operations or policy shortcuts.
3. Owned positional/child constructors+Article references+signals+recipient
   helper admission+Item self/root model/feedback. This is Zelda's Din closure and
   Sheik's held/thrown Needle/Vanish closure together.
4. Source-owned resource metadata/import+ItemLink lifecycle/selectors/matrices+
   Item terrain and Fighter capsule data: complete Chain, not a fake static chain.
5. Fresh actual owned DAT/facts resources, both members' complete typed bodies,
   source preparation, real transfer/scene/snapshot evidence. Missing asset roots
   are distinct from unsupported bindings. No legacy handler delegation or
   ignored callbacks qualify as complete activation.

Exact matching names still need contextual signature/typed body checks; the
census cannot prove arithmetic width, field access, output-lvalue eligibility,
fallthrough return, all function bodies, or candidate asset lifetime. Existing
whole-form preflights stop at the first error and should be rerun only after a
coherent dependency closure, not treated as this inventory's exhaustive proof.

## Article control closure checkpoint (2026-10-08)

Primary `item.joint(0, path)` visibility/existence, rotation and scale now have
literal prepared selectors, plus native uniform scale/animation-frame operations.
The indexed Item capsule methods and four stage-contact properties have their
own Item implementation. Item-source positional/child creation is connected to
the actual source factories and VM. Selector tables live in a fixed aggregate
immutable pool and are rebased with prepared-program copies.

The actual owned Zelda source-check now stops at line 802,
`item.inherit_owner_debug_display`; Sheik still stops at Chain metadata line 1543.
This does not close animation bundles, secondary model slots, resource collections,
joint world/matrix queries or the remaining Item primitive families. Evidence and
controlled boundaries are in
[the Article checkpoint](native_source_article_controls_checkpoint.md).

The follow-up [advancement checkpoint](native_source_article_advancement_checkpoint.md)
adds actor-owned creation values, real DATA advancement, revocable attachment and
current-owner queries, native owner-debug/map collision, rumble and quake behavior.
Zelda's complete bodies now compile; preparation stops at its typed
`cpu_pending_action` scheduler contract (line 421). Sheik remains at Chain
metadata 1543. Neither member is fully activated by these component checks.
