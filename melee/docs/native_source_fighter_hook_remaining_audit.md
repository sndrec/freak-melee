# Remaining source Fighter hook scheduler families

Read-only audit, 2026-10-08: the ten current stock `examples/*_full.fms` files,
`native_script_events.c`, `native_script_fighter_hooks.c`, PrepareProgram, actor
dispatch and actual shared native delivery sites. Only this document was added.
No production, template, build, game or commit changes. CPU migration belongs to
the other worker; its27 root contracts are excluded from the remaining list.

Later implementation: the [held-open/jump/dynamics checkpoint](native_source_held_jump_dynamics_checkpoint.md)
connects held-open identity/entry, per-set dynamics decisions and owned animated
jump physics. The wind and ground policy contracts remain separate open work.

The current root contract table has53 slots:26 nonCPU and27 CPU. Name presence in
the event catalogue is **not installed scheduler support**. PrepareProgram rejects
unknown root events rather than ignoring them; supported mutable instance events
are handled by a separate callback table and need explicit authored registration.
Motion/state phases and action `update` are separate classes, not missing root
hooks. Article/effect `on ...` declarations were excluded by their actual owner
sections; same-spelling Article hooks are not Fighter evidence.

The declaration census found35 used nonCPU hook names missing from both root
contracts and the installed instance-event slots. This is a static scheduler
inventory, not body/binding/asset admission. Every named family below has native
delivery evidence; declaring its ABI alone would still not activate the clone.

## Already installed and immediate form implication

Root static delivery covers load/load_special_attrs, tick/late_update/removal/death,
landing/landing_action, eight ground/air special entries, pickup/drop/ext Bool
payloads, held-item visibility, knockback enter/exit, aerial_jump and form_activated.
CPU capture/ranged are in the CPU family. The separate actor instance slots cover
damage, death1/death2, pre/post hitlag, deal_damage, state_change, reflect_hit,
reflect, hurtbox_detect and shield_hit. Death2 also has two native death-slot uses.
Those declared functions must be bound/tracked where the authored behavior wants
them active; they do not constitute a missing scheduler family.

After the CPU slice, current Zelda has **no remaining declared nonCPU root-hook
gap**. Sheik needs color_animation_reset at line932, independently of Chain
metadata/resources/terrain/capsules. This says nothing about complete form assets
or gameplay. Source publication explicitly clears copied stock callback tables
with ftData_AssignStockCallbacks(kind,Ft_Kind_None)
([runtime](../src/melee/ft/native_fighter_runtime.c:633)); raw ftData callback sites
therefore cannot secretly supply the missing authored behavior.

## Family1: color, common presentation, demo and taunt

| Used hook / templates | ABI | Real native delivery and order |
|---|---|---|
| color_animation_reset — Sheik932, GameWatch1339 | Void() | ftcolanim.c:171/206/270/280, ftData_UnkMotionStates4. Reset x488 overlay, deliver character callback, then common color selection. Cover all four sites including both animation-completion loops. Sheik/GW reapply their authored full-charge color. |
| presentation_item — DrMario320 | Void() | ft_0BEC.c:49/114/152 and ft_0BF0.c:37/78: after common presentation motion/setup, replacing character-specific item presentation. Deliver after actual source actor attachment in these entry paths; no legacy Definition thunk or per-kind item constructor fallback. |
| demo_enter — DrMario338 | Void(I32 mode) | ftdemo.c:172 after on_create_fighter[mode] and source-life check, before ftLib_800867E8. Preserve actual mode and actor/source lifetime; audit demo actor attachment separately from versus entry. |
| taunt — DrMario394, Peach1516, YoungLink3424 | Void() | ftCo_AppealS.c:62. Authored event replaces character-specific taunt entry, then pl_80040120 runs if the source still lives. Missing source handler uses a genuine common taunt entry, not a stock fighter dispatch. |

Ownership: root adds static ABI/PrepareProgram rows; the shared color/presentation/
demo/taunt units deliver them at these exact points. Add generation/kind/bank
guards after events and inside color loops before touching fp again. Do not call
the event once per general tick or turn taunt into a motion accessory callback.
This small cluster closes Sheik's immediate nonCPU scheduler error and the
presentation portion of DrMario/GW without pretending to close their other gaps.

## Family2: Peach held/common movement and aerial action arbitration

