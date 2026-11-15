# Interpreted fighter interaction checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

2026-10-11. All 32 selected interactions across Marth, Roy and Mewtwo have
passing stock/source evidence. The initial batch passes 31; one repeated-move
coverage check required correction, and its fresh-package rerun passes. Final
trace analysis with the corrected comparator passes all 32 actual recorded pairs.
No production gameplay, fighter script or compiled-client changes were needed.
This supplements the [follow-through checkpoint](native_source_roster_followthrough_checkpoint.md).

## Interaction matrix

`tests/native/fighter_roster_interactions_matrix.py` generates the controller
recordings and explicit coverage requirements. Only existing pad generators are
reused; the historical generated-C packaging paths are not used. The shared
`fighter_roster_special_parity.py --matrix` pipeline creates fresh ordinary
interpreted packages in private stores, verifies/accepts them, and attempts
every selected comparison after failures. Games are hidden, muted, rendered
and unpaced. User drafts and installed collections remain untouched.

| Fighter | Cases | Required interactions |
| --- | ---: | --- |
| Marth | 6 | Ground/air laser counter, melee counter, Peach Toad response, neutral-special contact, up-special contact. |
| Roy | 6 | The same interactions with Roy's own approach timing and move contracts. |
| Mewtwo | 20 | Aerial/shield/reflected Shadow Ball; ground/left/air/shield/target Disable; ground/left/air/shield capture; landing and repeated aerial Confusion; ownership-retaining fireball reflection; both forward-throw directions; Bunny Hood/Screw Attack/double-item jumps. |

Counter cases require the counterattack motion, rather than just the ready pose.
Contact cases require actual opponent damage. Disable's target case requires
victim motion 300 and damage; Confusion captures require the appropriate victim
capture motion, its later fall and damage. Forward throws require the thrown
victim motion, damage and a real Shadow Ball article.

The Shadow Ball reflector case requires a continuously recorded kind-112 owner
transfer from P1 to P2. Confusion's different reflection policy retains P2's
fireball ownership: the same recorded projectile reverses horizontal direction
instead. Worn-item cases require the expected item kind and state 2 owned by P1
inside the sample-261..269 second-jump window. Repeated aerial Confusion requires
a motion start/restart in each of three separate input windows.

The Mewtwo capture/airborne setups use explicit offline positioning fixtures.
Worn-item fixtures spawn real common items, subsequently collected through pad
input. These fixtures exercise the actual capture/item paths; they do not prove
an ordinary player could produce every setup from spawn without the placement.

## Generic coverage checks

The common parity runner/comparator now expose:

- `--expect-item-state KIND STATE OWNER BEGIN END`: at least one matching item
  observation inside the half-open sample interval.
- `--expect-item-direction-change KIND OWNER`: consecutive records at the same
  item-list index reverse horizontal velocity while keeping kind and owner.
- `--expect-motion-window PORT MOTION BEGIN END`: the sampled motion changes to
  the requested ID, or its animation clock restarts, inside the interval.

These are trace contracts usable by other fighters, not new fighter-specific
engine or scripting operations. The direction-change check uses live-list
indices, not permanent generation identities. The selected Confusion case has
one live fireball; this is not a general lifetime/identity proof for arbitrary
multi-projectile scenes. Animation clock resets also include loop wraps, so a
window check is observable playback evidence, not a universal callback-entry
counter. Confusion's actual recorded input and nonlooping move establish the
specific restart tested here.

Initially, the motion-window check inspected the action channel. That channel
logs changed motion IDs and omitted Confusion restarting motion 352. The sampled
fighter state instead shows frame 55 at sample 235, frame 1 at sample 236, with
both native and source behavior matching. The check now observes those sampled
clocks. A negative window beginning after that restart fails even though the
move remains active, so merely being in motion 352 cannot satisfy the gate.

## Evidence

Artifacts under `A:/FreakMelee-test-archives/`:

| Receipt | Result |
| --- | --- |
| `source-roster-interactions-oct11-a/receipt.json` | All three packages prepare; 31 PASS / 1 coverage FAIL; input pins unchanged. |
| `source-roster-interactions-oct11-b/receipt.json` | Fresh interpreted Mewtwo repeated-Confusion case passes all three windows; pins unchanged. |
| `source-interaction-coverage-oct11-a/receipt.json` | Seven positive/negative checks on actual traces pass: restart vs ongoing playback, item state/owner, retained reflection vs wrong owner. |
| `source-interaction-final-traces-oct11-a/receipt.json` | All 32 completed pairs pass the final comparator with their full coverage arguments. |

The final analysis checks accepted recording bytes and coverage arguments
against the matrix and checks both log hashes against their original gameplay
receipts before comparing. Its receipt pins the final matrix/comparator. This
is one final trace-analysis batch, not a new final 32-case gameplay run. Both
gameplay batches use the same client/content-tool binaries. No native build or
compiler warning claim is made by these Python-only changes.

## Remaining boundaries

P1 is independently scripted; opponents in this cohort use their stock behavior.
The subsequent [two-source checkpoint](native_source_roster_duo_checkpoint.md)
exercises 18 selected interactions with both participants interpreted.
This does not establish the same cross-fighter paths with both participants
scripted, all seeds/stages, all interrupts, death/retirement or every move.
Pixels/audio, whole-match snapshots/restoration and rollback frame budget remain
separate work. Kirby stays excluded; the full goal remains active. Commits are
local only.
