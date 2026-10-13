# Compact corrected-input experiment

`tests/native/native_snapshot_census/run.py --correction` runs four Ice Climbers
pairs with human P1 and three CPU ports, for eight actual fighter actors. It
uses the stock family and the accepted independent interpreted family separately.
Every client remains hidden and muted, keeps normal HSD drawing, and uses fast
pacing. This is correctness evidence, not a rollback performance benchmark.

The predicted input stream is neutral. The corrected stream differs only at
samples 240 through 247: P1 presses jump on sample 240 and holds right for all
eight samples. The initial header, all other pad fields and all other samples
are identical. A fresh corrected-input run supplies an independent trace oracle.

At boundary 240 the diagnostic retains compact A. At 248 it retains predicted
endpoint B as a complete recovery guard, loads only compact A, and consumes the
corrected stream through the actual game/render path. This produces corrected C.
It retains compact C and a complete C comparison oracle, restores compact A a
second time, and repeats the corrected interval to produce D. No broad snapshot
is loaded before or during either corrected pass. The first corrected endpoint
is expected to differ from B; D must reproduce C exactly.

All 37 compact components and all six complete raw endpoint regions are compared.
Both corrected intermediate traces must equal the fresh corrected run, including
actor/companion pose, motion, physics, hit/hurt state, items/resources, events and
RNG. Afterwards B and the original replay/observer cursor are recovered and the
continued predicted trace must equal its independent reference.

Replay alternate input files are opened before sample consumption. They require
inputs-only playback and identical initial headers. Stream switching seeks to the
current sample without consuming skipped inputs. Rewind operates on whichever
stream is active. Both files are closed at replay retirement. No file is opened
and no new scratch is allocated during correction.

The audio journal allows legitimate first-correction changes. The second pass
must reuse the corrected sound intents without new or canceled starts. This
proves repeated correction handling for this interval, not every possible sound
replacement or other output such as rumble.

## Item serial ownership fix

The first experiment matched every compact component and all intermediate traces
but differed in two stock endpoint bytes and seven interpreted endpoint bytes.
PDB section-relative symbols identified the missing global as `it_804D6D10`:
the next native Item identity, incremented by `Item_CreateItem` even when an item
is removed before capture. Its changed value propagated into a freed native item
and inactive script item identity/storage fields. These bytes were not masked.

The existing VS/ambient-item/HUD control component now captures and restores that
counter directly. It excludes the counter from permitted snapshot backing and
keeps the source increment arithmetic unchanged. The controlled owner fixture
explicitly advances the serial after Save and requires Load to recover it.

## Evidence and remaining boundaries

Evidence under `A:/FreakMelee-test-archives/`, October 12, 2026:

- `compact-correction-oct12-eight-a`: retained failed experiment, two/seven raw
  bytes differing despite exact compact components and reference traces.
- `compact-correction-oct12-eight-b`: PASS, 1,200 input samples per stock and
  interpreted case, two corrected eight-tick passes. Twelve compact owners differ
  from predicted B; all 37 repeat exactly. Complete raw C/D difference is zero.
- `compact-correction-oct12-owners-b`: PASS, 86,868 controlled owner assertions,
  including direct item-serial restoration and existing malformed/refusal checks.
- `compact-correction-oct12-same-input`: PASS, 480 samples per stock and
  interpreted eight-actor case; the prior same-input eight-tick experiment still
  reproduces every compact component, intermediate trace and full raw endpoint.
- `compact-correction-oct12-independent-verification.json`: source/executable
  pins, exact changed-pad extent, unchanged reference prefix, actual P1 motion
  changes, fresh/first/second corrected traces, all raw regions and recovered
  predicted continuation checked independently.

This is still a single bounded offline interval. Deeper/repeated history,
allocation/residency boundaries, full candidate prevalidation, production history
integration, general output reconciliation and the final fifteen-tick eight-actor
performance target remain open. The production snapshot path is still broad.
