# Samus composed-source gameplay checkpoint

2026-10-11. Fresh Workshop clones now use the independent `samus_full.fms`
template. The shipped source loads through the normal package path and passes
28 stock/interpreter grapple/lifecycle gameplay comparisons. All 282 bodies
prepare. This closes the source integration gap, not exhaustive whole-fighter,
visual or rollback acceptance.

## Normal loading and actual gameplay

`tests/native/samus_grapple_constructor/gameplay.py` copies a complete Samus
draft into a fresh A: directory and copies the exact shipped independent
Workshop source. Normal
`import-parameters`, `source-audit`, `script-build`, `package-project`,
`verify-package` and `accept` commands create an isolated installed collection.
The package contains 282 checked functions, 8,776 instructions and zero legacy
attribute imports. The installed acceptance collection remains private; existing
user drafts are not overwritten. The Workshop's fresh-draft selection and
shipped UI source now point at the full independent template.

The normal client then constructs the owned Fighter and runs its actual
scheduler, stage collision, animations, resource effects and capture/throw
paths. There is no manually populated parent Fighter, stage-response stub,
effect stub, common-state CLI override or direct callback injection in these
replays. Offline placement fixtures establish initial air/nearby-victim setups;
all subsequent move inputs and behavior run through the ordinary game. Windows
are hidden and audio is muted. The default runner processes the complete batch
even when a case fails.

Both facings cover:

- Standing grab without a victim, through resource removal and recovery.
- Ordinary airborne grapple, including extension, retraction and landing.
- Each of the four throws against an actual stock Mario victim, requiring
  Catch, CatchPull, CatchWait, the selected throw and victim damage. Endpoint
  capture-anchor binding, retraction and throw presentation execute normally.
- Air grapple contacting a Final Destination wall, entering AirCatchHit and
  resource modes 6/8, then expiring into DamageFall and removing the resource.
- The same wall contact followed by A, entering mode 7, completing pull via
  CliffJumpSlow2 and subsequently catching the ledge in these fixtures.
- The Up/Down/Up/A grapple code followed by a second grounded grab: the chain
  doubles from 30 to 60 nodes, L exercises target selection and A creates the
  endpoint fighter hitbox. The geometry trace requires that capsule to be active.
- The same code followed by an airborne wall grapple, with L held through the
  ordinary expiration time; releasing L permits expiration, or A initiates pull.
- Mario damaging Samus before and after grapple launch, requiring the live
  chain to disappear on that damage sample, in both directions.
- Modified ground chain creation followed by two airborne blast-zone deaths
  with live chains. The ordinary death/respawn flow clears the modified code;
  both subsequent grounded grabs must create ordinary 30-node chains.

Coverage checks require the corresponding fighter motions and resource states,
complete replay consumption and resource removal before the replay ends.
The existing strict parity comparator validates independent custom/custom
behavior identity and compares both players' fighter states/actions, item
states/events, damage, contact and RNG traces. It normalizes only the expected
fighter identity difference. Numeric tolerances or ignored digest fields are
not used.

Read-only resource diagnostics now compare each separately allocated chain
node in attachment-to-tip order. They include exact positions, velocities,
current/previous collision positions, line/environment flags, joint-tree pose
hashes and material/texture visual hashes. Stock linked lists and owned resource
arrays use the same trace order; no animation or matrix evaluation is advanced
by tracing. This is enabled only with the existing fighter-state trace option.

## Corrections found by the batch

The first wall-contact replay crashed in `efAsync_Dispatch`, reached from
`nativeArticleResource_SpawnEffectFacingAtPosition`. The generic resource API
passed a scalar float through the variadic effect call. All four admitted
facing generators (`0x3EF..0x3F2`) consume `Vec3*` followed by `f32*`; the native
Samus caller also passes `&fp->facing_dir`. The API now passes `&facing`, retaining
its synchronous stack payload and the existing operation lifetime guard. No
fighter-specific API or effect-dispatch workaround was introduced.

Once wall entry worked, strict comparison exposed hanging-velocity differences
starting at sample 238. `fn_802B8D38` computes
`velocity + (new_position - old_position)`, while the translation had computed
`(velocity + new_position) - old_position`. The script now retains the original
parentheses and F32 evaluation order. Before correction the maximum velocity
difference was 0.00000190735; after correction all 16 complete traces match
exactly, including post-expiry falling and terminal pull follow-up.

## Evidence and limits

`A:/FreakMelee-test-archives/samus-gameplay-oct11-final-d/receipt.json` is PASS,
with all hashed composition inputs, tools and client binaries unchanged. The
16 comparisons contain 9,600 controller samples, 19,200 fighter states, 392
action transitions, 1,070 item states, eight damage events and 9,600 RNG states.
Private package ID:
`3be71c0039710b2ce6dd9f174665bb486dcd587f4b699ddc62c268434f230be2`.
Full trace logs and replays remain beside that receipt on A:.

