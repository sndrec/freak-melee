# Attachment control versus original creator

The source Item owner now retains original creator separately from revocable attachment control and current damage owner. Clearing attachment must stop controller reads/cleanup without erasing provenance. This bounded template migration changes only Zelda DinFire and Sheik Chain; no other roster migration or complete runtime acceptance is claimed.

## Source evidence and exact defaults

The historical `nativeArticle_Creator` resolves `native_article.creator_spawn_id`, requires the same `attachment_owner`, and returns NULL after that attachment identity clears. `tools/native_fighter_item_script.c` translates this nullable query with explicit fallbacks: state/input/position values 0, motion -1, model/instance/root scale 1, incapacitated/hitlag false, and action category 0. State writes to a missing creator do nothing. `native_fighter_item_functions.c` executes creator helpers only for a live recipient and produces their typed neutral value when absent; Void calls are no-ops and Float helpers return zero.

These are historical behavior references, not runtime compatibility routes. New templates use `attachment_owner.exists`, typed fields and exact source helper code IDs. Original `creator` remains appropriate for provenance/appearance; current damage owner remains independent, including reflection.

## Changes frozen in this batch

| Article | Migrated behavior | Absence/retirement behavior |
| --- | --- | --- |
| Zelda DinFire | Steering input and release/steer decision helpers; destruction/burst cleanup | Existing presence + current-owner-match conditions retained. Typed Bool helpers replace the two same-owner string target queries. Missing control skips steering/release/forget. After child creation, presence is checked again before forgetting because creation can reenter and revoke control. |
| Sheik Chain | Input history, rehit state, collision-skip input, endpoint/clear-hitbox helpers, launch position/center | Explicit state/input/position defaults 0; center helper default 0. Optional Void helper calls are guarded no-ops. Existing resource/collision update flow continues with its previous defaults rather than gaining an early return or absent-receiver fault. |

Typed root helper signatures were checked against the same source's root declarations:

- Zelda: `din_can_steer() -> bool`, `din_release_requested() -> bool`, `forget_din() -> Void`.
- Sheik: `chain_endpoint(int, float, float, float) -> Void`, `chain_clear_hitboxes() -> Void`, `chain_center_y() -> float`.

The final diff is two FMS files, **44 inserted /23 removed lines**. Zelda has four typed helper sites; Sheik has six. No arithmetic, state IDs, signal timing, resources/model metadata, factory parameters or reflection ownership policy was changed. Palette/effect creator scopes and NeedleHeld appearance reads remain unchanged. Other initially explored roster transformations were undone before freeze; those files have no diff from this batch.

The existing Sheik Cleanup target call to current damage owner is unchanged. It is a different recipient contract from the old `creator` query; replacing it with an original-creator or attachment helper without its owner policy proof would change behavior. No new string target substitute or compatibility alias was introduced.

## Remaining roster audit

A source scan found attachment-clear/detach operations in 31 Article blocks across 16 FMS files. Remaining cases needing their own connected closure include:

| Family/block | Follow-up |
| --- | --- |
| Ness PKFlush/Bat | Revocable input/held-motion/cleanup reads; preserve zero state/input defaults and guarded optional bat cleanup. Current-owner helper behavior also needs its separate match policy. |
| Peach Turnip | Destroyed `forget_vegetable` must skip after control revocation, avoiding cleanup of a newer vegetable. |
| Link/Young Link Boomerang | Homing/catch/held-control fields, missing-recipient helper no-ops, zero state defaults and ignored writes after detachment. |
| Link/Young Link HShot | Control helpers and solver fields; existing presence gates protect much of the solver, but optional Dispose/Destroyed helper calls and unguarded scale reads require explicit no-op/scale1 treatment. Legacy scale aliases are still unsupported direct receiver spellings; this audit supplies no replacement primitive or metadata acceptance. |
| Young Link Milk | Revocable held-motion/command/model-scale checks; preserve missing-controller expiry and current-owner cleanup ordering. |
| Popo/Nana GumStrings | Controller endpoint fields default to zero after rope cleanup; member/paired recipient and complete resource lifetimes remain separate blockers. |
| DrMario Vitamin/Fox Blaster presentation | Distinguish origin-based appearance scaling from active follow/control. Do not convert provenance merely because the block also has a terminal detach/remove path. |
| GameWatch Rescue | Revocable held-motion decision needs migration; PaperBodyColor/PaperOutlineColor are appearance provenance and must remain original-creator queries. |
| Other clear-only blocks | Bomb/Arrow/Blaster/Illusion/ShadowBall/etc may have no subsequent creator fields. Audit comparison primitives and native interaction side effects separately before changing them. |

The historical `item.creator_is(other)` emitter actually compared revocable
attachment control, not original provenance. Current templates use the truthful
`item.attachment_owner_is(other)` predicate. The former
`item.current_owner_is_creator` spelling likewise compared attachment control;
current templates use `item.attachment_active` for that property. Neither change
justifies replacing genuine creator-provenance queries globally.

## Validation boundary

This batch verified scoped diffs, whitespace, root helper names/signatures and exact historical fallback values from source. Parent owns typed excerpt compilation, actor/receiver/snapshot tests and integrated build. Full Zelda/Sheik preflights may still fail on unrelated primitive, CPU, pose or Chain resource gaps; these edits do not establish whole-template or game parity. Runtime must validate the captured attachment relation, source bank/view and generations before helper execution and output publication, and stop on retirement/fault without restoring control or silently falling back to stock authored behavior.
