# Interpreted fighter behavior and explicit rollback state

Approved direction, 2026-10-07. This supersedes the earlier generated-C plan.
Ordinary Workshop/package execution now uses the typed interpreter and source
packages, without generated C or behavior DLLs. Single source gameplay and
alternate-family publication/transfer infrastructure have separate recorded
acceptance. All 26 scoped stock forms now have independent source templates and
normal source-package preparation. Current gameplay evidence includes the
[22 single-fighter CPU cases](native_source_roster_cpu_checkpoint.md),
[paired forms and companions](native_source_family_gameplay_checkpoint.md),
[176 special entries](native_source_roster_special_checkpoint.md),
[special follow-through](native_source_roster_followthrough_checkpoint.md) and
[two-source interactions](native_source_roster_duo_checkpoint.md).
The [results checkpoint](native_source_roster_results_checkpoint.md) records
match-end/model admission and the demo-table query fix, with 100 selected
outcomes passing across the full initial run and 24-case post-fix rerun.
These are bounded acceptance cohorts, not exhaustive whole-fighter completion.
Live Workshop transactions, further presentation/lifecycle acceptance and
explicit aggregate rollback state remain separate work. The implementation
notes dated October 7 below are historical checkpoints; their then-pending
package activation and roster migration must not be read as current blockers.

## Decision

Ordinary fighter, article, effect and recipient behavior will execute in a
native interpreter. Retain the readable `.fms` language, editable move/state
files, owned parameters and generic engine operations. Physics, collision,
animation, rendering and reusable engine primitives remain native.

Parse and type-check `.fms` directly into a typed representation, then lower it
to compact bytecode. Ordinary authoring, preview, packaging and installation
must require neither generated C, a C compiler, SDK headers nor behavior DLLs.
The evaluation's C-AST input is temporary test tooling, not the production
frontend. Handwritten native extensions remain an explicit advanced facility;
they are not a fallback for unsupported ordinary scripts.

Use one production script execution path. Migrate call sites and data ownership
properly, then remove the superseded C emitters, ordinary script DLL loading,
compiler distribution and associated UI flows. Do not retain a compatibility
backend, per-function C ingress wrappers or native authored-body fallbacks.
Keep historical comparison evidence accurately labelled while it is useful.

## Evidence informing the decision

The isolated evaluator lowers all 257 Popo and 266 Nana authored functions.
Authored conditions, arithmetic, loops, helpers, callback guards and field/state
access execute as interpreted instructions. Generic state accessors, immutable
definition setup and typed native engine operations remain native; the manifest
lists them. No authored body silently falls back to native execution.

On the user's i7-10700K, two sequential real-game batches used four CPU Ice
Climbers slots, eight live actors, the same seed/inputs and 64 measured windows.
Each window restored 15 ticks, ran all 15 simulation/draw passes, checked complete
restored state, then ran one advancing tick. Every semantic endpoint and every
observed authored function call count matched between C and interpreted runs;
the evaluator reported zero errors.

| Mean work per window | Generated C | Interpreted |
| --- | ---: | ---: |
| 15 replay simulation ticks | 17.099 ms | 18.425 ms |
| 16 simulation ticks including advancing tick | 18.239 ms | 19.672 ms |
| 16 simulation/draw passes | 38.750 ms | 40.841 ms |
| Full rollback window including tracking/restoration | 197.843 ms | 210.871 ms |

The observed simulation difference is 1.326 ms across 15 replay ticks. These are
sequential shared-PC measurements, not an isolated statistical estimate of
interpreter overhead. Page-write faults still contribute inside simulation and
drawing. Begin/end tracking across about 193 MB dominates the full window; actual
dirty pages are roughly 1.7 MB per tick. Ordinary interpreted simulation/drawing
averages approximately 5.4 ms per advancing tick in this workload.

**The 15-replay-plus-current-tick target within 16.667 ms remains unmet.** Choosing
the interpreter does not claim that target passed or that future snapshots will
meet a particular speed. Save/load below 100 microseconds is a desired target to
measure, not an established result. Repeated rendering also needs measurement;
do not discard state-changing draw callbacks or RNG calls to manufacture a pass.

