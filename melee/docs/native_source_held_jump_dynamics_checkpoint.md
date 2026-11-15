# Interpreter: held-open entry, animated jumps and dynamics decisions

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

2026-10-09. This batch closes three shared contracts used by Peach. The full
Peach template now prepares through the direct interpreter. This is not full
fighter, asset, rendering or match acceptance.

## Runtime changes

`motion.id` is now writable with an Int in the selected owned motion layout and
native 16-bit domain. It changes identity only; no animation, callback or motion
union initialization occurs. `state.held_open(blend)` then runs the actual common
entry, preserving saved previous ID, prepared held-motion selection, motion
change, animation and timer/flag order. The common entry validates its retained
Fighter and common-data pointer between effectful stages. It does not pin the
original held slot: an authored selector may legitimately replace that slot.
The actual subsequent held-item machinery owns its current selection.

Existing typed recipient observations (`creator.motion`, `holder.motion`,
`owner.motion`) remain valid. A preliminary SourceOnly flag on the property
blocked those reads; the combined source census caught it and the flag was
removed. The bounded write backend still validates the target's actual owned
program/data/layout. The readonly recipient test now uses AnimationID and also
checks that MotionID target validation is writable.

`jump.begin(..., animation_driven=true, ...)` requires a prepared root Void0
`on animation_driven_jump_physics:` hook before mutation. Shared entry keeps its
native order: motion selection, flags/velocity/input age/jump count, clear sound
state, jump sound, optional worn sound, then the late physics override. Source
actors install the exact hook code ID in Physics; ordinary native actors retain
their native callback. Peach's authored body calls normal air acceleration and
adjusted animation vertical velocity in the original order. No stock Peach
handler or machine-code pointer enters the script actor's physics selection.
The other known true-mode callers, Yoshi and Mewtwo's WIP templates, now declare
the same authored physics body. Their broader activation remains unaccepted.

The existing phase/snapshot system admits that prepared hook only in Physics.
Helpers can replace or clear the phase; motion selection replaces the override.
NewLife retains existing phase semantics while renewing generation/storage.
No mutable actor field or snapshot-format extension was added.

`on dynamics_interaction_start:` prepares as I32(I32 set, I32 set_count), default
zero. The actual dynamics scheduler calls it once per set before the native
solver. Signed thresholds are preserved; they are not joint indices. Cached
Fighter/program/layout, root/class and dynamics graph identities are retained
across authored decisions. Flatzone's temporary root scale is restored on a
live fault, without writing a retired/replaced root. Source actors use the
generic grounded/normal-size policy and no authored wind decision. The distinct
ground/wind hooks remain unmigrated; Definition/stock exceptions are not borrowed.

## Verification

The client, content tool and selected reference targets build. Fifty-five grouped
CTests pass. The recipient regression test now checks writable MotionID and
readonly AnimationID. Failed intermediate logs remain separate from the final
passing log.

| Connected cohort | Result | Fresh A: directory |
| --- | --- | --- |
| Held-open entry and bounded identity writes | 20,034 checks, 896 cases, 868 native comparisons, 1,732 checked VM calls | `script-held-state-final` |
| Animation-driven jump, phase and snapshot selection | 260 checks, 10 cases, 12 native comparisons | `script-animation-jump-oct09-final` |
| Dynamics decisions and retained scheduler graph | 2,495 checks, 223 cases, 96 scheduler entries, 120 native trace comparisons | `script-fighter-dynamics-final` |

These use the actual frontend, preparation, actor/storage/Execution/VM and native
GObj pools. Native entry/math bodies are mechanically extracted. Held motion and
animation transitions, package facts/constructor lookup and jump audio leaves
are controlled as described in their test READMEs. Fault/free injection during
audio stages is a continuation test, not a claim that audio spontaneously runs
authored callbacks. Snapshot tests restore actor records with explicitly saved
native peers; they do not prove complete scene rollback.

Dynamics solver/matrix/animation leaves are controlled; the class predicate,
descriptor free-list/unload and flag clearing bodies execute. Native graph
allocations have no individual generation query, so pointer/class/link checks
do not guarantee detection of arbitrary identical same-address resurrection.
The changed scheduler's one E7B4 warning was independently reproduced against
the pinned pre-batch body and is narrowly documented in the fixture README.

The older private `script_jump` fixture is historical: its synthetic zero-epoch
objects do not meet current pool/lifetime admission. The new connected fixture
supersedes its jump-entry comparisons; its separate IASA/input/walljump checks
still need deliberate migration to real pools. No bypass or current pass is
claimed for that old fixture.

Root verified all 4,992 recorded cohort input hashes, plus the rebuilt tool and
ten census sources. The audit receipt is
`A:/FreakMelee-test-archives/source-held-jump-dynamics-root-audit-final/receipt.json`.

## Source preparation and next boundary

The rebuilt tool's ten-template census passes Mario, Dr. Mario, Game & Watch,
Zelda and now Peach (261 functions, 67 imports, 4,241 instructions). Link, Young
Link, Sheik, Popo and Nana retain their secondary-resource schema boundaries.
The initial census regression from MotionID's read flag is preserved separately
from the corrected census.

The copied Peach draft already supplies all five primary Article slots and
source-check performs their owned asset/entity/program preparation successfully.
Next is publication and live constructor/lifecycle acceptance for those prepared
assets, then the documented secondary-model/collection schema. No new primary
asset admission gap is inferred from the old boundary. The preparation result
alone does not establish whole fighter
behavior, imported animation correctness, renderer parity or rollback speed.
No game was launched in this batch.
