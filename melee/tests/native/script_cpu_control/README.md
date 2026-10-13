# Generic source CPU control

`cpu.pending_action` is Bool RW; `behavior` is I32 RW; `previous_behavior` I32 R;
`destination.x/y` F32 R. `clear_target()` clears the actual cpu.x44 pointer.
Calls/fields require a live owned Fighter program/storage and its full native
pool epoch/reference. They are source-only and do not expose native pointers.

Scalar CPU fields validate that source at entry. Their bodies only read/write
native fields and cannot invoke an authored callback; they do not repeat a
continuation snapshot after each access. Engine operations in `Invoke` retain
their full before/after lifetime checks, including callbacks which retire or
replace the source. Target-ID publication retains its physical-target guards.

The runner accepts a fresh output directory on any drive. `--commands-only`
runs the controlled command/scalar/selector/VM fixture without the connected
actor/receiver cohort; its receipt explicitly records the narrower scope.

The registry-admission cases use one explicitly controlled no-op planner leaf.
They verify admission before entry and rejection after that leaf retires an item
payload. Null userdata, foreign classes, invalid backlinks, freed pool slots and
cyclic lists must reject before the kernel runs and leave output untouched.
The real admission code walks each registry once; list membership is already
known while visiting a node, so it does not search the same list again per node.
These cases do not claim planner decisions, geometry or CPU gameplay parity.

`cpu.queue(Cpu.recipe[,operand[,operand]])` supports all 51 explicitly audited
emitter recipes from the native CPUCommand enum, with exact zero/one/two operand
arity. Recognized numeric I32 command literals work; unknown/reserved/computed
commands fail loading. Operands retain numeric type until checked I32 conversion,
then wrap to the native u8 byte. Floating operands truncate toward zero; invalid
nonfinite/out-of-I32 conversion fails before writing. No run_script alias/reset
is used. Native append order is preserved. The entire command footprint must fit
the existing initialized 256-byte queue; invalid pointers/capacity fail atomically.
Only invalid cases differ from native partial-write/assert/undefined-overflow.

`cpu.nearest_enemy_at(x,y,z)` returns the actual native selector's U32 generation
handle or zero when its Bool result is false. It calls the native eligibility,
3D distance and strict-smaller selection algorithm. Equal distance keeps the
first eligible list entry; NaN/Inf coordinates are not rejected or normalized.
Both list walks are pool-bounded and check physical pool/class before userdata.
A successful selected handle is guarded against retirement before publication;
native false can mean absence or native query rejection and retains its original
zero-result contract. Caller retirement/program/schema failure is a VM error.

The console fixture compiles unchanged native command append and selector
bodies, actual binding/catalog/property/source/VM units, pool and registry.
Owned package/view/storage and stage/eligibility leaves are explicitly controlled.
It compares exact command bytes and selector results, checks ties/nonfinite
coordinates and capacity/class/cycle/caller failures, and exercises typed VM
scripts. This is not real scene/assets/gameplay evidence.

## Destination and scalar cohort

The same runner now also prepares and publishes two independently compiled
Fighter banks, attaches real source Fighter/Article actors, uses real fixed
GObj/receiver pools, and executes helpers through actual Execution/Storage/VM.
It extracts the exact production property read/write routes and the unchanged
`ftCo_CpuPrepareDestinationUpdate` and `ftCo_CpuSetDestination` bodies.
The stage-entry leaf is explicitly controlled and records whether it ran;
destination coordinates, spacing, latches and optional stage adjustment are
compared across the complete native Fighter payload.

Covered controls are `prepare_destination_update`, `set_destination`,
`set_approach_spacing`, RO `approach_spacing`, RW `mode`, `follow_distance`,
`combat_candidates_enabled`, `ranged_gate_enabled`, and existing `level` writes.
Scalar cases include signed Int extrema, negative zero, subnormals, infinities,
NaN payloads, negative follow distance, all gate combinations and adjacent bits.
Mode/level use well-formed signed 32-bit interpreter Int values; malformed
tagged payloads are outside this fixture's scalar-width claim.
Invalid follow distance and nonfinite destination are successful no-ops;
approach spacing has no clamp or finite gate. Central numeric conversion and
actual source VM cases include integer arguments for Float operations.

