# Input-clock and RNG ownership

2026-10-11 checkpoint. Two explicit components now restore the actual scheduler
in `lb_0195.c` and generator in `random.c`. They complement controller queues and
script state; production rollback still uses the broad page oracle.

## Scheduler state

`NativeInputClockSnapshot` owns 56 bytes: game/sample periods, next sample
deadline, virtual clock, both phase accumulators, both update gates, sampled,
scheduled and running flags. This is the existing native scheduler's complete
logical state, including cadence. It is not a second scheduler or per-tick
allocation. Save/Load operate directly on the actual private `input_clock`.

The raw controller queue and normalized button history remain a separate
companion. Restoring a phase without its corresponding queue would be incomplete.
The GM pause/frame-step flags, process masks and simulation/draw counters are
also separate remaining ownership work. No complete tick restore is claimed.

Typed validation checks exact size, canonical booleans/reserved bytes, the
relationship between game and sampling periods and both phase bounds. The
pre-initialization zero clock and initialized stopped clock are legal. Validation
precedes all writes. Buffer/counter overlap, short capacity and capture/load
inside the real initialization or raw-input callback reject. Activity is not
serialized: every accepted idle boundary observes zero. The existing broad
oracle likewise sees zero at those boundaries.

Load neither initializes the scene nor reads pads nor polls audio. Deadlines and
virtual time restore exactly; no wall-time rebasing or new sampling behavior is
introduced. Live wall-clock netplay resampling/resimulation still needs aggregate
acceptance. This checkpoint cannot establish that restoring old physical-device
deadlines alone constitutes correct rollback input delivery.

## Generator state

`NativeRandomSnapshot` owns the actual 32-bit seed plus a zero reserved word
(8 bytes). All public RNG operations keep their existing arithmetic and mutate
the same generator. Seed zero is legal; Load does not replace it with a nonzero
value. Ordinary HSD drawing and its RNG calls must still run during resimulation.

The source assignment audit finds only `random.c` binding `seed_ptr` to its own
static-storage seed, including `_HSD_RandForgetMemory`. Native match/replay code
writes the pointed value, not the pointer. Save/Validate/Load require that actual
binding. An external target rejects rather than appearing captured while its
mutable value is omitted. A future alternate seed-binding feature must add
explicit target ownership before this component can support it. The snapshot
does not accept or dereference a serialized pointer.

Buffers overlapping either the actual seed or its binding reject. Load validates
length/reserved bytes before mutation and calls no random operation, allocator,
callback or device. Match/scene lifetime retirement belongs to the future
aggregate; these scalar owners do not manufacture a new scene epoch.

## Evidence and limits

Artifacts under `A:/FreakMelee-test-archives/clock-random-oct11-*`:

- `owner-b`: 255 checks in actual production scheduler/RNG/controller/rumble
  units with controlled external devices. Save/Load reproduce 24 subsequent
  ticks, including half/double-speed transitions, stalled deadlines and skipped
  phase updates. Final clock, RNG and controller records match exactly. Two
  callback contexts refuse capture/load; malformed candidates leave state
  unchanged. Device/initializer counters do not advance during restore. Seed
  zero and integer wraparound match original arithmetic. The existing 600-tick
  input timing fixture also passes, including jitter and stopped sampling.
  `owner-a` was a strict-format-warning failure in the fixture's assert logger;
  it was corrected before accepted evidence.
- `eight-a`: four hidden/muted rendered 1,200-sample runs, stock and interpreted
  four Ice Climbers pairs/eight actual actors, diagnostic on/off. All 21 owners
  capture/validate at ten boundaries per scenario. Clock/RNG sizes are 56/8 bytes.
  Observed mean saves are 0.31/0.16 us stock and 0.23/0.13 us interpreted, with
  independent query/validation costs. These tiny diagnostic samples are not
  complete save/load performance or a rollback deadline measurement.
- `independent-verification.json`: all 1,615 frozen and original fixture pins,
  31 live pins and actual exit/log receipts verified. Every populated previous/
  current gameplay trace family matches exactly; diagnostic on/off also matches.
- `build-a.log`: native client, content tool and source-assets fixture build.

Remaining: GM scheduling and match globals, world/camera/HUD/effects/dynamics,
other OS heaps and mutable payloads, resident-definition/cross-owner closure,
aggregate prevalidation/load ordering/history and real restore/resimulation
timing. Neither total match size nor sub100us full restore is established.
