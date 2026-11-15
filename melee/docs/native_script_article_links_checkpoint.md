# Interpreter article links — 2026-10-10

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Pikachu/Pichu preparation now visits all **209 bodies per fighter** with
**9 failing bodies each**, down from 21. This batch implements reusable
relationships and Article-to-Article signals; neither package is accepted yet.
Full-roster preparation remains **22/26 forms**, excluding Kirby and counting
Sheik/Nana separately. Preparation does not establish gameplay parity.

## Implemented contract

- `articles.link_owned(source_handle, slot, target_handle)` installs a one-way
  link between two published source items controlled and damage-owned by the
  calling fighter. It returns a Bool and rejects occupied live slots, self-links
  and missing/foreign ownership without changing links.
- `item.link_id(slot)` preserves full-width creation IDs, returning −1 for an
  absent link. `item.link_reciprocal(slot, backlink)` checks both directions.
- `item.unlink(slot)` clears the source slot and every reciprocal slot on its
  live target without deleting either item or invoking callbacks. It remains
  valid during Destroyed after native removal is marked, while storage is live.
- All reads/mutations admit exact published source actors in the same scene,
  package author and program bank. Targets must be healthy and not removing.
  Reflection does not invalidate an existing link. Reusing an object address
  cannot revive a stale creation ID. Slot/handle width and integral types are
  checked; failure preserves caller-provided output.
- `articles.signal()` is now available in Article callbacks, through the same
  prepared callback identity and recipient/caller lifetime checks already used
  by Fighter callbacks. `articles.signal_owned()` remains Fighter-only.
  Pikachu/Pichu callsites directly compose `item.link_id()` with a declared
  Article signal. No legacy signal-link alias, stock-special delegation,
  new relationship allocation or fighter-specific API was added.

The implementation is in `native_script_binding_item_links.c`, with direct
catalog dispatch; the existing signal resolver/runtime handle both caller types.
[User documentation](scripting/article_links.md) describes only current methods
and explicitly identifies the remaining migration gap.

## Verification

The real source compiler, catalog, VM, published Fighter/Item actors, storage,
GObj allocation/free and fixed pool pass **441 checks**. Tests cover all 16
slot/backlink combinations against the existing native relationship bodies
extracted from `native_article.c`, repeated/occupied links, self/foreign-owner
rejection, reflection, wide IDs, freed/reused addresses and destruction cleanup.
Article callers additionally exercise recipient deletion/reuse, caller deletion
and recipient errors through real nested interpreted signal callbacks.

The fixture's asset metadata and position-write mutation injection are controlled.
No Item DAT constructor, full Thunder Jolt/Thunder behavior, rendering or compact
rollback acceptance is claimed. Its old destruction injection passed a null
result to a result-requiring hook; that fixture call now supplies a real result
and checks execution before freeing the object.

Seven Windows targets build successfully; script and parameter CTests pass 2/2.
Current shipping Pikachu/Pichu templates exactly match repository bytes. Two
final receipts independently rehash **1,702 and 189 inputs**. No game window,
audio, normal installed content or user draft was changed. Earlier fixture and
roster outputs remain archived, including a fixture assertion caused by its
initial null output argument; only the receipts below certify the final batch.

| Evidence | Receipt SHA-256 |
| --- | --- |
| `A:/FreakMelee-test-archives/script-article-links-oct10-e/receipt.json` | `d585d60dc819c8aa0cda6c88bccf0b3d809365b25be35314cf2d0a865591345c` |
| `A:/FreakMelee-test-archives/source-full-roster-audit-oct10-item-links-final/receipt.json` | `3aabe2046e3beeab9e8bccb73b4e5226f112bd603d08167ba870d0788c5938b0` |
| Independent rehash/shipping/build log | `A:/FreakMelee-test-archives/article-links-independent-oct10/receipt.json` |

## Remaining work

Pikachu/Pichu still need linked creation, linked position/normal/collision reads,
indexed article bone observations, terrain sweep/contact results and typed
Fighter action-category reads. The audit reports the first error in each visited
body, so later operations in these bodies still require preparation and gameplay
checks after the current sites are resolved. Ness has an animation-resource
metadata prerequisite; Samus lacks a valid independent draft in this census.
The broader fighter and compact rollback acceptance gates remain open.

The subsequent [linked creation and terrain observation checkpoint](native_script_linked_creation_checkpoint.md)
implements linked child creation, normal-producing sweeps and ground checks.
It reduces the current Pikachu/Pichu failing-body count from nine to five each;
the results above describe this earlier link/signal batch.
