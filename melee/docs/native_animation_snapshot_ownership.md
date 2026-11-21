# Native animation snapshot ownership

Actual native AObj/FObj typed storage and occupied-record snapshot utilities
are implemented. This is a partial owner slice, not a complete match or animation
graph snapshot. Native GObj/proc, Fighter satellites and ScriptFighterActor are
separate owners. Broad page capture remains necessary for JObjs, target refcounts,
constraints, materials and satellites.

The implemented slice is **actual AObj plus FObj storage**, retaining the
native interpreter and callbacks. Primary and interpolation JObjs cannot be
discarded or reconstructed from a current animation frame. The existing native
Fighter scratch cleanup removed unused archive-copy buffers; it did not remove
`Fighter.x8AC_animSkeleton`, `FighterBone.x4_jobj2`, their animation graphs or their
effect/collision dependencies.

## Actual constructors and ownership

| Owner | Native source path | Source lifetime and dependencies |
| --- | --- | --- |
| AObj | `sysdolphin/baselib/aobj.c:HSD_AObjAlloc`, `HSD_AObjLoadDesc` | Direct nativeAObjPool allocation, then source zero/NO_ANIM/rate1 initialization. Descriptor load builds a new FObj chain and optionally takes a target-joint reference. Non-native source retains ObjAlloc. |
| FObj | `sysdolphin/baselib/fobj.c:HSD_FObjAlloc`, `HSD_FObjLoadDesc` | Direct nativeFObjPool allocation/zeroing, one decoder per channel. Copies stream identity/format and start frame into the runtime chain. Non-native source retains ObjAlloc. |
| Fighter channels | `melee/lb/lbanim.c:lbAnim_InitFrames`, `fn_8001E60C`, `lbAnim_8001E6D8`, `lbAnim_8001E7E8` | Direct AObj/FObj calls independently of descriptor loading. Replaces existing animation, builds ordered channel lists, moves branch channels first, and can omit translation channels from interpolation. |
| JObj | `sysdolphin/baselib/jobj.c:HSD_JObjAlloc`, `JObjLoadJointSub`; `class.c:hsdNew`, `_hsdClassAlloc` | Uses class-sized memory pieces. Named descriptors call `hsdNew(info)` directly; changing only HSD_JObjAlloc would miss them. Class init/load/release/destroy protocols are genuine ownership. |
| RObj / Rvalue | `robj.c:HSD_RObjLoadDesc`, `HSD_RObjAlloc`, `HSD_RvalueAlloc` | Constraint runtime records own resolved joint links, flags, expression/IK/limit state and optional AObjs. |
| scale / envelope matrix | `jobj.c:HSD_JObjMakeMatrix`, `JObjLoad`, `JObjRelease`; `mtx.c:HSD_VecAlloc`, `HSD_MtxAlloc` | Optional satellites have independent allocation/free. Matrix setup creates/frees scale helpers as actual state changes. |
| ID entries | `id.c:HSD_IDInsertToTable`, `IDEntryAlloc`, removal | The default101-bucket table links live runtime objects to resident descriptor addresses. Entry reuse/order and updates are mutable. |

Repository searches find actual AObj allocations only in `aobj.c` and
`lbanim.c`, and FObj allocations only in `fobj.c` and `lbanim.c`. All of those
paths pass through the actual HSD_AObjAlloc/HSD_FObjAlloc functions. Final AObj
and FObj storage release passes through HSD_AObjFree/HSD_FObjFree. Thus these
four source bodies are actual direct native consumer mutation points. No
allocator wrapper, mirrored legacy heap or fallback exists. Obsolete native
ObjAlloc families/getters/init functions are removed; current native census and
diagnostics observe the real typed pools.

AObj consumers extend beyond fighter joints: CObj, DObj, fog, light, material,
texture, world-position, shape animation and RObj code use these same families.
Stage `granime.c` installs them on stage/material/constraint graphs. A family
pool must cover all scenes and consumers, not eight fighter roots alone.

`HSD_AObjSetFObj` removes the old entire chain before installing a replacement.
`HSD_AObjRemove` removes its chain, nulls that link, unreferences its optional
target joint, nulls that link, then frees storage. FObjRemoveAll recursively
releases next-before-current, so free ordering affects later address reuse.
Normal deletion must preserve these exact source operations; snapshot load
must restore their results without rerunning them.

`HSD_AObjLoadDesc` resolves obj_id through the actual ID table. An existing
joint receives `ref_INC`; otherwise it calls HSD_JObjLoadJoint on the descriptor
address. Removing that controller may therefore delete another runtime graph
through HSD_JObjUnref. AObj packed state alone cannot restore that external
joint's refcount or resurrect its other owners. They remain in the broad oracle
until their own explicit owner migration.

## State that must survive rollback

