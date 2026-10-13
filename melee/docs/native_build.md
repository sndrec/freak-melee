# Native Windows build

For the current direct Versus test shortcut, see
[native_match_launch.md](native_match_launch.md).

The native build operates on the full existing gameplay and HSD source trees.
The first target, `melee_game`, compiles these sources to Windows x86-64 objects.
It is not a playable executable: runtime/Aurora linkage and asset loading are
subsequent porting work. Compilation errors remain visible rather than excluding
the affected game code or substituting stubs.

The build extracts the two embedded font atlases from the supplied ISO, checking
the executable against the reference SHA-1 in `config/GALE01/config.yml` first.
Set `MELEE_ISO` to override the sibling ISO path. Generated headers and the
extracted executable stay under the ignored build directory. Font dimensions
and checksums are recorded in `assets/extraction.json`; no font data is committed.

Use LLVM Clang targeting Windows/MSVC, CMake, Ninja, and an installed Visual
Studio C++ toolchain/Windows SDK. A MinGW `clang` earlier on PATH is not the same
toolchain. Pass explicit compiler paths, for example in this workspace:

```powershell
cmake -S melee -B melee/build/native -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_C_COMPILER=C:/Users/loser/scoop/apps/llvm/current/bin/clang.exe -DCMAKE_CXX_COMPILER=C:/Users/loser/scoop/apps/llvm/current/bin/clang++.exe
cmake --build melee/build/native --parallel 4
```

For the initial full portability audit, append `-- -k 0` to the build command
and redirect output to a file in `build/native/`. Ninja then reports independent
failures across the source set. A nonzero exit is a failed compile audit, not a
successful game build. `game-sources.txt` records coverage and
`compile_commands.json` records the actual native commands.

The original `configure.py` remains the GameCube reference build. Its bundled
MSL/PowerPC runtime is not the native C runtime. Native source currently uses the
original SDK declarations while platform implementation and Aurora ABI coverage
are audited; this is not evidence that those APIs have been linked or validated.

Progress is committed locally on `native-windows`. Do not push upstream.

## First compile audit (2026-09-10)

- Windows/MSVC Clang 22.1.6 configured and passed CMake compile/link checks.
- The initial full pass compiled 878 of 984 game/HSD translation units.
- Native CRT headers and reference-only console layout assertions raised this to
  915 of 984. The generated font headers then allowed two more files to compile.
- Remaining failures include inconsistent callback signatures, several conflicting
  declarations, and MSL-specific stdout interception. Pointer truncation warnings
  also require repair even where compilation succeeds.
- Original main.dol verified as SHA-1
  `08e0bf20134dfcb260699671004527b2d6bb1a45`. Extracted 128 debug glyphs and
  287 SIS glyphs; both font-dependent native objects compiled.

Extractor validation tests (synthetic disc data, no game assets):

```powershell
python -m unittest discover -s melee/tools/tests -p test_prepare_native_assets.py -v
```

This is partial M0 progress. No game executable is linked and no game behavior
has been validated on the native path.

The next full pass compiled 982/984 files after callback repairs. Stage demo
callbacks take integer events (Final Destination explicitly checks event 26),
so their declarations and definitions now match the existing integer callback
table. Card operations and the Final Destination state helper retain integer
status results rather than converting them to booleans. Actual predicates use
boolean signatures consistently. Remaining compilation failures are the AXFX
allocation hook's size type and console-specific stdout interception.

Both remaining blockers are now repaired: AXFX allocation callbacks use `size_t`
consistently, and native reporting writes directly to the host CRT while retaining
the game's diagnostic-console observer. Oversized reports remain complete in the
host log; the bounded allocation-free console preview explicitly marks truncation.
The original GameCube stream hook remains specific to its reference build.

All 985 game/HSD translation units (the original 984 plus native reporting) now
compile. `native_report_test` links and passes CTest, checking formatting, observer
detach, and complete host output for an oversized message. This does not link the
game itself. Pointer-width warnings and unresolved runtime/platform functions are
the next audit boundaries.

```powershell
cmake --build melee/build/native --parallel 4
ctest --test-dir melee/build/native -R '^native_(report|objalloc|heap|panic|dat)$' --output-on-failure
```

## Aurora platform build

Aurora is included from the adjacent checkout, configurable with
`MELEE_AURORA_SOURCE`. Build its libraries explicitly:

```powershell
cmake --build melee/build/native --target melee_platform --parallel 4
```

The Windows platform build passed on 2026-09-10, including core, GX, OS, matrix,
video, controller, disc, card, and THP libraries. Aurora resolved its configured
prebuilt Dawn, SDL3, and nod packages and built the remaining required libraries.
This does not yet link Aurora to the game or exercise graphics at runtime.

Native client tests use `MELEE_BUILD_TESTS` (default ON), independently of Aurora's
third-party `BUILD_TESTING` cache setting. The CTest name filter above selects our
built test; Aurora also registers its own tests, whose executables are not part
of `melee_platform`. See [the platform audit](native_platform_audit.md) for the
remaining integration boundaries and a concrete pointer-width ABI mismatch.

## Native object pools

HSD object-pool base/current addresses and arena-alignment masks now retain the
full native pointer width. Pool slots reserve and align enough space for their
free-list pointer, including original four-byte payload requests. Heap capacity
and object counts remain bounded 32-bit quantities; these are runtime allocator
changes, not changes to serialized DAT fields.

`native_objalloc_test` exercises the actual fixed-pool implementation with storage
above 4 GiB: small payloads, explicit alignment, slot reuse, limits, clipped
reservations, exhaustion, overflow rejection, and surrounding memory sentinels.
The test fails if an OS-heap dependency is reached; it does not validate the
separate OS-heap allocation path or claim that gameplay is running. Native arena
rounding is checked at the same high address.

HSD object-ID keys also retain native pointer width, from joint/animation
descriptors through the hash table and the joint's stored ID. Skinning, reference
objects, metal fighters, stage material registration, and Kirby's joint setup
pass the full address. The pool test now also executes production ID-table code
with equal low-32-bit keys, hash-chain collisions, replacement, separate tables,
removal/reuse, and scene reset. Descriptor fields here describe native in-memory
data; original DAT records still need typed conversion before they can be used.

## HSD/Aurora heap integration

`native_heap_test` links HSD's production memory, object-pool, reporting, and panic
code to Aurora's actual OS/platform libraries. It supplies an aligned arena above
4 GiB and validates allocation/free/coalescing, explicit HSD heap selection,
OS-backed object pools, and heap destruction/recreation. No substitute heap
functions are linked into this test. Heap selection now lives with HSD memory
management instead of depending on the entire graphics initialization module.

Native panic callbacks receive source file/line/message instead of a PowerPC CPU
context. The host path flushes diagnostics and aborts; `native_panic` invokes a
real assertion in a child process and checks its nonzero exit and output. The
native debug initialization uses host floating-point exception clearing and
registers the native diagnostic callback instead of console exception handlers.
Unhandled Windows exception reporting is still provided by the host/runtime;
this does not implement an interactive native crash debugger.

All four native tests and the full game/HSD object rebuild passed on 2026-09-10.
This heap test does not execute complete game scene initialization or asset
loading. Runtime DLLs needed by the Aurora-linked executable are copied next to
it by Aurora's build helper.

