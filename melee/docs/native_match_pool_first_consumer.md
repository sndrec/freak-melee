# First explicit match-pool consumer proposal: GObj and GObjProc

2026-10-07 source proposal, updated after scene-transition audit and root approval.
The native GObj/GObjProc consumers are now wired through direct source call sites;
focused console evidence is in `tests/native/native_gobj_pool_consumer/README.md`.
Full client and all-scene acceptance remain pending root-run checks.
The standalone pool/snapshot first slice is tested against actual GObj96 and
GObjProc48 byte native structures.

## Implemented first-consumer storage and oracle integration

`native_gobj_pool_storage.c` owns process-reserved typed payload4096/16384 slots,
slot/free metadata, pool bookkeeping/epoch and scheduler tables in NON-HOST
`.mstate`. This uses actual PE loader storage, not an OS/HSD heap allocation that
scene preload can reclaim. The existing forced state section and
nativeRollbackState_GetGlobalRegions include every byte in the broad page oracle;
focused linked fixtures verify the actual owner addresses/lengths, not just source
pragmas. No page-region count change or separate untracked VirtualAlloc is needed.
All other runtime families remain page-tracked during this milestone.

Direct native allocation/final retirement replaces the four source call sites.
Obsolete native GObj ObjAlloc descriptors are removed; census now reads the real
NativeMatchPool counters. The old hardware target remains behind its existing
non-native compile branch. Actual constructors/deferred unlink/destructors remain
in source. GObj exhaustion returnsNULL; proc exhaustion retains the source
assertion. New slot initialization zeroes unspecified native bytes explicitly.

Scheduler tables reserve full byte-sized extents:256 p-link heads/tails,257 GX
heads/tails,256 priority heads,65536 per-link/priority tails and256 method entries.
Their used scene ranges remain controlled by actual HSD config. The method list
is bounded/validated for cycles and byte-sized indices. No heap-backed table roots
point into a destroyed scene heap. Per-scene logical retirement runs before
preload; the next actual registry init binds a new epoch. It invokes no extra
destructors. Allocation is disabled between retirement and rebind, so an early
mode callback attempting GObj construction must be audited in later scene checks.

Live baseline capacity evidence at f38818617 is precisely one VS scene:
`A:/FreakMelee-test-archives/eight-actor-object-census-run/census.log`,2,001 samples,
errors0, GObj peak111/proc196, partial typed live payload peak1,388,376 bytes. The
source still gives no all-scene hard maximum. Provisional capacities are approved
implementation bounds with substantial margin, not a stock-equivalent saturation
claim or proof that complete explicit snapshots will fit under1MiB.

The actual source ownership is coherent and bounded enough to start here:

| Source | Existing operation | Direct replacement after approval |
| --- | --- | --- |
| `gobjinit.c: HSD_GObj_80391304` | Allocates p/gx head/tail and priority/proc tables; initializes `gobj_alloc_data` and `gobjproc_alloc_data` on every minor-scene entry. | Use one process-reserved GObj/proc family for all scenes. Reset logical ownership at this actual registry-reset boundary, advancing the scene lifetime. Retire old scene ownership before preload can destroy its pointed payload. Match snapshots capture only their current scene epoch. |
| `gobjplink.c: CreateGObj` | Allocates GObj then initializes semantic fields and inserts into the ordered p-link registry. | Change the allocation call directly to the one GObj pool in every scene, preserving constructor and insertion order. Remove the now-unused allocation helper when replacing that path. No match/non-match allocator branch, fallback or ObjAlloc adapter/shim. |
| `gobjplink.c: HSD_GObjFree` | Defers running-object removal; calls userdata/object/proc removal, unlinks p/gx registry, returns GObj to ObjAlloc. | Keep the real destructor/deferred-unlink sequence for ordinary gameplay removal; change its final storage retirement directly to the pool. Snapshot load itself performs no destructor. |
| `gobjproc.c: HSD_GObj_SetupProc` / `HSD_GObjProc_RemoveProc` | Allocates/initializes proc and queues it; running proc removal is delayed before unlink/free. | Replace actual allocation/final free with the proc pool, preserving callbacks/priorities/deferred flags and actual list order. |
| `gobj.c: HSD_GObj_80390CFC` | Rotates invocation marker; visits mutable priority tables; permits delayed object/proc/repriority operations. | Add explicit boundary save/load of the actual scheduler globals/tables alongside both pools. Snapshot/load at tick boundaries only. No mid-callback restore. |

