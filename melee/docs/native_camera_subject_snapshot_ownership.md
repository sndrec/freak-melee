# Camera target snapshot ownership

2026-10-11 checkpoint. This covers the actual camera targets and their lists,
not the main camera, CObj/WObj graphs, stage globals or a complete match snapshot.

## Storage and behavior

`camera_subjects.c` owns the actual `CmSubject` records and four existing list
roots. `Camera_Init` calls the substantive owner initializer directly. The four
allocation/reset/release entrypoints moved intact; all other original camera
function bodies are unchanged. The main camera's initialization is unchanged
apart from replacing the subject allocation/link block.

Physical storage holds 256 records. Callers still request their original logical
capacity: 70 in versus, 6/8/12 elsewhere. Allocation order, exhaustion behavior,
reset values and active/free topology are preserved. No heap allocation or second
representation is introduced. Initialization clears the requested prefix; a
larger subsequent scene clears its full prefix before it can allocate from it.

Free targets link through `prev`. Releasing a target deliberately retains its
old `next`, state and pose/history bytes. Those bytes must survive a rewind even
though the free list does not traverse `next`. The component saves all requested
records, including free ones. The versus component is **8,456 bytes**: a 56-byte
header plus seventy actual native 120-byte records.

## Lifetime and validation

Camera initialization and GM-loop entry increment a host epoch; loop exit retires
the component. Same-address, same-capacity reinitialization or loop re-entry
cannot accept old history. GObj scene/storage identity is frozen at entry.
Actual targets/roots remain in `.mstate$m` during migration; host metadata stays
in `.data` and is never rewound by the broad page oracle.

Save/Validate/Load require the actual GM idle boundary. Simulation, input,
drawing, active/deferred callbacks, construction and script activity refuse.
Candidate pointers are checked numerically as exact slots of this requested
prefix, never dereferenced. Every next/prev pointer, including stale free-node
links, must refer to that prefix or be null. Active links must be reciprocal and
end at the declared tail; the free chain follows prev. Both chains together must
cover every record exactly once. Cycles, duplicates, missing nodes, foreign or
interior pointers, wrong epochs/count/backing and independent-buffer violations
reject. Unaligned candidate input is read with memcpy.

Validation finishes before any Load write. Save validates live topology before
writing either output or its byte count. Successful operations allocate nothing
and invoke no constructors, gameplay callbacks, devices or audio. Borrowers in
Fighter, Item and Ground records retain stable addresses. This owner alone does
not prove candidate agreement with those companions; aggregate validation must
check that each restored borrower refers to the correct restored active target.

## Evidence

Under `A:/FreakMelee-test-archives/camera-targets-oct11-*`:

- `owner-final`: actual GM/controller/input/RNG/player/Fighter-pool code plus
  actual target methods. A saved frame-eight state reproduces sixteen ticks
  exactly, including target release/reallocation and free-record history. There
  are 31,797 target checks, sixteen malformed target candidates, empty/full
  active lists, one/256-capacity endpoints, unaligned input, lifecycle refusals,
  late-record no-write checks and 144 activity boundaries. The existing 2,018
  GM/player checks also pass. External scheduler/host/device/render endpoints
  and actor construction are controlled; no whole HSD graph is restored.
- `eight-a`: four hidden/muted rendered 1,200-sample client runs. Stock and
  independent interpreted four Ice Climbers pairs exercise eight actual fighters.
  All **25 components** save and validate at all ten sampled boundaries per
  scenario. Diagnostic on/off gameplay traces match. The census never loads.
- `independent-verification.json`: 1,819 original/frozen fixture pins and 55 live
  pins, zero exits/log hashes, compiled sections, all 160 original camera bodies
  (initializer change checked explicitly) and previous/current populated trace
  families verified. Both scenarios match the prior player checkpoint exactly.
- `build.log`: client/content/source-assets/GObj-consumer targets build. The
  GObj-consumer CTest passes. The first owner fixture also passed; the final one
  adds complete empty/full-chain and physical-capacity coverage.

Target Save averages 1.51 us stock and 1.57 us interpreted; Validate averages
0.46/0.43 us. These ten observations per scenario are component measurements,
not complete-match or worst-case rollback timing.

Remaining: main camera smoothing/pause/orbit/quake and HSD camera objects,
stage/world/HUD/effect/dynamics globals, other heaps/payload ownership, resident
leases, cross-owner candidate graph agreement, aggregate restore order/history
and full-match differential/performance acceptance. Production rollback still
uses the broad page oracle; total bytes and the fifteen-tick deadline are unproven.