The heap integration test also runs the production class allocator and class
lifecycle. With a deliberately nonzero arena, growing its size-class table
reproduced a segmentation fault: the original resize cleared only four bytes
per pointer. Clearing `sizeof(*new_list)` bytes per entry fixes that failure.
The test now passes two table expansions while preserving live payloads, memory
reuse, inherited allocation/destruction, failed-constructor cleanup, instance
counts, and class-library reset. All four tests and the object build pass after
the fix. This is class/heap execution, not a booted game scene.

## Original DAT reader

`src/sysdolphin/baselib/dat.c` provides the immutable on-disc reader for typed
native conversion. It reads big-endian metadata/scalars, exposes bounded source
byte ranges and named-root offsets, and validates relocation slots and external
reference chains without rewriting the source buffer or allocating runtime
objects. Relocation target zero remains a valid reference to the start of data.

`native_dat` tests byte order, unchanged input, roots, external symbols, unsorted
relocations, and invalid/truncated records. The same executable can inspect local
DAT files passed as arguments. Eleven original ISO samples passed: EfFxData,
GrCn, GrFz, GrMc, GrNBa, GrPs, IfAll, MnSlChr, PlCo, PlMr, and PlMrNr. This includes
actual external chains (GrPs has 75 external symbols). Local inputs and results
are in `build/native/dat-samples/` and `build/native/dat-sample-results.log`;
original game data is not committed.

The reader is compiled with the full source tree, now 986 translation units.
It does not yet replace `HSD_ArchiveParse`: typed descriptor conversion and
shared-object relocation must be implemented before the game can consume its
results. Passing archive structure checks does not validate decoded visuals,
animations, or gameplay data.

### Typed texture records

`dat_texture.c` now converts image, palette, and LOD records into the existing
native HSD descriptor types. Texture and palette bytes remain in the original
owned buffer. The conversion checks tiled image/mipmap extents, including the
upper mip level needed by fractional LOD. Unsupported descriptors and unresolved
external payloads return errors rather than producing a partial native object.

DAT reference resolution distinguishes NULL, local offsets (including zero),
external-chain slots, and invalid fields. It currently scans the source tables
during conversion; a graph importer can build an index when importing larger
graphs. No lookups or decoding belong in the per-frame simulation path.

The native DAT test covers descriptor fields, byte preservation, payload bounds,
unknown formats, and fractional mip extents. Original-data probes converted 71
named palette descriptors across the eleven samples and all 60 image descriptors
reached through Mario's costume joint/material graph. Results are in
`build/native/dat-texture-results.log` and `build/native/mario-texture-results.log`.
These conversions have not yet been submitted to the renderer. The source build
now covers 987 translation units and all five native tests pass.

### Material graphs

`dat_import.c` owns a caller-supplied arena, a fixed-capacity identity hash table,
and an iterative decoding queue. It preserves shared typed records and returns
the same native pointer for a repeated root. Arena/record exhaustion fails the
context without returning a partial root. There are no heap allocations inside
the importer; callers size and own its storage at loading transitions.

`dat_material.c` assembles material and texture chains, colors, pixel-engine
settings, TEV settings, images, palettes, LOD records, and optional class names.
External references still require linking support, and non-NULL optional
`renderdesc` records lack a decoder; both fail explicitly instead of being dropped.
The source DAT buffer must outlive native graph use because raw pixel/palette
data and class-name strings retain references into it.

Synthetic graph tests validate shared pointer identity, field values, repeated
imports, unchanged source bytes, and capacity failures. All 59 material graphs
reached through Mario's original costume also import successfully; evidence is
in `build/native/mario-material-results.log`. These descriptors have not yet been
passed through `HSD_MObjLoadDesc` or rendered. Joint/geometry/animation import and
the game archive-loader connection remain open. There are now 989 game/HSD
translation units, with all five native tests passing.

### Material animation and track execution

The graph API is now `HSD_DatImportGraph`, with an explicit root type. Material
animation joints, material animations, texture animations, AObj/FObj descriptors,
and counted image/palette tables import through the same identity table and work
queue. Variable-length tables carry their counts; inconsistent repeated views
fail explicitly. Render-animation records and AObj references to joint descriptors
still require further decoders. Compressed FObj streams remain byte-for-byte
original; their scalar encoding differs from the big-endian DAT metadata.

Mario's original costume material-animation root imports as 150 native records
(`build/native/mario-matanim-results.log`). Synthetic coverage checks shared AObj
and image references, counts, untouched streams, and truncated-track rejection.

The Aurora-linked heap test imports a DAT AObj/FObj graph and executes it with
the original FObj loader/interpreter. Float and signed-16-bit linear tracks,
including negative values, produce the expected five frame samples and restart
identically without additional heap allocation during playback. The byte decoder
now assembles signed values without undefined signed left shifts. The test uses
the game's disabled floating-point-contraction setting. This verifies those
tracks, not every interpolation mode or a rendered character animation.

All five tests and the full 990-source object build pass. Game startup, skeletal
geometry/animation graph import, external linking, and rendered acceptance remain
unfinished.

### Skeleton and geometry descriptors

`dat_model.c` imports joints, display objects, rigid/envelope polygons, vertex
descriptors, matrices, skinning tables, joint/limit/IK reference objects, and
joint-animation descriptors. Sentinel-terminated tables become native arrays;
the import identity table preserves skeleton links and skinning back-references
to the same joint, including a joint at source offset zero. Matrices and weights
become native floats. Display-list and vertex payload bytes remain original.

Mario's original costume joint root imports as 720 native records (evidence in
`build/native/mario-model-results.log`). Synthetic coverage checks transforms,
matrices, reference-object identity, geometry fields, skinning weights, table
terminators, and source immutability. Unsupported morph-target polygons fail
instead of returning a graph with missing geometry. Spline/particle joint data,
expressions, morph targets, and external references still need decoding/linking.
Full indexed-vertex range validation and renderer submission are also pending;
this result does not prove a correct rendered Mario or a booted game.

The full 991-source object build and all five native tests pass, and Mario's
material-animation graph continues to import.

### Original HSD model runtime

The native build now archives the same full game/HSD objects into `melee_runtime`.
`native_model_runtime_test` links that archive with Aurora; it does not substitute
model-loading functions or provide dummy platform implementations. HSD object-pool
initialization lives in `object.c`, render-pass state in `renderpass.c`, and video
state storage in `video_state.c`, separating them from hardware startup. HSD math
call sites directly use Aurora's implemented C matrix/vector functions. Existing
PS vector normalization/magnitude implementations remain in use. Native camera
setup uses a progressive viewport without console field jitter.

The synthetic lifecycle test checks joint identity and a translated child matrix.
The original Mario costume also passes actual HSD object creation, matrix setup,
skinning-reference checks, release, and heap integrity checks: 61 joints, 68
polygons, and 379 skinning weights, from 720 imported descriptor records. Run:

```powershell
./build/native/native_model_runtime_test.exe build/native/dat-samples/PlMrNr.dat 0x19400
ctest --test-dir build/native -R '^native_' --output-on-failure
```

Evidence is in `build/native/mario-model-runtime-results.log`. All six native
tests pass, and the full 993-source archive builds. Unfiltered CTest also discovers
Aurora tests that have not been built; those report Not Run, so use the native
filter for this project's checks. No character has been rendered yet. Native GX
vertex submission and array ABI integration, animation coverage, game startup,
and gameplay fidelity validation remain open. Finite matrices and resolved
references do not establish numerical equivalence with the original game.