The new scalars/operations are source-only at language resolution, with
compiler rejection tests for receiver projections. Existing `cpu.level` reads
remain generic: real Article VM observes foreign owned and stock Fighters.
Its writes succeed on an owned target through actual retained receiver routing,
while stock writes fail before mutation. No successful substitute property
switch is used. Stale scene/generation, recycled object, substituted payload,
wrong class, fighter data, exact schema/parameter identity, destroyed storage,
program replacement and logical VM-fault cases are checked. Native leaf
retirement prevents result publication after the setter has performed its
native mutation; the fixture does not claim transactional rollback.

`run.py` runs both the connected cohort and the existing command/selector
cohort in hidden processes in one fresh A: archive. Its receipt pins native
inputs, generated extracts, compiler, headers, docs and all dependency leaves,
and checks input hashes again at completion. The command/selector cohort
retains its controlled program/view boundary; real actor claims apply to the
connected cohort. No scene, pairing activation, broader planner, controller
history or gameplay acceptance is implied.

## History, planner and output cohort

The connected cohort also links unchanged `native_cpu_history.c` and the
actual `ftLib_8008741C` lookup. It compares recording, cursor advance/wrap,
reserved-byte preservation, signed 127/128 stick quantization, direct trigger
truncation, all 30 delayed slots, malformed cursors and nonfinite pose values.
Input sources cover independently authored and stock Fighters, including the
signed Int representation of a high native creation ID. Pose output failure
preserves four distinct state destinations through actual VM publication.
Apply comparisons include samples whose pose is nonfinite: application copies
controller output without imposing the separate pose-read finite gate.

Four planner entry kernels and their native flag/latch phases are extracted
unchanged. Complete Fighter comparisons use two actual attached source actors;
only their object/generation and inline buffer/history addresses are normalized
for comparison. Geometry, target selection, random values, item selection and
deep attack/defense decision leaves are controlled and traced explicitly.
Actual native grabbing classification reenters an authored Bool policy through
the real VM, including a logical fault which blocks native continuation.
Retired selected Fighter/Item identities block publication. Equality-only x48
caches do not need live targets, and refresh operations may replace old targets.
The full native attack body retains its existing uninitialized-switch branch;
the fixture does not exercise that branch and suppresses its native compiler
warning. It does not claim that branch is defined or repaired.

All five controller output fields are tested against native S8/u8/u32 values,
including byte limits, successful invalid-assignment no-ops and signed button
bit patterns. Float/Bool assignments, malformed tagged Int widths, history
output aliases and wrong output types are rejected. The older command cohort
uses aborting unselected planner/Item-actor leaves, while the connected cohort
selects the actual kernels and Item actors. No paired activation, complete
planner geometry, final CPU scheduler, retail-history or gameplay acceptance
is implied by these controlled comparisons.


The selected-target creation-ID cases run actual property routing and compiled
source storage against self, foreign authored and stock Fighters. Null selection
publishes zero; all 32 ID bits survive the explicit Int state conversion. Unregistered data, wrong
class/backlink and physical slot reuse reject without dereferencing a stale data
pointer or publishing a replacement. Invalid source contexts and a faulted
caller also reject. Synthetic Fighter payloads and package metadata remain
controlled; these cases do not establish complete CPU decision/gameplay parity.

The planner-field batch adds actual reads/writes of travel count and selected
command, plus read-only elapsed count, cached floor availability and body width.
Whole-Fighter comparisons verify exact signed values and isolated field writes;
compiled source reads/writes exercise ordinary VM routing. Read-only fields,
wrong tagged types and out-of-range Int payloads reject without mutation.
Body-width receiver reads cover another authored fighter and a stock fighter,
including all Float bit patterns in the existing scalar corpus, compiled
article callbacks and stale receiver rejection. Source, schema and storage
rejection also cover the four new source-only fields. Synthetic actor/package
data remain controlled; no full CPU decision or rendered match is implied.
