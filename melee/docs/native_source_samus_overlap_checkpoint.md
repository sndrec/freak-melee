# Generic hitbox-overlap checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Article scripts can call
[`targets.overlaps_hitbox(fighter_id, hitbox_slot)`](scripting/targets.md#targetsoverlaps_hitbox).
It uses the existing native hit/hurt capsule kernel, preserving swept endpoints,
endpoint-cache writes, Item radius scaling, Fighter depth and early-exit order.
It does not filter combat status, apply damage or call a fighter-specific handler.
Physical identity, bounded capsule storage and retained bone/matrix dependencies
are admitted before sampling and checked before publishing the Bool result.
The operation adds no heap allocation; native HSD retains its existing lazy work.

Samus's bomb accessory now obtains its retained control owner through the
existing `attachment_owner.fighter.creation_id` receiver and calls its own
exported `bomb_jump` helper with center/radius after overlap. This is distinct
from current damage ownership. Reflection/interaction already clear that
control relation in the script. Neither the overlap API nor recipient dispatch
has a Samus-specific semantic.

Evidence is in
`A:/FreakMelee-test-archives/samus-overlap-checkpoint-oct10-a/receipt.json`.
The grouped geometry cohort passes 18,181 checks across 1,184 cases, including
1,373 native comparisons and 882 top-level VM calls. Its new 512 comparisons
cover 128 stock and 128 independently owned rig configurations, each directly
and through Article VM signals: all four hitbox slots, 0..15 hurt capsules,
cached/uncached endpoints, radius/depth scaling and exact collision-cache/output
bytes. The unchanged stock Samus loop is the reference; production overlap
calls the shared capsule kernel directly. Native matrices and actor/pool/storage
execute, while package assets, graph construction and unrelated bindings have
explicit controlled boundaries. Actual attachment-owner relations are checked
with a different damage owner; the qualified creation-ID expression is compiled
but its property-read boundary is not executed in this fixture.

The recipient regression cohort still passes 2,995 checks / 374 cases. Seven
native targets build and both focused scripting CTests pass. Samus's complete
172-body audit now has no body diagnostics; its only diagnostic is the deliberate
package-level refusal of the WIP source's missing independent declaration.
This is not independent Samus acceptance: grapple, lifecycle/reset ownership
and real-game comparisons remain open. No game launch or collection change.
