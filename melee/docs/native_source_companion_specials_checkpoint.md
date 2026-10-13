# Actual interpreted companion specials

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

The complete Popo/Nana source family now completes all four grounded special
entries in real, rendered native games without script faults. Stock and source
runs use identical controller samples. Both actors' action transitions and every
emitted Fighter state/geometry field match exactly throughout these scenarios.
This extends the earlier [startup checkpoint](native_source_companion_activation_checkpoint.md);
it does not establish complete paired gameplay or compact rollback acceptance.

## Corrected rope ownership

The initial up-B comparison faulted at sample 108. The rope's animation callback
read `holder.motion`, but Gumstrings is an accessory, not the Fighter's held
item. The interpreter correctly rejected that absent held-item relation.
Retail `itClimbersstring_UnkMotion3_Anim` checks `ip->owner`, cleans up when it is
absent and retains the rope while the owner's motion is in 347..356.

Both full scripts now use `current_owner.exists` and `current_owner.motion`,
preserving that cleanup and range test. No receiver binding was relaxed, no
stock callback forwarding was added and no fighter-specific engine primitive
was introduced. The frozen SDK68 rope candidate retains its historical compiler
spelling; its scalar/renderer receipts are independent reference evidence.

## Connected comparison

Native action/state tracing now visits both Player roles, recording secondary
actors as `companion`, `companion-action` and `companion-damage`. Primary records
are unchanged. This uses fixed trace storage and only runs when tracing is
enabled; gameplay scheduling and snapshot representation are unchanged.

The tracked `tests/native/source_companion_game/run.py` launches eight hidden,
muted games: stock and source for Ice Shot, Squall, Blizzard and Belay. Each
consumes 360 samples, with P1 Ice Climbers and a neutral human Mario at P2 on
Final Destination. Drawing remains enabled. No desktop input is sent. Both
actors' states include motion/frame, commands, controls, facing, velocity,
ground/air mode, hitlag, scale, position, offsets, pose, hitbox/hurtbox shapes,
attachments and visibility. The runner verifies that the actual packet contains
the current complete canonical scripts, not a stale installed revision.

All eight games exit normally at input EOF, without script faults. Each paired
comparison has 1,080 matching Fighter records: Popo, Nana and Mario at every
sample. Action timelines also match for both Ice Climbers. Detailed Item digests
match in addition to the visible fields used as the pass gate:

| Scenario | Matching Item records | Article |
| --- | ---: | --- |
| Neutral | 71 | Ice Shot, kind 106 |
| Side | 0 | No Item |
| Down | 99 | Blizzard, kind 107 |
| Up | 77 | Gumstrings, kind 113 |

The corrected source family also passes the actual publication fixture's 264
checks. It was accepted into a private test store only; the user's installed
collection was not changed. Its package ID is
`adaaac9faca59e652b45789f972577c48ecf0c27b16a9b6cb95ab8be16538062`.
The first corrected-source launch attempt used a cached but unaccepted packet
and was rejected before gameplay. The rejected runs remain separate from the
accepted comparisons.

Receipts:

- Publication: `A:/FreakMelee-test-archives/source-companion-rope-owner-publication-traced-oct10/receipt.json`,
  SHA-256 `2b9c8e2ca0597a7641dfef4665818c7dedc0e75030656d649c221b09931299a8`.
- Rendered comparison: `A:/FreakMelee-test-archives/source-companion-game-traced-oct10/receipt.json`,
  SHA-256 `4da438a45f9b9a2d0745837739757b2af89f01625fe15975a2e3ba21a1b53635`.
- Independent rehash of 1,801 current input pins and all sixteen game logs:
  `A:/FreakMelee-test-archives/source-companion-specials-verification-oct10.json`,
  SHA-256 `6f35d964e474c98db0247a843a5d3654a7e565255b1d58645eef22fe1c82eed5`.

The native client, content tool and family/package fixture targets build.
Existing Item trace `u32` format warnings remain. Synthetic `--replay-inputs`
playback deliberately does not validate recorded signatures, RNG or expected
state digests. Matching emitted fields does not prove every mutable field,
GPU image or restored snapshot. Airborne/interrupted moves, injury, separation,
partner death and full match-end coverage remain open. The source-owned
[Workshop preview checkpoint](native_source_workshop_preview_checkpoint.md)
and compact whole-match save/load remain separate boundaries.

Later actual CPU capture/damage and win/loss results coverage, including the
native capture-order crash diagnosis, is recorded in the
[capture/results checkpoint](native_source_companion_capture_results_checkpoint.md).
