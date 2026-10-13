# Real eight-actor rollback evaluation

`--rollback-benchmark` uses the normal `gm_801A4D34` game loop and the production
rollback session/page/audio/input implementation. It warms up 600 advancing
ticks, then measures 64 consecutive windows. Each window restores 15 ticks,
executes their complete simulation and HSD draw callbacks without presentation,
verifies the exact and address-normalized endpoint hashes, then executes one
new simulation/draw/presentation tick. The existing eight-tick probe is unchanged.

All four slots must be CPUs in a hidden, muted 99-stock match. At each measured
tick's entry, simulation exit and drawing exit, the live fighter registry must
contain exactly eight active actors, with one primary and one secondary per port,
matching the actual Player entity pointers, selected actor kinds and full spawn
numbers. Missing/dead/replaced Nana invalidates the run. The benchmark does not
substitute callback loops or postpone measurement until the pair respawns.

The wall-clock budget is 16.666667ms for restore + 15 replay ticks + one advancing
tick. Restore includes the actual page restore and controller-file rewind.
Replay timing includes input polling, all 15 simulations, required drawing,
snapshot/audio updates and inter-tick loop overhead. Forward timing starts before
snapshot begin and runs through real Aurora end-frame and snapshot/audio end.
Simulation and draw subcomponents are additionally reported. Draw includes
Aurora begin/end calls and HSD/GX work. Actor checks are part of measured overhead;
full endpoint hashes are excluded, with their reference/validation costs reported
separately. The verification gap means total is a sum of work durations, not a
claim that this diagnostic itself presents at 60Hz. No per-window logging happens
until all 64 windows finish.

Additional per-window fields separate session snapshot begin, session end
(page unprotection and audio reconciliation), and the input polling phase for
replay/forward ticks. Existing dirty-page counts and input poll/zero-queue wait
counts identify whether the unexplained duration lies in snapshot work or the
input path. Input polling includes Aurora events, real controller sampling,
replay reads and their semantic state digest, and device/audio polling. These
fields are nested within total wall time, not added to it. No write-fault handler
timer is installed, so page fault cost remains in its containing input, simulation
or render phase.

These are CPU wall durations through the real renderer submission/presentation
path, including any wait it performs. There is no separate GPU timestamp or
explicit GPU completion fence. Do not describe the result as standalone GPU time
or as proof of sustained paced scheduling. PASS means all windows reproduced
their endpoints with eight live actors; deadline misses remain separate metrics.

Timing/control records are fixed storage outside `.mstate`, and immutable code is
not snapshotted. Mutable interpreter state must use existing fighter/article
owned state or an explicitly registered gameplay region. The interpreter must
execute through the real callback slots and must retain all drawing state writes.

Prepare the stock baseline without launching anything:

```powershell
python tests/native/interpreter_evaluation/benchmark/prepare.py --output A:/FreakMelee-test-archives/eight-actor-rollback-stock --content-root A:/FreakMelee-test-archives/eight-actor-stock-content
```

The runner writes a deterministic seed and 2,000 connected neutral controller
samples; the real CPU/follower logic chooses actions. Add `--run` only after the
coordinating agent builds and authorizes the client batch. It launches with
CREATE_NO_WINDOW and the generated command always contains `--hidden --mute`.
Use fresh output for every run. `--fighter <id>` applies an installed paired
custom fighter to all four CPU slots through an isolated content root. Match the
same seed and inputs for generated-C and interpreter candidates. CPU callback
coverage remains workload-specific; use a separate untimed coverage/parity batch
to inventory reached callbacks, helpers and VM instructions.

The runner hashes the executable, fixture/source and candidate content before
execution, validates 64 ordered records and the endpoint-verification summary,
and stores the full game log/receipt. It never installs content or launches a
visible window. A nonzero exit, state divergence, actor loss, changed inputs or
incomplete recording is failure, never a timing result.
