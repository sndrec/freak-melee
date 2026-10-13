# Interpreted Fighter observations, landing input and body rebound

2026-10-09. This batch connects the next shared Peach contracts and corrects
Super Scope's native motion storage. It is not complete Peach, article-asset,
whole-match or rollback acceptance. No game or Dolphin was launched.

The subsequent [held-open/jump/dynamics checkpoint](native_source_held_jump_dynamics_checkpoint.md)
closes the three shared contracts listed as next work below; this document's
receipts and source census remain the historical observation/contact batch.

## Direct interpreted operations

The actual catalog and runtime now provide:

- `stats.count_as_thrown_item()`: sets the native action's counting flag;
  it does not increment statistics or invoke a stock special handler.
- `movement.special_landing_lag`: read-only F32 `x2EC`, preserving its bits.
- `engine.debug_level`: read-only I32 native `DbLevel`.
- `collision.contact_rebound_allowed()`: queries the retained contact recipient's
  prepared `body_rebound` Bool callback, with false as the absent-hook default.

The pure observations retain physical Fighter identity and an attached actor's
exact program/storage schema. Contact additionally admits the selected owned
data and imported motion layout for source actors. Neither missing nor faulted
source actors fall back to a stock fighter's decision. After either callback
result, contact validates both actors and the original relation before publishing.
Framework failure or retirement preserves output, rather than becoming false.

The native shared contact query now returns status and writes a Bool output only
on success. The native Peach consumer and C-reference exporter call it directly.
The changed advanced Host field moved to the structure tail so its existing
offset fingerprint rejects the old function signature. No ABI adapter was added.
Generated C remains a separate test oracle; ordinary packages use the interpreter.

## Captured landing input

The common landing entry still writes its ordinary motion-state union flag. It
then captures that Bool for the interpreted `landing_action` hook. Scripts use
`event.interrupt_allowed`, available only in that callback, and explicitly pass
it to helpers. No global motion-union getter or old-name alias was introduced.
Changing motion inside the callback cannot overwrite the captured event input.

Canonical Peach and current reference fixtures use this event property. The
reference C entry retains its actual `void(GObj*)` ABI and captures the stored
option locally before authored statements; the interpreted event receives Bool.

## Typed Super Scope state

Start now uses named charge-frame/release-latch fields, Fire its charge level,
and Rapid its remaining-cycle count. Both rapid input handlers use native
`user_data`; no GameCube GObj/Fighter byte offsets remain in these units.
The rapid reset reads typed common U32 data with the same signed conversion as
the existing entry. End was inspected and needed no layout change.

## Verification

Client and content-tool builds pass. There are **55 passing selected CTests**:
45 core/landing/reference tests, nine additional reference tests and the migrated
ordinary aerial-policy test. The first aerial run failed because two fixture
negatives poisoned retired userdata without freeing their actual pool slot;
the fixture now performs real retirement, with production guards unchanged.

Fresh private receipts under `A:/FreakMelee-test-archives/`:

| Cohort | Result | Receipt directory |
| --- | --- | --- |
| Actual Fighter scheduler and landing payload | 2,566 checks | `script-fighter-landing-oct09-final2` |
| Fighter scalar observations and counting flag | 1,856 checks, 216 cases, 388 native field pairs, 101 VM calls | `script-fighter-observations-final` |
| Prepared body rebound and native Peach continuation | 1,800 checks, 169 cases, 123 ordinary native predicate pairs, 129 source queries | `script-contact-rebound-final` |
| Typed Super Scope control and Item charge kernels | 1,927 checks, seven cases, 236 native pairs | `script-scope-layout-oct09-final2` |
| Ordinary aerial/contact policies | 781,090 checks, 69,120 ordinary aerial pairs | `aerial-policy-contact-final` |

Root independently verified all **8,086 recorded cohort input hashes** against
the final checkout. `source-observation-contact-root-audit-oct09-final2` records
that verification. The four connected/layout cohorts total 8,149 checks; the
large ordinary policy cohort is a separate oracle, not extra source-VM coverage.

Frontend, preparation, actor dispatch, typed storage, VM and actual GObj pools run
in the connected cohorts. Imported package facts, payload allocation and selected
engine leaves are controlled. The Scope cohort uses widened native structures,
mechanically extracted bodies and actual Item charge/ammunition kernels, but
controls motion/animation/input/presentation. Undefined Item-kernel fallthrough
inputs are excluded. These are not live rendering, statistics-accounting,
whole-fighter, dynamics-solver, heap-reservation or snapshot-performance proofs.
Historical frozen-SDK composition receipts remain immutable; they are not
silently relabeled as current full-template regeneration.

## Remaining closures

Fresh preparation census `source-observation-contact-census-oct09-final2`
advances Peach from line1163 to line1440, the deliberate motion-ID write before
held-open entry. Mario, Dr. Mario, Game & Watch and Zelda still prepare. Link,
Young Link, Sheik and Popo/Nana retain secondary-resource metadata boundaries.
Root verified the rebuilt tool and all ten census sources.

Peach still needs the held-open transition contract, prepared
`dynamics_interaction_start`, genuinely owned animation-driven jump physics, and
real admission of its five Article asset graphs. Body rebound is connected;
dynamics decisions and the other wind/ground policies are not accepted by this
batch. See the [remaining audit](native_source_peach_remaining_api_audit.md) and
[dynamics proposal](native_source_dynamics_rebound_hook_proposal.md).

User references: [observations](scripting/fighter_observations.md),
[body contact](scripting/contact_rebound.md), and
[landing event input](scripting/held_articles.md).
