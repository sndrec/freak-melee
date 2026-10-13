# Native Ground owner and occupied-record snapshots

2026-10-11 implementation checkpoint. This migrates actual stage-object userdata,
not an alternate representation of stage behavior. Whole-match explicit snapshots
remain incomplete; the production broad page oracle still covers all stage state.

## Ownership and initialization

Both `Ground_GetStageGObj` and `Ground_801C1A20` allocate directly from
`nativeGroundPool`, after their existing stage GObj creation. They publish the
actual `Ground*` as kind-3 userdata with `nativeGroundPool_Release` as its removal
callback. Stage-specific initialization, proc/draw callbacks, graph loading and
`Ground_801C4A08` cleanup retain their existing paths. There is no heap fallback.

The storage contains 4,096 stable records, matching the GObj reservation. Each
Ground requires its own GObj, so this introduces no earlier Ground-only capacity
limit. Actual Windows record size is 656 bytes: 2,686,976 reserved payload bytes.
The backing records, occupancy/generations, ordered free slots and descriptor
remain in NON-HOST `.mstate` for broad-oracle coverage during migration.

Scene entry preflights and binds storage to the same epoch as GObjs and Fighters.
Retirement logically invalidates it after outgoing scene callbacks and before
preload can reclaim referenced assets. It does not invoke old stage destructors.
Normal immediate, standalone-userdata and scheduler-deferred removal use the
actual HSD lifecycle and free exactly the occupied Ground record.

The allocator clears each newly occupied record. Retail's map constructor left
the stage-specific union uncleared, allowing reads before initialization to see
stale heap bytes. This is an intentional deterministic initialization change,
not an attempt to reproduce that undefined behavior. No stage decision is changed.

## Snapshot boundary and scope

Save/load require an idle GObj scheduler and script runtime, outside callbacks or
delayed removal. Save checks every occupied record's live stage GObj, userdata
kind, exact userdata pointer and removal callback. The generic packed serializer
records all slot generations, exact free order and only occupied payload bytes.

Validation checks the entire candidate before mutation, including epoch, pool
identity, sizes, occupied indices and stable GObj pointer geometry. It deliberately
does not require saved GObjs to be occupied in the **current** timeline: restore
must resurrect removed owners. The aggregate must restore the GObj family and
validate cross-family userdata relationships; the Ground loader does not recreate
or dereference an unrestored graph. It retains raw stable pointers to resident
assets and other runtime owners.

This is not stage closure. `stage_info`, mutable stage attributes, collision and
moving-object buffers, graph objects, secondary GObjs and stage/item globals still
need their own owners/serializers. No save/load speed or complete snapshot-size
claim follows from this slice.

## Focused verification

`tests/native/native_ground_pool` links actual storage, serializer, HSD GObj/
userdata lifecycle and scene pools. It exercises 12 cases and 12,411 assertions:
zero initialization, immediate/standalone/deferred removal, reuse generations,
occupied payload restoration, allocator replay order, malformed/truncated roots,
rejected-load nonmutation, callback boundaries, stale epochs and full capacity.
The model loader and stage-specific callbacks are absent controlled boundaries.

The fixture first checks typed Ground/GObj loads, then restores scheduler roots
with the broad `.mstate` oracle for further lifecycle operations. Two occupied
Ground records serialize to 34,136 bytes; all 4,096 serialize to 2,719,800 bytes.
The fixed generation/free-order metadata is included in those figures.

Receipts under `A:/FreakMelee-test-archives/`:

- `ground-owner-fixture-oct11-b/receipt.json`: focused Ground verification.
- `ground-gobj-consumer-oct11-a/receipt.json`: 16 cases, 20,734 assertions.
- `ground-fighter-consumer-oct11-a/receipt.json`: 10 cases, 1,650 assertions.
- `ground-scene-lifetime-oct11-c/receipt.json`: 13 scene transitions, 417 assertions.
- `ground-census-fixture-oct11-a/receipt.json`: 13 census cases, 45 assertions.
- `ground-owner-oct11-build.log`: native client and content-tool build.

The scene-routing fixture was updated for current article retirement and source
report declarations; it contains no articles/source actors. Its duplicate Fighter
source listing was removed. The census now names and counts `ground_owner`, and
the existing GObj fixture's bounded oracle includes the new stable storage.

## Rendered stage cohort

`tests/native/native_ground_pool/stages.py` runs old/new binaries with the same
seed and 2,000 controller samples for each of the 29 selectable versus stages.
It retains normal simulation and HSD rendering, uses hidden/muted fast pacing,
medium items and CPU Ganondorf, Zelda, Ice Climbers and Game & Watch. It compares
fighter/companion/action/damage/item/resource/RNG trace families and checks
completion and census topology errors. This is bounded gameplay evidence, not
proof of every hazard, animation, audio-clock transition or pixel.

Terminal cohort `ground-stage-cohort-oct11-c/receipt.json`: **28 of 29 stages
passed**, with exact old/new traces, completed inputs and zero census errors.
Green Greens failed in both binaries during `/GrGr.dat` import, descriptor
status 6 at offset `0x59024`, before gameplay. This is an existing importer
failure, not a Ground-storage regression, and remains a separate follow-up.
The whole cohort receipt correctly remains **FAIL**, not a full-stage acceptance.
The runner's unchanged-input check passed; all 12 pins were independently
rehashed after completion with no changes.

Earlier `a` launches rejected an invalid CLI fighter name. Interrupted `b` lacked
the copied baseline's UI assets initially and is not an acceptance cohort. The
terminal `c` cohort had baseline UI and runtime DLLs copied before launch.

The Green Greens loading dependency has subsequently been corrected at scene
call sites; see [native stage dependencies](native_stage_asset_dependencies.md)
for the cause, current verification and its distinct evidence boundary.
