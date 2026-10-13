# Direct owned Article signals

```powershell
python tests/native/script_article_signals/run.py --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/script-article-signals-review
```

Fresh A: archive; hidden compiler/console. Actual compiler/catalog, Fighter/Item
program preparation, actor ownership/view/storage, VM/payload and GObj pool are
used. Asset publication metadata and unrelated engine leaves are controlled;
position writes inject lifecycle changes while all signal decisions stay source.
No game/global build/template/user-project modification.

Signal source names/Article declarations resolve to callback IDs at load. Runtime
checks exact caller bank, recipient source owner and prepared signal code/ABI,
then invokes actual RunSignal. No text interpretation or hash occurs in ticks.
Retained Text spans are still checked against caller source bytes. Builtin Article
identifier arguments may carry physical kind constants: the encoded function's
owner supplies the declaration identity instead of assuming that token is a node.

Generic legacy signal_article used immutable author/logic admission, without
creator/current damage/attachment ownership equality. Direct target dispatch keeps
same-bank/exact-declaration admission across reflection. signal_owned_article was
a separate stricter API, not the contract of this method. Missing/stale/nonmatching
target returns false. A valid callback deleting its recipient remains delivered
while caller is live. Recipient Error is a real failure; caller retirement/fault
preserves output and stops subsequent VM statements. Self signals during Destroyed
are permitted while its storage remains live.

Target nonfinite payloads return false as old generic signal_article did. Old
item.signal directly called the local authored callback, so self F32 values retain
NaN/Inf bits and have no new finite filter. The tests distinguish these contracts.
Scalar creation IDs retain complete32bits/-1 sentinel, and wider or float inputs
reject without truncation. Lookups and signal-table searches are fixed bounded.
Actual native asset/spawn/render and full gameplay acceptance remain separate.

`articles.signal_owned(handle, DeclaredArticle, "name", value)` uses the same
prepared callback identity, with both current damage ownership and a live
attachment relation to the caller required. Its compiled source helper runs the
actual VM/actor signal path. Foreign ownership, detached control, missing handles
and a different declaration with the same physical kind reject delivery. Owned
dispatch also covers recipient deletion/reuse, caller deletion, recipient fault
and same-generation caller userdata replacement; no stale continuation publishes
success. Both APIs resolve names only during loading.

The same fixture now covers generic link IDs, reciprocal links, unlinking and
Fighter-owned one-way linking. Article callbacks forward prepared signals to
other Article callbacks through `articles.signal(item.link_id(0), Probe, ...)`.
It executes these expressions through the real source compiler, catalog and VM.
For all 16 slot/backlink combinations, link reads and unlink results are compared
with the existing native relationship functions extracted from `native_article.c`.
Tests also cover reflection, owner/control mismatches, occupied slots, repeated
links, full-width IDs, absent links, freed/reused GObj slots and rejected input.
This verifies the relationship primitives, not Thunder Jolt/Thunder gameplay,
linked child creation, terrain reads or asset constructor behavior.
