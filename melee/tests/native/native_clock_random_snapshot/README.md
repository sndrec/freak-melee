# Input-clock and random-state restore

`run.py` freezes the actual scheduler, random generator, controller/rumble owner
and headers. It compiles these units with controlled physical input, audio and
replay boundaries. No client, window or audio device is opened.

The fixture restores a half-speed phase together with a real queued controller
sample and seed, then reproduces 24 scheduler/RNG ticks including game-speed
changes, stalled deadlines and skipped phase updates. All controller records,
clock fields and seed must match. All three public RNG paths run. Additional
cases check malformed cadence/boolean/reserved fields, overlap, external seed
binding refusal, seed zero/overflow, stopped cadence and snapshot refusal during
the actual initialization and sample callbacks. Restore cannot call devices,
audio polling or constructors.

Run in an x64 MSVC environment:

```
python tests/native/native_clock_random_snapshot/run.py --compiler C:/Users/loser/scoop/apps/llvm/current/bin/clang.exe --output A:/FreakMelee-test-archives/clock-random-fresh
```

This is component restoration, not whole-match resurrection, real wall-clock
netplay sampling acceptance or aggregate rollback performance.