| Used hook | ABI / authored fallthrough | Native site and concrete behavior |
|---|---|---|
| held_motion — Peach1463 | I32(I32 motion,I32 previous_motion); incoming motion | native_fighter_held_motion.c:9, called by ftCo_ItemParasolOpen.c:59 and ItemParasolFallSpecial.c:24. Select only common/owned valid motion; invalid authored selection currently returns common motion. Replace Definition admission with source layout, retain previous-motion payload. |
| neutral_enter — Peach1454 | Void(F32 blend) | ft_08A1.c:99 before selecting Wait/common neutral motion. Its source notifies the actual held instance; retain that distinction from saved creator handles. Guard again before motion selection and subsequent common setup. |
| air_resume_motion — Peach1476 | I32(I32 previous_motion); incoming previous_motion | ft_0D27.c:79 mushroom resume mapping. Validate result against common + actual owned motion range before the existing resume switch; do not treat scalar0 as an identity mapping. |
| held_timer_expired — Peach1470 | Bool(); false | fighter.c:1797 after countdown reaches0 and its active bit clears. True handles expiry; false runs ftCo_80095744. Source handler itself decides parasol/reopen policy. Guard before generic fall and the rest of fighter update. |
| grounded_motion_setup — Peach1062 | Void() | Fighter_ChangeMotionState, fighter.c:1305, inside grounded branch before common held/ground-state flags reset. It is not merely landing; its source sets float readiness on every applicable grounded motion transition. |
| aerial_action_manual / automatic — Peach1064/1069 | Bool(); false | ftCo_AerialAction.c:10/31 shared input arbitration. True consumes the input; false lets the existing common chain continue. Error/retirement must consume/stop continuation, not be mistaken for false. |

Ownership: common movement/held selection and fighter motion/update sites together.
These events do not delegate Peach native special policy. Missing source handlers
must retain the appropriate generic common behavior; raw stock branches remain
only for genuine raw stock actors. Mapping return validation uses the published
source motion layout, not legacy Definition bounds or guessed constants.

## Family3: attack entries and common wait cleanup

| Used hooks | ABI | Real entry boundary |
|---|---|---|
| wait_enter / crouch_wait_enter — Link3328/3330, YoungLink3401/3403 | Void() | ft_08A1.c:136 and ftCo_SquatWait.c:82 after common motion/item/common flags. Replace the character switch cleanup tail, then return; preserve branch-specific preceding common behavior. |
| jab_enter — GW907 | Void() | ftCo_Attack1.c:95/215, both jab entry paths. Source selects authored jab and its accessory helper. Do not invoke again from generic phase animation. |
| rapid_jab_enter — GW909 | Void() | ftCo_Attack100.c:188. Source sets accessory even when common pickup consumes the attempted rapid entry; preserve that ordering. |
| down_tilt_enter — GW913 | Void() | ftCo_AttackLw3.c:78. Replaces character entry; source does its own pickup check, custom motion/start and helper binding. |
| aerial_attack_motion — GW925 | I32(I32 event.motion); incoming selected motion | ftCo_AttackAir.c:81: compute native C-stick selection, invoke mapping, validate common/owned motion, enter it. Invalid result returns the selected motion, not0 and not a stock-specific decider. |
| aerial_attack_enter — GW930, Link3319, YoungLink3392 | Void() | ftCo_AttackAir.c:166 after actual motion entry and ftAnim_8006EBA4. Stop if either motion/animation startup or event retires the source. |
| forward_smash_start — Link3339, YoungLink3412 | Void() | ftCo_AttackS4.c:164 after doEnter; this is a post-common-entry hook. |
| forward_smash_enter — GW919, Peach347 | Void(F32 event.angle) | ftCo_AttackS4.c:160 replaces the entry itself before doEnter. Native sets facing first. It has precedence over start; invoking both sequentially changes behavior. |

Ownership: shared Attack1/100/Lw3/Air/S4 plus Wait/Squat tails. This cluster must
preserve replacement versus notification semantics, pickup early returns and
animation-start retirement. Existing generic source primitives should remain the
body implementation; do not introduce fighter-specific enter functions.

## Family4: tether catch/pull/capture pair lifecycle

Link and YoungLink declare catch_allowed/start/frame/exit/wait_release,
catch_pull_anchor/ready/follow, capture_wait_enter and air_tether_input at
Link3229..3253/3422 and YoungLink3299..3323/3542.

