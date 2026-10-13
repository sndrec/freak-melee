# Interpreted companion capture and results

The full Popo/Nana family now completes CPU-driven damage/capture scenarios and
draws both members in win/loss results. This extends the
[grounded special checkpoint](native_source_companion_specials_checkpoint.md).
It is not complete paired gameplay or compact rollback acceptance.

## Capture ordering

The first CPU batch exposed native access violations in neutral-B and up-B at
samples 320 and 547. Until the crash, every emitted Fighter field and article
field matched stock. CDB located the invalid read in `fn_800DAC78`, called by
the victim's capture-entry callback. The holder's grab anchor contained the
float bits `0x3f3333333ee147ae` rather than a joint pointer.

A second debugger run stopped in `fn_800D9CE8` before setup: stock Mario was in
Catch (212), targeting the interpreted actor, whose reciprocal `victim_gobj`
was still null. `Fighter_UnkProcessGrab_8006CA5C` invokes the holder's `grab_cb`
before the victim's `grabbed_cb`. The source-lifetime guard required a mutual
capture link too early, returning before anchor initialization and CatchPull.
The subsequent victim callback then interpreted stale motion-union data as a
joint pointer.

Holder setup now retains the peer's existing capture relation and checks that
it stays unchanged across nested calls. Physical object/generation, source
program/storage, holder's target and both actor lifetimes remain checked.
Later capture callbacks still require the established mutual link. No script
or generic grab algorithm was replaced, and no invalid-pointer fallback was
introduced.

## Actual CPU regression

The tracked `tests/native/source_companion_game/run.py --scenario cpu-injury`
runs stock and source for all four grounded specials against level-9 Mario:
1,200 input samples, one stock, Final Destination, items off, rendering enabled,
hidden and muted. It checks damage in every case and capture in neutral/up,
matching both actors' action timelines and all emitted Fighter fields. Input
EOF, process exit and VM fault status are independent gates. No desktop input
is sent. Both formerly crashing cases now finish normally.

| Scenario | Damage records per run | Matching Fighter records | Matching Item records/digests |
| --- | ---: | ---: | ---: |
| Neutral | 15 | 1,668 | 72 |
| Side | 17 | 2,340 | 97 |
| Down | 25 | 3,600 | 552 |
| Up | 18 | 3,177 | 231 |

All eight processes exit zero at controller EOF without VM faults. Detailed
Item digests also match, although the runner reports them separately from its
visible-state gate. Gameplay traces end when entering results, which explains
counts below 3,600. These are emitted-state comparisons, not proof of every
mutable field, a restored snapshot or deterministic replay.

## Result model admission

`fn_8017A67C` still required a published C `NativeFighterDefinition` before
creating owned results actors. Ordinary source packages intentionally have no
such runtime definition. The admission gate now requires the actual published
`SourcePackage` for each selected member, retaining decoded result-clip checks
and alternate/companion selection rules.

Separate hidden/muted realtime win and loss games consume 900 samples each,
exit zero, and report no missing-result-clip warning or VM fault. Native IPC
screenshots at 10/13 seconds show both Popo and Nana in the winner display or
loser's panel. Both result actors remain interpreted and tick. The long custom
name still clips the placeholder result heading; native text/layout polish is
separate. Screenshots were inspected, not treated as pass markers alone.

## Other exercised cases

An earlier 18-game batch compares air entries for all four specials, left-facing
ground/air Belay, recovery near an edge and stock loss. Action timelines and
all emitted Fighter/Item fields and detailed Item digests match. Its nominal
"injury" scenario caused no damage, so it is not injury evidence; the CPU batch
above replaces that gap. The leftward Belay cases kill Popo, not a demonstrated
Nana-death scenario. These receipts pin the executable used at the time; the
results and capture fixes were built afterward.

## Retained evidence

All paths below are private test archives on A:, outside the installed collection.

- Tracked CPU regression: `A:/FreakMelee-test-archives/source-companion-cpu-injury-regression-oct10/receipt.json`.
  SHA-256 `772b54326e7d141f9b5a37bd29fa96a9e2b3558de4d7bea089e802939e5c7459`.
- Initial failing CPU comparison: `A:/FreakMelee-test-archives/source-companion-cpu-injury-oct10/receipt.json`.
- Crash and ordering debugger logs: `source-companion-injury-crash-oct10/debug.log`
  and `source-companion-grab-order-oct10/debug.log` under the same archive root.
- Win/loss images and logs: `A:/FreakMelee-test-archives/source-companion-results-oct10/receipt.json`,
  SHA-256 `215838dfc33a419e0a31c06557a55548f98698e2cc7396e929698880199c2db6`.
- Air/left/edge/death comparisons: `A:/FreakMelee-test-archives/source-companion-extended-oct10/receipt.json`,
  SHA-256 `dbc3e76fb02659076a4133480535ec78ebf976d7420f0dcdc073460942cf29ce`.

The native client, content tool and family/package integration targets build.
The source family package remains
`adaaac9faca59e652b45789f972577c48ecf0c27b16a9b6cb95ab8be16538062`.
No installed package was changed. Broader companion death/separation and all
linked interactions remain acceptance work. Whole-match compact save/load is
still open; Regions/Pages remains the broad oracle.

## Historical fighter migration census (2026-10-10)

The following preparation census predates the current interpreted roster fixes.
The [single-fighter CPU checkpoint](native_source_roster_cpu_checkpoint.md) now
records all 22 Single templates passing normal package creation and a gameplay
cohort; [family gameplay](native_source_family_gameplay_checkpoint.md) records
the paired forms. The historical census below is not current migration status.

A fresh private clone/source-check census covers the other sixteen non-Kirby
fighters: Captain Falcon, Ganondorf, Luigi, Jigglypuff, Fox, Falco, Marth, Roy,
Donkey Kong, Bowser, Pikachu, Pichu, Samus, Mewtwo, Yoshi and Ness. None currently
passes ordinary source preparation. First diagnostics include obsolete call
signatures, unsupported bindings, conflicting motion callbacks, source-owned
article metadata and expression/layout mismatches. Samus fails independent
clone construction before source-check. Former generated-C checkpoint results
do not establish admission or gameplay under the current interpreter frontend.

Census: `A:/FreakMelee-test-archives/remaining-stock-source-census-oct10/receipt.json`,
SHA-256 `598c5b2a41dc65915c5fc83199a6c3e496d3e73256ac9ca5b8db74f42711e636`.
Existing complete source examples still require their broader actual gameplay
acceptance; preparation alone is not completion. Kirby remains explicitly skipped.
