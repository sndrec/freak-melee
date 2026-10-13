# Ness PK Thunder: baseline 8 dependency audit

This is a source-only proposal. PK Fire and PSI Magnet remain the accepted
partial template; no Thunder override, clone registration or acceptance claim
is added here. This refresh reads accepted baseline 8, commit `4c76e0752`, and
its live compiler/Host bindings. Its 76 focused checks passed before this audit;
those generic checks do not constitute Thunder source or gameplay acceptance.
Only this proposal changes. No compiler, runtime, fixture, template or registry
edits, builds or games were performed for this refresh.

The smallest coherent next batch is the complete ball plus four trail article
definitions, with their constructors and cleanup events compared to unchanged
original C. Six generic contract families remain: handle signaling, original
creator input, collision-group reads, internal stage kind, camera release and
normalized cross-product Z. Four need new Host bindings; creator input and
collision group can use validated/direct property emission. The nine fighter
motions and complete Ness damage/death policy remain a later activation gate.

## Scope and owned data

`ftnessspecialhi.c` contains nine phases, motions 358–366: grounded Start, Hold,
End and Launch, followed by aerial Start, Hold, End, Launch and Rebound. All nine
IASA callbacks are empty. The ball is article slot 3; trail slots 4–7 create six
instances in this exact kind order:

```text
PKThunder1, PKThunder1, PKThunder1, PKThunder2, PKThunder3, PKThunder4
```

The ball owns six integer creation handles, sixteen XYZ position samples,
sixteen angles, speed, age, next trail index, a reflected flag, and a mutable
controller-enabled flag. This fits the existing 512-byte article state pool.
Trails own their integer sample index, blink counter and parent-enabled flag;
one existing link slot can retain each trail's ball parent. No history belongs
in a fighter union or a fighter-specific host operation. Unused stock pointers
and fields do not need authored storage.

Raw fighter imports (byte offsets) are int loop1=64, loop2=68 and gravity_delay=72
(stock u32 words explicitly assigned to signed frame counters, never float reads);
float fall_acceleration=80, launch_speed=84, launch_auxiliary=88,
launch_deceleration=92, knockdown_angle=96, wall_hug_angle=100,
freefall_blend=108 and landing_lag=112. The ball's five float imports are
lifetime=0, speed=4, spawn_angle_degrees=8, stick_threshold=12 and
turn_radius_degrees=16. Unused fighter offsets 60, 76 and 104 stay unimported.
Motion flags, effects, spawn joint, collision bounds and stage camera exception
must be named owned parameters. Raw joint ordinals must be checked against the
actual stock rig, as with the accepted PK Fire spawn joint; semantic bone
remapping cannot silently replace direct `fp->parts[index]` access.
The control-loop predicate is an authored comparison of `creator.motion` with
named ground/air Hold motion parameters, matching `ftNs_SpecialHi_CheckSpecialHiHold`.
Reflection and detachment update the ball's ordinary controller-enabled state;
neither belongs in a fighter-specific Host helper or BehaviorKind branch.

## Available constructors/references and missing handle dispatch

Already implemented in baseline 8:

```text
item.create_child(kind, x, y, z, facing, damage_owner, collision_group,
                  creation_value, backlink_slot) -> int
item.link_id(slot) -> int
articles.ref(handle, ExpectedItemKind).exists -> bool
articles.ref(handle, ExpectedItemKind).state.field[index] -> field type
```

Still proposed, not implemented:

```text
articles.signal(handle, "event_name", value) -> bool
```

`damage_owner` is an existing live fighter target ID; 0 means NULL. An invalid
nonzero ID fails construction. The returned item handle is the full int32
creation ID, with -1 for failure. `creation_value` is a float and carries only
the small trail index 0–5, never an item/fighter ID. The caller supplies zero Z
and the constructor supplies zero velocity. Current and previous positions
both equal the supplied vector, damage is zero, initial collision is enabled,
and `SpawnItem.x4_parent_gobj2` is the creating item. `SpawnItem.x40` uses the
explicit collision group. Assets and callbacks come from the parent's immutable
package author, regardless of current damage ownership.