* Catch.c:26/55/84 uses Bool catch_allowed as a replacement for stock-special
  eligibility tests. False denies; retirement consumes input and stops. Catch
  start is Void at140. Bool catch_frame at159/179 handles an animation frame;
  false continues the common animation-finished path. Exit is Void at224.
* CatchWait.c:73 invokes Void catch_wait_release. CatchPull.c:62 invokes Bool
  catch_pull_anchor; false restores the actual common bone anchor. At99,
  catch_pull_follow selects the actual common accessory1 follow callback. At119,
  catch_pull_ready controls the transition. Retain true/false meanings and guard
  both holder and captured victim through subsequent motion/animation work.
* CaptureWait.c:68 runs Void capture_wait_enter **on the holder actor**, while its
  native function is processing the caught victim. Payload `other` is the caught
  Fighter. Capture both full GObj references/data/generations and pin the holder
  program; postguards must also check the pair relationship. Do not execute the
  holder's code with a victim context or reacquire victim_gobj after nested work.
* AirCatch.c:60 delivers Bool air_tether_input in a shared airborne interrupt
  chain; true consumes, false continues, retirement stops. It must not call the
  old native Link/Samus tether path for an attached source actor.

Ownership: Catch/CatchWait/CatchPull/CaptureWait/AirCatch, generic receiver payload
capture/import and source Item resource borrowers. This family is coupled to real
tether resource/anchor lifetime, auxiliary Item identity and all source helper
bindings; do not claim it closed by adding10 event IDs. Use existing controlled
pair-dispatch conventions from other event payloads, with no pointer ScriptValue.

## Family5: dynamics, body contact, absorption and guard registration

Peach declares dynamics_interaction_start(set,set_count)->I32 at334 and
body_rebound()->Bool at337. ftdynamics.c:415 invokes the former before each native
lb_8001044C set update. Its return is first active **bone within that DynamicsDesc**,
not a set-array index; default authored fallthrough0. Guard native descriptor and
source after every event before advancing cached dyn pointers. Wind/ground
collision policy queries belong in the same eventual dynamics cluster.

native_fighter_contact.c:8 asks body_rebound on the **recipient**, not the source,
only after the contact flag/unk_gobj relation is established. Existing early
non-contact behavior is true; absent relevant recipient policy is false. Retain
source and recipient full identities, relationship and program through dispatch.
The old ftPeach special body also calls its recipient policy directly; the direct
source route must use the shared generic contract, not that special implementation.

GameWatch1298 declares absorb()->Void. Actual delivery is fighter.c:3088 via
ftData_OnAbsorb after AbsorbAttr direction is observed. Dispatch before damage/
count/direction reset and guard native continuation. Source callback tables are
cleared, so merely retaining that array lookup delivers nothing to a source actor.

Link3332, YoungLink3405 and Peach830 declare guard_hit. The old translator passes
`generated.on_guard_hit` directly to ftColl_8007B1B8; that native constructor sets
Fighter.shield_hit_cb. This is **instance callback registration**, not a root
frame scheduler. Existing typed ShieldHit slot/call site can provide the delivery,
but the guard creation/binding ABI and declarative spelling need an explicit
generic contract. Either resolve the guard-created handler to that slot or make
a source rewrite to on shield_hit + explicit binding with proved constructor
ordering. Do not add a duplicate static on_guard_hit delivery or silently alias
unrelated shield transition hooks. No such rewrite is performed by this audit.

## Trail requires a typed output record, not nominal hook admission

Link3302/YoungLink3375 declare on trail. Event catalogue explicitly sets
signature_supported=false because its argument is NativeFighterTrail*, with no
valid ScriptValue type. ftafterimage.c:read_trail explicitly rejects source
packages before legacy Definition handling. Adding a root enum would not fix it.

The real NativeFighterTrail record contains I32 bone/axis, F32 distances/fades and
I32 alpha/RGB components. Define a bounded invocation-owned typed output record
or complete scalar output marshalling, initialized with bone=-1, validated by
nativeFighterTrail_Valid against actual owned rig. Publish only after source/payload
guards. Use its real current joint matrix and three-sample renderer; no mutable
native pointer or invented static ribbon. Bone is costume joint index, not FtPart.
This presentation-record closure is distinct from ordinary Void callbacks.

## Other declared nonCPU hooks not used by these ten current templates

These remain unsupported as declared frontend events. They are grouped here to
complete the catalogue audit without treating hypothetical future templates as
the immediate implementation scope.

