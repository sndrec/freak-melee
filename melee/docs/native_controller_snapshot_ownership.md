# Explicit controller and rumble state

2026-10-11 checkpoint. This component owns the actual HSD controller state and
its fixed queue/rumble backing. It complements the existing object, allocator,
script and collision owners. Production rollback still uses the broad oracle;
this is not a complete match snapshot or aggregate performance acceptance.

## Owned state

`native_controller_snapshot.c` copies the full actual `PadLibData`, all four
Master/Copy/Game statuses, four rumble channel records, the complete configured
raw queue and every configured rumble list slot. This includes queued native
profile extension bits, normalized axes, button/trigger/release/repeat history,
clamp/ADC/scale configuration, queue indices/type/count, command cursors,
loop/wait/frame state, active lists and exact free-list order. Free records are
copied intact; their unused command pointers are not interpreted as active data.

Production `gmmain.c` initializes five queue records and twelve rumble slots.
On the current Windows ABI, the component is 1,896 bytes: a 1,000-byte typed
header, 320 queue bytes and 576 rumble slot bytes. Those are component bytes,
not total match state. Backing is supplied once through the actual `HSD_PadInit`
path; serializers allocate no memory, sample no devices and issue no output.

Physical controller mapping, output suppression and motor devices remain host
state in `rumble_output.c`. Snapshot Load does not overwrite them. The existing
resimulation begin/end protocol still suppresses speculative motors and
reconciles the restored channels explicitly at the end.

The default controller/rumble records are initialization prototypes. The
current source audit finds definitions and initialization reads, with no match
writes; they are not duplicated into this component. Rumble command bytes
referenced by head/list/stack cursors remain borrowed resident definitions.
Their lifetime and cross-owner candidate closure still require aggregate proof.
GM/LB input clocks, RNG and scheduling/pause globals are separate remaining owners.

## Lifetime and validation

The native backing catalog records the actual queue/list addresses and declared
capacities. Reinitializing the layout, even at the same addresses, retires
history. The actual raw-input callback now belongs to this module; its existing
setter changes the callback directly and retires history when its identity
changes. Reapplying the same callback does not invalidate snapshots. There is
no forwarding setter or duplicate controller representation.

Catalog, callback/activity and visited scratch are host metadata, excluded from
`.mstate` by the production source property. They remain stable through restore.
An activity depth around actual raw renewal rejects capture/validation/load from
the callback, including the all-disconnected early return path. All operations
require an idle game-thread boundary; concurrent input mutation is unsupported.

Candidate validation checks epoch/layout, queue bounds and circular-index
accounting, channel status and list counts. It follows all five chains (free
plus four active) using serialized slot records, never dereferencing candidate
next pointers. Every slot must be reached exactly once; foreign/interior/outside
pointers, cycles, duplicate ownership, missing slots and null/misaligned active
command cursors reject before writes. Exact lengths and overlap with this owner's
globals, catalog, queue and rumble backing also reject. Caller buffers must be
independent of every aggregate companion, not merely this component.

With the standard layout, Save/Load copy less than 2 KiB and validate twelve
list records. Scratch is fixed; the visited prefix is bounded by the declared
rumble capacity. Normal controller/rumble algorithms are unchanged. Resets,
filter replacement and device reconciliation are explicit operations and are
never invoked by a restore.

## Evidence

Under `A:/FreakMelee-test-archives/controller-snapshot-oct11-*`:

- `owner-final`: production controller/rumble/output/serializer units, 354
  checks and fifteen malformed candidate refusals. A full wrapped queue and
  active command lists reproduce twelve ticks exactly after Load, including
  normalization/history/extension bits, loop progression and next allocation
  identity. Thirty-three controlled pad samples include transfer/disconnect and
  all-disconnected early return. Snapshot operations produce no motor commands;
  explicit final reconciliation produces four. Hardware and replay/profile hooks
  are controlled boundaries; no physical controller opens. `owner-a` failed
  because its frozen header set omitted a required forward declaration;
  `owner-b` was the earlier passing subset.
- `eight-a`: stock and interpreted eight-actor matches, four hidden/muted
  rendered 1,200-sample runs. All nineteen components save/validate at all ten
  sampled boundaries in both scenarios. Controller saves average 0.50/0.48 us
  and validation 0.16/0.17 us in this observation, with separate queries. These
  are diagnostic component costs, not real full-aggregate save/load timings.
- `independent-verification.json`: 1,610 frozen/original fixture pins, 27 live
  pins, host-source metadata placement and every populated previous/current
  trace family match. Diagnostic on/off comparisons also match exactly.
- `build-a.log`: native client, content tool and source-assets fixture build.

Complete match resurrection, borrowed command residency, remaining global and
variable storage, multi-heap coverage, aggregate/ring orchestration and the
fifteen-resimulation-tick deadline remain open. The goal stays active.