The existing constructor installs the child's bounded backlink slot, creation
value, immutable author and validated creator bookkeeping before its `on created`
callback. The parent stores all six returned handles in an ordinary authored int array;
it does not consume six engine link slots. Do not add a reciprocal parent link
or register the child as a held item. Return the captured creation ID without
dereferencing an object after its callback.

Stock source: `itnesspkthundertrail.c:29`, `it_802AC43C`; ball spawn loop
`itnesspkthunderball.c:233`, `itNesspkthunderball_UnkMotion0_Anim`. The existing
`item.create_at_position` is insufficient: it always inherits the parent's
current owner, uses group 0, has no index input or backlink before the callback.
After reflection stock trail damage ownership must be NULL, even if the ball's
current owner is a reflector. The group's source is the ball's actual
`xAC4_ignoreItemID`, not a newly allocated group.

`ExpectedItemKind` resolves to a declared article in the same compiled
package. The compiler emits its constant kind and target field descriptor index;
the backend obtains offset, type, count and stride from that package's immutable
article state metadata. Reads are RO, array bounds remain checked, and
no runtime field-name lookup or allocation is needed. Lookup validates a live
creation ID, immutable author and kind. Missing/stale/mismatched references yield
false for exists and zero for field reads. The same reference syntax works
in fighter and article callbacks. A parent link supplies its int handle through
`item.link_id(0)`; scripts never pass or expose an engine pointer.

Handle signaling must validate the live ID and immutable package author, then
deliver the existing compiled event hash/value. It must work after reflection
and after damage ownership is cleared. Existing `articles.signal_owned` rejects
those required paths; `item.signal_link` remains useful for the one parent link
but cannot reach the ball's six individually stored trail handles. Handle signals
must not grant cross-package access. Signals carry ordinary float payloads, so
identity-sensitive reflection notification compares `creator.state.ball_handle`
against `item.creation_id` directly in the article callback rather than passing
the integer handle through a float argument.

## Remaining ball/trail additions

| Script operation | Exact engine/source semantics |
| --- | --- |
| `creator.input.x`, `creator.input.y` (RO) | Validated original creator's `input.lstick[0]` components; invalid creator reads 0. `ftLib_800865D8` in `ftlib.c:294` performs exactly these reads. The authored controller-enabled flag separately disables control after reflection. |
| `item.collision_group` (RO int) | `item->xAC4_ignoreItemID`; pass unchanged to each child constructor. |
| `stage.internal_kind` (RO int) | Direct `Stage_80225194()` result. The ball compares it with owned parameter 0x54. This is not the external stage selection ID. |
| `item.release_camera()` | Direct `Item_80267454(gobj)`; clears an allocated camera box and its actual flag when present. Ball constructor invokes it only on the stage exception, after disabling bounds removal. |
| `vectors.cross_normalized_z(ax,ay,az,bx,by,bz)` | Call actual `lbVector_CrossprodNormalized` with local Vec3 inputs/output and return output Z. Ball steering uses its sign, after `lbVector_Angle`; preserve order and zero-vector behavior. No steering helper or fighter name is needed. |

Handle signaling is the sixth remaining family. Preferred Host shape:
`bool signal_article_handle(HSD_GObj* caller, uint32_t handle, uint32_t event_hash,
float value)`, usable from fighter and article bodies. Resolve the actual kind
from the live recipient under the caller's immutable definition; no supplied
kind, persistent recipient pointer or writable reference is needed. Use the
existing bounded dispatch entry/leave contract and mark this operation as a
returning source-invalidating effect in the compiler.

The other new Host slots are the internal-stage query, item camera release and
normalized-cross query. `creator.input` should use the existing validated
`scripted_article_creator` result without imposing current-damage-owner equality;
`item.collision_group` emits `xAC4_ignoreItemID` directly with integer type.
`stage.internal_kind` appears in the current type classifier, but has no
expression emitter or Host binding. That row alone is not an available API.

