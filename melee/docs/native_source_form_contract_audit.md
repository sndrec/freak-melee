# Paired source form contract audit

Read-only audit of Zelda/Sheik source, the direct interpreter and the actual
transform engine. No production, template, project or runtime state was changed;
no game or global build was run. The templates remain proposed/unregistered
source, and this document is not a paired-play acceptance claim.

## Subsequent implementation checkpoint

The following implementation was authorized after the read-only audit below.
The substantive shared operation now lives in `ftformtransfer.c`; `ftcommon.c`
does not forward to it. Stock callers retain their finish argument. Its source
branch captures exact selected packages, banks, program/layout/parameter
identities, imported data and live Fighter/Item views. It requires the actual
alternate-family entitlement, an idle destination actor and its prepared
`FormActivated` event before changing roles. Finish dispatch calls the
destination's typed `nativeScriptFighter_Run` directly. All ordered common
field/status/item transfer operations and continuation barriers are retained.
No source Definition or native authored finish callback selects this branch.

The parent separately implemented alternate-only runtime publication and
`FormActivated` preparation, verifying actual two-member publication with 69
checks and actual actor preflight/dispatch with 254 checks. Ordinary simultaneous
companion execution remains a separate gate. The transfer comparison fixture
passed 682021 checks across 1024 healthy comparisons, 625 lifetime cases, 23
preconditions and 45 item cases; source controller/activation and unrelated
engine leaves are explicitly controlled. Its final receipt is
`A:/FreakMelee-test-archives/source-form-transfer-final/receipt.json`.

These are infrastructure checks. They do not establish Zelda/Sheik whole-template
closure or real paired gameplay. Mutable accessory helper/snapshot support,
transform presentation and complete member/receiver behavior remain acceptance
gates unless separately verified. The sections below preserve the original
audit contract and distinguish those requirements from this checkpoint.

## Authored flow and motion data

`examples/zelda_full.fms` and `examples/sheik_full.fms` already author the
transform start, velocity reduction, start/finish effects, completion and
ground/air collision decisions. They do not ask the source to copy script state
into the destination. `transform_transfer` clears its accessory helper, calls
`forms.activate_partner()` and immediately returns; the destination executes
its own zero-argument Void `form_activated` policy.

| Member | Ground start / finish | Air start / finish | Destination finish frame |
| --- | --- | --- | --- |
| Zelda | 355 / 356 | 357 / 358 | Its own imported extended Float at 128 |
| Sheik | 361 / 362 | 363 / 364 | Its own imported extended Float at 112 |

These lie in the declared owned layouts: Zelda starts at 341 with 18 rows,
Sheik at 341 with 24. Finish enters with flags zero and rate one, then installs
the finish-effect accessory helper. It deliberately does **not** call
`animation.start()` after finish entry. Start/finish ground-air switching retains
frame and flags `0x0c4c508e`, then reinstalls the appropriate accessory helper.

The actual stock callers confirm the counterpart policies:
`ftZd_SpecialLw_8013AEAC` calls `ftCommon_ActivatePartner` with
`ftSk_SpecialLw_80114758`; Sheik's `fn_8011412C` supplies Zelda's
`ftZd_SpecialLw_8013B4D8`. Those two finishes select the destination's own
ground/air state and imported finish frame. They must become destination code-ID
dispatch for source members, rather than callback pointers or calls into these
fighter-specific finish bodies.

## Current hard gates

- `native_fighter_runtime.c:prepare_family` explicitly accepts only Single with
  one member. FMSC and `native_fighter_family_project.c` now support authoring all
  three family modes and independently prepare each member. The parent verified
  that authoring boundary with private real assets/source (106 checks); member
  source Check opens only the chosen member. This does not make ordinary runtime
  activation support pairs. `bind_family` already contains the actual Player
  registration path.
- The catalog has no `forms.activate_partner`. The `form_activated` descriptor
  exists in `native_script_events.c`, but `native_script_fighter_program.c` has
  no hook mapping for it and rejects that hook during program preparation.
- `callbacks.accessory(helper/None)` is not a current binding. The actor's eight
  instance callback slots exclude accessory. Motion accessory IDs currently
  come from immutable motion patches; only clearing is exposed.
