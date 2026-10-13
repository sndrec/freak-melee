# Secondary-bone dynamics and wind snapshot ownership

This component owns actual `lb_00F9.c` dynamics and wind state. It is not the
complete match serializer; production still uses the broad page oracle.

## Actual storage and source behavior

`dynamics_state.c` contains the original 320 secondary-bone records, eight wind
records, dynamics free head, wind active/free heads, wind status and dynamics
update-disable byte. All engine callsites use this storage directly. The two
old direct HSD allocations, obsolete allocation-container types and redundant
constructor entry point are removed. All scene callers now invoke
`lbDynamics_Reset` directly.
The source reset still clears the same joint/type fields and constructs the
same free lists. Initial static bytes are zero; unused/free payload is retained
across resets, rather than treated as regenerated scratch. Consumers initialize
their active poses/parameters through the existing construction/copy routines.

Original allocation order, capacity, wind priority replacement, lifetime/decay,
force query and secondary-bone simulation bodies retain their decisions and
arithmetic. Fighter dynamics lifetime checks use the same 320-node ceiling.
Each reset establishes a new host owner epoch; match exit retires that epoch.
The host lease is excluded from simulation storage, while actual records remain
covered by the page oracle during migration.

## Capture, candidate validation and restoration

The component is 54,312 bytes, including records, heads, controls and epoch.
Save validates all internal topology before touching caller output. Validate
copies an unaligned candidate into fixed stack storage and maps pointer values
to actual record indices before traversing any link. Load validates the full
candidate before copying the actual state. No HSD callback, allocation, physics,
rendering or audio runs during Save/Load.

The dynamics free chain may contain any subset. Allocated records form disjoint
acyclic forward chains, cannot point into the free chain, and cannot have two
incoming node links. Wind active and free chains partition all eight records;
cycles, duplicate membership, foreign/interior pointers and missing records
reject. Wind status retains its original -1/0/1/2 meanings. Free dynamics and
wind records keep their old payload, including dormant joint bits. Free records
are not followed as live model roots. Buffer overlap, in-flight scheduler/VM
activity, truncation and expired/reset scene epochs reject before writes.

The work is bounded by 320 nodes and eight fields. It uses fixed automatic
scratch and sequential copies with no tick-time heap allocation. This initial
representation retains free-record history because the original constructors
initialize selected fields. It does not copy a heap or unused MEM1 reservation.

## External dependencies and acceptance limits

A fighter/item/stage owns each external `DynamicsDesc` head/count/position and
its joint graph. Those descriptors are not duplicated in this component.
Stage callbacks can also retain pointers into the wind array, such as Peach's
Castle. Those caller roots, graph class/reservation generations, allocation
partition agreement and candidate-wide borrower closure must be verified by
the aggregate before any companion writes. This component does not assert that
raw joint pointers prove live object ownership. It never follows those pointers.

The dedicated fixture links the actual storage/snapshot code and unchanged
selected production reset, allocation, release and wind-update functions.
Joint setup and idle state are controlled; unrelated render/physics functions
are omitted from that fixture. External descriptors are restored explicitly as
companions in the test. Live rendered comparisons are separate evidence and
capture/validate only, without a whole-match Load or performance deadline claim.

Initial receipt `A:/FreakMelee-test-archives/dynamics-oct11-owner-d/receipt.json`:
463 checks, eleven malformed candidates, original capacities and exact next
allocation addresses, dormant bytes, release/reuse, wind exhaustion/replacement/
decay/expiry, 24 repeated real wind ticks, full component equality and capture
output preservation on rejection. The complete secondary-bone solver is not
executed in that fixture.


Additional receipts under `A:/FreakMelee-test-archives/`:

- `dynamics-oct11-gm-b`: connected production control/player/camera-target
  restoration fixture passes with actual dynamics-owner exit retirement linked.
  It does not initialize or simulate a live bone graph.
- `dynamics-oct11-stages-b`: three old/current, hidden/muted, normally rendered
  CPU scenarios, 4,800 samples each: Peach's Castle, Big Blue and Final
  Destination. All populated gameplay traces agree and all forty dynamics
  capture/validation boundaries pass per scene. Mean Save 6.433-6.575 us and
  Validate 3.058-3.123 us, 54,312 bytes. These are component capture/validation
  timings, not Load or total rollback timings.
- `dynamics-oct11-body-verification.json`: all 23 retained `lb_00F9.c` function
  bodies preserve original decisions/arithmetic, allowing direct storage names,
  reset naming and epoch establishment. Fifteen scene constructor callsites
  differ only in calling the reset directly. The allocation-only entry is gone.
- Four build targets and focused GObj-consumer CTest pass. The broad oracle
  continues to cover this state; production does not yet use aggregate restore.

- `dynamics-oct11-eight-a`: stock and independent interpreted eight-actor runs
  pass all thirty components at ten boundaries each. Previous/current and
  diagnostic-on/off populated traces agree exactly. No live Load.
- `dynamics-oct11-independent-verification.json`: final owner/frozen/live input
  hashes, source bodies and compiled section placement verified. Actual storage
  is `.mstate$m`; the snapshot lifetime lease is host `.data`.


The 2026-10-11 Castle stage expansion adds a generic `NativeWindReference` and
Reference/Resolve methods for the physical eight-slot backing. They verify
numeric bounds, alignment, reserved bits and the reset epoch without requiring
wind-request occupancy. This matches stock replacement's retained caller aliases;
it is not a per-request generation claim or a second wind-payload serializer.
See [stage car/wind ownership](native_car_wind_snapshot_ownership.md).