No recursive visibility addition is needed. Existing
`item.joint("0").visible = true/false` resolves the root's first child and uses
channel 10, `HSD_JObjClearFlagsAll` / `SetFlagsAll` with `JOBJ_HIDDEN`
(`native_article_model.c`). This exactly covers constructor `it_80272940`
(showing the child subtree) and the trail's alternating subtree visibility.
`self_visible` uses the single-joint channel 15; existing `item.child_visible`
must retain its separate single-child behavior. Root scale/rotation use
`item.joint("").scale.z` and `.rotation.x`; no new trail model API is needed.

No new synchronous attached-effect API is needed: existing
`effects.spawn_on_part(effect, direct_joint)` calls `efSync_Spawn` with bounded
direct parts access. Existing article `vectors.angle` calls `lbVector_Angle`, and
`item.mirror_velocity()` calls `lbVector_Mirror` with actual shield normal
`xC58`. Existing `item.expire_tick()` binds `it_802751D8`; its decrement-then-test
body is identical to Thunder's `it_80273130`, so no new lifetime primitive is
needed.
`item.owner_bound`, `item.use_manual_lifetime`, `item.disable_hitboxes`,
`item.clear_current_owner`, joint transforms, animation-state flags, stage masks,
`physics.force_air_and_use_jumps`, normal ground/air physics, and rotation about
the Z axis for the existing article rotation operation cover those operations.
These corrections supersede
the preliminary message's proposed vector-angle/mirror and attached-effect gaps.
The ball's collision probe is existing `item.stage_collision(false)`, which
binds actual `it_8026DA08`; test the updated environment masks afterward in
authored code. `item.sweep_stage_flags()` binds `it_8026DAA8`, which adds
`it_80276308` / `it_802763E0` probes, and must not replace the stock call.
The interaction callback already has `item.interact(other)` bound to exact
`it_8026B894`, including all of that helper's reference clearing.

The later nine-phase fighter batch additionally needs the following parser
operations. These are separate from the ball/trail constructor batch:

| Script operation | Exact engine/source semantics |
| --- | --- |
| `floor.is_platform` (RO bool) | Test the cached `fighter->coll_data.floor.flags & LINE_FLAG_PLATFORM`, as grounded Launch entry does. No stage-line re-query or new Host slot is necessary. |
| `vectors.angle(ax,ay,az,bx,by,bz)` in fighter expressions | Expose the existing Host `vector_angle`/`lbVector_Angle`; currently only articles expose this six-input spelling. Existing fighter `vectors.angle_xy` calls `lbVector_AngleXY`, whose implementation differs from the actual stock function. |
| `physics.mirror_velocity(nx,ny,nz)` | Call the existing Host `vector_mirror` on `fighter->self_vel`, with the explicit collision normal. Rebound halves X/Y, clamps X and updates facing afterward in authored code. |
| `physics.rotate_velocity_about_axis(nx,ny,nz,angle)` | Direct actual `lbVector_RotateAboutUnitAxis` on `fighter->self_vel`; callers provide a unit axis. Wall-slide stock passes (0,0,1). Existing Host `vector_rotate` binds the distinct `lbVector_Rotate` implementation; do not assume bit-identical behavior from mathematically equivalent rotation shortcuts. |
| `state.freefall_blend(a,b,allow_interrupt,mobility,landing_lag,blend)` | Direct `ftCo_800969D8`, including animation blend and its `unk=false` common-core jump policy. Existing `state.freefall` maps `ftCo_80096900`, whose blend=0 and `unk=true` differ. |
| `state.down_bound()` | Direct `ftCo_80097D40`, including `mv.co.downspot.x4=0`; do not substitute generic landing or `ftCo_80097D88`. |

Existing `sqrt(x*x+y*y)` already expresses stock `lbVector_Len_xy`, which is
exactly that inline sqrtf formula, and `physics.ground_to_self()` maps actual
`ftCommon_SetSelfMovementFromGroundedMovement`. Rebound gravity/deceleration
uses existing `physics.fall` and `physics.air_decel` operations.

## Constructor and cleanup boundaries

Ball construction is the existing fighter article constructor with owner-origin
enabled, zero preconstructor velocity, a new collision group, and the requested
joint position as previous position. Original ball history initializes from the
requested position, not the owner-origin `spawn.pos`. The fighter can store that
requested XYZ in ordinary authored fields before construction and `on created`
can read those creator fields; no new metadata Vec3 is necessary. Preserve both
lifetime writes and both angle initialization loops around the initial state
change in original `it_802AB58C` / `it_802ABA4C`.