### GX vertex submission

Native vertex descriptors now retain a byte extent and byte-order flag. A graph
finalization pass walks polygon display lists in GX attribute order, derives
indexed-buffer extents, and checks accesses against the DAT data section before
renderer use. Shared vertex descriptors accumulate the largest required extent.
Original DAT vertex bytes remain big-endian. The decoder accepts HSD VAT-0
primitive streams and NOP padding; register writes, nested display lists, and
unsupported commands fail explicitly. This does not establish boundaries between
separate allocations inside a DAT data section.

HSD vertex-array calls now supply Aurora's actual five-argument ABI. Native GX
vertex functions call Aurora instead of writing to address 0xCC008000, and the
remaining raw FIFO stores in game/HSD code now use scalar GX parameter calls.
The model runtime test records real Aurora commands and checks the full native
pointer, array length, little-endian flag, stride, and big-endian FIFO float bytes.
Synthetic imports check exact array size, oversized indices, truncated primitives,
and unexpected commands. Mario's 68 polygons pass validation and the existing
runtime lifecycle test still passes. All six native tests and the 994-source
archive build pass. Rendering, texture object ABI, frame presentation, and full
game startup still require integration; command recording is not a rendered frame.

### Native texture object storage

The native SDK declarations now reserve Aurora's required 64-byte texture objects
and 40-byte palette objects with eight-byte alignment. The original 32/12-byte
storage was insufficient for Aurora's native pointers and metadata. Runtime tests
check canaries around initialized objects, a full-width user-data pointer, and
actual palette/texture command recording. Mario's original HSD texture setup also
records successfully for all 58 textured materials. All six native tests and the
full archive build pass; this is still command recording, not rendered acceptance.

Before repeated rendering, HSD's temporary texture/palette objects need persistent
native ownership: Aurora assigns a new resource ID on each initialization, while
HSD currently initializes stack objects during every setup. Proper reuse and
release must be implemented rather than accumulating fresh IDs each frame. Frame
presentation, additional graphics ABI checks, and game startup remain open.

### Persistent HSD texture ownership

Native `HSD_TObj` instances now own their Aurora texture and palette objects.
Repeated setup reuses unchanged resources. A new image pointer with the same
layout updates texture data without changing identity; dimensions, format, or
mipmap-layout changes destroy and replace the texture. Palette pointer changes
update palette data, while format/entry-count changes replace the palette.
Sampler and LOD settings remain updated during setup. Resources are embedded in
the existing HSD allocation rather than allocated during each draw. TObj release
emits destruction commands for its owned resources.

The regular synthetic model test exercises indexed texture/palette setup, three
repeat setups, changed image pointers, changed image dimensions, and final
release. Recorded Aurora metadata proves identity reuse/replacement. Mario's 58
textured materials preserve their native objects across repeat setup and release
exactly 60 texture resources and two palettes. All six native tests and the full
994-source archive build pass. These checks still record commands without GPU
execution. Renderer consumption/draining at asset-unload boundaries, in-place
pixel-data invalidation, frame presentation, and rendered acceptance remain open.

### First GPU-rendered model

`native_model_view` now initializes Aurora and GX, imports an original model DAT,
and draws it using the original HSD camera, lighting, material, and joint/polygon
routines. It uses the full existing `melee_runtime` archive. Run:

```powershell
cmake --build build/native --target native_model_view
./build/native/native_model_view.exe build/native/dat-samples/PlMrNr.dat 0x19400 300
```

The last argument limits presented frames; closing the window exits early. The
viewer drains the GX FIFO before model release and after resource-destruction
commands, then shuts down Aurora and releases its HSD heap. Camera and light
objects are released through HSD's own lifetime routines. This executable is a
rendering bring-up tool, not the game's startup or versus client.

D3D12 on the local RTX 5060 Ti rendered textured, lit Mario in his bind pose.
The inspected capture is `build/native/native-model-view.png`; final 120-frame
startup/shutdown evidence is `build/native/native-view-smoke.log`. A longer run
also closed normally after 5,941 frames. Initial black output was traced to
missing GX default-state initialization in the viewer. Aurora reports device
destruction during shutdown; some earlier runs also reported canceled readback
mappings while closing. The final 120-frame run exited zero with only the device
destruction message at shutdown. All six native tests still pass.

This does not establish animated-model, match, or Dolphin visual parity. The
viewer uses diagnostic lighting and a fixed camera. Skeletal animation playback,
full gameplay asset loading, original game startup, and the remaining project
milestones remain unfinished.

### Original FigaTree animation playback

`lbAnim_DecodeFigaTree` converts fighter clip headers and flat track tables into
caller-owned native storage, retaining immutable node-count bytes and compressed
streams in the DAT buffer. Bounds, track capacity, signed node counts, and stream
extents are checked. `extract_native_animation.py` locates the requested original
AJ archive on disc and extracts a named, 32-byte-aligned member DAT:

```powershell
python tools/extract_native_animation.py --iso '../Super Smash Bros. Melee (USA) (v1.02).iso' --archive PlMrAJ.dat --symbol PlyMario5K_Share_ACTION_Wait1_figatree --output build/native/dat-samples/MarioWait.dat
./build/native/native_model_view.exe build/native/dat-samples/PlMrNr.dat 0x19400 1800 build/native/dat-samples/MarioWait.dat 0xe68
```

Mario's idle clip has 61 nodes, 111 tracks and 50 frames. It binds to his complete
61-joint costume skeleton and plays through the original `lbAnim_8001E6D8`, AObj,
FObj and JObj routines. The viewer loops the clip and paces animation steps at
60 Hz. The 1,800-frame run exits successfully; an inspected animated pose is in
`build/native/native-animation-view.png`, and runtime evidence is in
`build/native/native-animation-run.log`. The generic latest viewer capture now
also shows the animated model. Aurora still reports canceled readback/device
teardown messages on some shutdowns.

The regular native model test includes a synthetic FigaTree: exact five-frame
translation samples, identical restart, no additional playback heap allocation,
source immutability, capacity exhaustion, bad node counts, and truncated streams.
All six native tests pass and the full archive now covers 995 game/HSD sources.
The viewer requires a complete non-instanced skeleton with matching node count;
it does not implement fighter part-selection masks, blending, gameplay animation
events, or fighter state transitions. This is original-track playback, not yet
full fighter-animation fidelity or versus gameplay acceptance.

### Original game-object scheduling

The viewer now creates original GObjs for its model and camera. Model updates
run through `HSD_GObj_80390CFC`, rendering traverses the original GX-link lists
and `HSD_GObj_JObjCallback`, and GObj destruction releases the attached HSD
objects. It therefore exercises the scheduler and ownership path that the game
will use, while remaining a diagnostic scene rather than fighter gameplay.

Native regression coverage checks callback priority/order, current callback
context, pausing link 63, deferred self-destruction with exactly one user-data
release, deferred callback removal, empty lists after cleanup, and no additional
heap allocation during those frames. The pause-mask shift now uses an unsigned
64-bit value. Linking the GObj dispatch table also exposed missing SDK fog-table
initialization: HSD now computes that table directly with the original GX
projection/range formula, then submits it to Aurora. Fog visuals remain untested.

All six native tests pass. The GObj-driven viewer completed 1,800 animated frames
and exited zero; `build/native/native-gobj-view.png` was visually inspected, and
`build/native/native-gobj-view-run.log` records the run and the known Aurora
shutdown readback/device messages. Full fighter creation, stage loading,
collision/gameplay callbacks, original scene startup, and all online features
remain unfinished.

