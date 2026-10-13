# Actual native GObj/GObjProc fixed-pool consumer slice

The scheduler now also has an explicit component. It saves actual occupied
GObj/proc pool payloads, generations/free order, active p/GX/proc table extents,
the invocation marker, delayed-record bytes, render kinds/flags and the pointed
process mask. Config and removal method bindings stay frozen for the scene.
Validation follows serialized records only and checks complete membership,
cycles, owner/prev links and every head/tail before writing live state.
Fixed host scratch is outside .mstate; there is no allocation or per-tick
snapshot tracking. The new fixture removes/reuses nodes, changes priorities and
flags, restores exact bytes and replays the original callback sequence without
a copied-region companion. It also covers malformed candidates, proc/draw
boundaries, empty/max-capacity pools and the full configured table dimensions.
VM actors/userdata/model graphs remain companion owners. The connected resource
fixture separately joins real interpreted actor state with this component.

Native CreateGObj, SetupProc and final storage retirement now call the real
NativeMatchPool operations directly. The native build no longer retains GObj
ObjAlloc descriptors or mirrors their counts. Existing constructors, p/GX/proc
ordering, userdata/object removal and deferred deletion remain in their actual
HSD source bodies. The original non-native hardware branch is unchanged.

`native_gobj_pool_storage.c` reserves 4096 GObjs and 16384 proc slots in the live
scene arena. Payloads, slot/free metadata and configured scheduling tables share
that contiguous backing. Descriptors and fixed pointer roots remain in `.mstate`;
mutable scheduler controls are also admitted external roots for copy-only arena
history. CMake must never mark this owner unit HOST_STATE. The broad oracle now
captures both the global region and the new backing while other owners migrate.
Full pool capacities remain a footprint reduction target. This does not yet
replace the complete match snapshot.

Physical storage persists for the process. Logical retirement is called at the
start of the next `gm_801A4014`, after the prior scene/state exit callbacks and
rollback shutdown, before preload can invalidate pointed userdata. It clears
registry tables/ownership without new destructor calls. The existing per-scene
HSD_GObj_80391304 binds empty pools with a new nonzero epoch before scene objects
are created. Retiring during a proc/draw/deferred callback is refused. Private
old-epoch pool references expire. Source/native entity ID owners and external
host caches remain separately responsible for their scene lifetimes.

Game allocation exhaustion returnsNULL for GObj construction, as its existing
callers expect. SetupProc retains its source assertion on exhaustion. There is
no fallback allocation or silent callback dropping. Metadata generation overflow
also refuses allocation. All new allocations zero the actual payload before
the unchanged source constructor: `x34_unk` and previously unspecified proc bits
therefore have an explicit deterministic zero policy, not undefined-byte parity.

The actual census before wiring (`f38818617`,
`A:/FreakMelee-test-archives/eight-actor-object-census-run/census.log`) reported
GObj peak111 and proc196 across2,001 zero-error VS samples, plus partial typed
payload peak1,388,376 bytes. That single workload does not prove all-scene capacity.
The large provisional margin also covers the source-declared540 article nodes;
other scenes, stage/effect saturation and custom packages still need census.

Focused console verification links the actual complete HSD source files:

```powershell
python tests/native/native_gobj_pool_consumer/run.py --compiler A:/FreakMelee-test-archives/baseline72/compiler --output A:/FreakMelee-test-archives/native-gobj-pool-consumer-check
```

Historical receipt `A:/FreakMelee-test-archives/native-gobj-pool-consumer-final/receipt.json`
passes8 scenarios/20,615 assertions. It covers real ordered creation/proc queues,
source draw callbacks, priority changes, userdata cleanup, deferred object and
running-proc removal, address/generation reuse, packed pool payload resurrection,
scene lifetime expiry, complete object exhaustion and source proc exhaustion
assertion. Malformed/cyclic method registration is rejected before pool mutation.

The fixture invokes real nativeRollbackState_GetGlobalRegions, proves both full
payload/slot/free/descriptor families and scheduler arrays/method table lie inside
the global region or live arena, copies/restores both and verifies coherent topology,
callbacks, invocation state and replay allocation identities afterward. Its control
and result buffers are outside `.mstate`. Light/model/renderer primitives are
trapped boundaries; there are no assets or gameplay/rendering acceptance claims.
The historical generic packed pool load alone did not restore scheduler roots.
The current explicit scheduler component restores those roots and both pools;
the central match aggregate must still join the other owners before callbacks.

`arena_cases.h` separately uses prebound copy-only history against those actual
objects/procedures/tables, without packed-record validation. It exercises repeated
restore, saved control roots, same-address deletion/recreation, generations,
arena cursor rewind, initialized reclaimed bytes, exhaustion and scene expiry.
`arena-oct12-gobj-final/receipt.json` passes 18 scenarios / 41,554 checks and reports
1,460,780 copied bytes for this provisional pool capacity, not a whole-match size.

Root performs full client build and hidden/muted game/census/rollback smoke.
Additional all-scene retirement evidence is still required, especially any mode
on_enter callback that tries to allocate a GObj before the registry initialization
and host references retained across a scene. Allocation is deliberately unavailable
after logical retirement until the next bind; no old arena fallback is provided.

The constructor-cleanup extension also links the complete userdata and attached
object removal bodies. Nested Free/RemoveUserData calls retain published userdata
for the removal hook, execute that hook once and retire the original physical
allocation once. Standalone userdata removal honors a nested destruction request
after the hook returns; scheduler-delayed removal keeps its existing order.
Peer destruction with same-address replacement preserves the new peer. Native
callback scene retirement/rebinding also leaves the next scene's same-address
userdata, attached object and list membership untouched. Destruction flags are
part of each pooled GObj payload, so both broad and packed snapshots cover them;
newly allocated objects reset all flags. The fixture controls absent script actors
and traps model/light rendering; it does not prove the Fighter constructor or
whole gameplay path. Final constructor-cohort receipts live in the corresponding
constructor checkpoint rather than changing the historical receipt above.

Actor-prefix extension: fighter, ground, item and item-link payload windows
share one process reservation. Scene history captures each initialized prefix
and its descriptor, generations and free order. The fixture checks repeated
restore and same-address, same-generation continuation into discarded future
slots, including zero initialization. Full capacity remains available; this is
not yet a whole-match snapshot or a production rollback replacement.
