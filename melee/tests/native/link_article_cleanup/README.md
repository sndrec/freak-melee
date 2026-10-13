# Pending immutable-author article cleanup composition

This namespace preserves a source-only preparation checkpoint, not accepted
cleanup behavior. No shared/default/accepted fixtures were changed.

## Reproduction

```
python -B tests/native/link_article_cleanup/prepare.py --snapshot .local/baseline40 --output .local/link-cleanup-FRESH
```

The runner assembles bounded component inputs into a normal test-only source
candidate, then clones, imports owned parameters, generates C and compiles a DLL.
It verifies immutable SDK/compiler/tool SHA inputs. There is no alternate importer
or manually supplied parameter fallback. No games or global builds run.

Component files each stay below2000lines. They retain the actual accepted Bow,
Arrow, Boomerang, Hookshot and Milk callback bodies. Top-level declarations,
functions and articles are composed directly; components are source inputs, not
runtime forwarding wrappers. Original input hashes are in source_inputs.json.

New authored remove signals:

- Arrow destroys both trails before item.remove, preserving it802A8A7C order;
  the existing destructor then performs its ordinary owner/reference cleanup.
- Bow calls item.remove; existing recipient-owned exported cleanup remains in
  its actual destructor. Missing optional recipient export still removes item.
- Boomerang and Milk retain their existing authored removal signals.
- Hookshot retains Dispose, exact creator callback unbinding and resource release.

Cleanup_AllArticles uses typed immutable-author refs/signals in stock order:
Boomerang, Hookshot, Arrow, Bow, Milk. Boomerang explicit removal also now checks
its immutable typed ref rather than current damage owner. Bow/Boomerang callbacks
are updated to direct Cleanup_AllArticles binding/calls; no forwarding function
or native stock handler is added.

## Actual preparation blocker

First normal Link import fails at assembled source line1291: the first Boomerang
attribute, lifetime offset0. Failed commands/log/source are preserved under
.local/link-article-cleanup39-prep. preparation_blocker.json records hashes.

Frozen39 nativeStockArticleSlots has Bomb/HShot/Arrow/Bow entries, but no
Boomerang or Milk entries. The importer intentionally rejects unknown article
DAT attributes; this is neither output-file collision nor capacity failure.
Required import metadata, proved by actual OnLoad bodies (parent has since added
these records for the pending40 build; immutable39 is unchanged):

| source kind | article | source slot | kind attribute offset |
| --- | --- | --- | --- |
| Link | It_Kind_Link_Boomerang | 1 | 44 |
| Young Link | It_Kind_CLink_Boomerang | 1 | 44 |
| Young Link | It_Kind_CLink_Milk | 5 | -1 (fixed native kind) |

See ftLink/ftlink.c OnLoad item_list[1],da->x2C and ftCLink/ftclink.c
OnLoad items[1],ea->x2C / items[5],It_Kind_CLink_Milk. Link has no reachable
Milk constructor. Do not invent a Link Milk asset mapping. The assembly now explicitly omits the unreachable Link Milk article/ref branch
because normal import filters another source kind's article. This correction was
made after preserving the failed39 input; no success is claimed until fresh40
import/generation runs. Link's reachable
empty Milk slot and Young Link's full Milk presence matrix must be distinct.

## Preserved preparation failures

Frozen39 lacked Boomerang/Milk article import metadata; the failed source and
commands remain in .local/link-article-cleanup39-prep and preparation_blocker.json.
After parent supplied those records, its first40 run reached generation and
rejected the new Arrow remove signal's forward call. Declaring destroy_trails
first fixed this existing-language ordering issue without changing either body.
The next fresh40 failure at the Arrow lifetime call is recorded in
preparation40_checkpoint.json. Its cause was incomplete assembly input: accepted
Bow source had already moved its DAT declarations into an adjacent owned FMP.