In cleanup `it_802AB90C`, disable ball hitboxes first, conditionally notify the
controller, clear the mutable controller, then visit all six trails in increasing
index order, detach each and clear the stored handle. Finally clear owner-bound.
Trail detach `it_802AC58C` disables hitboxes, clears its parent-enabled state,
clears current damage owner, and clears owner-bound; its animation callback
returns true on the next tick. It is not immediate destruction. Controller
detach `it_802AB9C0` clears ball/trail damage owners and owner-bound flags while
leaving controller/history and trail parent intact until the ball's next update.
Destroy callbacks may run after damage or collision callbacks already performed
cleanup; the authored cleanup must remain safe and preserve those repeated calls.

Reflection `it_802AC098` conditionally clears only the matching fighter handle,
clears mutable control, flips/normalizes angle, detaches all old trails, resets
age/index, fills history indices 1–15 from index 0, and recomputes velocity.
Steering wraps to [-tau,+tau]; reflection and shield bounce wrap to [0,+tau].
Do not replace either with a shared wrap helper using the other range. Trail
allocation failure still increments the next index, because stock does so.

The fighter callbacks installed on successful ball creation currently point to
`ftNs_Init_OnDamage`. That source sequence is yo-yo cleanup, PK Flash cleanup,
Thunder cleanup, then bat cleanup (`ftness.c:482`). A callback containing only
Thunder cleanup is an incomplete lifecycle override while the other three
behaviors remain untranslated. Keep activation gated until their source-ordered
authored cleanup is available or a separate bounded integration policy is
explicitly approved. Do not delegate this sequence to a stock Ness handler.
OnDeath's model-part reset and all four persistent handles likewise remain part
of the later full lifecycle port.

The compiler already permits **16** article declarations
(`SCRIPT_ARTICLE_DECLARATIONS` in `native_fighter_script_article_refs.h`). Fire's
two plus Thunder's five fit that bound. Full Ness needs eleven, matching the eleven
`it_8026B3F8` registrations at `ftness.c:469–479`, rather than eleven trail
instances. The accepted compiler capacity matches the current
`FT_CHARACTER_ITEM_CAPACITY`, exceeding Ness's eleven declarations, and staying
within the runtime's existing maximum of 64 article logics. Keep the limit
explicit and bounded; no engine link count increase is required for Thunder.

Do not eagerly enlarge the inline compiler program to 64 articles. Each current
`ArticleDeclaration` reserves 32 states with three callbacks, 22 logic callbacks
and 16 signals; each callback reserves 64 lines of 1024 bytes. That is roughly
8.8 MB of callback buffers per article, before other fields. The compiler
allocates this program at the build transition, not during gameplay, but a
64-entry pool would reserve roughly 0.56 GB just for those buffers. Record actual
`sizeof` values if later storage changes need a new compiler preflight. If a
future package needs more than 16,
coordinate both asset capacity and compiler storage organization; the runtime's
64 ceiling alone does not establish that a package can import 64 assets.

## Next coherent batch and dependency checklist

| Contract | Baseline 8 status | Next action |
| --- | --- | --- |
| Typed locals, helper argument types and exact 32-bit identity emission | Implemented in both fighter forms and article callbacks. | Reuse for handles, loop indices and constructor arguments. |
| Complete declaration catalog and capacity 16 | Implemented before callbacks/helper compilation; metadata travels by borrowed catalog. | No capacity/context refactor needed for Ness's eleven kinds. |
| Immutable-author typed RO references and integer link IDs | Implemented with target field descriptor indices and actual backend bounds. | Reuse for parent history/speed reads; never retain an object pointer. |
| Nine-argument child constructor | Implemented with explicit owner/group/value/backlink, installed before `created`. | Reuse for all six trail attempts, including NULL-owner reflection paths. |
| Recursive first-child visibility, root transforms, exact stage probe and interaction clearing | Implemented through the operations identified above. | Do not add duplicate Host operations. |
| Exact-site source lifetime and recipient exports | Implemented, including helper effects, constructors, signals and removed/recycled external destinations. | New handle signaling must join the existing effect table and dispatch-depth contract. Exports do not replace article signals. |
| Handle signals, creator input, collision group, internal stage kind, camera release, normalized-cross Z | Still missing as complete parser/backend contracts. | Add only these six families: four Host bindings and two direct property families. |
| Five article definitions, tuning, ordered cleanup and stock-C comparisons | Not authored/accepted. | First complete ball/trail batch, with no promoted fighter motion or partial damage-policy override. |
| Nine fighter phases and complete damage/death policy | Still gated. | Separate source audit/implementation after article acceptance; compare known vector/platform/freefall/down-bound gaps and all remaining motion behavior. |

