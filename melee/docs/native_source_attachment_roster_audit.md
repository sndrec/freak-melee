# Remaining roster attachment migration

This bounded follow-up migrates revocable Article control in eight FMS files and twelve Article blocks. It leaves Zelda/Sheik and all engine/catalog/test code untouched. The previous two-form migration is documented in `native_source_attachment_template_audit.md`.

| Templates / Article | Change |
| --- | --- |
| Ness PKFlush / Bat | Input defaults to zero when attachment is absent; held motion/state and optional bat cleanup use attachment. Presentation scaling retains original creator, gated by live attachment to preserve the former absent-control no-op. |
| Peach Turnip | Optional `forget_vegetable` cleanup uses guarded attachment helper, so revoked control cannot clear a newer held vegetable. |
| Link / Young Link Boomerang | Homing/catch/control fields use attachment; missing smash is false, release offsets zero, absent state writes/helpers are no-ops. Lifetime and launch/collision arithmetic stay unchanged. |
| Link / Young Link HShot | Solver/control reads and helpers use attachment. Existing presence-gated solver entry remains; optional cleanup helpers are guarded. Late instance-scale reads use default1; late launch-facing state uses default0. |
| Young Link Milk | Held motion/state/command/scale use attachment. Cleanup calls `milk_unset` only with live control and matching current owner. The absent-controller branch drops the old guaranteed-no-op helper and keeps detach/expiry. |
| Popo / Nana GumStrings | Controller endpoint X/Y/Z use explicit zero defaults after revocation; existing conditional Z read and solver order remain. |
| DrMario Vitamin | Active position following uses attachment; original-creator presentation scale remains, with attachment/existence gating preserving the old absent-incapacitated=false case. |
| GameWatch Rescue | Held-motion/optional matching-owner cleanup use attachment; appearance palette helpers remain original creator. |

Fox/Falco Blaster's explicit original-creator presentation query remains unchanged. Its audited detach paths immediately remove the Item; no continuing presentation-control path was established that justified relabeling the appearance source. Clear-only Bomb/Arrow/Illusion/ShadowBall/etc blocks and NeedleHeld appearance retain their existing provenance uses. No global `creator` replacement was performed.

## Exact defaults and helper ABI

Historical source evidence is `tools/native_fighter_item_script.c`: nullable creator state/input/position→0, motion→-1, model/instance/root scale→1, incapacitated/hitlag→false, category→0; missing state writes do nothing. `native_fighter_item_functions.c` guards a missing helper recipient and returns typed neutral/no-op. The migrated code uses those exact defaults or existing presence conditions; it does not introduce a generic defaulting receiver.

Every converted helper is a same-source root declaration with the existing signature. This includes Boomerang cleanup/catch, Tether control methods, Ness/Peach cleanup, Milk and Rescue cleanup. `Tether_Unbind(int expected)` now receives `int(item.creation_id)` explicitly: direct Item ID is U32, and the integral conversion normalizes width/sign, preserving the former signed bit pattern. It is **not** an overflow-range check; only floating conversions have that range check. Ordinary factory-published IDs already fit INT32_MAX. No helper signature or compatibility alias was added.

Original creator and current damage owner remain separate from attachment. In particular, current-owner-match conditions are retained around former `owner.call` helpers. Missing attachment suppresses those helpers even when original provenance is still live.

## Scope and validation

Final source diff: **254 inserted /157 removed lines**, **47 typed helper sites**. Pre-edit bytes and hashes are preserved at `A:/FreakMelee-test-archives/source-attachment-roster-preimage/`. Source review verified exact helper declarations, unchanged root function signatures, state/signal headers and while-loop order; whitespace checks pass. Solver arithmetic, IDs, constants, factory arguments and signal timing were not rewritten. The added zeros/ones are explicit preexisting nullable defaults.

Full compilation/runtime acceptance is still separate. Legacy receiver aliases such as `root_scale_y`, `instance_scale_y`, `model_scale`, `incapacitated` and `action.category`, remaining `owner.call`/target operations, pose/resource/paired-family gaps and other primitive closures are not made supported by changing their relation labels. This batch does not invent API replacements, stock-authored handlers, metadata-only acceptance or native fallback. Parent owns typed excerpts, integrated tests and gameplay validation.
