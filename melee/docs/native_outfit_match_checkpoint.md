# Coordinated outfits in room matches

Checked on 2026-10-18. This pass follows received outfits from character select
through remote caching, canonical match slots, rollback and Results.

## Selection and caching

Character select now publishes the selected outfit alongside its fighter,
including provisional selections before the player readies. Previously the
local card showed the outfit while peers received only the fighter identity.
Peers can fetch both packages before readiness. Automatic downloads remain
outside their installed collection.

Room-player collection and final match construction now accept custom fighter
selections with separate outfit packages. Both previously rejected this valid
combination. Custom selections still require zero stock-character/color IDs;
the outfit loader checks the exact parent and every family member.

The packet and match-builder tests preserve outfits across all four canonical
slots, including mixed stock/custom selections. The three relevant CTests pass:
`native_lobby_selection`, `native_lobby_match` and `native_match_selection`.
Log: `.local/native-outfit-match-units-1.log`.

## Particle reset at scene changes

The first live match attempt reached match construction but the Zelda client
refused the `particle-controls` snapshot owner. Particle allocator reset cleared
live lists while leaving the previous practice scene's sorted-list cache.
Those cached pointers could refer to destroyed allocation records.

Particle reset now clears both sorted-list caches and resets their display
serial. This keeps the existing snapshot reference checks intact. The focused
regression fails on the old reset and passes after the change. The complete
controlled snapshot-owner fixture passes 88,770 checks, including 183 particle
control checks. Frozen inputs, commands and results are recorded under:

- `A:/FreakMelee-test-archives/outfit-particle-reset-oct18-red`
- `A:/FreakMelee-test-archives/outfit-particle-reset-oct18-green`

## Live batch

`tools/review_native_outfit_matches.py` passes these three cases with copied
recipient profiles, hidden muted clients and an isolated local signaling service:

| Match | Canonical human slots | Fighter objects |
| --- | --- | --- |
| Fox versus Zelda/Sheik | One per PC | Three, including the dormant form |
| Zelda/Sheik versus Popo/Nana | One per PC | Four, including the dormant form |
| Four Popo/Nana slots | Two per PC | Eight active actors |

Both clients load the same outfit IDs in canonical slot order. Inspection checks
each actor's actual costume-joint pointer against the selected model. Zelda
changes to Sheik with both outfit members retained. The four-slot case uses
model variants 7, 2, 2 and 7, confirming independent outfit choices for repeated
instances of the same fighter family.

Each case exercises delayed input delivery, checkpoint restores, peer checksums,
one-stock match completion, Results and return to CSS. Both peers have identical
final frame numbers, fighter digests and RNG state. Eight automatically fetched
packages across the first two cases remain cached and uninstalled after return.
All clients run without the original author's draft projects. The fixture edits
a small material texture; full-body outfit design remains visual authoring work.

The full batch records two passing cases and a shared-PC Results driver failure:
the driver pressed START only on the first local controller. Melee requires each
local human to confirm. After correcting the driver, the targeted four-slot
rerun passes. Receipts preserve both the failed driver boundary and the rerun:

- `.local/native-outfit-matches-6/receipt.json`: the first two complete cases.
- `.local/native-outfit-matches-7/receipt.json`: the four-slot rerun.
- `.local/native-outfit-match-final-audit.json`: matching executable identities,
  final peer endpoints and cache-only installation boundaries across all cases.

The driver selects stages with controller input and reads their current positions
during Melee's entrance animation. Earlier attempts aimed at an icon's initial
moving position and stopped at the wrong stage. Gameplay actions use held inputs;
the earlier short synthetic pulse did not trigger Zelda's transform. These driver
failures do not establish game behavior defects.

The broader room, relay, other-PC and release gates remain in the project plan.