The five fighter Bow declarations (+0..+0x10) and nine Arrow float declarations
(+0..+0x20) are now restored directly in bow.component.fms. They match
ftLink/types.h ftLk_DatAttrs and it/itCharItems.h itLinkArrowAttributes plus their
actual ftlinkspecialn.c/itlinkarrow.c consumers. Model roots remain at0x24/0x28.
No manual FMP values, fallback importer or invented stock defaults were supplied.
Fresh40 preparation succeeds for both families; all article bodies are retained.

## Composed source comparison

```
python -B tests/native/link_article_cleanup/source_compare.py --snapshot .local/baseline40 --output .local/link-cleanup-FRESH
```

The source driver creates fresh normal Link, Young Link and independent recipient
projects, imports values, generates complete C and compiles ordinary DLLs. The
console fixture includes generated author C to invoke the actual private authored
functions directly. The independent recipient runs from its real separately
loaded DLL, with a different state block offset, its own owned parameter table
and an opt-in bow_unset export. All unused required Host operations trap.

The fixture extracts unchanged stock fighter full-removal/unset bodies and
Boomerang/Arrow/Bow/Hookshot/Milk removal/destruction bodies. It also executes the
actual Item_8026A8EC body and its four inline helpers: removal-in-progress guard,
actual destroyed callback, generation guard, native-model cleanup and HSD-free
order are unchanged. Production typed article refs, immutable signal dispatch,
creator lookup, recipient event dispatch, generation/depth checks, resources,
article model slots and shared joint code are linked or extracted directly.

The coherent matrix spans every reachable presence mask, current-owner transfer,
reflection, two trail slots, launched Arrow, Boomerang mode and used flag. It
invokes direct cleanup, bound damage plus death1, and death2/death3/death1 in
engine-defined order, then repeats full cleanup to detect duplicate frees.
High-bit retained IDs never pass through float. Parameter retuning of the foreign
recipient is checked separately without changing author parameter bytes.

Generated-only cases cover sentinel, absent, recycled generation, foreign
Definition, wrong logic and wrong kind for every reachable article; missing Bow
recipient export still destroys the item without mutating the recipient.
Hookshot resource release is checked with replaced part139/capture/hitbox borrows,
replaced callback bindings, a switched creator package and an unlinked creator.
The original creator's ordinary fields and foreign replacement bindings stay
untouched when ownership no longer matches. Every graph free poisons its storage.

## Controlled boundaries and limits

- HSD allocation/model-descriptor construction uses a fixed fixture pool. Stock
  and authored Hookshot graphs are explicitly populated with the verified22/15
  nodes; the full constructor and nine-state solvers are not rerun here.
- Item global spawn counts, hitbox registry, effect/audio cleanup and final HSD
  entity storage are controlled engine boundaries. Dynamic bones, camera/ECB
  borrowers and the held-item owner bit are disabled and trap if queried. This
  does not claim those unrelated Item engine subsystems have been compared.
- Resource/joint production bodies are linked; teardown exercises graph freeing
  and borrower invalidation, not geometry/channel solvers or actual rendering.
- Live lists and immutable Definition lookup are configured by the fixture;
  membership/generation validation and callback dispatch are actual production.
- Original Hookshot teardown leaves reserved/capture/hitbox joint borrows stale.
  Authored resources detach exact matching borrowers before any model free;
  replacement-borrower preservation is a deliberate safety extension. Original
  blind callback clearing is likewise replaced with exact authored identity checks.
- Retired/reused raw stock pointers and dead owner userdata are not executed as
  stock parity cases. Generated safety rejection is reported separately.
  An absent Boomerang retains its stale ordinary ID until a later reset; typed
  existence checks keep it inert, preserving the defined absent-removal policy.
- Link has no reachable Milk asset/constructor; its mask has four articles.
  Young Link has the full five-article matrix. No Link Milk mapping is invented.

Old partial receipts remain historical. The final acceptance receipt records
commands, stock/generated/component hashes, frozen inputs and exact case counts.
No default/template/shared code changed, and no game or full build ran. This
unregistered source evidence does not claim full Link fighter independence.
