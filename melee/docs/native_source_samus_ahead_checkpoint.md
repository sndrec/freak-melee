# Samus shared ahead-query checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

The 2026-10-10 batch implements Article-scoped
[`targets.read_opponent_ahead(x, y, z)` and
`targets.read_lockable_item_ahead(x, y, z)`](scripting/targets.md).
Both return a Bool and publish three distinct Float destinations only when
a target exists. No-target calls leave the coordinates unchanged. Fighter
selection follows native owner/shared-player/hidden/team exclusions,
exact-facing half-planes, strict squared-XY distance and registry-order ties.
It samples camera-target bones, including the original selected-target
resample. Item selection uses the original lock-on category/held/owner-bound
policy and returns the ECB-center position. The source owns priority and
steering. Neither operation references Samus or any special-move handler.

Borrowed Fighter identity and camera-rig admission precede native sampling;
retirement prevents successful publication. Registry traversal is bounded by
the physical pool. The new code uses stack storage; existing HSD sampling
retains its normal lazy scale/expression behavior. This is not arbitrary
destructive-callback containment or a claim that all HSD work allocates nothing.

Samus's missile script uses these Bool-return queries and the existing generic
`vectors.rotate(..., Axis.z, ..., out_x, out_y, out_z)` operation rather than
adding a Z-specific alias. The complete 172-body audit has one failing body,
the bomb accessory overlap call. Parameterized recipient calls are also still
needed in that body. Samus remains rejected, with no independent package or
real missile-gameplay acceptance. Grapple and remaining lifecycle work are open.

Evidence is in
`A:/FreakMelee-test-archives/samus-ahead-checkpoint-oct10-a/receipt.json`.
The connected target cohort runs actual compiler/catalog/VM/Article actors,
fixed physical pools, native camera/matrix functions and original search
routines as comparison references. Two 128-case cohorts compare direct and
interpreted results across exclusions, ties, no-target calls, item categories
and output offsets. Compiler/context/owner/rig negatives reject without
publishing a result. Team mode, assets and graph construction are controlled;
unrelated bindings remain unavailable. The shield extraction was updated to
its existing `ftCo_ShieldScale` owner after removal of its old inline helper;
the fixture does not exercise authored shield-scale hooks. Seven native
targets build and both focused scripting CTests pass. No game launch or user
collection change occurred.
