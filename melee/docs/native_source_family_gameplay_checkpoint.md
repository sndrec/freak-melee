# Interpreted family gameplay checkpoint

2026-10-11. Fresh independent Zelda/Sheik and Popo/Nana packages exercise normal
CPU gameplay and explicit transform/special sequences. All three CPU cases and
both transform sequences pass, with unchanged input pins and no reported VM faults.
This supplements the [single-fighter roster CPU checkpoint](native_source_roster_cpu_checkpoint.md).

## Construction and comparison

`tests/native/fighter_family_source_parity.py` creates each member through the
production stock asset writer, imports its current template parameters and
prepares its interpreted behavior with the content tool. It writes the existing
family manifest explicitly, then packages, verifies and accepts the pair into a
fresh private collection on A:. This tests normal member/package preparation and
actual actors, not the Workshop create-button transaction. No generated behavior
C, DLL or stock-special forwarding is used. User drafts and installed content
remain untouched.

Both native games are hidden, muted, rendered and unpaced, with the same replay
and initial seed `0x1BADB002`. Receipts pin scripts, tools, actual copied member
projects, canonical package, input sequence and executables before/after execution.
Games must exit normally at controller EOF and both independent members must
report zero VM faults. Source/fighter/item diagnostics reject a case.

The shared comparator now includes the secondary Player role's state, action
and damage traces, alongside primary fighters, Item state/model/material/event
traces, linked resources and RNG. Custom kind/behavior identities are validated
and normalized for both roles; other fields remain exact. A secondary role can
also be a dormant alternate form, including the neutral stock Zelda opponent.
Secondary record counts do not mean that all those actors were actively fighting.

## CPU workload

The level-nine CPU runs 1,800 samples against neutral human Zelda on Final
Destination with four stocks. Three cases start from Zelda, Sheik and Popo/Nana.
The alternate packages own both members in their respective starting order.
CPU play changes forms naturally; Nana's follower CPU runs in the companion case.
All three pass after the chain correction: 5,400 inputs per runtime, 10,800
primary and 10,800 secondary/dormant Fighter records, and 5,400 RNG states.
This is one fixed-seed trajectory, not every CPU decision or special branch.

## Explicit transform and special workload

Separate 3,200-sample grounded and airborne sequences run on Battlefield against
neutral human Mario. Both must demonstrate Zelda -> Sheik -> Zelda with valid
independent identities, and the appropriate startup/loop/finish phase groups for
Needles, Chain, Vanish, Nayru, Din and Farore. Expected ground/air finish variants
are alternatives where landing occurs; the actual motion sets are in each receipt.

| Sequence | Primary Fighter records | Item records | Linked resource records |
| --- | ---: | ---: | ---: |
| Ground | 6,400 | 567 | 2,457 |
| Air | 6,400 | 464 | 966 |

Both exact comparisons pass, consume all inputs, report zero source faults and
retain unchanged input hashes. Each also compares 3,200 dormant-role records and
3,200 RNG states. Both published source members execute over the transform cycle;
the optional top-level Update-body counter remains zero because these templates
own motion/event callbacks rather than a global Update body.

## Corrections

Sheik's chain state callback queried `holder.motion`, which requires an actual
held-item relation. The chain is an accessory. Stock `itSeakchain_UnkMotion4_Anim`
checks `ip->owner`, including an absent owner, then the motion range 349..354.
The script now checks `current_owner.exists` and `current_owner.motion` directly.
The original relation guard stays intact; no Sheik-specific API was added.

After that correction, all existing fighter/item/RNG fields matched, but the
strict resource comparison still failed: diagnostics did not enumerate stock
Sheik chain nodes. `nativeReplayResources_Trace` now follows the native head's
`prev` chain in allocation order, matching the source-owned graph view. This is
a read-only observation change, not a gameplay fix or an ignored comparison field.

The companion comparator was checked using copied real Ice Climbers logs with
separate deliberate secondary-state, action and damage changes. All three are
rejected in their own secondary category; original logs are untouched.

## Evidence and limits

Artifacts under `A:/FreakMelee-test-archives/`:

- `source-family-cpu-oct11-final/receipt.json`: all three current paired CPU
  comparisons pass after the chain correction and client rebuild, including
  both member reports, exact companion traces and unchanged pins.
- `source-family-cpu-oct11-a/receipt.json`: initial three CPU cases pass, before
  the explicit sequence exposed the chain relation fault.
- `source-family-ground-oct11-c/receipt.json` and
  `source-family-air-oct11-c/receipt.json`: current grounded/airborne comparisons,
  including exact stock/source chain nodes and full motion coverage.
- Ground/air a and b receipts retain the original relation failure and the
  subsequently exposed diagnostic omission. Neither is counted as passing.
- `source-family-trace-rejections-oct11/receipt.json`: three deliberate secondary
  trace differences rejected by the shared comparator.
- `source-family-chain-oct11-build.log`: rebuilt client/content tool after the
  chain reference diagnostic was added; the shipped template is refreshed too.

These checks establish preparation and these actual gameplay trajectories.
They do not establish exhaustive paired death/separation/capture/interruption,
all costumes/stages/opponents, complete results acceptance, pixel/audio parity,
complete mutable-state coverage or restored rollback correctness/performance.
Earlier companion capture/results receipts remain separate historical evidence.
Compact whole-match save/load and the 15-rollback-ticks budget remain open.
Kirby remains excluded; the overall goal stays active and commits remain local.