### Native stage collision descriptors

`mpDecodeCollision` converts an immutable DAT `coll_data` root into the existing
`MapCollData`, `MapLine`, `MapJoint`, and vertex structures. The caller provides
bounded writable storage; the original loader prunes empty lines and changes
their connections, so these tables cannot retain pointers into the source DAT.
The decoder preserves big-endian float values, signed connections, flags,
category ranges, joint bounds, and vertex ranges without runtime allocation.
It validates table extents, finite coordinates/bounds, indices, and ranges before
publishing the output descriptor. Partially written storage after failure must
be discarded. Empty collision maps are currently rejected.

`native_collision_test` covers a known floor fixture, independent writable
storage, capacity failure, invalid endpoints/connections/ranges, nonfinite
coordinates, and truncated joint data. An additional run on original archives
decoded GrNBa (26 vertices / 23 lines / 1 joint), GrCn (72 / 65 / 8), GrFz
(177 / 108 / 51), GrMc (68 / 54 / 9), and GrPs (155 / 136 / 8), including
dynamic line ranges in the latter two. These assets remain local and ignored.
Evidence is in `build/native/collision-stage-results.log`.

All seven native tests pass; the full archive now covers 996 game/HSD sources,
and the native model viewer relinks successfully. This checkpoint converts stage
collision data only. Original collision-system initialization, collision queries,
moving-stage callbacks, and fighter interaction with the stage remain untested
and are not yet connected to the viewer or a playable scene.

### Original collision initialization and floor queries

The collision test now links `melee_runtime` and uses the actual Aurora OS heap,
`mpLibLoad`, island construction, bounding checks, floor sweeps, and endpoint
queries. Five original stage archives initialize from decoded collision data;
their unscaled runtime vertices match the imported coordinates and the heap
remains valid. A two-floor fixture verifies nearest-hit selection at nonzero
line index, skipping that floor, upward/out-of-bounds misses, both endpoints,
and no heap allocation during queries. All seven native tests pass.

Native fixes replace the floor sweep's original eight-byte record arithmetic
and eight endpoint traversals' truncated pointer arithmetic with typed indexing.
Island segment allocations now use their actual native structure size. Moving
joint matrix calls use Aurora's `C_MTXMultVec`; moving-joint behavior is not yet
tested. Island visitation uses `memset` directly.

To link this subsystem without unrelated startup dependencies, the original
debug drawing routines moved unchanged to `mpdraw.c` (shared collision state is
declared in `mplib_internal.h`). Stage state and its scale accessor moved to
`stage_state.c`, the debug level to `db_state.c`, and the collision frame counter
to `mplib.c`, where its update lives. Existing callers and function identities
remain intact; there are no substitute collision routines or SDK stubs.
The archive covers 999 game/HSD sources, and the model viewer relinks.

`build/native/collision-runtime-stage-results.log` records the stage run.
The harness retires its whole test heap after its last query; it does not prove
production stage unload/reload. Original fighter ECB collision, dynamic stage
callbacks, scene startup, and a playable versus match remain unfinished.

### Moving collision joint integration

`native_model_runtime_test` now also binds a collision joint to an actual HSD
child joint using `mpLib_800552B0`, changes its transform through HSD setters,
and runs the original `mpLib_80055E9C` update. The fixture verifies translation,
nonuniform scale through Aurora matrix math, preservation of previous vertex
positions, floor hits on the transformed geometry, and hiding/restoring the
joint's collision. Updates and queries do not allocate additional heap memory.

The out-of-line `HSD_JObjSetMtxDirty` definition previously lived inside the
fighter entry-state file `ft_0C31.c`. It now lives in `jobj.c`, unchanged, so
ordinary HSD transform setters do not pull fighter startup into their link path.
All seven native tests pass, the viewer relinks, and the expanded runtime test
also passes with the original Mario model (61 joints, 68 polygons, 379 skinning
weights). Evidence: `build/native/moving-collision-model-results.log`.

This is a synthetic moving-platform integration check, not acceptance of a
complete animated stage. Stage scripts, fighter ECB response, moving-platform
passenger motion, and gameplay parity against Dolphin remain unverified.

### Original ECB landing path

The native collision test now runs `mpColl_800471F8` with the original fixed ECB
source and native platform data. It verifies landing on the higher floor,
Melee's 0.0001 contact offset, floor flags/normal, repeatable landing, a fast
fall requiring original movement subdivision, and missing beyond platform bounds.
No collision-step heap allocations occur. This exercises collision response,
not just a line query; it does not yet instantiate a fighter or drive its states.

The collision path retains its original fighter/item diagnostics. Their small
accessor implementations now live in `ftidentity.c` and `itidentity.c`, while
the existing joint-position and memory-clear functions moved to
`lbjoint_position.c` and `lbclear.c`. These direct moves avoid pulling unrelated
startup code into utility callers. Joint-position matrix multiplication and
the vector library's cross-product/projection calls now use Aurora C math APIs.
Projection behavior has not received new visual verification.

All seven native tests pass; the archive covers 1,003 game/HSD sources and both
the model viewer and model runtime test relink. Five original stage archives
still initialize in the expanded collision test. Evidence is recorded in
`build/native/ecb-stage-results.log`. Joint-derived fighter ECBs, wall/ceiling
response, full fighter states, and playable scene startup remain unfinished.

### Skeleton-derived ECB integration

The model runtime test now creates an HSD skeleton with six collision markers,
binds it through `mpColl_SetECBSource_JObj`, and evaluates the original
`mpColl_LoadECB_JObj` path. Checks cover world-to-character coordinate conversion,
collision-box changes after a joint pose changes, and landing through
`mpColl_800471F8` on the transformed platform from the moving-collision fixture.
The original floor flags and contact offset are preserved, with no heap
allocation during joint-derived ECB evaluation and landing.

All seven native tests pass. The same expanded runtime test also passes while
loading/releasing the original Mario model; evidence is in
`build/native/skeleton-ecb-results.log`. The marker skeleton is synthetic and
does not establish Mario's actual collision-marker mapping or fighter-state
behavior. Complete fighter asset initialization and state callbacks remain next.

### Fighter collision parameters and initialization

`ftData_DecodeCollision` now decodes the original 28-byte fighter `x44` record
into `ftData_x44_t`, preserving six signed part indices, ECB midpoint offset,
and ledge-snap dimensions. It rejects truncated records, negative part indices,
and nonfinite floats, publishing the output only on success. Part-count and
missing-slot validation belongs to the fighter part-table binding step.

The skeleton test now uses a minimal `Fighter` data fixture and the original
`ft_80081B38` collision initializer, checking decoded marker binding, weight,
and ledge parameters before pose/landing evaluation. That existing initializer
moved unchanged into `ftcollision_init.c` to separate it from motion-state
callbacks. This is not `Fighter_Create` or a complete fighter lifecycle.

The local Mario archive decodes collision part indices 24,31,8,55,49,4, midpoint
offset 0, and ledge values 12,15,10. These are fighter part-table indices; they
must not be assumed to equal a raw skeleton traversal when slots are omitted.
Evidence: `build/native/fighter-collision-data-results.log`. All seven native
tests pass and the viewer relinks; archive coverage is 1,005 game/HSD sources.
Actual fighter part-table import and full asset initialization remain unfinished.

