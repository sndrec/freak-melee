# Direct interpreted Item instance ownership

`src/melee/it/native_script_item_actor.c/.h` owns actual interpreted Item instance
storage/execution, while `native_script_item_snapshot.c/.h` owns its private packed
records. This does not route authored behavior through NativeFighterHost, generated
C callbacks or an ItemLogicTable trampoline. Native physical Item/GObj/model/collision/
effect/allocator owners remain separate and still need the broader snapshot.

Root owns immutable entity preparation/publication and assets; target_query owns
actual Item scheduler sites. The core owner fixture described below does not by
itself prove every constructor, physics branch, stage callback or package factory.

## Identity and memory

The immutable selector is `nativeScriptItem_Program(author_kind,owner_node)`.
Source owner is the actual Article/Item node in the pinned compile bank. Multiple
entities can share the same physical ItemKind. Workshop Item declarations use
their explicit recipe index and the universal four-state holdable protocol; those
physical/data fields are validated separately from source owner identity.

Native Item selection must set `native_article_author_kind` and
`native_script_owner` before Attach. Holdables additionally set
`native_holdable.owner_kind` and `definition_index`. Attach verifies these fields,
physical kind/entity, actual Item.x1C and the nativeGObjPool reference. Generation0
is legitimate: Item_Init resets the source counter to0 and the first constructor
uses it. The nonzero GObj slot generation and scene lifetime guard physical reuse.

The program spec pins bank/program identity, entity owner, dense state animation
IDs and phase function IDs, static lifecycle hooks, signal IDs, resident recipe
data and Article assets. NativeItemDefinition recipe callback members must all be
NULL. Source nodes and script values contain no native callback address. Program
publication must reject replacement/reuse while either fighters or Items reference
the same bank. Item HasActors/ReferencesProgram includes active owners and frames
still unwinding after detach. Epoch/program identity checks invalidate stale
snapshots after a content transition; no compatibility format is introduced.

512 stable actor reservations each provide8192 bytes of maximum owned typed state.
The current actor reservation is4386816 bytes; a separate8192-byte fixed GObj-slot
index maps physical objects to actor slots. Both, plus scene epoch and all mutable
metadata, belong to non-host `.mstate`. These are provisional bounded capacities,
not a workload census conclusion. The fixture reaches512 active instances and
refuses the513th without a heap fallback. Attach resets its chosen reservation;
no runtime allocation occurs in lookup, callback execution, snapshot or detach.

Lookup checks actual pool slot/generation/lifetime before using the actor index.
It does not scan512 interleaved8KiB state buffers per hook. Attachment reserves
the first inactive non-running actor slot. Detach clears the index and retires
storage, but a suspended execution retains its slot until running depth unwinds;
new instances cannot overwrite an older invocation's storage/execution view.

## Source execution contract

Attach creates the entity owner's typed storage/immutable parameters and binds
execution to common `nativeScriptRuntime_VM()`/`Payloads()` scratch. Attach executes
no script. Spawned and Created retain distinct actual native call points; Created
must not move ahead of the post-construction appearance/payload setup.

RunHook/RunPhase/RunSignal return NativeScriptVMStatus. Ok carries the declared
Bool/Void/I32 value. Error and SourceRetired leave caller output untouched and
require the native caller to stop before reading cached userdata. Missing static
hooks/phases have a typed neutral null-slot result, never a stock fallback.
Faulted actors remain attached and pinned, and subsequent dispatch reports Error.
Static ItemLogic Bool callbacks retain their native removal meaning; true still
requests removal even if the callback also changed state.

Common VM scratch is transient host state shared with Fighters. Nested calls use
balanced frames/registers/payloads; snapshots and scene transitions require idle
scratch and no active native proc/draw/deferred work. Event arguments use the
actual descriptor's typed contract. Item interaction/victim payloads are opaque
Object values captured by the native receiver layer; actor dispatch never casts
a pointer into an ID. Signals pass F32, held_request uses I32/F32/F32, and helpers
bound to transient callbacks must match result/parameter/owner ABI exactly.

Transient Accessory, EnteredHitlag, ExitedHitlag, Touched, JumpedOn, GrabDealt and
GrabbedForVictim declarations are lookup definitions. Their instance code IDs
start UINT32_MAX and require explicit BindCallback. A legitimate shared native
assignment supersedes the source binding; HookBound clears that code ID when its
actual native pointer is present. BindCallback validates owned helper/event code
and clears the corresponding native pointer. ResetStateCallbacks clears exactly
the five native state-reset slots (Accessory/EnteredHitlag/ExitedHitlag/Touched/
JumpedOn), preserving the two grab bindings. Native state setup performs this
reset at its actual final point after generation/revision validation.