The next batch implements the six missing families and the five complete article
definitions together. Preserve all six creation attempts in original kind order;
the three `PKThunder1` instances share a declaration but own separate state.
Child callbacks can already read parent state immediately through the accepted
catalog/ref/constructor contract. No new state-pointer API, engine link capacity,
generic export mechanism or compiler capacity is needed.

Source acceptance is separate from template activation. Keep the candidate in
owned scratch/dedicated fixture files until the complete ordered Ness damage
policy is available or root explicitly approves a bounded integration policy.
Article import/registration for a fixture is root-owned and must remain
unadvertised; it does not establish a complete playable Thunder move.

Root owns new Host bindings, engine operations, layouts and CMake integration.
Compiler regions need an explicit owner before implementation. The existing
typed-local/reference/lifetime units are already separate real implementations;
extend their actual call sites instead of rebuilding them or adding forwarding
wrappers to the large article parser. This document assigns no shared-file ownership.

### Integer locals and identity

Both fighter and article callbacks now support bounded typed locals:

```text
let int parent = item.link_id(0)
let int damage_owner = item.current_owner
let float radius = articles.ref(parent, It_Kind_Ness_PKThunder).state.speed
```

Both contexts use the existing sixteen-local/block-scope limit.
An integer handle must remain `int32_t` through declaration, reassignment,
argument evaluation and return. Do not recover an ID by casting a float. A
direct integer state/reference expression is also valid. Untyped `let name`
retains float semantics. Integer declarations, assignments, arguments and
indices reject implicit float inputs; explicit `int()`/`uint()` remain deliberate
conversions, never recovery of an already rounded identity.

Item handle -1 denotes failure. Compare the remaining 32-bit creation-ID bits
against live `Item.x1C`, including a high-bit ID represented by a negative
`int32_t`; do not copy the older owned-article resolver's `number < 0` test.
Cover adjacent IDs above 2^24, zero, a high-bit ID, and the reserved -1 sentinel.
Fighter target IDs keep their separate existing zero-for-NULL convention.

### Binding state references across article kinds

The accepted first pass collects each declared article's kind and complete
`ScriptStateTable`. It resolves `ExpectedItemKind`, field name and optional index
during callback compilation. A kind must be a literal declared in this same
program, the field
must exist on that target kind, and only array fields accept an index. All
reference fields are RO; assignment and use as writable query outputs fail.
Forward declarations must work identically to backward references.

The emitted recipe is immutable: expected kind, typed getter, target field
descriptor index and integer element index. For example, a float history read
emits the ball's field ordinal, not a trail-local offset. The backend uses that
descriptor's offset/count/stride. Integer/bool getters return their
actual types, and float reads preserve the copied float bits. A getter evaluates
the handle and index once and immediately returns a scalar copy. A reference is
an expression recipe, never a script variable containing an object or buffer
pointer. No runtime field-name search, allocation or persistent pointer is needed.

The backend derives the requesting author from the calling fighter's registered
kind or the calling item's immutable `native_article_author_kind`; authored
code cannot supply a different author. Resolve the live creation ID, check
expected item kind and exact immutable author, then check its active authored
logic against `xB8_itemLogicTable`. Validate index/count/stride and the final
byte range against the target's actual `local_state_bytes` and the 512-byte
pool before copying. The compiler's recipe is bound to that package's compiled
article declaration; do not reuse an offset from another kind or another DLL.
Missing, mismatched and out-of-range reads return typed zero; `.exists` returns
false. Invalid field/type/shape is a compile error, not a runtime zero fallback.