| Record / global | Required saved state | Why it is not disposable scratch |
| --- | --- | --- |
| HSD_AObj (40 bytes in current native fixture) | flags, current/rewind/end frame, rate, FObj head, optional target object | First-play changes rate advancement; looping stops/restarts decoders and sets rewind flags; NO_UPDATE changes callbacks while decoding continues. |
| HSD_FObj (64 bytes) | Full record: next; stream cursor/head/length; flags; op/op_intrp; type/quantization; packet count; start frame/term; time; p0/p1/d0/d1 | Interpreter states0..6 retain partially consumed packets, waits, interpolation history and pending/consumed KEY bits. Resetting from a frame can change event count/order. |
| HSD_JObj | Full native class record and refcounts, parent/child/next, union identity, flags, SRT, matrix, scale/envelope pointers, AObj/RObj links and ID | Pose writes are read by movement, collision, dynamics, effects and matrix/constraint evaluation. Dirty and visibility flags select future work. |
| JObj satellites | Occupied scale Vec and envelope matrices, lifetime/order/identity | Pointer presence changes allocation/free and matrix behavior. Current matrix values are consumed before some later recomputation. |
| RObj / Rvalue | Full records and ordered links; resolved targets; active flags, limit/IK values, expression metadata; animation links | JObjUpdateFunc writes IK hint rotation; RObj animation toggles active constraints. Expression evaluation caches nb_args in the owner. |
| ID table / entries | All bucket roots and occupied entries, including update ordering and free metadata | Later descriptor resolution and release test these identities. |
| Class bookkeeping | nb_exist/nb_peak, relevant initialization/class roots, class memory-piece owner/free state until migrated | Runtime class allocation/destruction changes counts and chooses later storage. Live counts alone do not save allocation order. |
| AObj globals | Finished/active counters and endcallback-list identity/nodes when populated | InitEndCallBack resets before graph animation. Interpret increments; Invoke executes only when finished>0 and active==0. |
| JObj globals | default/current joint, user callback-list identity/nodes, configured particle/sound/target callback identities | Source callback dispatch and class-amnesia paths depend on these globals. Function code itself is resident, but selections are mutable. |

`FObjUpdateAnim` mutates LINEAR slope-ready and KEY consumption flags while
producing a callback. KEY stop may flush additional events. These are native
event semantics, not a pose cache. Snapshot replay must execute the original
callback sequence with restored target owners and RNG. It must not invoke
callbacks while saving/loading state or reconstruct a pose as a substitute.

`ftAnim_8006E054` reads animated TransN positions into Fighter movement deltas,
then writes translated joints back. `ftAnim_8006E9B4` blends from the separate
interpolation graph. `ftAnim_8006FE08` installs tracks onto primary/interpolation
parts according to the actual blending state. `ftParts_800753D4` inserts paired
nodes in both trees; `ftParts_800755E8` removes both. These source dependencies
refute treating the second graph as an unused scratch buffer.

`HSD_JObjDispAll` calls SetupMatrix on instance nodes and children, then composes
their transforms with the current camera matrix. SetupMatrixSub can clear dirty
flags, evaluate IK/RObj and mutate matrices. `HSD_JObjMakeMatrix` allocates/frees scale
helpers. Full draw callbacks must remain inside replay and their state writes
must be saved. No draw-state or draw-dependent RNG exclusion is established.

JObjUpdateFunc dispatches SETBYTE/SETFLOAT callback lists and particle/sound/
target channels, as well as direct SRT, visibility and matrix writes. In this
checkout the private ufc_callbacks and AObj endcallback_list have no insertion
site (only static initialization and clear/amnesia writes); jsound_callback and
ptcltgt_callback also have no assignment site. They are dormant here, not a
reason to assume event channels never mutate state. The actual DPtcl setter is
used and must retain its source callback identity/order. The fixture injects an
end callback solely to prove the existing protocol's dependencies.

## Resident source data and bounded scratch exclusions

FigaTree/FigaTrack and AnimJoint/AObjDesc/FObjDesc hold source selections.
FObjLoadDesc/lbAnim constructors copy their channel recipes into separate runtime
records; the decoder advances `fobj.ad`, not descriptor bytes. The bounded fixture
successfully interprets CONSTANT/LINEAR/SPL0/SPL/KEY FLOAT streams on a protected
read-only page. Original packed streams can stay resident for that proven path.
Save their pointer/format selections in the runtime record, not their byte
capacity per tick. Source archives must remain resident for the snapshot epoch;
asset unload/selection transitions invalidate those snapshots.

Particle commands in `joint.u.ptcl` are shared read-only. Each live JObj owns
its pending first-draw events. Restoring the JObj restores those events, so a
hidden rollback frame can emit them at the original time. Spline, material,
shape and other event descriptors still need their own write audit. The page
oracle tracks their writes during this transition.

