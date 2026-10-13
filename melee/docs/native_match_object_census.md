# Native object ownership census

The opt-in `--object-census` diagnostic observes real native allocator/class
registries and the game object's p-link, GX and procedure priority/owner lists.
It validates bounds, cycles, ownership, tails and allocator population. Samples
use fixed storage, do not allocate or format output, and do not write game state.
Scene entry/exit and close flush an explicit untimed report. Its mutable counters
belong to excluded host state, not rollback state.

`FREAK_MELEE_OBJECT_CENSUS` sets an output file before launching. The mode rejects
rollback benchmarks and online sessions; its timings cannot be used for
performance acceptance. All-CPU offline direct matches are supported for this
diagnostic without changing the normal human-player launch rule.

## Eight-actor observation, 2026-10-07

The actual client completed 2,000 gameplay input samples with four level-9 CPU
Ice Climbers slots on Battlefield, items off and 99 stocks. All eight Fighter
instances were counted. The diagnostic made 2,001 observations including scene
construction; process exit was zero with no timeout or topology errors.

| Observation | Peak |
| --- | ---: |
| Live GObj | 111 |
| Scheduled GObjProc | 196 |
| ObjAlloc families | 21 |
| HSD classes | 16 |
| Object classifiers | 16 |
| Counted allocator payload | 771,256 bytes |
| Counted HSD class payload | 624,400 bytes |
| Combined counted payload | 1,388,376 bytes |

The observation helps identify ownership and capacity; it is not a complete
mutable snapshot. It includes scratch/immutable data and excludes some dynamic
satellites, static/global state, backing allocation bookkeeping and assets.
For example the 524,288-byte allocator family is Fighter's two 32 KiB x59C/x5A0
buffers for eight actors; active animation use and reconstruction must be audited
before deciding what to save. Do not replace the former 193 MB tracked-capacity
number with this estimate as if they described the same boundary.

One stage/workload does not establish global capacities. Menu/practice/stage/item
churn still need broader observations before actual pool consumers migrate.
No allocator path changed in this checkpoint.

Live command/binary hash/exit and log:
`A:/FreakMelee-test-archives/eight-actor-object-census-run/receipt.json` and
`census.log`. The console fixture checks 13 observations, 45 assertions and ten
deliberately malformed topologies; evidence is in
`native-match-object-census-final/receipt.json`.


## Corrected live animation census

After the Fighter scratch removal and owner migration, the registry-only count
omitted live animation/satellite families whose descriptors survived registry
head reset. The named-accessor observation completed 2,001 samples with zero
errors and unchanged diagnostic callback counters. AObj peak was 731 controllers;
FObj peak was 2,271 decoder records. Corrected simultaneous partial payload peak
was 1,067,688 bytes across 24 families and 16 classes. The previous 864,280-byte
estimate omitted these owners; neither count is complete mutable match state.

Receipt: `A:/FreakMelee-test-archives/live-script-packed-state-census/receipt.json`
and `census.log`. This observation predates conversion of AObj/FObj to typed
native pools. The current census reads those two real pools directly, retaining
named descriptor observations for Vec/Mtx/RObj/Rvalue/ID. Chosen pool capacities
are bounded reservations with headroom; this one workload is not global capacity
acceptance.
