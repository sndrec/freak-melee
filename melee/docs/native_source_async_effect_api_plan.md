# Direct async attached effect proposal

Read-only source audit, 2026-10-09. No new binding, queue protocol, numerical
fixture, or gameplay acceptance is claimed.

## Needed source contract

Current `peach_full.fms:999–1000` sets `effects.active = true` and calls
`effects.spawn_async_attached(attrs.float_particle, TransN)`. The original
`ftpeachfloat.c:26–29,45–47` does exactly that: set `x2219_b0`, resolve
`FtPart_TransN` with `ftParts_GetBoneIndex`, and call
`efAsync_Spawn(gobj, &fp->x60C, 0, 1236, joint)`.

Proposed operation:

```text
effects.spawn_async_attached(effect: IntegralAny, semanticPart: U32) -> Void
```

Fighter-only, effectful/MayInvalidate. Require the actual owned Fighter
program/storage, selected SourceFighterLayout, scene/native generation, and
exact imported rig. Resolve the semantic part once through the existing
owned part map; admit the selected native JObj's class/extent and revalidate
source/rig/joint after native class admission. Reuse the existing
`FeedbackSource` capture/current policy and
`nativeScriptEffectDispatch_Arguments(effect) == Joint`. Reject wide/negative
effect IDs before narrowing and any ID needing Position, JointScalar, or
another variadic recipe. This operation supplies precisely one JObj argument
to the eventual sync dispatcher, with no script-native pointer exposure.

The legacy exporter also permits a numeric **raw** joint expression
(`native_fighter_script_primitives.c:822–836`). That numeric path is distinct
from its named semantic-bone path. Peach needs only the semantic form, already
consistent with `effects.spawn`. A raw-joint operation, if later needed, must
have an explicit contract rather than reinterpret numeric semantic selectors.

The call must not toggle `effects.active` itself. That Bool RW property
already exists (`binding_properties.c:186,274`), as does the only other effect
call in this template, `effects.spawn_at_position(1234, ...)` at line 267.
There are no additional missing effect operations justified by this Peach
source. The declared source ModelEffect backend is a different feature;
`effects.spawn_root` prepares a named EF model actor and authored update code,
whereas this primitive uses a native graphical recipe with no authored
callback.

## Native ABI and ownership

`Fighter.x60C` is a `void*` **queue head** (`ft/types.h:1265`), not a counter,
script state field, output, or handle. `efAsync_Spawn` (`efasync.c:2358`)
allocates a real `EF_QueuedEffect` with
`HSD_ObjAlloc(&efAsync_AllocData)`. Allocation failure is a healthy native
no-op. `EF_SPAWN_ATTACH == 0` (`ef/types.h:50`) stores the ID and JObj and
consumes no extra variadic arguments. Other modes copy their scalar/vector
arguments into the node, but they are not part of this operation.

If `HSD_GObj_CurrentInvokedProc` exists with `s_link < 9`, Spawn prepends the
node to the source Fighter's queue, giving native LIFO order. Otherwise it
processes immediately. `efAsync_QueueProcessDeferred` mode 0
(`efasync.c:2286–2289`) calls `efSync_Spawn(id, gobj, jobj)` and frees the
queue node afterward. It initializes `native_policy` and `native_author` to
NULL, so it never enters the separate Definition-dependent authored model
policy branch at `efasync.c:2262`. That branch is not an adapter to use here.

The exact Peach recipe is `efSync_Spawn` case `0x4D4 == 1236`
(`efsync.c:946`): consume one JObj and call
`efLib_CreateGenerator_Attach(0x11E, joint)`. `eflib.c:852` invokes
`hsd_8039EFAC`; `generator.c:1022–1041` retains the joint with `ref_INC`.
`hsd_8039F05C` admits the loaded particle command table and initializes the
real generator. It returns NULL for missing/out-of-range resources. This
recipe uses common particle bank 0, command 286, not an imported EF model
graph, a source Effect declaration, or a stock Peach handler. Existing Joint
admission already includes 1236. No additional vararg family or model import
is needed for it; real particle table initialization remains an engine
resource prerequisite, not a fake successful graphics leaf.

## Drain, cancellation and concrete lifetime scope

`efAsync_QueueFlush` (`efasync.c:2330`) processes head-to-tail, caches the
next node, then clears the head. The ordinary Fighter drain is
`Fighter_8006C80C` (`fighter.c:2626`), before transformation/accessory delivery,
and skips a frozen `x221F_b3` Fighter. Motion entry also flushes at
`fighter.c:1125`, before collision/GFX cleanup. Thus this is not uniformly
"end of frame": its timing is determined by the real proc link and these
native stages.