RObj expression arg_buf contents are populated from all selected values before
synchronous evaluation; its address/size is lazily allocated and retained until
ForgetMemory. Its numeric content is a candidate for omission only at a proven
outside-evaluation boundary, after validating every supported expression channel
and the actual expression callback contract. Its allocator metadata and owner
pointer cannot be omitted yet. No production scratch exclusion is made here.

## Implemented owner contract

`sysdolphin/baselib/native_animation_pool.c/.h` owns process-reserved AObj and
FObj payload arrays plus existing NativeMatchPool generation/free-order machinery.
Backing, slot/free metadata, bookkeeping, globals and heap epoch stay non-host
`.mstate`; the current broad oracle captures them while other graph owners remain
heap-backed. Capacities4096/16384 are explicit bounded choices:5.60x/7.21x the
731/2271 peaks in one refreshed eight-actor workload. This is a lower bound on
needed capacity, not an all-scene proof. Recursive descriptor graphs and source
channel counts do not establish a smaller universal bound. Exhaustion asserts
explicitly, with no truncation or fallback.

The pair binds at the **actual HSD object-heap initialization boundary**, with
one shared nonzero heap epoch. HSD_InitComponent initializes at boot;
HSD_CreateMainHeap calls HSD_ObjInit during preload, after class amnesia and
before ForgetMemory/heap recreation. HSD_ObjInit preflights/binds both fixed
families before other initialization. Root's CreateMainHeap hook preflights
before class amnesia. Active callback/scheduler/deferred work and epoch overflow
are rejected before mutation. Rebind performs no allocation/destructor/callback.
It preserves source counters/list identity; actual InitEndCallBack and
ForgetMemory sites reset them. Non-native ObjAlloc initialization remains intact.

There is no reset at GObj bind or Fighter initialization. lbDvd_80018254 can
skip heap recreation for unchanged preload state. Then surviving JObj/class/ID
owners still need their animation slots. Logical garbage stays occupied until
the actual source heap reset, as with the former family. Constructors require
an already active heap epoch. The fixture runs the actual unchanged-preload
branch and actual CreateMainHeap sequence with controlled outer asset/class/heap
hooks; broader live scenes still require root's acceptance.

HSD_AObjAlloc/HSD_FObjAlloc call NativeMatchPool_Allocate directly and retain
source memset/flags/rate construction. Final frees validate actual pool identity
and return storage directly. Remove/SetFObj/refcount/callback behavior and
next-before-current chain release remain intact. FObj mutation callbacks and
AObj end callbacks bracket depth to reject reentrant save/load/reset while
preserving the source dispatch order.

Save packs both occupied families and logical metadata plus actual AObj
counters/list selection. Validate checks both pools, epoch, lengths, counters,
internal membership, FObj chain cycles, state/opcodes and numeric cursor bounds
before either family loads. The chain audit is linear and uses bounded fixed
stack storage. Cursor==head+length at completion and NULL before ReqAnim are
valid. Raw same-process resident asset/external target pointers remain identities
for this private ephemeral snapshot, not a serialized format, compatibility layer
or native compiler ABI.

Load restores freed/reused slots and ordered allocation state without source
constructors, destructors, references or callbacks. External JObj refcounts,
ID tables, mutation targets, archive lifetime and other owner links must be
restored coherently by the broader snapshot before native callbacks resume.
This owner validates its internal graph, not arbitrary external pointer domains.
It does not independently resurrect the entire animation graph.

Unused free-slot payload is omitted because every allocation clears all bytes.
Raw page hashes may differ on inaccessible free bytes after later-to-earlier
packed restoration even when occupied records, generations and free order agree.
No complete backend closure or performance claim is inferred from this owner.
Root owns builds and hidden/muted live acceptance; this agent performed no game
launches, global builds or commits.

## Evidence and payload accounting

The previous2001-sample eight-actor census at
`A:/FreakMelee-test-archives/eight-actor-object-census-run/census.log` had21 observed
families but omitted AObj/FObj/Vec/Mtx/RObj/Rvalue/ID. Source explains the gap:
HSD_CreateMainHeap calls HSD_ObjInit and then _HSD_ObjAllocForgetMemory, clearing
alloc_datas while those family descriptors remain genuine live allocator owners.
Do not infer zero usage or immutability from that omission. Its Fighter32KiB
entries also predate the confirmed scratch removal and current typed owner.