Both compilation paths and typed helper compilation already receive the
borrowed immutable catalog. It is build-time data, freed with the compiler
program; no per-fighter/per-item pointer or allocation was added. Emitted Host
requirements already cover the getters and constructors used. Extend that
scanner for the new signal/query operations and retain absent-host rejection.
See [article references](scripting/article_references.md) and the typed-local
section of [language](scripting/language.md) for the accepted syntax.

### Dispatch and constructor lifetime

Handle signals select an event from the target article's own compiled signal
table, after the same immutable-author/live-ID/logic checks. Damage ownership
does not authorize or deny dispatch. False means no eligible recipient/event;
true means the event was dispatched, even if the recipient removed itself.
Capture the necessary IDs and callback before invocation, then do not inspect
the recipient afterward. Float payloads remain ordinary event values, never IDs.

Baseline 8's accepted recipient exports have a different scope:
`export func name():` publishes a fighter's own zero-argument helper, and
`targets.call(fighter_id, "name")` asks that live recipient to run its own
export. Its bounded 64-entry hash/callback table provides explicit opt-in even
for foreign packages. It neither grants foreign state reads nor replaces
same-author article references. Thunder's creator calls stay within its own
module through existing `creator.call`, so Thunder needs no new export mechanism.

An exported function or signal can remove its recipient; neither dispatcher
may retain its Definition/function/object after the call. If a callee can also
remove the caller, generated code must exit the calling callback or validate
its original live creation identity before any further object access. Baseline 8
already implements this independently of a successful boolean result. A new
handle-signal Host must be included in that compiler effect metadata; do not
add a second lifetime scheme. Exact-site lowering snapshots full fighter spawn
or item creation identity and uses Clang statement expressions, preserving
short circuit and guarding result stores before further accesses. Effects
propagate through the bounded helper DAG; external creator/owner stores also
re-resolve eligible destinations and reject removed/recycled generations. See
[synchronous callbacks](scripting/synchronous_callbacks.md).

Thunder's detach events only clear fields/hitboxes and leave destruction to the
next animation tick, preserving its original ordered loop over six children.

The child constructor derives immutable author and original creator bookkeeping
from the validated parent, independently of the explicitly chosen damage owner.
Install backlink, creation value, facing/origin bookkeeping and zeroed local
storage before `on created`; debug-owner notification remains authored and
conditional on the chosen damage owner. Capture parent and child creation IDs.
After callback dispatch, return the captured child ID without dereferencing
either object: creation can signal another object whose callback removes them.
Do not infer success from a retained pointer or promise that the returned handle
is still live. This constructor contract and its returning caller guard are
already implemented; the candidate can now use integer locals without a new
constructor or local-type batch.

### Source comparison preflight

Reuse accepted generic fixtures instead of recreating their implementations:
`script_typed_locals_*`, `script_article_refs_*`, `article_refs_test`,
`article_operations_script_test`, `article_creation_test`,
`script_callback_lifetime_*`, `callback_lifetime_test` and `fighter_event_*`.
They cover actual compiled callback contexts, full-width IDs, target field
ordinals/types, reference bounds, constructor metadata and synchronous lifetime.
The next coordinated build should keep these as regressions against one matching
SDK; this read-only audit did not rerun them.

New boundary coverage should focus on the six missing contracts: unchanged
normalized-cross calls including zero vectors; creator input after reflection
and creator reuse; exact collision-group propagation; stage/camera behavior; and
handle signal dispatch with six independently retained trails. Handle dispatch
needs live/stale/high-bit/sentinel IDs, wrong author/active logic, missing event,
NULL or transferred damage owner, recipient removal, caller removal/address reuse
and dispatch-depth exhaustion. The generic operation must guard the caller
before storing a whole bool RHS. Pure queries must not acquire artificial RNG
or callback effects. These are boundary checks, not Thunder source acceptance.