This is a partial consumer milestone: userdata Fighter/Item/EF payload and HSD
model objects remain other owners. Copying GObj/GObjProc pools cannot resurrect
freed userdata/model graphs alone. The real page oracle must remain active for
all other state during this milestone; do not advertise the partial combination
as replacing the full game snapshot or measure it as complete explicit save/load.

## Concrete lifetime choice

Choose **process-lifetime physical storage, scene-lifetime logical ownership**
for this family. All HSD scenes allocate directly from the same two fixed pools;
there is no second allocator or fallback. A VS snapshot is valid only within its
particular scene epoch. Sudden death is another scene entry with another epoch,
even if the online session/preload assets continue. The next scene can reuse the
same addresses only after old snapshots and retained host references expire.

Actual call chain and ordering:

1. `gmmain.c` initializes Aurora/platform/HSD, then calls `gm_801A4510`.
2. `gm_1A3F.c: gm_801A4510` runs game-mode initialization and `runGameMode`.
3. For **each** minor scene, `gm_801A4014` calls `preloadState`, the mode state's
   `on_enter`, `gm_801A4BD4`, sets current `GameSceneInfo`, the scene's `on_enter`,
   then `gm_801A4D34`. After the loop it calls scene and state `on_exit` handlers.
4. `gm_801A4BD4` obtains GObj defaults, raises proc priority maximum to0x18,
   registers SObj methods, and invokes `HSD_GObj_80391304`. That resets every
   p/gx/proc table and allocator family, initializes invocation/deferred globals,
   and then creates `gm_804D672C` plus its no-op proc **anew**.
5. `preloadState` uses `lbDvd_80018CF4` / `lbDvd_80018254`. The former can call
   `lbHeap_80015900`, which recreates the HSD main heap using
   `HSD_CreateMainHeap`. Its class/object/ID/RNG/RObj forget callbacks demonstrate
   that old heap pointers can be invalid before the next GObj registry reset.
   Asset heaps2/3 can be explicitly retained. Preload equality can skip some
   cleanup, yet the GObj registry reset still occurs for every scene.

Therefore pool backing must live outside the repeatedly recreated HSD main heap.
Reserve it once during process startup, after platform setup and before the first
scene can allocate GObjs. Release it only on application shutdown. Separate
logical teardown belongs after all outgoing scene/state exit callbacks and
before the next `preloadState` can reclaim pointed HSD/userdata objects. The
existing registry initialization then binds empty pools with the next nonzero
scene lifetime and initializes scheduler tables before scene construction.

This logical teardown should not introduce a second destructor sweep. Source
transitions already abandon/reset the GObj registries and may destroy backing
heaps wholesale. Snapshot/pool retirement invalidates old ownership; ordinary
gameplay GObj removal still executes its real destructor/unlink sequence. The
source entry/exit order must stay intact. Add a retirement assertion that no
GObj/native callback is executing and no rollback session/window remains active.

## Which objects persist

**Scene-scoped:** VS fighters/partners, articles/items/effects, stage objects,
camera/subjects/HUD/countdowns and the GM no-op GObj. CSS has its own GObjs
(`mncharsel.c` creates classifiers2/3/4/14 across several p-links) and camera/
display objects; these are subject to the same next-scene registry reset.
`gm_Scene_Vs_OnEnter` creates match/HUD/timers after GObj initialization.
`gm_Scene_Vs_OnExit` produces result data; it does not establish process-lifetime
fighter/GObj ownership.

**Process- or mode-resident:** imported assets/preload cache/character family
definitions, native shell/Rml UI infrastructure, settings and host transport/
device state. Retaining asset heaps is not retaining their old GObj registry.
`native_home.c` stores workshop actor/model/camera pointers, but resolves preview
actors against current Player entities and full lifetime checks; those are
retained references to scene actors, not a separate immortal GObj allocation.
An explicit retirement hook must clear/invalidate such host scene references
before addresses can be reused. A future reference left outside restored game
state can otherwise alias a replay-created/new-scene object with recycled native
IDs. Nothing found in this audit establishes a GObj that must migrate physically
from CSS to VS or VS to results. Census should assert this rather than assume it.

The source's dynamic registry tables have scene sizes; current GM defaults yield
64 p-links,65 gx-link entries,25 proc-priority heads and25*64 proc-list entries.
With64-bit native pointers that is15,064 bytes of table storage. Reserve their
maximum supported extents once, reset logical extents on scene initialization,
and snapshot their used contents plus scheduler scalar state. Do not keep
reallocating these tables in a destructible OS heap after moving just the objects.
Audit any non-GM caller of `HSD_GObj_80391304` before adopting these maxima; the
source search currently finds the GM call above as the only game call site.

## Capacity and census plan