- `native_script_fighter_actor.c:validRecord` accepts motion IDs only when
  installed in a prepared motion patch. A mutable accessory helper cannot merely
  be written into that array: execution, helper ABI validation and snapshot
  validation must be extended coherently.
- `ftCommon_ActivatePartner` still snapshots `NativeFighterDefinition` and picks
  `destination.definition->on_form_activated`. With both ordinary source
  definitions null, it chooses the stock branch; passing null `stock_finish`
  then fails before transfer. Giving it a native authored thunk would hide this
  missing source lifecycle.

## What the existing common transfer actually does

`ftCommon_ActivatePartner` is a substantive shared data/lifecycle operation.
It checks active/dormant Player roles, same player, distinct actors and role
bits, captures both fighter identities and worn/held items, and uses a bounded
per-player reentry guard. Its ordered body then:

1. Swaps Player's role permutation and the two fighters' role bits.
2. Calls `Fighter_UnkInitReset_80067C98` on the destination, advancing its native
   spawn generation, then refreshes the captured destination generation.
3. Transfers common position, facing, damage/HP, velocity, ground/air, input
   timers, jump counts, shield/status and related combat fields. It applies the
   actual star/metal/other status transfer primitives in their current order.
4. Reattaches the two worn-item bindings and held item using the destination's
   own model attachment data. It reloads destination special attributes and
   updates camera/collision/common movement data.
5. Performs the shared notification, sleeps the source through
   `ftCo_800BFD04`, and invokes the destination finish.

`TRANSFER_ALIVE` barriers occur throughout the body, but their fighter identity
predicate currently compares Definition pointers and native generation/data.
Source execution needs exact selected package/bank/storage status at these
barriers, including logical retirement/fault and item lifecycle status where a
source item is transferred. Comparing two null Definition pointers is not a
source program ownership check. Mixed source/stock partners and a destination
without a prepared activation event should reject before role mutation.

This operation has lifecycle-capable steps after mutation begins. A late
retirement/fault cannot be described as a wholly atomic successful transfer or
silently retried as stock behavior. Preserve ordered continuation barriers and
return a status that distinguishes unsupported/error, retirement and completed
activation. A source primitive must not turn a framework Bool into an authored
success value or mask failure after partial native work.

## Smallest coherent direct-runtime slice

**Family publication.** Prepare two independent Single-member
`NativeSourcePackage` owners at stable addresses, with separate banks, facts,
parameters, rigs, motion tables and entity resources. The family owns the two
packages and immutable member identities. There is no need to merge two Fighter
declarations into one program or fabricate a Definition. Preflight both members
and all leases before publishing either, then register the actual alternate
mapping through `Player_RegisterNativeCharacter`. Only lift the alternate-form
gate for this slice; simultaneous companion policy is a different contract.

`Player_80031AD0` already constructs both mapped fighters, marks the second as
transformable/dormant and can choose the initial member through the frozen
preview mapping. `Player_GetEntityAtIndex` follows the mutable role permutation;
member identity must instead follow the family member's immutable kind, package
and data. Extend `nativeFighterRuntime_MemberId` accordingly if exposed; its
current Single-only implementation is not a pair lookup.

**Direct activation.** Add the zero-argument Void `form_activated` hook to the
Fighter program/actor contract and a generic `forms.activate_partner()` native
operation. Resolve the partner from the validated selected alternate family and
actual Player roles, not a donor kind or string. Execute the common transfer
body with a source-aware ownership branch and call
`nativeScriptFighter_Run(destination, FormActivated, ..., result)` directly after
the destination reset/rebind and common transfer. Keep actual unscripted stock
callers on their real stock finish contract; change their call sites directly if
the function's typed return/signature changes. Do not add a source wrapper that
passes a callback thunk into the old finish-pointer branch.

The activation hook should be required for both prepared alternate members.
Destination/source faults or retirement stop continuation. Ordinary dormancy is
not destruction: successful activation keeps both banks, actors and script
storage alive. The source template's explicit `return` prevents further authored
source writes after role handoff; do not globally classify dormancy as retirement.

