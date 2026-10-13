# Longer repeated compact rollback experiments

The subsequent per-frame sixteen-slot implementation and live directory tests
are documented in [native_compact_match_history.md](native_compact_match_history.md).
The receipts below cover the earlier sequential retained-capture checkpoint.

The live snapshot runner accepts `--depth 1..15` and `--rounds 1..64`. Defaults
remain eight frames and one interval. Every interval starts at sample
`240 + 120 * interval`; the input stream must extend beyond the last catch-up.
The native diagnostic reads `FREAK_MELEE_SNAPSHOT_REWIND_DEPTH` and
`FREAK_MELEE_SNAPSHOT_REWIND_ROUNDS` during initialization and rejects malformed,
zero or out-of-range values. Its final summary requires every requested interval
to have completed. Fifteen frames fit the existing sixteen-frame audio journal.

This repeatedly reuses the same transition-reserved compact and oracle storage.
Each later boundary retains a new own snapshot, advances through normal gameplay,
restores the earlier compact state and resimulates with drawing retained. Every
interval has a separate exact intermediate trace comparison, 37 compact record
comparisons and an independent complete raw endpoint comparison. Original endpoint
recovery happens after comparisons; continued traces must match the reference.

`--correction --depth 15` additionally runs both corrected passes against a fresh
corrected-input run. Correction still accepts only one interval: the diagnostic
recovers the predicted endpoint afterwards, whereas the fresh corrected run keeps
its corrected state. Multiple corrected intervals need a different continuation
contract and are rejected rather than compared against a mismatched oracle.

These are sequential older-boundary experiments, not a production history ring
or a fifteen-frame performance benchmark. No page-fault tracker participates in
compact resimulation, but the broad oracle scanning and reporting still dominate
these diagnostic runs. No timing claim may be inferred from their wall time.

## Item population and fighter-reference control ownership

The first six-interval run had exact intermediate traces and compact endpoints,
but raw differences appeared at the fifth stock interval and sixth interpreted
interval. PDB section-relative symbols traced them to `Item_804A0C64` and the
fighter reference-count array previously named `ft_8045996C`.

`Item_804A0C64` is numeric item population/cap bookkeeping despite its misleading
`HSD_ObjAllocUnk` type name. Its counts determine whether some items can spawn.
The fighter array is incremented/decremented through `ftData_80085560`, which
asserts against negative reference counts. It is now named
`ftData_CharacterReferenceCounts` at its definition and every direct reference.
The source's increment, reset and assertion behavior is unchanged.

The existing VS/item/HUD component captures both entire count sets alongside the
previous next-item serial. Buffer-independence checks exclude their live storage.
Candidate validation rejects negative fighter references before publication.
Load copies the numeric controls directly, without resource acquisition, removal,
constructors or gameplay callbacks. This does not establish complete resource
ownership or general asset residency; those remain aggregate requirements.

The controlled fixture changes population counts and fighter references after
Save, requires their exact restoration, and verifies malformed negative reference
counts refuse without mutation. Original failing live receipts remain available
as diagnostic evidence; raw differences are never masked out.

## Acceptance evidence

Evidence is stored under `A:/FreakMelee-test-archives/`:

- `compact-history-oct12-six-a`: original failures, stock 2 raw bytes at target
  720 and interpreted 1 raw byte at target 840; compact/trace comparisons exact.
- `compact-history-oct12-correction-a`: fifteen-frame correction passed before
  the additional control ownership fix, retained as an earlier source checkpoint.
- `compact-history-oct12-owners-b`: controlled owner suite, 86,874 assertions.
- `compact-history-oct12-six-b`: PASS, 1,200 samples per stock/interpreted case,
  six fifteen-tick intervals each (targets 240, 360, 480, 600, 720, 840).
  Every intermediate trace, all 222 compact endpoint records per case and all
  36 raw endpoint regions per case match. GObjs are created, destroyed and their
  physical slots reused within the intervals, including interpreted cases.
- `compact-history-oct12-correction-b`: PASS, 1,200 samples per case, two
  fifteen-tick corrected passes each. Fresh corrected traces and repeated compact
  endpoints match; full raw endpoint differences and new/canceled repeat audio
  starts are zero. Recovered predicted continuation matches independently.
- `compact-history-oct12-independent-verification.json`: all source/executable
  pins, all fourteen final compact/raw endpoint comparisons, exact interval
  traces, actual corrected P1 motion changes and predicted continuation checked.
  The complete fighter-data source remains identical after normalizing its one
  renamed global. Across both cohorts, 240 actual resimulation ticks run and
  518 compact endpoint records compare, with 84 complete raw regions identical.

Production integration, full candidate prevalidation, allocation/residency
closure, broader match lifecycle/output reconciliation and the fifteen-tick
eight-actor deadline remain required. Passing these experiments does not complete
the overall rollback or project goal.