There is no proven global GObj/proc hard maximum in the source. `objalloc.c`
defaults `num_limit` to-1 and grows when empty, so setting a capacity changes a
formerly heap-exhaustion boundary. Capacity therefore needs concrete census and
declared support rather than a fabricated stock maximum.

Source gives useful lower-bound contributions:

* Each actual Fighter constructor installs15 procs (`fighter.c:1019..1033`), so
  eight paired actors account for120 procs as well as eight GObjs.
* The normal Item construction branch installs10 procs per Item
  (`item.c:1014..1023`). Item creation eligibility/caps are grouped by hold-kind
  in `Item_8026784C`, using runtime `Item_804A0C64` limits; they are not a single
  source constant. Record these live limits alongside the census.
* The declared article resource limit is6collections*90nodes=540 possible
  additional ItemLink GObjs. The current node constructor adds rendering but
  does not install its own GObjProc. They still belong to the object capacity.
* EF async effect count64 is one allocation policy, not proof that every sync/
  model-group effect shares a global64-GObj cap. Stage/camera/HUD/particle-driver
  GObjs and menu scenes add other live objects and proc multiplicities.

Read-only census proposal, performed by root in later authorized runs: sample
existing `gobj_alloc_data` and `gobjproc_alloc_data` used/peak at scene entry,
after scene construction, every complete frame and immediately before reset.
Add direct creation/free counters grouped by scene kind and GObj classifier/
p-link/proc priority, with fixed storage and no per-frame logging. Record pending
deferred deletion, maximum simultaneous live count, cumulative churn, allocation
failures, and objects still retained in host state at retirement. A live registry
walk can prove classifier/priority topology but does not by itself count storage
objects not yet linked; allocator counters and events cover those intervals.

Census needs direct eight-actor VS, CSS/SSS/results and workshop/practice entry/
restart, repeated VS-to-sudden-death/results/CSS cycles, item/effect/stage-heavy
stocks and custom article-resource saturation. No runs are launched by this
read-only proposal. Log actual scene labels and maxima; trace-call-site counts
cannot establish runtime capacity. Retained asset heap combinations should be
included because preload reuse does not suppress logical registry resets.

A provisional **diagnostic reservation**, pending that evidence, could be4096
GObj slots and16384 proc slots (the current utility limit). Actual native payload
sizes96/48 imply393,216+786,432=1,179,648 payload bytes, plus slot/free metadata
and scheduler tables. A fully occupied packed snapshot is about1.28MiB before
other game owners; sixteen such worst-case records are about20.5MiB. This is a
reviewable reservation budget, **not an accepted capacity or a sub-1MB claim**.
Prefer measured smaller maxima plus explicit justified headroom once census is
available. If a supported constructor cannot fit, refuse the unsupported scene
or apply its real deterministic failure policy; never allocate from an OS fallback
or silently drop gameplay callbacks. In particular proc allocation currently
asserts on failure, so its exhaustion policy must be explicitly accepted/tested.

Pool zero-initialization creates a deterministic policy for omitted fields. For
example current CreateGObj initializes named fields but does not initialize
`x34_unk`. Audit actual native reads and document any formerly undefined behavior
before parity acceptance. Keep actual source initialization values after the pool
clears storage. The source's deferred deletion must remain intact.

Fixture closure needed before the first consumer is accepted:

1. Actual `CreateGObj`, p/gx insertion, proc queue/removal and ordinary destructor
   bodies against the fixed pools; compare their event and ordering trace with
   current real bodies under declared deterministic initialization.
2. Priority changes and running-object/proc deferred deletion, saving outside
   callbacks; restore pool occupancy/generations plus all head/tail/proc tables,
   invocation marker and delayed-removal state coherently.
3. Create/free/reuse both families across the snapshot window and compare exact
   addresses/free order/generations after restoration and subsequent replay.
4. Pool exhaustion for objects/procs, all source allocation-result branches and
   cleanup; no silent growth/fallback to OS allocation.
5. Process reservation plus per-scene logical retirement/setup, including all
   outgoing callbacks before preload invalidates userdata. No external surviving
   pool references; existing objects never move. Snapshot ring ownership is a
   single scene epoch even though its backing reservation can survive the process.
6. Whole-loop oracle comparison with other owners still page-saved, including
   callbacks and rendering. Separately disclose partial pool bytes/save/load time
   and unchanged remaining page cost. Complete production replacement waits for
   all owner families from `native_match_snapshot_inventory.md`.

Private pool references do not replace existing script entity IDs or change
NativeCallback lifetime APIs. Existing native creation numbers must restore with
their own owners. A host-side future timeline reference must be cleared on rewind;
otherwise reproducing the same slot generation can alias a replay-created object.