### Common fighter part tables

The native fighter decoder now imports both directions of `FighterPartsTable`
and optional four-byte omitted-slot records from PlCo.dat. Mapping bytes are
copied to caller-owned storage; counts, source extents, mapped indices and
omission-slot bounds are checked. The common semantic map has 54 entries,
including its unnamed final entry; `0xFF` remains the missing-part marker.
Omission counts are bounded by the original 32-bit mask, and its bit shift now
uses an unsigned operand in `ftParts_8007506C`.

All 33 original fighter tables and their optional omission records decode in
`build/native/parts-data-results.log`. Mario has 61 slots with no omissions;
several other fighters have omitted slots, so raw skeleton traversal is not a
general replacement for original part setup. Regression checks cover mappings,
omission record bytes, independent storage, capacity and index errors. All seven
native tests pass. Binding these decoded tables through `ftParts_SetupParts`
and completing character initialization remain unfinished.

### Original fighter part lookups

The original `ftParts_GetBoneIndex`, `ftPartsRemap`, and omitted-slot lookup
now link independently in `ftparts_table.c`, alongside their original table
globals moved from fighter startup. Call sites and implementations are retained.
The native fixture binds decoded tables to these globals and verifies semantic
bone lookup, cross-table remapping, missing/out-of-range slots, omitted slots,
and the highest 32-bit omission mask bit. All seven native tests pass.

This does not yet call `ftParts_SetupParts`: that routine installs the fighter
material class while binding draw objects, so full setup also requires the
original fighter-material runtime. No alternate model binding routine or
substitute material was introduced. Archive coverage is 1,006 sources.

### Fighter material portability fixes

Fighter material code previously cast `&ftMObj` to a larger structure to read
shader templates past the end of that object. The original binary placed those
templates in adjacent globals; that is not a native C layout contract. The
code now copies the existing named TEV/constant templates directly and removes
the fictitious extended class layout.

Light-overlay register selection now retains the integer result of
`lbGetFreeColorRegister`, including its -1 failure sentinel, instead of converting
it through `bool`. The material setup callback now has the two-argument signature
required by `HSD_MObjSetupFunc`, removing the unused third parameter and cast.

The full native archive and diagnostic executables build, and all seven existing
tests pass. These are source-backed portability corrections; the tests do not
yet execute or visually validate the fighter material overlay path. Full fighter
part setup and material rendering acceptance remain outstanding.

### Material utility runtime checks

Original shader-register allocation and register-to-GX mappings now live together
in `lbtev_register.c`, separated from unrelated joint utilities. Runtime checks
reserve registers 0 and 1 and verify allocation returns 2, then reserve all
available registers and verify the -1 failure result. They also check the GX
constant-selector mapping for register 2.

The original color-overlay selector moved to `ftidentity.c` and now compares
its pointer directly to NULL. A pointer with nonzero upper bits and zero lower
32 bits verifies selection no longer truncates native addresses; the test does
not dereference that opaque token. All seven native tests pass, and all diagnostic
executables relink. Archive coverage is 1,007 sources.

These checks execute the material utilities, not `ftMaterial_800BF534` or the
complete fighter material callback. Their runtime integration and visual
acceptance remain outstanding.

### Original fighter light-overlay execution

`ftMaterial_800BF534` now executes in the model runtime test. With registers 0
and 1 occupied, it selects register 2, copies the correct constant template,
references the fighter's light color, and emits GX commands. Disabling the light
overlay returns NULL. The active setup performs no heap allocation. This directly
exercises the earlier template-addressing and register-width fixes.

Original TEV routines/templates moved into `ftmaterial_tev.c`; common fighter
data and replacement-material pointer definitions are separated from startup.
Overlay light operations and their shared light state are separated from scene
hook creation, and the existing light-list loader now lives in `lblight.c`.
These retain original behavior and calls, without substitute effect routines.

All seven native tests pass, including the expanded test with Mario's original
model; evidence is in `build/native/fighter-overlay-results.log`. The viewer
relinks and archive coverage is 1,011 sources. This verifies opaque light-overlay
command generation, not a rendered image or the full `ftMaterial_800BF2B8`
callback. Refraction, material switching, transparent effects, and complete
fighter part setup remain unfinished.

### Refraction texture portability

Refraction initialization now copies its named image descriptor directly instead
of reading past a matrix global through an invented structure. Texture callbacks
have consistent function pointer types and direct calls. The original tiled
texture routines live in `lbrefract_texture.c`, independently of archive loading
and framebuffer capture.

RGB565 reads and writes explicitly preserve GameCube byte order. Native tests
check tile offsets and bytes for RGB565, IA8, and RGBA8, including RGBA8's split
planes. The RGB565 reader retains the existing decomp's quantized return values;
this is not a general full-color RGB565 round-trip test.

All seven native tests pass and diagnostic executables build. Archive coverage
is 1,012 sources. Evidence is in `build/native/refraction-texture-results.log`.
The complete refraction effect still needs native archive initialization and
framebuffer integration; these byte checks do not verify its rendered appearance.

### Full fighter material callback execution

The model runtime test installs the original `ftMObj` class with `hsdChangeClass`
and invokes its actual setup method for each textured model material. Ordinary,
reduced-alpha, custom pixel-state, and color-overlay branches all emit GX commands
without allocating during setup. Mario's original model passes for all 58
textured materials, then releases all 60 textures and two palettes.

Refraction's original material draw routine and shader constants now live in
`lbrefract_material.c`, with its shared state declared in
`lbrefract_internal.h`. This removes archive startup from the material callback's
link dependencies; the refraction branch retains the original implementation.
Archive coverage is 1,013 sources.

Evidence: `build/native/full-material-results.log`; all seven native tests pass.
These checks do not enable refraction, metal/replacement materials, or lighting
that needs a live camera. They verify class installation and full callback
execution for the listed branches, not rendered appearance. Full fighter part
setup and native refraction initialization remain outstanding.

### Original fighter part setup integration

`ftParts_SetupParts` and its per-joint binding routine now live together in
`ftparts_bind.c`. The unchanged original traversal builds the fighter's joint
slots and draw-object list and installs `ftMObj` on each material. It no longer
pulls unrelated fighter polygon and interpolation class initialization into the
native part-binding link path.

The model runtime fixture executes that original setup before its material
checks. With Mario's `PlCo.dat` mapping, it binds 61 joints and 59 draw objects,
checks tree depths, draw ranges, rendering flags and material methods, and then
executes the four material variants on all 58 textured materials. Binding and
material setup allocate no heap memory. All texture/palette resources still
release successfully.

Run the original-asset integration check with:

```powershell
build/native/native_model_runtime_test.exe build/native/dat-samples/PlMrNr.dat 0x19400 build/native/dat-samples/PlCo.dat
```

Evidence: `build/native/fighter-parts-results.log`. All seven native tests pass;
archive coverage is 1,014 sources. This is original model/part/material runtime
integration, not `Fighter_Create`, combat simulation, or rendered fighter effect
acceptance. Other fighters with omitted model slots still need full-model checks.

### Bound fighter collision integration

The original-asset model test can now also accept `PlMr.dat` after `PlCo.dat`.
It decodes Mario's x44 collision parameters, connects them to the part slots
created by `ftParts_SetupParts`, and calls original `ft_80081B38` and
`mpColl_LoadECB_JObj` on that same fighter/model fixture.

