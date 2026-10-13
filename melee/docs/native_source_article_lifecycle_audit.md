# Source article lifecycle: required ownership split

The subsequent implementation and evidence are recorded in
[the lifecycle checkpoint](native_source_article_lifecycle_checkpoint.md).
This document retains the original audit and its requirements.

Audit only. No new attach/detach bindings or persistent relation fields are
implemented by this document. Existing `item.owner_bound` already reads/writes
the actual `xDC8_word.flags.x13`; it must not be duplicated as a script variable.

## Verified legacy decisions

- `clear_current_owner` sets only `Item.owner = NULL`.
- `clear_attachment_owner` clears legacy union attachment pointer/creator ID.
  `nativeArticle_Creator` uses that pair, so this formerly also revoked scripted
  creator/owner access. Source receivers use independent `nativeItem_Creator`,
  and cannot silently assume that clearing the old union revokes that relation.
- `detach_owner` performs current-owner clear, then attachment-link clear. It
  does not call the native physical release routine, change `owner_bound`, or
  clear the Fighter's held/auxiliary slots.
- The emitted `attach_to_owner` checks a live script creator and equality with
  current damage owner, bounds the raw runtime part against the actual rig,
  then calls `Item_8026AB54`. That function performs native pickup setup and
  dispatches the source PickedUp hook. Optional auxiliary registration occurs
  afterward only if the participants still exist. Semantic bone count is not
  the raw part/joint bound; use prepared parts and the live joint instance.
- `nativeItem_BindFighter` uses legacy `nativeArticle_Creator` for authored Items.
  It cannot establish source authoring ownership through a Definition-free bank
  unchanged. `BindExisting` has a looser contract and is not a replacement.

## Implementation direction

Keep immutable original creator, mutable current damage owner and revocable
script attachment/control relationship separate. Original creator provenance
must survive detachment and reflection. Mutable source attachment must live in
real owned metadata outside the legacy Item-vars union, with full physical pool/
scene/native generation identity and explicit snapshot coverage. Initialize the
relations in the real constructor/factory before source callbacks observe them.
Do not add a fallback Definition, dummy state field, or unused old union mirror.

Expose a generic retained attachment relation where scripts need revocable
control. Rewrite such accesses in existing templates explicitly to preserve the
old detach decisions while immutable `creator` remains available for provenance.
Audit each call site: blanket replacement of every creator query would lose
legitimate original-creator data. Attachment clearing must not change damage
owner, physical attachment flags, or original creator as an unintended side effect.

The new lifecycle binding should admit the exact source Item/Fighter bank,
spec, storage and physical generation. Missing attachment/current ownership is
a healthy no-op for attach; malformed widths fault, and ordinary invalid raw
parts retain the emitted bounded no-op. Call the actual pickup routine once and
guard reentry before optional registration. For `articles.bind_auxiliary`,
capture one same-bank Item originally created by this Fighter; reflection does
not revoke immutable creation identity. Missing/foreign Items return false.

Required evidence: real source actors/VM and pickup bodies, actual relation and
snapshot round trips, cleared/restored links, foreign reflected damage owner,
missing/stale references and physical reuse, raw part bounds, PickedUp removal/
retirement, plus template behavior comparisons. Source `item.create_child` will
also need this relation model rather than reconstructing author/control state
through legacy union data. This remains open work in the full project goal.
