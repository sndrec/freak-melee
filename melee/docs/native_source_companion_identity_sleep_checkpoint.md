# Interpreter companion identity, history and Sleep checkpoint

This batch follows `f1062ac79`. It reuses existing active family/slot metadata
and the shared native CPU and Sleep paths. Simultaneous-companion package
preparation and activation remain closed.

## Member identity and constructor history

`family.member_id(index)` now resolves the package's immutable member ordinal
across both published Player roles. An alternate-form swap or dormant actor
does not change that ordinal. The caller must be a published interpreted member;
selected targets retain their physical lifetime, owned package, published
program, actor storage and Player/family relationship. No second registry is
introduced. Prepared and published programs have different addresses; admission
retains the actual published copy and checks its identity and compiler bank.

The constructor CPU hint is a separate native boundary. Only simultaneous
member one selects already-published member zero from its own Player/family.
The constructing member need not itself occupy a Player role. Its program and
storage are retained when attached; the current native constructor attaches
before CPU preparation. The complete initializer passes the full-width ID to
the authored `cpu_prepare` hook and seeds all thirty history records from the
returned donor. Missing donors preserve zero position and positive facing.
Primary, Single and alternate-form callers receive no companion hint.

See [family queries](scripting/family_queries.md) and
[paired fighters](scripting/paired_fighters.md).

## Sleep and death continuations

Sleep entry now retains its caller across motion change before writing cached
visibility and camera fields. The optional peer-death path retains the primary
again before publishing match status. Independent peer retirement permits a
healthy primary to continue; a retired, faulted, replaced or reused primary
does not continue through its cached fields.

The death wrapper checks after the inner death routine before inline Sleep.
The reset caller checks before cached rumble work and captures the successful
new generation after reset. The constructor's alternate-Sleep branch checks
before its final bookkeeping and return. These are narrow existing callsite
changes, with no new handler delegation or wrapper API.

This checkpoint supersedes the pre-Sleep continuation and next-cohort sections
of [the companion contract audit](native_source_companion_contract_audit.md).
That audit remains pinned as an input to the family/history proof; its earlier
Sleep status is historical. Whole construction and partial unload still need
integration, even though the specific Sleep continuations are now guarded.

## Evidence and limits

Client, content tool, source-assets and selected integration targets build.
All sixteen selected CTests pass; the final grouped build preserves thirteen
production input hashes. Root independently rehashed these receipts and all
their recorded inputs. No game, GPU or audio device was opened.

| Receipt | Scope | SHA256 |
|---|---|---|
| `A:/FreakMelee-test-archives/script-family-history-oct10-final/receipt.json` | 2,370 checks, 143 cases, 312 native comparisons, 55 VM calls and 47 negatives; 1,718 pinned inputs | `8cec3bf60b1dc36d0c415dfad9ad081009e52266a7bb7d21b133a823afcbe810` |
| `A:/FreakMelee-test-archives/script-companion-sleep-oct10-final/receipt.json` | 906 checks, sixty cases, 54 authored VM callbacks and eight foreign callbacks; 1,707 pinned inputs | `9b25cc17bbee7c3cb3bce9428b8f57f16789c8c9e029d421b5132160885b8598` |
| `A:/FreakMelee-test-archives/source-companion-final-census-20261009-205920/receipt.json` | Actual preparation of all ten retained full sources using the rebuilt content tool | `5b18acd4470595f9f063b9502b44a56853b2ba580186bed8707c0b806262ea71` |

Family comparisons use actual frontend/program publication, actor/storage, VM,
Player getters and role swap, native lookup and complete CPU initializer. Active
family/package/imported-data setup is controlled. The fixture does not activate
a simultaneous package or run whole Fighter construction, paired gameplay or
rollback history acceptance.

Sleep comparisons run source-exact native bodies, actual GObj/Fighter pools,
real callback VM faults and same-address pool reuse. Motion installation,
camera/effect/status/audio-reset leaves and successful whole-engine reset are
controlled. Constructor coverage is only the exact alternate-Sleep branch and
tail. The immutable-layout corruption probe can complete inner death cleanup
before the exact outer guard stops further work; no completed writes are undone
and arbitrary metadata-corruption containment is not claimed.

The next cohort is constructor failure/unload ownership, then simultaneous
admission and whole paired acceptance. Player currently publishes a primary
before constructing its secondary, which is necessary for the history hint.
Failure handling must preserve that healthy ordering while avoiding half-created
pairs and cleaning partial actors. Compact whole-match save/load remains
unfinished; Regions/Pages is still the broad rollback oracle.