All six marker references and ledge snap values survive initialization. In the
model's starting pose the local ECB is left -2, right 2, bottom 3.088366, and top
11.981503. Translating the model by (17,23,0) preserves these local extents within
0.0001, and restoring it preserves later material/resource checks. Initialization
and ECB evaluation allocate no heap memory. These values are native integration
evidence, not a Dolphin-verified gameplay pose target.

```powershell
build/native/native_model_runtime_test.exe build/native/dat-samples/PlMrNr.dat 0x19400 build/native/dat-samples/PlCo.dat build/native/dat-samples/PlMr.dat
```

Evidence: `build/native/fighter-ecb-results.log`. All seven native tests pass.
Animated fighter collision, original fighter creation, and combat callbacks
remain outstanding; this fixture supplies only the fields needed by the
integrated initialization path.

### Fighter joint and polygon rendering classes

Original fighter joint/interpolation/polygon class code now lives in
`ftparts_render.c`. Its matrix operations call Aurora's `C_MTXScale` and
`C_MTXConcat` directly instead of PPC entry points. The private scale scratch
state lives in that source file; the old definition-bearing static header is
removed. The interpolation class remains directly accessible to its loader.

The original refraction polygon loader is separated into `lbrefract_pobj.c`.
Its GameCube display-list cache flush is removed: Aurora's `GXCallDisplayList`
copies CPU bytes into the FIFO at submission. Refraction-specific mutation of
imported display lists still requires ownership and bounds work before enabling
that effect on native imported assets.

The model test loads polygons through the original fighter class, installs the
fighter joint class, and executes their matrix methods. Mario's 68 polygons emit
matrix commands in both ordinary and Z-scale branches with finite position
matrices and no heap allocation. This toggles the fighter Z-scale branch while
using the original model's joint scales; it does not establish visual correctness
of a squashed fighter. Resource release and all seven native tests pass.

Evidence: `build/native/fighter-render-results.log`. Archive coverage is 1,016
sources. Full rendering acceptance, refraction-enabled models, and animated
combat remain outstanding.

### Writable skinned display-list ownership

The fighter/refraction polygon loader's 0x2000 condition selects envelope
geometry. Import now copies those display lists into the existing bounded model
arena, where the original loader can rewrite texture matrix indices without
changing the source DAT. Ordinary unmodified payloads continue to reference the
source. The importer uses its existing arena allocator directly; there is no
per-frame allocation or separately owned display-list heap.

The geometry import fixture verifies initial byte equality, arena ownership,
and source immutability after modifying the imported copy. The polygon loader
also rejects truncated primitive headers and vertex counts exceeding its
remaining display-list bytes before rewriting that primitive. Its legacy vertex
stride interpretation still needs coverage for additional attribute layouts.

All seven native tests and the combined original Mario model/parts/collision/
material/matrix check pass. Evidence: `build/native/refraction-owned-results.log`.
This resolves source ownership for the existing skinned rewrite path; complete
refraction initialization and rendered effects remain unfinished.

### Polygon matrix cache corrections

`HSD_PObjSetMtxMark` now writes valid slots 0/1 and rejects negative/out-of-range
indices. Its previous conditional skipped valid writes and could write before
the cache for negative indices. Both HSD and fighter shared-vertex setup now
refresh when either joint identity or matrix mode differs. HSD shared setup
also records the joints it uploads, matching the cache's intended contract.

Runtime checks exercise both paths: a matching mode with a different joint and
a matching joint with a different mode each emit matrix uploads; a subsequent
complete match skips them. Invalid setter indices leave valid entries intact.
The synthetic and original Mario integration checks pass, as do all seven
native tests. Evidence: `build/native/matrix-cache-results.log` and
`build/native/matrix-cache-mario-results.log`. This verifies cache behavior and
command submission, not rendered combat fidelity.

### Rendered fighter-class preview

The diagnostic viewer accepts a final `PlCo.dat` argument to bind its Mario
model through original `ftParts_SetupParts`, install fighter joint/material
classes, and load the fighter polygon class. `native_fighter_preview.c` owns the
single diagnostic fighter's fixed storage. This is a fixture for original engine
integration, not a replacement fighter creation or gameplay implementation.

```powershell
build/native/native_model_view.exe build/native/dat-samples/PlMrNr.dat 0x19400 900 build/native/dat-samples/MarioWait.dat 0xe68 build/native/dat-samples/PlCo.dat
```

The run presented 900 frames, bound 61 animation nodes/111 tracks and 61 parts/59
draw objects, completed resource teardown, and the process exited. The captured
frame visibly contains textured Mario in the idle pose through Aurora. Evidence:
`build/native/fighter-preview.png`, `fighter-preview-out.log`, and
`fighter-preview-err.log`. Aurora logs device destruction at shutdown.

This uses diagnostic camera/lighting and the original 50-frame idle clip. It
has not been compared against Dolphin for exact lighting or animation parity.
Refraction, replacement materials, fighter input, and combat remain unfinished.

### Original interpolation skeleton initialization

`ftAnim_8006FE48` now executes in the native model runtime fixture. Its original
initialization resides in `ftanim_skeleton.c`; the interpolation skeleton loader
and part mapping live with the other bindings in `ftparts_bind.c`. Their behavior
is unchanged, without pulling action-state callbacks into this initialization.

For Mario, the original class loader creates 61 distinct interpolation joints
without draw objects, preserves tree depths and part mappings, and resets blend
timing. Changing the interpolation root leaves the rendered root unchanged.
Both skeleton lifecycles complete and the original model still releases its 60
textures and two palettes. Evidence: `build/native/interpolation-results.log`.
All seven native tests pass. Archive coverage is 1,017 sources.

The fixture runs this after descriptor-ID lookup checks: original HSD loading
reuses descriptor IDs for the newest skeleton. Already-resolved render joint
references remain intact. This does not establish a general multiple-instance
asset lookup contract. Cross-pose blending and full fighter animation callbacks
remain outstanding.

### Original fighter pose blending

Original `ftAnim_8006FE9C` and `ftAnim_8006FF74` now live with interpolation
initialization in `ftanim_skeleton.c`. Their existing joint blend/copy operations
reside in `lbjoint_pose.c`, separated from unrelated utility dependencies.
The native fixture runs these actual functions between its two skeletons.

Checks verify 25/75 translation weights, excluded-part preservation, direct-copy
part behavior, and quaternion interpolation from zero to a 90-degree rotation
at equal weights. Pose copying restores the source rotation format and marks
matrices dirty. The blend/copy phase allocates no heap memory; original Mario
model resources still release correctly.

Evidence: `build/native/pose-blend-results.log`. All seven native tests pass;
archive coverage is 1,018 sources. These are controlled pose checks, not animation
state changes driven by fighter actions or rendered transition acceptance.

### Original fighter animation track dispatch

The same-kind and remapped full-clip binding routines and their dispatcher now
live in `ftanim_tracks.c`. Fighter preview initialization assigns the decoded
clip to the original Fighter fields and invokes `ftAnim_8006FE08`. The generic
model-only preview retains its independent joint binder; the fighter preview
uses the original fighter part-selection path.