| Family / names | Actual shared native ownership and return boundary |
|---|---|
| throw_animation, throw_finish, throw_victim_motion, throw_victim_setup | Throw.c multiple ground/air animation/finish sites and motion selection; Thrown.c:35 setup. Void, Bool false, I32(motion,victim_motion) incoming victim_motion, Void. Guard thrower/victim pair and motion/animation transitions, including reversed setup recipient ownership. |
| heavy_pickup_finish, item_swing_motion, heavy_throw_enter, roll_enter, heavy_item_damage | ftpickupitem.c:376, ftswing.c:59, ItemThrow.c:376, Escape.c:87, Damage.c:986. Pickup false invokes ftCo_80096D9C; swing I32(type,variant) defaults-1 but must validate actual motion before access (old invalid branch asserts); other hooks Void. Item life/pair identity may change during entry. |
| wait_input, action_status, action_metadata, idle_variants, motion_is_angled | Wait.c:63, ft_0892.c:162/266/279, ftwaitanim.c:70. Bool false policies, metadata I32(I32) defaults incoming metadata. Runtime queries must stop/fault on retirement instead of exposing arbitrary native pointers/offsets. |
| jab_restart, jab_start, up_smash_enter, down_smash_enter | Attack1.c:114/249/329, AttackHi4.c:50/93, AttackLw4.c:49. Bool restart(next_motion), I32 start flags(default0), Void smash entries. Both restart paths and up-smash variants are substantive call sites. |
| auxiliary_item_attach, auxiliary_item_visibility_reset | ftCommon_8007F948 inline and ftCommon_8007F8E8, raw ftData_UnkMotionStates1/2. Void after real held/capture-slot checks. Typed callback arrays are cleared for sources; source-owned Item/auxiliary identity must replace the raw-table assumption. |
| shield_break, shield_start, shield_reflect_start, shield_transition, shield_scale, shield_alpha, spotdodge_enter | ShieldBreakFly.c:49, Guard.c:182/212/352/522/607/679/817/838/948/987/1109, Escape.c:243. Transition Void(I32 mode,Bool special_hit); scale F32 default0; alpha Bool false; others Void. Preserve replacement versus post-common-start notification and input/transition order. |
| capture_wait_setup, catch_pull_frame | CaptureWait.c:26 holder+other Object, Void; CatchPull.c:42 F32 default0. Same pair retention/anchor lifecycle as the used tether cluster. |
| dynamics_wind, dynamics_ground_collision, worn_item_jump_sound | ftdynamics.c:409/596/665, ItemScrew.c:67. Bool policies(defaultfalse), actual generic wind/ground collision/sound ordering, no stock special delegation. |
| secondary_damage, grab_catch, item_catch, grabbed | Fighter.take_dmg_2_cb / grab_cb / x2194 / grabbed_cb, fighter.c:2977/2672/2689/2677. Expand actual instance slots, reset/snapshot/binding code, and pair payloads. grabbed receives the victim as source with the grabber as `other`; raw native slot assignments may supersede script bindings. |

## Shared migration rules and next coherent order

The actor's runFunction returns scalar zero for an absent function. That is not
the event catalogue's authored fallthrough: incoming-motion/metadata defaults,
minus1 and context-specific common behavior must be supplied by the actual
HasHook/caller path or an explicit generic contract extension. Do not run an
absent I32 mapping as0. Likewise, an authored Bool false is a successful policy
answer; Error/SourceRetired must stop native continuation and never trigger a
stock-special fallback. Some input callers intentionally consume on retirement.

Add static root contracts and exact argument/default metadata alongside native
sites, not ahead of them. Pair events need retained Object import with scene epoch
before publication. Recheck physical slot/generation/data, native spawn generation,
kind, pinned program/layout and relationship after every effectful hook or engine
stage. If the source changes motion/form or nested code replaces held/auxiliary
state, cached fp/item/dynamics pointers cannot simply continue. Snapshot callback
selection and validate restored identities through the existing actor system.

Recommended next order: color/presentation/taunt cluster; Peach common held and
aerial arbitration; attack entries/wait cleanup; resource-coupled tether pair
lifecycle; dynamics/contact/guard registration; typed trail output. Add the unused
related hooks only where the same actual native sites make a coherent closure.
Zelda's next failures after CPU acceptance are body/asset/Item contracts, not
evidence that another nonCPU root hook should be guessed or silently accepted.
