# Actual native Fighter fixed owner consumer

The native Fighter pool reserves64 stable scene-epoch slots. Each32-byte-aligned
slot contains the actual Fighter first, its full0x424-byte backup, all140 parts,
124 primary DObj pointers and32 ordinary secondary DObj pointers. Actual slot
sizeof is19,136 bytes; current Fighter sizeof is13,440. Backing, slot/free
metadata and descriptor are deliberately NON-HOST .mstate, so the existing broad
page oracle captures this finite owner while other families remain page-tracked.

Normal Fighter_Create and ftDemo_CreateFighter allocate the slot directly through
NativeMatchPool; no legacy allocation adapter, mirrored allocator or fallback
exists. Backup assignment occurs at the original constructor phase. Actual
ftParts_80074E58 and ftCo_800C884C bind their embedded arrays at the same source
phases as before. Ordinary unload keeps all real callbacks/graph cleanup and
retires one owner slot at the final storage-free point. Allocation initializes the
complete slot; zeroing the Fighter again is unnecessary. Exhaustion returnsNULL
and releases the constructor's newly allocated GObj without creating a half owner.

Four native allocator families are obsolete and removed. The shared
fighter_x2040_alloc_data remains because Kirby hat/copy lists and Purin's extra
list actually use it; embedding ordinary x203C does not make that allocator dead.
Those extra lists, runtime JObj/AObj/FObj/dynamics/effect/CPU/script/Player/global
owners are outside this packed owner slice. Do not claim complete match snapshots.

Root's actual GObj scene bind calls CanBeginScene before any pool mutation and
BeginScene once with the new epoch; normal/demo family init does not rebind.
Initialization refuses an unbound or live-owner family. Root scene retirement
first checks zero VM/register/payload depth, logically retires script actor slots
without callbacks, then Fighter owners, then GObj/proc ownership before preload.
Fighter snapshots additionally refuse active proc/draw/deferred/VM invocation.

The connected constructor cohort uses complete unchanged `Fighter_Create`,
`Fighter_UnkInitReset_80067C98`, `Fighter_UnkProcessDeath_80068354`,
`Fighter_Unload_8006DABC`, parts assignment, and `efLib_DestroyAll` bodies. It
links the actual compiler, prepared/published program, Fighter actor, VM/storage,
callback lifetime checks, and native GObj/Fighter pools. The first Load observes
generation 0; the actual reset publishes its legitimate NewLife while preserving
the same source program and persistent storage. Healthy construction keeps native
Load -> reset/attributes -> Death -> CPU initialization -> neutral entry -> final
input bookkeeping ordering.

Native construction retains both physical allocations. Attach/Load, reset and
startup failure return NULL after freeing only the original still-live owners;
a faulted actor is cleaned up, while a retired/reused allocation is untouched.
`native_constructing` stays true through constructor callbacks and clears only
after success. Unload checks original physical owners after destructive callbacks,
guards the optional camera, and destroys live VFX before their supporting joints.
Failed-only resource cancellation and Item cleanup also cover Load-era children
after NewLife. Ordinary Item removal preserves its selective ownership policy.

```powershell
python tests/native/native_fighter_pool_consumer/connected_run.py --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/native-fighter-constructor-final
```

The connected cases cover genuine VM division faults at Load, attribute reset,
Death, CPU initialization and neutral entry; physical retirement and allocation
reuse at those boundaries; recursive Free/RemoveUserData from authored and stock
Remove callbacks; stock Load admission failure; and Load/Death/Remove-created VFX
on both native effect lists. Actual `efLib_DestroyAll` removes those physical GObjs
and parameter-table references before joint teardown. VFX allocation/data,
generator traversal and effect userdata destruction are controlled engine leaves;
this is actual native teardown coverage, not actual effect factory or authored
model-effect cancellation acceptance.

One quiescent replay case saves the existing broad global oracle, runs a genuine
Death VM fault through the actual constructor/teardown, restores the saved owner
region, and repeats. It compares the complete resulting region and call trace,
including real GObj/Fighter/actor pools, allocation metadata/free order and the
constructor spawn counter. The selected compiler/package and controlled asset
leaves stay immutable. This bounded failure replay does not establish a complete
game snapshot or restoration during an executing callback.

Other controlled leaves are asset/model construction, camera/audio/status work,
LoadSpecialAttrs and neutral engine work that dispatch the actual hooks, and the
CPU engine leaf that dispatches actual CpuInitialize. Command writes provide the
explicit physical-retirement test boundary. Failed Item/resource operations admit
empty controlled tables here; their populated ownership and borrower proof lives
in the separate connected Item fixture. Player construction/role clearing and
CPU history selection likewise have separate actual-body proofs. No manual
userdata transplantation into a live GObj, imported rig/controller, renderer,
audio, game acceptance, or complete match snapshot claim is made here.

The original pool/snapshot consumer remains runnable with explicitly controlled
script/effect/resource leaves. Its original ten scenarios still compare the
packed owner geometry, allocation reuse and broad global oracle; the connected
runner supplies the real actor/VM proof rather than attributing it to those leaves.

Packed save/load uses the existing occupied-only pool representation and adds
canonical owner geometry plus record validation: source slot alias pointers,
optional instance attribute/multi-jump alias, list counts, valid kind/GObj pointer
andNULL legacy archive scratch. Snapshot validation does not dereference graph or
GObj relations that may be restored by another owner. All-owner validation and
publication remain a central aggregate responsibility. Wrong aliases/lengths/
pool/epoch/duplicates are rejected before mutation; load invokes no callbacks.

```powershell
python tests/native/native_fighter_pool_consumer/run.py --compiler .local/baseline72/compiler --output A:/FreakMelee-test-archives/native-fighter-pool-consumer-check
```

Final fixture receipt:
`A:/FreakMelee-test-archives/native-fighter-pool-consumer-shape-final/receipt.json`.
Ten scenarios,1,037 assertions,35 normal and34 demo setup calls. A fully occupied
64-slot packed owner snapshot is1,225,272 bytes; this worst-case capacity is not
the eight-actor snapshot or total game footprint. No save/load timing claim.

The fixture mechanically extracts complete actual normal/demo create, family
init, parts/list assignment and unload source bodies without rewriting, and
links actual HSD GObj/proc pools and scene ownership code. It tests:

* Source-phase satellite bindings, highest attribute/part139/list123/list31 and
  authored state byte255 preservation, and instance attribute/multi-jump alias.
* Actual unload/reuse/resurrection, generation identity, packed owner restore,
  actual PE global-region recognition and coherent broad-oracle restoration.
* Wrong backup/parts/scratch/count record rejection with entire live payload,
  descriptor, metadata and free order unchanged.
* Live and unbound family initialization refusal, normal/demo callback retirement,
  full64-owner exhaustion with new-GObj cleanup and old-epoch snapshot rejection.

Controlled boundaries: assets/models/camera/script/CPU/character initialization
primitives. Real graph animation/rendering and complete interpreter state are
not accepted here. Existing no-access fixture also passes73 cases with zero32KiB
buffer allocations/frees and unchanged128 track attachments/eight interpolation
cleanup calls at `A:/FreakMelee-test-archives/fighter-animation-scratch-owner-final`.

Focused console launches use CREATE_NO_WINDOW and fresh A: output, with input/
body hashes and commands. Root owns global build and hidden/muted live game/census/
rollback checks. All-scene capacity64 remains provisional.
