# Read-only native object census

`--object-census` records actual native GObj/proc/Fighter/AObj/FObj pool used/peak/free counters and
the remaining HSD ObjAlloc used/peak/free/size/alignment and
reserved counts, live class instance/peak/size counts, linked GObj classifier and
p-link populations and proc-priority populations. It traverses the real current
ObjAlloc registry through the const source accessor and the actual HSD class tree.
It also checks that the named Vec/Mtx/RObj/Rvalue/ID descriptors are present in
the registry. `HSD_CreateMainHeap` now forgets the old registry before resetting
and registering these families. A missing named descriptor reports incomplete
ownership; the census does not supplement, repair or wrap the registry.
No allocator call is wrapped and no game object, RNG, render state, input or
scheduler field is modified.

The diagnostic is explicitly untimed (printed as `timing=UNTIMED`). The CLI
rejects `--rollback-benchmark` and explicit online combinations. Scene entry also
refuses an actual netplay request, including one configured by interactive UI
after CLI parsing. Ordinary offline all-CPU direct matches are allowed only when
this diagnostic or the existing benchmark mode is selected. Normal direct-match
human rules remain unchanged.

Integration boundaries:

* BeginScene follows `preloadState` and precedes the new scene GObj registry init.
  It only resets host diagnostics and opens configured output; it does not inspect
  old registries that preload may already have invalidated.
* Sample after actual scene `on_enter` observes constructed objects. Then one
  sample follows every complete HSD/Aurora draw tick, before rollback EndFrame.
  Replay/probe ticks are tagged separately. This is not a pure simulation timer.
* FinishScene follows scene/state exit callbacks and prints only retained copied
  counts, not dereferenced old records. Close flushes an active scene on shutdown.
* CMake excludes the census unit from `.mstate`; observations and scratch maps do
  not rewind or become part of explicit match state. Errors produce an incomplete
  census and nonzero main exit; data is never presented as a valid capacity proof.

Each sample validates p-link/GX heads/tails, priority and owner proc lists,
per-p-link/priority proc tails and equality with actual GObj/proc allocator used
counts. Fixed hash maps bound traversal to16,384 objects/procs; repeated pointers,
cycles, missing owners, topology/count disagreement, type bounds and invalid class
sizes mark incomplete evidence. There are256 sparse classifier/type entries and
256 priority/p-link counters. Sampling has no allocation, file output or game
callbacks. Fixed host scratch is diagnostic overhead, never accepted benchmark
work. Caller-owned source registries must remain valid at the sampling boundary;
this is not arbitrary-pointer or corrupted-address sandboxing.

Summary `payload_peak` is the maximum sampled sum of ObjAlloc used*size plus
actual class nb_exist*obj_size, with separate components. It is a **partial live
typed payload estimate**. It omits pointed arrays/dynamic satellites, stage buffers,
global state, metadata/free links and immutable assets; it is not complete packed
snapshot bytes. Existing allocator `peak` captures some between-sample churn even
when linked counts fall by frame end. Per-type maxima summed independently are
not a simultaneous full scene size. Per-pool payload peaks and shape-change counts
disclose when a descriptor's size/alignment changes across samples. Anonymous allocator addresses need source
owner mapping before capacity selection, rather than inferring types from size.

Output path comes from `FREAK_MELEE_OBJECT_CENSUS`, set by root to a fresh A:
diagnostic location. Without that variable summaries go to stdout. Output is
flushed once per scene and on close. Only root performs client builds/launches.

Prepare a normal eight-actor CPU VS scene without launching it:

```powershell
python tests/native/native_match_object_census/prepare.py --output A:/FreakMelee-test-archives/eight-actor-object-census-prepare --content-root A:/FreakMelee-test-archives/eight-actor-stock-content
```

The receipt contains exact command, environment, seed and input hash. Root's
execution must redirect output, use CREATE_NO_WINDOW and a bounded timeout;
command flags are always hidden/muted. Root should verify exit0, replay EOF at
the requested sample count, census output with errors0, and actual fighter/pair
counts before interpreting the scene's maxima. Later batches cover CSS/SSS/
results, workshop/practice, scene reuse/sudden death and item/effect-heavy stages.
One Battlefield CPU workload does not establish an all-scenes capacity.

Focused console verification:

```powershell
python tests/native/native_match_object_census/run.py --compiler .local/baseline72/compiler --output A:/FreakMelee-test-archives/native-match-object-census-test
```

The preceding receipt `A:/FreakMelee-test-archives/native-match-object-census-animation-named/receipt.json`
passes13 samples/45 assertions, including10 malformed list/registry cases, full
fixture game-byte preservation and12 flushed scene summaries. Fixtures use actual
native HSD structs and a controlled registry-head boundary. The current fixture
counts actual AObj/FObj owner descriptors directly and checks that a registered Vec
descriptor also appearing in the named list is counted once. Other named
descriptors remain visible after registry loss. The fixture does not allocate
real client objects or prove workload capacities.
