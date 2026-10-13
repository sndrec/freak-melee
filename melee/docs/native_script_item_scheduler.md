# Direct interpreted Item scheduler

The actual Item scheduler now selects authored code IDs from the attached
`NativeScriptItemProgram`. It does not install generated callbacks into an
`ItemLogicTable` or `ItemStateTable`. An attached actor remains attached when a
hook is missing or execution faults, so these paths never select stock authored
behavior as a fallback. Unscripted Items retain their actual stock callbacks.

The scheduler changes are in `item.c`, interaction/out-of-bounds dispatch in
`it_26B1.c`, and the entered-air collision gate in `itgroundcoll.c`. The separate
[instance ownership contract](native_script_item_actor_ownership.md) describes
program pinning, typed storage, transient bindings and packed snapshots.

## Construction and state selection

`Item_CreateAuthored(spawn, author, source_owner)` carries the exact source owner
node through the real constructor and selector. The owner is an explicit argument;
ordinary stock `SpawnItem` initializers acquire no new uninitialized field. The
selector prepares native data, validates the immutable spec and attaches storage
before the first authored callback. Failed selection/attachment releases the
partial GObj and returns NULL. Dynamic-bone ownership is initialized before that
possible failure so actual userdata cleanup is safe.

Spawned runs at the existing `Item_8026A810` point after common native creation,
model/proc registration and initial setup. Failure or source retirement returns
NULL rather than exposing a half-created object. Created remains a distinct
post-construction factory event: its existing appearance/attachment/creation-value
setup must complete before dispatch. This scheduler slice does not move Created
to Spawned or migrate the external article factory call sites.

Attach leaves state unselected until an actual `Item_80268E5C` call. State setup
obtains its exact numeric animation and phase IDs from prepared source metadata.
It checks source lifetime and selection revision after reentrant native stages,
then clears native phase pointers and the original five transient reset slots at
the original final assignment point. A nested state selection wins over an older
setup. An ordinary state change does not retire source execution: a false phase
continues the common native tail, and a true phase still requests removal.

`Item_802693E4` advances the actor event frame once at the existing update entry.
Animation/physics/collision preserve the original hitlag gates and native tails.
Attached articles execute source phases; source Workshop holdables use the shared
`nativeHoldable_*` protocol for native held/flying mechanics plus direct Workshop
hooks. The shared protocol is not an authored callback table.

## Dispatch and removal

Pickup, both drop paths, throw, entered-air, damage dealt/received, clank, reflect,
shield bounce/hit, absorb, interaction and out-of-bounds use typed direct events.
Accessory/hitlag/touched/jumped/grab callbacks are transient bound code IDs. A
legitimate per-instance native primitive can supersede one of these transient
slots; static source logic and phases remain exclusive to the source actor.

VM Error/SourceRetired stops the native caller before cached userdata reads. It
does not synthesize a removal Bool. Successful Bool true retains the original
removal type and actual destructor path. Deferred native removal is also a stop
condition outside the destructor, even while userdata remains physically live.

Destroyed executes while owned state is writable. Its dispatch retires that state
on return, including an absent or failed hook, so a suspended parent VM stops
before its next authored statement. The common destructor tail uses physical
generation guards and still completes. Final userdata removal detaches the actor
immediately before freeing the Item allocation; it runs no further authored code.

Interaction snapshots entry membership as GObj slot/generation plus Item creation
generation. It checks the Fighter target before and after each event, skips
removed/recycled entries and excludes newly created Items from that invocation.
It retains the original pre-callback owner for cleanup. Grab similarly revalidates
its target between events. External Object payloads use the real receiver capture
API and the full trusted source scene epoch, never pointer-to-ID casts.

Interaction scratch is fixed at 64 nested frames with 4096 references each
(3 MiB plus depth). It stays in ordinary native `.data`, outside rollback
`.mstate`; synchronous invocations balance the depth before returning. A scoped
production object check verifies those section assignments.

## Evidence and remaining scope

The [scheduler fixture](../tests/native/script_item_scheduler/README.md) compiles
real FMS through the production frontend, storage, VM, Item actor, receiver and
GObj lifecycle, and mechanically extracts the actual native scheduler bodies.
It passes 160 checks in 27 cases, including constructor failure cleanup, all
three phases, transient recipient events, nested state selection, true removal,
deferred retirement and interaction membership reuse.

Asset/model/geometry/audio leaves and immutable spec publication are controlled
boundaries in this fixture. Its property-write boundary deliberately triggers
reentry/removal to observe actual scheduler continuation. The fixture does not
prove engine scene/assets, full rollback aggregates, external source factory
publication or Workshop protocol parity; those require their respective suites
and subsequent integrated acceptance. No game launch is part of this evidence.
