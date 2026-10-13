# Player record snapshot ownership

2026-10-11 checkpoint. One explicit component, not a complete match snapshot
or replacement for the production page oracle.

## Actual state

`player_state.c` owns the existing six `StaticPlayer` records, fighter-family
mapping, common-data pointer and Player allocator descriptor. Original record
accessors/reset logic live there directly; `player.c` retains actor creation,
teardown, asset imports and engine interactions. No forwarding functions or
second representation. All 172 original function bodies compare exactly with
commit `c7f8b57cd`; 122 moved into the 1,406-line state module. The creation
module is now below 1,000 lines. The existing respawn callback and setter now
use the same typed `s32` function signature instead of converting through
`void*`; its body is unchanged.

The 23,080-byte component copies all six actual records, including:

- Character/slot/team/controller selection and physical transformation roles.
- Stocks, falls, KOs, coins, percent/stamina, spawn and platform positions,
  scale, flags and movement statistics.
- Full stale-move tables, action/hit/bonus statistics and extended tail bytes.
- Both original 16-bit attack-instance counters and their wrap-to-one behavior.
- GObj and Fighter references for both physical members of each player slot.

GObj/Fighter payloads, HSD graphs, interpreted actor storage and HUD objects are
separate owners. `Player_AllocData` belongs to the ordinary ObjAlloc component.
The static `pltrick.c` move-kind table has no source writes. Imported common
parameters and native family mappings are resident definitions frozen per loop.

## Lifetime and restore

The actual GM loop freezes mappings, common-data identity/content and both pool
storage identities at entry, increments a host epoch, and retires this component
before shutdown. Same-pointer loop re-entry cannot reuse history. The actual
GM idle predicate is directly shared: snapshots reject during input/simulation/
drawing and active/deferred HSD callbacks.

Validation checks exact extent, epochs, independent storage, role ordering,
stale queue index, the existing resident respawn callback, pool/slot shapes and
duplicate actor roots. Candidate foreign pointers are never dereferenced.
Structural validation permits older generations: GObj and typed Fighter
companions must restore first. Load resolves every exact occupied generation,
classifier, userdata link, owning player and family member before any write.
Failure on the last member leaves all records unchanged. Save/Load allocate
nothing, invoke no gameplay callbacks/constructors, and advance no devices/audio.

Frozen metadata is outside `.mstate`; actual player globals and their allocator
descriptor remain inside it while the broad oracle is retained.

## Evidence

Under `A:/FreakMelee-test-archives/player-oct11-*`:

- `owner-final`: 2,018 checks run production GM/input/cadence/controller/RNG,
  player accessors, stale updates and typed Fighter pool code. Saved frame eight
  repeats sixteen ticks exactly. Seventeen malformed player candidates,
  callback activity, common/mapping mutation, role changes, extended bytes,
  counter wrap and last-member GObj/Fighter generation resurrection are checked.
  Hardware, scheduler and actor construction are controlled; no full HSD graph
  restoration. Earlier `owner-a` through `owner-d` were fixture compile/linkage
  failures; `owner-e` passed before final strict-owner verification.
- `eight-final`: four hidden/muted rendered 1,200-sample client runs, stock and
  independent interpreted four Ice Climbers pairs/eight actual actors. All 24
  components save/validate at all ten boundaries per scenario. Traces match
  diagnostics on/off and the preceding GM checkpoint. The live census never loads.
- `independent-verification.json`: 1,815 fixture and 49 live pins, log hashes,
  zero exits, populated trace families, donor function bodies and actual compiled
  sections verified. Player Save averages 5.01 us stock and 4.52 us interpreted;
  Validate averages 0.23 us. Ten observations per scenario are not worst-case
  full-match timing.
- `build-final.log`: native client/content tool/source-assets/GObj-consumer
  targets build successfully. The GObj-consumer CTest also passes (0.89 seconds).

Remaining: other GM/world/stage/camera/HUD/effect/dynamics globals, multi-heap
payload ownership, resident definition leases, cross-component graph agreement,
aggregate validation/load ordering/history and complete restore/resimulation
performance. Player roots must agree with candidate GObj/Fighter/VM data; this
owner cannot prove that independently. Production still uses the broad oracle.
Total size and the fifteen-tick deadline remain unproven. Commits remain local.
