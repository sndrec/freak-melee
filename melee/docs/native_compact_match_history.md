# Compact match history storage

`native_match_history.[ch]` owns a fixed directory of at most sixteen frame
boundaries in caller-reserved host memory. Each slot has an exact frame identity,
published byte count and 39 compact component records. Ring wraparound cannot
make an old frame resolve to a newer snapshot at the same physical slot.

Bind validates capacity, eight-byte alignment, multiplication/address overflow,
backing size, scene identity and metadata/backing separation before changing the
directory. Callers must reserve genuinely host-owned storage, independent of
engine owners. Bind is a transition operation; it does not initialize game state.

Save requires the same scene epoch and consecutive boundaries (or replacement of
the newest boundary). It invalidates the destination before serialization and
publishes only a complete successful capture. Load checks frame and scene identity,
loads the immutable own capture, then invalidates every future frame. Saved older
frames remain available until overwritten by ring wraparound. Every later
resimulated boundary replaces the discarded prediction in its normal slot.

There is no allocation, file operation, callback, tick or rendering in the history
module. Capture/load use the existing direct compact owner API. Frame-directory
bookkeeping, scene ownership, publication and invalidation are the new behavior;
the page snapshot implementation is not wrapped or used by this module.

## Failure and lifetime contracts

A serializer refusal poisons history and leaves the destination unpublished.
A loader refusal also poisons history. The compact loader can still partially
mutate the world after a late refusal; the caller must abort or recover through
an independently known world. Discard invalidates all slots after an external
known-world recovery but does not clear failure. Rebinding requires an appropriate
known scene boundary, not an assumption that the failed world remained unchanged.

Snapshots require resident scene/package definitions and the engine's stable
physical pool/class bindings. The directory's epoch/frame checks do not prove
complete resource ownership or candidate-wide prevalidation. Borrowed slot pointers
must never be retained beyond overwrite/retirement; callers load by exact frame.

## Live integration and evidence

The offline snapshot runner's `--history` enables capture at every complete
post-draw boundary from sample 120 onwards. It reserves and prefaults sixteen
8 MiB slots during startup (128 MiB total backing); current payloads occupy only
the published compact byte count. The 8 MiB slot is an experimental bound, not a
proven maximum for all fighters/stages/items or the eventual production policy.

Every older-boundary load and replay reset now uses the history directory, including
both corrected passes. The experiment verifies all future frames are unavailable
after restore and a wrapped stale frame is refused. Correction's broad predicted
recovery discards corrected snapshots before saving the recovered boundary; it
cannot expose a corrected history beside a predicted world.

Evidence under `A:/FreakMelee-test-archives/`:

- `compact-ring-oct12-six-a`: PASS, stock and independent interpreted eight-actor
  matches, 1,200 samples each. Each captures 1,171 boundaries into the ring and
  performs 12 directory loads over six fifteen-tick rollback intervals. Every
  intermediate trace, all 222 compact endpoint records and 36 full raw regions
  per case match the independent reference.
- `compact-ring-oct12-correction-a`: PASS, 1,200 samples per case, 1,112 ring
  captures and three directory loads each. Both fifteen-tick corrected passes
  match a fresh corrected run, repeated compact/full raw endpoints match, and
  recovered predicted continuation is exact. Repeat audio has no new/canceled
  starts.
- `compact-ring-oct12-independent-verification.json`: unchanged source/executable
  pins, ring counts, every intermediate/continued trace, every compact owner and
  all six raw regions per endpoint verified separately.
- `compact-ring-oct12-unit-tests.log`: replay test and 173 directory/failure
  assertions pass. The directory test uses a tiny fake serializer/loader to test
  wraparound, stale/scene refusals, future invalidation, publication, rebinding,
  capture failure and simulated post-mutation load failure. It does not claim
  engine serializer or gameplay proof; the live cohorts provide that evidence.

Production netplay remains on page history. Its boundaries are captured before
input/simulation, while this experiment captures complete post-draw boundaries.
The phase contract, full candidate prevalidation, allocation/asset residency,
match retirement, output reconciliation and the fifteen-tick eight-actor
performance deadline still need acceptance before switching production.