Real-game measured coverage was 14 of 523 authored bodies. Additional fixtures
compare 4,096 scalar cases and 128 Gumstrings cases per member, including 40-node
loops and nested helpers. Those fixtures use controlled Host/resource outputs.
Broader fighter, article, recipient-lifetime and game acceptance remain required.

Authoritative evidence under `A:/FreakMelee-test-archives/`:

- `interpreter-lowering-reviewed/receipt.json`: actual typed lowering and DLL
  prototype builds against live native headers.
- `interpreter-scalar-first/receipt.json`, `interpreter-rope-first/receipt.json`:
  bounded native/interpreted correctness comparisons.
- `eight-actor-native-golden/receipt.json` and
  `eight-actor-interpreted-golden/receipt.json`: verified real-game windows.
- `interpreter-eight-actor-comparison.json`:64 equal semantic endpoints,
  identical observed function counts, zero VM errors, deadline explicitly false.

## Production interpreter contracts

- Preserve supported `.fms` semantics deliberately: declared Bool/int/Float,
  required Float/Double intermediates, unsigned identity operations, expression
  ordering, short circuiting and source-retirement guards. Define numeric rules
  in the language rather than inheriting accidental C syntax indefinitely.
- Describe native bindings by stable semantic names, argument/result types,
  ownership and invalidation effects. Resolve them at load. Do not ship native
  field offsets, callable machine addresses or Clang ASTs as script contracts.
- Keep the existing animation/IASA/physics/collision/accessory scheduling and
  original subaction interpreter. Scripts own fighter-specific decisions;
  general engine primitives supply the underlying operations.
- Integrate callback identities and code references directly into the native
  registry/scheduler. Preserve generation checks, synchronous recipient reentry,
  typed helper returns and bounded dispatch. No per-authored-function C wrapper.
- Code, constants and binding metadata are immutable while a match is active.
  Fighter/article variables belong to explicit per-instance rollback state.
  VM frames finish synchronously and are transient. Use fixed pools/stacks and
  allocate only when loading/applying content or changing program states.
- Preserve the last working preview when parse/type/load validation fails.
  Report runtime errors with source line, function, event, fighter/article and
  frame. Do not silently clamp arbitrary values or execute native fallback code.
- Keep editable source and required assets in shared packages. Build validated
  bytecode locally with the client, cache by source/dependency/binding identity,
  and agree on execution semantics and selected content before a match starts.
  No permanent old-format migration scheme is needed for this private project.

## Explicit savestates

Replace broad heap/page capture as the production rollback mechanism with an
audited native match snapshot. Account for fighters and CPU/input history,
articles/items, stage/collision state, animation state required by collision,
RNG, object scheduling, mutable callback bindings, script variables and object
lifetime/allocation bookkeeping. Preserve stable object identities and restore
creation/destruction and linked references coherently.

Immutable meshes, textures, animation definitions, scripts and other assets are
shared. Exclude unused capacity and host UI/network/device state. Rebuild scratch
data and derived caches only where doing so preserves the next simulation tick.
Animation poses and render-side writes cannot be assumed disposable without an
actual dependency audit.

Retain broad page capture as a development comparison reference until explicit
save/load reproduces the same states and timelines. Track outside-frame writes,
allocator reuse, callback retirement, items, hazards, hitlag, deaths and paired
fighters. Measure snapshot bytes and actual save/load durations. Persistent
dirty-page rearming is a possible interim optimization, not the final state
boundary and not an excuse to postpone identifying actual mutable state.

## Implementation handoff

1. Preserve this evaluation and the authored templates as reference evidence.
   Hold further generated-C-specific development. Existing incomplete fighter
   behavior translations remain useful independently of execution backend.
2. Implement the direct typed frontend and generic binding description. Cover
   existing syntax, metadata, helpers, arrays, articles/effects and callbacks;
   reject unsupported constructs explicitly.
3. Integrate interpreted program/code references, per-instance state, lifetimes
   and recipient calls into the existing engine. Apply programs at practice
   reset and pin them for matches/replays. Reproduce existing fighter comparisons.
4. Replace Workshop build/apply and package/network loading with direct script
   validation and interpreted-program loading. Remove superseded ordinary C/DLL
   paths after call sites are migrated; preserve optional advanced extensions.
