# Direct interpreter: airborne controls and attached effects

The 2026-10-09 batch connects eight generic Fighter operations: aerial motion
selection, C-stick edge observation, airborne item shooting, airborne item
throw/drop input, authored airborne special input, adjusted TransN vertical
velocity, owned landing helpers and async joint-only effects. Scripts retain
their move decisions and callback order. Generated C remains a separate test
oracle; ordinary packages execute interpreted bytecode.

Author-facing contracts are in [Aerial actions](scripting/aerial_actions.md)
and [Joint feedback](scripting/joint_feedback.md). The preceding
[air API plan](native_source_float_air_api_plan.md) and
[async effect audit](native_source_async_effect_api_plan.md) record the original
native bodies and scope boundaries.

## Native behavior and continuation

The selector returns a common aerial motion ID without invoking the distinct
fighter mapping hook. C-stick edge observation preserves the native threshold
test without adding game-mode or button eligibility. Shared item entries use
the current held item; absent items are healthy no-ops. Throw input returns
whether the input was handled. Successful item self-removal does not undo that
result, while a live item fault or substituted identity prevents publication.
The two actual common throw/drop functions return an optional retained Item
record for their chosen operation. A Fall callback can legitimately replace
item A with B; the drop captures B before its callback and the backend validates
that exact record. No current-slot reacquisition substitutes for it. Failed
admission is distinct from no chosen item and from successful later removal.
All native consumers and current C-reference calls use the revised signatures
directly. The changed advanced-host slot moves to the end of its structure so
the existing offset fingerprint rejects old two-argument binaries; no adapter
or old-binary fallback is provided.

Air-special input selects up/down/side/neutral in native priority. Missing
authored hooks return false before facing reversal. Selected hooks run directly
through the prepared fighter program; the cached native flag write is guarded
against retirement, recycling, fault and program/storage substitution.

The shared landing resolver is a substantive function used directly by its
three native callers and the script backend. Collision and environmental
continuations retain the original Fighter before cached writes. Source landing
accepts only an owned root zero-argument Void helper, runs it on accepted contact,
and otherwise preserves wall-jump then cliff handling. It adds no aerial-jump
policy. Adjusted vertical velocity calls the native TransN kernel, retaining the
bunny relation's presence test and F32 multiplication without clamping.

Async attached effects map a semantic role once through the imported owned rig.
Only native joint-only effect signatures are admitted. Immediate/deferred
dispatch, deferred LIFO order, cancellation and allocation failure retain the
native queue protocol. The call does not change the effect-active flag. Native
ObjAlloc may grow the heap; this batch makes no production no-allocation claim.

## Build and preparation

The full client, content tool and selected reference/integration targets build.
All 40 selected integration CTests, nine reference/compiler CTests and the
additional common-policy C-reference test pass (50 selected tests total).
The existing actor fixture now initializes its real Fighter classifier/backlink
and includes the retained-lifetime contract used by the extracted air-special
body; no successful lifetime mock replaces that contract.

`A:/FreakMelee-test-archives/source-air-async-census-oct09-final2/receipt.json`
pins the rebuilt tool and ten exact sources. Peach advances from line1000 async
effects to line1163 `stats.count_as_thrown_item`. Mario, Dr. Mario, Game & Watch
and Zelda still prepare. Link/Young Link, Sheik and Popo/Nana still reject their
unsupported secondary-resource metadata. Preparation is not whole-fighter
gameplay acceptance. Root rechecked the recorded tool and all ten source hashes.

## Connected comparison evidence

All three private cohorts use the production source frontend and VM rather than
generated C for the tested script calls. Native healthy comparisons are pinned
to pre-batch commit `7b077ac85`. Receipts describe their controlled leaves;
injected identity/lifetime mutations are deliberate test observations.

`A:/FreakMelee-test-archives/script-async-effect-oct09-final2/receipt.json`
passes 383 checks in 34 cases. Root rechecked all 1,581 recorded inputs. It
executes unchanged native async spawn/process/flush/clear/init bodies, the exact
effect1236 synchronous case, generator attachment and joint reference increment.
Actor/package-view publication, an eight-record queue allocator and the lower
generator resource/allocation leaf are controlled. It does not execute the full
particle renderer or prove production queue reservation, frozen/motion scheduler
timing or a packed queue snapshot codec.

`A:/FreakMelee-test-archives/script-air-physics-final2/receipt.json`
passes 4,133 checks in 71 cases, with 354 native comparisons and 169 source
invocations. Root rechecked all 1,657 recorded inputs. The cohort runs actual
program preparation, typed storage, Fighter actor, owned helper execution and
fixed GObj pool, with mechanically extracted resolver/all three native callers,
TransN adjustment and platform predicate. Selected package/table facts and
geometry/environment/wall/cliff/property leaves are controlled. It proves
bounded ordering, F32 and continuation contracts, not full collision, imported
DAT, rendered Float or in-game acceptance.

`A:/FreakMelee-test-archives/script-air-inputs-final2/receipt.json`
passes 1,900 checks in 240 cases, with 357 native comparisons and 236 VM calls.
Root rechecked all 1,665 recorded inputs. Actual frontend/preparation, Fighter
and Item actors, storage, execution and physical pools connect to mechanically
extracted selectors, input gates, three gun entries, throw/drop and Dropped
continuations. The cohort covers normal false/no-op branches, both facings,
hook availability, item self-removal, live fault/substitution, replacement-B
fault and failed replacement admission. Rig/common motion facts and
motion/animation/accessory/parasol/timer/geometry/render/audio/stat leaves are
controlled. Article Dropped is exercised; the separate Workshop holdable branch
is unavailable in this fixture. This is not actual animation, gun firing,
throw-release, RNG or whole airborne IASA/gameplay acceptance.

## Remaining boundaries

Existing Super Scope code still uses GameCube byte-offset overlays instead of
typed native motion state. Comparing its current entry body does not establish
correct native Scope state or subsequent firing. That needs a coherent typed
cleanup across its shared states, rather than an isolated entry-only patch.

An additional build of the historical `native_held_lifecycle_source_test` was
attempted and failed. Its SDK54 comparator lacks the current retained Fighter
and VM setup, and its original timer leaf still uses the old drop signature.
It is not included among the 50 passing tests or claimed as current acceptance.
The frozen original reference data was preserved; it needs a deliberate fixture
migration rather than successful lifetime mocks or rewritten native bodies.

Peach still needs thrown-item statistics, prepared body-rebound and dynamics
decisions, animation-driven jump policy, held-open transitions, pure observations
and full owned asset/gameplay acceptance. Secondary resource schemas remain
open for the other partial templates. The full goal stays active and commits
remain local. No game was launched for this batch.