Native client/content-tool rebuild and seven focused CTests pass: character
assets, parameters, resource metadata, script compilation and the three
resource-effect compiler/action/callback tests. The older standalone resource
geometry runner did not compile against baseline72 because that SDK lacks the
current `native_match_pool.h`; its failed attempt is not counted as regression
proof. Actual wall effects are exercised by the passing client replays.

The earlier [connected resource corpus](native_source_samus_pull_checkpoint.md)
remains the detailed graph/controller/palette and failure-boundary evidence.
The live trace digests now cover separate resource nodes, but do not establish
screenshot-level visual fidelity.

The expanded receipt is
`A:/FreakMelee-test-archives/samus-resource-gameplay-oct11-b/receipt.json`: PASS
for all 22 comparisons, with hashed inputs/tools/binaries unchanged. It contains
15,200 controller samples, 30,400 fighter states, 520 action transitions, 3,310
item states, eight damage events, 15,200 RNG states and 142,480 resource count/node
records. All resource records match strictly. The candidate package ID remains
unchanged; this extension improves observation and coverage, not its behavior.
The client rebuild and the seven focused CTests above pass again.

## Shipped template and live-chain lifecycle acceptance

`A:/FreakMelee-test-archives/samus-workshop-gameplay-oct11-final-a/receipt.json`
is PASS for all 28 cases with inputs/tools/binaries unchanged. It covers 24,400
controller samples, 48,800 fighter states, 688 action transitions, 4,154 item
states, 12 damage events, 24,400 RNG states and 175,424 resource count/node records.
Package ID: `a738ea7a1f647144898b5d8b9220abb12ad7dc73f2dbfb355265a3fe2a208550`.
The earlier six-case lifecycle development receipt
`samus-lifecycle-gameplay-oct11-c/receipt.json` also passes; its b predecessor
failed coverage because one jab missed and normal chain expiry preceded death.
The accepted setup uses ordinary attacks and natural blast-zone deaths. It does
not force a damage callback, reuse a stale pointer or weaken trace comparisons.

A wider 39-case independent specials refresh passed all 30 bomb/missile/Screw
Attack cases and exposed the first-orb Charge Shot bug: creation ID zero was
incorrectly treated as absent by `shot_handle <= 0`. Native code checks an Item
pointer, whereas the script stores a creation ID with -1 as the missing sentinel.
Changing the script to `< 0` restores first-orb creation. All nine Charge Shot
recordings then match stock exactly in
`A:/FreakMelee-test-archives/samus-complete-specials-oct11-corrected-b/receipt.json`.
The original specials refresh is retained as FAIL, including its incorrect
41-case count expectation; its 30 passing cases are individually recorded.

The full exported source follows the existing independent-template assembly
pattern. `native_samus_template_source` checks it against its separately
maintained components. Normal source audit reports 282 checked functions, 8,776
instructions, zero diagnostics, complete metadata/body coverage and package
acceptance. The refreshed selected-source census passes 26/26 supported forms
in `A:/FreakMelee-test-archives/clone-roster-current26-oct11-a/receipt.json`.
Preparation is not roster-wide gameplay acceptance.

## Rebuilt Workshop package CPU and repeated-projectile comparisons

The rebuilt client passes all ten cases in
`A:/FreakMelee-test-archives/samus-workshop-cpu-oct11-c/results.json`.
Each case consumes 1,800 controller samples and compares both players against
stock. The cohort covers Final Destination CPU levels 1/4/5/9, a shielding
opponent, Battlefield, recovery from both sides, an elevated fighter with a
target below and Screw Attack against a CPU opponent. All clone logs report
`faulted=0`. The runner creates and verifies a fresh independent package from
the shipped template through the ordinary draft/package path; it does not
inject stock Samus handlers. Package ID:
`a0dd41f7a507ff4f724a84a310b1afece9a94ed8430ec5b58e4e7c167e1952d8`.

Both repeated-projectile cases also pass in
`A:/FreakMelee-test-archives/samus-workshop-missile-repeat-oct11-c/results.json`:
homing and Super Missile, 900 controller samples each, with real opponent damage
and the expected missile subtype/count checks. Package ID:
`108133d9b8f7024d653404f07b807e7517de4ac9025b72a91ba0fc7934548b17`.
These are separate current-client results, not one sealed 41-case receipt.
Unlike the pinned grapple and corrected-charge receipts, these two existing
runners do not hash inputs before and after execution. Their raw logs, replays
and package artifacts remain beside the results. Predecessor a/b runs failed
obsolete packaging CLI/output parsing before gameplay; the current runners use
`package-project`, `verify-package` and the actual returned package identifier.

The client/content-tool rebuild and eight focused CTests pass, including the
shipped-template composition check. The copied `build/native/ui/samus_full.fms`
matches the source template byte-for-byte.

Terrain-blocked exits, an exhaustive interruption/retirement/reuse matrix,
a ledge-available branch at the exact pull-completion call and coordinated visual
acceptance remain open. Longer CPU fights, cross-fighter interactions and
whole-match rollback acceptance also remain separate. The full goal stays active.
Nothing was pushed upstream.