The pre-migration named census read all seven actual accessors, deduplicated
against the registry by descriptor identity, retained the real class tree
and printed stable family labels. The diagnostic does not mutate registration, allocation,
game/render/RNG state or callbacks. Diagnostic storage stays host-excluded.
`A:/FreakMelee-test-archives/native-match-object-census-animation-named/receipt.json`
passes13 samples/45 checks, with registered/named alias dedup, six direct-only
families, malformed-topology rejection and full fixture game-byte preservation.
The subsequent zero-error live census at
`A:/FreakMelee-test-archives/live-script-packed-state-census/census.log` observes
731 peak AObjs and2271 FObjs, with final counts502/1234. Vec/Mtx/RObj/Rvalue/ID
also appear directly. Its total is still a partial live payload estimate.
After migration AObj/FObj census entries come from the actual typed owners;
remaining satellite families still use their genuine named ObjAlloc descriptors.

For current native sizes, simultaneous animation decoder/controller payload is
`40 * live_AObj + 64 * live_FObj` bytes. The old census reported184-byte JObj
class instances, including625 hsd_jobj +392 interpolation +8 fighter JObjs at
its final sample (188600 bytes for those records alone). Their pointed materials,
constraints, matrices and other satellites are additional. Independent class
peaks cannot be summed into a simultaneous scene measurement.

Existing generic packed pool bytes are exactly
`sizeof(header) + 8 * capacity + object_size * used` (four bytes per slot
generation plus ordered free indices and occupied-record indices). This is a
per-pool calculation, not a measured complete match snapshot size.
Use actual simultaneous occupied counts and measured packed bytes after owner
integration; reserved capacity, pointed live bytes and immutable assets are
different quantities. No sub1MiB total or save/load timing claim is established.

The historical dependency receipt
`A:/FreakMelee-test-archives/native-animation-dependency-opcodes/receipt.json`
passes64 cases/947 checks with protected resident streams, exact full-state
replay, isolated missing-decoder divergence and real FigaTree constructor paths.
Allocation, ID lookup, target unref/flag behavior and mutation/end callbacks are
controlled fixture boundaries; actual decoder/AObj/RNG/lbAnim source runs. See
the fixture README for the exact command, hashes and remaining coverage limits.

The implemented owner receipt at
`A:/FreakMelee-test-archives/native-animation-pool-ctest-extraction/receipt.json`
passes90 cases/1079 checks (64 decoder,26 owner). Actual full AObj/FObj/lbAnim/RNG
and typed pool/snapshot units run. All30 opcode/rate/loop combinations now restore
through the real packed API. Owner tests cover reference fallback, free/reuse/
resurrection/allocation order, wrong-pool/cycle/cursor/counter/length/duplicate
rejection, no partial first-family load, full backing unchanged after failure,
empty/full capacities, actual source exhaustion/double-free/wrong-family/reset
panic branches, skipped/actual heap reset and actual global-region recognition.

Measured fixture bytes are163992 empty and1376408 with both families fully
occupied. Applying the schema to the old census's final502/1234 counts projects
263048 bytes. Independent731/2271 maxima yield an upper bound338576 for that
workload, not a simultaneous observed snapshot. These describe AObj/FObj only
and include fixed capacity-wide generation/free metadata. Other graphs/global
state remain additional; no full match size or save/load speed is established.


## Root live owner integration

The actual client and tool build; 33 focused CTests include the full owner fixture,
retail animation-bank import, existing animation/pose editing and interpreter
contracts. The console check macros abort even in release builds.

With `FREAK_MELEE_ANIMATION_SNAPSHOT_CHECK=1`, each benchmark boundary captures the
packed owner. After broad restore it compares that record, deliberately restores
later animation state, loads the earlier packed state, compares it again and runs
15 replay ticks. All 64 windows reproduce complete raw/normalized/semantic
endpoints alongside the script-owner overlay. The simultaneous packed animation
peak is 335,056 bytes. Capture time totals 312.82 ms across all forward/replayed
boundaries; two loads per window total 16.36 ms. These are diagnostic component
totals, not complete match save/load performance. Full rollback remains 179.12 ms
on average with all 64 deadline misses.

Receipt: `A:/FreakMelee-test-archives/live-packed-animation-owner-rollback/receipt.json`;
component markers and full-window result are in `stdout.log`. Retained decoder
streams, external JObjs/refcounts/materials and other graph/global owners still
use broad capture.

The one-stock stock-behavior run consumed 10,000 recorded input samples, completed
the actual VS scene and constructed Results (GS_VS=2, GS_RESULTS=5). Both scene
censuses had zero errors; the owned process exited zero. The initial 2,000-sample
attempt did not reach Results and is not accepted as match-end evidence.
Receipt: `A:/FreakMelee-test-archives/live-animation-owner-match-end-long/receipt.json`.
This covers one stock workload, not every preload/scene/stage path.

Capacity-wide generation/free metadata currently accounts for 163,992 empty
snapshot bytes. Cold, never-allocated slots have reconstructible initial metadata;
eliding that metadata and walking only occupied records are future generic pool
optimizations. This does not justify skipping decoder state or external owners.
