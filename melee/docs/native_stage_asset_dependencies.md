# Native stage assets: common item ownership before stage script import

2026-10-11 loading-order correction. The stage importer and item command validator
retain their existing data and geometry checks; no stage-specific exemption or
fallback model was added.

## Failure and dependency

A fresh native Green Greens match failed importing `/GrGr.dat`, descriptor status
6 at offset `0x59024`. That word is `0x28000000`: item opcode 10, selecting animation
0. The ALDYakuAll script runs on the common Random article. Its real model owner
is necessary to validate model/animation operands; an ownerless script is rejected.

`Stage_802251E8` calls `Ground_801C0754`, which loads and imports stage assets.
Previously all 15 scene call sites loaded common item assets afterward through
`Item_80266F70` or `Item_80266FA8`. On first boot the common article table was absent.
An earlier scene's table is also not a sound dependency for the new scene: preload
can retire its owning asset storage. The standalone stage-data test already loads
the real common assets before Green Greens, which explains why its geometry test
passed while the actual scene failed.

The native conversion eagerly validates immutable command graphs at asset load;
the retail ordering did not perform that native validation phase. Loading the
dependency first addresses the actual contract rather than weakening validation.

## Change

The 15 call sites now load common item descriptors before stage assets: versus,
the stage-specific single-player scene, results, result-adjacent/dummy scenes,
trophy fall and the nine existing demonstration scene initializers.

`Item_80266FCC` remains at its existing point, preserving item actor/counter setup
relative to the stage's initialization callbacks. The ordinary stage load/setup/
on-load sequence is unchanged. There is no lazy loader, compatibility adapter or
duplicate common-asset import. This is a PC dependency ordering change; maintaining
the former GameCube initialization order is not a requirement of the native port.

Static call-site inventory found 15 stage-load calls, each preceded by the actual
common load. The native client, content tool and stage-data test build succeeded.
The current stage-data executable completes successfully against the original ISO.

## Evidence and limits

`A:/FreakMelee-test-archives/stage-owner-order-oct11-green/receipt.json` records the
previous binary reproducing the exact known import failure and the fixed binary
completing 2,000 rendered controller samples with zero census errors. This is
candidate gameplay evidence for Green Greens, not an old/new frame comparison
against a baseline that never reached gameplay.

The full 29-stage rerun is
`A:/FreakMelee-test-archives/stage-owner-order-oct11-roster/receipt.json`:
**29 cases PASS**, with all 27 pins independently rehashed unchanged. The runner
distinguishes exact old/new
comparisons on the other stages from the explicitly required baseline Green Greens
failure plus successful candidate gameplay. It never treats an unexpected baseline
result as that known failure. Source, binary, replay and ISO inputs are pinned.

`stage-owner-order-oct11-results/receipt.json`: three freshly prepared interpreted
bases (Mario, Zelda/Sheik, Popo/Nana), win1 and lose outcomes, **six cases PASS**.
This exercises actual gameplay teardown, new results scene entry, model admission
and source actor retirement with the new ordering. The ordinary clip/model/scene
checks remain separate from result pixels and pose-by-pose animation parity.
The result cohort's 72 pins were independently rehashed unchanged after completion.
`stage-owner-order-oct11-call-sites.json` retains the 15 source call sites, prior
common-load checks and file hashes.

The cohort uses medium items, CPU Ganondorf/Zelda/Ice Climbers/Game & Watch, normal
HSD drawing and hidden/muted fast pacing. It is not every stage interaction,
pixel/audio comparison or a real-time audio-clock test. Reordering the demonstration
call sites is compiled and source-inventoried; those alternate scenes are not live
acceptance claims for this versus-focused project.

The build and standalone data-test logs are
`stage-owner-order-oct11-build.log` and `stage-owner-order-oct11-data-test.log` under
the same test-archive directory. The earlier Ground cohort's Green Greens failure
remains historical evidence; this correction supersedes it for current behavior.
