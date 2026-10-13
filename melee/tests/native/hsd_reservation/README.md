# HSD transition reservation

`run.py --compiler <clang.exe> --output A:/FreakMelee-test-archives/<fresh-name>`
compiles the real class, ObjAlloc, memory, ID, matrix/vector, MatchPool and census
units, compiling exact Aurora OS allocator bodies with only diagnostic
`OSDumpHeap` logging omitted. Matrix/vector and RObj allocator init/getter bodies
are extracted without unrelated animation/math methods. It does not build
the game or substitute allocator successes. Native assertions terminate the test.

The matrix covers ordinary heap allocation/reuse, atomic invalid reservation
binding, distinct pinned owners, exact rounded class bins and satellite alignment,
native class initialization/destruction counts, total construction quotas, native
number limits, unsupported heap limits, sticky unknown/direct-heap/pool-growth
failures, teardown outside the claim, duplicate/interior/wrong-type frees, and
generation reuse. A local backing/metadata restore checks resurrection of the
same reserved slot; this does not establish the game's snapshot ownership.

The bounded descriptor census counts JObj/DObj/PObj/MObj/Material/TObj/Tlut/TObjTev
class pieces, ID and explicit matrix satellites, plus potential lazy scale Vec
slots. For default unnamed materials/textures without bump/toon or explicit
shadow, it also budgets transient TEV expressions/constants and compiled
stage descriptors. It caps8 textures and a conservative16 expression/stage bound.
The budgets count total allocation calls, including temporary objects freed during
compilation, rather than pretending those objects persist. Native shader-register
legality is not proved by an allocation budget.

Custom combiners accept the native constant/texture/stored-register operand
vocabulary with active ADD/SUB channels, zero bias, scale1 and clamp enabled.
Unknown active bits, inactive whole combiners, unsupported arithmetic/operands
and excess stage demand stay uncovered. Constants are counted by unique native
source across both operand arrays, even inactive channels; register staging
expressions count each active occurrence. Each actual texture participation
group gets its whole budget, including repeated diffuse/specular/EXT use.

The341-material matrix runs actual MObjMakeTExp, TObjMakeTExp,
HSD_MObjCompileTev, TExp compile, the complete DAG optimizer/scheduler and shader
destruction. It compares native stage configurations and retained constants through
ordinary and reserved allocator paths, then recompiles both live materials and
checks complete reserved cleanup. Claims contain shader bins only, so unused
graph allocation capacity cannot hide an underestimated shader count.
Core material/texture structs and their default
method pointers are controlled setup; this does not exercise HSD graph loading.
Global toon/shadow pointers are explicitly empty in this fixture. Descriptor-only
census always keeps MaterialEnvironment uncovered, since native MObjLoad sets
RENDER_TOON and compile can splice external textures. No environment default is
fabricated in production. Unsupported combiners, invalid modes, excess textures
and stage budget overflow retain TevCompiler uncovered.

Four cases import the actual copied Link/YoungLink fighter DATs under
`A:/FreakMelee-test-archives/fighter-callback-source-census/fighter-{2,3}/`.
The runner verifies their exact SHA-256 identities, then actual DAT relocation
and graph import resolve arrow attribute blocks16200/16676 plus offsets36/40.
These block identities were established by the separate full character import
in `docs/native_source_arrow_descriptor_census.md`; no raw native descriptor is
invented or copied from a primary model. Imported materials/custom combiners
feed the same native shader comparisons. Their demand is5 expression/stage
slots and2/3 constants, with MaterialEnvironment still uncovered. This does not
execute JObj/DObj/PObj/TObj native constructors or model metadata validation.

Final evidence: `A:/FreakMelee-test-archives/hsd-custom-tev-final3/receipt.json`,
11,265 checks /341 material cases /4 actual imported roots. Generic operand
tests cover both ADD/SUB, inactive arrays, duplicate staging operands, repeated
participation and eight malformed shapes. Native compiler register legality for
arbitrary future combinations is still not proved by an allocation count.
Cleared TevCompiler means allocation-path coverage only. Before graph admission,
actual compile legality and an explicit assert/failure policy must be resolved;
no guessed constant-count ceiling establishes packed-register legality.

It also reports incomplete coverage for shape/envelope graphs,
references, instances, particle/spline arms and custom classes. Borrowed image/LOD
descriptors are not counted as cloned allocations. Animation demand is outside
this construction census. No secondary graph load is admitted by this batch.
Native descriptor allocators must already be initialized; future admission must
also validate actual default HSD class selection, rather than only class names.

Caller backing and reservation metadata must be pinned and captured alongside
native ID tables, class counters, ObjAlloc counters and any other native graph
state. The existing broad snapshot owner remains in place; these APIs do not
register arbitrary backing with it. Class and ObjAlloc warning messages are
compared against unchanged HEAD bodies; new units and test compile with strict
warnings. Fingerprints cover allocator/Runtime/Dolphin/Aurora headers and the exact
native pool/census interfaces, excluding unrelated scripting header changes.
Graphics, actor-only snapshots and live gameplay are not tested here.

The draw-input extension checks prerequisites from actual `TObjMakeMtx`,
`HSD_TObjSetup`, `HSD_PObjDisp`/its simple primitive setup and `HSD_DObjDisp`.
Every normally assigned texture requires nonzero repeats and an owned image
pointer in one of the eleven native-supported formats; palette formats also
require a real TLUT descriptor/data pointer. Texture scale zero remains legal.
Visible skin polygons require a bounded terminated vertex-descriptor list,
native attribute/type admission, indexed vertex pointer/stride, and a display
pointer when display count is nonzero. Fully front-and-back culled polygons
retain the native early return and may omit those unused primitive inputs.
Every drawable requires its material, even with an empty polygon list.

Draw admission failure leaves the caller's previous census output unchanged.
The 43 grouped cases cover those failures and native healthy exceptions; they
do not execute renderer methods or upload images. Synthetic shader cases now
supply explicit owned I4 bytes/nonzero repeats; imported Arrow inputs stay
unchanged. The importer remains responsible for real image/palette and display
command/indexed-vertex byte extents. Census uses trusted imported pointers and
does not turn pointer-presence checks into a malformed DAT decoder.

Draw-input evidence:
`A:/FreakMelee-test-archives/hsd-draw-admission-oct09-final/receipt.json`,
11,330 checks /43 draw-input cases /341 material cases /4 imported Arrow roots.
Shader packing, class/environment admission and unsupported graph families
remain separate gates; this extension makes no full render/gameplay claim.
