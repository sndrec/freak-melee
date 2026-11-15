# Samus connected factory and attachment checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

2026-10-11. The source candidate remains unregistered. This extends the
[retail constructor checkpoint](native_source_samus_resource_constructor_checkpoint.md)
through the actual Item factory and interpreted fighter create/remove helpers.

## Confirmed source correction

The post-attachment scaling signal used `holder.model.instance_scale_y`.
`holder` requires the fighter's actual held-item relation. Samus's tether is
bone-attached without occupying that slot, so the connected script faulted with
"Required receiver relation is absent" after successful construction.

The signal now reads `attachment_owner.model.instance_scale_y`, the retained
control relation already used by the chain and its normal state transitions.
This matches the original post-attachment owner-scale helper without requiring
a held item. No native handler delegation or fighter-specific API was added.

## Evidence

`A:/FreakMelee-test-archives/samus-factory-oct11-f/receipt.json` passes with
841,193 assertions across the earlier constructor/palette corpus and these
connected factory cases:

- 72 factory sequences: motions 212/214/357/358, scales 0.5/1/1.5, both facings,
  and normal/incomplete/completed modified-mode input sequences 0/1/4.
- Complete `Tether_Create` through the interpreter: unchanged shared Item
  factory, source actor attachment, real primary model and chain, pickup and
  native state selection, final owner scaling and callback installation.
- Creator, damage owner and attachment owner are the correct Fighter; the
  held-item slot remains empty. Code-sequence reset/preservation, modified mode,
  supplied launch origin, initialized collision group and final scale are checked.
- A separate actual primary graph runs unchanged
  `itLinkHookshot_ApplyOwnerScale`; final scalar/root scale agrees with the script.
- Removal clears the handle, reserved part 139 and all three bound tether
  callbacks, releases the actual Item and chain GObjs, and restores pool usage.
  Thirty-six removals run through the real bound Death1 callback.
- Four unsupported motions leave no Item. A controlled zero-available-GObj
  boundary executes real factory failure, leaves no published child or callbacks,
  and preserves the incomplete code sequence.

The common Item bank is normally imported. Its input matches `ItCo.usd` from
the current USA v1.02 ISO byte for byte (SHA-256
`5d62efd6437149335b9eef87624ae1253ba7bb260fb08cbae04a3d9215eacd11`).
The fixture publishes the owned motion table in addition to the already staged
parts/data/program tables, matching normal runtime publication. Source text,
authoring components, archive data, code and linked libraries are privately
copied and checked for unchanged hashes.

The client/source-asset targets build. The existing Arrow/Boomerang resource
regression also passes with the current fixture and the unchanged production
snapshot: `A:/FreakMelee-test-archives/script-item-resources-factory-regression-oct11/receipt.json`
(7,033 assertions, 25 cases and 39 animation comparisons).

## Remaining acceptance

The parent Fighter record and rig are staged explicitly. This is not full
Fighter construction, gameplay scheduling, collision/solver/catch integration,
rendered behavior, or whole-match rollback acceptance. Geometry/effect leaf
substitutions from the existing resource corpus remain installed but are not
needed by these creation/removal cases. Pool failure is a controlled capacity
injection, rather than actual maximum-volume gameplay.

The owned rig semantics are already complete (41 bindings,60 joints, no
omissions), verified again against the current ISO on October11; see the
[authoring checkpoint](native_source_samus_authoring_checkpoint.md).
All fighter callbacks, integrated
grapple behavior and lifecycle acceptance still precede independent
registration. No game was launched, installed packages were untouched and
nothing was pushed upstream.