5. Audit and implement explicit savestates, using broad capture as an oracle.
   Re-run eight-actor 15-tick timing with full fidelity checks and phase costs.
6. Finish remaining fighter/editor/release acceptance with the interpreted
   backend. Kirby remains deferred. No UI redesign is authorized by this change.

## Current implementation checkpoint, 2026-10-07

Direct source indexing, typed statements, bytecode lowering and fixed-pool VM
execution now build in the native client. Source signatures, parameter defaults,
owned arrays, helper returns, loops, output ordering and synchronous native
reentry have focused acceptance. The first source-checked native property/math
family executes through the real Fighter/Item units. These units are not yet
selected by Workshop/package loading; the working client still uses its existing
ordinary behavior path until those call sites migrate.

All 33 authored templates pass declaration indexing. This is not full-body
binding or gameplay acceptance. The native client/tool build and 11 focused
frontend/runtime/replay/rollback/pool CTests pass. Evidence and supported scope:
[frontend checkpoint](native_script_frontend.md).

Explicit snapshot work now has an occupied-object pool utility that restores
generations and ordered free slots as well as payload. GObj/proc/Fighter/Item and
resource-link fixtures check reuse, resurrection and linked references. Actual
HSD consumers and the whole-game snapshot are still pending. Scene tracing shows
that GObj/proc registries reset on every minor scene; the recommended storage
lifetime is one process-reserved family with scene epochs and snapshots pinned
to their epoch. See [consumer lifetime proposal](native_match_pool_first_consumer.md).
The original broad/page oracle remains unchanged; no speedup or reduced live
snapshot size is claimed yet.

The following storage/event slice adds actual declaration-owned byte buffers,
immutable supplied/default parameters and same-instance snapshots. A 64-tick
VM sequence reproduces its endpoint after restore. Source-audited callback
signatures now encode implicit returns explicitly in bytecode, including input
value preservation, read-only query policy and exact-vs-truth return rules.
The storage fixture passes 713 checks and event fixture 301; the native client/tool
build and 13 focused CTests pass. Two mutable trail payload callbacks remain
explicitly unsupported. Live engine scheduling, payload binding and
Workshop/package activation still have not migrated.

Native entry/payload/kinetics checkpoint: actual source-owned self article fields
and scoped event values now route through the execution entry alongside native
Fighter/Item properties and general physics/animation primitives. Callback IDs,
ownership and ABI are resolved once at load. The real actor fixture executes
128 ticks; kinetic comparisons preserve Float/Double arithmetic and generation
guards. Native client/tool builds and 17 focused CTests pass. A live untimed
eight-fighter census completed 2,000 gameplay input samples with no topology
errors. Its counted allocator/class payload peaked at 1,388,376 bytes, including
scratch/immutable data and omitting other state; it is not the new snapshot size.
Evidence: `native_script_frontend.md` and `native_match_object_census.md`.
Remaining bindings/metadata, scheduler selection and Workshop/package loading
remain required before ordinary C/DLL removal or gameplay/timing acceptance.

## Script owners, item primitives and loading closure

Typed Fighter owners and script actor records now have occupied-only serializers.
Script restore rebuilds execution/storage bindings from pinned immutable programs;
it saves declared state, callbacks and clocks rather than maximum reservations.
The live source-only callback workload reproduces all 64 broad-oracle endpoints
after explicit script-owner rewinds. See `native_fighter_snapshot_ownership.md`.
This is partial owner integration; full match save/load and the frame budget remain
open, with the original broad backend still selected.

Generic Item self fields, gravity/spin, lifetime, collision response, sound and
flags now execute directly through typed bytecode. Source comparisons and real
fireball/Rook expressions pass 10,966 checks; full entity spawning, owner/creator
contexts and item/article scheduling still require a coherent lifetime closure.
See `native_script_item_bindings.md`.

`native_source_package_loading_audit.md` traces ordinary Workshop/package loading
and its remaining native callback addresses. Source-only activation requires
candidate-owned data definitions, prevalidated programs/scheduler bindings and a
single publish transaction. Updating only the package flag would leave DLL/native
callbacks selected. Ordinary generated-C/DLL removal remains a required later step.
