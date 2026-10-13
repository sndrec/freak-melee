# Game & Watch copied-base inputs

This read-only census verifies the retail rig, common semantic bindings, article
slots, motion counts and resource names before normal Workshop import is enabled.
It uses the immutable SDK's DAT reader and verifies the source hashes it reads.
No game process, native callback or renderer is executed here.

```powershell
python tests/native/gamewatch_clone_metadata/audit.py --snapshot A:/FreakMelee-test-archives/baseline62 --iso "../Super Smash Bros. Melee (USA) (v1.02).iso" --output A:/FreakMelee-test-archives/gamewatch-clone-metadata62
```

The retail costume has 53 physical joints and 38 semantic bindings, with no
inserted omission joints or material-animation root. Article slots 0–9 follow
the stock load registrations; slot 10 contains outline visibility data and must
never be treated as an article. The imported colors are ordered RGBA bytes,
already preserved by the stock and owned asset layouts.

The live article import table is checked separately because SDK62 predates its
Game & Watch entries. These entries allow the ordinary parameter importer to
read each prop's tuning into owned named parameters. This does not enable a
default clone, supply its behavior, or establish full-fighter parity.
