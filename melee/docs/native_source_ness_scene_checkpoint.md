# Actual interpreted Ness scene and grounded moves

The October 10 batch brings the prepared Ness source packet into a real rendered
match. The eight-scenario stock/source comparison passes idle, all four grounded
specials and all three smashes, using normal controller input rather than forced
move entry. The full Ness clone is not yet accepted for every gameplay scenario.

## Shared scene corrections

The initial batch failed before actor creation at secondary-model preparation.
The real Yo-yo tip contains an allocated custom TEV record with zero active
channels. Native `TObjMakeTExp` skips `MakeColorGenTExp` in that case; the record
adds no custom expressions or constants. The census now follows that condition,
while retaining ordinary texture blending and the record's construction cost.
Active unsupported shader families remain rejected. No descriptor was rewritten
and no native rendering formula changed.

The next batch reached resource-graph planning. Its worst-case class-piece
requirement was 47,553 slots (33 pieces x 1,441 construction/live slots), exceeding
the former generic 16,384-slot ceiling. The ceiling is now 65,536; pools still
reserve only their requested size during scene setup and never grow during a
tick. The real tip's full 16 x 90 graph layout measures 23,847,352 bytes with
32-byte alignment. This is conservative reservation, not measured live usage.
Reducing that reservation and completing compact match snapshots remain open.

Rejected census/material/capacity paths now report their actual planning reason
instead of only the outer scene assertion. They retain rejection/unwind behavior.

## Yo-yo ownership correction

Once scene setup passed, six scenarios matched stock; both Yo-yo smashes faulted
at the same `holder.motion` query. The accessory is not a held item. Native
`itNessyoyo_UnkMotion3_Anim_inline` checks `ip->owner`, retires when absent, and
keeps the article only within the Yo-yo motion range. Native link-length and
model-scale reads also use `ip->owner`.

The script now uses `current_owner` for those motion/scale reads, checks owner
presence before reading motion, and uses the same presence test before calling
the owner cleanup export. No receiver check was relaxed, no stock handler was
delegated to, and no fighter-specific engine primitive was added.

## Evidence and limits

- Seven Windows targets build and the two focused script/parameter CTests pass.
- Actual imported Link/Young Link Arrow and Ness Yo-yo graphs pass 1,116 console
  checks: 36 ordinary/reserved constructions, 28 matrix visits, six replacement
  commits, six exhaustion preflights, twelve ID cases and three environment
  rejections. Full Yo-yo resource size planning also passes. This cohort does
  not create 1,440 live graphs or verify rendered pixels.
- The pool/snapshot console cohort passes 153,015 checks, including actual
  47,553-slot allocation/exhaustion, high-slot reuse, zero initialization,
  stale-generation rejection and complete release/validation. It does not
  establish whole-match compact save/load or its performance target.
- The revised canonical Ness audit accepts all 418 functions and 92 imports
  with zero diagnostics and 10,691 instructions.
- Sixteen hidden, muted, rendered games consume 360 samples each. The final
  eight stock/source pairs have equal action timelines, every emitted Fighter
  field, visible Item fields and detailed Item field traces. Every requested
  move is exercised, every source actor is observed, and no script fault occurs.
  The runner requires complete P1 trace coverage and input EOF.

These are neutral-opponent grounded scenarios, not complete state-byte or pixel
parity. Synthetic inputs do not verify replay RNG/digests. Airborne/interrupted
moves, charged release, damage, CPU policies, reflector/absorber interactions,
match-end and rollback through these moves remain separate acceptance work.

## Receipts

Under `A:/FreakMelee-test-archives/`:

- `source-secondary-yoyo-inactive-tev-oct10-b/receipt.json`
- `match-pool-large-class-pieces-oct10-b/receipt.json`
- `ness-yoyo-owner-package-oct10-a/receipt.json`
- `source-ness-ground-oct10-d/receipt.json` (all eight pairs pass)
- `ness-scene-ground-oct10-final/receipt.json` records closure pins and logs.

Earlier `source-ness-ground-oct10-a`, `-b` and `-c` receipts preserve the failed
scene and owner-query attempts; they are not passing acceptance evidence.
The first large-pool fixture incorrectly expected reused payload to retain old
data; it was corrected to verify the production pool's zero-initialization
contract before the passing rerun.

Final private packet ID:
`3cd7f26e6390561ef0b12cc6fcac1a5a9514ae965f9c60c124b4c2acff589160`.
It contains source and real copied DAT/CPU assets, with a packaging-only
placeholder portrait. Only private test stores were accepted; the user's
installed collection was not changed.