**Accessory code IDs.** Add a generic mutable accessory-phase binding for a
same-bank, root-owned, zero-argument Void helper, with `None` clearing it. Use the
actual accessory scheduler in `Fighter_8006C80C`; it already invokes interpreted
motion accessory IDs directly. Clear/reselect the mutable ID at the same motion
reset points as native accessory pointers. Extend snapshot validation to accept
validated helper IDs as well as immutable phase IDs, and preserve explicit
cue-vs-every-frame policy. Never accept a foreign-bank function ID merely because
its integer matches a local ID. Native shared-engine assignments can supersede
the source binding using the existing pointer-conflict rule.

**Persistent storage and generation.** `nativeScriptFighter_NewLife` already
updates actor/storage identity and generation and rebinds execution without
clearing the form's state bytes. Use that destination-generation transition;
do not detach/re-attach, rerun `load`, or copy the source's script state buffer.
Audit transient callback resets against the common reset/motion contracts.
Also reject a destination that is already executing an incompatible suspended
frame before resetting its generation; a per-player transfer flag alone does
not prove that an actor's existing invocation can be rebound safely.

Dormant actor records remain attached and retain exact bank references.
`Fighter_procUpdate` already skips its tick before running source code when
`x221F_b3` is set. `ftCo_800BFD04` changes visibility/camera/dormancy while leaving
the allocation alive. Pair retirement must therefore drain both live or dormant
actors, running frames, external leases and retained rollback history before
releasing either package.

## Receiver and snapshot boundaries

The current receiver implementation supplies native GObj pool slot/generation,
full scene lifetime, native generation and exact program/storage/schema checks.
It can keep the unchanged source view valid while it becomes dormant.
The destination's pre-activation receiver tokens must fail after its native
generation reset. Destination `form_activated` uses its own invocation context,
event frame, parameters and code bank; it must not reuse the source context.

`native_script_receiver.c:owned` deliberately requires the caller's expected
fighter kind and exact root program for authored fields/helpers. Zelda and Sheik
are different banks, so general cross-form authored access is not currently
allowed. The two templates do not need it for transfer. The minimum slice should
not weaken this check: let the engine operation select the destination's own
activation event. A future partner receiver/schema contract would be separate.
Related Item helpers must also retain their exact author/creator semantics when
the damage/held owner changes; a role swap must not make Sheik schema calls legal
on Zelda's bank by name alone.

Both actor snapshots already save state bytes, generation, callback/motion IDs
and bank/layout/parameter identity. Combined rollback acceptance must restore the
Player role permutation, both Fighter records and both source views together.
Nested source-to-destination execution shares fixed VM/receiver/payload pools;
family preparation must verify aggregate nesting/register capacity rather than
assuming each bank's independent maximum proves the nested call fits.

## Remaining source/asset prerequisites

The checked-in templates are not a claim that every current catalog spelling
already compiles. For the transform subset, `state.change(..., rate=1)` can be
authored with the direct flags/frame entry contract; it must not restart finish
animation. Ground velocity access, mutable accessory binding and the world-space
wind operation still need direct typed coverage or explicit source rewriting.
Do not introduce an alias/adapter to the historical generated backend.

Effect joint semantics need an explicit choice. Zelda's start joint 104 is a
raw instance joint index; the current generic `effects.spawn` resolves a semantic
Fighter part and rejects out-of-schema semantic indices. Provide a bounded
explicit raw-joint effect operation, or resolve an owned named joint selector at
load. Do not feed raw 104 into the semantic-part path. `bones.world_position`
already has a checked raw-joint operation. Verify the actual owned rigs and
effect resources for both members before claiming start/finish presentation.

Full member closure also includes Zelda's Din articles and Sheik's Needle/Chain/
Vanish entities and their lifecycle helpers. Source form transfer infrastructure
does not prove those unrelated moves, item-owner relations or all template
primitives are complete. Run whole-member compile/metadata checks and actual
owned asset preparation before trying paired gameplay.

## Required acceptance

Use two complete source banks and the actual shared transfer body: ground and
air Zelda→Sheik→Zelda; exact finish state/frame with no startup reset; persistent
distinct per-form variables; shared damage/jump/input/status transfer; held and
worn items; accessory helper binding/clearing and effects; both initial members;
stale receiver rejection after destination reset; callback error/physical or
deferred retirement/reentry; snapshots before, during quiescent handoff boundaries
and after reactivation. Failed family preparation/publication must preserve the
accepted family, and both members must block premature release while dormant or
referenced. Only then does a hidden real match establish paired execution.