`efAsync_QueueClear` (`efasync.c:2344`) frees pending nodes without spawning.
Fighter cleanup/reset uses it at `fighter.c:687`; destruction uses it at
3283 before the pointed native objects are reclaimed. Existing native
Fighter destruction owns cancellation if the source retires before drain.
The binding must preserve this queue ownership, use no alternate pending
list, and revalidate the retained source and selected joint after allocation
or immediate processing before publishing Void.

The live effect cleanup policy remains native: motion changes without
`Ft_MF_KeepGfx`, when `effects.active` is set, invoke
`ftCommon_8007DB24` (`fighter.c:1290`). It clears the active bit and calls
`efLib_DestroyAll`; that routine walks the source root with `hsd_8039D688`
(`eflib.c:326`) to remove attached particles as well as its model effects.
Pending queue cancellation and live particle destruction are separate
operations; do not make `effects.active = false` a queue clear or destroy.

The actual 1236 allocation/dispatch path contains **no authored callback**.
It creates a generator and retains a JObj, without running animation or a
source scheduler. For other currently admitted Joint model recipes,
`efLib_Create` initializes `update` and `native_policy` to NULL, constructs
the native object and schedules its later native update. `efSync_Spawn` can
immediately drain newly loaded animation (`efsync.c` final animation loop),
but its SP/DP particle callbacks are the native `efLib_Cb_SPtcl/DPtcl`
(`eflib.c:1086–1104`), not `nativeScriptEffect_Run`; source effect execution
is in the later `efLib_Update` path at 486. Some recipes assign native
graphical update routines, also delivered later, rather than invoking an
authored helper during queue drain.

No observed admitted mode-0 path reenters source code and retires its owner
during Flush. Therefore missing Flush continuation guards and raw queued
JObj provenance are **not evidence that a new protocol or detached-drain
algorithm is required for this slice**. The earlier hypothetical case in
which a nested removal frees cached next nodes must not be presented as
observed native behavior. Require the normal binding retention checks and
prove existing cancellation/rig lifetime ordering in a fixture. Add a
native continuation barrier or new retained queue record only if an actual
supported path demonstrates retirement or rig replacement between enqueue
and dispatch; do not impose a speculative protocol on healthy mode-0 use.

## Allocation and snapshot boundary

`HSD_ObjAlloc` can grow the free list through `HSD_ObjAllocAddFree` and
`HSD_MemAlloc` when empty (`objalloc.c:88–131`). The queue call has no
transition reservation of its own. Existing native HSD reservation APIs
can bind actual allocator demand, but this audit has not established a
queue/generator capacity or connected reservation for this call. Zero heap
growth during ticks therefore requires explicit transition-time capacity
proof; reusing a native allocator alone does not prove it.

Current same-process native snapshots cover these pending pointers/records
through existing storage: `nativeFighterPool` places the actual Fighter,
including `x60C`, in non-host `.mstate` (`native_fighter_pool.c:9–14`), and
its typed pool Save copies each entire occupied slot
(`native_match_snapshot_pool.c:69–73`). The main build forces writable
game/HSD globals, including `efAsync_AllocData` and ObjAlloc metadata, into
`.mstate` with no zero-initialized BSS (`CMakeLists.txt:474–485`).
`nativeRollbackRegions_Build` registers active main heap bytes, global state,
and exact OS allocator metadata (`native_rollback_regions.c:58–124`).
The actual queued node and generator allocations reside in that native heap
path. `NativeScriptFighterSnapshotRecord` does **not** independently encode
the queue records, particle objects, or their pointer graph. This is current
native heap/page-oracle participation, not a completed packed effect codec;
any later external reservation backing must be registered substantively.

## Bounded implementation evidence

One grouped fixture should compile and execute the source call with its
real descriptor, VM, owned Fighter/rig admission and actual queue functions.
Compare mode-0 inputs and LIFO order against native entry; cover immediate
proc/no-proc and deferred link<9 paths, frozen drain, motion-entry drain,
queue allocation failure, cancellation before destructor/rig release, and
source or joint identity change during native admission. Exercise actual
1236 generator dispatch and JObj ref ownership with loaded command data
controlled explicitly. Verify active/KeepGfx cleanup remains independent.
Use native queue save/load evidence if snapshot behavior is claimed.
Controlled particle tables, allocator backing and graphical leaves must be
listed; counters alone do not establish attached rendering or full Peach
gameplay. No broader scalar/offset async recipes are needed for this batch.
