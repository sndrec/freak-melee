# Explicit GObj scheduler ownership

2026-10-11 checkpoint. The actual GObj owner now saves and restores scheduler
roots/tables and both occupied GObj/GObjProc pool components together.
`native_gobj_pool_storage.c` retains ownership; `native_gobj_snapshot.h` defines
the private process-memory boundary. Production rollback still uses the broad
oracle until the complete match aggregate is implemented and verified.

## State and residency

The component stores the scene epoch/config identity, complete occupied object
and proc payloads, capacity generations and ordered free slots, active p-link
heads/tails, GX heads/tails, priority proc heads and per-link priority proc tails.
It also stores the rotating invocation marker, current priority, delayed-record
bytes (flags must be zero), retained current-render-object pointer, render
kinds/flags and the value of the configured
pointed process mask. The current priority can remain from the previous scene
until a proc pass runs; it is not restricted to the new scene's priority count.

Only configured table extents are copied. The remaining reserved table capacity
is cleared at scene binding and never addressed by that scene's scheduler.
Actual method bindings and config pointers are frozen for the scene. This is
not a new function-pointer API, disk/network format, compatibility layer or
reconstruction through source constructors. No destructor, callback or script
executes during loading. Userdata, HSD graphs, source resource registries and
Fighter/Item/Effect script actors remain companion owners.

The existing scene config and method count are retained directly at binding;
ordinary allocation, free, queueing and proc dispatch add no snapshot tracking.
Render dispatch increments/decrements a nesting counter around the actual
callbacks. The counter must be zero at an explicit snapshot boundary; it does
not replace or clear the existing retained render context.
Save/load uses caller-reserved buffers and fixed host scratch. The scratch is
outside `.mstate`, independently of the actual owner storage in `.mstate`.
There is no runtime allocation or page protection during these operations.
Work is linear in pool capacities, occupied nodes and configured table extents;
the generic pool operations currently repeat some validation. Whole-match
timing and the eight-actor fifteen-resimulation-tick deadline remain unmeasured.

## Boundary and validation

The boundary requires a live unchanged scene, actual pool/table bindings,
an idle VM, no active proc/draw callback, no next proc, and no delayed operation.
Scene retirement checks actual callback depth in addition to the existing
max-GX callback marker. The shadow renderer intentionally retains
`HSD_GObj_804D7814` between ticks, so its non-null value is not an active-callback
indicator. This pointer is saved/restored by the GObj owner instead.

Validation first checks both generic pool components, including complete
generation/free/occupied metadata. It then builds fixed maps of serialized
occupied records. All topology references resolve through those maps; no
candidate or current object pointer is dereferenced. The candidate render context
must be null or refer to a serialized occupied GObj. It checks complete p-link
membership, previous links and tails; unique GX membership and tails; all
priority chains, owners, previous links and per-link tails; and every owner's
proc child chain. Cycles, missing/duplicate/foreign nodes, mismatched ownership,
in-progress destruction, malformed byte counts and stale scenes reject before
any live state write. A new capture rejects a foreign or currently free retained
context, rather than pretending its absent payload is owned. Loading instead
checks candidate ownership and can resurrect a now-destroyed context object.
Newly created procs legitimately use invocation flag 3;
0..2 represent passes, not the complete valid flag domain.

Snapshot/result buffers cannot overlap either actual pool, scheduler tables,
owner metadata/controls, configured mask storage or host scratch. Complete
candidate validation precedes restoring pool records and scheduler controls.
Other pointed owners must also prevalidate and restore before dispatch/drawing
resumes; this component does not prove arbitrary userdata/model graph closure.

## Evidence

Fresh evidence: `A:/FreakMelee-test-archives/gobj-snapshot-oct11-*`.

- `owner-c`: 17 fixture scenarios / 41,334 checks, including 20,576 scheduler
  checks and 13 malformed candidates. Most additional checks are repeated real
  allocations reaching 4,096 GObjs / 16,384 procs, not distinct gameplay cases.
  The three-object/four-proc component is 164,872 bytes. An additional storage
  case covers full byte-sized config/table extents without running the source
  callback mask shift for links outside the source's 64-bit process mask.
- The actual callback sequence includes deferred self-removal, object/proc
  address reuse, priority changes, flags, max-GX and ordinary render callbacks.
  Explicit load restores original references/generations, free order and exact
  re-save; the original callbacks and next allocation addresses repeat.
  The earlier broad-region lifecycle cohort remains independent evidence.
- `resource-c`: ordinary 2,816 checks / 15 cases, animated 4,543 / 16 cases,
  including 39 native/VM animation comparisons and 68 scheduler/script checks
  per run. The connected 179,584-byte scheduler component pairs with actual
  Fighter/Item script actor serializers and real interpreter callback dispatch.
  A root fighter variable and both item variables, tick/callback state, callback
  binding and actor relationships re-save exactly. A future GObj disappears
  logically, the same callbacks run after restore, and its address repeats.
- `stages-a`: Battlefield, Great Bay and Brinstar each match the previous client
  across 2,000 samples with Link/Samus/Ness/Ice Climbers, medium items, retained
  rendering and no census failures. All populated trace families match.
- `source-a`: interpreted Samus, Link, Young Link and Ness each pass bounded
  stock comparisons across 1,800 controller samples. Launches hidden/muted.
- `build-a.log`: client/content/source-assets/GObj/census build passes. Focused
  GObj consumer and census CTests pass. Independent verification records source
  pins, previous-client bundle hashes and compiled/frozen owner-source matches.

Earlier failed fixture attempts remain in their archives: `owner-a` incorrectly
excluded new proc flag 3; `resource-a` lacked native Item msid publication in its
controlled transition; `resource-b` queried the program's declared callback slot
instead of the instance's mutable binding. Corrected fixtures follow the actual
source contracts; production VM behavior was not relaxed to make them pass.

Remaining aggregate owners include other module globals, HSD current selection,
direct mutable HSD buffers, source registries and complete world/render/audio
state. The production broad oracle and overall goal remain active. No full-game
size/performance, multi-PC or pixel-perfect visual acceptance is claimed here.