The Mario idle run bound 61 nodes and 111 tracks, presented 300 frames, and
exited with code 0. Evidence: `build/native/fighter-track-out.log` and
`fighter-track-err.log`. All seven native tests pass; archive coverage is 1,019
sources. The diagnostic enables looping explicitly and still advances HSD joint
animation through its existing GObj callback. Full fighter action timing,
subtree binding, and cross-character track-remapping acceptance remain open.

### Original fighter animation traversal

Fighter previews now advance through `ftAnim_8006E7B8`, preserving the original
part exclusion and animation callback traversal. The original frame-request and
animation-removal routines now reside alongside track binding in
`ftanim_tracks.c`, and the preview uses the original frame-request entry point.

Runtime checks request frame 2, retain it on the first tick, advance to 2.5 at
half rate on the next tick, and preserve it while the root part is excluded.
Traversal allocates no memory and removal clears the attached animation. The
combined Mario fixture passes. Evidence: `build/native/fighter-tick-results.log`.
A fighter preview run presented 300 frames and exited with code 0; logs are
`fighter-tick-out.log` and `fighter-tick-err.log`. All seven native tests pass.

The complete action animation processor, root-motion adjustments, blend timing,
and action-script callbacks remain unfinished. This integrates the original
pose traversal, not those additional gameplay responsibilities.

### Fighter animation update and root motion

The preview now invokes original `ftAnim_8006E9B4`, which coordinates pose
advancement and current-frame bookkeeping. It and the root-motion extractor
reside in `ftanim_update.c`; joint animation-time queries and model-scale lookup
are separated from unrelated initialization dependencies. The frame query now
returns zero when no eligible animation exists instead of falling off a
non-void function, consistent with the adjacent joint-time query behavior.

Native checks verify current-frame tracking at half rate, disabled animation,
no-animation queries, and a controlled root translation extracted at model scale
2 into fighter displacement fields while resetting the animated translation.
The original Mario integration passes, and the updated preview presented 300
frames and exited with code 0. Evidence: `build/native/fighter-update-results.log`,
`fighter-update-out.log`, and `fighter-update-err.log`. All seven native tests pass.
Archive coverage is 1,021 sources.

The blend-timing branch is linked but not yet exercised by a real clip
transition. Action scripts, secondary root-motion modes, physics consumption of
root displacement, and combat remain unfinished.

### Real-clip blend timing preview

The optional final viewer argument selects a diagnostic blend duration. With
`10`, it evaluates and freezes the outgoing idle pose, creates the original
interpolation skeleton, binds the target idle clip at phase 25, and lets original
`ftAnim_8006E9B4` advance and complete the blend. Removing outgoing tracks before
quaternion blending follows the original transition setup's requirement.

```powershell
build/native/native_model_view.exe build/native/dat-samples/PlMrNr.dat 0x19400 300 build/native/dat-samples/MarioWait.dat 0xe68 build/native/dat-samples/PlCo.dat 10
```

The blend reached exactly 10 frames with target animation frame 34; the run
presented 300 frames and exited with code 0. Both skeletons are released on
shutdown. Evidence: `build/native/fighter-blend-preview-out.log` and
`fighter-blend-preview-err.log`. All seven native tests pass.

This verifies original blend timing with a real clip under Aurora, using explicit
diagnostic setup. The full fighter state-transition initializer and action-script
triggering remain unfinished; rendered transition fidelity is not yet compared
against Dolphin.

### Original fighter transition initialization

The fighter preview now calls `ftAnim_8006EBE8` for initial playback and blended
transitions. The original initializer owns pose reset, outgoing track removal,
target binding, loop flags, playback rate, and blend timing. Its descriptor-pose
reset routines are in `lbjoint_pose.c`; transition functions and descriptor tree
traversal are organized in `ftanim_transition.c` and `ftanim_tree.c`.

The ten-frame real-idle transition again completed at animation frame 34; the
300-frame run exited with code 0. Evidence: `build/native/fighter-transition-out.log`
and `fighter-transition-err.log`. All seven native tests pass. Archive coverage
is 1,023 sources.

The diagnostic supplies unit model scaling and model scale slot 1 through a
minimal ftData fixture. These are explicit preview settings, not decoded fighter
attributes. Full character metadata import and `Fighter_Create`, action-script
triggers, and gameplay transition acceptance remain unfinished.

### Common fighter attributes and preview metadata

`ftData_DecodeCommonAttrs` decodes the pointer-free 0x181-byte common attribute
record into its original native type. Four-byte integer fields preserve their
bit patterns, floating-point fields are checked for finite values, and the final
throw-mask byte is copied without reading trailing padding. Layout assertions
protect the scalar representation. Truncated or nonfinite input leaves the
output unchanged; regression checks cover integer, float, and byte fields.

Mario decodes walk speed 1.1, dash speed 1.5, gravity 0.095, weight 100, model
scale 1.1, and two jumps. The preview now also reads its five model-part metadata
bytes: Mario's scale part is 42. These replace the earlier unit-scale/slot-1
fixture. Remaining visibility and costume metadata are still not imported.

Current fighter-preview invocation (the final character DAT is now required):

```powershell
build/native/native_model_view.exe build/native/dat-samples/PlMrNr.dat 0x19400 300 build/native/dat-samples/MarioWait.dat 0xe68 build/native/dat-samples/PlCo.dat 10 build/native/dat-samples/PlMr.dat
```

Earlier preview commands above record historical runs before this argument was
required. The current ten-frame transition presented 300 frames and exited with
code 0 using decoded metadata. Evidence: `build/native/fighter-attrs-results.log`,
`fighter-attrs-preview-out.log`, and `fighter-attrs-preview-err.log`. All seven
native tests pass. Full fighter creation, physics, and combat remain unfinished.

### Original movement helper execution

Ground acceleration, aerial acceleration/friction, velocity limits, gravity,
and related velocity helpers are organized in `ftmovement.c`, preserving their
original code and direct call sites. This separates velocity calculations from
item/audio/state-transition dependencies in `ftcommon.c`.

Native checks run the actual helpers with synthetic attributes and Mario's
decoded common attributes: gravity reaches terminal speed, fast fall uses its
own speed, positive/negative drift clamp correctly, friction stops at zero,
and stick acceleration respects target speed and the input threshold. Helper
evaluation allocates no heap memory. Evidence: `build/native/movement-results.log`.
All seven native tests pass; archive coverage is 1,024 sources.

This does not integrate a complete physics tick, ground-surface friction,
collision response, or state-specific movement callbacks into a playable
fighter. The test does not substitute its simple velocity checks for those
remaining gameplay responsibilities.

### Ground surface movement integration

Original grounded velocity/acceleration projection and the fighter-specific
friction lookup now live with velocity calculations in `ftmovement.c`. The
friction-free entry point contains its direct calculations instead of a
single-use inline forwarding function.

Native checks exercise the actual stage surface table's 0.1 multiplier, a
(0.6,0.8) floor normal, neutral friction when no floor exists, and Popo/Nana's
original exemption. Ground speed projects to (1.6,-1.2) at speed 2, while the
surface multiplier affects acceleration. Both paths zero their Z components
and allocate no memory. Evidence: `build/native/ground-movement-results.log`.
All seven native tests pass.

The fixture supplies the floor result directly; the complete fighter physics
tick still needs to join moving-platform collision, state callbacks, and these
movement calculations. This is not playable grounded movement yet.

### Native physics position integration