The current event catalog initially lacked several transient Article descriptors;
publication may not substitute Fighter descriptors or silently accept an unmapped
callback. Root adds audited descriptors/bindings and target_query migrates the
associated source calls. Core helper binding can use a correctly typed entity
helper without requiring an invented callback ABI.

StateAnimation validates a dense state index before any native reinitializer
mutation and returns the actual immutable animation ID. SelectState selects IDs,
state and an increasing revision; it does not prematurely clear native pointers.
The scheduler clears/assigns phases at the original source point. Revision checks
prevent an outer state setup from overwriting a nested state transition. State
change does not retire the VM source: authored code continues, and a false phase
continues its normal native tail. Initial unselected state is UINT32_MAX and must
match native msid=-1. Selected state must match exact native msid in packed state.
BeginTick advances the diagnostic/event frame once at the actual native Item update
boundary, independent of which authored hooks exist.

## Destruction and scene lifetime

Item_8026A8EC sets native_removal_in_progress before Destroyed and blocks recursive
native removal. The actor must remain writable during the complete Destroyed
callback. RunHook(Destroyed) retires its storage view afterward, including absent,
error and retirement paths, while retaining active attachment and the program pin
until actual Item_OnUserDataRemove calls Detach. The guarded Spec query remains
available through that physical cleanup tail.

This is necessary because HSD_GObjFree can defer physical removal while its proc
is executing. Retiring the owned view after Destroyed stops the parent VM before
later storage/native writes, even though the Item still physically exists until
the scheduler flushes deletion. Native destruction tail uses physical generation
guards and continues normal native cleanup; it must not confuse owned-view
retirement with permission to skip native destruction. Error paths never revive
the view or install stock callbacks.

CanBeginScene/BeginScene share the actual GObj scene epoch. Retirement occurs at
the coordinated boundary before preload reclaims pointed assets, with idle VM
and native callbacks. It retires all views/index entries without rerunning authored
Destroyed or resource destructors. Root's GObj owner integrates these calls; Item
core does not independently reset epochs on constructor/state change.

## Packed state

The private header records epoch/count/total bytes. Each occupied record captures
actor slot, native GObj generation reference, actual Item generation, author/source
owner/physical kind/recipe, program/layout/parameter and asset identities, selected
state/revision, phase and mutable transient callback IDs, tick/completed callback
counters, fault status and only the owner's declared state footprint. It omits
unused8192-byte reservation tails and shared VM scratch.

Validate checks complete record lengths/order, duplicate object references, program/
asset identities, function ownership/ABI, state/phase selection, Bool canonical
bytes and epoch before any mutation. Load preflights every restored native peer
and execution view before retiring existing views or copying state. Failed load
leaves the full actor reservation, index and epoch unchanged. The index rebuilds
from validated records; constructors/defaults, callbacks and destructors do not
run during load.

Aggregate restore must first restore GObj scheduler tables, occupancy/generations
and native Item payloads. This component then reconstructs its typed execution
views against still-pinned immutable programs. It does not recreate Item/model/
collision/effect owners or copy assets, and does not claim complete match rollback.

## Focused evidence

`A:/FreakMelee-test-archives/script-item-actor-motion-naming-refresh/receipt.json`
passes38 cases/1219 checks. The fixture compiles actual source parser/compiler, owned storage,
event/payload/execution/shared VM, actor/snapshot owner and real GObj constructor,
userdata, proc scheduling and deferred free sources. It verifies two source owners
sharing one kind, two recipes sharing NativeHoldable, generation0, state isolation,
typed payloads/Bool returns, transient registration/native takeover/reset, ordinary
state continuation, exact packed replay, peer resurrection/reuse, malformed state
and second-peer atomic rejection, retained runtime faults, missing/present Destroyed
retirement,512 capacity, empty snapshots and .mstate owner/host scratch separation.

The fixture's512 active records pack137232 bytes with its12-byte Article state
footprint. Empty packed state is16 bytes. These are measured component/schema
sizes, not all-template state usage, total snapshot bytes or timing results.
Records include immutable creator, revocable attachment and the actor-owned
creation scalar. The finite scalar defaults to zero at Attach, is supplied after
Spawned before Created, and rewinds with the record. Get/Set reject retired or
faulted actors; malformed nonfinite snapshot values reject transactionally.
Immutable specs/assets, native Item payload provider/removal callback and unrelated
engine leaves are controlled test boundaries. GObj lifetime/scheduler and actual
interpreter run; full native Item asset creation, physics/draw/stage/package factory
acceptance remains root/target_query's separate work. No game/global build/commit
was performed by this agent.
