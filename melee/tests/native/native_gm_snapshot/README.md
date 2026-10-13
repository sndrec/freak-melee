# GM boundary restore

The fixture runs the production `gm_801A4D34` match loop, GM mapping/repeat
logic, LB input cadence, controller/rumble, RNG and player record bodies. At frame eight it
captures those components; at frame 24 it rewinds and requires the repeated
16-tick window to produce exactly the same component records. Pause/frame-step,
scene callback, process and drawing boundaries run in their normal loop order.

Hardware, host UI/network/profiling, audiovisual endpoints and object scheduler
callbacks are controlled in `boundaries.c`. No client, window or audio device
opens. This proves loop-owner restoration, not full-game simulation or performance.
Live census separately runs actual stock and interpreted eight-actor clients.

Camera targets run their actual allocation/reset/release methods alongside
these components. All 70 requested records, including free history and stale
`next` fields, rewind and repeat exactly. Malformed topology (foreign/interior
or out-of-prefix pointers, cycles, duplicate/missing nodes, bad active reciprocity
and tail), epochs, count, overlap, activity and last-record corruption reject
without writes. Empty/full active lists, unaligned candidate input, equal-address
scene retirement and physical capacity endpoints are covered. Fighter/item/stage
borrower agreement and the main camera's pose/smoothing/quake objects remain
separate aggregate work.

Malformed candidates and snapshot attempts inside actual scene/process/draw
callbacks must reject without writes. A freed/reused scheduler-object slot
rejects Load until the actual pool component restores the original generation.
Same-pointer loop re-entry retires old history. Mapped-controller repeat/macro
state repeats a 128-frame sequence exactly.

Player records additionally restore stocks, spawn positions, coins, falls,
stale queues, both attack-instance counters and extended statistic bytes. Eight
fixture actors use the production typed fighter pool; GObj payloads and actor
construction are controlled, so this does not prove whole-game graph recovery.
Both GObj and Fighter freed/reused generations reject before any player write;
restoring companion pools permits the original root generations again. Family
mapping/common-data changes, malformed roots/role permutations/callbacks and
loop retirement reject. Stock counter wraparound is preserved.

Run in an x64 MSVC environment:

```
python tests/native/native_gm_snapshot/run.py --compiler C:/Users/loser/scoop/apps/llvm/current/bin/clang.exe --output A:/FreakMelee-test-archives/gm-snapshot-fresh
```