Extracted the original velocity decay and position integration body from
`Fighter_procUpdate` into `ftCommon_IntegrateMovement`; the original tick calls
it at the same point between the state physics callback and wind evaluation.
Native vector addition uses Aurora C_VECAdd. Arithmetic and branch order are
preserved, including the invisible-ceiling shield-knockback quirk. Original
animation movement-gate and joint rotation accessors now reside with their
respective update/rotation code, avoiding unrelated archive dependencies.

Native tests cover acceleration consumption, airborne knockback decay, nudge,
damage velocity interpolation, throw-triggered accumulated movement release,
the original shield-knockback quirk, and no heap allocation. All seven native
tests pass; viewer and model-runtime diagnostics relink successfully. Build
log: `build/native/integrate-build.log`. A source comparison confirms the moved
integration body differs only in native vector calls and explicit clearing.

This does not yet execute the complete Fighter_procUpdate: damage/KO, collision
notifications, sound, wind and callbacks still need full runtime integration.
The unchanged-animation accumulation branch and in-game parity remain unverified.

### Native wind and platform carry

The original ftColl_GetWindOffsetVec now resides with the stage device registry
in ftdevice.c; the original environment eligibility/cooldown functions reside
in ftCo_Environment.c. Fighter_procUpdate still calls the same functions in the
same order. Wind registration now accepts its actual callback type. Castle and
Garden callbacks use Ground_GObj pointers for their unused first parameter,
removing the mismatched void-pointer/integer declarations without casts.

All seven native tests pass. The collision harness exercises original registry
reset/registration, empty output, active/inactive callbacks, wind immunity,
type-2 cooldown and capture/barrel exclusions, and cooldown expiry. It also
executes original mpGetSpeed for static and translating floors, independently
moving endpoints, endpoint clamping, and missing floor. These checks allocate
no heap memory. Both model diagnostics relink. Evidence: build/native/wind-build.log
and build/native/platform-carry-build.log.

The wind callback itself is a controlled test fixture; actual stage wind/water
callbacks are compiled but not run here. Platform endpoints are supplied by the
fixture, so this is evidence for carry calculation, not an animated-stage or
full-fighter-tick acceptance test. Full damage/KO and fighter tick integration
remain outstanding.

### Combined native fighter position phase

Fighter_procUpdate now directly calls Fighter_UpdatePosition for its original
position phase, retaining the disabled-fighter guard before it and the full KO,
crowd and capsule processing after it. The phase contains original cooldown
updates, state physics callback, velocity integration, wind evaluation, ground
push, hitlag callback, platform carry and final wind application in original
order. Native vector calls and explicit vector clearing are the only changes
to the extracted body (verified by source comparison). Collision push and
hurt-capsule invalidation functions reside in ftcollision_update.c.

Native tests execute the combined phase with controlled callbacks, checking
ordering and final position. Hitlag pauses self movement, cooldowns and wind,
but retains ground push, hitlag callback and platform carry. Tests exercise
self movement plus translating-platform carry and hurt-capsule invalidation.
All seven tests pass without per-tick heap allocation; both model diagnostics
relink. Evidence: build/native/physics-phase-build.log.

This is not full Fighter_procUpdate or a playable fighter: KO processing,
crowd/sound effects and full state-machine/collision scheduling still need
integration. Test callbacks and platform endpoints are controlled fixtures.

### Native grounded fighter collision

Original grounded collision routines now reside in ftcollision_ground.c, and
original ECB rescaling resides beside initialization in ftcollision_init.c.
The native harness runs Fighter_UpdatePosition followed by ft_80082708 on its
floor fixture: a one-unit step stays supported, and a twenty-unit step leaves
the floor during subdivision at x=11 (rather than retaining requested x=21).
This exercises the original position synchronization and collision engine.

ft_80082708 previously used GroundOrAir for a support boolean: nonzero means
supported and zero triggers falling in callers. Its return type is now bool;
existing comparisons to GA_Ground (zero) are direct negations. A source check
verifies every modified character call site retains its original branch.
No numeric return behavior is changed.

All seven native tests pass and model diagnostics relink. Evidence:
build/native/ground-phase-build.log. Full Fighter_procMap, wall/ceiling cases,
state transitions after loss of support and in-game parity are not yet proven.

### Original startup is now the integration priority

Added the explicit melee_native build target using the original gmmain.c entry
point and full runtime archive. The target currently fails to link; it is not
a playable client. See docs/native_startup.md for the traced startup path,
remaining 97 unresolved symbols, and the ordered work toward original scene
initialization. The viewer remains a diagnostic, not an alternate client.
Original game exp/pow routines now have distinct symbols, and remaining linked
PPC matrix/vector calls use direct Aurora C implementations. Seven native tests
pass; diagnostics relink. Client link is the next acceptance gate, followed by
actual startup and scene execution, not further isolated preview expansion.

### Original entry now initializes Aurora

The original gmmain.c uses Aurora's main entry and initializes Aurora before
its existing OS/HSD/game sequence. Removed the console USB server startup wait,
per-frame development-server polling, unused PPC/EABI setup and 48 MB console
arena carveout. Replaced two compiler-runtime/data references with native C
operations. All game sources compile and seven native tests pass; the client
still fails to link on 95 unresolved symbols. Native event/presentation/shutdown
integration and actual startup remain pending. See docs/native_startup.md.

### Original scene loop uses Aurora events and frame submission

Adapted gm_801A4D34 directly: it polls window events while waiting for controller
input, retains original simulation/GObj/draw ordering, and submits screen frames
through Aurora with HSD clear/filter/gamma settings. Window-close state unwinds
the scene/mode loops to main and Aurora shutdown. This compiles; the client still
fails to link on 95 unresolved symbols. Seven existing tests pass, but they do
not cover this loop. Alarm-driven input pacing and VI startup remain outstanding;
actual boot, presentation and shutdown are unverified. See docs/native_startup.md.

### Native HSD video startup

Original HSD startup now initializes native video settings without allocating
console XFB/FIFO storage or installing retrace callbacks. Removed obsolete
allocation functions/options and the startup retrace wait; main calls
GXInit(NULL, 0). The native model-runtime test executes HSD_VIInit and checks
settings plus empty console queues/callbacks. Seven tests pass; diagnostics
relink. The client still has 93 unresolved symbols. Legacy movie,
error-screen and screenshot XFB consumers, input pacing and actual boot remain
pending. See docs/native_startup.md; evidence: build/native/video-init-build.log.

### Native startup diagnostics and initial input snapshot

Converted remaining executable console panic calls to HSD_Panic. Replaced
console linker-stack scanning with Windows current-thread stack bounds and
removed console scheduler validation from the frame loop. Initial controller
selection now pumps Aurora events and reads its synchronous pad snapshot,
without retrace/card-hardware waits. Early window close returns through main.
All seven tests pass, including the real native panic path; full client launch
and stack reporting are not runtime-verified. There are 89 unresolved symbols.
Evidence and remaining work: docs/native_startup.md.

### Audio backend boundary established

Audited original HSD synth/driver usage against Aurora: this checkout does not
supply AX voice mixing. docs/native_audio.md identifies original callback,
stream, sample-address and direct voice-state dependencies; native audio needs
real decoding/mixing/output, not empty AX substitutes. HSD's existing sound-mode
state now defaults to stereo and retains setter-driven active-node mix updates,
without console OS sound-mode storage. Persistence and audible playback remain
pending. Seven existing tests pass (not audio coverage); client link has 87
unresolved symbols. Evidence: build/native/audio-audit-build.log.