Then mechanically extract or compile unchanged original ball/trail functions,
renaming only fixture symbols, beside the generated candidate. Instrument engine
boundaries as ordered events and use the actual shared vector functions, rather
than equivalent fixture formulas. Compare all six creation attempts, both angle
initialization passes, both lifetime writes, sixteen XYZ/angle samples and
detachment ordering on every callback. Extend the existing Fire regression and
Magnet source fixtures against that same SDK before promotion. This preparatory
audit ran no global build or game.

## Comparison plan and remaining gates

Compile the unchanged original ball/trail source files alongside the generated
candidate, with engine boundaries instrumented in the same style as the accepted
Fire fixture. Cover constructor ordering and spawn fields with both facings,
NULL/current/reflected damage owners, all six trail indices and allocation
failure. Check pre-callback creation value/backlink visibility and full-width
creation IDs. Compare all sixteen history samples, angle histories, six handles,
timers, state, flags, model child/grandchild visibility and ordered callbacks.
Exercise live/stale/wrong-author/wrong-kind references, parent destruction,
controller loss, self-reflection, reflector owner change, trail recreation after
reflection, and repeated cleanup. Steering tests must include zero/parallel/
antiparallel sticks, both sides of the threshold and 45-degree branch, and both
angle wrap endpoints. Shield bounce compares actual `lbVector_Mirror`.

Only after source-C constructor/article acceptance should the nine fighter
phases be activated and compared. Ground Launch, slope following, launch floor
knockdown, ceiling/left/right rebound, shallow wall sliding, freefall blend,
ledge grab, damage cleanup and stage-specific camera behavior need dedicated
fixtures; ordinary controller replays do not prove those branches. Every future
game launch remains hidden, muted, accelerated, with drawing enabled and an
explicitly coordinated runtime slot. No game or global build was run for this
proposal, and the accepted Fire/Magnet files are unchanged by it.

## Baseline 8 source-preparation ledger

Scratch preparation is complete under `.local/ness-thunder-baseline8-prep`.
It is **uncompiled**, unregistered and has no source/gameplay acceptance claim.
No shared compiler/API/runtime, accepted template, fighter definition or tests
were edited; no build or game was run.

The 546-line `ness_thunder_articles.uncompiled.fms` contains the complete ball
and four trail declarations, source-ordered constructor tails, state callbacks,
reflection/shield/combat/interaction/destruction behavior and both detach
variants. The three near trails share one kind declaration but retain separate
instance state. Two bounded fighter helpers prepare creation history and perform
the article's original creator cleanup; they install no nine-motion override or
incomplete Ness damage/death policy. Constructor command clears preserve 3,2,1,0
order; previous Z is flattened while requested history Z remains intact.

`callback_field_order.md` maps stock fields/NULL slots, ordered engine calls and
the unchanged-C fixture plan. `callback_map.json` records 23 original source
entry points with verified live source locations. `static_inventory.json` is
only a text inventory, not a compiler check. `source_manifest.json` pins the
baseline, draft/artifact hashes and fifteen read source files. The draft hash is
recorded there rather than copied into multiple mutable documents.

The draft still requires handle signaling, validated creator input, direct
integer collision-group reads, internal stage kind and camera release. Root
retained `item.collision_group`: reading the actual field is clearer and more
reusable than adding fighter constructor-group scratch merely to avoid a tiny
getter. Existing recursive joint visibility, stage_collision(false), references,
constructors, integer locals and source lifetime cover their respective paths.

Normalized cross is the one tentative API reduction: all three original
`C_VECCrossProduct` component formulas are stored into ordinary float fields,
then existing `vectors.normalize` invokes actual `lbVector_Normalize`. This
preserves the zero-length path without using a determinant-sign shortcut.
Generated-C/source comparison must establish numeric behavior before dropping
the proposed cross query from an implementation batch; no bit-parity claim is
made. Native stock PI/tau double-literal promotion versus float parameters is
also a recorded comparison risk. Do not manufacture a match with correction
constants, RNG calls or delegated stock handlers.

Next work remains a coordinated generic API batch and unchanged-C article
comparison. Complete source-ordered Ness lifecycle cleanup continues to gate
fighter activation. This checkpoint records preparation only; it performed no
compiler build, game, template promotion or nine-phase fighter implementation.
