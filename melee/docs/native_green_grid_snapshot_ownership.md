# Green Greens block-grid storage

The Green Greens grid controller allocates thirty positions and a 5x6 block
record array. Those are mutable satellites of Ground, not resident assets.
Saving the Ground record alone retained their addresses but not their contents.

The native controller now binds optional typed storage to its actual Ground
slot. `native_green_grid.c` owns the arrays, owner references and occupancy bits;
the original controller receives the actual array addresses in its existing
fields. The two HSD heap allocations/clears and frees are replaced by binding
and retirement. Binding zeroes the arrays before the existing initialization
logic. All block movement, hit, destruction, collision and spawn decisions stay
in the original functions.

The reservation supports every Ground slot, so it does not impose a smaller
satellite limit than the existing actor reservation. One optional record per
Ground slot matches the controller's two pointer fields. Its physical storage
is reserved separately from the Ground union, avoiding a larger copied Ground
record for every other stage actor. Only occupied satellites are serialized.
This is fixed storage, not per-tick allocation.

## Lifetime and restoration

Scene begin/retirement are integrated into the actual Ground pool lifecycle.
Controller cleanup drops its ownership claim; actual Ground userdata retirement
also drops any remaining claim. A Ground generation cannot inherit an old grid
claim. Bind rejects duplicate ownership and requires the real stage classifier,
userdata kind/destructor, reciprocal owner link and grid-controller map ID.

Snapshots contain a lifetime/count header followed by strictly increasing slot
entries. Unoccupied reservation bytes and a full free-order array are not copied.
The owner references are paired GObj/Ground generation claims. Save validates
every live owner before touching output. Validate checks geometry, generation
claim shape, reserved fields, counts/order, block statuses and active indices
without demanding current occupation. Load requires companion GObj/Ground
payloads first and checks every owner and actual array address before modifying
the occupancy bits or grid payloads.

The bitmap scan is bounded to 64 words. Save/Load use fixed scratch and memcpy;
they allocate nothing and invoke no game callback or rendering. Empty scenes
use a 16-byte header. With cell references, two controlled grid owners serialize
to 12,368 bytes; a normal one-grid scene is 6,192 bytes. The capacity reservation is larger than
those payloads and is not the reported snapshot size.

## Borrowers and remaining closure

Active cells now carry paired GObj/Ground and GObj/Item generation references,
plus a retained ordinary-class joint span and initialized type identity. Numeric
validation proves pool/lifetime/slot geometry and joint backing without following
candidate actor pointers. The generic `nativeClass_TypeKnown` lookup identifies
class descriptors in the initialized tree before reading their sizes; a forged
info pointer is never dereferenced.

Save proves stage/item classifier, userdata kind/destructor, reciprocal actor
links, normal/bomb map role, and the material's grid-owner/joint relationship.
A bounded, direct-mapped 64-entry joint-span cache avoids repeated backing scans;
collisions only replace cache entries, never limit the number of live joints.
Load checks every saved live borrower against restored companion generations,
roles and joint types before writing any grid bytes. GObj, Ground, Item and class
payloads must restore first.

Inactive cells have zero borrower claims. The original cleanup leaves actor,
material and joint pointer bits behind after destruction; those bits remain in
the payload without being followed. New cell creation overwrites them before
reuse. The hit-source field is written but never consumed by the current stage
code; it likewise remains raw bytes, not an asserted live dependency. Column
compaction moves complete records; claims follow their contents, not old cell
positions. No original block gameplay function changed in this update.

These local checks do not establish candidate-wide graph closure before the
companion writes. The aggregate still must prove that the retained class block
exists in the same candidate, that joint spans are occupied by the right graph
objects, and that other stage/material/collision references agree. A class span
alone is not an object-generation reference or a complete stage restore.

The broad page oracle still covers the actual reservation while that integration
is unfinished. Other stage satellites, private/header variables, mutable borrowed
attributes, multi-heap/resident closure, remaining globals, aggregate history
and whole-match rollback/performance acceptance remain open.


Initial array-owner receipts under `A:/FreakMelee-test-archives/` (before cell
references; their payload sizes and coverage are historical):

- `green-grid-oct11-owner-b`: 153 grid checks/nine malformed candidates, empty
  and two-owner shapes, original zeroing, complete payload restoration, sixteen
  repeated controlled updates, late generation rejection without partial writes,
  sparse occupancy, companion resurrection and overlap/activity/retirement
  refusal. Actual Ground storage/serializer; controlled generic GObj construction
  and idle predicate. No real block gameplay simulation in this fixture.
- `green-grid-oct11-ground-a`: all twelve existing Ground consumer cases and
  12,411 assertions pass with satellite lifecycle integrated.
- `green-grid-oct11-stages`: four old/current rendered CPU scenarios, 4,800
  samples each (Green Greens, Brinstar, Kongo Jungle, Final Destination). Populated
  gameplay traces match exactly and all forty grid capture/validation boundaries
  pass per scene. Green Greens observes one 2,112-byte grid and mean Save 1.173 us;
  the other scenes use sixteen bytes. No live Load or whole-match timing proof.
- `green-grid-oct11-eight`: stock and independent interpreted eight-actor
  scenarios pass all 29 components at ten boundaries each. Diagnostic-on/off
  and previous/current populated traces agree exactly. No live Load.
- `green-grid-oct11-gm-a`: existing connected control/player/camera-target
  restoration fixture passes; it does not construct a real grid.
- `green-grid-oct11-independent-verification.json`: owner/frozen/live inputs,
  compiled simulation versus host sections and all fifty donor Green Greens
  bodies verified. Allowed changes are typed binding/retirement in the actual
  constructor/destructor. Four build targets and GObj-consumer CTest pass.

## Active-cell borrower checkpoint, 2026-10-11

- `green-borrowers-oct11-owner-b`: real Ground/Item pools and class allocation/
  companion snapshots, with controlled GObj backing and material destructor
  endpoint. Twenty malformed candidates are rejected. Tests cover corrupt
  pointers/types/spans, dormant invalid aliases, material owner disagreement,
  actor/item/Ground generation reuse, class restoration, column movement, output
  preservation on failed capture and late rejection before any grid writes.
  Two occupied grids serialize to 12,368 bytes. The reported assertion count
  includes byte-by-byte verification of an untouched 16 KB output buffer.
- `green-borrowers-oct11-ground-b`, `-item-a`, `-lifetime-b`, `-gm-b`: existing
  actual owner/lifecycle/control fixtures pass with the new dependencies.
  Fixtures with no model graph explicitly abort unexpected class-owner calls.
  The scene-lifetime fixture now includes its production census-abort declaration.
- `green-borrowers-oct11-stages-a`: old/current hidden, muted, normally rendered
  Green Greens CPU scenarios, 4,800 samples each. Populated gameplay traces
  agree; all forty grid Save/Validate boundaries pass. This is live capture and
  borrower validation, not live Load or complete aggregate rollback.
  One grid is 6,192 bytes; observed mean Save 5.915 us, Validate 2.265 us.

- `green-borrowers-oct11-eight-a`: stock and independent interpreted eight-actor
  scenarios pass all 29 Save/Validate components at ten boundaries each.
  Diagnostic-on/off and previous/current populated traces agree. No live Load.
- `green-borrowers-oct11-independent-verification.json`: final source/frozen/live
  input hashes verified; original block/material/Item/pool bodies unchanged;
  compiled actual storage remains in `.mstate$m` and the lease/cache in `.data`.
  Four build targets and focused GObj-consumer CTest pass.
