# Ice Shot / Blizzard source composition

The unregistered component authors Ice Shot 341/342 and Blizzard 357/358,
their accessory cues, four Ice article states, one Blizzard article state,
secondary article callbacks, and the corresponding Ice/Blizzard cleanup and
death fields. Both Popo and Nana receive separately imported named parameters.
It contains no stock fighter callback delegates or paired-identity API.

The console comparison uses immutable SDK66, complete original source bodies,
generated callbacks from the actual frozen content tool, and the real callback
lifetime registry. `run.py` extracts and stages both retail DATs, generates and
compiles each module, runs both matrices, and records input/output SHA256s.
It does not build the repository or launch a game.

```powershell
python tests/native/ice_climbers_composition/run.py --root . --baseline .local/baseline66 --output A:/FreakMelee-test-archives/ice-climbers66-final --iso 'C:/gayms/freak-melee/Super Smash Bros. Melee (USA) (v1.02).iso'
```

The matrix checks both entries and all four phases, finished animation,
ground/air collision transitions, boost reuse, cue/retained-item/allocation
combinations, emission-before-cue countdown ordering, facing, all article
phases, lifetime and shrink thresholds, slopes, damage truncation, wall/ceiling
contact combinations, shield velocity/lifetime policy, direct launch, original
creator destruction after damage-owner change, and interaction cleanup.
Trace comparisons retain motion flags, frames/rates, creation position,
particle IDs/modes/facing payloads, sound channels, debug/refresh ordering,
stage requests, and cleanup order. Shared scalar comparisons retain position,
velocity, facing, throw/command flags, item state/scale/life, collision history,
hitbox damage, callback presence, and projected owned variables.

`fixture.py` mechanically extracts 78 complete original bodies plus ten shared
bodies. Shared bodies include `nativeArticle_Create`, falling/facing/scaling,
lifetime setup, reflection and wall/ceiling contact queries. Retail big-endian
staging imports provide 24 values; five additional named defaults hold source
constants and form-specific voice/loop policy. The FMP has 29 typed entries per
form. Offsets exist only in the test-side decoder, never in authored runtime
behavior. Example retail values are air boost 1.5, landing lag 16, spawn offsets
12/18, Blizzard interval 5, Ice lifetime 60, launch speed 1.5, base damage 3,
Blizzard lifetime 10, speed 2 and vertical acceleration 0.125.

The original full cleanup callback also owns Squall and Belay. Its binding
presence is observed, but it is never called through a fake substitute.
`cleanup_standalone` is explicitly a partial component: comparison executes the
actual Ice cleanup and actual Blizzard cleanup bodies in source order.
Death comparisons project only this component's fields; stock x2234/x2238
remain outside it. The authored landing boost reset is present, but the full
common landing caller is not part of this fixture.

The following engine dependencies remain controlled: allocation/model
construction, animation/motion transitions, stage sweeps and floor geometry
alignment, shield bounce vectors, particle queuing/rendering, sounds, and
debug bookkeeping. Constructor comparisons execute the complete original
constructors and actual generic constructor with identical controlled
allocators; they validate SpawnItem field values and actual initialization
ordering, not the whole allocator. Joint scale materialization uses the actual
shared uniform-scale body versus controlled per-channel writes. Async controls
pin the actual queue pointer, mode 0/3 and facing pointer. SDK66's separate
engine fixtures cover the new generic API bodies; this receipt does not relabel
those independent checks as embedded engine coverage.

Source removal/recycling checks are separate authored safety cases. Original
saved creator pointers after retirement have no healthy pointer contract, so
the fixture does not execute them or claim original parity there. The actual
lifetime registry checks full-width fighter/item generations.

The earlier pause extraction in `ice-climbers65-preparation` was preparation
only. Early SDK66 attempts preserve syntax/fixture diagnostics. The fifth
attempt preserves the real draft mismatch: `ITEM_ANIM_UPDATE` is 2, while the
draft used 1. The source translation now uses the correct flag in every
article transition; no comparator masking or stock change was made.

Registered clone/import, installed package, independent activation, gameplay,
paired spawn/controls/capture ownership, Gumstrings, Squall and Belay remain
outside this source-only component. The earlier activation census pins these
foundation gaps. No remaining new generic API is required for this bounded
Ice Shot/Blizzard component.
